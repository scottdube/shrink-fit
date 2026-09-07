// config.h — pin map, limits, and the energy preset table.
// Machine: BT30 induction shrink-fit, Vertiv R48-3000e3 + WMY TECH ZVS stage.
// Board:   NodeMCU-32S (38-pin, classic ESP32) — requirements.md §6.12.
//
// PINS COME FROM docs/pinmap.md AND NOWHERE ELSE. That document carries the
// board's rules (flash pins 6–11 unusable, 34–39 input-only with no pull-ups,
// 0/2/12/15 strapping, ADC2 dead under WiFi) and the header-position table that
// prevents the wrong-pad failure. Change it there first, then here.
//
// Requirement IDs refer to ../../docs/requirements.md.
#pragma once
#include <Arduino.h>

// =============================================================================
// PINS — NodeMCU-32S
// =============================================================================

// ---- The step and the mains path (CR-05 rev. §6.10, §6.9) --------------------
// The DC SSR delivers the hard voltage step a Royer needs to commutate. The AC
// contactor is NOT the cycle switch any more: it feeds the rectifier, latches on
// its own aux contact from a hardwired MAINS START, and the E-stop drops its coil
// in hardware. Firmware has ONE path to mains — the drop relay — and no path to
// energise it. GPIO 16 can only ever open the contactor.
#define SSR_PIN            4      // via 2N3904, pattern proven on the test rig
#define MAINS_DROP_PIN     16     // controller's path to kill mains; active HIGH = drop
#define ESTOP_SENSE_PIN    36     // input-only; read-only status. The trip is hardwired.

// ---- Operator inputs ----------------------------------------------------------
// BENCH RIG DIVERGES FROM pinmap.md HERE, deliberately (Scott, 2026-09-07).
// pinmap.md puts START/STOP on 34/35 to keep the flexible pins free, which is
// right for the PCB where the pull-ups are printed on the board. On a breadboard
// those input-only pins cost an afternoon: no internal pull-up, so they float LOW
// and fake a press, they cannot be probed by grounding, and a pull-up landing on
// the switch side of the wire turns "disconnect" into "start a cycle".
//
// 26 and 27 are free by design (pinmap.md moved the lamps to the PCF8574), have
// real internal pull-ups, and need no external resistor — the same arrangement as
// the duration button, which worked on the first try.
//   GPIO 27 = Edge A position 9      GPIO 26 = Edge A position 10
// Button straight from pin to GND. Nothing else.
#define START_BUTTON_PIN   27     // CYCLE START, momentary to GND, internal pull-up
#define STOP_BUTTON_PIN    26     // CYCLE STOP — a firmware abort, NOT the E-stop
#define ENC_A_PIN          13     // rotary encoder, PCNT quadrature
#define ENC_B_PIN          19
#define ENC_SW_PIN         15     // strapping pin: must be HIGH at boot. A pulled-up
                                  // button idles HIGH, so this works — by luck. Re-check
                                  // if the switch is ever changed to active-high.

// ---- Sensors ------------------------------------------------------------------
#define FLOW_PULSE_PIN     39     // Hall flow sensor, pulses; PCNT counts them (CR-06)
#define ONEWIRE_PIN        33     // both DS18B20 coolant probes (CR-07)
// NO ON-BOARD THERMOCOUPLE. pinmap.md: "MAX31855 / thermocouple. Deleted — CR-02
// rewritten." A TC in a kW-class 90 kHz field cannot be read during a cycle
// (§8.3), and post-cycle verification is the calibration campaign's handheld
// K-type and temper colour, not a firmware channel. See TRAPS.

// ---- Buses --------------------------------------------------------------------
#define I2C_SDA_PIN        21     // INA228 (0x40) and the PCF8574 lamp expander
#define I2C_SCL_PIN        22
#define CAN_TX_PIN         GPIO_NUM_14   // TWAI. pinmap: P14 = header pin 31
#define CAN_RX_PIN         GPIO_NUM_32   // TWAI. pinmap: P32 = header pin 26 — NOT GPIO 12,
                                         // whose boot-strap a HIGH-idling RX would break
#define CAN_BITRATE_KBPS   125    // MEASURED on the rectifier 2026-09-07 (§6.14.0a)

// ---- Display and lamps --------------------------------------------------------
#define TFT_CS_PIN         5
#define TFT_DC_PIN         17
#define TFT_SCK_PIN        18     // VSPI default
#define TFT_MOSI_PIN       23     // VSPI default; MISO unused, display is write-only
// Lamps live on the PCF8574 (2N7002 gate drive — the expander cannot source enough
// for a 2N3904). Address is 0x20 for a PCF8574P, 0x38 for a PCF8574AP: READ THE
// CHIP MARKING before trusting this constant (pinmap.md, Bridgold listing
// contradicts itself). The bring-up `scan` reports whichever is actually there.
#define PCF8574_ADDR       0x20
#define LAMP_GREEN_BIT     0
#define LAMP_AMBER_BIT     1
#define LAMP_RED_BIT       2
// Interim status until the PCF8574 lamps are wired. TWO pins are driven with the
// same pattern, because the onboard LED is not guaranteed: many NodeMCU-32S
// boards carry only a power LED, and this one appears to be one of them. GPIO 2
// is a strapping pin (LOW/floating at boot); driving it after boot is fine.
// STATUS_LED2 is a DISCRETE LED you wire yourself, and is the one to trust.
//   GPIO 25 ── 330 Ω ── LED anode,  LED cathode ── GND
// GPIO 25 is free by design (pinmap.md moved the lamps to the expander), and it
// is LAMP_GREEN's pin on the finished board, so the wire has a future.
// State by BLINK PATTERN, never by colour — the shop's accessibility rule.
#define STATUS_LED_PIN     2      // onboard, may not exist on this board
#define STATUS_LED2_PIN    25     // discrete, wire it — edge A position 11

// =============================================================================
// CAN — Vertiv R48-3000e3, MEASURED 2026-09-07 (requirements §6.14.0a–e)
// =============================================================================
// Transceiver: Waveshare SN65HVD230 CAN Board — the hiBCTR modules were
// counterfeit and never drove a bus (docs/return-sn65hvd230-modules.md).
// The Waveshare carries the 120 Ω termination; the rectifier's port has NONE
// (55 kΩ measured), so ours is the bus's only terminator — correct for two nodes.
// JST harness: YELLOW = CAN H, BLUE = CAN L, NO GROUND WIRE — the port is on an
// isolated control ground and the link runs clean without one. Reversed polarity
// gives BUS-OFF, not silence.
//
// The session is a CONVERSATION: handshake, then keep-alive every 6 s. The
// rectifier forgets its setpoints after ~37 s of controller SILENCE (a session
// timeout, measured — not a setpoint timeout). That is the safety property the
// whole ONLINE-only design rests on: a hung controller cannot leave the bus at a
// commanded voltage for longer than 37 s. R48::service() must be called from the
// main loop unconditionally, and the loop must never block for 6 s.
#define CAN_KEEPALIVE_MS   6000
#define CAN_SESSION_LOST_MS 20000  // no rectifier frame for this long → telemetry offline

// =============================================================================
// INA228 on the 100 A shunt (CR-01)
// =============================================================================
// INA228 chosen over INA226: 85 V common-mode clears the 53 V bus.
// §6.16 RESOLVED 2026-09-07: the DC bus FLOATS relative to logic ground — 18 V AC
// measured between DC− and controller GND, which is the design (SR-10 closed, no
// continuity to chassis; SR-11 withdrew the earth bond). So the common mode at a
// bus-side shunt swings about ±25 V, and the INA228's −0.3 V minimum is violated
// on every mains cycle. THIS PART CANNOT SIT ON LOGIC GROUND with its inputs on
// the bus. It must be referenced to DC− with an isolated I²C, or replaced by an
// isolated sensor. Not yet decided — see §6.16.
#define INA228_ADDR        0x40
// The shunt case carries NO mV marking. 0.75 mΩ assumes the common 100 A / 75 mV
// rating — a PLACEHOLDER until four-wire injection (VR-11) measures it. The dose
// is only as good as this number. See the INA228 calibration item in status.md.
#define SHUNT_OHMS         0.00075f   // PLACEHOLDER — see VR-11
#define I_MAX_EXPECTED_A   80.0f      // headroom over the ~68 A the rectifier delivers
#define SHUNT_CALIBRATED   0          // set 1 when VR-11 has been done; gates DOSED mode

// =============================================================================
// WHAT IS ACTUALLY FITTED — declare it, do not fake it
// =============================================================================
// As of 2026-09-07 the machine is the S3 test rig's topology: the R48 feeds the
// ZVS directly and the SSR is the only actuator. There is no AC contactor and no
// E-stop chain (§6.9 designs them; they are not built). Firmware must not pretend
// otherwise — a pull-up resistor that makes GPIO 36 read "E-stop closed" is a lie
// that is invisible a month later, whereas a flag announces itself at every boot.
//
// Set each to 1 as the hardware lands, and the checks come back automatically.
#ifndef ESTOP_FITTED
#define ESTOP_FITTED       0      // 0 ⇒ estopClosed() returns true, warns loudly
#endif
#ifndef CONTACTOR_FITTED
#define CONTACTOR_FITTED   0      // 0 ⇒ mainsDrop() cannot drop anything, warns loudly
#endif
#if !ESTOP_FITTED || !CONTACTOR_FITTED
// A build with either of these at 0 has NO independent means of removing power.
// The SSR opening is the whole protection story — same as the test rig, which is
// how every cycle to date has been run, but it must be a stated posture.
#endif

// =============================================================================
// CYCLE MODES
// =============================================================================
// TIMED: the test rig's proven behaviour — SSR on for a fixed duration from the
//        ladder, no dose. This is what every cycle to date has been.
// DOSED: CR-01 — terminate on accumulated energy from the INA228. Requires a
//        calibrated shunt (SHUNT_CALIBRATED) or it is refused at PRECHECK.
static const uint32_t DUR_LADDER_S[] = {5, 10, 15, 20, 25, 30};   // seconds, per press
#define DUR_LADDER_COUNT   (sizeof(DUR_LADDER_S) / sizeof(DUR_LADDER_S[0]))

// =============================================================================
// LIMITS
// =============================================================================
#ifndef BENCH_MODE
#define BENCH_MODE 0
#endif
// ---- Arming (CR-05 rev. §6.10): all three, none sufficient alone -------------
#define ARM_MIN_MS         30000  // ≥30 s since mains came up (contactor close)
#define ARM_BUS_V_MIN      40.0f  // bus voltage over CAN must be at least this
#define ARM_SAMPLES        3      // held across this many consecutive telemetry samples
#define ARM_TIMEOUT_MS     90000  // not armed by then → FAULT. A dead rectifier and a
                                  // slow one are identical to a clock.

// ---- Cycle ---------------------------------------------------------------------
#define START_TEMP_MAX_C   60.0f  // CR-04 — enforced by the operator until a channel exists
#define COOLANT_TEMP_MAX_C 50.0f  // CR-07 — DS18B20
// CR-04 stand-in: with no nose thermocouple, a minimum dwell is the only thing
// stopping a warm holder being re-heated, which is the failure that permanently
// enlarges bores. PLACEHOLDER until CR-02/CR-04 are rewritten for the
// no-thermocouple design.
//
// BENCH_MODE shortens it to 3 s so bench testing is not spent waiting — it ate
// three button presses during the first button test. The real 60 s is untouched
// in env:controller, and the shortened value is announced at boot like every
// other bench compromise, never silent.
#if BENCH_MODE
#define COOLDOWN_MIN_MS    3000
#else
#define COOLDOWN_MIN_MS    60000
#endif
#define PEAK_FLAG_C        350.0f // CR-02 thresholds — applied to the CAMPAIGN's handheld
#define PEAK_LOCKOUT_C     375.0f // readings, not to a firmware channel

// ---- Bus voltage ---------------------------------------------------------------
// MOSFETs CONFIRMED: IRFP260N, 200 V / 50 A / 40 mΩ. 53 V is out of spec.
//   41.5 V → peak Vds ≈ 130 V (35% margin) — commissioning
//   48.0 V → peak Vds ≈ 151 V (25% margin) — OPERATING CEILING
//   53.0 V → peak Vds ≈ 167 V (17%)        — DO NOT USE
// π·Vdc is the idealised Royer peak; ringing adds 10–20%. Scope Vds (VR-05)
// before raising past 41.5 V and never above 48 V regardless of what it shows.
// The R48 slews setpoints at ~2.5 V/s (walk-in, measured) — a step command is a
// ~3 s ramp at the output, and the SSR stays open throughout so the ZVS never
// sees it.
#define BUS_SETPOINT_V     41.5f
#define BUS_VOLTAGE_CEIL   48.0f  // hard clamp — device limit, not a preference.
                                  // Enforced AGAIN inside R48::setVoltage(); this is
                                  // the value that one reads.
#define BUS_VOLTAGE_MIN    41.0f  // rectifier's ONLINE floor (§6.15)
// Current limit: the rectifier's own CC loop, a protection layer BELOW firmware.
// MEASURED 2026-09-07: the ONLINE limit is a fraction of a 56 A base (not the
// datasheet's 62.5), linear to ~3 A over 45–62 A; the CC loop is steady at a
// 17 V bus (60 A) and HUNTS at 9 V (45 A) into this coil. So it is a usable knob
// from ~50 A up to the ~68 A default and nothing below — fine power control
// below that comes from duty, not from this. (§6.14.0d)
#define BUS_CURRENT_LIM_A  55.0f
#define R48_I_BASE_A       56.0f  // 100 % of rated = rated power at nominal volts
static_assert(BUS_SETPOINT_V <= BUS_VOLTAGE_CEIL, "bus setpoint exceeds IRFP260N-derived ceiling");
static_assert(BUS_SETPOINT_V >= BUS_VOLTAGE_MIN,  "bus setpoint below the rectifier's ONLINE floor");

// ---- Sensor plausibility (CR-03, review Tier 1 #2) ----------------------------
// The dangerous failure is the sense chain reading LOW while real power is high.
// Hence an ABSOLUTE per-size ceiling plus a live sanity window.
#define PLAUS_V_MIN        8.0f   // the bus folds to 15–20 V in current limit (measured)
#define PLAUS_V_MAX        60.0f
#define PLAUS_I_MIN_A      5.0f   // below this mid-cycle ⇒ lost coupling or dead sense
#define PLAUS_I_MAX_A      75.0f  // rectifier delivers ~68 A at its default limit
#define PLAUS_GRACE_MS     1000   // ignore during the ~1 s startup transient
#define CAN_CROSS_TOL      0.20f  // rectifier telemetry vs INA228 max fractional disagreement
                                  // — but the rectifier's current filter LAGS 2–3 s on
                                  // the fall (measured), so this check must not run in
                                  // the first seconds after any change.

// =============================================================================
// OTA / console
// =============================================================================
// Credentials live in NVS, set once over serial (`ssid`, `pass`). WiFi is skipped
// if none are stored. OTA is serviced ONLY while IDLE. NO OTA PASSWORD — Scott's
// decision 2026-08-27 for a bench device on the shop LAN; revisit when this goes
// in the cabinet. The Mac must be ON THE IOT NETWORK to OTA: a failed espota
// invitation freezes the board's loop until it is power-cycled (TRAPS).
#define OTA_HOSTNAME       "shrinkfit-ctl"
#define TELNET_PORT        23

// =============================================================================
// Preset table (CR-08)
// =============================================================================
// Energies are §4.3 PREDICTIONS, not calibration. VR-07 replaces every one of
// them with measured values from the descending campaign (§8.1). Do not run
// production cycles on these numbers. Every entry stays provisional until the
// shunt is calibrated AND the campaign has measured it.
struct Preset {
  const char* name;
  float    energyJ;      // dose target (DOSED mode)
  uint32_t ceilingMs;    // ABSOLUTE abort, not a multiple of expected (CR-03)
  bool     provisional;  // true ⇒ predicted, never measured
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
