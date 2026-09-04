# Return / defect report — SN65HVD230 CAN transceiver breakout modules

**Status:** purchase record filled from InvenTree part 19 / PO-0013. Remaining
placeholders: IC marking lines 2–3 (microscope) and the three photos.

## Purchase

| | |
|---|---|
| Vendor | Amazon marketplace — seller **hiBCTR** |
| Order number | **113-5416189-8729069** |
| Order date | ordered **2026-08-15**, received **2026-08-18** |
| Listing title | hiBCTR SN65HVD230 CAN Transceiver Module (pack of 3) |
| Listing URL | https://www.amazon.com/dp/B0FDLDXCK9 (ASIN **B0FDLDXCK9**) |
| Quantity ordered / on hand | 3 / 3 (all three being returned) |
| Unit price | $2.66 (order total $7.98) |
| InvenTree | part 19; PO-0013; stock items 80 (RB-25, allocated to BO-0002) and 83 (B3-R3C6) |

## Claim

All three modules received are non-functional as CAN transceivers. The
receiver section operates; **the driver section never engages** on any of the
three. Each was tested individually on a bench supply with every input measured
directly at the IC leads, against the manufacturer's datasheet
(TI SLOS346O, *SN65HVD23x 3.3-V CAN Bus Transceivers*).

## Module identification

- Blue 6-pin breakout, silk `3V3 GND CTX CRX CANH CANL`
- `R1` marked `103` (10 kΩ, RS slope-control pull-down), `R2` marked `121`
  (120 Ω termination across CANH/CANL)
- IC marked `VP230` — the marking TI assigns to SN65HVD230
- IC marking, full: `[LINE 1]` / `[LINE 2]` / `[LINE 3]`
- Modules numbered 1, 2, 3. Modules 1 and 2 had been in service briefly;
  **module 3 was new, never previously powered**, and behaved identically.

## Test setup

- Module alone: nothing else on CANH/CANL except its own onboard 120 Ω.
- Supply: bench PSU set to 3.30 V, current-limited to 100 mA; measured
  **3.28 V at the module pins and steady under all test conditions**.
- `D` (driver input) driven from an ESP32 GPIO through a wire measured at 0 Ω;
  pin state read back in firmware; and **verified at the IC lead with a meter**.
- Two DMMs on CANH and CANL to module ground; supply current from the PSU readout.
- Stimulus A: `D` held low (continuous dominant). Stimulus B: **125 kHz 50 %
  square wave on `D`** — a CAN-shaped signal, in case of a dominant-state
  timeout. Meter readings under B are time-averages.

## Measurements at the IC leads (module 3, new; modules 1 and 2 identical)

| Lead | Pin | Idle (recessive) | Driven | Datasheet expectation |
|---|---|---|---|---|
| `D` | 1 | 3.3 V | 26 mV (A) / 1.65 V avg (B) | — input, confirmed driven |
| `GND` | 2 | 0 V | 0 V | — |
| `VCC` | 3 | 3.28 V | 3.28 V, no sag | 3.0–3.6 V |
| `R` | 4 | 2.2 V | 0.2 V | receiver follows bus; VOH ≥ 2.4 V |
| `RS` | 8 | 0.6 V | — | < 0.75·VCC = slope-control mode, **driver enabled** |
| `CANH` | 7 | 1.5 V | **1.5 V (B) / 0 V (A)** | **≥ 2.45 V dominant** |
| `CANL` | 6 | 1.5 V | **1.5 V (B) / 0 V (A)** | **0.5–1.25 V dominant** |
| CANH − CANL | | 0 V | **0 V** | **1.5–3.0 V dominant** |
| Supply current | | 13 mA | 13 mA (B) / 9 mA (A) | ~10 mA recessive; rises when driving into 120 Ω |

Additional: CANH/CANL to GND input resistance **14 kΩ** (datasheet min 20 kΩ);
`R` idle **2.2 V** with only a high-impedance GPIO input attached (datasheet
VOH min 2.4 V *at 8 mA load*). Both outside the SN65HVD230 specification.

## Conclusion

With supply, ground, mode pin and driver input all verified in-spec **at the
IC itself**, the device biases the bus and its receiver output tracks `D`, but
the bus driver produces no dominant state under either a DC or a CAN-rate
stimulus, and supply current does not rise. This is true of all three units,
including one never previously powered. The parts do not function as
SN65HVD230 transceivers and cannot be used on a CAN bus.

The out-of-spec input resistance and receiver output level are consistent with
the IC not being the part its marking claims.

## Requested

Refund for all 3 units ($7.98). Replacement is not requested — a replacement
from the same stock is not expected to differ.

## Attachments

1. `[photo — module top, showing silk and R1/R2 markings]`
2. `[photo — IC marking under magnification]`
3. `[photo — bench setup: PSU readout and both meters during the 125 kHz test]`
4. TI datasheet SLOS346O, §8.5 Electrical Characteristics: Driver

## Manufacturer notification (TI) — suspected counterfeit

TI's stated policy (ti.com anti-counterfeit page) is that verification and
analysis are **not provided for parts bought through unauthorized sources** —
brokers, independent distributors, third-party marketplace sellers. This purchase
is exactly that, so a submission will almost certainly be *recorded* rather than
*investigated*. It is still worth sending: TI collects these reports and the ASIN
and seller identify a live source.

- Address: **ti-counterfeit@list.ti.com**
- Form: TI "Counterfeit Validation Form" (E2E) / "Product Verification Form"
  SSDL004 — attach the measurement table above, the IC photo, and the listing.
- Marking to report verbatim: `VP230` / `[LINE 2]` / `[LINE 3]`
- Framing: **suspected**, not confirmed. The evidence is three units failing
  identically with two DC parameters outside the SN65HVD230 datasheet; TI can
  confirm or rule out the die. We cannot.

---

### Short version (for a vendor message box)

All 3 units are defective: the CAN driver never engages. Tested individually on
a 3.3 V bench supply (steady, current-limited), with VCC, GND, RS and D verified
at the IC pins against TI datasheet SLOS346O. With D driven low, and with a
125 kHz CAN-rate square wave, CANH and CANL stay at 1.5 V with 0 V differential
and supply current does not rise — datasheet requires CANH ≥ 2.45 V, CANL
0.5–1.25 V. One unit was new and never previously powered; it behaves the same.
Input resistance (14 kΩ vs ≥ 20 kΩ spec) and receiver output level (2.2 V vs
≥ 2.4 V spec) are also out of specification. Requesting a refund for all units.
