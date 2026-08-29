# Roadmap — staged build plan

Single reference for what gets built when, what each option costs in parts and
time, and what the enclosure must accommodate so it is built once.

**Prices carry the +40% escalator and are marked ᵉ** unless verified from a
receipt. Time estimates are rough and skew optimistic the same way prices do —
treat them as ordering information, not schedule.

---

## Stage 0 — Get tools in holders *(now)*

Covered in [`quickstart-1-8.md`](quickstart-1-8.md). 20 mm coil, bare bench,
no enclosure. Two cycles, then stop.

**Do not skip:** water running, stickout set first, taper shielded, coil on the
nose only.

---

## Stage 1 — Safe to leave on a bench *(next)*

Everything here exists because an exposed kW-class 56 A machine is genuinely
dangerous, not because it improves the process. **Nothing in later stages should
begin before this is done.**

| Item | Parts | Time | SW | Benefit |
|---|---|---|---|---|
| **Grounded metal enclosure** | $85ᵉ | weekend | — | The whole point. See provisioning below |
| **E-stop**, hardwired in the contactor coil | $35ᵉ | 1 h | — | SR-03. Must be hardware, never firmware |
| **DC− single-point earth bond** | $0 | 15 min | — | SR-11. Removes floating-rail hazard, makes scope work safe, enables low-side sensing later |
| **Freewheel diode** across ZVS DC input | $21ᵉ | 30 min | — | MR-22. Every cycle termination is an inductive kick into the switch. Found in a *working* reference build |
| **Cord + plug**, NEMA 6-20 | $28ᵉ | 1 h | — | SR-02. The plug is the local disconnect. E-stop does **not** kill AC |
| **Placard** — laser on anodised | $10ᵉ | 1 h | — | SR-13. Five facts the hardware actively contradicts |
| **Mains EMI filter** | $35ᵉ | 1 h | — | SR-12. Shares a panel with PathPilot and probing |
| **Grounding + protection review** | — | half day | — | Decide floating vs bonded on evidence, and size real overcurrent protection. The 63 A DC breaker is a *disconnect*, not protection — the PSU folds back at 57 A so it cannot trip on any fault the PSU can deliver. Everything downstream must survive 57 A indefinitely. See the SR-11 withdrawal note |
| **Contactor**, series with future SSR | $49ᵉ | 1 h | — | Positive opening. An SSR fails *short* and cannot be the E-stop element |

**Stage 1 total ≈ $263ᵉ, ~2 weekends.**

---

## Stage 2 — Repeatable process

Turns it from an experiment into a tool. Roughly in value order.

| Item | Parts | Time | SW | Stageable? | Benefit |
|---|---|---|---|---|---|
| **Holder nest** — commercial BT30 taper block | $63ᵉ | 2 h | — | ✅ | Repeatable seating. Coupling varies with position, so this is accuracy, not convenience |
| **Depth stop / length preset** | $20ᵉ | half day | — | ✅ | Stickout repeatability. You get one shot per cycle |
| **Focus disc + mica sandwich** | $35ᵉ | half day | — | ✅ | Taper protection. One warped taper costs more than this whole stage |
| **K-type thermometer** | $28ᵉ | — | — | ✅ | Post-cycle overshoot check. Keeps holders alive |
| **Timer relay** (dumb dosing) | $21ᵉ | 1 h | — | ✅ | 80% of the repeatability benefit for 5% of the controller effort |
| **Cooling station** — finned sleeves + blower | $56ᵉ | half day | — | ✅ | Cuts cycle turnaround; stops warm-holder overshoot |
| **Vertical rail + hard stops** | $42ᵉ | day | — | ✅ | Prevents coil-to-holder crashes. Cheap motion is fine — this does *not* set runout |

**Stage 2 total ≈ $265ᵉ, ~1 week of evenings.**

---

## Stage 3 — Full size range and real control

Only worth it once you actually want 1/4"–1/2" holders or the process bothers you.

| Item | Parts | Time | SW | Stageable? | Benefit |
|---|---|---|---|---|---|
| **35 mm coil** (+ G-11 former) | $60ᵉ | day | — | ✅ | Covers 1/4"–1/2". §4.3 predicts flat cycle times across the range on one coil |
| **CAN link to rectifier** | $28ᵉ | 1–2 days | **yes** | ✅ | **The only way to adjust bus voltage.** Also current limiting and independent telemetry. **The DHO914 decodes and triggers on CAN natively** — all serial decodes ship enabled, no licence — so protocol porting is observable rather than blind |
| **IRFP264N swap** (250 V) | $22ᵉ | 3 h | — | ✅ | Alternative to CAN for the voltage problem — buys back the 20% power that dropping to 48 V costs |
| **ESP32 + energy dosing** | $42ᵉ | 1–2 weeks | **yes** | ✅ | CR-01. The single largest quality gap vs. commercial. Skeleton already written |
| **INA228 shunt sense** | $17ᵉ | 1 day | **yes** | ✅ | Hardware energy accumulation, immune to loop jitter |
| **Flow + coolant temp interlocks** | $28ᵉ | half day | **yes** | ✅ | Coil melts without flow. Cheap once a controller exists |
| **Cycle logging** | $0 | 1 day | **yes** | ✅ | Release energy tracks bore wear at ~3–4%/µm — better than any affordable gauge |

**Stage 3 total ≈ $197ᵉ, several weekends + firmware.**

---

## Stage 4 — Comfort

| Item | Parts | Time | SW | Benefit |
|---|---|---|---|---|
| HMI — display + encoder | $42ᵉ | 3–4 days | **yes** | Preset selection without a laptop |
| Home Assistant telemetry | $0 | 1 day | **yes** | Trend logging. **Telemetry only — no control path over the network** |
| Scope characterisation (Vds, f) | $0 | 2 h | — | Answers VR-05. Only worth it at real power |

---

## Enclosure provisioning

**The enclosure is the one thing expensive to redo.** Build it for Stage 4 even
if you are only populating Stage 1. Everything below is cheap to provide now and
painful to retrofit.

### Volume to reserve

| Block | Approx. | Notes |
|---|---|---|
| PSU R48-3000e3 | 350 × 110 × 45 mm | Long slim brick; needs its own airflow path |
| ZVS board + heatsinks + **2× 120 mm fans** | **~250 × 140 × 150 mm** | Corrected 2026-08-27 — the fans are **120 mm, not 80**. Two stacked are 240 mm on their own, so the old 200 × 150 figure could not have been right. Push-pull, both on the outer face |
| Pump | 100 × 50 × 50 mm | Wet side |
| **Reservoir + radiator** *(not yet owned)* | 200 × 150 × 100 mm | Provision it — the loop will grow |
| Shunt | 100 × 50 × 40 mm | |
| **Controller bay** *(Stage 3)* | 150 × 100 × 50 mm | Reserve it now |
| **DIN rail** | 250 mm min | Breaker + contactor + SSR + terminals. Only the breaker exists today |

### Provision now, populate later

1. **DIN rail at full length** even if two positions sit empty.
2. **Controller bay with standoffs**, plus a wiring path from it to the shunt,
   the contactor coil, and the front panel.
3. **Front panel cut for Stage 4** — display window, encoder, start, E-stop.
   Blank plates where features are not yet fitted. Recutting a populated panel is
   the worst job on this list.
4. **Spare cable glands, capped.** Two minimum: sensors, and one unknown.
5. **Water bulkhead pass-throughs on a dedicated face**, physically separated
   from electronics (MR-21). Route so any leak drains *out*, not onto the board.

   **Dry-stub arrangement (preferred).** Solid copper rod ~2" from each board
   terminal, through the bulkhead, into a tee on the wet side; coil out the
   opposite tee port, water in the third. The solid rod is conductor and port
   plug in one — water physically cannot reach the board because there is no
   bore for it to travel down, rather than because a seal is holding.

   - No electrical penalty: at ~100 kHz, skin depth is 0.21 mm, so solid rod and
     tube of the same OD have identical resistance
   - ~23 W in the stub at 300 A circulating, conducted out both ends → roughly
     35–40 °C above its ends. Check by hand after a run
   - **⚠️ Insulated bushings on both pass-throughs are mandatory.** Both stubs sit
     at tank potential and at *different* potentials from each other. Bare rod
     through a metal bulkhead shorts the tank to chassis, or the two legs
     together. Rate the bushings well above 70 °C
   - Slope or drip-loop the wet side so water cannot track back along a rod

   This fixes board position, coil-head position, and the wet/dry boundary
   simultaneously — lay it out early.
6. **Ventilation sized for ~2.5 kW**, not today's 300 W. Intake low, exhaust high,
   filtered intake.
7. **Earth stud** — one, clearly marked, everything bonded to it.
8. **Lid interlock switch position**, even if wired as a jumper initially.
9. **Sensor wiring routed away from the tank.** Thermocouple and shunt leads are
   both attacked by the field (R-11); a channel on the opposite side costs
   nothing now.
10. **⚠️ The ZVS board must stand on its supplied standoffs, over open space.**
    Its underside carries exposed, solder-built-up bus traces at tank potential
    running the full board length — the kit sheet warns to "raise the heater
    board with 8 copper studs … to avoid short circuit at the bottom contact."
    **It cannot sit on a metal chassis floor.** Provide an insulated or cut-away
    mounting plate with clearance beneath, and remember the coil terminals exit
    from the *bottom* of the board — that constrains where the coil head can go.

### Architecture — coil to work, or work to coil?

**Geometric constraint:** a BT30 flange (~50 mm) cannot pass through a 20–35 mm
coil bore, so the nose must enter axially. One of them moves. There is no
both-static configuration.

**The bulkhead is a boundary inside the assembly, not a frame member.** It
delivers wet/dry separation, a thermal barrier, and insulated pass-throughs
whether it is bolted to the frame or riding on a slide. If board, bulkhead,
stubs, and coil move *together*, no copper flexes either way — so rigid copper
is **not** what differentiates these options.

| | **A — package moves** | **B — work moves** |
|---|---|---|
| Moving mass | ~2–4 kg (board, heatsinks, fans, coil, tee) | 0.5–0.6 kg holder |
| Flexes | Silicone hose, DC cable, 12 V | Nothing |
| Copper | Rigid | Rigid |
| Insertion force | **Into a solid base** | Pushes carriage down — needs a latch |
| Lift | Counterbalance / gas spring | Hand or simple carriage |
| Precedent | **R1 and most published builds** | Less common |

**DECIDED 2026-08-09 — A, package on the rail.** Both commercial machines and
every published DIY build move the head, not the work. Independent convergence
from two very different cost structures is strong evidence, and the reasons are
concrete: setting a holder onto a flat nest is a natural motion and getting a
300 °C one *out* is much better that way; coil travel serves both loading
clearance and gage-length adjustment with one mechanism; the nest stays a simple
block; and working height stays at bench level.

The flexing-plumbing objection is answered by moving the **board with the coil** —
copper stays rigid, only hose and DC cable flex, both designed for it.

Two requirements survive regardless:
- **The coil gets its own mount.** Legs reach with slack; hand-bent copper must
  never set a ±0.5 mm dimension (MR-24).
- **Hard stop at the bottom of travel**, so repeatability is a property of the
  machine rather than of the operator.

Original reasoning, kept for context:

- **Insertion is the moment that matters** — five seconds, one motion. Holder on
  a solid base means the push goes into the bench, not into a latch.
- **The flex bundle is bounded and does not grow with features.** Future control
  hardware (CAN, INA228, thermocouple, flow switch) lives with the controller and
  PSU, not on the ZVS board. Moving bundle stays at 2× DC, 12 V, and two hoses
  over 50–100 mm of travel.

Cost of A: counterbalance or gas spring so ~3 kg cannot drop onto a holder, plus
a positive bottom stop. R1 uses a manual handle on the column.

**Either way, board orientation is free** — 90°, parallel, or anywhere the two
stubs reach the bulkhead. Constraints on that freedom:

- **Stub length 2–4".** They are part of the tank. ~0.03 µH each at 2" (≈11% on
  a 0.55 µH coil — harmless, and part of the ±20% stray spread). At 8" it is
  ~0.24 µH total, ~44%, which materially retunes the tank.
- **Route the pair close and parallel.** Go-and-return conductors side by side
  largely cancel each other's field, so net added inductance is far lower than
  two stubs at opposite corners. They must not touch, and each needs its own
  insulated bushing.
- **The coil hangs on its former, not the plumbing.** Stubs and tee carry current
  and water; the carrier takes the weight. A compression joint at a live terminal
  under vibration works loose over months.

---

## Dependency notes

- **CAN or the FET swap** must precede running at real power — 53.4 V is 16%
  Vds margin on IRFP260N. Either fixes it; CAN also gives current limiting and
  telemetry, the swap also gives back 20% power.
- **Energy dosing depends on** INA228 + a controller. The timer relay delivers
  most of the benefit meanwhile.
- **Logging depends on** dosing, and is only meaningful if the nest (Stage 2)
  makes seating repeatable. Sloppy fixturing destroys the instrument.
- **The 35 mm coil is independent** of everything else — build it whenever the
  larger holders matter.

---

## State indication — decided in principle 2026-08-27, hardware open

**The machine must show what mode it is in.** Four states matter, and the reason
is not convenience: **three of them are about whether it is safe to touch the
holder.**

| State | Meaning |
|---|---|
| **IDLE** | field off, holder cool, safe |
| **HEATING** | field on — do not approach |
| **COOLDOWN** | field off but the holder is still hot |
| **FAULT** | interlock tripped, dose out of window, over-temp lockout |

`FAULT` does not exist in the firmware yet. It will come from CR-02's over-temp
lockout, CR-03's plausibility window, and CR-06 flow loss.

### Lamps or a round TFT?

Scott raised a 1.28 in round TFT as the richer option. It is, and there is a real
argument for each — but they are not competing for the same job.

**The deciding argument is how each one FAILS.**

A TFT that freezes **still shows its last frame.** A firmware hang, a locked SPI
bus, a crash mid-cycle — the display sits there looking authoritative, reading
HEATING when nothing is heating, or IDLE when the field is on. It fails
*deceptively*, and it fails toward the reading you least want to trust.

A lamp driven by a live firmware loop **stops blinking when the loop stops.** Its
failure is visible and honest: the pattern dies, and the absence of the pattern
is itself the signal.

That is the annunciator-versus-glass-cockpit distinction, and it is why
annunciator panels survived the arrival of glass. It applies here because the
question the indicator answers is *"can I put my hand near that holder"*.

### Decision

**Lamps carry state. A TFT, if fitted, carries numbers.**

- **Three 5 mm lamps — green / amber / red** — for IDLE / COOLDOWN / HEATING.
  All three are in stock, 100 each, at `B3-R2C3`.
- **FAULT = red flashing, green and amber dark.** Distinguished by pattern, not
  colour alone, per the shop rule. A fourth lamp is possible if that proves
  ambiguous in practice.
- **A round TFT is a Stage-4 addition for DATA** — joules delivered, seconds
  elapsed, bus volts, holder temperature. Genuinely useful, and it must never be
  the only thing telling you the field state.
  **Two are already owned** ([#470](http://192.168.50.10:8001/web/part/470/),
  genuine Waveshare 1.28 in GC9A01, not clones — clone pinouts and init sequences
  differ). So the data display costs nothing but firmware, which lowers the bar
  for fitting one. It does **not** change the conclusion above: lamps still carry
  state, because the argument was about failure mode, not cost.

Position also does work here: three lamps in a fixed row mean the *place* that is
lit identifies the state even when the colour is ambiguous. That matters for this
operator specifically.

### The constraint it creates

**The Super Mini's 18-pin header is fully allocated** — GPIO 1–13 plus TX/RX are
all spoken for in `config.h`. Three lamps need three more pins.

| Option | Cost |
|---|---|
| **I²C expander (PCF8574)** on the existing SDA/SCL | 8 outputs for **zero** extra MCU pins. Not in stock, ~$1. **Preferred** |
| Inner pads 33–37 (confirmed free, no PSRAM) | Solder-only — the module stops being socketable |
| Drop the thermocouple | No: CR-02 wants it |

Decide before the controller PCB is laid out; retrofitting three lamp drivers to a
milled single-sided board is the kind of change that costs a respin.

Panel lamps in the final build are 12 V ([#722](http://192.168.50.10:8001/web/part/722/),
[#724](http://192.168.50.10:8001/web/part/724/)), so each needs a transistor —
the same 2N3904 pattern already proven on the SSR.
