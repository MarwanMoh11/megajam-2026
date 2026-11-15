# Scene 6's play-in-editor shots into C:\Jam\Builds\script\s6\. Engine Python, repo root, under the editor lock:
#   "$PY" -I tools/script/scene6_pie.py [prefix]        # default prefix "pie"
#   "$PY" -I tools/script/scene6_pie.py --stop          # stop PIE (always before releasing the lock)
# Starts PIE (if needed), presses through the title card, Reel.Script.Jump 6, then shoots: Cole behind the crates,
# a loud moment through the screen, the room, and walking into the guard's view (a retake).
# Prefix "chain...": the whole scene through the test hooks (lever, crouch, choke-hold, drag, rig, fire exit).
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
    if prefix.startswith("chain"):                    # the whole scene through the test hooks
        k = float(os.environ.get("S6_SLOW", "1.0"))   # PIE in the background can run slow: scale the waits
        time.sleep(3.0 * k)
        snap(s, "01_start", prefix)
        L.console(s, "Reel.S6.Goto lever", "Reel.S6.Use")
        time.sleep(1.3 * k)
        snap(s, "02_crash", prefix)
        time.sleep(3.5 * k)
        snap(s, "03_lured", prefix)
        L.console(s, "Reel.S6.Crouch 1", "Reel.S6.Behind")
        time.sleep(1.0 * k)
        snap(s, "04_crouch_behind", prefix)
        L.console(s, "Reel.S6.Use")
        for i, t in enumerate((0.35, 1.0, 1.4, 1.3, 0.9, 1.2)):
            time.sleep(t * k)
            snap(s, "05_choke_%d" % i, prefix)
        time.sleep(0.8 * k)
        snap(s, "06_down", prefix)
        L.console(s, "Reel.S6.Use")
        for i, t in enumerate((0.8, 1.5, 1.5, 1.5)):
            time.sleep(t * k)
            snap(s, "07_drag_%d" % i, prefix)
        time.sleep(1.0 * k)
        snap(s, "08_hidden", prefix)
        L.console(s, "Reel.S6.Goto panel", "Reel.S6.Use")
        time.sleep(1.1 * k)
        snap(s, "09_sparks", prefix)
        time.sleep(1.6 * k)
        snap(s, "10_insert", prefix)
        time.sleep(2.2 * k)
        L.console(s, "Reel.S6.Goto exit", "Reel.S6.Use")
        time.sleep(1.8 * k)
        snap(s, "11_exit", prefix)
        time.sleep(1.2 * k)
        snap(s, "12_wedged", prefix)
        time.sleep(4.0 * k)
        snap(s, "13_next", prefix)
        return
    time.sleep(3.0)
    snap(s, "1_start", prefix)
    time.sleep(3.6)                                   # the first loud moment: ~5 s in
    snap(s, "2_loud", prefix)
    L.console(s, "TakeWarp 4560 -60 0", "TakeLook 25 -5")
    time.sleep(1.5)
    snap(s, "3_room", prefix)
    L.console(s, "TakeWarp 4850 200 0", "TakeLook 90 5")
    time.sleep(2.5)
    snap(s, "4_spotted", prefix)


if __name__ == "__main__":
    main()
