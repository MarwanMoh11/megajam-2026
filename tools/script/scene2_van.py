# Scene 2 set: the idling 1978 florist van across the street from the theater, its cramped interior (Max at the CRTs,
# the telecom rack, buckets of fake lilies, cables) and the security camera on the facade that feeds the main CRT.
# An overlay like tools/sets/*.py: clears its tag first (re-runnable), everything NoCollision. Does not save the level.
# Run under the editor lock:  "<engine python>" -I tools/ue_mcp.py py tools/script/scene2_van.py 110
# Numbers (cm, world): X west->east, +Y north, Z up, street 0. The van's bonnet points south (-Y).
import importlib
import math
import random
import sys

import unreal

sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C   # noqa: E402

importlib.reload(C)

TAG = "script_s2_set"
DIR = "/Game/Take/Script/S2"
CUBE, CYL, SPH, CONE = ("/Engine/BasicShapes/" + n for n in ("Cube", "Cylinder", "Sphere", "Cone"))

# --- the van's numbers ---------------------------------------------------------------------------------------------
YOFF = 0.0                                        # nudge north/south if the street needs it
X0, X1 = -5485.0, -5275.0                         # body sides (width 210)
Y0, Y1 = 1290.0 + YOFF, 1810.0 + YOFF             # nose .. rear doors (length 520)
CX = (X0 + X1) / 2
ZB, FLOOR, ROOF = 30.0, 55.0, 235.0               # body bottom, cargo deck top, roof top
CAB_Y1 = 1450.0 + YOFF                            # cab | cargo (no partition: open for the camera)
WS = ((Y0 + 10.0, 115.0), (Y0 + 50.0, 205.0))     # windshield bottom and top edge (y, z)
T = 4.0                                           # wall thickness
SEAT = (-5405.0, 1640.0 + YOFF)                   # Max's seat (hips)
MAX_SCALE = 0.9
MAX_MESH = "/Game/Take/Cast/NPCs/Clerk/SKM_NPC_Clerk"
MAX_ANIM = "/Game/Take/Anim/Mocap/SitIdle_A"      # Mixamo Sitting Idle, hip height 57.5 (tools/mocap_clips.json)
MAX_DX = 0.0                                      # actor x relative to SEAT: set from the measured pelvis
SEAT_TOP = FLOOR + 42.0                           # crate top under his hips
HEAD = (-5400.0, 1640.0 + YOFF, 172.0)            # headset centre (head bone + ~8 cm): measured after the pose
RACK_X0, RACK_X1 = -5300.0, X1 - T                # telecom rack against the east wall
DESK_Z = 120.0                                    # desk shelf top
MAIN = dict(x0=-5345.0, x1=-5300.0, y0=1612.0 + YOFF, y1=1668.0 + YOFF, z0=DESK_Z, z1=DESK_Z + 52.0)
CAM = (-3330.0, 120.0, 420.0)                     # security camera on the facade
SCANNER = (-3520.0, 320.0, 110.0)

rnd = random.Random(1978)
LOG = []


# --- materials -------------------------------------------------------------------------------------------------------
def lc(rgb):
    return unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0)


def _p(m, cls, name, x, y, value=None):
    e = C.mel.create_material_expression(m, cls, x, y)
    e.set_editor_property("parameter_name", name)
    if value is not None:
        e.set_editor_property("default_value", value)
    return e


def _custom(m, code, inputs, x, y, out=unreal.CustomMaterialOutputType.CMOT_FLOAT3):
    cu = C.mel.create_material_expression(m, unreal.MaterialExpressionCustom, x, y)
    cu.set_editor_property("code", code)
    cu.set_editor_property("output_type", out)
    ins = []
    for nm in inputs:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", nm)
        ins.append(ci)
    cu.set_editor_property("inputs", ins)
    return cu


def master(name, build):
    path = DIR + "/" + name
    if C.eal.does_asset_exist(path):
        return C.eal.load_asset(path)
    m = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, DIR, unreal.Material,
                                                                unreal.MaterialFactoryNew())
    m.set_editor_property("used_with_instanced_static_meshes", True)    # usage flags here, never at runtime
    build(m)
    C.mel.recompile_material(m)
    C.eal.save_asset(path, only_if_is_dirty=False)
    return m


def build_blink(m):
    """Unlit LEDs that blink, each instance on its own phase and rate (PerInstanceRandom)."""
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    col = _p(m, unreal.MaterialExpressionVectorParameter, "Color", -700, -100, lc((0.1, 1.0, 0.2)))
    sc = _p(m, unreal.MaterialExpressionScalarParameter, "Scale", -700, 50, 6.0)
    rate = _p(m, unreal.MaterialExpressionScalarParameter, "Rate", -700, 120, 1.3)
    tm = C.mel.create_material_expression(m, unreal.MaterialExpressionTime, -700, 200)
    pr = C.mel.create_material_expression(m, unreal.MaterialExpressionPerInstanceRandom, -700, 260)
    cu = _custom(m, "float r = Rate * (0.5 + 1.5 * R); float on = step(0.35 + 0.3 * frac(R * 13.0), "
                    "frac(Time * r + R * 7.0)); return Col * Scale * (0.08 + 0.92 * on);",
                 ["Col", "Scale", "Rate", "Time", "R"], -350, 0)
    for e, pin in ((col, "Col"), (sc, "Scale"), (rate, "Rate"), (tm, "Time"), (pr, "R")):
        C.mel.connect_material_expressions(e, "", cu, pin)
    C.mel.connect_material_property(cu, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


def build_screen(m):
    """Unlit CRT face: colour x scale, scanlines, a soft vignette and a faint flicker."""
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    col = _p(m, unreal.MaterialExpressionVectorParameter, "Color", -700, -100, lc((0.2, 0.8, 0.3)))
    sc = _p(m, unreal.MaterialExpressionScalarParameter, "Scale", -700, 50, 1.5)
    uv = C.mel.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -700, 150)
    tm = C.mel.create_material_expression(m, unreal.MaterialExpressionTime, -700, 220)
    cu = _custom(m, "float2 d = UV - 0.5; float v = saturate(1.0 - dot(d, d) * 2.2);"
                    "float s = 0.75 + 0.25 * sin(UV.y * 260.0 + Time * 9.0);"
                    "float f = 0.94 + 0.06 * sin(Time * 37.0);"
                    "return Col * Scale * v * s * f;", ["Col", "Scale", "UV", "Time"], -350, 0)
    for e, pin in ((col, "Col"), (sc, "Scale"), (uv, "UV"), (tm, "Time")):
        C.mel.connect_material_expressions(e, "", cu, pin)
    C.mel.connect_material_property(cu, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


def build_fog(m):
    """Fogged pale windshield: translucent, a little milky glow so it reads in the dark street."""
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    col = _p(m, unreal.MaterialExpressionVectorParameter, "Color", -700, -100, lc((0.62, 0.68, 0.68)))
    op = _p(m, unreal.MaterialExpressionScalarParameter, "Opacity", -700, 50, 0.32)
    fog = _p(m, unreal.MaterialExpressionScalarParameter, "Fog", -700, 120, 0.03)
    rough = _p(m, unreal.MaterialExpressionScalarParameter, "Roughness", -700, 190, 0.25)
    mul = C.mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -400, 100)
    C.mel.connect_material_expressions(col, "", mul, "A")
    C.mel.connect_material_expressions(fog, "", mul, "B")
    C.mel.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    C.mel.connect_material_property(op, "", unreal.MaterialProperty.MP_OPACITY)
    C.mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    C.mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


def instance(name, parent, vectors=None, scalars=None):
    path = DIR + "/" + name
    if C.eal.does_asset_exist(path):
        mi = C.eal.load_asset(path)
    else:
        mi = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    C.mel.set_material_instance_parent(mi, parent)
    for k, v in (vectors or {}).items():
        C.mel.set_material_instance_vector_parameter_value(mi, k, lc(v))
    for k, v in (scalars or {}).items():
        C.mel.set_material_instance_scalar_parameter_value(mi, k, float(v))
    C.mel.update_material_instance(mi)
    C.eal.save_asset(path, only_if_is_dirty=True)
    return mi


FLAT = {   # linear base colour, roughness (on the hall's M_HallFlat)
    "Cream": ((0.58, 0.50, 0.35), 0.55), "Mint": ((0.10, 0.30, 0.24), 0.6), "Chrome": ((0.55, 0.55, 0.57), 0.15),
    "Rubber": ((0.018, 0.018, 0.018), 0.9), "Black": ((0.014, 0.014, 0.015), 0.5),
    "DarkGlass": ((0.006, 0.008, 0.010), 0.05), "Wood": ((0.20, 0.12, 0.06), 0.8), "Ply": ((0.28, 0.20, 0.12), 0.85),
    "Steel": ((0.10, 0.10, 0.11), 0.4), "Beige": ((0.36, 0.31, 0.23), 0.6), "Gear": ((0.06, 0.065, 0.07), 0.45),
    "Galv": ((0.30, 0.31, 0.32), 0.35), "Stem": ((0.04, 0.13, 0.03), 0.7), "Leaf": ((0.03, 0.09, 0.025), 0.75),
    "Petal": ((0.80, 0.78, 0.70), 0.6), "Pink": ((0.70, 0.35, 0.40), 0.6), "Stamen": ((0.60, 0.38, 0.04), 0.6),
    "CableBlk": ((0.02, 0.02, 0.02), 0.55), "CableRed": ((0.30, 0.02, 0.02), 0.5),
    "CableYel": ((0.50, 0.38, 0.02), 0.5), "CableGry": ((0.16, 0.16, 0.16), 0.5), "Mat": ((0.035, 0.035, 0.03), 0.95),
    "Vinyl": ((0.10, 0.045, 0.025), 0.45), "Red": ((0.45, 0.02, 0.02), 0.4), "Ink": ((0.05, 0.16, 0.12), 0.6),
}
GLOWS = {  # base, emissive, scale
    "Headlight": ((0.8, 0.75, 0.6), (1.0, 0.88, 0.65), 1.2), "Tail": ((0.2, 0.0, 0.0), (1.0, 0.04, 0.02), 3.0),
    "Amber": ((0.3, 0.15, 0.0), (1.0, 0.45, 0.05), 3.0), "Bulb": ((0.8, 0.7, 0.5), (1.0, 0.72, 0.42), 12.0),
    "VU": ((0.3, 0.2, 0.05), (1.0, 0.62, 0.18), 2.0),
}
M = {}


def materials():
    flat = C.eal.load_asset("/Game/Take/Sets/Hall/M_HallFlat")
    for k, (rgb, r) in FLAT.items():
        M[k] = instance("MI_S2_" + k, flat, {"Color": rgb}, {"Roughness": r})
    for k, (rgb, em, s) in GLOWS.items():
        M[k] = instance("MI_S2_" + k, flat, {"Color": rgb, "Emissive": em}, {"Roughness": 0.4, "EmissiveScale": s})
    blink, screen, fog = master("M_S2_Blink", build_blink), master("M_S2_Screen", build_screen), \
        master("M_S2_FogGlass", build_fog)
    for k, rgb, rate in (("LedG", (0.1, 1.0, 0.15), 1.4), ("LedR", (1.0, 0.05, 0.03), 0.7),
                         ("LedA", (1.0, 0.5, 0.05), 2.2)):
        M[k] = instance("MI_S2_" + k, blink, {"Color": rgb}, {"Scale": 8.0, "Rate": rate})
    M["ScreenMain"] = instance("MI_S2_ScreenMain", screen, {"Color": (0.42, 0.55, 0.46)}, {"Scale": 1.6})
    M["ScreenG"] = instance("MI_S2_ScreenG", screen, {"Color": (0.10, 0.75, 0.18)}, {"Scale": 1.4})
    M["ScreenGrey"] = instance("MI_S2_ScreenGrey", screen, {"Color": (0.45, 0.50, 0.52)}, {"Scale": 1.1})
    M["Fog"] = instance("MI_S2_FogGlass", fog, {"Color": (0.66, 0.71, 0.70)},
                        {"Opacity": 0.42, "Fog": 0.09, "Roughness": 0.25})


# --- the kit (tools/sets/front.py's Kit, with a flat slab) ------------------------------------------------------------
class Kit:
    def __init__(self):
        self.g = {}

    def add(self, m, mt, loc, r, sc, shadow=False):
        self.g.setdefault((m, mt, shadow), []).append((tuple(loc), r, tuple(sc)))

    def box(self, b, mt, yaw=0.0, pitch=0.0, roll=0.0, **kw):
        """b = (x0, x1, y0, y1, z0, z1) before turning; turned about its centre."""
        self.add(CUBE, mt, ((b[0] + b[1]) / 2, (b[2] + b[3]) / 2, (b[4] + b[5]) / 2), (roll, pitch, yaw),
                 ((b[1] - b[0]) / 100.0, (b[3] - b[2]) / 100.0, (b[5] - b[4]) / 100.0), **kw)

    def cyl(self, x, y, z0, z1, d, mt, dy=None, **kw):
        self.add(CYL, mt, (x, y, (z0 + z1) / 2), (0.0, 0.0, 0.0), (d / 100.0, (dy or d) / 100.0, (z1 - z0) / 100.0),
                 **kw)

    def ball(self, c, d, mt, dz=None, **kw):
        self.add(SPH, mt, c, (0.0, 0.0, 0.0), (d / 100.0, d / 100.0, (dz or d) / 100.0), **kw)

    def seg(self, p, q, d, mt, extend=0.0, m=CYL, w=None, **kw):
        """A cylinder (or cube, or cone) from p to q, d across (w: a second width, for flat slabs)."""
        dx, dy, dz = q[0] - p[0], q[1] - p[1], q[2] - p[2]
        ln = math.sqrt(dx * dx + dy * dy + dz * dz)
        pitch = math.degrees(math.atan2(dz, math.hypot(dx, dy))) - 90.0
        self.add(m, mt, ((p[0] + q[0]) / 2, (p[1] + q[1]) / 2, (p[2] + q[2]) / 2),
                 (0.0, pitch, math.degrees(math.atan2(dy, dx))), (d / 100.0, (w or d) / 100.0, (ln + extend) / 100.0),
                 **kw)

    def rope(self, pts, d, mt, **kw):
        for p, q in zip(pts, pts[1:]):
            self.seg(p, q, d, mt, extend=d * 0.6, **kw)

    def flush(self):
        n = 0
        for (m, mt, shadow), items in self.g.items():
            if len(items) == 1:
                C.spawn(m, items[0][0], items[0][1], items[0][2], tag=TAG, shadow=shadow, mat=mt)
            else:
                C.scatter(m, items, tag=TAG, shadow=shadow, mat=mt)
            n += len(items)
        self.g = {}
        return n


K = Kit()


def sag(p, q, drop, n=8, wob=0.0):
    """Points of a cable hanging from p to q, `drop` below the straight line at its middle."""
    out = []
    for i in range(n + 1):
        t = i / float(n)
        out.append((p[0] + (q[0] - p[0]) * t + rnd.uniform(-wob, wob), p[1] + (q[1] - p[1]) * t + rnd.uniform(-wob, wob),
                    p[2] + (q[2] - p[2]) * t - drop * 4.0 * t * (1.0 - t)))
    return out


def floor_run(p, q, n=6, wob=6.0, z=FLOOR + 1.0):
    pts = [p]
    for i in range(1, n):
        t = i / float(n)
        pts.append((p[0] + (q[0] - p[0]) * t + rnd.uniform(-wob, wob), p[1] + (q[1] - p[1]) * t + rnd.uniform(-wob, wob),
                    z))
    return pts + [q]


def light(label, loc, cd, radius, rgb, shadows=False, src=0.0):
    a = C.eas.spawn_actor_from_class(unreal.PointLight, unreal.Vector(*loc))
    a.set_actor_label("Set_S2_" + label)
    a.set_folder_path("Sets/" + TAG)
    c = a.get_component_by_class(unreal.PointLightComponent)
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    for k, v in (("intensity_units", unreal.LightUnits.CANDELAS), ("intensity", float(cd)),
                 ("attenuation_radius", float(radius)), ("cast_shadows", bool(shadows)), ("source_radius", float(src)),
                 ("light_color", unreal.Color(r=int(rgb[0] * 255), g=int(rgb[1] * 255), b=int(rgb[2] * 255), a=255))):
        try:
            c.set_editor_property(k, v)
        except Exception:                                       # noqa: BLE001
            pass
    a.set_editor_property("tags", [unreal.Name(TAG)])
    return a


def text(t, x, y, z, yaw, size, rgb):
    a = C.eas.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(x, y, z), unreal.Rotator(0.0, 0.0, yaw))
    a.set_actor_label("Set_S2_Lettering")
    a.set_folder_path("Sets/" + TAG)
    c = a.text_render
    try:
        c.set_text(t)
    except Exception:                                           # noqa: BLE001
        c.set_editor_property("text", t)
    for (k, v) in (("world_size", float(size)), ("text_render_color", unreal.Color(r=rgb[0], g=rgb[1], b=rgb[2], a=255)),
                   ("horizontal_alignment", unreal.HorizTextAligment.EHTA_CENTER),
                   ("vertical_alignment", unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)):
        try:
            c.set_editor_property(k, v)
        except Exception:                                       # noqa: BLE001
            pass
    a.set_editor_property("tags", [unreal.Name(TAG)])
    return a


# --- the van, outside ------------------------------------------------------------------------------------------------
def body():
    S = dict(shadow=True)                                       # the shell casts shadows: the inside light stays in
    cr, mi = M["Cream"], M["Mint"]
    (wy0, wz0), (wy1, wz1) = WS
    for (xa, xb, skin) in ((X0, X0 + T, X0 - 1.0), (X1 - T, X1, X1)):
        K.box((xa, xb, Y0, Y1, ZB, 115.0), cr, **S)                         # below the window line, nose to tail
        K.box((xa, xb, wy0, wy1 + 5.0, 115.0, ROOF), cr, **S)               # A pillar
        K.box((xa, xb, wy1 + 5.0, wy1 + 100.0, 115.0, 128.0), cr, **S)      # under the cab window
        K.box((xa, xb, wy1 + 5.0, wy1 + 100.0, 198.0, ROOF), cr, **S)       # over it
        K.box((xa + 0.5, xb - 0.5, wy1 + 5.0, wy1 + 100.0, 128.0, 198.0), M["DarkGlass"], **S)   # cab window
        K.box((xa, xb, wy1 + 100.0, Y1, 115.0, ROOF), cr, **S)              # cargo side, no windows
        K.box((skin, skin + 1.0, Y0, Y1, ZB, 92.0), mi)                     # the faded mint band
        K.box((skin - 0.2, skin + 1.2, Y0, Y1, 92.0, 95.0), M["Chrome"])    # pinstripe
        K.box((skin - 0.3, skin + 1.3, wy1 + 92.0, wy1 + 96.0, ZB + 3, 198.0), M["Black"])   # cab door seam
        K.box((skin - 0.5, skin + 1.5, wy1 + 70.0, wy1 + 82.0, 118.0, 121.0), M["Chrome"])   # door handle
    K.box((X0, X1, Y0, Y0 + T, ZB, wz0), cr, **S)                             # the nose under the windshield
    K.box((X0, X1, Y0 - 1.0, Y0, ZB, 80.0), mi)
    K.box((X0, X1, wy1 - 2.0, wy1 + 6.0, wz1, ROOF), cr, **S)                # header over the windshield
    K.box((X0, X1, wy1 - 2.0, Y1, ROOF - 6.0, ROOF), cr, **S)                # roof
    K.box((X0 - 1.5, X1 + 1.5, wy1 + 4.0, Y1, ROOF - 1.0, ROOF + 1.5), M["Chrome"])   # drip rail
    K.box((X0, X1, Y1 - T, Y1, ZB, ROOF), cr, **S)                            # rear doors
    K.box((X0, X1, Y1, Y1 + 1.0, ZB, 92.0), mi)
    K.box((CX - 0.6, CX + 0.6, Y1, Y1 + 1.5, ZB + 5, ROOF - 8), M["Black"])  # rear door split
    for sx in (-1, 1):
        K.box((CX + sx * 55 - 30, CX + sx * 55 + 30, Y1, Y1 + 1.2, 150.0, 200.0), M["DarkGlass"])   # rear windows
        K.box((CX + sx * 92 - 6, CX + sx * 92 + 6, Y1, Y1 + 2.0, 70.0, 95.0), M["Tail"])
        K.box((CX + sx * 92 - 6, CX + sx * 92 + 6, Y1, Y1 + 2.0, 60.0, 70.0), M["Amber"])
    K.box((X0, X1, Y0 + T, Y1 - T, FLOOR - 10.0, FLOOR), M["Mat"], **S)       # cargo deck / cab floor
    K.box((X0 + 25, X1 - 25, Y0 + 20, Y1 - 20, 22.0, ZB), M["Black"], **S)   # chassis
    # the nose: grille, headlights, indicators, bumpers
    K.box((CX - 50, CX + 50, Y0 - 1.5, Y0, 52.0, 98.0), M["Black"])
    for i in range(7):
        z = 56.0 + i * 6.5
        K.box((CX - 48, CX + 48, Y0 - 2.5, Y0 - 1.5, z, z + 2.0), M["Chrome"])
    for sx in (-1, 1):
        hx = CX + sx * 78
        K.seg((hx, Y0 - 1.0, 86.0), (hx, Y0 - 4.0, 86.0), 22.0, M["Chrome"])
        K.seg((hx, Y0 - 3.0, 86.0), (hx, Y0 - 5.0, 86.0), 17.0, M["Headlight"])
        K.box((hx - 8, hx + 8, Y0 - 2.5, Y0, 62.0, 70.0), M["Amber"])
        K.seg((X0 - 4 if sx < 0 else X1 + 4, wy1 + 10, 175.0), (X0 - 22 if sx < 0 else X1 + 22, wy1 + 4, 172.0),
              2.0, M["Chrome"])                                                # mirror arm
        mx = X0 - 26 if sx < 0 else X1 + 26
        K.box((mx - 3, mx + 3, wy1, wy1 + 4, 150.0, 185.0), M["Black"])         # mirror
    K.box((X0 - 6, X1 + 6, Y0 - 12, Y0 - 2, 32.0, 47.0), M["Chrome"])          # bumpers
    K.box((X0 - 4, X1 + 4, Y1 + 1, Y1 + 11, 32.0, 47.0), M["Chrome"])
    # wheels (radius 35) with a dark arch ring behind each
    for wy in (Y0 + 95.0, Y1 - 110.0):
        for (xo, xi, xr) in ((X0 - 3.0, X0 + 19.0, X0 - 1.0), (X1 + 3.0, X1 - 19.0, X1 + 1.0)):
            K.seg((xr, wy, 37.0), (xr + (0.6 if xr > CX else -0.6), wy, 37.0), 86.0, M["Black"])
            K.seg((xo, wy, 35.0), (xi, wy, 35.0), 70.0, M["Rubber"], shadow=True)
            K.seg((xo + (-0.5 if xo > CX else 0.5) * 0, wy, 35.0), (xo + (1.0 if xo > CX else -1.0), wy, 35.0),
                  36.0, M["Chrome"])
    # the windshield: fogged pale glass, slanted, as its own actor so a shot can hide it
    (y0, z0), (y1, z1) = WS
    ln = math.hypot(y1 - y0, z1 - z0)
    pitch = math.degrees(math.atan2(z1 - z0, y1 - y0)) - 90.0
    C.spawn(CUBE, (CX, (y0 + y1) / 2, (z0 + z1) / 2), (0.0, pitch, 90.0), (0.012, (X1 - X0 - 2 * T) / 100.0, ln / 100.0),
            tag=TAG, label="Set_S2_Windshield", shadow=False, mat=M["Fog"])
    K.seg((X0 + T, y0, z0), (X1 - T, y0, z0), 4.0, M["Black"])               # rubber seal, bottom and top
    K.seg((X0 + T, y1, z1), (X1 - T, y1, z1), 4.0, M["Black"])
    K.seg((CX + 40, y0 + 2, z0 + 3), (CX + 10, y0 + 12, z0 + 30), 1.2, M["Black"])   # wipers
    K.seg((CX - 60, y0 + 2, z0 + 3), (CX - 90, y0 + 12, z0 + 30), 1.2, M["Black"])
    # the lettering on both flanks
    for (x, yaw) in ((X1 + 1.6, 0.0), (X0 - 1.6, 180.0)):
        text("FLEURS DU SOIR", x, Y1 - 190.0, 172.0, yaw, 24.0, (18, 72, 58))
        text("LIVRAISON NUIT  \u00b7  T\u00c9L. 26-14-78", x, Y1 - 190.0, 146.0, yaw, 9.0, (18, 72, 58))
        K.box((x - 0.4, x + 0.4, Y1 - 330.0, Y1 - 50.0, 158.0, 159.5) if x > CX else
              (x - 0.4, x + 0.4, Y1 - 330.0, Y1 - 50.0, 158.0, 159.5), M["Ink"])


# --- the cab ---------------------------------------------------------------------------------------------------------
def cab():
    (y0, z0), (y1, z1) = WS
    K.box((X0 + T, X1 - T, Y0 + T, y0 + 30.0, 95.0, z0 + 5.0), M["Black"])            # dashboard
    K.box((X0 + T, X1 - T, y0 + 22.0, y0 + 34.0, 108.0, 116.0), M["Vinyl"], pitch=0.0)  # dash top lip
    for i in range(3):
        K.cyl(X1 - 50 - i * 12, y0 + 24.0, 117.0, 118.0, 7.0, M["Chrome"])              # gauges
    K.box((-5410, -5355, Y0 + T, Y0 + 100, FLOOR, 85.0), M["Black"])                   # engine hump
    # driver's seat (east), wheel and column
    dx = X1 - 52.0
    K.box((dx - 28, dx + 28, Y0 + 110, Y0 + 155, FLOOR, 92.0), M["Vinyl"])
    K.box((dx - 28, dx + 28, Y0 + 152, Y0 + 162, 92.0, 150.0), M["Vinyl"], pitch=0.0)
    K.seg((dx, Y0 + 40, 95.0), (dx, Y0 + 70, 125.0), 4.0, M["Black"])
    K.seg((dx, Y0 + 67, 127.0), (dx, Y0 + 72, 122.0), 42.0, M["Black"])
    K.seg((dx, Y0 + 66, 127.0), (dx, Y0 + 73, 122.0), 36.0, M["Vinyl"])               # rim reads as a ring
    # a low jump seat west (the cab camera sits above it), maps and cups on the dash
    K.box((X0 + 10, X0 + 60, Y0 + 115, Y0 + 155, FLOOR, 88.0), M["Vinyl"])
    K.box((X0 + 20, X0 + 60, y0 + 18, y0 + 30, 116.0, 117.5), M["Petal"], yaw=12.0)   # an order slip
    K.cyl(X0 + 75, y0 + 25, 116.0, 126.0, 8.0, M["Red"])                               # a thermos


# --- the cargo box: rack, desk, CRTs, cables, lilies, the crate -------------------------------------------------------
def crt(x0, x1, y0, y1, z0, z1, screen, label=None, yaw=0.0):
    """A CRT on its west face: body, bezel, screen, two knobs. yaw turns it about its centre (only small angles)."""
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    K.box((x0, x1, y0, y1, z0, z1), M["Beige"], yaw=yaw, shadow=True)
    w, h = (y1 - y0), (z1 - z0)
    r = math.radians(yaw)
    fx, fy = cx - (x1 - x0) / 2 * math.cos(r), cy - (x1 - x0) / 2 * math.sin(r)     # centre of the west face
    K.box((fx - 1.5, fx + 0.5, fy - w * 0.46, fy + w * 0.46, z0 + h * 0.06, z1 - h * 0.06), M["Black"], yaw=yaw)
    loc = (fx - 2.0, fy, z0 + h * 0.52)
    sc = (0.012, w * 0.78 / 100.0, h * 0.70 / 100.0)
    if label:
        C.spawn(CUBE, loc, (0.0, 0.0, yaw), sc, tag=TAG, label=label, shadow=False, mat=screen)
    else:
        K.add(CUBE, screen, loc, (0.0, 0.0, yaw), sc)
    return loc


def interior():
    # plywood lining and a ceiling liner in the cargo box
    for xa in (X0 + T, X1 - T - 1.5):
        K.box((xa, xa + 1.5, CAB_Y1, Y1 - T, FLOOR, ROOF - 6.0), M["Ply"])
    K.box((X0 + T, X1 - T, CAB_Y1, Y1 - T, ROOF - 7.5, ROOF - 6.0), M["Ply"])
    for y in (CAB_Y1 + 60, CAB_Y1 + 180, CAB_Y1 + 300):                                   # ribs
        K.box((X0 + T, X1 - T, y, y + 4, ROOF - 12, ROOF - 7.5), M["Steel"])
    # the rack: four posts, shelves, units with LEDs, knobs and VU windows on their west faces
    ry0, ry1 = 1500.0 + YOFF, 1780.0 + YOFF
    for y in (ry0, ry1 - 3):
        for x in (RACK_X0, RACK_X1 - 3):
            K.box((x, x + 3, y, y + 3, FLOOR, 215.0), M["Steel"])
    leds = {"LedG": [], "LedR": [], "LedA": []}
    units = []                                     # (y0, y1, z0, z1, depth): the stacks, around the desk and screens
    z = FLOOR + 2
    for h in (14, 9, 18, 9, 22):                   # under the desk, full width
        units.append((ry0 + 2, ry1 - 2, z, z + h - 1, 0))
        z += h
    z = DESK_Z + 2                                 # beside the desk screens, south end
    for h in (9, 14, 9, 18, 9, 22, 9):
        if z + h > 214:
            break
        units.append((ry0 + 2, ry0 + 44, z, z + h - 1, 0))
        z += h
    for (z0, z1) in ((DESK_Z + 58, DESK_Z + 66), (208, 214)):    # over the main CRT, top row
        units.append((ry0 + 46, ry1 - 2, z0, z1, 0))
    for (y0, y1, z0, z1, _) in units:
        dark = rnd.random() < 0.6
        K.box((RACK_X0, RACK_X1 - 1, y0, y1, z0, z1), M["Gear"] if dark else M["Beige"], shadow=True)
        fx = RACK_X0 - 0.6
        n = int((y1 - y0) / 7)
        for i in range(n):
            if rnd.random() < 0.55:
                y = y0 + 4 + i * 7 + rnd.uniform(-1, 1)
                leds[rnd.choice(("LedG", "LedG", "LedR", "LedA"))].append(((fx, y, z0 + (z1 - z0) * 0.65), 0.0,
                                                                            (0.012, 0.012, 0.012)))
        if (z1 - z0) > 12 and rnd.random() < 0.7:
            for i in range(rnd.randint(2, 4)):
                K.seg((RACK_X0, y0 + 10 + i * 9, z0 + (z1 - z0) * 0.35), (RACK_X0 - 2.5, y0 + 10 + i * 9,
                                                                       z0 + (z1 - z0) * 0.35), 3.0, M["Chrome"])
            if rnd.random() < 0.6:
                vy = y0 + (y1 - y0) * 0.6
                K.box((RACK_X0 - 0.8, RACK_X0, vy, vy + 10, z0 + 3, z1 - 3), M["VU"])
    for k, items in leds.items():
        for it in items:
            K.add(CUBE, M[k], *it)
    # the desk shelf and the upper shelf, with brackets
    dy0, dy1 = 1545.0 + YOFF, 1775.0 + YOFF
    K.box((-5352.0, RACK_X0, dy0, dy1, DESK_Z - 3, DESK_Z), M["Ply"], shadow=True)
    K.box((-5332.0, RACK_X0, dy0 + 20, dy1, DESK_Z + 56, DESK_Z + 58), M["Ply"], shadow=True)
    for y in (dy0 + 5, dy1 - 5):
        K.seg((-5350.0, y, DESK_Z - 3), (RACK_X0, y, DESK_Z - 35), 2.5, M["Steel"])
    # the CRTs: the main one in front of Max (its screen its own actor, for the feed), three small ones
    main_c = crt(MAIN["x0"], MAIN["x1"], MAIN["y0"], MAIN["y1"], MAIN["z0"], MAIN["z1"], M["ScreenMain"],
                 label="Set_S2_MainCRT_Screen")
    crt(-5332.0, -5300.0, dy0 + 6, dy0 + 48, DESK_Z, DESK_Z + 34, M["ScreenG"], yaw=-14.0)
    crt(-5328.0, -5300.0, 1575.0 + YOFF, 1610.0 + YOFF, DESK_Z + 58, DESK_Z + 88, M["ScreenGrey"], yaw=-8.0)
    crt(-5328.0, -5300.0, 1680.0 + YOFF, 1722.0 + YOFF, DESK_Z + 58, DESK_Z + 92, M["ScreenG"], yaw=10.0)
    # patch panel (tilted toward Max) with jacks and short patch cords, and the big toggle
    py0, py1 = 1675.0 + YOFF, 1728.0 + YOFF
    K.box((-5340.0, -5306.0, py0, py1, DESK_Z, DESK_Z + 16), M["Gear"], pitch=-18.0)
    for r_ in range(3):
        for i in range(7):
            K.add(CYL, M["Black"], (-5341.0 + r_ * 6.0, py0 + 6 + i * 7, DESK_Z + 9 + r_ * 2.0), (0.0, 72.0, 0.0),
                  (0.022, 0.022, 0.02))
    for i in range(5):
        a = (-5342.0 + rnd.uniform(0, 10), py0 + 6 + rnd.randint(0, 6) * 7, DESK_Z + 10)
        b = (-5342.0 + rnd.uniform(0, 10), py0 + 6 + rnd.randint(0, 6) * 7, DESK_Z + 12)
        K.rope(sag(a, b, rnd.uniform(6, 14), 5), 1.0, M[rnd.choice(("CableRed", "CableYel", "CableGry"))])
    tx, ty = -5333.0, 1748.0 + YOFF
    K.box((tx - 12, tx + 12, ty - 14, ty + 14, DESK_Z, DESK_Z + 12), M["Steel"], shadow=True)
    K.box((tx - 12.5, tx - 12, ty - 6, ty + 6, DESK_Z + 2, DESK_Z + 10), M["Red"])           # warning plate
    K.seg((tx - 4, ty, DESK_Z + 12), (tx - 14, ty, DESK_Z + 30), 2.4, M["Chrome"])         # the lever
    K.ball((tx - 14.5, ty, DESK_Z + 31), 6.0, M["Red"])
    K.ball((tx - 6, ty + 9, DESK_Z + 13), 1.6, M["LedR"])
    # cables: draped from the rack and the desk, and across the floor to the cab and the batteries
    cols = ("CableBlk", "CableBlk", "CableGry", "CableRed", "CableYel")
    for i in range(9):
        y = ry0 + 10 + i * 30 + rnd.uniform(-8, 8)
        top = (RACK_X0 - 1, y, rnd.uniform(140, 210))
        low = (RACK_X0 - rnd.uniform(20, 70), y + rnd.uniform(-60, 60), FLOOR + 1)
        K.rope(sag(top, low, rnd.uniform(-25, -8), 6), rnd.uniform(1.2, 2.2), M[rnd.choice(cols)])
    for i in range(7):
        p = (RACK_X0 - rnd.uniform(5, 40), ry0 + rnd.uniform(0, 260), FLOOR + 1)
        q = (rnd.uniform(-5470, -5330), rnd.uniform(Y0 + 120, CAB_Y1 + 20), FLOOR + 1)
        K.rope(floor_run(p, q, 7, 8.0), rnd.uniform(1.5, 2.6), M[rnd.choice(cols)])
    for i in range(4):                                                                     # loops off the desk edge
        y = dy0 + 30 + i * 50
        K.rope(sag((-5350.0, y, DESK_Z - 1), (-5350.0, y + rnd.uniform(15, 35), DESK_Z - 1),
                   rnd.uniform(30, 50), 6), 1.4, M[rnd.choice(cols)])
    K.rope(sag((RACK_X0 - 2, ry0 + 60, 205), (-5450.0, ry0 - 80, ROOF - 12), 18, 6), 1.8, M["CableBlk"])   # to the cab
    # a car battery and a power inverter on the floor near the rack's south end
    K.box((-5345.0, -5315.0, ry0 - 40, ry0 - 10, FLOOR, FLOOR + 20), M["Black"], shadow=True)
    K.cyl(-5338.0, ry0 - 18, FLOOR + 20, FLOOR + 23, 3.0, M["Red"])
    K.box((-5350.0, -5320.0, ry0 - 75, ry0 - 50, FLOOR, FLOOR + 12), M["Steel"], shadow=True)
    # Max's crate and a coffee cup on the floor
    sx, sy = SEAT
    K.box((sx - 26, sx + 22, sy - 24, sy + 24, FLOOR, SEAT_TOP), M["Wood"], shadow=True)
    for dz in (0.3, 0.7):
        K.box((sx - 26.5, sx + 22.5, sy - 24.5, sy + 24.5, FLOOR + (SEAT_TOP - FLOOR) * dz - 1.5,
               FLOOR + (SEAT_TOP - FLOOR) * dz + 1.5), M["Ply"])
    K.cyl(sx + 30, sy - 40, FLOOR, FLOOR + 10, 8.0, M["Petal"])
    return main_c


def lily(x, y, z, yaw_out, tilt, length):
    """One fake lily: a stem leaning yaw_out by tilt degrees, a white trumpet (cone) at its tip and a stamen."""
    r, t = math.radians(yaw_out), math.radians(tilt)
    tip = (x + math.cos(r) * math.sin(t) * length, y + math.sin(r) * math.sin(t) * length, z + math.cos(t) * length)
    K.seg((x, y, z), tip, 0.9, M["Stem"])
    out = (tip[0] + math.cos(r) * 10, tip[1] + math.sin(r) * 10, tip[2] + 4)
    K.seg(out, tip, 6.5, M[rnd.choice(("Petal", "Petal", "Petal", "Pink"))], m=CONE)      # cone opens toward `out`
    K.ball(out, 1.6, M["Stamen"])
    if rnd.random() < 0.5:
        mid = ((x + tip[0]) / 2, (y + tip[1]) / 2, (z + tip[2]) / 2)
        K.add(SPH, M["Leaf"], mid, (rnd.uniform(-30, 30), 40.0, math.degrees(r) + 90), (0.03, 0.10, 0.22))


def bucket(x, y, n=7, fallen=None):
    """A galvanised bucket of fake lilies on the floor; fallen = yaw it lies toward (spilled stems)."""
    if fallen is None:
        K.cyl(x, y, FLOOR, FLOOR + 30, 26.0, M["Galv"], dy=26.0, shadow=True)
        K.cyl(x, y, FLOOR + 28, FLOOR + 31, 28.0, M["Galv"])
        for i in range(n):
            a = rnd.uniform(0, 360)
            lily(x + math.cos(math.radians(a)) * 5, y + math.sin(math.radians(a)) * 5, FLOOR + 28, a,
                 rnd.uniform(8, 30), rnd.uniform(45, 75))
        return
    r = math.radians(fallen)
    base = (x, y, FLOOR + 13)
    rim = (x + math.cos(r) * 30, y + math.sin(r) * 30, FLOOR + 13)
    K.seg(base, rim, 26.0, M["Galv"], shadow=True)
    for i in range(n):
        a = fallen + rnd.uniform(-25, 25)
        ra = math.radians(a)
        L = rnd.uniform(45, 70)
        p = (rim[0], rim[1], FLOOR + 6)
        tip = (p[0] + math.cos(ra) * L, p[1] + math.sin(ra) * L, FLOOR + 3)
        K.seg(p, tip, 0.9, M["Stem"])
        out = (tip[0] + math.cos(ra) * 8, tip[1] + math.sin(ra) * 8, FLOOR + 6)
        K.seg(out, tip, 6.5, M["Petal"], m=CONE)
        K.ball(out, 1.6, M["Stamen"])


def lilies():
    wx = X0 + T + 18.0
    for y in (1475.0, 1525.0, 1578.0, 1700.0, 1752.0):
        bucket(wx + rnd.uniform(-3, 3), y + YOFF, n=rnd.randint(6, 9))
    bucket(wx + 4, 1640.0 + YOFF + 52, n=5)                       # wedged right behind Max
    bucket(wx + 2, 1640.0 + YOFF - 55, n=6)
    for (x, y) in ((wx + 30, 1786.0), (wx + 62, 1784.0), (-5340.0, 1790.0)):   # the rear corners
        bucket(x, y + YOFF, n=rnd.randint(5, 8))
    bucket(-5440.0, 1575.0 + YOFF, n=5, fallen=-60.0)             # knocked over toward the cab
    bucket(-5378.0, 1765.0 + YOFF, n=4, fallen=200.0)
    for i in range(6):                                            # loose stems on the floor
        x, y = rnd.uniform(-5460, -5360), rnd.uniform(1470, 1790) + YOFF
        a = rnd.uniform(0, 360)
        tip = (x + math.cos(math.radians(a)) * 55, y + math.sin(math.radians(a)) * 55, FLOOR + 1.5)
        K.seg((x, y, FLOOR + 1.5), tip, 0.9, M["Stem"])
        K.seg((tip[0] + math.cos(math.radians(a)) * 8, tip[1] + math.sin(math.radians(a)) * 8, FLOOR + 5), tip, 6.5,
              M["Petal"], m=CONE)


# --- Max ---------------------------------------------------------------------------------------------------------------
def max_actor():
    sx, sy = SEAT
    a = C.eas.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(sx + MAX_DX, sy, FLOOR),
                                     unreal.Rotator(0.0, 0.0, -90.0))       # the mannequin faces +Y: -90 faces east
    a.set_actor_label("Set_S2_Max")
    a.set_folder_path("Sets/" + TAG)
    a.set_actor_scale3d(unreal.Vector(MAX_SCALE, MAX_SCALE, MAX_SCALE))
    c = a.skeletal_mesh_component
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    mesh = C.eal.load_asset(MAX_MESH)
    try:
        c.set_skeletal_mesh_asset(mesh)
    except Exception:                                           # noqa: BLE001
        c.set_editor_property("skeletal_mesh_asset", mesh)
    c.set_collision_profile_name("NoCollision")
    try:
        c.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
        c.override_animation_data(C.eal.load_asset(MAX_ANIM), True, True, 0.0, 1.0)
        c.set_update_animation_in_editor(True)
    except Exception as e:                                      # noqa: BLE001
        LOG.append("anim: %s" % e)
    a.set_editor_property("tags", [unreal.Name(TAG), unreal.Name("s2_max")])
    return a


def headset():
    hx, hy, hz = HEAD
    pts = []
    for i in range(9):                                          # the band, ear to ear over the crown
        t = math.pi * i / 8.0
        pts.append((hx - 1.0, hy - math.cos(t) * 9.5, hz - 2.0 + math.sin(t) * 11.0))
    K.rope(pts, 1.4, M["Black"])
    for s in (-1, 1):
        K.seg((hx - 1.0, hy + s * 9.0, hz - 4.0), (hx - 1.0, hy + s * 12.0, hz - 4.0), 8.0, M["Black"])
        K.seg((hx - 1.0, hy + s * 11.5, hz - 4.0), (hx - 1.0, hy + s * 12.5, hz - 4.0), 5.0, M["Steel"])
    K.rope([(hx - 1.0, hy + 11.0, hz - 6.0), (hx + 5.0, hy + 9.0, hz - 11.0), (hx + 10.0, hy + 4.0, hz - 13.0)], 0.8,
           M["Black"])                                          # boom to the mouth (his left: north)
    K.ball((hx + 10.5, hy + 3.5, hz - 13.0), 2.4, M["Black"])
    K.rope(sag((hx - 1.0, hy + 12.0, hz - 6.0), (RACK_X0 - 4, 1700.0 + YOFF, DESK_Z + 4), 25, 7), 0.8,
           M["CableGry"])                                       # its cord to the desk


# --- the security camera on the facade -------------------------------------------------------------------------------
def security_cam():
    x, y, z = CAM
    d = [SCANNER[i] - CAM[i] for i in range(3)]
    n = math.sqrt(sum(v * v for v in d))
    d = [v / n for v in d]
    back = (x - d[0] * 14, y - d[1] * 14, z - d[2] * 14)
    front = (x + d[0] * 14, y + d[1] * 14, z + d[2] * 14)
    dx, dy, dz = front[0] - back[0], front[1] - back[1], front[2] - back[2]
    pitch = math.degrees(math.atan2(dz, math.hypot(dx, dy))) - 90.0
    C.spawn(CUBE, (x, y, z), (0.0, pitch, math.degrees(math.atan2(dy, dx))), (0.13, 0.11, 0.28), tag=TAG,
            label="Set_S2_SecurityCam", shadow=True, mat=M["Cream"])
    K.seg(front, (front[0] + d[0] * 4, front[1] + d[1] * 4, front[2] + d[2] * 4), 8.0, M["Black"])
    K.ball((front[0] + d[0] * 4.2, front[1] + d[1] * 4.2, front[2] + d[2] * 4.2), 6.0, M["DarkGlass"], dz=1.0)
    K.seg((back[0], back[1], back[2] + 7), (front[0] + d[0] * 6, front[1] + d[1] * 6, front[2] + d[2] * 6 + 7), 1.0,
          M["Black"], m=CUBE, w=15.0)                           # sun hood
    K.ball((front[0], front[1], front[2] + 5), 1.4, M["LedR"])
    K.seg((x, y, z + 6), (x, y, 432.0), 3.0, M["Steel"])         # drop rod to the terrace soffit
    K.seg((x, y, 430.0), (-3218.0, y, 430.0), 3.0, M["Steel"])   # arm bolted to the facade
    K.box((-3220.0, -3216.0, y - 8, y + 8, 422.0, 438.0), M["Steel"])
    K.rope(sag((x, y, 428.0), (-3219.0, y + 30, 400.0), 10, 5), 1.0, M["CableBlk"])


# --- run --------------------------------------------------------------------------------------------------------------
gone = C.clear(TAG)
materials()
body()
cab()
main_c = interior()
lilies()
mx = max_actor()
headset()
security_cam()
light("CabFill", (-5400.0, Y0 + 120.0, 205.0), 1.5, 220.0, (1.0, 0.85, 0.7))
light("Bulb", (-5425.0, 1715.0 + YOFF, ROOF - 16.0), 10.0, 320.0, (1.0, 0.72, 0.45), shadows=True, src=2.0)
K.ball((-5425.0, 1715.0 + YOFF, ROOF - 14.0), 5.0, M["Bulb"])
K.seg((-5425.0, 1715.0 + YOFF, ROOF - 7.5), (-5425.0, 1715.0 + YOFF, ROOF - 11.0), 2.0, M["Black"])
light("CRTGlow", (main_c[0] - 20.0, main_c[1], main_c[2]), 4.0, 200.0, (0.62, 0.85, 0.70), src=10.0)
n = K.flush()
sk = mx.skeletal_mesh_component
bones = {b: sk.get_socket_location(b) for b in ("pelvis", "head", "hand_r")}
near = sorted(set(l for l in C.near(CX, (Y0 + Y1) / 2, 450.0) if not l.startswith("Set_S2") and TAG not in l))
"s2 van: cleared %s, %d kit pieces; main CRT screen centre (%.0f, %.0f, %.0f); bones %s; near: %s; %s" % (
    gone, n, main_c[0], main_c[1], main_c[2],
    {k: (round(v.x), round(v.y), round(v.z)) for k, v in bones.items()}, near[:30], LOG)
