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
| 25 | Lamp **GREEN** — IDLE | 12 V panel lamp via 2N3904 |
| 26 | Lamp **AMBER** — COOLDOWN | |
| 27 | Lamp **RED** — HEATING | FAULT = red flashing, others dark |
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

### Verification before copper

```
grep -A2 '"Reference" "U1"' *.kicad_pcb | grep -B1 'net "'
```

Every net must land on the pad number this table gives, not on the pad whose
silk name matches. That check is the one that catches the failure; ERC will not.
