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
