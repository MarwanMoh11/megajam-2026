"""Pictures in every frame, every frame flush on its wall (wave 3, agent pictures; tag set_pictures).

Runs after the other overlays (run_all.py). Finds everything at run time, never by fixed coordinates:
- Rodin frames (SM_Frame instances, whoever placed them: import_props over build_auction's Frame_ boxes, front.py's
  lobby walls): a trace finds the wall behind each one and the instance is moved along the wall's normal so its back
  sits GAP cm in front of the wall surface (never inside it). Frames that held a flat-colour picture (build_auction's
  Picture_ boxes, which this hides, or front's MI_Art_ boxes) get a picture plane just behind the moulding. Frames
  holding something else (front's mirrors, the one-sheet with its lettering, the lots board) are only made flush.
- rooms.py's framed canvases (Set_Rooms_*_canvas instances shaped like a framed picture): a picture plane on the canvas.
- front.py's poster cases on the street: the film's poster and a lobby card, lit from inside like a lightbox.
Pictures come from tools/build_pictures.py (/Game/Take/Pictures/MI_Pic_*). The frame's aspect goes to the plane's
custom primitive data 0 (the material crops the square picture to it); no two frames within NEAR cm share a picture.
Switch off: remove its line from run_all.py, or C.clear("set_pictures") (frames stay where they were moved).
"""
import importlib
import math
import re
import sys

import unreal

sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C  # noqa: E402

importlib.reload(C)

TAG = "set_pictures"
DEST = "/Game/Take/Pictures"
FRAME_MESH = "/Game/Take/Props/Frame/SM_Frame"
PLANE = "/Engine/BasicShapes/Plane"
ON = dict(flush=True, rodin=True, rooms=True, cases=True)
OPENING = (0.80, 0.80)    # picture size / a Rodin frame's outer size (along the wall, up): the rest hides under the moulding
GAP = 0.3                 # cm from the wall surface to a frame's back
PIC_OFF = 0.8             # cm from the wall surface to the picture
NEAR = 900.0              # cm: frames closer than this never share a picture
FLIP_U, FLIP_V = False, False
CASE_GLOW = 1.6           # the poster cases' lightbox (emissive multiplier on the picture)
PORTRAIT = ["por_julian", "por_maya", "por_vex", "poster", "st_office", "st_gallery", "st_lobby", "st_projection"]
LANDSCAPE = ["st_street", "st_lobby", "st_hall", "st_trio", "card_lobby", "st_office", "st_projection", "card_street",
             "st_stock", "st_backstage", "card_stock", "st_gallery", "por_julian", "por_maya", "por_vex"]
SQUAREISH = 1.25          # portraits go into landscape frames only up to this aspect

world = C.world()
report = []


def mesh_path(c):
    m = c.get_editor_property("static_mesh")
    return m.get_path_name().split(".")[0] if m else ""


def corners_aabb(xf, lo, hi):
    pts = [xf.transform_location(unreal.Vector(x, y, z)) for x in (lo.x, hi.x) for y in (lo.y, hi.y)
           for z in (lo.z, hi.z)]
    return [min(p.x for p in pts), max(p.x for p in pts), min(p.y for p in pts), max(p.y for p in pts),
            min(p.z for p in pts), max(p.z for p in pts)]


def local_box(mesh):
    b = mesh.get_bounding_box()
    return b.min, b.max


def ism_instances(pred):
    """[(actor, component, index, transform, aabb)] for instanced components whose mesh path passes pred."""
    out = []
    for a in C.actors():
        for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
            p = mesh_path(c)
            if not pred(a, c, p):
                continue
            lo, hi = local_box(c.get_editor_property("static_mesh"))
            for i in range(c.get_instance_count()):
                xf = c.get_instance_transform(i, True)
                out.append((a, c, i, xf, corners_aabb(xf, lo, hi)))
    return out


def trace(a, b, ignore):
    h = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(*a), unreal.Vector(*b),
                                               unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, ignore,
                                               unreal.DrawDebugTrace.NONE, True)
    if h is None:
        return None
    t = h.to_tuple()
    return (t[4].x, t[4].y, t[4].z) if t[0] else None


def wall_side(box, ignore):
    """(axis, s, face): the wall behind a thing hung on it. axis 0 = the wall's normal is X; s = +1 when the room is
    on the + side; face = the wall surface's coordinate on that axis. None when no wall is found."""
    ax = 0 if (box[1] - box[0]) < (box[3] - box[2]) else 1
    c = [(box[0] + box[1]) / 2, (box[2] + box[3]) / 2, (box[4] + box[5]) / 2]
    half = (box[2 * ax + 1] - box[2 * ax]) / 2
    best = None
    for s in (1, -1):
        st, en = list(c), list(c)
        st[ax] += s * 60.0
        en[ax] -= s * 60.0
        h = trace(st, en, ignore)
        if h is None:
            continue
        d_hit = abs(st[ax] - h[ax])
        if 60.0 - half - 1.0 < d_hit and (best is None or abs(h[ax] - c[ax]) < abs(best[2] - c[ax])):
            best = (ax, s, h[ax])
    return best


def inside(p, box, grow=10.0):
    return all(box[2 * k] - grow <= p[k] <= box[2 * k + 1] + grow for k in range(3))


def centre(b):
    return ((b[0] + b[1]) / 2, (b[2] + b[3]) / 2, (b[4] + b[5]) / 2)


def mi(name, glow=None):
    p = DEST + "/MI_Pic_" + name
    if glow is None:
        return C.eal.load_asset(p) if C.eal.does_asset_exist(p) else None
    lit = DEST + "/MI_Pic_%s_Lit" % name
    if not C.eal.does_asset_exist(lit) and C.eal.does_asset_exist(p):   # a lightbox copy, made once
        C.eal.duplicate_asset(p, lit)
        m = C.eal.load_asset(lit)
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(m, "Glow", glow)
        unreal.MaterialEditingLibrary.update_material_instance(m)
        C.eal.save_asset(lit)
    return C.eal.load_asset(lit) if C.eal.does_asset_exist(lit) else None


def picture(loc, ax, s, w, h, mat, aspect, seed, label):
    """A plane facing the room (normal s along axis ax), w along the wall, h tall, centred on loc."""
    n = [0.0, 0.0, 0.0]
    n[ax] = float(s)
    f = unreal.Vector(-n[0], -n[1], 0.0)                     # the viewer looks at the wall
    up = unreal.Vector(0.0, 0.0, 1.0)
    right = up.cross(f)
    rot = unreal.MathLibrary.make_rot_from_xy(right, unreal.Vector(0.0, 0.0, -1.0))
    a = C.eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*loc), rot)
    a.set_actor_label("Set_pictures_" + label)
    a.set_folder_path("Sets/" + TAG)
    a.set_editor_property("tags", [unreal.Name(TAG)])
    c = a.static_mesh_component
    c.set_static_mesh(C.eal.load_asset(PLANE))
    c.set_material(0, mat)
    c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    c.set_editor_property("cast_shadow", False)
    a.set_actor_scale3d(unreal.Vector(w / 100.0 * (-1 if FLIP_U else 1), h / 100.0 * (-1 if FLIP_V else 1), 1.0))
    c.set_custom_primitive_data_float(0, float(aspect))
    c.set_custom_primitive_data_float(1, float(seed))
    return a


def hide_actor(a):
    """C.hide for one actor (its label may be shared): clear(TAG) shows it again."""
    ts = C.tags(a)
    if any(t.startswith(TAG + ":hid=") for t in ts):
        return
    comps = [c for c in a.get_components_by_class(unreal.PrimitiveComponent) if c.is_visible()]
    for c in comps:
        c.set_visibility(False, False)
    C._set_tags(a, ts + ["%s:hid=%s" % (TAG, "|".join(c.get_name() for c in comps))])


# --- run -----------------------------------------------------------------------------------------------------------
gone, shown = C.clear(TAG)
pics = C.find(r"Picture_\d+")
pic_boxes = [C.bounds(p) for p in pics]
C.hide([r"Picture_\d+"], TAG)
art = ism_instances(lambda a, c, p: any(re.match(r"MI_Art_\d", (m.get_name() if m else ""))
                                        for m in c.get_materials()))
frames = ism_instances(lambda a, c, p: p == FRAME_MESH) if C.eal.does_asset_exist(FRAME_MESH) else []
ignore = list({id(f[0]): f[0] for f in frames}.values()) + pics
slots = []                                                  # (loc, ax, s, w, h, aspect, label, kind)
moved = flushless = 0
for (a, c, i, xf, b) in frames:
    ws = wall_side(b, ignore)
    if ws is None:
        flushless += 1
        continue
    ax, s, face = ws
    back = b[2 * ax] if s > 0 else b[2 * ax + 1]
    delta = (face + s * GAP) - back
    if ON["flush"] and abs(delta) > 0.05:
        t = xf.translation
        t = unreal.Vector(t.x + (delta if ax == 0 else 0.0), t.y + (delta if ax == 1 else 0.0), t.z)
        c.update_instance_transform(i, unreal.Transform(t, xf.rotation.rotator(), xf.scale3d), True, True, True)
        b = list(b)
        b[2 * ax] += delta
        b[2 * ax + 1] += delta
        moved += 1
    cen = centre(b)
    filled = any(inside(centre(pb), b) for pb in pic_boxes)
    art_front = None
    for (_, _, _, _, ab) in art:
        if inside(centre(ab), b):
            filled = True
            fr = ab[2 * ax + 1] if s > 0 else ab[2 * ax]
            art_front = fr if art_front is None else (max(art_front, fr) if s > 0 else min(art_front, fr))
    if not (ON["rodin"] and filled):
        continue
    along = 1 - ax
    w = (b[2 * along + 1] - b[2 * along]) * OPENING[0]
    h = (b[5] - b[4]) * OPENING[1]
    d = face + s * PIC_OFF
    if art_front is not None:
        d = max(d, art_front + 0.3) if s > 0 else min(d, art_front - 0.3)
    loc = list(cen)
    loc[ax] = d
    slots.append((loc, ax, s, w, h, w / h, "frame", "rodin"))

if ON["rooms"]:
    canv = ism_instances(lambda a, c, p: bool(re.match(r"Set_Rooms_.*_canvas(_dk)?$", a.get_actor_label())))
    room_ignore = []
    for (a, c, i, xf, b) in canv:
        ext = [b[1] - b[0], b[3] - b[2], b[5] - b[4]]
        ax = 0 if ext[0] < ext[1] else 1
        along = 1 - ax
        r = xf.rotation.rotator()
        if not (3.5 <= ext[ax] <= 6.5 and 35 <= ext[along] <= 260 and 35 <= ext[2] <= 260 and b[4] > 40
                and abs(r.roll) < 1 and abs(r.pitch) < 1 and abs(((r.yaw + 45) % 90) - 45) < 1):
            continue
        if not room_ignore:
            room_ignore = [x for x in C.actors() if x.get_actor_label().startswith("Set_Rooms_")]
        ws = wall_side(b, room_ignore + ignore)
        if ws is None:
            continue
        _, s, _ = ws
        loc = list(centre(b))
        loc[ax] = (b[2 * ax + 1] + 0.3) if s > 0 else (b[2 * ax] - 0.3)
        slots.append((loc, ax, s, ext[along], ext[2], ext[along] / ext[2], "canvas", "rooms"))

# --- assign: no neighbour within NEAR shares a picture; the least used first -------------------------------------
avail = {n for n in set(PORTRAIT + LANDSCAPE) if mi(n) is not None}
used = {}
placed = []
slots.sort(key=lambda sl: (round(sl[0][0] / 500.0), round(sl[0][1] / 500.0), sl[0][2]))
for k, (loc, ax, s, w, h, asp, label, kind) in enumerate(slots):
    pool = PORTRAIT if asp < 0.9 else [n for n in LANDSCAPE if not n.startswith("por_") or asp <= SQUAREISH]
    pool = [n for n in pool if n in avail]
    if not pool:
        continue
    near = {n for (p, n) in placed if math.dist(p, loc) < NEAR}
    choice = sorted(pool, key=lambda n: (n in near, used.get(n, 0), (pool.index(n) + k) % len(pool)))[0]
    used[choice] = used.get(choice, 0) + 1
    placed.append((loc, choice))
    picture(loc, ax, s, w, h, mi(choice), asp, (k * 0.618) % 1.0 * 10.0, "%s_%s" % (kind, choice))

# --- the street's poster cases: the poster and a lobby card, lit like a lightbox -----------------------------------------
cases = 0
if ON["cases"]:
    case_items = ism_instances(lambda a, c, p: p.lower().endswith("front_postercase")) + \
        [(a, a.static_mesh_component, -1, None, C.bounds(a)) for a in C.actors()
         if isinstance(a, unreal.StaticMeshActor) and mesh_path(a.static_mesh_component).lower()
         .endswith("front_postercase")]
    case_items.sort(key=lambda it: (it[4][2], it[4][0]))
    texts = [t for t in C.actors() if isinstance(t, unreal.TextRenderActor)]
    for k, (a, c, i, xf, b) in enumerate(case_items):
        name = ("poster", "card_street")[k % 2]
        m = mi(name, CASE_GLOW)
        if m is None:
            continue
        ws = wall_side(b, [a] + ignore)
        ax = 0 if (b[1] - b[0]) < (b[3] - b[2]) else 1
        s = ws[1] if ws else 1
        along = 1 - ax
        front = b[2 * ax + 1] if s > 0 else b[2 * ax]
        for t in texts:                                     # front's lettering stood in for the poster: hide it
            p = t.get_actor_location()
            if inside((p.x, p.y, p.z), b, 6.0):
                hide_actor(t)
        w = (b[2 * along + 1] - b[2 * along]) * 0.80
        h = (b[5] - b[4]) * 0.62 if name == "poster" else (b[2 * along + 1] - b[2 * along]) * 0.80 / 1.45
        h = min(h, (b[5] - b[4]) * 0.62)
        w = min(w, h * 0.70) if name == "poster" else w
        loc = list(centre(b))
        loc[2] = b[4] + (b[5] - b[4]) * 0.55
        loc[ax] = front + s * 0.6
        picture(loc, ax, s, w, h, m, w / h, k * 3.1, "case_" + name)
        cases += 1

C.save_level()
RESULT = ("pictures: frames %d (moved flush %d, no wall found %d), pictures %d (%d distinct), cases %d; cleared %d"
          % (len(frames), moved, flushless, len(placed), len(used), cases, gone))
print(RESULT)
RESULT
