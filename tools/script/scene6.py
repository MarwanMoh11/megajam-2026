# Scene 6, INT. BACKSTAGE CORRIDOR & RIGGING (Cole): the set overlay. Owner: the s6-backstage session.
# Run under the editor lock:  "$PY" -I tools/ue_mcp.py py tools/script/scene6.py 110
# Re-runnable: clear(TAG) removes what the last run placed and shows what it hid.
# The vault room behind the hall's screen (x 4420..5300, y -1000..1000, z 0..500) becomes backstage: the back of the
# screen (AScene6Backstage's own plane, film mirrored), crates where Cole starts, a wardrobe, the costume trunk, the
# counterweight lever and its cable to the sandbag over the thunder sheet (east wall), the door-control box (north
# wall: dressing), and outside the room the oak door's scanner (corridor side) and the prop store's door (the escape's way out).
# QA Nov 15 evening: one guard; the stairs, the landing and the booth door are gone.
# The moving pieces are engine cubes tagged s6_* that the scene actor finds and turns.
# Cover that hides Cole from the enforcers blocks the Visibility channel (their sight traces); nothing blocks the camera.
import sys
import importlib
import math
import re
import unreal
sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C
importlib.reload(C)

TAG = "script_s6"
OUT = r"C:\Jam\Builds\script\s6"
SHOTS = globals().get("S6_SHOTS", True)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary
Ch, R = unreal.CollisionChannel, unreal.CollisionResponseType
LOG = []

D = "/Game/Take/Dress/"
M = {"wood": D + "MI_Front_Panel", "pewter": D + "MI_Front_Trim", "ply": D + "MI_Back_Ply",
     "batten": D + "MI_Back_Batten", "steel": D + "MI_Back_Steel", "tape": D + "MI_Back_Tape", "black": D + "MI_Back_Cable",
     "canvas": D + "MI_Canvas_Light", "canvas_dk": D + "MI_Canvas_Dark", "concrete": D + "MI_Back_Concrete",
     "tube": D + "MI_Back_Tube"}
for _k in list(M):
    if not eal.does_asset_exist(M[_k]):
        LOG.append("no material " + M[_k])
        M[_k] = D + "MI_Front_Trim"
CUBE, CYL = "/Engine/BasicShapes/Cube", "/Engine/BasicShapes/Cylinder"


def prop_mesh(n):
    return "/Game/Take/Props/%s/SM_%s" % (n, n)


def sight(a, block_pawn):
    """Blocks the enforcers' sight traces (Visibility); the pawn too if block_pawn; never the camera."""
    for c in a.get_components_by_class(unreal.PrimitiveComponent):
        c.set_collision_enabled(unreal.CollisionEnabled.QUERY_ONLY)
        c.set_collision_object_type(Ch.ECC_WORLD_STATIC)
        c.set_collision_response_to_all_channels(R.ECR_IGNORE)
        c.set_collision_response_to_channel(Ch.ECC_VISIBILITY, R.ECR_BLOCK)
        if block_pawn:
            c.set_collision_response_to_channel(Ch.ECC_PAWN, R.ECR_BLOCK)


def add_tags(a, extra):
    ts = list(a.tags) + [unreal.Name(t) for t in extra]
    a.set_editor_property("tags", ts)


def box(label, x0, x1, y0, y1, z0, z1, mat, collide=False, see=False, rot=(0.0, 0.0, 0.0), tags=(), mesh=CUBE,
        movable=False):
    """A cube from x0..x1, y0..y1, z0..z1 (world, before rot about its centre)."""
    a = C.spawn(mesh, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), rot,
                (abs(x1 - x0) / 100, abs(y1 - y0) / 100, abs(z1 - z0) / 100), tag=TAG, label="S6_" + label,
                collide=collide, shadow=True, mat=M[mat])
    if see:
        sight(a, collide)
    if tags:
        add_tags(a, tags)
    if movable:
        a.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    return a


def piece(label, centre, size, mat, rot=(0.0, 0.0, 0.0), tags=(), movable=True):
    """A cube centred at centre, size (local x, y, z) cm, turned by rot (roll, pitch, yaw)."""
    a = C.spawn(CUBE, centre, rot, tuple(s / 100.0 for s in size), tag=TAG, label="S6_" + label, collide=False,
                shadow=True, mat=M[mat])
    if tags:
        add_tags(a, tags)
    if movable:
        a.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    return a


def prop(n, bx, yaw, label, collide=True, see=True, mode="fit"):
    m = prop_mesh(n)
    if not eal.does_asset_exist(m):
        LOG.append("no prop " + n)
        return box(label, bx[0], bx[1], bx[2], bx[3], bx[4], bx[5], "ply", collide, see)
    a = C.place(m, bx, yaw=yaw, mode=mode, tag=TAG, label="S6_" + label, collide=collide)
    if see:
        sight(a, collide)
    return a


def light(label, loc, cd, radius, color, tags=()):
    a = eas.spawn_actor_from_class(unreal.PointLight, unreal.Vector(*loc))
    a.set_actor_label("Set_S6_" + label)
    a.set_folder_path("Sets/" + TAG)
    c = a.point_light_component
    c.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    c.set_editor_property("intensity", cd)
    c.set_editor_property("attenuation_radius", radius)
    c.set_editor_property("light_color", unreal.Color(r=color[0], g=color[1], b=color[2], a=255))
    c.set_editor_property("cast_shadows", False)
    a.set_editor_property("tags", [unreal.Name(TAG)] + [unreal.Name(t) for t in tags])
    return a


# --- 0. clear the last run, hide the vault's dressing, move its table out of the way --------------------------------
gone, shown = C.clear(TAG)
for a in C.actors():
    if TAG in C.tags(a) or a.get_class().get_name() == "Scene6Backstage":
        eas.destroy_actor(a)
hide_rx = r"Set_Rooms_vault_.*|Vault_Lot.*|Gold_Bar.*|Vault_Shelf.*|Deposit_Boxes.*"
hidden = C.hide([hide_rx], TAG)


def move(lead, others, x, y):
    """Moves the actors matching lead (and others, by the same step) so lead's bounds centre is at (x, y).
    Idempotent: the step is measured from where lead is now."""
    found = C.find(lead)
    if not found:
        LOG.append("nothing to move: " + lead)
        return
    o, _e = found[0].get_actor_bounds(False)
    step = unreal.Vector(x - o.x, y - o.y, 0.0)
    for a in found + [b for p in others for b in C.find(p)]:
        a.set_actor_location(a.get_actor_location() + step, False, False)


move(r"Vault_Table(_\d+)?", [], 5120, -880)                     # the viewing table becomes the props table, SE corner
move(r"Safe(_\d+)?", [r"Safe_Handle(_\d+)?"], 4930, -820)       # the safe out of the thunder sheet's corner
move(r"Statue_Plinth(_\d+)?", [r"Statue(_\d+)?"], 5200, -250)   # the bust off the fire exit and the lure route

# --- 1. the back of the screen: black masking either side of the scene actor's film plane ----------------------------
box("Mask_S", 4421, 4426, -1000, -500, 0, 500, "canvas_dk")
box("Mask_N", 4421, 4426, 500, 1000, 0, 500, "canvas_dk")
box("Screen_Batten", 4424, 4440, -520, 520, 482, 492, "batten")
for y in (-505.0, 505.0):
    box("Screen_Leg", 4424, 4436, y - 6, y + 6, 0, 490, "batten")

# --- 2. cover: the shipping crates where Cole starts, a wardrobe, crates by the trunk ---------------------------------
# Cole starts south of them, facing north (the camera has the 3 m behind him); A's patrol turns just north of them.
prop("Crate", (4600, 4745, -570, -440, 0, 175), 0, "Crate_Start_A")
prop("Crate", (4745, 4870, -620, -500, 0, 125), 15, "Crate_Start_B")
prop("Crate", (4610, 4720, -440, -380, 0, 80), 0, "Crate_Start_C")
box("Wardrobe", 4436, 4536, -340, -60, 0, 215, "wood", collide=True, see=True)
box("Wardrobe_Top", 4430, 4542, -346, -54, 215, 222, "pewter")
prop("Coatrack", (4440, 4540, -40, 180, 0, 185), 0, "Costume_Rail", collide=True, see=False, mode="stretch")
for i, y in enumerate((-20, 30, 80, 130)):                   # costumes on the rail: hanging canvas
    box("Costume_%d" % i, 4470, 4510, y, y + 36, 60, 170, "canvas" if i % 2 else "canvas_dk", see=True)
prop("Crate", (4450, 4600, 520, 680, 0, 165), 90, "Crate_North_A")
prop("Roadcase", (4600, 4700, 560, 660, 0, 95), 0, "Roadcase_North", see=False)

# --- 3. the costume trunk on the east wall (the lid is the scene's: s6_trunk_lid, hinged on its east edge) -------------
prop("Roadcase", (5196, 5268, 50, 190, 0, 60), 90, "Trunk", see=False, mode="stretch")
piece("Trunk_Lid", (5232, 120, 64), (144, 74, 8), "wood", rot=(0.0, 0.0, -90.0), tags=("s6_trunk_lid",))

# --- 4. the counterweight lever on the west wall, its cable along the ceiling to the sandbag over the thunder sheet
#        (east wall, north end: the guard turns his back on the room to look at it) ------------------------------------
box("Lever_Plate", 4421, 4436, -490, -410, 70, 200, "steel")
piece("Lever_Handle", (4456, -450, 150), (8, 8, 80), "tube", tags=("s6_lever_handle",))
box("Lever_Knob", 4450, 4462, -456, -444, 186, 198, "black")
box("Cable_Up", 4428, 4432, -452, -448, 200, 470, "black")
box("Cable_Run_X", 4428, 5262, -452, -448, 468, 472, "black")
box("Cable_Run_Y", 5258, 5262, -452, 562, 468, 472, "black")
box("Sheet_Batten", 5250, 5270, 470, 650, 475, 485, "batten")
for y in (490.0, 630.0):
    box("Sheet_Rope", 5259, 5261, y - 1, y + 1, 370, 475, "black")
piece("Thunder_Sheet", (5272, 560, 260), (3, 140, 220), "steel", tags=("s6_sheet",))
piece("Sandbag", (5245, 560, 440), (30, 36, 46), "canvas", tags=("s6_sandbag",))
box("Sandbag_Rope", 5244, 5246, 559, 561, 463, 470, "black")
for i, (x, y) in enumerate(((4470, -560), (4505, -575), (4480, -600))):
    box("Floor_Sandbag_%d" % i, x - 18, x + 18, y - 14, y + 14, 0, 26, "canvas")

# --- 5. the door-control box on the north wall (dressing: what the guard stands over) ---------------------------------
box("Panel_Body", 4610, 4710, 975, 991, 100, 200, "steel")
box("Panel_Face", 4612, 4708, 970, 975, 102, 198, "pewter")
for x in (4625.0, 4660.0, 4695.0):
    box("Panel_Conduit", x - 3, x + 3, 982, 988, 200, 480, "black")
box("Panel_Plate", 4630, 4690, 984, 990, 210, 228, "tape")          # DOOR CONTROL - SECURITY ONLY

# --- 6. outside the room (agreed with s7-route and s8-escape, Nov 15 evening) -----------------------------------------
# The oak door's scanner on the corridor side (Scene 5's door, hall south wall, opening x 3580..3720 at y -2020..-2000;
# the door leaf and the wall cut are s7-route's): a box east of the opening behind s5's keypad, its cover the scene
# swings open.
box("Scanner_Body", 3722, 3768, -2030, -2020, 90, 160, "steel")
piece("Scanner_Cover", (3745, -2033, 125), (46, 4, 70), "pewter", tags=("s6_panel_cover",))
box("Scanner_Wires", 3727, 3763, -2034, -2031, 97, 153, "black")      # behind the cover
box("Scanner_Conduit", 3742, 3748, -2028, -2022, 160, 300, "black")
# The prop store's door off the corridor (opening x 1630..1770 in the wall y -2620..-2600; s8-escape's run goes through
# it to the alley fire exit): a leaf hinged on its west jamb, placed shut; the scene keeps it open (88 deg into the prop
# store, flat to the jamb) except during Scene 6. The alarm box east of the frame, the wedge at the open leaf's foot.
leaf = piece("Exit_Leaf", (1700, -2610, 122), (138, 5, 244), "steel", tags=("s6_exit_leaf",))
bar = piece("Exit_Bar", (1700, -2604, 100), (110, 5, 6), "tube")
bar.attach_to_actor(leaf, "", unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD,
                    unreal.AttachmentRule.KEEP_WORLD, False)
box("Exit_Alarm", 1780, 1820, -2600, -2590, 128, 176, "steel")
box("Exit_Alarm_Face", 1785, 1815, -2590, -2587, 134, 170, "tape")
piece("Exit_Wedge", (1648, -2700, 5), (14, 24, 10), "wood", tags=("s6_exit_wedge",))

# --- 7. light: work lights (the film light is the scene actor's) -----------------------------------------------------
light("Work_East", (5120, -80, 440), 55.0, 900.0, (255, 196, 140))
light("Work_Panel", (4720, 900, 450), 35.0, 600.0, (255, 180, 120))
light("Work_Exit", (1760, -2480, 400), 30.0, 500.0, (255, 180, 120))

# --- 8. Cole's mark and the scene actor -------------------------------------------------------------------------------
start = eas.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(4680, -700, 100), unreal.Rotator(0, 0, 90))
start.set_actor_label("Set_S6_StartSpot_Cole")
start.set_folder_path("Sets/" + TAG)
start.set_editor_property("tags", [unreal.Name(TAG)])
scene = None
cls = getattr(unreal, "Scene6Backstage", None)
if cls is not None:
    scene = eas.spawn_actor_from_class(cls, unreal.Vector(4427, 0, 245), unreal.Rotator(0, 0, 0))
    scene.set_actor_label("Set_S6_Scene6Backstage")
    scene.set_folder_path("Sets/" + TAG)
    scene.set_editor_property("tags", [unreal.Name(TAG)])
    scene.set_editor_property("start_spot", start)
    V = unreal.Vector
    for k, v in (("guard_post", V(4850, 800, 0)), ("lever_point", V(4500, -450, 0)), ("sheet_point", V(5270, 560, 0)),
                 ("lookout_point", V(5110, 520, 0)), ("trunk_point", V(5232, 120, 0)),
                 ("panel_point", V(3745, -2034, 125)), ("panel_stand", V(3745, -2115, 0)),
                 ("end_mark", V(3650, -2250, 0)),
                 ("exit_stand", V(1700, -2500, 0)), ("exit_alarm_point", V(1800, -2588, 152))):
        try:
            scene.set_editor_property(k, v)
        except Exception as e:
            LOG.append("no property %s (%s)" % (k, e))
    for k, v in (("guard_yaw", -90.0), ("end_yaw", 90.0), ("exit_open_degrees", -88.0),
                 ("route_to_exit", [V(4850, -1100, 0), V(4500, -2300, 0)])):   # the way out to the corridor
        try:
            scene.set_editor_property(k, v)
        except Exception as e:
            LOG.append("no %s (%s)" % (k, e))
else:
    LOG.append("AScene6Backstage not compiled yet: set placed, no scene actor")

# --- 8. shots ----------------------------------------------------------------------------------------------------------
shots = []
if SHOTS:
    shots.append(C.shot("s6_overview", (4460, -960, 470), (4950, 300, 120), fov=95.0, out_dir=OUT))
    shots.append(C.shot("s6_cole_start", (4600, -980, 170), (4700, 0, 150), fov=80.0, out_dir=OUT))
    shots.append(C.shot("s6_panel_guard", (4750, 300, 260), (4700, 950, 140), fov=80.0, out_dir=OUT))
    shots.append(C.shot("s6_trunk_sheet", (4800, -200, 260), (5250, 350, 100), fov=85.0, out_dir=OUT))
    shots.append(C.shot("s6_exit_propstore", (1850, -2300, 220), (1700, -2610, 100), fov=80.0, out_dir=OUT))
    shots.append(C.shot("s6_scanner", (3820, -2250, 200), (3765, -2025, 140), fov=70.0, out_dir=OUT))
    shots.append(C.shot("s6_plan", (4200, -1800, 2600), (4200, -1790, 0), fov=90.0, out_dir=OUT))

"s6: removed %d, shown %d, hid %d (%s...), scene actor %s, shots %s, log %s" % (
    gone, shown, len(hidden), ", ".join(hidden[:4]), "placed" if scene else "MISSING", shots, LOG)
