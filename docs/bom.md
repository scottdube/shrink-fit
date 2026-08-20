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
| Tank caps | 0.33 µF / 1200 V / 150 kHz MKPH — count TBC | §2 |
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
