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

### Controller — bringup done 2026-08-09, test rig NOT yet flashed

ESP32-S3 Super Mini flashed and verified. **4 MB flash, no PSRAM** (so GPIO 33–37
are free), TWAI running at 125 kbps listen-only on GPIO 4/5, zero bus errors.
Toolchain fixed — the stale Python 3.9 PlatformIO is gone and `pio` now resolves
to the 3.11 venv. The listen-only sniffer is ready for the rectifier the moment
the SN65HVD230 lands; it cannot transmit or jam the bus.

`testrig.cpp` — the timed-cycle rig that actually runs the coil A/B — is written
and has compiled clean, but **has never been flashed**. It is waiting on the
Phase 1 wiring, not on code.

**Toolchain regressed 2026-08-25.** `~/.platformio/penv` and `.pio/build` are
both gone; `pio` no longer resolves and the `~/.zshrc` line added on 08-09 now
points at nothing. Only `~/.platformio/python3` survives, which is the newer
Core layout — so this reads as a PlatformIO reinstall, not a deletion. Reinstall
Core before the next flash and re-point the PATH line.

## Controller PCB — schematic done, layout blocked

KiCad 10 project at `hardware/shrink-fit-controller/`. Scope decided 2026-08-25:
**full Phase 2 controller, populated in stages** — every block gets a footprint
and header, because a respin costs re-soldering everything already fitted while
copper is an afternoon.

**Done:**
- Schematic drawn, **ERC 0/0**, and every net verified against an expected
  (net → U1 pad) table — not just "ERC passed". Divider and driver chains checked
  by full membership.
- Super Mini symbol, pinout confirmed by **three independent sources in three
  orientations** (both faces photographed + a dimensioned drawing).
- Footprint adopted from a found file after fixing three defects — see
  [`TRAPS.md`](TRAPS.md), one of which would have been silent.
- Power resolved: the kit's own 12 V 3 A wall wart (#1102) + an LM2596 from
  stock. Class II, no earth conductor, so no back-door SR-11 bond.

**Blocking copper:**
1. **Row spacing is NOT verified.** 15.24 mm came from the found footprint, not
   from measurement. Test: press the module into a breadboard — 0.600 in
   straddles the centre channel, 0.700 in does not. **Do this before milling.**
2. **Mounting pattern undecided** because the enclosure is. Holes are free while
   milling and painful to retrofit beside a pour.

**Note:** the schematic was generated by `tools/gen_sch.py`, but KiCad has since
taken ownership of the file. **Do not re-run the generator** — it would destroy
GUI edits.

## FIRST CONTROL — 2026-08-27

**The controller actuated something for the first time.** `testrig.cpp` flashed
(it had compiled clean since 08-15 but was never loaded), `r 5` commanded, GPIO10
high for exactly **5000 ms**, and **the SSR's control LED lit for the duration**.

That proves the whole Phase 1 control chain: **GPIO10 → R1 (1 k) → Q1 (2N3904)
→ SSR-100DD control input.** Nothing between the firmware and the relay is
unverified any more.

Bench arrangement, and it matters: **ESP32 on USB alone, LM2596 output left
disconnected.** On the Super Mini the USB 5 V and the `5V` pin are the same net,
so feeding both fights two supplies onto one node — Scott caught that before
wiring it. The 12 V bench supply feeds only the SSR control side, with its
negative tied to the breadboard ground rail so Q1 has something to sink to.

**GPIO1 jumpered to ground.** With the bus divider not fitted the ADC floats and
reported a plausible-looking **10.3 V** of noise; grounded, it reads 0.0. A
meaningless column is safe, a convincingly wrong one is not.

### SSR output — PROVEN, same session

Bench supply 12 V through 1 k through the SSR output, meter across the SSR:
**12 V idle → near zero for the full commanded 10 s → back to 12 V.** The output
transistor conducts and releases on command.

**Phase 1's control circuit is therefore complete and measured end to end:**
GPIO10 → R1 → Q1 → SSR control → SSR output. Nothing in that chain is inferred.

Two things that briefly looked like faults and were not:

**A run that "only lasted a second" was my test script**, which wrote a newline
to the port to "just listen". The firmware treats any incoming byte as an abort —
correct behaviour, working as written. Do not send anything to the port during a
run unless you mean to stop it.

**Measuring across the SSR reads inverted from measuring the load.** 12 V across
the SSR means it is *blocking*, not passing. For a more intuitive watch, put the
meter across the 1 k instead: 0 V idle, 12 V while conducting.

Bench supply current limit was 500 mA against a ~35 mA draw, so that was never a
factor.

## Bench measurements — 2026-08-27

Taken with the board in hand, superseding assumptions. Ordered as measured.

| What | Measured | Supersedes |
|---|---|---|
| Super Mini row-to-row hole centres | **15.2 mm** | confirms 15.24 (0.600 in). The 17.78 prediction was wrong |
| ZVS board standoff height | **26.9 mm** | no prior figure |
| Coil terminal centre-to-centre | **70 mm** | no prior figure |
| Terminal centreline off the board | **13 mm** | no prior figure |
| Coil lead exit direction | **out of the board's short end, in the board's plane** | closes the coil-below vs coil-beside question |
| SSR output rating | **100 A / 220 VDC** (CoG ZGT-100 DD) | feared 5–60 VDC |
| 12 V supply | **model 1230, 12 V 3 A, Class II, centre-positive** | inferred from plug shape |

**Two requirements deleted by these.** The Super Mini footprint is closed — the
found file's 15.24 mm was right, and 17.78 was geometrically impossible on an
18.00 mm board anyway (hole centres 0.11 mm from the edge). And **26.9 mm of
standoff means the carriage needs only a plain flat plate** — the cut-away or
insulated mounting plate previously specified is unnecessary, since nothing can
reach the live underside across that gap.

### Fans are 120 mm, not 80 — corrected 2026-08-27

Two stacked touching, **push-pull, both on the outer face**, and they set the
board's length: 240 mm of fan alone. The roadmap's `200 × 150 × 120` envelope for
this block was impossible and now reads **~250 × 140 × 150**.

**Costs ~45 mm of column** — upright goes to ~565 mm, or ~525 mm with the nest
flush.

**But it makes the rail easier.** A longer board lets the two carriage blocks sit
further apart: 180 mm centres instead of 140 cuts the per-block couple by 22 %
for nothing. A 450 mm rail still covers it either way (390 mm needed at 180 mm
centres).

**Flip the fans.** As found, the coil-end fan exhausts *toward* the coil. Reversed,
exhaust goes up — working with buoyancy, and agreeing with the enclosure's
"intake low, exhaust high". Flip both to preserve push-pull.

**The bolt pattern is the fan pattern**, which is why it needed no measuring:
two 120 mm frames touching give X columns 105 mm apart and Y rows at
7.5 / 112.5 / 127.5 / 232.5. The 15 mm gap between the inner rows is where the
frames meet.

### What the terminal geometry settles

**The coil hangs below the board**, not beside it. Mounted long-edge vertical with
the terminal end down, the leads run straight out of that end. The 65 mm of column
height that "coil beside the board" would have saved is **not available** — the
upright stays at ~520 mm above the base, or ~480 mm with the nest dropped flush.

**The stubs live inside the standoff gap.** 13 mm off the board against 26.9 mm of
standoff leaves ~11.5 mm under them, so nothing projects past the mounting plane
and the carriage plate stays plain and flat.

**70 mm apart is generous for the bushings** but imposes a coil-winding spec that
did not exist before: the legs must converge from **70 mm at the terminals to
~25 mm at the winding**. Confirmed there is ample leg length on the wound coil to
do it. Lead inductance from the convergence is ~0.035 µH against a ~0.55 µH coil —
about 6 %, a ~3 % frequency shift — so MR-25's "leads as short as practical" is
not violated by the geometry.

**The factory plumbing is what the bulkhead replaces.** As shipped, silicone
pushes straight onto the board terminals and runs back across the board's face —
water over the electronics. The dry-stub arrangement exists to delete exactly
that. Worth noting because photographs of the board as delivered show the
arrangement being designed *out*, not the target.

Usefully, the water ports face **opposite** the coil exit — up and back over the
board. The board's own geometry therefore wants coolant routed overhead, which is
what the drag chain does anyway.

**Standoff isolation — CHECKED 2026-08-27: all eight open to both DC+ and DC−.**
Nothing carries tank potential to the carriage. The plate can be plain metal
bolted straight to the rail blocks; no insulating hardware, no cut-away. That is
the fourth independent confirmation that a flat plate is sufficient.

**What it does not clear is RF.** DC isolation is not 100 kHz isolation. The
carriage plate, rail and column form a large conductive structure bonded to
chassis and sitting inches from the tank, so it is capacitively coupled to it
whatever the meter says. Consequence for layout: **do not run sensor leads along
the carriage or the rail.** That is R-11's failure mode — the same physics that
makes a thermocouple unreadable inside the field (§8.3) attacks any unshielded
sense lead. The column raceway on the far side already exists for this.

**TODO when KiCad releases the project lock:** `hardware/shrink-fit-controller/README.md`
still describes the row spacing as settled by a found file rather than measured.
Not editable while `~*.lck` is present.

## SSR — identified 2026-08-27

**CoG ZGT-100 DD** (XIQI): output **100 A / 220 VDC**, control 3–32 VDC, load on
the **positive** side. 4× voltage headroom on a 53.5 V bus.

Combined with the six FR307 across the board's DC rails, this **closes MR-22
entirely** — the freewheel diode is fitted from the factory and the onboard array
clamps the far end of the SSR cable too. Nothing to buy, nothing to fit; the five
MBR60100PT go back to being spares.

Still true and unchanged: **an SSR fails short**, so it can never be the isolating
element. MR-23's series mechanical contactor stands.

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
