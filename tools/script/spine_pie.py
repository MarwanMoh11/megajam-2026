# Spine's play-in-editor pass over the whole script, shots into C:\Jam\Builds\script\spine\. Engine Python, repo
# root, under the editor lock:
#   "$PY" -I tools/script/spine_pie.py [prefix]        # default prefix "pie"; stops PIE at the end
# Title card, then Reel.Script.Jump 1..8 (a slug shot each, a splice shot on scene 1), a retake on scene 3 (the burn
# and the CUT! card), then scene 8 finished: "Cut. PRINT IT." and the credits twice on the way up.
import os, sys, time, json
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "sets"))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import lights_pie as L

OUT = r"C:\Jam\Builds\script\spine"
WORLD = {"world": {"refPath": "/Game/Take/UEDPIE_0_Lvl_Auction.Lvl_Auction"}, "maxResolution": 960}


def snap(s, name, prefix):
    res = L.tool(s, "SceneCaptureToolset.SceneCaptureToolset.CaptureGameViewport", WORLD)
    path = os.path.join(OUT, "%s_%s.png" % (prefix, name))
    print(name, L.save_image(res, path) and path, flush=True)


def main():
    prefix = sys.argv[1] if len(sys.argv) > 1 else "pie"
    s = L.sid()
    os.makedirs(OUT, exist_ok=True)
    if "true" in json.dumps(L.tool(s, "EditorToolset.EditorAppToolset.IsPIERunning", {})).lower():
        L.tool(s, "EditorToolset.EditorAppToolset.StopPIE", {})
        time.sleep(2.0)
    L.tool(s, "EditorToolset.EditorAppToolset.StartPIE", {})
    time.sleep(4.0)
    snap(s, "0_title", prefix)
    L.console(s, "TakeConfirm")
    time.sleep(1.5)
    try:
        for n in range(1, 9):
            L.console(s, "Reel.Script.Jump %d" % n)
            if n == 1:
                snap(s, "1_splice", prefix)
            time.sleep(1.2)
            snap(s, "%d_slug" % n, prefix)
            if n == 3:
                time.sleep(1.0)
                L.console(s, "Reel.Script.Fail")
                time.sleep(0.2)
                snap(s, "3_burn", prefix)
                time.sleep(0.6)
                snap(s, "3_cut", prefix)
                time.sleep(1.5)
                snap(s, "3_retake", prefix)
        time.sleep(1.0)
        L.console(s, "Reel.Script.Finish")
        time.sleep(1.6)
        snap(s, "9_printit", prefix)
        time.sleep(8.0)
        snap(s, "9_credits_a", prefix)
        time.sleep(10.0)
        snap(s, "9_credits_b", prefix)
    finally:
        print(L.tool(s, "EditorToolset.EditorAppToolset.StopPIE", {}), flush=True)


if __name__ == "__main__":
    main()
