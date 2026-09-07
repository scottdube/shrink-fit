# Status — 2026-08-09

**Near-term job:** get 2× carbide 1/8" tools into BT30 shrink holders to unblock
PCB milling. One-way, tools stay in. Everything else is phase 2.

---

## Proven

- PSU **R48-3000e3** on 240 V — label confirmed 53.5 V / 56.1 A / 3000 W
- ZVS board oscillates and heats — test bar reached ~310 °C (blue temper band)
- ~~PZEM validated against the AC clamp: 297 W DC vs 336 W AC = 88% efficiency~~
  **WITHDRAWN 2026-08-30.** Both PZEM channels are corrupted by the field. This
  line sat under *Proven* and vouched for the instrument that has since been
  disproven twice over — the most misleading sentence in these docs. The 88%
  agreement was never evidence of accuracy; at minimum the mains wrap divisor
  was not recorded with the reading, and a wrong divisor manufactures agreement
  with whatever it is compared against.
- DC output **floats** (no rail bonded to chassis)
- ~~**Power is ~10% of target.**~~ **REVISED 2026-08-30.** That figure came from
  the same withdrawn PZEM reading (297 W of 3000 W). Measured at the mains:
  7.7 A x 240 V ~= **1.8 kW**, i.e. ~60% of the kit rating, not 10%. Coupling is
  still the limiter and the stock 60 mm coil still barely sees the work — but
  the deficit is a factor of 1.7, not 10, and any plan scaled to "10x needed"
  was scaled against an instrument error

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

### Controller — bring-up 2026-08-09

ESP32-S3 Super Mini flashed and verified. **4 MB flash, no PSRAM** (so GPIO 33–37
are free), TWAI running at 125 kbps listen-only on GPIO 4/5, zero bus errors.
Toolchain fixed — the stale Python 3.9 PlatformIO is gone and `pio` now resolves
to the 3.11 venv. The listen-only sniffer is ready for the rectifier the moment
the SN65HVD230 lands; it cannot transmit or jam the bus.

~~`testrig.cpp` has never been flashed~~ — **flashed and running 2026-08-27**,
with two-button control and a state lamp. See "Test rig runs untethered" above.

~~Toolchain regressed; reinstall PlatformIO~~ — **resolved 2026-08-27.** Opening
the VS Code extension rebuilt `~/.platformio/penv` by itself, which is what the
original note predicted. Core 6.1.19, builds and uploads fine.

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

## ⚠️ THE PZEM CANNOT BE TRUSTED NEAR THE TANK — 2026-08-27

**Measured, not inferred.** During a run the PZEM reported **29.7 V**. A second
meter on the *same PSU terminals* read **53 V+ steadily throughout**. The supply
never sagged at all.

**R-11 moves from suspected to CONFIRMED.** Flagged since 2026-08-09 on the
strength of odd first-light readings; this is the direct measurement.

### What it invalidates

**Every PZEM number taken today**, and the analysis built on them:

| Reported | Status |
|---|---|
| 49.0 V / 5.3 A / 234 W (5/8" bar) | suspect |
| 29.7 V / 3.10 A / 92.5 W (1" bar) | voltage known wrong by 44% |
| Coupling and equilibrium figures derived from those | **withdrawn** |

The error runs in the direction that makes delivered power look **lower** than it
is — the dangerous direction, because it invites longer cycles to compensate.

### What survives

**The coupling argument is geometric, not instrumental.** A 5/8" bar in a 60 mm
coil couples poorly because of the diameter ratio, and no meter error changes
that. The small-coil case stands; only the specific wattages fall.

**It also resolves the conflict that appeared earlier today** — the model
predicted ~13 °C over 30 s while colour had previously been seen on the same bar.
If true input power is well above the reported figure, they stop disagreeing.
The model was fine; the input data was not.

### What it means for the design

**CR-01 doses energy from measured V and I.** An instrument reading 44% low
inside the field is not error budget — it is the control law being fed a lie.

- The **INA228 sits in the same environment** and must be assumed vulnerable
  until proven otherwise. R-11's mitigations — differential input close-coupled
  to the shunt, twisted/shielded pair, RC filtering, common-mode choke, physical
  separation from the tank — are now **mandatory, not precautionary**.
- **CR-11's CAN cross-check is load-bearing, not belt-and-braces.** Rectifier
  telemetry arrives over an isolated digital link and cannot be corrupted this
  way. It is the only independent channel in the design.
- **A DC clamp on the feed — correct range, zeroed — is the bring-up reference**,
  not the PZEM.

### Still unknown

~~**True current.** Only the voltage channel has been checked.~~
**ANSWERED 2026-08-30 — and not with a DC clamp.** PZEM read 3.5 A against
7.7 A true mains (23 A displayed ÷ 3 wraps). Both channels are corrupt.

The planned bring-up reference — *"DC clamp on the feed, 40 A, zeroed"* —
was never bought: `bom.md` declined the UNI-T UT210E on the grounds that a
Hall clamp must sit on the DC lead **inside** the field that attacks every
other sense lead, while the AC clamp reads the mains at 60 Hz feet from the
coil and is RF-immune *by placement*.

**That reasoning is now disproven, and I had already claimed it vindicated.**
Scott put a DC clamp on the feed the same afternoon and read **33 A** — against
32.6 A predicted independently from the mains side, agreement inside 2%. The
Hall clamp sat in the field the bom entry said would attack it and was *not*
attacked. One RF failure (the PZEM) is not evidence that every in-field
instrument fails; the PZEM's sense leads are the vulnerability, and a clamp
has none. **The DC clamp is a first-class instrument here — buy the entry back.**

**Record the wrap count with every mains reading.** `quickstart-1-8.md` §
documents a 5-turn wrap; the 2026-08-30 reading used 3. A divisor carried
over from the docs instead of the jaw would be wrong by 40%.

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

## OTA works — 2026-08-27

`pio run -e testrig_ota -t upload` from `firmware/`. The rig no longer has to be
carried to the bench to change firmware, which matters because it does not live
there — that was a real workflow cost through the whole calibration campaign, not
a convenience.

**Two things had to be fixed, and the diagnosis order is worth keeping:**

**The IoT VLAN blocked it.** espota needs the *device* to open a TCP connection
back to the laptop, and IoT VLAN rules block device→LAN while allowing LAN→device.
Ping succeeded the whole time and told us nothing. **The laptop must be on the
same network as the rig when flashing.** Moving the rig to the main SSID is the
alternative — one `wifi` command — and either side can be the one that moves.

**WiFi power save was costing 10–30× latency.** Arduino leaves it on, so the radio
sleeps between beacons: **42–140 ms before, 4–8 ms after `WiFi.setSleep(false)`**.
That fix was itself delivered over OTA, which proved the loop end to end.

**mDNS resolving was the diagnostic that mattered.** `shrinkfit-testrig.local`
answering proves `ArduinoOTA.begin()` ran and the firmware side was healthy — so
the fault had to be the network path. Without that, the obvious next move would
have been to go hunting through my own code.

**Guard, unchanged:** OTA is serviced only while IDLE. An upload that hangs at
"waiting for device" means the rig is mid-cycle or in its 30 s cooldown.

## Test rig runs untethered — 2026-08-27

**Two buttons and a state lamp, no laptop.** This is what removes the USB tether
that the grounding decision forbids while energised.

| Control | |
|---|---|
| **START** (green, GPIO2) | runs the stored duration. Ignored while running or in cooldown |
| **STOP** (red, GPIO3) | aborts a run. Does nothing when idle |
| **Lamp** (GPIO48) | idle = one blink / 2 s · **running = solid** · cooldown = double blink |

All three paths verified from the serial log while the buttons were pressed:
start, abort at 4347 ms of a 10 s run, and `START ignored — already running or in
cooldown`.

**Two buttons, not one, deliberately.** A single button that both starts and stops
is mode-dependent — press it without knowing the state and you start a cycle you
meant to end. **The STOP button is a firmware abort and is NOT the SR-03 E-stop**,
which is hardware in series with the contactor coil and must look different.

**Duration persists in NVS**, so the rig is set up once with USB attached (`d 20`)
and then run untethered indefinitely.

**Pattern-only indication proved legible in practice** — the three states were
distinguished on the bench with no colour involved at all. That is direct support
for the three-lamp plan in `roadmap.md`, where pattern and position carry the
signal and colour only reinforces it.

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

## 2026-08-30 — first real temperature: 320 °C in 30 s

**1" solid bar, 30 s cycle, thermocouple held against the bottom of an existing
~0.5" deep x 0.25" blind hole in the end.** Not packed, so contact was through
air over part of the bead -- the reading is a floor, not a peak. Actual metal
temperature was higher.

**This answers the gate question.** Target nose temperature is 300 °C nominal
with a 350 °C hard ceiling (requirements.md §4.3, §7). 320 °C on a solid 1" bar
is inside the shrink-fit window -- and a holder nose carries far less mass than
this bar, so it should reach temperature sooner, not later. The machine has the
power. That was not established before today.

**Correction:** an earlier draft of this line said the result was achieved "on a
shorted coil", implying reserve capacity that has not been shown to exist.
"Turns touching" is not a fault -- `quickstart-1-8.md` § specifies winding turns
touching *and insulating them*, because touching is how turns are packed into a
short axial length. Only **bare** touching turns are shorted turns, and the coil
appears to carry the fibreglass sleeving `design-references.md` A5 calls for.

The coupling deficit is geometric, not electrical: a 25 mm bar in the stock
60 mm coil is an area ratio of (25/60)^2 ~= **0.17**. The gain lives in winding
the 20 mm quickstart coil, not in repairing a short -- same task on the roadmap,
different reason, and the reason is what sets the target bore.

**What it does not answer:** the 1/8" holder is estimated to want ~390 K rise
(~410 °C), *above* the 350 °C ceiling (§4.3). That estimate is unsourced and may
be well off. It is the open question, and it is now the *only* thing between
this rig and the milling tools it was built for.

Do not carry the 30 s figure across to a holder. Different mass, different
coupling, different geometry -- it is this bar's number.

### Temper colours confirm the thermocouple — 2026-08-30

Photographed after the 30 s run. Bands read against `quickstart-1-8.md` §:

| Band | Position on bar | Temperature |
|---|---|---|
| Blue-grey | top third | ~310 °C |
| Purple | sharp band ~2/3 down | 280–290 °C |
| Brown-purple | just below | ~265 °C |
| Pale straw | lower quarter | 200–230 °C |
| none | foot | < 200 °C |

**Thermocouple read 320 °C down the bore at the top; the oxide says ~310 °C at
the same end.** These share no failure mode — one is a millivolt measurement in
an RF field, the other is a chemical reaction — so the agreement is real
corroboration, not two views of one instrument. The temperature question is
closed, and 320 °C is if anything conservative (the bead was unpacked).

**The gradient is the more valuable half.** The colour map draws the coupling
zone directly: fully hot over roughly the top two-thirds, steep transition, cold
foot. That independently confirms the energy reconciliation, which needed
~60–70% of the 0.4 kg bar involved to balance 52 kJ against a 300 K rise. The
hot end is also the bored end — heat at the nose, cold at the taper, which is
the right pattern for a holder.

**Caveats.** Temper oxide is cumulative and the check is now spent on this
piece; sand back to bright before reusing it as a witness. And if this bar is
the same one recorded earlier at ~310 °C, part of the banding predates this run
and the agreement is weaker than it looks — confirm the surface went in clean.

### OQ-3 partially closed — Haas publishes no thermal data at all

**2026-08-30.** Fetched the 04-1424 product page directly. Specifications cover
bore, gage length, concentricity and balanced RPM; **there is no heating
temperature, no maximum temperature, and no induction instruction.** Combined
with the unanswered material email, this closes the *expectation* half of OQ-3:
there is no Haas figure to wait for. The **350 °C ceiling is our own
conservative choice**, not a manufacturer limit — treat it as ours to defend
rather than as a spec to cite.

### A low-reading thermocouple defeats CR-02

Scott's reasoning was that a thermocouple erring low is manageable, since more
time or power closes the gap. **True for reaching temperature, exactly wrong for
the ceiling.** CR-02's thresholds — flag above 350 °C, lock the size preset
above 375 °C — fire on the thermocouple reading. A probe reading low never trips
them: 380 °C actual displays as 340 °C, no flag, no lockout, holder tempering
quietly. CR-02 is already the fallback, because §8.3 ruled out a real-time
cutoff entirely.

So the bias that is harmless for coil A/B work is the one bias the holder
interlock cannot tolerate. Absolute accuracy is not optional at the top of the
range, whatever is acceptable in the middle of it.

### The smoking insulation may be free to fix

Adafruit's glass-braid K-type ships with a short length of heat-shrink at the
tip **whose only purpose is to stop the braid fraying**, and their own guidance
is to cut it off above 250 °C. If the probe on the bench is glass-braid with
that tip wrap intact, removing it is the entire fix. If the wire is smooth
plastic to the bead, it is PVC/PTFE (~105–260 °C) and must be replaced —
[Adafruit #270](https://www.adafruit.com/product/270), bare bead, 500 °C,
~$14 [ESTIMATE, +40%, not verified live]. Bare bead over stainless tip: it drops
into a 1.5 mm witness hole and responds faster.

**CONFIRMED 2026-08-30 by photograph.** The bench probe *is* the Adafruit
glass-braid part — white fiberglass braid, blue tracer, black anti-fraying
sleeve at the tip. Nothing to buy: trim the black sleeve and the probe is rated
500 °C. No PTFE, so the smoke was the tip wrap alone.

The sleeve is **marked 125 °C**, which on heat-shrink is the maximum continuous
*operating* rating, not the shrink point (polyolefin shrinks at ~90–120 °C).
The part was carrying its own limit in print while being run at ~2.5x it.
Score along its length and peel rather than cutting across — a blade drawn
toward the bead can nick a conductor, and a half-severed wire gives a junction
that works until it does not.

**After trimming, keep the two conductors apart along their whole length.** A
thermocouple reads at the *hottest point where the wires touch* — the bead is
the junction only because it is the sole contact. With the sleeve gone the outer
braid can fray and let the conductors meet further back; if that second contact
sits in a hotter part of the field, **it silently becomes the junction** and the
reading is of that spot instead. The failure produces a plausible temperature,
which is what makes it dangerous. Slip fiberglass sleeving or a ceramic bead
over the pair if the braid opens up.

## OQ-3 ANSWERED — Haas HSF-325 manual gives 400 °C

**2026-08-30.** Scott found the operation manual for Haas's own shrink-fit
machine. The page 403s to a plain fetch; a browser reads it fine (client
fingerprinting, per the standing rule). Source:
<https://www.haascnc.com/service/online-manuals/hsf-325/hsf-325---operation.html>

### The ceiling

> *"This machine has a fast heating capacity! The maximum recommended heating
> temperature is **400 °C**. Temperatures higher than 400 °C may damage the
> toolholders. Please contact your toolholder supplier for detailed heating
> specifications."*

Stated twice, in the Manual and Auto sections. This is Haas's own machine used
with Haas's own holders, so it is the closest thing to an authoritative figure
that exists for this build.

**Our ceiling is 350 °C** (CR-02: flag >350, lock out >375), and its stated
justification was the absence of a manufacturer number. **That justification no
longer holds.** §4.3's estimate that the 1/8" holder wants ~410 °C sat *above*
the old ceiling and was the single blocking risk for the whole project; against
400 °C the gap is ~2.5%, and the 410 figure is itself flagged unsourced.

**DECISION PENDING — Scott's call, not mine.** Raising a limit that protects the
holders is not a change to make on a subagent's initiative. Recorded, not applied.

Note the circularity: the machine manual defers to the toolholder supplier;
Haas Tooling is that supplier, publishes nothing (OQ-3), and has not answered.
The 400 °C is therefore a machine-general limit, not a holder-specific one.

### Recommended times — and what they say about our power

| Tool D | Heat IN | Heat OUT | Cool IN | Cool OUT |
|---|---|---|---|---|
| 1-1/4" (32 mm) | 5.0 s | 6.0 s | 4.0 s | 4.5 s |
| 1" (25 mm) | 8.0 s | 8.5 s | 6.0 s | 6.0 s |
| 3/4" (19 mm) | 7.5 s | 9.0 s | 4.0 s | 5.0 s |
| 5/8" (16 mm) | 6.5 s | 7.0 s | 3.0 s | 3.5 s |
| 1/2" (13 mm) | 6.0 s | 9.0 s | 2.5 s | 3.0 s |
| 3/8" (10 mm) | 6.0 s | 7.0 s | 2.5 s | 3.0 s |

**Max consumption power: 10,000 W.** Against our measured ~1.8 kW that is 5.6x.

**This retires the worry that 30 s cycles are abnormal.** Haas puts ~80 kJ into
a 1" tool (8 s x 10 kW, and 10 kW is max consumption so the real figure is
lower); our 30 s at 1.8 kW is ~54 kJ, into a *solid* 1" bar at 0.17 coupling --
more mass and worse coupling than a holder nose. Same energy ballpark, delivered
more slowly. The approach is sound; the clock is just longer.

### Other specifics worth carrying

- **Cooling is an air blast, 2.5–6 s.** Not liquid, and far shorter than assumed.
- **"We recommend waiting 2 seconds after the heating period to insert the tool"**
  — Haas's equivalent of our TC_SETTLE / insertion delay.
- **Alarm E4: "No toolholder in heating head."** They interlock against
  energising an empty coil. We have no such check. Candidate requirement.
- Alarm E2 is heating-head over-temperature; E5 is power-module over-temperature.
- Coil-height table (H1 by diameter, H2 >= 6 mm minimum) gives their coil
  positioning over the clamping portion — relevant to coil and fixture design.

## Bench queue — 2026-08-31, in order

Written 2026-08-30. The PZEM is disassembled on the bench; reassemble first.

**1. One precise simultaneous reading: PZEM current vs DC clamp.** Same instant,
current only. No probes inside the box, no live leads brought out — the shunt-
millivolt measurement discussed earlier is not worth the hazard and is not needed.

| Ratio | Meaning |
|---|---|
| **exactly 6.0** | 50 A configured against a 300 A shunt — documented bug, known fix, and "3.3ish" was really 5.5 |
| **exactly 10.0** | outside the documented option set; the range theory is dead |

Community evidence: the failure mode is common (PZEM-051 reading 2x with a 50 A
shunt set to 100 A; PZEM-017 reading half with a 200 A shunt), register 0x0003
sets range on the 017 family, factory default 100 A. **But every documented
pairing caps at 6x.** Nobody reports 10x. The recorded figures — "about three and
a half", "three point threeish" — were eyeball recollections during a run, not
readings taken to settle a question, and 5.5 misremembered as 3.5 is exactly the
slip that survives when nothing depends on it.

**2. Trim the thermocouple tip sleeve.** Free, takes the probe from ~130 °C to
500 °C. Score along its length and peel; do not cut across.

**3. Does the ratio hold across two power levels?** Constant → a firmware
multiply makes the ammeter usable. Wandering → the current channel is corrupted
and the scale-constant story comes out of these docs.

**4. Watch the clamp through a whole cycle — is the draw flat?** Scott reports
steady 33 A. If that holds, it undermines CR-08's ≥100 Hz integration rate, which
was specified before anyone knew what the draw does. **Flat current means CAN's
~1 Hz telemetry integrates energy just as accurately, CAN alone can do CR-01, and
the INA228 comes off the parts list — the last item on it.** Check the first
second or two, and re-check once the 19.5 mm coil is on: flat at 0.17 coupling
may not stay flat at 0.7.

**5. Cross-drill a 5/8" witness bar**, 1.5 mm x 8 mm deep at mid-length. The 1"
bar does not fit the 19.5 mm coil, so without this the two coils cannot be
compared on the same workpiece and the coil log has no common baseline.

**6. First runs on the 19.5 mm coil: 5–10 s, stepping up, with a temper witness.
NOT 30 s.** Cycle time does not survive a coil change (see TRAPS).

### Standing decision: retire the PZEM

It is redundant for bench work — the DC clamp gives current, the second meter
gives voltage at the PSU, and both are proven against each other. It is inferior
for the controller, where CAN reads the rectifier's own output digitally from
outside the field. It has cost a day of investigation and produced no number
anyone will use. Item 1 above is to close the question, not to rehabilitate the
instrument.

**Board condition, for the record:** heavy white flux residue plus olive-green
streaking along the pad columns — copper corrosion product, so the flux was
activated rather than no-clean and has been working on the copper since
manufacture. Joints are dull and dark. This is a plausible contributor to the
*voltage* error (leakage across a divider reads low, and low is what we see)
alongside the RF, but it cannot produce a flat 10x on current. Not worth cleaning
now that the instrument is retired.

### The shunt outlives the PZEM — and tomorrow's test decides its fate too

**2026-08-30, Scott: "if we remove the PZEM what are we doing for a shunt?"** The
shunt is a separate part from the PZEM board — a bus-bar element bolted into the
positive lead. Retiring the board leaves it in the circuit.

**Item 4 in the bench queue (is the draw flat?) therefore decides two things:**

- **CAN suffices** → **remove the shunt as well.** It is two bolted joints in a
  33 A bus serving no purpose, and every high-current joint is a candidate hot
  spot. Rejoin the lead.
- **INA228 needed** → **the existing shunt becomes its shunt.** That is what an
  INA228 does: reads the millivolt drop and computes current.

**The unmarked rating stops mattering in that case.** The INA228 reports shunt
voltage as a raw register, so: run a known current (DC clamp), read the shunt-
voltage register, `R = V / I`, program the calibration register. **Calibrated
against the clamp, which is already the trusted reference.** The missing stamp —
an open unknown since the bench inventory, and the thing the whole 10x
investigation kept circling — simply becomes irrelevant.

Range check: 33 A through a plausible 750 µΩ gives ~25 mV against the INA228's
±163 mV full scale, and its 20-bit ADC makes the low fraction of full scale
harmless. Dissipation either way is 33² × 750 µΩ ≈ **0.8 W**, negligible.

If it stays: **mount the INA228 at the shunt**, not at the controller, and twist
the sense pair. The untwisted loop is not causing the 10x — a flat factor is not
RF — but there is no reason to build a pickup loop into a new install.

## 2026-08-30 (late) — simultaneous four-instrument read; range theory dead

All read at one instant, ZVS running:

| Instrument | Reading |
|---|---|
| PZEM | **30.8 V, 3.10 A, 96.1 W** |
| UNI-T DC clamp (feed) | **33.9 A** |
| KAIWEETS KM601 (at ZVS) | **52.30 V** |
| AC clamp (mains, 3 wraps) | 23.03 → **7.68 A** |

### The shunt-range hypothesis is refuted

**33.9 / 3.10 = 10.94.** Not 6.0, and not a ratio of any two values in the
PZEM-025's shunt-range set (50/100/200/300 A) — the largest possible mismatch
there is 6x. **A misconfigured range cannot produce this reading.** That is what
the test was for, and it settled it.

**Voltage: 52.30 / 30.8 = 1.70.** That ratio has now been 1.78, 1.60 and 1.70
across three readings. **A scale error does not move.** Corruption confirmed on
the voltage channel, independently of the current channel.

### The three trustworthy instruments close on each other

- AC side: 7.68 A x 240 V = **1843 W in**
- DC side: 33.9 A x 52.30 V = **1773 W out**
- **1773 / 1843 = 96%** — textbook efficiency for a telecom rectifier, from two
  independent measurements on opposite sides of the PSU.

**PZEM reports 96.1 W against 1773 W actual — off by 18x.** Retired, and now
demonstrably so rather than by inference.

### Deliberately not pursued further

One point cannot distinguish "constant scale error" from "corruption that happens
to look like 11x today"; that needs a second power level. **It is not worth bench
time.** The INA228 (arriving Sept 3) answers the underlying question — *can a
shunt in this bus be trusted?* — far better, because it exposes **raw shunt
millivolts** as a register. Two power levels against the clamp show directly
whether shunt voltage is proportional and stable, with no calibration constant
and no unknown firmware in between. Same experiment, confounding variable removed.

### Bench queue addition — measure tank frequency on the 19.5 mm coil FIRST

**Before running long cycles on the new coil.** The tank caps are marked **50 kHz**
(`requirements.md` §73 — corrected from an assumed 150), and `design-references.md`
§99 records that **both predicted bands are above it**:

| Coil | Predicted f | vs 50 kHz cap rating |
|---|---|---|
| 60 mm (today's) | ~59 kHz | slightly over |
| **19.5 mm (untested)** | **~108 kHz**, band 86–130 | **~2x over** |

Running 0.33 µF MKPH parts at twice rated frequency is dielectric heating that
gives no warning until a capacitor fails.

**Method — no LCR meter needed, and none is owned.** The KM601 has no inductance
range. Use the **DHO914 with a pickup loop**: 2–3 turns of insulated wire ~25 mm
across, twisted leads to the probe, held a few inches from the work coil, AC
coupled. FFT reads the frequency directly.

**Do not probe the tank.** Peak Vds ~= pi x Vdc ~= 164 V plus ringing, and the DC
output **floats** while the scope ground is earthed — clipping an earthed ground
to a floating tank makes a fault path through the instrument. The pickup loop has
no galvanic connection at all.

Inductance follows from the measured frequency and the known 3.96 µF:
**L = 1 / ((2*pi*f)^2 * C)** — 59 kHz → 1.84 µH, 108 kHz → 0.55 µH. In-situ, so
it includes stray lead inductance and the workpiece's effect, which a bench LCR
reading would miss.

## WORK PLAN — morning of 2026-08-31

Nothing ordered arrives before Sept 1, so today is measurement and machining
only. No panel or controller work is possible yet.

### Block 0 — five minutes, do first

**Trim the thermocouple tip sleeve.** Score along its length, peel; do not cut
across. Takes the probe from ~130 °C to its rated 500 °C, and every temperature
measurement below depends on it. Afterwards check the two conductors touch
nowhere but the bead.

### Block 1 — bench, no power (~15 min)

- **Reassemble the PZEM** enough to make the bus mechanically sound. Its readings
  are retired; the shunt stays bolted in because the INA228 (Sept 3) may use it.
- **Assess the 19.5 mm coil for installability** — do its legs reach the 70 mm
  barrel clamps, or does it need bending and annealing first? **This decides
  whether Block 4 happens today**, so answer it before committing to the day.
  Note `quickstart-1-8.md` §102 prefers a **21 mm** bore over 19.5 (3.0 mm/side
  clearance to a 15 mm nose rather than 2.25) — the wound coil is testable, but
  it is not the doc's recommended geometry.

### Block 2 — Tormach (~45 min)

**Cross-drill a 5/8" witness bar, 1.5 mm × 8 mm deep — in the section that
protrudes PAST the coil, not at mid-length** (see TRAPS: the probe cannot exit a
mid-length hole; 1.8 mm/side clearance in the bore against a ~2.5 mm braided
probe). Mark the insertion depth and reuse the mark every run.

**Drilling it:** 1100MX spindle is 10,000 RPM. HSS ~4,500 RPM at 3–4 IPM;
carbide is RPM-limited at 10,000, 5–6 IPM. **Mill a small flat first** — a 1.5 mm
drill walks on a 5/8" curved surface and snaps. Centre-drill, then **G83
full-retract pecks at ~1 mm** (5.3x diameter deep, so chip evacuation is the
whole problem). V-block, not flat vise jaws. Have a spare drill.
*(Tormach publishes no feeds/speeds chart for this machine — these are standard
practice, calculated, not a spec.)* The 1" bar
does not fit the 19.5 mm coil, so without a common test piece the two coils
cannot be compared and the coil log has no baseline. **This gates the whole A/B.**

### Block 3 — powered, existing 60 mm coil (~30 min)

**a) The flat-draw test — highest value of the day.** Watch the DC clamp through
a complete cycle, *including the first two seconds*. Does it hold 33.9 A or move?

| Result | Consequence |
|---|---|
| Flat | CAN alone does CR-01. **INA228 returns, shunt comes out of the bus** |
| Varies | Fast sampling is real. INA228 stays, mount it at the shunt |

**b) Baseline the witness bar** in the 60 mm coil — this is the "A" of the A/B.
Same insertion depth, same start temperature, note both. Peak temp read from
200 ms after field-off and take the **maximum**, not the first number.

### Block 1 RESULT — 2026-08-31

Block 0 done (TC sleeve trimmed). PZEM reassembled and installed; bus
mechanically sound. **The 19.5 mm coil is installable** — legs need bending to
point correctly, otherwise ready.

**ORDERING CONSTRAINT: Block 3 must happen before the coil is swapped.** The
baseline temperature run is the "A" of the A/B and needs the 60 mm coil still
fitted. Swap first and the comparison is gone.

### Block 4 — only if the coil is installable (~1 hr)

0. **Bend the legs. Anneal any bend being re-made** — copper at an existing bend
   is work-hardened and cracks when re-formed, and **a crack in a water-cooled
   coil is a leak onto a live board.** Torch to dull red (~500–600 °C) and let it
   cool; there is no quench rate to get wrong with copper. **Radius >= 3x OD**
   (~15 mm on 3/16" tube) — tighter kinks thin wall and restricts flow invisibly
   from outside. Pack with sand and plug the ends if it wants to kink.
   Then confirm both constraints: legs reach the 70 mm barrel clamps **and**
   clear the fans.
0b. **Flow-test before energising.** Run coolant, hold pressure, look for weeps
   at every bend and especially anywhere annealed and re-formed. Five minutes
   against finding the leak with 1.75 kW and a live board underneath.
1. Install the 19.5 mm coil.
2. **Frequency check FIRST, brief energisation only.** Pickup loop (2–3 turns,
   ~25 mm, twisted leads) into the DHO914, AC coupled, FFT. **Do not probe the
   tank** — peak Vds ≈ 164 V plus ringing on a floating output against an earthed
   scope ground.
   - **~108 kHz → the caps are at ~2× their 50 kHz rating.** Not a stop, but keep
     every cycle short and log it. `requirements.md` §147 has the tuning lever
     (more turns drops resonance; 8 turns on 12 caps lands ~59 kHz).
3. Short heating runs only: **5 s, then 10 s, stepping up** — never the 30 s that
   was safe on the old coil. Cycle time does not survive a coil change.
4. Re-run the flat-draw check on the new coil; flat at 0.17 coupling may not stay
   flat at 0.7.

### Block 5 — opportunistic (~15 min)

**Pump current with the DC clamp** — running and inrush. It is the last estimated
figure in §6.11 and it sizes the buck that carries it. Flow measurement waits on
the sensor.

### If the morning runs short

Order of value: **Block 0 → Block 3a → Block 2 → everything else.** Block 3a
decides a purchase and a piece of bus hardware; Block 2 gates the coil comparison
that the rest of the project's calibration depends on.

## 2026-08-31 — Block 3 result: baseline set, draw is flat enough for CAN

**60 mm coil, 1" bar, 30 s cycle, surface sanded bright before the run.**

| Measurement | Result |
|---|---|
| Peak temperature | **~310 °C** (blue temper band, fresh surface) |
| DC current | **32 A rising steadily to 36.8 A** over the cycle |

### The machine repeats

310 °C here against 320 °C on the thermocouple and ~310 °C on the oxide
yesterday — **three runs, three methods, same answer.** Reproducibility is worth
as much as the number: a rig that answers differently each time cannot be
calibrated at all. **This is the "before" for the coil A/B.**

The hot band is narrow and sharply bounded, mapping the 60 mm coil's coupling
zone directly, with a steep falloff above and below.

### CR-01 can run on CAN — the draw is flat enough

A 15% rise across 30 s is **0.5% per second**. At 1 Hz that is 30 samples on a
smooth monotonic ramp, and **trapezoidal integration of a linear ramp is exact**.
Even the crude midpoint — (32 + 36.8)/2 = 34.4 A — lands within ~1% of the true
integral. **100 Hz buys nothing on a signal shaped like this**, so CR-08's
≥100 Hz requirement is over-specified for this load and CAN telemetry suffices.

**Do not return the INA228 yet.** Two things could still overturn this, both
resolving within days: the R48's actual CAN report rate is unverified (assumed
~1 Hz; at 0.1 Hz there would be only 3 samples per cycle), and this was measured
at **0.17 coupling** — at 0.66 the load is a different animal and a 15% ramp
could become a 50% swing. $16 with a 30-day window against re-ordering and
waiting.

### The rising current is a free diagnostic

Steel's resistivity roughly doubles from 20 °C to 300 °C, so a hotter workpiece
absorbs more power and draws more current. **The current curve is therefore a
proxy for workpiece heating with no thermocouple involved.** If it plateaus, the
piece has reached thermal equilibrium and more time will not make it hotter.

**Ours was still climbing at 36.8 A when the cycle ended — the bar was still
heating when we stopped it.** There was more temperature available in that setup.

## 2026-08-31 — 19.5 mm coil first light: it works, and the bus collapses

### Measured

| Quantity | 60 mm coil (yesterday) | **19.5 mm coil** |
|---|---|---|
| Tank frequency | ~59 kHz predicted | **100.37 kHz measured** (DHO914, pickup loop) |
| Derived inductance | — | **0.635 µH** (from known 3.96 µF) |
| Bus voltage under load | 52.30 V | **18.0 V** (KM601) |
| Current | 33.9 A | **67.3 A** (UT210E) |
| Input power | 1,773 W | **1,211 W** |
| 5 s | — | very light straw (~200 °C) |
| 10 s | — | **250 °C** (TC at bar end; oxide reads brown-purple ~265 °C) |
| 30 s | 310 °C | — |
| Heating rate | 9.5 K/s | **~22–35 K/s** |

### Three times the heating on two-thirds the power

Work-heating scaling roughly with coupling: 1,773 x 0.17 = 301 versus
1,211 x 0.66 = 802, a ratio of **2.7x** against the ~3x rate actually observed.
**The numbers close.** This is coupling doing exactly what the model said.

### The supply is CURRENT-limited and the bus has collapsed

Not power-limited — that was my first reading and it was wrong. A current-limited
rectifier delivers all the amps it can and lets voltage fall to whatever the load
takes. At 0.66 coupling the tank impedance is low enough that **the bus falls from
52.3 V to 18 V** and the machine uses **1,211 W of an available 3,000 W**.

**Nothing is broken.** Peak Vds is now ~57 V against 200 V rated — *less* FET
stress than before. Current limiting is a designed rectifier mode, not a fault.
And it heats fast enough to be useful.

**If the power is wanted back:** more inductance *in the same axial length* — a
multi-layer winding (2 layers x 5 turns rather than 10 turns strung out to 48 mm,
see `design-references.md` §80). That raises tank impedance so the supply is not
driven into limit **and** drops frequency toward the caps' 50 kHz rating. Two
problems, one fix — but a Phase 2 job, not a blocker.

### The two-point test completed itself — INA228 CONFIRMED KEEP

| Operating point | Clamp | PZEM | Ratio |
|---|---|---|---|
| 60 mm coil | 33.9 A | 3.10 A | **10.94** |
| 19.5 mm coil | 67.3 A | ~6.38 A | **10.55** |

**Constant within reading error across a 2x change in current.** That is the
experiment set up yesterday and undecidable from one point.

- The PZEM's current channel is a **scale error, not corruption** — it misreports
  magnitude, not shape, and a constant is a firmware multiply.
- **Decisive consequence: a close-coupled shunt in this bus reads LINEARLY.** That
  was the gating question for CR-01's sensor. **Keep the INA228** (arrives Sept 3),
  and **the shunt stays bolted in the bus** rather than coming out.

### Capacitors

100 kHz against a **50 kHz** rating. Real, but the duty cycle makes it minor: the
rating is a continuous-operation thermal limit, and this machine runs ~10 cycles
a week at seconds each. Caps measured cool to the touch (though 5 minutes after a
5 s run, which is several thermal time constants — weak evidence, but the
exposure is genuinely small). **An IR thermometer would make this a two-second
non-contact check** and is the right ~$20 to spend if it keeps coming up.

## 2026-08-31 — ★ TOOL IN A HOLDER. Near-term goal achieved.

**A carbide tool is shrunk into a Haas BT30 1/8" holder**, on the 19.5 mm coil.
That was the project's stated near-term objective — *"get 1–2 carbide 1/8" tools
into holders to unblock PCB milling"* — and it is done.

### What it took

**25 s → machine cooldown → 10 s.** Roughly 35 s of heating, split.

**That split is the supply derating, made physical.** A single long cycle
underperforms because output falls as the R48 protects itself: by 25 s the input
is down near **795 W** (15 V x 53 A) from 1,211 W at the start. The gap let the
supply recover, so the second 10 s ran nearer full power on an already-hot
holder, and that combination carried it.

**~35 s split did what ~15 s at sustained full power would have.**

### This is now the strongest argument for the multi-layer rewind

Not elegance — throughput. Raising tank impedance lets the R48 deliver rated
current **inside its own envelope** instead of being driven into limit and
derating out of it. Same energy, one cycle, about half the time. It also drops
frequency toward the caps' 50 kHz rating and stops the bus collapsing. **Three
problems, one fix** (`design-references.md` §80 — 2 layers x 5 turns in the same
30 mm axial length, rather than 10 turns strung out to 48 mm).

### Capacitors — the duty-cycle argument holds empirically

**Cool to the touch after a 25 s run** at 2x rated frequency. The rating is a
continuous-operation thermal limit and this machine runs seconds at a time; that
was the argument on paper, and the bench now agrees with it.

### Known, not yet needed: release costs more than insertion

Haas's own table shows shrink-**out** running 15–20% longer than shrink-in at
every size. The near-term goal is one-way — tools stay in — so it does not bite
today. When a tool does need to come out, budget more than 35 s and expect the
holder at the top of its temperature range.

### Heat placement — confirmed correct by the holder itself

Photographed after the successful insertion. The oxide bands read:

| Zone | Colour | Temp |
|---|---|---|
| Clamping section / nose | **deep purple** | ~285 °C |
| Shoulder | gold-straw | ~215 °C |
| **Flange, drive slots, BT30 taper** | **bright, untouched** | ambient |

**The heat is confined to the clamping section.** The concern that a 30 mm coil
against a 15 mm clamp length might spread heat onto the taper — the precision
spindle interface — is **unfounded**. Coil length and positioning are right;
nothing to change.

**The band is even around the full circumference**, which means the coil sat
concentric with the nose and heated it uniformly. No hot spot from an off-centre
coil. That is a quality check available free on every cycle, and no instrument
can give it.

### And ~285 °C closes §4.3's estimate for good

`requirements.md` §4.3 put the 1/8" holder's requirement at **~410 °C** — the
figure that sat above the old 350 °C ceiling and was the project's headline
blocking risk. The doc flagged it unsourced and said *"you will find out
empirically."*

**It released at ~285 °C surface.** The estimate was high by over 100 degrees.
That risk is not narrowed, it is **closed** — and with Haas's 400 °C ceiling
there is now margin on both sides rather than a conflict.

**No metallurgical concern:** H13 tempers at 550–650 °C. 285 °C is nowhere near
it, and no plausible holder steel is troubled. The oxide is cosmetic and
cumulative — holders darken over their working life, which also means **the
colour is spent as a diagnostic on that holder**: it now reads "at least as hot
as it has ever been", not "as hot as this run".

### Reheat repositions an already-shrunk tool — 2026-08-31

**30 s → cooldown → 15 s** released and re-seated a tool already shrunk in, to a
chosen depth. Against **25 s → cooldown → 10 s** for the original insertion,
which fits: a reposition is a release *and* a re-seat in one heat, and Haas's own
table shows shrink-**out** running 15–20% longer than shrink-in.

**A mis-set depth is therefore recoverable, not a ruined holder.** Worth knowing
before it happens on a tool that matters.

**Still purple, no hotter — ~285–290 °C on both cycles.** The machine repeats at
temperature, and **the colour line still has not passed the nose taper** after a
second cycle. Heat placement holding up under repetition is better evidence than
the first photo was; one clean run could have been luck.

Two heat cycles is nothing for a holder — these are rated in the thousands.

### Depth-stop collars — buy (MR-15 / roadmap "length preset")

The entire reheat existed because there is **no shoulder in a shrink holder** for
the tool to bottom on; depth is set by hand, one shot per cycle. `roadmap.md`
lists this as *"Depth stop / length preset — stickout repeatability"* and MR-15
specifies insertion *"as a single motion against a preset depth stop."*

**Spec: 1/8" bore, ONE-PIECE SPLIT CLAMP type, aluminum.** Not set-screw —
a point-contact screw raises a burr on the carbide shank, and **a burr on a
shrink-fit surface ruins the interference**: it either will not seat or it scores
the bore. Clamp collars grip evenly with no point load.

**Clamp only on shank that stays outside the holder**, never on the length that
enters the bore.

Reference dimensions (NABCO 1A002): **0.813" OD, 0.250" width, 4-40 clamp screw**.
Scott buys from McMaster — search *"clamping shaft collar"*, filter bore to 1/8".
McMaster cannot be browsed programmatically; every URL redirects to their home
page, consistent with the note that it is the one vendor that blocks automation.

**Check before ordering: 0.813" OD is 20.6 mm against a 19.5 mm coil bore.** If
the holder's nose face sits *inside* the coil during insertion, that collar will
not clear. Measure the geometry; narrower-OD options exist in the same bore.

## 2026-08-31 — INA228 bring-up COMPLETE

On a **NodeMCU-32S** (the identified variant, §6.12), deliberately separate from
the S3 test rig so a working machine was never at risk.

| Check | Result |
|---|---|
| I²C scan | **0x40 responds** |
| MFG_ID | **0x5449** — genuinely TI |
| DEVICE_ID | **0x2281** — genuinely an INA228 |
| WiFi / OTA | joined IoT, **`shrinkfit-ina228.local`**, OTA verified end to end |
| R002 removed | **VIN+ to VIN− now 35 MΩ** — open |
| Analog front end | responds under `stream` |

**The ID registers are the check that mattered.** Anything can acknowledge an
address; 0x5449 + 0x2281 say it is the part it claims to be. Doing that *before*
the rework meant a dead board could not be ambiguous between arrived-dead and
killed-in-desoldering.

**35 MΩ is fully open for this purpose.** Against a ~750 µΩ shunt it diverts
about two parts in a hundred billion. The residual is the chip's input network
plus rework flux; neither matters.

### Two bad instructions of mine, recorded so they are not repeated

**"Touch VIN+/VIN− and watch VSHUNT move" is wrong twice over.** With R002 fitted
the pads are shorted through 2 mΩ against a finger's ~1 MΩ — nothing external can
move it. And *after* removal, touching **both** pads with one finger connects
them, driving the differential **toward zero**: the one action guaranteed to
produce the least signal.

**A real test, quantitative, two resistors:** 3.3 V → 1 kΩ → 10 Ω → GND, VIN+ on
the junction, VIN− on ground = **32.7 mV**, well inside the ±163 mV range.
VSHUNT should read ≈ **+32,700 µV**, and **−32,700** with the leads swapped —
which also confirms the polarity convention before committing to Kelvin taps.

**`stream` must be typed** — the sketch prints nothing otherwise. That was
mentioned once in passing and never as a step, and cost time.

### Next

Wire VIN+ upstream tap, VIN− downstream tap, VBUS to the same point as VIN−,
GND to DC−. Then known current on the clamp, read the shunt-voltage register,
**R = V / I**. Landing near 750 / 500 / 250 µΩ confirms the Kelvin taps;
an implausible value means they are on the wrong points.

### Telnet console — the stream OTA does not carry (2026-08-31)

`ArduinoOTA` uploads firmware only. A `WiFiServer` on **port 23** carries the
console; all output goes to both USB and telnet through one `say()` path.

**The asymmetry is the point.** espota needs the laptop on the device's VLAN
because *the device connects back to the host*. **Telnet is an ordinary outbound
connection from laptop to board**, so it crosses the VLAN espota cannot. Firmware
pushes still need the IoT switch; watching the stream does not — which matters
because the laptop cannot live on IoT (other jobs hit firewall problems).

```
nc shrinkfit-ina228.local 23
pio device monitor --port socket://shrinkfit-ina228.local:23   # familiar interface
```

**macOS has no telnet client** — `telnet` returns `command not found`. Use `nc`.

Verified: banner, commands accepted, live reads — VSHUNT +5.6 µV, VBUS 0.4 mV,
die 26.0 °C on floating inputs. Noise values, but the ADC is converting and the
die temperature is plausibly ambient.

A second connection **displaces** the first rather than being refused: a stale
session left by a closed laptop lid must never lock you out of your own board.

*(Housekeeping: the telnet firmware itself landed inside commit `bd0fbc7`, whose
message is about the pin map — a `git commit -a` swept it up. The code is
correct; only the commit message is misleading.)*

## CAN Stage 1 — firmware built, awaiting flash and wiring (2026-09-02)

Committed as `06dc90b`. **Not yet on the board**: the bring-up board is on external
power with no USB, and OTA needs the laptop on IoT.

**Sequence decided:** CAN first (needs the R48 on mains, DC output OPEN — nothing
connected, as benign as it gets), then open the PZEM box once to bring the shunt's
Kelvin leads out, then INA228 calibration at the bench supply in CC mode.

### Wiring for Stage 1

| From | To |
|---|---|
| SN65HVD230 **CTX** | NodeMCU **GPIO 14** (P14, pin 31) |
| SN65HVD230 **CRX** | NodeMCU **GPIO 32** (P32, pin 26) — **not GPIO 12**, which must be LOW at boot and would never start |
| SN65HVD230 3V3 / GND | NodeMCU 3V3 / GND |
| CANH / CANL | PSU breakout **2-pin JST** |
| ~~**220 Ω ∥ 270 Ω = 121 Ω**~~ | **WRONG — not needed.** The SN65HVD230 module has 120 Ω onboard: `R2` = `121`, across CANH–CANL. `R1` = `103` = 10 kΩ is the RS slope resistor. Fitting the pair as well gives a third terminator (~40 Ω). See TRAPS. |
| transceiver **GND** | R48 **DC−** — the JST carries only H and L, and the transceiver needs a common-mode reference |
| NodeMCU power | **USB wall wart, not the laptop** — telnet is the console |

### What success looks like
`can` over telnet → state RUNNING, rx counting up, **Vout ≈ 53.5 V** against the
KAIWEETS on the open terminals, Iout ≈ 0. Whichever of `Iout[0x81]` / `Iout[0x82]`
tracks the DC clamp later is the real current sub-ID; the other is discarded.

### If it does not
- `BUS-OFF` → termination. Check the 220∥270 is actually across H and L.
- `tx` climbing, `rx` stuck at 0 → R48 not on mains, H/L swapped, or no GND to DC−.
- Nothing at all → transceiver RS pin; the module's 10 kΩ to GND should hold it in high-speed mode.

## 2026-09-04 — CAN two-node loopback PASSES on Waveshare transceivers

**16/16 round trips, ~1.4 ms RTT, zero errors, first attempt** — once both
ends were verified *before* the bus went between them.

What was proven, in order, each by a direct measurement at the part:

1. All three hiBCTR blue "VP230" modules **never drive the bus** — D toggling at
   125 kHz at the chip lead, supply steady, `RS` in slope control, output stage
   never engages. Return in progress; see `docs/return-sn65hvd230-modules.md`.
2. Two Waveshare SN65HVD230 boards (Amazon, sold by Waveshare) **drive**:
   `CANH`/`CANL` split to 2.8/1.6 V and 2.7/1.6 V under the same stimulus.
   Onboard 120 Ω on each, so no external termination.
3. With both drivers proven and both boards on the Mac's USB (common ground, no
   bonding wire), the loopback echoed every frame.

**Consequence for the R48:** every earlier "rectifier not answering" was measured
through a transceiver that could not transmit. The question has not yet been
asked. Before it can be: the firmware still carries the Huawei request ID
(§6.13, withdrawn) and must be rewritten to the Vertiv protocol (§6.15). A
passive `canraw` listen needs no protocol and is the zero-risk first step.

**Bench state:** dev board (`usbserial-4`, ina228 firmware, Waveshare on
`P14`/`P32`) and new board (`usbserial-0001`, `can_loop_listener`, Waveshare).
Blue modules 1–3 marked and set aside for return.
