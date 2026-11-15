# The reel swap's play-in-editor shots into C:\Jam\Builds\script\s7swap\. Engine Python, repo root, under the lock:
#   "$PY" -I tools/script/scene7_swap_pie.py            # scrub: one still per shot of the coverage (Reel.Swap.Scrub)
#   "$PY" -I tools/script/scene7_swap_pie.py play       # the swap in real time (Reel.Swap.Play 1), a still every ~0.7 s
#   "$PY" -I tools/script/scene7_swap_pie.py hook       # part 1 from its Jump (s7-route's hook): Jump 701, then E
#   "$PY" -I tools/script/scene7_swap_pie.py --stop     # stop PIE (always before releasing the lock)
import os, sys, time, json
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "sets"))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import lights_pie as L

OUT = r"C:\Jam\Builds\script\s7swap"
WORLD = {"world": {"refPath": "/Game/Take/UEDPIE_0_Lvl_Auction.Lvl_Auction"}, "maxResolution": 1280}

# (beat, seconds into it, name): one or two moments per shot.
STILLS = [
    (1, 0.30, "b1_wide"), (1, 0.50, "b1_wide_latch"), (1, 1.00, "b1_door_swing"), (1, 1.80, "b1_door_open"),
    (2, 0.35, "b2_reel_clamp"), (2, 0.55, "b2_reel_pinch"), (2, 1.20, "b2_reel_lift"), (2, 1.90, "b2_turn"),
    (2, 2.60, "b2_turn_into_can"),
    (3, 0.50, "b3_bench_dummy"), (3, 1.50, "b3_spindle_seat"), (3, 2.20, "b3_spindle_door"),
    (4, 0.60, "b4_can_lid"), (4, 1.20, "b4_can_latch"), (4, 2.20, "b4_face_tuck"), (4, 3.40, "b4_face_glance"),
]


def snap(s, name):
    res = L.tool(s, "SceneCaptureToolset.SceneCaptureToolset.CaptureGameViewport", WORLD)
    path = os.path.join(OUT, "%s.png" % name)
    print("%.1f" % (time.time() - T0), name, L.save_image(res, path) and path, flush=True)


def main():
    global T0
    a = sys.argv[1:]
    s = L.sid()
    if a and a[0] == "--stop":
        print(L.tool(s, "EditorToolset.EditorAppToolset.StopPIE", {}))
        return
    mode = a[0] if a else "scrub"
    only = a[1:] if len(a) > 1 else None
    os.makedirs(OUT, exist_ok=True)
    if "true" not in json.dumps(L.tool(s, "EditorToolset.EditorAppToolset.IsPIERunning", {})).lower():
        L.tool(s, "EditorToolset.EditorAppToolset.StartPIE", {})
        time.sleep(4.0)
        L.console(s, "TakeConfirm")
        time.sleep(1.5)
        L.console(s, "Reel.Script.Jump 701")
        time.sleep(2.0)
    T0 = time.time()
    if mode == "scrub":
        for b, t, name in STILLS:
            if only and not any(o in name for o in only):
                continue
            L.console(s, "Reel.Swap.Scrub %d %.2f" % (b, t))
            time.sleep(0.6)
            snap(s, name)
    elif mode == "play":
        L.console(s, "Reel.Swap.Play 1")
        for i in range(17):
            time.sleep(0.7)
            snap(s, "play_%02d" % i)
    elif mode == "hook":
        snap(s, "hook_0_start")
        L.console(s, "Reel.Swap.Reset")
        print(L.console(s, "Reel.Script.Jump 701"))
        time.sleep(1.5)
        snap(s, "hook_1_mark")


if __name__ == "__main__":
    main()
