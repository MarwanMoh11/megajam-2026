# Scene 7, INT. PROJECTION BOOTH (docs/SCRIPT.md). Owner: the s7-booth session. A tagged, re-runnable overlay on the
# built level (tag script_s7), run under the editor lock after the existing pipeline:
#     "<engine python>" -I tools/ue_mcp.py py tools/script/scene7.py 110
# The booth is the projection box over the lobby doors (build_auction.py BOOTH: x -1000..-760, y -200..800, floor
# z 450). This overlay extends it east to x -600 under the catwalk head's grating, so it is a 4 m deep room:
#   shell    floor, ceiling, the new front wall with the viewing glass's opening (frame) and the projector's port, the
#            north wall with Pendleton's door, the south wall with the stairwell door (Cole), landings and steps down
#   drapes   crimson velvet under the booth's front: a pelmet and two legs to the hall floor (Pendleton lands below)
#   dress    a work bench, a film shelf, cans and spare reels, a fire bucket, a notice
#   hide     tags the old box's stairs, landing, front wall and ports "s7_hide": AScene7Booth hides them while Scene 7
#            runs (nothing is hidden in the editor or in any other scene)
#   scene    the AScene7Booth actor (needs the editor built with Scene7Booth) and its StartSpot
#   shots    PNGs of the set into C:\Jam\Builds\script\s7
# Switches: S7_PARTS (tuple of the names above) runs only those; S7_OFF = True only clears.
import sys, importlib, math, os, re, traceback
sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C
importlib.reload(C)
import unreal

TAG = "script_s7"
HIDE_TAG = "s7_hide"
PARTS = globals().get("S7_PARTS") or ("shell", "drapes", "dress", "hide", "scene", "shots")
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
    for i in range(6):
        y1 = Y0 - 200.0 - 30.0 * i
        box("Stair_South", DOOR_X[0] - 20.0, X1, y1 - 30.0, y1, F - 20.0 - 25.0 * (i + 1), F - 25.0 * (i + 1), "floor")
    box("Stair_South_Wall", DOOR_X[0] - 40.0, DOOR_X[0] - 20.0, Y0 - 400.0, Y0 - T, F - 180.0, CEIL, "wall")
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
    return "hide: %d actors tagged %s" % (n, HIDE_TAG)


def scene():
    if not hasattr(unreal, "Scene7Booth"):
        return "scene: AScene7Booth not in this editor build yet (skipped)"
    ox, oy, oz = ORIGIN
    sp = eas.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(ox + 110.0, oy - 400.0, oz + 95.0),
                                    unreal.Rotator(0.0, 0.0, 124.0))
    sp.set_actor_label("S7_StartSpot")
    sp.set_folder_path("Script/S7")
    C._set_tags(sp, [TAG])
    b = eas.spawn_actor_from_class(unreal.Scene7Booth, unreal.Vector(ox, oy, oz), unreal.Rotator(0.0, 0.0, 0.0))
    b.set_actor_label("S7_Booth")
    b.set_folder_path("Script/S7")
    C._set_tags(b, [TAG])
    b.set_editor_property("start_spot", sp)
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
