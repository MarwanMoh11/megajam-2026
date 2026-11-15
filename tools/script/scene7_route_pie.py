# Scene 7 parts 0 and 2 in play-in-editor: the rigged door, the walk to the booth, the swap (auto), the venue cut, the
# escape and the catch, walked with TakeWalkTo; a shot after every leg into C:\Jam\Builds\script\s7\. Owner: s7-route.
# Engine Python, repo root, under the editor lock:
#   "$PY" -I tools/script/scene7_route_pie.py [prefix]     # part 0 from Reel.Script.Jump 7, on through part 2
#   "$PY" -I tools/script/scene7_route_pie.py esc          # from Reel.Script.Jump 702 (the booth part) only
#   "$PY" -I tools/script/scene7_route_pie.py --stop       # stop PIE (always before releasing the lock)
import os, sys, time, json, math
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "sets"))
import lights_pie as L

OUT = r"C:\Jam\Builds\script\s7"
WORLD = {"world": {"refPath": "/Game/Take/UEDPIE_0_Lvl_Auction.Lvl_Auction"}, "maxResolution": 1280}
# (x, y, shot name or None): the route walk (330 cm/s), then the escape (470 cm/s)
ROUTE = [(3650, -2300, "r1_through_door"), (1700, -2310, None), (-300, -2300, "r2_corridor_west"),
         (-300, -1850, "r3_staff_door"), (-680, -1300, None), (-680, -1050, "r4_stair_foot"),
         (-680, -330, "r5_stair_top"), (-690, -60, "r6_booth_door"), (-860, 150, "r7_projector")]
ESCAPE = [(-1180, 0, "e1_balcony"), (-1350, -1050, "e2_balcony_south"), (-2650, -1300, "e3_room_door"),
          (-2650, -1650, "e4_caught")]


def snap(s, name, prefix):
    res = L.tool(s, "SceneCaptureToolset.SceneCaptureToolset.CaptureGameViewport", WORLD)
    path = os.path.join(OUT, "%s_%s.png" % (prefix, name))
    print("%.1f" % (time.time() - T0), name, L.save_image(res, path) and path, flush=True)


def walk(s, legs, speed, start, prefix):
    x0, y0 = start
    for (x, y, shot) in legs:
        L.console(s, "TakeWalkTo %d %d" % (x, y))
        time.sleep(math.hypot(x - x0, y - y0) / speed + 0.7)
        x0, y0 = x, y
        if shot:
            snap(s, shot, prefix)


def main():
    global T0
    a = sys.argv[1:]
    s = L.sid()
    if a and a[0] == "--stop":
        print(L.tool(s, "EditorToolset.EditorAppToolset.StopPIE", {}))
        return
    prefix = a[0] if a else "route"
    os.makedirs(OUT, exist_ok=True)
    if "true" not in json.dumps(L.tool(s, "EditorToolset.EditorAppToolset.IsPIERunning", {})).lower():
        L.tool(s, "EditorToolset.EditorAppToolset.StartPIE", {})
        time.sleep(4.0)
        L.console(s, "TakeConfirm")
        time.sleep(1.5)
    T0 = time.time()
    L.console(s, "Reel.S7.Auto 1", "Reel.Script.Jump %s" % ("702" if prefix.startswith("esc") else "7"))
    if not prefix.startswith("esc"):
        time.sleep(1.0)
        snap(s, "r0_door_opens", prefix)
        time.sleep(3.0)
        walk(s, ROUTE, 330.0, (3650, -1900), prefix)
        time.sleep(1.5)                     # the settle and the cut into the booth part
    time.sleep(1.2)
    snap(s, "s1_swap", prefix)
    time.sleep(2.2)
    snap(s, "s2_venue", prefix)
    time.sleep(1.4)
    snap(s, "s3_pendleton", prefix)
    time.sleep(2.6)
    snap(s, "s4_alarm", prefix)
    time.sleep(1.5)
    snap(s, "s5_back_on_vance", prefix)
    walk(s, ESCAPE[:1], 470.0, (-860, 150), prefix)
    time.sleep(0.4)
    snap(s, "e1b_stair_cut", prefix)
    time.sleep(1.6)
    walk(s, ESCAPE[1:], 470.0, (-1180, 0), prefix)
    time.sleep(1.5)
    snap(s, "e5_line", prefix)
    time.sleep(2.5)
    snap(s, "e6_handoff", prefix)


if __name__ == "__main__":
    main()
