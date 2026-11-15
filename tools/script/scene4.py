# Scene 4 overlay (s4-catwalk): the catwalk's bird's-eye camera, the champagne cart at the West Pillar, the tracking
# shot's camera, Nora's start spot, and the scene actor (AScene4Catwalk) at the rope gap with the guards, devices and
# Vance's lane. docs/script/s4.md. Re-runnable: clears its tag (script_s4), then builds again. Run under the editor
# lock, after the existing pipeline and after tools/script/scene3.py (the rope guards are Scene 3's):
#     "<engine python>" -I tools/ue_mcp.py py tools/script/scene4.py 110
# Switches: S4_OFF = True only clears. S4_SHOTS = True also writes stills to C:\Jam\Builds\script\s4.
# S4_PLAN = True also writes a top-down plan of the lane's area (to lay a lane that clears every prop).
# Also removes the drone (ADroneWatcher and its two path points) from the level: Marwan's QA, Nov 15 evening.
import sys, os, importlib, math
sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C
importlib.reload(C)
import unreal

TAG = "script_s4"
OFF = bool(globals().get("S4_OFF"))
SHOTS = bool(globals().get("S4_SHOTS"))
PLAN = bool(globals().get("S4_PLAN"))
OUT = r"C:\Jam\Builds\script\s4"
ADIR = "/Game/Take/Audio/Script/S4"
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
REPORT = []

# --- numbers (world cm; the hall: x -1000..4400, y -2000..2000, the rope at x 2400 with its gap at y 0) --------------
GAP = (2400.0, 0.0)                     # the scene actor: the threshold (curtain, doors) in the rope gap, facing +X
NORA = (650.0, -290.0, 900.0 + 96.0)    # Maya at the follow spot's use point on the lighting platform (deck z 900)
BIRDS_EYE = ((820.0, -420.0, 1240.0), (2050.0, -200.0, 0.0), 82.0)     # pos, look-at, fov: over the rail, down
CART = (1700.0, -1880.0, 0.0, 90.0)     # the service cart at the south wall by the bar's end, facing +Y (yaw)
CART_ROLL = (0.0, 0.0, 0.0)             # set below: its crash offset, local (it rolls along its +X)
CRASH = (1960.0, -1520.0)               # the mess: at the West Pillar (the column at 2200, -1300), short of the guards
# Vance's lane: from the bar (where Max told him to hold) across the hall, between the two gap guards, through the
# curtain, into the dark aisle. The last point is inside the auditorium.
# It keeps west of the seated benches (x 1900-2100, y +-200..470), then takes the centre aisle to the gap.
LANE = ((1450.0, -1320.0), (1700.0, -1080.0), (1540.0, -680.0), (1700.0, -150.0), (2300.0, -40.0), (2420.0, 0.0), (2720.0, 0.0))  # clears the bar column, the ice plinth (east) and the cocktail table (west); between the seat blocks
# Nora's rigs (AScene4Catwalk builds them at play from these points). The lamp hangs under a pipe batten in front of
# the East Door guard, lit on a harmless patch of floor until its clamp is loosened; the sandbag hangs on a batten
# straight over the cart's latch; Nora's own catwalk runs under her camera.
LAMP_AT = (1960.0, 160.0, 520.0)
LAMP_PARK = (1720.0, 520.0, 0.0)
BATTEN = (CART[0], CART[1], 560.0)
CATWALK = ((960.0, -1700.0, 900.0), (960.0, 1000.0, 900.0))
BEAM_MAT = "/Game/Take/Look/M_S4Beam"
# The guards (Scene 3 places them; labels from tools/script/scene3.py): label -> role, cone name, pace.
GUARDS = (
    ("S3_Guard_NorthGap", "EAST_DOOR", "EAST DOOR", (2260.0, 380.0), (0.0, 250.0), 0.0),
    ("S3_Guard_SouthArch", "WEST_PILLAR", "WEST PILLAR", (2260.0, -1480.0), (0.0, 250.0), 1.5),
    ("S3_Guard_SouthGap", "GUARD1", "GUARD 1", (2260.0, -620.0), (0.0, 250.0), 3.0),
    ("S3_Guard_NorthArch", "GUARD2", "GUARD 2", (2260.0, 1230.0), (0.0, 250.0), 4.5),
)
VOICES = {"nora_open_voice": "s4_nora_open", "guard1_spot_voice": "s4_guard1_spot", "nora_go_voice": "s4_nora_go",
          "guard1_hold_voice": "s4_guard1_hold", "guard_spotted_voice": "s4_guard_spotted",
          "nora_retake_voice": "s4_nora_retake"}


def tag(a, extra=()):
    C._set_tags(a, [TAG] + list(extra))
    a.set_folder_path("Script/S4")


def vec(t):
    return unreal.Vector(float(t[0]), float(t[1]), float(t[2]) if len(t) > 2 else 0.0)


def look_rot(pos, at):
    return unreal.MathLibrary.find_look_at_rotation(vec(pos), vec(at))


def camera(label, pos, at, fov):
    cam = eas.spawn_actor_from_class(unreal.CameraActor, vec(pos), look_rot(pos, at))
    cam.set_actor_label(label)
    cc = cam.get_editor_property("camera_component")
    cc.set_editor_property("field_of_view", float(fov))
    cc.set_editor_property("constrain_aspect_ratio", False)
    tag(cam)
    return cam


def cart():
    """A champagne service cart of its own (the hall's carts are Scene 6's side of the rope)."""
    x, y, z, yaw = CART
    c = eas.spawn_actor_from_class(unreal.CartDevice, vec((x, y, z)), unreal.Rotator(0.0, 0.0, yaw))
    c.set_actor_label("S4_ChampagneCart")
    # The crash offset, local to the cart (its +X is its way).
    dx, dy = CRASH[0] - x, CRASH[1] - y
    r = math.radians(yaw)
    lx = dx * math.cos(r) + dy * math.sin(r)
    ly = -dx * math.sin(r) + dy * math.cos(r)
    c.set_editor_property("crash_offset", unreal.Vector(lx, ly, 0.0))
    c.set_editor_property("roll_seconds", 1.6)
    c.set_editor_property("range", 1.0)          # the director lures nobody with it: Scene 4 moves its guards itself
    c.set_editor_property("device_name", "CHAMPAGNE CART")
    # The period model of the hall's own cart, if there is one.
    src = [a for a in C.actors() if isinstance(a, unreal.CartDevice) and a.get_actor_label() == "Cart"]
    if src:
        for p in ("model_mesh", "model_offset", "model_rotation", "model_scale", "use_model", "model_hides_body"):
            try:
                c.set_editor_property(p, src[0].get_editor_property(p))
            except Exception as e:
                REPORT.append("cart: %s not copied (%s)" % (p, e))
    tag(c)
    REPORT.append("cart at (%.0f, %.0f) -> crash (%.0f, %.0f), model from Cart: %s" % (x, y, CRASH[0], CRASH[1], bool(src)))
    return c


def nearest_lane(px, py):
    """The nearest point of the lane's hall side (x short of the gap) to (px, py)."""
    best, bd = None, 1e18
    for (ax, ay), (bx, by) in zip(LANE, LANE[1:]):
        for k in range(21):
            t = k / 20.0
            x, y = ax + (bx - ax) * t, ay + (by - ay) * t
            if x >= GAP[0] - 60.0:
                continue
            d = (x - px) ** 2 + (y - py) ** 2
            if d < bd:
                best, bd = (x, y), d
    return best, math.sqrt(bd)


def guards():
    out = []
    by_label = {a.get_actor_label(): a for a in C.actors() if isinstance(a, unreal.PatrolGuard)}
    for label, role, name, post, pace, phase in GUARDS:
        g = unreal.Scene4Guard()
        a = by_label.get(label)
        if a is None:
            REPORT.append("guard %s NOT FOUND (run scene3.py first)" % label)
        g.set_editor_property("actor", a)
        g.set_editor_property("role", getattr(unreal.Scene4Role, role))
        g.set_editor_property("label", name)
        g.set_editor_property("post", vec(post))
        # His cone is aimed at the stretch of the lane nearest the middle of his pace, long enough to reach past it:
        # every guard watches the lane until something takes him off it.
        mx, my = post[0] + pace[0] * 0.5, post[1] + pace[1] * 0.5
        (lx, ly), d = nearest_lane(mx, my)
        yaw = math.degrees(math.atan2(ly - my, lx - mx))
        g.set_editor_property("yaw", yaw)
        g.set_editor_property("pace", vec(pace))
        g.set_editor_property("sweep", 8.0)
        g.set_editor_property("period", 7.0)
        g.set_editor_property("phase", float(phase))
        g.set_editor_property("cone_range", max(1100.0, d * 1.5))
        g.set_editor_property("cone_half_angle", 30.0)
        REPORT.append("%s %s: yaw %.0f at lane (%.0f, %.0f) %.0f cm" % (name, label, yaw, lx, ly, d))
        out.append(g)
    return out


def remove_drone():
    """The drone and its two path points, gone from the level (the old flow's watcher; nothing in the script uses it)."""
    gone = []
    for a in C.actors():
        lab = a.get_actor_label()
        if isinstance(a, unreal.DroneWatcher) or lab.startswith("Drone_Path_"):
            gone.append(lab)
            eas.destroy_actor(a)
    REPORT.append("drone removed: %s" % (", ".join(gone) or "none left"))


def beam_material():
    """M_S4Beam: unlit, additive, two-sided; Color (vector) x Opacity (scalar) -> emissive. The lamp's visible shaft."""
    if unreal.EditorAssetLibrary.does_asset_exist(BEAM_MAT):
        return unreal.load_asset(BEAM_MAT)
    pkg, name = BEAM_MAT.rsplit("/", 1)
    m = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, pkg, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    mel = unreal.MaterialEditingLibrary
    col = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -600, 0)
    col.set_editor_property("parameter_name", "Color")
    col.set_editor_property("default_value", unreal.LinearColor(1.0, 0.9, 0.72, 1.0))
    op = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -600, 200)
    op.set_editor_property("parameter_name", "Opacity")
    op.set_editor_property("default_value", 0.22)
    # Softer at the cone's grazing edges: a fresnel turned around (bright face-on, gone at the silhouette).
    fr = mel.create_material_expression(m, unreal.MaterialExpressionFresnel, -600, 350)
    one = mel.create_material_expression(m, unreal.MaterialExpressionOneMinus, -400, 350)
    mel.connect_material_expressions(fr, "", one, "")
    mul = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -350, 100)
    mel.connect_material_expressions(col, "", mul, "A")
    mel.connect_material_expressions(op, "", mul, "B")
    mul2 = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -150, 150)
    mel.connect_material_expressions(mul, "", mul2, "A")
    mel.connect_material_expressions(one, "", mul2, "B")
    mel.connect_material_property(mul2, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m)
    unreal.EditorAssetLibrary.save_asset(BEAM_MAT, only_if_is_dirty=False)
    REPORT.append("made %s" % BEAM_MAT)
    return m


def check_lane():
    """Sweeps Vance's capsule (r 40, half-height 88) along each stretch of the lane against walls and props; reports
    what it would bump. The tracking shot walks him on his own feet, so the lane has to clear everything."""
    w = unreal.EditorLevelLibrary.get_editor_world()
    types = [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1, unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY2]
    bad = 0
    for i, ((ax, ay), (bx, by)) in enumerate(zip(LANE, LANE[1:])):
        a, b = unreal.Vector(ax, ay, 100.0), unreal.Vector(bx, by, 100.0)
        try:
            hits = unreal.SystemLibrary.capsule_trace_multi_for_objects(w, a, b, 40.0, 88.0, types, False, [],
                                                                        unreal.DrawDebugTrace.NONE, True)
        except Exception as e:
            REPORT.append("lane check failed: %s" % e)
            return
        if isinstance(hits, tuple):
            hits = hits[-1] if hits and isinstance(hits[-1], (list, tuple)) else []
        for h in hits or []:
            t = h.to_tuple()
            actor = next((x for x in t if isinstance(x, unreal.Actor)), None)
            loc = next((x for x in t if isinstance(x, unreal.Vector)), None)
            lab = actor.get_actor_label() if actor else "?"
            if lab.startswith("S4_") or "Floor" in lab:
                continue
            bad += 1
            REPORT.append("LANE %d->%d bumps %s at (%.0f, %.0f)" % (i, i + 1, lab, loc.x if loc else 0, loc.y if loc else 0))
    REPORT.append("lane check: %d bumps" % bad)


def scene():
    sp = eas.spawn_actor_from_class(unreal.TargetPoint, vec(NORA), unreal.Rotator(0.0, -30.0, 20.0))
    sp.set_actor_label("S4_StartSpot_Nora")
    tag(sp)
    bird = camera("S4_BirdsEyeCam", BIRDS_EYE[0], BIRDS_EYE[1], BIRDS_EYE[2])
    track = camera("S4_TrackCam", (LANE[0][0] - 100.0, LANE[0][1] - 260.0, 160.0), (LANE[0][0], LANE[0][1], 120.0), 55.0)
    ct = cart()
    # The catwalk's follow spot: the one nearest Nora (the hall also has a far, short-range "Searchlight").
    spots = sorted([a for a in C.actors() if isinstance(a, unreal.FollowSpot)],
                   key=lambda a: (a.get_actor_location() - vec(NORA)).length())
    cls = getattr(unreal, "Scene4Catwalk", None)
    if cls is None:
        REPORT.append("AScene4Catwalk not in this editor build yet: scene actor NOT placed (rerun after the restart)")
        return None
    s = eas.spawn_actor_from_class(cls, vec((GAP[0], GAP[1], 0.0)), unreal.Rotator(0.0, 0.0, 0.0))
    s.set_actor_label("S4_Scene4Catwalk")
    s.set_editor_property("start_spot", sp)
    s.set_editor_property("scene_camera", bird)
    s.set_editor_property("birds_eye_camera", bird)
    s.set_editor_property("track_camera", track)
    s.set_editor_property("cart", ct)
    s.set_editor_property("spot", None)     # Nora's own lamp (built by the scene) is the rigged spot now
    s.set_editor_property("lamp_at", vec(LAMP_AT))
    s.set_editor_property("lamp_parked_aim", vec(LAMP_PARK))
    s.set_editor_property("batten_at", vec(BATTEN))
    s.set_editor_property("catwalk_from", vec(CATWALK[0]))
    s.set_editor_property("catwalk_to", vec(CATWALK[1]))
    s.set_editor_property("beam_material", beam_material())
    s.set_editor_property("lane", [vec(p) for p in LANE])
    s.set_editor_property("guards", guards())
    for prop, name in VOICES.items():
        snd = unreal.load_asset("%s/%s" % (ADIR, name)) if unreal.EditorAssetLibrary.does_asset_exist("%s/%s" % (ADIR, name)) else None
        if snd:
            try:
                s.set_editor_property(prop, snd)
            except Exception as e:      # a property newer than this editor build
                REPORT.append("voice %s not set (%s)" % (prop, e))
    tag(s)
    REPORT.append("scene actor placed: spot=%s (%d follow spots), voices=%d" % (
        spots[0].get_actor_label() if spots else None, len(spots),
        sum(1 for n in VOICES.values() if unreal.EditorAssetLibrary.does_asset_exist("%s/%s" % (ADIR, n)))))
    return s


def shots():
    os.makedirs(OUT, exist_ok=True)
    REPORT.append(C.shot("s4_birds_eye", BIRDS_EYE[0], BIRDS_EYE[1], fov=BIRDS_EYE[2], out_dir=OUT))
    REPORT.append(C.shot("s4_cart_pillar", (1500.0, -1500.0, 260.0), (1900.0, -1700.0, 60.0), fov=70.0, out_dir=OUT))
    REPORT.append(C.shot("s4_threshold", (2000.0, -300.0, 220.0), (2420.0, 0.0, 170.0), fov=65.0, out_dir=OUT))


gone, shown = C.clear(TAG)
REPORT.append("cleared %d, shown %d" % (gone, shown))
remove_drone()
if not OFF:
    scene()
    check_lane()
    if PLAN:
        os.makedirs(OUT, exist_ok=True)
        REPORT.append(C.plan("s4_plan", 1000.0, 2800.0, -2000.0, 600.0, 400.0, out_dir=OUT))
    if SHOTS:
        shots()
"\n".join(str(r) for r in REPORT)
