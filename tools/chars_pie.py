# chars-locomotion PIE look (Nov 15 evening): Scene 5's Pendleton (arms in his speech) with a pistol raised, and Vance
# crouched / walking. Shots to C:\Jam\Builds\script\chars\. Engine Python, repo root, under the editor lock:
#   "$PY" -I tools/chars_pie.py [tag]      |      "$PY" -I tools/chars_pie.py --stop
import json
import os
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "sets"))
import lights_pie as L

OUT = r"C:\Jam\Builds\script\chars"
WORLD = {"world": {"refPath": "/Game/Take/UEDPIE_0_Lvl_Auction.Lvl_Auction"}, "maxResolution": 1280}

ARM = r'''
import unreal
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
out = []
cam = unreal.GameplayStatics.get_player_camera_manager(w, 0).get_camera_location()
for m in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.CastMember):
    if m.get_editor_property("kit") == unreal.CastKit.VANCE:
        m.set_editor_property("gun_grip", unreal.Transform(unreal.Vector(-7.5, 2.5, -3.0), unreal.Rotator(roll=1.0, pitch=-8.0, yaw=176.0), unreal.Vector(1, 1, 1)))
        m.%s
        out.append("%s " + m.get_actor_label())
for c in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.TakeCharacter):
    out.append("%%s anim=%%s" %% (c.get_actor_label(), c.mesh.get_anim_instance().get_class().get_name() if c.mesh.get_anim_instance() else None))
"; ".join(out)
'''

CROUCH = r'''
import unreal
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
out = []   # the player pawn is a camera rig: crouch the heroes themselves (Scene 6 drives Vex/Cole)
for c in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.TakeCharacter):
    c.set_crouching(True)
    out.append("%s %s" % (c.get_actor_label(), c.is_crouching()))
"crouch " + "; ".join(out)
'''


def snap(s, name, prefix):
    res = L.tool(s, "SceneCaptureToolset.SceneCaptureToolset.CaptureGameViewport", WORLD)
    path = os.path.join(OUT, "%s_%s.png" % (prefix, name))
    print(name, L.save_image(res, path) and path, flush=True)


def main():
    a = sys.argv[1:]
    s = L.sid()
    if a and a[0] == "--stop":
        print(L.tool(s, "EditorToolset.EditorAppToolset.StopPIE", {}))
        return
    if a and a[0] == "--lc":   # Live Coding first (CastMember.cpp body/constructor changes), then the look
        print("live coding:", str(L.tool(s, "LiveCodingToolset.CompileLiveCoding", {}, timeout=110))[:300], flush=True)
        a = a[1:]
    prefix = a[0] if a else "pie"
    os.makedirs(OUT, exist_ok=True)
    if "true" not in json.dumps(L.tool(s, "EditorToolset.EditorAppToolset.IsPIERunning", {})).lower():
        L.tool(s, "EditorToolset.EditorAppToolset.StartPIE", {})
        time.sleep(4.0)
        L.console(s, "TakeConfirm")
        time.sleep(1.5)
    L.console(s, "Reel.Script.Jump 5")
    time.sleep(3.0)
    snap(s, "1_speech_arms", prefix)
    print(L.py(s, ARM % ("aim_at(cam)", "aimed")))
    time.sleep(1.0)
    snap(s, "2_pistol_aim", prefix)
    print(L.py(s, ARM % ("stop_aiming()", "lowered")))
    time.sleep(1.0)
    snap(s, "3_pistol_low", prefix)
    # Scene 8: Vance forced to run, the chasers' pistols
    L.console(s, "Reel.Script.Jump 8")
    time.sleep(5.0)
    snap(s, "4_escape_run", prefix)
    # Scene 6: the player crouched (Cole)
    L.console(s, "Reel.Script.Jump 6")
    time.sleep(3.0)
    print(L.py(s, CROUCH))
    time.sleep(1.5)
    snap(s, "5_crouch", prefix)


main()
