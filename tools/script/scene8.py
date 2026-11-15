# Scene 8, the escape and the alley (Vance). Owner: s8-escape (was s8-alley). Re-runnable overlay (tag script_s8), run inside the editor
# after the existing pipeline:  "<engine python>" -I tools/ue_mcp.py py tools/script/scene8.py
# The alley is the strip south of the service wing (the building's south wall, y -3320, is its north side): a brick wall
# along y -4100 makes its far side, with a gap at x 2300..2900 (the side street the car leaves by); the alley's west end
# opens onto the street round the lobby (the car comes in round that corner); the crew's lorry closes its east end.
# AScene8Alley stands on the outside face of a steel fire exit at x 1300 (its door, guards, car, cameras are its own).
# The escape (Nov 15 evening): the scene opens in the security room where Scene 7's fight ends and Vance runs the real
# building to this door, Cole leading, guards chasing (ROUTE below; AScene8Alley.RouteWorld). This overlay cuts a real
# opening for the door in the prop store's south wall (Service_South), places the scene's StartSpot on the fight mark,
# and checks the route for blockers (printed as 'blocked').
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

# The prop store's south wall (build_auction.py Service_South): put back what the last run cut, then cut the door's
# opening (x CUT, z 0..CUT_TOP) by hiding it (and its collision) and rebuilding the wall round the opening.
CUT = (DOOR[0] - 62.0, DOOR[0] + 62.0)
CUT_TOP = 235.0
CUT_TAG = TAG + ":wallcut"
for w in C.actors():
    if any(t.startswith(CUT_TAG) for t in C.tags(w)):
        for c in w.get_components_by_class(unreal.PrimitiveComponent):
            c.set_visibility(True, False)
            c.set_collision_profile_name([t for t in C.tags(w) if t.startswith(CUT_TAG + "=")][0].split("=", 1)[1] if [t for t in C.tags(w) if t.startswith(CUT_TAG + "=")] else "BlockAll")
        C._set_tags(w, [t for t in C.tags(w) if not t.startswith(CUT_TAG)])
walls = [w for w in C.find("Service_South.*") if isinstance(w, unreal.StaticMeshActor)]
if walls:
    w = walls[0]
    b = C.bounds(w)
    wm = w.static_mesh_component.get_material(0)
    for c in w.get_components_by_class(unreal.PrimitiveComponent):
        c.set_visibility(False, False)
        prof = c.get_collision_profile_name()
        c.set_collision_profile_name("NoCollision")
    C._set_tags(w, C.tags(w) + [CUT_TAG + "=" + str(prof)])
    out.append("cut %s %s" % (w.get_actor_label(), [round(v) for v in b]))
else:
    b, wm = None, None
    out.append("NO Service_South wall found: the fire exit has no opening")

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
block("NearFacing_W", WEST, CUT[0], DOOR[1] - 4.0, DOOR[1], GROUND, 520.0, brick, collide=False)
block("NearFacing_E", CUT[1], EAST, DOOR[1] - 4.0, DOOR[1], GROUND, 520.0, brick, collide=False)
block("NearFacing_Top", CUT[0], CUT[1], DOOR[1] - 4.0, DOOR[1], CUT_TOP, 520.0, brick, collide=False)
if b:
    block("Wall_W", b[0], CUT[0], b[2], b[3], b[4], b[5], wm)
    block("Wall_E", CUT[1], b[1], b[2], b[3], b[4], b[5], wm)
    block("Wall_Top", CUT[0], CUT[1], b[2], b[3], CUT_TOP, b[5], wm)
    # Inside, over the door: a green EXIT sign and its glow (the way out reads from the prop store's door).
    block("ExitSign_In", DOOR[0] - 30.0, DOOR[0] + 30.0, b[3], b[3] + 6.0, 252.0, 272.0, M_TRIM, collide=False, shadow=False)
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
light("ExitGlow_In", (DOOR[0], -3255.0, 262.0), 40.0, 380.0, (0.25, 1.0, 0.45))

# --- the escape route ------------------------------------------------------------------------------------------------
# Feet points, world cm, from the security room (Scene 7's fight; s7-route-layout) to the prop store; AScene8Alley
# appends the fire exit's inside and outside. Rooms: tools/build_auction.py (security room x -3200..-2100,
# y -2000..-1400, z 450; the south fire stair x -2100..-1000; lobby; hall; staff door x -300; backstage corridor
# y -2600..-2020; prop store x 800..2600, y -3300..-2600, its door x 1700).
F2 = 450.0
START = (-2650.0, -1680.0, F2 + 95.0)
START_YAW = 90.0
COLE_START = (-1960.0, -1700.0, F2)            # on the fire stair's landing, outside the east door
COLE_MARK = (-2330.0, -1690.0, F2)             # just in through the east door
SMOKE = (-2650.0, -1260.0, F2)                 # out of the north door, on the lobby balcony (where the guards come)
ROUTE = [
    (-2060.0, -1700.0, F2),     # 0 the east door
    (-1880.0, -1850.0, F2),     # 1 the top of the upper flight (outer lane)
    (-1250.0, -1820.0, 225.0),  # 2 the half landing
    (-1250.0, -1560.0, 225.0),  # 3 across it to the inner lane
    (-1900.0, -1555.0, 0.0),    # 4 the foot of the lower flight
    (-1975.0, -1470.0, 0.0),    # 5 the door into the lobby
    (-1975.0, -1300.0, 0.0),    # 6 the lobby
    (-1150.0, -1180.0, 0.0),    # 7 south of the guests at the cocktail table (x -1397..-1202, y -1110..-945)
    (-1130.0, -800.0, 0.0),     # 8 up their east side
    (-1320.0, -680.0, 0.0),     # 9 west of the bench (x -1230..-1170) and the planter
    (-1320.0, -150.0, 0.0),     # 10 up to the opening
    (-1000.0, -40.0, 0.0),      # 11 the opening into the hall (y -350..350)
    (-520.0, -40.0, 0.0),       # 12 north of the S3 set piece and the projectionist's stair (to y -135)
    (-520.0, -950.0, 0.0),      # 13 down its east side
    (-200.0, -950.0, 0.0),      # 14 east, short of the guests at y -1110..-1012
    (-200.0, -1300.0, 0.0),     # 15 past them
    (-300.0, -1880.0, 0.0),     # 16 the staff door
    (-300.0, -2200.0, 0.0),     # 17 through it, into the backstage corridor
    (1700.0, -2300.0, 0.0),     # 18 east down the corridor: the chase
    (1700.0, -2750.0, 0.0),     # 19 through the prop store door
    (1325.0, -2900.0, 0.0),     # 20 between the crates, to the fire exit
]
CORRIDOR = 17                                  # the route point where the corridor starts
GUARDS_ENTER = (-1975.0, -1470.0, 0.0)         # they come down the fire stair after them
STUMBLE_X = 500.0                              # the stumble lands in the corridor about here


def route_len(pts, upto=None):
    n = 0.0
    for i in range(1, len(pts) if upto is None else upto + 1):
        p, q = pts[i - 1], pts[i]
        n += ((p[0] - q[0]) ** 2 + (p[1] - q[1]) ** 2 + (p[2] - q[2]) ** 2) ** 0.5
    return n


# The trail distance from the stair foot to the stumble, less the guards' starting gap (AScene8Alley.GapStart).
_corr = route_len(ROUTE, CORRIDOR) + (STUMBLE_X - ROUTE[CORRIDOR][0])
STUMBLE_AFTER = max(300.0, _corr - route_len(ROUTE, 5) - 1000.0)

# Blockers along the flat legs: a 32 cm sphere swept 60 cm over the floor (the stair legs are not checked).
blocked = []
for i in range(1, len(ROUTE)):
    p, q = ROUTE[i - 1], ROUTE[i]
    if abs(p[2] - q[2]) > 1.0:
        continue
    r = unreal.SystemLibrary.sphere_trace_single(C.world(), unreal.Vector(p[0], p[1], p[2] + 60.0),
                                                 unreal.Vector(q[0], q[1], q[2] + 60.0), 32.0,
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [],
                                                 unreal.DrawDebugTrace.NONE, True)
    hit = r[0] if isinstance(r, tuple) else r
    if hit:
        parts = hit.to_tuple()
        who = [x for x in parts if isinstance(x, unreal.Actor)]
        loc = [x for x in parts if isinstance(x, unreal.Vector)]
        blocked.append("%d->%d %s at %s" % (i - 1, i, who[0].get_actor_label() if who else "?",
                                            [round(v) for v in (loc[0].x, loc[0].y, loc[0].z)] if loc else "?"))
out.append("blocked: %s" % (blocked or "none"))

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
    # The escape (world cm). Where Scene 7's fight leaves Vance (s7-route's fight mark), Cole outside the east door.
    sp = eas.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(*START), unreal.Rotator(0.0, 0.0, START_YAW))
    sp.set_actor_label("S8_StartSpot")
    sp.set_folder_path("Script/S8")
    sp.set_editor_property("tags", [unreal.Name(TAG)])
    a.set_editor_property("StartSpot", sp)
    a.set_editor_property("RouteWorld", [unreal.Vector(*p) for p in ROUTE])
    for k, v in (("ColeStartWorld", COLE_START), ("ColeMarkWorld", COLE_MARK), ("SmokeWorld", SMOKE),
                 ("GuardsEnterWorld", GUARDS_ENTER)):
        a.set_editor_property(k, unreal.Vector(*v))
    a.set_editor_property("StumbleAfter", STUMBLE_AFTER)
    a.set_editor_property("CarCueDistance", 1300.0)
    a.set_editor_property("MinGap", 450.0)                                # just behind his follow camera's boom (360)
    a.set_editor_property("GapEnd", 800.0)                                # corners slow him: room to spare
    a.set_editor_property("GuardRunSpeed", 420.0)
    out.append("route %d points, %.0f m; guards from the stair foot, stumble %.0f cm after they appear"
               % (len(ROUTE), route_len(ROUTE) / 100.0, STUMBLE_AFTER))
    out.append("placed %s at %s" % (a.get_actor_label(), DOOR))

print("SCENE8 " + "; ".join(out))
