// energy.cpp — INA228 shunt front end.
// VERIFY on bring-up: the shunt's actual mV rating (SHUNT_OHMS assumes 100 A /
// 75 mV) and that ADCRANGE stays at 0 — the ±40.96 mV range would clip at 56 A.
#include "energy.h"
#include <Adafruit_INA228.h>

static Adafruit_INA228 s_ina;

bool EnergyMeter::begin(uint8_t addr, float shuntOhms, float maxCurrentA) {
  if (!s_ina.begin(addr)) return false;
  s_ina.setShunt(shuntOhms, maxCurrentA);

  // Short conversion times, minimal averaging: the dose is accumulated in
  // hardware regardless, and the plausibility window wants to react fast.
  s_ina.setAveragingCount(INA228_COUNT_4);
  s_ina.setVoltageConversionTime(INA228_TIME_150_us);
  s_ina.setCurrentConversionTime(INA228_TIME_150_us);

  _up = true;
  return true;
}

void EnergyMeter::resetDose() {
  if (_up) s_ina.resetAccumulators();
}

EnergyMeter::Sample EnergyMeter::read() {
  Sample s;
  if (!_up) return s;
  s.busV    = s_ina.readBusVoltage() / 1000.0f;   // mV → V
  s.currA   = s_ina.readCurrent()    / 1000.0f;   // mA → A
  s.energyJ = s_ina.readEnergy();                 // J
  s.valid   = true;
  return s;
}
