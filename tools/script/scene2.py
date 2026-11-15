# Scene 2. INT. FLORIST VAN (Max). Owner: s2-van. Places AScene2Van (tag script_s2) and sets its world points: the
# opening move (theater steps -> across the street -> through the van's windshield -> settled on Max) and the CRT
# feed's security camera. The van set itself is tools/script/scene2_van.py (tag script_s2_set): run that first.
# Run under the editor lock, after the existing pipeline (tools/sets/run_all.py):
#     "<engine python>" -I tools/ue_mcp.py py tools/script/scene2_van.py 110
#     "<engine python>" -I tools/ue_mcp.py py tools/script/scene2.py 60
import sys
import importlib

import unreal

sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C   # noqa: E402

importlib.reload(C)

TAG = "script_s2"
V = unreal.Vector

# --- the van (scene2_van.py's numbers): bonnet south, cab y 1290..1450, cargo to 1810, x -5485..-5275 -------------------
VAN_X = -5380.0
POINTS = dict(
    steps_pos=V(-3400.0, 250.0, 220.0),        # on the theater steps, beside the queue
    steps_look=V(-3520.0, 320.0, 150.0),       # Vance at the scanner: where the pan starts
    windshield_pos=V(VAN_X, 1080.0, 170.0),    # just in front of the fogged windshield
    windshield_look=V(VAN_X, 1330.0, 160.0),
    inside_pos=V(VAN_X - 20.0, 1400.0, 160.0),  # through the glass, in the cab
    max_cam_pos=V(-5450.0, 1450.0, 150.0),     # the settled shot on Max at his CRT
    max_look=V(-5340.0, 1690.0, 120.0),
    feed_from=V(-3330.0, 120.0, 420.0),        # the security camera on the facade (Set_S2_SecurityCam)
    feed_at=V(-3520.0, 320.0, 110.0),          # the palm scanner's glass on the podium
)

# the sounds (tools/script/scene2_audio.py imports them): property -> asset name under /Game/Take/Audio/Script/S2
SOUNDS = dict(line_open="vo_s2_max_open", line_retake="vo_s2_max_retake", line_lock="vo_s2_max_lock",
              line_not_yet="vo_s2_max_notyet", line_close="vo_s2_max_close", line_bouncer_a="vo_s2_bouncer_a",
              line_bouncer_b="vo_s2_bouncer_b", sfx_dial="s2_dial_click", sfx_spark="s2_spark", sfx_lock="s2_lock",
              sfx_toggle="s2_toggle", amb_hum="s2_hum")
SND_DIR = "/Game/Take/Audio/Script/S2/"

C.clear(TAG)
cls = unreal.load_class(None, "/Script/Reel.Scene2Van")
out = []
if cls is None:
    out.append("AScene2Van not compiled yet: nothing placed")
else:
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    a = eas.spawn_actor_from_class(cls, V(VAN_X, 1550.0, 0.0), unreal.Rotator(0, 0, 0))
    a.set_actor_label("Script_S2_Van")
    a.set_folder_path("Script/S2")
    a.set_editor_property("tags", [unreal.Name(TAG)])
    for k, v in POINTS.items():
        a.set_editor_property(k, v)
    got = []
    for k, n in SOUNDS.items():
        if unreal.EditorAssetLibrary.does_asset_exist(SND_DIR + n):
            a.set_editor_property(k, unreal.EditorAssetLibrary.load_asset(SND_DIR + n))
            got.append(n)
    cam = C.find("Script_S1_.*Cam.*")          # Scene 1's scanner close-up, if s1 placed one
    if cam:
        a.set_editor_property("vance_camera", cam[0])
    out.append("placed %s, %d/%d sounds, vance cam %s" % (a.get_actor_label(), len(got), len(SOUNDS),
                                                          cam[0].get_actor_label() if cam else "none"))
"; ".join(out)
