"""The 1978 florist van (Fleurs du Soir) for Scene 2, modelled in Blender 5.3.

Run inside Blender (the Blender MCP's execute_blender_code, or Blender's Text editor):
    p = r"C:\\Jam\\megajam\\tools\\blender\\florist_van.py"; exec(compile(open(p, encoding="utf-8").read(), p, "exec"), {"__name__": "__main__"})
Re-runnable: it clears its own collections first. Builds collection "FloristVan" (everything parented to the empty
"FloristVan") and a grey studio in "VanStudio" for looking at it.

Metres, Z up, ground z = 0, origin at the centre of the van's footprint. Nose at -Y. +X is the van's left (the driver:
left-hand drive, fuel cap), -X its right (kerb side, sliding door). Same box as tools/script/scene2_van.py's van:
2.10 wide (x +-1.05), 5.20 long (y -2.60..2.60), body 0.30..2.35, windshield (y, z) -2.52,1.15 to -2.10,2.07, wheels
r 0.35. In the level its origin goes to (CX, (Y0+Y1)/2, 0) = (-5380, 1550, 0) cm, bonnet south.
The shell is hollow (4 cm walls) so Scene 2's camera can push through the windshield into the existing interior.
The windshield glass is its own object (Van_Windshield) so a shot can hide it. Material names match scene2_van.py's
M keys so Unreal can reuse those materials by slot name.
"""
import math

import bmesh
import bpy
from mathutils import Matrix, Vector

REPO = r"C:\Jam\megajam"
FONT_TITLE = REPO + r"\Reel\SourceArt\Fonts\IMFellEnglish\IMFeENit28P.ttf"
FONT_LINE = REPO + r"\Reel\SourceArt\Fonts\JosefinSans\JosefinSans-SemiBold.ttf"

W = 1.05                                  # half width
YN, YR = -2.60, 2.60                      # nose, tail
ZB, ROOF = 0.30, 2.35                     # body bottom, roof top
SHELL = 0.04                              # wall thickness
WHEEL_R, WHEEL_Z, WHEEL_X = 0.35, 0.35, 0.955
WHEEL_Y = (-1.65, 1.50)
ARCH_R, ARCH_Z = 0.44, 0.36
WS_BASE, WS_TOP = Vector((-2.52, 1.15)), Vector((-2.10, 2.07))   # windshield, (y, z)
SIDES = (1, -1)                           # +1 the van's left (+X), -1 its right (-X)
RIG_X = -1.0                              # roof rig side: Blender -X lands on the level's east (+X), see export()
DISH_AIM = Vector((-0.816, -0.577, 0.25)).normalized()           # at the theatre's palm scanner (level: east-south)
TUB_X = 0.82                              # wheel tub wall, inside the shell (clear of the rack's face at 0.80)

MATS = {  # name: (linear base colour, roughness, metallic); colours from scene2_van.py
    "Cream": ((0.58, 0.50, 0.35), 0.42, 0.0),
    "Mint": ((0.10, 0.30, 0.24), 0.45, 0.0),
    "Grime": ((0.10, 0.085, 0.06), 0.9, 0.0),
    "Chrome": ((0.80, 0.80, 0.82), 0.12, 1.0),
    "Black": ((0.014, 0.014, 0.015), 0.5, 0.0),
    "Rubber": ((0.018, 0.018, 0.018), 0.9, 0.0),
    "DarkGlass": ((0.006, 0.008, 0.010), 0.05, 0.0),
    "Steel": ((0.10, 0.10, 0.11), 0.4, 0.6),
    "Beige": ((0.36, 0.31, 0.23), 0.6, 0.0),
    "Petal": ((0.80, 0.78, 0.70), 0.6, 0.0),
    "Stem": ((0.04, 0.13, 0.03), 0.7, 0.0),
    "Leaf": ((0.03, 0.09, 0.025), 0.75, 0.0),
    "Stamen": ((0.60, 0.38, 0.04), 0.6, 0.0),
    "Red": ((0.45, 0.02, 0.02), 0.4, 0.0),
    "Ink": ((0.05, 0.16, 0.12), 0.6, 0.0),
    "Interior": ((0.30, 0.26, 0.19), 0.8, 0.0),
    "Whitewall": ((0.80, 0.78, 0.72), 0.7, 0.0),
}
GLOW = {  # name: (base, emission colour, strength)
    "Headlight": ((0.8, 0.75, 0.6), (1.0, 0.88, 0.65), 0.6),
    "Tail": ((0.2, 0.0, 0.0), (1.0, 0.04, 0.02), 0.6),
    "Amber": ((0.3, 0.15, 0.0), (1.0, 0.45, 0.05), 0.6),
}

COLL = None
ROOT = None


# --- materials -------------------------------------------------------------------------------------------------------
def _bsdf(m):
    try:
        m.use_nodes = True
    except Exception:  # noqa: BLE001  (always on in newer Blender)
        pass
    return next(n for n in m.node_tree.nodes if n.type == "BSDF_PRINCIPLED")


def _set(node, key, val):
    if key in node.inputs:
        node.inputs[key].default_value = val


def _mat(name, col, rough, metal=0.0, emit=None, alpha=1.0):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    b = _bsdf(m)
    _set(b, "Base Color", (*col, 1.0))
    _set(b, "Roughness", rough)
    _set(b, "Metallic", metal)
    if emit:
        _set(b, "Emission Color", (*emit[0], 1.0))
        _set(b, "Emission Strength", emit[1])
    if alpha < 1.0:
        _set(b, "Alpha", alpha)
        try:
            m.surface_render_method = "BLENDED"
        except Exception:  # noqa: BLE001
            pass
    m.diffuse_color = (*col, alpha)
    return m


def materials():
    for name, (col, rough, metal) in MATS.items():
        _mat(name, col, rough, metal)
    for name, (col, ecol, s) in GLOW.items():
        _mat(name, col, 0.3, 0.0, emit=(ecol, s))
    _mat("Fog", (0.70, 0.76, 0.80), 0.25, alpha=0.35)
    _mat("VanGround", (0.16, 0.16, 0.16), 0.85)


# --- scene plumbing --------------------------------------------------------------------------------------------------
def fresh_collection(name):
    c = bpy.data.collections.get(name)
    if c:
        for o in list(c.all_objects):
            bpy.data.objects.remove(o, do_unlink=True)
    else:
        c = bpy.data.collections.new(name)
        bpy.context.scene.collection.children.link(c)
    return c


def mk(name, bm, mats, smooth=True, angle=35.0):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    for mn in ([mats] if isinstance(mats, str) else mats):
        me.materials.append(bpy.data.materials[mn])
    if smooth:
        for p in me.polygons:
            p.use_smooth = True
        try:
            me.set_sharp_from_angle(angle=math.radians(angle))
        except Exception:  # noqa: BLE001
            pass
    ob = bpy.data.objects.new(name, me)
    COLL.objects.link(ob)
    ob.parent = ROOT
    return ob


# --- primitives ------------------------------------------------------------------------------------------------------
def box(name, x0, x1, y0, y1, z0, z1, m, bev=0.0, segs=2):
    x0, x1 = sorted((x0, x1))
    y0, y1 = sorted((y0, y1))
    z0, z1 = sorted((z0, z1))
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    for v in bm.verts:
        v.co = Vector(((x0 + x1) / 2 + v.co.x * (x1 - x0), (y0 + y1) / 2 + v.co.y * (y1 - y0),
                       (z0 + z1) / 2 + v.co.z * (z1 - z0)))
    if bev > 0:
        bmesh.ops.bevel(bm, geom=list(bm.edges), offset=bev, segments=segs, profile=0.5, affect="EDGES",
                        clamp_overlap=True)
    return mk(name, bm, m)


def cyl(name, p0, p1, r, m, segs=24, r1=None):
    p0, p1 = Vector(p0), Vector(p1)
    d = p1 - p0
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=segs, radius1=r,
                          radius2=(r if r1 is None else r1), depth=d.length)
    rot = d.to_track_quat("Z", "Y").to_matrix().to_4x4()
    bmesh.ops.transform(bm, matrix=Matrix.Translation((p0 + p1) / 2) @ rot, verts=bm.verts)
    return mk(name, bm, m)


def ell(name, c, size, m, rot=None, segs=16):
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=segs, v_segments=max(6, segs // 2), radius=1.0)
    R = (rot if rot is not None else Matrix.Identity(3)).to_4x4()
    bmesh.ops.transform(bm, matrix=Matrix.Translation(c) @ R @ Matrix.Diagonal((*size, 1.0)), verts=bm.verts)
    return mk(name, bm, m)


def fillet(pts, r, segs=6):
    """Round every corner of a closed 2D polygon with radius r (clamped to half of each edge)."""
    out = []
    n = len(pts)
    for i in range(n):
        p, a, b = Vector(pts[i]), Vector(pts[i - 1]), Vector(pts[(i + 1) % n])
        da, db = (a - p).normalized(), (b - p).normalized()
        ang = da.angle(db)
        t = min(r / math.tan(ang / 2), (a - p).length * 0.49, (b - p).length * 0.49)
        rr = t * math.tan(ang / 2)
        c = p + (da + db).normalized() * (rr / math.sin(ang / 2))
        s, e = p + da * t, p + db * t
        a0, a1 = math.atan2(s.y - c.y, s.x - c.x), math.atan2(e.y - c.y, e.x - c.x)
        d = (a1 - a0 + math.pi) % (2 * math.pi) - math.pi
        for k in range(segs + 1):
            ak = a0 + d * k / segs
            out.append((c.x + rr * math.cos(ak), c.y + rr * math.sin(ak)))
    return out


def prism(name, pts, o, u, v, w, depth, m, r=0.0, segs=6):
    """A 2D polygon (a, b) laid on o + u*a + v*b, extruded +-depth/2 along w."""
    if r > 0:
        pts = fillet(pts, r, segs)
    o, u, v, w = (Vector(t) for t in (o, u, v, w))
    bm = bmesh.new()
    bot = [bm.verts.new(o + u * a + v * b - w * depth / 2) for a, b in pts]
    top = [bm.verts.new(o + u * a + v * b + w * depth / 2) for a, b in pts]
    bm.faces.new(bot)
    bm.faces.new(top)
    n = len(pts)
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new((bot[i], bot[j], top[j], top[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return mk(name, bm, m)


def lathe(name, prof, m, steps=48, axis=(1, 0, 0)):
    """Spin a closed (axial, radial) profile round the axis through the origin (axis X: verts (a, 0, r))."""
    bm = bmesh.new()
    if axis == (1, 0, 0):
        vs = [bm.verts.new((a, 0.0, r)) for a, r in prof]
    else:
        vs = [bm.verts.new((r, 0.0, a)) for a, r in prof]
    es = [bm.edges.new((vs[i], vs[(i + 1) % len(vs)])) for i in range(len(vs))]
    bmesh.ops.spin(bm, geom=vs + es, cent=(0, 0, 0), axis=axis, angle=2 * math.pi, steps=steps, use_merge=True,
                   use_duplicate=False)
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-6)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return bm


def side(s, y, z, off=0.0):
    return Vector((s * (W + off), y, z))


def strip_v(name, s, y, z0, z1, m="Black", wdt=0.007):
    return box(name, s * (W - 0.002), s * (W + 0.0025), y - wdt / 2, y + wdt / 2, z0, z1, m)


def strip_h(name, s, y0, y1, z, m="Black", wdt=0.007):
    return box(name, s * (W - 0.002), s * (W + 0.0025), y0, y1, z - wdt / 2, z + wdt / 2, m)


def ws_point(sv, out=0.0):
    """A point on the windshield plane: sv metres up the slope from its base, `out` along its outward normal."""
    d = (WS_TOP - WS_BASE).normalized()
    p = WS_BASE + d * sv + Vector((-d.y, d.x)) * out
    return Vector((0.0, p.x, p.y))


def ws_frame():
    d = (WS_TOP - WS_BASE).normalized()
    return Vector((1, 0, 0)), Vector((0, d.x, d.y)), Vector((0, -d.y, d.x))


def rect(cx, cy, hw, hh):
    return [(cx - hw, cy - hh), (cx + hw, cy - hh), (cx + hw, cy + hh), (cx - hw, cy + hh)]


# --- the shell -------------------------------------------------------------------------------------------------------
CAB_WIN = [(-2.32, 1.28), (-1.26, 1.28), (-1.26, 1.98), (-2.00, 1.98)]   # (y, z), front edge parallel to the A pillar


def body():
    prof = [(-2.56, 0.30), (-2.635, 0.40), (-2.66, 0.58), (-2.66, 0.98), (-2.62, 1.10), (WS_BASE.x, WS_BASE.y),
            (WS_TOP.x, WS_TOP.y), (-2.03, 2.24), (-1.90, ROOF), (2.52, ROOF), (YR, 2.27), (YR, ZB)]
    bm = bmesh.new()
    L = [bm.verts.new((-W, y, z)) for y, z in prof]
    R = [bm.verts.new((W, y, z)) for y, z in prof]
    bm.faces.new(L)
    bm.faces.new(R)
    n = len(prof)
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new((L[i], L[j], R[j], R[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    for z in (0.40, 0.92, 0.95):                                       # grime | mint | chrome stripe | cream
        bmesh.ops.bisect_plane(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:], dist=1e-5, plane_co=(0, 0, z),
                               plane_no=(0, 0, 1))
    bm.normal_update()
    for f in bm.faces:
        c = f.calc_center_median()
        f.material_index = (4 if f.normal.z < -0.7 else 3 if c.z < 0.40 else 1 if c.z < 0.92 else
                            2 if c.z < 0.95 else 0)
    ob = mk("Van_Body", bm, ["Cream", "Mint", "Chrome", "Grime", "Black"] + ["Interior"] * 5, smooth=False)

    # the openings, cut through the hollow shell
    cut = bpy.data.collections.new("VanCutters")
    bpy.context.scene.collection.children.link(cut)

    def cutter(o):
        COLL.objects.unlink(o)
        cut.objects.link(o)
        o.parent = None

    U, V, N = ws_frame()
    ls = (WS_TOP - WS_BASE).length
    s0, s1 = 0.10, ls - 0.05
    cutter(prism("cut_ws", rect(0, 0, 0.93, (s1 - s0) / 2), ws_point((s0 + s1) / 2), U, V, N, 0.5, "Black", r=0.07))
    for s in SIDES:
        cutter(prism("cut_cab", CAB_WIN, (s * (W - SHELL / 2), 0, 0), (0, 1, 0), (0, 0, 1), (1, 0, 0), 0.3, "Black",
                     r=0.06))
        for wy in WHEEL_Y:
            cutter(cyl("cut_arch", (s * TUB_X, wy, ARCH_Z), (s * 1.30, wy, ARCH_Z), ARCH_R, "Black", segs=56))
    for cx in (-0.50, 0.50):
        cutter(prism("cut_rear", rect(cx, 1.75, 0.32, 0.25), (0, YR - SHELL / 2, 0), (1, 0, 0), (0, 0, 1), (0, 1, 0),
                     0.3, "Black", r=0.06))

    bv = ob.modifiers.new("Bevel", "BEVEL")
    bv.limit_method = "ANGLE"
    bv.angle_limit = math.radians(35)
    bv.width = 0.10
    bv.segments = 6
    so = ob.modifiers.new("Shell", "SOLIDIFY")
    so.thickness = SHELL
    so.offset = -1.0
    so.use_even_offset = True
    so.material_offset = 5
    bo = ob.modifiers.new("Openings", "BOOLEAN")
    bo.operand_type = "COLLECTION"
    bo.collection = cut
    bo.solver = "EXACT"
    try:
        bo.material_mode = "TRANSFER"
    except Exception:  # noqa: BLE001
        pass

    # bake the stack into the mesh, drop the cutters
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    me = bpy.data.meshes.new_from_object(ob.evaluated_get(dg), preserve_all_data_layers=True, depsgraph=dg)
    ob.modifiers.clear()
    old = ob.data
    ob.data = me
    bpy.data.meshes.remove(old)
    me.name = "Van_Body"
    last = len(me.materials) - 1          # the bake merges the five Interior slots: inner faces index past the end
    for p in me.polygons:
        p.use_smooth = True
        p.material_index = min(p.material_index, last)
    me.set_sharp_from_angle(angle=math.radians(30))
    wn = ob.modifiers.new("WeightedNormal", "WEIGHTED_NORMAL")
    wn.mode = "FACE_AREA"
    wn.keep_sharp = True
    for o in list(cut.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    bpy.data.collections.remove(cut)
    return ob


def glass():
    U, V, N = ws_frame()
    ls = (WS_TOP - WS_BASE).length
    s0, s1 = 0.09, ls - 0.04
    prism("Van_Windshield", rect(0, 0, 0.94, (s1 - s0) / 2), ws_point((s0 + s1) / 2, -SHELL / 2), U, V, N, 0.005,
          "Fog", r=0.07)
    big = [(-2.335, 1.268), (-1.248, 1.268), (-1.248, 1.992), (-2.006, 1.992)]
    for s in SIDES:
        prism("Glass_Cab", big, (s * (W - SHELL / 2), 0, 0), (0, 1, 0), (0, 0, 1), (1, 0, 0), 0.004, "DarkGlass",
              r=0.06)
        # the quarter-vent bar: vertical from the sill to where it meets the slanted front edge
        cyl("VentBar", (s * (W - 0.012), -2.06, 1.28), (s * (W - 0.012), -2.06, 1.85), 0.008, "Chrome", segs=8)
    for cx in (-0.50, 0.50):
        prism("Glass_Rear", rect(cx, 1.75, 0.33, 0.26), (0, YR - SHELL / 2, 0), (1, 0, 0), (0, 0, 1), (0, 1, 0),
              0.004, "DarkGlass", r=0.06)


# --- wheels and arches -----------------------------------------------------------------------------------------------
def wheel(s, wy):
    cx = s * WHEEL_X
    turn = Matrix.Translation((cx, wy, WHEEL_Z)) @ Matrix.Rotation(0.0 if s > 0 else math.pi, 4, "Z")
    bm = lathe("tire", fillet([(-0.105, 0.205), (0.105, 0.205), (0.105, WHEEL_R), (-0.105, WHEEL_R)], 0.045, 4),
               "Rubber")
    for f in bm.faces:
        c = f.calc_center_median()
        f.material_index = 1 if (c.x > 0.09 and 0.255 < math.hypot(c.y, c.z) < 0.31) else 0
    bmesh.ops.transform(bm, matrix=turn, verts=bm.verts)
    mk("Tire", bm, ["Rubber", "Whitewall"])
    cyl("Rim", (cx, wy, WHEEL_Z), (cx + s * 0.095, wy, WHEEL_Z), 0.215, "Steel", segs=32)
    ell("Hubcap", (cx + s * 0.097, wy, WHEEL_Z), (0.03, 0.155, 0.155), "Chrome", segs=24)


def liner(s, wy):
    """The wheel well and tub: the top of a cylinder round the axle, from the tub wall out to the shell, 12 mm thick so
    it also reads from the cargo side."""
    t0 = math.asin((ZB - ARCH_Z) / ARCH_R)
    ts = [t0 + (math.pi - 2 * t0) * k / 32 for k in range(33)]
    arc = [(wy + ARCH_R * math.cos(t), ARCH_Z + ARCH_R * math.sin(t)) for t in ts]
    xa, xb = s * TUB_X, s * (W - SHELL + 0.003)
    bm = bmesh.new()
    A = [bm.verts.new((xa, y, z)) for y, z in arc]
    B = [bm.verts.new((xb, y, z)) for y, z in arc]
    for i in range(len(arc) - 1):
        f = bm.faces.new((A[i], A[i + 1], B[i + 1], B[i]))
        f.normal_update()
        mid = f.calc_center_median()
        if f.normal.dot(Vector((mid.x, wy, ARCH_Z)) - mid) < 0:
            f.normal_flip()
    f = bm.faces.new(A)
    f.normal_update()
    if f.normal.x * s < 0:
        f.normal_flip()
    bmesh.ops.solidify(bm, geom=bm.faces[:], thickness=0.012)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    mk("WheelWell", bm, "Black", smooth=False)


def arch_trim(s, wy):
    """A chrome moulding round the arch on the flank."""
    t0 = math.asin((0.40 - ARCH_Z) / ARCH_R)
    ts = [t0 + (math.pi - 2 * t0) * k / 40 for k in range(41)]
    r0, r1 = ARCH_R, ARCH_R + 0.028
    bm = bmesh.new()
    rings = []
    for t in ts:
        c, sn = math.cos(t), math.sin(t)
        rings.append([bm.verts.new((s * x, wy + r * c, ARCH_Z + r * sn))
                      for (x, r) in ((W - 0.001, r0), (W - 0.001, r1), (W + 0.007, r1), (W + 0.007, r0))])
    for i in range(len(rings) - 1):
        a, b = rings[i], rings[i + 1]
        for k in range(4):
            bm.faces.new((a[k], a[(k + 1) % 4], b[(k + 1) % 4], b[k]))
    bm.faces.new(rings[0])
    bm.faces.new(rings[-1])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    mk("ArchTrim", bm, "Chrome")


# --- front, flanks, rear, roof ---------------------------------------------------------------------------------------
def front():
    yf = -2.66
    box("Grille", -0.50, 0.50, yf - 0.012, yf + 0.01, 0.56, 0.96, "Black", bev=0.008)
    for (x0, x1, z0, z1) in ((-0.52, 0.52, 0.955, 0.975), (-0.52, 0.52, 0.545, 0.565),
                             (-0.52, -0.50, 0.545, 0.975), (0.50, 0.52, 0.545, 0.975)):
        box("GrilleFrame", x0, x1, yf - 0.022, yf + 0.005, z0, z1, "Chrome", bev=0.004)
    for i in range(7):
        z = 0.60 + i * 0.05
        box("GrilleBar", -0.48, 0.48, yf - 0.02, yf - 0.012, z, z + 0.014, "Chrome", bev=0.003)
    for s in SIDES:
        hx = s * 0.78
        cyl("HeadlightBezel", (hx, yf + 0.01, 0.80), (hx, yf - 0.035, 0.80), 0.10, "Chrome", segs=32)
        ell("Headlight", (hx, yf - 0.035, 0.80), (0.085, 0.02, 0.085), "Headlight", segs=24)
        box("Indicator", hx - 0.07, hx + 0.07, yf - 0.014, yf + 0.01, 0.60, 0.665, "Amber", bev=0.008)
        # marker lamp and quarter-panel badge on each front corner
        box("MarkerFront", s * (W - 0.002), s * (W + 0.008), -2.35, -2.27, 0.99, 1.04, "Amber", bev=0.003)
    box("BumperFront", -1.10, 1.10, -2.80, -2.56, 0.30, 0.45, "Chrome", bev=0.035, segs=4)
    box("BumperStripFront", -1.06, 1.06, -2.808, -2.79, 0.36, 0.40, "Rubber", bev=0.004)
    box("PlateFront", -0.24, 0.24, -2.822, -2.80, 0.32, 0.44, "Black", bev=0.006)
    U, V, N = ws_frame()
    for (px, length) in ((0.48, 0.52), (-0.30, 0.52)):                 # parked wipers along the base
        a = ws_point(0.11, 0.012) + Vector((px, 0, 0))
        b = ws_point(0.14, 0.012) + Vector((px - length, 0, 0))
        cyl("Wiper", a, b, 0.008, "Black", segs=8)
        cyl("WiperPivot", ws_point(0.11, 0.0) + Vector((px, 0, 0)), a + N * 0.01, 0.014, "Black", segs=12)


def flank(s):
    # door seams
    strip_v("SeamCabRear", s, -1.20, 0.42, 2.18)
    strip_v("SeamCabFront", s, -2.42, 0.42, 1.12)
    strip_h("SeamCabTop", s, -1.97, -1.20, 2.18)
    strip_h("SeamCabBottom", s, -2.42, -1.20, 0.42)
    box("HandleCab", s * (W - 0.002), s * (W + 0.016), -1.37, -1.25, 1.17, 1.20, "Chrome", bev=0.006)
    box("CabStep", s * (W - 0.05), s * (W + 0.10), -2.12, -1.45, 0.30, 0.325, "Chrome", bev=0.005)
    # the rain gutter along the roof side
    cyl("DripRail", side(s, -1.95, 2.215, 0.004), side(s, 2.50, 2.215, 0.004), 0.008, "Chrome", segs=10)
    # mirror: two arms off the door to a tall black head with its glass facing back
    cyl("MirrorArm", side(s, -2.02, 1.86), side(s, -2.07, 1.84, 0.16), 0.008, "Chrome", segs=10)
    cyl("MirrorArm", side(s, -2.10, 1.44), side(s, -2.07, 1.52, 0.16), 0.008, "Chrome", segs=10)
    box("MirrorHead", s * (W + 0.12), s * (W + 0.24), -2.10, -2.05, 1.48, 1.88, "Black", bev=0.012)
    box("MirrorGlass", s * (W + 0.128), s * (W + 0.232), -2.05, -2.046, 1.49, 1.87, "Chrome")
    # marker lamps mid and rear, mud flap behind the rear wheel
    box("MarkerMid", s * (W - 0.002), s * (W + 0.008), 0.40, 0.48, 1.00, 1.05, "Amber", bev=0.003)
    box("MarkerRear", s * (W - 0.002), s * (W + 0.008), 2.40, 2.48, 2.08, 2.13, "Tail", bev=0.003)
    box("MudFlap", s * 0.84, s * 1.04, 1.99, 2.005, 0.07, 0.38, "Rubber", bev=0.004)
    if s < 0:   # kerb side: the sliding door, its handle and the track behind it
        strip_v("SeamSlideFront", s, -1.12, 0.42, 2.18)
        strip_v("SeamSlideRear", s, -0.02, 0.42, 2.18)
        strip_h("SeamSlideTop", s, -1.12, -0.02, 2.18)
        box("HandleSlide", s * (W - 0.002), s * (W + 0.016), -1.06, -0.96, 1.17, 1.20, "Chrome", bev=0.006)
        box("SlideTrack", s * (W - 0.002), s * (W + 0.012), -0.02, 0.95, 1.29, 1.32, "Chrome", bev=0.004)
    else:       # driver's side: the fuel cap
        cyl("FuelCap", side(s, 1.10, 0.70, -0.002), side(s, 1.10, 0.70, 0.010), 0.055, "Chrome", segs=24)


def lettering(s):
    def text(name, body_text, font, size, y, z, mat, space=1.0):
        cu = bpy.data.curves.new(name, "FONT")
        cu.body = body_text
        cu.font = bpy.data.fonts.load(font, check_existing=True)
        cu.size = size
        cu.extrude = 0.0          # flat: painted lettering, and extruded glyphs cost ~10x the triangles
        cu.resolution_u = 2
        cu.align_x = "CENTER"
        cu.align_y = "BOTTOM_BASELINE"
        cu.space_character = space
        cu.materials.append(bpy.data.materials[mat])
        ob = bpy.data.objects.new(name, cu)
        COLL.objects.link(ob)
        ob.parent = ROOT
        a, b, c = Vector((0, s, 0)), Vector((0, 0, 1)), Vector((s, 0, 0))    # reads toward the tail on the left
        rot = Matrix(((a.x, b.x, c.x), (a.y, b.y, c.y), (a.z, b.z, c.z))).to_4x4()
        ob.matrix_world = Matrix.Translation((s * (W + 0.003), y, z)) @ rot
        return ob

    yc = 0.40
    t = text("Txt_Title", "Fleurs du Soir", FONT_TITLE, 0.48, yc, 1.66, "Ink")
    text("Txt_Line", "LIVRAISON NUIT  \u00b7  T\u00c9L. 26-14-78", FONT_LINE, 0.072, yc, 1.42, "Ink", space=1.15)
    box("Underline", s * (W - 0.002), s * (W + 0.0022), yc - 1.05, yc + 1.05, 1.585, 1.594, "Ink")
    return t


def bouquet(s):
    """The painted lilies near the tail: seven stems fanned from a red bow."""
    by, bz = 2.10, 1.08
    for k in range(7):
        a = math.radians(-30 + 10 * k)
        ln = 0.60 + (0.10 if k % 2 else 0.0)
        tip = (by + math.sin(a) * ln, bz + math.cos(a) * ln)
        mid = (by + math.sin(a) * ln * 0.5 + 0.015 * math.cos(a), bz + math.cos(a) * ln * 0.5)
        cyl("Stem", side(s, by, bz, 0.003), side(s, *mid, 0.003), 0.0055, "Stem", segs=8)
        cyl("Stem", side(s, *mid, 0.003), side(s, *tip, 0.003), 0.0055, "Stem", segs=8)
        for j in range(5):
            ph = a + math.radians(j * 72)
            c = side(s, tip[0] + math.sin(ph) * 0.042, tip[1] + math.cos(ph) * 0.042, 0.004)
            ell("Petal", c, (0.0025, 0.017, 0.048), "Petal", Matrix.Rotation(-ph, 3, "X"), segs=10)
        ell("Stamen", side(s, *tip, 0.0065), (0.003, 0.016, 0.016), "Stamen", segs=10)
        if k in (1, 4, 6):
            ang = a + (0.75 if k < 4 else -0.75)
            c = (by + math.sin(a) * ln * 0.4 + math.sin(ang) * 0.05, bz + math.cos(a) * ln * 0.4 + math.cos(ang) * 0.05)
            ell("Leaf", side(s, *c, 0.003), (0.0025, 0.024, 0.075), "Leaf", Matrix.Rotation(-ang, 3, "X"), segs=10)
    ell("Bow", side(s, by, bz + 0.01, 0.006), (0.004, 0.026, 0.022), "Red", segs=12)
    for sg in (-1, 1):
        ell("BowLoop", side(s, by + sg * 0.05, bz + 0.03, 0.005), (0.003, 0.022, 0.05), "Red",
            Matrix.Rotation(-sg * math.radians(62), 3, "X"), segs=10)
        ell("BowTail", side(s, by + sg * 0.03, bz - 0.06, 0.005), (0.003, 0.014, 0.065), "Red",
            Matrix.Rotation(-sg * math.radians(160), 3, "X"), segs=10)


def rear():
    yr = YR
    box("SeamRearSplit", -0.004, 0.004, yr - 0.002, yr + 0.0025, 0.42, 2.20, "Black")
    box("SeamRearTop", -0.93, 0.93, yr - 0.002, yr + 0.0025, 2.196, 2.204, "Black")
    for sx in (-1, 1):
        box("SeamRearSide", sx * 0.926, sx * 0.934, yr - 0.002, yr + 0.0025, 0.42, 2.20, "Black")
        for hz in (0.75, 1.95):
            box("Hinge", sx * 0.90, sx * 0.97, yr - 0.002, yr + 0.02, hz - 0.04, hz + 0.04, "Black", bev=0.006)
        box("Tail", sx * 0.80, sx * 0.91, yr - 0.002, yr + 0.016, 0.72, 0.98, "Tail", bev=0.008)
        box("TailAmber", sx * 0.80, sx * 0.91, yr - 0.002, yr + 0.016, 0.60, 0.71, "Amber", bev=0.008)
        box("TailBezel", sx * 0.79, sx * 0.92, yr - 0.002, yr + 0.008, 0.59, 0.99, "Chrome", bev=0.006)
    box("HandleRear", 0.04, 0.20, yr - 0.002, yr + 0.02, 1.18, 1.21, "Chrome", bev=0.006)
    box("BumperRear", -1.08, 1.08, 2.50, 2.74, 0.30, 0.44, "Chrome", bev=0.035, segs=4)
    box("BumperStripRear", -1.04, 1.04, 2.738, 2.748, 0.35, 0.39, "Rubber", bev=0.004)
    box("PlateRear", -0.24, 0.24, yr - 0.002, yr + 0.012, 0.46, 0.57, "Black", bev=0.006)
    cyl("Exhaust", (-0.55, 1.90, 0.22), (-0.55, 2.70, 0.22), 0.026, "Steel", segs=16)


def plates():
    def plate_text(name, y, z, rot_z):
        cu = bpy.data.curves.new(name, "FONT")
        cu.body = "4178 FS 75"
        cu.font = bpy.data.fonts.load(FONT_LINE, check_existing=True)
        cu.size = 0.075
        cu.extrude = 0.0
        cu.resolution_u = 2
        cu.align_x = "CENTER"
        cu.align_y = "CENTER"
        cu.space_character = 1.05
        cu.materials.append(bpy.data.materials["Petal"])
        ob = bpy.data.objects.new(name, cu)
        COLL.objects.link(ob)
        ob.parent = ROOT
        ob.matrix_world = Matrix.Translation((0, y, z)) @ Matrix.Rotation(rot_z, 4, "Z") @ Matrix.Rotation(
            math.radians(90), 4, "X")
    plate_text("Txt_PlateFront", -2.825, 0.38, 0.0)            # faces -Y
    plate_text("Txt_PlateRear", YR + 0.015, 0.515, math.pi)    # faces +Y


def roof():
    box("RoofVent", -0.30, 0.30, 1.00, 1.50, ROOF - 0.01, ROOF + 0.09, "Steel", bev=0.02)
    box("RoofVentLid", -0.32, 0.32, 0.98, 1.52, ROOF + 0.09, ROOF + 0.11, "Steel", bev=0.008)
    ax0, ax1 = RIG_X * 0.80, RIG_X * 0.86
    cyl("AntennaBase", (ax0, 2.20, ROOF - 0.01), (ax0, 2.20, ROOF + 0.05), 0.04, "Black", segs=16, r1=0.03)
    cyl("Antenna", (ax0, 2.20, ROOF + 0.05), (ax1, 2.24, ROOF + 1.50), 0.004, "Chrome", segs=6)
    ell("AntennaTip", (ax1, 2.24, ROOF + 1.50), (0.008, 0.008, 0.008), "Chrome", segs=8)
    # the dish on its post, aimed at the theatre
    post0, post1 = Vector((RIG_X * 0.65, 0.40, ROOF - 0.01)), Vector((RIG_X * 0.65, 0.40, ROOF + 0.30))
    cyl("DishPost", post0, post1, 0.022, "Steel", segs=12)
    box("DishFoot", RIG_X * 0.57, RIG_X * 0.73, 0.32, 0.48, ROOF - 0.01, ROOF + 0.02, "Steel", bev=0.006)
    k = 0.08 / 0.23 ** 2
    rs = [0.015, 0.04, 0.07, 0.10, 0.13, 0.16, 0.19, 0.215, 0.23]
    prof = [(k * r * r, r) for r in rs] + [(k * r * r - 0.012, r) for r in reversed(rs)]
    bm = lathe("dish", prof, "Beige", steps=40, axis=(0, 0, 1))
    vertex = post1 + DISH_AIM * 0.03
    bmesh.ops.transform(bm, matrix=Matrix.Translation(vertex) @ DISH_AIM.to_track_quat("Z", "Y").to_matrix().to_4x4(),
                        verts=bm.verts)
    mk("Dish", bm, "Beige")
    cyl("DishFeedArm", vertex, vertex + DISH_AIM * 0.22, 0.006, "Steel", segs=8)
    cyl("DishFeed", vertex + DISH_AIM * 0.20, vertex + DISH_AIM * 0.25, 0.02, "Black", segs=12)
    # the cable from the dish across the roof and down the back corner
    cu = bpy.data.curves.new("Cable", "CURVE")
    cu.dimensions = "3D"
    cu.bevel_depth = 0.006
    cu.bevel_resolution = 2
    pts = [(RIG_X * x, y, z) for x, y, z in (
        (0.65, 0.42, ROOF + 0.006), (0.78, 1.00, ROOF + 0.006), (0.90, 2.40, ROOF + 0.006), (0.94, 2.565, 2.335),
        (0.97, YR + 0.007, 2.22), (0.97, YR + 0.007, 1.40), (0.90, YR + 0.007, 1.26))]
    sp = cu.splines.new("POLY")
    sp.points.add(len(pts) - 1)
    for i, p in enumerate(pts):
        sp.points[i].co = (*p, 1.0)
    cu.materials.append(bpy.data.materials["Black"])
    ob = bpy.data.objects.new("Cable", cu)
    COLL.objects.link(ob)
    ob.parent = ROOT


def underbody():
    box("Chassis", -0.75, 0.75, -2.20, 2.30, 0.16, ZB + 0.01, "Black")
    for wy in WHEEL_Y:
        cyl("Axle", (-0.85, wy, WHEEL_Z), (0.85, wy, WHEEL_Z), 0.04, "Black", segs=12)


# --- the studio, only for looking at it ------------------------------------------------------------------------------
def studio():
    c = fresh_collection("VanStudio")
    bm = bmesh.new()
    vs = [bm.verts.new((x, y, 0.0)) for x, y in ((-20, -20), (20, -20), (20, 20), (-20, 20))]
    bm.faces.new(vs)
    me = bpy.data.meshes.new("VanGround")
    bm.to_mesh(me)
    bm.free()
    me.materials.append(bpy.data.materials["VanGround"])
    g = bpy.data.objects.new("VanGround", me)
    c.objects.link(g)
    sun = bpy.data.lights.new("VanKey", "SUN")
    sun.energy = 3.5
    sun.angle = math.radians(3)
    so = bpy.data.objects.new("VanKey", sun)
    so.rotation_euler = (math.radians(50), 0.0, math.radians(30))
    c.objects.link(so)
    scn = bpy.context.scene
    if scn.world is None:
        scn.world = bpy.data.worlds.new("World")
    w = scn.world
    try:
        w.use_nodes = True
    except Exception:  # noqa: BLE001
        pass
    bg = next((n for n in w.node_tree.nodes if n.type == "BACKGROUND"), None)
    if bg:
        bg.inputs[0].default_value = (0.36, 0.39, 0.44, 1.0)
        bg.inputs[1].default_value = 0.9
    for eng in ("BLENDER_EEVEE", "BLENDER_EEVEE_NEXT"):
        try:
            scn.render.engine = eng
            break
        except TypeError:
            pass
    for name in ("Cube", "Light"):
        o = bpy.data.objects.get(name)
        if o:
            bpy.data.objects.remove(o, do_unlink=True)


def export(out_dir=REPO + r"\Reel\SourceArt\FloristVan"):
    """Bake the van into two meshes and write them as FBX: SM_FloristVan (everything but the windshield) and
    SM_FloristVan_Windshield. Unreal's FBX import maps Blender (x, y, z) to (x, -y, z) cm; the set script then turns the
    actor 180 degrees, so in the level a Blender point lands at (-x, y, z): nose south, Blender -X on the east side."""
    import os
    os.makedirs(out_dir, exist_ok=True)
    coll = bpy.data.collections["FloristVan"]
    vl = bpy.context.view_layer
    vl.update()
    dg = bpy.context.evaluated_depsgraph_get()
    temps = []
    for o in coll.objects:
        if o.type not in {"MESH", "CURVE", "FONT"}:
            continue
        me = bpy.data.meshes.new_from_object(o.evaluated_get(dg), preserve_all_data_layers=True, depsgraph=dg)
        t = bpy.data.objects.new("tmp_" + o.name, me)
        t.matrix_world = o.matrix_world.copy()
        bpy.context.scene.collection.objects.link(t)
        temps.append(t)
    ws = [t for t in temps if t.name == "tmp_Van_Windshield"]
    van = [t for t in temps if t.name == "tmp_Van_Body"] + [t for t in temps
                                                             if t.name not in ("tmp_Van_Windshield", "tmp_Van_Body")]
    out = []
    for objs, name in ((van, "SM_FloristVan"), (ws, "SM_FloristVan_Windshield")):
        for o in vl.objects:
            o.select_set(False)
        for o in objs:
            o.select_set(True)
        vl.objects.active = objs[0]
        if len(objs) > 1:
            with bpy.context.temp_override(active_object=objs[0], object=objs[0], selected_objects=objs,
                                           selected_editable_objects=objs):
                bpy.ops.object.join()
        o = objs[0]
        o.name = o.data.name = name
        path = os.path.join(out_dir, name + ".fbx")
        bpy.ops.export_scene.fbx(filepath=path, use_selection=True, object_types={"MESH"}, use_mesh_modifiers=True,
                                 mesh_smooth_type="FACE", add_leaf_bones=False, bake_anim=False, axis_forward="-Z",
                                 axis_up="Y", apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE")
        out.append("%s: %d tris, slots %s" % (name, sum(len(p.vertices) - 2 for p in o.data.polygons),
                                              [m.name for m in o.data.materials]))
        bpy.data.objects.remove(o, do_unlink=True)
    print("\n".join(out))


def main():
    global COLL, ROOT
    materials()
    COLL = fresh_collection("FloristVan")
    ROOT = bpy.data.objects.new("FloristVan", None)
    ROOT.empty_display_size = 0.5
    COLL.objects.link(ROOT)
    old = bpy.data.collections.get("VanCutters")
    if old:
        for o in list(old.objects):
            bpy.data.objects.remove(o, do_unlink=True)
        bpy.data.collections.remove(old)
    b = body()
    glass()
    for s in SIDES:
        for wy in WHEEL_Y:
            wheel(s, wy)
            liner(s, wy)
            arch_trim(s, wy)
        flank(s)
        t = lettering(s)
        bouquet(s)
    front()
    rear()
    plates()
    roof()
    underbody()
    studio()
    bpy.context.view_layer.update()
    print("van: %d objects; body %d faces, %d materials; title %.2f x %.2f m" % (
        len(COLL.objects), len(b.data.polygons), len(b.data.materials), t.dimensions.x, t.dimensions.y))


if __name__ == "__main__":
    main()
