# Package hall (QA Nov 14, Marwan's notes 12, 15, 16, 22): the great hall dressed as a 1940s auction gala on live
# television. An overlay on build_auction.py's grey box, after dress_auction.py, build_cast.py and import_props.py;
# run_all.py runs it, or alone under the editor lock:
#     "<engine python>" -I tools/ue_mcp.py py tools/sets/hall.py 110
# Idempotent: it clears what its last run placed (tag set_hall) and shows what it hid, then builds again. Every piece is
# NoCollision unless it says collide=True (pawn only, like import_props' floor furniture): no sight line or camera answers
# anything here. The grey boxes it covers keep their collision. Numbers in cm; layout constants in hall_layout.py.
#   seats      a gilt ballroom chair (Rodin hall_chair) over every bench, facing the dais; paddles and catalogues on a
#              few empty seats; the phone bidders' chairs; writes hall_seats.json and hall_band.json for package cast
#   band       the bandstand grown round the old riser: the grand piano (lid to the house, keys south) and its bench,
#              the drums one step up at the back, the upright bass, the singer's floor microphone, the trumpet's stand,
#              the PA stack dressed as a period theatre speaker
#   broadcast  the TV riser: a studio camera on its pedestal (RCA TK-30 type), a monitor on a stand, the commentator's
#              road case and desk microphone, two lamps, an ON AIR sign, the operator's step, a black skirt, the cables
#              taped down to the staff door; by the dais a microphone boom and two more lamps on the podium
#   bar        a fluted walnut bar with a black top, a brass foot rail, a coupe tower, an ice bucket, a tray of flutes
#   phones     the phone-bid desk under green baize: three bakelite telephones, pads, two lamps, a placard
#   lots       the lots table under burgundy velvet: a bust, an urn, a jewel case, lot cards; urns of flowers on the dais
#   tables     the hall's cocktail-table glasses stood on the Rodin tables' real tops (they floated 16 cm over them)
#   extras     kentia palms in the hall's corners and wall bays
# Switches: HALL_PARTS (a tuple of the part names above) runs only those; HALL_OFF = True only clears (back to before).
import sys, importlib, json, math, os, re, traceback
sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C
import hall_layout as L
importlib.reload(C)
importlib.reload(L)
import unreal

TAG = "set_hall"
PARTS = globals().get("HALL_PARTS") or ("seats", "band", "broadcast", "bar", "phones", "lots", "tables", "extras",
                                         "dais")                 # --- W3 stage --- dais
OFF = bool(globals().get("HALL_OFF"))
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
HDIR = "/Game/Take/Sets/Hall"
CUBE, CYL, SPH = "/Engine/BasicShapes/Cube", "/Engine/BasicShapes/Cylinder", "/Engine/BasicShapes/Sphere"
FRONT = 270.0          # every hall_* Rodin mesh (and the piano, the chair) comes in facing +Y: this turns it to +X
REPORT = []
N = [0]


def prop_path(n):
    c = n[:1].upper() + n[1:]
    return "/Game/Take/Props/%s/SM_%s" % (c, c)


# --- materials: one flat master (ISM-safe) and one that keeps a Rodin texture's colour, muted ---------------------------
def _param(m, cls, name, x, y, value=None):
    e = mel.create_material_expression(m, cls, x, y)
    e.set_editor_property("parameter_name", name)
    if value is not None:
        e.set_editor_property("default_value", value)
    return e


def _lc(rgb):
    return unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0)


def master(name, build):
    path = HDIR + "/" + name
    if eal.does_asset_exist(path):
        return eal.load_asset(path)
    at = unreal.AssetToolsHelpers.get_asset_tools()
    m = at.create_asset(name, HDIR, unreal.Material, unreal.MaterialFactoryNew())
    # usage flags here, never at runtime (dress_auction.py: a flag set on the editor's first frame crashed the GPU)
    for k in ("used_with_instanced_static_meshes", "used_with_nanite"):
        try:
            m.set_editor_property(k, True)
        except Exception:                                       # noqa: BLE001
            pass
    build(m)
    mel.recompile_material(m)
    eal.save_asset(path, only_if_is_dirty=False)
    return m


def build_flat(m):
    col = _param(m, unreal.MaterialExpressionVectorParameter, "Color", -600, -200, _lc((0.2, 0.2, 0.2)))
    rough = _param(m, unreal.MaterialExpressionScalarParameter, "Roughness", -600, 0, 0.6)
    emc = _param(m, unreal.MaterialExpressionVectorParameter, "Emissive", -600, 150, _lc((0.0, 0.0, 0.0)))
    ems = _param(m, unreal.MaterialExpressionScalarParameter, "EmissiveScale", -600, 350, 0.0)
    mul = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, 200)
    mel.connect_material_expressions(emc, "", mul, "A")
    mel.connect_material_expressions(ems, "", mul, "B")
    mel.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


def build_prop(m):
    P = C.ip()
    bt = _param(m, unreal.MaterialExpressionTextureSampleParameter2D, "BaseTex", -900, -200)
    bt.set_editor_property("texture", eal.load_asset("/Engine/EngineResources/DefaultTexture"))
    nt = _param(m, unreal.MaterialExpressionTextureSampleParameter2D, "NormalTex", -900, 300)
    nt.set_editor_property("texture", eal.load_asset("/Engine/EngineMaterials/FlatNormal"))
    nt.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    desat = _param(m, unreal.MaterialExpressionScalarParameter, "Desat", -900, 0, 0.35)
    bright = _param(m, unreal.MaterialExpressionScalarParameter, "Bright", -900, 80, 0.8)
    rough = _param(m, unreal.MaterialExpressionScalarParameter, "Roughness", -500, 200, 0.6)
    cu = P["custom"](m, "float l = dot(C, float3(0.2126, 0.7152, 0.0722)); return lerp(C, l.xxx, Desat) * Bright;",
                     ["C", "Desat", "Bright"], -500, -100)
    mel.connect_material_expressions(bt, "RGB", cu, "C")
    mel.connect_material_expressions(desat, "", cu, "Desat")
    mel.connect_material_expressions(bright, "", cu, "Bright")
    mel.connect_material_property(cu, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(nt, "RGB", unreal.MaterialProperty.MP_NORMAL)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)


def instance(name, parent, vectors=None, scalars=None, textures=None):
    path = HDIR + "/" + name
    if eal.does_asset_exist(path):
        mi = eal.load_asset(path)
    else:
        at = unreal.AssetToolsHelpers.get_asset_tools()
        mi = at.create_asset(name, HDIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    for k, v in (vectors or {}).items():
        mel.set_material_instance_vector_parameter_value(mi, k, _lc(v))
    for k, v in (scalars or {}).items():
        mel.set_material_instance_scalar_parameter_value(mi, k, float(v))
    for k, v in (textures or {}).items():
        if v:
            mel.set_material_instance_texture_parameter_value(mi, k, v)
    mel.update_material_instance(mi)
    eal.save_asset(path, only_if_is_dirty=False)
    return mi


# The muted period palette (linear base colour, roughness): walnut, burgundy, bottle green, cream, brass, black.
FLAT = {
    "Walnut": ((0.060, 0.030, 0.016), 0.45), "WalnutDark": ((0.030, 0.016, 0.009), 0.5),
    "Brass": ((0.30, 0.21, 0.08), 0.3), "Chrome": ((0.42, 0.42, 0.44), 0.2), "Black": ((0.014, 0.014, 0.015), 0.5),
    "Rubber": ((0.018, 0.018, 0.018), 0.85), "Tape": ((0.045, 0.045, 0.045), 0.95), "Cloth": ((0.010, 0.010, 0.012), 0.95),
    "Baize": ((0.022, 0.055, 0.032), 0.95), "Burgundy": ((0.10, 0.014, 0.024), 0.9), "Cream": ((0.50, 0.46, 0.38), 0.85),
    "BurgundyDark": ((0.055, 0.008, 0.014), 0.95),     # --- W3 stage --- the drapes' deep pleats
    "Glass": ((0.30, 0.31, 0.30), 0.05), "Marble": ((0.025, 0.024, 0.024), 0.15), "Foliage": ((0.03, 0.06, 0.03), 0.9),
    "Leather": ((0.02, 0.012, 0.010), 0.6), "Paper": ((0.62, 0.60, 0.54), 0.9),
}
GLOWS = {"OnAir": ((0.05, 0.0, 0.0), (1.0, 0.05, 0.03), 6.0), "Lamp": ((0.2, 0.15, 0.1), (1.0, 0.72, 0.42), 2.5)}
# Rodin props on M_HallProp: (look of the texture kept: desaturation, brightness, roughness)
PROPS = {"hall_chair": (0.30, 0.50, 0.8), "hall_tvcamera": (0.5, 0.7, 0.45), "hall_drums": (0.3, 0.8, 0.35),
         "hall_bass": (0.25, 0.75, 0.4), "hall_mic": (0.5, 0.8, 0.25), "hall_micstand": (0.5, 0.8, 0.25),
         "hall_phone": (0.4, 0.8, 0.3), "hall_monitor": (0.5, 0.75, 0.45)}
M = {}


def materials():
    flat = master("M_HallFlat", build_flat)
    for k, (rgb, r) in FLAT.items():
        M[k] = instance("MI_Hall_" + k, flat, {"Color": rgb}, {"Roughness": r})
    for k, (rgb, em, sc) in GLOWS.items():
        M[k] = instance("MI_Hall_" + k, flat, {"Color": rgb, "Emissive": em}, {"Roughness": 0.4, "EmissiveScale": sc})
    M["Stage"] = eal.load_asset("/Game/Take/Dress/MI_Front_Stage")
    pm = master("M_HallProp", build_prop)
    P = C.ip()
    for n, (de, br, ro) in PROPS.items():
        path = prop_path(n)
        if not eal.does_asset_exist(path):
            P["import_one"](n)
            if not eal.does_asset_exist(path):
                REPORT.append("WARNING %s not imported" % n)
                continue
        mip = HDIR + "/MI_HallProp_" + n
        fresh = not eal.does_asset_exist(mip)
        base, normal = P["textures"](n)
        mi = instance("MI_HallProp_" + n, pm, None, {"Desat": de, "Bright": br, "Roughness": ro},
                      {"BaseTex": base, "NormalTex": normal})
        if not fresh:
            continue
        mesh = eal.load_asset(path)
        for i in range(max(1, mesh.get_num_sections(0))):
            mesh.set_material(i, mi)
        if n == "hall_chair":                                   # 141 of them: Nanite, as import_props does for repeats
            try:
                ns = mesh.get_editor_property("nanite_settings")
                ns.set_editor_property("enabled", True)
                mesh.set_editor_property("nanite_settings", ns)
            except Exception as e:                              # noqa: BLE001
                REPORT.append("chair nanite: %s" % e)
        eal.save_asset(path, only_if_is_dirty=False)


# --- spawning helpers --------------------------------------------------------------------------------------------------
def _lbl(what):
    N[0] += 1
    return "Hall_%s_%03d" % (what, N[0])


def box(what, x0, x1, y0, y1, z0, z1, mat, rot=0.0, collide=False, shadow=False):
    return C.spawn(CUBE, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), rot,
                   ((x1 - x0) / 100.0, (y1 - y0) / 100.0, (z1 - z0) / 100.0), tag=TAG, label=_lbl(what),
                   collide=collide, shadow=shadow, mat=M[mat])


def obox(what, cx, cy, cz, lx, ly, lz, mat, rot=0.0, shadow=False):
    """A box lx by ly by lz centred on (cx, cy, cz), turned by rot (a yaw or (roll, pitch, yaw))."""
    return C.spawn(CUBE, (cx, cy, cz), rot, (lx / 100.0, ly / 100.0, lz / 100.0), tag=TAG, label=_lbl(what),
                   shadow=shadow, mat=M[mat])


def cyl(what, x, y, z0, z1, dia, mat, dia_y=None, shadow=False):
    return C.spawn(CYL, (x, y, (z0 + z1) / 2), 0.0, (dia / 100.0, (dia_y or dia) / 100.0, (z1 - z0) / 100.0),
                   tag=TAG, label=_lbl(what), shadow=shadow, mat=M[mat])


def rod(what, p0, p1, thick, mat, round_=True):
    """A rod from p0 to p1 (a cylinder, or a square bar)."""
    d = [p1[i] - p0[i] for i in range(3)]
    ln = math.sqrt(sum(v * v for v in d))
    yaw = math.degrees(math.atan2(d[1], d[0]))
    pitch = math.degrees(math.atan2(d[2], math.hypot(d[0], d[1])))
    mid = tuple((p0[i] + p1[i]) / 2 for i in range(3))
    if round_:      # the cylinder's axis is its Z: pitch it down onto X first
        return C.spawn(CYL, mid, (0.0, pitch - 90.0, yaw), (thick / 100.0, thick / 100.0, ln / 100.0), tag=TAG,
                       label=_lbl(what), shadow=False, mat=M[mat])
    return C.spawn(CUBE, mid, (0.0, pitch, yaw), (ln / 100.0, thick / 100.0, thick / 100.0), tag=TAG,
                   label=_lbl(what), shadow=False, mat=M[mat])


def put(name_or_path, x, y, z, face, height=None, scale=None, roll=0.0, front=FRONT, collide=False, shadow=True,
        mat=None):
    """A mesh standing on (x, y, z), its footprint centred there, facing the yaw `face`, height cm tall (or scaled)."""
    path = name_or_path if name_or_path.startswith("/") else prop_path(name_or_path)
    m = eal.load_asset(path)
    if not m:
        REPORT.append("WARNING no mesh " + path)
        return None
    bb = m.get_bounding_box()
    s = scale or height / max(bb.max.z - bb.min.z, 0.1)
    r = (roll, 0.0, (face + front) % 360.0)
    rc = C.ip()["rotate"](((bb.min.x + bb.max.x) / 2 * s, (bb.min.y + bb.max.y) / 2 * s, bb.min.z * s), r)
    return C.spawn(m, (x - rc[0], y - rc[1], z - rc[2]), r, s, tag=TAG, label=_lbl(path.split("SM_")[-1]),
                   collide=collide, shadow=shadow, mat=mat)


def put_xf(name_or_path, x, y, z, face, height=None, scale=None, front=FRONT):
    """put()'s transform only, for scatter()."""
    path = name_or_path if name_or_path.startswith("/") else prop_path(name_or_path)
    bb = eal.load_asset(path).get_bounding_box()
    s = scale or height / max(bb.max.z - bb.min.z, 0.1)
    r = (0.0, 0.0, (face + front) % 360.0)
    rc = C.ip()["rotate"](((bb.min.x + bb.max.x) / 2 * s, (bb.min.y + bb.max.y) / 2 * s, bb.min.z * s), r)
    return (x - rc[0], y - rc[1], z - rc[2]), r, s


def text(s, x, y, z, yaw, size, rgb):
    N[0] += 1
    a = eas.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(x, y, z), unreal.Rotator(0.0, 0.0, yaw))
    a.set_actor_label("Set_Hall_Text_%03d" % N[0])
    a.set_folder_path("Sets/" + TAG)
    c = a.get_component_by_class(unreal.TextRenderComponent)
    try:
        c.set_text(s)
    except Exception:                                       # noqa: BLE001
        c.set_editor_property("text", s)
    for k, v in (("world_size", float(size)),
                 ("text_render_color", unreal.Color(r=int(rgb[0] * 255), g=int(rgb[1] * 255), b=int(rgb[2] * 255),
                                                    a=255)),
                 ("horizontal_alignment", unreal.HorizTextAligment.EHTA_CENTER),
                 ("vertical_alignment", unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)):
        try:
            c.set_editor_property(k, v)
        except Exception:                                       # noqa: BLE001
            pass
    a.set_editor_property("tags", [unreal.Name(TAG)])
    return a


LIGHTS = globals().get("HALL_LIGHTS", True)


def practical(x, y, z, cd, radius, rgb=(1.0, 0.78, 0.52)):
    """A small warm point light where a practical lamp stands. No shadows (cheap); HALL_LIGHTS = False: none."""
    if not LIGHTS:
        return None
    N[0] += 1
    a = eas.spawn_actor_from_class(unreal.PointLight, unreal.Vector(x, y, z))
    a.set_actor_label("Set_Hall_Light_%03d" % N[0])
    a.set_folder_path("Sets/" + TAG)
    c = a.get_component_by_class(unreal.PointLightComponent)
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    for k, v in (("intensity_units", unreal.LightUnits.CANDELAS), ("intensity", float(cd)),
                 ("attenuation_radius", float(radius)), ("cast_shadows", False),
                 ("light_color", unreal.Color(r=int(rgb[0] * 255), g=int(rgb[1] * 255), b=int(rgb[2] * 255), a=255))):
        try:
            c.set_editor_property(k, v)
        except Exception:                                       # noqa: BLE001
            pass
    a.set_editor_property("tags", [unreal.Name(TAG)])
    return a


def along(x, y, yaw, fwd, left=0.0):
    """The point fwd cm ahead of (x, y) facing yaw, and left cm to its left."""
    r = math.radians(yaw)
    return x + fwd * math.cos(r) - left * math.sin(r), y + fwd * math.sin(r) + left * math.cos(r)


def hide_where(pattern, x0, x1, y0, y1, z0=-50.0, z1=2000.0):
    """Hides the actors matching pattern whose centre lies in the box."""
    labels = []
    for a in C.find(pattern):
        b = C.bounds(a)
        cx, cy, cz = (b[0] + b[1]) / 2, (b[2] + b[3]) / 2, (b[4] + b[5]) / 2
        if x0 <= cx <= x1 and y0 <= cy <= y1 and z0 <= cz <= z1:
            labels.append(re.escape(a.get_actor_label()))
    return C.hide(labels, TAG) if labels else []


# --- seats -------------------------------------------------------------------------------------------------------------
def seats():
    C.hide([r"Bench_\d+", r"Prop_Chair_Instances", r"Paddle_\d+"], TAG)
    xfs = [put_xf("hall_chair", x, y, 0.0, yaw, height=L.CHAIR_H) for (x, y, yaw) in L.chair_seats()]
    C.scatter(prop_path("hall_chair"), xfs, tag=TAG, label="Hall_Chairs_Instances", shadow=False)
    # Things left on a few empty seats: auction catalogues and bidding paddles (these seats are not in hall_seats.json).
    seats_ = L.chair_seats()
    for k, i in enumerate(L.seats_with_props()):
        x, y, _yaw = seats_[i]
        sx, z = x + 6.0, L.SEAT_Z + 0.5
        spin = (k * 37) % 30 - 15.0
        if k % 2:
            obox("Catalogue", sx, y, z + 0.6, 21.0, 28.0, 1.2, "Paper", rot=spin)
            obox("Catalogue_Cover", sx, y, z + 1.25, 21.2, 28.2, 0.2, "Burgundy", rot=spin)
        else:
            cx, cy = along(sx, y, spin, -4.0)
            C.spawn(CYL, (cx, cy, z + 0.4), (0.0, 0.0, spin), (0.22, 0.22, 0.008), tag=TAG, label=_lbl("Paddle"),
                    shadow=False, mat=M["Cream"])
            hx, hy = along(sx, y, spin, 12.0)
            obox("Paddle_Handle", hx, hy, z + 0.6, 14.0, 2.6, 1.2, "Walnut", rot=spin)
    # The phone bidders' chairs (their box chairs keep the collision).
    hide_where(r"Chair_(Back_)?\d+", 1840.0, 2160.0, -1680.0, -1600.0, 0.0, 120.0)
    for (x, y) in L.PHONE_SEATS:
        put("hall_chair", x, y, 0.0, 90.0, height=L.CHAIR_H, shadow=False)
    REPORT.append("seats: %d chairs, %d with props; %s" % (len(xfs), L.PROPS_ON_SEATS, L.write()))


# --- band --------------------------------------------------------------------------------------------------------------
def band():
    C.hide([r"Prop_Piano_\d+"], TAG)                            # import_props' grand, stuck behind the shape piano
    x0, x1, y0, y1 = L.STAND
    z = L.RISER_Z
    # The bandstand grown round the old riser (3640-3920 x 1320-1600): north and east of it, the same stage floor.
    box("Bandstand", x0, x1, 1600.0, y1, 0.0, z, "Stage", collide=True, shadow=True)
    box("Bandstand", 3920.0, x1, y0, 1600.0, 0.0, z, "Stage", collide=True, shadow=True)
    dx0, dx1, dy0, dy1, dz = L.DRUM_RISER
    box("Drum_Riser", dx0, dx1, dy0, dy1, z, dz, "Stage", collide=True, shadow=True)
    # A walnut fascia and a brass nosing on the faces the house sees (west, south) and the drum riser's front.
    box("Fascia", x0 - 1.5, x0, y0, y1, 0.0, z - 2.0, "Walnut")
    box("Fascia", x0, x1, y0 - 1.5, y0, 0.0, z - 2.0, "Walnut")
    box("Fascia", x0, x1, y1, y1 + 1.5, 0.0, z - 2.0, "Walnut")
    box("Nosing", x0 - 2.0, x0 + 1.0, y0 - 2.0, y1 + 2.0, z - 2.5, z + 0.3, "Brass")
    box("Nosing", x0 - 2.0, x1, y0 - 2.0, y0 + 1.0, z - 2.5, z + 0.3, "Brass")
    box("Fascia", dx0 - 1.0, dx0, dy0, dy1, z, dz - 1.5, "Walnut")
    box("Nosing", dx0 - 1.5, dx0 + 1.0, dy0, dy1, dz - 2.0, dz + 0.3, "Brass")
    # The grand piano: lid open to the house (west), keys on its south end, the pianist on the bench facing north.
    px, py, pyaw = L.PIANO
    put("/Game/Take/Props/Piano/SM_Piano", px, py, z, pyaw, scale=1.0, front=0.0)   # keys on its +Y: yaw 180 = keys south
    bx, by, bh = L.PIANO_BENCH
    box("Piano_Bench", bx - 45, bx + 45, by - 18, by + 18, z + bh - 6.0, z + bh - 1.5, "WalnutDark", shadow=True)
    box("Piano_Bench_Pad", bx - 43, bx + 43, by - 16, by + 16, z + bh - 1.5, z + bh, "Leather")
    for sx in (-40, 40):
        for sy in (-14, 14):
            box("Piano_Bench_Leg", bx + sx - 2, bx + sx + 2, by + sy - 2, by + sy + 2, z, z + bh - 6.0, "WalnutDark")
    # The drums one step up at the back, the drummer behind them facing the house.
    sx_, sy_, sh = L.DRUM_STOOL
    put("hall_drums", sx_ - 82.0, sy_, dz, 180.0, scale=1.0)
    cyl("Drum_Stool_Seat", sx_, sy_, dz + sh - 7.0, dz + sh, 36.0, "Leather")
    cyl("Drum_Stool_Post", sx_, sy_, dz + 2.0, dz + sh - 7.0, 4.0, "Chrome")
    cyl("Drum_Stool_Foot", sx_, sy_, dz, dz + 2.0, 42.0, "Chrome")
    # The upright bass in front of its player, leaning back to him, its face to the house.
    bxx, byy, bz, byaw, _s = L.BAND["BAND_BASS"]
    qx, qy = along(bxx, byy, byaw, 32.0, 8.0)
    put("hall_bass", qx, qy, bz, byaw, height=188.0, roll=-12.0)
    # The singer's floor microphone, at the bandstand's front edge.
    sxx, syy, sz, syaw, _s = L.BAND["BAND_SINGER"]
    mx, my = along(sxx, syy, syaw, 36.0)
    put("hall_micstand", mx, my, sz, syaw + 180.0, height=158.0)
    # The trumpet's music stand: a walnut stand front with the band's monogram, a slanted desk, the part, a lamp.
    txx, tyy, tz, tyaw, _s = L.BAND["BAND_TRUMPET"]
    fx, fy = along(txx, tyy, tyaw, 52.0)
    obox("Stand_Front", fx, fy, tz + 48.0, 4.0, 58.0, 96.0, "WalnutDark", rot=tyaw, shadow=True)
    obox("Stand_Band", fx, fy, tz + 90.0, 4.6, 59.0, 4.0, "Brass", rot=tyaw)
    ex, ey = along(fx, fy, tyaw, 2.4)
    text("AQ", ex, ey, tz + 55.0, tyaw, 30.0, (0.85, 0.66, 0.30))
    dx_, dy_ = along(fx, fy, tyaw, -14.0)
    obox("Stand_Desk", dx_, dy_, tz + 102.0, 32.0, 56.0, 2.0, "WalnutDark", rot=(0.0, 35.0, tyaw))
    px_, py_ = along(fx, fy, tyaw, -15.0)
    obox("Stand_Part", px_, py_, tz + 103.6, 26.0, 40.0, 0.4, "Paper", rot=(0.0, 35.0, tyaw))
    lx, ly = along(fx, fy, tyaw, -4.0)
    obox("Stand_Lamp", lx, ly, tz + 98.0, 6.0, 30.0, 4.0, "Lamp", rot=tyaw)
    practical(lx, ly, tz + 95.0, 3.0, 150.0)
    # The PA stack (the cover from the door: its box and collision stay) dressed on the face the house sees (west) as
    # a period theatre speaker: a walnut bass cabinet with grille cloth, a multicell horn above.
    pa_x0, pa_y0, pa_y1 = 4080.0, 1210.0, 1290.0
    box("PA_Cabinet", pa_x0 - 5, pa_x0, pa_y0 + 2, pa_y1 - 2, 6.0, 135.0, "Walnut", shadow=True)
    box("PA_Grille", pa_x0 - 6, pa_x0 - 5, pa_y0 + 10, pa_y1 - 10, 16.0, 125.0, "Cloth")
    box("PA_Horn", pa_x0 - 22, pa_x0, pa_y0 + 6, pa_y1 - 6, 150.0, 222.0, "Black", shadow=True)
    for k in range(4):
        yy = pa_y0 + 6 + (pa_y1 - pa_y0 - 12) * k / 3.0
        box("PA_Cell", pa_x0 - 24, pa_x0 - 21, yy - 1.0, yy + 1.0, 150.0, 222.0, "WalnutDark")
    box("PA_Cell", pa_x0 - 24, pa_x0 - 21, pa_y0 + 6, pa_y1 - 6, 185.0, 187.0, "WalnutDark")
    REPORT.append("band: bandstand %s, piano, drums, bass, mic, stand, PA" % (L.STAND,))


# --- broadcast -----------------------------------------------------------------------------------------------------------
RISER = (480.0, 780.0, -230.0, 230.0, 80.0)   # --- W3 stage --- build_auction RISER (was 150..450)


def drop_riser_film_camera():
    """import_props put its film camera on the riser's old tripod: take that one instance out (build_auction no longer
    builds the tripod, so a full rebuild has none)."""
    n = 0
    for a in C.find(r"Prop_Camera_Instances|Prop_Camera_\d+"):
        if isinstance(a, unreal.StaticMeshActor):
            b = C.bounds(a)
            if RISER[0] <= (b[0] + b[1]) / 2 <= RISER[1] and RISER[2] <= (b[2] + b[3]) / 2 <= RISER[3]:
                C.hide([re.escape(a.get_actor_label())], TAG)
                n += 1
            continue
        for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
            for i in reversed(range(c.get_instance_count())):
                t = c.get_instance_transform(i, True).translation
                if RISER[0] <= t.x <= RISER[1] and RISER[2] <= t.y <= RISER[3]:
                    c.remove_instance(i)
                    n += 1
    return n


def cable(points, z=0.0, pair=True):
    """Two cables along an axis-aligned polyline on the floor, gaffer-taped every 160 cm."""
    for (ax, ay), (bx, by) in zip(points, points[1:]):
        along_x = abs(bx - ax) >= abs(by - ay)
        for off in ((-3.5, 3.5) if pair else (0.0,)):
            if along_x:
                box("Cable", min(ax, bx) - 2, max(ax, bx) + 2, ay + off - 1.7, ay + off + 1.7, z, z + 1.6, "Rubber")
            else:
                box("Cable", ax + off - 1.7, ax + off + 1.7, min(ay, by) - 2, max(ay, by) + 2, z, z + 1.6, "Rubber")
        ln = abs(bx - ax) + abs(by - ay)
        k = 1
        while k * 160.0 < ln - 40.0:
            t = k * 160.0 / ln
            cx, cy = ax + (bx - ax) * t, ay + (by - ay) * t
            if along_x:
                box("Gaffer", cx - 6, cx + 6, cy - 11, cy + 11, z, z + 2.0, "Tape")
            else:
                box("Gaffer", cx - 11, cx + 11, cy - 6, cy + 6, z, z + 2.0, "Tape")
            k += 1


def broadcast():
    x0, x1, y0, y1, top = RISER
    gone = drop_riser_film_camera()
    # The riser: black skirt cloth on its four faces, a pewter-dark edge.
    box("Riser_Skirt", x0 - 1.0, x0, y0, y1, 0.0, top - 2.0, "Cloth")
    box("Riser_Skirt", x1, x1 + 1.0, y0, y1, 0.0, top - 2.0, "Cloth")
    box("Riser_Skirt", x0, x1, y0 - 1.0, y0, 0.0, top - 2.0, "Cloth")
    box("Riser_Skirt", x0, x1, y1, y1 + 1.0, 0.0, top - 2.0, "Cloth")
    box("Riser_Edge", x0 - 1.5, x1 + 1.5, y0 - 1.5, y1 + 1.5, top - 2.0, top + 0.4, "Black")
    # The camera on its pedestal, aimed at the dais; the operator (a guest placeholder at 280, 40) stands behind it.
    dx = x0 - 150.0                                     # --- W3 stage --- the riser moved; everything on it follows
    put("hall_tvcamera", dx + 352.0, 40.0, top, 0.0, height=178.0)
    box("Tally", dx + 380.0, dx + 386.0, 36.0, 44.0, top + 178.0, top + 182.0, "OnAir")
    # The monitor on its stand, turned to the operator; the commentator's road case and desk microphone.
    put("hall_monitor", dx + 195.0, -150.0, top, math.degrees(math.atan2(40.0 + 150.0, 280.0 - 195.0)), height=150.0)
    put("/Game/Take/Props/Roadcase/SM_Roadcase", dx + 392.0, -178.0, top, 0.0, height=52.0, front=0.0, shadow=True)
    put("hall_mic", dx + 392.0, -178.0, C.top_of(C.find(r"Set_Hall_Roadcase_\d+")[-1]), 180.0, height=34.0)
    # Two lamps on stands beside and behind the camera (out of its shot), aimed at the dais.
    for s in (1, -1):
        put("/Game/Take/Props/Lightstand/SM_Lightstand", dx + 272.0, s * 186.0, top, 0.0, height=185.0, shadow=False)
    # ON AIR on a pole at the riser's north-west corner, lit both ways (to the house door and to the dais).
    ox, oy = dx + 165.0, 212.0
    cyl("OnAir_Pole", ox, oy, top, top + 172.0, 4.0, "Black")
    box("OnAir_Box", ox - 6, ox + 6, oy - 34, oy + 34, top + 170.0, top + 196.0, "Black")
    for s in (-1, 1):
        box("OnAir_Glass", ox + s * 6.0 - 0.5, ox + s * 6.0 + 0.5, oy - 30, oy + 30, top + 173.0, top + 193.0, "OnAir")
        text("ON AIR", ox + s * 6.8, oy, top + 183.0, 0.0 if s > 0 else 180.0, 15.0, (0.02, 0.0, 0.0))
    # The operator's step, at the riser's west face (north of the LIVE BROADCAST sign).
    box("Step", x0 - 44.0, x0, 130.0, 215.0, 0.0, 26.0, "WalnutDark", shadow=True)
    box("Step", x0 - 22.0, x0, 130.0, 215.0, 26.0, 52.0, "WalnutDark", shadow=True)
    # The cables: down the riser's south face, along its foot, between the rows, out under the staff door.
    box("Cable", dx + 196.0, dx + 200.0, y0 - 2.0, y0, 2.0, top, "Rubber")
    box("Cable", dx + 203.0, dx + 207.0, y0 - 2.0, y0, 2.0, top, "Rubber")
    cable([(dx + 201.0, -241.0), (dx + 100.0, -241.0), (dx + 100.0, -1150.0), (-300.0, -1150.0), (-300.0, -2005.0)])
    # By the dais: a microphone boom reaching over the podium from the floor below its south-west corner, and two more
    # lamps on stands aimed at the podium (none within 1.2 m of Vance, his bodyguard, the guard or a mark).
    bx, by = 3420.0, -840.0
    tip = (3700.0, -585.0, 238.0)
    yaw = math.degrees(math.atan2(tip[1] - by, tip[0] - bx))
    obox("Boom_Base", bx, by, 22.0, 96.0, 60.0, 22.0, "Black", rot=yaw, shadow=True)
    for (fw, lf) in ((40.0, 0.0), (-40.0, 26.0), (-40.0, -26.0)):
        wx, wy = along(bx, by, yaw, fw, lf)
        C.spawn(CYL, (wx, wy, 11.0), (90.0, 0.0, yaw), (0.22, 0.22, 0.06), tag=TAG, label=_lbl("Boom_Wheel"),
                shadow=False, mat=M["Rubber"])
    sx, sy = along(bx, by, yaw, -38.0)
    obox("Boom_Seat", sx, sy, 62.0, 30.0, 30.0, 6.0, "Leather", rot=yaw)
    cyl("Boom_Seat_Post", sx, sy, 33.0, 59.0, 4.0, "Chrome")
    cyl("Boom_Column", bx, by, 33.0, 205.0, 11.0, "Black")
    top_ = (bx, by, 210.0)
    back = along(bx, by, yaw, -95.0)
    rod("Boom_Arm", (back[0], back[1], 203.0), tip, 5.0, "Chrome")
    cx_, cy_ = along(back[0], back[1], yaw, 8.0)
    obox("Boom_Counterweight", cx_, cy_, 203.0, 26.0, 20.0, 20.0, "Black", rot=yaw)
    obox("Boom_Pivot", top_[0], top_[1], 207.0, 16.0, 12.0, 10.0, "Black", rot=yaw)
    rod("Boom_Hanger", tip, (tip[0], tip[1], tip[2] - 22.0), 1.2, "Black")
    cyl("Boom_Mic", tip[0], tip[1], tip[2] - 40.0, tip[2] - 22.0, 8.0, "Chrome")
    cable([(bx - 30.0, by), (bx - 30.0, -1150.0)], pair=False)
    for (lx, ly) in ((3450.0, -1060.0), (3460.0, 1010.0)):
        put("/Game/Take/Props/Lightstand/SM_Lightstand", lx, ly, 0.0,
            math.degrees(math.atan2(-480.0 - ly, 3810.0 - lx)), height=210.0, shadow=False)
    REPORT.append("broadcast: film camera instances taken off the riser %d" % gone)


# --- bar ---------------------------------------------------------------------------------------------------------------
def bar():
    C.hide([r"Prop_Bar_Instances"], TAG)                       # the four round ribbed blocks; the box keeps collision
    x0, x1, y0, y1 = 800.0, 1600.0, -1560.0, -1470.0           # build_auction's Bar box (top 110); customers on +Y
    box("Bar_Body", x0, x1, y0, y1 - 6.0, 0.0, 100.0, "WalnutDark", shadow=True)
    box("Bar_Front", x0, x1, y1 - 6.0, y1 - 2.0, 8.0, 100.0, "Walnut", shadow=True)
    box("Bar_Kick", x0, x1, y1 - 8.0, y1 - 4.0, 0.0, 8.0, "Black")
    reeds = [((x0 + 15.0 + i * 25.0, y1 - 1.0, 54.0), 0.0, (0.05, 0.05, 0.84))
             for i in range(int((x1 - x0 - 20.0) // 25.0) + 1)]
    C.scatter(CYL, reeds, tag=TAG, label="Hall_Bar_Reeds", mat=M["Walnut"])
    box("Bar_Band", x0, x1, y1 - 2.0, y1 + 1.5, 92.0, 96.0, "Brass")
    box("Bar_Top", x0 - 8.0, x1 + 8.0, y0 - 6.0, y1 + 10.0, 100.0, 110.0, "Marble", shadow=True)
    box("Bar_Top_Edge", x0 - 9.0, x1 + 9.0, y1 + 9.0, y1 + 11.0, 100.0, 110.0, "Brass")
    C.spawn(CYL, ((x0 + x1) / 2, y1 + 22.0, 18.0), (0.0, 90.0, 0.0), (0.05, 0.05, (x1 - x0 - 30.0) / 100.0), tag=TAG,
            label=_lbl("Bar_Rail"), shadow=False, mat=M["Brass"])
    for k in range(5):
        rx = x0 + 25.0 + k * (x1 - x0 - 50.0) / 4.0
        box("Bar_Rail_Bracket", rx - 1.5, rx + 1.5, y1, y1 + 22.0, 16.0, 20.0, "Brass")
    # A coupe tower at the west end, an ice bucket with a bottle, a tray of flutes at the east end.
    cx, cy, z = 880.0, -1512.0, 110.0
    bowls, stems = [], []
    for tier, n in enumerate((3, 2, 1)):
        for i in range(n):
            for j in range(n):
                gx, gy = cx + (i - (n - 1) / 2.0) * 11.0, cy + (j - (n - 1) / 2.0) * 11.0
                gz = z + tier * 13.0
                stems.append(((gx, gy, gz + 4.5), 0.0, (0.012, 0.012, 0.09)))
                bowls.append(((gx, gy, gz + 10.5), 0.0, (0.105, 0.105, 0.035)))
    C.scatter(CYL, stems, tag=TAG, label="Hall_Coupe_Stems", mat=M["Glass"])
    C.scatter(SPH, bowls, tag=TAG, label="Hall_Coupe_Bowls", mat=M["Glass"])
    cyl("Ice_Bucket", 1010.0, -1512.0, 110.0, 132.0, 21.0, "Chrome", shadow=True)
    cyl("Ice", 1010.0, -1512.0, 131.0, 133.0, 19.0, "Glass")
    C.spawn(CYL, (1012.0, -1510.0, 140.0), (0.0, 12.0, 30.0), (0.075, 0.075, 0.30), tag=TAG, label=_lbl("Bottle"),
            shadow=False, mat=M["Foliage"])
    cyl("Tray", 1460.0, -1515.0, 110.0, 111.5, 38.0, "Chrome")
    flutes = [((1460.0 + 11.0 * math.cos(a), -1515.0 + 11.0 * math.sin(a), 120.5), 0.0, (0.05, 0.05, 0.18))
              for a in [k * 2.0 * math.pi / 6 for k in range(6)]] + [((1460.0, -1515.0, 120.5), 0.0, (0.05, 0.05, 0.18))]
    C.scatter(CYL, flutes, tag=TAG, label="Hall_Flutes", mat=M["Glass"])
    for lx in (1000.0, 1400.0):                                # the bar's own light, over the top
        practical(lx, -1540.0, 240.0, 14.0, 420.0)
    REPORT.append("bar: fluted walnut bar, %d coupes, bucket, tray" % len(bowls))


# --- phones ------------------------------------------------------------------------------------------------------------
def phones():
    C.hide([r"Phone_\d+"], TAG)                                # the three glowing boxes
    x0, x1, y0, y1, top = 1800.0, 2200.0, -1560.0, -1480.0, 100.0
    box("Desk_Baize", x0 - 4, x1 + 4, y0 - 4, y1 + 4, 0.0, top + 1.5, "Baize", shadow=True)
    box("Desk_Hem", x0 - 4.5, x1 + 4.5, y0 - 4.5, y1 + 4.5, 0.0, 3.0, "Brass")
    z = top + 1.5
    for (px, _py) in L.PHONE_SEATS:
        put("hall_phone", px, -1532.0, z, 270.0, height=17.0)
        obox("Pad", px + 24.0, -1528.0, z + 0.5, 15.0, 21.0, 1.0, "Paper", rot=8.0)
        C.spawn(CYL, (px + 24.0, -1540.0, z + 1.6), (0.0, 90.0, 20.0), (0.008, 0.008, 0.15), tag=TAG,
                label=_lbl("Pencil"), shadow=False, mat=M["Black"])
    for lx in (x0 + 30.0, x1 - 30.0):
        cyl("Lamp_Foot", lx, -1500.0, z, z + 2.0, 13.0, "Brass")
        cyl("Lamp_Stem", lx, -1500.0, z + 2.0, z + 38.0, 1.6, "Brass")
        cyl("Lamp_Shade", lx, -1500.0, z + 34.0, z + 44.0, 18.0, "Baize", dia_y=12.0)
        cyl("Lamp_Glow", lx, -1500.0, z + 33.0, z + 34.0, 15.0, "Lamp", dia_y=10.0)
        practical(lx, -1505.0, z + 30.0, 8.0, 260.0)
    obox("Placard", 2000.0, -1487.0, z + 9.0, 76.0, 3.0, 18.0, "WalnutDark", rot=0.0)
    text("TELEPHONE BIDS", 2000.0, -1484.3, z + 9.0, 90.0, 7.5, (0.85, 0.66, 0.30))
    REPORT.append("phones: baize desk, 3 telephones, pads, lamps, placard")


# --- lots ----------------------------------------------------------------------------------------------------------------
def lots():
    x0, x1, y0, y1, z0, top = 4190.0, 4350.0, 600.0, 800.0, 20.0, 115.0   # build_auction LOTS (on the dais)
    hide_where(r"Lot_\d+", x0, x1, y0, y1, top - 5.0, top + 80.0)
    box("Lots_Cloth", x0 - 4, x1 + 4, y0 - 4, y1 + 4, z0, top + 1.0, "Burgundy", shadow=True)
    box("Lots_Fringe", x0 - 4.5, x1 + 4.5, y0 - 4.5, y1 + 4.5, z0 + 6.0, z0 + 9.0, "Brass")
    z = top + 1.0
    put("/Game/Take/Props/Bust/SM_Bust", 4240.0, 650.0, z, 180.0, height=36.0, front=0.0, shadow=True)
    put("/Game/Take/Props/Vase/SM_Vase", 4300.0, 748.0, z, 180.0, height=30.0, front=0.0, shadow=True)
    box("Jewel_Case", 4285.0, 4315.0, 632.0, 668.0, z, z + 10.0, "Walnut", shadow=True)
    box("Jewel_Case_Lid", 4284.0, 4316.0, 631.0, 669.0, z + 10.0, z + 11.5, "Brass")
    for (cx, cy, s) in ((4212.0, 650.0, "LOT 39"), (4212.0, 748.0, "LOT 40"), (4270.0, 700.0, "LOT 42")):
        obox("Lot_Card", cx, cy, z + 3.5, 0.6, 14.0, 7.0, "Paper", rot=(0.0, -12.0, 0.0))
        text(s, cx - 1.2, cy, z + 3.6, 180.0, 3.4, (0.05, 0.04, 0.03))
    # Urns of flowers at the dais's front corners.
    for s in (1, -1):
        ux, uy = 3660.0, s * 1150.0
        put("/Game/Take/Props/Vase/SM_Vase", ux, uy, 20.0, 180.0, height=72.0, front=0.0, shadow=True)
        C.spawn(SPH, (ux, uy, 20.0 + 72.0 + 26.0), 0.0, (0.80, 0.80, 0.62), tag=TAG, label=_lbl("Urn_Foliage"),
                shadow=False, mat=M["Foliage"])
        for k in range(7):
            a = k * 2.0 * math.pi / 7
            C.spawn(SPH, (ux + 28.0 * math.cos(a), uy + 28.0 * math.sin(a), 20.0 + 72.0 + 34.0 + (k % 3) * 9.0), 0.0,
                    0.16, tag=TAG, label=_lbl("Urn_Flower"), shadow=False, mat=M["Cream"])
    REPORT.append("lots: table dressed, urns on the dais")


# --- tables --------------------------------------------------------------------------------------------------------------
def tables():
    """Stands every glass by a Rodin cocktail table in the hall on the table's real top."""
    tops = []
    for a in C.find(r"Prop_Cocktail_Instances|Prop_Cocktail_\d+"):
        comps = a.get_components_by_class(unreal.InstancedStaticMeshComponent) or [a.static_mesh_component]
        for c in comps:
            mesh = c.get_editor_property("static_mesh")
            bb = mesh.get_bounding_box()
            xfs = [c.get_instance_transform(i, True) for i in range(c.get_instance_count())] \
                if isinstance(c, unreal.InstancedStaticMeshComponent) else [a.get_actor_transform()]
            for t in xfs:
                p = t.translation
                if -1000.0 <= p.x <= 4400.0 and -2000.0 <= p.y <= 2000.0 and p.z < 300.0:
                    tops.append((p.x, p.y, p.z + bb.max.z * t.scale3d.z))
    moved = 0
    labels = []
    for g in C.find(r"Glass_\d+"):
        b = C.bounds(g)
        gx, gy = (b[0] + b[1]) / 2, (b[2] + b[3]) / 2
        near = [t for t in tops if math.hypot(t[0] - gx, t[1] - gy) < 45.0]
        if not near or abs(b[4] - near[0][2]) < 1.0:
            continue
        labels.append(re.escape(g.get_actor_label()))
        cyl("Glass", gx, gy, near[0][2], near[0][2] + 14.0, 6.0, "Glass")
        moved += 1
    if labels:
        C.hide(labels, TAG)
    REPORT.append("tables: %d cocktail tables in the hall, %d glasses stood on them" % (len(tops), moved))


# --- extras ------------------------------------------------------------------------------------------------------------
PALMS = ((-880.0, 1880.0), (-880.0, -1880.0), (1550.0, 1880.0), (2450.0, -1880.0), (-650.0, -1880.0))


def extras():
    """Kentia palms in the hall's corners and wall bays (package front's Rodin palm), clear of marks, doors and devices."""
    path = "/Game/Take/Props/Front_palm/SM_Front_palm"
    if not eal.does_asset_exist(path):
        REPORT.append("extras: no %s yet (package front imports it)" % path)
        return
    for k, (x, y) in enumerate(PALMS):
        put(path, x, y, 0.0, k * 70.0, height=230.0, shadow=False)
    REPORT.append("extras: %d palms" % len(PALMS))


# --- dais (W3 stage) ---------------------------------------------------------------------------------------------------------
# The focus of the room: the big screen framed like a picture house's (black masking, a gilt frame, burgundy legs and a
# pelmet), the lot's plinth in walnut and brass under its sign, the auctioneer's lectern dressed, a runner from the
# dais edge to the case, two torcheres flanking it. Numbers from tools/sets/hall_zones.json. No collision anywhere.
def dais():
    with open(os.path.join(os.path.dirname(os.path.abspath(L.__file__)), "hall_zones.json"), encoding="utf-8") as f:
        Z = json.load(f)
    sx, _sy, sz = Z["screen"]["centre"]
    w, h = Z["screen"]["size"]
    hw, z0, z1 = w / 2.0, sz - h / 2.0, sz + h / 2.0
    dz = Z["dais"]["z"]
    face = sx + 2.0                                            # the screen box's face (heist lights sx, 2 cm in front)
    # Masking 60 cm round the picture, a gilt frame round that.
    for s in (1, -1):
        y0, y1 = sorted((s * hw, s * (hw + 60.0)))
        box("Screen_Mask", face - 4, face, y0, y1, z0 - 60.0, z1 + 60.0, "Cloth")
        y0, y1 = sorted((s * (hw + 60.0), s * (hw + 70.0)))
        box("Screen_Frame", face - 9, face, y0, y1, z0 - 70.0, z1 + 70.0, "Brass")
    box("Screen_Mask", face - 4, face, -hw, hw, z1, z1 + 60.0, "Cloth")
    box("Screen_Mask", face - 4, face, -hw, hw, z0 - 60.0, z0, "Cloth")
    box("Screen_Frame", face - 9, face, -hw - 70.0, hw + 70.0, z1 + 60.0, z1 + 70.0, "Brass")
    box("Screen_Frame", face - 9, face, -hw - 70.0, hw + 70.0, z0 - 70.0, z0 - 60.0, "Brass")
    # Burgundy legs either side, pleated (two depths, two tones), floor to pelmet; the pelmet with a brass fringe.
    top = z1 + 220.0
    for s in (1, -1):
        for i in range(11):
            y = s * (hw + 90.0 + i * 24.0)
            deep = i % 2
            box("Drape", face - 30 - deep * 10, face - 14 - deep * 10, y - 13, y + 13, dz, top,
                "BurgundyDark" if deep else "Burgundy")
    pw = hw + 90.0 + 11 * 24.0
    box("Pelmet", face - 46, face - 10, -pw, pw, z1 + 100.0, top + 30.0, "Burgundy")
    box("Pelmet_Fringe", face - 48, face - 44, -pw, pw, z1 + 92.0, z1 + 100.0, "Brass")
    cyl("Pelmet_Crest", face - 50, 0.0, z1 + 130.0, z1 + 190.0, 60.0, "Brass")
    # The lot's plinth in walnut, a brass cap and plaque; the runner from the dais edge to it.
    cx, cy, ctop = Z["case"]
    box("Case_Plinth_Skin", cx - 38, cx + 38, cy - 38, cy + 38, dz, ctop - 3.0, "Walnut", shadow=True)
    box("Case_Plinth_Cap", cx - 41, cx + 41, cy - 41, cy + 41, ctop - 3.0, ctop - 0.5, "Brass")
    box("Case_Plinth_Base", cx - 42, cx + 42, cy - 42, cy + 42, dz, dz + 8.0, "WalnutDark")
    box("Case_Plaque", cx - 40, cx - 38.5, cy - 16, cy + 16, ctop - 40.0, ctop - 24.0, "Brass")
    text("LOT 41", cx - 40.6, cy, ctop - 32.0, 180.0, 5.0, (0.05, 0.035, 0.02))
    dx0 = Z["dais"]["min"][0]
    box("Dais_Runner", dx0 + 4, cx - 45, cy - 95, cy + 95, dz, dz + 0.6, "Burgundy")
    for s in (1, -1):
        box("Dais_Runner_Edge", dx0 + 4, cx - 45, s * 95 - 3, s * 95 + 3, dz, dz + 0.8, "Brass")
    # Two torcheres flanking the lot, warm on it.
    for s in (1, -1):
        tx, ty = cx + 140.0, s * 260.0
        cyl("Torchere_Foot", tx, ty, dz, dz + 4.0, 40.0, "Brass")
        cyl("Torchere_Stem", tx, ty, dz + 4.0, dz + 190.0, 5.0, "Brass")
        cyl("Torchere_Bowl", tx, ty, dz + 186.0, dz + 200.0, 36.0, "Brass")
        cyl("Torchere_Glow", tx, ty, dz + 199.0, dz + 200.5, 30.0, "Lamp")
        practical(tx, ty, dz + 230.0, 25.0, 600.0)
    # The auctioneer's lectern: walnut skin on the podium box, brass edge, a crest to the house, lamp and gavel.
    px, py = Z["podium"]
    box("Lectern", px - 34, px + 34, py - 44, py + 44, dz, dz + 118.0, "Walnut", shadow=True)
    box("Lectern_Top", px - 38, px + 38, py - 48, py + 48, dz + 118.0, dz + 123.0, "WalnutDark")
    box("Lectern_Edge", px - 39, px + 39, py - 49, py + 49, dz + 116.0, dz + 118.0, "Brass")
    C.spawn(CYL, (px - 35.0, py, dz + 80.0), (0.0, 90.0, 0.0), (0.34, 0.34, 0.02), tag=TAG, label=_lbl("Lectern_Crest"),
            shadow=False, mat=M["Brass"])
    cyl("Lectern_Lamp_Stem", px + 20.0, py + 30.0, dz + 123.0, dz + 158.0, 2.0, "Brass")
    cyl("Lectern_Lamp_Shade", px + 14.0, py + 30.0, dz + 152.0, dz + 160.0, 16.0, "Baize", dia_y=9.0)
    practical(px + 10.0, py + 30.0, dz + 148.0, 6.0, 220.0)
    C.spawn(CYL, (px + 5.0, py - 25.0, dz + 126.0), (0.0, 90.0, 30.0), (0.05, 0.05, 0.22), tag=TAG,
            label=_lbl("Gavel"), shadow=False, mat=M["WalnutDark"])
    REPORT.append("dais: screen masked and framed %dx%d, legs and pelmet, plinth, runner, torcheres, lectern" % (w, h))


# --- run -------------------------------------------------------------------------------------------------------------------
gone, shown = C.clear(TAG)
REPORT.append("cleared %d, shown %d" % (gone, shown))
if not OFF:
    materials()
    for part in PARTS:
        try:
            globals()[part]()
        except Exception as e:                                  # noqa: BLE001 (one broken part must not stop the rest)
            REPORT.append("FAIL %s: %s" % (part, traceback.format_exc().strip().splitlines()[-1][:300]))
            unreal.log_error("[hall] %s\n%s" % (part, traceback.format_exc()))
C.save_level()
"\n".join(REPORT)
