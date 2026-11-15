# Scene 2's play-in-editor shots into C:\Jam\Builds\script\s2\. Engine Python, repo root, under the editor lock:
#   "$PY" -I tools/script/scene2_pie.py [prefix]      # the opening, the dials, a lock (Reel.S2.Easy), the cut to Vance
#   "$PY" -I tools/script/scene2_pie.py --fail        # bad patches until the hesitation fails it, the retake
#   "$PY" -I tools/script/scene2_pie.py --stop        # stop PIE (always before releasing the lock)
import json
import os
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "sets"))
import lights_pie as L  # noqa: E402

OUT = r"C:\Jam\Builds\script\s2"
WORLD = {"world": {"refPath": "/Game/Take/UEDPIE_0_Lvl_Auction.Lvl_Auction"}, "maxResolution": 1280}


def snap(s, name, prefix):
    res = L.tool(s, "SceneCaptureToolset.SceneCaptureToolset.CaptureGameViewport", WORLD)
    path = os.path.join(OUT, "%s_%s.png" % (prefix, name))
    print(name, L.save_image(res, path) and path, flush=True)


def key(s, k, down):
    L.console(s, "Reel.S2.Key %s %d" % (k, 1 if down else 0))


def start(s):
    if "true" not in json.dumps(L.tool(s, "EditorToolset.EditorAppToolset.IsPIERunning", {})).lower():
        L.tool(s, "EditorToolset.EditorAppToolset.StartPIE", {})
        time.sleep(4.0)
        L.console(s, "TakeConfirm")
        time.sleep(1.5)
    L.console(s, "Reel.S2.Easy 1")
    L.console(s, "Reel.Script.Jump 2")


def main():
    a = sys.argv[1:]
    s = L.sid()
    if a and a[0] == "--stop":
        print(L.tool(s, "EditorToolset.EditorAppToolset.StopPIE", {}))
        return
    os.makedirs(OUT, exist_ok=True)
    if a and a[0] == "--fail":
        start(s)
        time.sleep(0.5)
        key(s, "SpaceBar", True)            # skip the opening
        key(s, "SpaceBar", False)
        time.sleep(0.8)
        for _ in range(10):                  # patch before sync: each costs hesitation
            key(s, "SpaceBar", True)
            key(s, "SpaceBar", False)
            time.sleep(0.15)
        snap(s, "f1_burn", "fail")
        time.sleep(2.2)
        snap(s, "f2_retake", "fail")
        return
    prefix = a[0] if a else "pie"
    start(s)
    time.sleep(0.6)
    snap(s, "1_steps", prefix)
    time.sleep(1.6)
    snap(s, "2_street", prefix)
    time.sleep(1.2)
    snap(s, "3_glass", prefix)
    time.sleep(2.0)
    snap(s, "4_max", prefix)
    key(s, "D", True)
    time.sleep(0.6)
    key(s, "D", False)
    key(s, "Right", True)
    time.sleep(0.3)
    snap(s, "5_dials", prefix)
    key(s, "Right", False)
    L.console(s, "Reel.S2.Easy 100")
    time.sleep(1.5)
    snap(s, "6_synced", prefix)
    key(s, "SpaceBar", True)
    key(s, "SpaceBar", False)
    time.sleep(0.4)
    snap(s, "7_lock", prefix)
    L.console(s, "Reel.S2.Easy 1")
    time.sleep(2.8)
    snap(s, "8_vance", prefix)
    time.sleep(6.0)
    snap(s, "9_after", prefix)


if __name__ == "__main__":
    main()
