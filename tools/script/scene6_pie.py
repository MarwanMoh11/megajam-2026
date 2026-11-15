# Scene 6's play-in-editor shots into C:\Jam\Builds\script\s6\. Engine Python, repo root, under the editor lock:
#   "$PY" -I tools/script/scene6_pie.py [prefix]        # default prefix "pie"
#   "$PY" -I tools/script/scene6_pie.py --stop          # stop PIE (always before releasing the lock)
# Starts PIE (if needed), presses through the title card, Reel.Script.Jump 6, then shoots: Cole behind the crates,
# a loud moment through the screen, the room from the wardrobe, and walking into Enforcer B's view (a retake).
import os, sys, time, json
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "sets"))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import lights_pie as L

OUT = r"C:\Jam\Builds\script\s6"
WORLD = {"world": {"refPath": "/Game/Take/UEDPIE_0_Lvl_Auction.Lvl_Auction"}, "maxResolution": 1280}


def snap(s, name, prefix):
    res = L.tool(s, "SceneCaptureToolset.SceneCaptureToolset.CaptureGameViewport", WORLD)
    path = os.path.join(OUT, "%s_%s.png" % (prefix, name))
    print(name, L.save_image(res, path) and path, flush=True)


P_YAW = """
import unreal
w = unreal.EditorLevelLibrary.get_game_world()
y = [a.get_actor_rotation().yaw for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Actor)
     if "S6_Projectionist" in [str(t) for t in a.tags]]
str(round(y[0]) if y else 999)
"""


def wait_facing_door(s, tries=40):
    """Poll the projectionist's yaw until he has faced the door (90) twice running: a glance is not starting."""
    last = None
    for _ in range(tries):
        ok = '"returnValue":"90"' in L.py(s, P_YAW, 30)
        if ok and last:
            return True
        last = ok
        time.sleep(0.4)
    return False


def main():
    a = sys.argv[1:]
    s = L.sid()
    if a and a[0] == "--stop":
        print(L.tool(s, "EditorToolset.EditorAppToolset.StopPIE", {}))
        return
    prefix = a[0] if a else "pie"
    os.makedirs(OUT, exist_ok=True)
    if "true" not in json.dumps(L.tool(s, "EditorToolset.EditorAppToolset.IsPIERunning", {})).lower():
        L.tool(s, "EditorToolset.EditorAppToolset.StartPIE", {})
        time.sleep(4.0)
        L.console(s, "TakeConfirm")
        time.sleep(1.5)
    L.console(s, "Reel.Script.Jump 6")
    if prefix.startswith("chain"):                    # the whole scene through the test hooks
        time.sleep(5.6)                               # into the first loud moment (scene t 5..8 s)
        L.console(s, "Reel.S6.Behind A", "Reel.S6.Use")
        time.sleep(0.5)
        snap(s, "1_choke", prefix)
        time.sleep(float(os.environ.get("S6_DRAG_WAIT", "9")))   # PIE in the background runs slow: the drag is ~3 s game time
        snap(s, "2_trunk", prefix)
        L.console(s, "TakeWarp 4525 -450 0", "TakeLook 180 0", "Reel.S6.Use")
        time.sleep(1.2)
        snap(s, "3_crash", prefix)
        time.sleep(float(os.environ.get("S6_LURE_WAIT", "10")))
        snap(s, "4_lured", prefix)
        print("facing door:", wait_facing_door(s), flush=True)
        L.console(s, "Reel.S6.Behind P", "Reel.S6.Use")
        time.sleep(0.8)
        snap(s, "5_projectionist", prefix)
        L.console(s, "TakeWarp 4895 930 250", "TakeLook 90 5", "Reel.S6.Use")
        time.sleep(1.5)
        snap(s, "6_door", prefix)
        time.sleep(8.0)
        snap(s, "7_next", prefix)
        return
    time.sleep(3.0)
    snap(s, "1_start", prefix)
    time.sleep(3.6)                                   # the first loud moment: ~5 s in
    snap(s, "2_loud", prefix)
    L.console(s, "TakeWarp 4560 -60 0", "TakeLook 25 -5")
    time.sleep(1.5)
    snap(s, "3_room", prefix)
    L.console(s, "TakeWarp 5120 -500 0", "TakeLook 90 5")
    time.sleep(2.5)
    snap(s, "4_spotted", prefix)


if __name__ == "__main__":
    main()
