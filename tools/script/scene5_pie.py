# Scene 5's play-in-editor shots into C:\Jam\Builds\script\s5\. Engine Python, repo root, under the editor lock:
#   "$PY" -I tools/script/scene5_pie.py [prefix]        # default prefix "pie"
#   "$PY" -I tools/script/scene5_pie.py --stop          # stop PIE (always before releasing the lock)
# Starts PIE (if needed), presses through the title card, Reel.Script.Jump 5, then shoots: the speech, the blackout,
# Vance on the south wall in the dark, Vance at the door, and what comes after the finish.
import os, sys, time
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "sets"))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import lights_pie as L

OUT = r"C:\Jam\Builds\script\s5"
WORLD = {"world": {"refPath": "/Game/Take/UEDPIE_0_Lvl_Auction.Lvl_Auction"}, "maxResolution": 1280}


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
    prefix = a[0] if a else "pie"
    os.makedirs(OUT, exist_ok=True)
    import json
    if "true" not in json.dumps(L.tool(s, "EditorToolset.EditorAppToolset.IsPIERunning", {})).lower():
        L.tool(s, "EditorToolset.EditorAppToolset.StartPIE", {})
        time.sleep(4.0)
        L.console(s, "TakeConfirm")
        time.sleep(1.5)
    L.console(s, "Reel.Script.Jump 5")
    time.sleep(4.5)
    snap(s, "1_speech", prefix)
    time.sleep(9.0)                                  # the speech runs out, the house cuts
    snap(s, "2_blackout", prefix)
    L.console(s, "TakeWarp 3150 -1830 0", "TakeLook 0 -8")
    time.sleep(2.5)
    snap(s, "3_wall", prefix)
    L.console(s, "TakeLook -100 -10")
    time.sleep(0.6)
    snap(s, "3b_wall_back", prefix)
    L.console(s, "TakeWarp 3650 -1880 0", "TakeLook -80 -5")
    time.sleep(2.0)
    snap(s, "4_door", prefix)
    time.sleep(5.5)
    snap(s, "5_after", prefix)


if __name__ == "__main__":
    main()
