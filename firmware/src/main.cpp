// main.cpp — shrink-fit cycle state machine. NodeMCU-32S build.
//
// Control law: terminate on accumulated ENERGY, never on elapsed time (CR-01) —
// once the shunt is calibrated. Until then TIMED mode reproduces the test rig's
// proven behaviour exactly. Time is otherwise only an absolute abort ceiling (CR-03).
//
// THREE THINGS THIS FIRMWARE IS NOT:
//   1. Not the E-stop. SR-03/§6.9: the E-stop drops the AC contactor coil in
//      hardware. estopClosed() is status only.
//   2. Not a temperature controller. §8.3: a thermocouple inside a kW-class
//      90 kHz field cannot be read during the cycle, and the on-board TC is
//      deleted (pinmap.md). Temperature is the calibration campaign's handheld
//      and temper colour — a next-cycle interlock, never a mid-cycle abort.
//   3. Not the output enable, and not the mains switch. CR-05 rev. §6.10: CAN sets
//      voltage, the SSR delivers the hard step. Mains is a hardwired MAINS START
//      that latches the contactor; firmware can only DROP it (MAINS_DROP_PIN),
//      never energise it.
//
// Merge lineage (2026-09-07): state machine from the original S3 skeleton; pins
// and step/contactor semantics from pinmap.md and §6.9/§6.10; TIMED mode and the
// duration ladder from testrig.cpp; R48 session behaviour from ina228_test.cpp
// as verified on the rectifier (§6.14.0a–e).
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include "config.h"
#include "r48_can.h"
#include "energy.h"

// ---- output: USB and telnet are one stream --------------------------------
// OTA uploads firmware; it does not carry the console. Telnet does, and it works
// where espota does not: an ordinary OUTBOUND connection from the laptop crosses
// the VLAN that espota's reverse connection cannot (TRAPS). Everything printed
// goes to both, so there is never a "which window is this in?" question.
static WiFiServer telnetSrv(TELNET_PORT);
static WiFiClient telnetCli;
static Preferences prefs;
static bool otaReady = false;

static void say(const char *s) {
  Serial.print(s);
  if (telnetCli && telnetCli.connected()) telnetCli.print(s);
}
static void sayfln(const char *s = "") { say(s); say("\n"); }
static void sayf(const char *fmt, ...) {
  char buf[256];
  va_list ap; va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  say(buf);
}

// ---- devices ----------------------------------------------------------------
R48            rect;
R48::Telem     rectTelem;
EnergyMeter    meter;
static bool    meterUp = false;

// ---- cycle state ------------------------------------------------------------
enum class St   { BOOT, ARMING, IDLE, PRECHECK, HEATING, SETTLE, MEASURE, COOLDOWN, FAULT };
enum class Mode { TIMED, DOSED };
St        state = St::BOOT;
Mode      mode  = Mode::TIMED;         // DOSED only once SHUNT_CALIBRATED
uint8_t   sel   = PRESET_COUNT - 1;    // DOSED preset: default 1/2" — campaign starts there
uint8_t   durIx = 1;                   // TIMED ladder index: 10 s
uint32_t  tState = 0, tHeatStart = 0, tMainsUp = 0;
float     doseTargetJ = 0, doseJ = 0;
bool      presetLocked[PRESET_COUNT] = {false};
const char* faultMsg = nullptr;
uint8_t   armSamples = 0;
bool      lastStartLevel = false;      // edge-detect: a held button must not re-fire

// Duration control and its indicator, ported verbatim in behaviour from
// testrig.cpp. The button steps the ladder; the LED plays the setting back as
// runSeconds/5 flashes whenever IDLE, continuously. That continuous display is
// the fix for the UX trap found on the rig: a press that only REPORTS the state
// is a press that also CHANGES it, so the display cannot live on the button.
static const uint32_t RO_PER = 400;    // per flash: 200 on, 200 off
static const uint32_t RO_GAP = 1600;   // dark tail, separates one count from the next
static uint32_t idleAnchor = 0;        // restart the burst on a change so it replays at once
static bool     lastDurLevel = false;

// The duration button lands on ENC_SW_PIN (GPIO 15) deliberately: it is already
// in the pin map, it HAS an internal pull-up (unlike 34-39), and when the encoder
// is fitted later its push switch is the same pin doing the same job. No pin map
// change, no external resistor, nothing to rewire.
#define DURATION_BUTTON_PIN ENC_SW_PIN

// BENCH_MODE: no rectifier, no SSR, no buttons. Set ONLY by env:controller_bench.
// It bypasses the CR-05 arming gate and ignores the physical inputs, because on
// the bench GPIO 34/35 float (input-only pins, no internal pull-ups) and would
// start cycles on noise. It must never be set in a build that drives a coil, so
// it is loud at boot and on every status line rather than a quiet constant.
#ifndef BENCH_MODE
#define BENCH_MODE 0
#endif

// ---- io helpers -------------------------------------------------------------
// The SSR is the step (CR-05). It is the ONLY thing that puts the bus on the ZVS.
static void ssr(bool on)         { digitalWrite(SSR_PIN, on ? HIGH : LOW); }
// Firmware's one path to mains: open the contactor. There is no close.
// With no contactor fitted this cannot remove power. Say so, rather than let a
// caller believe a safety action happened. fault() still opens the SSR.
static void mainsDrop() {
#if CONTACTOR_FITTED
  digitalWrite(MAINS_DROP_PIN, HIGH);
#else
  say("  (no contactor fitted — mains NOT dropped; SSR open is the only action)\n");
#endif
}
// BENCH_MODE returns true here rather than at each call site: guarding the
// callers individually is how doHeating() kept its E-stop check while
// doPrecheck() lost it, and the cycle died 5 ms in on a floating pin.
static bool estopClosed()        {
#if BENCH_MODE || !ESTOP_FITTED
  return true;               // no chain to sense; the boot/status warning says so
#else
  return digitalRead(ESTOP_SENSE_PIN) == HIGH;
#endif
}
static bool startPressed()       { return digitalRead(START_BUTTON_PIN) == LOW; }
static bool stopPressed()        { return digitalRead(STOP_BUTTON_PIN)  == LOW; }
// Flow: a Hall pulse train on FLOW_PULSE_PIN counted by PCNT (CR-06). Until the
// sensor is wired this is BYPASSED, exactly as the S3 test rig has run every
// cycle to date. Loud at boot so nobody mistakes it for a check.
#ifndef FLOW_INTERLOCK
#define FLOW_INTERLOCK 0
#endif
static bool flowProven() {
#if FLOW_INTERLOCK
  // TODO(step 4): PCNT on FLOW_PULSE_PIN, rate over a 1 s window > threshold.
  return false;
#else
  return true;
#endif
}
static float coolantTempC()      { return NAN; }   // TODO(step 4): DS18B20 on ONEWIRE_PIN

static void enter(St s) { state = s; tState = millis(); }

// A fault always opens the SSR. Whether it also drops mains depends on the
// class: a sense chain that is lying (CR-03) or a time ceiling means the bus is
// not trustworthy and mains goes; a lost rectifier session or an operator abort
// leaves mains up so cooling keeps running (§6.10: the E-stop itself must not
// drop the 12 V rail, and neither should a firmware fault take the pump).
static void fault(const char* why, bool dropMains = false) {
  ssr(false);
  if (dropMains) mainsDrop();
  faultMsg = why;
  enter(St::FAULT);
  sayf("FAULT%s: %s\n", dropMains ? " (mains dropped)" : "", why);
}

// ---- CR-10 cycle log --------------------------------------------------------
// Serial for now; persist to LittleFS/SD next. This log IS the metrology (§8.2).
static void logCycle(bool completed) {
  sayf("{\"ms\":%lu,\"mode\":\"%s\",\"size\":\"%s\",\"durS\":%lu,"
                "\"J\":%.1f,\"targetJ\":%.1f,\"elapsedMs\":%lu,"
                "\"rectV\":%.0f,\"rectI\":%.1f,\"completed\":%d,\"provisional\":%d}\n",
                millis(), mode == Mode::TIMED ? "timed" : "dosed",
                mode == Mode::DOSED ? PRESETS[sel].name : "-",
                mode == Mode::TIMED ? (unsigned long)DUR_LADDER_S[durIx] : 0UL,
                doseJ, doseTargetJ, millis() - tHeatStart,
                rectTelem.vOut, rectTelem.iOut, completed ? 1 : 0,
                mode == Mode::DOSED ? (PRESETS[sel].provisional ? 1 : 0) : 1);
}

// ---- plausibility (CR-03) ---------------------------------------------------
// Energy integration cannot detect its own miscalibration. If the sense chain
// reads low while real power is high, the dose accrues slowly and heating
// continues past target. The absolute ceiling bounds the damage; this catches it
// sooner. In TIMED mode only the rectifier's own telemetry is checked.
static bool plausible() {
#if BENCH_MODE
  return true;                       // nothing is connected to be plausible about
#endif
  if (millis() - tHeatStart < PLAUS_GRACE_MS) return true;
  if (rectTelem.online) {
    if (rectTelem.vOut < PLAUS_V_MIN || rectTelem.vOut > PLAUS_V_MAX) { faultMsg = "bus V out of window (CAN)"; return false; }
  }
  if (mode == Mode::DOSED) {
    EnergyMeter::Sample s = meter.read();
    doseJ = s.energyJ;
    if (!s.valid)                                           { faultMsg = "INA228 read failed";      return false; }
    if (s.currA < PLAUS_I_MIN_A || s.currA > PLAUS_I_MAX_A) { faultMsg = "current out of window";   return false; }
    // Independent channel: different sensor, different vendor, different silicon.
    // The rectifier's current telemetry lags 2–3 s on the fall (measured), so this
    // only runs once both have had time to agree.
    if (rectTelem.online && !isnan(rectTelem.iOut) && rectTelem.iOut > 5.0f &&
        millis() - tHeatStart > 4000) {
      float disagree = fabsf(rectTelem.iOut - s.currA) / rectTelem.iOut;
      if (disagree > CAN_CROSS_TOL) { faultMsg = "INA228 vs rectifier disagree"; return false; }
    }
  }
  return true;
}

// ---- states -----------------------------------------------------------------
// ARMING (CR-05 rev. §6.10): all three, none sufficient alone —
//   bus voltage over CAN ≥ ARM_BUS_V_MIN, held across ARM_SAMPLES consecutive
//   telemetry samples, and ≥ ARM_MIN_MS since mains came up. Not met within
//   ARM_TIMEOUT_MS → FAULT: a dead rectifier and a slow one look identical to a
//   clock, and only the voltage condition separates them.
static void doArming() {
  uint32_t now = millis();
#if BENCH_MODE
  if (now - tMainsUp >= 2000) {
    sayfln("ARMED (BENCH MODE — arming gate BYPASSED, no rectifier required)");
    enter(St::IDLE);
  }
  return;
#endif
  if (now - tMainsUp > ARM_TIMEOUT_MS) return fault("rectifier never reached arming voltage (CR-05)");
  static uint32_t lastRx = 0;
  if (rectTelem.online && rectTelem.lastRxMs != lastRx) {          // a NEW sample
    lastRx = rectTelem.lastRxMs;
    armSamples = (rectTelem.vOut >= ARM_BUS_V_MIN) ? armSamples + 1 : 0;
  }
  if (armSamples >= ARM_SAMPLES && now - tMainsUp >= ARM_MIN_MS) {
    // Bus proven. Set the operating point — ONLINE, clamped in R48, reverts on
    // its own if this controller dies. The SSR stays open: the ZVS sees nothing
    // of the rectifier's ~3 s walk-in to the new setpoint.
    rect.setCurrentLimit(BUS_CURRENT_LIM_A);
    rect.setVoltage(BUS_SETPOINT_V);
    sayf("ARMED: bus %.0f V over CAN, %u samples, %lu s after mains\n",
                  rectTelem.vOut, armSamples, (now - tMainsUp) / 1000);
    enter(St::IDLE);
  }
}

static void doPrecheck() {
  if (!estopClosed())        return fault("E-stop open");
  if (!flowProven())         return fault("no coolant flow (CR-06)");
#if !BENCH_MODE
  float ct = coolantTempC();
  if (!isnan(ct) && ct > COOLANT_TEMP_MAX_C) return fault("coolant too hot (CR-07)");
  if (!rectTelem.online)     return fault("rectifier CAN offline (CR-11)");
  if (rectTelem.vOut < ARM_BUS_V_MIN) return fault("bus sagged below arming voltage");
#endif

  if (mode == Mode::DOSED) {
    if (!SHUNT_CALIBRATED)   return fault("DOSED refused: shunt not calibrated (VR-11)");
    if (!meterUp)            return fault("DOSED refused: INA228 not present");
    if (presetLocked[sel])   return fault("preset locked — recalibrate (CR-02)");
    doseTargetJ = PRESETS[sel].energyJ;
    meter.resetDose();
  } else {
    doseTargetJ = 0;
  }
  doseJ = 0;
  tHeatStart = millis();
  ssr(true);                 // CR-05: the STEP. Not a ramp, not CAN, not the contactor.
  if (mode == Mode::TIMED) sayf("HEATING: timed %lu s\n", (unsigned long)DUR_LADDER_S[durIx]);
  else                     sayf("HEATING: dosed %s (%.0f J)\n", PRESETS[sel].name, doseTargetJ);
  enter(St::HEATING);
}

static void doHeating() {
  uint32_t el = millis() - tHeatStart;
  if (!BENCH_MODE && stopPressed())  { ssr(false); logCycle(false); enter(St::SETTLE); return; }
  if (!plausible())                  return fault(faultMsg, true);
  if (!flowProven())                 return fault("flow lost mid-cycle");
  if (!estopClosed())                return fault("E-stop opened mid-cycle");

  if (mode == Mode::TIMED) {
    if (el >= DUR_LADDER_S[durIx] * 1000UL) { ssr(false); logCycle(true); enter(St::SETTLE); }
  } else {
    if (el > PRESETS[sel].ceilingMs)     return fault("absolute time ceiling (CR-03)", true);
    if (doseJ >= doseTargetJ)            { ssr(false); logCycle(true); enter(St::SETTLE); }  // CR-01
  }
}

static void doMeasure() {
  // No on-board thermocouple. Peak temperature is the campaign's handheld reading
  // and temper colour; CR-02's flag/lockout thresholds apply to those, entered by
  // the operator — TODO(step 3): a console command to record them against the
  // last cycle so presetLocked[] can be set from a measurement.
  enter(St::COOLDOWN);
}

// ---- console ----------------------------------------------------------------
// Non-blocking, printable-only line assembly. readStringUntil() blocks a full
// second and echoes line noise as commands — a trap already paid for once on the
// bring-up board.
static const char* stName() {
  switch (state) {
    case St::BOOT: return "BOOT"; case St::ARMING: return "ARMING";
    case St::IDLE: return "IDLE"; case St::PRECHECK: return "PRECHECK";
    case St::HEATING: return "HEATING"; case St::SETTLE: return "SETTLE";
    case St::MEASURE: return "MEASURE"; case St::COOLDOWN: return "COOLDOWN";
    default: return "FAULT";
  }
}

static void status() {
  sayf("state %-8s mode %-5s  dur %lus  preset %s\n", stName(),
                mode == Mode::TIMED ? "TIMED" : "DOSED",
                (unsigned long)DUR_LADDER_S[durIx], PRESETS[sel].name);
  sayf("  R48: %s  %.0f V  %.1f A   %s\n",
                rectTelem.online ? "online" : "OFFLINE", rectTelem.vOut, rectTelem.iOut,
                state == St::ARMING ? "(arming: need bus>=40V x3 samples and 30s)" : "");
  sayf("  INA228: %s   shunt %s   dose %.0f/%.0f J\n",
                meterUp ? "present" : "absent",
                SHUNT_CALIBRATED ? "calibrated" : "UNCALIBRATED (DOSED refused)",
                doseJ, doseTargetJ);
  if (state == St::HEATING)
    sayf("  heating %lu ms of %lus\n", millis() - tHeatStart,
                  (unsigned long)DUR_LADDER_S[durIx]);
  if (state == St::FAULT) sayf("  FAULT: %s   ('ack' to clear)\n", faultMsg ? faultMsg : "?");
#if BENCH_MODE
  sayfln("  *** BENCH MODE — arming gate and interlocks BYPASSED, buttons ignored ***");
#endif
  sayf("  fitted: E-stop %s  contactor %s  flow %s  shunt %s\n",
       ESTOP_FITTED ? "yes" : "NO", CONTACTOR_FITTED ? "yes" : "NO",
       FLOW_INTERLOCK ? "yes" : "NO", SHUNT_CALIBRATED ? "cal" : "NO");
}

static void wifiBegin();

static void handleLine(const char* line) {
  String s(line); s.trim();
  if (!s.length()) return;
  if      (s == "st" || s == "status") status();
  else if (s == "start") {
    if (state != St::IDLE) sayf("refused: state is %s, not IDLE\n", stName());
    else enter(St::PRECHECK);
  }
  else if (s == "stop") {
    if (state == St::HEATING) { ssr(false); logCycle(false); enter(St::SETTLE); sayfln("aborted"); }
    else sayfln("not heating");
  }
  else if (s == "ack") {
    if (state == St::FAULT) { faultMsg = nullptr; armSamples = 0; tMainsUp = millis();
                              enter(St::ARMING); sayfln("fault cleared -> ARMING"); }
    else sayfln("not in FAULT");
  }
  else if (s.startsWith("dur")) {
    if (s.length() > 3) {
      uint32_t want = s.substring(3).toInt();
      for (uint8_t i = 0; i < DUR_LADDER_COUNT; i++) if (DUR_LADDER_S[i] == want) durIx = i;
    } else durIx = (durIx + 1) % DUR_LADDER_COUNT;      // step the ladder, like the rig's button
    sayf("duration %lu s\n", (unsigned long)DUR_LADDER_S[durIx]);
  }
  else if (s.startsWith("mode")) {
    if (s.indexOf("dosed") > 0) {
      if (!SHUNT_CALIBRATED) sayfln("DOSED refused: shunt not calibrated (VR-11)");
      else mode = Mode::DOSED;
    } else mode = Mode::TIMED;
    sayf("mode %s\n", mode == Mode::TIMED ? "TIMED" : "DOSED");
  }
  else if (s.startsWith("preset")) {
    uint8_t i = s.substring(6).toInt();
    if (i < PRESET_COUNT) { sel = i; sayf("preset %s\n", PRESETS[sel].name); }
    else { for (uint8_t k = 0; k < PRESET_COUNT; k++) sayf("  %u %s\n", k, PRESETS[k].name); }
  }
  // Separate ssid/pass commands: a combined "wifi <ssid> <pass>" splits on the
  // first space, so an SSID containing one silently puts half of itself into the
  // password and the only symptom is a handshake timeout that reads as a typo.
  else if (s.startsWith("ssid ")) { prefs.putString("ssid", s.substring(5));
                                    sayf("ssid saved (%d chars) — reboot to join\n", s.length()-5); }
  else if (s.startsWith("pass ")) { prefs.putString("pass", s.substring(5));
                                    sayf("pass saved (%d chars) — reboot to join\n", s.length()-5); }
  else if (s == "wifi?")          { sayf("ssid \"%s\"  pass %d chars  %s\n",
                                         prefs.getString("ssid","").c_str(),
                                         prefs.getString("pass","").length(),
                                         otaReady ? WiFi.localIP().toString().c_str() : "not connected"); }
  else if (s == "wifi clear")     { prefs.remove("ssid"); prefs.remove("pass"); sayfln("cleared"); }
  else sayfln("commands: st | start | stop | ack | dur [s] | mode timed|dosed | preset [n]"
              " | ssid <n> | pass <p> | wifi? | wifi clear");
}

static void pollConsole() {
  static char buf[64]; static uint8_t n = 0;
  while (Serial.available()) {
    int c = Serial.read();
    if (c == '\n' || c == '\r') { if (n) { buf[n] = 0; n = 0; handleLine(buf); } }
    else if (c >= 0x20 && c < 0x7F && n < sizeof(buf) - 1) buf[n++] = (char)c;
  }
}

// ---- wifi / OTA / telnet -----------------------------------------------------
// Credentials live in NVS and are typed at the serial console — they never appear
// in the repo or in a build. Set once with:  ssid <name>   then   pass <secret>
static void wifiBegin() {
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  if (!ssid.length()) { sayfln("no wifi credentials — 'ssid <name>' then 'pass <secret>'"); return; }

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(OTA_HOSTNAME);
  WiFi.begin(ssid.c_str(), pass.c_str());
  sayf("wifi: joining %s ", ssid.c_str());
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) { delay(300); say("."); }
  if (WiFi.status() != WL_CONNECTED) { sayfln(" FAILED — continuing on USB only"); return; }

  WiFi.setSleep(false);      // power save cost 40–140 ms and made espota time out
  sayf(" %s  (%s.local)\n", WiFi.localIP().toString().c_str(), OTA_HOSTNAME);

  // OTA is serviced ONLY while IDLE (see loop) — an update landing mid-cycle
  // with the field on is the obvious disaster, and gating it costs one condition.
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.onStart([]() { ssr(false); sayfln("\nOTA start — SSR forced open"); });
  ArduinoOTA.onEnd([]()   { sayfln("\nOTA done — rebooting"); });
  ArduinoOTA.onError([](ota_error_t e) { sayf("\nOTA error %u\n", e); });
  ArduinoOTA.begin();
  telnetSrv.begin();
  telnetSrv.setNoDelay(true);
  sayf("telnet console: telnet %s %d\n", WiFi.localIP().toString().c_str(), TELNET_PORT);
  otaReady = true;
}

static void serviceNet() {
  if (!otaReady) return;
  // OTA only while IDLE or FAULT — never with the SSR closed.
  if (state == St::IDLE || state == St::FAULT) ArduinoOTA.handle();

  // One client at a time; a second connection displaces the first rather than
  // being refused — a stale session from a closed laptop lid must never lock you
  // out of your own machine.
  if (telnetSrv.hasClient()) {
    if (telnetCli && telnetCli.connected()) telnetCli.stop();
    telnetCli = telnetSrv.available();
    telnetCli.printf("\n=== shrink-fit controller ===\n"
                     "commands: st | start | stop | ack | dur [s] | mode timed|dosed | preset [n]\n");
  }
  static char tb[64]; static uint8_t tn = 0;
  while (telnetCli && telnetCli.available()) {
    int c = telnetCli.read();
    if (c == '\n' || c == '\r') { if (tn) { tb[tn] = 0; tn = 0; handleLine(tb); } }
    else if (c >= 0x20 && c < 0x7F && tn < sizeof(tb) - 1) tb[tn++] = (char)c;
  }
}

// ---- setup / loop -----------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(300);
  sayfln("\n=== shrink-fit controller — NodeMCU-32S ===");

  pinMode(SSR_PIN, OUTPUT);        ssr(false);
  pinMode(MAINS_DROP_PIN, OUTPUT); digitalWrite(MAINS_DROP_PIN, LOW);
  pinMode(ESTOP_SENSE_PIN,  INPUT);   // 34–39: no internal pull-ups; external 10 k required
  pinMode(START_BUTTON_PIN, INPUT);
  pinMode(STOP_BUTTON_PIN,  INPUT);
  pinMode(ENC_SW_PIN,       INPUT_PULLUP);   // duration button now, encoder push later
  pinMode(STATUS_LED_PIN,   OUTPUT);
  pinMode(STATUS_LED2_PIN,  OUTPUT);

  prefs.begin("shrinkctl", false);
  // Restore the saved duration, snapping anything off-ladder back to a rung --
  // a stored value from an older ladder would otherwise be unreachable by the
  // button and invisible in the flash count.
  {
    uint32_t saved = prefs.getUInt("dur", DUR_LADDER_S[durIx]);
    bool onLadder = false;
    for (uint8_t i = 0; i < DUR_LADDER_COUNT; i++)
      if (DUR_LADDER_S[i] == saved) { durIx = i; onLadder = true; }
    if (!onLadder) { durIx = 1; prefs.putUInt("dur", DUR_LADDER_S[durIx]); }
  }
  idleAnchor = millis();

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 400000);
  meterUp = meter.begin(INA228_ADDR, SHUNT_OHMS, I_MAX_EXPECTED_A);
  if (!meterUp) sayfln("WARN: INA228 not found — DOSED mode unavailable, TIMED only");
#if !FLOW_INTERLOCK
  sayfln("WARN: flow interlock BYPASSED (FLOW_INTERLOCK=0) — as on the S3 test rig");
#endif
  sayfln("NOTE: no on-board thermocouple; peak temperature is the handheld + temper colour");
#if BENCH_MODE
#if !ESTOP_FITTED
  sayfln("WARNING: NO E-STOP FITTED — GPIO 36 ignored, estopClosed() forced true.");
#endif
#if !CONTACTOR_FITTED
  sayfln("WARNING: NO AC CONTACTOR — firmware cannot remove mains. Opening the SSR is");
  sayfln("         the ONLY protective action. (ZVS fed straight off the R48.)");
#endif
  sayfln("\n*** BENCH MODE ***  arming gate BYPASSED, interlocks BYPASSED, buttons IGNORED.");
  sayfln("    SSR pin GPIO 4 still drives — put an LED on it, not a coil.");
#endif
  sayfln("commands: st | start | stop | ack | dur [s] | mode timed|dosed | preset [n]\n");

  if (!rect.begin(CAN_TX_PIN, CAN_RX_PIN, CAN_BITRATE_KBPS))
    sayfln("WARN: TWAI init failed — rectifier link unavailable");

  wifiBegin();

  tMainsUp = millis();       // TODO(step 4): from the contactor aux / E-stop sense edge
  enter(St::ARMING);
}

void loop() {
  static uint32_t tFast = 0, tSlow = 0, tLed = 0;
  uint32_t now = millis();

  // The rectifier session must be serviced unconditionally: it is what keeps the
  // setpoints alive, and its silence timeout is the safety net if this loop dies.
  pollConsole();
  serviceNet();
  rect.service();

  if (now - tSlow >= 250) { tSlow = now; rect.poll(rectTelem); }

  if (now - tFast >= 1) {
    tFast = now;
    switch (state) {
      case St::BOOT:     enter(St::ARMING); break;
      case St::ARMING:   doArming();   break;
      case St::IDLE: {
        // Duration button: step the ladder, save, and replay the new count.
        bool d = (digitalRead(DURATION_BUTTON_PIN) == LOW);
        if (d && !lastDurLevel) {
          durIx = (durIx + 1) % DUR_LADDER_COUNT;
          prefs.putUInt("dur", DUR_LADDER_S[durIx]);
          idleAnchor = millis();
          sayf("duration %lu s (%u flashes)\n",
               (unsigned long)DUR_LADDER_S[durIx], (unsigned)(DUR_LADDER_S[durIx] / 5));
        }
        lastDurLevel = d;
        // Edge, not level: after COOLDOWN a still-held button would otherwise
        // start another cycle the moment the state returns to IDLE.
        bool now_s = BENCH_MODE ? false : startPressed();
        if (now_s && !lastStartLevel) enter(St::PRECHECK);
        lastStartLevel = now_s;
        break;
      }
      case St::PRECHECK: doPrecheck(); break;
      case St::HEATING:  doHeating();  break;
      case St::SETTLE:   if (now - tState >= 200) enter(St::MEASURE); break;
      case St::MEASURE:  doMeasure();  break;
      case St::COOLDOWN:
        // CR-04 as a state: re-heating a warm holder permanently enlarges bores.
        // No nose TC, so a minimum dwell stands in until CR-04 is rewritten.
        if (now - tState >= COOLDOWN_MIN_MS) enter(St::IDLE);
        break;
      case St::FAULT:
        // Latched. Explicit acknowledge only — never the start button, so a
        // reflexive second press cannot clear and re-run. TODO(step 3): console.
        break;
    }
  }

  // Status by blink pattern, never colour (shop rule). One LED, as on the rig.
  //   HEATING  solid
  //   IDLE     plays the duration back as runSeconds/5 flashes, then a dark gap
  //   FAULT    fast blink
  //   other    slow blink
  bool on;
  if (state == St::HEATING) on = true;
  else if (state == St::IDLE) {
    uint8_t  n   = (uint8_t)(DUR_LADDER_S[durIx] / 5); if (!n) n = 1;
    uint32_t lit = (uint32_t)n * RO_PER;
    uint32_t ph  = (now - idleAnchor) % (lit + RO_GAP);
    on = (ph < lit) && ((ph % RO_PER) < 200);
  } else {
    uint32_t per = (state == St::FAULT) ? 150 : 1000;
    on = ((now / (per / 2)) & 1);
  }
  digitalWrite(STATUS_LED_PIN,  on ? HIGH : LOW);
  digitalWrite(STATUS_LED2_PIN, on ? HIGH : LOW);
  (void)tLed;
}
