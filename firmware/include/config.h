// config.h — pin map, limits, and the energy preset table.
// Machine: BT30 induction shrink-fit, R48-3000e3 + WMY TECH ZVS stage.
// Requirement IDs refer to ../../docs/requirements.md.
#pragma once
#include <Arduino.h>

// ---- CAN to the R48-3000e3 rectifier (CR-11) --------------------------------
// Board: ESP32-S3 Super Mini (HW-747). Uses the S3's built-in **TWAI** peripheral
// — no MCP2515. This sidesteps the 3.3 V MCP2515 trouble the R48 community
// reported, and drops the SPI wiring and level shifting entirely.
//
// Transceiver: SN65HVD230 breakout, 3.3 V native.
//
// VERIFY ON ARRIVAL — the following is read from a stock listing photo, not the
// shipped board:
//   R1 expected 103 (10 kΩ) → Rs slope control. Keep it if present: slower edges,
//     less RF pickup near the tank, no downside at 125 kbps. Rs tied straight to
//     GND instead = high-speed mode, also fine.
//   R2 expected 121 (120 Ω) → bus termination. Measure CANH–CANL unpowered; ~120 Ω
//     confirms it. This must be the bus's ONLY termination — the rectifier measured
//     55 kΩ (unterminated). Use exactly one module; two would put 60 Ω across it.
//   IC marking — confirm it is actually an SN65HVD230. Cheap modules sometimes ship
//     a substituted TJA1050 or MCP2551, both 5 V parts that misbehave on 3.3 V.
//
// TWAI routes through the GPIO matrix, so these pins are free choices.
#define CAN_TX_PIN         GPIO_NUM_4
#define CAN_RX_PIN         GPIO_NUM_5
#define CAN_BITRATE_KBPS   125     // VERIFY: measure the narrowest bit time on the
                                   // scope (1/t_bit) before trusting the libraries
#define CAN_POLL_MS        500

// CAN idle is RECESSIVE = TX high. If CAN_TX_PIN sits low before the TWAI driver
// installs, the transceiver drives dominant and jams the bus. First thing to check
// if the bus looks dead at power-up.

// ---- INA228 on the 100 A shunt (CR-01) --------------------------------------
// INA228 chosen over INA226: 85 V common-mode limit clears the 53.5 V bus.
// SR-11 bonds DC− to earth, so low-side sensing sits near zero common mode.
#define I2C_SDA_PIN        8
#define I2C_SCL_PIN        9
#define INA228_ADDR        0x40
// Super Mini header pins are 1–13 + TX/RX; inner pads carry 14–21 and 33–48.
// MEASURED 2026-08-09 on this board: ESP32-S3 rev 2, 4 MB flash, **no PSRAM** —
// so GPIO 33–37 are electrically free (they'd be consumed by octal PSRAM). They
// still need soldering to inner pads. Remaining reservations: 19/20 are native
// USB, and 0/3/45/46 are strapping pins.
// The shunt case carries NO mV marking. This 0.75 mΩ default assumes the common
// 100 A / 75 mV rating — it is a PLACEHOLDER. Determine it by four-wire injection
// per VR-11 (10 A through it, measure the sense terminals) before trusting a dose.
// VR-07 folds a residual error into the measured joule targets provided the value
// is STABLE, so repeatability matters more than absolute accuracy — but a badly
// wrong value makes CR-03's 5–70 A plausibility window meaningless.
#define SHUNT_OHMS         0.00075f   // PLACEHOLDER — see VR-11
#define I_MAX_EXPECTED_A   80.0f      // headroom over the 56.1 A rectifier ceiling
// CURRENT_LSB = I_MAX_EXPECTED / 2^19          = 1.526e-4 A/bit
// SHUNT_CAL   = 13107.2e6 * CURRENT_LSB * R    ≈ 1500   (ADCRANGE=0)
// ENERGY_LSB  = 16 * 3.2 * CURRENT_LSB         ≈ 7.81 mJ/bit
// 7.81 mJ on a 5.7 kJ dose is ~0.00014% — resolution is not the limiting factor.

// ---- Thermocouple (CR-02, TR-01..05) ----------------------------------------
// NEVER read during the cycle. Induced voltage, junction self-heating, and ramp
// lag each independently invalidate it — see requirements §8.3.
#define MAX31855_SCK_PIN   12
#define MAX31855_MISO_PIN  13
#define MAX31855_CS_PIN    11
#define TC_SETTLE_MS       200     // TR-04: gate sampling to ≥200 ms after field-off

// ---- Bus voltage sense (resistor divider) -----------------------------------
// 220 k (top, to DC+) / 10 k (bottom, to DC−) = 23:1. 53.5 V → 2.33 V and 48 V →
// 2.09 V, both clear of the 3.3 V rail; draws ~0.24 mA. Put 0.1 µF across the
// bottom leg — you are sensing next to a kW oscillator (R-11) and that filter is
// not optional.
//
// 220 k, not the 180 k originally specified here: 180 k is not stocked. The EAONE
// kit steps 100 k → 220 k → 300 k, and nothing else on hand reaches 19:1 in two
// parts (checked against inventory 2026-08-25). Rejected 100 k + 68 k + 10 k in
// series = 18.8:1, which hits the number but spends three parts and two extra
// joints to save a calibration step the `cal` command performs anyway. Source
// impedance is 9.6 kΩ; the 0.1 µF supplies the ADC sampling charge, so the
// higher impedance costs nothing.
// GPIO 1 is ADC1_CH0 on the S3; ADC1 avoids the WiFi conflict that afflicts ADC2.
#define BUS_SENSE_PIN      1
#define BUS_DIV_RATIO      23.0f   // CALIBRATE — see the testrig `cal` command

// ---- Contactor and interlocks ------------------------------------------------
// CR-05: the contactor delivers the voltage STEP. CAN sets voltage; it must never
// be used to enable the output — a ramp is exactly what causes ZVS shoot-through.
// SR-03: E-stop is HARDWARE in series with the contactor coil. This pin cannot be
// the E-stop and must never be presented as one.
#define CONTACTOR_PIN      10
#define FLOW_SWITCH_PIN    6      // CR-06, active high = flow proven
#define ESTOP_SENSE_PIN    7      // read-only status; the trip itself is hardwired
#define START_BUTTON_PIN   2      // not GPIO 3 — strapping pin (JTAG source select)
// Free for the Stage-4 encoder: GPIO 1, 3

// ---- Cycle limits ------------------------------------------------------------
#define STARTUP_BLOCK_MS   30000  // CR-05: block DC to the ZVS until the PSU settles
#define START_TEMP_MAX_C   60.0f  // CR-04: refuse to start on a warm holder
#define COOLANT_TEMP_MAX_C 50.0f  // CR-07
#define PEAK_FLAG_C        350.0f // CR-02: flag + require acknowledgement
#define PEAK_LOCKOUT_C     375.0f // CR-02: lock the preset until recalibrated

// ---- Bus voltage -------------------------------------------------------------
// MOSFETs CONFIRMED: IRFP260N, 200 V / 50 A / 40 mΩ. 53.5 V is out of spec.
//   41.5 V → peak Vds ≈ 130 V (34.8% margin) — commissioning
//   48.0 V → peak Vds ≈ 151 V (24.5% margin) — OPERATING CEILING
//   53.5 V → peak Vds ≈ 168 V (16%)          — DO NOT USE
//
// π·Vdc is the IDEALISED Royer peak. Ringing routinely adds 10–20%, which at
// 48 V means 165–180 V on a 200 V part. Scope Vds (VR-05) before raising this
// past 41.5 V, and never above 48 V regardless of what the scope says.
#define BUS_SETPOINT_V     41.5f
#define BUS_VOLTAGE_CEIL   48.0f  // hard clamp — device limit, not a preference
#define BUS_CURRENT_LIM_A  50.0f  // rectifier-level limit, independent of firmware
static_assert(BUS_SETPOINT_V <= BUS_VOLTAGE_CEIL, "bus setpoint exceeds IRFP260N-derived ceiling");

// ---- Sensor plausibility (review Tier 1 #2) ---------------------------------
// The dangerous failure is the sense chain reading LOW while real power is high:
// dose accumulates slowly, heating continues, and a ratio-based timeout arrives
// far too late. Hence an ABSOLUTE per-size ceiling plus a live sanity window.
#define PLAUS_V_MIN        35.0f
#define PLAUS_V_MAX        60.0f
#define PLAUS_I_MIN_A      5.0f   // below this mid-cycle ⇒ lost coupling or dead sense
#define PLAUS_I_MAX_A      70.0f
#define PLAUS_GRACE_MS     500    // ignore the window during the startup transient
#define CAN_CROSS_TOL      0.20f  // rectifier telemetry vs INA228 max fractional disagreement

// ---- Preset table (CR-08) ----------------------------------------------------
// Energies are §4.3 PREDICTIONS, not calibration. VR-07 replaces every one of
// them with measured values from the descending campaign (§8.1). Do not run
// production cycles on these numbers.
struct Preset {
  const char* name;
  float  energyJ;        // dose target
  uint32_t ceilingMs;    // ABSOLUTE abort, not a multiple of expected
  bool   provisional;    // true ⇒ predicted, never measured
};

static const Preset PRESETS[] = {
  // name      energyJ  ceilingMs  provisional
  { "1/8\"",     5700,     25000,  true },   // Risk R-1: needs ~410 °C, marginal
  { "3/16\"",    7200,     25000,  true },   // Risk R-1
  { "1/4\"",     9200,     25000,  true },
  { "5/16\"",   11700,     26000,  true },
  { "3/8\"",    14500,     27000,  true },
  { "1/2\"",    21800,     30000,  true },   // start the campaign here
};
static const uint8_t PRESET_COUNT = sizeof(PRESETS)/sizeof(PRESETS[0]);
