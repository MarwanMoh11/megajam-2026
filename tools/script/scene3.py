# Scene 3 overlay (s3-foyer): the foyer's critic, the main hall's ice sculptures, the four rope guards with
# clipboards, the scene actor (AScene3Foyer) and its start spot. docs/script/s3.md. Re-runnable: clears its tag
# (script_s3) and shows what it hid, then builds again. Run under the editor lock, after the existing pipeline:
#     "<engine python>" -I tools/ue_mcp.py py tools/script/scene3.py 110
# Switches: S3_OFF = True only clears. S3_SHOTS = True also writes stills to C:\Jam\Builds\script\s3.
import sys, importlib, math, os
sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C
importlib.reload(C)
import unreal

TAG = "script_s3"
OFF = bool(globals().get("S3_OFF"))
SHOTS = bool(globals().get("S3_SHOTS"))
OUT = r"C:\Jam\Builds\script\s3"
SDIR = "/Game/Take/Script/S3"
HDIR = "/Game/Take/Sets/Hall"
CUBE, CYL, SPH, CONE = ("/Engine/BasicShapes/Cube", "/Engine/BasicShapes/Cylinder", "/Engine/BasicShapes/Sphere",
                        "/Engine/BasicShapes/Cone")
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
REPORT = []

# --- numbers (docs/script/s3.md) --------------------------------------------------------------------------------------
STAIR_X0, STAIR_X1, STAIR_TOP = -2600.0, -1500.0, 450.0
CRITIC = (-2420.0, 90.0, 180.0)                         # x, y, yaw: four steps up the grand stair, facing down it
START = (-3020.0, 220.0, 0.0)
CRITIC_FROM = ("Cast_Guest_016", "Cast_Guest_012", "Cast_Guest_011")   # lobby tails guests                           # Vance in from the doors (feet; the spot is +96)
GUARD_X = 2260.0                                        # between the general rows (end x 2130) and the rope (2400)
GUARDS = (                                              # label, y0, y1 (a tight loop along the rope), s4 role tag
    ("S3_Guard_SouthArch", -1550.0, -1150.0, "S3_WestPillar"),
    ("S3_Guard_SouthGap", -700.0, -300.0, ""),
    ("S3_Guard_NorthGap", 300.0, 700.0, "S3_EastDoor"),
    ("S3_Guard_NorthArch", 1150.0, 1550.0, ""),
)
ICE = ((300.0, -860.0, 20.0), (300.0, 860.0, -20.0), (1500.0, -1060.0, 35.0), (1500.0, 1060.0, -35.0))   # x, y, yaw


def stair_z(x, y):
    if abs(y) > 240.0 or x <= STAIR_X0:
        return 0.0
    return min(max((x - STAIR_X0) / (STAIR_X1 - STAIR_X0), 0.0), 1.0) * STAIR_TOP


def tag(a, extra=()):
    C._set_tags(a, [TAG] + list(extra))
    a.set_folder_path("Script/S3")
    return a


# --- materials --------------------------------------------------------------------------------------------------------
def ice_material():
    path = SDIR + "/M_S3_Ice"
    if eal.does_asset_exist(path):
        return eal.load_asset(path)
    at = unreal.AssetToolsHelpers.get_asset_tools()
    m = at.create_asset("M_S3_Ice", SDIR, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("two_sided", True)
    try:
        m.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE)
    except Exception:                                           # noqa: BLE001
        pass

    def const3(rgb, x, y):
        e = mel.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, x, y)
        e.set_editor_property("constant", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
        return e

    def const(v, x, y):
        e = mel.create_material_expression(m, unreal.MaterialExpressionConstant, x, y)
        e.set_editor_property("r", float(v))
        return e

    mel.connect_material_property(const3((0.62, 0.84, 1.0), -500, -200), "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(const3((0.05, 0.09, 0.13), -500, -100), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(const(0.04, -500, 0), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(const(1.0, -500, 60), "", unreal.MaterialProperty.MP_SPECULAR)
    fres = mel.create_material_expression(m, unreal.MaterialExpressionFresnel, -700, 200)
    lerp = mel.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -400, 200)
    mel.connect_material_expressions(const(0.28, -700, 120), "", lerp, "A")
    mel.connect_material_expressions(const(0.8, -700, 160), "", lerp, "B")
    mel.connect_material_expressions(fres, "", lerp, "Alpha")
    mel.connect_material_property(lerp, "", unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(m)
    eal.save_asset(path, only_if_is_dirty=False)
    return m


M = {}


def materials():
    for k in ("Brass", "Burgundy", "Walnut", "Cream", "Black"):
        M[k] = eal.load_asset(HDIR + "/MI_Hall_" + k)
    M["Ice"] = ice_material()


def piece(mesh, loc, rot_, scale, mat, label, collide=False):
    a = C.spawn(mesh, loc, rot_, scale, tag=TAG, label=label, collide=collide, shadow=False, mat=mat)
    a.set_folder_path("Script/S3")
    return a


# --- the critic -------------------------------------------------------------------------------------------------------
def critic():
    """A copy of a dressed tails guest from the cast pipeline (a bare spawned CastMember has no look: an all-tint
    mannequin), moved four steps up the stair with a coupe."""
    x, y, yaw = CRITIC
    src = [a for a in C.actors() if isinstance(a, unreal.CastMember) and a.get_actor_label() in CRITIC_FROM
           and TAG not in C.tags(a)]
    src.sort(key=lambda a: CRITIC_FROM.index(a.get_actor_label()))
    if src:
        m = eas.duplicate_actor(src[0], C.world(), unreal.Vector(0.0, 0.0, 0.0))
        m.set_actor_location_and_rotation(unreal.Vector(x, y, stair_z(x, y)), unreal.Rotator(0.0, 0.0, yaw), False, False)
        came = src[0].get_actor_label()
    else:
        m = eas.spawn_actor_from_class(unreal.CastMember, unreal.Vector(x, y, stair_z(x, y)), unreal.Rotator(0.0, 0.0, yaw))
        m.set_editor_property("kit", unreal.CastKit.GUEST_TAILS)
        came = "a bare spawn"
    m.set_actor_label("S3_Critic")
    m.set_editor_property("holds_glass", True)
    tag(m, ["NoSetLife"])
    REPORT.append("critic at (%.0f, %.0f, %.0f), copied from %s" % (x, y, stair_z(x, y), came))
    return m


# --- the ice sculptures ------------------------------------------------------------------------------------------------
def sculpture(i, x, y, yaw):
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    n = "S3_Ice%d_" % i
    piece(CUBE, (x, y, 42.5), yaw, (1.2, 1.2, 0.85), M["Cream"], n + "Plinth", collide=True)
    piece(CUBE, (x, y, 110.0), yaw, (0.9, 0.9, 0.5), M["Ice"], n + "Block")
    piece(CONE, (x, y, 245.0), yaw, (0.8, 0.8, 2.2), M["Ice"], n + "Body")
    for side in (1.0, -1.0):                                    # two swept wings, either side of the body
        ox, oy = -s * side * 48.0, c * side * 48.0
        piece(CUBE, (x + ox, y + oy, 285.0), (side * 28.0, 0.0, yaw), (0.06, 0.95, 1.6), M["Ice"], n + "Wing")
    piece(SPH, (x, y, 372.0), yaw, (0.42, 0.42, 0.5), M["Ice"], n + "Head")
    lt = eas.spawn_actor_from_class(unreal.PointLight, unreal.Vector(x, y, 150.0), unreal.Rotator(0.0, 0.0, 0.0))
    lt.set_actor_label(n + "Glow")
    lc = lt.point_light_component
    lc.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    lc.set_intensity(260.0)
    lc.set_light_color(unreal.LinearColor(0.55, 0.78, 1.0, 1.0))
    lc.set_editor_property("attenuation_radius", 420.0)
    lc.set_editor_property("cast_shadows", False)
    lc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    tag(lt)


# --- the rope guards ---------------------------------------------------------------------------------------------------
def guard(label, y0, y1, role):
    yaw = 180.0                                                 # facing the hall (west)
    g = eas.spawn_actor_from_class(unreal.PatrolGuard, unreal.Vector(GUARD_X, y0, 0.0), unreal.Rotator(0.0, 0.0, yaw))
    g.set_actor_label(label)
    pts = []
    for j, (yy, st) in enumerate(((y0, "CHECKING THE LIST"), (y1, "CHECKING THE LIST"))):
        p = eas.spawn_actor_from_class(unreal.PatrolPoint, unreal.Vector(GUARD_X, yy, 0.0), unreal.Rotator(0.0, 0.0, yaw))
        p.set_actor_label("%s_P%d" % (label, j))
        p.set_editor_property("bars", 2)
        p.set_editor_property("state_text", st)
        tag(p)
        pts.append(p)
    g.set_editor_property("patrol_points", pts)
    g.set_editor_property("label", "PENDLETON GUARD")
    tag(g, ["script_s3_guard"] + ([role] if role else []))
    m = eas.spawn_actor_from_class(unreal.CastMember, unreal.Vector(GUARD_X, y0, 0.0), unreal.Rotator(0.0, 0.0, yaw))
    m.set_actor_label("S3_Cast_" + label[3:])
    m.set_editor_property("kit", unreal.CastKit.GUARD)
    m.set_editor_property("tint", g.get_editor_property("body_tint"))
    m.attach_to_actor(g, "", unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD,
                      unreal.AttachmentRule.KEEP_WORLD, False)
    tag(m, ["NoSetLife"])
    # The clipboard, in front of his right hand, tilted up to his eyes (yaw 180: front -X, right -Y).
    fx, fy, rx, ry = -1.0, 0.0, 0.0, -1.0
    bx, by, bz = GUARD_X + fx * 30.0 + rx * 14.0, y0 + fy * 30.0 + ry * 14.0, 112.0
    for what, sc, dz, mat in (("Board", (0.24, 0.32, 0.015), 0.0, M["Walnut"]), ("Paper", (0.20, 0.27, 0.004), 1.0, M["Cream"])):
        b = piece(CUBE, (bx, by, bz + dz), (0.0, -35.0, yaw), sc, mat, "%s_Clip%s" % (label, what))
        b.attach_to_actor(g, "", unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD,
                          unreal.AttachmentRule.KEEP_WORLD, False)
    REPORT.append("%s: x %.0f, y %.0f..%.0f %s" % (label, GUARD_X, y0, y1, role))
    return g


# --- the scene actor ---------------------------------------------------------------------------------------------------
def scene(critic_actor, guards):
    sp = eas.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(START[0], START[1], 96.0),
                                    unreal.Rotator(0.0, 0.0, START[2]))
    sp.set_actor_label("S3_StartSpot")
    tag(sp)
    cls = getattr(unreal, "Scene3Foyer", None)
    if cls is None:
        REPORT.append("AScene3Foyer not in this editor build yet: scene actor NOT placed (rerun after the restart)")
        return None
    s = eas.spawn_actor_from_class(cls, unreal.Vector(START[0], START[1], 0.0), unreal.Rotator(0.0, 0.0, 0.0))
    s.set_actor_label("S3_Scene3Foyer")
    s.set_editor_property("start_spot", sp)
    s.set_editor_property("critic", critic_actor)
    s.set_editor_property("rope_guards", guards)
    s.set_editor_property("critic_speed", 170.0)
    s.set_editor_property("finish_after_vo", 10.5)          # Max's V.O. runs 9.6 s
    chief = [a for a in C.actors() if a.get_actor_label() == "Chief" and isinstance(a, unreal.PatrolGuard)]
    s.set_editor_property("hide_during_scene", chief)
    voiced = []
    for prop, asset in (("critic_sound", "S3_Critic"), ("play_along_sound", "S3_PlayAlong"),
                        ("play_along_reply_sound", "S3_PlayAlongReply"), ("brush_off_sound", "S3_BrushOff"),
                        ("brush_off_reply_sound", "S3_BrushOffReply"), ("max_vo_sound", "S3_MaxVo"),
                        ("guard_tackle_sound", "S3_GuardTackle"), ("guard_alibi_sound", "S3_GuardAlibi"),
                        ("splash_sound", "S3_Splash"), ("desk_sound", "S3_Desk"),
                        ("desk_alibi_sound", "S3_DeskAlibi"), ("desk_again_sound", "S3_DeskAgain")):
        path = "/Game/Take/Audio/Script/S3/" + asset
        if eal.does_asset_exist(path):
            s.set_editor_property(prop, eal.load_asset(path))
            voiced.append(asset)
    REPORT.append("voices: %d (%s)" % (len(voiced), ", ".join(voiced) or "none yet: subtitles only"))
    tag(s)
    REPORT.append("scene actor placed; hides %d (the chief at the stair's foot)" % len(chief))
    return s


def shots():
    os.makedirs(OUT, exist_ok=True)
    REPORT.append(C.shot("s3_foyer_stair", (-3100.0, 500.0, 220.0), (-2400.0, 60.0, 160.0), fov=60.0, out_dir=OUT))
    REPORT.append(C.shot("s3_hall_ice", (-700.0, 0.0, 260.0), (1500.0, 0.0, 200.0), fov=70.0, out_dir=OUT))
    REPORT.append(C.shot("s3_rope_guards", (1500.0, 300.0, 230.0), (2300.0, 0.0, 120.0), fov=75.0, out_dir=OUT))


gone, shown = C.clear(TAG)
REPORT.append("cleared %d, shown %d" % (gone, shown))
if not OFF:
    materials()
    cr = critic()
    for i, (x, y, yaw) in enumerate(ICE):
        sculpture(i, x, y, yaw)
    gs = [guard(*g) for g in GUARDS]
    scene(cr, gs)
    if SHOTS:
        shots()
"\n".join(str(r) for r in REPORT)
