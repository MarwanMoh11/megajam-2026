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
#   QA Nov 15 evening: Vance's seat (the free aisle seat nearest SEAT_WANT in the south row end), the seat camera, the
#             exit camera from the side aisle, Pendleton's second angle, the walk out of the row (StandAt, RowExit,
#             AisleAt; StartSpot is now AisleAt: a retake starts there), the watchers' film/check cycles (no beat), and
#             the long film M_S5_Film (tools/script/scene5_film.py) on the HouseDimmer
import sys, importlib, json, os, math
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
SEAT_WANT = (2550.0, -583.0)                    # Vance's aisle seat (hall_seats.json: the south end of a row)
SEAT_Z = 0.0
SCREEN = (4380.0, 0.0)                          # the screen's middle, for the watchers' film yaw
CAMERA = (3330.0, -260.0, 205.0)                # Pendleton's close shot, from the front floor
PENDLETON_AT = (3890.0, -480.0, 170.0)
DOOR_MARK = (3650.0, -1900.0, 0.0)
DOOR_LAMP = (3745.0, -1980.0, 140.0)
BOOTH_PORT = (-600.0, 150.0, 600.0)            # s7-booth's port in the projection box over the lobby doors
# name, npc mesh, kit, x, y, yaw0, check a, check b, away s, check s, phase, speaker
# Timeline from the blackout (the rise hands the player Vance at about 5 s): the guard is checking the wall as he
# walks out of the row (0.5-4.5 s) and again at 14-18 s; the usher turns to the aisle at about 9 s. Each on the
# film for 8-9 s at a time, with the HUD's warning 1.5 s before a turn.
WATCHERS = [
    ("Usher", "Clerk", "Staff", 3420.0, -1050.0, -110.0, -150.0, -70.0, 8.0, 3.5, 0.917, "USHER"),
    ("Guard", "Guard", "Guard", 2500.0, -1760.0, 20.0, -8.0, 58.0, 9.0, 4.0, 0.703, "GUARD"),
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


def look_cam(label, at, target, fov):
    cam = tagged(EAS.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(*at), unreal.Rotator(0, 0, 0)))
    cam.set_actor_label(label)
    cam.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*at), unreal.Vector(*target)), False)
    cc = cam.get_editor_property("camera_component")
    cc.set_editor_property("field_of_view", fov)
    cc.set_editor_property("constrain_aspect_ratio", False)
    return cam


def free_seat():
    seats = json.load(open("C:/Jam/megajam/tools/sets/hall_seats.json"))
    seats = [s for s in seats if abs(s["y"] + 583.0) < 2 and 1900 <= s["x"] <= 3150]
    guests = [a.get_actor_location() for a in C.actors() if isinstance(a, unreal.CastMember)]
    seats.sort(key=lambda s: abs(s["x"] - SEAT_WANT[0]))
    for s in seats:
        if all(math.hypot(g.x - s["x"], g.y - s["y"]) > 45 for g in guests):
            return (s["x"], s["y"])
    REPORT.append("no free aisle seat: using SEAT_WANT (a guest may share it)")
    return SEAT_WANT


def build():
    sx, sy = free_seat()
    stand = (sx + 50.0, sy - 60.0, 0.0)
    row_exit = (sx + 60.0, sy - 190.0, 0.0)
    aisle = (sx + 160.0, -1150.0, 0.0)
    REPORT.append("seat (%.0f, %.0f)" % (sx, sy))
    scene = tagged(EAS.spawn_actor_from_class(unreal.Scene5Auditorium, unreal.Vector(*SCENE_AT), unreal.Rotator(0, 0, 0)))
    scene.set_actor_label("Script_S5_Auditorium")

    door_yaw = math.degrees(math.atan2(DOOR_MARK[1] - aisle[1], DOOR_MARK[0] - aisle[0]))
    spot = tagged(EAS.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(aisle[0], aisle[1], 100.0), unreal.Rotator(0.0, 0.0, door_yaw)))
    spot.set_actor_label("Script_S5_Start")

    cam = tagged(EAS.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(*CAMERA), unreal.Rotator(0, 0, 0)))
    cam.set_actor_label("Script_S5_PendletonCam")
    look = unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*CAMERA), unreal.Vector(*PENDLETON_AT))
    cam.set_actor_rotation(look, False)
    cc = cam.get_editor_property("camera_component")
    cc.set_editor_property("field_of_view", 38.0)
    cc.set_editor_property("constrain_aspect_ratio", False)

    seat_cam = look_cam("Script_S5_SeatCam", (sx + 230.0, sy - 260.0, 150.0), (sx, sy, 105.0), 40.0)
    exit_cam = look_cam("Script_S5_ExitCam", (sx + 560.0, -1350.0, 175.0), (sx + 60.0, sy - 170.0, 110.0), 50.0)
    pend_cam2 = look_cam("Script_S5_PendletonCam2", (3020.0, -760.0, 235.0), (PENDLETON_AT[0], PENDLETON_AT[1], 175.0), 28.0)

    watches = []
    for (name, npc, kit, x, y, yaw0, ya, yb, away, check, phase, speaker) in WATCHERS:
        m = tagged(EAS.spawn_actor_from_class(unreal.CastMember, unreal.Vector(x, y, 0.0), unreal.Rotator(0.0, 0.0, yaw0)))
        m.set_actor_label("Script_S5_" + name)
        m.set_editor_property("kit", getattr(unreal.CastKit, kit.upper()))
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
        w.set_editor_property("away_yaw", math.degrees(math.atan2(SCREEN[1] - y, SCREEN[0] - x)))
        w.set_editor_property("away_seconds", away)
        w.set_editor_property("check_seconds", check)
        w.set_editor_property("turn_seconds", 0.9)
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
    scene.set_editor_property("seat_camera", seat_cam)
    scene.set_editor_property("exit_camera", exit_cam)
    scene.set_editor_property("pendleton_camera2", pend_cam2)
    scene.set_editor_property("seat_at", unreal.Vector(sx, sy, SEAT_Z))
    scene.set_editor_property("seat_yaw", 0.0)
    scene.set_editor_property("stand_at", unreal.Vector(*stand))
    scene.set_editor_property("row_exit", unreal.Vector(*row_exit))
    scene.set_editor_property("aisle_at", unreal.Vector(*aisle))
    scene.set_editor_property("door_mark", unreal.Vector(*DOOR_MARK))
    scene.set_editor_property("door_lamp_at", unreal.Vector(*DOOR_LAMP))
    scene.set_editor_property("booth_point", unreal.Vector(*BOOTH_PORT))
    scene.set_editor_property("pendleton_spot_candelas", 7000.0)
    if dimmer:
        # The beam leaves s7's booth port (the projection box over the lobby doors), not Maya's platform.
        local = dimmer.get_actor_transform().inverse_transform_location(unreal.Vector(*BOOTH_PORT))
        dimmer.set_editor_property("projector_offset", local)
        film = "/Game/Take/Script/S5/M_S5_Film"
        if EAL.does_asset_exist(film):
            dimmer.set_editor_property("film_material", EAL.load_asset(film))
            dimmer.set_editor_property("film_leader_skip_seconds", 0.0)
            REPORT.append("dimmer film -> M_S5_Film")
        else:
            REPORT.append("no M_S5_Film: run tools/script/scene5_film.py first (the old M_OldFilm stays)")
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
