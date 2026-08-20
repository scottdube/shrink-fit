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
//   any key during a run  ->  ABORT
//
// Build:  pio run -e testrig -t upload && pio device monitor -e testrig
#include <Arduino.h>
#include "config.h"

// ---- limits -----------------------------------------------------------------
static const uint32_t MAX_RUN_S  = 60;      // hard cap; no test needs more
static const uint32_t COOLDOWN_S = 30;      // enforced between runs
static uint32_t       runSeconds = 20;
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
static void handleLine(String s) {
  s.trim();
  if (!s.length()) return;

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
      }
      startRun();
      break;
    case 'd':
      if (v > 0 && v <= MAX_RUN_S) { runSeconds = (uint32_t)v; Serial.printf("duration = %lu s\n", runSeconds); }
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
  analogSetPinAttenuation(BUS_SENSE_PIN, ADC_11db);   // full 0–3.3 V span

  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) { delay(10); }

  Serial.println("\n===== timed test rig =====");
  Serial.printf("SSR GPIO %d (via 2N7000 from 12 V) | button GPIO %d | bus sense GPIO %d\n",
                CONTACTOR_PIN, START_BUTTON_PIN, BUS_SENSE_PIN);
  Serial.printf("duration %lu s | max %lu | cooldown %lu | divider %.2f:1\n",
                runSeconds, MAX_RUN_S, COOLDOWN_S, divRatio);
  Serial.println("WATER RUNNING FIRST. Board is marked 'disable output no load'.");
  Serial.println("First time: bus up, then 'cal <volts from PZEM>' to fix the divider.");
  Serial.println("r | r <sec> | d <sec> | w <Wh> | cal <V> | s");
}

void loop() {
  if (Serial.available()) {
    if (state == St::RUNNING) { while (Serial.available()) Serial.read(); endRun("aborted"); }
    else handleLine(Serial.readStringUntil('\n'));
  }

  static bool prev = HIGH;
  bool now = digitalRead(START_BUTTON_PIN);
  if (prev == HIGH && now == LOW) {
    if (state == St::RUNNING) endRun("button abort"); else startRun();
    delay(30);                               // debounce
  }
  prev = now;

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
