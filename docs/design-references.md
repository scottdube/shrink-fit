# Design references

Builds studied for layout and detail ideas. Recorded so decisions can be traced
to where they came from, and so divergences are deliberate rather than accidental.

---

## R1 — John Robert Loxley Wood, "DYI Heat Shrink Fit Machine"

[YouTube](https://www.youtube.com/watch?v=4Rs_m3mM2Kg) · 7:02 · SolidWorks design
walkthrough + finished build. Listed in the source briefing §3.2 as builder log
#5, where it was dismissed as "additional software/hardware interface concepts."
That undersells it — this is the closest published build to ours.

### Why it matters

**Same parts lineage.** 48 V / 3000 W supply → 3 kW ZVS induction module → PZEM
+ shunt metering. That is our kit. This build is an existence proof that the
combination works, and its circuit diagram is the only published schematic of it
found so far.

### Adopt

| # | Detail | Rationale |
|---|---|---|
| A1 | **Freewheel diode across the ZVS input** (theirs: 48 V / 5 kW) | See "the gap it found" below — this is the important one |
| A2 | **DC SSR for cycle switching** (theirs: 60 VDC / 80 A) | Switches in microseconds — a *better* hard step than a mechanical contactor, and no DC contact arcing. But see D2 |
| A3 | **ZVS board mounted on the moving head, adjacent to the coil** | Keeps coil leads short. Lead inductance adds to the tank and lead resistance burns power — the briefing's §3.2 log #3 used 3 ft uncooled extensions and had heat problems |
| A4 | **Commercial taper socket block as the holder nest** | Off-the-shelf BT30 tool-assembly fixture. Cheaper and more repeatable than fabricating, and directly serves MR-17 / MR-22 |
| A5 | **Coil sleeved in white ceramic/glass insulation** | Prevents shorted turns, adds a radiant barrier. Cheap |
| A6 | **Flexible armoured conduit carrying water lines over the top** | Strain relief and abrasion protection on the only lines that must flex |
| A7 | Holder taper-down, nose-up, tool inserted from above | Matches commercial practice and our MR-17. Gravity assists insertion |

### Diverge

| # | Theirs | Ours | Why |
|---|---|---|---|
| D1 | Countdown timer on an LCD | Energy dosing (CR-01) | Time dosing is the gap §4 exists to close. Their display appears to show a seconds countdown |
| D2 | SSR alone | SSR **and** a mechanical contactor in series | A DC SSR fails **short**. It cannot be the E-stop element (SR-03). Use the SSR for cycle switching and a positively-opening contactor for isolation |
| D3 | Wooden base | Metal enclosure, non-combustible base | SR-01 / SR-07 |
| D4 | ZVS board a few inches from the hot zone, fans between | Physical separation (SR-06) | A3's short-lead benefit is real but must not put the board in the radiant field of a 300 °C nose. Resolve by moving the *coil terminals* close, not the whole board |
| D5 | No visible flange/taper heat shield | Mica-sandwiched copper disc (§6.2) | Taper protection is not optional |

Note the tension between **A3 and D4** — both are correct and they pull opposite
ways. The resolution is short *leads*, not a co-located board.

### The gap it found

Their schematic carries a **48 V / 5 kW diode across the induction module
input**. Nothing in our requirements covered this, and it is not optional.

The ZVS board's two DC feed chokes are large inductors carrying up to ~56 A. When
the switching element opens, that current has nowhere to go and the collapsing
field produces a large forward spike across the switch. Our machine terminates
the cycle on **every single dose** — thousands of interruptions over the
machine's life, each one an inductive kick into the SSR or contactor contacts.

Without a freewheel path this destroys the switching element, and can carry the
spike back into the IRFP260Ns, which have only 24.5% margin at the 48 V bus
before any transient is counted.

→ Added as **MR-22**.

---

## R2 — spaco.org, "Work Coils for ZVS Induction Heater"

[Link](https://spaco.org/Blacksmithing/ZVSInductionHeater/WorkCoilsForZVSInductionHeater.htm)
· Blacksmith's build log with a spreadsheet of measured coil data.

**The most useful practical coil reference found so far.** Empirical rather than
analytical — which is the right register for this problem.

| Finding | Effect on our spec |
|---|---|
| Soft copper **refrigeration tubing**, 1/4" OD typical; 1/8" OD for tight winds | Confirms 3/16" as a sensible middle |
| **Minimum practical ID ≈ 3/4" (19 mm) for 1/4" tube** | Our 20 mm bore with 1/4" is right at their floor — the 22 mm suggestion holds |
| 1/8" tube winds on 5/8" (15.9 mm) mandrels cleanly | Interpolating, 3/16" is comfortable at ~17 mm. **Our 20 mm bore with 3/16" is easy**, not "special care" |
| Coils below 1 3/8" (35 mm) ID need care generally | Anneal, wind under tension, consider filling |
| **Multi-layer is standard** — 2-turn × 3-layer, 3-turn × 2-layer | The technique for getting inductance into a short axial length. Relevant to phase 2, not needed now |
| **LC formulas land within ~20%**; stray L and C shift the rest | See below — this corrects us |
| Always insulate, always water cool | Confirms |

### Correction: our frequency numbers were over-precise

Resonance figures throughout `requirements.md` and `quickstart-1-8.md` were
quoted to three significant figures from Wheeler plus f = 1/(2π√LC). R2's
measured experience says **expect ±20%** from unmeasured circuit inductance and
capacitance.

Restating honestly:

| Coil | Predicted | Realistic band |
|---|---|---|
| 1/8" quick-start (20 mm, 5–6 turns) | 108 kHz | **86–130 kHz** |
| Phase-2 (35 mm, 8 turns) | 59 kHz | **47–71 kHz** |

~~Both bands stay under the capacitors' 150 kHz rating~~ — **withdrawn 2026-08-27.**
The part is marked **50 kHz**, not 150. Both predicted bands are ABOVE it. See
`TRAPS.md`; resolve by measuring actual frequency on the scope during VR-05.
No decision changes yet —
but MR-05's window should be read as a target to *measure toward*, not a
specification to hit on paper.

### Worth stealing

R2 keeps **a spreadsheet of coils built and how each performed.** For a problem
this empirical that is a better instrument than a requirements document — a row
per coil with bore, turns, tube, measured frequency, and observed heating would
have answered in three builds what several rounds of modelling did not.

## Still to review

- Cobra HQ Plus — "CNC Shrink Fit With Cheap Induction Heater"
- Husky Machining — "DIY Heat Shrink Toolholder Machine"
- Offworld Gear — "DIY ZVS Induction Heater build for Shrink Fit tooling"
- TeamJC777 — "Heat Shrink Fit Machine DIY Induction Heater"
