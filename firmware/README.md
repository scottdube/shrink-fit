# shrink-fit firmware

ESP32-S3 controller for the BT30 induction shrink-fit machine. Requirements and
rationale live in [`../docs/requirements.md`](../docs/requirements.md); every
requirement ID in the source refers to that document.

**Status:** skeleton. The state machine, dosing path, and interlocks are written;
the R48 CAN frame format is unported and the HMI is stubbed.

## Control law

Cycles terminate on **accumulated energy**, never on elapsed time. Time is only
an absolute abort ceiling. This is what "process-controlled" means on a
commercial machine, and it is the single largest quality gap this build closes —
it makes the process immune to line sag, rectifier foldback, and coil resistance
drift.

```
BOOT_BLOCK ──30s──▶ IDLE ──start──▶ PRECHECK ──▶ HEATING ──dose met──▶ SETTLE
                     ▲                                                   │
                     └──── COOLDOWN ◀── MEASURE ◀────200 ms──────────────┘
                                  any check fails ──▶ FAULT (latched)
```

## What this firmware is not

1. **Not the E-stop.** SR-03 puts it in series with the contactor coil, in
   hardware. `estopClosed()` is status only.
2. **Not a temperature controller.** A thermocouple inside a kW-class 90 kHz
   field is defeated by induced voltage, junction self-heating, and ramp lag —
   independently, and decisively (§8.3). Temperature is post-cycle verification
   and a *next-cycle* interlock.
3. **Not the output enable.** CAN sets the rectifier's voltage; the contactor
   delivers the step. Enabling over CAN produces the slow ramp that drives both
   ZVS MOSFETs into conduction.

## Layout

| Path | Contents |
|---|---|
| `include/config.h` | Pin map, limits, commissioning bus voltage, preset table |
| `src/r48_can.*` | Rectifier supervisory link — voltage, current limit, telemetry |
| `src/energy.*` | INA228 on the 100 A shunt; hardware energy accumulation |
| `src/main.cpp` | Cycle state machine, interlocks, plausibility, CR-10 log |

## Bring-up order

1. **Set `BUS_SETPOINT_V` to 41.5 V** and leave it there. Peak Vds ≈ 130 V, 35%
   margin on 200 V parts. Walk it up only after VR-05 confirms the actual device
   rating.
2. Port the CAN frames from
   [anikrooz/Emerson-Vertiv-R48](https://github.com/anikrooz/Emerson-Vertiv-R48).
   Do not guess them — the rectifier retains the last set current across power
   cycles, so a malformed frame latches a setpoint you did not intend.
3. Confirm the shunt's mV rating and fix `SHUNT_OHMS`. The 0.75 mΩ default
   assumes 100 A / 75 mV.
4. Verify `CAN_BITRATE` against the rectifier.
5. Run VR-01 through VR-06 on an instrumented dummy before any holder enters the
   coil.

## The preset table is not calibration

Every energy in `PRESETS[]` is a §4.3 *prediction* and carries
`provisional = true`. VR-07's descending campaign (1/2" first, 1/8" last)
replaces all six with measured values. Production cycles must not run on
predicted doses.

`provisional` is logged with every cycle so the record shows which numbers were
measured and which were guessed.

## The log is the metrology

`logCycle()` output is not diagnostics — it is the machine's bore-wear
instrument. Release energy tracks remaining interference at roughly **3–4% per
micron** of bore growth, which beats any bore-measurement method that fits the
budget (§8.2). Two consequences:

- Every cycle must be logged, including aborts. A missing cycle is a permanent
  hole in the trend.
- CR-04 (start-temperature gate) and MR-17 (nest repeatability) are *metrology*
  requirements, not just safety ones — they control the confounders that would
  otherwise swamp the signal.

Serial JSON for now; persist to LittleFS or SD next. Telemetry may publish to
Home Assistant, but **no control path over the network** (OQ-5).
