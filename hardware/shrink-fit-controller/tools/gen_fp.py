"""Project-local 2.54 mm headers with 1.78 mm pads.

KiCad's stock PinHeader footprints use 1.7 mm pads on a 1.0 mm drill. On this
shop's bit set that drill snaps UP to 1.181 mm, leaving a 0.259 mm annulus
against pcbmill's 0.25 mm gate -- passing by 9.5 microns, which is not a margin.

1.78 mm pads give 0.299 mm and match the proven NodeMCU-32S_38pin_THT footprint
already in this project. Same drill, same pitch, 80 microns more copper.
"""
import io, os

PAD, DRILL, PITCH = 1.78, 1.0, 2.54
OUT = "shrink-fit-controller.pretty"

def header(n):
    name = f"PinHeader_1x{n:02d}_P2.54mm_Vertical_1.78pad"
    h = (n - 1) * PITCH
    body = 1.27                     # silk box half-width
    s = [f'(footprint "{name}"',
         '\t(version 20240108)', '\t(generator "gen_fp.py")',
         '\t(layer "F.Cu")', '\t(attr through_hole)',
         f'\t(property "Reference" "J" (at 0 {-body-1.2:.2f} 0) (layer "F.SilkS")'
         ' (effects (font (size 1 1) (thickness 0.15))))',
         f'\t(property "Value" "{name}" (at 0 {h+body+1.2:.2f} 0) (layer "F.Fab")'
         ' (effects (font (size 1 1) (thickness 0.15))))']
    # silk outline, with pin 1 corner squared off
    s.append(f'\t(fp_rect (start {-body:.2f} {-body:.2f}) (end {body:.2f} {h+body:.2f})'
             ' (stroke (width 0.12) (type solid)) (fill none) (layer "F.SilkS"))')
    s.append(f'\t(fp_line (start {-body-0.5:.2f} {-body:.2f}) (end {-body-0.5:.2f} {body:.2f})'
             ' (stroke (width 0.3) (type solid)) (layer "F.SilkS"))')   # pin-1 tick
    for i in range(n):
        shape = "rect" if i == 0 else "circle"     # square pad = pin 1, always
        s.append(f'\t(pad "{i+1}" thru_hole {shape} (at 0 {i*PITCH:.2f})'
                 f' (size {PAD} {PAD}) (drill {DRILL}) (layers "*.Cu" "*.Mask"))')
    s.append(')')
    return name, "\n".join(s) + "\n"

os.makedirs(OUT, exist_ok=True)
made = []
for n in (2, 3, 4, 5, 6):
    name, txt = header(n)
    io.open(f"{OUT}/{name}.kicad_mod", "w", encoding="utf-8").write(txt)
    made.append(name)
print("wrote:", *made, sep="\n  ")
print(f"\npad {PAD} on drill {DRILL} -> snaps to 1.181 -> annulus {(PAD-1.181)/2:.3f} mm (gate 0.25)")
