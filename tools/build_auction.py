# Builds the auction house grey-box, /Game/Take/Lvl_Auction, from nothing. Run it inside the editor:
#   "<engine python>" -I tools/ue_mcp.py py tools/build_auction.py
# It is safe to re-run: it empties the level and lays everything out again from the numbers below (cm).
#
# The building (about 85 x 62 m, two floors, open top so the flight between characters can look down into it):
#   west    the lobby block: a two-storey lobby with an atrium and a grand stair, a cloakroom and restrooms on the
#           ground floor, an office and the security room above them, and a fire stair at each end (north, south)
#   centre  the great hall: seating, the rope, the aisle and the dais; a gallery along its west end and a walkway
#           along its north wall on the second floor, with the bridge out over the floor
#   north   the north wing: the preview gallery and the stock room below; director's office, projection room, archive
#   south   the service wing: the backstage corridor behind the hatches, then kitchen, prop store and loading dock
#   east    the vault corridor and the vault, behind the dais wall
#   street  a pavement and a road along the front (west), the street all round: the take starts out here, on the red
#           carpet by the queue, and Julian walks in through the scanner at the front doors (Scene 1)
# The last section places the rules: the watchers, the devices, the marks, the scene triggers and the crew posts.
# X runs from the entrance (-X) to the dais (+X). +Y is north. Everything west of the rope connects; the only way
# past the rope is the gap and the aisle (the walkway above is railed, the hatches have counters).
# The props and people at the end are placeholders with no rules: none blocks a sight line or the aim.
import math
import random

import unreal

MAP = "/Game/Take/Lvl_Auction"
MAT_DIR = "/Game/Take/Materials"

# --- the building ----------------------------------------------------------------------------------------------------
HX0, HX1 = -1000.0, 4400.0        # the great hall, 54 m long
HY0, HY1 = -2000.0, 2000.0        # and 40 m wide
HALL_H = 1400.0
F2 = 450.0                        # the second floor (top of its slab); the catwalk is on it
CAT_Z = F2
SLAB = 20.0
# --- W3 stage --- the catwalk level: everything over the hall is hung high in the roof (tools/sets/hall_zones.json)
CAT_Z = 900.0                     # the catwalk's deck top (the hall is 14 m high; the old second floor was 4.5 m)
CAT_X0 = -760.0                   # the catwalk head runs along the projection box's front, x CAT_X0..GALLERY_X1
BOOTH = (-1000.0, -760.0, -200.0, 800.0, 1150.0)   # the projection box over the lobby doors: x0, x1, y0, y1, roof top
HANG_Z = HALL_H - 40.0            # the hangers run up to the roof trusses' underside
# --- end W3 stage
TOP = 900.0                       # top of the two-storey walls
LX0, LX1 = -3200.0, -1000.0       # the lobby block, as wide as the hall; the lobby itself is LY0..LY1
LY0, LY1 = -1400.0, 1400.0
VOID_X0, VOID_X1, VOID_Y = -2700.0, -1500.0, 800.0   # the atrium: a hole in the lobby's second floor
STRIP_X = -2100.0                 # side strips: rooms west of this, the fire stairs east of it
DOOR_HALF, DOOR_H = 350.0, 400.0  # the opening from the lobby into the hall
D_HALF, D_H = 70.0, 250.0         # an ordinary doorway
GALLERY_X1 = -600.0               # the hall's west gallery (second floor) runs from the west wall to here
BY0 = -2600.0                     # backstage corridor: from here to the hall's south wall (HY0)
BACK_H = 500.0
SY0 = -3300.0                     # service rooms: from here to the corridor
NY1 = 2900.0                      # north wing: from the hall's north wall to here
EX1 = 5300.0                      # east wing: from the hall's east wall to here
VAULT_Y0, VAULT_Y1 = -1000.0, 1000.0
HATCHES = (600.0, 3000.0, 4150.0) # service hatches in the south wall (X centres): a counter Vex looks over
HATCH_HALF, HATCH_Z0, HATCH_Z1 = 150.0, 110.0, 320.0
WALK_Y0 = 1800.0                  # the long walkway runs along the north wall from here to HY1 (W3 stage: was 1650)
BRIDGE_X0, BRIDGE_X1 = 1400.0, 1700.0   # the bridge crosses from the walkway out over the floor
BRIDGE_Y0 = -200.0                # W3 stage: was -350; the projector's throw passes south of its end
RAIL_H = 100.0
T = 20.0                          # wall thickness

# --- the street front ------------------------------------------------------------------------------------------------
PAVE_X0 = LX0 - 900.0             # the pavement in front of the building, level with its floors
ROAD_X0 = LX0 - 2300.0            # the road beyond the kerb, and the street's west edge
FRONT_DOOR_HALF, FRONT_DOOR_H = 450.0, 380.0   # the front doors stand open: three lanes wide
TERRACE_X0 = LX0 - T - 230.0      # the terrace over the doors (the old canopy): Maya's searchlight stands on it

# --- the route (Julian walks in from the street, then +X along Y = 0) -------------------------------------------------
# Scene 1 is played on the street (Marwan, Nov 14). Three lanes lead to the doors: the red carpet in the middle is
# for invited guests and the doorman looks straight down it; the queue lane north of it ends at the scanner; the
# south lane is the crew's and is roped off. Julian has no invitation: he starts at the back of the queue, which
# has backed up along the kerb, walks down it and turns into the queue lane.
CARPET_HALF = 150.0               # the carpet: Y -150..150, from the kerb to the grand stair
LANE_ROPE = 200.0                 # the ropes each side of the carpet
LANE_Y0, LANE_Y1 = 210.0, 430.0   # the queue lane
START = (PAVE_X0 + 60.0, 360.0)   # QA scene1: Julian's place in the queue, at the lane's mouth (was the back, 980)
SCAN_X = LX0 - T - 300.0          # the scanner's gate: at the queue lane's head, clear of the terrace above
SCAN_Y, SCAN_HALF = 320.0, 110.0
SCAN_DEPTH = 180.0                # its frame (and its red tape) reaches this far out in front of the gate
MARK_SCANNER = (SCAN_X - 270.0, 300.0)   # the gold mark at the scanner, 90 cm outside the tape
DOORMAN = (LX0 - T - 70.0, 0.0)   # at the doors, looking down the carpet
CHIEF = (VOID_X0 + 20.0, 0.0)     # the security chief at the foot of the grand stair, looking at the doors
MARK_DESK = (-2430.0, 630.0)      # the registration desk: Julian signs in here (W3 opening: in front of the moved desk)
MARK_FLOOR = (-700.0, 0.0)        # Scene 2 starts here: the checkpoint just inside the hall
# The broadcast drone films the centre aisle, and its camera riser blocks the aisle: Julian goes round by a side aisle.
RISER = (1450.0, 1750.0, 750.0, 1210.0)   # x0, x1, y0, y1: north of the general rows, off the doors' view (W3 stage)
MARK_SEAT = (2000.0, 560.0)       # the end of a general row (W3 stage: was -300, 1090)
# --- W3 stage --- the seats: two blocks of rows facing the dais either side of the centre aisle (hall_layout.py)
ROWS_FRONT = tuple(3150.0 - 100.0 * i for i in range(8))   # reserved bidders, past the rope: 3150 .. 2450
ROWS_REAR = tuple(2100.0 - 100.0 * i for i in range(3))    # general seats, before it: 2100 .. 1900 (round D)
ROW_Y0, ROW_Y1_FRONT, ROW_Y1_REAR = 180.0, 614.0, 490.0   # round D: every chair taken    # row blockers |y| from the aisle to the row's far end
JULIAN_PLACE = (2100.0, -211.0)   # his bidder's seat: the rear block's front row, south aisle seat, by the rope mark
# --- end W3 stage
MARK_COCKTAILS = (950.0, 1010.0)  # beside a cocktail table on the north flank, off the rows' ends (W3 stage: 1100, 950)
MARK_ROPE = (2200.0, 0.0)         # the safe mark before the rope, and a checkpoint
ROPE_X = 2400.0
GAP = 150.0                       # half-width of the gap in the rope: the beam crosses it
AISLE_X1 = 3185.0                 # the reserved rows wall the aisle from the rope to here (W3 stage: cases to 3550)
TAPE_X0, TAPE_X1 = 2450.0, 2850.0 # red tape inside the aisle: 4 m = 1.6 s at 250 cm/s  # --- W3 heist2 --- was 3550: the front rows' aisle (2850..3185) is among the bidders, where Julian waits for the dark (the front floor beyond is the room's)
DAIS_X0, DAIS_H = 3600.0, 20.0
MARK_DAIS = (3700.0, 0.0)         # the mark past the rope: a checkpoint no frame covers
GUARD = (3250.0, -450.0)          # W3 stage: at the front of the house (was 3000, -600 in what are now seats)
GUARD_YAW_ON, GUARD_YAW_OFF = 150.0, -90.0    # down the aisle for 3 bars, away (south) for 1
GUARD_RANGE, GUARD_HALF_ANGLE = 1000.0, 50.0  # covers the aisle between the rows, both edges, rope to front row
DRONE_PATH = ((-600.0, 0.0), (1750.0, 0.0))   # the drone's loop over the floor: there in 4 bars, back in 4
DRONE_RADIUS = 300.0              # its red circle; it turns back 1.5 m short of the rope mark
# The dais: Vance at the podium, the lift mark behind him, the bodyguard beside the podium.
VANCE = (3890.0, -480.0)
VANCE_RANGE, VANCE_HALF_ANGLE = 240.0, 30.0   # his frame covers the lift mark's disc and little else
MARK_LIFT = (4010.0, -480.0)
BODYGUARD = (3830.0, -330.0)
BODYGUARD_YAW, BODYGUARD_RANGE, BODYGUARD_HALF_ANGLE = -44.6, 330.0, 22.0   # its north edge passes 75 cm from the mark
DAIS_TAPE = (3620.0, 4380.0, -1150.0, -150.0)  # red tape on the dais's south half (x0, x1, y0, y1)
# Scene 3, The Getaway. The case trips the alarm; two ways out, both at the north end of the dais, neither straight.
MARK_CASE = (4060.0, 0.0)         # the gold mark at the case (it appears with the keycard): Scene 3's checkpoint
LOTS = (4190.0, 4350.0, 600.0, 800.0)   # the lots table upstage: the straight line from the case to the trap
TRAP = (4300.0, 950.0)            # the stage trap, 10 m from the case: Vex's winch opens it
PA = (4080.0, 4275.0, 1210.0, 1290.0)   # the band's PA stacks, just off the dais: the only cover from the door
BAND = (3640.0, 3920.0, 1320.0, 1600.0) # the band's riser, on the north flank
SIDE_DOOR_Y = 1400.0              # the gold side exit, in the east wall past the band
LANE_X = 4338.0                   # the lane to it, between the PA stacks and the east wall
MARK_BOW = (LANE_X, 1265.0)       # The Bow: stand here while the drone is blinded
MARK_SIDE = (LANE_X, 1400.0)      # the gold door: 14 m from the case round the lots table and the PA
SEC_DOOR_Y = -1500.0              # the security door, in the east wall on the south flank (the vault corridor)
# Four guards walk in at the alarm, two through each door: one holds the door, one walks out along the flank.
SEC_N_DOOR, SEC_N_POST = (LANE_X, 1560.0), ((4300.0, 1500.0), (4130.0, 1545.0))
SEC_S_DOOR, SEC_S_POST = (LANE_X, -1640.0), ((4300.0, -1540.0), (3900.0, -1450.0))
DOOR_RANGE, DOOR_HALF_ANGLE = 800.0, 22.0
FLANK_RANGE, FLANK_HALF_ANGLE = 900.0, 30.0
CURTAIN_H = 600.0                 # the curtain: across the front of the dais and down both its sides
AISLE_SHUTTER_X = AISLE_X1 + 15.0 # a shutter closes the aisle at the dais end: no way back
AISLE_SHUTTER_OPEN = 820.0         # W3 stage: where it hangs while open, out of the room's sight line (was 330)

# --- the crew --------------------------------------------------------------------------------------------------------
# Nobody teleports (Marwan, Nov 14): each of the crew starts where the job would be and walks to the next place.
MAYA_START = (TERRACE_X0 + 55.0, SCAN_Y)        # on the terrace over the doors, at the arrivals searchlight
SEARCHLIGHT = (TERRACE_X0 + 25.0, SCAN_Y + 150.0, F2 + 150.0)   # on a stand at the terrace's rail, over the scanner
# Her way to the hall: in through the door behind her, round the lobby balcony, through the gallery door, and out
# along a catwalk to the lighting platform over the benches (about 20 s). The follow spot and the banner's winch
# are on the platform.
CAT_Y0, CAT_Y1 = -420.0, -260.0   # the catwalk, from the west gallery (GALLERY_X1) to the platform
PLAT = (450.0, 760.0, -830.0, -200.0)   # the platform: x0, x1, y0, y1 (step 3: 2.3 m longer, for the pyro board)
SPOT = (735.0, -280.0)            # the follow spot, at the platform's east rail
SPOT_USE = (650.0, -290.0)        # where Maya stands to work it
BANNER_USE = (650.0, -510.0)      # the banner's winch, on the platform
PYRO = (735.0, -745.0)            # the pyro's firing board, at the platform's south end
PYRO_USE = (650.0, -730.0)
PYRO_POINT = (3950.0, 0.0)        # where the charges go off: the middle of the dais
PYRO_CD = 420.0                   # the flare light: bright enough to read as a flash, not a white screen
PYRO_RANGE = 1300.0               # the whole dais and the trap; not the guards at the two doors (17 m)
PYRO_FLARES = ((3640.0, -900.0), (3640.0, -450.0), (3640.0, 0.0), (3640.0, 450.0), (3640.0, 900.0))
BATTEN = (ROPE_X - 40.0, 0.0, 560.0)   # the banner hangs over the gap
VEX_START = (3650.0, -3100.0)     # on the loading dock, by the shutter the lorry is backed up to, with his kit
VEX = (3000.0, -2230.0)           # his first post: backstage at the second hatch (5 s from the dock)
CART = (3000.0, -1900.0)          # the mid-floor cart, in the hall just inside that hatch; it rolls +Y
CART_ROLL = 350.0                 # ...and crashes 9.5 m from the guard, out of reach of Vance and the bodyguard
CART_USE = (3000.0, -2080.0)      # its release, on Vex's side of the counter
CART2 = (4150.0, -1900.0)         # the dais cart, inside the third hatch
CART2_ROLL = 700.0                # it crashes into the dais's south edge: Vance and the bodyguard are in reach
CART2_USE = (4150.0, -2080.0)
DESK = (3550.0, -2460.0)          # the sound desk, against the corridor's back wall between the two hatches
DESK_USE = (3550.0, -2340.0)
DESK_SECONDS = 6.0                # the spec says 4 s; a flight back costs 1.6 s of it (STATUS.md, step 3)
WINCH = (4330.0, -2510.0)         # the curtain's counterweight winch, at the east end of the corridor (the wings)
WINCH_USE = (4300.0, -2390.0)

# --- the look (lighting; the film grade is tools/build_look.py) -------------------------------------------------------------
EXPOSURE_EV = 0.3                 # fixed exposure (EV100): a night interior
SKY_FILL = 14.0                   # skylight strength: the fill in every shadow (the night sky is dim)
MOON_LUX = 0.35                   # the directional light: cool, from behind the dais
LAMP_CD = 26.0                    # ceiling lamps under the second floor (candelas)
FOG_DENSITY = 0.03                # haze: the spots and the follow spot draw shafts in it
KEY_CD = 2300.0                   # the hall's key spots down the centre aisle
RIM_CD = 2200.0                   # the cool rim lights from the dais end
WARM = (1.0, 0.90, 0.80)          # step 4 pass 3: was (1.0, 0.82, 0.60); at the alarm's full colour it went amber
COOL = (0.55, 0.70, 1.0)
WORK = (0.70, 0.92, 1.0)          # backstage and catwalk work light
REDL = (1.0, 0.10, 0.06)
GOLDL = (1.0, 0.70, 0.20)

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def make_mi(name, rgb):
    path = MAT_DIR + "/" + name
    if eal.does_asset_exist(path):
        mi = eal.load_asset(path)
    else:
        mi = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, MAT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, eal.load_asset("/Engine/BasicShapes/BasicShapeMaterial"))
    mel.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    mel.update_material_instance(mi)
    eal.save_asset(path, only_if_is_dirty=False)
    return mi


def spawn(cls, loc, yaw=0.0, pitch=0.0, label=None, folder=None):
    a = eas.spawn_actor_from_class(cls, unreal.Vector(loc[0], loc[1], loc[2]),
                                   unreal.Rotator(roll=0.0, pitch=pitch, yaw=yaw))
    if a is None:
        raise RuntimeError("could not spawn %s" % cls)
    if label:
        a.set_actor_label(label)
    if folder:
        a.set_folder_path(folder)
    return a


def setp(obj, **props):
    for k, v in props.items():
        obj.set_editor_property(k, v)


CUBE = eal.load_asset("/Engine/BasicShapes/Cube")
N = [0]


def box(label, x0, x1, y0, y1, z0, z1, mat, folder="Set"):
    N[0] += 1
    a = spawn(unreal.StaticMeshActor, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2),
              label="%s_%03d" % (label, N[0]), folder=folder)
    c = a.static_mesh_component
    c.set_static_mesh(CUBE)
    c.set_material(0, mat)
    a.set_actor_scale3d(unreal.Vector(abs(x1 - x0) / 100.0, abs(y1 - y0) / 100.0, abs(z1 - z0) / 100.0))
    return a


def unseen(a):
    """Stops a walker but not a camera, and nobody sees it."""
    a.set_actor_hidden_in_game(True)
    c = a.static_mesh_component
    c.set_collision_response_to_channel(unreal.CollisionChannel.ECC_CAMERA, unreal.CollisionResponseType.ECR_IGNORE)
    c.set_collision_response_to_channel(unreal.CollisionChannel.ECC_VISIBILITY,
                                        unreal.CollisionResponseType.ECR_IGNORE)
    c.set_editor_property("cast_shadow", False)
    return a


def blocker(label, x0, x1, y0, y1, z0, z1):
    return unseen(box(label, x0, x1, y0, y1, z0, z1, m_wall, folder="Set/Blockers"))


def rail(x0, y0, x1, y1, z=None):
    """An open rail along a second-floor edge (a top bar and posts) with an unseen wall behind it. W3 stage: z (the
    deck's top) for the catwalk level, which also gets a knee rail."""
    z0, z1 = (F2 if z is None else z), (F2 if z is None else z) + RAIL_H
    if z is not None:
        if abs(x1 - x0) > abs(y1 - y0):
            box("Rail", min(x0, x1), max(x0, x1), y0 - 2, y0 + 2, z0 + 46, z0 + 50, m_trim, folder="Set/Rails")
        else:
            box("Rail", x0 - 2, x0 + 2, min(y0, y1), max(y0, y1), z0 + 46, z0 + 50, m_trim, folder="Set/Rails")
    if abs(x1 - x0) > abs(y1 - y0):
        x0, x1 = min(x0, x1), max(x0, x1)
        box("Rail", x0, x1, y0 - 4, y0 + 4, z1 - 8, z1, m_trim, folder="Set/Rails")
        blocker("RailBlock", x0, x1, y0 - 5, y0 + 5, z0, z1 + 60)
        n = max(1, int(round((x1 - x0) / 300.0)))
        for i in range(n + 1):
            x = x0 + (x1 - x0) * i / n
            box("Post", x - 4, x + 4, y0 - 4, y0 + 4, z0, z1 - 8, m_trim, folder="Set/Rails")
    else:
        y0, y1 = min(y0, y1), max(y0, y1)
        box("Rail", x0 - 4, x0 + 4, y0, y1, z1 - 8, z1, m_trim, folder="Set/Rails")
        blocker("RailBlock", x0 - 5, x0 + 5, y0, y1, z0, z1 + 60)
        n = max(1, int(round((y1 - y0) / 300.0)))
        for i in range(n + 1):
            y = y0 + (y1 - y0) * i / n
            box("Post", x0 - 4, x0 + 4, y - 4, y + 4, z0, z1 - 8, m_trim, folder="Set/Rails")


def wall(label, axis, pos, a0, a1, z0, z1, mat, openings=()):
    """A wall T thick. axis "x": it runs along X from a0 to a1 and fills Y = pos..pos+T. axis "y": it runs along Y
    and fills X = pos..pos+T. openings: (centre along the wall, half-width, bottom, top) cut out of it."""
    def seg(s0, s1, sz0, sz1, tag=""):
        if s1 - s0 < 1.0 or sz1 - sz0 < 1.0:
            return
        if axis == "x":
            box(label + tag, s0, s1, pos, pos + T, sz0, sz1, mat)
        else:
            box(label + tag, pos, pos + T, s0, s1, sz0, sz1, mat)
    a = a0
    for (c, h, oz0, oz1) in sorted(openings):
        seg(a, c - h, z0, z1)
        seg(c - h, c + h, z0, max(z0, oz0), "_Sill")
        seg(c - h, c + h, min(z1, oz1), z1, "_Lintel")
        a = c + h
    seg(a, a1, z0, z1)


def wall2(label, axis, pos, a0, a1, mat, ground=(), upper=(), top=TOP):
    """A two-storey wall: ground-floor doorways (centres) below, second-floor doorways (centres) above."""
    wall(label, axis, pos, a0, a1, 0.0, F2 - SLAB, mat, [(c, D_HALF, 0.0, D_H) for c in ground])
    wall(label, axis, pos, a0, a1, F2 - SLAB, top, mat, [(c, D_HALF, F2, F2 + D_H) for c in upper])


def stairs(label, x, y, yaw, run, rise, width, z0, n, mat, base=0.0):
    """n steps climbing from (x, y, z0) in the direction yaw (0, 90, 180 or 270), solid down to the floor (W3 stage:
    or down to base), with an unseen ramp over the step edges so a walker glides up instead of bumping."""
    dx, dy = round(math.cos(math.radians(yaw))), round(math.sin(math.radians(yaw)))
    d, h = run / n, rise / n
    for i in range(n):
        a0, a1 = i * d, (i + 1) * d
        if dx:
            xs, ys = sorted((x + dx * a0, x + dx * a1)), (y - width / 2, y + width / 2)
        else:
            xs, ys = (x - width / 2, x + width / 2), sorted((y + dy * a0, y + dy * a1))
        box(label, xs[0], xs[1], ys[0], ys[1], base, z0 + (i + 1) * h, mat, folder="Set/Stairs")
    # The ramp's top face runs from one tread before the first step (z0) to the top step's edge (z0 + rise).
    p = math.atan2(rise, run)
    th = 20.0
    am = (n - 2) * d / 2.0
    cx = x + dx * am + math.sin(p) * dx * th / 2
    cy = y + dy * am + math.sin(p) * dy * th / 2
    cz = z0 + rise / 2.0 - math.cos(p) * th / 2
    N[0] += 1
    a = spawn(unreal.StaticMeshActor, (cx, cy, cz), yaw=float(yaw), pitch=math.degrees(p),
              label="%s_Ramp_%03d" % (label, N[0]), folder="Set/Blockers")
    a.static_mesh_component.set_static_mesh(CUBE)
    a.static_mesh_component.set_material(0, m_wall)
    a.set_actor_scale3d(unreal.Vector(math.hypot(run, rise) / 100.0, width / 100.0, th / 100.0))
    unseen(a)


def exit_sign(axis, face, c, inward, z=262.0, half=35.0):
    """A green EXIT sign on a wall face. axis "x": the wall runs along X and its face is Y = face."""
    lo, hi = sorted((face, face + inward * 10.0))
    if axis == "x":
        box("Exit_Sign", c - half, c + half, lo, hi, z, z + 28, m_exit, folder="Set/Exits")
    else:
        box("Exit_Sign", lo, hi, c - half, c + half, z, z + 28, m_exit, folder="Set/Exits")


def fire_door(axis, face, c, inward, side=1):
    """A closed fire exit on a wall face: the door, its push bar, the sign over it, and a red extinguisher and
    alarm point on the wall beside it (side picks which side)."""
    lo, hi = sorted((face, face + inward * 8.0))
    blo, bhi = sorted((face + inward * 8.0, face + inward * 14.0))
    elo, ehi = sorted((face, face + inward * 12.0))
    e = c + side * 95.0
    if axis == "x":
        box("Fire_Door", c - 60, c + 60, lo, hi, 0, 230, m_door, folder="Set/Exits")
        box("Fire_Door_Bar", c - 50, c + 50, blo, bhi, 100, 108, m_trim, folder="Set/Exits")
        deco("Extinguisher", e - 7, e + 7, elo, ehi, 60, 104, m_red, folder="Set/Exits")
        deco("Fire_Alarm", e - 6, e + 6, lo, hi, 135, 147, m_red, folder="Set/Exits")
    else:
        box("Fire_Door", lo, hi, c - 60, c + 60, 0, 230, m_door, folder="Set/Exits")
        box("Fire_Door_Bar", blo, bhi, c - 50, c + 50, 100, 108, m_trim, folder="Set/Exits")
        deco("Extinguisher", elo, ehi, e - 7, e + 7, 60, 104, m_red, folder="Set/Exits")
        deco("Fire_Alarm", lo, hi, e - 6, e + 6, 135, 147, m_red, folder="Set/Exits")
    exit_sign(axis, face, c, inward)


def tint(rgb):
    return unreal.Color(r=int(rgb[0] * 255), g=int(rgb[1] * 255), b=int(rgb[2] * 255), a=255)


def lamp(x, y, z=F2 - SLAB - 30.0, cd=LAMP_CD, radius=1200.0, rgb=None):
    """A ceiling lamp for a room under the second floor (the moon can't reach it). No shadows: cheap."""
    N[0] += 1
    a = spawn(unreal.PointLight, (x, y, z), label="Lamp_%03d" % N[0], folder="Light/Lamps")
    c = a.get_component_by_class(unreal.PointLightComponent)
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    try:
        c.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    except Exception as e:
        unreal.log_warning("lamp units: %s" % e)
    setp(c, intensity=cd, attenuation_radius=radius, cast_shadows=False, light_color=tint(rgb or WARM))
    return a


def spot(label, x, y, z, yaw, pitch, cd, cone, radius, rgb, shadows=False, haze=1.0):
    """A movable spot light: the keys, the rims and the accents of the look (docs/STATUS.md, "Look")."""
    N[0] += 1
    a = spawn(unreal.SpotLight, (x, y, z), yaw=yaw, pitch=pitch, label="%s_%03d" % (label, N[0]),
              folder="Light/Spots")
    c = a.get_component_by_class(unreal.SpotLightComponent)
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    try:
        c.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    except Exception as e:
        unreal.log_warning("spot units: %s" % e)
    setp(c, intensity=cd, attenuation_radius=radius, outer_cone_angle=cone, inner_cone_angle=cone * 0.6,
         cast_shadows=shadows, volumetric_scattering_intensity=haze, light_color=tint(rgb))
    return a


def use_offset(actor, world_xyz):
    """A world point as an offset in the actor's own space (for a device's use point)."""
    return unreal.MathLibrary.inverse_transform_location(actor.get_actor_transform(), unreal.Vector(*world_xyz))


# --- placeholder props and people (no rules: none of them blocks a sight line, the aim or a camera) -------------------
SPHERE = eal.load_asset("/Engine/BasicShapes/Sphere")
CYL = eal.load_asset("/Engine/BasicShapes/Cylinder")
IGNORE = unreal.CollisionResponseType.ECR_IGNORE
rnd = random.Random(7)             # the same layout on every run
PICK = [0, 0]                      # running counters: guest colours, picture colours


def shape(mesh, label, x, y, z, sx, sy, sz, mat, yaw=0.0, pitch=0.0, folder="Set/Props"):
    """A basic shape centred on (x, y, z), sx by sy by sz cm (before it is turned)."""
    N[0] += 1
    a = spawn(unreal.StaticMeshActor, (x, y, z), yaw=yaw, pitch=pitch, label="%s_%03d" % (label, N[0]),
              folder=folder)
    c = a.static_mesh_component
    c.set_static_mesh(mesh)
    c.set_material(0, mat)
    a.set_actor_scale3d(unreal.Vector(sx / 100.0, sy / 100.0, sz / 100.0))
    return a


def soft(a):
    """Stops a walker; cameras, sight lines and the aim pass through it."""
    c = a.static_mesh_component
    c.set_collision_response_to_channel(unreal.CollisionChannel.ECC_CAMERA, IGNORE)
    c.set_collision_response_to_channel(unreal.CollisionChannel.ECC_VISIBILITY, IGNORE)
    return a


def ghost(a):
    """Dressing nobody can touch."""
    a.static_mesh_component.set_collision_profile_name("NoCollision")
    return a


def prop(label, x0, x1, y0, y1, z0, z1, mat, folder="Set/Props"):
    return soft(box(label, x0, x1, y0, y1, z0, z1, mat, folder=folder))


def deco(label, x0, x1, y0, y1, z0, z1, mat, folder="Set/Props"):
    return ghost(box(label, x0, x1, y0, y1, z0, z1, mat, folder=folder))


def flat(label, x0, x1, y0, y1, mat, z=0.0, th=1.2):
    """A carpet or a painted line on a floor."""
    a = deco(label, x0, x1, y0, y1, z, z + th, mat)
    a.static_mesh_component.set_editor_property("cast_shadow", False)
    return a


def guest(x, y, z=0.0, facing=0.0, seated=False, mat=None):
    """A placeholder person: a body with shoulders and a head. Not a watcher: no frame, no rules."""
    PICK[0] += 1
    m = mat or GUEST_MATS[(PICK[0] * 7) % len(GUEST_MATS)]
    h = 62.0 if seated else 150.0
    body = shape(CYL, "Guest", x, y, z + h / 2, 26.0, 48.0, h, m, yaw=facing, folder="Set/Guests")
    ghost(shape(SPHERE, "Guest_Head", x, y, z + h + 12.0, 24.0, 24.0, 26.0, m_head, folder="Set/Guests"))
    return ghost(body) if seated else soft(body)


def table(x, y, w, d, h=75.0, z=0.0, mat=None):
    prop("Table", x - w / 2, x + w / 2, y - d / 2, y + d / 2, z + h - 6, z + h, mat or m_trim)
    prop("Table_Foot", x - w / 6, x + w / 6, y - d / 6, y + d / 6, z, z + h - 6, m_case)


def round_table(x, y, z=0.0, dia=70.0, h=105.0):
    soft(shape(CYL, "Cocktail_Table", x, y, z + h - 2, dia, dia, 4.0, m_head))
    soft(shape(CYL, "Cocktail_Stem", x, y, z + (h - 4) / 2, 8.0, 8.0, h - 4, m_trim))
    ghost(shape(CYL, "Glass", x + 12, y - 8, z + h + 7, 6.0, 6.0, 14.0, m_gold))
    ghost(shape(CYL, "Glass", x - 14, y + 10, z + h + 7, 6.0, 6.0, 14.0, m_gold))


def mingle(x, y, n=2, z=0.0):
    """A cocktail table with n guests standing round it."""
    round_table(x, y, z)
    a0 = rnd.uniform(0.0, 360.0)
    for i in range(n):
        a = a0 + i * 360.0 / n + rnd.uniform(-25.0, 25.0)
        guest(x + 82 * math.cos(math.radians(a)), y + 82 * math.sin(math.radians(a)), z=z, facing=a + 180.0)


def chair(x, y, facing, z=0.0, mat=None):
    """facing: 0, 90, 180 or 270, the way the sitter looks."""
    mat = mat or m_trim
    fx, fy = round(math.cos(math.radians(facing))), round(math.sin(math.radians(facing)))
    prop("Chair", x - 22, x + 22, y - 22, y + 22, z, z + 45, mat)
    if fx:
        prop("Chair_Back", x - fx * 22, x - fx * 16, y - 22, y + 22, z + 45, z + 92, mat)
    else:
        prop("Chair_Back", x - 22, x + 22, y - fy * 22, y - fy * 16, z + 45, z + 92, mat)


def stool(x, y, z=0.0):
    soft(shape(CYL, "Stool", x, y, z + 35, 34.0, 34.0, 70.0, m_trim))


def sofa(x0, x1, y0, y1, z, back):
    """back: which side the backrest is on, "W", "E", "N" or "S"."""
    prop("Sofa", x0, x1, y0, y1, z, z + 42, m_sofa)
    bx0, bx1, by0, by1 = {"W": (x0, x0 + 18, y0, y1), "E": (x1 - 18, x1, y0, y1),
                          "S": (x0, x1, y0, y0 + 18), "N": (x0, x1, y1 - 18, y1)}[back]
    prop("Sofa_Back", bx0, bx1, by0, by1, z + 42, z + 85, m_sofa)


def plant(x, y, z=0.0):
    prop("Planter", x - 22, x + 22, y - 22, y + 22, z, z + 45, m_trim)
    ghost(shape(SPHERE, "Plant", x, y, z + 88, 78.0, 78.0, 100.0, m_plant))


def painting(axis, face, c, inward, z=150.0, w=160.0, h=110.0):
    """A framed picture on a wall face (face and inward as for exit_sign)."""
    PICK[1] += 1
    mat = ART_MATS[PICK[1] % len(ART_MATS)]
    f0, f1 = sorted((face, face + inward * 3.0))
    p0, p1 = sorted((face + inward * 3.0, face + inward * 5.0))
    if axis == "x":
        deco("Frame", c - w / 2 - 8, c + w / 2 + 8, f0, f1, z - 8, z + h + 8, m_gold)
        deco("Picture", c - w / 2, c + w / 2, p0, p1, z, z + h, mat)
    else:
        deco("Frame", f0, f1, c - w / 2 - 8, c + w / 2 + 8, z - 8, z + h + 8, m_gold)
        deco("Picture", p0, p1, c - w / 2, c + w / 2, z, z + h, mat)


def easel(x, y, z=0.0):
    """A lot on a stand, its picture facing -X (toward the room's west)."""
    PICK[1] += 1
    prop("Easel", x + 3, x + 9, y - 4, y + 4, z, z + 160, m_trim)
    deco("Easel_Frame", x - 3, x + 3, y - 52, y + 52, z + 62, z + 178, m_gold)
    deco("Easel_Picture", x - 5, x - 3, y - 45, y + 45, z + 70, z + 170, ART_MATS[PICK[1] % len(ART_MATS)])


def rope_line(pts, z=0.0):
    """Gold stanchions at each point and a rope between neighbours (straight along X or Y)."""
    for (x, y) in pts:
        soft(shape(CYL, "Stanchion", x, y, z + 47, 7.0, 7.0, 94.0, m_gold))
        ghost(shape(CYL, "Stanchion_Foot", x, y, z + 1.5, 28.0, 28.0, 3.0, m_gold))
    for (ax, ay), (bx, by) in zip(pts, pts[1:]):
        if ay == by:
            prop("Rope", min(ax, bx), max(ax, bx), ay - 2, ay + 2, z + 76, z + 82, m_rope)
        else:
            prop("Rope", ax - 2, ax + 2, min(ay, by), max(ay, by), z + 76, z + 82, m_rope)


def tripod(x, y, z=0.0):
    soft(shape(CYL, "Tripod", x, y, z + 65, 5.0, 5.0, 130.0, m_trim))
    deco("Camera", x - 12, x + 12, y - 8, y + 8, z + 130, z + 146, m_floor)


def crates(x, y, sizes, z=0.0, mat=None):
    """Crates stacked on (x, y): one per size, each centred on the one below."""
    for s in sizes:
        box("Crate", x - s / 2, x + s / 2, y - s / 2, y + s / 2, z, z + s, mat or m_trim, folder="Set/Props")
        z += s


def car(x, y, length, mat, z=-21.0):
    """A parked car, lengthways along Y."""
    box("Car", x - 90, x + 90, y - length / 2, y + length / 2, z + 24, z + 84, mat, folder="Set/Street")
    box("Car_Cabin", x - 80, x + 80, y - length * 0.27, y + length * 0.2, z + 84, z + 134, m_window,
        folder="Set/Street")
    for sx in (-1, 1):
        for sy in (-1, 1):
            wy = y + sy * (length / 2 - 75)
            deco("Wheel", x + sx * 78, x + sx * 96, wy - 30, wy + 30, z, z + 60, m_tire, folder="Set/Street")


def tree(x, y, z=-21.0):
    box("Trunk", x - 15, x + 15, y - 15, y + 15, z, z + 260, m_trunk, folder="Set/Street")
    ghost(shape(SPHERE, "Tree", x, y, z + 390, 340.0, 340.0, 300.0, m_plant, folder="Set/Street"))


def street_lamp(x, y, arm=-1):
    box("Lamp_Post", x - 6, x + 6, y - 6, y + 6, 0, 480, m_trim, folder="Set/Street")
    deco("Lamp_Arm", x, x + arm * 100, y - 4, y + 4, 470, 478, m_trim, folder="Set/Street")
    deco("Lamp_Head", x + arm * 60, x + arm * 104, y - 10, y + 10, 458, 470, m_head, folder="Set/Street")


# --- the level -------------------------------------------------------------------------------------------------------
if eal.does_asset_exist(MAP):
    les.load_level(MAP)
else:
    les.new_level(MAP)
for old in eas.get_all_level_actors():
    eas.destroy_actor(old)

m_floor = make_mi("MI_Box_Floor", (0.045, 0.045, 0.05))
m_wall = make_mi("MI_Box_Wall", (0.16, 0.16, 0.18))
m_rope = make_mi("MI_Box_Rope", (0.30, 0.05, 0.05))
m_trim = make_mi("MI_Box_Trim", (0.30, 0.30, 0.33))
m_deck = make_mi("MI_Box_Deck", (0.07, 0.10, 0.15))
m_dais = make_mi("MI_Box_Dais", (0.10, 0.075, 0.05))
m_case = make_mi("MI_Box_Case", (0.20, 0.24, 0.27))
m_gold = make_mi("MI_Box_Gold", (1.0, 0.62, 0.08))
m_exit = make_mi("MI_Box_Exit", (0.03, 1.0, 0.22))
m_door = make_mi("MI_Box_Door", (0.07, 0.17, 0.11))
m_street = make_mi("MI_Box_Street", (0.02, 0.02, 0.022))
m_pave = make_mi("MI_Box_Pave", (0.10, 0.10, 0.105))
m_carpet = make_mi("MI_Box_Carpet", (0.33, 0.03, 0.04))
m_window = make_mi("MI_Box_Window", (0.02, 0.05, 0.09))
m_plant = make_mi("MI_Box_Plant", (0.05, 0.22, 0.07))
m_trunk = make_mi("MI_Box_Trunk", (0.10, 0.06, 0.035))
m_tire = make_mi("MI_Box_Tire", (0.012, 0.012, 0.012))
m_red = make_mi("MI_Box_Red", (0.75, 0.04, 0.03))
m_head = make_mi("MI_Box_Head", (0.55, 0.50, 0.45))
m_glow = make_mi("MI_Box_Glow", (0.15, 0.55, 0.95))
m_sofa = make_mi("MI_Box_Sofa", (0.16, 0.07, 0.05))
GUEST_MATS = [make_mi("MI_Guest_%d" % i, c) for i, c in enumerate(
    ((0.03, 0.04, 0.09), (0.05, 0.05, 0.055), (0.22, 0.04, 0.07), (0.42, 0.38, 0.30), (0.07, 0.16, 0.14)))]
ART_MATS = [make_mi("MI_Art_%d" % i, c) for i, c in enumerate(
    ((0.05, 0.25, 0.28), (0.55, 0.33, 0.08), (0.30, 0.06, 0.10), (0.12, 0.16, 0.40)))]
CAR_MATS = [make_mi("MI_Car_%d" % i, c) for i, c in enumerate(
    ((0.35, 0.03, 0.03), (0.03, 0.07, 0.22), (0.35, 0.36, 0.38), (0.015, 0.015, 0.018)))]

# Step 4, the film look (docs/ART_STYLE.md §15.3): its assets come from tools/build_look.py (run here if missing).
# Only what carries meaning keeps its colour: those meshes are unlit signal colours with a custom-depth stencil
# (1 red = danger, 2 gold = Julian). Red or gold anywhere else is dressing and goes monochrome in the grade.
if not eal.does_asset_exist("/Game/Take/Look/MI_FilmGrade"):
    _look = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir() + "../tools/build_look.py")
    exec(open(_look).read(), {"__name__": "build_look"})
m_sig_red = eal.load_asset("/Game/Take/Look/MI_Signal_Red")
m_sig_gold = eal.load_asset("/Game/Take/Look/MI_Signal_Gold")
GRADE = eal.load_asset("/Game/Take/Look/MI_FilmGrade")


def signal(a, mat, stencil):
    """A map mesh that keeps its colour in the grade: unlit signal material, custom depth with this stencil."""
    c = a.static_mesh_component
    c.set_material(0, mat)
    c.set_editor_property("render_custom_depth", True)
    c.set_editor_property("custom_depth_stencil_value", stencil)
    return a


# Ground floors (they run under the walls, so doorways have a floor), and the street around the building: a road
# along the front, 21 cm below the pavement, and unseen walls at the street's edge so nobody walks off it.
SX0, SX1, SYA, SYB = ROAD_X0, EX1 + 1200, SY0 - 1200, NY1 + 1200
box("Street", SX0, SX1, SYA, SYB, -40, -21, m_street)
box("Pavement", PAVE_X0, LX0, HY0 - 400, HY1 + 400, -21, 0, m_pave)
blocker("Street_Edge", SX0 - 20, SX0, SYA, SYB, -40, 400)
blocker("Street_Edge", SX1, SX1 + 20, SYA, SYB, -40, 400)
blocker("Street_Edge", SX0, SX1, SYA - 20, SYA, -40, 400)
blocker("Street_Edge", SX0, SX1, SYB, SYB + 20, -40, 400)
box("Floor_Hall", HX0, HX1, HY0, HY1, -20, 0, m_floor)
box("Floor_Lobby", LX0, LX1, HY0, HY1, -20, 0, m_floor)
box("Floor_Backstage", HX0, HX1, BY0, HY0, -20, 0, m_deck)
box("Floor_Service", HX0, HX1, SY0, BY0, -20, 0, m_deck)
box("Floor_North", HX0, HX1, HY1, NY1, -20, 0, m_floor)
box("Floor_East", HX1, EX1, BY0, VAULT_Y1, -20, 0, m_deck)

# --- the great hall ----------------------------------------------------------------------------------------------------
# West: the lobby opening below, the gallery door above. North: two doorways into the preview gallery (both west of
# the rope) and two second-floor doors off the walkway. South: the hatches and a staff door. East: solid.
wall("Hall_West", "y", HX0 - T, HY0, HY1, 0, F2 - SLAB, m_wall, [(0.0, DOOR_HALF, 0.0, DOOR_H)])
wall("Hall_West", "y", HX0 - T, HY0, HY1, F2 - SLAB, HALL_H, m_wall, [(0.0, 150.0, F2, F2 + D_H)])
box("Hall_East", HX1, HX1 + T, HY0 - T, HY1 + T, 0, HALL_H, m_wall)
wall("Hall_North", "x", HY1, HX0 - T, HX1, 0, F2 - SLAB, m_wall, [(150.0, 100.0, 0.0, D_H), (1900.0, 100.0, 0.0, D_H)])
wall("Hall_North", "x", HY1, HX0 - T, HX1, F2 - SLAB, HALL_H, m_wall)   # W3 stage: no upper doors (no walkway at F2)
wall("Hall_South", "x", HY0 - T, HX0 - T, HX1, 0, HALL_H, m_wall,
     [(c, HATCH_HALF, HATCH_Z0, HATCH_Z1) for c in HATCHES] + [(-300.0, D_HALF, 0.0, D_H)])
exit_sign("y", HX0, 0.0, 1, z=400.0, half=60.0)

# Columns down both sides of the hall, and trusses across the open top.
for cx in (-500.0, 400.0, 1300.0, 2200.0, 3100.0, 4000.0):
    for cy in (-1300.0, 1300.0):
        box("Column", cx - 60, cx + 60, cy - 60, cy + 60, 0, HALL_H, m_trim)
    box("Truss", cx - 15, cx + 15, HY0, HY1, HALL_H - 40, HALL_H, m_trim)

# Auction seating: rows either side of the centre aisle, from 4.5 m off the dais back toward the doors (W3 stage). Each
# row's hidden box keeps the collision (tools/sets/hall.py stands the chairs on it), so the rows wall the aisle.
for (rows, y1) in ((ROWS_FRONT, ROW_Y1_FRONT), (ROWS_REAR, ROW_Y1_REAR)):
    for bx in rows:
        box("Bench", bx - 35, bx + 35, ROW_Y0, y1, 0, 60, m_trim)
        box("Bench", bx - 35, bx + 35, -y1, -ROW_Y0, 0, 60, m_trim)

# Lots on plinths along the north wall, the champagne bar and the phone-bid desks on the south side.
for lx in (-300.0, 600.0, 2500.0, 3400.0):
    box("Plinth", lx - 40, lx + 40, 1480, 1560, 0, 100, m_trim)
    box("Lot", lx - 25, lx + 25, 1495, 1545, 100, 160, m_case)
box("Bar", 800, 1600, -1560, -1470, 0, 110, m_trim)
box("Phone_Desk", 1800, 2200, -1560, -1480, 0, 100, m_trim)

# --- W3 stage --- The catwalk level, high in the roof (deck top CAT_Z, open rails, hung on rods from the trusses, no
# legs). Every deck is open grating: it stops a walker but not a sight line or a camera, so the drone and the follow
# spot see through it. The way up is the projection box over the lobby doors: the gallery door (F2) opens into it,
# two flights climb inside, and its front door lets out onto the catwalk head at CAT_Z.
bx0, bx1, by0, by1, btop = BOOTH
box("Booth_Floor", bx0, bx1, by0, by1, F2 - SLAB, F2, m_deck, folder="Set/Catwalk")
wall("Booth_Front", "y", bx1 - T, by0 - T, by1 + T, F2 - SLAB, btop, m_wall, [(0.0, D_HALF, CAT_Z, CAT_Z + 220.0)])
wall("Booth_Side", "x", by0 - T, bx0, bx1 - T, F2 - SLAB, btop, m_wall)
wall("Booth_Side", "x", by1, bx0, bx1 - T, F2 - SLAB, btop, m_wall)
box("Booth_Roof", bx0, bx1, by0 - T, by1 + T, btop - SLAB, btop, m_deck, folder="Set/Catwalk")
for py in (by0 + 60.0, by1 - 60.0):                       # corbels under the box, on the back wall
    box("Booth_Corbel", bx0, bx0 + 60.0, py - 20, py + 20, F2 - SLAB - 120.0, F2 - SLAB, m_trim, folder="Set/Catwalk")
for py in (-500.0, 500.0):                                # the projection ports in its front (dark glass)
    box("Booth_Port", bx1 - 1.0, bx1 + 1.0, py - 30, py + 30, 700.0, 740.0, m_glow, folder="Set/Catwalk")
lane_a, lane_b, mid_y = (bx0 + 55.0), (bx1 - T - 55.0), 514.0     # two lanes 110 wide; the landing between them
half = (CAT_Z - F2) / 2.0
stairs("Booth_Stair", lane_a, 150.0, 90.0, mid_y - 150.0, half, 110.0, F2, 13, m_deck, base=F2 - SLAB)
box("Booth_Landing", bx0, bx1 - T, mid_y, by1, F2 - SLAB, F2 + half, m_deck, folder="Set/Stairs")
stairs("Booth_Stair", lane_b, mid_y, 270.0, mid_y - 150.0, half, 110.0, F2 + half, 13, m_deck, base=F2 - SLAB)
box("Booth_Top", bx0, bx1 - T, by0, 150.0, CAT_Z - SLAB, CAT_Z, m_deck, folder="Set/Catwalk")
rail(bx0, 150.0, lane_b - 55.0, 150.0, z=CAT_Z)             # the top landing's edge over the first flight
lamp((bx0 + bx1) / 2, 300.0, z=btop - 40.0, cd=6.0, radius=500.0, rgb=WORK)

# The catwalk head along the box's front, the catwalk out to the lighting platform, the walkway along the north wall
# and the bridge from it out over the house.
deck = lambda lbl, x0, x1, y0, y1: soft(box(lbl, x0, x1, y0, y1, CAT_Z - SLAB, CAT_Z, m_deck, folder="Set/Catwalk"))
deck("Catwalk_Head", CAT_X0, GALLERY_X1, -450.0, HY1)
deck("Catwalk", GALLERY_X1, PLAT[0], CAT_Y0, CAT_Y1)
deck("Light_Platform", PLAT[0], PLAT[1], PLAT[2], PLAT[3])
deck("Walkway", GALLERY_X1, HX1, WALK_Y0, HY1)
deck("Bridge", BRIDGE_X0, BRIDGE_X1, BRIDGE_Y0, WALK_Y0)
rail(CAT_X0, -450.0, CAT_X0, by0 - T, z=CAT_Z)
rail(CAT_X0, by1 + T, CAT_X0, HY1, z=CAT_Z)
rail(CAT_X0, -450.0, GALLERY_X1, -450.0, z=CAT_Z)
rail(GALLERY_X1, -450.0, GALLERY_X1, CAT_Y0, z=CAT_Z)
rail(GALLERY_X1, CAT_Y1, GALLERY_X1, WALK_Y0, z=CAT_Z)
rail(GALLERY_X1, CAT_Y0, PLAT[0], CAT_Y0, z=CAT_Z)
rail(GALLERY_X1, CAT_Y1, PLAT[0], CAT_Y1, z=CAT_Z)
rail(PLAT[0], PLAT[2], PLAT[0], CAT_Y0, z=CAT_Z)
rail(PLAT[0], CAT_Y1, PLAT[0], PLAT[3], z=CAT_Z)
rail(PLAT[0], PLAT[2], PLAT[1], PLAT[2], z=CAT_Z)
rail(PLAT[0], PLAT[3], PLAT[1], PLAT[3], z=CAT_Z)
rail(PLAT[1], PLAT[2], PLAT[1], PLAT[3], z=CAT_Z)
rail(GALLERY_X1, WALK_Y0, BRIDGE_X0, WALK_Y0, z=CAT_Z)
rail(BRIDGE_X1, WALK_Y0, HX1, WALK_Y0, z=CAT_Z)
rail(BRIDGE_X0, BRIDGE_Y0, BRIDGE_X0, WALK_Y0, z=CAT_Z)
rail(BRIDGE_X1, BRIDGE_Y0, BRIDGE_X1, WALK_Y0, z=CAT_Z)
rail(BRIDGE_X0, BRIDGE_Y0, BRIDGE_X1, BRIDGE_Y0, z=CAT_Z)


def hanger(x, y):
    box("Catwalk_Hanger", x - 3, x + 3, y - 3, y + 3, CAT_Z, HANG_Z, m_trim, folder="Set/Catwalk")


def header(x0, x1, y0, y1):
    """A steel beam just under the trusses, carrying the hangers between them."""
    box("Catwalk_Header", x0, x1, y0, y1, HANG_Z - 30.0, HANG_Z, m_trim, folder="Set/Catwalk")


for y in (-450.0, 300.0, 1100.0, 1900.0):                 # the head: rods on its open edge, a beam to the truss at -500
    hanger(GALLERY_X1 - 8, y)
header(GALLERY_X1 - 15, GALLERY_X1, -460.0, HY1)
for x in (-500.0, 0.0, 400.0):                            # the catwalk: both rails, at the trusses and between
    for y in (CAT_Y0 - 8, CAT_Y1 + 8):
        hanger(x, y)
for (x, y) in ((PLAT[0] + 8, PLAT[2] - 8), (PLAT[1] + 8, PLAT[2] - 8), (PLAT[0] + 8, PLAT[3] + 8), (PLAT[1] + 8, PLAT[3] + 8)):
    hanger(x, y)
for y in (PLAT[2] - 8, PLAT[3] + 8):                      # the platform's beams, from the truss at 400
    header(390.0, PLAT[1] + 15, y - 8, y + 8)
for x in (-500.0, 400.0, 1300.0, 2200.0, 3100.0, 4000.0):  # the walkway's open edge, at every truss
    hanger(x, WALK_Y0 - 8)
for x in (BRIDGE_X0 - 8, BRIDGE_X1 + 8):                  # the bridge: rods each side, beams from the north wall
    for y in (BRIDGE_Y0, 500.0, 1200.0):
        hanger(x, y)
# --- end W3 stage
# The broadcast's camera riser fills the centre aisle at the head of the rear rows.
box("Broadcast_Riser", RISER[0], RISER[1], RISER[2], RISER[3], 0, 80, m_trim)

# The rope: a low wall with a gap. Past it, display cases wall an aisle to the dais.
box("Rope_South", ROPE_X - 10, ROPE_X + 10, HY0, -GAP, 0, 110, m_rope)
box("Rope_North", ROPE_X - 10, ROPE_X + 10, GAP, HY1, 0, 110, m_rope)
# W3 stage: no display cases: the reserved rows wall the aisle (their Bench boxes above).

# The dais: a low stage with the podium, the gold case, and a dark screen on the end wall.
box("Dais", DAIS_X0, HX1, -1200, 1200, 0, DAIS_H, m_dais)
box("Podium", 3780, 3840, -520, -440, DAIS_H, DAIS_H + 120, m_trim)
box("Case_Plinth", 4130, 4190, -30, 30, DAIS_H, DAIS_H + 90, m_trim)
signal(box("Case_Reel", 4140, 4180, -20, 20, DAIS_H + 90, DAIS_H + 130, m_gold), m_sig_gold, 2)
box("Screen", HX1 - 10, HX1, -500, 500, 235, 965, m_floor)   # W3 stage: 10 x 7.3 m, an old print's frame (was 14 x 6)

# Scene 3's set. The lots table stands between the case and the stage trap; the band's PA stacks stand between
# the dais and the gold side exit and are the only cover from the guards at that door; a shutter frame closes the
# aisle; the two doors in the end wall are where security comes in.
box("Lots_Table", LOTS[0], LOTS[1], LOTS[2], LOTS[3], DAIS_H, DAIS_H + 95, m_trim)
box("PA_Stack", PA[0], PA[1], PA[2], PA[3], 0, 250, m_floor)
box("Band_Riser", BAND[0], BAND[1], BAND[2], BAND[3], 0, 30, m_dais)
# W3 stage: no grey gantry over the aisle's front mouth (it stood in the room's sight line to the dais): the aisle's
# shutter hangs open up in the dark under the catwalk level and drops from there (Scene 3 block, AISLE_SHUTTER_OPEN).
signal(box("Side_Exit", HX1 - 8, HX1, SIDE_DOOR_Y - 65, SIDE_DOOR_Y + 65, 0, 235, m_gold), m_sig_gold, 2)
box("Security_Door", HX1 - 8, HX1, SEC_DOOR_Y - 65, SEC_DOOR_Y + 65, 0, 235, m_door)

# --- the lobby block (two storeys) -----------------------------------------------------------------------------------
# The shell: the front with the entrance doors standing open, and the block's north and south faces.
wall("Front", "y", LX0 - T, HY0 - T, HY1 + T, 0, F2 - SLAB, m_wall, [(0.0, FRONT_DOOR_HALF, 0.0, FRONT_DOOR_H)])
wall("Front", "y", LX0 - T, HY0 - T, HY1 + T, F2 - SLAB, TOP, m_wall, [(SCAN_Y, D_HALF + 15.0, F2, F2 + D_H)])
box("Block_North", LX0, HX0 - T, HY1, HY1 + T, 0, TOP, m_wall)
box("Block_South", LX0, HX0 - T, HY0 - T, HY0, 0, TOP, m_wall)
for s in (1, -1):   # the two door leaves, swung out
    box("Front_Door_Leaf", LX0 - T - 290, LX0 - T, s * FRONT_DOOR_HALF, s * (FRONT_DOOR_HALF + 8), 0,
        FRONT_DOOR_H - 5, m_door, folder="Set/Exits")
exit_sign("y", LX0, 0.0, 1, z=395.0, half=60.0)

# The lobby's second floor: a balcony all round the atrium, railed, on four pillars.
box("Balcony_West", LX0, VOID_X0, LY0, LY1, F2 - SLAB, F2, m_deck)
box("Balcony_East", VOID_X1, LX1 - T, LY0, LY1, F2 - SLAB, F2, m_deck)
box("Balcony_North", VOID_X0, VOID_X1, VOID_Y, LY1, F2 - SLAB, F2, m_deck)
box("Balcony_South", VOID_X0, VOID_X1, LY0, -VOID_Y, F2 - SLAB, F2, m_deck)
rail(VOID_X0, -VOID_Y, VOID_X0, VOID_Y)
rail(VOID_X1, -VOID_Y, VOID_X1, -200)
rail(VOID_X1, 200, VOID_X1, VOID_Y)
rail(VOID_X0, VOID_Y, VOID_X1, VOID_Y)
rail(VOID_X0, -VOID_Y, VOID_X1, -VOID_Y)
for px in (VOID_X0, VOID_X1):
    for py in (-VOID_Y, VOID_Y):
        box("Lobby_Pillar", px - 50, px + 50, py - 50, py + 50, 0, F2 - SLAB, m_trim)

# The grand stair: up the middle of the atrium to the east balcony, facing the door to the hall's gallery.
stairs("Grand_Stair", VOID_X0 + 100, 0.0, 0, VOID_X1 - (VOID_X0 + 100), F2, 400.0, 0.0, 30, m_trim)
box("Lobby_Desk", -2300, -2100, 430, 830, 0, 110, m_trim)   # W3 opening: moved onto the way in (was y 900..1300)
for (bx, by0, by1) in ((-3050.0, 800.0, 1200.0), (-1200.0, 200.0, 600.0)):
    box("Lobby_Bench", bx - 30, bx + 30, -by1, -by0, 0, 50, m_trim)
    box("Lobby_Bench", bx - 30, bx + 30, by0, by1, 0, 50, m_trim)
for (lx, ly) in ((-2950, -800), (-2950, 800), (-1260, -800), (-1260, 800), (-2100, 1100), (-2100, -1100)):
    lamp(lx, ly)

# Each side strip (s = +1 north, -1 south): a room west of STRIP_X on both floors, the fire stair east of it.
ST_X0, ST_X1 = STRIP_X + T, LX1 - T        # the stair shaft, inside
LAND_X, TURN_X = -1850.0, -1350.0          # the flights run between these
for s in (1, -1):
    def ys(a, b):
        return tuple(sorted((s * a, s * b)))
    inner = LY1 if s > 0 else LY0 - T       # the lobby's side wall (its low face)
    strip = ys(LY1 + T, HY1)                # the strip's inside, low to high
    wall2("Lobby_Side", "x", inner, LX0, LX1 - T, m_wall, ground=(-2650.0, -1975.0), upper=(-2650.0, -1975.0))
    wall2("Strip_Div", "y", STRIP_X, strip[0], strip[1], m_wall, upper=(s * 1700.0,))
    box("Floor_Strip_F2", LX0, STRIP_X, strip[0], strip[1], F2 - SLAB, F2, m_deck)
    # The fire stair: up the inner lane to a half landing, back down the outer lane to the second floor.
    box("Stair_Landing", LAND_X - 0, ST_X0, strip[0], strip[1], F2 - SLAB, F2, m_deck, folder="Set/Stairs")
    box("Stair_Half", TURN_X, ST_X1, strip[0], strip[1], 0, F2 / 2, m_trim, folder="Set/Stairs")
    core = ys(1690.0, 1710.0)
    box("Stair_Core", LAND_X, TURN_X, core[0], core[1], 0, F2 + 110, m_wall, folder="Set/Stairs")
    stairs("Fire_Stair", LAND_X, s * 1555.0, 0, TURN_X - LAND_X, F2 / 2, 270.0, 0.0, 15, m_trim)
    stairs("Fire_Stair", TURN_X, s * 1855.0, 180, TURN_X - LAND_X, F2 / 2, 290.0, F2 / 2, 15, m_trim)
    lane = ys(LY1 + T, 1690.0)
    rail(LAND_X, lane[0], LAND_X, lane[1])
    fire_door("x", s * HY1, -1965.0, -s, side=-1)
    exit_sign("x", s * LY1, -1975.0, -s)
    exit_sign("x", s * LY1, -1975.0, -s, z=F2 + 262.0)
    lamp(-2650.0, s * 1700.0)

# The rooms in the strips. North: cloakroom below, an office above. South: restrooms below, security above.
box("Cloak_Counter", -2900, -2400, 1500, 1560, 0, 100, m_trim)
for rx in (-3100.0, -2800.0, -2500.0):
    box("Cloak_Rack", rx, rx + 200, 1880, 1960, 0, 170, m_case)
for rx in (-2900.0, -2600.0, -2300.0):
    box("Stall", rx - 5, rx + 5, -1980, -1750, 0, 200, m_trim)
box("Office_Desk", -2900, -2500, 1800, 1900, F2, F2 + 75, m_trim)
box("Office_Cabinet", -3180, -3120, 1500, 1900, F2, F2 + 180, m_case)
box("Security_Desk", -3000, -2300, -1960, -1880, F2, F2 + 80, m_trim)
for mx in (-2900.0, -2650.0, -2400.0):
    box("Security_Monitor", mx - 60, mx + 60, -1975, -1965, F2 + 90, F2 + 160, m_glow)

# --- the north wing (two storeys) ------------------------------------------------------------------------------------
box("North_Outer", HX0 - T, HX1 + T, NY1, NY1 + T, 0, TOP, m_wall)
box("North_West", HX0 - T, HX0, HY1 + T, NY1, 0, TOP, m_wall)
box("North_East", HX1, HX1 + T, HY1 + T, NY1, 0, TOP, m_wall)
box("Floor_North_F2", HX0, HX1, HY1 + T, NY1, F2 - SLAB, F2, m_deck)
wall("North_Div", "y", 2300.0, HY1 + T, NY1, 0, F2 - SLAB, m_wall, [(2450.0, D_HALF, 0.0, D_H)])
for dx in (800.0, 2600.0):
    wall("North_Div_F2", "y", dx, HY1 + T, NY1, F2, TOP, m_wall, [(2450.0, D_HALF, F2, F2 + D_H)])
# Below: the preview gallery (lots on plinths) and the stock room, with a fire exit at the east end.
for lx in (-600.0, 0.0, 600.0, 1200.0, 1800.0):
    box("Preview_Plinth", lx - 45, lx + 45, 2700, 2790, 0, 100, m_trim)
    box("Preview_Lot", lx - 28, lx + 28, 2717, 2773, 100, 170, m_case)
box("Preview_Bench", -200, 1400, 2380, 2440, 0, 50, m_trim)
for cx in (2600.0, 3000.0, 3400.0):
    box("Stock_Crate", cx, cx + 200, 2650, 2850, 0, 160, m_trim)
fire_door("y", HX1, 2450.0, -1)
for lx in (-400.0, 800.0, 1800.0, 2900.0, 3900.0):
    lamp(lx, 2450.0)
# Above: the director's office, the projection room, the archive.
box("Director_Desk", -400, 200, 2650, 2760, F2, F2 + 78, m_trim)
box("Projector", 1600, 1800, 2060, 2160, F2 + 90, F2 + 150, m_case)
box("Projector_Stand", 1660, 1740, 2080, 2140, F2, F2 + 90, m_trim)
for cx in (2900.0, 3300.0, 3700.0, 4100.0):
    box("Archive_Shelf", cx, cx + 60, 2300, 2850, F2, F2 + 220, m_case)

# --- the service wing: the backstage corridor behind the hatches, and three rooms off it ------------------------------
wall("Back_South", "x", BY0 - T, HX0 - T, HX1 + T, 0, BACK_H, m_wall,
     [(-100.0, D_HALF, 0.0, D_H), (1700.0, D_HALF, 0.0, D_H), (3000.0, D_HALF, 0.0, D_H)])
box("Back_West", HX0 - T, HX0, SY0, HY0 - T, 0, BACK_H, m_wall)
wall("Back_East", "y", HX1, BY0, HY0 - T, 0, BACK_H, m_wall, [(-2300.0, D_HALF, 0.0, D_H)])
box("Service_East", HX1, HX1 + T, SY0, BY0 - T, 0, BACK_H, m_wall)
box("Service_South", HX0 - T, HX1 + T, SY0 - T, SY0, 0, BACK_H, m_wall)
for dx in (800.0, 2600.0):
    box("Service_Div", dx, dx + T, SY0, BY0 - T, 0, BACK_H, m_wall)
for cx in (0.0, 1500.0, 2600.0):
    box("Back_Crate", cx, cx + 120, BY0 + 20, BY0 + 140, 0, 120, m_trim)
for lx in (-600.0, 400.0, 1400.0, 2400.0, 3400.0, 4200.0):   # the hall wall shades the corridor
    lamp(lx, -2330.0, z=400.0, rgb=WORK)
box("Kitchen_Counter", -900, 600, SY0 + 20, SY0 + 100, 0, 95, m_trim)
box("Kitchen_Island", -500, 300, -3000, -2900, 0, 95, m_trim)
for cx in (1000.0, 1400.0, 2100.0):
    box("Prop_Crate", cx, cx + 250, SY0 + 40, SY0 + 290, 0, 180, m_trim)
box("Prop_Shelf", 2500, 2580, -3200, -2700, 0, 220, m_case)
box("Dock_Shutter", 3300, 4000, SY0, SY0 + 10, 0, 350, m_door)
for cx in (2750.0, 4150.0):
    box("Dock_Crate", cx, cx + 180, -3150, -2970, 0, 150, m_trim)
fire_door("y", HX1, -2950.0, -1)

# --- the east wing: the vault corridor and the vault, behind the dais wall ----------------------------------------------
box("East_South", HX1 + T, EX1 + T, BY0 - T, BY0, 0, BACK_H, m_wall)
box("East_Outer", EX1, EX1 + T, BY0, VAULT_Y1 + T, 0, BACK_H, m_wall)
box("East_North", HX1 + T, EX1, VAULT_Y1, VAULT_Y1 + T, 0, BACK_H, m_wall)
wall("Vault_Wall", "x", VAULT_Y0, HX1 + T, EX1, 0, BACK_H, m_wall, [(4860.0, 110.0, 0.0, 260.0)])
box("Vault_Door", 4975, 5005, VAULT_Y0 - 220, VAULT_Y0, 0, 260, m_case)
for sy in (-700.0, -300.0, 100.0, 500.0):
    box("Vault_Shelf", EX1 - 80, EX1, sy, sy + 250, 0, 200, m_case)
    box("Vault_Lot", EX1 - 65, EX1 - 15, sy + 90, sy + 160, 200, 250, m_gold)
box("Vault_Table", 4600, 4760, -100, 100, 0, 90, m_trim)
fire_door("y", EX1, -1800.0, -1)

# =====================================================================================================================
# Dressing: placeholder props and people, room by room. No rules anywhere in it.
# =====================================================================================================================
FRONT = LX0 - T                    # the facade's outer face

# --- the street front: three lanes to the doors, the terrace over them, the queue, the press, signs, cars --------------
def sign(text, x, y, z, yaw, size=32.0, rgb=(1.0, 0.85, 0.45), folder="Set/Signs"):
    """Words in the world. The text faces the way yaw points (stand on that side to read it), on a dark board."""
    N[0] += 1
    a = spawn(unreal.TextRenderActor, (x, y, z), yaw=yaw, label="Sign_%03d" % N[0], folder=folder)
    c = a.get_component_by_class(unreal.TextRenderComponent)
    try:
        c.set_text(text)
    except Exception:
        c.set_editor_property("text", text)
    for (k, v) in (("world_size", size), ("text_render_color", tint(rgb)),
                   ("horizontal_alignment", unreal.HorizTextAligment.EHTA_CENTER),
                   ("vertical_alignment", unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)):
        try:
            c.set_editor_property(k, v)
        except Exception as e:
            unreal.log_warning("sign: %s: %s" % (k, e))
    r = math.radians(yaw)
    board = shape(CUBE, "Sign_Board", x - 4.0 * math.cos(r), y - 4.0 * math.sin(r), z, 4.0,
                  len(text) * size * 0.62 + 30.0, size * 1.7, m_floor, yaw=yaw, folder=folder)
    ghost(board).static_mesh_component.set_editor_property("cast_shadow", False)
    return a


signal(flat("Carpet", PAVE_X0 + 60.0, VOID_X0 + 80, -CARPET_HALF, CARPET_HALF, m_carpet), m_sig_red, 1)  # it is tape
# The scanner arch at the head of the queue lane (the watcher itself is placed with the rules).
for s in (1, -1):
    prop("Scanner_Post", SCAN_X - 10, SCAN_X + 10, SCAN_Y + s * SCAN_HALF, SCAN_Y + s * (SCAN_HALF + 20), 0, 240, m_case,
         folder="Set/Street")
deco("Scanner_Lintel", SCAN_X - 10, SCAN_X + 10, SCAN_Y - SCAN_HALF - 20, SCAN_Y + SCAN_HALF + 20, 240, 262, m_case,
     folder="Set/Street")
# The terrace over the doors, with a rail: the arrivals searchlight stands on it, and a door leads in to the balcony.
box("Terrace", TERRACE_X0, FRONT, -600, 600, F2 - SLAB, F2, m_trim, folder="Set/Street")
deco("Terrace_Fascia", TERRACE_X0 - 10, TERRACE_X0, -600, 600, 395, 445, m_gold, folder="Set/Street")
rail(TERRACE_X0, -600, TERRACE_X0, 600)
rail(TERRACE_X0, -600, FRONT, -600)
rail(TERRACE_X0, 600, FRONT, 600)
for s in (1, -1):
    prop("Terrace_Post", TERRACE_X0 + 4, TERRACE_X0 + 16, s * 578, s * 590, 0, F2 - SLAB, m_trim, folder="Set/Street")
deco("Sign", FRONT - 6, FRONT, -700, 700, 730, 850, m_gold, folder="Set/Street")
for c in (900.0, 1300.0, 1700.0):
    for s in (1, -1):
        for (z0, z1) in ((90.0, 300.0), (F2 + 70.0, F2 + 290.0)):
            deco("Window", FRONT - 4, FRONT, s * c - 120, s * c + 120, z0, z1, m_window, folder="Set/Street")
        prop("Planter", FRONT - 70, FRONT - 10, s * c - 110, s * c + 110, 0, 50, m_trim, folder="Set/Street")
        deco("Hedge", FRONT - 66, FRONT - 14, s * c - 104, s * c + 104, 50, 95, m_plant, folder="Set/Street")
# --- QA front (note 13: "the queue rope isn't realistic and floaty") ------------------------------------------------
# The street's and lobby's velvet ropes hang in a sagging curve (a short chain along a catenary) from hooks just under
# the posts' tops (94 cm: import_props stretches the Rodin post to that height). Where the straight rope stood, an
# unseen pawn-only box still stops the walker; cameras and sight lines pass. tools/sets/front.py does the same to a
# level built before this.
ROPE_HOOK, ROPE_DIA = 88.0, 3.6


def velvet_span(a, b, z=0.0, folder="Set/Props"):
    (ax, ay), (bx, by) = a, b
    L = math.hypot(bx - ax, by - ay)
    ux, uy = (bx - ax) / L, (by - ay) / L
    hx0, hy0, hx1, hy1 = ax + ux * 4.0, ay + uy * 4.0, bx - ux * 4.0, by - uy * 4.0   # the hooks, on the posts' sides
    sag, n = min(28.0, 0.06 * L + 4.0), max(6, min(24, int(L / 16.0)))
    pts = [(hx0 + (hx1 - hx0) * i / n, hy0 + (hy1 - hy0) * i / n, z + ROPE_HOOK - 4.0 * sag * (i / n) * (1 - i / n))
           for i in range(n + 1)]
    for (p, q) in zip(pts, pts[1:]):
        dx, dy, dz = q[0] - p[0], q[1] - p[1], q[2] - p[2]
        ln = math.sqrt(dx * dx + dy * dy + dz * dz)
        ghost(shape(CYL, "Rope_Sag", (p[0] + q[0]) / 2, (p[1] + q[1]) / 2, (p[2] + q[2]) / 2, ROPE_DIA, ROPE_DIA,
                    ln + ROPE_DIA * 0.6, m_rope, yaw=math.degrees(math.atan2(dy, dx)),
                    pitch=math.degrees(math.atan2(dz, math.hypot(dx, dy))) - 90.0, folder=folder))
    blk = soft(box("Rope_Block", min(ax, bx) - 2, max(ax, bx) + 2, min(ay, by) - 2, max(ay, by) + 2, z + 76, z + 82,
                   m_rope, folder="Set/Blockers"))
    blk.set_actor_hidden_in_game(True)
    blk.static_mesh_component.set_visibility(False, False)


def velvet_rope(pts, z=0.0):
    """rope_line's posts (import_props replaces them with the Rodin stanchion) with sagging velvet between them."""
    for (x, y) in pts:
        soft(shape(CYL, "Stanchion", x, y, z + 47, 7.0, 7.0, 94.0, m_gold))
        ghost(shape(CYL, "Stanchion_Foot", x, y, z + 1.5, 28.0, 28.0, 3.0, m_gold))
    for a, b in zip(pts, pts[1:]):
        velvet_span(a, b, z)


# Ropes: both sides of the carpet from the kerb to the doors (and a little way inside), the queue lane's north side,
# a rope across the carpet at the doors (the doorman lifts it for an invitation), a rope across the crew lane.
CARPET_X = (PAVE_X0 + 150.0, -3760.0, -3560.0, -3330.0, FRONT - 6.0)   # the carpet's foot is open: 1.5 m of it
for ry in (LANE_ROPE, -LANE_ROPE):
    velvet_rope([(rx, ry) for rx in CARPET_X])                         # QA front: velvet_rope was rope_line (4 calls)
    velvet_rope([(LX0 + 12.0, ry), (LX0 + 250.0, ry)])
velvet_rope([(-3900.0, LANE_Y1 + 10.0), (-3710.0, LANE_Y1 + 10.0), (SCAN_X, LANE_Y1 + 10.0)])
velvet_span((FRONT - 6.0, -LANE_ROPE), (FRONT - 6.0, LANE_ROPE), folder="Set/Street")   # QA front: was a straight bar
blocker("Carpet_Rope_Block", FRONT - 26, FRONT - 14, -LANE_ROPE, LANE_ROPE, 0, 260)
velvet_rope([(SCAN_X, -LANE_ROPE - 12.0), (SCAN_X, -FRONT_DOOR_HALF + 10.0)])   # QA front: was a bar with no posts
blocker("Crew_Lane_Block", SCAN_X - 6, SCAN_X + 6, -FRONT_DOOR_HALF, -LANE_ROPE, 0, 260)
# --- end QA front
# --- QA scene1 (notes 1, 2, 10: the line moves; AQueueLine runs it, the Scene 1 section below places it) ----------
# The queue for the scanner, front first: the warm light at the scanner, back down the lane, then along the kerb.
# Julian takes QUEUE_JULIAN, three guests ahead of him and five behind. The guests are ghosts (no collision): they
# walk, and AQueueLine gives each one a blocker that walks with it. Eight guests and six rnd draws, as before, so
# nothing placed after this block moves.
QUEUE_PLACES = [MARK_SCANNER, (SCAN_X - 355.0, 315.0), (SCAN_X - 440.0, 325.0), START] + \
    [(PAVE_X0 + 85.0, qy) for qy in (450.0, 535.0, 620.0, 705.0, 790.0)]
QUEUE_JULIAN = 3
for qi, (qx, qy) in enumerate(QUEUE_PLACES):
    if qi == QUEUE_JULIAN:
        continue
    ax, ay = QUEUE_PLACES[qi - 1] if qi > 0 else (qx + 100.0, qy)   # each faces the place ahead of it
    ghost(guest(qx, qy, facing=math.degrees(math.atan2(ay - qy, ax - qx))))
for _ in range(6):
    rnd.uniform(-10, 10)
# --- end QA scene1
# The press, behind the carpet's south rope.
for px in (-3950.0, -3800.0, -3650.0):
    guest(px, -300.0, facing=90.0 + rnd.uniform(-20, 20))
    tripod(px + 45, -245.0)
# Lane signs.
sign("QUEUE", -3905.0, LANE_Y1 + 16.0, 150.0, 135.0)
sign("INVITED GUESTS", PAVE_X0 + 145.0, LANE_ROPE + 8.0, 150.0, 135.0)
sign("INVITED GUESTS", PAVE_X0 + 40.0, 0.0, 262.0, 180.0, size=26.0)
sign("PRESS", -3800.0, -LANE_ROPE - 8.0, 130.0, 90.0, size=26.0)
sign("CREW ONLY", SCAN_X - 8.0, -(LANE_ROPE + FRONT_DOOR_HALF) / 2, 150.0, 180.0, size=26.0)
for ly in (-1900.0, -700.0, 700.0, 1900.0):
    street_lamp(PAVE_X0 + 40, ly)
for by in range(-2100, 2101, 350):
    if abs(by) > 300:
        prop("Bollard", PAVE_X0 + 8, PAVE_X0 + 22, by - 7, by + 7, 0, 80, m_trim, folder="Set/Street")
car(PAVE_X0 - 130, 0.0, 760.0, CAR_MATS[3])            # the limousine at the foot of the carpet
car(PAVE_X0 - 130, -1500.0, 440.0, CAR_MATS[0])
car(PAVE_X0 - 130, -850.0, 440.0, CAR_MATS[2])
car(PAVE_X0 - 130, 1000.0, 440.0, CAR_MATS[1])
car(PAVE_X0 - 130, 1700.0, 440.0, CAR_MATS[2])
car(ROAD_X0 + 200, -400.0, 440.0, CAR_MATS[1])
car(ROAD_X0 + 200, 600.0, 440.0, CAR_MATS[0])
# The other three sides: trees, staff cars, a lorry at the dock shutter, bins by the kitchen.
for tx in (-2600.0, -1100.0, 400.0, 1900.0, 3400.0, 4900.0):
    tree(tx, NY1 + 600)
for tx in (-2600.0, -1100.0, 400.0, 1900.0):
    tree(tx, SY0 - 700)
car(EX1 + 450, -700.0, 440.0, CAR_MATS[2])
car(EX1 + 450, 100.0, 440.0, CAR_MATS[0])
car(EX1 + 800, -300.0, 440.0, CAR_MATS[1])
box("Lorry_Trailer", 3400, 3900, SY0 - 660, SY0 - 45, 70, 360, m_trim, folder="Set/Street")
box("Lorry_Cab", 3440, 3860, SY0 - 940, SY0 - 680, 40, 290, CAR_MATS[1], folder="Set/Street")
for wx in (3420.0, 3880.0):
    for wy in (SY0 - 810, SY0 - 550, SY0 - 200):
        deco("Wheel", wx - 12, wx + 12, wy - 45, wy + 45, -21, 69, m_tire, folder="Set/Street")
for dx in (-700.0, -400.0):
    box("Bin", dx, dx + 240, SY0 - 160, SY0 - 45, -21, 110, m_door, folder="Set/Street")

# --- the lobby --------------------------------------------------------------------------------------------------------
deco("Desk_Screen", -2180, -2170, 590, 670, 110, 150, m_glow)   # W3 opening: with the desk
guest(-2050.0, 1100.0, facing=180.0)                    # the receptionist
guest(-2370.0, 1050.0, facing=0.0)
for (px, py) in ((-3140, 1330), (-3140, -1330), (-1090, 1330), (-1090, -1330), (-1090, 430), (-1090, -430)):
    plant(px, py)
    if abs(py) > 1000:
        plant(px, py, F2)                               # and one above it on the balcony
for cx in (-2950.0, -2320.0, -1500.0):                  # pictures on both side walls, both floors
    for z in (150.0, F2 + 130.0):
        painting("x", LY1, cx, -1, z=z)
        painting("x", LY0, cx, 1, z=z)
mingle(-2900.0, 950.0, 2)
mingle(-1300.0, -1000.0, 3)
mingle(-2950.0, -1000.0, 2)
# The balcony: sofas along the front, guests at the rail looking down into the atrium.
for s in (1, -1):
    sofa(-3180, -3100, s * 600 - 150, s * 600 + 150, F2, "W")
table(-3020.0, 0.0, 60, 120, h=40, z=F2)
guest(-2300.0, 1340.0, z=F2, facing=270.0)
guest(-2180.0, 1345.0, z=F2, facing=255.0)
guest(-1900.0, -865.0, z=F2, facing=90.0)

# --- the side strips: cloakroom and restrooms below, the office and the security room above ----------------------------
guest(-2650.0, 1640.0, facing=270.0)                    # the cloakroom attendant
for rx in (-3100.0, -2800.0, -2500.0):
    for i in range(3):
        deco("Coat", rx + 20 + i * 60, rx + 55 + i * 60, 1860, 1880, 70, 165, GUEST_MATS[(i + int(rx)) % 5])
prop("Cloak_Bench", -3180, -3140, 1500, 1800, 0, 45, m_trim)
prop("Sink_Counter", -3180, -3120, -1900, -1500, 0, 85, m_head)
deco("Mirror", -3199, -3195, -1880, -1520, 110, 190, m_window)
for tx in (-3050.0, -2750.0, -2450.0, -2200.0):
    prop("Toilet", tx - 20, tx + 20, -1975, -1920, 0, 40, m_head)
chair(-2700.0, 1950.0, 270, z=F2)
guest(-2700.0, 1950.0, z=F2 + 45, facing=270.0, seated=True)
chair(-2800.0, 1730.0, 90, z=F2)
chair(-2600.0, 1730.0, 90, z=F2)
deco("Office_Screen", -2740, -2660, 1840, 1848, F2 + 75, F2 + 115, m_glow)
plant(-2160.0, 1940.0, F2)
painting("x", HY1, -2300.0, -1, z=F2 + 130.0)
chair(-2850.0, -1830.0, 270, z=F2)
guest(-2850.0, -1830.0, z=F2 + 45, facing=270.0, seated=True, mat=GUEST_MATS[1])
chair(-2500.0, -1830.0, 270, z=F2)
box("Lockers", -3180, -3130, -1800, -1500, F2, F2 + 190, m_case, folder="Set/Props")
box("Server_Rack", -2190, -2130, -1960, -1860, F2, F2 + 200, m_floor, folder="Set/Props")
deco("Server_Lights", -2192, -2190, -1950, -1870, F2 + 60, F2 + 160, m_glow)

# --- the great hall: the audience, the bar, the phone bids, the dais --------------------------------------------------
for bx in (-400.0, -200.0, 0.0, 200.0, 400.0, 600.0):  # bidders on the benches, facing the dais
    for s in (1, -1):
        for slot in rnd.sample(range(6), 3):
            gy = s * (320.0 + slot * 115.0)
            guest(bx, gy, z=60.0, facing=0.0, seated=True)
            if rnd.random() < 0.25:     # --- QA hall --- (the draw stays: the rest of the layout's dice depend on it)
                N[0] += 1               # no paddle hanging in the air: tools/sets/hall.py lays paddles on empty seats
                                        # (the label counter steps as before, so later labels keep their numbers)
for (mx, my, n) in ((-750, -1300, 3), (-750, 1250, 2), (150, 1560, 3), (1100, 1150, 2), (1900, 1100, 3),
                    (150, -1300, 2), (-800, -560, 2), (-800, 560, 2)):
    mingle(float(mx), float(my), n)
for i in range(8):                                      # the champagne bar
    ghost(shape(CYL, "Bottle", 860.0 + i * 95, -1525.0 + (i % 2) * 22, 124.0, 9.0, 9.0, 28.0,
                (m_plant, m_gold, m_rope)[i % 3]))
prop("Bar_Shelf", 900, 1500, -1995, -1955, 0, 200, m_case)
deco("Bar_Bottles", 920, 1480, -1955, -1947, 120, 146, m_gold)
deco("Bar_Bottles", 920, 1480, -1955, -1947, 165, 191, m_plant)
guest(1200.0, -1700.0, facing=90.0, mat=GUEST_MATS[1])  # the bartender
for i, sx in enumerate((900.0, 1050.0, 1200.0, 1350.0, 1500.0)):
    stool(sx, -1420.0)
    if i in (0, 3):
        guest(sx, -1420.0, z=70.0, facing=270.0, seated=True)
guest(1130.0, -1330.0, facing=250.0)
for px in (1880.0, 2000.0, 2120.0):                     # the phone bids
    deco("Phone", px - 12, px + 12, -1535, -1515, 100, 108, m_glow)
    chair(px, -1640.0, 90)
    guest(px, -1640.0, z=45.0, facing=90.0, seated=True)
for cx in (-650.0, 1000.0, 2900.0, 3800.0):             # pictures under the walkway, on the south and west walls
    painting("x", HY1, cx, -1, z=160.0, w=240.0, h=150.0)
for cx in (-750.0, 1900.0, 2700.0, 4100.0):
    painting("x", HY0, cx, 1, z=160.0, w=240.0, h=150.0)
for cy in (-1500.0, -900.0, 900.0, 1500.0):
    painting("y", HX0, cy, 1, z=160.0, w=200.0, h=130.0)
for cx in (-500.0, 400.0, 1300.0, 2200.0, 3100.0, 4000.0):   # a banner on every column, facing the aisle
    for s in (1, -1):
        deco("Column_Banner", cx - 45, cx + 45, s * 1232, s * 1238, 600, 1150, m_carpet)
        deco("Column_Banner_Hem", cx - 45, cx + 45, s * 1231, s * 1239, 588, 600, m_gold)
plant(4330.0, -1100.0, DAIS_H)
for s in (1, -1):
    easel(4230.0, s * 650.0, DAIS_H)                    # W3 stage: off the (narrower) screen (was 480)
# Scene 3's dressing: lots on the table, the closed trap, the band on its riser, signs at the two doors.
for i, m in enumerate((m_gold, m_case, m_head)):
    deco("Lot", LOTS[0] + 30 + i * 45, LOTS[0] + 60 + i * 45, LOTS[2] + 40 + i * 30, LOTS[2] + 80 + i * 30,
         DAIS_H + 95, DAIS_H + 125 + i * 12, m)
flat("Stage_Trap", TRAP[0] - 75, TRAP[0] + 75, TRAP[1] - 75, TRAP[1] + 75, m_floor, z=DAIS_H)
sign("STAGE TRAP", HX1 - 14.0, TRAP[1], DAIS_H + 150.0, 180.0, size=20.0)
for i, (bx, by) in enumerate(((3700.0, 1400.0), (3780.0, 1520.0), (3860.0, 1400.0), (3700.0, 1540.0), (3880.0, 1540.0))):
    ghost(shape(CYL, "Band", bx, by, 30.0 + 75.0, 26.0, 48.0, 150.0, GUEST_MATS[1], yaw=-90.0, folder="Set/Guests"))
    ghost(shape(SPHERE, "Band_Head", bx, by, 30.0 + 162.0, 24.0, 24.0, 26.0, m_head, folder="Set/Guests"))
    deco("Instrument", bx - 10, bx + 10, by - 34, by - 22, 30 + 70, 30 + 130 + i * 8, m_gold)
sign("THE AURELIA QUINTET", (BAND[0] + BAND[1]) / 2, BAND[2] - 4.0, 60.0, -90.0, size=16.0)
for py in (PA[2] - 3.0,):
    deco("PA_Grille", PA[0] + 15, PA[1] - 15, py, py + 3, 30, 235, m_trim)
sign("EXIT", HX1 - 14.0, SIDE_DOOR_Y, 262.0, 180.0, size=26.0, rgb=(1.0, 0.72, 0.15))
sign("SECURITY", HX1 - 14.0, SEC_DOOR_Y, 262.0, 180.0, size=22.0, rgb=(1.0, 0.25, 0.2))
sign("CURTAIN WINCH", WINCH[0] - 30.0, BY0 + 6.0, 215.0, 90.0, size=20.0)
# --- QA hall --- no tripod here: tools/sets/hall.py stands a TV studio camera on its pedestal on the riser (note 16)
N[0] += 2                                               # the tripod's two labels, so later labels keep their numbers
ghost(shape(CYL, "Guest", (RISER[0] + RISER[1]) / 2 - 20.0, (RISER[2] + RISER[3]) / 2 + 40.0, 80.0 + 75.0, 26.0, 48.0, 150.0, GUEST_MATS[1],
            folder="Set/Guests"))
ghost(shape(SPHERE, "Guest_Head", (RISER[0] + RISER[1]) / 2 - 20.0, (RISER[2] + RISER[3]) / 2 + 40.0, 80.0 + 162.0, 24.0, 24.0, 26.0, m_head,
            folder="Set/Guests"))
sign("LIVE BROADCAST", RISER[0] - 6.0, (RISER[2] + RISER[3]) / 2, 45.0, 180.0, size=24.0, rgb=(1.0, 0.25, 0.2))
sign("LOT 41: THE DIRECTOR'S CUT", 4125.0, 0.0, DAIS_H + 165.0, 180.0, size=18.0)
sign("CREW ONLY", 3000.0, BY0 + 6.0, 215.0, 90.0, size=22.0)
for (kx, ky, ks) in ((3760.0, -3200.0, 70.0), (3840.0, -3190.0, 50.0), (3560.0, -3210.0, 60.0)):
    prop("Flight_Case", kx - ks / 2, kx + ks / 2, ky - 30, ky + 30, 0, ks, m_floor)
# W3 stage: no west gallery any more (its benches, onlookers, tripod and plants are gone); flight cases on the
# catwalk walkway, and the reception where the old benches stood: cocktail tables and guests standing round them
# (their own dice, so the rest of the layout's draws stay as they were).
for (x0, x1, h) in ((1000, 1110, 60), (2200, 2330, 85), (3500, 3620, 60)):
    prop("Flight_Case", x0, x1, 1950, 1995, CAT_Z, CAT_Z + h, m_floor)
_rnd_keep = rnd
rnd = random.Random(1115)
for (mx, my, n) in ((950, 650, 3), (1000, -650, 3), (1350, -380, 2), (1300, 420, 2), (1700, -650, 2), (600, -950, 2),
                    (550, 900, 2)):
    mingle(float(mx), float(my), n)
for s in (1, -1):                                       # standing room: bidders along the reserved rows' ends
    for i, x in enumerate((2500.0, 2590.0, 2675.0, 2770.0, 2850.0, 2945.0, 3030.0, 3110.0)):
        guest(x, s * (700.0 + (i % 2) * 30.0), facing=rnd.uniform(-12.0, 12.0))
rnd = _rnd_keep

# --- the north wing ---------------------------------------------------------------------------------------------------
for cx in (-900.0, -300.0, 300.0, 900.0, 1500.0, 2100.0):   # the preview gallery
    painting("x", NY1, cx, -1, z=170.0, w=180.0, h=120.0)
for cx in (-600.0, 700.0, 1300.0):
    painting("x", HY1 + T, cx, 1, z=160.0)
rope_line([(-800.0, 2640.0), (-300.0, 2640.0), (300.0, 2640.0), (900.0, 2640.0), (1500.0, 2640.0), (2000.0, 2640.0)])
guest(0.0, 2570.0, facing=90.0)
guest(640.0, 2560.0, facing=80.0)
guest(1150.0, 2550.0, facing=100.0)
guest(1700.0, 2250.0, facing=40.0)
guest(1770.0, 2320.0, facing=220.0)
plant(-940.0, 2840.0)
plant(2240.0, 2840.0)
box("Stock_Shelf", 2500, 3900, 2025, 2085, 0, 220, m_case, folder="Set/Props")   # the stock room
for (cx, cy, sz) in ((2700, 2750, 90), (3100, 2750, 70), (3480, 2760, 110)):
    box("Stock_Crate_Top", cx - sz / 2, cx + sz / 2, cy - sz / 2, cy + sz / 2, 160, 160 + sz, m_case,
        folder="Set/Props")
crates(3950.0, 2740.0, (120, 90))
crates(4200.0, 2200.0, (100,))
for i in range(3):
    prop("Wrapped_Canvas", 2325 + i * 9, 2331 + i * 9, 2580 + i * 40, 2800 + i * 30, 0, 150 - i * 15, m_head)
table(3650.0, 2330.0, 110, 60, h=30)
flat("Rug", -500, 300, 2450, 2850, m_carpet, z=F2)       # the director's office
chair(-100.0, 2815.0, 270, z=F2, mat=m_sofa)
chair(-250.0, 2570.0, 90, z=F2)
chair(50.0, 2570.0, 90, z=F2)
sofa(-970, -890, 2200, 2600, F2, "W")
table(-780.0, 2400.0, 60, 120, h=40, z=F2)
box("Bookshelf", 350, 750, 2850, 2895, F2, F2 + 220, m_case, folder="Set/Props")
for cy in (2300.0, 2650.0):
    painting("y", HX0, cy, 1, z=F2 + 130.0, w=120.0, h=170.0)   # film posters
ghost(shape(SPHERE, "Award", 150.0, 2700.0, F2 + 78 + 13, 22.0, 22.0, 26.0, m_gold))
deco("Desk_Screen", -180, -100, 2700, 2708, F2 + 78, F2 + 118, m_glow)
plant(-940.0, 2080.0, F2)
plant(750.0, 2090.0, F2)
for (cx, cy, n) in ((1250.0, 2120.0, 4), (1310.0, 2190.0, 3), (2050.0, 2110.0, 5)):   # the projection room
    for i in range(n):
        ghost(shape(CYL, "Film_Can", cx, cy, F2 + 5 + i * 10, 42.0, 42.0, 9.0, m_gold if i == n - 1 else m_trim))
box("Reel_Shelf", 1000, 2400, 2850, 2895, F2, F2 + 200, m_case, folder="Set/Props")
for i in range(7):
    ghost(shape(CYL, "Reel", 1100.0 + i * 200, 2846.0, F2 + 120, 60.0, 60.0, 6.0, m_trim, yaw=90.0, pitch=90.0))
table(2250.0, 2350.0, 180, 80, z=F2)
deco("Edit_Screen", 2200, 2300, 2378, 2386, F2 + 75, F2 + 125, m_glow)
chair(2250.0, 2270.0, 90, z=F2)
guest(1700.0, 2270.0, z=F2, facing=270.0)               # the projectionist
table(2760.0, 2150.0, 120, 70, z=F2)                    # the archive
for i in range(3):
    deco("Archive_Box", 2715 + i * 34, 2745 + i * 34, 2130, 2170, F2 + 75, F2 + 100, m_head)
box("Filing_Cabinets", 4340, 4395, 2100, 2800, F2, F2 + 130, m_trim, folder="Set/Props")
guest(3500.0, 2180.0, z=F2, facing=0.0)

# --- the service wing -------------------------------------------------------------------------------------------------
prop("Costume_Rail", 300, 900, -2572, -2566, 165, 170, m_trim)      # backstage
for rx in (300.0, 900.0):
    prop("Costume_Rail_Post", rx - 4, rx + 4, -2573, -2565, 0, 165, m_trim)
for i in range(8):
    deco("Costume", 325 + i * 70, 370 + i * 70, -2580, -2558, 55, 163, (GUEST_MATS + ART_MATS)[i % 9])
table(-650.0, -2060.0, 400, 60)
deco("Makeup_Mirror", -820, -480, HY0 - T - 4, HY0 - T, 90, 170, m_window)
deco("Makeup_Lights", -830, -470, HY0 - T - 5, HY0 - T, 172, 180, m_gold)
chair(-650.0, -2140.0, 90)
prop("Flight_Case", 1900, 2050, -2580, -2500, 0, 70, m_floor)
prop("Flight_Case", 2100, 2200, -2580, -2500, 0, 110, m_floor)
soft(shape(CYL, "Cable_Drum", 2350.0, -2540.0, 25.0, 80.0, 80.0, 50.0, m_trim))
table(1150.0, -2060.0, 300, 60, h=80)
ghost(shape(SPHERE, "Prop_Orb", 1080.0, -2060.0, 93.0, 24.0, 24.0, 24.0, m_gold))
deco("Prop_Box", 1180, 1230, -2080, -2040, 80, 105, m_rope)
guest(1150.0, -2170.0, facing=90.0)
guest(2300.0, -2400.0, facing=180.0)
box("Fridge", 620, 780, SY0 + 20, SY0 + 120, 0, 200, m_head, folder="Set/Props")   # the kitchen
deco("Range_Hood", -600, 0, SY0 + 10, SY0 + 90, 190, 230, m_trim)
for (px, m) in ((-500.0, m_trim), (-300.0, m_floor), (-100.0, m_trim)):
    ghost(shape(CYL, "Pot", px, SY0 + 60.0, 95.0 + 11, 30.0, 30.0, 22.0, m))
for tx in (-350.0, 100.0):
    deco("Tray", tx - 30, tx + 30, -2970, -2930, 95, 97, m_trim)
    for i in range(3):
        ghost(shape(CYL, "Glass", tx - 18 + i * 18, -2950.0, 104.0, 6.0, 6.0, 14.0, m_gold))
ghost(shape(CYL, "Plates", -150.0, -2950.0, 101.0, 24.0, 24.0, 12.0, m_head))
guest(-300.0, -3130.0, facing=270.0, mat=GUEST_MATS[3])
guest(100.0, -2840.0, facing=270.0, mat=GUEST_MATS[3])
table(-380.0, -2730.0, 90, 60, h=80)
box("Kitchen_Shelf", -990, -940, -3200, -2800, 0, 200, m_case, folder="Set/Props")
for i, m in enumerate((m_trim, m_rope, m_case)):       # the prop store: scenery flats, a bust, a column, lamps
    box("Scenery_Flat", 830 + i * 14, 842 + i * 14, -3250 + i * 50, -2900 + i * 50, 0, 300 - i * 30, m,
        folder="Set/Props")
box("Chair_Stack", 1230, 1275, -2770, -2725, 0, 140, m_trim, folder="Set/Props")
prop("Lamp_Shelf", 1950, 2400, -2700, -2640, 0, 60, m_case)
for lx in (2000.0, 2150.0, 2300.0):
    deco("Spare_Lamp", lx - 20, lx + 20, -2690, -2650, 60, 110, m_floor)
box("Bust_Plinth", 1845, 1895, -3175, -3125, 0, 100, m_trim, folder="Set/Props")
ghost(shape(SPHERE, "Bust", 1870.0, -3150.0, 124.0, 44.0, 44.0, 50.0, m_head))
shape(CYL, "Fake_Column", 1750.0, -3200.0, 125.0, 60.0, 60.0, 250.0, m_head)
for (cx, cy) in ((3450.0, -3050.0), (3700.0, -2850.0)):  # the loading dock: pallets, drums, a hazard line
    box("Pallet", cx - 60, cx + 60, cy - 60, cy + 60, 0, 12, m_trunk, folder="Set/Props")
    crates(cx, cy, (100, 80), z=12.0)
box("Hand_Truck", 3180, 3230, SY0 + 30, SY0 + 40, 0, 120, m_trim, folder="Set/Props")
for (dx, dy) in ((4250.0, -2700.0), (4320.0, -2730.0), (4270.0, -2780.0)):
    shape(CYL, "Drum", dx, dy, 44.0, 58.0, 58.0, 88.0, CAR_MATS[1])
flat("Hazard_Line", 3300, 4000, SY0 + 60, SY0 + 80, m_gold)
guest(3550.0, -2750.0, facing=250.0)
guest(3950.0, -3100.0, facing=180.0)

# --- the east wing ----------------------------------------------------------------------------------------------------
box("Vault_Desk", 4520, 4600, -1700, -1400, 0, 100, m_trim, folder="Set/Props")    # the vault corridor
deco("Vault_Desk_Screen", 4540, 4548, -1590, -1510, 100, 140, m_glow)
chair(4470.0, -1550.0, 0)
guest(4470.0, -1550.0, z=45.0, facing=0.0, seated=True, mat=GUEST_MATS[1])
deco("Keypad", 4712, 4728, VAULT_Y0 - 6, VAULT_Y0, 120, 150, m_glow)
table(5100.0, -2200.0, 120, 70, h=80)
crates(5180.0, -1300.0, (90, 60))
for (y0, y1) in ((-900, -200), (200, 900)):             # the vault
    box("Deposit_Boxes", HX1 + T + 5, HX1 + T + 50, y0, y1, 0, 220, m_case, folder="Set/Props")
for i in range(3):
    deco("Gold_Bar", 4660 + (i % 2) * 8, 4700 + (i % 2) * 8, -12 + i * 4, 6 + i * 4, 90 + i * 7, 97 + i * 7, m_gold)
box("Safe", 4700, 4900, 850, 990, 0, 200, m_trim, folder="Set/Props")
deco("Safe_Handle", 4790, 4810, 844, 850, 95, 115, m_gold)
crates(5080.0, -520.0, (120,))
box("Statue_Plinth", 4825, 4875, 475, 525, 0, 100, m_trim, folder="Set/Props")
ghost(shape(SPHERE, "Statue", 4850.0, 500.0, 130.0, 55.0, 55.0, 60.0, m_gold))

# =====================================================================================================================
# The look (lighting only): a night gala. A cool moon, a dim sky fill and haze; warm keys down Julian's path, cool
# rims from the dais end, red at the watchers and gold at the reel. Everything is movable: nothing is baked.
# No Lumen (docs/STATUS.md, "Look"): the post-process volume turns it off for this level.
# =====================================================================================================================
def try_set(obj, **props):
    for k, v in props.items():
        try:
            obj.set_editor_property(k, v)
        except Exception as e:
            unreal.log_warning("look: %s.%s: %s" % (obj.get_class().get_name(), k, e))


moon = spawn(unreal.DirectionalLight, (0, 0, 2000), yaw=158.0, pitch=-40.0, label="Moon", folder="Light")
moon_c = moon.get_component_by_class(unreal.DirectionalLightComponent)
moon_c.set_mobility(unreal.ComponentMobility.MOVABLE)
try_set(moon_c, intensity=MOON_LUX, light_color=tint(COOL), volumetric_scattering_intensity=0.4)
spawn(unreal.SkyAtmosphere, (0, 0, 0), label="Sky", folder="Light")
sky = spawn(unreal.SkyLight, (0, 0, 1800), label="SkyLight", folder="Light")
sky_c = sky.get_component_by_class(unreal.SkyLightComponent)
sky_c.set_mobility(unreal.ComponentMobility.MOVABLE)
try_set(sky_c, real_time_capture=True, intensity=SKY_FILL, light_color=tint((0.75, 0.82, 1.0)))
fog = spawn(unreal.ExponentialHeightFog, (0, 0, 0), label="Haze", folder="Light")
fog_c = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
try_set(fog_c, fog_density=FOG_DENSITY, fog_height_falloff=0.02, enable_volumetric_fog=True,
        volumetric_fog_scattering_distribution=0.55, volumetric_fog_extinction_scale=1.0,
        volumetric_fog_distance=9000.0)
ppv = spawn(unreal.PostProcessVolume, (0, 0, 0), label="Exposure", folder="Light")
ppv.set_editor_property("bUnbound", True)
pps = ppv.get_editor_property("settings")
try_set(pps, override_auto_exposure_min_brightness=True, auto_exposure_min_brightness=EXPOSURE_EV,
        override_auto_exposure_max_brightness=True, auto_exposure_max_brightness=EXPOSURE_EV,
        override_vignette_intensity=True, vignette_intensity=0.0,     # step 4: the grade does the vignette (§15.3)
        override_bloom_intensity=True, bloom_intensity=0.4,
        override_bloom_threshold=True, bloom_threshold=1.0,
        override_tone_curve_amount=True, tone_curve_amount=0.0,       # the grade does the tone (tools/build_look.py)
        override_expand_gamut=True, expand_gamut=0.0,
        override_blue_correction=True, blue_correction=0.0,
        weighted_blendables=unreal.WeightedBlendables(array=[unreal.WeightedBlendable(weight=1.0, object=GRADE)]),
        override_dynamic_global_illumination_method=True,
        dynamic_global_illumination_method=unreal.DynamicGlobalIlluminationMethod.NONE,
        override_reflection_method=True, reflection_method=unreal.ReflectionMethod.NONE,
        override_motion_blur_amount=True, motion_blur_amount=0.0)
ppv.set_editor_property("settings", pps)

# The street: lamp heads, the canopy, a warm key on the carpet at the scanner, red at the scanner's eye.
for ly in (-1900.0, -700.0, 700.0, 1900.0):
    lamp(PAVE_X0 - 30.0, ly, z=450.0, cd=57.0, radius=1700.0)
for cy in (-330.0, 0.0, 330.0):
    lamp(FRONT - 115.0, cy, z=400.0, cd=15.0, radius=800.0)
spot("Key_Scanner", SCAN_X - 360.0, SCAN_Y - 20.0, 470.0, 0.0, -58.0, 420.0, 30.0, 1200.0, WARM, shadows=True)
lamp(SCAN_X - 30.0, SCAN_Y, z=235.0, cd=2.2, radius=320.0, rgb=REDL)
lamp(VEX_START[0], VEX_START[1] + 150.0, z=400.0, cd=30.0, radius=1100.0, rgb=WORK)
lamp((PLAT[0] + PLAT[1]) / 2, (PLAT[2] + PLAT[3]) / 2, z=CAT_Z + 280.0, cd=20.0, radius=900.0, rgb=WORK)
# The lobby: a chandelier's worth over the atrium, a key down the grand stair.
lamp((VOID_X0 + VOID_X1) / 2, 0.0, z=820.0, cd=250.0, radius=2600.0)   # step 4 pass 3: 420 lit the lobby past Julian
spot("Key_Stair", VOID_X0 - 200.0, 0.0, 860.0, 0.0, -55.0, 450.0, 26.0, 2400.0, WARM, shadows=True)
# The hall: warm keys straight down the centre aisle (Julian's path), the last two casting shadows.
for kx in (-500.0, 400.0, 1300.0):
    spot("Key_Aisle", kx, 0.0, HALL_H - 60.0, 0.0, -90.0, KEY_CD, 22.0, 1700.0, WARM, haze=2.0)
spot("Key_Rope", MARK_ROPE[0], 0.0, HALL_H - 60.0, 0.0, -90.0, KEY_CD, 22.0, 1700.0, WARM, shadows=True, haze=2.0)
spot("Key_Tape", (TAPE_X0 + TAPE_X1) / 2, 0.0, HALL_H - 60.0, 0.0, -90.0, KEY_CD, 22.0, 1700.0, WARM, haze=2.0)
# The dais: a key on Vance at the podium, gold on the reel's case.
spot("Key_Podium", 3560.0, VANCE[1], 900.0, 0.0, -68.0, 720.0, 17.0, 1500.0, WARM, shadows=True, haze=1.2)
spot("Fill_Dais", 4000.0, -350.0, 1250.0, 0.0, -90.0, 1400.0, 48.0, 1900.0, WARM, haze=0.4)   # pass 3: was 700
spot("Key_Reel", 4160.0, 0.0, 1000.0, 0.0, -90.0, 660.0, 9.0, 1300.0, GOLDL, shadows=True, haze=2.0)
# Rims: cool back light from the dais end, so everyone between Julian and the dais reads as a silhouette.
for ry in (-750.0, 750.0):
    spot("Rim_Hall", HX1 - 80.0, ry, 1180.0, 180.0, -24.0, RIM_CD, 36.0, 6500.0, COOL, haze=2.5)
# Red at the rope, on both sides of the gap. Work light on the bridge and the walkway (the making-of).
for ry in (-GAP - 60.0, GAP + 60.0):
    lamp(ROPE_X, ry, z=150.0, cd=12.0, radius=420.0, rgb=REDL)
lamp((BRIDGE_X0 + BRIDGE_X1) / 2, 300.0, z=CAT_Z + 280.0, cd=15.0, radius=900.0, rgb=WORK)
for wx in (300.0, 2400.0, 3700.0):
    lamp(wx, (WALK_Y0 + HY1) / 2, z=CAT_Z + 280.0, cd=15.0, radius=1100.0, rgb=WORK)

# =====================================================================================================================
# The rules: the director, the characters, marks, watchers, devices, the scene triggers and the crew posts.
# =====================================================================================================================
A, B, C = unreal.TakeCameraId.A, unreal.TakeCameraId.B, unreal.TakeCameraId.C
spawn(unreal.TakeDirector, (HX0 + 100, HY0 + 100, 0), label="TakeDirector", folder="Take")

# The three characters. Z is the capsule's centre. The cover story (Marwan, Nov 14): Aurelia hired a film crew to
# light and broadcast its gala, and that crew is the player's. Maya and Vex carry crew passes, so nobody watches
# them; Julian is an actor playing a bidder with no invitation, so every watcher is something that would check him.
# Each is introduced by one line the first time the player lands on them.
CAPSULE = 98.0
julian = spawn(unreal.Julian, (START[0], START[1], CAPSULE), yaw=-90.0, label="Julian", folder="Take")
setp(julian, intro_line='DIRECTOR: "Julian, our bidder with no invitation. Every guard is watching him."')
maya = spawn(unreal.TakeCharacter, (MAYA_START[0], MAYA_START[1], F2 + CAPSULE), yaw=180.0, label="Maya",
             folder="Take")
setp(maya, id=B, tint=unreal.LinearColor(0.10, 0.35, 0.80, 1.0), start_look_pitch=-42.0,
     intro_line='DIRECTOR: "Maya, our gaffer, up on the lights. Crew pass: nobody watches her."')
vex = spawn(unreal.TakeCharacter, (VEX_START[0], VEX_START[1], CAPSULE), yaw=143.0, label="Vex", folder="Take")
setp(vex, id=C, tint=unreal.LinearColor(0.10, 0.55, 0.30, 1.0),
     intro_line='DIRECTOR: "Vex, props and sound. Crew pass: backstage is his."')


def mark(label, name, xy, z=0.0):
    a = spawn(unreal.Mark, (xy[0], xy[1], z), label=label, folder="Take")
    setp(a, mark_name=name)
    return a


def trigger(label, name, x0, x1, y0, y1, z=0.0, scene=0, title="", checkpoint=None, checkpoint_name=""):
    """A scene trigger box: fires once when Julian walks in; may start a scene and may save a checkpoint."""
    a = spawn(unreal.SceneTrigger, ((x0 + x1) / 2, (y0 + y1) / 2, z), label=label, folder="Take")
    setp(a, trigger_name=name, extent=unreal.Vector((x1 - x0) / 2, (y1 - y0) / 2, 200.0), scene=scene, title=title,
         checkpoint_name=checkpoint_name)
    if checkpoint is not None:
        setp(a, checkpoint_mark=checkpoint)
    return a


def tape(label, x0, x1, y0, y1, z=0.0):
    a = spawn(unreal.RedTapeZone, ((x0 + x1) / 2, (y0 + y1) / 2, z), label=label, folder="Take")
    setp(a, extent=unreal.Vector((x1 - x0) / 2, (y1 - y0) / 2, 200.0))
    return a


# --- Scene 1, The Door (on the street) ------------------------------------------------------------------------------
mark("Mark_Start", "THE QUEUE", START)
mark("Mark_Scanner", "SCANNER", MARK_SCANNER)
scanner = spawn(unreal.ScannerWatcher, (SCAN_X, SCAN_Y, 0), label="Scanner", folder="Take")
setp(scanner, label="SCANNER", gate_half_width=SCAN_HALF, frame_depth=SCAN_DEPTH,
     seen_line='DOORMAN: "No match. Hold there, sir."', blinded_line='DOORMAN: "Who turned that light on?!"')   # W3 opening: the man at the scanner
tape("RedTape_Scanner", SCAN_X - SCAN_DEPTH, SCAN_X, SCAN_Y - SCAN_HALF, SCAN_Y + SCAN_HALF)
# The carpet is the invited guests' lane: red tape from the kerb to the doors, the doorman's frame straight down it.
tape("RedTape_Carpet", PAVE_X0 + 60.0, FRONT - 30.0, -LANE_ROPE + 10.0, LANE_ROPE - 10.0)
doorman = spawn(unreal.PatrolGuard, (DOORMAN[0], DOORMAN[1], 0), yaw=180.0, label="Doorman", folder="Take")
setp(doorman, label="DOORMAN", frame_range=760.0, frame_half_angle=14.0,
     seen_line='DOORMAN: "Invitation, sir?"')
# Inside, the carpet runs on to the grand stair under the security chief's eye.
tape("RedTape_LobbyCarpet", LX0 + 10.0, CHIEF[0] - 20.0, -CARPET_HALF, CARPET_HALF)
chief = spawn(unreal.PatrolGuard, (CHIEF[0], CHIEF[1], 0), yaw=180.0, label="Chief", folder="Take")
setp(chief, label="CHIEF", frame_range=500.0, frame_half_angle=17.0,
     seen_line='CHIEF: "You. That carpet is for invited guests."')
# Maya's arrivals searchlight on the terrace, over the scanner. A short range and a wide aim assist: from where she
# stands the scanner is picked at once, and the doorman under the terrace never is.
searchlight = spawn(unreal.FollowSpot, SEARCHLIGHT, label="Searchlight", folder="Take")
# It blinds for 6 s, not the follow spot's 4: the flight back to Julian takes 1.6 s of it, and this is the lesson.
setp(searchlight, device_name="SEARCHLIGHT", effect=unreal.DeviceEffect.BLIND, range=300.0, duration=6.0, uses=0,
     fired_from=B, use_verb="Swing the searchlight onto the", aim_assist_degrees=75.0)
setp(searchlight, use_offset=use_offset(searchlight, (MAYA_START[0], MAYA_START[1], F2)))
ghost(shape(CYL, "Searchlight_Stand", SEARCHLIGHT[0], SEARCHLIGHT[1], (F2 + SEARCHLIGHT[2]) / 2, 8.0, 8.0,
            SEARCHLIGHT[2] - F2, m_trim, folder="Set/Street"))
trigger("Trigger_Door", "Door", LX0 + 20.0, LX0 + 240.0, LANE_ROPE + 10.0, FRONT_DOOR_HALF)
mark("Mark_Desk", "THE DESK", MARK_DESK)
# --- QA scene1: the queue line (AQueueLine, QueueLine.h has the beat sheet). At the origin, so its points read as
# world: the places (the street dressing put a guest on each but Julian's), where a guest stands to be scanned (in
# the scanner's light, before the gate), the way in through the gate and the doors, and where they go inside.
queue_line = spawn(unreal.QueueLine, (0.0, 0.0, 0.0), label="QueueLine", folder="Take")
setp(queue_line, places=[unreal.Vector(qx, qy, 0.0) for qx, qy in QUEUE_PLACES], julian_place=QUEUE_JULIAN,
     scan_point=unreal.Vector(SCAN_X - 90.0, SCAN_Y, 0.0),
     way_in=[unreal.Vector(SCAN_X + 130.0, SCAN_Y, 0.0), unreal.Vector(FRONT + 160.0, SCAN_Y + 60.0, 0.0)],
     inside_spots=[unreal.Vector(sx, sy, 0.0) for sx, sy in ((-2750.0, 1150.0), (-2600.0, 1250.0), (-2450.0, 1150.0))])
# --- W3 opening: the receptionist stands at the moved desk in play (build_cast still dresses him at (-2050, 1100)).
setp(queue_line, clerk_from=unreal.Vector(-2050.0, 1100.0, 0.0), clerk_point=unreal.Vector(-2050.0, 630.0, 0.0),
     clerk_yaw=180.0)
# --- end QA scene1

# --- Scene 2, The Floor -----------------------------------------------------------------------------------------------
m_floor_mark = mark("Mark_Floor", "THE FLOOR", MARK_FLOOR)
mark("Mark_Seat", "AISLE SEAT", MARK_SEAT)
mark("Mark_Cocktails", "COCKTAILS", MARK_COCKTAILS)
m_rope_mark = mark("Mark_Rope", "ROPE", MARK_ROPE)
m_dais_mark = mark("Mark_Dais", "THE DAIS", MARK_DAIS, DAIS_H)
m_lift = mark("Mark_Lift", "LIFT", MARK_LIFT, DAIS_H)
# The spec says 1.5 s. A flight back from Maya costs 1.6 s of the spot's 4 s, so 1.0 s is what fits (STATUS.md).
setp(m_lift, action_seconds=1.0, action_trigger="Keycard", action_verb="Lifting the keycard")
# The keycard starts Scene 3 and is a checkpoint: a blooper before the case puts him back on the dais mark (no
# frame covers it), still holding the keycard.
setp(m_lift, action_scene=3, action_title="SCENE 3|THE GETAWAY", action_checkpoint_name="THE KEYCARD",
     action_checkpoint_mark=m_dais_mark)

# Walking onto the hall floor starts Scene 2 (a checkpoint); the rope mark and the dais are checkpoints too. Stepping
# into the aisle brings Vance and the bodyguard on: 8.5 s of walking from there to the end of a lift, and they
# arrive within 8.
trigger("Trigger_Scene2", "Scene2", HX0 + 20.0, ROPE_X - 30.0, HY0 + 10.0, HY1 - 10.0, scene=2,
        title="SCENE 2|THE FLOOR", checkpoint=m_floor_mark, checkpoint_name="THE FLOOR")
trigger("Trigger_Rope", "Rope", MARK_ROPE[0] - 180.0, ROPE_X - 20.0, -400.0, 400.0,
        checkpoint=m_rope_mark, checkpoint_name="THE ROPE")
trigger("Trigger_PastRope", "PastRope", ROPE_X + 20.0, ROPE_X + 300.0, -GAP + 10.0, GAP - 10.0)
trigger("Trigger_Dais", "Dais", MARK_DAIS[0] - 140.0, MARK_DAIS[0] + 140.0, -150.0, 150.0,
        checkpoint=m_dais_mark, checkpoint_name="THE DAIS")

# The red tape: the aisle, and the dais's south half (the podium and the lift mark).
tape("RedTape_Aisle", TAPE_X0, TAPE_X1, -GAP + 10.0, GAP - 10.0)
tape("RedTape_Dais", DAIS_TAPE[0], DAIS_TAPE[1], DAIS_TAPE[2], DAIS_TAPE[3], DAIS_H)

# The drone: a loop down the centre aisle and back, 8 bars; it arrives on the 4-bar line after Scene 2 starts. It
# flies above the catwalk's rail.
d1 = spawn(unreal.PatrolPoint, (DRONE_PATH[0][0], DRONE_PATH[0][1], 0), label="Drone_Path_1", folder="Take")
setp(d1, bars=4)
d2 = spawn(unreal.PatrolPoint, (DRONE_PATH[1][0], DRONE_PATH[1][1], 0), label="Drone_Path_2", folder="Take")
setp(d2, bars=4)
drone = spawn(unreal.DroneWatcher, (DRONE_PATH[0][0], DRONE_PATH[0][1], 0), label="Drone", folder="Take")
setp(drone, label="DRONE", path_points=[d1, d2], cone_radius=DRONE_RADIUS, fly_height=720.0, arrive_on="Scene2",
     instrument=unreal.WatcherInstrument.TRUMPET, seen_line='BROADCAST: "Who is the gentleman in white?"')

# The beam across the gap in the rope.
beam = spawn(unreal.BeamWatcher, (ROPE_X, -GAP, 0), label="Beam", folder="Take")
setp(beam, label="ELECTRIC EYE", end_b=unreal.Vector(0, 2 * GAP, 0), live_from_bar=1, drop_every_bars=4, drop_beat_in_bar=4,
     instrument=unreal.WatcherInstrument.HI_HAT)

# The guard stands beside the aisle and turns on bar lines.
p1 = spawn(unreal.PatrolPoint, (GUARD[0], GUARD[1], 0), yaw=GUARD_YAW_ON, label="Patrol_1", folder="Take")
setp(p1, bars=3)
p2 = spawn(unreal.PatrolPoint, (GUARD[0], GUARD[1], 0), yaw=GUARD_YAW_OFF, label="Patrol_2", folder="Take")
setp(p2, bars=1)
guard = spawn(unreal.PatrolGuard, (GUARD[0], GUARD[1], 0), yaw=GUARD_YAW_ON, label="Guard", folder="Take")
setp(guard, label="GUARD", frame_range=GUARD_RANGE, frame_half_angle=GUARD_HALF_ANGLE, patrol_points=[p1, p2],
     instrument=unreal.WatcherInstrument.BASS, seen_line='GUARD: "Sir? That aisle is closed."')

# Vance at the podium: his frame is on the lift mark behind him, except while he speaks (4 bars from every band
# break: bars 8, 16, 24...), when it turns to the crowd. A Lure only turns his head.
v1 = spawn(unreal.PatrolPoint, (VANCE[0], VANCE[1], DAIS_H), yaw=180.0, label="Vance_Speaks", folder="Take")
setp(v1, bars=4, state_text="SPEAKING", arrive_line='VANCE: "Ladies and gentlemen: the Director\'s Cut."')
v2 = spawn(unreal.PatrolPoint, (VANCE[0], VANCE[1], DAIS_H), yaw=0.0, label="Vance_Watches", folder="Take")
setp(v2, bars=4)
vance = spawn(unreal.PatrolGuard, (VANCE[0], VANCE[1], DAIS_H), yaw=180.0, label="Vance", folder="Take")
setp(vance, label="VANCE", frame_range=VANCE_RANGE, frame_half_angle=VANCE_HALF_ANGLE, patrol_points=[v1, v2],
     patrol_offset_bars=1, arrive_on="PastRope", instrument=unreal.WatcherInstrument.DRUM_ROLL,
     seen_line='VANCE: "Security!"')
vance.set_editor_property("bTurnsOnly", True)
# The bodyguard beside the podium, facing out at an angle: his frame covers the lift mark and its near approach.
bodyguard = spawn(unreal.PatrolGuard, (BODYGUARD[0], BODYGUARD[1], DAIS_H), yaw=BODYGUARD_YAW, label="Bodyguard",
                  folder="Take")
setp(bodyguard, label="BODYGUARD", frame_range=BODYGUARD_RANGE, frame_half_angle=BODYGUARD_HALF_ANGLE,
     arrive_on="PastRope", seen_line='BODYGUARD: "Step away from the podium."')

# Maya's devices on the lighting platform: the follow spot, and the winch of the banner over the rope.
spot_dev = spawn(unreal.FollowSpot, (SPOT[0], SPOT[1], CAT_Z), label="Spot", folder="Take")
setp(spot_dev, device_name="FOLLOW SPOT", effect=unreal.DeviceEffect.BLIND, range=3500.0, duration=4.0, uses=0,
     fired_from=B, use_verb="Put the follow spot on the")
setp(spot_dev, use_offset=use_offset(spot_dev, (SPOT_USE[0], SPOT_USE[1], CAT_Z)))
banner = spawn(unreal.BannerDevice, BATTEN, label="Banner", folder="Take")
# Nov 15 puzzle redesign: no banner hangs in the room. The same device is the breaker of the aisle's electric eye
# (no matte, it lands at once, the eye is dark for 10 s); tools/sets/puzzle.py sets the same values on a built level.
setp(banner, device_name="BREAKER", effect=unreal.DeviceEffect.MASK, range=600.0, duration=10.0, uses=1,
     fired_from=B, drop_distance=0.0, drop_seconds=0.02, raise_seconds=0.02, matte_size=unreal.Vector(1.0, 1.0, 1.0),
     use_verb="Pull the breaker on the aisle's electric eye",
     fire_line='NORA: "Breaker\'s out. The eye is dead for ten seconds."',
     effect_offset=unreal.Vector(ROPE_X - BATTEN[0], 0.0, 20.0 - BATTEN[2]))
setp(banner, use_offset=use_offset(banner, (BANNER_USE[0], BANNER_USE[1], CAT_Z)))

# Vex's devices, along the backstage corridor: the mid-floor cart (the guard), the sound desk (everything in the
# hall, 4 s), the dais cart (Vance and the bodyguard: x2).
cart = spawn(unreal.CartDevice, (CART[0], CART[1], 0), yaw=90.0, label="Cart", folder="Take")
setp(cart, device_name="CHAMPAGNE CART", effect=unreal.DeviceEffect.LURE, range=1200.0, duration=6.0, uses=1,
     fired_from=C, crash_offset=unreal.Vector(CART_ROLL, 0, 0), roll_seconds=1.0,
     use_verb="Kick the champagne cart through the hatch")
setp(cart, use_offset=use_offset(cart, (CART_USE[0], CART_USE[1], 0)))
cart2 = spawn(unreal.CartDevice, (CART2[0], CART2[1], 0), yaw=90.0, label="Cart_Dais", folder="Take")
setp(cart2, device_name="DAIS CART", effect=unreal.DeviceEffect.LURE, range=1200.0, duration=6.0, uses=1,
     fired_from=C, crash_offset=unreal.Vector(CART2_ROLL, 0, 0), roll_seconds=1.6,
     use_verb="Kick the dais cart through the hatch")
setp(cart2, use_offset=use_offset(cart2, (CART2_USE[0], CART2_USE[1], 0)))
desk = spawn(unreal.SoundDesk, (DESK[0], DESK[1], 0), yaw=90.0, label="Sound_Desk", folder="Take")
# Its range covers the hall's far corner (64 m) and stops short of the scanner on the street (73 m).
setp(desk, device_name="SOUND DESK", effect=unreal.DeviceEffect.BLIND, range=6500.0, duration=DESK_SECONDS, uses=1,
     fired_from=C, use_verb="Push feedback through the band's PA")
setp(desk, use_offset=use_offset(desk, (DESK_USE[0], DESK_USE[1], 0)))

# --- Scene 3, The Getaway ---------------------------------------------------------------------------------------------
# The case: its gold mark appears with the keycard. Standing on it takes the reel, trips the alarm and saves the
# scene's checkpoint. The rest of that bar and one more are held; then the floor is red tape, security walks in
# through both doors, the drone follows him, and 8 bars later the clock runs out.
m_case_mark = mark("Mark_Case", "THE CASE", MARK_CASE, DAIS_H)
setp(m_case_mark, appear_on="Keycard", action_seconds=1.2, action_trigger="Case", action_verb="Opening the case",
     action_checkpoint_name="THE CASE")
# The free way out: Vex's winch drops the curtain (the guards at both doors lose the dais) and opens the trap.
m_trap = mark("Mark_Trap", "THE TRAP", TRAP, DAIS_H)
setp(m_trap, appear_on="Winch", action_needs="Case", action_seconds=0.4, action_trigger="ExitTrap",
     action_verb="Dropping through the stage trap", action_while_seen=True, stand_radius=85.0)
# The paid way out: the gold door past the band. Two guards hold it; something saved has to take them both.
m_bow = mark("Mark_Bow", "THE BOW", MARK_BOW)
setp(m_bow, action_needs="Case", action_seconds=0.5, action_trigger="Bow", action_verb="Taking a bow",
     needs_blind_drone=True)
m_side = mark("Mark_Side", "THE GOLD DOOR", MARK_SIDE)
setp(m_side, action_needs="Case", action_seconds=0.3, action_trigger="ExitSide", action_verb="Out by the gold side exit")


def security(label, name, door, post, facing, line=""):
    """A guard who walks in at the alarm. door: where he stands and keeps standing. post: (start, end) for the one
    who walks out along the flank instead."""
    if post is None:
        g = spawn(unreal.PatrolGuard, (door[0], door[1], 0), yaw=facing, label=label, folder="Take")
        setp(g, frame_range=DOOR_RANGE, frame_half_angle=DOOR_HALF_ANGLE)
    else:
        a = spawn(unreal.PatrolPoint, (post[0][0], post[0][1], 0), yaw=facing, label=label + "_In", folder="Take")
        setp(a, bars=1)
        b = spawn(unreal.PatrolPoint, (post[1][0], post[1][1], 0), yaw=facing, label=label + "_Post", folder="Take")
        setp(b, bars=1)
        g = spawn(unreal.PatrolGuard, (post[0][0], post[0][1], 0), yaw=facing, label=label, folder="Take")
        setp(g, frame_range=FLANK_RANGE, frame_half_angle=FLANK_HALF_ANGLE, patrol_points=[a, b],
             patrol_from_arrival=True, patrol_once=True)
    setp(g, label=name, arrive_on="Case", hide_until_arrived=True, arrive_line=line,
         seen_line='SECURITY: "There! The man in white!"')
    return g


security("Security_N_Door", "SECURITY 4", SEC_N_DOOR, None, -90.0, 'SECURITY: "Nobody leaves!"')
security("Security_N_Flank", "SECURITY 3", None, SEC_N_POST, -90.0)
security("Security_S_Door", "SECURITY 2", SEC_S_DOOR, None, 90.0)
security("Security_S_Flank", "SECURITY 1", None, SEC_S_POST, 90.0)

# Maya's pyro board, at the south end of her platform. The charges stand along the front of the dais.
pyro = spawn(unreal.PyroDevice, (PYRO[0], PYRO[1], CAT_Z), label="Pyro", folder="Take")
setp(pyro, device_name="PYRO BOARD", effect=unreal.DeviceEffect.BLIND, range=PYRO_RANGE, duration=6.0, uses=1,
     fired_from=B, use_verb="Fire the pyro along the stage", locked_until="Case",
     fire_line='NORA: "Pyro! Eyes shut!"', flare_candelas=PYRO_CD)
setp(pyro, use_offset=use_offset(pyro, (PYRO_USE[0], PYRO_USE[1], CAT_Z)),
     effect_offset=use_offset(pyro, (PYRO_POINT[0], PYRO_POINT[1], DAIS_H)),
     flare_offsets=[use_offset(pyro, (fx, fy, DAIS_H)) for (fx, fy) in PYRO_FLARES])

# --- QA heist ---
# The swap in the dark (QA note 6; Reel/Source/Reel/Take/HouseDimmer.h tells the trick). Maya's house dimmer at the
# west rail of her platform takes the hall's lights down for the preview of Lot 41, the projector on the platform's
# east rail lights the screen over the dais with the old film, and every head turns to it. The case mark above keeps
# action_trigger="Case": with ATakeDirector.bHeistTrick on, the C++ makes it fire "Swap" at runtime; off, the old flow.
# --- W3 heist2 --- the room watches (RoomWatch.h): in house light the seated guests see Julian on the floor before the
# dais or on the dais (a glance, a murmur at 1 s, THE ROOM SAW HIM at 2.5 s), and lifting the keycard or opening the
# case there. So the dimmer unlocks as Julian enters the hall (Scene2), and the dark is the way up. Every position
# here comes from tools/sets/hall_zones.json (package stage); the old numbers are the fallback.
import json as _hz_json
_hz_path = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir() + "../tools/sets/hall_zones.json")
try:
    with open(_hz_path, encoding="utf-8") as _hz_f:
        HZ = _hz_json.load(_hz_f)
except Exception as _hz_e:
    unreal.log_warning("QA heist2: no hall_zones.json (%s): the old positions" % _hz_e)
    HZ = {}
_hz_floor = HZ.get("floor_z", 0.0)
DIMMER = tuple(HZ.get("dimmer", (470.0, -640.0, CAT_Z)))          # the board, at the platform's west rail
DIMMER_USE = tuple(HZ.get("maya_post", (545.0, -640.0, CAT_Z)))   # where Maya stands to throw it
PROJECTOR = tuple(HZ.get("projector", (745.0, -600.0, CAT_Z + 160.0)))   # the preview projector's lens
_hz_screen = HZ.get("screen", {})
SCREEN_FACE = tuple(_hz_screen.get("centre", (HX1 - 12.0, 0.0, 600.0)))  # the middle of the screen's face
SCREEN_SIZE = tuple(_hz_screen.get("size", (1000.0, 730.0)))             # across, tall
SCREEN_NORMAL = tuple(_hz_screen.get("normal", (-1.0, 0.0, 0.0)))        # facing the hall
HOUSE_POINT = (1700.0, 0.0)               # the hall's middle: the preview's range (35 m) is measured from here
_hz_front = HZ.get("front_floor", {"min": [3185.0, -1200.0], "max": [3600.0, 1200.0]})
_hz_dais = HZ.get("dais", {"min": [3600.0, -1200.0], "max": [4400.0, 1200.0], "z": DAIS_H})
if hasattr(unreal, "HouseDimmer"):
    dimmer = spawn(unreal.HouseDimmer, DIMMER, label="HouseDimmer", folder="Take")
    setp(dimmer, device_name="HOUSE DIMMER", effect=unreal.DeviceEffect.BLIND, range=3500.0, duration=0.75, uses=0,
         fired_from=B, use_verb="Bring the house lights down for the preview", use_radius=100.0,
         locked_until="Scene2", locked_why="for the preview, once Julian is in the hall",
         fire_line='NORA: "House lights down. Rolling the preview."', preview_seconds=16.0,
         house_min=unreal.Vector(HX0, HY0, -100.0), house_max=unreal.Vector(HX1, HY1, HALL_H + 100.0),
         screen_size=unreal.Vector2D(SCREEN_SIZE[0], SCREEN_SIZE[1]))
    try:
        setp(dimmer, screen_normal=unreal.Vector(*SCREEN_NORMAL))
    except Exception as _hz_e:
        unreal.log_warning("QA heist2: HouseDimmer has no screen_normal yet (old C++): %s" % _hz_e)
    setp(dimmer, use_offset=use_offset(dimmer, DIMMER_USE),
         screen_picture_roll=180.0,   # --- W3 heist2 --- seen in play: the film upright, not mirrored (leader digits)
         effect_offset=use_offset(dimmer, (HOUSE_POINT[0], HOUSE_POINT[1], 0.0)),
         projector_offset=use_offset(dimmer, PROJECTOR), screen_offset=use_offset(dimmer, SCREEN_FACE))
else:
    unreal.log_warning("QA heist: unreal.HouseDimmer is missing (C++ not built yet): no house dimmer, the old case flow")
if hasattr(unreal, "RoomWatch"):
    _hz_dz = _hz_dais.get("z", DAIS_H)
    room = spawn(unreal.RoomWatch, ((_hz_front["min"][0] + _hz_dais["max"][0]) / 2.0, 0.0, _hz_floor),
                 label="RoomWatch", folder="Take")
    setp(room, front_min=unreal.Vector(_hz_front["min"][0], _hz_front["min"][1], _hz_floor - 100.0),
         front_max=unreal.Vector(_hz_front["max"][0], _hz_front["max"][1], _hz_floor + 250.0),
         dais_min=unreal.Vector(_hz_dais["min"][0], _hz_dais["min"][1], _hz_dz - 100.0),
         dais_max=unreal.Vector(_hz_dais["max"][0], _hz_dais["max"][1], _hz_dz + 300.0))
else:
    unreal.log_warning("QA heist2: unreal.RoomWatch is missing (C++ not built yet): the room does not watch")
# --- W3 heist2 --- where Julian waits for the dark: the front rows' aisle, off the red tape (TAPE_X1), out of the
# room's view; a mark, so the drone's off-mark rule never catches him while he waits as long as he likes.
FRONT_ROW = (_hz_front["min"][0] - 85.0, -60.0)
mark("Mark_FrontRow", "THE FRONT ROW", FRONT_ROW)
# --- W3 heist2 --- the dais's gold marks show only once the house has gone dark (the C++ fires "Dark"): in house
# light nothing on the dais calls Julian up in front of the room.
setp(m_dais_mark, appear_on="Dark")
setp(m_lift, appear_on="Dark")
# --- end QA heist ---

# Vex's counterweight winch, in the wings at the east end of the corridor. The curtain: across the front of the
# dais and down both its sides.
winch = spawn(unreal.WinchDevice, (WINCH[0], WINCH[1], 0), yaw=90.0, label="Winch", folder="Take")
panels = []
for (ax, ay, bx, by) in ((DAIS_X0 + 6.0, -1200.0, DAIS_X0 + 6.0, 1200.0), (DAIS_X0, 1200.0, HX1, 1200.0),
                         (DAIS_X0, -1200.0, HX1, -1200.0)):
    p = unreal.TakeCurtainPanel()
    p.set_editor_property("a", unreal.Vector(ax, ay, DAIS_H))
    p.set_editor_property("b", unreal.Vector(bx, by, DAIS_H))
    panels.append(p)
setp(winch, device_name="CURTAIN WINCH", effect=unreal.DeviceEffect.MASK, range=0.0, duration=0.0, uses=1, fired_from=C,
     use_verb="Let the counterweight go: the curtain drops", locked_until="Case", land_trigger="Winch",
     fire_line='COLE: "Curtain coming down. The trap is yours."', panels=panels, curtain_height=CURTAIN_H,
     raised_height=HALL_H - CURTAIN_H - 60.0)
setp(winch, use_offset=use_offset(winch, (WINCH_USE[0], WINCH_USE[1], 0)))


def shutter(label, x, y, yaw, width, height, walkers=False):
    a = spawn(unreal.Shutter, (x, y, 0), yaw=yaw, label=label, folder="Take")
    setp(a, size=unreal.Vector(12.0, width, height), open_height=height, blocks_walkers=walkers)
    return a


# The shutters: the aisle at the dais end (no way back), and the hall's side doors.
# Nov 15 puzzle redesign: the aisle's steel shutter fell 8 m out of nothing. It is the bidders' gate now: two low
# walnut leaves at the aisle's front mouth, standing open against the front floor, that swing shut on the alarm
# (AShutter bSwingGate; tools/sets/puzzle.py sets the same on a built level and stands the newel posts).
GATE_HINGE, GATE_LEAF = GAP + 22.0, GAP + 20.0
for g_label, g_side, g_yaw in (("Shutter_Aisle", -1.0, 0.0), ("Shutter_Aisle_North", 1.0, 180.0)):
    g_leaf = spawn(unreal.Shutter, (AISLE_SHUTTER_X, g_side * GATE_HINGE, 0), yaw=g_yaw, label=g_label, folder="Take")
    setp(g_leaf, swing_gate=True, open_swing_degrees=g_side * 90.0, size=unreal.Vector(5.0, GATE_LEAF, 88.0),
         gate_foot_height=10.0, blocks_walkers=True, drop_seconds=0.35,
         colour=unreal.LinearColor(0.16, 0.075, 0.035, 1.0))
shutter("Shutter_North_1", 150.0, HY1 + 10.0, 90.0, 200.0, D_H)
shutter("Shutter_North_2", 1900.0, HY1 + 10.0, 90.0, 200.0, D_H)
shutter("Shutter_Staff", -300.0, HY0 - 10.0, 90.0, 2 * D_HALF, D_H)
lamp(HX1 - 90.0, SIDE_DOOR_Y, z=250.0, cd=30.0, radius=700.0, rgb=GOLDL)
lamp(HX1 - 90.0, SEC_DOOR_Y, z=250.0, cd=8.0, radius=500.0, rgb=REDL)
lamp(TRAP[0], TRAP[1], z=DAIS_H + 260.0, cd=10.0, radius=420.0, rgb=GOLDL)

# The level's game mode.
world = ues.get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", unreal.TakeGameMode)

les.save_current_level()
print("AUCTION built: %d actors; world %s" % (len(eas.get_all_level_actors()), world.get_path_name()))

# Step 4, pass 3: dress the set (docs/ART_STYLE.md §15.9, §15.10). Runs last, with this file's numbers; it re-runs
# alone too ("<engine python>" -I tools/ue_mcp.py py tools/dress_auction.py) and saves the level itself.
_dress = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir() + "../tools/dress_auction.py")
exec(open(_dress, encoding="utf-8").read(), dict(globals(), __name__="dress_auction"))

# Step 5: the costumed cast, the band's instruments and the devices' particle effects. Runs after the dressing; it
# re-runs alone too ("<engine python>" -I tools/ue_mcp.py py tools/build_cast.py) and saves the level itself.
_cast = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir() + "../tools/build_cast.py")
exec(open(_cast, encoding="utf-8").read(), dict(globals(), __name__="build_cast"))

# --- QA found --- Last: the set overlays (tools/sets/run_all.py: front, hall, rooms, devices, heroes), so a rebuild re-applies them.
# --- W3 stage --- removed (orchestrator): run_all ran before import_props had placed the props and undid front's
# stanchion removals. The integrator runs build_frames, import_props (materials, place, check) and then run_all itself.
