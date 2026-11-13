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
#           carpet by the queue, and Julian walks in through the front doors
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

# --- the street front ------------------------------------------------------------------------------------------------
PAVE_X0 = LX0 - 900.0             # the pavement in front of the building, level with its floors
ROAD_X0 = LX0 - 2300.0            # the road beyond the kerb, and the street's west edge
FRONT_DOOR_HALF, FRONT_DOOR_H = 300.0, 380.0   # the front doors stand open

# --- the route (Julian walks in from the street, then +X along Y = 0) -------------------------------------------------
START = (LX0 - 700.0, 0.0)        # on the pavement at the foot of the red carpet, facing the doors
HALL_MARK = (900.0, 0.0)          # the checkpoint in the hall: a blooper starts the next take here
HALL_MARK_RADIUS = 1200.0         # how close Julian must come to save it
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
# The shell: the front with the entrance doors standing open, and the block's north and south faces.
wall("Front", "y", LX0 - T, HY0 - T, HY1 + T, 0, TOP, m_wall, [(0.0, FRONT_DOOR_HALF, 0.0, FRONT_DOOR_H)])
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

# =====================================================================================================================
# Dressing: placeholder props and people, room by room. No rules anywhere in it.
# =====================================================================================================================
FRONT = LX0 - T                    # the facade's outer face

# --- the street front: red carpet, the scanner arch (a placeholder for Scene 1), the queue, the press, cars -----------
flat("Carpet", START[0] + 100, VOID_X0 + 80, -150, 150, m_carpet)
SCAN_X = FRONT - 110.0
for s in (1, -1):
    prop("Scanner_Post", SCAN_X - 10, SCAN_X + 10, s * 160, s * 180, 0, 240, m_case, folder="Set/Street")
    prop("Scanner_Wing", SCAN_X - 5, SCAN_X + 5, s * 180, s * 300, 0, 100, m_case, folder="Set/Street")
deco("Scanner_Lintel", SCAN_X - 10, SCAN_X + 10, -180, 180, 240, 262, m_case, folder="Set/Street")
deco("Scanner_Eye", SCAN_X - 14, SCAN_X - 10, -12, 12, 244, 258, m_red, folder="Set/Street")
box("Canopy", FRONT - 520, FRONT, -560, 560, 410, 430, m_trim, folder="Set/Street")
deco("Canopy_Fascia", FRONT - 530, FRONT - 520, -560, 560, 395, 445, m_gold, folder="Set/Street")
for s in (1, -1):
    prop("Canopy_Post", FRONT - 516, FRONT - 504, s * 540, s * 552, 0, 410, m_trim, folder="Set/Street")
deco("Sign", FRONT - 6, FRONT, -700, 700, 520, 640, m_gold, folder="Set/Street")
for c in (900.0, 1300.0, 1700.0):
    for s in (1, -1):
        for (z0, z1) in ((90.0, 300.0), (F2 + 70.0, F2 + 290.0)):
            deco("Window", FRONT - 4, FRONT, s * c - 120, s * c + 120, z0, z1, m_window, folder="Set/Street")
        prop("Planter", FRONT - 70, FRONT - 10, s * c - 110, s * c + 110, 0, 50, m_trim, folder="Set/Street")
        deco("Hedge", FRONT - 66, FRONT - 14, s * c - 104, s * c + 104, 50, 95, m_plant, folder="Set/Street")
# The queue north of the carpet, behind ropes; the doorman at its head; the press behind a rope to the south.
QX = (PAVE_X0 + 60.0, -3880.0, -3710.0, -3540.0)
for qy in (200.0, 440.0, -200.0):
    rope_line([(qx, qy) for qx in QX])
for qx in (-3990.0, -3890.0, -3790.0, -3690.0, -3600.0):
    guest(qx, 320.0 + rnd.uniform(-25, 25), facing=rnd.uniform(-15, 15))
guest(-3450.0, 235.0, facing=180.0, mat=GUEST_MATS[1])
for px in (-3950.0, -3800.0, -3650.0):
    guest(px, -300.0, facing=90.0 + rnd.uniform(-20, 20))
    tripod(px + 45, -245.0)
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
deco("Desk_Screen", -2180, -2170, 1060, 1140, 110, 150, m_glow)
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
guest(-2300.0, 865.0, z=F2, facing=270.0)
guest(-2180.0, 870.0, z=F2, facing=255.0)
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
            if rnd.random() < 0.25:
                deco("Paddle", bx + 14, bx + 18, gy + 24, gy + 38, 150, 172, m_head)
for (mx, my, n) in ((-750, -1300, 3), (-750, 1250, 2), (150, 1200, 3), (1100, 1150, 2), (1900, 1100, 3),
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
guest(3885.0, -480.0, z=DAIS_H, facing=180.0, mat=GUEST_MATS[1])   # the auctioneer at the podium
for s in (1, -1):
    plant(4330.0, s * 1100.0, DAIS_H)
    easel(4230.0, s * 480.0, DAIS_H)
# The second floor of the hall: benches, onlookers and a film camera on the west gallery; cases on the walkway.
for s in (1, -1):
    prop("Gallery_Bench", -990, -940, s * 400, s * 900, F2, F2 + 45, m_trim)
guest(-660.0, 500.0, z=F2, facing=0.0)
guest(-665.0, 575.0, z=F2, facing=-12.0)
guest(-660.0, -700.0, z=F2, facing=0.0)
tripod(-660.0, -1000.0, F2)
plant(-950.0, 1550.0, F2)
plant(-950.0, -1900.0, F2)
for (x0, x1, h) in ((1000, 1110, 60), (2200, 2330, 85), (3500, 3620, 60)):
    prop("Flight_Case", x0, x1, 1950, 1995, F2, F2 + h, m_floor)

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

# Julian's marks: where the take starts on the street, and the checkpoint in the hall.
m = spawn(unreal.Mark, (START[0], START[1], 0), label="Mark_Start", folder="Take")
setp(m, mark_name="START")
m = spawn(unreal.Mark, (HALL_MARK[0], HALL_MARK[1], 0), label="Mark_Hall", folder="Take")
setp(m, mark_name="THE HALL", checkpoint_radius=HALL_MARK_RADIUS)
m.set_editor_property("bCheckpoint", True)
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
