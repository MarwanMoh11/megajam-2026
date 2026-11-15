# Scene 5's play-in-editor shots into C:\Jam\Builds\script\s5\. Engine Python, repo root, under the editor lock:
#   "$PY" -I tools/script/scene5_pie.py [prefix]        # default prefix "pie"
#   "$PY" -I tools/script/scene5_pie.py --stop          # stop PIE (always before releasing the lock)
# Starts PIE (if needed), presses through the title card, Reel.Script.Jump 5, then shoots: the speech (two frames for
# the mouth), the seat cutaway, the dark seat shot, the exit shot, the sneak, the film, the door, and what follows.
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
    # QA Nov 15 evening: the mouth (two frames apart), the seat cutaway, the blackout's seat shot, the rise, the
    # sneak HUD with the watchers' looks, the film on the screen.
    time.sleep(2.0)
    snap(s, "1a_speech", prefix)
    time.sleep(0.25)
    snap(s, "1b_speech", prefix)
    time.sleep(3.0)                                  # the cutaway lands at 42% of the speech (~5 s)
    snap(s, "1c_cutaway", prefix)
    time.sleep(3.0)
    snap(s, "1d_angle2", prefix)
    time.sleep(4.6)                                  # the speech runs out, the house cuts, the seat shot
    snap(s, "2_seat_dark", prefix)
    time.sleep(2.2)                                  # he stands: the exit shot
    snap(s, "2b_exit", prefix)
    time.sleep(2.0)
    snap(s, "2c_exit_walk", prefix)
    time.sleep(3.0)                                  # the follow camera, the sneak HUD
    snap(s, "3_sneak", prefix)
    L.console(s, "TakeLook 45 -4")
    time.sleep(0.6)
    snap(s, "3b_screen", prefix)
    time.sleep(6.0)
    snap(s, "3c_screen_later", prefix)
    L.console(s, "TakeWarp 3650 -1880 0", "TakeLook -80 -5")
    time.sleep(2.0)
    snap(s, "4_door", prefix)
    time.sleep(5.5)
    snap(s, "5_after", prefix)


if __name__ == "__main__":
    main()
