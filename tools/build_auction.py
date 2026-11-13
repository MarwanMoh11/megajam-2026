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
# X runs from the entrance (-X) to the dais (+X). +Y is north. Everything west of the rope connects; the only way
# past the rope is the gap and the aisle (the walkway above is railed, the hatches have counters).
import math

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
HATCHES = (600.0, 3500.0)         # service hatches in the south wall (X centres): a counter Vex looks over
HATCH_HALF, HATCH_Z0, HATCH_Z1 = 150.0, 110.0, 320.0
WALK_Y0 = 1650.0                  # the long walkway runs along the north wall from here to HY1
BRIDGE_X0, BRIDGE_X1 = 1400.0, 1700.0   # the bridge crosses from the walkway out over the floor
BRIDGE_Y0 = -350.0
RAIL_H = 100.0
T = 20.0                          # wall thickness

# --- the route (Julian walks +X along Y = 0) -------------------------------------------------------------------------
START = (900.0, 0.0)
MARK_ROPE = (2200.0, 0.0)         # the safe mark before the rope
ROPE_X = 2400.0
GAP = 150.0                       # half-width of the gap in the rope: the beam crosses it
AISLE_X1 = 3550.0                 # display cases wall the aisle from the rope to here: the only way to the dais
TAPE_X0, TAPE_X1 = 2450.0, 3550.0 # red tape inside the aisle: 11 m = 4.4 s at 250 cm/s
DAIS_X0, DAIS_H = 3600.0, 20.0
GOAL = (3900.0, 0.0)
GUARD = (3000.0, -600.0)          # he looks across the aisle (yaw +90) for 3 bars, then away (yaw -90) for 1
GUARD_RANGE, GUARD_HALF_ANGLE = 900.0, 45.0   # covers the aisle's full width for its whole length

# --- the crew --------------------------------------------------------------------------------------------------------
MAYA = (1640.0, -100.0)           # on the bridge's end, facing the rope, within reach of the lamp
SPOT = (1675.0, -230.0)           # the lamp, on the deck by the rail
SPOT_USE = (1620.0, -230.0)       # where Maya stands to work it
BATTEN = (ROPE_X - 40.0, 0.0, 560.0)   # the banner hangs over the gap
BANNER_USE = (1620.0, 230.0)      # its winch, on the bridge
VEX = (3500.0, -2230.0)           # backstage, at the second hatch, facing the hall
CART = (3500.0, -1900.0)          # the cart, in the hall just inside that hatch; it rolls +Y
CART_ROLL = 350.0
CART_USE = (3500.0, -2080.0)      # its release, on Vex's side of the counter
EXPOSURE_EV = 0.5                 # fixed exposure (EV100)
SKY_FILL = 1.8                    # skylight strength: the fill in every shadow
LAMP_CD = 30.0                    # ceiling lamps under the second floor (candelas)

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


def rail(x0, y0, x1, y1):
    """An open rail along a second-floor edge (a top bar and posts) with an unseen wall behind it."""
    z0, z1 = F2, F2 + RAIL_H
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


def stairs(label, x, y, yaw, run, rise, width, z0, n, mat):
    """n steps climbing from (x, y, z0) in the direction yaw (0, 90, 180 or 270), solid down to the floor, with an
    unseen ramp over the step edges so a walker glides up instead of bumping."""
    dx, dy = round(math.cos(math.radians(yaw))), round(math.sin(math.radians(yaw)))
    d, h = run / n, rise / n
    for i in range(n):
        a0, a1 = i * d, (i + 1) * d
        if dx:
            xs, ys = sorted((x + dx * a0, x + dx * a1)), (y - width / 2, y + width / 2)
        else:
            xs, ys = (x - width / 2, x + width / 2), sorted((y + dy * a0, y + dy * a1))
        box(label, xs[0], xs[1], ys[0], ys[1], 0.0, z0 + (i + 1) * h, mat, folder="Set/Stairs")
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


def fire_door(axis, face, c, inward):
    """A closed fire exit on a wall face: the door, its push bar, the sign over it."""
    lo, hi = sorted((face, face + inward * 8.0))
    blo, bhi = sorted((face + inward * 8.0, face + inward * 14.0))
    if axis == "x":
        box("Fire_Door", c - 60, c + 60, lo, hi, 0, 230, m_door, folder="Set/Exits")
        box("Fire_Door_Bar", c - 50, c + 50, blo, bhi, 100, 108, m_trim, folder="Set/Exits")
    else:
        box("Fire_Door", lo, hi, c - 60, c + 60, 0, 230, m_door, folder="Set/Exits")
        box("Fire_Door_Bar", blo, bhi, c - 50, c + 50, 100, 108, m_trim, folder="Set/Exits")
    exit_sign(axis, face, c, inward)


def lamp(x, y, z=F2 - SLAB - 30.0, cd=LAMP_CD, radius=1200.0):
    """A ceiling lamp for a room under the second floor (the sun can't reach it). No shadows: cheap."""
    N[0] += 1
    a = spawn(unreal.PointLight, (x, y, z), label="Lamp_%03d" % N[0], folder="Light/Lamps")
    c = a.get_component_by_class(unreal.PointLightComponent)
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    try:
        c.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    except Exception as e:
        unreal.log_warning("lamp units: %s" % e)
    setp(c, intensity=cd, attenuation_radius=radius, cast_shadows=False)


def use_offset(actor, world_xyz):
    """A world point as an offset in the actor's own space (for a device's use point)."""
    return unreal.MathLibrary.inverse_transform_location(actor.get_actor_transform(), unreal.Vector(*world_xyz))


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

# Ground floors (they run under the walls, so doorways have a floor), and the street around the building.
box("Street", LX0 - 1200, EX1 + 1200, SY0 - 1200, NY1 + 1200, -40, -21, m_street)
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
wall("Hall_North", "x", HY1, HX0 - T, HX1, F2 - SLAB, HALL_H, m_wall,
     [(600.0, D_HALF, F2, F2 + D_H), (3000.0, D_HALF, F2, F2 + D_H)])
wall("Hall_South", "x", HY0 - T, HX0 - T, HX1, 0, HALL_H, m_wall,
     [(c, HATCH_HALF, HATCH_Z0, HATCH_Z1) for c in HATCHES] + [(-300.0, D_HALF, 0.0, D_H)])
exit_sign("y", HX0, 0.0, 1, z=400.0, half=60.0)

# Columns down both sides of the hall, and trusses across the open top.
for cx in (-500.0, 400.0, 1300.0, 2200.0, 3100.0, 4000.0):
    for cy in (-1300.0, 1300.0):
        box("Column", cx - 60, cx + 60, cy - 60, cy + 60, 0, HALL_H, m_trim)
    box("Truss", cx - 15, cx + 15, HY0, HY1, HALL_H - 40, HALL_H, m_trim)

# Auction seating: benches either side of the centre aisle, behind Julian's start.
for bx in (-400.0, -200.0, 0.0, 200.0, 400.0, 600.0):
    box("Bench", bx - 35, bx + 35, 250, 1000, 0, 60, m_trim)
    box("Bench", bx - 35, bx + 35, -1000, -250, 0, 60, m_trim)

# Lots on plinths along the north wall, the champagne bar and the phone-bid desks on the south side.
for lx in (-300.0, 600.0, 2500.0, 3400.0):
    box("Plinth", lx - 40, lx + 40, 1480, 1560, 0, 100, m_trim)
    box("Lot", lx - 25, lx + 25, 1495, 1545, 100, 160, m_case)
box("Bar", 800, 1600, -1560, -1470, 0, 110, m_trim)
box("Phone_Desk", 1800, 2200, -1560, -1480, 0, 100, m_trim)

# The second floor inside the hall: the west gallery, the walkway along the north wall, the bridge. Open rails.
box("Gallery_West", HX0, GALLERY_X1, HY0, WALK_Y0, F2 - SLAB, F2, m_deck, folder="Set/Catwalk")
box("Walkway", HX0, HX1, WALK_Y0, HY1, F2 - SLAB, F2, m_deck, folder="Set/Catwalk")
box("Bridge", BRIDGE_X0, BRIDGE_X1, BRIDGE_Y0, WALK_Y0, F2 - SLAB, F2, m_deck, folder="Set/Catwalk")
rail(GALLERY_X1, HY0, GALLERY_X1, WALK_Y0)
rail(GALLERY_X1, WALK_Y0, BRIDGE_X0, WALK_Y0)
rail(BRIDGE_X1, WALK_Y0, HX1, WALK_Y0)
rail(BRIDGE_X0, BRIDGE_Y0, BRIDGE_X0, WALK_Y0)
rail(BRIDGE_X1, BRIDGE_Y0, BRIDGE_X1, WALK_Y0)
rail(BRIDGE_X0, BRIDGE_Y0, BRIDGE_X1, BRIDGE_Y0)
for sx in (BRIDGE_X0 + 10, BRIDGE_X1 - 10):
    box("Bridge_Leg", sx - 10, sx + 10, BRIDGE_Y0 - 10, BRIDGE_Y0 + 10, 0, F2 - SLAB, m_trim, folder="Set/Catwalk")
for sx in (0.0, 900.0, 2600.0, 3500.0):
    box("Walkway_Leg", sx - 10, sx + 10, WALK_Y0 - 10, WALK_Y0 + 10, 0, F2 - SLAB, m_trim, folder="Set/Catwalk")
for sy in (-1500.0, -700.0, 700.0, 1500.0):
    box("Gallery_Leg", GALLERY_X1 - 10, GALLERY_X1 + 10, sy - 10, sy + 10, 0, F2 - SLAB, m_trim, folder="Set/Catwalk")
for ly in (-1000.0, 1000.0):
    lamp(-800.0, ly)

# The rope: a low wall with a gap. Past it, display cases wall an aisle to the dais.
box("Rope_South", ROPE_X - 10, ROPE_X + 10, HY0, -GAP, 0, 110, m_rope)
box("Rope_North", ROPE_X - 10, ROPE_X + 10, GAP, HY1, 0, 110, m_rope)
box("Cases_South", ROPE_X + 10, AISLE_X1, -GAP - 60, -GAP, 0, 110, m_case)
box("Cases_North", ROPE_X + 10, AISLE_X1, GAP, GAP + 60, 0, 110, m_case)

# The dais: a low stage with the podium, the gold case, and a dark screen on the end wall.
box("Dais", DAIS_X0, HX1, -1200, 1200, 0, DAIS_H, m_dais)
box("Podium", 3780, 3840, -520, -440, DAIS_H, DAIS_H + 120, m_trim)
box("Case_Plinth", 4130, 4190, -30, 30, DAIS_H, DAIS_H + 90, m_trim)
box("Case_Reel", 4140, 4180, -20, 20, DAIS_H + 90, DAIS_H + 130, m_gold)
box("Screen", HX1 - 10, HX1, -700, 700, 300, 900, m_floor)

# --- the lobby block (two storeys) -----------------------------------------------------------------------------------
# The shell: the front with the entrance doors (closed), and the block's north and south faces.
box("Front", LX0 - T, LX0, HY0 - T, HY1 + T, 0, TOP, m_wall)
box("Block_North", LX0, HX0 - T, HY1, HY1 + T, 0, TOP, m_wall)
box("Block_South", LX0, HX0 - T, HY0 - T, HY0, 0, TOP, m_wall)
box("Lobby_Doors", LX0, LX0 + 15, -300, 300, 0, 380, m_door)
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
box("Lobby_Desk", -2300, -2100, 900, 1300, 0, 110, m_trim)
for bx in (-3050.0, -1200.0):
    box("Lobby_Bench", bx - 30, bx + 30, -600, -200, 0, 50, m_trim)
    box("Lobby_Bench", bx - 30, bx + 30, 200, 600, 0, 50, m_trim)
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
    fire_door("x", s * HY1, -1965.0, -s)
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
    box("Security_Monitor", mx - 60, mx + 60, -1975, -1965, F2 + 90, F2 + 160, m_floor)

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
    lamp(lx, -2330.0, z=400.0)
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

# Light: a high sun, a sky and a skylight, all movable (nothing to bake).
sun = spawn(unreal.DirectionalLight, (0, 0, 2000), yaw=-35.0, pitch=-72.0, label="Sun", folder="Light")
sun_c = sun.get_component_by_class(unreal.DirectionalLightComponent)
sun_c.set_mobility(unreal.ComponentMobility.MOVABLE)
sun_c.set_editor_property("intensity", 5.0)
spawn(unreal.SkyAtmosphere, (0, 0, 0), label="Sky", folder="Light")
sky = spawn(unreal.SkyLight, (0, 0, 1800), label="SkyLight", folder="Light")
sky_c = sky.get_component_by_class(unreal.SkyLightComponent)
sky_c.set_mobility(unreal.ComponentMobility.MOVABLE)
sky_c.set_editor_property("real_time_capture", True)
sky_c.set_editor_property("intensity", SKY_FILL)
ppv = spawn(unreal.PostProcessVolume, (0, 0, 0), label="Exposure", folder="Light")
ppv.set_editor_property("bUnbound", True)
pps = ppv.get_editor_property("settings")
setp(pps, override_auto_exposure_min_brightness=True, auto_exposure_min_brightness=EXPOSURE_EV,
     override_auto_exposure_max_brightness=True, auto_exposure_max_brightness=EXPOSURE_EV)
ppv.set_editor_property("settings", pps)

# The rules.
spawn(unreal.TakeDirector, (HX0 + 100, HY0 + 100, 0), label="TakeDirector", folder="Take")

# The three characters. Z is the capsule's centre.
CAPSULE = 98.0
julian = spawn(unreal.Julian, (START[0], START[1], CAPSULE), label="Julian", folder="Take")
maya = spawn(unreal.TakeCharacter, (MAYA[0], MAYA[1], F2 + CAPSULE), label="Maya", folder="Take")
setp(maya, id=unreal.TakeCameraId.B, tint=unreal.LinearColor(0.10, 0.35, 0.80, 1.0))
vex = spawn(unreal.TakeCharacter, (VEX[0], VEX[1], CAPSULE), yaw=90.0, label="Vex", folder="Take")
setp(vex, id=unreal.TakeCameraId.C, tint=unreal.LinearColor(0.10, 0.55, 0.30, 1.0))

# Julian's marks.
m = spawn(unreal.Mark, (START[0], START[1], 0), label="Mark_Start", folder="Take")
setp(m, mark_name="START")
m = spawn(unreal.Mark, (MARK_ROPE[0], MARK_ROPE[1], 0), label="Mark_Rope", folder="Take")
setp(m, mark_name="ROPE")
m = spawn(unreal.Mark, (GOAL[0], GOAL[1], DAIS_H), label="Mark_Goal", folder="Take")
setp(m, mark_name="GOAL")
m.set_editor_property("bGoal", True)

# The red tape fills the aisle.
tape = spawn(unreal.RedTapeZone, ((TAPE_X0 + TAPE_X1) / 2, 0, 0), label="RedTape_Aisle", folder="Take")
setp(tape, extent=unreal.Vector((TAPE_X1 - TAPE_X0) / 2, GAP - 10, 200))

# The beam across the gap in the rope.
beam = spawn(unreal.BeamWatcher, (ROPE_X, -GAP, 0), label="Beam", folder="Take")
setp(beam, label="BEAM", end_b=unreal.Vector(0, 2 * GAP, 0), live_from_bar=1, drop_every_bars=4, drop_beat_in_bar=4)

# The guard stands beside the aisle and turns on bar lines.
p1 = spawn(unreal.PatrolPoint, (GUARD[0], GUARD[1], 0), yaw=90.0, label="Patrol_1", folder="Take")
setp(p1, bars=3)
p2 = spawn(unreal.PatrolPoint, (GUARD[0], GUARD[1], 0), yaw=-90.0, label="Patrol_2", folder="Take")
setp(p2, bars=1)
guard = spawn(unreal.PatrolGuard, (GUARD[0], GUARD[1], 0), yaw=90.0, label="Guard", folder="Take")
setp(guard, label="GUARD", frame_range=GUARD_RANGE, frame_half_angle=GUARD_HALF_ANGLE, patrol_points=[p1, p2])

# Maya's devices, both worked from the bridge: the follow spot and the banner over the rope.
spot = spawn(unreal.FollowSpot, (SPOT[0], SPOT[1], F2), label="Spot", folder="Take")
setp(spot, device_name="SPOT", effect=unreal.DeviceEffect.BLIND, range=3500.0, duration=4.0, uses=0,
     fired_from=unreal.TakeCameraId.B, use_verb="SPOT")
setp(spot, use_offset=use_offset(spot, (SPOT_USE[0], SPOT_USE[1], F2)))
banner = spawn(unreal.BannerDevice, BATTEN, label="Banner", folder="Take")
setp(banner, device_name="BANNER", effect=unreal.DeviceEffect.MASK, range=600.0, duration=10.0, uses=1,
     fired_from=unreal.TakeCameraId.B, drop_distance=300.0, use_verb="DROP THE BANNER",
     effect_offset=unreal.Vector(ROPE_X - BATTEN[0], 0.0, 20.0 - BATTEN[2]))
setp(banner, use_offset=use_offset(banner, (BANNER_USE[0], BANNER_USE[1], F2)))

# Vex's device: the champagne cart, released from his side of the hatch.
cart = spawn(unreal.CartDevice, (CART[0], CART[1], 0), yaw=90.0, label="Cart", folder="Take")
setp(cart, device_name="CART", effect=unreal.DeviceEffect.LURE, range=1200.0, duration=6.0, uses=1,
     fired_from=unreal.TakeCameraId.C, crash_offset=unreal.Vector(CART_ROLL, 0, 0), roll_seconds=1.0,
     use_verb="RELEASE THE CART")
setp(cart, use_offset=use_offset(cart, (CART_USE[0], CART_USE[1], 0)))

# The level's game mode.
world = ues.get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", unreal.TakeGameMode)

les.save_current_level()
print("AUCTION built: %d actors; world %s" % (len(eas.get_all_level_actors()), world.get_path_name()))
