# Builds the step-1 test box, /Game/Take/Lvl_TestBox, from nothing. Run it inside the editor:
#   "<engine python>" -I tools/ue_mcp.py py tools/build_testbox.py
# It is safe to re-run: it empties the level and lays everything out again from the numbers below (cm).
# Julian walks along +X: start mark, the rope (a low wall with a gap the beam crosses), the red tape, the goal mark.
import unreal

MAP = "/Game/Take/Lvl_TestBox"
MAT_DIR = "/Game/Take/Materials"

# --- layout ----------------------------------------------------------------------------------------------------------
X0, X1 = -600.0, 2600.0          # floor, along Julian's walk
Y0, Y1 = -800.0, 800.0           # floor, across (the north side, Y1, is open: the cameras look in from there,
                                 # so Julian walks left to right on screen)
ROPE_X = 800.0                   # the rope line
GAP = 200.0                      # half-width of the gap in the rope
TAPE_X0, TAPE_X1 = 850.0, 1950.0 # red tape past the rope: 11 m = 4.4 s at 250 cm/s
START = (-300.0, 0.0)
MARK_ROPE = (600.0, 0.0)
GOAL = (2250.0, 0.0)
GUARD_A = (1400.0, -650.0)       # patrol point 1: he looks across Julian's path (yaw +90) for 3 bars
GUARD_B = (1400.0, -650.0)       # patrol point 2: same spot, back turned (yaw -90) for 1 bar        # patrol point 2
GUARD_RANGE, GUARD_HALF_ANGLE = 760.0, 45.0
CART_DOOR = (2450.0, -700.0)      # the cart rolls -X from here
CART_ROLL = 400.0                # door to crash, cm
BATTEN = (760.0, 420.0, 550.0)   # the banner hangs here and drops in front of the beam's second post
SPOT = (300.0, 700.0, 0.0)
# Cameras are framed for a 16:9 screen (the lens angle is across the width; a wider window only shows more).
CAM_B = ((1000.0, 1900.0, 1600.0), (1000.0, -100.0, 0.0))    # (position, look-at): Maya's catwalk
CAM_B_FOV = 75.0
CAM_A_DOOR = ((-1150.0, 420.0, 400.0), (900.0, -60.0, -110.0)) # the film's first angle: down the lane (60 deg lens)
CAM_A_ROPE = ((1500.0, 1500.0, 450.0), (1500.0, -100.0, 30.0)) # the film's second angle: side-on over the tape
ZONE_SPLIT_X = 500.0             # Julian past this X: the film cuts to the second angle
EXPOSURE_EV = 0.5                # fixed exposure (EV100), so dark floors stay dark

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


def spawn(cls, loc, yaw=0.0, pitch=0.0, label=None):
    a = eas.spawn_actor_from_class(cls, unreal.Vector(loc[0], loc[1], loc[2]),
                                   unreal.Rotator(roll=0.0, pitch=pitch, yaw=yaw))
    if a is None:
        raise RuntimeError("could not spawn %s" % cls)
    if label:
        a.set_actor_label(label)
    return a


def setp(obj, **props):
    for k, v in props.items():
        obj.set_editor_property(k, v)


CUBE = eal.load_asset("/Engine/BasicShapes/Cube")


def box(label, x0, x1, y0, y1, z0, z1, mat):
    a = spawn(unreal.StaticMeshActor, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), label=label)
    c = a.static_mesh_component
    c.set_static_mesh(CUBE)
    c.set_material(0, mat)
    a.set_actor_scale3d(unreal.Vector((x1 - x0) / 100.0, (y1 - y0) / 100.0, (z1 - z0) / 100.0))
    return a


def camera(label, cam_id, shot_name, pos_look, initial=False, fov=None):
    pos, look = pos_look
    rot = unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*pos), unreal.Vector(*look))
    a = spawn(unreal.TakeCamera, pos, yaw=rot.yaw, pitch=rot.pitch, label=label)
    setp(a, camera_id=cam_id, shot_name=shot_name)
    a.set_editor_property("bInitialShot", initial)
    if fov:
        a.camera_component.set_editor_property("field_of_view", fov)
    return a


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

# Floor, two walls (the north and west sides are open to the cameras), and the rope: a low wall with a gap.
box("Floor", X0, X1, Y0, Y1, -20, 0, m_floor)
box("Wall_East", X1, X1 + 20, Y0, Y1, 0, 300, m_wall)
box("Wall_South", X0 - 20, X1 + 20, Y0 - 20, Y0, 0, 300, m_wall)
box("Rope_South", ROPE_X - 10, ROPE_X + 10, Y0, -GAP, 0, 110, m_rope)
box("Rope_North", ROPE_X - 10, ROPE_X + 10, GAP, Y1, 0, 110, m_rope)

# Light: a sun, a sky and a skylight, all movable (nothing to bake).
sun = spawn(unreal.DirectionalLight, (0, 0, 900), yaw=-60.0, pitch=-55.0, label="Sun")
sun_c = sun.get_component_by_class(unreal.DirectionalLightComponent)
sun_c.set_mobility(unreal.ComponentMobility.MOVABLE)
sun_c.set_editor_property("intensity", 5.0)
spawn(unreal.SkyAtmosphere, (0, 0, 0), label="Sky")
sky = spawn(unreal.SkyLight, (0, 0, 700), label="SkyLight")
sky_c = sky.get_component_by_class(unreal.SkyLightComponent)
sky_c.set_mobility(unreal.ComponentMobility.MOVABLE)
sky_c.set_editor_property("real_time_capture", True)

# Fixed exposure everywhere.
ppv = spawn(unreal.PostProcessVolume, (0, 0, 0), label="Exposure")
ppv.set_editor_property("bUnbound", True)
pps = ppv.get_editor_property("settings")
setp(pps, override_auto_exposure_min_brightness=True, auto_exposure_min_brightness=EXPOSURE_EV,
     override_auto_exposure_max_brightness=True, auto_exposure_max_brightness=EXPOSURE_EV)
ppv.set_editor_property("settings", pps)

# Navmesh bounds over the floor.
nav = spawn(unreal.NavMeshBoundsVolume, ((X0 + X1) / 2, (Y0 + Y1) / 2, 100), label="NavBounds")
nav.set_actor_scale3d(unreal.Vector((X1 - X0 + 200) / 200.0, (Y1 - Y0 + 200) / 200.0, 2.5))

# The rules.
spawn(unreal.TakeDirector, (X0 + 100, Y0 + 100, 0), label="TakeDirector")

# Julian and his marks.
spawn(unreal.Julian, (START[0], START[1], 90.0), label="Julian")
m = spawn(unreal.Mark, (START[0], START[1], 0), label="Mark_Start")
setp(m, mark_name="START")
m = spawn(unreal.Mark, (MARK_ROPE[0], MARK_ROPE[1], 0), label="Mark_Rope")
setp(m, mark_name="ROPE")
m = spawn(unreal.Mark, (GOAL[0], GOAL[1], 0), label="Mark_Goal")
setp(m, mark_name="GOAL")
m.set_editor_property("bGoal", True)

# The red tape past the rope, wall to wall.
tape = spawn(unreal.RedTapeZone, ((TAPE_X0 + TAPE_X1) / 2, (Y0 + Y1) / 2, 0), label="RedTape_PastTheRope")
setp(tape, extent=unreal.Vector((TAPE_X1 - TAPE_X0) / 2, (Y1 - Y0) / 2 - 10, 200))

# The beam across the gap in the rope.
beam = spawn(unreal.BeamWatcher, (ROPE_X, -GAP, 0), label="Beam")
setp(beam, label="BEAM", end_b=unreal.Vector(0, 2 * GAP, 0), live_from_bar=1, drop_every_bars=4, drop_beat_in_bar=4)

# The patrol guard: he stands on the far side of the tape and turns on bar lines.
p1 = spawn(unreal.PatrolPoint, (GUARD_A[0], GUARD_A[1], 0), yaw=90.0, label="Patrol_1")
setp(p1, bars=3)
p2 = spawn(unreal.PatrolPoint, (GUARD_B[0], GUARD_B[1], 0), yaw=-90.0, label="Patrol_2")
setp(p2, bars=1)
guard = spawn(unreal.PatrolGuard, (GUARD_A[0], GUARD_A[1], 0), yaw=90.0, label="Guard")
setp(guard, label="GUARD", frame_range=GUARD_RANGE, frame_half_angle=GUARD_HALF_ANGLE, patrol_points=[p1, p2])

# The devices. The test box has no camera C, so all three are fired from B.
spot = spawn(unreal.FollowSpot, SPOT, label="Spot")
setp(spot, device_name="SPOT", effect=unreal.DeviceEffect.BLIND, range=3500.0, duration=4.0, uses=0,
     fired_from=unreal.TakeCameraId.B)
cart = spawn(unreal.CartDevice, (CART_DOOR[0], CART_DOOR[1], 0), yaw=180.0, label="Cart")
setp(cart, device_name="CART", effect=unreal.DeviceEffect.LURE, range=1200.0, duration=6.0, uses=1,
     fired_from=unreal.TakeCameraId.B, crash_offset=unreal.Vector(CART_ROLL, 0, 0), roll_seconds=1.0)
banner = spawn(unreal.BannerDevice, BATTEN, label="Banner")
setp(banner, device_name="BANNER", effect=unreal.DeviceEffect.MASK, range=600.0, duration=10.0, uses=1,
     fired_from=unreal.TakeCameraId.B, drop_distance=300.0,
     effect_offset=unreal.Vector(ROPE_X - BATTEN[0], GAP - BATTEN[1], 20.0 - BATTEN[2]))

# Cameras: the film's two angles and Maya's catwalk.
cam_door = camera("CamA_Door", unreal.TakeCameraId.A, "A_DOOR", CAM_A_DOOR, initial=True)
cam_rope = camera("CamA_Rope", unreal.TakeCameraId.A, "A_ROPE", CAM_A_ROPE)
camera("CamB_Catwalk", unreal.TakeCameraId.B, "B_CATWALK", CAM_B, fov=CAM_B_FOV)

# The film's zone trigger: which angle camera A shows depends on where Julian is.
z = spawn(unreal.ShotZone, ((X0 + ZONE_SPLIT_X) / 2, 0, 150), label="ShotZone_Door")
setp(z, extent=unreal.Vector((ZONE_SPLIT_X - X0) / 2, (Y1 - Y0) / 2, 300), shot=cam_door, shot_name="A_DOOR")
z = spawn(unreal.ShotZone, ((ZONE_SPLIT_X + X1) / 2, 0, 150), label="ShotZone_Rope")
setp(z, extent=unreal.Vector((X1 - ZONE_SPLIT_X) / 2, (Y1 - Y0) / 2, 300), shot=cam_rope, shot_name="A_ROPE")

# The level's game mode.
world = ues.get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", unreal.TakeGameMode)

les.save_current_level()
origin, extent = nav.get_actor_bounds(False)
print("TESTBOX built: %d actors; nav bounds extent %s; world %s" % (
    len(eas.get_all_level_actors()), extent, world.get_path_name()))
