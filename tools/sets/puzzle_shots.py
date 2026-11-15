# The puzzle package's shots, into C:\Jam\Builds\qa\puzzle\. Under the editor lock:
#     "<engine python>" -I tools/ue_mcp.py py tools/sets/puzzle_shots.py 110
# PUZZLE_SHOTS = ("gap", ...) takes only those; PUZZLE_SHOT_PREFIX names the set (default "after").
import sys, importlib
sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C
importlib.reload(C)

D = r"C:\Jam\Builds\qa\puzzle"
SHOTS = {
    "doors": ((-900, 0, 175), (4390, 0, 520)),          # what a guest sees coming in: the slide, nothing hanging
    "seat": ((2050, -120, 150), (4390, 0, 560)),        # from Julian's seat by the rope
    "gap": ((2080, -260, 190), (2420, 20, 70)),         # the gap: posts, eye houses, cards
    "rope": ((2230, -420, 170), (2400, -1500, 50)),     # along the velvet rope to the south wall
    "above": ((1500, -1500, 900), (2700, 0, 0)),        # the angle of Marwan's screenshot
}
want = globals().get("PUZZLE_SHOTS") or tuple(SHOTS)
pre = globals().get("PUZZLE_SHOT_PREFIX") or "after"
out = [C.shot("%s_%s" % (pre, k), SHOTS[k][0], SHOTS[k][1], fov=70, w=1280, h=720, out_dir=D) for k in want]
"\n".join(out)
