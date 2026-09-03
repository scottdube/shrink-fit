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

## The headroom is real but the display consumes it

**21 pins assigned. Spares are 0, 2 and 12 — all strapping pins**, all awkward.

**Drop the TFT and 5, 17, 18, 23 come back** — four clean spares, which is
comfortable. **The display is the swing item**, and it is Stage 4 work.

That is worth knowing now: the board has enough pins for everything *including*
a display, with almost nothing left over. Any new requirement after that lands on
a strapping pin or displaces something.

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
