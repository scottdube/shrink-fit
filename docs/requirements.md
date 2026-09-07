# DIY Induction Shrink-Fit Machine — Requirements & Commercial Gap Analysis

**Target machine:** BT30 shrink chucks for Tormach 1100MX
**Status:** Phase-2 reference. Not the current job.
**Budget:** see [`bom.md`](bom.md) — ~$730, over the original $250–500 envelope.

> ### ⚠️ Read this first
>
> **The immediate job is [`quickstart-1-8.md`](quickstart-1-8.md)** — get 1–2
> carbide 1/8" tools into holders to unblock PCB milling. One-way insertion,
> Frankenstein acceptable. That needs almost none of what follows.
>
> **This document over-reached.** It was written as a commercial-vs-DIY gap
> analysis and grew into a 70-requirement production-machine spec while the coil
> was still unwound. Several sections are corrections to earlier sections of
> itself — a sign that modelling was substituting for measurement in a domain
> where measurement costs an afternoon. The first bench test taught more than
> the analysis preceding it.
>
> Treat it as a **menu, not a mandate**. It is a decent reference for phase 2,
> when the pressure is off and a prioritised roadmap can be built from it —
> safety items first, then conveniences. Nothing here is required to put a tool
> in a holder this week.

---

## 1. Scope

### 1.1 In scope
A benchtop machine that heats the nose of a BT30 shrink-fit chuck to release or
clamp a solid-carbide tool shank, with repeatable cycle control and thermal
protection of the taper and shank.

### 1.2 Explicit non-goals
- CAT40 / HSK63 capability. The power budget does not support it (§4.1).
- HSS or steel shanks. Standard shrink chucks are carbide-only; HSS expands at
  nearly the holder's rate and will not release reliably.
- Throughput. One operator, one tool change at a time, minutes between cycles.
- Any form of certification. Risk is managed by enclosure and interlock
  practice, not by compliance (§7).

### 1.3 Governing physics
Release requires bore growth of interference + working clearance ≈ 0.04 mm.

```
ΔT = ΔD / (D · α_steel) = 0.04 / (12 mm × 11.5e-6 /K) ≈ 290 K
```

→ ~300 °C rise at the nose. Industry practice brackets this at **250–400 °C**.
Above the holder's tempering temperature the steel draws soft and the bore
enlarges *permanently* — this is a one-way failure, not a recoverable one.

---

## 2. Hardware baseline (to be verified on receipt)

Kit ordered: eBay 286924140726 — "3000W Induction Heater ZVS Module with DC
Ammeter + 53V Power Supply".

**Received 2026-08-09.** Receiving inspection status:

| Item | Expected | **Actual** | Status |
|---|---|---|---|
| PSU | Emerson-class 53.5 V / 56 A | **R48-3000e3**, nameplate `-48V 3000W`, AC in 200–250 V ~22 A | ✅ |
| PSU on 110 V | ~1348 W | **53.5 V, 25.2 A, 1348 W** (100–120 V in, 17 A) | ✅ exact |
| PSU on 240 V | ~3000 W | **53.5 V, 56.1 A, 3000 W** (200–240 V in, 17 A) | ✅ exact |
| PSU thermal | — | −20…+65 °C; **no derate to 45 °C, output stops >65 °C**; label warns against sustained high power | ✅ validates ≤15% duty |
| ZVS board | Mazzilli/Royer | **WMY TECH**, silkscreen `INPUT: DC48–… 58A (MAX)` | ✅ |
| DC chokes | "assume undersized" | **Two large iron-powder toroids, heavy gauge** | ✅ better than expected |
| Coil terminals | — | Two barrel clamps for water-cooled tube | ✅ |
| Aux connectors | — | `DC 12V`, `Pump`, 3× 2-pin JST; 80 mm fan on standoffs | ✅ |
| MOSFETs | ≥224 V for VR-05 | **IRFP260N** (IR, TO-247) — **200 V**, 50 A, 40 mΩ | ❌ **VR-05 fails at 53.5 V** |
| Tank capacitors | 6× 0.33 µF / 1200 V | **0.33 µF, 630 VAC / 1200 VDC, 50 kHz, MKPH self-healing**, 12 off | ⚠️ 12 not 6, and **50 kHz not the 150 assumed** |
| Metering | Ammeter + shunt | **PZEM-025** + **100 A shunt** (mV rating TBC) | ✅ shunt present |
| DC breaker | not expected | **CHCYZO DZ47-63DC, B63, 12–125 V DC** | ⚠️ see below |
| Cooling | not expected | **2× 80 mm fans on heatsinks, 12 V water pump, clear tubing** | ✅ more complete than expected |
| Work coil | 60 mm ID copper tube | Not yet inspected | ⚠️ OPEN |
| Water fittings | Slip-on vinyl | Not yet inspected — replace regardless (§6.5) | ⚠️ OPEN |
| `TB2400-17A` | — | Unidentified black module on the ZVS board | ⚠️ OPEN |

**Board silkscreen — operational rule.** The ZVS board is marked **"Disable
output no load"** in two places. Never energise without the work coil connected.
Note this is distinct from VR-01, which runs an *empty* coil — coil present, no
holder. That test remains valid.

**DC breaker findings (DZ47-63DC B63).**
- Voltage rating 12–125 V DC is adequate for a 53.5 V bus. ✅
- 56 A operating against a 63 A frame is 89% of rating — acceptable only because
  duty cycle is ≤15%.
- **B curve trips magnetically at 3–5× In (189–315 A).** Contactor-close inrush
  into the ZVS input capacitance may exceed that. Expect possible nuisance trips
  during commissioning; a C-curve (5–10×) device is the fix if it happens.
- **This does not satisfy SR-02.** SR-02 requires a dual-pole breaker on the
  **AC mains**; this is a DC-side device. An AC breaker is still to be sourced.

### 2.1 The breakout board and its 2-pin JST — CAN control

The PSU ships with a reseller adapter card that mates with the rectifier's blade
edge connector and breaks it out to screw lugs. Inspection of that card shows the
2-pin JST is wired to the **narrow signal fingers**, not the wide power pads —
so it is a low-current signal line, not an auxiliary supply.

The R48-3000e3 is a **CAN-controlled** telecom rectifier. Two pins, signal-level,
on a CAN-managed rectifier ⇒ **almost certainly CAN-H / CAN-L**. Confirm before
connecting (§2.2).

Documented CAN capability (from the community ESP32 libraries):

| Function | Range |
|---|---|
| Set output voltage | **41.5 – 58.5 V**, 0.1 V steps |
| Set output current | 1 – 50 A, 0.1 A steps (settings below ~5 A appear ignored) |
| Telemetry readback | Output voltage, output current |
| Power-up behaviour | Boots **AC ON**, outputs default voltage and *last set* current |

**This resolves the VR-05 problem in firmware — and VR-05 is now confirmed to
be a real failure, not a hypothetical.** The devices are **IRFP260N, 200 V**.

| Bus | Peak Vds ≈ π·V | Margin on 200 V | Verdict |
|---|---|---|---|
| 53.5 V | 168 V | 16% | ❌ fails VR-05 |
| 48.0 V | 151 V | 24.5% | ✅ compliant |
| 41.5 V | 130 V | 34.8% | ✅ commissioning |

**⚠️ π·Vdc is the idealised Royer peak.** Leakage-inductance ringing routinely
pushes real peaks 10–20% above it without snubbing — which at 48 V would mean
165–180 V on a 200 V part. **VR-05 must be measured on a scope, not inferred
from this table**, before 48 V is accepted as the operating bus.

**Decisions:**
- **Operating ceiling: 48 V.** Not a preference — 53.5 V is out of specification
  for these devices.
- **Commissioning: 41.5 V**, pending scoped VR-05.
- 41.5 V is *commissioning only*. Power scales as V², so it costs ~40% and
  stretches §4.3's 14–17 s predictions to roughly 24–29 s, which touches the
  30 s stop rule. At 48 V they stretch ~20%, to roughly **18–21 s** — still
  inside the ≤25 s envelope.

**Tank capacitors — 12 confirmed, not 6.** Tank C = 12 × 0.33 µF = **3.96 µF**.
This changes the coil design and materially *improves* the capacitor risk.

Two configurations, both with the MR-02 eight-turn coil at ~1.8 µH:

| Config | Tank C | f | I_rms total | **Per cap** |
|---|---|---|---|---|
| 6 caps (assumed) | 2.0 µF | 84 kHz | 112 A | 18.7 A |
| Re-tune to ~90 kHz on 12 caps (4–5 turns, ~0.8 µH) | 3.96 µF | 90 kHz | 220 A | 18.3 A |
| **12 caps, keep 8 turns** | **3.96 µF** | **~59 kHz** | 159 A | **13.2 A** |

**Adopt the third row.** Keeping the turn count and letting resonance fall to
~59 kHz drops per-capacitor current **28%**, from ~18.5 A to ~13.2 A — the
single largest reduction available in the tank, and it costs nothing.

Why the re-tune option is worse: holding frequency constant makes total
circulating current scale with C, so per-cap current stays flat and you gain
nothing while halving tank impedance (Z = √(L/C): 0.67 Ω → 0.48 Ω).

Why ~59 kHz is fine:
- Skin depth only deepens (~0.06 mm), and it was already irrelevant against a
  4–7 mm nose wall — conduction dominates either way.
- ~~Far below the capacitors' 150 kHz rating.~~ **WRONG — corrected 2026-08-27.** The
  part is marked **50 kHz**. Predicted bands (86–130 kHz small coil, 47–71 kHz
  phase-2) sit at or above it. Reaching 50 kHz against the fixed 3.96 µF needs
  ~2.56 µH, a far larger coil than the coupling argument wants. Unresolved: the
  marking may be a ripple-current reference rather than a ceiling. **Measure the
  real frequency during VR-05 before running hard.**
- Comfortably inside the range a Royer stage will oscillate over.

✅ **CONFIRMED PARALLEL 2026-08-09** from the board underside. Two heavy
solder-reinforced buses run the board length, both terminating at the copper coil
terminals, with capacitor lead holes arrayed between them — each cap bridges the
two buses. Series pairs would show isolated intermediate pads; there are none.
Tank = **3.96 µF**.

It also gives a genuine **2:1 continuous power range** (P ∝ V²), which supersedes
the original assumption that a ZVS stage has no power control. §4.3's
self-compensation means this is not *needed*, but it is available as a clean
lever if measured small-size cycles come out too short to control.

### 2.2 Verify before connecting

| Step | Method | Expected if CAN |
|---|---|---|
| 1 | AC disconnected — resistance across the two JST pins | ~120 Ω (terminated) or open (unterminated) |
| 2 | Resistance, each pin to chassis and to each DC lug | High — **not** continuity |
| 3 | Powered, no load, DMM on each pin to its reference | ~2.5 V recessive idle. **Not 12 V, not 53 V** |
| 4 | Scope differentially | Periodic status frames; many R48 units broadcast unprompted |

If step 3 reads 12 V it is an aux supply; 0 V with a pull-up suggests an
enable/inhibit input; bus potential means do not connect anything rated lower.

**~~Assumption A1~~ — CONFIRMED 2026-08-09.** 240 V single-phase is available.
All performance figures below assume the PSU is fed 240 V. The 110 V case in
§4.1 is retained only to document why the feed matters.

---

## 3. Operating envelope

**Target holders: Haas Tooling BT30 shrink fit, TSC, 1/8"–1/2".**

| Haas P/N | Bore | Gage | Nose OD | Max body | Clamp length |
|---|---|---|---|---|---|
| 04-1424 | 1/8" | 2.56" | **15.0 mm** | **20.0 mm** | 15.0 mm |
| 04-1425 | 1/8" | 3.94" | ~15 mm ᵉ | ~20 mm ᵉ | ~15 mm ᵉ |
| 04-1427 | 3/16" | 3.94" | ~17 mm ᵉ | ~22 mm ᵉ | — |
| 04-1428 | 1/4" | 2.56" | ~19 mm ᵉ | ~24 mm ᵉ | — |
| 04-1430 | 5/16" | 2.56" | ~21 mm ᵉ | ~26 mm ᵉ | — |
| 04-1432 | 3/8" | 2.56" | ~23 mm ᵉ | ~28 mm ᵉ | — |
| 04-1434 | 1/2" | 2.56" | **27.0 mm** | **32.0 mm** | 37.0–47.0 mm |

Bold = published (min/max body diameter from Haas). ᵉ = linear interpolation,
**verify with calipers on arrival.**

Common to the family: OAL 4.46", concentricity 0.00012" (3 µm), weight
0.5–0.6 kg, balanced to 25,000 rpm, M12×1.75 retention knob, through-tool
coolant.

| Parameter | Value |
|---|---|
| Holder interface | BT30, Haas shrink fit, TSC |
| Nose OD range | 15–27 mm |
| Shank diameters | 1/8, 3/16, 1/4, 5/16, 3/8, 1/2" carbide |
| Bore engagement | 15 mm (1/8") to 47 mm (1/2") |
| Holder material | Not published by Haas — **see OQ-3** |
| Target nose temperature | 300 °C nominal (see §4.3 — small sizes need more) |
| Hard ceiling | 350 °C |
| Heat cycle | ≤ 25 s |
| Cool to handling | ≤ 90 s |
| Duty cycle | ≤ 15% |
| Expected holder life | ~5,000 heat cycles if never overheated |

---

## 4. Commercial vs. DIY gap analysis

Reference class: Haimer Power Clamp Basic / Economic Plus — 10–20 kW, ~5 s heat,
~30 s cool, one V2008 coil + 5 stop discs covering 3–32 mm chucks, $5k–15k.

| # | Dimension | Commercial | This kit as shipped | Consequence if left open | Close? |
|---|---|---|---|---|---|
| 1 | **Raw power** | 10–20 kW | ~2.5 kW @ 240 V | 20 s vs 5 s cycle; heat soaks into shank | **Cannot close** — mitigate |
| 2 | **Control law** | Process/energy controlled, recipe per chuck | Manual on/off, no timer | Non-repeatable; overheat → permanent bore growth | **Close — highest value/$** |
| 3 | **Coil coupling** | 1 coil + 5 stop discs, 3–32 mm | Fixed 60 mm ID | ~30–35% coupling on a 21 mm nose → cycle 3× longer | **Close — highest value/hour** |
| 4 | **Taper protection** | Machine geometry + discs | Nothing | Warped taper = scrapped holder | **Close — mandatory** |
| 5 | **Cooling** | Contact sleeves, ~30 s | Nothing | Long cycles; re-heating a warm holder overshoots | Close — cheap |
| 6 | **Length preset** | Integrated presetter/stop | Nothing | Stick-out varies shot to shot | Close — mechanical only |
| 7 | **Hold-down** | Pneumatic | Gloved hand | Vapor-lock ejection, burns | Close — spring arbor |
| 8 | **Startup sequencing** | N/A (not a ZVS) | None | ZVS shoot-through on PSU ramp → dead board | **Close — mandatory** |
| 9 | **Coolant interlock** | Chiller + flow monitor | Bucket, slip-on hose | Coil melts if flow stops | Close — flow switch |
| 10 | **Enclosure / interlocks** | CE, enclosed, E-stop | Bare board | Fire, shock | Close — practice |
| 11 | **Recipes / traceability** | Stored per holder type | None | Minor for one operator | Partial (firmware) |
| 12 | **Throughput** | Seconds, tool-room duty | ~1 change / 2 min | None at this scale | **Accept** |

### 4.0 First light — 2026-08-09

Bench test on 240 V, stock 60 mm coil, ~30 mm steel bar, no enclosure.
**It heats.** Clear temper colours on the bar: straw band plus a distinct blue
band ≈ **310 °C** — inside the shrink-fit window on the first attempt.

PZEM readings, steady state: **53.4 V, 5.56 A, 297 W.** One transient dip to
42.4 V / 5.25 A that recovered.

**AC-side cross-check (240 V mains, single-conductor clamp):**

| PZEM V | PZEM A | PZEM W | Clamp A | AC W | Implied η |
|---|---|---|---|---|---|
| **53.4** | 5.57 | **297** | **1.40** | **336** | **88%** ✅ |
| 38.8 | 4.61 | 172 | 1.83 | 439 | 39% ✗ |
| 37.0 | 4.38 | 163 | 1.32 | 317 | 51% ✗ |
| 35.5 | 4.38 | 158 | 1.58 | 379 | 42% ✗ |

**Only the first row is trustworthy, and it validates the PZEM.** 88% is textbook
for a telecom rectifier at ~10% load. The other three are nonsense (a rectifier
cannot run at 40% efficiency here) — and they are nonsense because **1.3–1.8 A on
a 40 A range is 3–5% of full scale**, below where this meter is specified. All
four clamp readings sit inside each other's error bars. Redo with a **5-turn
wrap** through the jaw (reading ×5, divide after) to move into the accurate band.

**Consequence: 300 W appears to be real, not a measurement artifact.** R-11
drops in likelihood; **coupling is the explanation**, consistent with the stock
coil resonating near 37 kHz.

**Diagnostic target for the coil rewind:** at the 2500 W design point the mains
clamp should read **≈12 A** (2500 W ÷ 0.88 ÷ 240 V). It currently reads 1.4 A —
a factor of **8.4** to close. This metric is independent of the PZEM entirely and
is the cleanest progress measure available during the rewind.

**Two findings.**

**(1) 300 W is ~10% of the design target** and needs resolving before anything
else. Three candidate causes, not yet separated:
- Loose coupling — stock 60 mm coil, partly filled. A 10-turn stock coil against
  3.96 µF resonates near **37 kHz**, below the 55–65 kHz target, and power
  transfer falls with frequency.
- PZEM shunt scaling — if configured for a 300 A shunt against a 100 A part it
  reads ⅓ of actual. Note the observed ~5.6 A is *below* VR-01's 7 A empty-coil
  benchmark, which is backwards for a loaded coil.
- **RF corruption of the PZEM's own sense leads** — see R-11.

**(2) The §4.1 power model is wrong in shape.** It treated DC input as fixed at
~2500 W with coupling acting as an efficiency factor on the way to the work. A
Royer does not behave that way: **the load sets the draw.** Poor coupling means
the board pulls *less* DC current, not that it pulls full current and wastes the
difference. Read §4.1's "DC in" column as *DC available*, not DC drawn.

The conclusion is unchanged and strengthened — the stock coil is the bottleneck,
and MR-01's 35 mm rewind is the highest-value single action in the build. This
test measured that empirically instead of predicting it.

**Running voltage was 53.4 V — above the 48 V ceiling (§2.1).** Tolerable at
300 W; not tolerable as power comes up. Resolve via CAN or a scoped VR-05 before
loading the board.

### 4.1 The gap that cannot be closed — and why it's survivable

Power sets cycle time, and cycle time sets how far heat travels up the carbide
shank before you can insert. Penetration ≈ √(αt), α_carbide ≈ 2.3e-5 m²/s:

| Cycle | Penetration | vs. 35–40 mm engaged shank |
|---|---|---|
| 5 s (commercial) | ~11 mm | Negligible |
| 20 s | ~21 mm | Acceptable |
| 40 s | ~30 mm | Marginal |
| 68 s | ~40 mm | **Fully soaked — no net clearance** |

Delivered power, 1/2" BT30 nose (32 kJ):

| Configuration | DC in | Into work | Cycle |
|---|---|---|---|
| 110 V + stock 60 mm coil | 1348 W | ~470 W | **~68 s** ✗ |
| 110 V + 35 mm coil | 1348 W | ~810 W | ~40 s |
| 240 V + stock 60 mm coil | ~2500 W | ~875 W | ~37 s |
| **240 V + 35 mm coil** | **~2500 W** | **~1500 W** | **~21 s** ✓ |

**Neither the 240 V feed nor the coil rewind is sufficient alone.** Both are
required to land inside the acceptable band. This is the central finding.

### 4.2 What the stop discs are actually for

Haimer's standard coil covers 3–32 mm **bore** chucks with 5 stop discs. That is
far too wide a range for a fixed coil to couple well across — so the discs are
primarily **field-shaping apertures that adapt one coil to many chuck
diameters**, with the mechanical stop and shank shielding as secondary functions.

This corrects the framing in the source briefing, which presents the copper disc
purely as a carbide shield. The disc is not the primary defense against shank
heating — **cycle speed is** (§4.1). A disc on a 68 s cycle protects nothing.

**However** — this machine spans 3.2–12.7 mm bore, only the lower ~40% of
Haimer's range. §4.3 shows that over this narrower span the discs are *not*
needed for power adaptation. They are retained as mechanical stops and shank
shields only.

### 4.3 Size-range analysis — the fixed coil self-compensates

Two effects run opposite each other across the range. Modelled on a 35 mm ID
coil at 2500 W DC with 85% board+coil efficiency, using published clamp lengths
as the heated-zone base and a 1.6× conduction-spread factor:

| Size | Nose OD | Heated mass | Req. ΔT | **Energy** | Coupling | P into work | **Cycle** |
|---|---|---|---|---|---|---|---|
| 1/8" | 15 mm | 32 g | 393 K | **5.7 kJ** | 18% | 391 W | **14.7 s** |
| 3/16" | 17 mm ᵉ | 49 g | 320 K | **7.2 kJ** | 24% | 501 W | **14.4 s** |
| 1/4" | 19 mm ᵉ | 71 g | 283 K | **9.2 kJ** | 30% | 627 W | **14.6 s** |
| 5/16" | 21 mm ᵉ | 97 g | 262 K | **11.7 kJ** | 36% | 765 W | **15.3 s** |
| 3/8" | 23 mm ᵉ | 128 g | 247 K | **14.5 kJ** | 43% | 918 W | **15.8 s** |
| 1/2" | 27 mm | 207 g | 229 K | **21.8 kJ** | 60% | 1264 W | **17.3 s** |

Energy spans **3.8:1** across the range; coupling spans **3.2:1** in the opposite
direction. Predicted cycle time is flat within ±10% — **14.4 s to 17.3 s across
the entire 1/8"–1/2" family on one fixed coil.**

(Supersedes the two-point estimate in draft 1, which used guessed heated-zone
lengths. Conclusion unchanged; absolutes shifted ~20%.)

The small holder needs far less energy *and* couples far worse. The two effects
very nearly cancel, so **cycle time is roughly constant across the whole
1/8"–1/2" range with a single fixed coil.**

Consequences:
- No disc set is required for power adaptation (contra §4.2's general case).
- One coil, one geometry, six energy presets. Materially simpler machine.
- Do **not** "improve" small-size coupling with a reducing sleeve or smaller
  coil. The poor coupling is doing useful work — it is what keeps the 1/8"
  cycle long enough to control.

**⚠️ Caveat added after first light (§4.0).** The bullet above was written
assuming power is abundant and the risk is a 1/8" cycle too *short* to dose
accurately. First light measured only ~300 W, so power may instead be scarce. If
the built 35 mm coil yields a 1/8" cycle **longer** than ~25 s, this advice
inverts and a second, smaller coil for the 1/8"–3/16" sizes becomes correct.
Decide from the measured 1/2" power: the small-size figure follows by the
coupling ratio in MR-01's table.

The (d/D)² coupling model is crude and ignores workpiece resistance. Treat the
~1:1 result as a prediction to be confirmed in VR-07, not a specification.

### 4.4 Why small holders need a *higher* temperature

Required bore growth = interference + insertion clearance. Interference scales
with diameter; the clearance needed to slide a tool in by hand does not.

```
1/8" : (0.006 + 0.008) / (3.175 × 11.5e-6) ≈ 390 K
1/2" : (0.025 + 0.008) / (12.7  × 11.5e-6) ≈ 230 K
```

So the 1/8" holder wants ~390 K rise (≈410 °C) — **at or above the 250–400 °C
damage window**, in a nose with the least mass to absorb an overshoot. This is
the documented reason slim holders get burned up in the field, and it is not a
DIY-specific problem.

Interference figures above are estimates. Confirm against measured bore and
shank sizes before finalizing the CR-08 preset table.

**Risk R-1:** 1/8" and 3/16" holders are marginal for *any* shrink machine and
will consume holder life faster than larger sizes. The comfortable range for
this machine is **1/4"–1/2"**.

**DECISION 2026-08-09 — 1/8" stays in scope, provisionally.** The verdict above
rests on an unsourced insertion-clearance estimate (0.008 mm). It is not
evidence, and the campaign measures it directly: plot required ΔD against bore
diameter across sizes and the clearance term is the y-intercept. By 1/4" there
will be four measured points. The size drops out only if it trips a §8.1 stop
rule.

**The temperature ceiling is deferred pending OQ-3** and this blocks nothing.
The campaign runs 1/2" → 3/8" → 5/16" → 1/4" first, and every one of those needs
≤283 K rise (~300 °C) — comfortably inside any plausible ceiling. The question
only becomes decision-relevant at 3/16", by which point both the Haas answer and
four measured data points should be in hand.

---

## 5. Control requirements

### CR-01 — Energy dosing, not time dosing *(closes gap #2)*
The controller SHALL terminate the heat cycle on **accumulated DC energy**
(∫V·I·dt), not elapsed time.

Rationale: this is what "process-controlled" means on commercial machines. It
makes the cycle immune to line sag, PSU foldback, bus voltage drift, and coil
resistance change with temperature. The kit already ships a shunt and ammeter;
adding a divider on the 53 V bus and integrating in firmware costs ~$5.

*Limitation:* measures energy *in*, not energy *into the holder*. Coupling still
varies with holder seating. Energy dosing is a large improvement over time, not
a substitute for CR-02.

### CR-02 — Post-cycle temperature verification and lockout
*(Revised — the original "real-time thermocouple cutoff" is not achievable; see
§8.3.)*

The controller SHALL sample nose temperature **200 ms after field-off**, log it
per CR-10, and:
- peak > **350 °C** → flag the cycle, require operator acknowledgement
- peak > **375 °C** → lock out that size preset until recalibrated

Rationale: a thermocouple inside a kW-class 90 kHz field cannot be read during
the cycle (§8.3). Temperature is therefore a *verification and next-cycle
interlock*, not a mid-cycle abort. Real-time protection rests entirely on CR-01
(energy dose) and CR-03 (time fault).

Thermal decay of a 0.5 kg holder is slow relative to 200 ms, so the post-field
reading is within a few °C of true peak.

### CR-03 — Absolute time ceiling and sensor plausibility
*(Revised — the original 1.5×-expected ratio was too loose; see review Tier 1 #2.)*

The controller SHALL abort and latch a fault on **an absolute per-size time
ceiling**, not a multiple of expected cycle time.

Rationale: with CR-02 demoted to post-cycle (§8.3), energy dosing is the only
real-time protection. Its dangerous failure is the sense chain reading *low*
while real power is high — dose accrues slowly, heating continues, and a ratio
applied to a wrong dose arrives far too late. At 1.5× on a 1/8" holder that is
roughly a 590 K rise. Destroyed holder.

The controller SHALL additionally abort if, after a startup grace period:
- bus voltage leaves 35–60 V, or
- current leaves 5–70 A, or
- the INA228 and the rectifier's CAN telemetry (CR-11) disagree by >20%

The last is the only check that catches a miscalibrated sense chain, which
energy integration structurally cannot detect in itself.

### CR-04 — Start-temperature gate
The controller SHALL refuse to start if measured nose temperature exceeds
**60 °C**. Prevents overshoot when re-heating a warm holder — the failure mode
that permanently enlarges bores.

### CR-05 — Startup sequencing *(closes gap #8; **REVISED 2026-08-30, see §6.10**)*
On power-up the controller SHALL block the 53 V DC line from the ZVS board for
**≥30 s**, then apply it as a hard step **via the SSR** (not the contactor —
see §6.10 for why the original wording is stale and what now gates arming).

Rationale: a Royer oscillator needs a voltage step to commutate. A slow PSU ramp
puts both MOSFETs into conduction simultaneously. Switch the **DC output**, not
the AC input, and do **not** add pre-charge or soft-start on that line — the
inrush is the starting mechanism.

**CAN does not replace the contactor.** Commanding the rectifier on over CAN
produces exactly the slow ramp this requirement exists to prevent. The rectifier
sets the *voltage*; the contactor delivers the *step*. Likewise SR-03: the E-stop
path is hardware cutting the contactor coil — a CAN disable is not an E-stop.

### CR-11 — CAN supervisory link *(new, see §2.1)*
The controller SHALL supervise the rectifier over an **isolated** CAN transceiver:

- Set bus voltage per commissioning stage and, if needed, per holder size
- Set an output **current limit** — an independent hardware protection layer that
  acts even if the ESP32 sense chain or firmware misbehaves
- Read back rectifier-reported voltage and current as the **independent
  cross-check** required by the sensor-plausibility rule

Rectifier telemetry updates far slower than CR-08's ≥100 Hz integration rate, so
it is a plausibility reference, **not** the primary sensor. Two independent
measurement chains from different vendors is the point.

Isolation is required because SR-10 has not yet established whether a DC output
rail is chassis-bonded. Note also that community reports favour 5 V MCP2515
modules over 3.3 V operation — level-shift rather than fight it.

### CR-06 — Coolant interlock
Heat cycle start SHALL be inhibited unless coolant flow is proven (flow switch),
and SHALL abort on flow loss mid-cycle.

### CR-07 — Coolant overtemp
Coolant return above **50 °C** SHALL inhibit cycle start.

### CR-08 — Presets
The controller SHALL store an energy target per shank diameter — **1/8, 3/16,
1/4, 5/16, 3/8, 1/2"** — selectable without a host computer. Six presets,
populated empirically from VR-07. Expected span ~7.5 kJ to ~24 kJ (§4.3).

Integration SHALL sample at ≥100 Hz so dose resolution is <1% on the shortest
cycle.

### CR-09 — Single-action start, always-available abort
One deliberate action starts a cycle. E-stop and abort SHALL be reachable
without looking away from the work.

### CR-10 — Cycle logging *(this is the metrology, see §8.2)*
Every cycle SHALL log: timestamp, holder size/ID, delivered energy, start nose
temperature, peak nose temperature, cycle duration, and outcome.

Rationale: release energy is a highly sensitive proxy for remaining interference
(§8.2) — roughly **3–4% energy change per micron of bore growth**. The logged
trend is a better instrument than any affordable direct bore measurement. This
promotes CR-04 (start-temp gate) and MR-17 (nest repeatability) from safety
requirements to *metrology* requirements: they control the confounders that
would otherwise swamp the trend.

---

## 6. Physical requirements

### 6.1 Work coil *(closes gap #3)*

| MR-01 | Coil ID | **33–35 mm.** Set by the 27 mm 1/2" nose plus clearance. The coil never has to pass the 32 mm max-body section — that sits back toward the flange. |

**Three diameters, do not confuse them.** With 3/16" (4.76 mm) tube and a
**35 mm clear bore**:

| Dimension | Value | Used for |
|---|---|---|
| **Clear bore (ID)** | **35.0 mm** | Clearance. The number to build to. What the nose passes through |
| Mean / centreline | 39.8 mm | **Inductance and coupling.** Where the current actually flows |
| Coil OD | 44.5 mm | Envelope. What the former must clear |

**MR-01 sizing — corrected 2026-08-09.** An earlier table computed coupling from
the *clear bore*. Coupling is set by the current path, so it must use the **mean
diameter** (bore + one tube OD). The absolutes below are ~29% lower than
previously stated:

| Clear bore | Mean dia | 1/2" (27 mm) | 1/8" (15 mm) | Gap | |
|---|---|---|---|---|---|
| 30 mm | 34.8 | 60.3% | 18.6% | 1.5 mm | too tight |
| **33 mm** | **37.8** | **51.1%** | **15.8%** | **3.0 mm** | tight; thin liner only |
| **35 mm** | **39.8** | **46.1%** | **14.2%** | **4.0 mm** | default |
| 38 mm | 42.8 | 39.9% | 12.3% | 5.5 mm | gives away coupling |
| 60 mm (stock) | 66.0 | 16.7% | 5.2% | 16.5 mm | the measured bottleneck (§4.0) |

**§4.3 is unaffected.** Self-compensation depends on the *ratio* of large-to-small
coupling, which is 46.1/14.2 = **3.25** — versus 3.23 under the old table.
Unchanged. Only the absolute power predictions were optimistic, and §4.0 already
flagged those as unreliable for a different reason.

**Expected gain from the rewind stands at ~2.8×** (16.7% → 46.1%), plus resonance
moving 37 → 59 kHz.

**Build implication.** Nothing thick may sit between coil and work — every
millimetre of wall pushes the mean diameter out and costs coupling. Sleeve the
**tube itself** in thin glass or ceramic (0.5–1 mm, as reference R1 does) and
carry the coil from an **outer** G-11 support. A 2 mm former wall inside the coil
would drop 1/2" coupling from 46% to ~38%.

The 4 mm radial gap at 35 mm has to contain the former wall or ceramic sleeve
(~1.5–2 mm) plus air (~2–2.5 mm) plus MR-24's ±0.5 mm position tolerance. 33 mm
buys **+13% coupling** for 1 mm less per side — take it only if the former and
hard stops are built tightly enough to never touch a 300 °C nose.

**Expected gain from the rewind alone:** 60 → 35 mm roughly **triples** coupling
on the 1/2" nose (20% → 59.5%), with a further gain from resonance moving
37 → 59 kHz. Watch the §4.0 mains-clamp metric.

**Validate before committing.** Wind a bare unsleeved test coil at 35 mm, put a
27 mm bar in it, and compare the mains clamp against the stock coil. An
afternoon's work that de-risks the real G-11 build.
| MR-02 | Turns | 8, of 3/16" (4.76 mm) copper tube |
| MR-03 | Axial length | ≤ 40 mm — must not extend toward the flange |
| MR-04 | Inductance | ~1.5–1.8 µH target |
| MR-05 | Resonance | **55–65 kHz**, measured with a pickup loop, not assumed |
| MR-06 | Cooling | Water through the tube, mandatory |

**MR-05 revised 2026-08-09.** The original 85–95 kHz window assumed a 2 µF tank.
With the confirmed **3.96 µF** (12 caps), 1.8 µH resonates at ~59 kHz. Do **not**
reduce turns to chase the old window — see §2.1: keeping eight turns is what
drops per-capacitor current 28%.

The briefing's "1.5 µH benchmark" was always a proxy for "land near 90 kHz on the
stock tank." With twice the capacitance, the inductance target holds and the
frequency target moves. Inductance is the real specification; frequency is the
observable that confirms it.

### 6.2 Stop / focus discs *(revised from the briefing, then narrowed by §4.3)*

| MR-07 | Discs serve as **mechanical stop and shank shield only** — not as power adapters. §4.3 shows the fixed coil already self-compensates across 1/8"–1/2". Two discs (small/large) are expected to suffice; confirm in VR-07 before making a full set. |
| MR-08 | Continuous copper ring — no radial slit (a slit defeats the shielding eddy current) |
| MR-09 | OD only slightly larger than the chuck nose; oversize steals field from the work |
| MR-10 | **Mica washer on both faces of the disc** |

Rationale for MR-10: copper shields by eddy-current reaction, which means it
absorbs power and gets hot. It is also the highest-conductivity material in the
assembly. Placing it bare between a 300 °C nose and the carbide closes the
magnetic path and opens an excellent thermal one. The source briefing specifies
ceramic only *below* the disc (to protect the plastic housing) — insulation is
needed on **both** sides.

### 6.3 Coil former and carriage

| MR-11 | Former machined from **Garolite G-11** (180 °C continuous), not 3D printed |
| MR-12 | Printed plastic permitted only for the outer carriage, remote from the nose |
| MR-13 | Vertical travel with hard stops preventing coil-to-holder contact |
| MR-14 | Coil concentric to holder centerline for *uniform heating* |

Rationale for MR-11: ASA's glass transition is ~105 °C; the briefing's own
expected housing temperature is 121–149 °C. The part is over its limit before
radiant load is counted. A G-11 former is an afternoon on the 1100MX.

Note on MR-14: coil concentricity does **not** set tool runout — that comes from
the holder's ground bore and the shank. The briefing's attribution of the <3 µm
runout spec to machine alignment is incorrect. Budget accordingly: cheap linear
motion is fine, precision rails are not where the money belongs.

### 6.4 Length preset and hold-down

| MR-15 | Adjustable depth stop setting shank insertion length before heating. Range must cover **15 mm (1/8") to 47 mm (1/2")** of bore engagement per §3. |
| MR-16 | ~~Spring-loaded hold-down~~ — **downgraded to optional.** System is TSC; a drilled coolant-through retention knob vents the bore and removes the vapor-lock mechanism. Build only if a solid knob is ever fitted, or if ejection is observed in practice. |
| MR-17 | Holder nest locating on the BT30 flange, height-adjustable to center the nose in the coil |

Vapor lock: sealed air in a blind bore expands and pushes the tool out during
the seconds before the steel grips. Moot on vented/through-coolant holders — see
OQ-1.

**The drilled knob earns its place twice, and the second reason is not about
venting.** Scott, 2026-08-23: *"the TSC studs are to allow for a welding wire to
be run up through the holder during heat shrink removal of a tool."* The through
passage is the only way to push a shank out from behind when it will not release
from a hot holder. So a solid knob does not merely re-introduce vapor lock and
re-instate MR-16 — it also removes the recovery path for a stuck tool, with no
substitute. **Fit drilled knobs to every shrink-fit holder.** Note this is
unrelated to coolant: the 1100MX has no through-spindle coolant at all.

### 6.5 Cooling station

| MR-18 | Split finned aluminum contact sleeves, separate station from the coil |
| MR-19 | Forced air over the sleeves |
| MR-20 | Threaded compression or quick-disconnect fittings throughout — no slip-on vinyl |
| MR-21 | Coolant loop physically bulkheaded from the electrical enclosure |

Water quenching the nose is metallurgically harmless at 300 °C — far below H13's
~1010 °C austenitizing point, so no phase transformation occurs. The real
objections are thermal-shock fatigue over hundreds of cycles, corrosion in the
bore and taper, and shocking the carbide and its coating if the tool is still
installed. Sleeve + air is the default; a nose-only dunk is acceptable practice.

---

### 6.6 Switching and protection

| MR-22 | ~~Freewheel diode across the ZVS board's DC input~~ — **CLOSED 2026-08-27. Satisfied by the board as shipped.** No part to fit. |
| MR-23 | **REVISED 2026-08-30 — see §6.9.** Cycle switching by **DC SSR** (≥60 V, ≥80 A); isolation and E-stop by an **AC mains contactor**, not a DC one. |
| MR-24 | Coil axial position repeatable to **±0.5 mm** relative to the holder nose. |
| MR-25 | Coil leads as short as practical; coil terminals close to the tank, board outside the radiant field (SR-06). |

**MR-22 REVISED 2026-08-27 — the board is already protected.** Inspection of the
underside found **six FR307 (3 A, 1000 V, ~500 ns) in parallel directly across the
DC+ / DC− rails**, cathodes to DC+. That is a catch-diode array, and it is
correctly sized: a catch diode never carries running current, only the choke's
collapsing field. Surge rating is **150 A each = 900 A for 8.3 ms** against a
~58 A demand lasting a few ms — **16× margin**, and ~1.4 mW average. Six small
fast diodes beat one large slow one here, and reverse-biased across the rails
they also crowbar a reversed supply.

**What survives:** the board's diodes clamp the *board*. Our SSR sits **upstream**,
and the cable between SSR and board is its own inductance — roughly 2 µH over a
2 m run, ~3.4 mJ at 58 A, appearing as a spike **across the SSR**. So MR-22
becomes SSR-local protection. One of the five MBR60100PT still gets fitted, in a
different place and for a different reason.

**RESOLVED 2026-08-27 — MR-22 closes completely, no part to fit.**

The SSR is a **CoG ZGT-100 DD** (XIQI): output **100 A / 220 VDC**, control
3–32 VDC, load on the **positive** side. Two things follow.

*It has 4× headroom.* 220 V against a 53.5 V bus, not the 5–60 VDC that would
have made a transient dangerous.

*And it is already protected, because the onboard diodes clamp the FAR end of the
cable.* When the SSR opens, the chokes freewheel through the FR307 array, which
holds the board's input node at about −0.7 V. The cable current then decays into
those same diodes at 0.7 V / 2 µH = 350 kA/s, clearing 58 A in ~166 µs, with only
the clamp voltage across the cable. Worst case at the SSR is **bus + 1.4 V ≈
55 V** — a quarter of its rating. The 3.4 mJ of cable energy lands in the onboard
array, not in the SSR.

So the five MBR60100PT stay in stock as spares and come **off the BOM**. Nothing
is fitted.

*Noted in passing:* the supplied wiring drawing shows a "ZERO CROSS CIRCUIT"
block, which is meaningless on DC — a repurposed AC-SSR drawing. It matters only
because **CR-05 depends on a hard voltage step**; confirm on the scope during
bring-up that turn-on is a clean edge and not deliberately delayed.

**Original rationale.** The two DC feed chokes carry up to ~56 A. When the switch
opens, that current has nowhere to go and the collapsing field produces a large
forward spike. The machine terminates on **every dose** — thousands of
interruptions over its life. Without a freewheel path this erodes the switching
element and can push the spike into the IRFP260Ns, which hold only 24.5% margin
at 48 V before any transient. Found by studying reference R1
([design-references.md](design-references.md)); it was missing from every earlier
draft.

**MR-23 rationale.** A DC SSR switches in microseconds — a better hard step for
CR-05 than a mechanical contactor, with no DC arc erosion. But SSRs **fail
short** and leak when off, so an SSR alone cannot satisfy SR-03. Series both:
SSR for the thousands of cycle switchings, contactor for positive opening.

**MR-24 rationale.** §8.2 makes nest repeatability a *metrology* requirement.
Coupling varies with overlap, so ~2 mm of axial error on a 40 mm coil is ~5%
energy error ≈ 1.5 µm of apparent bore growth — the same magnitude as the damage
being detected. Sloppy fixturing does not merely cost a cycle; it destroys the
instrument.

### 6.7 Operational requirements

| OR-01 | **Insertion window.** Tool insertion SHALL complete within **5 s** of field-off, as a single motion against a preset depth stop (MR-15). |
| OR-02 | **Seized-tool recovery.** A documented procedure SHALL exist for a tool that grips partway in. |
| OR-03 | **Power loss mid-cycle.** On restart the controller SHALL assume unknown holder temperature, force a COOLDOWN pass, and log the cycle as aborted. |
| OR-04 | **Tool-side preconditions.** Shank clean, undamaged, burr-free, and within h6 before insertion. |
| OR-05 | **Calibration validity.** The VR-07 table SHALL be re-run after any change to coil geometry, tank capacitance, bus voltage setpoint, shunt, or holder set. |

**OR-01/OR-02 rationale.** Nothing in earlier drafts covered what happens
*between* field-off and the tool being seated. The bore begins shrinking
immediately, and a fumbled insertion leaves the tool gripped partway — an
unusable assembly that must be reheated, now starting from a hot holder with a
half-seated tool. The recovery procedure is: **reheat immediately at the same
dose and push through to the stop, or withdraw fully.** Do not let it cool
partway in, and do not add dose to force it.

**OR-04 rationale.** An out-of-tolerance shank changes interference, which
changes required dose — silently invalidating the calibration that is also the
bore-wear instrument (§8.2).

## 7. Safety requirements

| SR-01 | Grounded metal enclosure for all mains and high-current DC |
| SR-02 | **Local disconnecting means, in sight of the machine, that de-energises the AC side.** Cord-and-plug satisfies this if the receptacle is accessible. A local breaker or disconnect switch is required only if hardwired or if the receptacle is not reachable from the operating position. If a local device is fitted it MUST be **dual-pole** — see below. |
| SR-02a | **Label the AC input terminal marked `N` as LIVE, and re-identify the conductor landing on it.** Wording and method below. |
| SR-03 | **REVISED 2026-08-30 — see §6.9.** E-stop cutting the **AC mains contactor** coil, killing the rectifier and every downstream rail |
| SR-04 | Fused DC bus |
| SR-05 | No exposed conductor above 50 V with the lid closed |
| SR-06 | Coil and hot-holder zone physically separated from electronics |
| SR-07 | Operating position clear of combustibles; extinguisher within reach |
| SR-08 | High-temp gloves at the station |
| SR-09 | Documented warning: no watches, rings, or pacemakers near an energized coil |
| SR-10 | ~~Verify DC output bonding~~ — **CLOSED 2026-08-09.** No continuity from either DC rail to chassis. **Output floats.** |
| SR-11 | ~~Bond DC− to chassis/earth~~ — **WITHDRAWN as a requirement 2026-08-09.** Deferred to the phase-2 protection review. Design as floating; see below. |
| SR-12 | **Mains filtering on the AC input**, and the machine SHALL NOT run while the 1100MX is cutting. A kW-class 90 kHz oscillator sharing a panel with the PathPilot PC and probing electronics is a real interference source. |
| SR-13 | **As-built placard fixed to the machine**, carrying the constraints that cannot be inferred from the hardware. Content below. |

**SR-13 rationale.** Several facts about this machine are actively
counter-intuitive — the hardware suggests the wrong answer, not merely no
answer. They live in this document, which will not be present when someone opens
the enclosure in three years. Put them on the machine:

```
⚠  240 V — NO NEUTRAL.  Terminal marked "N" is LIVE (L2, ~120 V to ground).
⚠  DC BUS CEILING 48 V.  IRFP260N = 200 V parts. Do not raise. (VR-05)
⚠  NEVER ENERGISE WITHOUT THE WORK COIL CONNECTED. (board silkscreen)
⚠  E-STOP CUTS DC ONLY.  Mains and rectifier stay live. Unplug to isolate.
ⓘ  DC− is bonded to earth at ONE point. Do not add a second bond. (SR-11)
ⓘ  Cycles terminate on ENERGY, not time. Presets are calibrated — see docs.
```

Laser-mark it (xTool F1 Ultra) on anodised aluminium or stainless and rivet it
inside the enclosure lid, plus a short version at the AC terminal block per
SR-02a.

### SR-11 withdrawn — the safety case did not hold

**The chassis is earthed regardless**, through the AC cord's ground. That is the
protective earth. A DC bond adds nothing to it — it only decides where the DC
system sits *relative* to that earth.

| Fault | Floating | Bonded |
|---|---|---|
| Coil insulation → holder; operator touches holder and earthed machine | No direct return path — limited to leakage and capacitive coupling | **Full path through the bond, up to 168 V** |
| DC+ → chassis | Nothing happens; undetected | 57 A indefinitely, nothing trips |
| Mains L → chassis | Cleared by AC ground | Identical — unaffected by the DC bond |

For the dominant realistic fault — **coil to holder, where the sleeving is the
only barrier** — floating is safer. Bonding creates the shock path.

The bond's genuine justifications were **measurement convenience**: avoiding an
accidental bond through a scope ground clip, and simplifying low-side current
sense. Those are engineering conveniences, and earlier drafts framed them as
safety. They are not.

**Design as floating.** Coherent, not a compromise:
- Controller references DC− and floats with the system — which is what the INA228
  low-side sense wants anyway
- Isolated CAN transceiver keeps the rectifier link from bridging it
- **No USB while energised.** Program, disconnect, run
- Scope work wants a differential probe, not a ground clip

**Also note the DC breaker is a disconnecting means, not protection.** A 63 A
device on a supply that folds back at 57 A is correctly sized as a *switch* — it
should not trip in normal use. Reading it as failed overcurrent protection
mistakes its purpose. Real protection sizing belongs in the phase-2 review.

**Why it isn't bonded from the factory.** The R48-3000e3 is a telecom rectifier.
In a telecom plant many rectifiers parallel into a common bus and the bond is made
**once, at the battery plant** — if each rectifier bonded its own output you would
get multiple bond points and circulating ground currents. Floating is correct for
its designed use; the bond belongs to the system, not the component. The kit's
one-page reseller sheet omits it because it is assembly instructions, not an
installation guide.

**The hazard case is weaker than first stated.** 53.5 V DC floating sits below the
60 V DC extra-low-voltage threshold, and with a floating output a single fault to
chassis energises nothing. Bonding also *creates* a hazard — it puts the coil at
up to 168 V above earth (§SR-11 consequences).

**The real argument is that it will not stay floating.** A scope, a USB-powered
ESP32, and an earthed laptop each bond it at an arbitrary point through hardware
never designed to carry fault current. Choosing the bond point beats letting a
probe ground clip choose it. Absent any earthed instrument, floating would be the
better call.

> ### ⚠️ The 63 A DC breaker cannot protect against a downstream fault
>
> The PSU folds back at **~57 A**, below the breaker's 63 A rating. A bolted short
> downstream therefore sits at ~57 A **indefinitely** — the breaker never sees
> enough current to open.
>
> **The PSU's current limit is the actual protection.** Consequence: everything
> downstream must survive **57 A continuously**, not merely until a breaker clears.
> That includes the bond conductor — **8 AWG**, not 10 (10 AWG is ~55 A in chassis
> wiring, right at the edge of a fault that never ends).
>
> Revisit the breaker rating in phase 2; a device below the PSU's fold-back point
> would restore actual overcurrent protection.

**SR-11 rationale and consequences.** With DC− earth-referenced:
- Low-side shunt sensing sits at near-zero common mode, so an INA226/INA228-class
  front end works directly. (High-side would exceed a 36 V common-mode limit at a
  53.5 V bus.)
- USB into the ESP32 and a scope ground clip are both safe — the hazard that
  motivated the original SR-10 is removed, not merely characterised.
- Galvanic isolation on the CAN transceiver (CR-11) becomes belt-and-braces
  rather than load-bearing. Keep it anyway; it costs little.

**Do not skip the bond just because the supply floats.** Floating is what makes
the bond safe to add; it is not a substitute for having one.

**SR-02 rationale (revised 2026-08-09).** An earlier draft called for a dual-pole
AC breaker unconditionally. That was over-specified: the shop panel breaker
already protects the branch circuit, and the PSU draws ~13.6 A at 240 V
(~16.3 A at 200 V low-line), so a **NEMA 6-20 cord and plug is adequate and the
plug is the disconnect**.

What the panel breaker does *not* provide is a local isolation point. This
matters here because **the E-stop does not de-energise the AC side** — SR-03 cuts
the contactor coil, dropping DC to the ZVS stage, while the rectifier and all
mains wiring stay live. During commissioning, with hands near a live board
repeatedly, there must be an obvious local means to kill everything. A reachable
plug is that means. A receptacle behind the bench is not.

**Why dual-pole, if a local device is fitted.** On North American 240 V
single-phase both conductors are hot; there is no neutral. A single-pole device
leaves one leg live at 120 V to ground with the machine apparently off.

**SR-02a rationale.** The PSU's AC input block is labelled `E / N / L` — a
European L-N-E convention. On a 240 V circuit there is no neutral, so **the
terminal marked `N` carries a live conductor at ~120 V to ground.** Anyone who
services this machine trusting that label — including a future owner — gets a
shock from a terminal they believed was neutral.

**Label wording** (phrased so the reader need not work out which side was
repurposed):

```
⚠  240 V — NO NEUTRAL
   BOTH "L" AND "N" ARE LIVE
   120 V TO GROUND EACH
```

**The conductor matters more than the label.** A NEMA 6-20 cord is typically
black/white/green. Landing the white conductor on a terminal marked `N` states
"neutral" twice to anyone who looks. Standard practice for a white conductor used
as an ungrounded conductor is **re-identification at every termination** — black
or red tape or heat-shrink at both the plug and the terminal block. That travels
with the cord and protects someone who never sees the enclosure label. Better
still, use a black/red/green cord and remove the ambiguity at the source.

The source briefing's §2.6 insurance discussion doesn't change any of these —
they're correct practice regardless of jurisdiction or carrier.

---

## 8. Verification / acceptance tests

| VR-01 | **Idle current.** Empty coil, 53 V. Stable draw ~7 A. Materially higher → add turns. Run before any holder is inserted. |
| VR-02 | **Resonance.** Pickup loop + scope. 85–95 kHz loaded. |
| VR-03 | **Cap bank thermal.** Thermocouple on the tank caps. 10 consecutive 20 s cycles at 15% duty → stays below 70 °C. |
| VR-04 | **MOSFET thermal.** Same run, case temp within spec. |
| VR-05 | **Peak Vds.** Scope one FET. Expect ≈ π·V_bus ≈ 167 V. Confirm ≥25% margin to the device rating. |
| VR-06 | **Startup sequence.** 20 consecutive power cycles, no shoot-through, no blown fuse. |
| VR-07 | **Energy calibration.** Descending campaign — see §8.1. Builds the energy-vs-size table empirically. **This table is the machine's calibration.** |
| VR-08 | **Taper temperature.** During a full cycle, flange/taper stays below 80 °C. |
| VR-09 | **Repeatability.** 10 insert/remove cycles on one holder; stick-out variation and runout unchanged. |
| VR-10 | **Interlocks.** Verify CR-04, CR-06, CR-07 each trip on demand. |
| VR-11 | **Shunt calibration.** Determine `SHUNT_OHMS` by four-wire measurement, not from a nameplate. See below. |

**VR-11 — the shunt is unmarked, and no datasheet exists for it.** It is an
unbranded FL-2-style part. Its rating is set by convention rather than by a
document: PZEM DC meters with external shunts assume a **75 mV** full-scale
output and are configured only for the shunt's *current* rating (50/100/200/300 A).
That makes **0.75 mΩ** the strong default for a 100 A unit — a convention, not a
verified spec for this specific part.

**No external load is required to resolve it. The machine calibrates itself.**

If the shunt is actually 0.5 mΩ while firmware assumes 0.75 mΩ, the INA228 reads
**33% low** — the dangerous direction (R-4). But CR-03's cross-check tolerance is
20%, so a 33% disagreement against the rectifier's CAN telemetry **faults on the
first cycle**, and the disagreement ratio *is* the correction factor:

```
SHUNT_OHMS_true = SHUNT_OHMS_assumed × (I_ina228 / I_rectifier)
```

Procedure: leave `SHUNT_OHMS` at 0.75 mΩ, run the first commissioning cycle on
the instrumented dummy at 41.5 V, and either the channels agree (shunt is 75 mV)
or the fault reports the ratio needed to correct it. Two independent instruments,
no external load, no fixture.

This is safe because VR-07 already requires the first cycles to be on a dummy,
and because a low-reading sense chain is bounded by the absolute time ceiling.

**Faster, if a DC clamp meter is on hand:** clamp the DC lead during any cycle
and compare against the INA228 reading. One measurement, done. VR-01's idle-
current test (~7 A) also works as a low-current calibration point.

**Two second-order notes:**
- At 56 A a 0.75 mΩ shunt dissipates ~2.4 W and will run warm. If the element is
  manganin, tempco is negligible; a cheaper alloy could drift several percent.
  Give it airflow and calibrate warm rather than cold.
- **VR-07 absorbs a residual error anyway.** The campaign calibrates the whole
  chain end-to-end in joules, so a slightly wrong `SHUNT_OHMS` is folded into the
  measured dose values *provided it is stable*. Absolute accuracy matters less
  than repeatability — but get it roughly right so CR-03's plausibility windows
  (5–70 A) stay meaningful.

VR-03 note: the kit is rated for continuous metal melting; shrink-fit duty is
~10–15%. A cap bank that would fail in a furnace is likely fine here. Design
margin comes from duty cycle, not component ratings — which is exactly why VR-03
is measured rather than calculated.

---

### 8.1 Calibration campaign — descending order

**Sequence:** instrumented dummy → 1/2" → 3/8" → 5/16" → 1/4" → 3/16" → 1/8".

Rationale is not only that the large end is forgiving. Each size yields a
measured (nose OD, energy, ΔT) triple, so by 1/4" there are four points and the
coupling curve is *fitted from data*. The 1/8" dose becomes an interpolation off
a measured trend rather than an extrapolation from the crude (d/D)² model in
§4.3.

Descending order also front-loads the **electrical** stress testing: the 1/2" is
the highest-energy, longest cycle, and therefore the worst case for coil and
capacitor heating. Run VR-03 and VR-04 at this size.

**Per-size procedure:**
1. Establish a cold-bore reference — go/no-go pin feel (§8.2), not a measurement.
2. Thermocouple on the nose. Ramp dose upward from ~70% of the §4.3 predicted
   energy.
3. **Full cooldown below 60 °C between every attempt.** A 0.5 kg holder takes
   minutes. This wait is the safety margin — a ramp without it stacks heat and
   overshoots by attempt three.
4. Log per CR-10.
5. Re-check the pin feel cold at end of campaign, and compare release energy
   against the campaign's own first-cycle value.

### 8.2 Metrology — why not micron pin gauges

Detecting over-temper damage means resolving **2–3 µm** of bore growth: on a 1/2"
holder that is ~10% of the interference; on a 1/8" it is half of it.

Pin gauges cannot do this, for a reason that is about *increment*, not accuracy.
Class X pins hold ±1 µm, but sets step in 0.0005" (12.7 µm) or 0.01 mm. A 1 µm-
step set spanning 1/8"–1/2" would be ~9,500 pins. Micron-interval pin gauges are
not a product.

Direct alternatives, and why each is rejected here:

| Method | Resolution | Why not |
|---|---|---|
| Dial bore gauge | ~1 µm | Smallest range ~6 mm — cannot reach 1/8" or 3/16" |
| Split-ball small hole gauge | ±5 µm at best | Transfer gauge, operator-dependent — below the signal |
| Three-point internal mic (Holtest) | 1 µm | ~$400 each, narrow range, six needed |
| Air gauging | 0.1 µm | Master ring per size; far beyond budget |

**The machine is the instrument.** Release requires bore growth of
(interference + clearance). As interference decays, required ΔT and therefore
delivered energy fall:

```
1/2" : interference 0.025 → 0.020 mm  ⇒  21.8 → 18.5 kJ   (−15%)
1/8" : interference 0.006 → 0.003 mm  ⇒   5.7 →  4.5 kJ   (−21%)
```

That is ~3–4% energy change per micron — an order of magnitude more sensitive
than anything above, at zero added cost, measured on every cycle rather than
once a year.

**Recommended metrology stack:**
1. **Primary — logged release energy trend (CR-10).** Continuous, free, most
   sensitive. Valid only if start temperature and nest position are controlled;
   hence CR-04 and MR-17.
2. **Secondary — one Class X pin per size** (~$15 each) sized a few µm under the
   cold bore, used as a *feel* check at intervals. Resolves ~5 µm by hand. A
   coarse alarm, not a number, but it is independent of the primary.
3. **Tertiary — runout on a ground test pin**, indicated in the spindle. Uses
   equipment already on hand; catches grip degradation the energy trend might
   attribute to something else.

Absolute interference is not required to run the machine — only the trend is.
If a one-time absolute figure is wanted for validating §4.4, back-calculate it
from measured release temperature rather than trying to gauge the bore.

**Model check after each size.** Compare measured energy to §4.3 prediction:

| Observation | Meaning | Action |
|---|---|---|
| Within ~20% | Model holds | Continue down |
| Measured consistently **lower** | Coupling falls slower than modelled | Small sizes heat faster than predicted — tighten dose resolution, expect a short 1/8" cycle and reduced thermocouple authority (CR-02) |
| Measured consistently **higher** | Coupling falls faster than modelled | Small sizes will be slow; watch for cycles exceeding the stop rules below |

**Stop rules — defined before starting, per size:**
- Cycle exceeds **30 s** → heat is soaking the whole 0.5 kg holder rather than
  the nose; release may be unreachable without cooking it.
- Nose temperature to release exceeds **375 °C**, measured per §8.3 (drilled-
  pocket dummy: ±5 °C; surface probe on a real holder: ±20 °C — derate the
  trigger to 355 °C when using a surface probe).
- Bore grows measurably across the campaign.
- Release is not repeatable at the ceiling dose.

Any stop rule triggering means **that size is not viable on this machine** — use
ER11 or a collet holder for it. This is a legitimate outcome for 1/8" and
possibly 3/16" (§4.4, Risk R-1), and it is a decision to make from data rather
than to discover by scrapping a holder.

### 8.3 Temperature measurement — method and limits

Three independent effects make a contact thermocouple unreadable *during* the
cycle:

**1. Induced voltage.** A thermocouple is a wire loop sitting inside a kW-class
90 kHz magnetic field. K-type output is ~41 µV/°C, so 375 °C is about 15 mV.
Induced noise in that loop trivially exceeds the signal, and a MAX31855-class
front end will read garbage or saturate.

**2. Self-heating of the junction.** The bead is metal in a strong induction
field. It heats on its own and reads high — a well-known instrumentation trap in
induction work, independent of the noise problem.

**3. Ramp lag.** For a first-order sensor on a linear ramp, lag error = rate × τ.
At 350 K over 15 s the nose rises ~23 K/s, so even τ = 1 s reads **23 K low**. A
sheathed probe at τ = 3 s reads 70 K low. The original CR-02 would have "held"
350 °C while the nose passed 420 °C.

Any one of these kills real-time contact measurement. Together they are decisive.

**What works: read after field-off.** Kill the DC, wait ~200 ms for the field to
collapse, then sample. A 0.5 kg holder decays slowly, so this is within a few °C
of true peak. Sufficient for stop rules and for calibration — both are per-cycle
decisions, not mid-cycle ones.

**Where the accurate measurement happens.** On the instrumented dummy (§8.1), a
thermocouple in a drilled pocket in the wall reads bulk metal with no contact
resistance and τ ≈ 0.5 s — post-field accuracy ±5 °C. This is where the
energy→ΔT transfer function gets established, and it is the strongest argument
for building the dummy. On real holders a spring-loaded surface probe on the
nose end face is the practical compromise at roughly ±20 °C.

**Probe requirements:**

| TR-01 | Insulated-junction (ungrounded) probe — avoids ground loops with the induction hardware |
| TR-02 | Tightly twisted pair, routed radially outward, perpendicular to the coil axis |
| TR-03 | Junction at the nose **end face**, not mid-coil — lower field at the coil end |
| TR-04 | Amplifier input RC filtered; sampling gated to ≥200 ms after field-off |
| TR-05 | Dummy holder: 1.5 mm drilled pocket, ~3 mm deep in the wall, TC peened or thermally potted in |

**Optional upgrade — calibrated IR.** An optical pyrometer has no field coupling
and millisecond response, which would restore genuine real-time control. It
requires a high-emissivity target (high-temp flat black paint on a witness spot,
ε ≈ 0.95) — uncalibrated IR on bare machined steel is worth ±100 °C and useless
here. It also needs line of sight past the coil to the nose. Note that many
low-cost thermopile parts top out at 380 °C, exactly the region of interest;
specify range deliberately. Defer unless the energy-dose architecture proves
insufficient.

## 9. Accepted gaps

| Gap | Rationale |
|---|---|
| ~18–21 s cycle vs. commercial 5 s | Power-bound, and further bounded by the 48 V ceiling the IRFP260N imposes. Accepted. |
| No automated insertion | One operator. System is TSC, so vapor lock does not apply (OQ-1). |
| No real-time temperature protection | Not a choice — §8.3 makes it unachievable with a contact TC. Covered by CR-01 dose, CR-03 absolute ceiling + plausibility window, and CR-11's independent channel. |
| Open-loop coupling estimate | VR-07's descending campaign replaces every predicted dose with a measured one. |
| No certification | Managed by §7 practice. |
| Manual stop-disc changes | Two discs, seconds to swap. §4.3 removed the need for a full set. |

---

### 9.1 Risk register

| ID | Risk | Impact | Likelihood | Mitigation | Status |
|---|---|---|---|---|---|
| R-1 | 1/8" needs ~410 °C — at or above the damage window | Rapid holder wear, possibly unusable size | Medium | Descending campaign measures the clearance term; §8.1 stop rules drop the size if it trips | **Open** — 1/8" provisionally in scope |
| R-2 | IRFP260N 200 V gives only 24.5% margin at 48 V; π·Vdc is idealised and ringing adds 10–20% | Destroyed FETs on first power-up | **High** | Commission at 41.5 V; scope Vds (VR-05) before raising; MR-22 freewheel diode | **Open** — gates first power-up |
| R-3 | Tank cap current | Cap failure, possible cascade into FETs | **Low** (was Medium) | 12 caps confirmed → ~13.2 A each at 8 turns / 59 kHz, down 28% from the 6-cap assumption; ≤15% duty; VR-03 measured | **Reduced** 2026-08-09 |
| R-4 | Sense chain reads low while real power is high | Overheated holder, permanent bore growth | Low | CR-03 absolute ceiling + plausibility window + CR-11 independent channel | **Mitigated** |
| R-5 | Interference/clearance estimate (0.008 mm) is unsourced and drives the 1/8" verdict | Wrong scope decision | Medium | Measured as the y-intercept of ΔD vs diameter across the campaign | Open |
| R-6 | Coil coupling model is crude (d/D)² | §4.3 self-compensation may not hold | Medium | Model check after each size (§8.1); descending order surfaces divergence early | Open |
| R-7 | Holder material unknown | Ceiling may be wrong in either direction | Medium | OQ-3 email; interim 350/375; stop rules resolve it without a reply | Open |
| R-8 | Inductive kick on every cycle termination | Switch erosion, spike into FETs | High if unmitigated | Board carries 6× FR307 across the rails (900 A surge); MR-22 now covers the SSR-local case | **Mitigated at the board 2026-08-27; SSR-local open** |
| R-9 | B63 breaker nuisance-trips on contactor-close inrush | Commissioning friction | Medium | Accept; swap to C-curve if observed | Accepted |
| R-10 | EMI into PathPilot / probing | Corrupt probe cycles, mill faults | Low | SR-12 | Open |
| R-11 | **RF corruption of the current-sense chain.** §8.3 established a thermocouple cannot be read inside a kW-class 60–90 kHz field. The same physics threatens any unshielded sense lead — including the shunt pair that CR-01's dose depends on. Suspected in the first-light PZEM readings (§4.0). | Dose error → wrong holder temperature. Attacks the primary control law directly | **Medium** | INA228 differential input close-coupled to the shunt, twisted/shielded pair, RC filtering, common-mode choke, physical separation from the tank. Cross-check against CAN telemetry (CR-11), which is immune. AC-side clamp on the mains is the RF-immune reference during bring-up | **CONFIRMED 2026-08-27 — no longer a suspicion.** The PZEM read **29.7 V while a second meter at the same PSU terminals read 53 V+ throughout the run.** A 44% error on the voltage channel, in the direction that makes power look lower than it is. Every PZEM number taken near a live tank is now suspect until proven otherwise. |

### 9.2 Gap → requirement traceability

| §4 gap | Closed by | Verified by |
|---|---|---|
| 1 Raw power | Accepted — 240 V feed + 35 mm coil recover what is recoverable | §4.3 model check |
| 2 Control law | CR-01, CR-08, CR-10 | VR-07 |
| 3 Coil coupling | MR-01…MR-06 | VR-01, VR-02 |
| 4 Taper protection | MR-07…MR-10 | VR-08 |
| 5 Cooling | MR-18…MR-21, CR-06, CR-07 | VR-10 |
| 6 Length preset | MR-15, OR-01 | VR-09 |
| 7 Hold-down | MR-16 (optional — TSC, OQ-1 closed) | — |
| 8 Startup sequencing | CR-05, MR-23 | VR-06 |
| 9 Coolant interlock | CR-06, MR-20, MR-21 | VR-10 |
| 10 Enclosure / interlocks | SR-01…SR-12 | VR-10 |
| 11 Recipes / traceability | CR-08, CR-10 | VR-07 |
| 12 Throughput | Accepted (§9) | — |

## 10. Open questions

- ~~**OQ-1** — through-coolant?~~ **CLOSED 2026-08-09.** System is TSC capable and
  will run drilled coolant-through M12×1.75 knobs, venting the bore. Vapor lock
  does not apply; MR-16 downgraded to optional. Re-open only if a solid knob is
  fitted to a shrink holder.
- ~~**OQ-2** — which shank diameters?~~ **Answered:** 1/8, 3/16, 1/4, 5/16, 3/8,
  1/2" — six presets (CR-08). Note the 1/8" and 3/16" are offered in a 3.94"
  gage length as well as 2.56"; the nest (MR-17) must accommodate both.
- **OQ-3** — Holder material. Haas does not publish it. Email sent; awaiting
  reply. Governs whether the CR-02 thresholds (350 °C flag / 375 °C lockout)
  can be raised, held, or must drop. **Interim posture: hold at 350/375** — that
  sits inside the 250–400 °C industry window and is conservative for any
  plausible alloy.

  **Not load-bearing.** If Haas never answers, the §8.1 stop rules already
  resolve it: a size whose measured release temperature exceeds the ceiling
  fails its stop rule and drops out. OQ-3 improves the decision; it does not
  gate it. Do not wait on a reply to start the campaign.
- ~~**OQ-4** — 240 V available?~~ **CLOSED 2026-08-09.** Confirmed available.
- ~~**OQ-5** — controller platform?~~ **CLOSED 2026-08-09.** **Local ESP32
  firmware**, following the `bench-power-supply` pattern (PlatformIO, local
  control loop, hardware trip path). Consequences: CR-01 energy integration,
  CR-03 time ceiling, and the CR-09 abort path all execute locally with no
  network dependency. CR-10 logging may still publish to Home Assistant, but
  **telemetry only — no control path over the network.**

---

## 11. Sources

- [Haimer Power Clamp coil technology](https://www.haimer.com/en/products/shrinking-technology/application-and-know-how/the-coil-technology) — one coil + 5 stop discs, 3–32 mm
- [Haimer Power Clamp brochure (PDF)](https://www.productivity.com/wp-content/uploads/2020/06/Haimer-PowerClamp-2018-Interactive-1.pdf) — 10–20 kW, ~5 s heat, ~30 s cool
- [Haimer Power Clamp Basic manual (PDF)](https://www.haimer.com/HAIMER%20WEBSITE%20IMAGES%20AND%20CONTENT/HAIMER%20WEBSITE%20IMAGES%20AND%20VIDEOS/News%20&%20Media/Mediacenter/manuals/power-clamp-b/917000-0020_EN__B__.pdf)
- [Kennametal shrink-fit technical (PDF)](https://www1.mscdirect.com/images/solutions/kennametal/millingTechInfoShrinkfit.pdf) — process-controlled heating
- [Diebold shrink technology](https://www.hsk.com/en-us/products/shrink-technology/)
- [Eng-Tips: heat causing runout, 1045 vs H13](https://www.eng-tips.com/threads/is-heat-causing-runout-on-shrink-fit-tooling-1045-vs-h13.481088/) — holder material matters
- [Common issues in heat shrink mounting](https://www.hippsc.com/blogs/blog/common-issues-and-solutions-in-the-heat-shrink-tool-holder-mounting-process) — 250–400 °C window, permanent bore enlargement
- [Haas Tooling BT30 1/8" shrink holder 04-1424](https://www.haastooling.com/p/04-1424) — 15.0/20.0 mm body, 15 mm clamp length
- [Haas Tooling BT30 1/2" shrink holder 04-1434](https://www.haastooling.com/p/04-1434) — 27.0/32.0 mm body, 37–47 mm clamp, TSC, M12×1.75
- [Haas Tooling shrink fit holder range](https://www.haastooling.com/c/shrink-fit-holders)
- [Big Daishowa Shrink Fit Holder Slim](https://www.bigdaishowa.com/en/products/tool-holders/basic-arbors/shrink-fit-holder-slim) — slim-nose rationale
- [Practical Machinist: shrink holder temperature](https://www.practicalmachinist.com/forum/threads/shrink-holder-temperature.239813/) — slimline holders overheated due to low mass; ~5k cycle life
- Source briefing: `~/Downloads/diy-induction-ai-briefing.md` — ZVS latch-up mechanism, coupling estimates, vapor lock, builder logs
- Kit: [eBay 286924140726](https://www.ebay.com/itm/286924140726) *(specs unverified — see §2)*

## 6.9 E-stop moved to the AC side — revision of SR-03 and MR-23

**Decided by Scott, 2026-08-30: "I want all of the deadly current killed if I
have to hit the E-stop."** This supersedes the DC-side arrangement in SR-03 and
MR-23.

### What was wrong with the original

SR-03 cut a DC contactor coil, dropping the ZVS stage while **the rectifier and
all mains wiring stayed live** — §8.5 admitted this in as many words and answered
it with "a reachable plug is the disconnect." An E-stop that leaves 240 V energised
is answering the wrong hazard: hitting it during commissioning, with hands near
the board, leaves the most dangerous conductors in the machine untouched.

### Revised architecture

```
MAINS ─▶ [AC CONTACTOR] ─▶ R48 rectifier ─▶ [SSR] ─▶ ZVS board
              ▲
      E-stop NC in series with the coil,
      latched by an auxiliary contact
```

- **E-stop breaks the AC contactor coil.** Rectifier dead, DC bus dead with it.
- **Latching start/stop circuit:** start button energises the contactor, an
  auxiliary contact holds it in, E-stop or power loss drops it out and it stays
  out until someone presses start. **This satisfies CR-05's no-auto-restart
  requirement in hardware, with no firmware involvement.**
- **The controller drops the same coil** through a relay in that circuit, so a
  CR-03 time fault kills mains too — and a welded SSR is stopped by either path.
- **SSR unchanged**, handling the thousands of cycle switchings and normal aborts
  in microseconds without touching the contactor.

### Why AC is also the easier engineering

| | AC contactor | DC contactor |
|---|---|---|
| Arc quenching | current crosses zero 120×/s, self-extinguishing | magnetic blowout, **polarity-sensitive** |
| Failure mode if mis-wired | none | blowout drives the arc *into* the contacts |
| Rating to verify | ordinary | true DC breaking capacity at ≥60 V |
| Cost | ~$25 | ~$49 |

**Stored energy does not defeat it.** The R48's output capacitance is a few
thousand µF at 53 V — roughly 5 J, which a 1.7 kW load drains in about 3 ms.
Killing mains kills the bus.

### The DC contactor is retired

MR-23 justified it because *"SSRs fail short, so an SSR alone cannot satisfy
SR-03."* An AC contactor answers that completely: a shorted SSR is inert with no
mains behind it. **Removed from the BOM** — $49 and every DC-arc and polarity
consideration replaced by ~$25 of ordinary gear.

**What is given up:** cutting DC without restarting the rectifier. The SSR
already does that in microseconds and is the normal abort path, so nothing real
is lost. An E-stop costing a rectifier cold-start is correct behaviour.

**Still true and unchanged:** the cord-and-plug remains the physical disconnect
for service. An E-stop is not a lockout.

## 6.10 CR-05 revised — what delivers the step, and what gates arming

Two corrections, one inherited and one created by §6.9.

### The SSR delivers the step, not the contactor

CR-05's rationale is the load-bearing part and is unchanged: **a Royer needs a
hard voltage step to commutate. A slow ramp puts both MOSFETs into conduction
simultaneously.** No pre-charge, no soft-start on that line — the inrush *is* the
starting mechanism.

But CR-05's *text* said the contactor delivers that step, and **that stopped being
true when MR-23 was written** — MR-23 assigns cycle switching to the DC SSR
precisely because it "switches in microseconds, a better hard step for CR-05 than
a mechanical contactor." The two requirements have disagreed since.

§6.9 moved the contactor to the AC side without checking CR-05 first. **It
survives, but by luck rather than by analysis:**

```
AC CONTACTOR ─▶ rectifier ramps (slowly, harmlessly) ─▶ bus at 53 V
                                                          │
                                              SSR OPEN — ZVS sees nothing
                                                          │
                                        SSR CLOSES ─▶ hard step ─▶ ZVS
```

The ZVS never sees the rectifier's ramp because the SSR is open across it. Had
the SSR not already owned this job, moving the contactor to AC would have fed the
Royer a slow ramp and destroyed the IRFP260Ns.

### What gates arming

The original ≥30 s is a timer alone, and a timer cannot tell a slow rectifier
from a dead one. **Arming SHALL require all three:**

1. **Bus voltage ≥ threshold**, read over CAN
2. **Held across ≥M consecutive samples** — telemetry is ~1 Hz, so a single
   reading is a weak thing to arm a kilowatt on, and it may be a second stale
3. **≥30 s since contactor close**

**Neither condition is sufficient alone.** Scott, 2026-08-30: *"is there any
chance CAN reports full volts but it would sag too low if a cycle were allowed to
begin right then?"* Yes — four mechanisms produce exactly that:

- **Voltage arrives before capability.** Output capacitance charges to 53 V while
  the rectifier's current limit is still ramping. The bus reads full and cannot
  yet deliver 33 A.
- **Soft-start.** Telecom rectifiers ramp deliberately over seconds.
- **Telemetry lag.** At ~1 Hz the reading may predate the contactor settling.
- **No-load ≠ loaded.** The 53 V figure came from a warm, settled rectifier.

| Gate alone | Failure |
|---|---|
| Timer only | arms into a bus that never came up |
| Voltage only | arms into a rectifier still ramping its current capability |

**If the threshold is not reached within a timeout, that is a FAULT, not a
delay.** A dead rectifier and a slow one are identical to a clock, and only the
voltage condition separates them.

**Runtime counterpart:** bus sag below a floor *during* a cycle is a CR-03
plausibility fault. The startup gate stops a bad beginning; this stops a bad
ending.

### Power architecture (Scott, 2026-08-30)

1. **Master switch (DPST, maintained)** → 12 V logic rail **and** enables the
   contactor control circuit. Does not itself pull the contactor in.
2. **MAINS START (momentary)** → contactor pulls in, auxiliary contact latches it.
3. Controller waits on the three-condition gate above.
4. **CYCLE START** → SSR closes, hard step, heating begins.
5. **STOP** → SSR opens. Contactor stays in.
6. **E-STOP** → drops the contactor coil **only**.

**The E-stop SHALL NOT drop the 12 V rail.** A hot holder and a coil full of
static water with the pump stopped can boil that water; the FETs also hold heat
with nothing moving it. Cooling, logging and the CR-02 post-cycle temperature
reading all depend on the logic surviving the stop.

**The master switch must not latch the contactor.** A maintained switch holding
the contactor circuit means releasing the E-stop re-energises mains immediately —
an auto-restart, which is what the momentary-plus-latch arrangement exists to
prevent. Maintained is correct for the 12 V rail, where a reboot is harmless.

### 6.11 Power distribution — 24 V primary, split bucks (Scott, 2026-08-30)

**One AC-DC supply at 24 V**, feeding the contactor coil directly and bucking down
for everything else. Entirely from stock — **no new parts**.

```
MAINS ─▶ S-250-24 (24 V 10 A, WS2-S3)
             ├──▶ contactor coil (~0.3 A)  ── E-stop NC in series
             ├──▶ LM2596 ─▶ 12 V  logic + display   (~0.5 A)
             └──▶ LM2596 ─▶ 12 V  pump + fans       (~1.0–1.8 A)
```

**Two bucks, not one — deliberately.** A DC pump motor draws 3–5x its running
current at startup. Sharing a rail drags the logic supply down at exactly the
wrong moment, and a controller brownout mid-cycle is an uncommanded state with
the field on. **$5 of separation against an intermittent fault that would be
miserable to diagnose.**

**Bulk capacitance at the ESP32 regardless:** 220 µF electrolytic + 0.1 µF
ceramic close to the supply pin. Long buck-to-module runs sag on WiFi bursts.

**Why 24 V primary rather than 12 V:** no 12 V DC coil exists in the MC-9b family
(see bom.md), and a separate coil supply keeps pull-in inrush and collapse
transient off the logic rail — which matters because the coil takes a snubber
rather than a plain flyback diode.

**LM2596 limits worth respecting:** 3 A chip rating, **~2 A honest continuous**
on the little modules without added heatsinking; **Vin 40 V max**, so never feed
one from the 53 V bus. 24 V in is comfortable.

**Open — measure, do not model.** The 12 V pump's running current and startup
spike are the only numbers in this budget being guessed, across a 3x range, and
they size the buck that carries them. **Put the DC clamp on it.** If it runs
≤1 A it shares with the fans; if the inrush is heavy it wants its own, and the
XY6020L (6–70 V, 20 A, RB-07) absorbs it — check whether that one is spoken for
by another build first.

## 6.12 Controller board — DECIDED 2026-08-31 (closes OQ-5)

**Two boards, two purposes.** This was agreed in conversation on 2026-08-30 and
never written down; the rest of these docs still assume the Super Mini throughout.

### The test rig STAYS on the ESP32-S3 Super Mini

It works: OTA proven, TWAI listen-only at 125 kbps with zero bus errors, buttons,
status LED, duration ladder. **Do not disturb a working rig.** The Super Mini
footprint work — symbol verified against three independent sources, row spacing
*measured* at 15.2 mm — remains valid and in use. It simply does not carry to the
controller.

**Consequence: the INA228 I²C work goes on the S3 rig.** No conflict, nothing
waits on the board change.

### The controller PCB moves to a 38-pin ESP32 devkit

**~5 already in stock** (B3-R6C3 and RB-12). Reasons, in order of weight:

1. **The requirements are not stable enough to design pin-exact against.** In two
   days: CR-02 rewritten, SR-03 / MR-23 / CR-05 revised, PZEM retired, MAX31855
   and the DC contactor deleted, power architecture changed. **Spare GPIOs are
   cheap insurance against another week like that after the board is milled.**
2. **Pin budget.** ~22 needed (CAN 2, SSR 1, contactor relay 1, encoder 3, two
   start buttons, E-stop sense, 3 lamps, 1-Wire, flow, I²C 2, TFT SPI 6) against
   **13 usable** on the Super Mini.
3. **The shop already has a proven `NodeMCU-32S_38pin_THT` footprint.** An S3
   controller needs a new one, and **two footprints in this shop have been built
   mirrored.** That is a real risk the ESP32 sidesteps.
4. **Nothing here needs an S3-only feature.** TWAI, WiFi/OTA, I²C and SPI are all
   on the ESP32, plus a hardware **PCNT** quadrature counter for the encoder.
5. Scott's S3 boards were bought for voice-assistant work.

### Open before the PCB is laid out

- ~~IDENTIFY THE VARIANT FIRST.~~ **DONE 2026-08-31 — it is a NodeMCU-32S.**
  Silk reads **`5V`** (not `VIN`) and **P-prefixed** pin names (P15, P2, P0, P21,
  P22…), branded NODEMCU / ESP-32S / V1.1. **Both tells match, and this is the
  variant the shop already has a proven `NodeMCU-32S_38pin_THT` footprint for** —
  no new footprint to draw, and no exposure to the mirroring failure that has hit
  two footprints in this shop.
  **Mark the other boards in that bin.** There are at least four different 38-pin
  products across B3-R6C3 and RB-12; only this type matches the footprint, and a
  DOIT DevKit-V1 (silks `VIN`, `D`/`GPIO` names) grabbed by mistake and plugged
  into a board built for this one is a destroyed module.
  Pins confirmed present for I²C: **P21 = SDA, P22 = SCL**, 3V3 and GND on the
  opposite rail. SVP/SVN are GPIO36/39, the input-only ADC1 pins.
- **GPIO 6–11 are the internal SPI flash bus** → NPTH holes, no copper, and do not
  delete the holes or the module cannot seat.
- **ADC2 is dead while WiFi is active** → any analog input goes on ADC1 (GPIO 32–39).
- **GPIO 34–39 are input-only** — fine for buttons, flow and E-stop sense, useless
  for lamps.
- Firmware port is modest: pin renumbering, a different status-LED pin (GPIO48
  does not exist on ESP32), and serial via CP2102 rather than native USB. OTA and
  TWAI are identical APIs.

## 6.13 R48 CAN protocol — ***WITHDRAWN 2026-09-02, WRONG MANUFACTURER***

**Everything in this section is the Huawei R4850G2 protocol. Our rectifier is a
Vertiv R48-3000e3 — a different company's product entirely.** The IDs, the
encodings and the request frame below are all wrong for it. Superseded by §6.14.
Kept only so the mistake is legible.

### Original section, invalid:

## 6.13-OLD R48 CAN protocol — from the community, verified against source

Source: craigpeacock/Huawei_R4850G2_CAN (`r4850.c`, read 2026-09-02). Written for
the **R4850G2 (50 A)**; ours is the **R48-3000e3 (56.1 A)** — see the trap below.

**Bus:** 125 kbps, 29-bit extended IDs. The PSU breakout's 2-pin JST is CAN-H/CAN-L
and measured **55 kΩ = unterminated** — fit a **120 Ω** across it at our end.

### Request (send every 1 s — the rectifier answers, it does not volunteer)
`ID 0x108040FE`, DLC 8, all bytes 0x00.

### Telemetry response
`ID 0x1081407F`, DLC 8. `data[1]` is the sub-ID; **`data[4..7]` big-endian uint32,
divide by 1024.**

| `data[1]` | Meaning |
|---|---|
| 0x75 | **Output voltage** (V) |
| 0x81 | **Output current** (A) — *the README says 0x82; the code says 0x81. Settle on the bench against the DC clamp.* |
| 0x78 | Input voltage (V) |
| 0x73 | Output power (W) |
| 0x7F / 0x80 | Output / input stage temperature (°C) |

### Set commands
`ID 0x108180FE`, DLC 8. `data[0] = 0x01`. Value in `data[6..7]` big-endian.

| `data[1]` | Sets | Value |
|---|---|---|
| 0x00 | Output voltage, **online** (takes effect now, not persisted) | **volts × 1024** |
| 0x01 | Output voltage, offline (default at power-up) | volts × 1024 |
| 0x03 | Current limit, **online** | **(amps / MAX_CURRENT) × 1024** |
| 0x04 | Current limit, offline | same |

### TRAP — MAX_CURRENT is the rectifier's rating, and ours is not 50 A
The current command is a **fraction of the unit's maximum**, not amps. The
community code hard-codes `MAX_CURRENT 50` for the G2. **Our label says 56.1 A.**
Using 50 would mis-scale every current limit by ~12% — a commanded 30 A would
land near 33.7 A. **Settle it empirically:** command a limit, load the bus, read
the DC clamp, and adjust the constant until commanded equals measured.

### Firmware guard — not optional
**Clamp every voltage command in firmware to ≤ 48.0 V** (the IRFP260N operating
ceiling; peak Vds ≈ π·V, 16% margin at 53.5). The rectifier accepts up to 58.5 V,
which would put ~184 V on 200 V FETs with ringing on top. A typo in a serial
command must not be able to do that. Commissioning at 41.5 V.

**Use ONLINE commands only during bring-up.** Offline writes persist across
power cycles; a mistake there follows the rectifier to the next session.


## 6.14 R48-3000e3 CAN — the ACTUAL protocol (Vertiv, not Huawei)

Source: **`anikrooz/Emerson-Vertiv-R48`** (`src/VertivPsu.cpp`, read 2026-09-02) —
titled *"CAN bus control of Vertiv R48-3000e3 and possibly others"*, reverse
engineered from the **Emerson M820B** controller. Our exact model, named.

**How the wrong protocol got in:** §2.1 called this "a CAN-controlled telecom
rectifier" and the search that followed found the Huawei R4850G2, which is
extensively documented. **R48 and R4850 are different products from different
companies.** The datasheet says Vertiv on it; nobody checked.

### Bus
125 kbps, **extended 29-bit** — the bit rate is the one thing the two share.

### IDs (all extended)
| ID | Purpose |
|---|---|
| `0x0607FF83` | **set voltage / set current (amps)** |
| `0x0647FF83` | settings query |
| `0x0677FF83` | set current as a percentage |
| `0x06080783` | control |
| `0x0707FF83` / `0x0717FF83` | sync |
| `0x00860F8003` | init handshake reply — `rxBuf[3] == 0x58` |
| `0x00860F8007` | status/telemetry reply |

### Set output voltage — `0x0607FF83`
`{0x03, 0xF0, 0x00, 0x21, ...}`, value `(volts×10 − 320) × 0x40000 / 10`,
result across bytes 4–7 with `mesg[4] = 0x42`.

### Set output current — `0x0607FF83`
`{0x03, 0xF0, 0x00, 0x1A, ...}`, bit-packed, scaled `amps/5`.

### Telemetry — status frame, `data[0] == 0x11`
- **Vout** = `(rxBuf[1]<<8 | rxBuf[2]) / 1022`
- **Iout** = `(rxBuf[6]<<8 | rxBuf[7]) × 2`

**Note `/1022`, not `/1024`.** Not a typo in their code — worth confirming
against the KAIWEETS on first contact.

### It is a CONVERSATION, not a poll — this is the big behavioural difference
The Huawei answers a request. **This one requires an init handshake, then
periodic traffic on a ~6 s loop.** It gives up on you:

| Silence | Result |
|---|---|
| 20 s | `_acOff = 1` — **output drops** |
| 25 s | `_initialising = 1` — handshake must restart |

**Consequence for CR-01/CR-11: the controller must keep talking for the
rectifier to keep delivering.** A firmware hang doesn't just lose telemetry, it
drops the bus after 20 s. That is a *safety feature* we get free — but it also
means the CAN task can never be allowed to stall during a heating cycle.

### Still unknown, and it blocks everything
**Where CAN physically is on this unit.** The 2-pin JST was assumed to be
CAN-H/CAN-L and never confirmed (§2.2 said "confirm before connecting"). A forum
report says the JST is an **enable line — ground one wire to turn the supply on**.
The breakout card exposes only mains (3-pos), DC+/−, and that JST. **CAN may not
be broken out at all**, in which case it means tapping the rectifier's own edge
connector.

### Evidence from the bench, 2026-09-02
`tx_err=128, rx_err=0, bus_err ~800/s, rx=0` — the ESP32 transmits, **nothing
acknowledges**, it retries to error-passive. A CAN node ACKs any well-formed
frame regardless of ID, so **this is not the wrong-protocol symptom; it is the
no-node-on-the-bus symptom.** Two independent faults: wrong language, and
probably wrong wires.


### 6.14.0 The frames the source actually sends — filled in 2026-09-06

§6.14 above recorded the reply IDs but not what triggers them. Read from
`VertivPsu.cpp` directly and now implemented in `ina228_test.cpp` (Stage 1,
read-only):

| Step | ID | Data | Interval |
|---|---|---|---|
| handshake 1 (`sendSync`) | `0x0707FF83` | `04 F0 01 5A 00 00 00 00` | every 10 s until HELLO |
| handshake 2 (`sendSync2`) | `0x0717FF83` | `04 F0 5A 00 00 00 00 00` | with handshake 1 |
| `gimme5` | `0x06080783` | `20 F0 00 80 00 00 00 00` | with handshake |
| **HELLO** (reply) | `0x00860F8003` | `data[3] == 0x58` first contact, `0x5D` on recovery | — |
| control (keep-alive) | `0x06080783` | `00 F0 <ctrl> 80 00 00 00 00`, `ctrl` = dcOff<<7 \| fanFull<<4 \| flashLed<<3 \| acOff<<2 \| 1 → **`0x01`** = DC on, fan auto, AC on | every 6 s |
| settings query (keep-alive) | `0x0647FF83` | `03 F0 00 08 3F 94 62 9B` | every 6 s |
| `data[3] == 0x23` from R48 | any | resend both handshakes | — |
| `data[3] == 0x5B` from R48 | any | flag only (`got5B`) | — |

**Telemetry** is any frame with `data[0] == 0x11`; the source does not filter by
ID. `Vout = (d[1]<<8 | d[2]) / 1022`, `Iout = (d[6]<<8 | d[7]) × 2`. **Both
constants are from the source and unverified here** — the firmware prints the
raw bytes beside the decode so first contact can settle them against the meter.

**Reconciliation with §6.15.** The source's set-voltage encoding
`(V×10 − 320) × 0x40000 / 10` with `msg[4] = 0x42` produces **exactly the
IEEE-754 float bytes** — 52.0 → `42 50 00 00` either way. It is a mantissa
shortcut valid for 32–64 V, not a different encoding. §6.15's float description
is the correct general form and is what the firmware uses. Its current-percent
formula does *not* reproduce §6.15's verified examples and is disregarded.

**Conflict on `0x1A`.** The source calls it "set current in amps"; the forum
(§6.15) calls it "AC input current limit". §6.15's decodes are verified, the
source's are not; `0x1A` is **not implemented** in either sense.

**Firmware modes (Stage 1):** `LISTEN` at boot transmits nothing and prints every
frame with its ID; `r48init` starts the handshake and keep-alive; `r48stop`
returns to listen. Set commands exist only behind `-DR48_STAGE2`, with the
48.0 V clamp applied before encoding and OFFLINE (`0x24`) deliberately absent.

### 6.14.0a FIRST CONTACT — 2026-09-07, ~12:40. The R48 talks.

Dev ESP32 + Waveshare on a USB brick, driven over telnet, `CANH`/`CANL` only to
the JST, **no ground wire**. 60 s in LISTEN: silence — **the rectifier does not
broadcast until spoken to.** `r48init`: **160 frames in 40 s, tx_err=0
rx_err=0 bus_err=0.** `Vout 53.10 V` decoded against **53.4 V on the meter** —
the `/1022` divisor holds to 0.6 %. Output unloaded, `Iout 0.0 A`; the `×2` is
still unverified and needs a known load.

**Corrections to what the source recorded, measured on the wire:**

- **Reply IDs are `0x060F8003` and `0x060F8007`**, not `0x00860F80xx`. The
  source's values carry the MCP2515 library's extended-frame flag (bit 31); a
  29-bit ID cannot exceed `0x1FFFFFFF`, so they could never match. Our TX IDs
  never had the flag, which is why the R48 heard us before we recognised it.
  The sync reply is `0x0707F803` (our `0x0707FF83` with the low nibble pattern
  swapped — `FF83` → `F803`).
- The handshake acknowledgement arrived as **`0x060F8003` with `data[3] = 0x5D`**
  (the source's "recovery" value), not `0x58`, on first contact.
- **`data[0] = 0x41` info frames** on `0x060F8007`, sub-ID in `data[3]`: seen
  `0x51, 0x55, 0x56, 0x5A, 0x5B, 0x5C`. **`0x5A`–`0x5C` are the model string in
  ASCII**, four chars each: `081R` `4830` `00e3` → **"R48-3000e3"**. The rest
  are unknown and logged raw.
- `data[0] = 0x11` status frames arrive as documented; `Vout` in `data[1..2]`.

**Polarity.** With `CANH`/`CANL` reversed the bus went **BUS-OFF** (52 bus
errors in 45 s), not error-passive — the rectifier hears an inverted frame as
malformed and throws error flags at it, which our controller reads as bit
errors. I had argued a swap could only produce silence; it produces bus-off.
**Bus-off on first contact with an unknown node means try the other polarity
before anything else.**

**Ground.** None. The JST's CAN ground is isolated (6.14.1b); the Waveshare's
`GND` was left unconnected and the link is clean. The earlier `DC−` tie was for
a transceiver assumed to sit on the output ground — it does not.

### 6.14.0b TELEMETRY DECODE VERIFIED under load — 2026-09-07 ~13:20

ZVS short cycle (2 blinks = 10 s) on the shrink-fit coil, R48 feeding it, DC
clamp on the bus, status frames captured at ~2.5/s over telnet.

| | raw | decode | meter |
|---|---|---|---|
| Vout unloaded | `0xD400` = 54272 | **53 V** (`raw/1024`) | 53.4 V |
| Vout in load | `0x5400`/`0x5000` | **21 / 20 V** | — |
| Iout peak | `0x86BA` = 34490 | **67.4 A** (`raw/512`) | **68 A** |

- **Vout is integer volts.** Every raw value is a multiple of 1024; the source's
  `/1022` was an approximation. Resolution 1 V — coarse, adequate for CR-11.
- **Iout = raw / 512.** The source's `×2` omitted a `/1024`. Agrees with the
  clamp to under 1 %.
- **Current telemetry lags ~2–3 s on the fall** (36 k → 700 raw over 2.4 s after
  Vout had already recovered) — a filter inside the rectifier. CR-11's
  disagreement window must be wider than that or it will fault on every cycle end.
- **Operating point confirmed: the R48 current-limits at ~68 A and folds to
  20 V** into the ZVS. That is the August bench measurement (67.3 A at 18 V,
  §4.x) reproduced from the rectifier's own telemetry. The 62.5 A datasheet
  figure is ~8 % under what it actually delivers, on both instruments.
- Ramp: 0 → 68 A in ~4 s; Vout collapses within the first second.

### 6.14.0c STAGE 2 — ONLINE voltage command and its timeout, measured 2026-09-07 ~14:00

R48 unloaded, meter on `DC+`/`DC−`, session in `RUN`, keep-alive every 6 s.

| Step | Sent | Telemetry | Meter |
|---|---|---|---|
| 1 | `r48v 45` → `0x0607FF83` `03 F0 00 21 42 34 00 00` | 53→45, **1 V per frame, ~2.5 V/s** (walk-in) | **45 V** |
| 2 | nothing (10 s refresh running) | 45 for 35 s, 89 frames | 45 V |
| 3 | `r48stop` — total silence | **45 held 36.8 s, then 46→53 over 3.2 s; 53 at 40.0 s** | back at 53; "roughly 5 s" for the walk-up by eye |
| 4 | `r48v 50` — deliberately over the clamp | wire got `42 40 00 00` = **48.0**; status `hold V=50.0` records the ask | **48 V** — fan did *not* ramp (5 V step vs 8 V), amber off |

- **IEEE-754 encoding confirmed on the wire**: `42 34 00 00` = 45.0, the rectifier obeyed it.
- **Setpoint changes are slewed at ~2.5 V/s in both directions** — the walk-in
  feature. A step command is not a step at the output. CR-01 timing must allow
  ~3 s for an 8 V change.
- **ONLINE timeout measured at ~37 s of controller silence** (forum said 30). It
  is a real, unconditional revert to the rectifier's own default (53 V). This is
  the property that makes ONLINE-only firmware safe: a hung controller cannot
  leave the bus at a commanded voltage for longer than 37 s.
- **The rectifier keeps streaming status after the controller goes silent** —
  139 frames in 55 s of silence. The source's "20 s → acOff / 25 s → re-init"
  are the *controller's* internal flags, not rectifier behaviour. Telemetry is
  free even when we are not talking.
- **The clamp is proven on the wire.** A request for 50 V left the board as
  48.0; the held value records what was asked, the send path records what was
  allowed. No command string reaches the encoder unclamped.
- **Long hold, 14:03–14:05:** 45 V held **99 s** with refresh + keep-alive running,
  meter and telemetry agreeing — the setpoint survives indefinitely while the
  controller talks. Then **refresh OFF, keep-alive only, 14:05:22–14:07:10:
  still 45 V after 108 s.** So the ~37 s revert is a **session timeout, not a
  setpoint timeout**: the rectifier drops the setpoint when the controller goes
  quiet, not when the setpoint stops being repeated. The 10 s refresh in the
  firmware is therefore redundant — kept as belt-and-braces — and **CR-01's only
  hard obligation is to keep the session alive (any traffic inside ~37 s).**
- **Fan ramps on a setpoint change** while unloaded and returns to normal after
  the revert. **The amber LED blinks *after* a state change ends** — after the
  revert here, after a load cycle earlier — and is off during the commanded
  state. That reads as Vertiv's "alarm logged and cleared" indication, not an
  active fault; the exact meaning is an open item for the manual.

### 6.14.0d STAGE 2 — ONLINE current limit under load, measured 2026-09-07 ~14:40

`r48i 50` → `0x0607FF83` `03 F0 00 22 3F 77 95 0B` (0.967 of a 51.7 A base),
then one ZVS cycle (2 blinks), DC clamp on the bus, meter on the output.

| | Commanded | Telemetry (`raw/512`) | Clamp | Uncommanded (§6.14.0b) |
|---|---|---|---|---|
| Current | 50 A | **55.6 A** peak, plateau 54–55.6 | **55 A** | 68 A |
| Bus voltage | — | **15 V** | 15 V | 20 V |

- **The ONLINE current command works**: 68 → 55 A. Plateau reached in ~2 s.
- **The 100 % base is ~57 A, not 51.7.** Two independent estimates: 55.6 / 0.967
  = **57.5 A** from this test; 68 / 1.21 = **56.2 A** from the uncommanded limit.
  57 A ≈ 3000 W / 53 V — **the rectifier's 100 % is rated power at nominal
  voltage.** The datasheet's 62.5 A is not this unit's 1.21 value; its actual
  1.21 value is the 68 A we measure. §6.15's 51.7 A is superseded; firmware
  `R48_I_BASE = 57.0`. One point only — linearity (gain vs offset) is unverified
  until a second command level is measured.
- The ZVS ran cleanly at 15 V / 55 A. Its current tracks bus voltage roughly
  linearly (68 A at 20 V, 55 A at 15 V), as expected for a Royer.
- Telemetry current filter on the fall again: ~3 s from 55 to 0 after the bus
  had recovered.

**Second point, `r48i 45` (f = 0.789 of the 57 A base) — NOT clean:**

| | Telemetry | Clamp | Bus |
|---|---|---|---|
| Plateau | 44.5 → **48.2 A**, rising through the cycle | **"44ish"** | **9–11 V** |

- Telemetry and clamp disagree by ~9 % here, where they agreed within 1 % at
  68 A / 20 V and 55 A / 15 V. Either the clamp was read early in the plateau
  (44.5 at t+1.5 s, 48.0 at t+5 s — the telemetry filter settles slowly), or the
  rectifier's current telemetry over-reads at low output voltage. Unresolved.
- Implied base: **61.1 A** from telemetry, **55.8 A** from the clamp. Three of the
  four estimates now cluster at 56–57.5; the telemetry one at 48 A is the outlier.
  A two-point line through the telemetry values gives a +15 A offset, which is
  not physical. **The 57 A base stands as the best single estimate; linearity is
  unproven.**
- **The bus folded to 9 V** — below the ~12 V floor set for the ZVS gate drive.
  It ran, but this is the regime to stay out of. Any further point goes *up*
  (`r48i 62`, f = 1.09, bus ~17 V), never down.

**RESOLVED (Scott, from the bench):** the clamp was not read early — it **hunted
43–48 A** through the cycle, the same band as the telemetry's 44.5–48.2. The two
instruments agree; **the rectifier's current-limit loop oscillates at a 9 V bus
into the ZVS**, which is a resonant load at its gate-drive floor. Unstable
operating point, not a measurement error and not a nonlinear command. Mean of
the band → base **~58 A**. Four estimates now: 57.5, 56.2, 58.3, and 3000 W/53 V
= 56.6. **Base ≈ 57 A, gain, no offset.** `R48_I_BASE = 57.0` stands.

**Design constraint for CR-01, from this:** the current limit cannot be used to
throttle *this* coil below ~45 A, because the bus folds under ~12 V and the CC
loop hunts. Fine power control below that point comes from **duty cycle**
(on-time per CR-01 dose), not from the current setpoint. The current limit is a
ceiling and a coarse knob, not a fine one.

**Third point, `r48i 62` (f = 1.088) — clean:** telemetry **60.1 A**, clamp
**60 A**, bus **17 V**, steady, no hunting. Base from this point 55.1 A.

**Calibration, final:** stable points at f = 0.967 → 55.6/55 A and f = 1.088 →
60.1/60 A, plus the uncommanded limit 68 A = 1.21. Base estimates 55.1–57.5; a
line through two points only 12 % apart amplifies noise into a spurious offset,
so: **base = 56 ± 1.5 A, linear to ~3 A over 45–62 A, no demonstrated offset.**
68/1.21 = 56.2 is the cleanest single measurement and equals 3000 W / 53 V.
`R48_I_BASE = 56.0` in source — **not yet on the board** (the OTA was aborted
mid-invitation; see TRAPS). Flashes by USB.

**CC-loop stability range, measured:** steady at 17 V / 60 A, hunting at 9 V /
45 A into this coil. **The current limit is a usable knob from ~50 A up to the
68 A default — bus 15–20 V, roughly 750–1360 W. Below that, duty cycle.**

**Stage 2 is verified for voltage, hold, revert, clamp, and current limit —
every one on the wire, on telemetry, and on a meter.**

**Controller footprint (C6), Scott 2026-09-07:** leaning **SOIC on the PCB** —
the Waveshare's 4-and-2 pin arrangement will not sit cleanly on a board. Decision
deferred until the design is closer; the LCSC order follows the decision.

### 6.14.0e The R48 JST harness — wire colours, recorded 2026-09-07

Scott made a harness for the rectifier's 2-pin JST. **The polarity that works
is the one measured today** (reversed gives bus-off, §6.14.0a):

| Wire colour | Signal |
|---|---|
| **Yellow** | **CAN H** |
| **Blue** | **CAN L** |

No ground wire — the JST's CAN ground is isolated (§6.14.1b) and the link runs
clean without one. If the bus ever goes bus-off on first contact after a rewire,
check these two before anything else.

### 6.14.0f The 0x41 info frames DECODED — four new channels, 2026-09-07

The rectifier streams `0x41` frames on `0x060F8007` with a sub-ID in `data[3]`
and a **big-endian IEEE-754 float in `data[4..7]`** — the same encoding as the set
commands. Identified by commanding known values and watching them come back,
which validates the whole set rather than guessing:

| Sub | Meaning | Evidence |
|---|---|---|
| **0x01** | **Output voltage setpoint** | mirrored 41.53 / 48.06 / 41.80 / 42.51 / 43.01 as commanded |
| 0x02 | Output current | 0 at no load |
| **0x03** | **Current limit, fraction of rated** | read back **0.982422** against our commanded 55/56 = 0.982 |
| **0x04** | **Internal temperature °C** | steady 19.85–20.09 across every setpoint; shop ambient |
| **0x05** | **AC input voltage** | 241.8–241.9 V — the shop's 240 V mains |
| 0x07 | unknown, 0 | |
| 0x5A–0x5C | model string ASCII | `081R` `4830` `00e3` = R48-3000e3 |

**This matters beyond diagnostics.** `0x04` gives CR-07 a coolant/thermal channel
the design did not otherwise have, and `0x05` makes a mains sag visible — both
from a source completely independent of our own sensing. All four are now in
`R48::Telem`, shown in `st`, and **`rectC` and `acV` are written into every cycle
log line** beside the dose.

### 6.14.0g Fan behaviour — tracks the setpoint, graduated, measured 2026-09-07

Scott noticed the fan running full and asked what drives it. Measured at **zero
output current** with temperature flat at ~20 °C throughout:

| Setpoint | Fan |
|---|---|
| 41.5 V | full |
| 41.8 V | full |
| 42.5 V | quieter, not much |
| 43.0 V | slightly quieter again |
| 48.0 V | **much lower** |

**Graduated, not a threshold.** Not thermal (0x04 never moved), not load (0 A),
and **not the `0x33` fan command** — sending explicit `auto` changed nothing, so
the unit was already in auto or `0x33` does not behave as §6.15 describes.

**Design consequence.** `BUS_SETPOINT_V` is 41.5 V for FET margin (peak Vds
≈130 V, 35%); the ceiling is 48 V (≈151 V, 25%). **Quiet and safe pull in
opposite directions.** Fan noise is an annoyance; FET margin is what destroys
IRFP260Ns. Stay low until the ZVS has been scoped at voltage (VR-05), which is
the measurement that would justify running higher.

### 6.14.1 The 2-pin JST is NOT CAN — measured 2026-09-02

**R48 on mains, DC output open, transceiver unplugged, black probe on DC−:
both JST pins float below 0.2 V.**

A recessive CAN pair idles at **~2.5 V on both lines** — that is how the
transceiver biases itself when nothing is transmitting. Two pins near zero means
nothing is driving them. **It is not a CAN bus.**

This kills the §2.1 assumption, which reasoned from "2-pin, on signal fingers, on
a CAN-managed rectifier ⇒ almost certainly CAN-H/CAN-L" and said *confirm before
connecting*. It was never confirmed, and firmware was written against it. The
likely真 function is the **enable/inhibit line** — a forum report says grounding
one of its wires turns the supply on.

### Where CAN actually is

**On the rectifier's edge connector, and the breakout card does not pick it up.**
From the Endless Sphere thread: a user *"modified a connector unit to add a screw
terminal block for the CAN."* People add the tap; the cards do not ship with it.

**Procedure to find it:**
1. Inspect the card's gold fingers under the microscope. Trace which run to the
   mains block, the DC lugs and the JST. **Fingers with no trace leaving them are
   the candidates.**
2. R48 running, probe those fingers against DC−. **CAN is a pair sitting together
   at ~2.5 V** — unmistakable once found, and absent everywhere else.
3. Solder two wires to that pair. That is the whole modification.

Back-probe from the card's solder side; the fingers are inside the connector once
the card is seated.

### 6.14.1a Third measurement, 2026-09-07 — the JST floats relative to the output

R48 on mains, output up at **53.4 V** (`DC−`→`DC+`), **nothing** attached to the
JST. Both JST pins read **0 V against `DC−` AND 0 V against `DC+`.** Two points
53 V apart cannot both be at the JST's potential; the only reading that
satisfies it is a **floating, high-impedance node** that follows the meter's
other lead. Anything referenced to the output — a CAN transceiver in standby
included — would read ~0 to one rail and ~53 V to the other through its own
input resistors. The JST is **not referenced to the DC output.**

This is a *different* fact from 6.14.1's "not an active bus" (which was measured
through a transceiver later shown unable to transmit, and proves nothing). It
leaves two candidates: an enable/inhibit line (per the forum report), or CAN on
an **isolated** control ground that is not brought to the breakout. The
discriminators are (A) resistance and diode-mode across the pair, both
polarities, R48 off — symmetric ~55 kΩ fits a CAN differential input,
asymmetric with a diode drop fits an opto — and (B) shorting the pair with the
R48 on: an inhibit drops the output; a CAN pair shorted is merely recessive.

**Consequence either way:** there is no output rail the Waveshare's `GND` can
safely be tied to. If the JST is isolated CAN, the only safe first contact is
an **isolated** adapter with *only* `CANH`/`CANL` attached — the CANable — whose
floating CAN side settles at the rectifier's reference through its own input
resistors. The dev ESP32 does not go near it until that reference is known.

### 6.14.1b RESOLVED 2026-09-07 — the JST is CAN, on an isolated ground

Tests from 6.14.1a, R48 off: **55 kΩ across the pair in both polarities, OL both
ways in diode mode.** Symmetric and resistive — a CAN transceiver's differential
input (spec 40–100 kΩ), not an opto. R48 on, output 53.4 V: **shorting the pair
changed nothing** — not an inhibit. With 6.14.1a's floating reading, the picture
is consistent and complete: **CAN, referenced to an isolated control ground that
is not on the breakout.** Scott's reading of the fingers was right; 6.14.1's
"not CAN" was measured through a transceiver that could not transmit.

Two consequences for wiring, both from the 55 kΩ:

1. **The port is unterminated** — a terminated one reads ~120 Ω. Normal for a
   rectifier built to sit mid-bus. On this bus **our end supplies the only
   terminator**: CANable switch **ON**. (OFF was for the two-Waveshare loopback.)
2. **No rail is a safe `GND` reference.** First contact is the **isolated**
   CANable with only `CANH`/`CANL` attached — its CAN side floats to the
   rectifier's reference through the input resistors. Which pin is H and which
   is L is unknown; a passive listen is harmless either way, so try one, then swap.

**Before any non-isolated node joins** (the dev ESP32 + Waveshare, even on a USB
brick): measure **each JST pin to mains earth** with the R48 on. Near earth
potential → the control ground is SELV and a floating brick-powered ESP32 is
acceptable. Tens of volts or more → the CAN circuit is primary-referenced and
the ESP32 stays off it until there is isolation between them.

### 6.14.2 ROOT CAUSE — the breakout card omits the comms interface by design

An Amazon listing for the same class of R48-3000e3 adapter says it plainly:

> *"The interface board is custom-made by our shop. It is **not the original
> factory interface board and does not have the original factory communication
> interface, only the input and output interface.**"*

**That is our card.** Mains in, DC out, and one 2-pin enable line — exactly what
was measured. The aftermarket cards drop the communication pins to save cost.

**Nothing was wrong with the transceiver, the wiring, the 220∥270 termination or
the firmware's bus setup.** `tx_err=128 / rx_err=0` was correct and honest: there
was no CAN node on those wires because those wires are not CAN.

### Options, in order of preference

1. **Source a factory interface board** (or an aftermarket one that explicitly
   lists CAN terminals). The listing's disclaimer proves a comms-bearing version
   exists. Cleanest — no modification, no probing near mains.
2. **Tap the rectifier's edge connector directly.** The socket carries ~40 signal
   contacts in two rows. **AC MAINS ENTERS THROUGH THIS SAME CONNECTOR** — the
   card's 3-position mains block proves it. Do not probe it live and blind.
   Requires the pinout, which is not published (searched: Vertiv datasheets, the
   user manual, both community repos, the Endless Sphere thread — none give pin
   numbers).
3. **Defer CAN.** The machine works without it; see the cost below.

### What deferring CAN actually costs

- **No throttle.** The bus collapses to 18 V under the 19.5 mm coil because
  nothing limits current. §6.11's current-limit protection is unavailable.
- **CR-01 energy dosing loses its independent voltage reference**, though the
  INA228 covers V and I on its own once calibrated.
- **CR-05's arming gate** (§6.10) wanted CAN-reported bus voltage. Without it,
  arming falls back to the ≥30 s timer alone — which cannot tell a slow rectifier
  from a dead one.

**None of these block tools going into holders.** They block the machine becoming
repeatable and self-protecting, which is Phase 2.

### 6.14.3 What the official datasheet does and does not say

**Vertiv R48-3000e3 datasheet (NetSure 731-A412 variant), "Control and Monitoring":**

> *"Rectifier Alarm and Signaling — **Alarm and status reported via CAN bus to
> system controller**"*

**CAN is confirmed factory capability**, not a community discovery. It reaches the
system controller through the card-edge connector — the only interface the module
has.

**Also from the spec table, and it corrects §6.14:**

| | |
|---|---|
| Output current | **62.5 A @ −48 VDC** |
| **Output current limit set point** | **0 to 62.5 A** |

The label's 56.1 A and this 62.5 A are the same 3000 W at different voltages —
it is a **constant-power** supply. **The CAN current-limit range is 0–62.5 A**,
which is the scaling reference the set command needs, not the label figure.

**What no document gives: the connector pinout.** Searched the Vertiv datasheet
(both variants), the user manual host, ManualsLib, Scribd, both community repos
(`anikrooz/Emerson-Vertiv-R48`, `PurpleAlien/R48_Rectifier`) and the Endless
Sphere thread across pages. **Not published anywhere reachable.**

### Why the forums do not have it either

**Because nobody there needs it.** Every community implementation buys an adapter
card that already brings CAN out to terminals, plugs in, and goes straight to
protocol. The threads are entirely about message IDs and encodings — there is no
pinout discussion because there is no pinout problem.

**Our card is the cheap variant that omits the comms interface.** That is why we
hit a wall the community never sees, and why the answer is a $6–10 part rather
than reverse engineering.

### Decision

**Buy an adapter card that exposes CAN.** They exist on AliExpress and eBay
(search "R48-3000e3 adapter board" / "interface board"), typically $6–15, and the
listings that include comms say so. Verify before ordering that the listing shows
**CAN terminals** — the one on Amazon (B0DMF91H91) states outright that it has
"only the input and output interface."

Do **not** probe the edge connector live: **AC mains enters through it**, and the
pinout is unknown, so there is nothing to probe safely toward.

### 6.14.4 The module manual confirms the CAN feature set — and explains the missing pinout

**Vertiv eSure Rectifier Module User Manual (UM1R481000e3, same family), Table 1.7
"Exchange of Information between Rectifier and Controller":**

| Commands the rectifier ACCEPTS | Telemetry it REPORTS |
|---|---|
| **Turn On/Off** | Input voltage, **output voltage**, **output current** |
| Current Walk-in On/Off | **Current limit setting**, over-voltage setting |
| HVSD (High Voltage Shutdown) Reset | Temperature, on/off status |
| **Current Limit Adjustment** | Fault alarms (HVSD, fan fail), protection alarms |
| **Voltage Regulation** | Thermal/AC derating, AC fail, address, code, versions |

**Everything CR-11 wants is a documented factory feature** — voltage regulation
and current limit adjustment are both in the accepted-command list. This is not a
community hack on an undocumented interface.

**Why no pinout exists in this manual:** §2.1 says *"Refer to the system
documentation supplied with the system the rectifier is installed in."* **The
module plugs into a NetSure shelf; the backplane pinout belongs to the shelf's
documentation, not the module's.** That is the structural reason this has been so
hard to find, and it means the remaining document to chase is the **NetSure
731-A412 (or similar shelf) manual**, not anything with "R48" in the title.

### Searched and exhausted
Vertiv datasheets (both variants), UM1R481000e3 full text (28 pp), ManualsLib,
Scribd, Manualzz, 48vrectifier.com, `anikrooz/Emerson-Vertiv-R48`,
`PurpleAlien/R48_Rectifier`, Endless Sphere thread (multiple pages), DIY Solar
forum. **No backplane pin assignment published anywhere reachable.**

### Scott's hypothesis is still open and still untested

**That the JST's two pins ARE CAN-H/CAN-L**, with the rectifier's transceiver
inactive standalone. **My "both pins below 0.2 V, therefore not CAN" was
over-concluded** — an unpowered or standby transceiver releases its bias and
floats near zero. And the **55 kΩ measured across the pair in August is
consistent with a CAN transceiver's differential input** (typically 20–40 kΩ);
an enable line would be a few kΩ to one rail, not tens of kΩ across a pair.

**The test that settles it, and it costs nothing:**
1. **Scope the pair** — DHO914, probe one JST pin, ground to DC−, 20 µs/div,
   R48 running. Any traffic at all means it is a live bus.
2. **Connect our powered transceiver and measure without transmitting.** The
   SN65HVD230 biases the pair to ~2.5 V. If it sits there cleanly, the lines are
   free; if something drags it toward zero, the R48 end is loading them.

### 6.14.5 Documentation route CLOSED — and there is nothing to buy

**NetSure 731 A41 shelf manual (38 pp, Vertiv-hosted) searched in full.** Its
only connector table (Table 2-4) covers AC and DC distribution terminals. **The
shelf manual treats the rectifier slot as a black box** — modules slide in and
work; no backplane pin assignment appears anywhere.

**The pinout is not published in any reachable document.** Both R48 datasheets,
the module user manual (UM1R481000e3, 28 pp, full text), the NetSure shelf manual
(38 pp, full text), four manual-hosting sites, both community repos, and two
forums. It is not a search failure at this point; the document does not exist in
public.

**And there is nothing to order.** The eBay "R48-3000e3 tail adapter" ($10.88,
3-week ship) is **the same power-only card Scott already has** — verified from
its product photo: two DC screw terminals, a 3-position mains block, no comms.
The Amazon equivalent states outright it has "only the input and output
interface". **No CAN-bearing adapter has been found for sale.**

### Which leaves exactly one path, and it is free

**Test Scott's hypothesis that the JST pair IS CAN-H/CAN-L.** It has never been
tested, only inferred against — and the inference was weak (see §6.14.4).

1. **Scope the pair.** DHO914, probe one JST pin, ground clip to DC−, 20 µs/div,
   AC coupled, R48 running. **Any activity means it is a live bus.** Flat means
   nothing is transmitting — which still does not prove it is not CAN, only that
   the rectifier is silent standalone.
2. **Bias test.** Connect the powered SN65HVD230 and measure the pair **without
   transmitting**. Ours pulls both lines to ~2.5 V. **Clean 2.5 V = the lines are
   free.** Dragged toward 0 V = something at the R48 end is loading them, and
   what that something is tells you what the pins are.

If both come back negative, CAN is genuinely unreachable on this hardware and the
throttle has to be abandoned or solved another way — a series resistor in the
rectifier's own sense line, or accepting fixed 53.5 V and controlling energy
purely by time.

## 6.15 R48 CAN protocol — CORRECTED and verified (supersedes §6.14's encodings)

Source: **Endless Sphere thread 114785, post #3 by "dominik h"** — commands lifted
from an **Emerson 820 controller**, plus some found by trial. Far more complete
than the GitHub repo, and it **corrects the encoding**.

### The encoding is IEEE-754 FLOAT, not a scaled integer

§6.14 recorded `(volts×10 − 320) × 0x40000 / 10` from the repo. **Wrong.** Values
are **32-bit big-endian floats**. Decoded and verified against the post's own
stated limits:

| Bytes | Float | Post says |
|---|---|---|
| `42 24 00 00` | **41.0** | minimum voltage ✓ |
| `42 50 00 00` | **52.0** | example ✓ |
| `42 6A 00 00` | **58.5** | maximum voltage ✓ |
| `3D CC CC CD` | **0.1** | 10% of rated current ✓ |
| `3F 9A E1 48` | **1.21** | 121% of rated current ✓ |
| `40 A0 00 00` | **5.0** | AC input limit 5 A ✓ |

**All six decode exactly.** This is settled.

### Frame: ID `0x0607FF83` (or `0x06080783`), 125 kbps, 8 bytes

`03 F0 00 <cmd> <float, 4 bytes big-endian>`

| `<cmd>` | Function | Range |
|---|---|---|
| **0x21** | **Voltage, ONLINE** — lost after 30 s | 41 – 58.5 V |
| **0x24** | Voltage, OFFLINE (permanent) | 41 – 58.5 V |
| **0x22** | **Current limit, ONLINE** | 0.1 – 1.21 (fraction of rated) |
| 0x19 | Current limit, OFFLINE | 0.1 – 1.21 |
| **0x30** | **DC output**: `00 00 00 00` = ON, `00 01 00 00` = OFF | — |
| 0x32 | Walk-in on/off | |
| 0x29 | Walk-in ramp time | |
| 0x1A | AC input current limit ("diesel power limit") | amps |
| 0x33 | Fan: `00 01 00 00` full, `00 00 00 00` auto | |
| 0x39 | Restart after overvoltage on/off | |

### Current is a FRACTION of rated, and 121% is the datasheet number

*"The output current is set in percent to the rated value... **The rated current
written in the datasheet is 121%.**"*

So the datasheet's **62.5 A is the 1.21 value**, making the 100% base ≈ **51.7 A**
— which matches the datasheet's own output-voltage-vs-current graph, whose curve
is annotated **51.7** and **62.5**. **Commanding 30 A means sending 30/51.7 =
0.58**, not 30/62.5. §6.14's scaling note is superseded.

### ONLINE settings expire — repeat every 15–30 s

*"online commands working only when permanently send via CAN (every 15 seconds)"*
and *"online-voltage value gets lost after 30 seconds"*.

**This is the same watchdog behaviour recorded in §6.14, from a second
independent source.** A controller that stops talking loses its setpoint. For
CR-01 that is a free interlock and a hard constraint: **the CAN task must not
stall during a cycle.**

### Model caveat — ours is not in either list

*"The r48-1000e3, the R48-2000e3 built before 2020, the R48-3500e3 and the
R48-4300E3 can only be set permanent (offline) voltage between 48–58 V. 42–48 is
only possible with online voltage... The R48-2000e3 built 2020 and 2022 can be
set permanent from 41–58.5 V."*

**The R48-3000e3 appears in neither group.** Its offline range is unknown.
**Use ONLINE commands (0x21 / 0x22) during bring-up regardless** — they expire on
their own, so a mistake cannot persist, and they cover the full 41–58.5 V range
on every variant listed.

**Firmware clamp at 48.0 V stands** (§6.14) — the bus accepts 58.5 V, which is
~184 V peak on 200 V FETs before ringing.

## 6.16 OPEN — INA228 input protection and ground reference

Raised by Scott at the bench 2026-09-04, from a measurement, not from theory.

**What was observed.** With the INA228 module's *signal* wires pulled but its
`3V3` and `GND` still connected, a phantom voltage appeared between the ESP32's
ground and the USB shell, and it vanished the moment the module was removed. A
part that is "powered but not wired up" is still a connection.

**The mechanism.** The INA228 has ESD diodes from `VIN+`/`VIN-` to its supply
rails. With those inputs on the shunt and the logic rail down, the measured
circuit conducts into the 3V3 rail through structures rated for static
discharge, not sustained current. On the bench that was a stray millivolt
reading. On the machine, with a 48 V bus and real current behind it, it is a
failure — and the exposure is worst at exactly the moment nobody is watching:
**bus live, logic supply off.**

**Presumed answer, NOT yet a spec.** Matched series resistors on `VIN+`/`VIN-`
to limit fault current through those diodes, plus deliberate power sequencing so
logic 3V3 is up whenever the bus can be. The resistors must be matched — a
mismatch becomes a measurement offset, which is the one error this instrument
exists to avoid. A diode is *not* the fix and was considered and set aside: it
does not address the common-mode question below, and it adds a drop in the
sense path.

**Two things to settle before this becomes a requirement:**

1. **The INA228 common-mode input range.** Believed -0.3 V to +85 V. That is from
   memory and nothing should be designed against it until the datasheet is read.
2. **Is the DC bus negative the same node as logic ground?** *This is not
   answered anywhere in these documents.* If it is, series resistors and
   sequencing are sufficient. If the bus floats, the common-mode voltage is
   unbounded, the answer is isolation rather than resistors, and that is a
   different part and a different board layout.

Item 2 gates item 1 and gates the board. It is the "what does this physically
touch?" question, and it has to be answered before layout, not after.
