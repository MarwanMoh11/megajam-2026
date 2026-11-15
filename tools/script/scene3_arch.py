# Scene 3's hall architecture (s3-foyer-hall, QA Nov 15: "right above the entrance is the area where the projector is
# stored with floating stairs? Fix that."). The projection booth over the hall doors (build_auction.py BOOTH, extended
# by tools/script/scene7.py to x -600) had a landing and six steps outside its south door that stopped 2.7 m above the
# hall floor, a balcony outside its north door on nothing, and a stair wall hanging from 2.7 m. This overlay leaves
# scene7.py's actors alone and builds what they need under and around them, so the booth reads as built in:
#   south   the stair carried on down to the hall floor (11 more steps), every step solid to the floor, a stepped
#           parapet on its hall side, newel posts at the foot, and a masonry mass between it and the hall's west wall
#   north   a pier under the balcony, a parapet on its hall side, and the masonry mass behind it up to the booth
#   shots   PNGs into C:\Jam\Builds\script\s3 (arch_*.png)
# Tag script_s3_arch (re-runnable: clears its tag first). Run under the editor lock:
#     "<engine python>" -I tools/ue_mcp.py py tools/script/scene3_arch.py 110
# Switches: S3A_OFF = True only clears. S3A_SHOTS = False skips the shots. S3A_SOUTH = True builds the south part.
# Since Nov 15 evening scene7.py (s7-route) builds the projectionist's stair itself, enclosed, x -780..-580,
# y -1160..-200, so the south part is off by default: it would sit inside their stairwell.
import sys, importlib, os
sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C
importlib.reload(C)
import unreal

TAG = "script_s3_arch"
OFF = bool(globals().get("S3A_OFF"))
SHOTS = globals().get("S3A_SHOTS", True)
SOUTH = bool(globals().get("S3A_SOUTH", False))
OUT = r"C:\Jam\Builds\script\s3"
eal = unreal.EditorAssetLibrary
CUBE = "/Engine/BasicShapes/Cube"
M = "/Game/Take/Materials/"
MAT = dict(floor=M + "MI_Box_Deck", wall=M + "MI_Box_Wall", trim=M + "MI_Box_Trim")
REPORT = []

# The booth and scene7.py's numbers (world, cm).
HALL_X0 = -1000.0                 # the hall's west wall (the doors: y -350..350, 4 m high)
DOOR_HALF = 350.0
F = 450.0                         # the booth's floor top
CEIL = 870.0                      # the booth's ceiling (under the catwalk head's grating at 880)
SX0, SX1 = -780.0, -600.0         # the stair's and the landings' width in x (scene7: -760..-600, plus the 20 cm slot)
PAR = 20.0                        # parapet thickness, on the hall side (x SX1..SX1+PAR)
RAIL = 95.0                       # parapet height over the tread
RISE, RUN = 25.0, 30.0            # scene7's steps
LAND_S = (-400.0, -220.0)         # scene7's south landing, y
LAND_N = (800.0, 1040.0)          # scene7's north landing and its end wall, y (from the booth's north wall)
STEPS_S7 = 6                      # scene7's steps: i = 0..5, y -430-30i .. -400-30i, top 425-25i
STEPS = int(F / RISE) - 1         # 17: the last one's top is 25 cm over the hall floor


def mat(k):
    return eal.load_asset(MAT[k])


def box(label, x0, x1, y0, y1, z0, z1, m, collide=True):
    return C.spawn(CUBE, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), 0.0,
                   (abs(x1 - x0) / 100.0, abs(y1 - y0) / 100.0, abs(z1 - z0) / 100.0),
                   tag=TAG, label="S3A_" + label, folder="Script/S3", collide=collide, shadow=True, mat=mat(m))


def step_y(i):
    return -430.0 - RUN * i, -400.0 - RUN * i


def south():
    n = 0
    # Under the landing: solid to the floor, with its parapet.
    box("Landing_Pier", SX0, SX1, LAND_S[0], LAND_S[1], 0.0, F - 20.0, "wall")
    box("Landing_Parapet", SX1, SX1 + PAR, LAND_S[0], LAND_S[1] + 20.0, 0.0, F + RAIL, "wall")
    box("Landing_Parapet_Cap", SX1 - 4.0, SX1 + PAR + 4.0, LAND_S[0], LAND_S[1] + 24.0, F + RAIL, F + RAIL + 6.0,
        "trim", collide=False)
    n += 3
    for i in range(STEPS):
        y0, y1 = step_y(i)
        top = F - RISE * (i + 1)
        if i < STEPS_S7:
            # scene7's tread is there (x -760..-600, 20 thick): fill under it, and the 20 cm slot by the wall.
            box("Step_Fill", SX0, SX1, y0, y1, 0.0, top - 20.0, "floor")
            box("Step_Slot", SX0, -760.0, y0, y1, top - 20.0, top, "floor")
        else:
            box("Step", SX0, SX1, y0, y1, 0.0, top, "floor")
        # The stepped parapet on the hall side, and its brass cap.
        box("Parapet", SX1, SX1 + PAR, y0, y1, 0.0, top + RAIL, "wall")
        box("Parapet_Cap", SX1 - 4.0, SX1 + PAR + 4.0, y0, y1, top + RAIL, top + RAIL + 6.0, "trim", collide=False)
        n += 3
    foot = step_y(STEPS - 1)[0]
    # Newel posts at the foot, and a brass nosing strip on the first tread.
    box("Newel", SX1 - 10.0, SX1 + PAR + 10.0, foot - 40.0, foot, 0.0, 120.0, "trim")
    box("Newel_Cap", SX1 - 14.0, SX1 + PAR + 14.0, foot - 44.0, foot + 4.0, 120.0, 128.0, "trim", collide=False)
    # The masonry between the stair and the hall's west wall, clear of the doors, up to the booth's ceiling; a cornice.
    my1 = -DOOR_HALF - 20.0
    box("Mass_South", HALL_X0, SX0, foot - 40.0, my1, 0.0, CEIL, "wall")
    box("Mass_South_Cornice", HALL_X0, SX0 + 8.0, foot - 48.0, my1 + 8.0, CEIL, CEIL + 10.0, "trim", collide=False)
    n += 4
    return "south: %d actors, %d steps to the floor (foot at y %.0f)" % (n, STEPS, foot)


def north():
    y0, y1 = LAND_N
    box("North_Pier", SX0, SX1, y0, y1, 0.0, F - 20.0, "wall")
    box("North_Parapet", SX1, SX1 + PAR, y0, y1, 0.0, F + RAIL, "wall")
    box("North_Parapet_Cap", SX1 - 4.0, SX1 + PAR + 4.0, y0 - 4.0, y1 + 4.0, F + RAIL, F + RAIL + 6.0, "trim",
        collide=False)
    box("Mass_North", HALL_X0, SX0, y0 + 20.0, y1, 0.0, CEIL, "wall")
    box("Mass_North_Cornice", HALL_X0, SX0 + 8.0, y0 + 12.0, y1 + 8.0, CEIL, CEIL + 10.0, "trim", collide=False)
    # A trim band where the booth's floor meets the piers (the booth's floor edge, z 430..450, across the front).
    box("Booth_Fascia", SX1, SX1 + PAR, LAND_S[1] + 20.0, y0, F - 50.0, F, "trim", collide=False)
    return "north: pier, parapet, mass; booth fascia"


def clashes():
    """Cast, watchers and props standing inside the new solids (to move or report)."""
    vols = [] if not SOUTH else [(SX0, SX1 + PAR, step_y(STEPS - 1)[0] - 40.0, LAND_S[1] + 20.0),
            (HALL_X0, SX0, step_y(STEPS - 1)[0] - 40.0, -DOOR_HALF - 20.0)]
    vols += [(HALL_X0, SX1 + PAR, LAND_N[0], LAND_N[1])]
    hits = []
    for a in C.actors():
        if TAG in C.tags(a) or a.get_actor_label().startswith(("S7_", "Booth_", "Wall", "Column", "Hall")):
            continue
        p = a.get_actor_location()
        if p.z > 300.0:
            continue
        for (x0, x1, y0, y1) in vols:
            if x0 - 30 <= p.x <= x1 + 30 and y0 - 30 <= p.y <= y1 + 30:
                hits.append("%s (%.0f, %.0f, %.0f)" % (a.get_actor_label(), p.x, p.y, p.z))
                break
    return "inside the new solids: " + (", ".join(hits[:30]) if hits else "nothing")


def shots():
    os.makedirs(OUT, exist_ok=True)
    REPORT.append(C.shot("arch_from_hall", (600.0, 150.0, 220.0), (-800.0, 100.0, 450.0), fov=75.0, out_dir=OUT))
    REPORT.append(C.shot("arch_south_stair", (0.0, -1100.0, 200.0), (-700.0, -500.0, 250.0), fov=70.0, out_dir=OUT))
    REPORT.append(C.shot("arch_entering", (-850.0, 60.0, 180.0), (-650.0, -100.0, 520.0), fov=85.0, out_dir=OUT))
    REPORT.append(C.shot("arch_north", (0.0, 1300.0, 200.0), (-700.0, 800.0, 350.0), fov=70.0, out_dir=OUT))


gone, shown = C.clear(TAG)
REPORT.append("cleared %d" % gone)
if not OFF:
    if SOUTH:
        REPORT.append(south())
    REPORT.append(north())
    REPORT.append(clashes())
    if SHOTS:
        shots()
"\n".join(str(r) for r in REPORT)
