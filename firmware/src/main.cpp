// main.cpp — shrink-fit cycle state machine.
//
// Control law: terminate on accumulated ENERGY, never on elapsed time (CR-01).
// Time exists only as an absolute abort ceiling (CR-03).
//
// SKELETON. Compiles in structure; the R48 CAN frame format is unported (see
// r48_can.cpp) and the HMI is stubbed. The control path is deliberately
// independent of the HMI so a display can land later without touching this.
//
// THREE THINGS THIS FIRMWARE IS NOT:
//   1. Not the E-stop. SR-03 puts the E-stop in series with the contactor coil,
//      in hardware. estopClosed() is status only.
//   2. Not a temperature controller. §8.3 — a thermocouple inside a kW-class
//      90 kHz field cannot be read during the cycle. Temperature is post-cycle
//      verification and a NEXT-cycle interlock (CR-02).
//   3. Not the output enable. CR-05 — CAN sets voltage, the contactor delivers
//      the step. Never enable the rectifier as a substitute.
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_MAX31855.h>
// CAN is the S3's built-in TWAI peripheral — no external controller. See r48_can.cpp.
#include "config.h"
#include "r48_can.h"
#include "energy.h"

// ---- devices ----------------------------------------------------------------
R48                 rect;
R48::Telem          rectTelem;
EnergyMeter         meter;
Adafruit_MAX31855   tc(MAX31855_SCK_PIN, MAX31855_CS_PIN, MAX31855_MISO_PIN);  // software SPI

// ---- cycle state ------------------------------------------------------------
enum class St { BOOT_BLOCK, IDLE, PRECHECK, HEATING, SETTLE, MEASURE, COOLDOWN, FAULT };
St        state = St::BOOT_BLOCK;
uint8_t   sel = PRESET_COUNT - 1;     // default to 1/2" — the campaign starts there
uint32_t  tState = 0, tHeatStart = 0;
float     doseTargetJ = 0, doseJ = 0, peakC = NAN, startC = NAN;
bool      presetLocked[PRESET_COUNT] = {false};
const char* faultMsg = nullptr;

// ---- io helpers -------------------------------------------------------------
static void contactor(bool on) { digitalWrite(CONTACTOR_PIN, on ? HIGH : LOW); }
static bool flowProven()       { return digitalRead(FLOW_SWITCH_PIN) == HIGH; }
static bool estopClosed()      { return digitalRead(ESTOP_SENSE_PIN) == HIGH; }
static bool startPressed()     { return digitalRead(START_BUTTON_PIN) == LOW; }
static float noseTempC()       { return tc.readCelsius(); }
static float coolantTempC()    { return NAN; }   // TODO: second TC on the return line

static void enter(St s) { state = s; tState = millis(); }

static void fault(const char* why) {
  contactor(false);
  faultMsg = why;
  enter(St::FAULT);
  Serial.printf("FAULT: %s\n", why);
}

// ---- CR-10 cycle log --------------------------------------------------------
// Serial for now. Persist to LittleFS/SD next; this log IS the metrology (§8.2)
// — release energy tracks remaining interference at ~3–4% per micron of bore
// growth, which beats any affordable direct measurement. A cycle that isn't
// logged is a data point permanently lost from that trend.
static void logCycle(bool released) {
  Serial.printf("{\"ms\":%lu,\"size\":\"%s\",\"J\":%.1f,\"targetJ\":%.1f,"
                "\"startC\":%.1f,\"peakC\":%.1f,\"durMs\":%lu,\"released\":%d,"
                "\"provisional\":%d}\n",
                millis(), PRESETS[sel].name, doseJ, doseTargetJ,
                startC, peakC, millis() - tHeatStart, released ? 1 : 0,
                PRESETS[sel].provisional ? 1 : 0);
}

// ---- plausibility (review Tier 1 #2) ----------------------------------------
// Energy integration cannot detect its own miscalibration. If the sense chain
// reads low while real power is high, the dose accrues slowly and heating
// continues well past target. The absolute ceiling in PRESETS bounds the damage;
// this window catches it far sooner.
static bool plausible(const EnergyMeter::Sample& s) {
  if (millis() - tHeatStart < PLAUS_GRACE_MS) return true;
  if (!s.valid)                                   { faultMsg = "INA228 read failed";   return false; }
  if (s.busV  < PLAUS_V_MIN || s.busV  > PLAUS_V_MAX) { faultMsg = "bus V out of window"; return false; }
  if (s.currA < PLAUS_I_MIN_A || s.currA > PLAUS_I_MAX_A) { faultMsg = "current out of window"; return false; }

  // Independent channel: different sensor, different vendor, different silicon.
  if (rectTelem.online && !isnan(rectTelem.iOut) && rectTelem.iOut > 1.0f) {
    float disagree = fabsf(rectTelem.iOut - s.currA) / rectTelem.iOut;
    if (disagree > CAN_CROSS_TOL) { faultMsg = "INA228 vs rectifier disagree"; return false; }
  }
  return true;
}

// ---- states -----------------------------------------------------------------
static void doPrecheck() {
  float t = noseTempC();
  if (isnan(t))                      return fault("thermocouple open");
  if (t > START_TEMP_MAX_C)          return fault("holder too warm (CR-04)");
  if (!flowProven())                 return fault("no coolant flow (CR-06)");
  float ct = coolantTempC();
  if (!isnan(ct) && ct > COOLANT_TEMP_MAX_C) return fault("coolant too hot (CR-07)");
  if (!estopClosed())                return fault("E-stop open");
  if (presetLocked[sel])             return fault("preset locked — recalibrate (CR-02)");
  if (!rectTelem.online)             return fault("rectifier CAN offline (CR-11)");

  startC       = t;
  doseTargetJ  = PRESETS[sel].energyJ;
  doseJ        = 0;
  peakC        = NAN;

  meter.resetDose();
  tHeatStart = millis();
  contactor(true);              // CR-05: the STEP. Not a ramp, not CAN.
  enter(St::HEATING);
}

static void doHeating() {
  EnergyMeter::Sample s = meter.read();
  doseJ = s.energyJ;

  if (!plausible(s))                                  return fault(faultMsg);
  if (!flowProven())                                  return fault("flow lost mid-cycle");
  if (millis() - tHeatStart > PRESETS[sel].ceilingMs) return fault("absolute time ceiling (CR-03)");

  if (doseJ >= doseTargetJ) {                         // CR-01: the normal exit
    contactor(false);
    enter(St::SETTLE);
  }
}

static void doMeasure() {
  peakC = noseTempC();                                // TR-04: ≥200 ms after field-off
  if (!isnan(peakC)) {
    if (peakC > PEAK_LOCKOUT_C) {
      presetLocked[sel] = true;
      Serial.printf("LOCKOUT %s: peak %.0f C > %.0f\n",
                    PRESETS[sel].name, peakC, PEAK_LOCKOUT_C);
    } else if (peakC > PEAK_FLAG_C) {
      Serial.printf("FLAG %s: peak %.0f C — acknowledge required\n",
                    PRESETS[sel].name, peakC);
    }
  }
  logCycle(true);   // TODO: operator confirms release; assumed for now
  enter(St::COOLDOWN);
}

// ---- setup / loop -----------------------------------------------------------
void setup() {
  Serial.begin(115200);

  pinMode(CONTACTOR_PIN, OUTPUT); contactor(false);
  pinMode(FLOW_SWITCH_PIN,  INPUT);
  pinMode(ESTOP_SENSE_PIN,  INPUT);
  pinMode(START_BUTTON_PIN, INPUT_PULLUP);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 400000);
  if (!meter.begin(INA228_ADDR, SHUNT_OHMS, I_MAX_EXPECTED_A))
    Serial.println("WARN: INA228 not found — dosing unavailable");
  if (!tc.begin())
    Serial.println("WARN: MAX31855 not found");
  if (!rect.begin(CAN_TX_PIN, CAN_RX_PIN, CAN_BITRATE_KBPS))
    Serial.println("WARN: TWAI init failed");

  enter(St::BOOT_BLOCK);
}

void loop() {
  static uint32_t tFast = 0, tSlow = 0;
  uint32_t now = millis();

  // slow: rectifier telemetry + setpoints
  if (now - tSlow >= CAN_POLL_MS) {
    tSlow = now;
    rect.poll(rectTelem);
  }

  // fast: dose integration and the plausibility window
  if (now - tFast >= 1) {
    tFast = now;
    switch (state) {

      case St::BOOT_BLOCK:
        // CR-05: hold the contactor open while the rectifier boots and settles.
        // It comes up AC-ON at its default voltage, so the bus is live before
        // the contactor ever closes — which is what makes the step possible.
        if (now > STARTUP_BLOCK_MS) {
          rect.setCurrentLimit(BUS_CURRENT_LIM_A);
          rect.setVoltage(BUS_SETPOINT_V);
          enter(St::IDLE);
        }
        break;

      case St::IDLE:
        if (startPressed()) enter(St::PRECHECK);
        break;

      case St::PRECHECK: doPrecheck(); break;
      case St::HEATING:  doHeating();  break;

      case St::SETTLE:
        if (now - tState >= TC_SETTLE_MS) enter(St::MEASURE);
        break;

      case St::MEASURE:  doMeasure();  break;

      case St::COOLDOWN: {
        // CR-04 enforced as a state, not just a check: re-heating a warm holder
        // is the failure mode that permanently enlarges bores.
        float t = noseTempC();
        if (!isnan(t) && t < START_TEMP_MAX_C) enter(St::IDLE);
        break;
      }

      case St::FAULT:
        // Latched. Requires an explicit acknowledge — deliberately not the
        // start button, so a reflexive second press cannot clear and re-run.
        break;
    }
  }
}
