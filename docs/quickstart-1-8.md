# Quick start — get 1/8" tools into holders

**Goal:** insert 1–2 carbide 1/8" tools into BT30 shrink holders to unblock PCB
milling. One-way — tools stay in. Frankenstein is fine.

**This document is the near-term job.** [`requirements.md`](requirements.md) is
the future-state reference; ignore it for now.

---

## What "insert once, never remove" deletes

Almost everything. Insertion-only, two cycles total, no repeatability
requirement means these stop mattering:

| Dropped | Why |
|---|---|
| Energy dosing, INA228, CAN control | A stopwatch is fine for two cycles |
| Cycle logging, metrology, pin gauges | The bore-wear instrument tracks thousands of cycles you will never run |
| Holder-life / 5,000-cycle concerns | You need one good insertion per holder |
| Most of the risk register | Written against a production machine |
| Capacitor current anxiety (VR-03) | 2 × 20 s at ~15% duty. They will not notice |
| Shunt calibration, plausibility windows | Nothing is being controlled from them |

**Your original instinct was right for this case.** Heat, try, cool, heat longer.
The objections I raised to that method — heat accumulation, invisible cumulative
damage — are real over hundreds of cycles and essentially irrelevant over two.
The failure mode of "not hot enough" is just *the tool doesn't go in*, which
costs nothing.

## The one change that matters: size the coil to the 1/8" holder

The 35 mm coil was specified for the 27 mm nose on the **1/2"** holder. You are
not using that holder. The 1/8" nose is **15 mm**.

Coupling scales as (d_nose / d_coil_mean)²:

| Coil | Mean dia | Coupling on a 15 mm nose |
|---|---|---|
| Stock 60 mm | 66 mm | **5.2%** |
| 35 mm (the spec) | 39.8 mm | 14.2% |
| **20 mm bore** | **26 mm** | **33.3%** |

A 20 mm bore is a **~6× coupling improvement** over the stock coil.

**Spec: wind on a 3/4" mandrel, 5–7 turns, whatever lands nicely inside 31 mm.**
One continuous piece, two straight legs to the board's barrel clamps, no brazing.
About **1 m** of tube covers helix plus legs.

That's the whole specification. Two constraints, both geometric:

- **Small bore.** The 7× coupling gain over the stock 60 mm coil is the entire
  point, and it's the measured bottleneck (§4.0). A 3/4" mandrel lands ~19.5 mm.
- **Short enough not to reach the flange.** Measured 2026-08-09: only **31 mm**
  from nose tip to end of taper on the 1/8" short holder. The 19.5 mm bore also
  can't pass the 20 mm body, so the coil stops there regardless.

**Everything else was over-thought and has been removed.** Earlier drafts computed
turn-by-turn inductance and frequency, and treated an unbranded capacitor's
"150 kHz" marking as a hard ceiling. That marking is about *sustained* dielectric
heating; you are running two cycles at ~15% duty. The predicted frequencies also
carried ±20% error bars by the source's own account, which made the 5-vs-6 turn
distinction smaller than the model's own uncertainty.

Published DIY builds wind a coil that fits the nose and run it. They work.

**The test that matters** is the instrumented test piece below — one 20-second run
tells you more than any of the arithmetic did. If something runs hot, look at it
then.

### Making the coil

**Reusing the stock tube is real work, not a freebie.** It is already
work-hardened from being formed once; unwinding hardens it further, and rewinding
to a *smaller* radius is the more severe bend. It will kink.

Two paths:

**A — Buy new soft tube (recommended).** Annealed refrigeration tubing, ~$15–20
for far more than needed, any hardware or HVAC supply. **3/16" (4.76 mm) bends
easier than 1/4"**: bend-radius-to-diameter ratio is 2.6 vs 2.1 at this coil
size, and 2.1 is at the limit of what unsupported tube tolerates. Water flow
through 3/16" is ample for 20 s cycles.

**B — Anneal and reuse.** Torch the tube to dull red (~500–600 °C) and let it
cool. Copper anneals on heating; unlike steel it does **not** harden from
quenching, so you can quench to speed it up. Then it forms cleanly. The stock
coil's existing sleeving is worth salvaging either way.

**A helix is not an elbow.** The bend is distributed continuously rather than
localised, so annealed tube wound tight to a mandrel under steady tension
generally comes out clean. Kinks come from stopping, restarting, or letting the
tube lift off the mandrel. Keep constant tension and keep it seated.

**Springback MEASURED 2026-08-09: 0.5 mm on diameter.** 3/4" (19.05 mm) mandrel
produced a **19.50 mm** coil ID with annealed 5 mm tube. So:

```
mandrel = desired ID − 0.5 mm
```

**Sizing.** 19.5 mm ID leaves only 2.25 mm/side on a 15 mm nose — and 1.8 mm on
the 15.9 mm test bar, which is what you actually hand-place. Too tight risks
touching a 400 °C nose with a live coil; too loose only costs cycle time. Those
are not symmetric.

**Turn a 20.5 mm mandrel → 21 mm ID.** 3.0 mm/side on the nose, 2.55 on the bar,
coupling 33.3% vs 37.5%. Four points of coupling for 0.75 mm/side, and the bend
radius improves enough to reduce flattening. Against the stock coil's 5.8%,
anything in this range is a 5–6× win — the choice between 29% and 37% is
fine-tuning, not the decision.

The calculation below is kept because it predicted the measured result exactly,
so it can be trusted for other tube or other diameters:

`R_i/R_f = 1 − 3(σ_y R_i / E t) + 4(σ_y R_i / E t)³`

with t = 0.5 mm wall, E = 117 GPa, centreline radius 12.03 mm:

| Anneal state | σ_y | Final bore |
|---|---|---|
| **Fully annealed** | ~33 MPa | **19.55 mm** |
| Half-hard | ~250 MPa | 23.4 mm |

19.55 mm against a 20 mm target moves coupling 36% → 37.3% and clearance to the
15 mm nose 2.5 → 2.3 mm per side. Neither matters. (A 19.5 mm mandrel would hit
20 mm exactly if you care.)

**⚠️ The estimate holds only if the tube is genuinely soft.** Springback scales
with yield strength — half-hard tube springs back **seven times** as much and
lands at 23.4 mm. New refrigeration tubing arrives annealed and can be wound
straight from the coil. The stock coil tube is work-hardened from being formed
once; anneal it to dull red before winding or the number does not apply.

**Part the stub and coil ends on the lathe too.** Compression fittings need a
square, burr-free end, and that matters given the 3/16"-vs-5 mm tolerance
question — a parting cut gives it directly where a tubing cutter leaves a rolled
lip and a reduced bore.

**Measure the stock coil's tube OD before buying anything.** The kit's water
hose has to seal on whatever you wind, and a size mismatch means leaks onto a
board carrying 56 A. Match the stock OD unless you are prepared to adapt the
hose.

If the stock tube is **1/4" (6.35 mm)**, wind to a **22 mm bore, not 20 mm**.
Centreline bend radius goes 12.7 → 14.2 mm (R/D 2.0 → 2.23), which is the
difference between fighting it and it just working. Coupling drops 32% → 28% —
a rounding error next to the 5% you have now.

**Pitch:** 6 turns touching ≈ 29 mm at 3/16", 38 mm at 1/4". Wind them touching
and **insulate** — bare touching turns are shorted turns.

**Sleeving:** salvage it from the stock coil first. You are replacing the tube,
not the sleeving, and it is already the right size and already paid for. Cut the
old coil's ends free and work the braid off; it stretches.

To buy: **uncoated braided fibreglass sleeving**, ID to slip over the tube OD.
McMaster-Carr, Amazon, or an auto parts store (spark-plug wire heat sleeve).
Prefer uncoated — silicone-coated is only good to ~250 °C, plain fibreglass to
500 °C+, and this sits next to a 300 °C nose. ~$10.

### Water routing — the T-fitting idea

Running a short rigid stub out of each board terminal, then teeing to the
flexible hose away from the board, is **a good idea and worth doing.** It:

- moves any leak away from live electronics — the briefing's source builds
  specifically report hoses popping off and dumping water onto the board
- relieves hose strain on the barrel clamps
- lets you swap coils without breaking water seals
- gives you a **bleed point** — trapped air in the coil is a hot spot that melts it

Three cautions:

1. **Keep the two legs electrically isolated.** They are the tank terminals, at
   different potentials. Metal stubs are fine, but do not let the two fittings
   touch or share a metal bracket — that is a dead short across the tank.
   Non-conductive hose between them, physically separated.
2. **No aluminium in the wet path.** Aluminium against copper in water galvanically
   corrodes fast. Brass or copper only.
3. Water itself is not a short — tap water through 3 mm bore over 0.5 m is
   ~1.4 MΩ. The risk is metal-to-metal, not the coolant.

**Insurance against collapse:** pack the tube with dry sand and cap both ends
before winding, then empty it. Usually unnecessary with properly annealed tube,
but cheap if the first attempt kinks.

## Did the new coil actually help?

**Instrumented test piece, not temper colours.** Colours resolve maybe ±40 °C by
eye and depend on surface finish and exposure time — too crude to sit under any
of the numbers in these documents.

**Build the test piece:**
- ~5/8" (15.9 mm) steel bar, ~50 mm long. The diameter matters: it must
  represent the 15 mm holder nose **and** fit both coils, or the comparison is
  rigged. A 30 mm bar flatters the stock coil and won't enter the new one.
- Cross-drill 1.5 mm, ~8 mm deep at mid-length, to roughly the centre
- K-type bead in the hole, peened or thermally potted
- Route the TC wire **radially outward**, perpendicular to the coil axis (TR-02)

**Protocol — fixed time, read peak:**
1. Piece cold, inserted to the same marked depth
2. Same bus voltage
3. Run **exactly 20 s**, then kill the field
4. Wait 200 ms and read peak temperature (TR-04 — the TC cannot be read *during*
   the cycle; see `requirements.md` §8.3)
5. Cool below hand-warm before the next run

Same energy window, different heat delivered. One number per coil, ±5 °C.

**Cost: ~$20** for a standalone K-type thermometer — and you need one anyway to
avoid cooking a $150 holder on the real job. Not an extra purchase.

Predicted coupling on a 15.9 mm bar: stock 60 mm coil ~5.8%, the 20 mm coil
~42%. Roughly **7×**. A 20 s run that reaches ~40 °C on the stock coil should
reach a few hundred on the new one. If the ratio comes out under 3×, something
is wrong with the build.

Temper colours are a **one-shot** cross-check — straw ≈ 230 °C, purple ≈ 290 °C,
blue ≈ 310 °C — because the oxide is **cumulative**. Once a surface has gone blue
it stays blue; a later run at 230 °C shows nothing new. To use the check again,
**file or sand a fresh witness band** — ten seconds, and it's live.

Two further caveats:

- **Chart temperatures assume ~1 hour of exposure.** A 30 s run needs a *higher*
  temperature to reach the same colour, so short-cycle colour reads low. The
  blue band from first light may have been hotter than 310 °C.
- Where colour genuinely earns its place is as a **cumulative indicator on the
  real holder** — a nose going purple over fifty cycles says you have been
  running hot habitually, which no single-run measurement reveals.

**Secondary: PZEM watts at 53.4 V.** Better coupling makes the board draw more.
Validated against the AC clamp at full voltage (§4.0 of `requirements.md`), so
it is trustworthy there.

**Tertiary: AC clamp on the mains, 5-turn wrap** through the jaw (reading ÷5).
RF-immune, and it becomes *more* useful as the coil improves and current climbs
into the meter's honest range.

**Don't bother with:** the PZEM's Wh counter (1 Wh resolution against ~2–6 Wh
cycles — too coarse), or measuring resonant frequency (needs a scope, and both
predicted bands clear the capacitor rating either way, so it changes no decision).

### Log it

Per reference R2, keep a row per coil. Three builds of this answers more than
any amount of modelling:

| Coil | Bore | Turns | Tube | Test piece | Time→straw | PZEM W | Clamp A | Notes |
|---|---|---|---|---|---|---|---|---|
| stock | 60 mm | ~10 | ? | 5/8" bar | | | | baseline |
| v1 | 20 mm | 5–6 | 3/16" | 5/8" bar | | | | |

## Procedure

1. **Water running before you energize.** Non-negotiable — the coil melts
   otherwise, and the board is silkscreened "Disable output no load."
2. **Set tool stickout first.** Sharpie mark on the shank, or a stop collar, or
   a rod down the bore. You get one shot and about 5 seconds.
3. **Taper protection.** A washer or plate resting on the flange with a scrap of
   mica or ceramic paper under it. Keep the coil on the **nose only** — do not
   let it reach down toward the flange. A warped taper scraps the holder.
4. **Heat and watch for temper colours** on the bare nose:

   | Colour | ≈ Temp |
   |---|---|
   | Pale straw | 200–230 °C |
   | Brown-purple | 265 °C |
   | Purple | 280–290 °C |
   | Dark blue | 310 °C |

5. **Try the tool.** Won't go? Let it cool below hand-warm, heat ~25% longer,
   try again. Repeat.
6. **Insert in one motion to the stop and hold it down** until it grips —
   a couple of seconds.
7. **Cool.** Air is fine. Don't quench with the tool in it.

## Honest note on 1/8"

The 1/8" is the hardest size on any shrink machine, commercial included, because
the clearance you need to slide a tool in by hand doesn't scale down with the
bore. Estimated requirement is ~390 K rise (≈410 °C), at the top of the
250–400 °C window.

That estimate rests on an assumed 0.008 mm insertion clearance that I have no
source for. It may well be lower, in which case ~300 °C does it. **You will find
out empirically in about twenty minutes**, which is faster than any amount of
further analysis.

If it needs to run hot, for a one-way insert that is an acceptable trade: the
cost of over-tempering is reduced grip and shortened holder life, and the holder
will outlive the tool it's holding.

## Buy nothing

Everything needed is on the bench. Optional $15: a freewheel diode across the
ZVS DC input — it protects the switch and the FETs on every cycle termination.
Two cycles won't hurt anything without it, but you'll want it when the machine
becomes permanent.

---

## Then what

Once tools are cutting, phase 2 is a prioritised roadmap rather than a
requirements document — safety items first, then conveniences. The material in
`requirements.md` feeds that, but it needs re-sorting by value, not by category.

## First real holder — the bracketing protocol

**Scott's rule, 2026-08-30: underestimate the time and add to it. Do not try to
predict it exactly.** The costs are lopsided — under-heating spends a cycle,
over-heating spends a holder — so there is no case for aiming at the centre.
Same asymmetry this document already applies to coil fit ("too loose only costs
cycle time").

Three refinements, two of which bite:

**1. Heat is cumulative, so the increments must shrink.** A second attempt on a
holder still at 200 °C is not 10 s from ambient; it is 10 s added to 200 °C.
Constant increments accelerate as you climb the ladder. Either cool to ambient
between attempts and keep the steps even, or stay hot and make each step
*smaller* than the last. Adding a fixed 5 s repeatedly is the one option that
does not work.

**2. The hazard is being *marginally* under, not well under.** A bore that is
nearly expanded enough lets the tool enter partway and then seizes on it as it
cools — a tool stuck at the wrong depth, which is a worse position than one that
would not start. **So make the first attempt clearly short and the steps
meaningful.** Creeping up in 1 s slices maximises time spent in exactly the
band that seizes.

**3. Attempt insertion with the holder still in the coil.** If the tool binds,
the field is right there and it can be backed out immediately. Carry the holder
to the bench first and that recovery is gone. Have the tool in hand before the
cycle ends — Haas's guidance is to insert **2 s** after heating stops, which is
not long enough to go looking for anything.

**If it does not drop under light finger pressure, do not push.** That is the
signal to re-heat, not to persuade.

Log every rung: coil, holder P/N, seconds, cumulative seconds, whether it went,
and the temper witness if one is fitted. The ladder converges after a couple of
holders and then becomes the size preset.
