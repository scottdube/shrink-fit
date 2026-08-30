// testrig.cpp — timed-cycle test rig with bus-voltage logging. Not the machine.
//
// Solves the problem that manual runs can't produce comparable data: holding the
// work, timing the run, and reading meters is too many hands. Here the run is
// exactly N seconds every time, the bus voltage is logged through it, and the
// PZEM's Wh counter is turned into joules-per-run.
//
// Energy method: the PZEM display steps in whole Wh, and one run is only ~4 Wh —
// 25% resolution. Fix it by running several times between two Wh readings and
// dividing. Four runs takes that to ~6%, and the rig makes repeats identical by
// construction. That is what the automation actually buys.
//
// Serial (115200):
//   r          run for the current duration
//   r <sec>    set duration and run          e.g.  r 20
//   d <sec>    set duration only
//   w <Wh>     mark a PZEM Wh reading; second mark prints Wh and J per run
//   cal <V>    one-point calibration — enter the true bus volts from the PZEM
//   s          status
//   wifi <ssid> <pass>    store credentials in NVS and connect (OTA)
//   any key during a run  ->  ABORT
//
// BUTTONS (the normal way to drive this — no laptop needed):
//   START     runs the stored duration. Ignored while running or in cooldown.
//   STOP      aborts a run. Does nothing when idle.
//   DURATION  steps the duration ladder and blinks the new setting back:
//             N flashes = N x 5 s, so 20 s is four flashes. Idle only.
// Two buttons, not one, because a single button that both starts and stops is
// mode-dependent: press it not knowing the state and you start a cycle you meant
// to end. STOP is a FIRMWARE ABORT and is not the SR-03 E-stop.
//
// LED: pattern AND colour, never colour alone.
//   idle      one short blink every 2 s
//   RUNNING   solid on
//   cooldown  double blink every 2 s
// One plain LED, so PATTERN is the entire signal.
//
// Build:  pio run -e testrig -t upload && pio device monitor -e testrig
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "config.h"

// Duration survives a power cycle so the rig can be set up once with USB
// attached and then run untethered on the buttons — which is the point of
// having buttons at all (and SR: no USB while energised).
static Preferences prefs;

// Duration ladder. The calibration protocol steps duration up between attempts,
// so this has to be settable at the machine, not over USB.
// 10 s rungs, so the LED read-back is one flash per 10 s: the whole ladder is
// 1..6 flashes and countable at a glance. 5 s rungs put 60 s at twelve flashes,
// which nobody can count. Fine time resolution is the wrong tool here anyway —
// the machine doses on ENERGY; these timed cycles are for coil comparison.
// Serial `d <sec>` still sets any value 1..60; the button then snaps to rung 0.
static const uint32_t DUR_LADDER[] = {10, 20, 30, 40, 50, 60};
static const uint8_t  DUR_COUNT = sizeof(DUR_LADDER)/sizeof(DUR_LADDER[0]);

// LED read-back of the setting: N pulses, non-blocking.
static uint32_t readoutUntil = 0;
static uint8_t  readoutN     = 0;

// ---- OTA --------------------------------------------------------------------
static void fieldOn(bool on);            // defined below in the io section
static bool otaReady = false;

static void otaBegin() {
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  if (ssid.length() == 0) {
    Serial.println("wifi: no credentials stored - OTA disabled. Use: wifi <ssid> <pass>");
    return;
  }
  Serial.printf("wifi: connecting to \"%s\" ...\n", ssid.c_str());
  WiFi.mode(WIFI_STA);
  // Power save is ON by default in Arduino and the radio sleeps between beacons,
  // which shows up as 30-130 ms ping latency and makes OTA invitations time out.
  // Measured on this rig 2026-08-27. Costs a little idle current, buys single-
  // digit latency and reliable OTA.
  WiFi.setSleep(false);
  WiFi.setHostname(OTA_HOSTNAME);
  WiFi.begin(ssid.c_str(), pass.c_str());
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 12000) delay(200);
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("wifi: FAILED - carrying on without OTA");
    WiFi.mode(WIFI_OFF);
    return;
  }
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.onStart([]() {
    fieldOn(false);                       // belt and braces; we only get here idle
    Serial.println("\nOTA start - field forced off");
  });
  ArduinoOTA.onEnd([]()   { Serial.println("\nOTA done, rebooting"); });
  ArduinoOTA.onError([](ota_error_t e){ Serial.printf("OTA error %u\n", e); });
  ArduinoOTA.begin();
  otaReady = true;
  Serial.printf("wifi: %s   OTA host \"%s\"\n",
                WiFi.localIP().toString().c_str(), OTA_HOSTNAME);
}

// ---- limits -----------------------------------------------------------------
static const uint32_t MAX_RUN_S  = 60;      // hard cap; no test needs more
static const uint32_t COOLDOWN_S = 30;      // enforced between runs
static uint32_t       runSeconds = 10;   // overwritten from NVS at boot
static float          divRatio   = BUS_DIV_RATIO;

// ---- state ------------------------------------------------------------------
enum class St { IDLE, RUNNING, COOLDOWN };
static St       state    = St::IDLE;
static uint32_t tStart   = 0;
static uint32_t tCoolEnd = 0;
static uint32_t runCount = 0;

static float    vStart = 0, vMin = 0, vEnd = 0;

// Wh accounting across a series of runs
static bool     whMarked      = false;
static float    whMark        = 0;
static uint32_t runsAtWhMark  = 0;

// ---- io ---------------------------------------------------------------------
static void fieldOn(bool on) { digitalWrite(CONTACTOR_PIN, on ? HIGH : LOW); }

// ---- status LED --------------------------------------------------------------
// ONE plain LED. Pattern is the whole signal - no colour to rely on.
//   idle      one short blink every 2 s
//   RUNNING   solid on
//   cooldown  double blink every 2 s
static void ledService(int state) {
#if STATUS_LED_ENABLE
  // Read-back takes priority: N pulses at 400 ms, then normal patterns resume.
  if (millis() < readoutUntil) {
    uint32_t r = (readoutUntil - millis());
    digitalWrite(STATUS_LED_PIN, ((r % 400) < 200) ? HIGH : LOW);
    return;
  }
  uint32_t t = millis() % 2000;
  bool on;
  switch (state) {
    case 1:  on = true;                                   break;  // RUNNING
    case 2:  on = (t < 90) || (t > 220 && t < 310);       break;  // cooldown
    default: on = (t < 90);                               break;  // idle
  }
  digitalWrite(STATUS_LED_PIN, on ? HIGH : LOW);
#else
  (void)state;
#endif
}

// analogReadMilliVolts applies the factory ADC calibration — materially better
// than raw analogRead on the S3, whose ADC is noisy and non-linear.
static float busVolts() {
  uint32_t acc = 0;
  for (int i = 0; i < 8; i++) acc += analogReadMilliVolts(BUS_SENSE_PIN);
  return (acc / 8.0f) * divRatio / 1000.0f;
}

// ---- runs -------------------------------------------------------------------
static void startRun() {
  if (state == St::COOLDOWN) {
    Serial.printf("cooldown: %lu s remaining\n", (tCoolEnd - millis()) / 1000);
    return;
  }
  if (state != St::IDLE) return;

  runCount++;
  vStart = busVolts();
  vMin   = vStart;

  Serial.printf("\n=== RUN %lu — %lu s — V0 %.1f ===\n", runCount, runSeconds, vStart);
  Serial.println("any key aborts");
  tStart = millis();
  state  = St::RUNNING;
  fieldOn(true);                            // hard step — see CR-05
}

static void endRun(const char* why) {
  fieldOn(false);
  uint32_t actual = millis() - tStart;
  vEnd = busVolts();

  Serial.printf("=== FIELD OFF (%s) %lu ms ===\n", why, actual);
  Serial.printf("run %lu | %lu ms | V start %.1f  min %.1f  end %.1f  sag %.1f\n",
                runCount, actual, vStart, vMin, vEnd, vStart - vMin);
  Serial.println("read peak temp ~200 ms from now (TR-04), then the meters");

  tCoolEnd = millis() + COOLDOWN_S * 1000UL;
  state    = St::COOLDOWN;
}

// ---- Wh accounting ----------------------------------------------------------
static void markWh(float wh) {
  if (!whMarked) {
    whMark = wh; runsAtWhMark = runCount; whMarked = true;
    Serial.printf("Wh mark set: %.1f (at run %lu). Run some cycles, then 'w' again.\n",
                  wh, runCount);
    return;
  }

  float    dWh  = wh - whMark;
  uint32_t runs = runCount - runsAtWhMark;
  if (runs == 0) { Serial.println("no runs since the mark"); whMark = wh; return; }

  float perRun = dWh / runs;
  Serial.printf("\n--- ENERGY ---\n");
  Serial.printf("%.1f Wh over %lu runs of %lu s\n", dWh, runs, runSeconds);
  Serial.printf("%.2f Wh/run = %.0f J/run = %.0f W average\n",
                perRun, perRun * 3600.0f, perRun * 3600.0f / runSeconds);
  Serial.printf("(display steps 1 Wh, so ~%.0f%% resolution at this run count)\n",
                100.0f / (dWh > 0 ? dWh : 1));
  Serial.println("---------------\n");

  whMark = wh; runsAtWhMark = runCount;     // roll the mark forward
}

// ---- commands ---------------------------------------------------------------
static void saveDuration() { prefs.putUInt("dur", runSeconds); }

// Step to the next rung, wrapping. Blinks the new value back as runSeconds/5
// pulses so the setting is readable without a serial monitor.
static void stepDuration(bool down) {
  uint8_t i = 0;
  while (i < DUR_COUNT && DUR_LADDER[i] != runSeconds) i++;
  if (i >= DUR_COUNT) i = 0;
  else i = down ? (uint8_t)((i + DUR_COUNT - 1) % DUR_COUNT)
                : (uint8_t)((i + 1) % DUR_COUNT);
  runSeconds = DUR_LADDER[i];
  saveDuration();
  readoutN     = (uint8_t)(runSeconds / 10);
  readoutUntil = millis() + (uint32_t)readoutN * 400UL;
  Serial.printf("duration = %lu s (%u flashes, %s)\n",
                runSeconds, readoutN, down ? "down" : "up");
}

static void handleLine(String s) {
  s.trim();
  if (!s.length()) return;

  if (s.startsWith("wifi")) {
    String rest = s.substring(4); rest.trim();
    int sp = rest.indexOf(' ');
    if (sp < 1) { Serial.println("usage: wifi <ssid> <password>"); return; }
    prefs.putString("ssid", rest.substring(0, sp));
    prefs.putString("pass", rest.substring(sp + 1));
    Serial.println("wifi: stored in NVS. Reboot to connect.");
    return;
  }

  if (s.startsWith("cal")) {
    float truth = s.substring(3).toFloat();
    if (truth < 10 || truth > 70) { Serial.println("cal <actual bus volts, 10..70>"); return; }
    float measuredMv = 0;
    for (int i = 0; i < 8; i++) measuredMv += analogReadMilliVolts(BUS_SENSE_PIN);
    measuredMv /= 8.0f;
    if (measuredMv < 50) { Serial.println("ADC reads ~0 — bus off, or divider not wired"); return; }
    divRatio = truth / (measuredMv / 1000.0f);
    Serial.printf("divider ratio = %.3f (was %.3f). Put this in config.h BUS_DIV_RATIO.\n",
                  divRatio, BUS_DIV_RATIO);
    return;
  }

  char c = s.charAt(0);
  float v = (s.length() > 1) ? s.substring(1).toFloat() : 0;

  switch (c) {
    case 'r':
      if (v > 0) {
        if (v > MAX_RUN_S) { Serial.printf("capped at %lu s\n", MAX_RUN_S); v = MAX_RUN_S; }
        runSeconds = (uint32_t)v;
        saveDuration();
      }
      startRun();
      break;
    case 'd':
      if (v > 0 && v <= MAX_RUN_S) { runSeconds = (uint32_t)v; saveDuration();
                                     Serial.printf("duration = %lu s (saved)\n", runSeconds); }
      else Serial.printf("1..%lu\n", MAX_RUN_S);
      break;
    case 'w':
      if (v > 0) markWh(v);
      else Serial.println("w <Wh reading from the PZEM>");
      break;
    case 's':
      Serial.printf("state=%d duration=%lu s runs=%lu bus=%.1f V ratio=%.3f\n",
                    (int)state, runSeconds, runCount, busVolts(), divRatio);
      break;
    default:
      Serial.println("r | r <sec> | d <sec> | w <Wh> | cal <V> | s");
  }
}

// ---- setup / loop -----------------------------------------------------------
void setup() {
  pinMode(CONTACTOR_PIN, OUTPUT);
  fieldOn(false);                            // before anything else
  pinMode(START_BUTTON_PIN, INPUT_PULLUP);
  pinMode(STOP_BUTTON_PIN,  INPUT_PULLUP);
  pinMode(DURATION_BUTTON_PIN, INPUT_PULLUP);
#if STATUS_LED_ENABLE
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
#endif

  prefs.begin("testrig", false);
  runSeconds = prefs.getUInt("dur", 10);
  if (runSeconds < 1 || runSeconds > MAX_RUN_S) runSeconds = 10;
  analogSetPinAttenuation(BUS_SENSE_PIN, ADC_11db);   // full 0–3.3 V span

  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) { delay(10); }

  Serial.println("\n===== timed test rig =====");
  Serial.printf("SSR GPIO %d (via 2N3904 low-side, 12 V) | button GPIO %d | bus sense GPIO %d\n",
                CONTACTOR_PIN, START_BUTTON_PIN, BUS_SENSE_PIN);
  Serial.printf("duration %lu s | max %lu | cooldown %lu | divider %.2f:1\n",
                runSeconds, MAX_RUN_S, COOLDOWN_S, divRatio);
  Serial.println("WATER RUNNING FIRST. Board is marked 'disable output no load'.");
  Serial.println("First time: bus up, then 'cal <volts from PZEM>' to fix the divider.");
  Serial.printf("BUTTONS: START GPIO %d runs %lu s | STOP GPIO %d aborts | DURATION GPIO %d steps\n",
                START_BUTTON_PIN, runSeconds, STOP_BUTTON_PIN, DURATION_BUTTON_PIN);
  Serial.println("ladder: 10 20 30 40 50 60 s. Tap = up, hold 1 s = down.");
  Serial.println("Blinks back at 1 flash per 10 s, so 1..6 flashes.");
  Serial.println("STOP is a firmware abort, NOT the E-stop (SR-03).");
  Serial.println("r | r <sec> | d <sec> | w <Wh> | cal <V> | s | wifi <ssid> <pass>");

  otaBegin();
}

void loop() {
  // OTA is serviced ONLY when idle. An update cannot start, and cannot proceed,
  // while a cycle is running or cooling down.
  if (otaReady && state == St::IDLE) ArduinoOTA.handle();

  if (Serial.available()) {
    if (state == St::RUNNING) { while (Serial.available()) Serial.read(); endRun("aborted"); }
    else handleLine(Serial.readStringUntil('\n'));
  }

  // START: only ever starts. Never doubles as a stop - see the header note.
  static bool prevStart = HIGH;
  bool nowStart = digitalRead(START_BUTTON_PIN);
  if (prevStart == HIGH && nowStart == LOW) {
    if (state == St::IDLE) startRun();
    else Serial.println("START ignored - already running or in cooldown");
    delay(30);
  }
  prevStart = nowStart;

  // STOP: only ever stops.
  static bool prevStop = HIGH;
  bool nowStop = digitalRead(STOP_BUTTON_PIN);
  if (prevStop == HIGH && nowStop == LOW) {
    if (state == St::RUNNING) endRun("STOP button");
    delay(30);
  }
  prevStop = nowStop;

  // DURATION: idle only. Changing it mid-run would make the log lie about what
  // was actually commanded.
  // Short press steps UP, hold steps DOWN. Bidirectional because the calibration
  // protocol converges from both sides, and wrapping all the way round to go back
  // one rung is exactly the annoyance that makes people stop using a control.
  static bool     prevDur    = HIGH;
  static uint32_t durDownAt  = 0;
  static bool     durHandled = false;
  bool nowDur = digitalRead(DURATION_BUTTON_PIN);

  if (prevDur == HIGH && nowDur == LOW) {          // pressed
    durDownAt = millis(); durHandled = false; delay(30);
  }
  if (nowDur == LOW && !durHandled && (millis() - durDownAt) > 1000) {
    if (state == St::IDLE) stepDuration(true);     // held -> down
    else Serial.println("DURATION ignored - only adjustable when idle");
    durHandled = true;
  }
  if (prevDur == LOW && nowDur == HIGH) {          // released
    if (!durHandled) {
      if (state == St::IDLE) stepDuration(false);  // short -> up
      else Serial.println("DURATION ignored - only adjustable when idle");
    }
    durHandled = true; delay(30);
  }
  prevDur = nowDur;

  ledService(state == St::RUNNING ? 1 : (state == St::COOLDOWN ? 2 : 0));

  switch (state) {
    case St::RUNNING: {
      uint32_t el = millis() - tStart;
      float v = busVolts();
      if (v < vMin) vMin = v;
      if (el >= runSeconds * 1000UL) { endRun("complete"); break; }

      static uint32_t tTick = 0;             // 1 Hz so you can watch the meters
      if (millis() - tTick >= 1000) { tTick = millis();
        Serial.printf("  %2lu / %lu s   %.1f V\n", el / 1000, runSeconds, v); }
      break;
    }
    case St::COOLDOWN:
      if ((int32_t)(millis() - tCoolEnd) >= 0) { Serial.println("ready\n"); state = St::IDLE; }
      break;
    default: break;
  }
}
