# Shrink-Fit Induction Controller — PCB

KiCad 10 project. **Milled in-house, single-sided**, so the constraints that
decide this layout are not the ones a board house would impose.

## Scope: full controller, staged populate

Decided 2026-08-25. Every Phase 2 block gets a footprint and a header; only the
Phase 1 blocks get populated now.

The reasoning is *not* the enclosure's "expensive to redo" argument — a milled
board is an afternoon, and copper is the cheap part. It is that **a respin costs
desoldering and re-soldering everything already on the board.** A footprint on a
board you are milling anyway costs nothing, so the asymmetry runs entirely one
way.

Rejected: a Phase-1-minimum board. It would route faster and mill sooner, but it
buys a few hours against a guaranteed rebuild of the one thing on this project
that has to be *reliable* rather than fast.

## Blocks

| | Block | Phase | Parts |
|---|---|---|---|
| ✅ | MCU | 1 | U1 ESP32-S3 Super Mini (HW-747) |
| ✅ | SSR drive | 1 | Q1 2N3904, R1 1 k |
| ✅ | Bus voltage sense | 1 | R2 220 k, R3 10 k, C1 100 nF |
| ✅ | Discrete inputs | 1 | flow switch, E-stop sense, start button |
| ⬜ | Isolated CAN | 2 | U2 SN65HVD230, U3 ADUM1201, U4 B0505S-1W |
| ⬜ | Thermocouple | 2 | U5 MAX31855 (SPI) |
| ⬜ | Bus current / energy | 2 | U6 INA228 (I²C, on the 100 A shunt) |

The SSR-100DD, its heat sink, and the MBR60100PT freewheel diode are
**chassis-mounted, not on this board** — they carry bus current and need the
heat path.

## Pin map — from `firmware/include/config.h`, not restated

| GPIO | Net | Block | Note |
|---|---|---|---|
| 1 | `BUS_SENSE` | bus divider | ADC1_CH0. ADC1 avoids the WiFi conflict on ADC2 |
| 2 | `START_BTN` | input | not GPIO 3 — that's a strapping pin (JTAG source select) |
| 3 | *free* | — | reserved for the Stage-4 encoder |
| 4 | `CAN_TX` | CAN | TWAI. **Idle is recessive = high** — if this sits low before the driver installs, the transceiver jams the bus |
| 5 | `CAN_RX` | CAN | |
| 6 | `FLOW_SW` | interlock | active high = flow proven |
| 7 | `ESTOP_SENSE` | interlock | **read-only status.** The E-stop itself is hardwired in series with the contactor coil (SR-03) and this pin must never be presented as one |
| 8 | `SDA` | I²C | INA228 @ 0x40 |
| 9 | `SCL` | I²C | |
| 10 | `SSR_DRV` | SSR drive | CR-05: this pin delivers the voltage **step**. CAN sets bus voltage and must never enable the output — a ramp is what causes ZVS shoot-through |
| 11 | `TC_CS` | thermocouple | |
| 12 | `TC_SCK` | thermocouple | |
| 13 | `TC_MISO` | thermocouple | |

GPIO 33–37 are electrically free (measured: no PSRAM) but land on inner pads, so
they need soldering rather than a header. Not used.

## Grounding

**The controller floats with the bus, referenced to DC−.** The DC− → earth bond
was withdrawn: the chassis is earthed through the AC cord regardless, and bonding
*creates* a 168 V shock path for the dominant fault (coil → holder).

Consequences that bind this layout:
- The bus divider references DC−, which is board GND. Low-side sensing is
  therefore near zero common mode.
- **CAN must be isolated** — U3/U4 exist for this reason, not for noise.
- **No USB while energised.** The USB shield ties board GND to the host.

## Single-sided layout constraints

- All routing on **B.Cu** with a GND pour on B.Cu. THT parts mount on the bare
  top face and solder on the copper face, so **any SMD part must be flipped to
  B.Cu**.
- Crossings become top-side jumper wires. If it will not route on one layer with
  a pour and a few jumpers, order it instead.
- **Don't thread traces between 0.1" header pads** — ~0.84 mm between 1.7 mm pads
  at 2.54 mm pitch is too tight to isolate reliably. Route around.
- **Socket rows get 1.78 mm pads, not the stock 1.70.** Stock KiCad
  `PinSocket_1x*_P2.54mm_Vertical` passes pcbmill's 0.25 mm annulus gate by
  **9.5 µm** — technically fine, no margin. 1.78 gives 0.300 mm.
- **Zone minimum width ≥ 0.5 mm.** KiCad defaults to 0.25 mm, which lets the pour
  create slivers narrower than the V-bit can cut; the mill leaves them as stray
  copper that looks like nothing on screen.
- Pad connections: **thermal reliefs**, not solid.
- Isolation barrier between CAN primary and secondary gets an explicit **Rule
  Area keepout**, not default clearance.
- **Keepout under the Super Mini's antenna end.**
- **NPTH mechanical holes** (hole, no pad) under any module pin that lands on the
  MCU's internal flash bus, so those pins pass through touching nothing.

## Open — blocking layout

1. **ESP32-S3 Super Mini footprint does not exist.** Nothing in `~/code` or the
   stock KiCad libraries has it. It must be built from the physical board, and
   the two failure modes here are the expensive ones: mirrored handedness, and
   wiring by pin *number* when the symbol is numbered by header position but
   named by GPIO. See the measurement list below.
2. **No 5 V / 12 V source is on the BOM.** The bus is 41.5–53.5 V and nothing
   steps it down. Either a buck gets specified, or the controller is fed
   separately — and if separately, that supply's ground reference has to be
   settled against the floating-bus decision above before layout.
3. **Mounting is undecided** because the enclosure is. Holes are free while
   milling and painful to retrofit next to a pour, so the enclosure has to be
   picked — or at least its mounting pattern fixed — before the board outline is.

## Measurement list — Super Mini

Component side up, USB connector away from you. Calipers plus reading the silk:

- Board **length × width**, and how far the USB-C shell **overhangs** the edge
- **Pins per row**, and confirm the **2.54 mm pitch**
- **Row-to-row centre distance**
- Distance from **pin 1 centre** to the two nearest board edges
- The **silk label of every pin, in order, down each row** — left row top to
  bottom, then right row top to bottom
- Which end carries the **PCB antenna**
