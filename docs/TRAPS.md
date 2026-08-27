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
