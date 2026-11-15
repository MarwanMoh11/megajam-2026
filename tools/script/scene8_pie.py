# Scene 8's play-in-editor shots into C:\Jam\Builds\script\s8\. Engine Python, repo root, under the editor lock:
#   "$PY" -I tools/script/scene8_pie.py [prefix]        # the escape on autopilot (Reel.S8.Autopilot 1): Cole's entrance,
#                                                       # the stairs, the lobby, the hall, the corridor chase, the prop
#                                                       # store, the alley, the dive, the drive, the mirror; a shot every
#                                                       # STEP seconds and the beat log
#   "$PY" -I tools/script/scene8_pie.py --stop          # stop PIE (always before releasing the lock)
import json
import os
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "sets"))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import lights_pie as L   # noqa: E402

OUT = r"C:\Jam\Builds\script\s8"
WORLD = {"world": {"refPath": "/Game/Take/UEDPIE_0_Lvl_Auction.Lvl_Auction"}, "maxResolution": 1280}
STEP = 2.2
SHOTS = 24


def snap(s, name, prefix):
    res = L.tool(s, "SceneCaptureToolset.SceneCaptureToolset.CaptureGameViewport", WORLD)
    path = os.path.join(OUT, "%s_%s.png" % (prefix, name))
    print(name, L.save_image(res, path) and path, flush=True)


def log(s, pat, n=30):
    r = L.tool(s, "ConsoleToolset.LogsToolset.GetLogEntries", {"pattern": pat, "maxEntries": n})
    print(json.dumps(r)[:3000], flush=True)


def main():
    a = sys.argv[1:]
    s = L.sid()
    if a and a[0] == "--stop":
        L.console(s, "Reel.S8.Autopilot 0")
        print(L.tool(s, "EditorToolset.EditorAppToolset.StopPIE", {}))
        return
    prefix = a[0] if a else "pie"
    os.makedirs(OUT, exist_ok=True)
    if "true" not in json.dumps(L.tool(s, "EditorToolset.EditorAppToolset.IsPIERunning", {})).lower():
        L.tool(s, "EditorToolset.EditorAppToolset.StartPIE", {})
        time.sleep(4.0)
        L.console(s, "TakeConfirm")
        time.sleep(1.5)
    L.console(s, "Reel.S8.Autopilot 1")
    L.console(s, "Reel.Script.Jump 8")
    time.sleep(0.6)
    for i in range(SHOTS):
        snap(s, "%02d" % i, prefix)
        time.sleep(STEP)
    log(s, "S8 beat")
    log(s, "SCRIPT")


if __name__ == "__main__":
    main()
