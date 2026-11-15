# Scene 8's final in-car shot in play-in-editor (s8-car-mirror): Reel.S8.Finale holds the shot at a few moments and
# each is saved to C:\Jam\Builds\script\s8\finale_<T>.png. Engine Python, repo root, under the editor lock, after
# tools/script/scene8_cabin.py has built the S8 materials:
#   "$PY" -I tools/script/scene8_finale_pie.py [T ...]      # default: 1.0 (shot A) 4.4 and 6.2 (shot B)
#   "$PY" -I tools/script/scene8_finale_pie.py --stop       # finale off, stop PIE (always before releasing the lock)
import os
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "sets"))
import lights_pie as L   # noqa: E402

OUT = r"C:\Jam\Builds\script\s8"
WORLD = {"world": {"refPath": "/Game/Take/UEDPIE_0_Lvl_Auction.Lvl_Auction"}, "maxResolution": 1280}


def main():
    a = sys.argv[1:]
    s = L.sid()
    if a and a[0] == "--stop":
        L.console(s, "Reel.S8.Finale off")
        print(L.tool(s, "EditorToolset.EditorAppToolset.StopPIE", {}))
        return
    times = a or ["1.0", "4.4", "6.2"]
    os.makedirs(OUT, exist_ok=True)
    if "true" not in str(L.tool(s, "EditorToolset.EditorAppToolset.IsPIERunning", {})).lower():
        L.tool(s, "EditorToolset.EditorAppToolset.StartPIE", {})
        time.sleep(4.0)
        L.console(s, "TakeConfirm")                       # past the title card
        time.sleep(1.5)
    for t in times:
        L.console(s, "Reel.S8.Finale %s" % t)
        time.sleep(1.5)                                     # streaming, the capture, auto exposure settling
        res = L.tool(s, "SceneCaptureToolset.SceneCaptureToolset.CaptureGameViewport", WORLD)
        path = os.path.join(OUT, "finale_%s.png" % t.replace(".", "_"))
        print(t, L.save_image(res, path) and path, flush=True)


if __name__ == "__main__":
    main()
