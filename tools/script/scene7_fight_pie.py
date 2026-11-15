# Scene 7's fist fight in play-in-editor, shot into C:\Jam\Builds\script\s7fight\. Engine Python, repo root, under the
# editor lock. Starts the fight directly (Reel.S7Fight.Start, wherever the script is) and shoots it as it plays:
#   "$PY" -I tools/script/scene7_fight_pie.py [prefix]      # every prompt answered right (Reel.S7Fight.Auto 1)
#   "$PY" -I tools/script/scene7_fight_pie.py miss          # each beat missed once, then retaken (Auto 2)
#   "$PY" -I tools/script/scene7_fight_pie.py pie --to8    # then Reel.Script.Jump 8 and a few frames of Scene 8
#   "$PY" -I tools/script/scene7_fight_pie.py --stop        # stop PIE (always before releasing the lock)
import os, sys, time, json
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "sets"))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import lights_pie as L

OUT = r"C:\Jam\Builds\script\s7fight"
WORLD = {"world": {"refPath": "/Game/Take/UEDPIE_0_Lvl_Auction.Lvl_Auction"}, "maxResolution": 1280}
T0 = 0.0


def snap(s, name, prefix):
    res = L.tool(s, "SceneCaptureToolset.SceneCaptureToolset.CaptureGameViewport", WORLD)
    path = os.path.join(OUT, "%s_%s.png" % (prefix, name))
    print("%.1f" % (time.time() - T0), name, L.save_image(res, path) and path, flush=True)


def main():
    global T0
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
    # Scene 7 part 0 is a walk: the quietest scene to borrow while the fight takes Vance over.
    L.console(s, "Reel.Script.Jump 7")
    time.sleep(2.0)
    L.console(s, "Reel.S7Fight.Auto %d" % (2 if prefix.startswith("miss") else 1), "Reel.S7Fight.Start")
    T0 = time.time()
    span = 30.0 if prefix.startswith("miss") else 20.0
    i = 0
    while time.time() - T0 < span:
        snap(s, "%02d" % i, prefix)
        i += 1
        time.sleep(0.35)
    L.console(s, "Reel.S7Fight.Auto 0")
    if "--to8" in a:    # then cut to Scene 8 from where the fight leaves Vance (a jump: the booth's FinishScene isn't in play)
        L.console(s, "Reel.Script.Jump 8")
        t8 = time.time()
        j = 0
        while time.time() - t8 < 6.0:
            snap(s, "s8_%02d" % j, prefix)
            j += 1
            time.sleep(0.6)


if __name__ == "__main__":
    main()
