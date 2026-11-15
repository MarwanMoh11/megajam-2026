# Scene 4 overlay (s4-catwalk): the catwalk's bird's-eye camera, the champagne cart at the West Pillar, the tracking
# shot's camera, Nora's start spot, and the scene actor (AScene4Catwalk) at the rope gap with the guards, devices and
# Vance's lane. docs/script/s4.md. Re-runnable: clears its tag (script_s4), then builds again. Run under the editor
# lock, after the existing pipeline and after tools/script/scene3.py (the rope guards are Scene 3's):
#     "<engine python>" -I tools/ue_mcp.py py tools/script/scene4.py 110
# Switches: S4_OFF = True only clears. S4_SHOTS = True also writes stills to C:\Jam\Builds\script\s4.
import sys, importlib, math
sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C
importlib.reload(C)
import unreal

TAG = "script_s4"
OFF = bool(globals().get("S4_OFF"))
SHOTS = bool(globals().get("S4_SHOTS"))
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
LANE = ((1250.0, -1330.0), (1700.0, -900.0), (2100.0, -330.0), (2300.0, -40.0), (2420.0, 0.0), (2720.0, 0.0))
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


def scene():
    sp = eas.spawn_actor_from_class(unreal.TargetPoint, vec(NORA), unreal.Rotator(0.0, -30.0, 20.0))
    sp.set_actor_label("S4_StartSpot_Nora")
    tag(sp)
    bird = camera("S4_BirdsEyeCam", BIRDS_EYE[0], BIRDS_EYE[1], BIRDS_EYE[2])
    track = camera("S4_TrackCam", (LANE[0][0] - 100.0, LANE[0][1] - 260.0, 160.0), (LANE[0][0], LANE[0][1], 120.0), 55.0)
    ct = cart()
    spots = [a for a in C.actors() if isinstance(a, unreal.FollowSpot)]
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
    s.set_editor_property("spot", spots[0] if spots else None)
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
    import os
    os.makedirs(OUT, exist_ok=True)
    REPORT.append(C.shot("s4_birds_eye", BIRDS_EYE[0], BIRDS_EYE[1], fov=BIRDS_EYE[2], out_dir=OUT))
    REPORT.append(C.shot("s4_cart_pillar", (1500.0, -1500.0, 260.0), (1900.0, -1700.0, 60.0), fov=70.0, out_dir=OUT))
    REPORT.append(C.shot("s4_threshold", (2000.0, -300.0, 220.0), (2420.0, 0.0, 170.0), fov=65.0, out_dir=OUT))


gone, shown = C.clear(TAG)
REPORT.append("cleared %d, shown %d" % (gone, shown))
if not OFF:
    scene()
    if SHOTS:
        shots()
"\n".join(str(r) for r in REPORT)
