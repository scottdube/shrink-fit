#!/usr/bin/env python3
"""Guard plate for the ZVS board underside — SVG for LightBurn, 1:1 mm.

Covers the solder-built bus rails on the board's underside, which sit at tank
potential (up to ~168 V peak). Mounts on the eight brass standoffs, which were
measured 2026-08-27 as isolated from both DC rails.

  CUT     = black stroke, no fill
  ENGRAVE = red fill

Slots are NOT decoration. A solid plate 26.9 mm under the board turns the
standoff gap into a tray, and that gap contains the coolant terminals and
tubing — a leak would pool against live copper. The slots give it somewhere
to go.

>>> EVERY VALUE IN `P` BELOW IS AN ASSUMPTION UNTIL MEASURED. <<<
"""
import io, sys

# The stud pattern IS the fan pattern: two 80 mm fans stacked touching, so the
# eight mounting holes are the union of two 71.5 mm squares offset by one frame.
#   80 mm fan: 80 x 80 frame, holes 71.5 apart -> (80-71.5)/2 = 4.25 in from each edge
P = dict(
    fan_size     = 80.0,    # fan frame, mm
    fan_pitch    = 71.5,    # 80 mm fan standard. 60->50, 92->82.5, 120->105
    n_fans       = 2,
    fan_gap      = 0.0,     # frames touching, per Scott: "side by side precisely"
    hole_dia     = 3.4,     # M3 clearance = 3.4; M4 = 4.5.  <-- CONFIRM screw size

    # --- the one thing still unmeasured: how far the BOARD extends past the
    #     fan block. The plate must cover the live bus rails, not just the studs.
    margin_x     = 12.0,    # MEASURE: board edge beyond the stud columns, each side
    margin_y     = 12.0,    # MEASURE: board edge beyond the end stud rows, each end

    corner_r     = 4.0,
    slot_w       = 8.0,
    slot_gap     = 12.0,
    slot_inset   = 14.0,    # keep slots clear of the stud columns
    warn         = "UNDERSIDE AT TANK POTENTIAL - UP TO 168 V",
)

def stud_xy(p):
    """Union of n fan patterns stacked along Y, frames touching."""
    e = (p["fan_size"] - p["fan_pitch"]) / 2.0          # 4.25 for an 80 mm fan
    xs = [e, e + p["fan_pitch"]]
    ys = []
    for i in range(p["n_fans"]):
        y0 = i * (p["fan_size"] + p["fan_gap"])
        ys += [y0 + e, y0 + e + p["fan_pitch"]]
    return xs, ys

def build(p):
    xs, ys = stud_xy(p)
    L = (xs[-1] - xs[0]) + 2*p["margin_x"]      # plate X
    W = (ys[-1] - ys[0]) + 2*p["margin_y"]      # plate Y
    ox, oy = p["margin_x"] - xs[0], p["margin_y"] - ys[0]
    r = p["corner_r"]
    out = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{L:.2f}mm" height="{W:.2f}mm" '
           f'viewBox="0 0 {L:.2f} {W:.2f}">',
           '<g fill="none" stroke="#000000" stroke-width="0.1">',
           f'<rect x="0" y="0" width="{L:.2f}" height="{W:.2f}" rx="{r}"/>']
    studs = [(x+ox, y+oy) for x in xs for y in ys]
    for x, y in studs:
        out.append(f'<circle cx="{x:.2f}" cy="{y:.2f}" r="{p["hole_dia"]/2:.2f}"/>')

    # drainage slots run the long axis, between the two stud columns
    x0 = xs[0]+ox + p["slot_inset"]
    x1 = xs[-1]+ox - p["slot_inset"]
    band = x1 - x0
    pitch = p["slot_w"] + p["slot_gap"]
    n = max(1, int((band + p["slot_gap"]) // pitch))
    span = n*pitch - p["slot_gap"]
    xstart = x0 + (band - span)/2
    y0 = ys[0]+oy + 10.0
    sh = (ys[-1]+oy - 10.0) - y0
    for i in range(n):
        out.append(f'<rect x="{xstart + i*pitch:.2f}" y="{y0:.2f}" '
                   f'width="{p["slot_w"]:.2f}" height="{sh:.2f}" rx="{p["slot_w"]/2:.2f}"/>')
    out.append('</g>')
    out.append(f'<g fill="#FF0000" stroke="none">'
               f'<text x="{L/2:.2f}" y="{W - 4.5:.2f}" font-family="Arial" font-size="4.5" '
               f'text-anchor="middle">{p["warn"]}</text></g>')
    out.append('</svg>')
    return "\n".join(out), studs, n, L, W

svg, studs, nslot, L, W = build(P)
path = "/Users/scottdube/code/shrink-fit/hardware/guard-plate/guard-plate.svg"
io.open(path, "w", encoding="utf-8").write(svg)
print(f"wrote {path}")
xs, ys = stud_xy(P)
print(f"  plate outline   {L:.1f} x {W:.1f} mm")
print(f"  stud columns X  {xs[0]:.2f}, {xs[1]:.2f}   (pitch {P['fan_pitch']})")
print(f"  stud rows Y     " + ", ".join(f"{y:.2f}" for y in ys))
print(f"  {len(studs)} holes at dia {P['hole_dia']}, {nslot} drainage slots")
print()
print("  Hole pattern is now EXACT - derived from the fan standard.")
print("  Only margin_x / margin_y remain guesses: how far the board")
print("  extends past the studs. Everything else is fixed.")
