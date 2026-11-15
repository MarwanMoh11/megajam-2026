# Scene 5 (INT. PRIVATE AUDITORIUM) set dressing: the great hall east of the velvet rope as Pendleton's private
# screening of LE SOLEIL NOIR (1974). An overlay on the built level, after the tools/sets pipeline (puzzle.py's `sale`
# part letters the film's title card on the screen). Under the editor lock:
#     "<engine python>" -I tools/ue_mcp.py py tools/script/scene5_dress.py 110
# Idempotent: puts back the materials it changed, shows what it hid, clears its own tag (script_s5_dress), then builds.
#   signage  every auction text east of the rope (LOT 41 on the dais and the case plaque, the lot cards) is hidden;
#            the dais sign and the plaque are re-lettered for the film
#   velvet   the rows' chairs (Hall_Chairs_Instances) in deep crimson velvet (MI_S5_Velvet: a little fresnel sheen)
#   podium   Podium and hall.py's lectern skin in polished brass (MI_S5_Brass)
#   door     the heavy oak door to the Backstage & Projection Stairwell on the south wall at x 3650: architrave,
#            two raised panels, brass kick plate, push plate and lever, a brass sign, and the electronic deadbolt's
#            keypad at (3745, -1984, 125) with an empty lamp housing on top (the C++ AScene5Auditorium adds the
#            blinking red lamp at (3745, -1980, 140)). The slab blocks the pawn.
#   material M_S5_TitleText: unlit, masked, emissive = vertex colour x font mask x Glow (6), for TextRenderComponents
# Materials changed on existing actors are kept in the actor's tags (script_s5_dress:mat=comp#slot=path|...).
# Switches: S5_DRESS_OFF = True only clears; S5_PARTS = (...) runs only those parts; S5_SHOTS = True also takes the
# shots into C:\Jam\Builds\script\s5\; S5_REBUILD_MATERIALS = True rebuilds the two masters.
import sys, importlib, re, traceback
sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C
importlib.reload(C)
import unreal

TAG = "script_s5_dress"
PARTS = globals().get("S5_PARTS") or ("material", "signage", "velvet", "podium", "door")
OFF = bool(globals().get("S5_DRESS_OFF"))
SHOTS = bool(globals().get("S5_SHOTS"))
REBUILD = bool(globals().get("S5_REBUILD_MATERIALS"))
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
S5 = "/Game/Take/Script/S5"
CUBE, CYL = "/Engine/BasicShapes/Cube", "/Engine/BasicShapes/Cylinder"
FONT = "/Engine/EngineFonts/RobotoDistanceField"
REPORT = []
N = [0]
MT = TAG + ":mat="

ROPE_X, HALL_X1, HY = 2400.0, 4400.0, 2000.0


# --- materials -------------------------------------------------------------------------------------------------------------
def _new(name, cls, factory):
    path = S5 + "/" + name
    if eal.does_asset_exist(path):
        return eal.load_asset(path), False
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, S5, cls, factory), True


def _param(m, cls, name, x, y, default=None):
    e = mel.create_material_expression(m, cls, x, y)
    e.set_editor_property("parameter_name", name)
    if default is not None:
        e.set_editor_property("default_value", default)
    return e


def _lc(rgb):
    return unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0)


def master_lit():
    """M_S5_Lit: Color + fresnel sheen (SheenColor, SheenPower), Roughness, Metallic, Specular."""
    m, made = _new("M_S5_Lit", unreal.Material, unreal.MaterialFactoryNew())
    if made or REBUILD:
        mel.delete_all_material_expressions(m)
        col = _param(m, unreal.MaterialExpressionVectorParameter, "Color", -700, -300, _lc((0.2, 0.2, 0.2)))
        sheen = _param(m, unreal.MaterialExpressionVectorParameter, "SheenColor", -700, -100, _lc((0.0, 0.0, 0.0)))
        sp = _param(m, unreal.MaterialExpressionScalarParameter, "SheenPower", -900, 0, 3.0)
        fres = mel.create_material_expression(m, unreal.MaterialExpressionFresnel, -700, 50)
        try:
            mel.connect_material_expressions(sp, "", fres, "ExponentIn")
        except Exception:                                       # noqa: BLE001
            fres.set_editor_property("exponent", 3.0)
        mul = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -450, -50)
        mel.connect_material_expressions(fres, "", mul, "A")
        mel.connect_material_expressions(sheen, "", mul, "B")
        add = mel.create_material_expression(m, unreal.MaterialExpressionAdd, -250, -200)
        mel.connect_material_expressions(col, "", add, "A")
        mel.connect_material_expressions(mul, "", add, "B")
        mel.connect_material_property(add, "", unreal.MaterialProperty.MP_BASE_COLOR)
        for i, (k, v, p) in enumerate((("Roughness", 0.6, "MP_ROUGHNESS"), ("Metallic", 0.0, "MP_METALLIC"),
                                       ("Specular", 0.5, "MP_SPECULAR"))):
            e = _param(m, unreal.MaterialExpressionScalarParameter, k, -450, 150 + 90 * i, v)
            mel.connect_material_property(e, "", getattr(unreal.MaterialProperty, p))
        mel.recompile_material(m)
        eal.save_asset(S5 + "/M_S5_Lit", only_if_is_dirty=False)
    return m


def mi(name, parent, color, rough, metal=0.0, spec=0.5, sheen=(0.0, 0.0, 0.0), sheen_power=3.0):
    m, _ = _new(name, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(m, parent)
    for k, v in (("Color", color), ("SheenColor", sheen)):
        mel.set_material_instance_vector_parameter_value(m, k, _lc(v))
    for k, v in (("Roughness", rough), ("Metallic", metal), ("Specular", spec), ("SheenPower", sheen_power)):
        mel.set_material_instance_scalar_parameter_value(m, k, float(v))
    mel.update_material_instance(m)
    eal.save_asset(S5 + "/" + name, only_if_is_dirty=False)
    return m


M = {}


def materials():
    lit = master_lit()
    M["Velvet"] = mi("MI_S5_Velvet", lit, (0.13, 0.006, 0.012), 0.93, spec=0.15, sheen=(0.45, 0.05, 0.07),
                     sheen_power=2.5)
    M["Brass"] = mi("MI_S5_Brass", lit, (0.78, 0.55, 0.20), 0.22, metal=1.0)
    M["Oak"] = mi("MI_S5_Oak", lit, (0.050, 0.024, 0.011), 0.55, spec=0.4)
    M["OakDark"] = mi("MI_S5_OakDark", lit, (0.024, 0.012, 0.006), 0.5, spec=0.4)
    M["Steel"] = mi("MI_S5_Steel", lit, (0.035, 0.035, 0.038), 0.38, metal=0.8)
    M["Keys"] = mi("MI_S5_Keys", lit, (0.10, 0.10, 0.105), 0.45, metal=0.6)


def material():
    """M_S5_TitleText: the C++ title lettering. Unlit, masked on the font; emissive = vertex colour x mask x Glow."""
    m, made = _new("M_S5_TitleText", unreal.Material, unreal.MaterialFactoryNew())
    if not (made or REBUILD):
        REPORT.append("material: M_S5_TitleText kept")
        return
    mel.delete_all_material_expressions(m)
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("opacity_mask_clip_value", 0.5)
    m.set_editor_property("two_sided", True)
    font = mel.create_material_expression(m, unreal.MaterialExpressionFontSampleParameter, -800, 100)
    font.set_editor_property("parameter_name", "Font")
    font.set_editor_property("font", eal.load_asset(FONT))
    font.set_editor_property("font_texture_page", 0)
    vc = mel.create_material_expression(m, unreal.MaterialExpressionVertexColor, -800, -200)
    glow = _param(m, unreal.MaterialExpressionScalarParameter, "Glow", -800, -50, 6.0)
    a = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -500, -150)
    mel.connect_material_expressions(vc, "", a, "A")
    mel.connect_material_expressions(glow, "", a, "B")
    b = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -250, -50)
    mel.connect_material_expressions(a, "", b, "A")
    mel.connect_material_expressions(font, FONT_MASK, b, "B")
    mel.connect_material_property(b, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(font, FONT_MASK, unreal.MaterialProperty.MP_OPACITY_MASK)
    mel.recompile_material(m)
    eal.save_asset(S5 + "/M_S5_TitleText", only_if_is_dirty=False)
    REPORT.append("material: M_S5_TitleText built (mask %s)" % FONT_MASK)


FONT_MASK = "A"


# --- remembering what was changed ------------------------------------------------------------------------------------------
def remat(a, mat):
    """Every static mesh slot of a to mat; the old materials go in the actor's tags (once)."""
    ts = C.tags(a)
    comps = a.get_components_by_class(unreal.StaticMeshComponent)
    if not any(t.startswith(MT) for t in ts):
        old = []
        for c in comps:
            for i in range(c.get_num_materials()):
                mm = c.get_material(i)
                old.append("%s#%d=%s" % (c.get_name(), i, mm.get_path_name() if mm else ""))
        C._set_tags(a, ts + [MT + "|".join(old)])
    for c in comps:
        for i in range(c.get_num_materials()):
            c.set_material(i, mat)
    return len(comps)


def restore():
    n = 0
    for a in C.actors():
        ts = C.tags(a)
        mt = [t for t in ts if t.startswith(MT)]
        if not mt:
            continue
        comps = {c.get_name(): c for c in a.get_components_by_class(unreal.StaticMeshComponent)}
        for item in mt[0][len(MT):].split("|"):
            if "=" not in item:
                continue
            key, path = item.split("=", 1)
            name, slot = key.rsplit("#", 1)
            if name in comps:
                comps[name].set_material(int(slot), eal.load_asset(path) if path else None)
        C._set_tags(a, [t for t in ts if not t.startswith(MT)])
        n += 1
    return n


def hide_actor(a):
    comps = [c for c in a.get_components_by_class(unreal.PrimitiveComponent) if c.is_visible()]
    for c in comps:
        c.set_visibility(False, False)
    if comps:
        C._set_tags(a, C.tags(a) + ["%s:hid=%s" % (TAG, "|".join(c.get_name() for c in comps))])
    return bool(comps)


# --- spawning helpers ------------------------------------------------------------------------------------------------------
def _lbl(what):
    N[0] += 1
    return "S5_%s_%03d" % (what, N[0])


def box(what, x0, x1, y0, y1, z0, z1, mat, collide=False, shadow=True):
    return C.spawn(CUBE, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), 0.0,
                   ((x1 - x0) / 100.0, (y1 - y0) / 100.0, (z1 - z0) / 100.0), tag=TAG, label=_lbl(what),
                   collide=collide, shadow=shadow, mat=M[mat])


def cyl(what, x, y, z0, z1, dia, mat, rot=0.0, shadow=False):
    """A cylinder dia across, z0..z1 along its axis when rot is 0; rot turns it about its centre."""
    return C.spawn(CYL, (x, y, (z0 + z1) / 2), rot, (dia / 100.0, dia / 100.0, (z1 - z0) / 100.0), tag=TAG,
                   label=_lbl(what), shadow=shadow, mat=M[mat])


TEXT_PROPS = ("world_size", "text_render_color", "horizontal_alignment", "vertical_alignment", "x_scale", "y_scale",
              "font")


def text(s, loc, rot, size=None, rgb=None, like=None):
    """A TextRenderActor; like: a TextRenderComponent to copy size, colour, alignment and font from."""
    N[0] += 1
    a = eas.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(*loc), rot)
    a.set_actor_label("Set_S5_Text_%03d" % N[0])
    a.set_folder_path("Sets/" + TAG)
    c = a.get_component_by_class(unreal.TextRenderComponent)
    try:
        c.set_text(s)
    except Exception:                                           # noqa: BLE001
        c.set_editor_property("text", s)
    props = []
    if like is not None:
        for k in TEXT_PROPS:
            try:
                props.append((k, like.get_editor_property(k)))
            except Exception:                                   # noqa: BLE001
                pass
        try:
            c.set_material(0, like.get_material(0))
        except Exception:                                       # noqa: BLE001
            pass
    else:
        props = [("world_size", float(size)),
                 ("text_render_color", unreal.Color(r=int(rgb[0] * 255), g=int(rgb[1] * 255), b=int(rgb[2] * 255),
                                                    a=255)),
                 ("horizontal_alignment", unreal.HorizTextAligment.EHTA_CENTER),
                 ("vertical_alignment", unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)]
    for k, v in props:
        try:
            c.set_editor_property(k, v)
        except Exception:                                       # noqa: BLE001
            pass
    a.set_editor_property("tags", [unreal.Name(TAG)])
    return a


# --- signage ---------------------------------------------------------------------------------------------------------------
AUCTION = re.compile(r"LOT\b|\bBID|SELLING|PADDLE|AUCTION|SALE\b|DIRECTOR'S CUT|SOLD", re.I)
RELETTER = {"LOT 41: THE DIRECTOR'S CUT": "LE SOLEIL NOIR  \u00b7  1974", "LOT 41": "No. 1974"}


def signage():
    hidden, relettered = [], []
    for a in C.actors():
        if not isinstance(a, unreal.TextRenderActor):
            continue
        ts = C.tags(a)
        if TAG in ts or "set_puzzle" in ts:                     # the slide is puzzle.py's own (its `sale` part)
            continue
        l = a.get_actor_location()
        if not (ROPE_X < l.x < HALL_X1 + 50.0 and abs(l.y) < HY):
            continue
        c = a.get_component_by_class(unreal.TextRenderComponent)
        s = str(c.get_editor_property("text"))
        if not AUCTION.search(s) or not hide_actor(a):
            continue
        hidden.append("%s %r" % (a.get_actor_label(), s))
        if s.strip() in RELETTER:
            text(RELETTER[s.strip()], (l.x, l.y, l.z), a.get_actor_rotation(), like=c)
            relettered.append(s.strip())
    REPORT.append("signage: hid %d (%s); re-lettered %s" % (len(hidden), "; ".join(hidden), relettered))


# --- velvet ----------------------------------------------------------------------------------------------------------------
def velvet():
    done = []
    for a in C.find(r"Set_Hall_Chairs_Instances(_\d+)?|Hall_Chairs_Instances(_\d+)?"):
        o, e = a.get_actor_bounds(False)
        if o.x + e.x < 1900.0 or o.x - e.x > 3150.0:
            continue
        remat(a, M["Velvet"])
        done.append(a.get_actor_label())
    REPORT.append("velvet: %s" % (done or "NO chair instances found"))


# --- podium ----------------------------------------------------------------------------------------------------------------
def podium():
    done = []
    for a in C.find(r"Podium|Set_Hall_Lectern(_Top|_Edge|_Crest)?_\d+"):
        remat(a, M["Brass"])
        done.append(a.get_actor_label())
    REPORT.append("podium: brass on %s" % done)


# --- door ------------------------------------------------------------------------------------------------------------------
DX, WALL = 3650.0, -2000.0            # the door's centre line; the south wall's hall face
DW, DH = 140.0, 260.0
KEYPAD = (3745.0, -1984.0, 125.0)     # 12 x 6 x 18; the C++ lamp sits at (3745, -1980, 140)


def door():
    x0, x1 = DX - DW / 2, DX + DW / 2                          # 3580 .. 3720
    W, F = WALL, -1985.0                                        # wall face, architrave face
    # The architrave: legs and head 15 cm wide, an inner bead at the reveal, plinth blocks, a cornice over the head.
    for (a0, a1) in ((x0 - 15.0, x0), (x1, x1 + 15.0)):
        box("Door_Architrave", a0, a1, W, F, 0.0, DH + 15.0, "OakDark")
    box("Door_Architrave", x0 - 15.0, x1 + 15.0, W, F, DH, DH + 15.0, "OakDark")
    for (a0, a1) in ((x0 - 3.0, x0), (x1, x1 + 3.0)):
        box("Door_Bead", a0, a1, W, F + 2.0, 0.0, DH, "Oak", shadow=False)
    box("Door_Bead", x0 - 3.0, x1 + 3.0, W, F + 2.0, DH - 3.0, DH, "Oak", shadow=False)
    for (a0, a1) in ((x0 - 18.0, x0 + 1.0), (x1 - 1.0, x1 + 18.0)):
        box("Door_Plinth", a0, a1, W, F + 2.0, 0.0, 24.0, "OakDark")
    box("Door_Cornice", x0 - 25.0, x1 + 25.0, W, F + 5.0, DH + 15.0, DH + 23.0, "OakDark")
    box("Door_Cornice_Bead", x0 - 22.0, x1 + 22.0, W, F + 3.0, DH + 23.0, DH + 26.0, "Oak", shadow=False)
    # The slab, recessed 7 cm behind the architrave's face; it blocks the pawn.
    S = -1992.0
    box("Door_Slab", x0, x1, W, S, 0.0, DH, "Oak", collide=True)
    # Two raised panels (a short one low, a tall one high), each in a dark moulding.
    for (z0, z1) in ((24.0, 100.0), (122.0, 236.0)):
        p0, p1 = x0 + 16.0, x1 - 16.0
        box("Door_Panel", p0 + 5.0, p1 - 5.0, S, S + 2.0, z0 + 5.0, z1 - 5.0, "Oak", shadow=False)
        for (a0, a1, b0, b1) in ((p0, p1, z0, z0 + 5.0), (p0, p1, z1 - 5.0, z1), (p0, p0 + 5.0, z0, z1),
                                 (p1 - 5.0, p1, z0, z1)):
            box("Door_Moulding", a0, a1, S, S + 3.5, b0, b1, "OakDark", shadow=False)
    # Brass: kick plate, push plate, a lever on a rose with an escutcheon (the handle side is east, by the keypad).
    box("Door_KickPlate", x0 + 6.0, x1 - 6.0, S, S + 0.8, 3.0, 21.0, "Brass", shadow=False)
    hx = x1 - 18.0
    box("Door_PushPlate", hx - 6.0, hx + 6.0, S, S + 0.8, 138.0, 178.0, "Brass", shadow=False)
    box("Door_Escutcheon", hx - 5.0, hx + 5.0, S, S + 1.0, 96.0, 124.0, "Brass", shadow=False)
    C.spawn(CYL, (hx, S + 3.0, 112.0), (90.0, 0.0, 0.0), (0.065, 0.065, 0.05), tag=TAG, label=_lbl("Door_Rose"),
            shadow=False, mat=M["Brass"])                       # roll 90: the cylinder's axis along Y, off the door
    box("Door_Lever", hx - 20.0, hx + 1.0, S + 4.0, S + 7.0, 110.0, 114.0, "Brass", shadow=True)
    C.spawn(CYL, (hx, S + 1.2, 102.0), (90.0, 0.0, 0.0), (0.022, 0.022, 0.006), tag=TAG, label=_lbl("Door_Keyhole"),
            shadow=False, mat=M["OakDark"])
    # The sign above: a brass plate on the wall, dark letters.
    box("Door_Sign", DX - 66.0, DX + 66.0, W, W + 2.5, DH + 34.0, DH + 54.0, "Brass", shadow=False)
    text("BACKSTAGE  \u00b7  PROJECTION", (DX, W + 3.2, DH + 44.0), unreal.Rotator(0.0, 0.0, 90.0), size=7.5,
         rgb=(0.05, 0.03, 0.015))
    # The electronic deadbolt: a dark steel keypad on a mounting block, a key grid, an empty lamp housing on top.
    kx, ky, kz = KEYPAD
    box("Deadbolt_Mount", kx - 5.0, kx + 5.0, W, ky - 3.0, kz - 7.0, kz + 7.0, "Steel", shadow=False)
    box("Deadbolt_Keypad", kx - 6.0, kx + 6.0, ky - 3.0, ky + 3.0, kz - 9.0, kz + 9.0, "Steel", shadow=True)
    keys = []
    for r in range(4):
        for col in range(3):
            keys.append(((kx - 3.2 + 3.2 * col, ky + 3.3, kz + 3.0 - 3.0 * r), 0.0, (0.022, 0.006, 0.02)))
    C.scatter(CUBE, keys, tag=TAG, label="S5_Deadbolt_Keys", shadow=False, mat=M["Keys"])
    box("Deadbolt_Display", kx - 4.0, kx + 4.0, ky + 3.0, ky + 3.4, kz + 5.0, kz + 7.5, "OakDark", shadow=False)
    C.spawn(CYL, (kx, -1981.0, kz + 9.0 + 2.0), 0.0, (0.05, 0.05, 0.04), tag=TAG, label=_lbl("Deadbolt_LampHousing"),
            shadow=False, mat=M["Steel"])
    REPORT.append("door: oak door %d x %d at x %.0f (slab blocks the pawn), sign, keypad at %s" % (DW, DH, DX, KEYPAD))


# --- shots -----------------------------------------------------------------------------------------------------------------
SHOT_DIR = r"C:\Jam\Builds\script\s5"
SHOT_LIST = {
    "door": ((3300, -1500, 170), (3650, -2000, 130)),
    "slide": ((2750, -330, 150), (4388, 0, 600)),
    "rows": ((3350, -820, 240), (2550, -250, 40)),
    "podium": ((3480, -760, 175), (3810, -480, 110)),
}


def shots():
    want = globals().get("S5_SHOT_NAMES") or tuple(SHOT_LIST)
    for k in want:
        REPORT.append(C.shot("s5_" + k, SHOT_LIST[k][0], SHOT_LIST[k][1], fov=70, w=1280, h=720, out_dir=SHOT_DIR))


# --- run -------------------------------------------------------------------------------------------------------------------
_restored = restore()
gone, shown = C.clear(TAG)
REPORT.append("cleared %d, shown %d, materials restored on %d" % (gone, shown, _restored))
if not OFF:
    for part in PARTS:
        try:
            if part != "material" and not M:
                materials()
            globals()[part]()
        except Exception as e:                                  # noqa: BLE001 (one broken part must not stop the rest)
            REPORT.append("FAIL %s: %s" % (part, traceback.format_exc().strip().splitlines()[-1][:300]))
            unreal.log_error("[scene5_dress] %s\n%s" % (part, traceback.format_exc()))
C.save_level()
if SHOTS:
    shots()
"\n".join(REPORT)
