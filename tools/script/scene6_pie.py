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
