# Scene 5, INT. PRIVATE AUDITORIUM (s5-auditorium): places AScene5Auditorium and what it drives. Tag script_s5.
# Idempotent overlay, run after the level pipeline (and after scene5_dress.py, the set dressing). Under the editor lock:
#     "<engine python>" -I tools/ue_mcp.py py tools/script/scene5.py 110
# S5_OFF = True only clears. Needs the editor built with Scene5Auditorium (a new UCLASS).
#   scene     Script_S5_Auditorium (SceneNumber 5), its StartSpot (Vance's aisle seat, south end of a reserved row) and
#             SceneCamera (Pendleton's close shot from the front floor)
#   pendleton the level's old auctioneer "Vance" PatrolGuard at the podium (top hat, tails, cane) plays Pendleton
#   watchers  an usher at the front floor's south corner and a guard at the rope's south end, each with a torch the
#             C++ sweeps (Script_S5_Usher, Script_S5_Guard: standalone CastMembers)
#   the door  the oak door and its deadbolt are scene5_dress.py's; the C++ owns the red lamp at DoorLampAt
import sys, importlib
sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C
importlib.reload(C)
import unreal

TAG = "script_s5"
OFF = bool(globals().get("S5_OFF"))
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
EAL = unreal.EditorAssetLibrary
NPC = "/Game/Take/Cast/NPCs/%s/SKM_NPC_%s"
REPORT = []

# World cm (hall_zones.json): rows x 1900..3150, chairs |y| <= 583; dais x 3600..4400; south wall face y -2000.
SCENE_AT = (3650.0, -1900.0, 0.0)
START = (2650.0, -690.0, 100.0, 20.0)           # Vance at his aisle seat, looking up at the stage
CAMERA = (3330.0, -260.0, 205.0)                # Pendleton's close shot, from the front floor
PENDLETON_AT = (3890.0, -480.0, 170.0)
DOOR_MARK = (3650.0, -1900.0, 0.0)
DOOR_LAMP = (3745.0, -1980.0, 140.0)
BOOTH_PORT = (-600.0, 150.0, 600.0)            # s7-booth's port in the projection box over the lobby doors
# name, npc mesh, kit, x, y, yaw0, sweep a, sweep b, seconds, phase, speaker
WATCHERS = [
    ("Usher", "Clerk", "Staff", 3420.0, -1050.0, -110.0, -150.0, -70.0, 7.0, 0.0, "USHER"),
    ("Guard", "Guard", "Guard", 2500.0, -1760.0, 20.0, -8.0, 58.0, 8.0, 0.5, "GUARD"),
]


def by_label(name, cls=None):
    for a in C.actors():
        if a.get_actor_label() == name and (cls is None or isinstance(a, cls)):
            return a
    return None


def tagged(a):
    a.set_folder_path("Script/S5")
    C._set_tags(a, C.tags(a) + [TAG])
    return a


def build():
    scene = tagged(EAS.spawn_actor_from_class(unreal.Scene5Auditorium, unreal.Vector(*SCENE_AT), unreal.Rotator(0, 0, 0)))
    scene.set_actor_label("Script_S5_Auditorium")

    spot = tagged(EAS.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(*START[:3]), unreal.Rotator(0.0, 0.0, START[3])))
    spot.set_actor_label("Script_S5_Start")

    cam = tagged(EAS.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(*CAMERA), unreal.Rotator(0, 0, 0)))
    cam.set_actor_label("Script_S5_PendletonCam")
    look = unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*CAMERA), unreal.Vector(*PENDLETON_AT))
    cam.set_actor_rotation(look, False)
    cc = cam.get_editor_property("camera_component")
    cc.set_editor_property("field_of_view", 38.0)
    cc.set_editor_property("constrain_aspect_ratio", False)

    watches = []
    for (name, npc, kit, x, y, yaw0, ya, yb, secs, phase, speaker) in WATCHERS:
        m = tagged(EAS.spawn_actor_from_class(unreal.CastMember, unreal.Vector(x, y, 0.0), unreal.Rotator(0.0, 0.0, yaw0)))
        m.set_actor_label("Script_S5_" + name)
        m.set_editor_property("kit", getattr(unreal.CastKit, kit))
        m.set_editor_property("seated", False)
        skm = NPC % (npc, npc)
        if EAL.does_asset_exist(skm):
            m.set_editor_property("hero_mesh", EAL.load_asset(skm))
        else:
            REPORT.append("no mesh " + skm)
        w = unreal.Scene5Watch()
        w.set_editor_property("who", m)
        w.set_editor_property("yaw_a", ya)
        w.set_editor_property("yaw_b", yb)
        w.set_editor_property("sweep_seconds", secs)
        w.set_editor_property("phase", phase)
        w.set_editor_property("speaker", speaker)
        watches.append(w)

    pend = by_label("Vance", unreal.PatrolGuard)
    if not pend:
        REPORT.append("no Vance PatrolGuard for Pendleton")
    dimmer = next((a for a in C.actors() if isinstance(a, unreal.HouseDimmer)), None)
    if not dimmer:
        REPORT.append("no HouseDimmer")

    scene.set_editor_property("start_spot", spot)
    scene.set_editor_property("scene_camera", cam)
    scene.set_editor_property("pendleton", pend)
    scene.set_editor_property("dimmer", dimmer)
    scene.set_editor_property("watches", watches)
    scene.set_editor_property("door_mark", unreal.Vector(*DOOR_MARK))
    scene.set_editor_property("door_lamp_at", unreal.Vector(*DOOR_LAMP))
    scene.set_editor_property("booth_point", unreal.Vector(*BOOTH_PORT))
    if dimmer:
        # The beam leaves s7's booth port (the projection box over the lobby doors), not Maya's platform.
        local = dimmer.get_actor_transform().inverse_transform_location(unreal.Vector(*BOOTH_PORT))
        dimmer.set_editor_property("projector_offset", local)
        REPORT.append("dimmer projector_offset -> %s (world %s)" % (local, BOOTH_PORT))
    REPORT.append("placed scene, start, camera, %d watchers, pendleton=%s, dimmer=%s"
                  % (len(watches), pend.get_actor_label() if pend else None, dimmer.get_actor_label() if dimmer else None))


gone, shown = C.clear(TAG)
REPORT.append("cleared %d, shown %d" % (gone, shown))
if not OFF:
    if not hasattr(unreal, "Scene5Auditorium"):
        REPORT.append("Scene5Auditorium class missing: build the editor with it first")
    else:
        build()
"\n".join(REPORT)
