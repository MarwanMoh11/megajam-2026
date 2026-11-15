# Scene 3's play-in-editor shots into C:\Jam\Builds\script\s3\. Engine Python, repo root, under the editor lock:
#   "$PY" -I tools/script/scene3_pie.py [alibi|tackle|desk]   # default alibi; desk: the reception desk + the booth
#   "$PY" -I tools/script/scene3_pie.py --stop          # stop PIE (always before releasing the lock)
# Starts PIE (if needed), presses through the title card, Reel.Script.Jump 3, then shoots the open, the splash, the
# choice; alibi: Play Along (Reel.S3.Key 1), the hall (Max's V.O.), the rope (the guard sends him back), the cut on;
# tackle: Brush Off (Reel.S3.Key 2), straight to the rope, tackled, the retake.
import os, sys, time, json
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "sets"))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import lights_pie as L

OUT = r"C:\Jam\Builds\script\s3"
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
    mode = a[0] if a else "alibi"
    prefix = "pie_" + mode
    os.makedirs(OUT, exist_ok=True)
    if "true" not in json.dumps(L.tool(s, "EditorToolset.EditorAppToolset.IsPIERunning", {})).lower():
        L.tool(s, "EditorToolset.EditorAppToolset.StartPIE", {})
        time.sleep(4.0)
        L.console(s, "TakeConfirm")
        time.sleep(1.5)
    L.console(s, "Reel.Script.Jump 3")
    time.sleep(2.0)
    snap(s, "1_open", prefix)
    time.sleep(4.0)                                   # the stumble and the splash, his line under way
    snap(s, "2_splash", prefix)
    time.sleep(6.0)                                   # the choice opens as his line lands
    snap(s, "3_choice", prefix)
    if mode == "desk":
        # Play Along, then to the reception desk (front.py DESK, its C toward the doors): the prompt, E, the clerk.
        L.console(s, "Reel.S3.Key 1")
        time.sleep(5.0)
        L.console(s, "TakeWarp -2420 640 0", "TakeLook 0 -5")
        time.sleep(1.0)
        snap(s, "4_desk_prompt", prefix)
        L.console(s, "Reel.S3.Key E")
        time.sleep(0.3)
        snap(s, "5_desk_used", prefix)
        time.sleep(3.0)
        L.console(s, "Reel.S3.Key E")
        time.sleep(0.5)
        snap(s, "6_desk_again", prefix)
        # The hall's west wall and the projection booth, from just inside the doors, looking back up.
        L.console(s, "TakeWarp -300 -150 0", "TakeLook 180 18")
        time.sleep(1.0)
        snap(s, "7_booth_from_hall", prefix)
        return
    if mode == "tackle":
        L.console(s, "Reel.S3.Key 2")
        time.sleep(1.0)
        snap(s, "4_brushoff", prefix)
        L.console(s, "TakeWarp 2200 0 0", "TakeLook 0 0")
        time.sleep(0.8)
        snap(s, "5_tackle", prefix)
        time.sleep(2.5)
        snap(s, "6_retake", prefix)
        return
    L.console(s, "Reel.S3.Key 1")
    time.sleep(1.5)
    snap(s, "4_playalong", prefix)
    time.sleep(4.5)
    L.console(s, "TakeWarp 1400 0 0", "TakeLook 0 0")
    time.sleep(1.5)
    snap(s, "5_hall_vo", prefix)
    L.console(s, "TakeWarp 2200 0 0", "TakeLook 0 0")
    time.sleep(1.0)
    snap(s, "6_alibi", prefix)
    time.sleep(5.0)
    snap(s, "7_cut_on", prefix)


if __name__ == "__main__":
    main()
