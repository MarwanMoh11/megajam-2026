# Scene 8. EXT. REAR ALLEYWAY - NIGHT (Vance). Owner: s8-alley. Re-runnable overlay (tag script_s8), run inside the editor
# after the existing pipeline:  "<engine python>" -I tools/ue_mcp.py py tools/script/scene8.py
# The alley is the strip south of the service wing (the building's south wall, y -3320, is its north side): a brick wall
# along y -4100 makes its far side, with a gap at x 2300..2900 (the side street the car leaves by); the alley's west end
# opens onto the street round the lobby (the car comes in round that corner); the crew's lorry closes its east end.
# AScene8Alley stands on the outside face of a steel fire exit at x 1300 (its door, guards, car, cameras are its own).
import importlib
import re
import sys

import unreal

sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C   # noqa: E402

importlib.reload(C)

TAG = "script_s8"
eas = C.eas
CUBE = "/Engine/BasicShapes/Cube"
M_WALL = "/Game/Take/Dress/MI_Front_Facade"       # the street side of the building (dress_auction.py)
M_DOOR = "/Game/Take/Materials/MI_Box_Door"
M_TRIM = "/Game/Take/Materials/MI_Box_Trim"
M_WINDOW = "/Game/Take/Materials/MI_Box_Window"
M_BRICK = "/Game/Take/Script/S8/MI_S8_Brick"        # the dress pass's brick, if it exists
M_PUDDLE = "/Game/Take/Script/S8/MI_S8_Puddle"
GROUND = -21.0                                       # the street's top (the building's floors are at 0)
DOOR = (1300.0, -3322.0, GROUND)                     # the fire exit's outside face, centre of its sill
FAR_Y = -4090.0                                      # the far brick wall's alley face (AScene8Alley.FarWallY = FAR_Y - DOOR.y)
GAP = (2300.0, 2900.0)
WEST, EAST = -1000.0, 3300.0
out = []


def mat(path, fallback):
    return path if unreal.EditorAssetLibrary.does_asset_exist(path) else fallback


def block(label, x0, x1, y0, y1, z0, z1, m, collide=True, shadow=True):
    a = C.spawn(CUBE, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), 0.0,
                ((x1 - x0) / 100.0, (y1 - y0) / 100.0, (z1 - z0) / 100.0), tag=TAG, label="S8_" + label,
                folder="Script/S8", collide=collide, shadow=shadow, mat=m)
    return a


def light(label, loc, cd, radius, rgb, cls=unreal.PointLight, rot=(0.0, 0.0, 0.0)):
    a = eas.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(*rot))
    a.set_actor_label("Set_S8_" + label)
    a.set_folder_path("Sets/Script/S8")
    a.set_editor_property("tags", [unreal.Name(TAG)])
    c = a.get_component_by_class(unreal.LocalLightComponent)
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    c.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    c.set_editor_property("intensity", cd)
    c.set_editor_property("attenuation_radius", radius)
    c.set_editor_property("light_color", unreal.Color(r=int(rgb[0] * 255), g=int(rgb[1] * 255), b=int(rgb[2] * 255), a=255))
    c.set_editor_property("cast_shadows", False)
    return a


C.clear(TAG)

# --- what stood in the alley: the south row of street trees inside it -------------------------------------------------
culprits = set()
for (x, y) in ((-1100.0, -4000.0), (400.0, -4000.0), (1900.0, -4000.0)):
    for lbl in C.near(x, y, r=150.0):
        if any(k in lbl for k in ("Tree", "Trunk", "Canopy", "Leaves", "Foliage")):
            culprits.add(lbl)
if culprits:
    out.append("hid %s" % sorted(C.hide([re.escape(lbl) for lbl in culprits], TAG)))

# --- the set --------------------------------------------------------------------------------------------------------
brick = mat(M_BRICK, M_WALL)
block("FarWall_W", WEST, GAP[0], FAR_Y - 30.0, FAR_Y, GROUND, 650.0, brick)
block("FarWall_E", GAP[1], EAST, FAR_Y - 30.0, FAR_Y, GROUND, 650.0, brick)
block("FarWall_Cap_W", WEST, GAP[0], FAR_Y - 40.0, FAR_Y + 10.0, 650.0, 670.0, M_TRIM, collide=False)
block("FarWall_Cap_E", GAP[1], EAST, FAR_Y - 40.0, FAR_Y + 10.0, 650.0, 670.0, M_TRIM, collide=False)
# brick facing over the building's own south wall along the alley, so both sides read as old brick
block("NearFacing", WEST, EAST, DOOR[1] - 4.0, DOOR[1], GROUND, 520.0, brick, collide=False)
# bins and crates along the far wall (clear of the car's lane, y -3660..-3840), a drainpipe, a step at the door
for i, bx in enumerate((-300.0, 160.0, 1150.0)):                      # none between the drive camera and the gap
    block("Bin_%d" % i, bx, bx + 120.0, FAR_Y + 10.0, FAR_Y + 95.0, GROUND, GROUND + 115.0, M_DOOR)
    block("BinLid_%d" % i, bx - 4.0, bx + 124.0, FAR_Y + 6.0, FAR_Y + 99.0, GROUND + 115.0, GROUND + 121.0, M_TRIM,
          collide=False)
block("Crates", 650.0, 790.0, FAR_Y + 10.0, FAR_Y + 130.0, GROUND, GROUND + 120.0, M_TRIM)
block("Drainpipe", 1050.0, 1062.0, DOOR[1] - 16.0, DOOR[1] - 4.0, GROUND, 520.0, M_TRIM, collide=False)
block("DoorStep", DOOR[0] - 90.0, DOOR[0] + 90.0, DOOR[1] - 40.0, DOOR[1], GROUND, GROUND + 6.0, M_TRIM, collide=False)
# the wet cobbles and the rain: Scene 1's materials (s1-marquee, tools/script/scene1_dress.py), reused as they are
PLANE = "/Engine/BasicShapes/Plane"
M_WET = "/Game/Take/Script/S1/MI_S1_WetCobble"
M_RAIN = "/Game/Take/Script/S1/MI_S1_Rain"
if unreal.EditorAssetLibrary.does_asset_exist(M_WET):
    C.spawn(PLANE, ((WEST + EAST) / 2, (DOOR[1] + FAR_Y) / 2, GROUND + 0.4), 0.0,
            ((EAST - WEST) / 100.0, (DOOR[1] - FAR_Y) / 100.0, 1.0), tag=TAG, label="S8_WetCobble",
            folder="Script/S8", collide=False, shadow=False, mat=M_WET)
else:
    puddle = mat(M_PUDDLE, M_WINDOW)
    for i, (px, py, sx, sy) in enumerate(((150.0, -3700.0, 260.0, 140.0), (900.0, -3560.0, 180.0, 110.0),
                                          (1750.0, -3800.0, 300.0, 160.0), (-500.0, -3650.0, 220.0, 120.0))):
        block("Puddle_%d" % i, px - sx / 2, px + sx / 2, py - sy / 2, py + sy / 2, GROUND, GROUND + 0.6, puddle,
              collide=False, shadow=False)
if unreal.EditorAssetLibrary.does_asset_exist(M_RAIN):
    n = 0
    for y in (-3450.0, -3700.0, -3950.0):                                 # along the alley (planes in x-z)
        C.spawn(PLANE, ((WEST + EAST) / 2, y, 560.0), (90.0, 0.0, 0.0), ((EAST - WEST) / 100.0, 11.2, 1.0),
                tag=TAG, label="S8_Rain_%d" % n, folder="Script/S8", collide=False, shadow=False, mat=M_RAIN)
        n += 1
    for x in (-600.0, 200.0, 1000.0, 1800.0, 2600.0):                     # across it (planes in y-z)
        C.spawn(PLANE, (x, (DOOR[1] + FAR_Y) / 2, 560.0), (90.0, 0.0, 90.0), (8.0, 11.2, 1.0),
                tag=TAG, label="S8_Rain_%d" % n, folder="Script/S8", collide=False, shadow=False, mat=M_RAIN)
        n += 1
    out.append("rain: %d planes" % n)

# --- light: a caged lamp over the exit, a lamp at each end, the side street's glow -----------------------------------
block("LampCage", DOOR[0] - 15.0, DOOR[0] + 15.0, DOOR[1] - 40.0, DOOR[1] - 4.0, 305.0, 330.0, M_TRIM, collide=False)
light("Lamp_Door", (DOOR[0], DOOR[1] - 60.0, 300.0), 400.0, 1100.0, (1.0, 0.72, 0.42))
light("Lamp_West", (WEST + 300.0, DOOR[1] - 60.0, 420.0), 250.0, 1200.0, (1.0, 0.66, 0.35))
light("Lamp_SideStreet", (2600.0, FAR_Y - 300.0, 450.0), 300.0, 1400.0, (0.6, 0.75, 1.0))

# --- the scene actor ------------------------------------------------------------------------------------------------
cls = unreal.load_class(None, "/Script/Reel.Scene8Alley")
if cls is None:
    out.append("AScene8Alley not compiled yet: set placed, no scene actor")
else:
    a = eas.spawn_actor_from_class(cls, unreal.Vector(*DOOR), unreal.Rotator(0, 0, 0))
    a.set_actor_label("Script_S8_Alley")
    a.set_folder_path("Script/S8")
    a.set_editor_property("tags", [unreal.Name(TAG)])
    a.set_editor_property("FarWallY", FAR_Y - DOOR[1])
    # the drive camera stands east of the turn into the gap, in front of the lorry: it sees the car come at it,
    # swing into the side street and fishtail away (from the west end the far wall hid the turn)
    a.set_editor_property("DriveCamAt", unreal.Vector(1850.0, -330.0, 180.0))
    a.set_editor_property("MirrorFov", 50.0)                              # the eyes and a little of the night
    out.append("placed %s at %s" % (a.get_actor_label(), DOOR))

print("SCENE8 " + "; ".join(out))
