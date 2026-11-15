# Scene 8's play-in-editor shots into C:\Jam\Builds\script\s8\. Engine Python, repo root, under the editor lock:
#   "$PY" -I tools/script/scene8_pie.py [prefix]        # the scene through: burst, run, dive (warped to the car), drive, mirror, end
#   "$PY" -I tools/script/scene8_pie.py fail            # stand still: the guards' fire closes in, the retake card
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
CAR_DOOR = (2150.0, -3560.0, -21.0)        # beside the stopped car's passenger door (AScene8Alley defaults)


def snap(s, name, prefix):
    res = L.tool(s, "SceneCaptureToolset.SceneCaptureToolset.CaptureGameViewport", WORLD)
    path = os.path.join(OUT, "%s_%s.png" % (prefix, name))
    print(name, L.save_image(res, path) and path, flush=True)


def log(s, pat, n=12):
    r = L.tool(s, "ConsoleToolset.LogsToolset.GetLogEntries", {"pattern": pat, "maxEntries": n})
    print(json.dumps(r)[:1500], flush=True)


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
    L.console(s, "Reel.Script.Jump 8")
    if prefix == "fail":
        time.sleep(6.5)
        snap(s, "1_closing", prefix)
        time.sleep(4.5)
        snap(s, "2_retake", prefix)
        log(s, "S8 beat|SCRIPT blooper")
        return
    time.sleep(0.5)
    snap(s, "1_burst", prefix)
    time.sleep(1.4)
    snap(s, "2_car", prefix)
    time.sleep(1.6)                                   # the run: Vance's camera, the guards out, shots
    time.sleep(0.8)
    snap(s, "3_run", prefix)
    L.console(s, "TakeWarp %.0f %.0f %.0f" % CAR_DOOR)
    time.sleep(0.4)
    snap(s, "4_dive", prefix)
    time.sleep(1.6)
    snap(s, "5_drive", prefix)
    time.sleep(1.9)
    snap(s, "6_mirror", prefix)
    time.sleep(1.6)
    snap(s, "7_print", prefix)
    time.sleep(3.0)
    snap(s, "8_end", prefix)
    log(s, "S8 beat|SCRIPT")


if __name__ == "__main__":
    main()
