# Scene 4's play-in-editor shots into C:\Jam\Builds\script\s4\. Engine Python, repo root, under the editor lock:
#   "$PY" -I tools/script/scene4_pie.py [prefix]        # default "pie": the clean run (spot, cart, cue, the shot)
#   "$PY" -I tools/script/scene4_pie.py fail            # Vance sent while the lane is watched (a retake)
#   "$PY" -I tools/script/scene4_pie.py --stop          # stop PIE (always before releasing the lock)
import os, sys, time, json
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "sets"))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import lights_pie as L

OUT = r"C:\Jam\Builds\script\s4"
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
    L.console(s, "Reel.Script.Jump 4")
    time.sleep(2.5)
    snap(s, "1_birdseye", prefix)
    if prefix == "fail":
        L.console(s, "Reel.S4.Press cue")
        time.sleep(0.8)
        snap(s, "2_spotted", prefix)
        time.sleep(2.0)
        snap(s, "3_retake", prefix)
        return
    L.console(s, "Reel.S4.Press 1")
    time.sleep(0.15)
    snap(s, "2a_spot_swing", prefix)
    time.sleep(0.8)
    snap(s, "2b_spot_blind", prefix)
    L.console(s, "Reel.S4.Press 2")
    time.sleep(0.2)
    snap(s, "3a_bag_drop", prefix)
    time.sleep(2.0)
    snap(s, "3b_crash", prefix)
    time.sleep(1.5)
    snap(s, "4_lane_clear", prefix)
    L.console(s, "Reel.S4.Press cue")
    time.sleep(2.5)
    snap(s, "5_track_a", prefix)
    time.sleep(4.0)
    snap(s, "6_track_b", prefix)
    time.sleep(4.0)
    snap(s, "7_doors", prefix)
    time.sleep(4.0)
    snap(s, "8_next", prefix)


if __name__ == "__main__":
    main()
