"""Generate the shrink-fit controller schematic (KiCad 10, v20260306).

Every net attaches at a pin ENDPOINT via a local label or a wire vertex —
no mid-segment magic. All coordinates are on the 1.27 mm grid.
Pin numbers on U1 are header positions; every net here is placed by the
GPIO NAME's known position in the verified symbol, and the netlist is
checked against an expected (net, pad) table afterwards.
"""
import hashlib, io, re

def uid(seed):
    h = hashlib.md5(seed.encode()).hexdigest()
    return f"{h[0:8]}-{h[8:12]}-{h[12:16]}-{h[16:20]}-{h[20:32]}"

# ---------------- symbol library bodies (lib coords, y UP) ----------------

def prop(name, value, y, hide=False, justify=None):
    h = "\n\t\t\t\t(hide yes)" if hide else ""
    j = f"\n\t\t\t\t(justify {justify})" if justify else ""
    return (f'\t\t(property "{name}" "{value}"\n'
            f'\t\t\t(at 0 {y} 0)\n'
            f'\t\t\t(show_name no)\n'
            f'\t\t\t(do_not_autoplace no)\n'
            f'\t\t\t(effects\n\t\t\t\t(font\n\t\t\t\t\t(size 1.27 1.27)\n\t\t\t\t){h}{j}\n\t\t\t)\n'
            f'\t\t)\n')

def lpin(ptype, x, y, ang, length, name, num):
    return (f'\t\t\t(pin {ptype} line\n'
            f'\t\t\t\t(at {x} {y} {ang})\n'
            f'\t\t\t\t(length {length})\n'
            f'\t\t\t\t(name "{name}"\n\t\t\t\t\t(effects\n\t\t\t\t\t\t(font\n\t\t\t\t\t\t\t(size 1.27 1.27)\n\t\t\t\t\t\t)\n\t\t\t\t\t)\n\t\t\t\t)\n'
            f'\t\t\t\t(number "{num}"\n\t\t\t\t\t(effects\n\t\t\t\t\t\t(font\n\t\t\t\t\t\t\t(size 1.27 1.27)\n\t\t\t\t\t\t)\n\t\t\t\t\t)\n\t\t\t\t)\n'
            f'\t\t\t)\n')

def rect(x1, y1, x2, y2, fill="background"):
    return (f'\t\t\t(rectangle\n\t\t\t\t(start {x1} {y1})\n\t\t\t\t(end {x2} {y2})\n'
            f'\t\t\t\t(stroke\n\t\t\t\t\t(width 0.254)\n\t\t\t\t\t(type default)\n\t\t\t\t)\n'
            f'\t\t\t\t(fill\n\t\t\t\t\t(type {fill})\n\t\t\t\t)\n\t\t\t)\n')

def poly(pts, width=0.254, fill="none"):
    p = " ".join(f"(xy {x} {y})" for x, y in pts)
    return (f'\t\t\t(polyline\n\t\t\t\t(pts\n\t\t\t\t\t{p}\n\t\t\t\t)\n'
            f'\t\t\t\t(stroke\n\t\t\t\t\t(width {width})\n\t\t\t\t\t(type default)\n\t\t\t\t)\n'
            f'\t\t\t\t(fill\n\t\t\t\t\t(type {fill})\n\t\t\t\t)\n\t\t\t)\n')

def sym(name, props, graphics, pins, extra=""):
    return (f'\t(symbol "{name}"\n'
            f'\t\t(exclude_from_sim no)\n\t\t(in_bom yes)\n\t\t(on_board yes)\n'
            f'\t\t(in_pos_files yes)\n\t\t(duplicate_pin_numbers_are_jumpers no)\n'
            f'{extra}{props}'
            f'\t\t(symbol "{name}_0_1"\n{graphics}\t\t)\n'
            f'\t\t(symbol "{name}_1_1"\n{pins}\t\t)\n'
            f'\t\t(embedded_fonts no)\n\t)\n')

LIB = {}

LIB["R"] = sym("R",
    prop("Reference","R",7.62)+prop("Value","R",-7.62)+prop("Footprint","",-10.16,hide=True)+prop("Datasheet","",-12.7,hide=True)
    +prop("Description","Resistor, THT axial",-15.24,hide=True),
    rect(-1.016,2.54,1.016,-2.54,"none"),
    lpin("passive",0,5.08,270,2.54,"~","1")+lpin("passive",0,-5.08,90,2.54,"~","2"))

LIB["C"] = sym("C",
    prop("Reference","C",5.08)+prop("Value","C",-5.08)+prop("Footprint","",-7.62,hide=True)+prop("Datasheet","",-10.16,hide=True)
    +prop("Description","Capacitor",-12.7,hide=True),
    poly([(-1.524,0.508),(1.524,0.508)],0.4064)+poly([(-1.524,-0.508),(1.524,-0.508)],0.4064),
    lpin("passive",0,2.54,270,2.032,"~","1")+lpin("passive",0,-2.54,90,2.032,"~","2"))

LIB["Q_2N3904"] = sym("Q_2N3904",
    prop("Reference","Q",7.62)+prop("Value","2N3904",-7.62)+prop("Footprint","",-10.16,hide=True)+prop("Datasheet","",-12.7,hide=True)
    +prop("Description","NPN BJT, TO-92 pinout 1=E 2=B 3=C (American EBC, flat face toward you)",-15.24,hide=True),
    poly([(-2.54,1.905),(-2.54,-1.905)],0.508)
    +poly([(-2.54,0.762),(2.54,2.54)])
    +poly([(-2.54,-0.762),(2.54,-2.54)])
    +poly([(2.54,-2.54),(0.889,-2.032),(1.778,-1.143),(2.54,-2.54)],0.254,"outline"),
    lpin("passive",2.54,-5.08,90,2.54,"E","1")+lpin("passive",-5.08,0,0,2.54,"B","2")+lpin("passive",2.54,5.08,270,2.54,"C","3"))

for n in (2,4,5):
    pins = ""
    for i in range(1,n+1):
        pins += lpin("passive",5.08,-(i-1)*2.54,180,2.54,"~",str(i))
    LIB[f"CONN_{n:02d}"] = sym(f"CONN_{n:02d}",
        prop("Reference","J",2.54)+prop("Value",f"CONN_{n:02d}",-(n-1)*2.54-2.54)+prop("Footprint","",-(n-1)*2.54-5.08,hide=True)
        +prop("Datasheet","",-(n-1)*2.54-7.62,hide=True)+prop("Description","Generic header / screw terminal",-(n-1)*2.54-10.16,hide=True),
        rect(-5.08,1.27,2.54,-(n-1)*2.54-1.27),
        pins)

LIB["PWR_FLAG"] = sym("PWR_FLAG",
    prop("Reference","#FLG",3.81,hide=True)+prop("Value","PWR_FLAG",2.54)+prop("Footprint","",-2.54,hide=True)
    +prop("Datasheet","",-5.08,hide=True)+prop("Description","ERC power-source marker",-7.62,hide=True),
    poly([(0,0),(0,1.016),(-0.762,1.524),(0,2.032),(0.762,1.524),(0,1.016)]),
    lpin("power_out",0,0,90,0,"~","1"))

# --- pull the verified SuperMini symbol out of the project lib, unchanged ---
libfile = io.open("shrink-fit-controller.kicad_sym", encoding="utf-8").read()
m = re.search(r'(\t\(symbol "ESP32-S3-SuperMini-HW747".*?\n\t\)\n)', libfile, re.S)
assert m, "SuperMini symbol not found in project lib"
SUPERMINI = m.group(1)

# append the new symbols to the project lib (skip ones already there)
add = "".join(body for name, body in LIB.items() if f'(symbol "{name}"' not in libfile)
libfile = libfile.rstrip()[:-1].rstrip() + "\n" + add + ")\n"
io.open("shrink-fit-controller.kicad_sym","w",encoding="utf-8").write(libfile)

# embedded copies get the lib-nickname prefix on the TOP-LEVEL name only.
# Exact-name replacement: a regex alternation mangled the one-letter names
# ("R", "C") by eating the closing quote — measured on the first ERC run.
def prefixed(name, body):
    tag = f'(symbol "{name}"'
    assert body.lstrip("\t").startswith(tag), f"{name}: unexpected body head"
    return body.replace(tag, f'(symbol "shrink-fit-controller:{name}"', 1)
EMBED = "".join(prefixed(n, b) for n, b in LIB.items())
EMBED += prefixed("ESP32-S3-SuperMini-HW747", SUPERMINI)

# ---------------- schematic (sheet coords, y DOWN) ----------------

ROOT = "8f2a1c40-0000-4000-8000-5f7269746501"
out = []

def label(net, x, y, ang):
    just = {0:"left bottom",180:"right bottom",90:"left bottom",270:"right bottom"}[ang]
    out.append(f'\t(label "{net}"\n\t\t(at {x} {y} {ang})\n'
               f'\t\t(effects\n\t\t\t(font\n\t\t\t\t(size 1.27 1.27)\n\t\t\t)\n\t\t\t(justify {just})\n\t\t)\n'
               f'\t\t(uuid "{uid("lbl"+net+str(x)+str(y))}")\n\t)\n')

def wire(x1,y1,x2,y2):
    out.append(f'\t(wire\n\t\t(pts\n\t\t\t(xy {x1} {y1}) (xy {x2} {y2})\n\t\t)\n'
               f'\t\t(stroke\n\t\t\t(width 0)\n\t\t\t(type default)\n\t\t)\n'
               f'\t\t(uuid "{uid(f"w{x1},{y1},{x2},{y2}")}")\n\t)\n')

def junction(x,y):
    out.append(f'\t(junction\n\t\t(at {x} {y})\n\t\t(diameter 0)\n\t\t(color 0 0 0 0)\n'
               f'\t\t(uuid "{uid(f"j{x},{y}")}")\n\t)\n')

def nc(x,y):
    out.append(f'\t(no_connect\n\t\t(at {x} {y})\n\t\t(uuid "{uid(f"nc{x},{y}")}")\n\t)\n')

def text(s, x, y):
    s = s.replace('"','\\"')
    out.append(f'\t(text "{s}"\n\t\t(exclude_from_sim no)\n\t\t(at {x} {y} 0)\n'
               f'\t\t(effects\n\t\t\t(font\n\t\t\t\t(size 1.27 1.27)\n\t\t\t)\n\t\t\t(justify left bottom)\n\t\t)\n'
               f'\t\t(uuid "{uid("t"+s[:40]+str(x))}")\n\t)\n')

def inst_prop(name, value, x, y, hide=False, justify="left"):
    h = "\n\t\t\t(hide yes)" if hide else ""
    return (f'\t\t(property "{name}" "{value}"\n\t\t\t(at {x} {y} 0)\n{h and h or ""}'
            f'\t\t\t(show_name no)\n\t\t\t(do_not_autoplace no)\n'
            f'\t\t\t(effects\n\t\t\t\t(font\n\t\t\t\t\t(size 1.27 1.27)\n\t\t\t\t)\n\t\t\t\t(justify {justify})\n\t\t\t)\n\t\t)\n')

def instance(libname, ref, value, x, y, rot, npins, rx, ry, vx, vy):
    pins = "".join(f'\t\t(pin "{i}"\n\t\t\t(uuid "{uid(f"{ref}p{i}")}")\n\t\t)\n' for i in range(1,npins+1))
    out.append(
        f'\t(symbol\n\t\t(lib_id "shrink-fit-controller:{libname}")\n'
        f'\t\t(at {x} {y} {rot})\n\t\t(unit 1)\n\t\t(body_style 1)\n'
        f'\t\t(exclude_from_sim no)\n\t\t(in_bom yes)\n\t\t(on_board yes)\n'
        f'\t\t(in_pos_files yes)\n\t\t(dnp no)\n\t\t(fields_autoplaced no)\n'
        f'\t\t(uuid "{uid("sym"+ref)}")\n'
        + inst_prop("Reference", ref, rx, ry)
        + inst_prop("Value", value, vx, vy)
        + inst_prop("Footprint", "", x, y, hide=True)
        + inst_prop("Datasheet", "", x, y, hide=True)
        + pins +
        f'\t\t(instances\n\t\t\t(project "shrink-fit-controller"\n'
        f'\t\t\t\t(path "/{ROOT}"\n\t\t\t\t\t(reference "{ref}")\n\t\t\t\t\t(unit 1)\n\t\t\t\t)\n\t\t\t)\n\t\t)\n\t)\n')

# ---- left column: connectors. Pin i connects at (x+5.08, y+(i-1)*2.54) ----
def conn(ref, value, n, y0, nets):
    instance(f"CONN_{n:02d}", ref, value, 35.56, y0, 0, n, 27.94, y0-3.81, 27.94, y0+(n-1)*2.54+3.81)
    for i, net in enumerate(nets, start=1):
        px, py = 40.64, y0+(i-1)*2.54
        if net:
            label(net, px, py, 0)

conn("J1","12V IN (kit wall wart)",2, 38.1, ["+12V","GND"])
conn("PS1","LM2596 module 12V->5V",4, 50.8, ["+12V","GND","+5V","GND"])
conn("J2","SSR-100DD input",2, 68.58, ["+12V","SSR_K"])
conn("J3","Bus sense DC+/DC-",2, 78.74, ["BUS_V+","GND"])
conn("J4","Flow switch",2, 88.9, ["+3V3","FLOW_SW"])
conn("J5","E-stop sense",2, 99.06, ["ESTOP_SENSE","GND"])
conn("J6","Start button",2, 109.22, ["START_BTN","GND"])
conn("J7","CAN TTL (Phase 2)",4, 121.92, ["+3V3","GND","CAN_TX","CAN_RX"])
conn("J8","MAX31855 (Phase 2)",5, 139.7, ["+3V3","GND","TC_SCK","TC_CS","TC_MISO"])
conn("J9","INA228 I2C (Phase 2)",4, 157.48, ["+3V3","GND","SDA","SCL"])

# ---- PWR_FLAGs on stubs so ERC sees a source on externally-fed rails ----
for i,(y,net) in enumerate([(38.1,"+12V"),(40.64,"GND"),(55.88,"+5V"),(78.74,"BUS_V+")]):
    # the label already sits AT the pin; hang the flag on a stub at the same point
    instance("PWR_FLAG", f"#FLG{i+1}", "PWR_FLAG", 40.64, y, 0, 1, 43.18, y-2.54, 43.18, y-1.27)

# ---- U1 ----
instance("ESP32-S3-SuperMini-HW747","U1","ESP32-S3 Super Mini HW-747",101.6,104.14,0,18,95.25,87.63,95.25,120.65)
LEFT = [("TX",None),("RX",None),("GPIO1","BUS_SENSE"),("GPIO2","START_BTN"),("GPIO3",None),
        ("GPIO4","CAN_TX"),("GPIO5","CAN_RX"),("GPIO6","FLOW_SW"),("GPIO7","ESTOP_SENSE")]
for i,(nm,net) in enumerate(LEFT):
    x,y = 88.9, 93.98 + i*2.54
    if net: label(net,x,y,180)
    else:   nc(x,y)
RIGHT = [("GPIO8","SDA"),("GPIO9","SCL"),("GPIO10","SSR_DRV"),("GPIO11","TC_CS"),
         ("GPIO12","TC_SCK"),("GPIO13","TC_MISO"),("3V3","+3V3"),("GND","GND"),("5V","+5V")]
for i,(nm,net) in enumerate(RIGHT):
    x,y = 114.3, 114.3 - i*2.54
    label(net,x,y,0)

# ---- divider ----
label("BUS_V+",170.18,66.04,90)
wire(170.18,66.04,170.18,68.58)
instance("R","R2","220k",170.18,73.66,0,2,172.72,72.39,172.72,74.93)
wire(170.18,78.74,170.18,81.28); junction(170.18,80.01)
instance("R","R3","10k",170.18,86.36,0,2,172.72,85.09,172.72,87.63)
wire(170.18,80.01,177.8,80.01); junction(177.8,80.01)
wire(177.8,80.01,182.88,80.01); label("BUS_SENSE",182.88,80.01,0)
instance("C","C1","100nF",177.8,86.36,0,2,180.34,85.09,180.34,87.63)
wire(177.8,80.01,177.8,83.82)
wire(177.8,88.9,177.8,91.44); wire(177.8,91.44,170.18,91.44); junction(170.18,91.44)
wire(170.18,91.44,170.18,93.98); label("GND",170.18,93.98,270)

# ---- SSR driver ----
label("SSR_DRV",193.04,81.28,180)
wire(193.04,81.28,195.58,81.28)
instance("R","R1","1k",200.66,81.28,90,2,198.12,77.47,198.12,74.93)
wire(205.74,81.28,208.28,81.28)
instance("Q_2N3904","Q1","2N3904",213.36,81.28,0,3,217.17,80.01,217.17,82.55)
wire(215.9,76.2,215.9,73.66); label("SSR_K",215.9,73.66,90)
wire(215.9,86.36,215.9,88.9); label("GND",215.9,88.9,270)

# ---- notes ----
text("WIRE BY PIN NAME, NEVER BY NUMBER. U1 numbers are header positions: GPIO1 = pin 3, GPIO10 (SSR drive) = pin 12. Verify pads in the .kicad_pcb after layout.",160.02,35.56)
text("GND IS DC-. The controller floats with the bus (SR-11 bond withdrawn). No USB while energised; no earthed scope ground clip - differential probe only.",160.02,40.64)
text("Divider 220k/10k = 23:1 (180k not stocked). Calibrate with testrig 'cal <V>'. C1 is not optional: sensing beside a kW oscillator (R-11).",160.02,45.72)
text("SSR-100DD input 3-32V, fed from +12V; Q1 low-side. GPIO10 delivers the voltage STEP (CR-05) - CAN sets bus volts, never enables output.",160.02,50.8)
text("Phase 2 headers J7/J8/J9: footprints reserved, unpopulated. CAN stack (ADUM1201 + B0505S + SN65HVD230) is off-board - isolation required. INA228 shunt value is a PLACEHOLDER until VR-11 four-wire cal.",160.02,55.88)
text("GPIO3 reserved: Stage-4 encoder.",83.82,92.71)
text("J4 flow: switch to 3V3, firmware INPUT_PULLDOWN, active high = flow proven. J5/J6: contacts to GND, INPUT_PULLUP, active low.",160.02,60.96)

# ---------------- write ----------------
sch = (f'(kicad_sch\n\t(version 20260306)\n\t(generator "eeschema")\n'
       f'\t(generator_version "10.0")\n\t(uuid "{ROOT}")\n\t(paper "A3")\n'
       f'\t(title_block\n\t\t(title "Shrink-Fit Induction Controller")\n'
       f'\t\t(date "2026-08-26")\n\t\t(rev "A")\n'
       f'\t\t(comment 1 "U1 wired by pin NAME (GPIO) - numbers are header positions. Pinout verified from 3 sources incl. both faces of the physical board.")\n'
       f'\t\t(comment 2 "GND = DC-. Floating with the bus by design. Phase 2 blocks are headers, unpopulated.")\n'
       f'\t)\n\t(lib_symbols\n{EMBED}\t)\n'
       + "".join(out) +
       f'\t(sheet_instances\n\t\t(path "/"\n\t\t\t(page "1")\n\t\t)\n\t)\n'
       f'\t(embedded_fonts no)\n)\n')
io.open("shrink-fit-controller.kicad_sch","w",encoding="utf-8").write(sch)
print(f"schematic written: {len(sch)} bytes, {len(out)} top-level items")
