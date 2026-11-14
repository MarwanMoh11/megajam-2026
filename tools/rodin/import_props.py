# Phase 2 of the Rodin props (docs/ART_STYLE.md §12, §15.9, §15.10): import the GLBs from C:\Jam\Art\Rodin into
# /Game/Take/Props, give them the set's values, and dress /Game/Take/Lvl_Auction with them. Runs inside the editor on
# the open map, AFTER tools/build_auction.py and tools/dress_auction.py (a re-run of either brings the boxes back and
# the dressing's chandelier spheres; run this file's "place" again after them):
#     "<engine python>" -I tools/ue_mcp.py py tools/rodin/import_props.py
# Every step is re-runnable. A wrapper that sets these globals and execs this file runs part of it (an MCP call gives
# up at ~110 s, so import a few props per call):
#     PROPS_ONLY = ("import",); PROPS_NAMES = ("podium", "pedestal", "reel")
#
#   import     GLB -> /Game/Take/Props/<Prop>/ (mesh, textures, Interchange's material). replace_existing=True and
#              save=True on purpose (a retake overwrites, and the Preview engine can crash). The mesh is renamed
#              SM_<Prop>, gets a single box collision (placed props are NoCollision; device bodies keep a box, as the
#              cube they replace had) and Nanite where it repeats (NANITE below)
#   materials  M_Prop (lit): the base texture's luminance remapped between two greys (Dark, Light), so the value sits
#              in the band §15.9 gives that kind of surface, and (QA note 8) the texture's own hue kept at that value
#              (KeepColour, PropSat; 0 = the grey band as before); the normal map kept; one roughness per prop;
#              optional emissive on the brightest texels (the chandelier's bulbs). One MI_Prop_<Prop> per prop
#   place      swaps: the box keeps its collision and goes invisible (component visibility only); a Prop_ actor
#              (NoCollision, no navigation) fills the box it replaced. Repeats go on one instanced component per prop.
#              Devices (the two carts, the sound desk) get the mesh on their Body: the roll, the hover highlight and the
#              click volume are untouched, but FTakeShapes::SetColour recolours the Body flat at runtime
#   unplace    undoes place (deletes Prop_ actors, shows the boxes, restores device bodies): for before/after shots
#   check      what was placed per prop, triangles, and anything NEW within 1.2 m of a mark, watcher, device, crew
#              post or character, or grown more than 10 cm past the box it replaced
#   lineup     every imported prop in a row at LINEUP (facing +X once FRONT is right) for one screenshot of the
#              facings; "unlineup" deletes it. Tune FRONT until every prop faces +X there.
import math
import os
import re
import sys

import unreal

ONLY = globals().get("PROPS_ONLY") or ("import", "materials", "place", "check")
NAMES = globals().get("PROPS_NAMES")
SRC = r"C:\Jam\Art\Rodin"
DEST = "/Game/Take/Props"
DRESS = "/Game/Take/Dress"
LINEUP = globals().get("PROPS_LINEUP") or (-5600.0, -2400.0)       # x, first y: outside the building, west of the street (nothing plays there)

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
CUBE = eal.load_asset("/Engine/BasicShapes/Cube")
REPORT = []

# --- the props ----------------------------------------------------------------------------------------------------------
WARM = (1.0, 0.88, 0.78)            # wood (§15.9's panels and parquet keep about this ratio)
STONE = (1.0, 0.97, 0.93)
STEEL = (0.95, 0.96, 1.0)
VELVET = (1.0, 0.60, 0.75)          # MI_Front_Velvet's 0.06, 0.035, 0.045
NEUT = (1.0, 1.0, 1.0)
BONE = (0.80, 0.76, 0.66)           # the sconces' glow

# prop: (Dark, Light) linear grey, tint, roughness, casts shadow, emissive (lo, scale) or None.
# Light stays at or under 0.45 (MI_Front_Linen, the lightest lit set surface): Julian (0.80) stays lightest in frame.
LOOK = {
    "podium": ((0.035, 0.34), WARM, 0.55, True, None),
    "pedestal": ((0.20, 0.40), STONE, 0.6, True, None),
    "chandelier": ((0.03, 0.34), STEEL, 0.4, False, (0.55, 3.0)),
    "piano": ((0.012, 0.30), NEUT, 0.3, True, None),
    "chair": ((0.03, 0.20), VELVET, 0.9, False, None),
    "column": ((0.16, 0.34), STONE, 0.5, True, None),
    "cocktail": ((0.03, 0.34), WARM, 0.5, False, None),
    "bar": ((0.035, 0.30), WARM, 0.5, True, None),
    "backbar": ((0.02, 0.30), WARM, 0.5, True, None),
    "bust": ((0.18, 0.40), STONE, 0.6, False, None),
    "vase": ((0.18, 0.40), STONE, 0.6, False, None),
    "frame": ((0.08, 0.34), STEEL, 0.35, False, None),
    "stanchion": ((0.08, 0.34), STEEL, 0.35, False, None),
    "sofa": ((0.02, 0.10), VELVET, 1.0, True, None),
    "stool": ((0.02, 0.30), STEEL, 0.5, False, None),
    "roadcase": ((0.012, 0.25), STEEL, 0.4, False, None),
    "crate": ((0.08, 0.28), WARM, 0.9, False, None),
    "lightstand": ((0.012, 0.30), STEEL, 0.4, False, None),
    "camera": ((0.015, 0.28), STEEL, 0.4, False, None),
    "coatrack": ((0.012, 0.16), NEUT, 0.8, False, None),
}
SIGNAL = ("reel",)                  # keeps the box's own material and stencil (MI_Gold, stencil 2: the case is gold)
DEVICES = {"cart": ("CartDevice",), "desk": ("SoundDesk",)}
NANITE = {"chair", "column", "frame", "stanchion", "crate", "roadcase", "cocktail", "stool", "bust", "vase"}
ALL = sorted(set(LOOK) | set(SIGNAL) | set(DEVICES))

# The yaw (degrees, multiples of 90) that turns each imported mesh to face +X. Rodin's front comes in along an
# unknown axis after Interchange's glTF conversion: set from the lineup screenshot in phase 2.
FRONT = {n: 0.0 for n in ALL}
FRONT["piano"] = 270.0              # Rodin drew the grand side-on: its keyboard comes in on +Y (seen from above, Nov 15)

# What each prop replaces. parts: label patterns grouped by touching bounds into one piece; hide: patterns hidden
# with no prop of their own; mode: fit (uniform, inside), fitxy (uniform from the footprint), stretch (per axis),
# stretchz (uniform footprint, height stretched), row (chairs along the long side), modules (n stretched sections
# along the long side); face: +X 0, +Y 90, -X 180, -Y 270, "auto" (long side to long side), "away:<pattern>" (away
# from the part matching it), "wall" (frames: away from the wall, out of the picture's side).
RULES = [
    dict(prop="podium", parts=[r"Podium_\d+"], mode="fit", face=180),
    dict(prop="pedestal", parts=[r"Case_Plinth_\d+"], mode="fit", face=180),
    dict(prop="reel", parts=[r"Case_Reel_\d+"], mode="fit", face=180),
    dict(prop="pedestal", parts=[r"Plinth_\d+"], mode="fit", face=270),
    dict(prop="bust", parts=[r"Lot_\d+", r"Preview_Lot_\d+"], mode="fitxy", face=270, alternate="vase", grow=1.25,
         on=r"Plinth_\d+|Preview_Plinth_\d+|Lot_Plinth_\d+", every_other=True),
    dict(prop="column", parts=[r"Column_\d+"], mode="stretchz", face=0, grow=1.08),   # flutes stretch, base too
    dict(prop="chair", parts=[r"Bench_\d+"], mode="row", face=0, spacing=62.0, height=90.0),
    dict(prop="cocktail", parts=[r"Cocktail_Table_\d+", r"Cocktail_Stem_\d+"], mode="fit", face=0),
    dict(prop="bar", parts=[r"Bar_\d+"], mode="modules", n=4, face=90),
    dict(prop="backbar", parts=[r"Bar_Shelf_\d+"], mode="modules", n=2, face=90, hide=[r"Bar_Bottles_\d+"]),
    dict(prop="stool", parts=[r"Stool_\d+"], mode="fit", face=90),
    dict(prop="frame", parts=[r"Frame_\d+"], mode="frame", face="wall", depth=8.0),
    dict(prop="stanchion", parts=[r"(Dress_)?Stanchion_\d+", r"(Dress_)?Stanchion_Foot_\d+"], mode="fit", face=0),
    dict(prop="sofa", parts=[r"Sofa_\d+", r"Sofa_Back_\d+"], mode="fit", face="away:Sofa_Back_"),
    dict(prop="roadcase", parts=[r"Flight_Case_\d+", r"Dress_Road_Case_\d+"], mode="stretch", face="auto"),
    dict(prop="crate", parts=[r"Crate_\d+", r"Stock_Crate_\d+", r"Stock_Crate_Top_\d+", r"Dress_Crate_\d+"],
         mode="stretch", face="auto"),
    dict(prop="coatrack", parts=[r"Cloak_Rack_\d+"], mode="stretch", face="auto"),
    dict(prop="camera", parts=[r"Tripod_\d+", r"Camera_\d+"], mode="fit", face=0),
    dict(prop="lightstand", parts=[r"Spare_Lamp_\d+"], mode="fit", face=0),
    # The dressing's five spheres and the centre one (dress_auction.py CHANDELIER: 790 cm up, a 70 cm ring) go; the
    # rod to the ceiling stays and the sputnik hangs from it, 260 cm across, its top 40 cm over the old ring.
    dict(prop="chandelier", parts=[r"Dress_Chandelier_\d+"], mode="fit", face=0, group_pad=80.0,
         size=(260.0, 260.0, 260.0), top=40.0),
]
# New pieces with no box (x0, x1, y0, y1, z0, z1). The grand piano on the band's riser (BAND x 3640-3920, y 1320-1600,
# top 30 cm): its keyboard on the east side (face 0, +X), its body reaching west, so a pianist seated at the keyboard
# faces west, down the hall to the house. It leaves the back line (bass 3693,1571; drums 3811,1561) clear and covers
# today's singer, trumpet and seated pianist: step 5 (tools/build_cast.py) re-seats the pianist at SEAT and moves the
# front line after it, from the transform "check" prints. Nothing of the band or the box upright is moved or hidden.
NEW = [dict(prop="piano", box=(3680.0, 3855.0, 1340.0, 1490.0, 30.0, 160.0), mode="fit", face=0,
            seat=35.0)]       # the pianist's seat: this many cm east of the keyboard edge, on the keyboard's middle
GAMEPLAY = ("Mark", "Watcher", "Device", "Crew", "Julian", "Maya", "Vex", "Character", "Guard", "Drone", "Spot")
# Collision (Nov 14 rules): the watchers see with Visibility line traces (TakeDirectorSight.cpp) and the camera boom
# probes Camera, so no prop answers either. The heroes and the raised pieces are NoCollision (a hero's hidden box keeps
# the collision it had); the floor fillers block Pawn only, a box each, so Julian can't walk through the furniture.
BLOCKERS = {"chair", "column", "cocktail", "bar", "backbar", "stool", "stanchion", "sofa", "roadcase", "crate",
            "coatrack", "camera", "lightstand", "piano"}
LOOSE = []                          # (prop, box): floor fillers over a piece that let the pawn through before


# --- helpers ------------------------------------------------------------------------------------------------------------
def cap(n):
    return n[:1].upper() + n[1:]


def warn(s):
    REPORT.append("WARNING: " + s)
    unreal.log_warning("[props] " + s)


def try_set(obj, **props):
    for kk, v in props.items():
        try:
            obj.set_editor_property(kk, v)
        except Exception as e:                                  # noqa: BLE001 (a missing property must not stop a run)
            warn("%s.%s: %s" % (obj.get_name(), kk, e))


def lc(rgb, a=1.0):
    return unreal.LinearColor(rgb[0], rgb[1], rgb[2], a)


def label(a):
    return a.get_actor_label()


def bounds(a):
    o, e = a.get_actor_bounds(False)
    return [o.x - e.x, o.x + e.x, o.y - e.y, o.y + e.y, o.z - e.z, o.z + e.z]


def union(bs):
    return [min(b[0] for b in bs), max(b[1] for b in bs), min(b[2] for b in bs), max(b[3] for b in bs),
            min(b[4] for b in bs), max(b[5] for b in bs)]


def touches(a, b, pad):
    return a[0] < b[1] + pad and b[0] < a[1] + pad and a[2] < b[3] + pad and b[2] < a[3] + pad \
        and a[4] < b[5] + pad and b[4] < a[5] + pad


def tags(a):
    return [str(t) for t in a.get_editor_property("tags")]


def add_tag(a, t):
    if t not in tags(a):
        a.set_editor_property("tags", list(a.get_editor_property("tags")) + [unreal.Name(t)])


def drop_tags(a, prefix):
    a.set_editor_property("tags", [unreal.Name(t) for t in tags(a) if not t.startswith(prefix)])


def mesh_path(n):
    return "%s/%s/SM_%s" % (DEST, cap(n), cap(n))


def save(path):
    eal.save_asset(path, only_if_is_dirty=False)


# --- 1. import ------------------------------------------------------------------------------------------------------------
def sme():
    try:
        return unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    except Exception:                                           # noqa: BLE001
        return unreal.EditorStaticMeshLibrary


def import_one(n):
    src = os.path.join(SRC, n, "base_basic_pbr.glb")
    if not os.path.exists(src):
        warn("%s: no %s" % (n, src))
        return
    folder = "%s/%s" % (DEST, cap(n))
    before = set(eal.list_assets(folder, recursive=True)) if eal.does_directory_exist(folder) else set()
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", src)
    t.set_editor_property("destination_path", folder)
    t.set_editor_property("replace_existing", True)            # on purpose: a retake replaces the last import
    try_set(t, replace_existing_settings=True)
    t.set_editor_property("automated", True)
    t.set_editor_property("save", True)                        # on purpose: saved as it lands
    at.import_asset_tasks([t])
    found = [eal.load_asset(p) for p in (list(t.get_editor_property("imported_object_paths") or [])
                                         or sorted(set(eal.list_assets(folder, recursive=True)) - before))]
    meshes = [m for m in found if isinstance(m, unreal.StaticMesh)]
    if not meshes:
        meshes = [eal.load_asset(p) for p in eal.list_assets(folder, recursive=True)]
        meshes = [m for m in meshes if isinstance(m, unreal.StaticMesh)]
    if not meshes:
        warn("%s: the import made no static mesh" % n)
        return
    if len(meshes) > 1:
        warn("%s: %d meshes imported; using the first (%s)" % (n, len(meshes), meshes[0].get_name()))
    m = meshes[0]
    if m.get_path_name().split(".")[0] != mesh_path(n):
        if eal.does_asset_exist(mesh_path(n)):
            eal.delete_asset(mesh_path(n))
        eal.rename_asset(m.get_path_name().split(".")[0], mesh_path(n))
        m = eal.load_asset(mesh_path(n))
    lib = sme()
    try:
        lib.remove_collisions(m)
        lib.add_simple_collisions(m, unreal.ScriptCollisionShapeType.BOX)
    except Exception as e:                                      # noqa: BLE001
        warn("%s: collision: %s" % (n, e))
    if n in NANITE:
        try:
            ns = m.get_editor_property("nanite_settings")
            ns.set_editor_property("enabled", True)
            m.set_editor_property("nanite_settings", ns)
        except Exception as e:                                  # noqa: BLE001
            warn("%s: nanite: %s" % (n, e))
    save(mesh_path(n))
    b = m.get_bounding_box()
    REPORT.append("import %s: %s, %.0f x %.0f x %.0f cm" % (n, mesh_path(n), b.max.x - b.min.x, b.max.y - b.min.y,
                                                            b.max.z - b.min.z))


# --- 2. materials -----------------------------------------------------------------------------------------------------------
# --- QA look (note 8, Nov 14): the props keep their own colour. The texture's luminance still lands in the prop's
# value band (Dark..Light, so the value hierarchy holds), but its hue and saturation come back: KeepColour 0 = the
# grey band as before, 1 = the texture's colour at the band's value; PropSat scales that colour (the grade mutes it
# again). Both are M_Prop parameters (per-prop overrides in the editor); TUNGSTEN warms the chandelier's bulbs.
KEEP_COLOUR, PROP_SAT = 1.0, 0.75
TUNGSTEN = (0.80, 0.50, 0.24)
BASE_HLSL = ("float3 lw = float3(0.2126, 0.7152, 0.0722);\n"
             "float l = dot(C.rgb, lw);\n"
             "float3 band = lerp(Dark.rgb, Light.rgb, saturate(l));\n"
             "float3 hue = lerp(float3(1.0, 1.0, 1.0), min(C.rgb / max(l, 0.02), 3.0), PropSat);\n"
             "return lerp(band, min(hue * dot(band, lw), 1.0), KeepColour);")
# --- end QA look
EMIT_HLSL = ("float l = dot(C, float3(0.2126, 0.7152, 0.0722));\n"
             "return Col * Scale * saturate((l - Lo) / max(1.0 - Lo, 0.01));")


def textures(n):
    """(base, normal) Texture2D of a prop's import, told apart by compression and sRGB, then by name."""
    base = normal = None
    for p in eal.list_assets("%s/%s" % (DEST, cap(n)), recursive=True):
        t = eal.load_asset(p)
        if not isinstance(t, unreal.Texture2D):
            continue
        nm = t.get_name().lower()
        if t.get_editor_property("compression_settings") == unreal.TextureCompressionSettings.TC_NORMALMAP \
                or "normal" in nm:
            normal = normal or t
        elif t.get_editor_property("srgb") and not any(k in nm for k in ("rough", "metal", "orm", "occlu")):
            base = base or t
    return base, normal


def custom(m, code, names, x, y, out="CMOT_FLOAT3"):
    cu = mel.create_material_expression(m, unreal.MaterialExpressionCustom, x, y)
    cu.set_editor_property("code", code)
    cu.set_editor_property("output_type", getattr(unreal.CustomMaterialOutputType, out))
    ins = []
    for nm in names:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", nm)
        ins.append(ci)
    cu.set_editor_property("inputs", ins)
    return cu


def param(m, cls, name, x, y, value=None):
    e = mel.create_material_expression(m, cls, x, y)
    e.set_editor_property("parameter_name", name)
    if value is not None:
        e.set_editor_property("default_value", value)
    return e


def build_master(default_base, default_normal):
    path = DEST + "/M_Prop"
    if eal.does_asset_exist(path):
        m = eal.load_asset(path)
        mel.delete_all_material_expressions(m)
    else:
        m = at.create_asset("M_Prop", DEST, unreal.Material, unreal.MaterialFactoryNew())
    # Usage flags here, never at runtime (dress_auction.py: a flag set on the editor's first frame crashed the GPU).
    try_set(m, used_with_instanced_static_meshes=True, used_with_nanite=True)
    bt = param(m, unreal.MaterialExpressionTextureSampleParameter2D, "BaseTex", -900, -200)
    bt.set_editor_property("texture", default_base)
    nt = param(m, unreal.MaterialExpressionTextureSampleParameter2D, "NormalTex", -900, 300)
    nt.set_editor_property("texture", default_normal)
    nt.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    dark = param(m, unreal.MaterialExpressionVectorParameter, "Dark", -900, 0, lc((0.03, 0.03, 0.03)))
    light = param(m, unreal.MaterialExpressionVectorParameter, "Light", -900, 100, lc((0.30, 0.30, 0.30)))
    rough = param(m, unreal.MaterialExpressionScalarParameter, "Roughness", -500, 200, 0.6)
    metal = param(m, unreal.MaterialExpressionScalarParameter, "Metallic", -500, 260, 0.0)
    elo = param(m, unreal.MaterialExpressionScalarParameter, "EmitLo", -900, 500, 0.6)
    ecol = param(m, unreal.MaterialExpressionVectorParameter, "EmitColor", -900, 560, lc(TUNGSTEN))   # QA look: was BONE
    esc = param(m, unreal.MaterialExpressionScalarParameter, "EmitScale", -900, 660, 0.0)
    cb = custom(m, BASE_HLSL, ["C", "Dark", "Light", "KeepColour", "PropSat"], -500, -100)   # QA look: + 2 inputs
    mel.connect_material_expressions(bt, "RGB", cb, "C")
    mel.connect_material_expressions(dark, "", cb, "Dark")
    mel.connect_material_expressions(light, "", cb, "Light")
    mel.connect_material_expressions(param(m, unreal.MaterialExpressionScalarParameter, "KeepColour", -900, 160,
                                           KEEP_COLOUR), "", cb, "KeepColour")   # QA look
    mel.connect_material_expressions(param(m, unreal.MaterialExpressionScalarParameter, "PropSat", -900, 220,
                                           PROP_SAT), "", cb, "PropSat")         # QA look
    ce = custom(m, EMIT_HLSL, ["C", "Lo", "Col", "Scale"], -500, 450)
    mel.connect_material_expressions(bt, "RGB", ce, "C")
    mel.connect_material_expressions(elo, "", ce, "Lo")
    mel.connect_material_expressions(ecol, "", ce, "Col")
    mel.connect_material_expressions(esc, "", ce, "Scale")
    mel.connect_material_property(cb, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(nt, "RGB", unreal.MaterialProperty.MP_NORMAL)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
    mel.connect_material_property(ce, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m)
    save(path)
    return m


def build_materials(names):
    have = [n for n in names if n in LOOK and eal.does_asset_exist(mesh_path(n))]
    if not have:
        warn("materials: nothing imported yet")
        return
    b0, n0 = textures(have[0])
    master = build_master(b0 or eal.load_asset("/Engine/EngineResources/DefaultTexture"),
                          n0 or eal.load_asset("/Engine/EngineMaterials/FlatNormal"))
    for n in have:
        (dk, lt), tint, rough, _shadow, emit = LOOK[n]
        base, normal = textures(n)
        if not base:
            warn("%s: no base texture found; the band's middle grey is used" % n)
        path = "%s/%s/MI_Prop_%s" % (DEST, cap(n), cap(n))
        mi = eal.load_asset(path) if eal.does_asset_exist(path) else at.create_asset(
            "MI_Prop_" + cap(n), "%s/%s" % (DEST, cap(n)), unreal.MaterialInstanceConstant,
            unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, master)
        if base:
            mel.set_material_instance_texture_parameter_value(mi, "BaseTex", base)
        if normal:
            mel.set_material_instance_texture_parameter_value(mi, "NormalTex", normal)
        mel.set_material_instance_vector_parameter_value(mi, "Dark", lc([dk * c for c in tint]))
        mel.set_material_instance_vector_parameter_value(mi, "Light", lc([lt * c for c in tint]))
        mel.set_material_instance_scalar_parameter_value(mi, "Roughness", float(rough))
        if emit:
            mel.set_material_instance_scalar_parameter_value(mi, "EmitLo", float(emit[0]))
            mel.set_material_instance_scalar_parameter_value(mi, "EmitScale", float(emit[1]))
        mel.update_material_instance(mi)
        save(path)
        m = eal.load_asset(mesh_path(n))
        for i in range(max(1, m.get_num_sections(0)) if hasattr(m, "get_num_sections") else 1):
            m.set_material(i, mi)
        save(mesh_path(n))
    REPORT.append("materials: M_Prop and %d instances" % len(have))


# --- 3. place ------------------------------------------------------------------------------------------------------------
def rotate(v, r):
    """v turned by r = (roll, pitch, yaw) in degrees, as FRotationMatrix does."""
    sr, cr = math.sin(math.radians(r[0])), math.cos(math.radians(r[0]))
    sp, cp = math.sin(math.radians(r[1])), math.cos(math.radians(r[1]))
    sy, cy = math.sin(math.radians(r[2])), math.cos(math.radians(r[2]))
    ax = (cp * cy, cp * sy, sp)
    ay = (sr * sp * cy - cr * sy, sr * sp * sy + cr * cy, -sr * cp)
    az = (-(cr * sp * cy + sr * sy), cy * sr - cr * sp * sy, cr * cp)
    return tuple(v[0] * ax[i] + v[1] * ay[i] + v[2] * az[i] for i in range(3))


def as_rot(r):
    return tuple(r) if isinstance(r, (tuple, list)) else (0.0, 0.0, float(r))


def fit(mesh, box, r, mode):
    """(location, scale) that put `mesh`, turned by r (roll, pitch, yaw in multiples of 90, or a yaw), into box
    (x0, x1, y0, y1, z0, z1): centred on the box's footprint, standing on its floor."""
    r = as_rot(r)
    bb = mesh.get_bounding_box()
    mn, mx = (bb.min.x, bb.min.y, bb.min.z), (bb.max.x, bb.max.y, bb.max.z)
    d = [max(mx[i] - mn[i], 0.1) for i in range(3)]
    perm = []                                                   # the world axis each of the mesh's axes ends up on
    for i in range(3):
        u = rotate(tuple(1.0 if j == i else 0.0 for j in range(3)), r)
        perm.append(max(range(3), key=lambda j: abs(u[j])))
    t = (box[1] - box[0], box[3] - box[2], box[5] - box[4])
    ratio = [t[perm[i]] / d[i] for i in range(3)]               # per mesh axis: the box's size over the mesh's
    if mode == "stretch":
        sc = ratio
    elif mode == "stretchz":
        s = min(ratio[i] for i in range(3) if perm[i] != 2)
        sc = [ratio[i] if perm[i] == 2 else s for i in range(3)]
    elif mode == "fitxy":
        s = min(ratio[i] for i in range(3) if perm[i] != 2)
        sc = [s, s, s]
    else:
        s = min(ratio)
        sc = [s, s, s]
    corners = [rotate((x * sc[0], y * sc[1], z * sc[2]), r) for x in (mn[0], mx[0]) for y in (mn[1], mx[1])
               for z in (mn[2], mx[2])]
    lo = [min(c[i] for c in corners) for i in range(3)]
    hi = [max(c[i] for c in corners) for i in range(3)]
    loc = ((box[0] + box[1]) / 2 - (lo[0] + hi[0]) / 2, (box[2] + box[3]) / 2 - (lo[1] + hi[1]) / 2, box[4] - lo[2])
    return loc, tuple(sc)


def best_turn(mesh, box, yaws):
    """The 90-degree turn (roll, pitch, yaw with yaw from `yaws`) whose turned mesh best matches the box's
    proportions: a flat portrait frame onto a landscape picture on a wall, whatever axis Rodin modelled it along."""
    bb = mesh.get_bounding_box()
    d = (bb.max.x - bb.min.x, bb.max.y - bb.min.y, bb.max.z - bb.min.z)
    t = (box[1] - box[0], box[3] - box[2], box[5] - box[4])
    thin_t = min(range(3), key=lambda i: t[i])
    best, score = (0.0, 0.0, yaws[0]), None
    for roll in (0.0, 90.0):
        for pitch in (0.0, 90.0):
            for yaw in yaws:
                r = (roll, pitch, yaw)
                w = [abs(c) for c in rotate(d, r)]
                if min(range(3), key=lambda i: w[i]) != thin_t:     # its thin side against the wall
                    continue
                a, b = [i for i in range(3) if i != thin_t]         # then the least stretched face
                sc = abs(math.log(max(t[a], 0.1) / max(w[a], 0.1)) - math.log(max(t[b], 0.1) / max(w[b], 0.1)))
                if score is None or sc < score - 1e-6:
                    best, score = r, sc
    return best


def grown(box, k):
    """The box's footprint grown k times about its centre; its height unchanged."""
    cx, cy = (box[0] + box[1]) / 2, (box[2] + box[3]) / 2
    hx, hy = (box[1] - box[0]) / 2 * k, (box[3] - box[2]) / 2 * k
    return [cx - hx, cx + hx, cy - hy, cy + hy, box[4], box[5]]


class Placer:
    def __init__(self):
        self.meshes = {n: eal.load_asset(mesh_path(n)) for n in ALL if eal.does_asset_exist(mesh_path(n))}
        self.xf = {}                # prop -> [(loc, yaw, scale, replaced box)]
        self.signal = []            # (prop, loc, yaw, scale, source component)
        self.n = 0

    def add(self, prop, box, yaw, mode, replaced):
        """yaw: the way the prop should face (FRONT is added), or a full (roll, pitch, yaw) used as it is."""
        m = self.meshes.get(prop)
        if not m:
            return False
        r = tuple(yaw) if isinstance(yaw, (tuple, list)) else (0.0, 0.0, (yaw + FRONT.get(prop, 0.0)) % 360.0)
        loc, sc = fit(m, box, r, mode)
        self.xf.setdefault(prop, []).append((loc, r, sc, replaced))
        return True

    def spawn_all(self):
        for prop, items in sorted(self.xf.items()):
            shadow = LOOK.get(prop, (None, None, None, False))[3]
            mi = eal.load_asset("%s/%s/MI_Prop_%s" % (DEST, cap(prop), cap(prop)))
            if len(items) > 1 and self.instanced(prop, items, mi, shadow):
                continue
            for (loc, yaw, sc, _r) in items:
                self.actor(prop, loc, yaw, sc, mi, shadow)

    def actor(self, prop, loc, r, sc, mat, shadow, src=None):
        self.n += 1
        r = as_rot(r)
        a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*loc), unreal.Rotator(*r))
        a.set_actor_label("Prop_%s_%03d" % (cap(prop), self.n))
        a.set_folder_path("Props/" + cap(prop))
        a.set_actor_scale3d(unreal.Vector(*sc))
        c = a.static_mesh_component
        c.set_static_mesh(self.meshes[prop])
        if mat:
            c.set_material(0, mat)
        collide(c, prop)
        try_set(c, cast_shadow=bool(shadow), receives_decals=False, can_ever_affect_navigation=False)
        if src is not None:                                     # the reel: the case's gold and its stencil
            try_set(c, render_custom_depth=src.get_editor_property("render_custom_depth"),
                    custom_depth_stencil_value=src.get_editor_property("custom_depth_stencil_value"))
        return a

    def instanced(self, prop, items, mi, shadow):
        """One actor with an InstancedStaticMeshComponent for every copy; False (and nothing left) if that fails."""
        a = None
        try:
            a = eas.spawn_actor_from_class(unreal.Actor, unreal.Vector(0.0, 0.0, 0.0))
            a.set_actor_label("Prop_%s_Instances" % cap(prop))
            a.set_folder_path("Props/" + cap(prop))
            sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
            root = sds.k2_gather_subobject_data_for_instance(a)[0]
            h, fail = sds.add_new_subobject(unreal.AddNewSubobjectParams(
                parent_handle=root, new_class=unreal.InstancedStaticMeshComponent))
            if str(fail).strip():
                raise RuntimeError(str(fail))
            lib = unreal.SubobjectDataBlueprintFunctionLibrary
            c = lib.get_object(lib.get_data(h))
            c.set_static_mesh(self.meshes[prop])
            if mi:
                c.set_material(0, mi)
            collide(c, prop)
            try_set(c, cast_shadow=bool(shadow), receives_decals=False, can_ever_affect_navigation=False)
            for (loc, r, sc, _r) in items:
                c.add_instance(unreal.Transform(unreal.Vector(*loc), unreal.Rotator(*as_rot(r)),
                                                unreal.Vector(*sc)), True)
            self.n += len(items)
            return True
        except Exception as e:                                  # noqa: BLE001
            warn("%s: instanced component failed (%s); placing %d actors" % (prop, e, len(items)))
            if a:
                eas.destroy_actor(a)
            return False


def collide(c, prop):
    if prop not in BLOCKERS:
        c.set_collision_profile_name("NoCollision")
        return
    C, R = unreal.CollisionChannel, unreal.CollisionResponseType
    c.set_collision_enabled(unreal.CollisionEnabled.QUERY_ONLY)
    c.set_collision_object_type(C.ECC_WORLD_STATIC)
    c.set_collision_response_to_all_channels(R.ECR_IGNORE)
    c.set_collision_response_to_channel(C.ECC_PAWN, R.ECR_BLOCK)


def blocks_pawn(a):
    c = a.static_mesh_component
    return c.get_collision_enabled() != unreal.CollisionEnabled.NO_COLLISION and \
        c.get_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN) == unreal.CollisionResponseType.ECR_BLOCK


def hide(a):
    a.static_mesh_component.set_visibility(False, False)       # collision, sight lines and navigation unchanged
    add_tag(a, "props:hidden")


def actors_matching(pats, pool):
    rx = re.compile(r"^(%s)$" % "|".join(pats))
    return [a for a in pool if rx.match(label(a))]


def groups(actors, pad):
    """Actors whose bounds touch (within pad) are one piece."""
    bs = [bounds(a) for a in actors]
    parent = list(range(len(actors)))

    def root(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i

    for i in range(len(actors)):
        for j in range(i + 1, len(actors)):
            if touches(bs[i], bs[j], pad):
                parent[root(i)] = root(j)
    out = {}
    for i in range(len(actors)):
        out.setdefault(root(i), []).append(i)
    return [([actors[i] for i in g], union([bs[i] for i in g])) for g in out.values()]


def facing(rule, members, box, mesh):
    f = rule["face"]
    if isinstance(f, (int, float)):
        return float(f)
    if f == "auto":                                             # the mesh's long side along the box's long side
        bb = mesh.get_bounding_box()
        mesh_long_x = (bb.max.x - bb.min.x) >= (bb.max.y - bb.min.y)
        box_long_x = (box[1] - box[0]) >= (box[3] - box[2])
        return 0.0 if mesh_long_x == box_long_x else 90.0
    if f.startswith("away:"):
        back = [a for a in members if label(a).startswith(f[5:])]
        if back:
            b = bounds(back[0])
            dx, dy = (box[0] + box[1]) / 2 - (b[0] + b[1]) / 2, (box[2] + box[3]) / 2 - (b[2] + b[3]) / 2
            return 0.0 if abs(dx) >= abs(dy) and dx > 0 else 180.0 if abs(dx) >= abs(dy) else 90.0 if dy > 0 else 270.0
        return 0.0
    return 0.0


def place():
    unplace(quiet=True)
    pool = [a for a in eas.get_all_level_actors() if isinstance(a, unreal.StaticMeshActor)
            and not label(a).startswith("Prop_")]
    P = Placer()
    counts = {}
    for rule in RULES:
        prop = rule["prop"]
        if NAMES and prop not in NAMES and rule.get("alternate") not in NAMES:
            continue
        if prop not in P.meshes:
            warn("%s: not imported; its boxes stay" % prop)
            continue
        found = actors_matching(rule["parts"], pool)
        if rule.get("on"):                                      # only lots standing on a plinth
            under = actors_matching([rule["on"]], pool)
            found = [a for a in found if any(touches(bounds(a), bounds(u), 3.0) for u in under)] \
                + [a for a in found if label(a).startswith("Preview_Lot_")]
            found = list({label(a): a for a in found}.values())
        for k_, (members, box) in enumerate(groups(found, rule.get("group_pad", 2.0))):
            mesh_prop = rule["alternate"] if rule.get("every_other") and k_ % 2 and rule.get("alternate") in P.meshes \
                else prop
            mesh = P.meshes[mesh_prop]
            target = grown(box, rule["grow"]) if rule.get("grow") else box
            if rule.get("size"):                                # a box of its own, hung from the group's top
                w, dd, h = rule["size"]
                cx, cy, top = (box[0] + box[1]) / 2, (box[2] + box[3]) / 2, box[5] + rule.get("top", 0.0)
                target = [cx - w / 2, cx + w / 2, cy - dd / 2, cy + dd / 2, top - h, top]
            if rule["mode"] == "row":
                long_x = (box[1] - box[0]) > (box[3] - box[2])
                length = (box[1] - box[0]) if long_x else (box[3] - box[2])
                n = max(1, int(length // rule["spacing"]))
                step = length / n
                for i in range(n):
                    a0 = (box[0] if long_x else box[2]) + i * step
                    cell = [a0 + 3, a0 + step - 3, box[2], box[3]] if long_x else [box[0], box[1], a0 + 3, a0 + step - 3]
                    P.add(prop, cell + [box[4], box[4] + rule["height"]], facing(rule, members, box, mesh), "fit", box)
            elif rule["mode"] == "modules":
                long_x = (box[1] - box[0]) > (box[3] - box[2])
                n = rule["n"]
                for i in range(n):
                    if long_x:
                        w = (box[1] - box[0]) / n
                        cell = [box[0] + i * w, box[0] + (i + 1) * w, box[2], box[3], box[4], box[5]]
                    else:
                        w = (box[3] - box[2]) / n
                        cell = [box[0], box[1], box[2] + i * w, box[2] + (i + 1) * w, box[4], box[5]]
                    P.add(mesh_prop, cell, float(rule["face"]), "stretch", box)
            elif rule["mode"] == "frame":
                frame_box, yaw = frame_target(members[0], box, pool, rule["depth"])
                P.add(prop, frame_box, best_turn(mesh, frame_box, (yaw, (yaw + 180.0) % 360.0)), "stretch", box)
            elif prop in SIGNAL:
                src = members[0].static_mesh_component
                r = (0.0, 0.0, (facing(rule, members, box, mesh) + FRONT.get(prop, 0.0)) % 360.0)
                loc, sc = fit(mesh, target, r, rule["mode"])
                P.signal.append((prop, loc, r, sc, src))
            else:
                P.add(mesh_prop, target, facing(rule, members, box, mesh), rule["mode"], box)
            if mesh_prop in BLOCKERS and not any(blocks_pawn(a) for a in members):
                LOOSE.append((mesh_prop, target))
            for a in members:
                if not any(re.match(r"^(%s)$" % p, label(a)) for p in rule.get("keep", [])):
                    hide(a)
            counts[mesh_prop] = counts.get(mesh_prop, 0) + 1
        for a in actors_matching(rule.get("hide", []), pool):
            hide(a)
    for spec in NEW:
        if spec["prop"] in P.meshes and (not NAMES or spec["prop"] in NAMES):
            P.add(spec["prop"], list(spec["box"]), float(spec["face"]), spec["mode"], None)
            counts[spec["prop"]] = counts.get(spec["prop"], 0) + 1
            LOOSE.append((spec["prop"], list(spec["box"])))
    P.spawn_all()
    for (prop, loc, yaw, sc, src) in P.signal:
        P.actor(prop, loc, yaw, sc, src.get_material(0), False, src=src)
    for prop, classes in DEVICES.items():
        if prop in P.meshes and (not NAMES or prop in NAMES):
            counts[prop] = swap_bodies(P.meshes[prop], classes)
    REPORT.append("place: " + ", ".join("%s %d" % kv for kv in sorted(counts.items())))


def frame_target(frame, box, pool, depth):
    """A picture frame's box grown `depth` cm off the wall (so the moulding stands proud of the picture), and the yaw
    that faces the frame into the room (the side its picture is on)."""
    thin_x = (box[1] - box[0]) < (box[3] - box[2])
    pics = [a for a in pool if label(a).startswith("Picture_") and touches(bounds(a), box, 3.0)]
    out = 1.0
    if pics:
        p = bounds(pics[0])
        out = 1.0 if ((p[0] + p[1]) / 2 - (box[0] + box[1]) / 2 if thin_x
                      else (p[2] + p[3]) / 2 - (box[2] + box[3]) / 2) > 0 else -1.0
    b = list(box)
    if thin_x:
        if out > 0:
            b[1] = b[0] + depth
        else:
            b[0] = b[1] - depth
        return b, 0.0 if out > 0 else 180.0
    if out > 0:
        b[3] = b[2] + depth
    else:
        b[2] = b[3] - depth
    return b, 90.0 if out > 0 else 270.0


def swap_bodies(mesh, classes):
    n = 0
    for a in eas.get_all_level_actors():
        if type(a).__name__ not in classes:
            continue
        body = a.get_editor_property("body")
        s, l = body.get_editor_property("relative_scale3d"), body.get_editor_property("relative_location")
        if not any(t.startswith("props:body:") for t in tags(a)):
            add_tag(a, "props:body:%g,%g,%g,%g,%g,%g" % (s.x, s.y, s.z, l.x, l.y, l.z))
        else:                                                   # fit to the cube it had, not to the last swap
            v = [float(x) for x in [t for t in tags(a) if t.startswith("props:body:")][0][11:].split(",")]
            s, l = unreal.Vector(v[0], v[1], v[2]), unreal.Vector(v[3], v[4], v[5])
        box = [l.x - 50 * s.x, l.x + 50 * s.x, l.y - 50 * s.y, l.y + 50 * s.y, l.z - 50 * s.z, l.z + 50 * s.z]
        loc, sc = fit(mesh, box, 0.0, "fit")                    # no yaw: CartDevice sets the body's rotation as it rolls
        body.set_static_mesh(mesh)
        body.set_editor_property("relative_scale3d", unreal.Vector(*sc))
        body.set_editor_property("relative_location", unreal.Vector(*loc))
        n += 1
    return n


def unplace(quiet=False):
    gone = shown = bodies = 0
    for a in eas.get_all_level_actors():
        lb = label(a)
        if lb.startswith("Prop_"):
            eas.destroy_actor(a)
            gone += 1
            continue
        tg = tags(a)
        if "props:hidden" in tg and isinstance(a, unreal.StaticMeshActor):
            a.static_mesh_component.set_visibility(True, False)
            drop_tags(a, "props:hidden")
            shown += 1
        body_tags = [t for t in tg if t.startswith("props:body:")]
        if body_tags:
            v = [float(x) for x in body_tags[0][11:].split(",")]
            body = a.get_editor_property("body")
            body.set_static_mesh(CUBE)
            body.set_editor_property("relative_scale3d", unreal.Vector(v[0], v[1], v[2]))
            body.set_editor_property("relative_location", unreal.Vector(v[3], v[4], v[5]))
            drop_tags(a, "props:body:")
            bodies += 1
    if not quiet:
        REPORT.append("unplace: %d props removed, %d boxes shown, %d device bodies restored" % (gone, shown, bodies))


# --- 4. check -------------------------------------------------------------------------------------------------------------
def glb_tris(n):
    try:
        sys.path.insert(0, os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()),
                                        "..", "tools", "rodin"))
        import generate                                         # noqa: E402
        return generate.glb_triangles(os.path.join(SRC, n, "base_basic_pbr.glb")) or 0
    except Exception:                                           # noqa: BLE001
        return 0


def check():
    acts = eas.get_all_level_actors()
    props = [a for a in acts if label(a).startswith("Prop_")]
    play = [a for a in acts if any(k in type(a).__name__ for k in GAMEPLAY)]
    per, tris = {}, 0
    for a in props:
        n = label(a).split("_")[1].lower()
        k_ = 1
        for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
            k_ = c.get_instance_count()
        per[n] = per.get(n, 0) + k_
        tris += k_ * glb_tris(n)
    REPORT.append("check: " + ", ".join("%s %d" % kv for kv in sorted(per.items())) + "; about %.1fM triangles"
                  % (tris / 1e6))
    news = [a for a in props if label(a).split("_")[1].lower() in [s["prop"] for s in NEW]]
    for a in news:
        b = bounds(a)
        spec = [s for s in NEW if s["prop"] == label(a).split("_")[1].lower()][0]
        loc, rt, sc = a.get_actor_location(), a.get_actor_rotation(), a.get_actor_scale3d()
        REPORT.append("%s: location (%.0f, %.0f, %.0f), yaw %.0f, scale %.3f; bounds x %.0f-%.0f, y %.0f-%.0f, "
                      "z %.0f-%.0f" % (label(a), loc.x, loc.y, loc.z, rt.yaw, sc.x, b[0], b[1], b[2], b[3], b[4], b[5]))
        if spec.get("seat") is not None:                        # keyboard on the +X side when face is 0
            REPORT.append("%s seat for a pianist facing yaw 180: (%.0f, %.0f, %.0f)"
                          % (label(a), b[1] + spec["seat"], (b[2] + b[3]) / 2, spec["box"][4]))
        for g in play:
            p = g.get_actor_location()
            dx = max(b[0] - p.x, 0.0, p.x - b[1])
            dy = max(b[2] - p.y, 0.0, p.y - b[3])
            if math.hypot(dx, dy) < 120.0:
                warn("%s is %.0f cm from %s (%s): the 1.2 m rule" % (label(a), math.hypot(dx, dy), label(g),
                                                                       type(g).__name__))
    # New pawn blockers (the piece under them let Julian through): where they stand and what gameplay is nearest.
    for prop, b in LOOSE:
        near = min(((math.hypot(max(b[0] - p.x, 0.0, p.x - b[1]), max(b[2] - p.y, 0.0, p.y - b[3])), label(g))
                    for g in play for p in [g.get_actor_location()]), default=(1e9, "-"))
        REPORT.append("new blocker %s x %.0f-%.0f y %.0f-%.0f: %.0f cm from %s%s"
                      % (prop, b[0], b[1], b[2], b[3], near[0], near[1],
                         "  CROSSES Y -200..200 (carpet, aisle)" if b[2] < 200 and b[3] > -200 else ""))


# --- 5. lineup ------------------------------------------------------------------------------------------------------------
def lineup():
    unlineup()
    y = LINEUP[1]
    for n in ALL:
        if not eal.does_asset_exist(mesh_path(n)):
            continue
        m = eal.load_asset(mesh_path(n))
        loc, sc = fit(m, [LINEUP[0] - 80, LINEUP[0] + 80, y - 80, y + 80, 0.0, 160.0], FRONT.get(n, 0.0), "fit")
        a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*loc),
                                       unreal.Rotator(0.0, 0.0, FRONT.get(n, 0.0)))
        a.set_actor_label("Lineup_" + cap(n))
        a.set_folder_path("Props/Lineup")
        a.set_actor_scale3d(unreal.Vector(*sc))
        a.static_mesh_component.set_static_mesh(m)
        a.static_mesh_component.set_collision_profile_name("NoCollision")
        t = eas.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(LINEUP[0] - 120, y, 190.0),
                                       unreal.Rotator(0.0, 0.0, 180.0))
        t.set_actor_label("Lineup_Text_" + cap(n))
        t.set_folder_path("Props/Lineup")
        t.text_render.set_editor_property("text", n)
        y += 200.0
    REPORT.append("lineup: at x %.0f, y %.0f to %.0f; every prop should face +X" % (LINEUP[0], LINEUP[1], y))


def unlineup():
    for a in eas.get_all_level_actors():
        if label(a).startswith("Lineup_"):
            eas.destroy_actor(a)


# --- run ----------------------------------------------------------------------------------------------------------------------
names = [n for n in (NAMES or ALL)]
if "import" in ONLY:
    for n in names:
        import_one(n)
if "materials" in ONLY:
    build_materials(names)
if "unplace" in ONLY:
    unplace()
if "place" in ONLY:
    place()
if "check" in ONLY:
    check()
if "lineup" in ONLY:
    lineup()
if "unlineup" in ONLY:
    unlineup()
if any(s in ONLY for s in ("place", "unplace", "lineup", "unlineup")):
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("\n".join(REPORT) or "nothing to do")
