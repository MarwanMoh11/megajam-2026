# Builds the auction house grey-box, /Game/Take/Lvl_Auction, from nothing. Run it inside the editor:
#   "<engine python>" -I tools/ue_mcp.py py tools/build_auction.py
# It is safe to re-run: it empties the level and lays everything out again from the numbers below (cm).
# X runs from the entrance (-X) to the dais (+X). +Y is the catwalk side, -Y the backstage side. The top is open.
# Julian has the floor, Maya the catwalk, Vex the backstage corridor: none of them can reach another's space.
import unreal

MAP = "/Game/Take/Lvl_Auction"
MAT_DIR = "/Game/Take/Materials"

# --- the building ----------------------------------------------------------------------------------------------------
HX0, HX1 = -1000.0, 4400.0        # the great hall, 54 m long
HY0, HY1 = -2000.0, 2000.0        # and 40 m wide
HALL_H = 1400.0
LX0, LX1 = -3200.0, -1000.0       # the entrance lobby, 22 x 28 m
LY0, LY1 = -1400.0, 1400.0
LOBBY_H = 800.0
DOOR_HALF, DOOR_H = 350.0, 500.0  # the opening from the lobby into the hall
BY0 = -2600.0                     # backstage corridor: from here to the hall's south wall (HY0)
BACK_H = 500.0
HATCHES = (600.0, 3500.0)         # service hatches in the south wall (X centres): a counter Vex looks over
HATCH_HALF, HATCH_Z0, HATCH_Z1 = 150.0, 110.0, 320.0
CAT_Z = 450.0                     # catwalk deck, top surface
WALK_Y0 = 1650.0                  # the long walkway runs along the north wall from here to HY1
WALK_X0 = -800.0
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
MAYA = (1640.0, 0.0)              # on the bridge's end, facing the rope
SPOT = (1675.0, -230.0)           # the lamp, on the deck by the rail
SPOT_USE = (1620.0, -230.0)       # where Maya stands to work it
BATTEN = (ROPE_X - 40.0, 0.0, 560.0)   # the banner hangs over the gap
BANNER_USE = (1620.0, 230.0)      # its winch, on the bridge
VEX = (3500.0, -2230.0)           # backstage, at the second hatch, facing the hall
CART = (3500.0, -1900.0)          # the cart, in the hall just inside that hatch; it rolls +Y
CART_ROLL = 350.0
CART_USE = (3500.0, -2080.0)      # its release, on Vex's side of the counter
EXPOSURE_EV = 0.5                 # fixed exposure (EV100)

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
    a.set_actor_scale3d(unreal.Vector((x1 - x0) / 100.0, (y1 - y0) / 100.0, (z1 - z0) / 100.0))
    return a


def blocker(label, x0, x1, y0, y1, z0, z1):
    """An unseen wall: it stops a walker but not a camera."""
    a = box(label, x0, x1, y0, y1, z0, z1, m_wall, folder="Set/Blockers")
    a.set_actor_hidden_in_game(True)
    c = a.static_mesh_component
    c.set_collision_response_to_channel(unreal.CollisionChannel.ECC_CAMERA, unreal.CollisionResponseType.ECR_IGNORE)
    c.set_collision_response_to_channel(unreal.CollisionChannel.ECC_VISIBILITY,
                                        unreal.CollisionResponseType.ECR_IGNORE)
    c.set_editor_property("cast_shadow", False)
    return a


def rail(x0, y0, x1, y1):
    """An open rail along a deck edge (a top bar and posts) with an unseen wall behind it."""
    z0, z1 = CAT_Z, CAT_Z + RAIL_H
    along_x = abs(x1 - x0) > abs(y1 - y0)
    if along_x:
        box("Rail", x0, x1, y0 - 4, y0 + 4, z1 - 8, z1, m_trim, folder="Set/Catwalk")
        blocker("RailBlock", x0, x1, y0 - 5, y0 + 5, z0, z1 + 60)
        n = max(1, int(round((x1 - x0) / 300.0)))
        for i in range(n + 1):
            x = x0 + (x1 - x0) * i / n
            box("Post", x - 4, x + 4, y0 - 4, y0 + 4, z0, z1 - 8, m_trim, folder="Set/Catwalk")
    else:
        box("Rail", x0 - 4, x0 + 4, y0, y1, z1 - 8, z1, m_trim, folder="Set/Catwalk")
        blocker("RailBlock", x0 - 5, x0 + 5, y0, y1, z0, z1 + 60)
        n = max(1, int(round((y1 - y0) / 300.0)))
        for i in range(n + 1):
            y = y0 + (y1 - y0) * i / n
            box("Post", x0 - 4, x0 + 4, y - 4, y + 4, z0, z1 - 8, m_trim, folder="Set/Catwalk")


def wall_x(label, y, x0, x1, z0, z1, mat, openings=()):
    """A wall along X at Y = y (thickness T, outside the room), with (centre, half, z0, z1) openings cut in it."""
    cuts = sorted(openings)
    x = x0
    for (c, h, oz0, oz1) in cuts:
        if c - h > x:
            box(label, x, c - h, y, y + T, z0, z1, mat)
        if oz0 > z0:
            box(label + "_Sill", c - h, c + h, y, y + T, z0, oz0, mat)
        if oz1 < z1:
            box(label + "_Lintel", c - h, c + h, y, y + T, oz1, z1, mat)
        x = c + h
    if x < x1:
        box(label, x, x1, y, y + T, z0, z1, mat)


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

# Floors.
box("Floor_Hall", HX0, HX1, HY0, HY1, -20, 0, m_floor)
box("Floor_Lobby", LX0, LX1, LY0, LY1, -20, 0, m_floor)
box("Floor_Backstage", HX0, HX1, BY0, HY0 - T, -20, 0, m_deck)

# The great hall's walls. West: the lobby opening. South: the service hatches. North and east: solid.
box("Hall_West", HX0 - T, HX0, HY0, -DOOR_HALF, 0, HALL_H, m_wall)
box("Hall_West", HX0 - T, HX0, DOOR_HALF, HY1, 0, HALL_H, m_wall)
box("Hall_West_Lintel", HX0 - T, HX0, -DOOR_HALF, DOOR_HALF, DOOR_H, HALL_H, m_wall)
box("Hall_East", HX1, HX1 + T, HY0 - T, HY1 + T, 0, HALL_H, m_wall)
box("Hall_North", HX0 - T, HX1, HY1, HY1 + T, 0, HALL_H, m_wall)
wall_x("Hall_South", HY0 - T, HX0 - T, HX1, 0, HALL_H, m_wall,
       openings=[(c, HATCH_HALF, HATCH_Z0, HATCH_Z1) for c in HATCHES])

# The lobby: lower walls, an entrance, a desk, four pillars.
box("Lobby_West", LX0 - T, LX0, LY0, LY1, 0, LOBBY_H, m_wall)
box("Lobby_South", LX0 - T, LX1 - T, LY0 - T, LY0, 0, LOBBY_H, m_wall)
box("Lobby_North", LX0 - T, LX1 - T, LY1, LY1 + T, 0, LOBBY_H, m_wall)
box("Lobby_Doors", LX0, LX0 + 15, -300, 300, 0, 380, m_trim)
box("Lobby_Desk", -2300, -2100, 700, 1200, 0, 110, m_trim)
for px in (-2700.0, -1600.0):
    for py in (-800.0, 800.0):
        box("Lobby_Pillar", px - 50, px + 50, py - 50, py + 50, 0, LOBBY_H, m_trim)

# Backstage: the corridor behind the south wall. Nobody on the floor can get in: the hatches have counters.
box("Back_South", HX0 - T, HX1 + T, BY0 - T, BY0, 0, BACK_H, m_wall)
box("Back_West", HX0 - T, HX0, BY0, HY0 - T, 0, BACK_H, m_wall)
box("Back_East", HX1, HX1 + T, BY0, HY0 - T, 0, BACK_H, m_wall)
for cx in (0.0, 1500.0, 2600.0):
    box("Back_Crate", cx, cx + 120, BY0 + 20, BY0 + 140, 0, 120, m_trim)

# Columns down both sides of the hall, and trusses across the open top.
for cx in (-500.0, 400.0, 1300.0, 2200.0, 3100.0, 4000.0):
    for cy in (-1300.0, 1300.0):
        box("Column", cx - 60, cx + 60, cy - 60, cy + 60, 0, HALL_H, m_trim)
    box("Truss", cx - 15, cx + 15, HY0, HY1, HALL_H - 40, HALL_H, m_trim)

# Auction seating: benches either side of the centre aisle, behind Julian's start.
for bx in (-400.0, -200.0, 0.0, 200.0, 400.0, 600.0):
    box("Bench", bx - 35, bx + 35, 250, 1000, 0, 60, m_trim)
    box("Bench", bx - 35, bx + 35, -1000, -250, 0, 60, m_trim)

# Lots on plinths along the north wall, and the champagne bar on the south side.
for lx in (-300.0, 600.0, 2500.0, 3400.0):
    box("Plinth", lx - 40, lx + 40, 1480, 1560, 0, 100, m_trim)
    box("Lot", lx - 25, lx + 25, 1495, 1545, 100, 160, m_case)
box("Bar", 800, 1600, -1560, -1470, 0, 110, m_trim)

# The catwalk: a walkway along the north wall and a bridge out over the floor. Open rails.
box("Walkway", WALK_X0, HX1, WALK_Y0, HY1, CAT_Z - 20, CAT_Z, m_deck, folder="Set/Catwalk")
box("Bridge", BRIDGE_X0, BRIDGE_X1, BRIDGE_Y0, WALK_Y0, CAT_Z - 20, CAT_Z, m_deck, folder="Set/Catwalk")
rail(WALK_X0, WALK_Y0, BRIDGE_X0, WALK_Y0)
rail(BRIDGE_X1, WALK_Y0, HX1, WALK_Y0)
rail(WALK_X0, WALK_Y0, WALK_X0, HY1)
rail(BRIDGE_X0, BRIDGE_Y0, BRIDGE_X0, WALK_Y0)
rail(BRIDGE_X1, BRIDGE_Y0, BRIDGE_X1, WALK_Y0)
rail(BRIDGE_X0, BRIDGE_Y0, BRIDGE_X1, BRIDGE_Y0)
for sx in (BRIDGE_X0 + 10, BRIDGE_X1 - 10):
    box("Bridge_Leg", sx - 10, sx + 10, BRIDGE_Y0 - 10, BRIDGE_Y0 + 10, 0, CAT_Z - 20, m_trim, folder="Set/Catwalk")
for sx in (0.0, 900.0, 2600.0, 3500.0):
    box("Walkway_Leg", sx - 10, sx + 10, WALK_Y0 - 10, WALK_Y0 + 10, 0, CAT_Z - 20, m_trim, folder="Set/Catwalk")

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
ppv = spawn(unreal.PostProcessVolume, (0, 0, 0), label="Exposure", folder="Light")
ppv.set_editor_property("bUnbound", True)
pps = ppv.get_editor_property("settings")
setp(pps, override_auto_exposure_min_brightness=True, auto_exposure_min_brightness=EXPOSURE_EV,
     override_auto_exposure_max_brightness=True, auto_exposure_max_brightness=EXPOSURE_EV)
ppv.set_editor_property("settings", pps)

# The rules.
spawn(unreal.TakeDirector, (HX0 + 100, HY0 + 100, 0), label="TakeDirector", folder="Take")

# The three characters. Z is the capsule's centre.
CAPSULE = 90.0
julian = spawn(unreal.Julian, (START[0], START[1], CAPSULE), label="Julian", folder="Take")
maya = spawn(unreal.TakeCharacter, (MAYA[0], MAYA[1], CAT_Z + CAPSULE), label="Maya", folder="Take")
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
spot = spawn(unreal.FollowSpot, (SPOT[0], SPOT[1], CAT_Z), label="Spot", folder="Take")
setp(spot, device_name="SPOT", effect=unreal.DeviceEffect.BLIND, range=3500.0, duration=4.0, uses=0,
     fired_from=unreal.TakeCameraId.B, use_verb="SPOT")
setp(spot, use_offset=use_offset(spot, (SPOT_USE[0], SPOT_USE[1], CAT_Z)))
banner = spawn(unreal.BannerDevice, BATTEN, label="Banner", folder="Take")
setp(banner, device_name="BANNER", effect=unreal.DeviceEffect.MASK, range=600.0, duration=10.0, uses=1,
     fired_from=unreal.TakeCameraId.B, drop_distance=300.0, use_verb="DROP THE BANNER",
     effect_offset=unreal.Vector(ROPE_X - BATTEN[0], 0.0, 20.0 - BATTEN[2]))
setp(banner, use_offset=use_offset(banner, (BANNER_USE[0], BANNER_USE[1], CAT_Z)))

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
