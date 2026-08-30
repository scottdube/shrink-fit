# Status — 2026-08-09

**Near-term job:** get 2× carbide 1/8" tools into BT30 shrink holders to unblock
PCB milling. One-way, tools stay in. Everything else is phase 2.

---

## Proven

- PSU **R48-3000e3** on 240 V — label confirmed 53.5 V / 56.1 A / 3000 W
- ZVS board oscillates and heats — test bar reached ~310 °C (blue temper band)
- PZEM validated against the AC clamp at full voltage: 297 W DC vs 336 W AC = **88% efficiency**, textbook
- DC output **floats** (no rail bonded to chassis)
- **Power is ~10% of target.** Cause: coupling. Stock 60 mm coil resonates near
  37 kHz and the work barely fills it

## On the bench

| | |
|---|---|
| Power | R48-3000e3, WMY TECH ZVS (4× **IRFP260N, 200 V**), 12× 0.33 µF/1200 V/**50 kHz** caps (marking read 2026-08-27; docs had said 150), 2 iron-powder chokes |
| Metering | PZEM-025 + 100 A shunt (mV rating unmarked), Sperry DSA-500 clamp (**AC only**) |
| Instruments | **Rigol DHO914** |
| Cooling | 2× 80 mm fans, 12 V pump, clear tubing |
| Protection | CHCYZO DZ47-63DC B63 (DC side only) |
| Coil | Stock 60 mm — **to be replaced** |

## Ordered

3/16" copper tube · fiberglass sleeving · tee fittings

## Still needed

| Item | ~$ | For |
|---|---|---|
| **K-type thermometer** | 20 | The measurement instrument, and keeping a $150 holder alive |
| Hose clamps | 5 | Silicone creeps; this is the leak-onto-live-board failure |
| Mica / ceramic paper + washer | 10 | Taper protection |
| **Verify hose fits 3/16" copper** | ? | Kit hoses were sized to the stock tube. If the tees don't adapt it, silicone tubing is needed |

**Power: settled.** 240 V 20 A receptacles every 6 ft, plus 2× 50 A per wall.
PSU (~13.6 A at 240 V) takes any 20 A drop; the MIG180 (50 A, no extension cord)
takes a 50 A. No panel work.

**SR-12 unchanged.** Outlet count says nothing about branch-circuit grouping —
several receptacles commonly share one branch, and that grouping isn't known here.
Even on separate branches, conducted EMI still couples through the shared panel
bus and service neutral. Keep the mains filter, and keep "don't run while the mill
is cutting" until measured otherwise.

---

## Do now — nothing new required

- [x] ~~Measure the stock coil tube OD~~ → **4.9 mm.** Kit silicone hose and its
      sleeving both transfer to the new 5 mm tube. No adapters needed.
- [x] ~~Label the `N` terminal~~ → done. Still to do: **re-identify the conductor**
      landing on it (black/red tape both ends) per SR-02a — the label alone doesn't
      travel with the cord.
- [ ] **Strip the fibreglass sleeving off the stock coil.** Free, right size.
      Leave the tube itself in one piece as a known-good fallback.
- [ ] **Turn the mandrel on the 15L.** Trial mandrel ~19 mm → wind two turns →
      measure springback → turn the real one to compensate.
- [ ] **Drill the test piece** — 5/8" steel bar ~50 mm, cross-drill 1.5 mm × 8 mm
      deep at mid-length. Needs the K-type thermometer to be useful.
- [x] ~~Bond DC− to chassis/earth~~ — **withdrawn.** The chassis is earthed via the
      AC cord regardless; a DC bond adds nothing to protective earthing and
      *creates* a 168 V shock path for the dominant fault (coil → holder). Design
      as floating: controller referenced to DC−, isolated CAN, no USB while
      energised. Grounding and protection sizing move to the phase-2 review.

### Controller — SUPERSEDED, see "Test rig runs untethered" above

ESP32-S3 flashed and verified 2026-08-09. **4 MB flash, no PSRAM** (GPIO 33–37
free), TWAI up at 125 kbps listen-only on GPIO 4/5, zero bus errors. The
listen-only sniffer is ready for the rectifier and cannot transmit or jam.

~~`testrig.cpp` has never been flashed~~ — **flashed and running 2026-08-27**,
with two-button control and a state lamp. See above.

~~Toolchain regressed; reinstall PlatformIO~~ — **resolved 2026-08-27.** Opening
the VS Code extension rebuilt `~/.platformio/penv` on its own, which is what the
original note guessed would happen. Core 6.1.19, builds and uploads fine.

## When the tube arrives

7. Anneal if needed, sleeve the **straight** tube, wind **~30 mm of coil length** on the mandrel — 5–6 turns at 3/16", 20 mm bore.
8. Plumb with the tees: rigid stub off each board terminal, hose teed away from the board. **Keep the two legs electrically isolated** — they're tank terminals. No aluminium in the wet path.
9. **Baseline then compare:** test piece cold, same insertion depth, run exactly 20 s, kill field, wait 200 ms, read peak. Stock coil, then new coil.
   **Expect ~7×.** Under 3× means something is wrong with the build.
10. Log a row per coil: bore, turns, tube, peak temp, PZEM W, clamp A.

## Then the real job

Stickout set first · coil on the nose only · taper shielded · heat, try, cool,
heat 25% longer, repeat · insert in one motion to the stop and hold.

---

## Deliberately deferred

**Scope work (peak Vds, frequency).** The answer would only change whether to
swap FETs or rush the CAN build — neither matters for two cycles at low power.
Probing carries real risk to the scope and board. Revisit when running real
power.

**Everything in `requirements.md`** — energy dosing, CAN control, INA228,
logging, metrology, enclosure, the 35 mm phase-2 coil. All of it is better;
none of it is required to put a tool in a holder.

## Open, and honest about it

- **Insertion clearance is an unsourced assumption.** It drives the claim that
  1/8" needs ~410 °C. Could be well under. You will learn this empirically in
  twenty minutes.
- **Are the 12 caps parallel or 6 series pairs?** Every frequency number assumes
  parallel. Doesn't matter at two cycles.
- **Shunt mV rating unmarked.** Doesn't matter until energy dosing.
- **OQ-3 — Haas holder material.** Emailed, no reply. Non-blocking.
