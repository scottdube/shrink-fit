# Bill of materials

Budget from [`requirements.md`](requirements.md) header: **$250–500 excluding the
kit**. Status as of 2026-08-09.

> ### Pricing rule
>
> **Unverified prices carry a +40% escalator and are marked ᵉ.** Prices without
> a marker are actual, from receipts.
>
> Estimates sourced from training data run systematically low — stale by several
> years, anchored on the cheap end of forum-reported pricing, and blind to
> shipping, tax, minimums, and tariffs. Observed error on this project: UT210E
> quoted $40, actual **$65.99** (+65%).
>
> The bias is not uniform — 3/16" copper tube was quoted $15, actual **$8.69**
> (−42%). Branded instruments skew low, commodities skew high. The escalator is
> deliberately applied anyway: overestimating a $9 tube costs nothing,
> underestimating a $66 meter breaks a budget.

## Verified prices

| Item | Actual |
|---|---|
| 3/16" OD (5 mm) × 5/32" ID (4 mm) copper tube, 5 ft, T2 seamless | **$8.69** |
| Brass tee compression, 3/16" OD union, 5-pack | **$12.99** |
| UNI-T UT210E clamp meter *(declined — see below)* | **$65.99** |

## In hand (kit, eBay 286924140726)

| Item | Detail | Ref |
|---|---|---|
| Rectifier | R48-3000e3, 53.5 V / 56.1 A on 240 V | §2 |
| ZVS board | WMY TECH, 4× **IRFP260N** (200 V) | §2 |
| Tank caps | 0.33 µF / 630 VAC / 1200 VDC / **50 kHz** MKPH, 12 off | §2 |
| DC chokes | 2× iron-powder toroid, heavy gauge | §2 |
| Metering | PZEM-025 + 100 A shunt | §2 |
| DC breaker | CHCYZO DZ47-63DC B63 | §2 |
| Cooling | 2× 80 mm fan, 12 V pump, tubing | §2 |
| Work coil | 60 mm ID — **to be replaced** | MR-01 |

## To buy

| Item | Qty | Req | Est. |
|---|---|---|---|
| 3/16" copper tube, coil stock | 3 ft | MR-02 | $15 |
| Garolite G-11, coil former stock | 1 | MR-11 | $35 |
| Mica washers / sheet | — | MR-10 | $15 |
| Copper sheet, focus discs | 1 | MR-07 | $20 |
| **Freewheel diode, ≥100 V ≥100 A fast recovery** | 1 | **MR-22** | $15 |
| **DC SSR, ≥60 V ≥80 A** | 1 | MR-23 | $30 |
| ~~DC contactor (series, positive-opening)~~ **AC mains contactor, ~25 A** | 1 | MR-23, SR-03 (**revised** — requirements §6.9) | $18 |
| ~~AC mains breaker, dual-pole~~ | — | SR-02 | **$0** — cord-and-plug; the panel breaker protects the circuit and the plug is the disconnect |
| NEMA 6-20 cord + plug | 1 | SR-02 | $20 |
| Mains EMI filter | 1 | SR-12 | $25 |
| ESP32-S3 devkit | 1 | OQ-5 | $15 |
| INA228 breakout | 1 | CR-01 | $12 |
| MCP2515 + **isolated** CAN transceiver | 1 | CR-11 | $20 |
| MAX31855 + K-type probes | 2 | CR-02, CR-07 | $30 |
| Coolant flow switch | 1 | CR-06 | $15 |
| E-stop, start button, encoder | — | CR-09 | $30 |
| Grounded metal enclosure | 1 | SR-01 | $60 |
| BT30 taper socket block (nest) | 1 | MR-17 | $45 |
| Linear rail + carriage | 1 | MR-13 | $30 |
| Finned Al cooling sleeves + blower | — | MR-18, MR-19 | $40 |
| Compression fittings, hose | — | MR-20 | $30 |
| Class X pins, one per size | 6 | §8.2 | $90 |
| Radiator, reservoir, fittings | — | MR-06 | $40 |
| Fasteners, wire, lugs, sleeving | — | — | $40 |

**Subtotal ≈ $737 at raw estimate. With the +40% escalator: ≈ $1,030.**

That is **2–4× the original $250–500 envelope**, not "somewhat over" as
previously stated. The earlier recommendation to revise the budget to $700–750
was itself built on the low estimates.

## Considered and declined

| Item | Cost | Why not |
|---|---|---|
| DC clamp meter (UNI-T UT210E) | $67 | Redundant with two methods already available, and **worse positioned than one of them.** A Hall-effect DC clamp must sit on the DC lead near the tank, where a kW-class 37–90 kHz field attacks it the same way it attacks every other sense lead (R-11). The AC clamp already on hand reads the mains at 60 Hz, feet from the coil — RF-immune by placement. Both of the DC clamp's jobs (PZEM verification, VR-11 shunt calibration) are covered by the mains-clamp method plus CAN self-calibration. Buy it for general shop use if wanted; it is not a project dependency. |

## Budget finding

**The build exceeds the stated $250–500 envelope by roughly $240–490.** Three
line items account for most of the overrun and none were in the original scope:

| Driver | Cost | Why it appeared |
|---|---|---|
| Class X pin set | $90 | §8.2 secondary metrology — genuinely optional, the energy trend is primary |
| Switching + protection (SSR, contactor, diode, AC breaker, EMI filter) | $130 | MR-22/MR-23/SR-02/SR-12 — all added after inspecting the hardware and reference R1 |
| Enclosure + nest | $105 | SR-01 and MR-17 were always required, never costed |

**Recommendation:** revise the budget to **$700–750**, rather than cutting
requirements to fit a number set before the hardware was in hand. The one line
that can be deferred without touching a requirement is the pin set (−$90),
since CR-10's logged energy trend is the primary instrument and the pins are an
independent cross-check, not a dependency.

Nothing in the switching-and-protection group should be cut. MR-22 alone
protects the IRFP260Ns, which R-2 already flags as the highest-likelihood
failure in the project.

---

## What the ZVS kit actually shipped

Read off the seller's own **Package Included** list, eBay item `286924140726`
(WMY-TECH), on 2026-08-25. Ordered 2026-07-29; listed at **$338.36** the day this
was read. Quantities are vendor-stated and the box was received — not divided,
not inferred.

The kit was a single InvenTree record at qty 0, so **every core component of this
machine was invisible to inventory.** Now decomposed the same way the EAONE
resistor and McIgIcM transistor kits were, with `#100` kept at qty 0 so the
purchase history survives.

| InvenTree | Item |
|---|---|
| [#1099](http://192.168.50.10:8001/web/part/1099/) | WMY-TECH 3000 W induction heater ZVS module |
| [#1100](http://192.168.50.10:8001/web/part/1100/) | Copper work coil, **ID 60 mm** — the stock coil |
| [#1101](http://192.168.50.10:8001/web/part/1101/) | Mini water pump |
| [#1102](http://192.168.50.10:8001/web/part/1102/) | **Power adapter 12 V 3 A**, US plug |
| [#1103](http://192.168.50.10:8001/web/part/1103/) | Silicone water tubing, 2 m |
| [#1104](http://192.168.50.10:8001/web/part/1104/) | R48-3000e3 rectifier PSU, 53.5 V 3000 W |
| [#1105](http://192.168.50.10:8001/web/part/1105/) | DC circuit breaker |
| [#1106](http://192.168.50.10:8001/web/part/1106/) | DC 100 A ammeter with shunt |

**No fans are in the kit.** They are mounted on the ZVS module itself, which
settles a question the enclosure provisioning had left open.

### Three things the listing sourced that the docs previously asserted

1. **Foldback is the seller's own spec**, not an inference: *"When the current of
   the power supply reaches 57 A, the voltage will drop to 45 V to prevent power
   overload."* This is why the kit's DC breaker is a **disconnect and not
   protection** — no fault the supply can deliver will ever trip it, and
   everything downstream has to survive 57 A indefinitely.
2. **The PSU is sold USED** — seller states 9/10. It is not a new part, which is
   worth knowing before trusting it to hold a setpoint unattended overnight.
3. **120 V input halves it.** 100–120 VAC gives 53.5 V at only **25.2 A**; the
   full 56 A needs 200–240 VAC. The 240 V feed is not a convenience.

### Still not answered by the listing

**The shunt's mV rating.** The kit lists a "DC 100A Ammeter" and the shunt case
carries no marking. `config.h` uses 0.75 mΩ on the assumption of a common
100 A / 75 mV part and flags it as a **placeholder**. VR-11 stands: determine it
by four-wire injection before any energy dose is trusted.

### Locations are not set

All eight records were created without a `default_location` — the components are
in or around the machine, and per the location policy `default_location` means
*where a spare goes home*, which does not describe any of them. Set them when the
enclosure exists.

## Phase-2 stock check — 2026-08-30

Checked against InvenTree, searching by function rather than by the part numbers
already chosen.

### Covered — buy nothing

| Need | On hand | Where |
|---|---|---|
| CAN transceiver | SN65HVD230 x3 (1 allocated to BO-0002) | RB-25 + B3-R3C6 |
| CAN controller | **none needed** — ESP32-S3 TWAI, proven listen-only at 125 kbps | on board |
| CAN isolation | ADUM1201 x3 | A3-R8C1 |
| Spare transceiver | MCP2562-E/P DIP-8 | B3-R3C1 |

**The BOM line "MCP2515 + isolated CAN transceiver" is stale.** The S3's internal
TWAI replaces the MCP2515; ADuM1201 + SN65HVD230 supplies the isolation.

### Missing — all controller-phase, none blocking bench work

| Item | Req | Est. (+40%) |
|---|---|---|
| INA228 breakout | CR-01 | $17 |
| ~~ACS758 Hall current sensor~~ | R-11 | **$0 — withdrawn** |
| ~~MAX31855 and its own K-type probe~~ | CR-02 | **$0 — removed** |
| Coolant flow switch | CR-06 | $21 |
| ~~E-stop~~ + encoder | CR-09 | **$21** — E-stop on hand, see below |

Nothing in stock does current sensing. The Hall parts on hand (KY-024, A1324LUA-T,
TLV493D) are magnetic field sensors, not current sensors — usable only with a core
and a calibration campaign, which is a project, not a part.

### Reconsider the INA228 against today's evidence

The INA228 is **a shunt sensor with sense leads in the field** — precisely the
architecture that failed today, and R-11 already says assume it is vulnerable.
The DC clamp, Hall with no sense leads at all, sat in the same field and read
true. That is the best evidence this bench has produced about what survives here,
and it points away from shunts.

**Buy both.** The INA228 for its hardware energy accumulator, which integrates in
silicon so loop jitter cannot corrupt the dose and which no Hall sensor matches;
the ACS758 as fallback, because this exact failure has already cost a day.
**Mount the INA228 on the shunt, not at the controller** — R-11's own mitigation,
and the difference between centimetres of lead and feet.

### The thermocouple gets its own probe

The glass-braid K-type on the bench belongs to Scott's DMM; it is meter kit, not
stock, and **it must not be wired into the controller.** Every error caught today
was caught by an instrument sharing no failure mode with the one that lied — the
DC clamp against the PZEM, temper colour against the thermocouple. Merging the
handheld reference into the controller destroys exactly that independence, and
the next controller under-read would have nothing left to catch it.

### MAX31855 removed — coolant is a 1-Wire job (Scott, 2026-08-30)

A K-type thermocouple and a cold-junction-compensating amplifier for water that
never exceeds ~60 °C is instrument mismatched to the measurement. **DS18B20
Waterproof Temperature Probe, 1-Wire, stainless — 2 in stock at B3-R4C8.**

With CR-02 rewritten (see TRAPS) so temperature characterisation happens at
calibration rather than per cycle, and calibration using Scott's own DMM probe
on a drillable witness piece, **nothing left in the design needs a K-type
amplifier.** MAX31855 and both K-type probes come off the list: ~$28 saved and
one less breakout to mount.

Three properties that matter beyond cost:

- **It fails loudly.** 1-Wire reads carry a CRC8, so a corrupted read is
  *rejected* rather than returned as a plausible number. On a bench where two
  instruments spent a day lying convincingly, an interface that cannot lie
  silently is worth more than its accuracy figure.
- **Two probes give ΔT.** Inlet and outlet across the coil, with the CR-06 flow
  switch, is a **calorimetric** measure of power into the coolant — an energy
  figure from water temperature and mass flow, sharing no failure mode with the
  entire electrical chain. A third independent instrument from parts on hand.
- **Built for immersion**, and RB-25 already holds the brass tees to fit it.

**Limit to respect: DS18B20 tops out at +125 °C.** Correct for coolant, useless
anywhere near the nose. The 300–400 °C measurements stay K-type and handheld.

### ACS758 withdrawn — I reasoned from the wrong property (2026-08-30)

Scott asked where it goes in the circuit. **In series with the DC bus, the same
position as the shunt** — and asking that question is what broke the argument
for it.

**It does not inherit the DC clamp's immunity.** The clamp survived today's field
because it has *no galvanic connection to the circuit* and senses at the jaws.
The ACS758 sits in the circuit and sends an analog output down a wire. I reasoned
"Hall sensor worked, therefore Hall sensor" without checking whether the property
doing the work carried across. It does not.

What it would genuinely buy: ~4 V full-scale output against the shunt's ~50 mV,
so identical RF pickup corrupts one and not the other, plus galvanic isolation
between power path and signal.

**What it costs is the whole point of CR-01.** It feeds the ESP32 ADC, so energy
integration moves into software and back under loop jitter — precisely what the
INA228's hardware accumulator exists to remove. ADC2 is unusable while WiFi is
active, constraining pins. And it is another series element in a 56 A path.

**The INA228 is also less exposed than R-11 implies.** The PZEM failed on a *long
analog path* — shunt, leads, then the measuring box. An INA228 close-coupled to
the shunt has roughly a centimetre of analog path and I²C thereafter. Different
architecture, not the same mistake in better packaging.

**Standing plan:** INA228 on the shunt, cross-checked against CAN telemetry, with
the DC clamp as the handheld bring-up reference it has already proven itself to be.

### E-stop on hand — APT LA39 (2026-08-30)

Red mushroom, twist-release, Siemens Electrical Apparatus (Suzhou) manufacture.
Marked **EN 60947-5-1 / GB-T 14048.5**, `Ui AC660V`, `Ith 10A`, **AC-15 DC-13**,
and carrying the **positive-opening symbol** — contacts forced apart
mechanically, so a welded contact still opens. That marking is the one that
matters on an E-stop.

**Correct for the role the design already specifies.** SR-03 is *"E-stop cutting
the DC contactor coil"* — not the bus — and AC-15/DC-13 are coil-switching
utilisation categories. `Ith 10A` is thermal current, not breaking capacity.

**Checks before it is fitted:**

- ~~Confirm the block is NC.~~ **VERIFIED 2026-08-30, Scott:** twisted out
  (released) = closed; mushroom pressed in = open. Normally closed, as required.
  Worth having done on the bench — an E-stop wired through a normally-open
  contact mounts correctly, looks correct, and does nothing.
- **Run the contactor coil at 12–24 V, not 53 V.** `Ui AC660V` is an insulation
  rating. DC arcs have no zero crossing to extinguish at, so DC breaking
  capability is always far below the AC figure.
- **Do not fit a plain flyback diode across the coil.** It is the reflex and it
  is wrong on a safety circuit — a plain diode can double or triple contactor
  drop-out time, making the E-stop slower the moment you add it. Use a **diode +
  zener** or an RC snubber, which clamps the inductive kick without stretching
  release.
- Consider stacking a **second NC block** — two in series is standard practice,
  and LA39 blocks are modular.

Standing caveat from §895: **this does not de-energise the AC side.** The
cord-and-plug remains the disconnect.

### Contactor selected — Baomain MC-9b-24VDC (2026-08-30)

**[ASIN B0B4CBZCBC](https://www.amazon.com/Baomain-Magnetic-Contactors-MC-9b-Listed/dp/B0B4CBZCBC) — $31.16, verified live (no escalator).**

LS Electric Metasol MC-9b frame:

| | |
|---|---|
| AC-1 (resistive) | **25 A** — load is 13.6 A, 1.8x margin |
| AC-3 (motor) | 9 A |
| Rated | 3 HP @ 230 V, 5 HP @ 460 V 3-phase |
| Main contacts | 3 poles, all NO |
| Auxiliary | **1NO + 1NC** |
| Coil | **24 V DC** |
| Ui / Uimp | 690 V / 6 kV |
| Mounting | 35 mm DIN rail, screw terminals, 18–10 AWG |
| Certification | UL, CE, IEC 60947-4-1 |

**The Amazon listing is self-contradictory and nearly disqualified the part.** Its
spec block reads *"AC-3 (Motor Loads): 9A 24V DC"* and *"AC-1 (Resistive Loads):
25A 24V DC"* — literally, DC contact ratings at 24 V, which would be the wrong
part and dangerous on a 240 V feed. **Ui 690 V was the tell**: nobody builds a
690 V insulation rating into a 24 V DC device. Confirmed against LS Electric's
own frame spec — 230/460 VAC, DC coil option. The listing pasted the coil voltage
onto the contact-rating lines.

### Why 24 V rather than Scott's preferred 12 V

No 12 V DC coil exists in this family (56 listings checked; the DC option is
24 V). The 12 V alternatives were a 2-pole with **no auxiliary contact** — so no
latch without an added relay — and a 3-pole ATO at **$118.71 shipping Sept 18–24**,
four times the price and three weeks later.

**24 V is also the better architecture, not a compromise.** A separate ~$15 supply
for the coil keeps its pull-in inrush and collapse transient **off the logic rail
entirely** — which matters given the coil needs a snubber rather than a plain
flyback diode (a plain diode stretches drop-out time on a safety circuit).

### Wiring notes

- **Two main poles switch L1 and L2.** The third stays **unused** — the 12 V rail
  must NOT be on it, because it has to survive the E-stop to keep the pump
  running on a hot coil (§6.9).
- **Aux 1NO is the latch.** Aux 1NC is spare, and is the natural "mains live"
  input to the controller — it reports that the contactor actually pulled in,
  rather than that firmware merely commanded it.

## ORDERED 2026-08-30 — ALL ARRIVED 2026-09-03

Every item below is in hand and on the books. All four are on the **assembly
table** with the rig except the spare flow sensor, which went home to B3-R7C2.

| Item | Price | Status | Notes |
|---|---|---|---|
| **Baomain MC-9b-24VDC** contactor, ASIN B0B4CBZCBC | $31.16 | ✅ PO-0153, assy table | 3NO mains + 1NO/1NC aux, 24 V DC coil, UL |
| **Taiss LA38-11BNRYG** 22 mm momentary ×3 (R/Y/G), ASIN B07GPJ6876 | $14.99 | ✅ PO-0150, assy table | 1NO+1NC each, 22 mm cutout — matches the LA39 E-stop. **3 pieces**, $5.00 each |
| **INA228** module (GODIYMODULES), ASIN B0GX5HM4X3 | $15.99 | ✅ PO-0152, assy table | R002 **already removed**, VIN+/VIN− 35 MΩ. Bring-up passed |
| **Yotache YF-S401 flow sensor** ×2, 0.3–3 L/min, ASIN B0H28L84GL | $17.99 | ✅ PO-0151, split | **2 pieces**, $9.00 each. One to the coolant loop, one spare in B3-R7C2 |

**$80.13 total**, all prices as booked from the orders — no escalator, none
estimated. The flow sensor came in at $17.99 for two rather than the ~$10
placeholder guessed on 2026-08-30.

The flow sensor range matters: **0.3–3 L/min brackets a small 12 V pump**, where
the 1" HVAC paddle switches that dominate search results would never actuate at
this loop's flow rate at all — reading "no coolant" forever, or sticking closed.

### On arrival

- **Contactor:** confirm coil is 24 V **DC** and contacts carry the AC rating,
  not the 24 V the Amazon listing pasted onto them (see the entry above — Ui
  690 V was what settled it). Third main pole stays **unused**; the 12 V rail
  must not be on it (§6.9).
- **Buttons:** verify the 22 mm cutout matches the LA39 E-stop before cutting the
  panel. Red's **NC** block is the mains STOP if one is fitted; the master switch
  may make it unnecessary.
- **INA228: DONE 2026-09-02.** R002 desoldered, VIN+ to VIN− measured **35 MΩ**
  — open, so no pour or stray footprint bridges them.
  Still to do: calibrate the external shunt against the DC clamp — read the
  shunt-voltage register at known current, R = V/I, program the calibration
  register.

  **The "keep the R002 taped to the board" instruction is RETRACTED.** It existed
  only so the module could go back inside the 30-day window if the flat-draw test
  retired energy dosing. **The module is staying:** at 0.66 coupling the 19.5 mm
  coil collapses the bus 52.3 V → 18 V, which is exactly the non-flat load that
  1 Hz CAN telemetry cannot integrate, so CR-01 needs the INA228 after all.
  Received 2026-09-03 (PO-0152), now on the assembly table with the rig.

  **The R002 is stocked separately** at Scott's direction — InvenTree part 1172,
  stock #771, in B3-R3C2. Its tolerance is **unverifiable**: 2 mΩ reads as a dead
  short on every meter here, and desoldering is the stress that cracks a
  low-value chip's terminations. Do not use it as a reference; its one real use
  is putting the module back to stock form if it is ever repurposed.

### Flow sensor — fitting notes

- **Set CR-06's threshold above the sensor's floor, not at it.** If normal flow
  is ~1.5 L/min, trip at ~0.8 — far enough from the 0.3 L/min stall point that a
  slowing turbine does not nuisance-trip mid-cycle.
- **Mount where it stays flooded.** A bubble parked in the rotor reads as no flow;
  avoid the high point of the loop.
- **Supply side, before the coil.** Cooler water, and most YF-series housings are
  only rated ~80 °C — confirm this one.
- **Pulse output beats a contact:** CR-06 gets a real rate to threshold, and a
  slowly declining flow logged over weeks is a clogging loop reporting itself.
  A switch can never say that.

**It also closes the last open number in the power budget.** The pump's current
draw is the only figure in §6.11 estimated across a 3x range, and it sizes the
buck carrying it. One session with the DC clamp and this sensor characterises the
pump — running current, inrush, and what flow you actually get for it — settling
both the buck sizing and CR-06's threshold together.
