# Scene 1 dressing: EXT. THE GRAND LUMIERE THEATER - NIGHT (docs/SCRIPT.md). Rain on cobbles, neon in puddles, the
# Art Deco marquee. An overlay run after the level pipeline (tools/sets/*), idempotent: it first deletes what it placed
# (tag script_s1_dress), shows what it hid and puts back the floor materials it swapped. Nothing it places collides.
#   1. Renames the house: hides (visibility only) front.py's street-facing auction lettering (THE AURELIA board, the
#      fascia billing, the poster cases, the newsstand bill) and puts up THE GRAND LUMIERE: a stepped deco marquee on
#      the facade with a sunburst crown and chasing bulbs, a vertical LUMIERE blade, re-lettered fascia and posters.
#   2. Wet street: M_S1_WetStreet on the street and pavement floors: dark wet stone, near-mirror puddles with rain
#      ripples, and (the level has no SSR or Lumen) the signs' reflections computed in the material: each glowing
#      sign is a rectangle the reflected view ray is tested against. Three neon signs give the puddles colour.
#   3. Rain: a few big unlit translucent planes with procedural falling streaks (M_S1_Rain), no collision.
# Run under the editor lock:
#     "<engine python>" -I tools/ue_mcp.py py tools/script/scene1_dress.py 110
# Kept clear: the queue lane, the scan point (-3610,320), the gate (x -3520, y 210..430), the doorman, the podium
# (-3610,235), the carpet (y -150..150) and the doors: everything new is on the facade above head height, on the
# floor's material, or a NoCollision rain plane.
import importlib
import math
import sys

import unreal

sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C   # noqa: E402

importlib.reload(C)

TAG = "script_s1_dress"
S1 = "/Game/Take/Script/S1"
REBUILD = True                                   # rebuild the material graphs on every run
ON = dict(hide=True, marquee=True, blade=True, fascia=True, posters=True, news=True, neon=True, lights=True,
          wet=True, rain=True)
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
MP = unreal.MaterialProperty
CUBE, CYL, SPH, PLANE = ("/Engine/BasicShapes/" + n for n in ("Cube", "Cylinder", "Sphere", "Plane"))
FONT = "/Engine/EngineFonts/RobotoDistanceField"
NAME = "LUMI\u00c8RE"                             # LUMIERE if the font lacks the E grave (see font_has)

# --- the level's numbers (build_auction.py / front.py) ---------------------------------------------------------------
FRONT = -3220.0                                  # the street face of the front wall
TERRACE_X0 = FRONT - 230.0                       # the terrace's street edge; its fascia x -3460..-3450, z 395..445
PAVE_X0 = -4100.0                                # the kerb
STREET = (-6000.0, FRONT + 5.0, -2400.0, 2400.0)  # floors whose x range ends at the facade and overlap this
REPORT = []


def say(s):
    REPORT.append(s)


# --- assets -----------------------------------------------------------------------------------------------------------
def _new(name, cls, factory):
    path = S1 + "/" + name
    if eal.does_asset_exist(path):
        return eal.load_asset(path), False
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, S1, cls, factory), True


def _custom(m, code, ins, out=unreal.CustomMaterialOutputType.CMOT_FLOAT3, x=-300, y=0):
    cu = mel.create_material_expression(m, unreal.MaterialExpressionCustom, x, y)
    cu.set_editor_property("code", code)
    cu.set_editor_property("output_type", out)
    lst = []
    for nm in ins:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", nm)
        lst.append(ci)
    cu.set_editor_property("inputs", lst)
    return cu


def _param(m, name, v, x=-900, y=0):
    if isinstance(v, tuple):
        e = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, x, y)
        e.set_editor_property("default_value", unreal.LinearColor(v[0], v[1], v[2], 1.0))
    else:
        e = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, x, y)
        e.set_editor_property("default_value", float(v))
    e.set_editor_property("parameter_name", name)
    return e


def _node(m, cls, x=-900, y=0):
    return mel.create_material_expression(m, cls, x, y)


def _wire(src, dst, pin, out=""):
    mel.connect_material_expressions(src, out, dst, pin)


def material(name, build):
    m, made = _new(name, unreal.Material, unreal.MaterialFactoryNew())
    if made or REBUILD:
        mel.delete_all_material_expressions(m)
        build(m)
        mel.recompile_material(m)
        eal.save_asset(S1 + "/" + name, only_if_is_dirty=False)
    return m


def instance(name, parent, scalars=None, vectors=None):
    f = unreal.MaterialInstanceConstantFactoryNew()
    mi, made = _new(name, unreal.MaterialInstanceConstant, f)
    mel.set_material_instance_parent(mi, parent)
    for k, v in (scalars or {}).items():
        mel.set_material_instance_scalar_parameter_value(mi, k, float(v))
    for k, v in (vectors or {}).items():
        mel.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
    mel.update_material_instance(mi)
    eal.save_asset(S1 + "/" + name, only_if_is_dirty=False)
    return mi


# Bulbs: unlit, a three-phase chase. Bulbs sit on a 26 cm grid in (y + z), so round((y + z) / 26) is one number per
# bulb and the lit third steps from bulb to bulb (no per-instance data needed).
BULB_CODE = r"""
float idx = floor((WP.y + WP.z) / 26.0 + 0.5);
float ph = frac(Time * Speed - idx / 3.0);
float on = ph < 0.34 ? 1.0 : 0.0;
return Color.rgb * Glow * (Base + (1.0 - Base) * on);
"""


def build_bulb(m):
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("used_with_instanced_static_meshes", True)
    cu = _custom(m, BULB_CODE, ("WP", "Time", "Color", "Glow", "Speed", "Base"))
    for (nm, src) in (("WP", _node(m, unreal.MaterialExpressionWorldPosition, y=-200)),
                      ("Time", _node(m, unreal.MaterialExpressionTime, y=-100)),
                      ("Color", _param(m, "Color", (1.0, 0.72, 0.38), y=0)), ("Glow", _param(m, "Glow", 9.0, y=100)),
                      ("Speed", _param(m, "Speed", 2.2, y=200)), ("Base", _param(m, "Base", 0.3, y=300))):
        _wire(src, cu, nm)
    mel.connect_material_property(cu, "", MP.MP_EMISSIVE_COLOR)


NEON_CODE = r"""
float f = frac(sin(floor(Time * 11.0) * 91.7) * 43758.5453);
float k = 1.0 - Flicker * (f > 0.86 ? 0.85 : 0.0);
return Color.rgb * Glow * k;
"""


def build_neon(m):
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("used_with_instanced_static_meshes", True)
    cu = _custom(m, NEON_CODE, ("Time", "Color", "Glow", "Flicker"))
    for (nm, src) in (("Time", _node(m, unreal.MaterialExpressionTime, y=-100)),
                      ("Color", _param(m, "Color", (1.0, 0.1, 0.1), y=0)), ("Glow", _param(m, "Glow", 12.0, y=100)),
                      ("Flicker", _param(m, "Flicker", 0.0, y=200))):
        _wire(src, cu, nm)
    mel.connect_material_property(cu, "", MP.MP_EMISSIVE_COLOR)


TEXT_CODE = r"""
float k = 1.0 - Flicker * (frac(sin(floor(Time * 11.0) * 91.7) * 43758.5453) > 0.86 ? 0.85 : 0.0);
return VC.rgb * Glow * k * saturate(Mask * 1.6);
"""


def build_text(m):
    """Lettering that glows: unlit, masked on the distance-field font, emissive = text colour x Glow."""
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("opacity_mask_clip_value", 0.5)
    m.set_editor_property("two_sided", True)
    font = _node(m, unreal.MaterialExpressionFontSampleParameter, y=300)
    font.set_editor_property("parameter_name", "Font")
    font.set_editor_property("font", eal.load_asset(FONT))
    font.set_editor_property("font_texture_page", 0)
    cu = _custom(m, TEXT_CODE, ("VC", "Glow", "Mask", "Time", "Flicker"))
    _wire(_node(m, unreal.MaterialExpressionVertexColor, y=-200), cu, "VC")
    _wire(_param(m, "Glow", 5.0, y=0), cu, "Glow")
    _wire(font, cu, "Mask", "A")
    _wire(_node(m, unreal.MaterialExpressionTime, y=100), cu, "Time")
    _wire(_param(m, "Flicker", 0.0, y=200), cu, "Flicker")
    mel.connect_material_property(cu, "", MP.MP_EMISSIVE_COLOR)
    mel.connect_material_property(font, "A", MP.MP_OPACITY_MASK)


# Rain: sparse 4 cm columns, each with streaks falling at its own speed; faded close to the camera (no giant streaks).
RAIN_CODE = r"""
float hc = WP.x * 0.83 + WP.y * 0.56 + WP.z * 0.07;
float c = floor(hc / 4.0);
float h1 = frac(sin(c * 12.9898) * 43758.5453);
float h2 = frac(sin(c * 78.233) * 24634.6345);
float h3 = frac(sin(c * 39.3468) * 11743.2318);
if (h3 > Density) return 0.0;
float fx = frac(hc / 4.0) - 0.5;
float lineM = saturate(1.0 - abs(fx) * 3.2);
float period = 380.0 + 320.0 * h1;
float lenF = (45.0 + 55.0 * h2) / period;
float y = frac((WP.z + Time * Speed * (0.85 + 0.3 * h2)) / period + h1 * 5.0);
float seg = saturate(1.0 - abs(y - lenF * 0.5) / (lenF * 0.5));
float nfade = saturate((length(WP - Cam) - 120.0) / 250.0);
return lineM * seg * nfade * Alpha;
"""


def build_rain(m):
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    cu = _custom(m, RAIN_CODE, ("WP", "Time", "Cam", "Speed", "Density", "Alpha"),
                 out=unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    for (nm, src) in (("WP", _node(m, unreal.MaterialExpressionWorldPosition, y=-200)),
                      ("Time", _node(m, unreal.MaterialExpressionTime, y=-100)),
                      ("Cam", _node(m, unreal.MaterialExpressionCameraPositionWS, y=0)),
                      ("Speed", _param(m, "Speed", 1100.0, y=100)), ("Density", _param(m, "Density", 0.35, y=200)),
                      ("Alpha", _param(m, "Alpha", 0.35, y=300))):
        _wire(src, cu, nm)
    col = _param(m, "Color", (0.55, 0.62, 0.75), x=-500, y=400)
    mul = _node(m, unreal.MaterialExpressionMultiply, x=-150, y=300)
    _wire(col, mul, "A")
    _wire(cu, mul, "B")
    mel.connect_material_property(mul, "", MP.MP_EMISSIVE_COLOR)
    mel.connect_material_property(cu, "", MP.MP_OPACITY)


# The wet street. Surf: float4(normal x, normal y, puddle, grout) from world xy: running-bond stones of Cell (cm) with
# bevelled edges and a tilt per stone, two-octave value-noise puddles, ripple rings in the puddles.
SURF_CODE = r"""
float2 p = WP.xy;
float2 cell = float2(CellX, CellY);
float2 q = p / cell;
q.x += frac(floor(q.y) * 0.5);
float2 id = floor(q);
float2 f = frac(q);
float2 e = min(f, 1.0 - f) * cell;
float grout = 1.0 - smoothstep(0.4, 1.4, min(e.x, e.y));
float2 n = 0;
n.x = (f.x < 0.5 ? -1.0 : 1.0) * (1.0 - smoothstep(0.4, 3.0, e.x));
n.y = (f.y < 0.5 ? -1.0 : 1.0) * (1.0 - smoothstep(0.4, 3.0, e.y));
n *= 0.45;
float hs = frac(sin(dot(id, float2(127.1, 311.7))) * 43758.5453);
float hs2 = frac(sin(dot(id, float2(269.5, 183.3))) * 43758.5453);
n += (float2(hs, hs2) - 0.5) * 0.10;
float nz = 0;
float amp = 0.65;
float2 pp = p / 280.0 + Seed;
for (int o = 0; o < 2; o++) {
    float2 i0 = floor(pp);
    float2 g = frac(pp);
    g = g * g * (3.0 - 2.0 * g);
    float a = frac(sin(dot(i0, float2(127.1, 311.7))) * 43758.5453);
    float b = frac(sin(dot(i0 + float2(1, 0), float2(127.1, 311.7))) * 43758.5453);
    float c = frac(sin(dot(i0 + float2(0, 1), float2(127.1, 311.7))) * 43758.5453);
    float d = frac(sin(dot(i0 + float2(1, 1), float2(127.1, 311.7))) * 43758.5453);
    nz += amp * lerp(lerp(a, b, g.x), lerp(c, d, g.x), g.y);
    amp = 0.35;
    pp = pp * 2.7 + 13.1;
}
float puddle = smoothstep(1.0 - Puddles, 1.0 - Puddles + 0.05, nz);
float2 rc = p / 22.0;
float2 rid = floor(rc);
float rh = frac(sin(dot(rid, float2(41.3, 289.1))) * 43758.5453);
float rh2 = frac(sin(dot(rid, float2(17.7, 113.9))) * 24634.6345);
float2 dv = (frac(rc) - (0.25 + 0.5 * float2(rh, rh2))) * 22.0;
float dist = length(dv);
float ph = frac(Time * 0.9 + rh * 3.1);
float x = (dist - ph * 9.0) / 0.9;
float ring = 2.0 * x * exp(-x * x) * (1.0 - ph);
float2 rn = dist > 0.01 ? dv / dist : float2(0, 0);
n = n * (1.0 - puddle) + rn * ring * Ripple * puddle;
return float4(n.x, n.y, puddle, grout);
"""
BASE_CODE = r"""
float3 c = lerp(Tint.rgb, Tint.rgb * 0.35, S.w);
return lerp(c, float3(0.010, 0.011, 0.014), S.z);
"""
ROUGH_CODE = "return lerp(lerp(0.30, 0.55, S.w), 0.035, S.z);"
NORMAL_CODE = r"""
float3 n = normalize(float3(S.x, S.y, 1.0));
return normalize(lerp(VN, n, saturate(VN.z)));
"""
# The signs as rectangles on planes x = const (the blade too, as a thin strip): A = (x, y0, y1, z0), B = (z1, rgb).
# The reflected view ray is cut with each plane; glow falls off with distance from the rectangle (blurrier off the
# puddles); the terrace slab (x -3455..-3220, |y| < 620, z 430) hides what is above it from the floor beneath.
REFL_CODE = r"""
float3 N = normalize(float3(S.x, S.y, 1.0));
float3 V = normalize(WP - Cam);
float3 R = reflect(V, N);
if (R.z <= 0.002 || R.x <= 0.002) return float3(0, 0, 0);
float soft = lerp(55.0, 7.0, S.z);
float amp = lerp(0.10, 1.0, S.z);
float fres = 0.3 + 0.7 * pow(1.0 - saturate(dot(-V, N)), 3.0);
float4 A[RN] = { RA };
float4 B[RN] = { RB };
float tT = (430.0 - WP.z) / R.z;
float3 HT = WP + R * tT;
bool under = HT.x > -3455.0 && HT.x < -3220.0 && abs(HT.y) < 620.0;
float3 acc = float3(0, 0, 0);
for (int i = 0; i < RN; i++) {
    float t = (A[i].x - WP.x) / R.x;
    if (t <= 0.0) continue;
    if (under && tT < t && A[i].w > 440.0) continue;
    float3 H = WP + R * t;
    float dy = max(max(A[i].y - H.y, H.y - A[i].z), 0.0);
    float dz = max(max(A[i].w - H.z, H.z - B[i].x), 0.0);
    float d2 = dy * dy + dz * dz;
    acc += B[i].yzw * exp(-d2 / (soft * soft));
}
return acc * amp * fres * Gain;
"""


def build_wet(m, rects):
    m.set_editor_property("tangent_space_normal", False)
    m.set_editor_property("used_with_instanced_static_meshes", True)
    surf = _custom(m, SURF_CODE, ("WP", "Time", "CellX", "CellY", "Puddles", "Ripple", "Seed"),
                   out=unreal.CustomMaterialOutputType.CMOT_FLOAT4, x=-600, y=0)
    wp = _node(m, unreal.MaterialExpressionWorldPosition, x=-1000, y=-300)
    tm = _node(m, unreal.MaterialExpressionTime, x=-1000, y=-200)
    for (nm, src) in (("WP", wp), ("Time", tm), ("CellX", _param(m, "CellX", 16.0, x=-1000, y=-100)),
                      ("CellY", _param(m, "CellY", 11.0, x=-1000, y=0)),
                      ("Puddles", _param(m, "Puddles", 0.38, x=-1000, y=100)),
                      ("Ripple", _param(m, "Ripple", 0.9, x=-1000, y=200)),
                      ("Seed", _param(m, "Seed", 0.0, x=-1000, y=300))):
        _wire(src, surf, nm)
    base = _custom(m, BASE_CODE, ("S", "Tint"), x=-250, y=-300)
    _wire(surf, base, "S")
    _wire(_param(m, "Tint", (0.040, 0.040, 0.046), x=-600, y=-350), base, "Tint")
    mel.connect_material_property(base, "", MP.MP_BASE_COLOR)
    rough = _custom(m, ROUGH_CODE, ("S",), out=unreal.CustomMaterialOutputType.CMOT_FLOAT1, x=-250, y=-150)
    _wire(surf, rough, "S")
    mel.connect_material_property(rough, "", MP.MP_ROUGHNESS)
    nrm = _custom(m, NORMAL_CODE, ("S", "VN"), x=-250, y=0)
    _wire(surf, nrm, "S")
    _wire(_node(m, unreal.MaterialExpressionVertexNormalWS, x=-600, y=250), nrm, "VN")
    mel.connect_material_property(nrm, "", MP.MP_NORMAL)
    ra = ", ".join("float4(%.1f, %.1f, %.1f, %.1f)" % (r[0], r[1], r[2], r[3]) for r in rects)
    rb = ", ".join("float4(%.1f, %.3f, %.3f, %.3f)" % (r[4], r[5][0], r[5][1], r[5][2]) for r in rects)
    code = REFL_CODE.replace("RN", str(len(rects))).replace("RA", ra).replace("RB", rb)
    refl = _custom(m, code, ("S", "WP", "Cam", "Gain"), x=-250, y=200)
    _wire(surf, refl, "S")
    _wire(wp, refl, "WP")
    _wire(_node(m, unreal.MaterialExpressionCameraPositionWS, x=-600, y=400), refl, "Cam")
    _wire(_param(m, "Gain", 2.5, x=-600, y=500), refl, "Gain")
    mel.connect_material_property(refl, "", MP.MP_EMISSIVE_COLOR)


# --- colours ----------------------------------------------------------------------------------------------------------
WARM = (1.0, 0.72, 0.38)
RED, PINK, CYAN = (1.0, 0.06, 0.08), (1.0, 0.18, 0.62), (0.10, 0.78, 1.0)
# The glowing rectangles for the puddles (x, y0, y1, z0, z1, rgb x strength): the marquee, its crown, the blade, the
# fascia, the three neon signs.
RECTS = [(-3262.0, -760.0, 760.0, 690.0, 890.0, (1.0, 0.66, 0.30)),
         (-3250.0, -260.0, 260.0, 905.0, 1180.0, (0.55, 0.36, 0.16)),
         (-3285.0, -1012.0, -988.0, 480.0, 1150.0, (1.6, 1.05, 0.5)),
         (-3463.0, -600.0, 600.0, 385.0, 455.0, (0.9, 0.6, 0.28)),
         (-3226.0, -1325.0, -1175.0, 278.0, 362.0, (2.4, 0.15, 0.18)),
         (-3226.0, -1320.0, -1180.0, 228.0, 262.0, (1.6, 0.3, 1.0)),
         (-3226.0, 1160.0, 1340.0, 600.0, 680.0, (0.25, 1.6, 2.4))]


# --- clear ------------------------------------------------------------------------------------------------------------
def unswap():
    """Puts back the floor materials a previous run swapped (kept in the actor's tags as slot=path)."""
    n = 0
    for a in C.actors():
        ts = C.tags(a)
        mt = [t for t in ts if t.startswith(TAG + ":mat=")]
        if not mt:
            continue
        for it in mt[0].split("=", 1)[1].split("|"):
            ci, si, path = it.split(",", 2)
            comps = a.get_components_by_class(unreal.StaticMeshComponent)
            if int(ci) < len(comps):
                comps[int(ci)].set_material(int(si), eal.load_asset(path) if path != "-" else None)
        C._set_tags(a, [t for t in ts if not t.startswith(TAG + ":mat=")])
        n += 1
    return n


# --- placing ----------------------------------------------------------------------------------------------------------
class Kit:
    """Basic shapes gathered per (mesh, material) and laid down as one instanced actor each. All NoCollision."""

    def __init__(self):
        self.g = {}

    def add(self, m, mt, loc, r, sc):
        self.g.setdefault((m, mt), []).append((tuple(loc), r, tuple(sc)))

    def box(self, b, mt):
        self.add(CUBE, mt, ((b[0] + b[1]) / 2, (b[2] + b[3]) / 2, (b[4] + b[5]) / 2), (0.0, 0.0, 0.0),
                 ((b[1] - b[0]) / 100.0, (b[3] - b[2]) / 100.0, (b[5] - b[4]) / 100.0))

    def ball(self, c, d, mt):
        self.add(SPH, mt, c, (0.0, 0.0, 0.0), (d / 100.0, d / 100.0, d / 100.0))

    def seg(self, p, q, d, mt, m=CUBE, extend=0.0):
        """A bar (cube) or rod (cylinder) from p to q, d across (front.py's seg)."""
        dx, dy, dz = q[0] - p[0], q[1] - p[1], q[2] - p[2]
        ln = math.sqrt(dx * dx + dy * dy + dz * dz)
        pitch = math.degrees(math.atan2(dz, math.hypot(dx, dy))) - 90.0
        self.add(m, mt, ((p[0] + q[0]) / 2, (p[1] + q[1]) / 2, (p[2] + q[2]) / 2),
                 (0.0, pitch, math.degrees(math.atan2(dy, dx))), (d / 100.0, d / 100.0, (ln + extend) / 100.0))

    def disc(self, x0, x1, y, z, d, mt):
        """A disc facing the street (a cylinder along x)."""
        self.seg((x0, y, z), (x1, y, z), d, mt, m=CYL)

    def flush(self):
        n = 0
        for (m, mt), items in self.g.items():
            if len(items) == 1:
                C.spawn(m, items[0][0], items[0][1], items[0][2], tag=TAG, collide=False, shadow=False, mat=mt)
            else:
                C.scatter(m, items, tag=TAG, collide=False, shadow=False, mat=mt)
            n += len(items)
        self.g = {}
        return n


K = Kit()
NTEXT = [0]


def text(t, x, y, z, yaw, size, rgb, mt=None):
    """Words in the world centred on (x, y, z), readable from the yaw side (180: from the street)."""
    a = C.eas.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(x, y, z), unreal.Rotator(0.0, 0.0, yaw))
    a.set_actor_label("Set_S1_Text")
    a.set_folder_path("Sets/" + TAG)
    c = a.text_render
    c.set_editor_property("text", t)
    for (k, v) in (("world_size", float(size)),
                   ("text_render_color", unreal.Color(r=int(rgb[0] * 255), g=int(rgb[1] * 255), b=int(rgb[2] * 255),
                                                      a=255)),
                   ("horizontal_alignment", unreal.HorizTextAligment.EHTA_CENTER),
                   ("vertical_alignment", unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)):
        try:
            c.set_editor_property(k, v)
        except Exception:                                       # noqa: BLE001
            pass
    if mt is not None:
        c.set_editor_property("text_material", mt)
    try:
        c.set_collision_profile_name("NoCollision")
        c.set_editor_property("cast_shadow", False)
    except Exception:                                           # noqa: BLE001
        pass
    a.set_editor_property("tags", [unreal.Name(TAG)])
    NTEXT[0] += 1
    return a


def font_has(ch):
    """True if the distance-field font has the character (an un-remapped offline font indexes by code point)."""
    try:
        f = eal.load_asset(FONT)
        chars = f.get_editor_property("characters")
        remapped = f.get_editor_property("is_remapped")
        return (not remapped) and len(chars) > ord(ch)
    except Exception:                                           # noqa: BLE001
        return False


def snap(y, z):
    """z moved down onto the bulb grid: (y + z) a multiple of 26 (see BULB_CODE)."""
    return z - ((y + z) % 26.0)


def hide_texts(pred):
    """Hides (visibility only) the TextRenderActors pred(label, text, x, y, z) picks; clear() shows them again."""
    out = []
    for a in C.actors():
        if not isinstance(a, unreal.TextRenderActor) or TAG in C.tags(a):
            continue
        v = a.get_actor_location()
        t = str(a.text_render.get_editor_property("text"))
        if not pred(a.get_actor_label(), t, v.x, v.y, v.z):
            continue
        comps = [c for c in a.get_components_by_class(unreal.PrimitiveComponent) if c.is_visible()]
        if not comps:
            continue
        for c in comps:
            c.set_visibility(False, False)
        C._set_tags(a, C.tags(a) + ["%s:hid=%s" % (TAG, "|".join(c.get_name() for c in comps))])
        out.append(t)
    return out


AUCTION_WORDS = ("AURELIA", "AUCTION", "DIRECTOR'S CUT", "ONE PRINT", "SOLD TONIGHT")


def street_auction_text(label, t, x, y, z):
    if x > FRONT + 5.0:                                          # the lobby and beyond: not Scene 1's
        return False
    if abs(x - (FRONT - 35.5)) < 4.0 and 1030.0 < abs(y) < 1570.0 and z < 210.0:   # every line of the poster cases
        return True
    return any(w in t.upper() for w in AUCTION_WORDS)


def floors():
    """The street's and pavement's floor slabs: flat, at ground level, west of the facade."""
    out = []
    for a in C.actors():
        if not isinstance(a, unreal.StaticMeshActor) or TAG in C.tags(a):
            continue
        o, e = a.get_actor_bounds(False)
        if not (STREET[0] < o.x + e.x <= STREET[1]):
            continue
        if not (o.y - e.y < STREET[3] and o.y + e.y > STREET[2]):
            continue
        if e.z > 30.0 or o.z + e.z > 25.0 or o.z + e.z < -30.0 or e.x < 100.0 or e.y < 100.0:
            continue
        lab = a.get_actor_label()
        if any(k in lab.lower() for k in ("carpet", "runner", "rug", "mat_")):
            continue
        out.append(a)
    return out


def swap(a, mi):
    items = []
    for ci, c in enumerate(a.get_components_by_class(unreal.StaticMeshComponent)):
        for si in range(c.get_num_materials()):
            old = c.get_material(si)
            items.append("%d,%d,%s" % (ci, si, old.get_path_name() if old else "-"))
            c.set_material(si, mi)
    C._set_tags(a, C.tags(a) + ["%s:mat=%s" % (TAG, "|".join(items))])


# ======================================================================================================================
gone, shown = C.clear(TAG)
put_back = unswap()
say("cleared: %d removed, %d shown, %d floors put back" % (gone, shown, put_back))
if not eal.does_directory_exist(S1):
    eal.make_directory(S1)

M_BULB = material("M_S1_Bulb", build_bulb)
M_NEON = material("M_S1_Neon", build_neon)
M_TEXT = material("M_S1_TextGlow", build_text)
M_RAIN = material("M_S1_Rain", build_rain)
M_WET = material("M_S1_WetStreet", lambda m: build_wet(m, RECTS))
BULB = instance("MI_S1_Bulb_Warm", M_BULB, {"Glow": 9.0, "Speed": 2.2, "Base": 0.3}, {"Color": WARM})
BULB_STILL = instance("MI_S1_Bulb_Still", M_BULB, {"Glow": 7.0, "Speed": 0.0, "Base": 1.0}, {"Color": WARM})
N_RED = instance("MI_S1_Neon_Red", M_NEON, {"Glow": 14.0}, {"Color": RED})
N_PINK = instance("MI_S1_Neon_Pink", M_NEON, {"Glow": 10.0, "Flicker": 1.0}, {"Color": PINK})
N_CYAN = instance("MI_S1_Neon_Cyan", M_NEON, {"Glow": 12.0}, {"Color": CYAN})
T_MARQ = instance("MI_S1_Text_Marquee", M_TEXT, {"Glow": 6.0})
T_SOFT = instance("MI_S1_Text_Fascia", M_TEXT, {"Glow": 2.5})
T_NEON = instance("MI_S1_Text_Neon", M_TEXT, {"Glow": 14.0})
T_FLICK = instance("MI_S1_Text_NeonFlicker", M_TEXT, {"Glow": 10.0, "Flicker": 1.0})
WET_ROAD = instance("MI_S1_WetCobble", M_WET, {"CellX": 16.0, "CellY": 11.0, "Puddles": 0.40, "Seed": 0.0},
                    {"Tint": (0.034, 0.034, 0.040)})
WET_PAVE = instance("MI_S1_WetPavement", M_WET, {"CellX": 62.0, "CellY": 46.0, "Puddles": 0.33, "Seed": 7.3},
                    {"Tint": (0.060, 0.058, 0.062)})
RAIN = instance("MI_S1_Rain", M_RAIN, {"Speed": 1100.0, "Density": 0.35, "Alpha": 0.35})


def front_mat(n):
    for d in ("/Game/Take/Dress/", "/Game/Take/Materials/"):
        if eal.does_asset_exist(d + n):
            return eal.load_asset(d + n)
    return None


GOLD = front_mat("MI_Front_Trim")
LACQUER = front_mat("MI_Front_Canopy")
name = NAME if font_has(NAME[4]) else "LUMIERE"
say("font: %s" % ("E grave available" if name == NAME else "no E grave: LUMIERE"))

# --- 1. rename the house -----------------------------------------------------------------------------------------------
if ON["hide"]:
    hid = hide_texts(street_auction_text)
    say("hid %d auction texts: %s" % (len(hid), "; ".join(sorted(set(h[:24] for h in hid)))))

if ON["marquee"]:
    # The board: a lacquer panel over the old AURELIA board, gold frame, a ziggurat of steps, a sunburst crown.
    X = -3262.0                                                   # the board's street face
    K.box((X, -3240.0, -760.0, 760.0, 690.0, 890.0), LACQUER)
    for b in ((X - 6, -3240.0, -790.0, 790.0, 890.0, 905.0), (X - 6, -3240.0, -790.0, 790.0, 675.0, 690.0),
              (X - 6, -3240.0, -790.0, -760.0, 690.0, 890.0), (X - 6, -3240.0, 760.0, 790.0, 690.0, 890.0)):
        K.box(b, GOLD)
    for s in (-1.0, 1.0):                                         # stepped deco pilasters either side
        y0, y1 = sorted((s * 790.0, s * 840.0))
        K.box((X - 10, -3236.0, y0, y1, 610.0, 960.0), GOLD)
        K.box((X - 6, -3236.0, y0 + 6, y1 - 6, 960.0, 995.0), GOLD)
        K.box((X - 3, -3236.0, y0 + 14, y1 - 14, 995.0, 1030.0), GOLD)
        K.ball(((X + -3236.0) / 2, s * 815.0, 1045.0), 22.0, BULB_STILL)
        for z in range(624, 950, 26):                             # a bulb strip down each pilaster's face
            K.ball((X - 13.0, s * 815.0, snap(s * 815.0, float(z))), 7.0, BULB)
    # the crown: a dark half-disc, gold rays, a gold half-sun on the top bar, bulbs round the rim
    K.disc(-3240.0, -3236.0, 0.0, 905.0, 560.0, LACQUER)
    rays = 15
    for i in range(rays):
        a = math.radians(6.0 + i * 168.0 / (rays - 1))
        r0, r1 = 70.0, (265.0 if i % 2 == 0 else 215.0)
        K.seg((-3248.0, r0 * math.cos(a), 905.0 + r0 * math.sin(a)),
              (-3248.0, r1 * math.cos(a), 905.0 + r1 * math.sin(a)), 9.0 if i % 2 == 0 else 6.0, GOLD)
    K.disc(-3256.0, -3249.0, 0.0, 905.0, 120.0, GOLD)
    for i in range(19):
        a = math.radians(5.0 + i * 170.0 / 18)
        K.ball((-3252.0, 285.0 * math.cos(a), 905.0 + 285.0 * math.sin(a)), 8.0, BULB)
    # chasing bulbs round the board on the 26 cm grid (y + z = 26k)
    for z in (702.0, 858.0):                                      # 27 x 26, 33 x 26
        for k in range(-28, 29):
            K.ball((X - 4.0, k * 26.0, z), 7.0, BULB)
    for yy in (-28 * 26.0, 28 * 26.0):
        for z in (728.0, 754.0, 780.0, 806.0, 832.0):
            K.ball((X - 4.0, yy, z), 7.0, BULB)
    text("THE GRAND", X - 2.0, 0.0, 834.0, 180.0, 30.0, (1.0, 0.86, 0.6), T_MARQ)
    text(name, X - 2.0, 0.0, 770.0, 180.0, 92.0, (1.0, 0.80, 0.48), T_MARQ)
    text("CIN\u00c9MA  \u00b7  EST. 1927" if name == NAME else "CINEMA  \u00b7  EST. 1927",
         X - 2.0, 0.0, 717.0, 180.0, 13.0, (0.9, 0.7, 0.45), T_SOFT)

if ON["blade"]:
    # A vertical blade sign off the facade's south end: LUMIERE stacked, gold edges, bulbs down its street edge.
    BY, BX0, BX1 = -1000.0, -3340.0, -3222.0
    K.box((BX0, BX1, BY - 11.0, BY + 11.0, 480.0, 1150.0), LACQUER)
    for b in ((BX0 - 5, BX0, BY - 13, BY + 13, 470.0, 1160.0), (BX0, BX1, BY - 13, BY + 13, 1150.0, 1160.0),
              (BX0, BX1, BY - 13, BY + 13, 470.0, 480.0)):
        K.box(b, GOLD)
    for (dz, w) in ((1160.0, 100.0), (1185.0, 70.0), (1210.0, 40.0)):       # the stepped top
        K.box((BX0 + (118 - w) / 2, BX0 + (118 - w) / 2 + w, BY - 9, BY + 9, dz, dz + 25.0), GOLD)
    K.ball(((BX0 + BX1) / 2, BY, 1248.0), 20.0, BULB_STILL)
    for z in range(494, 1150, 26):
        K.ball((BX0 - 7.0, BY, snap(BY, float(z))), 7.0, BULB)
    letters = list(name)
    for i, ch in enumerate(letters):
        z = 1085.0 - i * 88.0
        for (side, yaw) in ((BY + 12.5, 90.0), (BY - 12.5, -90.0)):
            text(ch, (BX0 + BX1) / 2, side, z, yaw, 74.0, (1.0, 0.82, 0.5), T_MARQ)

if ON["fascia"]:
    text("TONIGHT  \u00b7  LE SOLEIL NOIR (1974)  \u00b7  PRIVATE SCREENING", TERRACE_X0 - 14.0, 0.0, 420.0, 180.0, 23.0,
         (1.0, 0.82, 0.55), T_SOFT)

if ON["posters"]:
    BLACK = front_mat("MI_Front_Canopy")
    for y in (1100.0, 1500.0):
        px = FRONT - 35.0
        K.disc(px + 0.6, px + 0.3, y, 132.0, 50.0, GOLD)                    # the black sun, a gold corona
        K.disc(px + 0.3, px, y, 132.0, 42.0, BLACK)
        for (t, s, z, rgb) in (("ONE NIGHT ONLY", 5.5, 165.0, (0.85, 0.66, 0.36)),
                               ("LE SOLEIL", 13.0, 150.0, (0.36, 0.07, 0.08)),
                               ("NOIR", 22.0, 112.0, (0.36, 0.07, 0.08)),
                               ("1974", 8.0, 95.0, (0.30, 0.06, 0.07)),
                               ("THE LOST PRINT", 5.5, 85.0, (0.20, 0.17, 0.14)),
                               ("PRIVATE SCREENING", 5.0, 77.0, (0.20, 0.17, 0.14))):
            text(t, px - 0.8, y, z, 180.0, s, rgb)

if ON["news"]:
    nx, ny = -3560.0, -1500.0                                     # front.py's newsstand bill (its north side)
    for (t, s, z) in (("SCREENS TONIGHT", 5.5, 60.0), ("AT THE LUMIERE", 5.0, 50.0)):
        text(t, nx, ny + 85.0, z, 90.0, s, (0.10, 0.09, 0.08))

if ON["neon"]:
    def tube_rect(x, y0, y1, z0, z1, mt, d=3.0):
        for (p, q) in (((x, y0, z0), (x, y1, z0)), ((x, y0, z1), (x, y1, z1)),
                       ((x, y0, z0), (x, y0, z1)), ((x, y1, z0), (x, y1, z1))):
            K.seg(p, q, d, mt, m=CYL, extend=d)
    fx = FRONT - 6.0
    K.box((fx - 2.0, FRONT, -1340.0, -1160.0, 220.0, 372.0), LACQUER)    # a backing plate for BAR / COCKTAILS
    tube_rect(fx - 4.0, -1325.0, -1175.0, 278.0, 362.0, N_RED)
    text("BAR", fx - 5.0, -1250.0, 320.0, 180.0, 58.0, (1.0, 0.1, 0.1), T_NEON)
    text("cocktails", fx - 5.0, -1250.0, 245.0, 180.0, 30.0, (1.0, 0.2, 0.65), T_FLICK)
    K.box((fx - 2.0, FRONT, 1150.0, 1350.0, 590.0, 690.0), LACQUER)
    tube_rect(fx - 4.0, 1160.0, 1340.0, 600.0, 680.0, N_CYAN)
    text("HOTEL", fx - 5.0, 1250.0, 640.0, 180.0, 52.0, (0.1, 0.8, 1.0), T_NEON)
    K.seg((fx - 4.0, 1170.0, 586.0), (fx - 4.0, 1330.0, 586.0), 3.0, N_PINK, m=CYL)

placed = K.flush()
say("placed %d shapes, %d texts" % (placed, NTEXT[0]))

if ON["lights"]:
    for (lab, loc, rgb, inten, rad) in (("BarNeon", (-3300.0, -1250.0, 320.0), RED, 120.0, 600.0),
                                        ("HotelNeon", (-3300.0, 1250.0, 620.0), CYAN, 120.0, 750.0),
                                        ("Marquee", (-3330.0, 0.0, 780.0), WARM, 200.0, 800.0)):
        a = C.eas.spawn_actor_from_class(unreal.PointLight, unreal.Vector(*loc))
        a.set_actor_label("Set_S1_Light_" + lab)
        a.set_folder_path("Sets/" + TAG)
        lc = a.get_component_by_class(unreal.PointLightComponent)
        lc.set_mobility(unreal.ComponentMobility.MOVABLE)
        for (k, v) in (("intensity_units", unreal.LightUnits.CANDELAS), ("intensity", inten), ("attenuation_radius", rad), ("cast_shadows", False),
                       ("light_color", unreal.Color(r=int(rgb[0] * 255), g=int(rgb[1] * 255), b=int(rgb[2] * 255),
                                                    a=255)), ("source_radius", 20.0)):
            try:
                lc.set_editor_property(k, v)
            except Exception as e:                              # noqa: BLE001
                say("light %s: %s" % (k, e))
        a.set_editor_property("tags", [unreal.Name(TAG)])
    say("lights: 3 point lights, no shadows")

# --- 2. the wet street ----------------------------------------------------------------------------------------------------
if ON["wet"]:
    fl = floors()
    for a in fl:
        o, e = a.get_actor_bounds(False)
        swap(a, WET_PAVE if o.x - e.x >= PAVE_X0 - 30.0 else WET_ROAD)
    say("wet floors: %s" % ", ".join("%s(x%.0f..%.0f)" % (a.get_actor_label(), a.get_actor_bounds(False)[0].x -
                                                          a.get_actor_bounds(False)[1].x,
                                                          a.get_actor_bounds(False)[0].x +
                                                          a.get_actor_bounds(False)[1].x) for a in fl))

# --- 3. rain ----------------------------------------------------------------------------------------------------------
if ON["rain"]:
    n = 0
    for x in (-4800.0, -4520.0, -4280.0, -4030.0, -3790.0, -3560.0):   # facing the street (planes in y-z)
        C.spawn(PLANE, (x, 0.0, 560.0), (90.0, 0.0, 90.0), (36.0, 11.2, 1.0), tag=TAG, label="S1_Rain_%d" % n,
                collide=False, shadow=False, mat=RAIN)
        n += 1
    for y in (-700.0, 260.0, 1150.0):                            # along the street (planes in x-z)
        C.spawn(PLANE, (-4130.0, y, 560.0), (90.0, 0.0, 0.0), (13.0, 11.2, 1.0), tag=TAG, label="S1_Rain_%d" % n,
                collide=False, shadow=False, mat=RAIN)
        n += 1
    say("rain: %d planes" % n)

print("\n".join(REPORT))
