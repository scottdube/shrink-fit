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
| **DC contactor** (series, positive-opening) | 1 | MR-23, SR-03 | $35 |
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
| ACS758 Hall current sensor (hedge, see below) | CR-01 / R-11 | $21 |
| MAX31855 **and its own K-type probe** | CR-02 | $14 + $14 |
| Coolant flow switch | CR-06 | $21 |
| E-stop + encoder | CR-09 | $42 |

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
