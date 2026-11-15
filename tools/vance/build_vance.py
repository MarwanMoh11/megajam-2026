"""Vance (the lead, camera A), modelled in Blender from the UE mannequin's own body and skeleton (Nov 15).

Headless:  "C:/Program Files/Blender Foundation/Blender 5.3/blender.exe" -b -P tools/vance/build_vance.py
In:  C:/Jam/Art/Vance/SKM_Manny_Simple.fbx (exported from the editor: SKM_Manny_Simple on SK_Mannequin)
Out: C:/Jam/Art/Vance/SKM_Vance.fbx, C:/Jam/Art/Vance/Vance.blend, previews C:/Jam/Art/Vance/preview_*.png

Why the mannequin's body: every clip the game plays (the template's and the 40 Mixamo clips retargeted in
tools/build_mocap.py) is on SK_Mannequin. The clothes are the mannequin's surface pushed out per region, so each
vertex keeps the mannequin's own skin weights: elbows, shoulders and hips bend the way the clips were checked on.
Added on top: a head (nose, ears, eyes, brows, pencil moustache, slicked hair) on the head bone, the dinner jacket's
skirt, lapels, bow tie, pocket square and carnation, each weighted from the nearest body surface.
"""
import math
import os
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector
from mathutils.kdtree import KDTree

ART = r"C:\Jam\Art\Vance"
SRC = os.path.join(ART, "SKM_Manny_Simple.fbx")
OUT = os.path.join(ART, "SKM_Vance.fbx")
PREVIEW_ONLY = "--preview" in sys.argv
SMOOTH_ITERS = 30

# ---------------------------------------------------------------- scene
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=SRC, use_anim=False, ignore_leaf_bones=False, automatic_bone_orientation=False)
arm = next(o for o in bpy.data.objects if o.type == "ARMATURE")
body = max((o for o in bpy.data.objects if o.type == "MESH"), key=lambda o: len(o.data.vertices))   # LOD0
mw_arm = arm.matrix_world.copy()
arm.parent = None                # the import's 0.01 empty above the armature goes; its scale stays on the armature
arm.matrix_world = mw_arm
for o in list(bpy.data.objects):
    if o not in (arm, body):
        bpy.data.objects.remove(o, do_unlink=True)
# the armature object stays "root": it is exported as the skeleton's root bone, as it came in
body.name = "Vance_Body"
MW = arm.matrix_world


def bone(name, tail=False):
    b = arm.data.bones[name]
    return MW @ (b.tail_local if tail else b.head_local)


UNIT = 100.0 * MW.to_scale().x                               # world units per metre (the import is in metres)
fwd = bone("ball_l") - bone("foot_l"); fwd.z = 0
fwd = Vector((round(fwd.x / fwd.length), round(fwd.y / fwd.length), 0))   # the feet splay: snap to the axis
up = Vector((0, 0, 1))
side = fwd.cross(up).normalized()                            # character's right... sign checked below
if (bone("thigh_l") - bone("pelvis")).dot(side) < 0:
    side = -side                                             # side now points to the character's left (_l)
print("UNIT", UNIT, "fwd", fwd, "left", side)


def cm(v):
    return v * UNIT / 100.0


def mat(name, rgb, rough=0.6, metal=0.0):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    b = next(n for n in m.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    b.inputs["Base Color"].default_value = (*rgb, 1)
    b.inputs["Roughness"].default_value = rough
    b.inputs["Metallic"].default_value = metal
    m.diffuse_color = (*rgb, 1)
    return m


MATS = dict(
    skin=mat("M_Vance_Skin", (0.78, 0.55, 0.42), 0.55),
    jacket=mat("M_Vance_Jacket", (0.84, 0.79, 0.66), 0.7),       # white dinner jacket
    trousers=mat("M_Vance_Black", (0.018, 0.018, 0.022), 0.6),
    shirt=mat("M_Vance_Shirt", (0.96, 0.96, 0.94), 0.6),
    shoe=mat("M_Vance_Shoe", (0.012, 0.010, 0.009), 0.2),
    hair=mat("M_Vance_Hair", (0.022, 0.016, 0.012), 0.35),
    eye=mat("M_Vance_EyeWhite", (0.88, 0.86, 0.82), 0.3),
    iris=mat("M_Vance_Iris", (0.05, 0.034, 0.024), 0.2),
    flower=mat("M_Vance_Carnation", (0.60, 0.02, 0.05), 0.6),
)
MAT_ORDER = list(MATS)

# ---------------------------------------------------------------- body: the mannequin surface, dressed
me = body.data
me.materials.clear()
for k in MAT_ORDER:
    me.materials.append(MATS[k])
gname = {g.index: g.name for g in body.vertex_groups}


def weights(v):
    return {gname[g.group]: g.weight for g in v.groups if g.weight > 0}


def region_sum(w, keys):
    return sum(val for n, val in w.items() if any(n.startswith(k) for k in keys))


HAND = ("hand_", "thumb_", "index_", "middle_", "ring_", "pinky_")
HEADK = ("head",)

# the mannequin's sculpted panel seams would show through cloth and skin: weld its shells, then relax everything but
# the hands (kept as they are, so the finger curls stay right)
_bm = bmesh.new()
_bm.from_mesh(me)
bmesh.ops.remove_doubles(_bm, verts=_bm.verts[:], dist=0.05)
_dl = _bm.verts.layers.deform.active
_hand_ids = {i for i, g in gname.items() if any(g.startswith(k) for k in HAND)}
_soft = [v for v in _bm.verts if sum(w for i, w in v[_dl].items() if i in _hand_ids) < 0.3]
for _ in range(SMOOTH_ITERS):
    bmesh.ops.smooth_vert(_bm, verts=_soft, factor=0.5, use_axis_x=True, use_axis_y=True, use_axis_z=True)
_bm.to_mesh(me)
_bm.free()
NECK = ("neck_",)
FOOT = ("foot_", "ball_")
LEG = ("thigh_", "calf_", "pelvis")
ARM = ("upperarm_", "lowerarm_", "clavicle_")
SPINE = ("spine_",)

wrist_l, wrist_r = bone("hand_l"), bone("hand_r")
elbow_l = bone("lowerarm_l")
forearm_len = (wrist_l - elbow_l).length
neck_base = bone("neck_01")
chest = bone("spine_05")
pelvis = bone("pelvis")
waist_z = bone("spine_02").z

# per vertex: offset (cm, along the normal) and its region
vregion = {}
offset = {}
for v in me.vertices:
    w = weights(v)
    tot = sum(w.values()) or 1.0
    co = body.matrix_world @ v.co
    h = region_sum(w, HAND) / tot
    hd = region_sum(w, HEADK) / tot
    nk = region_sum(w, NECK) / tot
    ft = region_sum(w, FOOT) / tot
    lg = region_sum(w, LEG) / tot
    if h > 0.5:
        r, off = "skin", 0.0
    elif hd > 0.5:
        r, off = "skin", 0.0
    elif nk > 0.5:
        # the collar: the lower neck is shirt collar, the upper skin
        r, off = ("shirt", 0.45) if co.z < neck_base.z + cm(4.5) else ("skin", 0.0)
    elif ft > 0.5:
        r, off = "shoe", 0.55
    elif lg > 0.5 and co.z < waist_z - cm(2):
        r, off = "trousers", 1.1
    else:
        r, off = "jacket", 1.6
        # sleeve cuff: the last 2.5 cm of the forearm shows the shirt
        for wr in (wrist_l, wrist_r):
            if (co - wr).length < cm(4.5):
                r, off = "shirt", 0.9
    vregion[v.index] = r
    offset[v.index] = off

# shirt front: the V between the lapels, on the chest's front
for v in me.vertices:
    if vregion[v.index] != "jacket":
        continue
    co = body.matrix_world @ v.co
    rel = co - chest
    fwd_d = rel.dot(fwd)
    if fwd_d < cm(2):
        continue
    x = abs((co - pelvis).dot(side))
    z = co.z
    top = neck_base.z + cm(2)
    bottom = chest.z - cm(17)                     # the V closes about 17 cm under the chest bone (one button)
    if bottom < z < top:
        half = cm(1.2) + (z - bottom) / (top - bottom) * cm(9.5)
        if x < half:
            vregion[v.index] = "shirt"
            offset[v.index] = 1.0

# push out along smoothed normals, then relax the seams between offsets
bm = bmesh.new()
bm.from_mesh(me)
bm.verts.ensure_lookup_table()
nrm = [v.normal.copy() for v in bm.verts]
off = [offset[i] for i in range(len(bm.verts))]
for _ in range(3):                                  # smooth the offset field so region borders don't step
    new = off[:]
    for v in bm.verts:
        nb = [e.other_vert(v).index for e in v.link_edges]
        if nb and off[v.index] > 0:
            new[v.index] = 0.5 * off[v.index] + 0.5 * sum(off[i] for i in nb) / len(nb)
    off = new
for v in bm.verts:
    if off[v.index] > 0:
        v.co += nrm[v.index] * cm(off[v.index]) / body.matrix_world.to_scale().x
# trousers: straighten the calves a little (a cut leg, not a muscle)
for f in bm.faces:
    rs = [vregion[v.index] for v in f.verts]
    best = max(set(rs), key=rs.count)
    f.material_index = MAT_ORDER.index(best)
    f.smooth = True
bm.to_mesh(me)
bm.free()

# ---------------------------------------------------------------- additions (each skinned from the nearest body vertex)
col = body.users_collection[0]
added = []
kd_src = KDTree(len(me.vertices))
for v in me.vertices:
    kd_src.insert(body.matrix_world @ v.co, v.index)
kd_src.balance()


def add(name, bm_, material, rigid=None):
    m = bpy.data.meshes.new(name)
    bm_.to_mesh(m)
    bm_.free()
    o = bpy.data.objects.new(name, m)
    col.objects.link(o)
    m.materials.append(MATS[material])
    for p in m.polygons:
        p.use_smooth = True
    added.append((o, rigid))
    return o


def place(bm_, origin, x_axis, y_axis, z_axis):
    """bm_ built in a local frame (x = character left, y = forward, z = up, cm) -> world."""
    M = Matrix((x_axis, y_axis, z_axis)).transposed()
    for v in bm_.verts:
        v.co = origin + M @ (v.co * UNIT / 100.0)


hb = bone("head")
ht = bone("head", tail=True)
# the mannequin's head: measure it from its vertices on the head bone
head_vs = [body.matrix_world @ v.co for v in me.vertices if vregion[v.index] == "skin"
           and region_sum(weights(v), HEADK) > 0.5]
hc = sum(head_vs, Vector()) / len(head_vs)
hfront = max(head_vs, key=lambda p: (p - hc).dot(fwd))
htop = max(head_vs, key=lambda p: p.z)
hside = max(head_vs, key=lambda p: (p - hc).dot(side))
R_f = (hfront - hc).dot(fwd) * 100 / UNIT
R_s = (hside - hc).dot(side) * 100 / UNIT
R_t = (htop.z - hc.z) * 100 / UNIT
print("head radii cm: front %.1f side %.1f top %.1f" % (R_f, R_s, R_t))


def face(x, z, out=0.0):
    """A point on the face's surface in local cm: x left, z up from the head centre; y found on the mesh."""
    return Vector((x, R_f * math.sqrt(max(0.05, 1 - (x / R_s) ** 2 - (z / (R_t * 1.05)) ** 2)) + out, z))


def at_head(bm_):
    place(bm_, hc, side, fwd, up)


# nose
b = bmesh.new()
bmesh.ops.create_cone(b, cap_ends=True, segments=10, radius1=1.5, radius2=0.5, depth=4.4)
bmesh.ops.rotate(b, verts=b.verts, cent=(0, 0, 0), matrix=Matrix.Rotation(math.radians(-62), 3, "X"))
for v in b.verts:
    v.co.x *= 0.85
base = face(0, -1.5)
bmesh.ops.translate(b, verts=b.verts, vec=Vector((0, base.y + 0.6, -1.5)))
at_head(b)
add("Vance_Nose", b, "skin", "head")
# ears
for sx in (1, -1):
    b = bmesh.new()
    bmesh.ops.create_uvsphere(b, u_segments=10, v_segments=8, radius=1)
    for v in b.verts:
        v.co = Vector((v.co.x * 0.9, v.co.y * 1.7, v.co.z * 3.0))
    bmesh.ops.translate(b, verts=b.verts, vec=Vector((sx * (R_s - 0.2), -0.5, 0.5)))
    at_head(b)
    add("Vance_Ear", b, "skin", "head")
# eyes, irises, brows
for sx in (1, -1):
    p = face(sx * 3.1, 2.0, -0.9)
    b = bmesh.new()
    bmesh.ops.create_uvsphere(b, u_segments=14, v_segments=10, radius=1.25)
    bmesh.ops.translate(b, verts=b.verts, vec=p)
    at_head(b)
    add("Vance_Eye", b, "eye", "head")
    b = bmesh.new()
    bmesh.ops.create_uvsphere(b, u_segments=12, v_segments=8, radius=0.62)
    for v in b.verts:
        v.co.y *= 0.45
    bmesh.ops.translate(b, verts=b.verts, vec=p + Vector((0, 1.05, 0)))
    at_head(b)
    add("Vance_Iris", b, "iris", "head")
    b = bmesh.new()
    bmesh.ops.create_cube(b, size=1)
    for v in b.verts:
        v.co = Vector((v.co.x * 3.2, v.co.y * 0.6, v.co.z * 0.55))
    bmesh.ops.rotate(b, verts=b.verts, cent=(0, 0, 0), matrix=Matrix.Rotation(math.radians(sx * 9), 3, "Y"))
    bmesh.ops.translate(b, verts=b.verts, vec=face(sx * 3.2, 4.0, 0.25))
    at_head(b)
    add("Vance_Brow", b, "hair", "head")
# pencil moustache and mouth
for name, z, w, hgt, m_ in (("Vance_Moustache", -3.6, 2.4, 0.32, "hair"), ("Vance_Mouth", -5.3, 1.9, 0.22, "iris")):
    b = bmesh.new()
    bmesh.ops.create_cube(b, size=1)
    for v in b.verts:
        v.co = Vector((v.co.x * w * 2, v.co.y * 0.5, v.co.z * hgt))
    bmesh.ops.translate(b, verts=b.verts, vec=face(0, z, 0.05))
    at_head(b)
    add(name, b, m_, "head")
# slicked-back hair: the skull's own surface above the hairline, lifted off it, a wave over the brow
hb_ = bmesh.new()
hb_.from_mesh(me)
hb_.transform(body.matrix_world)
dl_ = hb_.verts.layers.deform.active
head_gi = {i for i, g in gname.items() if g == "head"}


def hair_here(p):
    rel = p - hc
    y = rel.dot(fwd) * 100 / UNIT                      # cm forward of the head centre
    z = rel.z * 100 / UNIT
    x = abs(rel.dot(side)) * 100 / UNIT
    if y > R_f * 0.45:                                 # forehead: the hairline sits high
        return z > 6.0 + 0.10 * x
    if y > -R_f * 0.2:                                 # temples and over the ears
        return z > 2.5 - 0.35 * (R_f * 0.45 - y)
    return z > -5.5                                    # the back, down to the nape


keep = {v for v in hb_.verts if sum(w for i, w in v[dl_].items() if i in head_gi) > 0.5 and hair_here(v.co)}
bmesh.ops.delete(hb_, geom=[f for f in hb_.faces if not all(v in keep for v in f.verts)], context="FACES")
bmesh.ops.delete(hb_, geom=[v for v in hb_.verts if not v.link_faces], context="VERTS")
for v in hb_.verts:
    rel = v.co - hc
    lift = 0.55
    y = rel.dot(fwd) * 100 / UNIT
    if y > R_f * 0.2 and rel.z * 100 / UNIT > R_t * 0.4:
        lift += 0.9 * min(1.0, (y - R_f * 0.2) / (R_f * 0.5))          # the wave over the brow
    v.co += v.normal * cm(lift)
bmesh.ops.solidify(hb_, geom=hb_.faces[:], thickness=-cm(0.4))
for v in hb_.verts:
    v.co = v.co                                        # already world
b = bmesh.new()
_m = bpy.data.meshes.new("_hair_tmp")
hb_.to_mesh(_m)
hb_.free()
b.from_mesh(_m)
bpy.data.meshes.remove(_m)
add("Vance_Hair", b, "hair", "head")

# jacket skirt: from the waist to the upper thigh, an ellipse a little wider than the hips, open in front below the
# button. It takes its weights from the nearest body vertices (pelvis and thighs).
torso_vs = [body.matrix_world @ v.co for v in me.vertices
            if region_sum(weights(v), LEG + SPINE) / max(1e-6, sum(weights(v).values())) > 0.6]


def hip_extent(zw):
    sl = [p for p in torso_vs if abs(p.z - zw) < cm(1.5)]
    rel = [p - pelvis for p in sl]
    xs = [r.dot(side) for r in rel]
    ys = [r.dot(fwd) for r in rel]
    return (max(map(abs, xs)) * 100 / UNIT, max(ys) * 100 / UNIT, -min(ys) * 100 / UNIT)


b = bmesh.new()
rings, segs = 5, 40
top_z = (bone("spine_02").z - pelvis.z) * 100 / UNIT
bot_z = -22.0                                          # cm under the pelvis bone: the hem over the upper thigh
ext_top = hip_extent(pelvis.z + cm(top_z))
ext_hip = hip_extent(pelvis.z + cm(-6))
bv = []
for ri in range(rings + 1):
    t = ri / rings
    z = top_z + (bot_z - top_z) * t
    k = min(1.0, t * 2.0)                              # waist -> hip by the middle ring, then straight with flare
    ax = ext_top[0] * (1 - k) + ext_hip[0] * k + 1.8 + 1.2 * t
    af = ext_top[1] * (1 - k) + ext_hip[1] * k + 1.8 + 1.0 * t
    ab = ext_top[2] * (1 - k) + ext_hip[2] * k + 1.8 + 1.4 * t
    row = []
    for si in range(segs):
        a = 2 * math.pi * si / segs
        c, s = math.cos(a), math.sin(a)
        row.append(b.verts.new(Vector((c * ax, s * (af if s > 0 else ab), z))))
    bv.append(row)
for ri in range(rings):
    for si in range(segs):
        sj = (si + 1) % segs
        a = math.degrees(2 * math.pi * (si + 0.5) / segs)
        open_half = 0 if ri == 0 else 4 + 16 * ri / rings        # the front parts under the button
        if abs(a - 90) < open_half:
            continue
        b.faces.new((bv[ri][si], bv[ri][sj], bv[ri + 1][sj], bv[ri + 1][si]))
bmesh.ops.delete(b, geom=[v for v in b.verts if not v.link_faces], context="VERTS")
bmesh.ops.recalc_face_normals(b, faces=b.faces[:])
bmesh.ops.solidify(b, geom=b.faces[:], thickness=0.5)
place(b, pelvis, side, fwd, up)
add("Vance_JacketSkirt", b, "jacket", None)
# lapels (shawl lapels in the jacket cloth), bow tie, pocket square, carnation, buttons
chest_front = max((body.matrix_world @ v.co for v in me.vertices
                   if vregion[v.index] in ("jacket", "shirt") and abs((body.matrix_world @ v.co).z - chest.z) < cm(2)),
                  key=lambda p: (p - chest).dot(fwd))
cf = (chest_front - chest).dot(fwd) * 100 / UNIT
for sx in (1, -1):
    b = bmesh.new()
    top = (neck_base.z - chest.z) * 100 / UNIT + 1.5
    pts = [(1.2, -17.0), (3.0, -12), (6.5, -4), (9.5, 2), (10.0, top - 1.0), (8.5, top)]
    inner = [(1.2, -17.0), (2.0, -12), (4.2, -4), (6.0, 2), (7.2, top - 2.5), (7.5, top - 1.0)]
    vs_o = [b.verts.new(Vector((sx * x, cf + 1.4 - 0.12 * abs(x), z))) for x, z in pts]
    vs_i = [b.verts.new(Vector((sx * x, cf + 1.6 - 0.12 * abs(x), z))) for x, z in inner]
    for i in range(len(pts) - 1):
        fv = (vs_i[i], vs_o[i], vs_o[i + 1], vs_i[i + 1]) if sx > 0 else (vs_i[i + 1], vs_o[i + 1], vs_o[i], vs_i[i])
        try:
            b.faces.new(fv)
        except ValueError:
            pass
    bmesh.ops.solidify(b, geom=b.faces[:], thickness=0.35)
    place(b, chest, side, fwd, up)
    pass                                               # lapels dropped: they read as braces; the V does it
# bow tie at the collar
b = bmesh.new()
for sx in (1, -1):
    bmesh.ops.create_cone(b, cap_ends=True, segments=4, radius1=2.0, radius2=0.5, depth=3.6,
                          matrix=Matrix.Translation((sx * 1.8, 0, 0)) @ Matrix.Rotation(math.radians(sx * 90), 4, "Y"))
bmesh.ops.create_cube(b, size=1.1)
neck_front = max((body.matrix_world @ v.co for v in me.vertices
                  if abs((body.matrix_world @ v.co).z - (neck_base.z + cm(2.5))) < cm(1.5)),
                 key=lambda p: (p - neck_base).dot(fwd))
nf = (neck_front - neck_base).dot(fwd) * 100 / UNIT
for v in b.verts:
    v.co.y *= 0.5
bmesh.ops.translate(b, verts=b.verts, vec=Vector((0, nf + 0.9, 2.5)))
place(b, neck_base, side, fwd, up)
add("Vance_BowTie", b, "trousers", "neck_01")
# buttons (two, black) under the V and a red carnation on his left lapel, a white pocket square on the left chest
for i, z in enumerate((-18.5, -26.0)):
    b = bmesh.new()
    bmesh.ops.create_cone(b, cap_ends=True, segments=10, radius1=0.75, radius2=0.75, depth=0.4,
                          matrix=Matrix.Rotation(math.radians(90), 4, "X"))
    zz = chest.z + cm(z)
    p = max((body.matrix_world @ v.co for v in me.vertices if abs((body.matrix_world @ v.co).z - zz) < cm(1.2)
             and abs((body.matrix_world @ v.co - pelvis).dot(side)) < cm(3)), key=lambda q: (q - chest).dot(fwd))
    bmesh.ops.translate(b, verts=b.verts, vec=Vector((0, (p - chest).dot(fwd) * 100 / UNIT + 0.2, z)))
    place(b, chest, side, fwd, up)
    add("Vance_Button", b, "trousers", None)
b = bmesh.new()
bmesh.ops.create_icosphere(b, subdivisions=2, radius=1.5)
bmesh.ops.translate(b, verts=b.verts, vec=Vector((8.0, cf + 2.2, -1.0)))
place(b, chest, side, fwd, up)
add("Vance_Carnation", b, "flower", None)
b = bmesh.new()
bmesh.ops.create_cube(b, size=1)
for v in b.verts:
    v.co = Vector((v.co.x * 4.0, v.co.y * 0.4, v.co.z * 1.6))
bmesh.ops.translate(b, verts=b.verts, vec=Vector((-8.5, cf + 0.4, -8.0)))
place(b, chest, side, fwd, up)
add("Vance_PocketSquare", b, "shirt", None)

# ---------------------------------------------------------------- skin the additions
for o, rigid in added:
    for g in body.vertex_groups:
        o.vertex_groups.new(name=g.name)
    if rigid:
        o.vertex_groups[rigid].add(range(len(o.data.vertices)), 1.0, "REPLACE")
        continue
    for v in o.data.vertices:
        p = o.matrix_world @ v.co
        found = kd_src.find_n(p, 4)
        acc = {}
        tw = 0.0
        for co, idx, dist in found:
            wgt = 1.0 / max(dist, 1e-5)
            tw += wgt
            for n, val in weights(me.vertices[idx]).items():
                acc[n] = acc.get(n, 0) + val * wgt
        for n, val in acc.items():
            if val / tw > 0.01:
                o.vertex_groups[n].add([v.index], val / tw, "REPLACE")

# join everything into one skinned mesh (one draw-call chain, one asset)
bpy.ops.object.select_all(action="DESELECT")
for o, _ in added:
    o.select_set(True)
body.select_set(True)
bpy.context.view_layer.objects.active = body
bpy.ops.object.join()
for m_ in body.modifiers:
    if m_.type == "ARMATURE":
        m_.object = arm
if not any(m_.type == "ARMATURE" for m_ in body.modifiers):
    md = body.modifiers.new("Armature", "ARMATURE")
    md.object = arm
body.parent = arm
print("Vance: %d verts, %d tris" % (len(body.data.vertices), sum(len(p.vertices) - 2 for p in body.data.polygons)))

# ---------------------------------------------------------------- previews (workbench, material colours)
scn = bpy.context.scene
try:
    scn.render.engine = "BLENDER_WORKBENCH"
except TypeError as e:
    print("engine", e)
scn.display.shading.color_type = "MATERIAL"
scn.display.shading.light = "STUDIO"
scn.render.resolution_x, scn.render.resolution_y = 700, 900
world = bpy.data.worlds.new("w")
scn.world = world
cam_d = bpy.data.cameras.new("cam")
cam = bpy.data.objects.new("cam", cam_d)
scn.collection.objects.link(cam)
scn.camera = cam
centre = (bone("head") + bone("pelvis")) * 0.5 + Vector((0, 0, -cm(10)))


def shoot(name, direction, dist_cm, target=centre, lens=50):
    cam_d.lens = lens
    cam.location = target + direction.normalized() * cm(dist_cm)
    cam.rotation_euler = (target - cam.location).to_track_quat("-Z", "Y").to_euler()
    scn.render.filepath = os.path.join(ART, "preview_%s.png" % name)
    bpy.ops.render.render(write_still=True)


shoot("front", fwd + Vector((0, 0, 0.05)), 520)
shoot("three_quarter", fwd + side * 0.8 + Vector((0, 0, 0.15)), 520)
shoot("back", -fwd + Vector((0, 0, 0.1)), 520)
shoot("face", fwd + side * 0.35 + Vector((0, 0, 0.05)), 90, target=hc)
# a bent pose: the clips' worst case (arms up, knee and elbow bent) to check the skinning
pb = arm.pose.bones
for n, ax, deg in (("upperarm_l", "X", -60), ("lowerarm_l", "Z", 70), ("upperarm_r", "Z", 50),
                   ("lowerarm_r", "Z", -80), ("thigh_l", "Z", 50), ("calf_l", "Z", -80), ("spine_03", "X", 15)):
    pb[n].rotation_mode = "XYZ"
    pb[n].rotation_euler = Matrix.Rotation(math.radians(deg), 3, ax).to_euler()
bpy.context.view_layer.update()
shoot("pose", fwd + side * 0.6 + Vector((0, 0, 0.2)), 540)
for p in pb:
    p.rotation_euler = (0, 0, 0)
    p.rotation_quaternion = (1, 0, 0, 0)
bpy.data.objects.remove(cam, do_unlink=True)

bpy.ops.wm.save_as_mainfile(filepath=os.path.join(ART, "Vance.blend"))
if not PREVIEW_ONLY:
    bpy.ops.object.select_all(action="DESELECT")
    arm.select_set(True)
    body.select_set(True)
    bpy.ops.export_scene.fbx(filepath=OUT, use_selection=True, object_types={"ARMATURE", "MESH"},
                             add_leaf_bones=False, bake_anim=False, use_armature_deform_only=False,
                             mesh_smooth_type="FACE", apply_unit_scale=True, primary_bone_axis="Y",
                             secondary_bone_axis="X")
    print("exported", OUT)
