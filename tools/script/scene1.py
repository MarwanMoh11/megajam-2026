# Scene 1 overlay (s1-marquee): the scene actor (AScene1Marquee, the palm scanner's podium at the door) and the queue's
# tuning for the script (the scan beat long enough for a hand on the glass, the Bouncer moved behind the podium).
# The street's dressing (the marquee, rain, wet cobbles, neon) is tools/script/scene1_dress.py (tag script_s1_dress).
# Re-runnable: clears its tag (script_s1), then builds again. Run under the editor lock, after the existing pipeline:
#     "<engine python>" -I tools/ue_mcp.py py tools/script/scene1.py 110
# Switch: S1_OFF = True only clears.
import sys, importlib
sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C
importlib.reload(C)
import unreal

TAG = "script_s1"
OFF = bool(globals().get("S1_OFF"))
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
REPORT = []

# --- numbers (build_auction.py: SCAN_X -3520, SCAN_Y 320; the scan point is (-3610, 320)) ----------------------------
PODIUM = (-3610.0, 255.0, 0.0, 90.0)          # x, y, z, yaw: +X (the plate's face) toward the guest at the scan point
DOORMAN = (-3625.0, 180.0, 0.0)               # the Bouncer behind the podium (was -3615, 205: the podium's spot)
DOORMAN_YAW = 154.0
SCAN_SECONDS = 1.8                            # a guest at the plate: the hand, the BEEP, green, then in


def clear():
    n = 0
    for a in list(eas.get_all_level_actors()):
        if a and unreal.Name(TAG) in list(a.tags):
            eas.destroy_actor(a)
            n += 1
    REPORT.append("cleared %d" % n)


def find_label(label):
    for a in eas.get_all_level_actors():
        if a and a.get_actor_label() == label:
            return a
    return None


clear()
if not OFF:
    cls = getattr(unreal, "Scene1Marquee", None)
    if cls is None:
        REPORT.append("NO CLASS Scene1Marquee (the editor needs the build with it)")
    else:
        a = eas.spawn_actor_from_class(cls, unreal.Vector(PODIUM[0], PODIUM[1], PODIUM[2]),
                                       unreal.Rotator(0.0, 0.0, PODIUM[3]))
        a.set_actor_label("Scene1Marquee")
        a.set_folder_path("Script/S1")
        C._set_tags(a, [TAG])
        REPORT.append("scene actor %s" % a.get_name())
    q = find_label("QueueLine")
    if q:
        q.set_editor_property("scan_seconds", SCAN_SECONDS)
        q.set_editor_property("doorman_point", unreal.Vector(*DOORMAN))
        q.set_editor_property("doorman_yaw", DOORMAN_YAW)
        REPORT.append("queue tuned")
    else:
        REPORT.append("NO QueueLine")
print("S1 " + "; ".join(REPORT))
