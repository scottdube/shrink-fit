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

for n in (2,3,4,5,6):
    pins = ""
    for i in range(1,n+1):
        pins += lpin("passive",5.08,-(i-1)*2.54,180,2.54,"~",str(i))
    LIB[f"CONN_{n:02d}"] = sym(f"CONN_{n:02d}",
        prop("Reference","J",2.54)+prop("Value",f"CONN_{n:02d}",-(n-1)*2.54-2.54)+prop("Footprint","",-(n-1)*2.54-5.08,hide=True)
        +prop("Datasheet","",-(n-1)*2.54-7.62,hide=True)+prop("Description","Generic header / screw terminal",-(n-1)*2.54-10.16,hide=True),
        rect(-5.08,1.27,2.54,-(n-1)*2.54-1.27),
        pins)


LIB["RotaryEncoder_EC11"] = sym("RotaryEncoder_EC11",
    prop("Reference","SW",8.89)+prop("Value","EC11 + switch",-11.43)
    +prop("Footprint","",-13.97,hide=True)+prop("Datasheet","",-16.51,hide=True)
    +prop("Description","EC11 rotary encoder with push switch",-19.05,hide=True),
    rect(-6.35,6.35,6.35,-8.89),
    lpin("passive",-10.16, 3.81,0,3.81,"A","A")
   +lpin("passive",-10.16, 0.00,0,3.81,"C","C")
   +lpin("passive",-10.16,-3.81,0,3.81,"B","B")
   +lpin("passive", 10.16, 2.54,180,3.81,"S1","S1")
   +lpin("passive", 10.16,-2.54,180,3.81,"S2","S2")
   +lpin("passive", 0.00,-12.70,90,3.81,"MP","MP"),
    extra='\t\t(duplicate_pin_numbers_are_jumpers yes)\n')


# PCF8574 in DIP-16. Pin numbers are the real package pins so the stock
# Package_DIP footprint maps 1:1. Address pins A0-A2 all to GND = 0x20; the
# INA228 sits at 0x40, no collision.
_L = [("A0","1"),("A1","2"),("A2","3"),("P0","4"),("P1","5"),("P2","6"),("P3","7"),("VSS","8")]
_R = [("VDD","16"),("SDA","15"),("SCL","14"),("INT","13"),("P7","12"),("P6","11"),("P5","10"),("P4","9")]
_pins = ""
for i,(nm,num) in enumerate(_L):
    _pins += lpin("input" if nm.startswith("A") else ("power_in" if nm=="VSS" else "bidirectional"),
                  -12.7, 8.89-i*2.54, 0, 2.54, nm, num)
for i,(nm,num) in enumerate(_R):
    t = "power_in" if nm=="VDD" else ("open_collector" if nm=="INT" else "bidirectional")
    _pins += lpin(t, 12.7, 8.89-i*2.54, 180, 2.54, nm, num)
LIB["PCF8574"] = sym("PCF8574",
    prop("Reference","U",12.7)+prop("Value","PCF8574 (0x20)",-12.7)
    +prop("Footprint","",-15.24,hide=True)+prop("Datasheet","",-17.78,hide=True)
    +prop("Description","I2C 8-bit I/O expander, quasi-bidirectional",-20.32,hide=True),
    rect(-10.16,10.16,10.16,-10.16), _pins)

# 2N7002 in SOT-23: 1=G 2=S 3=D. Same footprint geometry as the 2N3904 symbol
# body, different pins, because the PCF8574 sources only ~100 uA when HIGH --
# far too little for a 2N3904 base, but a MOSFET gate is voltage-driven and
# does not care.
LIB["Q_2N7002"] = sym("Q_2N7002",
    prop("Reference","Q",5.08)+prop("Value","2N7002",-5.08)+prop("Footprint","",-7.62,hide=True)
    +prop("Datasheet","",-10.16,hide=True)+prop("Description","N-MOSFET, logic level, SOT-23",-12.7,hide=True),
    rect(-2.54,3.81,2.54,-3.81,"none")
    +poly([(-2.54,0),(-1.27,0)])+poly([(-1.27,2.54),(-1.27,-2.54)],0.508)
    +poly([(0,2.54),(2.54,2.54),(2.54,5.08)])+poly([(0,-2.54),(2.54,-2.54),(2.54,-5.08)]),
    lpin("input",-5.08,0,0,2.54,"G","1")+lpin("passive",2.54,-5.08,90,2.54,"S","2")+lpin("passive",2.54,5.08,270,2.54,"D","3"))

LIB["PWR_FLAG"] = sym("PWR_FLAG",
    prop("Reference","#FLG",3.81,hide=True)+prop("Value","PWR_FLAG",2.54)+prop("Footprint","",-2.54,hide=True)
    +prop("Datasheet","",-5.08,hide=True)+prop("Description","ERC power-source marker",-7.62,hide=True),
    poly([(0,0),(0,1.016),(-0.762,1.524),(0,2.032),(0.762,1.524),(0,1.016)]),
    lpin("power_out",0,0,90,0,"~","1"))

# --- pull the verified SuperMini symbol out of the project lib, unchanged ---
libfile = io.open("shrink-fit-controller.kicad_sym", encoding="utf-8").read()
# The NodeMCU-32S symbol and its NodeMCU-32S_38pin_THT footprint are the proven
# pair from sln-geo-aux-heat -- a board that was built and works. Copied in
# unchanged rather than redrawn; a redrawn footprint is how this shop produced
# two mirrored ones.
_i = libfile.index('(symbol "NodeMCU-32S"')
_d = 0
for _j in range(_i, len(libfile)):
    if libfile[_j] == '(': _d += 1
    elif libfile[_j] == ')':
        _d -= 1
        if _d == 0: break
MCU = "\t" + libfile[_i:_j+1] + "\n"

_i = libfile.index('(symbol "LM2596_Module"')
_d = 0
for _j in range(_i, len(libfile)):
    if libfile[_j] == '(': _d += 1
    elif libfile[_j] == ')':
        _d -= 1
        if _d == 0: break
BUCK = "\t" + libfile[_i:_j+1] + "\n"

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
EMBED += prefixed("NodeMCU-32S", MCU)
EMBED += prefixed("LM2596_Module", BUCK)

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

def instance(libname, ref, value, x, y, rot, npins, rx, ry, vx, vy, fp=""):
    pins = "".join(f'\t\t(pin "{i}"\n\t\t\t(uuid "{uid(f"{ref}p{i}")}")\n\t\t)\n' for i in range(1,npins+1))
    out.append(
        f'\t(symbol\n\t\t(lib_id "shrink-fit-controller:{libname}")\n'
        f'\t\t(at {x} {y} {rot})\n\t\t(unit 1)\n\t\t(body_style 1)\n'
        f'\t\t(exclude_from_sim no)\n\t\t(in_bom yes)\n\t\t(on_board yes)\n'
        f'\t\t(in_pos_files yes)\n\t\t(dnp no)\n\t\t(fields_autoplaced no)\n'
        f'\t\t(uuid "{uid("sym"+ref)}")\n'
        + inst_prop("Reference", ref, rx, ry)
        + inst_prop("Value", value, vx, vy)
        + inst_prop("Footprint", fp, x, y, hide=True)
        + inst_prop("Datasheet", "", x, y, hide=True)
        + pins +
        f'\t\t(instances\n\t\t\t(project "shrink-fit-controller"\n'
        f'\t\t\t\t(path "/{ROOT}"\n\t\t\t\t\t(reference "{ref}")\n\t\t\t\t\t(unit 1)\n\t\t\t\t)\n\t\t\t)\n\t\t)\n\t)\n')

# ---- left column: connectors. Pin i connects at (x+5.08, y+(i-1)*2.54) ----
HDR = "shrink-fit-controller:PinHeader_1x{:02d}_P2.54mm_Vertical_1.78pad"

# The LM2596 modules mount ON the board: the proven LM2596_Module_THT footprint
# exists (2.0 mm pads on a 1.0 mm drill = 0.410 mm annulus, comfortable) and it
# deletes eight flying leads. Pin order is IN+, IN-, OUT+, OUT- -- confirmed from
# the source symbol, because a swapped in/out on a buck destroys the module.
def buck(ref, value, x, y, outnet):
    # Pin geometry read from the symbol, not assumed: IN+ (-12.7, +5.08),
    # IN- (-12.7, -5.08), OUT+ (12.7, +5.08), OUT- (12.7, -5.08). Guessing this
    # from the connector layout is what produced 14 ERC violations first time.
    instance("LM2596_Module", ref, value, x, y, 0, 4, x-12.7, y-11.43,
             x-12.7, y+13.97, fp="shrink-fit-controller:LM2596_Module_THT")
    label("+24V",  x-12.7, y-5.08, 180)
    label("GND",   x-12.7, y+5.08, 180)
    label(outnet,  x+12.7, y-5.08, 0)
    label("GND",   x+12.7, y+5.08, 0)

def conn(ref, value, n, y0, nets):
    instance(f"CONN_{n:02d}", ref, value, 35.56, y0, 0, n, 27.94, y0-3.81,
             27.94, y0+(n-1)*2.54+3.81, fp=HDR.format(n))
    for i, net in enumerate(nets, start=1):
        if net:
            label(net, 40.64, y0+(i-1)*2.54, 0)

conn("J1", "24V IN (S-250-24)",           2,  25.4,  ["+24V","GND"])
buck("PS1","LM2596 24V->5V  logic",  71.12, 33.02, "+5V")
buck("PS2","LM2596 24V->12V motors", 71.12, 58.42, "+12V")
conn("J2", "SSR-100DD input",             2,  73.66, ["+12V","SSR_K"])
conn("J3", "Contactor drop relay",        2,  83.82, ["+12V","RELAY_K"])
conn("J4", "Flow sensor (Hall, 0.3-3L)",  3,  93.98, ["+5V","GND","FLOW_PULSE"])
conn("J5", "E-stop sense",                2, 106.68, ["ESTOP_SENSE","GND"])
conn("J6", "CYCLE START",                 2, 116.84, ["CYCLE_START","GND"])
conn("J7", "CYCLE STOP",                  2, 127.0,  ["CYCLE_STOP","GND"])
# EC11 on the board, wired the way the part actually works: C is the common and
# goes to GND, A and B are the quadrature contacts, S1/S2 are the push switch.
# There is no +3V3 pin on an encoder -- the pull-ups are the MCU's internal ones
# (GPIO 13/19/15 all support them; 34-39 would not have).
SW1X, SW1Y = 66.04, 140.97
instance("RotaryEncoder_EC11","SW1","EC11 + switch",SW1X,SW1Y,0,6,SW1X-8,SW1Y-12,SW1X-8,SW1Y+14,
         fp="Rotary_Encoder:RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm")
label("ENC_A", SW1X-10.16, SW1Y-3.81, 180)
label("GND",   SW1X-10.16, SW1Y,      180)
label("ENC_B", SW1X-10.16, SW1Y+3.81, 180)
label("ENC_SW",SW1X+10.16, SW1Y-2.54, 0)
label("GND",   SW1X+10.16, SW1Y+2.54, 0)
label("GND",   SW1X,       SW1Y+12.70, 270)
conn("J9", "CAN isolated stack",          4, 154.94, ["+3V3","GND","CAN_TX","CAN_RX"])
conn("J10","INA228 I2C",                  4, 172.72, ["+3V3","GND","SDA","SCL"])
conn("J11","DS18B20 1-Wire x2",           3, 190.5,  ["+3V3","GND","OW_DATA"])
conn("J12","Round TFT (SPI)",             6, 203.2,  ["+3V3","GND","TFT_CS","TFT_DC","TFT_SCK","TFT_MOSI"])
conn("J13","LAMPS 12V  G / A / R",         4, 226.06, ["+12V","LAMP_G","LAMP_A","LAMP_R"])

# ---- U2 : PCF8574 + three 2N7002 lamp drivers ------------------------------
# Lamp logic lives entirely in the ESP32; the PCF8574 has no firmware. Firmware
# should rewrite the lamp byte EVERY loop, not only on change: outputs hold
# their last state if the MCU hangs, and a refreshed byte turns a hang into a
# lamp that stops responding rather than one that lies.
# Boot note: PCF8574 powers up with all outputs HIGH, so all three lamps light
# for the ~100 ms until firmware writes the first byte. That is a free bulb
# test, not a fault.
U2X, U2Y = 205.74, 165.1
instance("PCF8574","U2","PCF8574 (0x20)",U2X,U2Y,0,16,U2X-11,U2Y-13,U2X-11,U2Y+14,
         fp="Package_DIP:DIP-16_W7.62mm")
# left side: A0 A1 A2 -> GND (address 0x20); P0/P1/P2 -> gates; P3 nc; VSS -> GND
for i,net in enumerate(["GND","GND","GND","LAMP_G_DRV","LAMP_A_DRV","LAMP_R_DRV",None,"GND"]):
    x,y = U2X-12.7, U2Y-(8.89-i*2.54)
    if net: label(net,x,y,180)
    else:   nc(x,y)
# right side: VDD SDA SCL INT P7 P6 P5 P4
for i,net in enumerate(["+3V3","SDA","SCL",None,None,None,None,None]):
    x,y = U2X+12.7, U2Y-(8.89-i*2.54)
    if net: label(net,x,y,0)
    else:   nc(x,y)
instance("C","C2","100nF",U2X+17.78,U2Y-6.35,0,2,U2X+20.32,U2Y-7.62,U2X+20.32,U2Y-5.08,
         fp="Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P5.00mm")
label("+3V3",U2X+17.78,U2Y-8.89,90); label("GND",U2X+17.78,U2Y-3.81,270)

# three identical low-side drivers, gate straight from the expander pin
for k,(q,dn) in enumerate([("Q2","G"),("Q3","A"),("Q4","R")]):
    qx,qy = 246.38, 152.4 + k*17.78
    instance("Q_2N7002",q,"2N7002",qx,qy,0,3,qx+3.81,qy-1.27,qx+3.81,qy+1.27,
             fp="Package_TO_SOT_SMD:SOT-23")
    label(f"LAMP_{dn}_DRV", qx-5.08, qy, 180)          # gate
    label("GND",            qx+2.54, qy+5.08, 270)      # source
    label(f"LAMP_{dn}",     qx+2.54, qy-5.08, 90)       # drain -> lamp cathode
text("Lamps: anode to +12V (J13.1), cathode to the 2N7002 drain. Output LOW at the expander = lamp OFF; HIGH = ON. 2N7002 is SOT-23 SMD - on a single-sided milled board it goes on B.Cu, flipped.",170.18,93.98)

# ---- PWR_FLAGs so ERC sees a source on rails fed from off-board ----
for i,(y,net) in enumerate([(25.4,"+24V"),(27.94,"GND")]):
    instance("PWR_FLAG", f"#FLG{i+1}", "PWR_FLAG", 40.64, y, 0, 1, 43.18, y-2.54, 43.18, y-1.27)
# No PWR_FLAG on +5V or +12V: the LM2596 symbol declares OUT+ as a power OUTPUT,
# so a flag there is a second source on the same net. ERC flagged it; the module
# IS the source. Flags stay only on +24V and GND, which arrive from off-board.

# ---- U1 : NodeMCU-32S -------------------------------------------------------
# Nets are placed by PIN NUMBER from the symbol's own geometry, then rendered
# beside the name, because on this symbol the two deliberately disagree:
# pin number 26 is the pin named P32. Wiring by number lands nets on the wrong
# GPIOs and passes ERC, DRC and visual review alike.
UX, UY = 127.0, 127.0
NET_BY_PIN = {
     1:("GND",        "GND"),        2:("P23", "TFT_MOSI"),   3:("P22", "SCL"),
     4:("TX0",  None),                5:("RX0",  None),        6:("P21", "SDA"),
     7:("GND",        "GND"),        8:("P19", "ENC_B"),       9:("P18", "TFT_SCK"),
    10:("P5",  "TFT_CS"),           11:("P17", "TFT_DC"),     12:("P16", "RELAY_K"),
    13:("P4",  "SSR_DRV"),          14:("P0",   None),        15:("P2",   None),
    16:("P15", "ENC_SW"),           17:("SD1",  None),        18:("SD0",  None),
    19:("CLK",  None),
    20:("3V3", "+3V3"),             21:("EN",   None),        22:("P36_VP","ESTOP_SENSE"),
    23:("P39_VN","FLOW_PULSE"),     24:("P34", "CYCLE_START"),25:("P35", "CYCLE_STOP"),
    26:("P32", "CAN_RX"),           27:("P33", "OW_DATA"),    28:("P25",  None),
    29:("P26",  None),              30:("P27",  None),        31:("P14", "CAN_TX"),
    32:("P12",  None),              33:("GND",  "GND"),       34:("P13", "ENC_A"),
    35:("SD2",  None),              36:("SD3",  None),        37:("GND",  "GND"),
    38:("5V",  "+5V"),
}
# Pin geometry read from the symbol itself -- never retyped.
_lib = io.open("shrink-fit-controller.kicad_sym", encoding="utf-8").read()
_k = _lib.index('(symbol "NodeMCU-32S_1_1"')
_d = 0
for _e in range(_k, len(_lib)):
    if _lib[_e] == '(': _d += 1
    elif _lib[_e] == ')':
        _d -= 1
        if _d == 0: break
_blk = _lib[_k:_e+1]                      # <-- BOUNDED. Nothing outside the symbol.
GEO = {int(n): (float(x), float(y)) for x, y, a, n in
       re.findall(r'\(pin\s+\S+\s+\S+\s*\n?\s*\(at ([-\d.]+) ([-\d.]+) (\d+)\).*?\(number "(\d+)"',
                  _blk, re.S)}
assert len(GEO) == 38, f"expected 38 pins, parsed {len(GEO)}"
# Sanity: the MCU's pins sit at x = +/-15.24. Anything else means the parse
# escaped the symbol again -- which it did once, silently, and ERC found it.
assert all(abs(abs(x) - 15.24) < 0.01 for x, _ in GEO.values()), "GEO parse escaped the symbol"

instance("NodeMCU-32S","U1","NodeMCU-32S (ESP32, 38-pin)",UX,UY,0,38,UX-14,UY-30,UX-14,UY+30,
         fp="shrink-fit-controller:NodeMCU-32S_38pin_THT")
for num,(name,net) in NET_BY_PIN.items():
    px,py = GEO[num]
    x, y  = UX + px, UY - py
    ang   = 180 if px < 0 else 0
    if net: label(net, x, y, ang)
    else:   nc(x, y)

# ---- SSR driver : unchanged, 2N3904 off a real GPIO that can source ----------
label("SSR_DRV",193.04,81.28,180)
wire(193.04,81.28,195.58,81.28)
instance("R","R1","1k",200.66,81.28,90,2,198.12,77.47,198.12,74.93,
         fp="Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal")
wire(205.74,81.28,208.28,81.28)
instance("Q_2N3904","Q1","2N3904",213.36,81.28,0,3,217.17,80.01,217.17,82.55,
         fp="Package_TO_SOT_THT:TO-92_Inline")
wire(215.9,76.2,215.9,73.66); label("SSR_K",215.9,73.66,90)
wire(215.9,86.36,215.9,88.9); label("GND",215.9,88.9,270)

# ---- notes ----
text("WIRE BY PIN NAME, NEVER BY NUMBER. On this symbol pin NUMBER 26 is the pin NAMED P32 - numbers are header positions. Verify every net against the pad number in docs/pinmap.md after layout; ERC will not catch it.",170.18,25.4)
text("FLASH PINS - NPTH, NO COPPER: pins 17,18,19,35,36 (SD1,SD0,CLK,SD2,SD3) are the internal SPI flash bus, GPIO6-11. Holes with no pad. Do NOT delete the holes or the module cannot seat.",170.18,33.02)
text("34/35/36/39 (pins 24,25,22,23) are INPUT ONLY with NO internal pull-up. Every button and sensor on them needs an external 10k to 3V3 or it floats and reads randomly.",170.18,40.64)
text("GND IS DC-. The controller floats with the bus. No USB while energised; no earthed scope ground clip - differential probe or a non-contact pickup loop only.",170.18,48.26)
text("SSR-100DD input 3-32V from +12V, Q1 low-side. P4 delivers the voltage STEP (CR-05); the AC contactor never does - a Royer fed a slow ramp shoots through and kills the IRFP260Ns.",170.18,55.88)
text("LAMPS ARE NOT ON THE MCU. Three 12V panel lamps hang off a PCF8574 (J13) via 2N7002 MOSFETs. The expander sources only ~100uA when high, which is too little for a 2N3904 base - a MOSFET gate is voltage-driven and does not care.",170.18,63.5)
text("MAINS START IS NOT HERE. It is hardwired in the contactor coil circuit (requirements 6.9) and must never reach the MCU: firmware must not be able to energise mains by itself.",170.18,71.12)
text("Bus voltage now comes from the INA228 (J10), not a discrete divider. The old 220k/10k and its filter cap are deleted.",170.18,78.74)
text("SPARE: P25 (28), P26 (29), P27 (30) - freed by moving lamps to the expander. P0/P2/P12 are strapping pins, usable only with care.",170.18,86.36)

# ---------------- write ----------------
sch = (f'(kicad_sch\n\t(version 20260306)\n\t(generator "eeschema")\n'
       f'\t(generator_version "10.0")\n\t(uuid "{ROOT}")\n\t(paper "A3")\n'
       f'\t(title_block\n\t\t(title "Shrink-Fit Induction Controller")\n'
       f'\t\t(date "2026-08-31")\n\t\t(rev "B")\n'
       f'\t\t(comment 1 "U1 = NodeMCU-32S. Wired by pin NAME; numbers are header positions and pin 26 is named P32. Symbol and footprint are the proven pair from sln-geo-aux-heat.")\n'
       f'\t\t(comment 2 "Rev B: MCU changed S3 Super Mini -> NodeMCU-32S. 24V primary, two bucks. MAX31855 deleted. Lamps on a PCF8574. Bus sense via INA228.")\n'
       f'\t)\n\t(lib_symbols\n{EMBED}\t)\n'
       + "".join(out) +
       f'\t(sheet_instances\n\t\t(path "/"\n\t\t\t(page "1")\n\t\t)\n\t)\n'
       f'\t(embedded_fonts no)\n)\n')
io.open("shrink-fit-controller.kicad_sch","w",encoding="utf-8").write(sch)
print(f"schematic written: {len(sch)} bytes, {len(out)} top-level items")
