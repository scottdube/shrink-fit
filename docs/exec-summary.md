# DIY Induction Shrink-Fit Machine — Executive Summary

**Scope:** Benchtop machine to clamp/release carbide tools in Haas BT30 shrink-fit
holders, 1/8"–1/2", for the Tormach 1100MX.
**Date:** 2026-08-09 · **Status:** Requirements draft complete, kit on order.
**Detail:** [`requirements.md`](requirements.md) — 55 requirements, gap analysis, test plan.

---

## Bottom line

**Viable.** A ~$400 build around the ordered 3000 W ZVS kit will do the job at
**14–17 second cycles** against a commercial machine's 5 seconds. The cycle-time
gap is real but not disqualifying at one tool change at a time. Two open
decisions remain (below); neither blocks starting the build.

## What determines success

**1. Two changes are both mandatory, and neither works alone.**
Feed the power supply **240 V** (already in the shop for the mill) *and* rewind
the stock 60 mm work coil to **35 mm ID**. On 110 V with the stock coil the 1/2"
cycle runs ~95 s — long enough that heat soaks the entire holder and the tool
never releases cleanly. With both changes it lands at 17 s. Either one alone
leaves it marginal. This is the central engineering finding.

**2. One coil covers the whole size range — the physics self-compensates.**
Small holders need ~4× less energy *and* couple ~3× worse into a fixed coil. The
effects nearly cancel, giving flat 14–17 s cycles from 1/8" to 1/2" on a single
coil geometry. This removes the matched coil/disc sets commercial machines need
and materially simplifies the machine. Counter-intuitively, the poor small-holder
coupling is *useful* and should not be "improved."

**3. A $5 firmware change closes the largest quality gap.**
Commercial machines are process-controlled; the kit ships with a hand on a
switch. Terminating each cycle on **accumulated energy (∫V·I·dt)** rather than
elapsed time makes the process immune to line sag, supply foldback, and coil
drift. The kit already includes the shunt. This is the single highest
value-per-dollar item in the project.

It also delivers free metrology: release energy tracks remaining interference at
~3–4% per micron of bore growth, which is more sensitive than any affordable
bore-measurement method. **The machine measures its own holder wear.**

**4. Temperature cannot be measured during the cycle.**
A thermocouple inside a kW-class 90 kHz field is defeated three ways at once —
induced voltage swamping a 15 mV signal, inductive self-heating of the junction,
and ~23 K of ramp lag per second of time constant. Temperature is therefore
post-cycle verification and a next-cycle lockout, not a live safety cutoff. All
real-time protection rests on energy dosing plus an absolute time ceiling, which
raises the bar on sense-chain integrity.

## Position vs. commercial

| | Haimer Power Clamp | This build |
|---|---|---|
| Cost | $5,000–15,000 | Kit + ~$400 |
| Heat cycle | ~5 s | 14–17 s |
| Control | Process-controlled | Energy-dosed (equivalent in kind) |
| Coil sets | 1 coil + 5 stop discs | 1 coil, 6 software presets |
| Throughput | Tool-room duty | 1 change / ~2 min — sufficient |

The meaningful gap is power, and it is accepted deliberately.

## Open decisions

1. **Temperature ceiling, and whether 1/8" stays in scope.** Analysis suggests
   the 1/8" holder may need ~410 °C — above the 350 °C working ceiling. That
   conclusion rests on one unsourced assumption (insertion clearance), and the
   planned calibration campaign measures it directly. *Recommendation: keep 1/8"
   in scope as provisional, set the ceiling at 375 °C with documented rationale,
   decide from data.*
2. **Controller platform** — local ESP32 firmware vs. ESPHome. Energy
   integration and the abort path argue for local firmware.

## Next steps

1. Verify kit contents against the receiving checklist on arrival.
2. Confirm 240 V at the bench location.
3. Build and commission the electrical stage on an instrumented dummy holder —
   no real holder in the coil until the sense chain is proven.
4. Run the descending calibration campaign, 1/2" downward, with pre-defined stop
   rules per size.
5. Ask Haas Tooling for the holder material — it sets the temperature ceiling.
