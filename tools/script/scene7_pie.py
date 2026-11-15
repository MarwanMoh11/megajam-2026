# Scene 7's play-in-editor shots into C:\Jam\Builds\script\s7\. Engine Python, repo root, under the editor lock:
#   "$PY" -I tools/script/scene7_pie.py [prefix]        # the whole scene on Reel.S7.Auto 1, a shot every ~2 s
#   "$PY" -I tools/script/scene7_pie.py miss            # auto until the fight, then a missed prompt (retake)
#   "$PY" -I tools/script/scene7_pie.py --stop          # stop PIE (always before releasing the lock)
import os, sys, time, json
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "sets"))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import lights_pie as L

OUT = r"C:\Jam\Builds\script\s7"
WORLD = {"world": {"refPath": "/Game/Take/UEDPIE_0_Lvl_Auction.Lvl_Auction"}, "maxResolution": 1280}


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
    L.console(s, "Reel.S7.Auto 1", "Reel.Script.Jump 7")
    T0 = time.time()
    if prefix.startswith("miss"):
        time.sleep(15.5)                              # past the alarm, into the first prompt
        L.console(s, "Reel.S7.Auto 0")
        snap(s, "1_prompt", prefix)
        time.sleep(2.5)
        snap(s, "2_hit", prefix)
        time.sleep(1.5)
        snap(s, "3_cut", prefix)
        time.sleep(2.5)
        snap(s, "4_retake", prefix)
        L.console(s, "Reel.S7.Auto 1")
        time.sleep(4.0)
        snap(s, "5_fight", prefix)
        return
    for i in range(18):
        time.sleep(1.2)
        snap(s, "%02d" % i, prefix)


if __name__ == "__main__":
    main()
