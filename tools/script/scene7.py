# Scene 7 (docs/SCRIPT.md, docs/script/s7.md). Owner: s7-route (was s7-booth). A tagged, re-runnable overlay on the
# built level (tag script_s7), run under the editor lock after the existing pipeline:
#     "<engine python>" -I tools/ue_mcp.py py tools/script/scene7.py 110
# The booth is the projection box over the lobby doors (build_auction.py BOOTH: x -1000..-760, y -200..800, floor
# z 450). This overlay extends it east to x -600 under the catwalk head's grating, so it is a 4 m deep room:
#   shell    floor, ceiling, the new front wall with the viewing glass's opening (frame) and the projector's port, the
#            north wall with Pendleton's door, the south wall with the stair door, and the projectionist's stair: an
#            enclosed flight from the south landing down to the hall floor (foot door at y -1140), lit inside
#   cut      the hall's south wall opened at Scene 5's oak door (x 3580..3720) so Vance walks through it into the
#            backstage corridor (the pieces it replaces are hidden and lose collision; s5's slab is tagged s7_oak_slab)
#   drapes   crimson velvet under the booth's front: a pelmet and two legs to the hall floor (Pendleton lands below)
#   dress    a work bench, a film shelf, cans and spare reels, a fire bucket, a notice
#   hide     tags the old box's stairs, landing, front wall and ports "s7_hide": AScene7Booth hides them while Scene 7
#            runs (nothing is hidden in the editor or in any other scene)
#   scene    AScene7Route (part 0: the door and the walk) and AScene7Booth (part 2, tag Part=2) and their StartSpots
#   shots    PNGs of the set into C:\Jam\Builds\script\s7
# Switches: S7_PARTS (tuple of the names above) runs only those; S7_OFF = True only clears.
import sys, importlib, math, os, re, traceback
sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C
importlib.reload(C)
import unreal

TAG = "script_s7"
HIDE_TAG = "s7_hide"
PARTS = globals().get("S7_PARTS") or ("shell", "drapes", "dress", "hide", "cut", "scene", "shots")
CUT_TAG = "s7_cut"                  # a level piece this overlay hid to open the oak door's wall
SLAB_TAG = "s7_oak_slab"            # Scene 5's closed door slab: AScene7Route hides it while the door swings
OAK = (3580.0, 3720.0, 260.0)       # the oak door's opening: x0, x1, height (scene5_dress.py DX +- DW/2, DH)

OFF = bool(globals().get("S7_OFF"))
eal = unreal.EditorAssetLibrary
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
SHOTS = r"C:\Jam\Builds\script\s7"
CUBE = "/Engine/BasicShapes/Cube"
CYL = "/Engine/BasicShapes/Cylinder"
M = "/Game/Take/Materials/"
MAT = dict(floor=M + "MI_Box_Deck", wall=M + "MI_Box_Wall", trim=M + "MI_Box_Trim", red=M + "MI_Box_Red",
           glass=M + "MI_Box_Window", door=M + "MI_Box_Door", gold=M + "MI_Box_Gold", velvet=M + "MI_Box_Rope",
           glow=M + "MI_Box_Glow", steel="/Game/Take/Dress/MI_Back_Steel", tube="/Game/Take/Dress/MI_Back_Tube")
REPORT = []

# The booth (world). ORIGIN is where AScene7Booth stands: booth space = world - ORIGIN, +X east to the glass.
X0, XOLD, X1 = -1000.0, -760.0, -600.0
Y0, Y1 = -200.0, 800.0
F = 450.0                         # floor top
CEIL = 870.0                      # under the catwalk head's grating (deck z 880..900)
T = 20.0
ORIGIN = (-800.0, 300.0, F)
WIN_Y = (320.0, 580.0)            # the viewing glass (world y), booth space 20..280
WIN_Z = (F + 110.0, F + 310.0)
PORT = (150.0, F + 150.0)         # the projector's port (world y, z)
DOOR_X = (-740.0, -640.0)         # both doors, world x (booth space 60..160)
DOOR_TOP = F + 220.0


def mat(k):
    return eal.load_asset(MAT[k])


def box(label, x0, x1, y0, y1, z0, z1, m, collide=True, shadow=True):
    """A cube spanning the box (world), tagged."""
    a = C.spawn(CUBE, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), 0.0,
                (abs(x1 - x0) / 100.0, abs(y1 - y0) / 100.0, abs(z1 - z0) / 100.0),
                tag=TAG, label="S7_" + label, folder="Script/S7", collide=collide, shadow=shadow, mat=mat(m))
    return a


def prop(name, loc, yaw=0.0, scale=1.0, label=None):
    path = "/Game/Take/Props/%s/SM_%s" % (name, name)
    if not eal.does_asset_exist(path):
        REPORT.append("no prop " + name)
        return None
    return C.spawn(path, loc, yaw, scale, tag=TAG, label="S7_" + (label or name), folder="Script/S7", collide=True)


def shell():
    n0 = len(eas.get_all_level_actors())
    box("Floor", XOLD - 20.0, X1, Y0 - T, Y1 + T, F - 20.0, F, "floor")
    box("Ceiling", X0, X1, Y0 - T, Y1 + T, CEIL, CEIL + 10.0, "trim", collide=False)
    # The header over the old box's front wall (hidden in Scene 7), so the room is closed above the new ceiling line.
    box("Header", XOLD - 30.0, XOLD - 20.0, Y0, Y1, CEIL, 1150.0, "wall", collide=False)
    # The front wall, x X1-T..X1, around the glass and the port.
    fx0, fx1 = X1 - T, X1
    box("Front_Low", fx0, fx1, Y0 - T, Y1 + T, F, WIN_Z[0], "wall")
    box("Front_High", fx0, fx1, Y0 - T, Y1 + T, WIN_Z[1], CEIL, "wall")
    box("Front_South", fx0, fx1, Y0 - T, WIN_Y[0], WIN_Z[0], WIN_Z[1], "wall")
    box("Front_North", fx0, fx1, WIN_Y[1], Y1 + T, WIN_Z[0], WIN_Z[1], "wall")
    # The glass's frame (the pane itself is AScene7Booth's Glass, so it can break): sill, head, jambs, a mullion.
    for (lbl, a0, a1, z0, z1) in (("Sill", WIN_Y[0] - 8, WIN_Y[1] + 8, WIN_Z[0] - 8, WIN_Z[0]),
                                  ("Head", WIN_Y[0] - 8, WIN_Y[1] + 8, WIN_Z[1], WIN_Z[1] + 8),
                                  ("JambS", WIN_Y[0] - 8, WIN_Y[0], WIN_Z[0], WIN_Z[1]),
                                  ("JambN", WIN_Y[1], WIN_Y[1] + 8, WIN_Z[0], WIN_Z[1])):
        box("Frame_" + lbl, fx0 - 6.0, fx1 + 6.0, a0, a1, z0, z1, "trim", collide=False)
    box("Sill_Ledge", fx0 - 20.0, fx0, WIN_Y[0] - 8, WIN_Y[1] + 8, WIN_Z[0] - 4.0, WIN_Z[0], "trim", collide=False)
    # The projector's port: a small dark glass in the wall's hall face, a hood over it.
    box("Port", fx1, fx1 + 2.0, PORT[0] - 22.0, PORT[0] + 22.0, PORT[1] - 16.0, PORT[1] + 16.0, "glass", collide=False)
    box("Port_Hood", fx1, fx1 + 14.0, PORT[0] - 28.0, PORT[0] + 28.0, PORT[1] + 16.0, PORT[1] + 20.0, "trim", collide=False)
    # North and south walls of the extension, each with a door opening at DOOR_X.
    for (side, y0, y1) in (("North", Y1, Y1 + T), ("South", Y0 - T, Y0)):
        box("Wall%s_W" % side, XOLD - 20.0, DOOR_X[0], y0, y1, F, CEIL, "wall")
        box("Wall%s_E" % side, DOOR_X[1], X1 - T, y0, y1, F, CEIL, "wall")
        box("Wall%s_Lintel" % side, DOOR_X[0], DOOR_X[1], y0, y1, DOOR_TOP, CEIL, "wall")
        for jx in (DOOR_X[0], DOOR_X[1]):
            box("Wall%s_Casing" % side, jx - 4.0, jx + 4.0, y0 - 4.0, y1 + 4.0, F, DOOR_TOP + 6.0, "trim", collide=False)
    # Outside the north door: a dark landing. Outside the south door: the stairwell's landing and steps down.
    box("Landing_North", DOOR_X[0] - 20.0, X1, Y1 + T, Y1 + 220.0, F - 20.0, F, "floor")
    box("Landing_North_Wall", DOOR_X[0] - 20.0, X1, Y1 + 220.0, Y1 + 240.0, F - 20.0, CEIL, "wall")
    box("Landing_South", DOOR_X[0] - 20.0, X1, Y0 - 200.0, Y0 - T, F - 20.0, F, "floor")
    # The projectionist's stair (s7-route): 25 risers of 18 cm, 28 cm treads, south from the landing to the hall
    # floor, each step a solid block down to the floor; walls both sides, a roof, a door at the foot facing south.
    sx0, sx1 = DOOR_X[0] - 20.0, X1                  # -760 .. -600
    n, rise, run = 25, F / 25.0, 28.0
    top_y = Y0 - 200.0                               # -400
    for i in range(n - 1):
        y1 = top_y - run * i
        box("Stair_South", sx0, sx1, y1 - run, y1, 0.0, F - rise * (i + 1), "floor")
    foot = Y0 - 940.0                                # -1140: the foot wall's inner face
    for (lbl, a0, a1) in (("W", sx0 - T, sx0), ("E", sx1, sx1 + T)):
        box("Stair_Wall_" + lbl, a0, a1, foot - T, Y0 - T, 0.0, CEIL, "wall")
    box("Stair_Foot_W", sx0 - T, sx0 + 20.0, foot - T, foot, 0.0, CEIL, "wall")
    box("Stair_Foot_E", sx1 - 20.0, sx1 + T, foot - T, foot, 0.0, CEIL, "wall")
    box("Stair_Foot_Lintel", sx0 + 20.0, sx1 - 20.0, foot - T, foot, 230.0, CEIL, "wall")
    for jx in (sx0 + 20.0, sx1 - 20.0):
        box("Stair_Foot_Casing", jx - 4.0, jx + 4.0, foot - T - 4.0, foot + 4.0, 0.0, 236.0, "trim", collide=False)
    box("Stair_Roof", sx0 - T, sx1 + T, foot - T, Y0 - T, CEIL, CEIL + 10.0, "trim", collide=False)
    box("Sign_Projection", sx0 + 30.0, sx1 - 30.0, foot - T - 3.0, foot - T, 245.0, 270.0, "gold", collide=False)
    for (ly, lz) in ((foot + 150.0, 300.0), (Y0 - 420.0, F + 230.0)):
        lt = eas.spawn_actor_from_class(unreal.PointLight, unreal.Vector((sx0 + sx1) / 2, ly, lz), unreal.Rotator(0, 0, 0))
        lt.set_actor_label("S7_Stair_Lamp")
        lt.set_folder_path("Script/S7")
        C._set_tags(lt, [TAG])
        lc = lt.get_component_by_class(unreal.PointLightComponent)
        lc.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
        lc.set_editor_property("intensity", 9.0)
        lc.set_editor_property("attenuation_radius", 520.0)
        lc.set_editor_property("light_color", unreal.Color(r=255, g=190, b=130, a=255))
    # The mag-lock Cole overrode in Scene 6 (matches s6's facade): a steel box west of the south door with a lit lamp,
    # a steel bar across the door's head and a conduit up to the ceiling.
    box("MagLock", DOOR_X[0] - 54.0, DOOR_X[0] - 10.0, Y0, Y0 + 8.0, F + 98.0, F + 162.0, "steel", collide=False)
    box("MagLock_Lamp", DOOR_X[0] - 38.0, DOOR_X[0] - 26.0, Y0 + 8.0, Y0 + 11.0, F + 140.0, F + 152.0, "glow", collide=False)
    box("MagLock_Bar", DOOR_X[0], DOOR_X[1], Y0, Y0 + 10.0, DOOR_TOP - 12.0, DOOR_TOP, "tube", collide=False)
    box("MagLock_Conduit", DOOR_X[0] - 36.0, DOOR_X[0] - 30.0, Y0, Y0 + 6.0, F + 162.0, CEIL, "tube", collide=False)
    # A red EXIT-style stair sign over the south door (inside).
    box("Sign_Stair", DOOR_X[0] + 25.0, DOOR_X[1] - 25.0, Y0 + 2.0, Y0 + 6.0, DOOR_TOP + 15.0, DOOR_TOP + 40.0, "red", collide=False)
    return "shell: %d actors" % (len(eas.get_all_level_actors()) - n0)


def drapes():
    n = 0
    # A pelmet under the booth's front edge and two velvet legs down to the hall floor.
    box("Pelmet", X1, X1 + 25.0, Y0 - 30.0, Y1 + 30.0, F - 90.0, F - 20.0, "velvet", collide=False)
    for (y0, y1) in ((Y0 - 30.0, Y0 + 70.0), (Y1 - 70.0, Y1 + 30.0)):
        box("Drape", X1, X1 + 30.0, y0, y1, 0.0, F - 90.0, "velvet", collide=False)
        n += 1
    return "drapes: pelmet + %d legs" % n


def dress():
    out = []
    ox, oy, oz = ORIGIN
    # The bench by the projector, where the canister and the dummy reel lie (AScene7Booth: Canister (-70,-260,90),
    # DummyReel (-70,-330,92) in booth space): its top at z F+88.
    box("Bench", ox - 120.0, ox - 20.0, oy - 380.0, oy - 200.0, F + 80.0, F + 88.0, "trim")
    box("Bench_Leg", ox - 115.0, ox - 25.0, oy - 375.0, oy - 205.0, F, F + 80.0, "wall")
    # A film shelf on the back wall and spare reels and cans on it.
    a = prop("Rooms_filmshelf", (X0 + 30.0, oy + 250.0, F), 90.0, 1.0)
    out.append("shelf" if a else "no shelf")
    for i, (dy, dz) in enumerate(((-60, 95), (0, 95), (60, 95), (-30, 150), (40, 150))):
        r = prop("Reel", (X0 + 30.0, oy + 60.0 + dy, F + dz), 90.0, 0.55, label="SpareReel")
    # Cans stacked on the floor by the shelf.
    for i in range(4):
        C.spawn(CYL, (X0 + 45.0, oy + 380.0, F + 4.0 + 8.5 * i), 0.0, (0.42, 0.42, 0.08), tag=TAG,
                label="S7_Can", folder="Script/S7", collide=False, mat=mat("trim"))
    # A fire bucket on the north wall, a notice by the alarm.
    C.spawn(CYL, (ox - 160.0, Y1 - 25.0, F + 15.0), 0.0, (0.3, 0.3, 0.3), tag=TAG, label="S7_FireBucket",
            folder="Script/S7", collide=False, mat=mat("red"))
    box("Notice", ox - 60.0, ox - 20.0, Y1 - 3.0, Y1, F + 120.0, F + 175.0, "gold", collide=False)
    return "dress: " + ", ".join(out)


def hide():
    # The old box's insides, and Scene 5's Pendleton ("Vance" PatrolGuard) and his bodyguard at the podium with their
    # attached CastMembers: Scene 7 plays both as its own puppets.
    rx = re.compile(r"^(Booth_(Stair|Landing|Front|Port)(_\d+)?|Vance|Bodyguard)$")
    n = 0
    for a in eas.get_all_level_actors():
        if rx.match(a.get_actor_label()):
            for b in [a] + list(a.get_attached_actors()):
                ts = C.tags(b)
                if HIDE_TAG not in ts:
                    C._set_tags(b, ts + [HIDE_TAG])
                n += 1
    gone = 0
    for a in eas.get_all_level_actors():
        if re.match(r"^Booth_(Stair|Landing|Front|Port|Top)(_\d+)?$", a.get_actor_label()):
            a.set_actor_hidden_in_game(True)
            a.set_actor_enable_collision(False)
            a.set_is_temporarily_hidden_in_editor(True)
            gone += 1
    return "hide: %d actors tagged %s; %d old-box pieces hidden for good" % (n, HIDE_TAG, gone)


def cut():
    """Open the hall's south wall at the oak door: hide the wall piece(s) there (no collision) and rebuild them around
    the opening with the same material. Also tags Scene 5's slab and clears anything standing in the doorway."""
    x0d, x1d, hd = OAK
    out = []
    for a in eas.get_all_level_actors():            # undo an earlier run first
        if CUT_TAG in C.tags(a):
            a.set_actor_hidden_in_game(False)
            a.set_actor_enable_collision(True)
            a.set_is_temporarily_hidden_in_editor(False)
    for a in eas.get_all_level_actors():
        lbl = a.get_actor_label()
        if lbl.startswith("S7_") or lbl.startswith("Set_S7_"):
            continue
        o, e = a.get_actor_bounds(False)
        bx = (o.x - e.x, o.x + e.x, o.y - e.y, o.y + e.y, o.z - e.z, o.z + e.z)
        in_door = bx[0] < x1d - 5 and bx[1] > x0d + 5 and bx[2] < -1990 and bx[3] > -2060 and bx[4] < hd - 10 and bx[5] > 10
        # The slab and everything mounted on it (panels, plates, the lever): they go when the door swings.
        on_slab = any(k in lbl for k in ("Door_Slab", "Door_Panel", "Door_Moulding", "Door_KickPlate", "Door_PushPlate",
                                         "Door_Escutcheon", "Door_Rose", "Door_Lever", "Door_Keyhole"))
        if on_slab and in_door:
            C._set_tags(a, list(set(C.tags(a) + [SLAB_TAG])))
            out.append("slab " + lbl)
            continue
        if not in_door or not isinstance(a, unreal.StaticMeshActor):
            continue
        if not (lbl.startswith("Hall_South") or lbl.startswith("Dress_Batten") or lbl.startswith("Dress_")):
            out.append("in the doorway, left: " + lbl)
            continue
        smc = a.static_mesh_component
        m = smc.get_material(0)
        a.set_actor_hidden_in_game(True)
        a.set_actor_enable_collision(False)
        a.set_is_temporarily_hidden_in_editor(True)
        C._set_tags(a, list(set(C.tags(a) + [CUT_TAG])))
        out.append("cut " + lbl)
        if not lbl.startswith("Hall_South"):
            continue
        y0, y1, z0, z1 = bx[2], bx[3], bx[4], bx[5]
        pieces = [(bx[0], x0d, z0, z1), (x1d, bx[1], z0, z1), (x0d, x1d, max(z0, hd), z1)]
        for (a0, a1, b0, b1) in pieces:
            if a1 - a0 < 1.0 or b1 - b0 < 1.0:
                continue
            w = C.spawn(CUBE, ((a0 + a1) / 2, (y0 + y1) / 2, (b0 + b1) / 2), 0.0,
                        ((a1 - a0) / 100.0, (y1 - y0) / 100.0, (b1 - b0) / 100.0),
                        tag=TAG, label="S7_OakWall", folder="Script/S7", collide=True, mat=m)
    # The corridor side of the opening: a plain steel-trimmed frame, so the cut reads as a door from backstage too.
    for jx in (x0d, x1d):
        box("Oak_Back_Jamb", jx - 6.0, jx + 6.0, -2032.0, -2018.0, 0.0, hd + 8.0, "trim", collide=False)
    box("Oak_Back_Head", x0d - 6.0, x1d + 6.0, -2032.0, -2018.0, hd, hd + 12.0, "trim", collide=False)
    # The stair's approach (in front of the foot door): hall dressing standing there (a guest, a cocktail table) moves
    # 2.6 m east, out of the way. Idempotent: once moved they are outside the box.
    for a in eas.get_all_level_actors():
        lbl = a.get_actor_label()
        if not re.match(r"^(Guest|Cocktail)_", lbl):
            continue
        o, e = a.get_actor_bounds(False)
        if o.x - e.x < -560.0 and o.x + e.x > -800.0 and o.y - e.y < -1140.0 and o.y + e.y > -1440.0 and o.z - e.z < 200.0:
            a.set_actor_location(a.get_actor_location() + unreal.Vector(260.0, 0.0, 0.0), False, False)
            out.append("moved " + lbl)
    return "cut: " + "; ".join(out)


def scene():
    if not hasattr(unreal, "Scene7Booth"):
        return "scene: AScene7Booth not in this editor build yet (skipped)"
    ox, oy, oz = ORIGIN
    # The projector mark = s7-reelswap's use spot at its projector (-745, 166), facing it: part 0 walks him onto it;
    # a cold Jump 702 lands there.
    sp = eas.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(ox + 55.0, oy - 134.0, oz + 95.0),
                                    unreal.Rotator(0.0, 0.0, 52.0))
    sp.set_actor_label("S7_StartSpot")
    sp.set_folder_path("Script/S7")
    C._set_tags(sp, [TAG])
    b = eas.spawn_actor_from_class(unreal.Scene7Booth, unreal.Vector(ox, oy, oz), unreal.Rotator(0.0, 0.0, 0.0))
    b.set_actor_label("S7_Booth")
    b.set_folder_path("Script/S7")
    C._set_tags(b, [TAG, "Part=2"])
    b.set_editor_property("start_spot", sp)
    b.set_editor_property("slug", unreal.Text("INT. PROJECTION BOOTH - CONTINUOUS"))
    REPORT.append(route())
    # Fit the projector model to ~175 cm tall, its back on the booth's old floor line.
    pc = b.get_editor_property("projector")
    sm = pc.get_editor_property("static_mesh")
    if sm:
        bb = sm.get_bounding_box()
        h = max(1.0, bb.max.z - bb.min.z)
        s = 175.0 / h
        pc.set_relative_scale3d(unreal.Vector(s, s, s))
        pc.set_relative_location(unreal.Vector(40.0, -150.0, -bb.min.z * s), False, False)
        REPORT.append("projector %.0fx%.0fx%.0f cm scaled %.2f" % ((bb.max.x - bb.min.x) * s, (bb.max.y - bb.min.y) * s, h * s, s))
    for k in ("master_reel", "dummy_reel", "lower_reel"):
        c = b.get_editor_property(k)
        m = c.get_editor_property("static_mesh")
        if m:
            bb = m.get_bounding_box()
            d = max(bb.max.x - bb.min.x, bb.max.y - bb.min.y, bb.max.z - bb.min.z)
            s = 70.0 / max(1.0, d)
            c.set_relative_scale3d(unreal.Vector(s, s, s))
            if k == "master_reel":
                REPORT.append("reel %.0f %.0f %.0f -> scale %.2f" % (bb.max.x - bb.min.x, bb.max.y - bb.min.y, bb.max.z - bb.min.z, s))
    REPORT.append(tune(b))
    return "scene: S7_Booth placed"


def route():
    """AScene7Route (part 0): Vance on Scene 5's door mark facing the door; the leaf in s5's oak."""
    if not hasattr(unreal, "Scene7Route"):
        return "route: AScene7Route not in this editor build yet (skipped)"
    sp = eas.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(3650.0, -1900.0, 95.0), unreal.Rotator(0.0, 0.0, -90.0))
    sp.set_actor_label("S7_Route_StartSpot")
    sp.set_folder_path("Script/S7")
    C._set_tags(sp, [TAG])
    r = eas.spawn_actor_from_class(unreal.Scene7Route, unreal.Vector(3650.0, -2010.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0))
    r.set_actor_label("S7_Route")
    r.set_folder_path("Script/S7")
    C._set_tags(r, [TAG])
    r.set_editor_property("start_spot", sp)
    r.set_editor_property("arrive_radius", 220.0)     # anywhere in the booth past the door: the swap's bench blocks the mark
    oak = None
    for a in eas.get_all_level_actors():
        if SLAB_TAG in C.tags(a) and isinstance(a, unreal.StaticMeshActor) and "Door_Slab" in a.get_actor_label():
            oak = a.static_mesh_component.get_material(0)
    if oak:
        r.set_editor_property("door_material", oak)
    return "route: S7_Route placed (door material %s)" % (oak.get_name() if oak else "default")


# Booth-space camera moves found in the PIE shots (from, to, fov), and the cast's Mixamo clips swapped for their
# SK_Mannequin copies in /Game/Take/Anim/Mocap (the YBot-skeleton clips don't play on the cast meshes).
SHOT_TUNE = {
    "Swap": ((-30, -340, 210), (-30, -160, 185), 45),
    "Line": ((-140, -10, 175), (60, 300, 165), 35),
    "Screen": ((700, 100, 120), (5188, -300, 150), 30),
    "ParryHit": ((160, 60, 140), (10, -60, 140), 50),
    "Cole": ((170, -90, 175), (110, -450, 120), 55),
}
MOCAP = ("Use", "Lift", "Refuse", "Talk", "Stand")


def tune(b):
    shots = b.get_editor_property("shots")
    for k, (f, t, fov) in SHOT_TUNE.items():
        sh = shots[k]
        fv, tv = unreal.Vector(*f), unreal.Vector(*t)
        sh.set_editor_property("location", fv)
        sh.set_editor_property("rotation", unreal.MathLibrary.find_look_at_rotation(fv, tv))
        sh.set_editor_property("fov", float(fov))
        shots[k] = sh
    b.set_editor_property("shots", shots)
    anims = b.get_editor_property("anims")
    swapped = []
    for k in MOCAP:
        a = anims.get(k) if hasattr(anims, "get") else anims[k]
        if a is None:
            continue
        m = unreal.load_asset("/Game/Take/Anim/Mocap/" + a.get_name())
        if m:
            anims[k] = m
            swapped.append(k)
    b.set_editor_property("anims", anims)
    sm = smoke_material()
    if sm:
        b.set_editor_property("smoke_material", sm)
    return "tune: %d shots, mocap %s, smoke %s" % (len(SHOT_TUNE), swapped, bool(sm))


def smoke_material():
    """M_S7_Smoke: unlit translucent grey, soft at the edges (fresnel), for Cole's smoke puffs."""
    path = "/Game/Take/Script/S7/M_S7_Smoke"
    if eal.does_asset_exist(path):
        return unreal.load_asset(path)
    mel = unreal.MaterialEditingLibrary
    m = unreal.AssetToolsHelpers.get_asset_tools().create_asset("M_S7_Smoke", "/Game/Take/Script/S7", unreal.Material,
                                                                unreal.MaterialFactoryNew())
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    col = mel.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -400, 0)
    col.set_editor_property("constant", unreal.LinearColor(0.33, 0.32, 0.31, 1.0))
    mel.connect_material_property(col, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    fr = mel.create_material_expression(m, unreal.MaterialExpressionFresnel, -600, 200)
    fr.set_editor_property("exponent", 1.5)
    inv = mel.create_material_expression(m, unreal.MaterialExpressionOneMinus, -400, 200)
    mel.connect_material_expressions(fr, "", inv, "")
    mul = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -200, 200)
    mul.set_editor_property("const_b", 0.55)
    mel.connect_material_expressions(inv, "", mul, "A")
    mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    return m


def shots():
    os.makedirs(SHOTS, exist_ok=True)
    ox, oy, oz = ORIGIN
    out = []
    out.append(C.shot("s7_set_inside", (ox - 170.0, oy - 420.0, oz + 170.0), (ox + 120.0, oy + 150.0, oz + 120.0), 70.0, out_dir=SHOTS))
    out.append(C.shot("s7_set_glass", (ox - 150.0, oy + 100.0, oz + 160.0), (ox + 400.0, oy + 150.0, oz + 60.0), 70.0, out_dir=SHOTS))
    out.append(C.shot("s7_set_hall", (ox + 900.0, oy + 200.0, 170.0), (ox + 200.0, oy + 150.0, oz + 150.0), 60.0, out_dir=SHOTS))
    return "shots: " + ", ".join(os.path.basename(p) for p in out)


def run():
    # s7-reelswap's placed swap actor also carries our tag: keep it (drop the tag) so the clear doesn't destroy it.
    for a in eas.get_all_level_actors():
        ts = C.tags(a)
        if "s7_swap" in ts and TAG in ts:
            C._set_tags(a, [t for t in ts if t != TAG])
    gone, shown = C.clear(TAG)
    REPORT.append("cleared %d" % gone)
    if OFF:
        return
    for name in PARTS:
        try:
            REPORT.append(globals()[name]())
        except Exception:                                   # noqa: BLE001
            REPORT.append("%s FAILED: %s" % (name, traceback.format_exc().splitlines()[-1]))


run()
print("\n".join(str(r) for r in REPORT))
