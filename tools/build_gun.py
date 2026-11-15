"""The guards' and enforcers' pistol (QA Nov 15 evening, chars-locomotion: "Render proper gun models for them").

Builds /Game/Take/Props/Guns/SM_Pistol from parts with GeometryScript (no download, no licence): a 1911-pattern
service pistol, 21.6 cm long, with a chamfered slide, front and rear sights, a dust cover, a trigger guard and
trigger, a hammer, a beavertail, walnut grip panels, a magazine base, a barrel bushing and a dark bore. Two materials:
M_Gun instances MI_Gun_Steel (blued gunmetal) and MI_Gun_Walnut. Origin at the middle of the grip (where the palm
closes), barrel along +X, top +Z, centimetres: ACastMember::GunGrip puts that origin in hand_r.

It also prints the grip for hand_r measured from the template's pistol aim (MF_Pistol_Idle_ADS, frame 0): the
rotation that points the barrel straight ahead (mesh +Y) with the top up in that pose. ACastMember's default
GunGrip comes from this.

Run inside the editor, under the editor lock:
  "<engine python>" -I tools/ue_mcp.py py tools/build_gun.py 110
"""
import math
import traceback

import unreal

DEST = "/Game/Take/Props/Guns"
NAME = "SM_Pistol"
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
GS = unreal.GeometryScript_Primitives
OUT = []

# The grip's middle in the profile below (the profile is drawn with the slide's underside at z = 2).
GRIP = (-3.4, -3.7)
STEEL, WOOD, DARK = 0, 1, 2


def log(s):
    OUT.append(str(s))
    unreal.log("[build_gun] %s" % s)


def material():
    path = DEST + "/M_Gun"
    if EAL.does_asset_exist(path):
        return EAL.load_asset(path)
    m = AT.create_asset("M_Gun", DEST, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("used_with_static_lighting", True)

    def param(cls, name, x, y, value):
        e = MEL.create_material_expression(m, cls, x, y)
        e.set_editor_property("parameter_name", name)
        e.set_editor_property("default_value", value)
        return e
    col = param(unreal.MaterialExpressionVectorParameter, "Colour", -500, -100, unreal.LinearColor(0.05, 0.05, 0.055, 1))
    met = param(unreal.MaterialExpressionScalarParameter, "Metallic", -500, 50, 1.0)
    rou = param(unreal.MaterialExpressionScalarParameter, "Roughness", -500, 150, 0.35)
    MEL.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(met, "", unreal.MaterialProperty.MP_METALLIC)
    MEL.connect_material_property(rou, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.recompile_material(m)
    EAL.save_asset(path, only_if_is_dirty=False)
    return m


def instance(name, colour, metallic, rough):
    path = "%s/%s" % (DEST, name)
    mi = EAL.load_asset(path) if EAL.does_asset_exist(path) else AT.create_asset(
        name, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, material())
    MEL.set_material_instance_vector_parameter_value(mi, "Colour", unreal.LinearColor(*colour, 1.0))
    MEL.set_material_instance_scalar_parameter_value(mi, "Metallic", metallic)
    MEL.set_material_instance_scalar_parameter_value(mi, "Roughness", rough)
    MEL.update_material_instance(mi)
    EAL.save_asset(path, only_if_is_dirty=False)
    return mi


def opts(mat):
    o = unreal.GeometryScriptPrimitiveOptions()
    o.set_editor_property("material_id", mat)
    o.set_editor_property("polygroup_mode", unreal.GeometryScriptPrimitivePolygroupMode.PER_FACE)
    return o


def xf(x=0.0, y=0.0, z=0.0, pitch=0.0, yaw=0.0, roll=0.0):
    return unreal.Transform(unreal.Vector(x, y, z), unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw), unreal.Vector(1, 1, 1))


# Parts are drawn lying on their side: the profile in X (forward) / Y (up), the thickness along Z. One turn at the end
# stands the gun up (checked from the bounds).
def slab(mesh, mat, pts, half):
    """A profile (x forward, y up) extruded to +-half thickness."""
    verts = [unreal.Vector2D(x - GRIP[0], y - GRIP[1]) for x, y in pts]
    area = sum(verts[i].x * verts[(i + 1) % len(verts)].y - verts[(i + 1) % len(verts)].x * verts[i].y for i in range(len(verts)))
    if area < 0:
        verts.reverse()
    GS.append_simple_extrude_polygon(mesh, opts(mat), xf(z=-half), verts, 2.0 * half, 0, True)


def rod(mesh, mat, x0, x1, y, radius, steps=16):
    """A cylinder along the barrel's axis from x0 to x1 at height y."""
    length = x1 - x0
    GS.append_cylinder(mesh, opts(mat), xf(x=(x0 + x1) * 0.5 - GRIP[0], y=y - GRIP[1], pitch=90.0), radius, length,
                       steps, 0, True, unreal.GeometryScriptPrimitiveOriginMode.CENTER)


def bar(mesh, mat, a, b, thick, half):
    """A straight bar of the trigger guard from a to b (profile coordinates)."""
    dx, dy = b[0] - a[0], b[1] - a[1]
    n = math.hypot(dx, dy) or 1.0
    ox, oy = -dy / n * thick * 0.5, dx / n * thick * 0.5
    # a little longer at both ends so the joints close
    ex, ey = dx / n * thick * 0.4, dy / n * thick * 0.4
    pts = [(a[0] - ex + ox, a[1] - ey + oy), (b[0] + ex + ox, b[1] + ey + oy), (b[0] + ex - ox, b[1] + ey - oy),
           (a[0] - ex - ox, a[1] - ey - oy)]
    slab(mesh, mat, pts, half)


def build_mesh():
    mesh = unreal.DynamicMesh()
    # slide: chamfered top edges, the front a little rounded at the bottom
    slab(mesh, STEEL, [(-5.6, 2.0), (16.0, 2.0), (16.0, 4.55), (15.55, 5.0), (-5.15, 5.0), (-5.6, 4.55)], 1.15)
    # the slide's flat top rib (narrower, a touch higher: the chamfer reads from the side)
    slab(mesh, STEEL, [(-5.0, 4.9), (15.4, 4.9), (15.4, 5.15), (-5.0, 5.15)], 0.7)
    # rear grip serrations on the slide: thin dark grooves
    for i in range(7):
        x = -4.9 + i * 0.42
        slab(mesh, DARK, [(x, 2.3), (x + 0.16, 2.3), (x + 0.16, 4.7), (x, 4.7)], 1.17)
    # ejection port
    slab(mesh, DARK, [(2.5, 3.6), (6.3, 3.6), (6.3, 4.6), (2.5, 4.6)], 1.17)
    # sights
    slab(mesh, STEEL, [(14.6, 5.0), (15.3, 5.0), (15.2, 5.55), (14.75, 5.55)], 0.16)
    slab(mesh, STEEL, [(-4.7, 5.0), (-3.9, 5.0), (-3.9, 5.6), (-4.7, 5.6)], 0.62)
    # frame: dust cover under the slide's front, the receiver behind it
    slab(mesh, STEEL, [(-5.0, 0.6), (9.6, 0.6), (10.6, 1.3), (10.6, 2.02), (-5.0, 2.02)], 1.05)
    # barrel bushing, barrel and bore
    rod(mesh, STEEL, 15.9, 16.55, 3.35, 0.95)
    rod(mesh, DARK, 16.4, 16.62, 3.35, 0.48, 12)
    # recoil spring plug under the barrel
    rod(mesh, STEEL, 15.9, 16.45, 2.2, 0.5, 12)
    # grip frame with the beavertail
    slab(mesh, STEEL, [(1.0, 0.7), (-2.1, -8.0), (-6.7, -8.0), (-4.9, -0.2), (-6.5, 1.35), (-6.2, 2.02), (-3.8, 2.02),
                       (-1.0, 0.7)], 1.2)
    # mainspring housing strip at the back of the grip
    slab(mesh, DARK, [(-4.95, -0.4), (-5.25, -0.4), (-6.95, -7.7), (-6.6, -7.7)], 1.0)
    # walnut panels, proud of the frame on both sides
    slab(mesh, WOOD, [(0.15, -0.6), (-2.55, -7.35), (-6.15, -7.35), (-4.75, -0.6)], 1.62)
    # magazine base
    slab(mesh, STEEL, [(-2.0, -8.0), (-6.8, -8.0), (-6.9, -8.55), (-1.9, -8.55)], 1.1)
    # hammer (spur up and back)
    slab(mesh, STEEL, [(-5.3, 3.2), (-5.4, 4.2), (-6.7, 4.9), (-7.1, 4.55), (-6.0, 3.6), (-5.9, 3.0)], 0.45)
    # slide stop and thumb safety, small levers on the left side
    slab(mesh, STEEL, [(1.5, 1.7), (4.0, 1.7), (4.0, 2.2), (1.5, 2.3)], 1.32)
    slab(mesh, STEEL, [(-5.0, 2.0), (-3.0, 2.1), (-3.2, 2.6), (-5.2, 2.5)], 1.32)
    # trigger guard: a bar loop
    guard = [(1.4, 0.7), (1.2, -0.6), (1.5, -1.6), (2.6, -2.2), (4.6, -2.1), (5.9, -1.1), (6.4, 0.1), (6.6, 0.7)]
    for a, b in zip(guard, guard[1:]):
        bar(mesh, STEEL, a, b, 0.42, 0.42)
    # trigger blade
    slab(mesh, STEEL, [(3.0, 0.7), (3.55, 0.7), (3.45, -0.5), (3.15, -1.15), (2.85, -1.05), (3.05, -0.3)], 0.32)
    return mesh


def stand_up(mesh):
    """The parts lie on their side (the slide's top toward +Y). Turn them so it points +Z, barrel still +X."""
    GTX = unreal.GeometryScript_MeshTransforms
    for roll in (90.0, -90.0):
        GTX.transform_mesh(mesh, xf(roll=roll))
        box = unreal.GeometryScript_MeshQueries.get_mesh_bounding_box(mesh)
        ext = (box.max.x - box.min.x, box.max.y - box.min.y, box.max.z - box.min.z)
        if ext[2] > 10.0 and box.max.z > -box.min.z:
            log("stood up with roll %.0f: bounds %s .. %s" % (roll, box.min, box.max))
            return True
        GTX.transform_mesh(mesh, xf(roll=-roll))
    log("could not stand the gun up")
    return False


def make_asset():
    mesh = build_mesh()
    stand_up(mesh)
    normals = getattr(unreal, "GeometryScript_Normals", None) or getattr(unreal, "GeometryScript_MeshNormals", None)
    if normals:
        normals.compute_split_normals(mesh, unreal.GeometryScriptSplitNormalsOptions(),
                                      unreal.GeometryScriptCalculateNormalsOptions())
    path = "%s/%s" % (DEST, NAME)
    o = unreal.GeometryScriptCreateNewStaticMeshAssetOptions()
    o.set_editor_property("enable_recompute_normals", normals is None)
    o.set_editor_property("enable_recompute_tangents", True)
    o.set_editor_property("enable_nanite", False)
    o.set_editor_property("enable_collision", False)
    if EAL.does_asset_exist(path):
        sm = EAL.load_asset(path)
        unreal.GeometryScript_StaticMeshFunctions.copy_mesh_to_static_mesh(
            mesh, sm, unreal.GeometryScriptCopyMeshToAssetOptions(), unreal.GeometryScriptMeshWriteLOD())
    else:
        sm, outcome = unreal.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(mesh, path, o)
    mats = [instance("MI_Gun_Steel", (0.035, 0.036, 0.04), 1.0, 0.32),
            instance("MI_Gun_Walnut", (0.12, 0.05, 0.025), 0.0, 0.55),
            instance("MI_Gun_Dark", (0.006, 0.006, 0.007), 0.6, 0.6)]
    statics = []
    for i, mi in enumerate(mats):
        statics.append(unreal.StaticMaterial(material_interface=mi, material_slot_name=["Steel", "Walnut", "Dark"][i]))
    sm.set_editor_property("static_materials", statics)
    sm.set_editor_property("cast_shadow", True) if hasattr(sm, "cast_shadow") else None
    EAL.save_asset(path, only_if_is_dirty=False)
    box = sm.get_bounding_box()
    log("%s: %d triangles, bounds %s .. %s" % (path, sm.get_num_triangles(0), box.min, box.max))


# --- the grip: hand_r in the template's pistol aim ------------------------------------------------------------------------
CHAIN = ["root", "pelvis", "spine_01", "spine_02", "spine_03", "spine_04", "spine_05", "clavicle_r", "upperarm_r",
         "lowerarm_r", "hand_r"]


def grip_probe():
    anim = EAL.load_asset("/Game/Characters/Mannequins/Anims/Pistol/MF_Pistol_Idle_ADS")
    if not anim:
        log("no MF_Pistol_Idle_ADS")
        return
    cs = unreal.Transform()
    for bone in CHAIN:
        try:
            local = unreal.AnimationLibrary.get_bone_pose_for_frame(anim, bone, 0, False)
        except Exception as e:  # noqa: BLE001
            log("pose for %s: %s" % (bone, e))
            return
        cs = local.multiply(cs)   # local then parent (UE: A * B applies A first)
    hand = cs
    log("hand_r in the aim pose (mesh space): loc %s rot %s" % (hand.translation, hand.rotation.rotator()))
    # The gun wanted in mesh space: barrel +X along the mannequin's forward (+Y), top +Z up -> yaw 90.
    want = unreal.Transform(hand.translation, unreal.Rotator(roll=0.0, pitch=0.0, yaw=90.0), unreal.Vector(1, 1, 1))
    rel = want.multiply(hand.inverse())
    log("GunGrip rotation for hand_r: %s" % rel.rotation.rotator())
    # Axes of hand_r in mesh space (to place the grip in the palm).
    for name, v in (("X", unreal.Vector(1, 0, 0)), ("Y", unreal.Vector(0, 1, 0)), ("Z", unreal.Vector(0, 0, 1))):
        log("hand_r %s axis in mesh space: %s" % (name, unreal.MathLibrary.transform_direction(hand, v)))


try:
    if not EAL.does_directory_exist(DEST):
        EAL.make_directory(DEST)
    make_asset()
except Exception as e:  # noqa: BLE001

    log("build failed: %s" % e)
    log(traceback.format_exc())
grip_probe()
RESULT = "\n".join(OUT)
print(RESULT)
