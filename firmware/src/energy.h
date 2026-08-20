// energy.h — INA228 on the 100 A low-side shunt. Primary dose sensor (CR-01).
//
// Why hardware accumulation: the INA228 integrates power in its own ENERGY
// register at its conversion rate, so the dose is immune to loop jitter, missed
// ticks, and scheduling delay. Software integration at 1 kHz is kept alongside
// it — not as a second opinion (same silicon, not independent) but to feed the
// live plausibility window. The genuinely independent channel is the
// rectifier's CAN telemetry (CR-11).
#pragma once
#include <Arduino.h>

class EnergyMeter {
public:
  struct Sample {
    float busV   = NAN;
    float currA  = NAN;
    float energyJ = 0.0f;   // hardware accumulator since last reset
    bool  valid  = false;
  };

  bool begin(uint8_t addr, float shuntOhms, float maxCurrentA);

  // Zero the hardware accumulator. Call immediately before closing the
  // contactor, never during a cycle.
  void resetDose();

  // Call at ≥100 Hz during HEATING (CR-08). 1 kHz is comfortable on I2C at
  // 400 kHz and leaves the window responsive.
  Sample read();

private:
  bool _up = false;
};
