# Controller pin map — NodeMCU-32S (38-pin ESP32)

Board decided and identified in `requirements.md` §6.12. This is the document the
schematic gets built from; **settle it here first**, because changing the MCU
re-pins everything and the existing KiCad schematic is stale against it.

## The board's rules — these are not preferences

| Pins | Rule |
|---|---|
| **6, 7, 8, 9, 10, 11** | **Internal SPI flash. Unusable.** NPTH holes on the PCB, no copper — and do not delete the holes or the module cannot seat. |
| **34, 35, 36, 39** | **Input only.** No output, no internal pull-up. Fine for buttons and pulses, useless for lamps. |
| **0, 2, 12, 15** | **Strapping pins.** 0 and 15 must be HIGH at boot; 2 LOW or floating; **12 must be LOW** — a pull-up there prevents boot entirely. |
| **1, 3** | TX0/RX0, the USB console. Leave them. |
| ADC2 (0, 2, 4, 12–15, 25–27) | **Dead while WiFi is active.** Any analog input must be on ADC1 (32–39). |

Leaves **16 unrestricted output-capable** pins (4, 5, 13, 14, 16–19, 21–23,
25–27, 32, 33) plus **4 input-only**.

## Assignment

### Outputs

| GPIO | Function | Note |
|---|---|---|
| 4 | **SSR control** | via 2N3904, pattern already proven |
| 16 | **AC contactor drop relay** | controller's path to kill mains (§6.9) |
| ~~25~~ | *(free)* | lamps moved to the PCF8574 |
| ~~26~~ | *(free)* | |
| ~~27~~ | *(free)* | |
| 5 | TFT **CS** | |
| 17 | TFT **DC** | |
| 18 | TFT **SCK** | VSPI default |
| 23 | TFT **MOSI** | VSPI default. MISO unused — display is write-only |

TFT **RST** ties to board reset and **BL** ties on: two pins saved, no function lost.

### Buses

| GPIO | Function |
|---|---|
| 21 | I²C **SDA** — INA228 |
| 22 | I²C **SCL** |
| 33 | **1-Wire** — both DS18B20 coolant probes share it |
| 14 | **CAN TX** (TWAI) |
| 32 | **CAN RX** (TWAI) |

**CAN RX must not land on GPIO 12.** A transceiver's RX idles HIGH, GPIO 12 must
be LOW at boot, and the board simply would not start — a fault that looks like a
dead module.

### Inputs

| GPIO | Function | Note |
|---|---|---|
| 34 | **CYCLE START** | input-only, needs external pull-up |
| 35 | **CYCLE STOP** | input-only, needs external pull-up |
| 36 | **E-stop sense** | so firmware knows why it stopped |
| 39 | **Flow pulse** | Hall sensor, PCNT counts it |
| 13 | **Encoder A** | PCNT quadrature |
| 19 | **Encoder B** | |
| 15 | **Encoder switch** | strapping pin, must be HIGH at boot — a pulled-up button idles HIGH, so this is compatible **by luck, and worth re-checking if the button ever changes to active-high** |

**MAINS START is not here.** It is hardwired in the contactor coil circuit
(§6.9) and never reaches the MCU. Firmware must not be able to energise mains by
itself.

**34–39 have no internal pull-ups.** Every button on them needs an external
10 kΩ to 3V3, or it floats and reads randomly.

## Headroom — and how to get it without giving anything up

**21 pins assigned. The only spares are 0, 2 and 12 — all strapping pins**, all
awkward. That is tight: any new requirement lands on a strapping pin or displaces
something.

**The fix is the PCF8574, not dropping anything.** Put the three lamps on an I²C
expander:

- Lamps are **slow static outputs** — the one load an expander is genuinely good at
- **The I²C bus already exists** for the INA228, so it costs zero MCU pins
- **GPIO 25, 26, 27 come back** — three clean spares, ~$1

That leaves the board with the display fitted, the lamps driven, and real
headroom. `roadmap.md` reached for the same part when the Super Mini was the
constraint; the reasoning survives the board change.

**An earlier draft of this section led with "drop the TFT and four pins come
back."** That framed a decision Scott had already taken — he wants a display and
an encoder for settings — as though the display were the problem. It is not. The
lamps are simply the cheapest thing to move off the MCU.

## Deliberately unassigned

- **Contactor aux "mains live" sense.** The spare 1NC on the MC-9b would tell
  firmware the contactor actually pulled in rather than that it was merely
  commanded. Genuinely useful, no pin for it while the TFT is fitted.
- **MAX31855 / thermocouple.** Deleted — CR-02 rewritten (see TRAPS).
- **PZEM.** Retired.

## Still to settle before copper

- Whether the TFT is in scope for the first board.
- Lamp drive: 12 V panel lamps (#722/#724) each need a 2N3904, same as the SSR.
- Pull-up values and placement for 34/35/36/39.

## Symbol pin numbers — the table that prevents the wrong-pad failure

**Symbol and footprint are the proven pair from `sln-geo-aux-heat`**, now copied
into this project: symbol `NodeMCU-32S`, footprint `NodeMCU-32S_38pin_THT`. Both
are on a board that was built and works.

**On this symbol the pin NAMES and NUMBERS deliberately disagree** — names are
GPIO, numbers are header position. **Pin number 26 is the pin named P32.**
Wiring by number lands nets on the wrong GPIOs and passes ERC, DRC and visual
review. Wire by NAME; verify against this table.

| Function | GPIO | Symbol name | **Pin #** |
|---|---|---|---|
| SSR control | 4 | P4 | **13** |
| AC contactor drop relay | 16 | P16 | **12** |
| Lamp GREEN | 25 | P25 | **28** |
| Lamp AMBER | 26 | P26 | **29** |
| Lamp RED | 27 | P27 | **30** |
| TFT CS | 5 | P5 | **10** |
| TFT DC | 17 | P17 | **11** |
| TFT SCK | 18 | P18 | **9** |
| TFT MOSI | 23 | P23 | **2** |
| I²C SDA | 21 | P21 | **6** |
| I²C SCL | 22 | P22 | **3** |
| 1-Wire (DS18B20 ×2) | 33 | P33 | **27** |
| CAN TX | 14 | P14 | **31** |
| CAN RX | 32 | P32 | **26** |
| Cycle START | 34 | P34 | **24** |
| Cycle STOP | 35 | P35 | **25** |
| E-stop sense | 36 | P36_VP | **22** |
| Flow pulse | 39 | P39_VN | **23** |
| Encoder A | 13 | P13 | **34** |
| Encoder B | 19 | P19 | **8** |
| Encoder switch | 15 | P15 | **16** |

**Power:** 3V3 = pin **20** (power_out — the module *supplies* 3.3 V), 5V = pin
**38** (power_in), GND = pins **1, 7, 33, 37**.

**FLASH PINS — NPTH, no copper:** CLK **19**, SD0 **18**, SD1 **17**, SD2 **35**,
SD3 **36**. These are the internal SPI flash bus (GPIO 6–11). Holes with no pad
so the module's pins pass through touching nothing. **Do not delete the holes** —
without them the module cannot seat.

## Physical header positions, counted from the USB end — MEASURED 2026-09-07

The symbol-pin table above is for KiCad. This is for **wiring at the bench**,
where Scott counts holes from the USB connector because the silk is unreadable in
place. The two are different numbering systems and confusing them wasted an hour.

**Edge A** — the edge carrying `3V3`, `EN` and `5V`. Verified by measurement:
position 6 = GND and 19 = 3V3 read 3.3 V apart; CAN worked on 8 and 13; the
status LED works on 11; Scott counted `P27` at 9 independently.

| Pos | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|---|---|---|---|---|---|---|---|---|---|---|
| | `5V` | `CMD` | **`SD3`** | **`SD2`** | `P13` | **`GND`** | `P12` | **`P14`** | `P27` | `P26` |

| Pos | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 |
|---|---|---|---|---|---|---|---|---|---|
| | **`P25`** | `P33` | **`P32`** | `P35` | `P34` | `SVN`/39 | `SVP`/36 | `EN` | **`3V3`** |

**Edge B** — **the pin nearest the USB is `CLK`, NOT `GND`.** An earlier draft of
this table put a phantom `GND` first, which made every Edge B position one too
high and sent the duration button to the wrong hole.

| Pos | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
|---|---|---|---|---|---|---|---|---|---|
| | **`CLK`** | **`SD0`** | **`SD1`** | **`P15`** | `P2` | `P0` | **`P4`** | `P16` | `P17` |

| Pos | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 |
|---|---|---|---|---|---|---|---|---|---|
| | `P5` | `P18` | `P19` | `GND` | `P21` | `RX` | `TX` | `P22` | `P23` |

**Bold = confirmed on the hardware.** `P15` at B4 is Scott's, found by silk after
the button failed at my B5. **The rest of Edge B is still inferred** — treat B7
(`P4`, the SSR) as unconfirmed until something is wired there and seen to work.

**Flash pins are positions 1–4 on Edge B and 2–4 on Edge A.** Nothing goes there.

### Bench wiring as built, 2026-09-07

| Signal | GPIO | Edge | Pos | State |
|---|---|---|---|---|
| Status LED (+330 Ω) | 25 | A | 11 | working |
| Duration button | 15 | B | **4** | working |
| START button (no resistor) | **27** | A | **9** | moved off 34 — see below |
| STOP button (no resistor) | **26** | A | **10** | moved off 35 — see below |
| SSR | 4 | B | **7** | not wired |
| 3V3 / GND for pull-ups | — | A | 19 / 6 | — |

### The bench rig diverges from this map on START/STOP — deliberately

**PCB: keep 34/35.** The reasoning in the Inputs table stands — input-only pins
are exactly right for buttons on a board where the pull-ups are printed in copper,
and it preserves the flexible pins.

**Breadboard: 27 and 26.** On a bench rig those same pins cost an afternoon:

- No internal pull-up, so an unconnected pin **floats LOW and reads as pressed** —
  it fired a 10 s cycle by itself the first time the board armed.
- They cannot be identified by grounding them, so the `pins` command is blind to
  them and every mapping question had to be answered by meter and inference.
- A pull-up landing on the **switch side** of the wire rather than at the pin
  turns *disconnecting the wire* into *starting a cycle*, which is a genuinely
  confusing symptom to debug.

GPIO 26 and 27 are free by design (lamps moved to the PCF8574), have real internal
pull-ups, and take a button straight to GND with no resistor — the same
arrangement as the duration button on GPIO 15, which worked first try.

**When the PCB is laid out, change `START_BUTTON_PIN`/`STOP_BUTTON_PIN` in
config.h back to 34/35 and fit the pull-ups in copper.** The divergence is in
config.h with this reasoning beside it, not hidden.

### Verification before copper

```
grep -A2 '"Reference" "U1"' *.kicad_pcb | grep -B1 'net "'
```

Every net must land on the pad number this table gives, not on the pad whose
silk name matches. That check is the one that catches the failure; ERC will not.

## DECIDED 2026-08-31 — TFT on the first board, lamps on the expander

**TFT is in scope for board one.** Assigned: CS 5, DC 17, SCK 18, MOSI 23.

**Three lamps move to a PCF8574** on the existing I²C bus. **GPIO 25, 26 and 27
come back as clean spares**, so the board has real headroom with the display
fitted.

### The lamp drivers change from 2N3904 to 2N7002 — this is not cosmetic

**PCF8574 outputs are quasi-bidirectional.** Driven LOW they sink 25 mA happily.
Driven HIGH they are only a **weak pull-up sourcing ~100 µA.**

**That breaks the 2N3904 pattern proven on the SSR.** An NPN needs base current
*sourced into* it: 100 µA × hFE 150 ≈ 15 mA of collector current — marginal for a
lamp and worse over temperature. The failure would be dim or intermittent lamps
that work on the bench and sag in the panel, which is a miserable thing to chase.

**A MOSFET gate is voltage-driven, not current-driven.** The weak pull-up charges
it fine — lamps do not care about switching speed. **2N7002, 50 in stock at
RB-12** (AO3400A also on hand, lower Rds(on), same package).

```
PCF8574 Px ──[ gate ]  2N7002  ──┬── lamp cathode
                       source ── GND     lamp anode ── +12 V
```

**Addressing:** PCF8574 sits at 0x20–0x27, PCF8574**A** at 0x38–0x3F. The INA228
is at 0x40 — no collision either way. Five expander outputs remain spare.

**Ordered 2026-09-02: Bridgold 5-pack, $8.69, ASIN B0GF1Q1GNG.** The listing
contradicts itself — title says PCF8574**AP**, package contents say PCF8574**P**.
**Read the marking on the chip before setting the firmware address**: P = 0x20
(with A0–A2 grounded), AP = 0x38. Same pinout, same board, one constant differs.
The `scan` command on the bring-up sketch reports whichever it actually is. Nothing else — the MOSFETs are in
stock and the SSR keeps its proven 2N3904, since it is driven from a real GPIO
that can source properly.
