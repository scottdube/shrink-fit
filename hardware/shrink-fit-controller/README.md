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

1. **Super Mini footprint** — symbol done and verified against three sources;
   the *footprint* needs **one** more number, the row-to-row hole centre
   distance. See Dimensions below. Handedness is settled.
2. ~~No 5 V / 12 V source~~ — **resolved.** A 12 V supply already exists for the
   fans and pump, and there are 15 LM2596 buck modules in stock. This introduced
   a grounding constraint instead: see Power below, and note **the deciding
   meter reading has not been taken.**
3. **Mounting is undecided** because the enclosure is. Holes are free while
   milling and painful to retrofit next to a pour, so the enclosure has to be
   picked — or at least its mounting pattern fixed — before the board outline is.

## Super Mini pinout — verified

Read off photographs of both faces of the board (InvenTree part #60, taken
2026-08-15). **HW-747 V0.0.2.** Component side up, USB-C at the top:

| Pin # | Name | | Pin # | Name |
|---|---|---|---|---|
| 1 | TX | | 18 | 5V |
| 2 | RX | | 17 | GND |
| 3 | **GPIO1** | | 16 | 3V3 |
| 4 | GPIO2 | | 15 | GPIO13 |
| 5 | GPIO3 | | 14 | GPIO12 |
| 6 | GPIO4 | | 13 | GPIO11 |
| 7 | GPIO5 | | 12 | **GPIO10** |
| 8 | GPIO6 | | 11 | GPIO9 |
| 9 | GPIO7 | | 10 | GPIO8 |

Left row is pins 1–9 top to bottom; right row is 10–18 bottom to top (DIP order).

> **Wire every net by pin NAME, never by number.** The numbers are header
> positions and the names are GPIOs, so they collide misleadingly — `GPIO1` is
> pin number **3**, and `GPIO10`, which drives the SSR, is pin number **12**.
> Wiring by number lands nets on the wrong GPIO and survives ERC, DRC and visual
> review. After wiring, confirm which pad each net actually landed on in the
> `.kicad_pcb`.

Inner pads (not on the headers, need soldering): 14–17, 19, 21, 33–42, 45–48.
This confirms the `config.h` comment. The **antenna is at the end opposite the
USB-C**; BOOT and RESET buttons are at the USB end.

## Dimensions

From a dimensioned drawing supplied 2026-08-25. It is a **third independent
source** and arrives rotated 90 degrees from both photographs, with the pin
labels still matching — so the pinout above is confirmed by three views in three
orientations, which is the only way to rule out a mirror.

| | mm |
|---|---|
| Board, along the pin rows | **23.50** |
| Board, across the rows | **18.00** |
| Pin 1 to pin 9 centres | **20.32** |

**The 20.32 self-validates the pitch:** 8 gaps x 2.54 = 20.32 exactly. Pitch is
2.54 mm and does not need measuring.

**Derived, not given:** 23.50 - 20.32 = 3.18 mm of board beyond the end pin
centres. If the row is centred, that is **1.59 mm** from each end pin centre to
the board edge — and 1.59 mm is 1/16 inch, which is consistent with symmetry.
Treat it as a prediction to confirm, not a dimension to build on.

### Still needed — one number

**Row-to-row hole centre distance.** The drawing does not dimension it, and it
cannot be read off a render: the pads are ovals that overhang the board outline,
so the hole centres sit inboard of the 18.00 mm edge by an amount the drawing
does not give.

> **Prediction to check, not to assume: 17.78 mm (0.700 in).** These modules are
> built to straddle a breadboard centre channel, 0.7 in is the standard for that,
> and it leaves ~0.11 mm of board outside each hole centre — consistent with pads
> that overhang. If calipers say otherwise, the calipers win.

Also worth a number for the enclosure, though not for the footprint: how far the
**USB-C shell overhangs** the board edge.

## Power

**Resolved 2026-08-25:** a 12 V supply already exists for the fans and pump.

| Role | Part | Stock |
|---|---|---|
| 12 V -> 5 V logic | [#28 LM2596 Buck ADJ](http://192.168.50.10:8001/web/part/28/) | 15 |
| Isolated fallback | [#491 HLK-5M05B AC-DC 5 V 5 W](http://192.168.50.10:8001/web/part/491/) | 3 |
| CAN-side isolation | [#27 B0505S-1W](http://192.168.50.10:8001/web/part/27/) | 5 |

### The constraint this creates

The controller **must** sit at DC-. That is forced by the sensing, not chosen:
the bus divider references DC- and the INA228 has to touch the shunt. Sensing
dictates the isolation architecture, never the other way round.

An LM2596 is a **non-isolated** buck — output ground is input ground. Feeding the
controller from the 12 V rail therefore ties **12 V PSU V- to 48 V DC-**. If that
supply's negative is earthed, or Y-capped to earth as most SMPS bricks are, then
**DC- is earthed** — which silently re-creates the SR-11 bond that was
deliberately withdrawn, and re-creates it in the one machine where the operator's
hand is inside the coil during the cycle.

**The supply's own plug answers most of this, and a 2-prong wall wart answers it
in the good direction.** Class II means there is no earth conductor, so the
output *cannot* be earth-bonded through its cord and tying V- to DC- imports no
earth reference. The SR-11 concern dissolves for that case.

An earlier draft of this section proposed metering V- against the earth pin of
the supply's own AC cord. **That test does not exist on a 2-prong supply** — there
is no such pin. Corrected test, if one is wanted: meter V- against the
*machine's* chassis earth, which is earthed through the NEMA 6-20 cord. That is
where an unwanted bond would actually appear. Expect open.

**The answer flips if the 12 V supply is not a wall wart.** An enclosed or
open-frame supply (Mean Well LRS and similar) mounted inside the machine has an
earth terminal, is usually Y-capped to it, and frequently bonds V- to earth by
default or by jumper. Confirm which type is fitted before trusting the paragraph
above.

### Residual: the Y-cap, and why it reinforces an existing rule

A switching wall wart typically bridges primary to secondary with a Y-cap. That
couples the output to **mains, not earth** — so the whole LV system can float at
roughly half mains AC relative to earth, current-limited to a few hundred uA.
This is the familiar laptop-brick tingle. It is not the SR-11 hazard: there is no
DC fault path and the current is capped.

It does give **"no USB while energised"** a second, independent reason. The host
is earthed, the controller floats, and the USB shield bonds them. The same
applies to an earthed scope ground clip — which matters, because VR-05 calls for
scoping Vds. Use a differential probe or a mains-isolated scope, never a bare
ground clip on this machine. A linear (transformer) wall wart has no Y-cap and
genuinely floats.

### The concern that survives

With earthing resolved, **noise becomes the dominant issue, not safety.** The
same rail runs the pump. An inductive motor sharing the controller's ground
return puts switching transients on the exact reference the bus divider measures
against, and a wall wart sized for a pump will also sag on pump start — which the
ADC sees as a bus-voltage change that never happened.

**STATUS: supply type not yet confirmed; nothing measured.** The BOM carries the
LM2596.

### Independent of that result

The pump and fans must not share the controller's ground return. The pump is
inductive and its switching transients would ride on the very reference the bus
divider measures against. **Star the controller's feed at the PSU terminals** —
never daisy-chained off the pump — plus bulk and ceramic caps at the module
supply pin.
