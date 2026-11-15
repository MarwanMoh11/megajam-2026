# The guard brain's play-in-editor test (stealth-ai). Engine Python, repo root, under the editor lock:
#   "$PY" -I tools/stealth/guard_pie.py [prefix]     # shots into C:\Jam\Builds\qa\stealth\<prefix>_*.png
#   "$PY" -I tools/stealth/guard_pie.py --stop       # stop PIE (always before releasing the lock)
# Starts PIE (if needed), presses through the title card, warps Julian to the hall's north flank, spawns a test guard
# (Reel.Guard.Test) and shoots: patrol, a noise (turn, investigate, search, return), then Julian in his view (suspicious,
# alert, chase, caught). The GUARD log lines from the editor log are printed at the end.
import os, sys, time, json
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "sets"))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import lights_pie as L

OUT = r"C:\Jam\Builds\qa\stealth"
LOG = r"C:\Jam\megajam\Reel\Saved\Logs\Reel.log"
WORLD = {"world": {"refPath": "/Game/Take/UEDPIE_0_Lvl_Auction.Lvl_Auction"}, "maxResolution": 1280}
START = (600.0, 600.0, 0.0, 45.0, -10.0)   # the north flank, looking up the hall


def snap(s, name, prefix):
    res = L.tool(s, "SceneCaptureToolset.SceneCaptureToolset.CaptureGameViewport", WORLD)
    path = os.path.join(OUT, "%s_%s.png" % (prefix, name))
    print(time.strftime("%H:%M:%S"), name, L.save_image(res, path) and path, flush=True)


def main():
    a = sys.argv[1:]
    s = L.sid()
    if a and a[0] == "--stop":
        print(L.tool(s, "EditorToolset.EditorAppToolset.StopPIE", {}))
        return
    prefix = a[0] if a else "pie"
    os.makedirs(OUT, exist_ok=True)
    log0 = os.path.getsize(LOG) if os.path.exists(LOG) else 0
    if "true" not in json.dumps(L.tool(s, "EditorToolset.EditorAppToolset.IsPIERunning", {})).lower():
        L.tool(s, "EditorToolset.EditorAppToolset.StartPIE", {})
        time.sleep(4.0)
        L.console(s, "TakeConfirm")
        time.sleep(1.5)
    x, y, z, yaw, pitch = START
    L.console(s, "TakeWarp %g %g %g" % (x, y, z), "TakeLook %g %g" % (yaw, pitch), "Reel.Guard.Debug 1")
    time.sleep(0.6)
    # The guard 10 m ahead, side-on, pacing 3.5 m either way across the view. Julian then steps back out of his range.
    L.console(s, "Reel.Guard.Test 1000 350")
    time.sleep(0.3)
    L.console(s, "TakeWarp %g %g %g" % (x - 600 * 0.7071, y - 600 * 0.7071, z), "TakeLook %g %g" % (yaw, pitch))
    time.sleep(2.5)
    snap(s, "1_patrol", prefix)
    # A noise 4 m to the right of the guard's centre: he turns ("What was that?"), walks there, searches, comes back.
    L.console(s, "Reel.Guard.Noise 450 1600 1")
    time.sleep(0.6)
    snap(s, "2_heard", prefix)
    time.sleep(3.0)
    snap(s, "3_investigate", prefix)
    time.sleep(3.5)
    snap(s, "4_search", prefix)
    time.sleep(9.0)
    snap(s, "5_return", prefix)
    L.console(s, "Reel.Guard.Log")
    # Julian walks into his view: 6 m in front of the patrol line.
    L.console(s, "TakeWarp %g %g %g" % (x + 450 * 0.7071, y + 450 * 0.7071, z), "TakeLook %g %g" % (yaw, pitch))
    time.sleep(0.8)
    snap(s, "6_suspicious", prefix)
    time.sleep(2.5)
    snap(s, "7_alert", prefix)
    time.sleep(2.5)
    snap(s, "8_chase", prefix)
    L.console(s, "Reel.Guard.Log")
    time.sleep(0.5)
    with open(LOG, "r", encoding="utf-8", errors="replace") as f:
        f.seek(log0)
        for line in f:
            if "GUARD" in line:
                print(line.rstrip()[-170:])


if __name__ == "__main__":
    main()
