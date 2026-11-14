# The hall package's after shots, from the survey's camera positions (C:\Jam\Builds\qa\survey\survey_shots.py), into
# C:\Jam\Builds\qa\hall\. Under the editor lock:  "<engine python>" -I tools/ue_mcp.py py tools/sets/hall_shots.py 110
# HALL_SHOTS = ("band_a", ...) takes only those; HALL_SHOT_PREFIX names the set (default "after").
import sys, importlib
sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C
importlib.reload(C)

D = r"C:\Jam\Builds\qa\hall"
SHOTS = {
    "benches_a": ((-600, -1100, 165), (100, -500, 60)), "benches_b": ((800, 1100, 165), (0, 500, 50)),
    "band_a": ((3200, 900, 200), (3780, 1460, 100)), "band_b": ((3500, 1750, 165), (3780, 1400, 100)),
    "broadcast_a": ((-200, -600, 200), (300, 0, 100)), "broadcast_b": ((800, 400, 200), (300, 0, 120)),
    "bar_a": ((1200, -800, 165), (1200, -1700, 100)), "dais_a": ((3300, -900, 200), (3810, -480, 120)),
    "dais_b": ((3000, 0, 250), (4100, 0, 120)), "overview": ((-900, -1800, 600), (2500, 500, 0)),
    "band_c": ((3000, 1250, 230), (3820, 1620, 90)), "phones": ((2000, -1050, 170), (2000, -1560, 90)),
}
want = globals().get("HALL_SHOTS") or tuple(SHOTS)
pre = globals().get("HALL_SHOT_PREFIX") or "after"
out = [C.shot("%s_%s" % (pre, k), SHOTS[k][0], SHOTS[k][1], fov=70, w=960, h=540, out_dir=D) for k in want]
"\n".join(out)
