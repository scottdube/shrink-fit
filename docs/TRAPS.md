# Traps — shrink-fit build

Corrections and surprises. Each entry is something that already cost time or
would have.

---

## A downloaded footprint numbered its pads by SILK NAME

`ESP32-S3-SuperMini.kicad_mod`, found online 2026-08-26, is geometrically sound
and was a fourth independent confirmation of the pinout. It was also unusable as
shipped, in a way that produces **no error from any tool**.

Its pads are named `"TX"`, `"1"`, `"5V"` — the silkscreen labels. The project
symbol numbers pins 1–18 by **header position**. So the file's pad `"1"` is
GPIO1's hole, which is our pad **3**. Drop it in unchanged and the netlist maps
most signals onto the wrong physical pins. ERC passes. DRC passes. It looks right
on screen. You find out when the board is cut, populated, and dead.

**Rule: before adopting any third-party module footprint, print its pad numbers
beside its pad names and compare them against the symbol.** Never assume a pad
"number" is a number.

Renumbered to header positions on import; the mapping is in `tools/`-adjacent
history and the footprint's own `descr`.

## …and its pad/drill combination fails this shop's annulus gate

Same file: 1.508 mm pads on a 1.0 mm drill. `pcbmill consolidate_drills` snaps
1.0 to the **nearest owned bit**, which is 1.181 — *upward* — leaving
(1.78−1.181)/2 → for the original 1.508 pad, **0.164 mm** of annulus against a
0.25 mm gate. It fails, and it fails *after* consolidation, so nothing in KiCad
warns you.

Third-party footprints are drawn for board houses that drill any diameter. **They
will not respect an owned bit set.** Fixed here to 1.78 mm pads on a drill
specified as exactly 1.181 → 0.300 mm annulus.

## Row spacing is STILL NOT VERIFIED — do not treat 15.24 as settled

The adopted footprint uses 15.24 mm (0.600 in) row-to-row. That number came from
the found file, not from measurement, and it was accepted here because it was
internally self-consistent — **which is the same error as taking the first search
hit.** An earlier prediction in this project said 17.78 mm (0.700 in); that was
also unverified.

A photogrammetric attempt on 2026-08-26 failed and is worth recording as a
negative result: using the confirmed 2.54 mm pitch as scale, the two pad columns
returned **18.15 mm and 16.80 mm** — disagreeing by 1.35 mm — and the built-in
self-check (pad 1 to pad 9 must equal exactly 20.32 mm) came back 25.16 and
23.79. When that self-check fails the scale is wrong and every derived number
with it. Handheld perspective distortion is the cause.

**Settle it before milling.** Cheapest test needs no calipers: press the module
into a breadboard. **0.600 in straddles the centre channel; 0.700 in does not.**

## A generated schematic stops being generated the moment KiCad opens it

`tools/gen_sch.py` produced the schematic and is the right way to author a pinout
whose truth lives in one reviewed table. But KiCad has now opened, re-serialised
and re-saved the file — re-indenting, alphabetising `lib_symbols`, and rewriting
the root sheet UUID in every symbol's `instances` path.

**The generator is a bootstrap, not a source of truth.** Re-running it now
destroys any GUI edit. Treat the `.kicad_sch` as the artifact from here.

## A `.kicad_pro` cloned from another project carries that project's revision

The project file was seeded from `sln-geo-aux-heat.kicad_pro` to inherit its
proven milled-board DRC. It also silently inherited `sch_revision: "C"`. KiCad
corrected it to "A" on first open, but nothing had flagged it.

**When cloning a project file for its settings, diff it against a fresh one** —
or at minimum grep for revision, title, and text_variables. (`geo` appearing in a
grep of that file is a false positive: `tuning_profile_track_geometries`.)

## KiCad's local-history folder is its own git repo and breaks `git add -A`

KiCad 10 creates `<project>/.history/` containing an *initialised but empty* git
repository. `git add -A` on the parent fails hard:

    fatal: 'hardware/.../.history/' does not have a commit checked out

Not a submodule, not corruption — just a nested repo with no commits. Added to
`.gitignore`. Disable via Preferences → Common → Project Backup if unwanted.

## Never write into the KiCad project directory while it is open

Standing shop rule, and it bit on 2026-08-26: with `~*.kicad_sch.lck` present,
the on-disk file is *not* what the editor has in memory. A stray dangling wire
visible on disk was absent from the GUI's own ERC. Read-only inspection is fine;
writes are not. Put notes in `docs/`, outside the project directory.

## InvenTree: `total_stock` is a property, not a database field

`Part.objects.filter(...).order_by("-total_stock")` throws `FieldError`. It is
computed from stock items, so it cannot be sorted or filtered in the ORM. Sort in
Python:

```python
sorted(qs, key=lambda p: -float(p.total_stock))
```

## PlatformIO vanished (2026-08-25)

`~/.platformio/penv` and `.pio/build` are both gone; only `~/.platformio/python3`
remains, which is the newer Core layout — so this reads as a reinstall rather
than a deletion. The `~/.zshrc` PATH line added on 08-09 now points at nothing.
**Reinstall Core and re-point that line before the next flash.** See `status.md`.

## A continuous rating is not a transient rating

The ZVS board carries **six FR307 in parallel directly across the DC+ / DC− rails**.
Reading only the 3 A continuous rating, these were dismissed as "far too small to
be the freewheel diode on a 58 A bus" — twice, in two different sessions of
reasoning. Scott traced the copper and found them landing on the two big rail
pads, which is exactly where a catch diode belongs.

**The dismissal compared the wrong two numbers.** A catch diode across a DC bus
never carries the running current. It conducts only while the choke's field
collapses:

| | |
|---|---|
| Continuous, 6 parallel | 18 A ← the number that produced the wrong answer |
| **Surge, 6 parallel** | **900 A for 8.3 ms** ← the number that governs |
| Actual demand | 58 A for a few ms, 9.7 A per device |
| Average dissipation | ~1.4 mW |

**Rule: before calling a part undersized, work out what duty it actually sees.**
Continuous, repetitive-peak and non-repetitive-surge ratings differ by one to two
orders of magnitude on the same device, and a protection component almost always
lives on the surge line.

Two corollaries worth carrying:

- **Six small fast diodes can beat one large slow one.** What this job needs is
  surge capacity and fast recovery (~500 ns), not steady current. A 60 A
  rectifier would be slower and worse at catching the edge.
- **The 1000 V rating was the clue, not the current rating.** An unclamped
  inductive kick goes far above the rail; that is what the part is chosen for.

### And a second reading error underneath it

Before the copper was traced, the same six diodes were confidently explained as
**gate-steering diodes** on the strength of the classic Mazzilli topology. That
was reasoning from a remembered schematic instead of from the board on the bench.
The board is the authority. See also the pad-numbering trap above — three of the
four hardware mistakes in this project came from reading a photograph or a
recollection where a meter or a caliper was available.

## The tank caps are marked 50 kHz, not 150

Four documents asserted "150 kHz" for the tank capacitors, unsourced. The part is
marked **`0.33µF J630X (1200V) 50KHz`** — and 12 × 0.33 = **3.96 µF**, matching the
tank capacitance already measured, so the reading is sound.

This matters because the coil sets frequency against a fixed capacitance. The
20 mm quickstart coil is predicted at **~108 kHz** (86–130 kHz realistic band) —
at or above twice the marking. Reaching 50 kHz needs ~2.56 µH, a much larger coil
than the coupling argument wants. The two pull against each other.

**Not yet a blocker.** On MKP parts that figure is often the frequency at which
the ripple-current rating is quoted rather than an absolute ceiling. But it is no
longer safe to repeat "well under the cap rating" — **measure the real operating
frequency during VR-05**, which is the same scope trace already required for Vds.

## Replacing a slice between two markers deletes what is between them

Twice in one session, a doc or source edit was written as *"replace everything
from marker A to marker B"* — and both times it silently destroyed content that
had been added between them.

- In `testrig.cpp`, replacing from `// ---- status LED` to `// ---- runs` deleted
  `busVolts()`. Caught immediately by the compiler.
- In `docs/status.md`, replacing from `### Controller` to `## When the tube
  arrives` deleted **216 lines** — an entire day of findings. Caught only because
  a later edit failed to find its anchor.

**The compiler catches this in code. Nothing catches it in prose.** A markdown
file has no build step, so a slice-delete looks exactly like a successful edit
and the script prints "ok".

**Rule: replace the specific string you mean to change, never a range between two
landmarks.** If a range really is needed, assert on the length of what is being
removed first.

Recovery is straightforward if it is committed — `git show <commit>~1:path` gets
the file back — which is the argument for committing at every milestone rather
than at the end of a session.

## TR-04's 200 ms is an electrical floor, not the time to read

**2026-08-30.** §8.3 gates thermocouple sampling to ≥200 ms after field-off, and
it is easy — I did it — to read that as "take the reading at 200 ms." It is not.
There are two independent clocks and they differ by two orders of magnitude:

- **200 ms** — when the reading stops being *corrupted*. Induced voltage has
  decayed, the junction has stopped self-heating inductively. Electrical.
- **~10–15 s** — when the reading *peaks*. Thermal, and it depends on how deep
  the junction sits. For a bead ~12.7 mm down a blind hole in steel,
  t ≈ x²/α = (0.0127)² / 1.2e-5 ≈ 13 s of conduction lag.

Heat is still arriving at the bead long after the field stops. **Sample from
200 ms onward and take the maximum**; a single reading at 200 ms is low by a
wide margin, and reads exactly like a coil that underperforms.

## An oversized TC hole insulates the junction it was drilled for

A junction loose in a hole larger than the bead is surrounded by air, which
conducts ~1/50th as well as steel. It reads low *and* lags — the curve keeps
its shape and loses its amplitude, which is the failure mode that survives
review. Pack the hole (copper anti-seize) or wedge the bead against the wall.

Both errors push the same direction as the withdrawn PZEM data: they make the
machine look weaker than it is, which invites longer cycles to compensate.

## A press that reports state is a press that changes it

**2026-08-30.** The duration button stepped the setting on every press and
flashed the new value back. Scott, reasonably, read that flash as a *check* --
pressed to see the setting, lost count, pressed again to confirm, and
incremented it both times. He asked how long to wait before a press stops
counting as an adjustment. There is no such interval: there was no read-only
press to wait for.

I had bracketed the read-back in darkness an hour earlier, which made the count
legible and left the real defect untouched -- the second fix in a row aimed at
the symptom. **Legibility was never the problem. Requiring a press at all was.**

Idle now displays the duration continuously (N flashes, 1.6 s gap, repeat) and
the button only ever changes it. Instrument-panel principle: the setting is
always visible, so nothing has to be interrogated. Ask of any control that
reports state -- why is the operator asking rather than looking?

## R-11 confirmed on the current channel too -- by energy, not by comparison

**2026-08-30.** PZEM read **3.5 A** peak while the clamp read **23 A**. The
comparison alone does not settle it, because the two may not have been on the
same conductor -- a resonant tank legitimately circulates Q times its DC input,
so 3.5 A in and 23 A round the tank is a physically ordinary pair.

**The energy argument settles it without needing to know.** At 53 V, 3.5 A for
30 s is 5.6 kJ. Raising even a 100 g slug of steel by 300 K needs ~14 kJ, and
the bar under test was several times that. The reading is impossible, wherever
the clamp sat.

Both PZEM channels are therefore corrupt, and the failure is not calibration --
it is the field. **The INA228 planned to replace it is the same kind of
instrument in the same place and must be assumed to fail the same way until
proven otherwise, in the field, against a clamp.**

### What this costs: CR-01 has no instrument

Energy dosing terminates on the integral of V*I*dt. Both factors are currently
unmeasurable in the field. That promotes the R48's **CAN telemetry from a
cross-check (CR-11) to the primary power instrument** -- it reports output
voltage and current digitally from outside the field, which is the one place
the corruption cannot reach. CAN was already the only way to command bus
voltage; it is now also the only trustworthy way to observe power. The parts
are in stock and allocated.

Until CAN is built, cycle termination stays on **time**, and every wattage in
these docs stays withdrawn.

## Two instruments, two faults — do not let one story cover both

**2026-08-30, later.** A DC clamp on the feed read **33 A** where the PZEM read
3.3 A, and the round 10× invites a single tidy explanation. It does not fit.

| Channel | Error | Signature |
|---|---|---|
| Current | 3.3 A vs 33 A = **10.0×**, round | constant ratio → smells like a **scale constant** |
| Voltage | 29.7 V vs 53 V = **1.78×**, not round | **moved to ~33 V when the sense leads were twisted** |

A scaling error is indifferent to how wires are dressed. The voltage channel
responded to lead dressing, so it is RF. The current channel may be nothing more
than the shunt's full-scale being 10× the value the PZEM is configured for --
and `status.md` has recorded that shunt as *"mV rating unmarked"* since the
bench inventory, i.e. this was a known unknown that nobody closed.

**Discriminating test:** run at two clearly different power levels. Ratio pinned
at 0.1 → scale constant, repairable in firmware. Ratio wanders → RF, unrecoverable.

If it is the constant, **CR-01 energy dosing gets its instrument back** — which
was written off as impossible earlier the same day. Do not leave that conclusion
standing on the assumption that both faults share a cause.

### The wider error

I declared the DC clamp unnecessary in `bom.md`, then read a single successful
mains reading as *vindication* of that call, and wrote it into `status.md` as
settled. Hours later the declined instrument produced the decisive number. **One
instrument failing to RF is not evidence that all in-field instruments fail** --
the PZEM's weakness is its sense leads, and a clamp has none. Purchasing
decisions justified by a risk that has not yet been observed should be recorded
as predictions, not as findings.

## "Turns touching" is the spec, not the fault

**2026-08-30.** I described the coil as *shorted* three times across a session,
and twice framed the 320 °C result as achieved *despite* that handicap. Scott
asked what I meant, which is when it came apart: `quickstart-1-8.md` § says
**"Wind them touching and insulate — bare touching turns are shorted turns."**
Touching is how turns are packed into a short axial length. It is the design.
Only bare touching turns short, and the coil carries the fibreglass sleeving
`design-references.md` A5 specifies for exactly this reason.

**The error had a consequence beyond vocabulary.** A shorted coil implies stored
headroom — un-short it and the power arrives. The real deficit is geometric: a
25 mm bar in a 60 mm coil couples to (25/60)^2 ≈ 0.17 of the bore. Both point at
"rewind the coil", so the wrong diagnosis survived pointing at the right task —
but only the geometric one tells you what bore to wind to, which is the entire
actionable content.

A fault carried in a parenthetical from an earlier session, never re-derived,
and then used to explain a result it had nothing to do with.

## Peak power quoted as if it were cycle-average

**2026-08-30.** Scott asked where "1,800 W" came from. Tracing it exposed two
things worth keeping.

**The number is well supported.** 33 A (DC clamp on the feed) x 53 V (second
meter at the PSU under load) = **1,750 W**, no assumptions in the product. The
mains path corroborates from the other end: 23 A displayed / 3 wraps = 7.67 A,
x 240 V x PF 0.99 = 1,840 W at the wall, less ~5% PSU loss = 1,750 W DC. Two
different physical principles at opposite ends of the machine agreeing.

**But 240 V was never measured** — it came from an old status.md line, and was
never put to Scott. The cross-path agreement is what supports it: at 120 V the
AC path gives 920 W and disagrees with the DC path by 1.9x. Fine as evidence,
but it is inference, not a reading of the outlet.

**And both figures are peaks, which I then used as cycle averages.** 1.8 kW x
30 s = 54 kJ assumes the machine held peak draw for the entire cycle. It does
not: resistivity rises with temperature and coupling shifts as the work heats,
so mean draw is below peak. Every energy figure derived from that 54 kJ --
the temper-band reconciliation, the Haas 80 kJ comparison -- is loose in the
same direction.

**Quote peak as peak.** An energy claim needs a mean, and a clamp shows an
instantaneous value; the conversion between them is a measurement nobody has
made here. It is the same class of error as the withdrawn PZEM wattages: a
number that is individually defensible, used for something it does not support.

## An operating point is not a capability — and the difference is a burnt holder

**2026-08-30.** Scott challenged "1,800 W" a second time, on better grounds than
the first. The measurement is sound (33 A steady at 240 V AC confirmed), but a
Royer draws what the load permits: tank impedance sets the current, coupling
sets the tank impedance. **1,750 W is what this coil and this bar demand, not
what the machine can deliver.** I measured an operating point and used it to
characterise the machine.

**Why it matters more than bookkeeping.** Draw is 33 A of the R48's 56.1 A, and
both the kit claim and the PSU label sit near 3,000 W — about **1.7x headroom,
and improving coupling is exactly what claims it.** A 30 s cycle that produced a
300 K rise at 1.75 kW moves toward a ~500 K rise at 3 kW. Losses rise with
temperature so it will not scale that cleanly, but the direction is certain and
400 °C is not far away.

### Operating rule

**Cycle time does not survive a coil change.** The 19.5 mm coil is a large
coupling improvement over the 60 mm, so its first runs start at **5–10 s with a
temper witness and step up** — never at the 30 s that has been safe so far.
Carrying the old time across is how the first test of a better coil destroys a
holder.

### The same argument is the argument for CR-01

If coupling sets the draw, **time is a bad proxy for delivered energy** — a
fixed timer under-doses a poorly coupled load and over-doses a well coupled one,
with nothing on the panel to say which. Energy dosing on the integral of V*I*dt
compensates for coupling by construction. Scott reached this from a clamp
reading rather than from the requirement; it is the strongest argument for CAN
being load-bearing rather than a convenience, and it belongs next to CR-01.

## CR-02 assumes a measurement that CR-01 exists because we cannot make

**2026-08-30, Scott.** Asked how a thermocouple is supposed to attach to a tool
holder, and noted that avoiding temperature measurement is the whole point of
energy dosing. Both halves land.

**CR-02 specifies "sample nose temperature 200 ms after field-off" and never
specifies how the sensor reaches the nose.** A holder is loaded and unloaded
every cycle: it cannot be drilled, cannot be clamped, and a spring-loaded probe
against a 400 °C nose has contact repeatability worse than the quantity it is
measuring. The requirement was written as though the sensing problem were solved.

**The deeper contradiction:** CR-01 energy dosing exists *because* per-cycle
holder temperature is not measurable. CR-02 assumes it is. Both cannot be true,
and CR-02 is the one without a mechanism.

**Haas settles which way it goes.** The HSF-325 -- a production machine -- does
not measure temperature at all. Its entire control scheme is diameter -> seconds,
open loop, with 400 °C published as operator guidance rather than as a feedback
signal. The industry answer is: control the input, do not measure the output.
Dosing is that idea improved, since energy self-compensates for the coupling and
voltage variation a fixed timer cannot see.

### Where the thermocouple actually belongs

- **CR-07 coolant overtemp** (§528) — permanently mounted in a coolant path,
  nothing to attach per cycle. Buildable as written. One of the BOM's two probes
  has this home.
- **Calibration** — establishing the dose→temperature curve once per holder size
  on a witness piece that *can* be drilled. After that the curve is the
  instrument and the probe comes off.

**CR-02 needs rewriting** to move temperature from per-cycle verification to
calibration-time characterisation. Scott's call; flagged, not changed. Note the
thresholds need revisiting anyway now that Haas gives 400 °C against our 350/375.

### The general shape

A requirement can be internally consistent, survive review, and still assume a
capability that a *different* requirement in the same document exists to work
around. Neither one looks wrong alone. What exposed it was someone asking how
the part gets physically attached — a question no amount of re-reading the spec
would have raised.

## The "long analog path" story was invented, against a fact Scott had given

**2026-08-30.** I justified the INA228 by claiming the PZEM failed because of a
long analog path — shunt, leads, then the measuring box — and that an INA228
close-coupled to the shunt would be "a fundamentally different architecture."

**Scott had already told me the opposite**, during the RF diagnosis: *"there's no
leads that go from the PZEM to the shunt, it's all contained in one box."* I
built a causal story that contradicted an observation he had personally made,
and used it to justify a purchase.

### What is actually supported

| Channel | Leads? | Evidence | Reading |
|---|---|---|---|
| Voltage | **Long sense leads to the PSU** | reading moved when Scott twisted them | RF pickup, demonstrated |
| Current | **None — shunt integrated with the board** | Scott, at the bench | cannot be lead-borne RF; consistent with a scale constant |

The two-fault split stands up **better** on Scott's facts than on my theory. But
the conclusion I drew from the theory does not survive it.

### The consequence for the INA228

**The bus cable is there regardless.** The shunt sits in the conductor carrying
the tank's return current; mounting an INA228 beside it changes nothing about
that exposure. An INA228 on that shunt **is the same architecture as the PZEM's
current channel**, not an improvement on it.

So the open question is not "will the INA228 do better" but "**does a
close-coupled shunt in this bus work at all**" — and the PZEM's own current
channel is already that experiment, half-run.

- **Ratio pinned at 0.1 across two power levels** → the shunt reads true and is
  merely scaled wrong. Shunts work here; the INA228 is sound, and the PZEM may
  be repairable with a constant.
- **Ratio wanders** → shunts do not survive this bus, and $17 buys a reproduction
  of a known failure.

**The ratio test now gates the purchase.** It costs a bench session and no parts.
Do not order until it has run.

## The witness-bar hole cannot be at mid-length — the wire has nowhere to go

**2026-08-31, Scott:** *"if I drill this hole, I'm gonna have to bend the wire of
the K-type probe at a ninety degree angle to get it up through the inside of the
coil. Is that kosher?"*

`quickstart-1-8.md` specifies **"cross-drill 1.5 mm × 8 mm deep at mid-length"**
and that instruction was written without checking whether the thermocouple can
physically leave the hole. It cannot:

- **Clearance.** A 19.5 mm coil bore around a 15.875 mm bar leaves **1.8 mm per
  side.** The glass-braid probe is ~2–2.5 mm over the braid. It does not fit, and
  forcing it crushes the braid and presses the conductors together — which is the
  false-junction failure recorded above.
- **A wire routed up the bore sits in the strongest part of the field**, the one
  place this whole build works to keep sense leads out of.

Bending the wire itself is harmless — chromel/alumel is ductile and a gentle bend
does not affect the junction. The bend was never the problem; the destination was.

### Fix

**Site the cross-hole in the section of bar that protrudes past the coil**, so the
wire exits radially into free air. **Mark the insertion depth and use the same
mark every run.**

This trades "hottest station" for "same station every time", which is the correct
trade: **comparing two coils needs identical geometry, not maximum temperature.**
A cooler station held constant tells you which coil is better; a hotter station
that moves between runs tells you nothing. Same conclusion as the thermocouple
packing discussion — chase repeatability, not absolute accuracy.

### Pattern

Fourth time in two days that asking *what does this part physically touch* has
changed a plan, and the third time it invalidated something already written down.
The instruction was internally sensible and geometrically impossible, and no
amount of re-reading it would have surfaced that. See [[ask-where-it-attaches]].

## espota needs the LAPTOP on the device's VLAN — second time

**2026-08-31.** The INA228 bring-up board came up clean on IoT
(`192.168.30.54`, OTA server confirmed running on `shrinkfit-ina228.local:3232`)
and `pio run -e ina228_ota -t upload` still failed with **"No response from
device"**. The laptop was on `192.168.1.203`.

**espota runs backwards from the intuition.** The host sends an *invitation*, and
then **the device connects BACK to the host**. Across VLANs with no reverse
route, that return connection never lands — while the board pings fine, answers
mDNS, and reports its OTA server up. **Everything looks healthy on the device
side because the device side IS healthy.**

**Fix: put the laptop on the same network as the board.** Nothing on the board
needs changing.

This is the same fault diagnosed during the test rig's OTA bring-up
(2026-08-27). It has now cost time twice, on two different boards, and it will
recur every time a new board joins IoT while the laptop sits on the house LAN.

**Check first, before debugging anything else:**

```
ifconfig | grep "inet " | grep -v 127.0.0.1     # laptop
```

If that subnet does not match the address the board printed at boot, stop —
there is no fault to find.

## An ESP32-S3 cannot use GPIO 26–32, and GPIO 32 is our CAN RX

The classic ESP32 exposes GPIO 32 as an ordinary pin; on the **S3 that range is
the SPI flash / PSRAM bus**. Our CAN RX sits on GPIO 32, so firmware that runs on
a NodeMCU-32S will build for an S3, flash, boot — and never receive a frame,
because the pin it is listening on is wired to the flash chip.

Nothing warns you. TWAI routes through the GPIO matrix, so the driver installs
happily and reports `RUNNING`; you get `tx_err` climbing with `rx_err` at zero,
which reads exactly like *the other node is absent*. That is the same signature
the rectifier gave us, which is precisely the confusion this test exists to
resolve — so getting it from our own wiring would have been expensive.

`env:can_loop_s3` therefore moves the bus to **GPIO 4/5** and the role jumper to
**GPIO 6**. Rule: **any pin assignment carried from a classic ESP32 to an S3 has
to be re-checked against 26–32 before it is trusted**, and this applies to the
whole controller pinmap, not just CAN.

## Two CAN failures that look alike and are not: jammed vs unacknowledged

Measured on the bench 2026-09-04 with both boards in front of us. `twai_transmit`
returning queue-full has two completely different causes, and the counters tell
them apart:

| Counters | Meaning |
|---|---|
| `tx_err=0 rx_err=0 bus_err=0`, queue fills | **No transceiver wired.** GPIO32 floats, the controller never sees bus-idle, so it *waits* rather than erroring. Nothing is wrong with the bus because there is no bus. |
| `tx_err=128`, `bus_err` climbing fast | **Transceiver wired, nothing acknowledging.** Frames go out and are never ACKed. |

The first was the bare new board; the second is what the dev board shows against
the R48. Both present as "it won't transmit", and reading either as the other
sends you to the wrong end of the problem — which is the whole failure mode this
two-node test exists to escape. Both firmwares now print the distinction rather
than the raw counters.

Corollary: **a frame that never leaves is not a lost frame.** Both firmwares
originally retired the pending sequence number on every attempt, so one stuck
frame reprinted `seq=5 NO ECHO` once a second and read as five losses. The
pending frame is now retired only when a new one actually goes out.

## pyserial asserts DTR and RTS on open — which is EN and GPIO0

Opening an ESP32's port with default pyserial settings drives the auto-reset
transistors and holds the chip in reset or download mode. The symptom is a
screenful of repeating binary garbage that looks exactly like a baud mismatch or
a boot loop, and it sent us to `esptool flash_id` to check the crystal — the chip
was fine the whole time.

Set `dtr = False` and `rts = False` **both before and after `open()`**, and do not
pulse RTS unless a reset is actually wanted. `pio device monitor` already does
this; hand-rolled readers do not.

## The SN65HVD230 modules ARE terminated — we assumed they were not

The blue breakout carries **`R2` = `121` = 120 Ω directly across CANH/CANL**, and
`R1` = `103` = 10 kΩ as the fixed RS slope resistor — which is why the board has
no `RS` pin and sits in slope-control mode rather than standby. Two modules on a
bench link therefore give 120 Ω at each end already: a correct 60 Ω bus, with
nothing to add.

**We believed the opposite for a full evening.** An ambiguous `01C` read off one
resistor became "the modules have no onboard termination", and that conclusion
was written into `can_loop.cpp`, `status.md` and a commit message before anyone
looked at the part with the labels facing up. Nothing measured it; a marking
report was promoted to a fact and then cited as one.

**Consequence for the R48 work:** the 220 ∥ 270 = 121 Ω pair built for that test
was a *second* terminator at our end — about 60 Ω from our side alone, and ~40 Ω
if the rectifier terminates too. That is below spec and loads the drivers. It
does not explain `tx_err=128` on its own (an absent peer gives that anyway), but
it means the R48 attempt had a confound in it that we did not know about, and
any retry has to remove the pair first.

**Rule: read the resistor markings on the actual module before specifying
termination.** A 3-digit `121` is 120 Ω and is the giveaway; position across the
CANH/CANL pins confirms it without needing the code at all.

## A wire has two ends; verifying one proves nothing

Cost most of an evening on 2026-09-04. The bench CAN test put the second ESP32
into a hard boot loop the moment the two boards were "grounded" together. The
ground wire's far end was in the dev board's **positive rail**, not its ground.

Every diagnostic pointed the wrong way because each was individually sound:

- **DC volts between the two grounds read 0 V.** True and useless — both grounds
  *were* at the same potential. The wire was not on one of them.
- **Continuity from the landing point to a known ground beeped.** Also true: that
  was the *near* end, which was fine the whole time. The far end was never tested.
- Disconnecting fixed it and reconnecting broke it again, which "confirmed" a
  ground-bonding theory (floating bench supply, Y-cap leakage) that was wrong.

**The tell was in the errors and we read past it.** `csum err` on every boot with
a *different* computed value each time — `0x64`, `0xbc`, `0x1b`, `0xe7` against a
constant expected `0x6b`. Corrupt flash gives the *same* wrong answer every time.
A different one each read means the flash bus is being electrically disturbed,
which points at injected current, not at a data problem.

**Procedure that catches it, before the wire goes in:** land one end, leave the
other free, and **measure DC volts between the free end and the hole it is about
to enter**. Ground to ground reads 0 V; a rail reads the supply. One measurement,
taken at the two points that actually matter, and it costs five seconds.

The corollary is the rule: **never verify a wire at the end you can reach.**

## On a NodeMCU-32S, GND is three holes from the SPI flash bus

Counting from the USB end on the `3V3`/`5V` edge: `5V`(1), `CMD`(2), **`SD3`(3)**,
**`SD2`(4)**, `P13`(5), `GND`(6). Positions 2–4 are the ESP32's SPI flash bus.

A wire aimed at that `GND` and landed two holes early sits on `SD2`, and the
board stops booting with `flash read err, 988` or `csum err` — diagnosed
2026-09-04 after the dev board went from healthy to unbootable purely from
wiring changes, and came back the instant every wire was pulled.

**Use the `GND` between `P19` and `P21` on the opposite edge instead.** Both its
neighbours are ordinary GPIOs, so an off-by-one costs nothing, and it is nowhere
near flash. Identify it by the labels either side of it, never by counting holes
from the end of the board — the counting is what fails, in a dim shop, on a black
board with white silk.

Related: `P12` is MTDI and sets flash voltage at boot. Grounded is the safe
state; pulled high it stops the board booting. Never tie it to 3V3.

## Suspect what is still CONNECTED before the wiring (it was not the breadboard)

2026-09-04. An evening of CAN bring-up produced: floating voltages between pins
that should have been shorted, `flash read err, 988`, repeated boot loops that
appeared and vanished with wire changes, and readings that moved when nothing
electrical had. Every one of those was diagnosed as a wiring error, and each
diagnosis was individually plausible.

**Scott lifted the ESP32 off the breadboard and the anomalous readings went
away.** I concluded the breadboard was leaking. Wrong: he then found the INA228
module was still connected to 3V3 and GND with only its signal wires pulled, and
removing *it* is what cleared the phantom voltage. The breadboard was fine.

The mechanism is the INA228's ESD diodes from `VIN+`/`VIN-` to its supply rails.
With those inputs still on the shunt, the module conducts from the measured
circuit into the logic 3V3 rail and ground through diodes never intended to carry
it. A part that is "powered but not wired up" is still a connection.

Why it evaded us for hours: an intermittent bridge is *indistinguishable* from a
mis-landed wire, and it moves when you touch things, so every "fix" appeared to
work or fail at random. It also perfectly mimics the flash-pin fault, because
bridging onto an `SD*`/`CLK`/`CMD` row IS a flash-pin fault — just one nobody
wired.

**Rule: when a board misbehaves in ways that change as wires are moved, remove
things one at a time and re-measure, before diagnosing the wiring.** Removal
falsifies a whole subsystem in seconds and belongs before pin-counting, not
after. Pulling the signal wires off a module does NOT remove it from the circuit
— its power pins and its input ESD diodes are both still connections.

Second-order lesson, aimed at me: I proposed lifting the board, got the right
result, and attached it to the wrong cause — then wrote that cause down as fact
within the minute. A correct experiment does not license a confident conclusion
about *which* removed thing mattered when several were removed at once.

## RETRACTED — "one dead transceiver clamps the bus" (I misread the measurement)

**This entry is wrong and is kept only as a record of how.** I read Scott's
"three point three volts and zero volts at CANL and CANH" as *CANH 3.3 V, CANL
0 V* — a driven dominant proving a good part. He was answering two questions at
once: **3.3 V was the module's supply, and BOTH bus lines were at 0 V.** A fresh
never-used module measured identically to the suspect one, so nothing was
demonstrated to be dead. I announced a confirmed diagnosis, wrote it up and
committed it, all on a misparsed sentence.

The lesson is procedural: **when a reply carries fewer numbers than the question
asked for, the mapping is an assumption.** Ask which pin each number belongs to
before building on it. Everything below was written under the wrong reading.

---

2026-09-04. The dev board's SN65HVD230 was believed to have a failed `CANH`
driver: isolated, powered at 3.3 V, with its input grounded to command a
dominant, it reads **0 V on both `CANH` and `CANL`** — the low side pulls `CANL`
down and `CANH` is dragged to meet it through the 120 Ω termination. A healthy
module in the same state reads 1.4 V on both (recessive), and driven reads
**3.3 V / 0 V**.

**A device holding `CANH` at ground clamps the bus for everyone on it.** With
three modules bussed together, every measurement of every module returned the
same ~0.89 V no matter what was done to it. That single dead part produced:

- identical readings whatever was grounded, `CTX` or `CRX`
- **a brand-new replacement module measuring exactly as dead** — it was wired
  onto the same clamped bus
- neither board ever hearing the other, with both apparently healthy
- five successive theories (RS standby, low supply, dead drivers, reversed silk,
  damaged parts) each tested against a bus that one device was pinning, and each
  producing a plausible, wrong answer

**The one durable finding here, independent of the retraction: on a shared bus,
isolate the device under test before diagnosing it.**
A bus is a wired-OR: any node can dominate it, so a measurement taken with
several attached is a measurement of the worst one, not of the one you are
probing. Disconnect down to a single node plus its termination, confirm its
supply *at its own pins during the test*, then command it.

The tell in hindsight: **0 V on both lines is neither recessive (~VCC/2) nor
dominant (~3.0/0.3), and does not correspond to any state a working transceiver
has.** A reading matching no valid state is a broken part, and it was visible
long before it was believed.

## A supply that reads 3.3 V and cannot power anything

2026-09-04. The whole CAN bring-up failed on this and it took most of a day,
because the fault presents as a dead component and survives replacing it.

The dev board's transceiver measured **3.3 V between the module's own `3V3` and
`GND` pins** and shared a **0 Ω ground** with the MCU. Its `D` input was driven
low through a wire measured at 0 Ω, with the MCU pin read back as `0` in
firmware and confirmed at 0 V *against the module's own ground*. Genuine part,
marked `VP230`. Correct 120 Ω termination, no shorts. Every precondition the
datasheet requires.

It produced **0 V on both `CANH` and `CANL` in BOTH commanded states.**

That is the diagnostic tell: a powered transceiver biases the bus to roughly
VCC/2 when recessive (~1.4 V measured on a good one) and splits it when
dominant. **Producing neither is not a broken driver — it is a chip that is not
running.** A reading that matches no valid state of the part means it has no
working supply, whatever the voltmeter says.

**A voltmeter draws about a microamp.** A contact resistance of tens or hundreds
of ohms is invisible to it and reads a flawless 3.3 V, then collapses under the
~10 mA the chip needs to bias and the ~70 mA to drive. The measurement that would
have caught it is a **load** test, not a voltage one.

Cost: a brand-new never-used module tested identically dead, which sent us
chasing damaged parts. Three modules were swapped through the same faulty
wiring, all reading dead, all fine.

**Rules:**
- **Voltage present is not power available.** Confirm a supply under its real
  load, or by the powered device behaving, never by an unloaded reading.
- **A reading that matches no valid state of the part indicts the supply or the
  measurement, not the function.** Do not diagnose a driver until the device is
  demonstrably running.
- For bring-up, run supply pins as **direct jumpers from board pin to device
  pin**. Every breadboard contact in a supply path is a candidate for this.

## FOUND — the transceiver supply folds under load (the day's root cause)

Measured 2026-09-04 by watching the module's `3V3` pin **while** the driver was
commanded dominant: it **dropped to 0 V, recovered, dropped again**, cycling for
the whole 25 s hold. That is a relaxation oscillation through series resistance
in the supply path — the chip loads it, it collapses, the chip browns out and
stops drawing, it recovers, the chip restarts, repeat.

Every symptom of the day was this one fault at different load levels:

| Wiring | Recessive (10 mA) | Dominant (~30 mA) |
|---|---|---|
| original breadboard path | **collapsed → 0 V both lines** ("dead chip") | collapsed |
| direct `3V3`/`GND` jumpers | **held → 1.5 V bias, chip alive** | **collapsed → 0 V both lines** |

The rewire improved the path by roughly one contact's worth — enough for the
bias current, not enough for the driver. So "the chip is dead", "the driver is
dead", "three modules are dead", and "the wiring is fine because it reads 3.3 V"
were all the same thing seen from different sides.

**Why it hid for a day:** a voltmeter draws ~1 µA. Tens or hundreds of ohms in
the path is invisible to it and reads a flawless 3.3 V at the module's own pins —
which I took as proof of power at least four times. It is not `EN`: a load on
`EN` holds the board in reset, and the board answered commands throughout.

**The measurement that found it:** meter on the supply pin *during* the load, not
before it. Should have been the second measurement of the day, not the last.

**Fix:** replace the `3V3` (and `GND`) run with a fresh wire, and put **10–100 µF
across the module's `3V3`/`GND` at its pins** to carry the dominant transient.
Verify by the driver producing 3.0/1.0 V, not by the supply reading 3.3 V.

**Rule:** for anything with a driver — transceiver, motor, LED string, radio —
**read the supply pin while the part is driving.** Unloaded voltage is a
statement about the meter, not the supply.

## The blue "VP230" modules do not drive the bus — measured at the chip leads

2026-09-04, end of a full day. Three SN65HVD230 breakout modules (one never in
service), one at a time on a bench supply that held 3.3 V steady under load,
every input measured **directly at the SOIC lead**, not at the module pin:

| Lead | Pin | Idle | Driven |
|---|---|---|---|
| `D` | 1 | 3.3 V | 26 mV (DC hold) / **1.65 V avg (125 kHz square wave)** |
| `GND` | 2 | 0 V | 0 V |
| `VCC` | 3 | 3.3 V | 3.3 V, steady |
| `R` | 4 | 2.2 V | 0.2 V — receiver reports dominant |
| `RS` | 8 | 0.6 V | — (slope control; standby needs > 2.5 V) |
| `CANH` / `CANL` | 7 / 6 | 1.5 V / 1.5 V | **1.5 V / 1.5 V — no change** |
| supply current | | 13 mA | 13 mA (square wave), 9 mA (DC hold) |

The chip is alive, biases the bus, sees `D`, and its receiver follows `D`. **The
output stage never engages.** Datasheet dominant is `CANH` ≥ 2.45 V, `CANL`
0.5–1.25 V, ~30 mA. Not once, on any module, at any pulse width.

**Two invalid tests were removed on the way, and both are worth keeping:**

1. **The DC hold is not a CAN test.** Holding `D` low for 40 s is nothing like an
   8 µs dominant bit, and a transceiver with a dominant-state timeout would fail
   it while being perfectly good. The 125 kHz square wave is the right stimulus
   and a meter reads its average cleanly: a working driver shows `CANH` ~2.6 V /
   `CANL` ~1.6 V; this shows 1.5/1.5.
2. **The ESP32 TWAI no-ACK self-test does not reach the transceiver.** Both boards
   passed it while their transceivers demonstrably could not drive. It is
   satisfied inside the controller. Never cite it as hardware verification.

**Two anomalies that point at the chip, not the wiring:**
- `R` idles at **2.2 V** with only a GPIO input attached; datasheet minimum is 2.4 V
  *under an 8 mA load*.
- Bus-pin input resistance measures **14 kΩ** to ground; datasheet minimum is 20 kΩ.

Both are consistent with a die that is not what the `VP230` marking claims — these
breakouts are a known counterfeit target — and with a **5 V transceiver die**
whose receiver limps along at 3.3 V while its driver lacks the headroom to work.

**Untested, deliberately:** the modules at 5 V. Abs-max `VCC` is 6 V, so a genuine
part survives it; a counterfeit 5 V die would start driving. It has to be done
with `CRX` disconnected from the ESP32 first, because `R` would swing to 5 V and
the ESP32 is not 5 V tolerant.

**Rule from the day, restated once:** test the part in isolation, at its own
leads, with a stimulus shaped like the real signal, and read the supply *during*
the load. Every hour lost today was one of those four skipped.
