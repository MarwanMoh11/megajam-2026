# The street in front of the auction house and its two-storey lobby (QA package "front", Sat Nov 14). Marwan's notes:
# 13 "stuff is floating off the table; the queue rope isn't realistic and floaty", 11 "too few props, walls repeated
# with exact mirrors", and the street's and lobby's share of 14 (designed with purpose) and 20.
# An overlay on build_auction.py + dress_auction.py + import_props.py: it clears its tag first (idempotent), hides
# (visibility only: collision, sight lines and navigation stay) the boxes it replaces, and places Rodin props and
# kitbashed dressing, all NoCollision or pawn-only (no watcher's line trace and no camera probe answers them).
# Switches: ON below (False leaves that part as the grey box had it). Run under the editor lock:
#     "<engine python>" -I tools/ue_mcp.py py tools/sets/front.py 110
# Gameplay kept clear: the three lanes, the scanner, the doorman's line down the carpet, the queue lane and the kerb the
# queue backs up along (scene1 moves the line there), the lobby route to the desk and the hall; nothing new stands within
# 1.2 m of a mark, a watcher, a device or a crew post (KEEP).
import importlib
import math
import random
import re
import sys

import unreal

sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C   # noqa: E402

importlib.reload(C)

TAG = "set_front"
ON = dict(stanchions=True, ropes=True, tables=True, cars=True, lamps=True, marquee=True, posters=True, press=True,
          street=True, carpet=True, desk=True, seating=True, palms=True, stair=True, balcony=True, walls=True, signs=True)

# --- the level's numbers (build_auction.py) ---------------------------------------------------------------------------
LX0, LX1, LY0, LY1, T, F2, RAIL_H = -3200.0, -1000.0, -1400.0, 1400.0, 20.0, 450.0, 100.0
FRONT = LX0 - T                          # the street face of the front wall
PAVE_X0 = LX0 - 900.0                    # the kerb
TERRACE_X0 = FRONT - 230.0
VOID_X0, VOID_X1, VOID_Y = -2700.0, -1500.0, 800.0
STAIR_X0, STAIR_N, STAIR_RUN, STAIR_HALF = VOID_X0 + 100.0, 30, 1100.0, 200.0
CARPET_HALF, LANE_ROPE, LANE_Y1 = 150.0, 200.0, 430.0
SCAN_X, SCAN_Y = LX0 - T - 300.0, 320.0
HALL_FACE = LX1 - T                      # the lobby's face of the hall's west wall
START = (PAVE_X0 + 60.0, 360.0)          # Julian's place in the moving queue (scene1, Nov 14)
INSIDE = ((-2750.0, 1150.0), (-2600.0, 1250.0), (-2450.0, 1150.0))   # where scanned guests stop in the lobby (scene1)
KEEP = [(START[0], START[1], 0.0),
        (SCAN_X - 270.0, 300.0, 0.0), (SCAN_X, SCAN_Y, 0.0),       # the scanner's mark and gate
        (LX0 - T - 70.0, 0.0, 0.0), (VOID_X0 + 20.0, 0.0, 0.0),    # the doorman, the chief
        (-2430.0, 630.0, 0.0),                                      # the desk mark (W3 opening: moved with the desk)
        (TERRACE_X0 + 55.0, SCAN_Y, F2), (TERRACE_X0 + 25.0, SCAN_Y + 150.0, F2)] + \
    [(x, y, 0.0) for (x, y) in INSIDE]
# Routes on the ground floor (polylines): the queue (front first, then along the kerb), the guests' way in through the
# gate and the doors to their spots inside, and Julian's way from the doors to the desk and on to the hall.
ROUTES = [[(SCAN_X - 270.0, 300.0), (SCAN_X - 355.0, 315.0), (SCAN_X - 440.0, 325.0), START, (PAVE_X0 + 85.0, 450.0),
           (PAVE_X0 + 85.0, 790.0)],
          [(SCAN_X - 90.0, SCAN_Y), (SCAN_X + 130.0, SCAN_Y), (FRONT + 160.0, SCAN_Y + 60.0), INSIDE[0]],
          [(FRONT + 160.0, SCAN_Y + 60.0), INSIDE[1]], [(FRONT + 160.0, SCAN_Y + 60.0), INSIDE[2]],
          [(LX0 + 130.0, 330.0), (-2430.0, 630.0), (-2400.0, 330.0), (LX1, 300.0)], [(-2400.0, 330.0), (LX1, -300.0)]]

# --- meshes and materials ----------------------------------------------------------------------------------------------
CUBE, CYL, SPH, CONE = ("/Engine/BasicShapes/" + n for n in ("Cube", "Cylinder", "Sphere", "Cone"))
PROPS = "/Game/Take/Props"


def mesh(n):
    return C.ip()["mesh_path"](n)


def mat(n):
    for d in ("/Game/Take/Dress/", "/Game/Take/Materials/"):
        if C.eal.does_asset_exist(d + n):
            return C.eal.load_asset(d + n)
    return None


VELVET, TRIM, GLOW, DARK, LINEN, PANEL = (mat(n) for n in ("MI_Front_Velvet", "MI_Front_Trim", "MI_Front_Sconce",
                                                           "MI_Front_Canopy", "MI_Front_Linen", "MI_Front_Panel"))
GLASS, MARBLE, RUNNER, FOLIAGE = (mat(n) for n in ("MI_Front_Glass", "MI_Front_Marble", "MI_Front_Runner",
                                                   "MI_Front_Foliage"))
GREEN, NAVY, WOOD = mat("MI_Box_Door"), mat("MI_Box_Deck"), mat("MI_Box_Sofa")
ART = [mat("MI_Art_%d" % i) for i in range(4)]


MIRROR_SHEEN = r"""
float s = UV.x + UV.y;
float b = saturate(1.0 - abs(s - 0.75) * 5.0) * 0.7 + saturate(1.0 - abs(s - 1.05) * 16.0) * 0.5;
return Sheen * b * float3(1.0, 0.97, 0.9);
"""


def mirror_mat(path="/Game/Take/Dress/M_Front_Mirror", rebuild=False):
    """Old silvered glass: metallic, nearly smooth, a little smoky, so Lumen reflects the room in it, plus a faint
    diagonal sheen (emissive) so it reads as glass even when it faces a dark room. The two mirrors used MI_Front_Glass
    (near-black, built for windows) and read as black holes in their frames (Nov 15)."""
    mel = unreal.MaterialEditingLibrary
    if C.eal.does_asset_exist(path):
        m = C.eal.load_asset(path)
        if not rebuild:
            return m
        mel.delete_all_material_expressions(m)
    else:
        d, n = path.rsplit("/", 1)
        m = unreal.AssetToolsHelpers.get_asset_tools().create_asset(n, d, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("used_with_instanced_static_meshes", True)
    uv = mel.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -500, 250)
    sh = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -500, 320)
    sh.set_editor_property("parameter_name", "Sheen")
    sh.set_editor_property("default_value", 0.12)
    cu = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -300, 250)
    cu.set_editor_property("code", MIRROR_SHEEN)
    cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    ins = []
    for nm in ("UV", "Sheen"):
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", nm)
        ins.append(ci)
    cu.set_editor_property("inputs", ins)
    mel.connect_material_expressions(uv, "", cu, "UV")
    mel.connect_material_expressions(sh, "", cu, "Sheen")
    mel.connect_material_property(cu, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    for (prop, v, y) in ((unreal.MaterialProperty.MP_BASE_COLOR, (0.62, 0.60, 0.55), -200),
                         (unreal.MaterialProperty.MP_METALLIC, 1.0, 0), (unreal.MaterialProperty.MP_ROUGHNESS, 0.07, 100)):
        if isinstance(v, tuple):
            e = mel.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -300, y)
            e.set_editor_property("constant", unreal.LinearColor(v[0], v[1], v[2], 1.0))
        else:
            e = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -300, y)
            e.set_editor_property("r", v)
        mel.connect_material_property(e, "", prop)
    mel.recompile_material(m)
    C.eal.save_asset(path)
    return m


MIRROR = mirror_mat()


def variant(prop, suffix, dark, light, rough=0.5, keep=0.0):
    """A colour variant of a Rodin prop's material: its textures on M_Prop, a band from dark to light (rgb), KeepColour
    `keep` (0: the band's own colour, 1: the texture's hue)."""
    P = C.ip()
    cap = P["cap"](prop)
    path = "%s/%s/MI_Set_%s_%s" % (PROPS, cap, cap, suffix)
    if C.eal.does_asset_exist(path):
        mi = C.eal.load_asset(path)
    else:
        mi = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "MI_Set_%s_%s" % (cap, suffix), "%s/%s" % (PROPS, cap), unreal.MaterialInstanceConstant,
            unreal.MaterialInstanceConstantFactoryNew())
    C.mel.set_material_instance_parent(mi, C.eal.load_asset(PROPS + "/M_Prop"))
    base, normal = P["textures"](prop)
    if base:
        C.mel.set_material_instance_texture_parameter_value(mi, "BaseTex", base)
    if normal:
        C.mel.set_material_instance_texture_parameter_value(mi, "NormalTex", normal)
    C.mel.set_material_instance_vector_parameter_value(mi, "Dark", P["lc"](dark))
    C.mel.set_material_instance_vector_parameter_value(mi, "Light", P["lc"](light))
    C.mel.set_material_instance_scalar_parameter_value(mi, "Roughness", float(rough))
    C.mel.set_material_instance_scalar_parameter_value(mi, "KeepColour", float(keep))
    C.mel.update_material_instance(mi)
    C.eal.save_asset(path, only_if_is_dirty=True)
    return mi


# --- the kit: basic shapes collected per (mesh, material, collision) and laid down as one instanced actor each --------
class Kit:
    def __init__(self):
        self.g = {}

    def add(self, m, mt, loc, r, sc, collide=False, shadow=False):
        self.g.setdefault((m, mt, collide, shadow), []).append((tuple(loc), r, tuple(sc)))

    def box(self, b, mt, yaw=0.0, pitch=0.0, roll=0.0, **kw):
        """b = (x0, x1, y0, y1, z0, z1) before turning; turned about its centre."""
        self.add(CUBE, mt, ((b[0] + b[1]) / 2, (b[2] + b[3]) / 2, (b[4] + b[5]) / 2), (roll, pitch, yaw),
                 ((b[1] - b[0]) / 100.0, (b[3] - b[2]) / 100.0, (b[5] - b[4]) / 100.0), **kw)

    def cube(self, c, size, mt, r=(0.0, 0.0, 0.0), **kw):
        self.add(CUBE, mt, c, r, tuple(s / 100.0 for s in size), **kw)

    def cyl(self, x, y, z0, z1, d, mt, dy=None, **kw):
        self.add(CYL, mt, (x, y, (z0 + z1) / 2), (0.0, 0.0, 0.0), (d / 100.0, (dy or d) / 100.0, (z1 - z0) / 100.0),
                 **kw)

    def ball(self, c, d, mt, dz=None, **kw):
        self.add(SPH, mt, c, (0.0, 0.0, 0.0), (d / 100.0, d / 100.0, (dz or d) / 100.0), **kw)

    def seg(self, p, q, d, mt, extend=0.0, m=CYL, **kw):
        """A cylinder (or cube) from p to q, d across."""
        dx, dy, dz = q[0] - p[0], q[1] - p[1], q[2] - p[2]
        ln = math.sqrt(dx * dx + dy * dy + dz * dz)
        pitch = math.degrees(math.atan2(dz, math.hypot(dx, dy))) - 90.0
        self.add(m, mt, ((p[0] + q[0]) / 2, (p[1] + q[1]) / 2, (p[2] + q[2]) / 2),
                 (0.0, pitch, math.degrees(math.atan2(dy, dx))), (d / 100.0, d / 100.0, (ln + extend) / 100.0), **kw)

    def flush(self):
        n = 0
        for (m, mt, collide, shadow), items in self.g.items():
            if len(items) == 1:
                C.spawn(m, items[0][0], items[0][1], items[0][2], tag=TAG, collide=collide, shadow=shadow, mat=mt)
            else:
                C.scatter(m, items, tag=TAG, collide=collide, shadow=shadow, mat=mt)
            n += len(items)
        self.g = {}
        return n


K = Kit()
LOG = []
rnd = random.Random(14)


def say(s):
    LOG.append(s)


def kept_clear(x, y, z=0.0, r=0.0, why=""):
    """False (and a note) if (x, y) is within 1.2 m (+ r) of a mark, watcher, device or crew post."""
    for (kx, ky, kz) in KEEP:
        if abs(kz - z) < 200.0 and math.hypot(kx - x, ky - y) < 120.0 + r:
            say("skipped %s at (%.0f, %.0f): near (%.0f, %.0f)" % (why, x, y, kx, ky))
            return False
    if z < 100.0:
        for route in ROUTES:
            for (ax, ay), (bx, by) in zip(route, route[1:]):
                L2 = (bx - ax) ** 2 + (by - ay) ** 2
                t = max(0.0, min(1.0, ((x - ax) * (bx - ax) + (y - ay) * (by - ay)) / L2)) if L2 else 0.0
                if math.hypot(ax + t * (bx - ax) - x, ay + t * (by - ay) - y) < 120.0 + r:
                    say("skipped %s at (%.0f, %.0f): on a route" % (why, x, y))
                    return False
    return True


def text(t, x, y, z, yaw, size, rgb=(0.80, 0.72, 0.55), pitch=0.0):
    """Words in the world, centred on (x, y, z), facing yaw (stand on that side to read them)."""
    a = C.eas.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(x, y, z), unreal.Rotator(0.0, pitch, yaw))
    a.set_actor_label("Set_front_Text")
    a.set_folder_path("Sets/" + TAG)
    c = a.text_render
    try:
        c.set_text(t)
    except Exception:                                           # noqa: BLE001
        c.set_editor_property("text", t)
    for (k, v) in (("world_size", float(size)),
                   ("text_render_color", unreal.Color(r=int(rgb[0] * 255), g=int(rgb[1] * 255), b=int(rgb[2] * 255),
                                                      a=255)),
                   ("horizontal_alignment", unreal.HorizTextAligment.EHTA_CENTER),
                   ("vertical_alignment", unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)):
        try:
            c.set_editor_property(k, v)
        except Exception:                                       # noqa: BLE001
            pass
    a.set_editor_property("tags", [unreal.Name(TAG)])
    return a


def loc_of(a):
    v = a.get_actor_location()
    return v.x, v.y, v.z


def hide_in(patterns, box):
    """C.hide restricted to actors whose location lies in box (x0, x1, y0, y1, z0, z1)."""
    rx = re.compile(r"^(%s)$" % "|".join(patterns))
    out = []
    for a in C.actors():
        if not rx.match(a.get_actor_label()) or TAG in C.tags(a):
            continue
        x, y, z = loc_of(a)
        if not (box[0] <= x <= box[1] and box[2] <= y <= box[3] and box[4] <= z <= box[5]):
            continue
        comps = [c for c in a.get_components_by_class(unreal.PrimitiveComponent) if c.is_visible()]
        if not comps:
            continue
        for c in comps:
            c.set_visibility(False, False)
        C._set_tags(a, C.tags(a) + ["%s:hid=%s" % (TAG, "|".join(c.get_name() for c in comps))])
        out.append(a)
    return out


def instances(label):
    """[(component, index, x, y, bottom z, top z)] of every instance on an import_props instanced actor."""
    out = []
    for a in C.find(label):
        for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
            bb = c.get_editor_property("static_mesh").get_bounding_box()
            for i in range(c.get_instance_count()):
                t = c.get_instance_transform(i, True)
                t = t[-1] if isinstance(t, tuple) else t
                p, s = t.translation, t.scale3d
                out.append((c, i, p.x, p.y, p.z + s.z * bb.min.z, p.z + s.z * bb.max.z))
    return out


def place(prop, box, yaw=0.0, mode="fit", mt=None, collide=False, shadow=True):
    m = mesh(prop)
    if not C.eal.does_asset_exist(m):
        say("no mesh for %s" % prop)
        return None
    loc, r, sc = C.fit(m, box, yaw, mode)
    return C.spawn(m, loc, r, sc, tag=TAG, collide=collide, shadow=shadow, mat=mt)


def scatter_fit(prop, boxes, mode="fit", mt=None, collide=False, shadow=False):
    """One instanced actor: the prop fitted into each (box, yaw)."""
    m = mesh(prop)
    if not boxes or not C.eal.does_asset_exist(m):
        return None
    items = []
    for (b, yaw) in boxes:
        loc, r, sc = C.fit(m, b, yaw, mode)
        items.append((loc, r, sc))
    return C.scatter(m, items, tag=TAG, collide=collide, shadow=shadow, mat=mt)


# ======================================================================================================================
C.clear(TAG)
FRONT_AREA = (PAVE_X0 - 100.0, LX1, -1000.0, 1000.0, -50.0, 200.0)

# --- the stanchions: a 94 cm post (import_props' stretchz), aged brass, standing on the ground ----------------------------
POSTS = []                               # (x, y) of every post in the front
HOOK = 88.0                              # rope hooks: under the post's ball top
if ON["stanchions"] or ON["ropes"]:
    gone = []
    for (c, i, x, y, z0, z1) in instances("Prop_Stanchion_Instances"):
        if FRONT_AREA[0] <= x <= FRONT_AREA[1] and abs(y) <= FRONT_AREA[3] and z0 < 50.0:
            gone.append((c, i))
    # The posts stand where the grey boxes are (hidden by import_props or not): those survive every rerun.
    for a in C.find(r"(Dress_)?Stanchion_\d+"):
        x, y, z = loc_of(a)
        if FRONT_AREA[0] <= x <= FRONT_AREA[1] and abs(y) <= FRONT_AREA[3] and z < 100.0 and \
                all(math.hypot(x - px, y - py) > 10.0 for (px, py) in POSTS):
            POSTS.append((x, y))
    hide_in([r"(Dress_)?Stanchion_\d+", r"(Dress_)?Stanchion_Foot_\d+"], FRONT_AREA)
    if ON["stanchions"]:
        # The Rodin post stood 65 cm (import_props' old "fit") and read gilt: this area's instances go, and the same
        # mesh stands here 94 cm tall (the source rule is stretchz now) in an aged-brass band. A rebuild brings the
        # instances back and this overlay, run after it, takes them away again.
        for (c, i) in sorted(gone, key=lambda ci: -ci[1]):
            c.remove_instance(i)
        say("stanchions: %d instances replaced" % len(gone))

# --- the ropes: velvet sagging from post to post where a straight bar hung -------------------------------------------------
NEW_POSTS = []
if ON["ropes"]:
    bars = hide_in([r"Rope_\d+", r"Dress_Queue_Rope_\d+"], FRONT_AREA)

    def post_at(x, y, inward):
        best = min(POSTS + NEW_POSTS, key=lambda p: math.hypot(p[0] - x, p[1] - y), default=None)
        if best and math.hypot(best[0] - x, best[1] - y) < 25.0:
            return best
        p = (x + inward[0] * 12.0, y + inward[1] * 12.0)       # a bar end with no post: one stands there now
        NEW_POSTS.append(p)
        return p

    spans = 0
    for a in bars:
        b = C.bounds(a)
        if b[1] - b[0] >= b[3] - b[2]:
            ends, inw = ((b[0], (b[2] + b[3]) / 2), (b[1], (b[2] + b[3]) / 2)), ((1, 0), (-1, 0))
        else:
            ends, inw = (((b[0] + b[1]) / 2, b[2]), ((b[0] + b[1]) / 2, b[3])), ((0, 1), (0, -1))
        pa, pb = post_at(ends[0][0], ends[0][1], inw[0]), post_at(ends[1][0], ends[1][1], inw[1])
        L = math.hypot(pb[0] - pa[0], pb[1] - pa[1])
        if L < 20.0:
            continue
        ux, uy = (pb[0] - pa[0]) / L, (pb[1] - pa[1]) / L
        h0 = (pa[0] + ux * 4.0, pa[1] + uy * 4.0, HOOK)
        h1 = (pb[0] - ux * 4.0, pb[1] - uy * 4.0, HOOK)
        sag, n = min(28.0, 0.06 * L + 4.0), max(6, min(24, int(L / 16.0)))
        pts = [(h0[0] + (h1[0] - h0[0]) * k / n, h0[1] + (h1[1] - h0[1]) * k / n, HOOK - 4.0 * sag * (k / n) * (1 - k / n))
               for k in range(n + 1)]
        for p, q in zip(pts, pts[1:]):
            K.seg(p, q, 3.6, VELVET, extend=2.2)
        for h in (h0, h1):                                       # the brass snap hook on each post
            K.ball(h, 4.6, TRIM)
        spans += 1
    POSTS.extend(NEW_POSTS)
    say("ropes: %d bars hidden, %d sagging spans, %d new posts" % (len(bars), spans, len(NEW_POSTS)))
if ON["stanchions"] and POSTS:
    brass = variant("stanchion", "Brass", (0.030, 0.020, 0.008), (0.36, 0.26, 0.11), 0.45)
    scatter_fit("stanchion", [((x - 14.0, x + 14.0, y - 14.0, y + 14.0, 0.0, 94.0), 0.0) for (x, y) in POSTS],
                mode="stretchz", mt=brass, collide=True, shadow=False)

# --- the cocktail tables: glasses and ashtrays on the table's real top ------------------------------------------------------
LOBBY = (LX0, LX1, LY0, LY1, -10.0, F2 + 300.0)
if ON["tables"]:
    tops = [(x, y, z1) for (c, i, x, y, z0, z1) in instances("Prop_Cocktail_Instances")]
    tops += [((b[0] + b[1]) / 2, (b[2] + b[3]) / 2, b[5]) for b in
             (C.bounds(a) for a in C.find(r"Cocktail_Table_\d+") if a.static_mesh_component.is_visible())]
    glasses = hide_in([r"Glass_\d+"], LOBBY)
    tables = {}
    for g in glasses:
        gx, gy, gz = loc_of(g)
        t = min(tops, key=lambda t: math.hypot(t[0] - gx, t[1] - gy), default=None)
        if t and math.hypot(t[0] - gx, t[1] - gy) < 50.0:
            tables.setdefault(t, []).append((gx, gy))
    for (tx, ty, tz), gl in tables.items():
        for (gx, gy) in gl:                                      # a coupe: foot, stem, bowl
            K.cyl(gx, gy, tz, tz + 0.6, 6.5, GLASS)
            K.cyl(gx, gy, tz + 0.6, tz + 8.0, 0.9, GLASS)
            K.ball((gx, gy, tz + 10.0), 10.5, GLASS, dz=4.5)
        a = math.atan2(sum(g[1] for g in gl) / len(gl) - ty, sum(g[0] for g in gl) / len(gl) - tx) + math.pi
        ax, ay = tx + 16.0 * math.cos(a), ty + 16.0 * math.sin(a)   # the ashtray opposite them, a cigarette on it
        K.cyl(ax, ay, tz, tz + 2.4, 11.0, GLASS)
        K.seg((ax - 3.0, ay - 1.0, tz + 2.8), (ax + 4.0, ay + 1.5, tz + 2.6), 0.9, LINEN)
        K.box((tx - 6, tx + 6, ty - 6, ty + 6, tz, tz + 0.4), LINEN, yaw=rnd.uniform(0, 90))   # a folded napkin
    say("tables: %d glasses moved onto %d table tops" % (len(glasses), len(tables)))

# --- the cars at the kerb: 1940s sedans (Rodin) over the boxes, each its own colour -------------------------------------------
SEDAN_FRONT = 90.0                       # the heading of the Rodin sedan's bonnet at yaw 0 (it is modelled along Y)
if ON["cars"]:
    cars = hide_in([r"Car_\d+", r"Car_Cabin_\d+", r"Wheel_\d+"], (-5700.0, PAVE_X0, -2400.0, 2400.0, -100.0, 300.0))
    paints = [None,
              variant("front_sedan", "Maroon", (0.010, 0.002, 0.002), (0.30, 0.06, 0.05), 0.3),
              variant("front_sedan", "Green", (0.003, 0.010, 0.005), (0.09, 0.22, 0.13), 0.3),
              variant("front_sedan", "Navy", (0.003, 0.004, 0.012), (0.08, 0.11, 0.27), 0.3),
              variant("front_sedan", "Cream", (0.03, 0.027, 0.02), (0.42, 0.38, 0.30), 0.35),
              variant("front_sedan", "Grey", (0.010, 0.010, 0.012), (0.24, 0.25, 0.27), 0.3)]
    # (x, y, length, heading: -90 = toward -Y, paint): the kerb lane runs south, the far lane north.
    for (x, y, ln, head, p) in ((PAVE_X0 - 130.0, 0.0, 600.0, -90.0, 0), (PAVE_X0 - 130.0, -1500.0, 520.0, -90.0, 1),
                                (PAVE_X0 - 130.0, -850.0, 510.0, -90.0, 5), (PAVE_X0 - 130.0, 1000.0, 520.0, -90.0, 2),
                                (PAVE_X0 - 130.0, 1700.0, 505.0, -90.0, 4), (PAVE_X0 - 1300.0, -400.0, 515.0, 90.0, 3),
                                (PAVE_X0 - 1300.0, 600.0, 520.0, 90.0, 0)):
        place("front_sedan", (x - 93.0, x + 93.0, y - ln / 2, y + ln / 2, -21.0, 152.0),
              yaw=(head - SEDAN_FRONT) % 360.0, mode="stretch", mt=paints[p], shadow=True)
    say("cars: %d boxes hidden" % len(cars))

# --- street lamps: cast-iron posts with a globe (Rodin) over the box posts -------------------------------------------------
if ON["lamps"]:
    lamps = hide_in([r"Lamp_Post_\d+", r"Lamp_Arm_\d+", r"Lamp_Head_\d+"],
                    (PAVE_X0 - 200.0, PAVE_X0 + 200.0, -2100.0, 2100.0, -50.0, 600.0))
    ys = sorted(set(round(loc_of(a)[1]) for a in lamps if "Post" in a.get_actor_label()))
    scatter_fit("front_streetlamp", [((PAVE_X0 + 40 - 40, PAVE_X0 + 40 + 40, y - 40, y + 40, 0.0, 430.0), 0.0)
                                     for y in ys], mode="stretch", shadow=True)
    say("lamps: %d posts" % len(ys))

# --- the marquee: the house's name in lights over the doors, the night's billing on the terrace's fascia -------------------
if ON["marquee"]:
    fx = TERRACE_X0 - 13.0                                       # in front of the fascia (x -3460..-3450, z 395..445)
    text("TONIGHT  \u00b7  THE DIRECTOR'S CUT  \u00b7  ONE PRINT, ONE NIGHT", fx, 0.0, 420.0, 180.0, 24.0,
         (0.86, 0.74, 0.50))
    y = -588.0
    while y <= 588.0:
        for z in (391.0, 449.0):
            K.ball((fx, y, z), 6.0, GLOW)
        y += 24.0
    sx = FRONT - 9.0                                             # the board on the facade (y -700..700, z 730..850)
    text("THE AURELIA", sx, 0.0, 808.0, 180.0, 78.0, (0.88, 0.78, 0.56))
    text("AUCTION HOUSE  \u00b7  FINE ART & RARE FILM  \u00b7  EST. 1911", sx, 0.0, 752.0, 180.0, 17.0,
         (0.72, 0.64, 0.48))
    y = -700.0
    while y <= 700.0:
        for z in (725.0, 855.0):
            K.ball((sx - 2.0, y, z), 7.0, GLOW)
        y += 28.0
    for yy in (-706.0, 706.0):
        z = 739.0
        while z <= 842.0:
            K.ball((sx - 2.0, yy, z), 7.0, GLOW)
            z += 26.0

# --- the poster cases for the lost film, on the north facade between the planters ----------------------------------------
POSTER_LINES = (("NOW SHOWING  \u00b7  ONE NIGHT", 6.0, 161.0), ("THE", 7.0, 150.0), ("DIRECTOR'S", 13.0, 139.0),
                ("CUT", 22.0, 121.0), ("1941", 9.0, 100.0), ("THE ONLY PRINT ON EARTH", 5.5, 86.0),
                ("AT AUCTION TONIGHT", 5.5, 77.0))
if ON["posters"]:
    for y in (1100.0, 1500.0):
        place("front_postercase", (FRONT - 34.0, FRONT - 6.0, y - 55.0, y + 55.0, 0.0, 205.0), yaw=90.0,
              mode="stretch", collide=True)
        for (t, s, z) in POSTER_LINES:
            text(t, FRONT - 35.5, y, z, 180.0, s, (0.36, 0.07, 0.08))

# --- the press pen: lamps on stands, two flash cameras on tripods, a crate of bulbs, spent bulbs on the pavement -------------
if ON["press"]:
    for (x, y) in ((-3985.0, -400.0), (-3615.0, -400.0)):
        if kept_clear(x, y, why="press lamp"):
            place("lightstand", (x - 35.0, x + 35.0, y - 35.0, y + 35.0, 0.0, 210.0), yaw=90.0, shadow=True)
    for (x, y) in ((-3875.0, -385.0), (-3700.0, -392.0)):        # a Speed Graphic on a tripod, its flash gun up
        head = (x, y, 142.0)
        for k in range(3):
            a = math.radians(90.0 + k * 120.0 + 30.0)
            K.seg((x + 28.0 * math.cos(a), y + 28.0 * math.sin(a), 0.0), head, 2.2, DARK)
        K.cyl(x, y, 140.0, 145.0, 9.0, DARK)
        K.box((x - 8, x + 8, y - 7, y + 9, 145.0, 161.0), DARK)                 # the body
        K.seg((x, y + 9.0, 153.0), (x, y + 19.0, 153.0), 9.0, DARK)             # bellows and lens
        K.seg((x, y + 19.0, 153.0), (x, y + 21.0, 153.0), 7.0, GLASS)
        K.cyl(x + 11.0, y + 2.0, 145.0, 170.0, 3.5, TRIM)                      # the flash gun's tube
        K.seg((x + 11.0, y + 12.0, 172.0), (x + 11.0, y + 3.0, 172.0), 19.0, TRIM, m=CONE)   # its reflector
        K.ball((x + 11.0, y + 7.0, 172.0), 4.5, GLOW)
    if kept_clear(-3790.0, -418.0, why="bulb crate"):
        place("crate", (-3818.0, -3762.0, -438.0, -398.0, 0.0, 34.0), yaw=0.0, mode="stretch")
        for k in range(6):
            K.ball((-3808.0 + k * 7.0, -418.0 + rnd.uniform(-8, 8), 36.5), 4.0, GLASS)
    for k in range(7):                                           # spent flash bulbs underfoot
        K.ball((rnd.uniform(-3960, -3640), rnd.uniform(-340, -230), 1.8), 3.6, GLASS)

# --- the street: a newsstand and a hydrant on the south pavement, a mailbox and a sandwich board on the north -----------
if ON["street"]:
    nx, ny = -3560.0, -1500.0                                     # the newsstand, its counter to the street (-X)
    if kept_clear(nx, ny, r=100.0, why="newsstand"):
        K.box((nx - 40, nx + 40, ny - 80, ny + 80, 0.0, 140.0), GREEN, collide=True, shadow=True)
        for (px, py) in ((nx - 38, ny - 78), (nx - 38, ny + 78), (nx + 38, ny - 78), (nx + 38, ny + 78)):
            K.cyl(px, py, 140.0, 222.0, 5.0, GREEN)
        K.box((nx - 80, nx + 50, ny - 95, ny + 95, 222.0, 230.0), DARK, shadow=True)
        K.box((nx - 112, nx - 78, ny - 95, ny + 95, 207.0, 212.0), mat("MI_Art_2") or VELVET, pitch=-16.0)
        for (x0, z1) in ((nx - 80, 60.0), (nx - 70, 90.0), (nx - 60, 115.0)):   # three tiers of papers
            K.box((x0, nx - 40, ny - 80, ny + 80, 0.0, z1), GREEN, collide=True)
            for k in range(4):
                py = ny - 60 + k * 40
                K.box((x0 + 2, x0 + 30, py - 18, py + 18, z1, z1 + 1.4), LINEN, yaw=rnd.uniform(-4, 4))
                K.box((x0 + 3, x0 + 9, py - 15, py + 15, z1 + 1.4, z1 + 1.6), DARK)
        for k in range(5):                                       # magazines along the counter
            py = ny - 64 + k * 32
            K.box((nx - 38, nx - 30, py - 12, py + 12, 140.0, 172.0), ART[k % 4] or LINEN, pitch=-10.0)
        text("NEWS  \u00b7  CIGARS", nx - 41.0, ny, 128.0, 180.0, 10.0, (0.80, 0.72, 0.52))
        K.box((nx - 30, nx + 30, ny + 81, ny + 84, 20.0, 115.0), LINEN)            # the bill on its north side
        for (t, s, z) in (("EXTRA!", 11.0, 100.0), ("LOST FILM", 9.0, 84.0), ("FOUND", 9.0, 73.0),
                          ("SOLD TONIGHT", 6.5, 60.0), ("AT THE AURELIA", 5.0, 50.0)):
            text(t, nx, ny + 85.0, z, 90.0, s, (0.10, 0.09, 0.08))
    hx, hy = PAVE_X0 + 38.0, -1230.0                              # the hydrant
    if kept_clear(hx, hy, why="hydrant"):
        K.cyl(hx, hy, 0.0, 5.0, 34.0, GREEN, collide=True)
        K.cyl(hx, hy, 5.0, 62.0, 25.0, GREEN)
        K.ball((hx, hy, 62.0), 27.0, GREEN, dz=17.0)
        K.cyl(hx, hy, 68.0, 75.0, 6.0, GREEN)
        K.seg((hx, hy - 20.0, 44.0), (hx, hy + 20.0, 44.0), 9.0, GREEN)
        K.seg((hx - 19.0, hy, 40.0), (hx - 12.0, hy, 40.0), 12.0, GREEN)
    mx, my = PAVE_X0 + 45.0, 1580.0                               # the mailbox
    if kept_clear(mx, my, why="mailbox"):
        for (lx, ly) in ((mx - 19, my - 19), (mx - 19, my + 19), (mx + 19, my - 19), (mx + 19, my + 19)):
            K.box((lx - 2, lx + 2, ly - 2, ly + 2, 0.0, 32.0), NAVY)
        K.box((mx - 23, mx + 23, my - 23, my + 23, 32.0, 100.0), NAVY, collide=True, shadow=True)
        K.seg((mx, my - 23.0, 100.0), (mx, my + 23.0, 100.0), 46.0, NAVY)
        K.box((mx - 24, mx - 23, my - 12, my + 12, 84.0, 88.0), DARK)
        text("U.S. MAIL", mx - 24.5, my, 64.0, 180.0, 6.0, (0.78, 0.74, 0.62))
    bx, by = -3600.0, 720.0                                       # the sandwich board, read from the queue
    if kept_clear(bx, by, why="sandwich board"):
        for s in (-1, 1):
            K.box((bx + s * 12.5 - 1.2, bx + s * 12.5 + 1.2, by - 33, by + 33, 0.0, 112.0), PANEL,
                  pitch=s * 12.8, collide=(s < 0))
            K.box((bx + s * 14.0 - 0.4, bx + s * 14.0 + 0.4, by - 28, by + 28, 8.0, 104.0), DARK, pitch=s * 12.8)
        for (t, sz, z) in (("TONIGHT ONLY", 8.0, 92.0), ("THE", 6.0, 78.0), ("DIRECTOR'S CUT", 7.5, 68.0),
                           ("PRIVATE SALE", 6.0, 50.0), ("9 P.M.", 7.0, 38.0)):
            text(t, bx - 15.2 + 0.227 * (z - 56.0), by, z, 180.0, sz,
                 (0.86, 0.80, 0.66), pitch=12.8)
    # The south door's bay tree gives way to a palm: the two sides of the doors no longer mirror each other.
    if ON["palms"]:
        bay = hide_in([r"Dress_Bay_(Pot|Stem|Crown)_\d+"], (FRONT - 100.0, FRONT, -800.0, -300.0, -10.0, 300.0))
        if bay:
            y = sum(loc_of(a)[1] for a in bay) / len(bay)
            place("front_palm", (FRONT - 130.0, FRONT + 10.0, y - 70.0, y + 70.0, 0.0, 200.0), yaw=35.0, shadow=True)

# --- the red carpet: pewter edge strips from the kerb to the stair ------------------------------------------------------
if ON["carpet"]:
    for s in (-1, 1):
        y0, y1 = sorted((s * (CARPET_HALF - 4.0), s * CARPET_HALF))
        K.box((PAVE_X0 + 60.0, VOID_X0 + 80.0, y0, y1, 1.2, 2.2), TRIM)

# --- the lobby: the reception desk (Rodin) and what is on it -----------------------------------------------------------
# --- W3 opening: the desk moved onto the way in (centre y 1100 -> 630): it faces the doors beside the stair's foot,
# where the eye goes coming in; the paddle board moves with it. DESK_DY moves both.
DESK_DY = -470.0
DESK = (-2300.0, -2100.0, 900.0 + DESK_DY, 1300.0 + DESK_DY, 0.0, 110.0)
if ON["desk"]:
    hide_in([r"Lobby_Desk_\d+", r"Desk_Screen_\d+"], LOBBY)
    d = place("front_desk", DESK, yaw=90.0, mode="stretch", collide=False, shadow=True)
    top = C.top_of(d) if d else DESK[5]
    # The desk is a C: its curve faces the doors (-X), its opening the clerk (+X). Things stand on the marble ring.
    cx, cy, rx, ry = DESK[1] - 33.0, (DESK[2] + DESK[3]) / 2, 167.0, 200.0

    def on_ring(deg, k=0.84):
        a = math.radians(deg)
        return cx - k * rx * math.cos(a), cy + k * ry * math.sin(a)

    def tangent(deg):
        return math.degrees(math.atan2(math.cos(math.radians(deg)) * ry, math.sin(math.radians(deg)) * rx))

    x, y = on_ring(-18.0)                                         # the register, open, a pen across it
    yaw = tangent(-18.0)
    for s in (-1, 1):
        K.cube((x + s * 9.0 * math.cos(math.radians(yaw + 90)), y + s * 9.0 * math.sin(math.radians(yaw + 90)),
                top + 1.2), (17.0, 26.0, 2.4), DARK, (0.0, 0.0, yaw))
        K.cube((x + s * 8.6 * math.cos(math.radians(yaw + 90)), y + s * 8.6 * math.sin(math.radians(yaw + 90)),
                top + 2.7), (16.0, 25.0, 0.8), LINEN, (s * 3.0, 0.0, yaw))
    K.seg((x - 4.0, y - 6.0, top + 3.6), (x + 5.0, y + 7.0, top + 3.6), 0.9, DARK)
    x, y = on_ring(38.0)                                          # a banker's lamp, green shade
    K.cyl(x, y, top, top + 2.5, 15.0, TRIM)
    K.cyl(x, y, top + 2.5, top + 32.0, 2.2, TRIM)
    yaw = tangent(38.0)
    K.add(CYL, GREEN, (x, y, top + 34.0), (0.0, 90.0, yaw), (0.13, 0.11, 0.30), shadow=True)
    K.add(CYL, GLOW, (x, y, top + 32.6), (0.0, 90.0, yaw), (0.10, 0.04, 0.27))
    x, y = on_ring(-58.0)                                         # the service bell
    K.cyl(x, y, top, top + 1.6, 10.0, DARK)
    K.ball((x, y, top + 1.6), 9.0, TRIM, dz=7.0)
    K.ball((x, y, top + 5.4), 1.6, TRIM)
    x, y = on_ring(10.0)                                          # a stack of bidding paddles
    for k in range(6):
        a = math.radians(rnd.uniform(0, 360))
        K.cyl(x + 1.5 * math.cos(a), y + 1.5 * math.sin(a), top + k * 0.9, top + k * 0.9 + 0.8, 21.0, LINEN)
        K.box((x - 1.5, x + 1.5, y - 22.0, y - 9.0, top + k * 0.9, top + k * 0.9 + 0.8), PANEL,
              yaw=k * 17.0 + rnd.uniform(-8, 8))
    x, y = on_ring(66.0)                                          # the house telephone, a candlestick
    K.cyl(x, y, top, top + 3.0, 11.0, DARK)
    K.cyl(x, y, top + 3.0, top + 28.0, 2.6, DARK)
    K.seg((x - 6.0, y, top + 30.0), (x, y, top + 28.0), 4.5, DARK, m=CONE)
    K.seg((x + 3.5, y - 2.0, top + 24.0), (x + 3.5, y - 2.0, top + 12.0), 3.0, DARK)
    x, y = on_ring(-38.0)                                         # a brass name plate for the desk
    K.cube((x, y, top + 4.5), (2.0, 32.0, 9.0), TRIM, (0.0, -12.0, tangent(-38.0) + 90.0))
    # Behind the clerk: the bidders' paddle board, numbered paddles on hooks.
    rx0 = -1985.0
    if kept_clear(rx0, 1100.0 + DESK_DY, why="paddle board"):
        K.box((rx0, rx0 + 28.0, 1020.0 + DESK_DY, 1180.0 + DESK_DY, 0.0, 205.0), PANEL, collide=True, shadow=True)
        K.box((rx0 - 2.0, rx0, 1020.0 + DESK_DY, 1180.0 + DESK_DY, 196.0, 205.0), TRIM)
        text("BIDDERS", rx0 - 2.5, 1100.0 + DESK_DY, 188.0, 180.0, 9.0, (0.80, 0.72, 0.52))
        for row, z in enumerate((160.0, 128.0, 96.0)):
            for k in range(6):
                py = 1035.0 + DESK_DY + k * 26.0
                K.add(CYL, LINEN, (rx0 - 2.0, py, z), (0.0, 90.0, 0.0), (0.15, 0.15, 0.012))
                K.box((rx0 - 2.4, rx0 - 1.2, py - 1.4, py + 1.4, z - 19.0, z - 7.0), PANEL)
                if (row * 6 + k) % 2 == 0:
                    text(str(100 + row * 6 + k + 1), rx0 - 3.0, py, z, 180.0, 6.0, (0.15, 0.10, 0.08))

# --- the lobby's seating: a group by the hall doors, a settee and a pair of club chairs by the front wall ------------------
if ON["seating"]:
    chair_box = lambda x, y: (x - 46.0, x + 46.0, y - 46.0, y + 46.0, 0.0, 92.0)   # noqa: E731
    ARM = SOFA = 90.0                  # the Rodin armchair and sofa face +Y at yaw 0: facing f wants yaw f - ARM
    leather = None
    # The north-east group: a sofa with its back to the north wall, two club chairs, a low table on a rug, a reading
    # lamp, and the house telephone on a side table.
    gx, gy = -1290.0, 1100.0
    if kept_clear(gx, gy, r=150.0, why="seating group"):
        K.box((gx - 175, gx + 175, gy - 160, gy + 150, 0.0, 1.0), RUNNER)
        place("sofa", (gx - 110.0, gx + 110.0, gy + 95.0, gy + 185.0, 0.0, 90.0), yaw=(270.0 - SOFA) % 360.0, collide=True)
        for (x, y, f) in ((gx - 150.0, gy - 60.0, 20.0), (gx + 150.0, gy - 60.0, 160.0)):
            place("front_armchair", chair_box(x, y), yaw=(f - ARM) % 360.0, collide=True)
        K.box((gx - 50, gx + 50, gy - 12, gy + 26, 40.0, 45.0), PANEL, shadow=True)
        for (lx, ly) in ((gx - 45, gy - 8), (gx - 45, gy + 22), (gx + 45, gy - 8), (gx + 45, gy + 22)):
            K.box((lx - 2, lx + 2, ly - 2, ly + 2, 1.0, 40.0), PANEL)
        K.box((gx + 10, gx + 34, gy - 4, gy + 14, 45.0, 46.4), LINEN, yaw=8.0)            # the catalogue
        K.cyl(gx - 22.0, gy + 6.0, 45.0, 47.0, 11.0, GLASS)                               # an ashtray
        K.cyl(gx - 165.0, gy + 140.0, 0.0, 2.0, 30.0, TRIM)                               # the reading lamp
        K.cyl(gx - 165.0, gy + 140.0, 2.0, 150.0, 3.0, TRIM)
        K.cyl(gx - 165.0, gy + 140.0, 150.0, 172.0, 32.0, GLOW, dy=32.0)
        tx, ty = gx + 150.0, gy + 140.0                                                    # the house phone
        K.cyl(tx, ty, 0.0, 60.0, 4.0, PANEL)
        K.cyl(tx, ty, 0.0, 2.0, 26.0, PANEL)
        K.cyl(tx, ty, 60.0, 63.0, 44.0, PANEL, shadow=True)
        K.cyl(tx, ty, 63.0, 66.0, 11.0, DARK)
        K.cyl(tx, ty, 66.0, 91.0, 2.6, DARK)
        K.seg((tx - 6.0, ty - 2.0, 93.0), (tx, ty, 91.0), 4.5, DARK, m=CONE)
        K.seg((tx + 3.5, ty - 2.0, 87.0), (tx + 3.5, ty - 2.0, 75.0), 3.0, DARK)
        K.cube((tx - 10.0, ty + 10.0, 64.0), (7.0, 10.0, 0.4), LINEN, (0.0, 0.0, 20.0))
    # By the front wall (the benches' places): a settee and two side tables to the north, two club chairs to the south.
    benches = hide_in([r"Lobby_Bench_\d+"], (LX0, LX0 + 300.0, LY0, LY1, -10.0, 100.0))
    for a in benches:
        b = C.bounds(a)
        bx, by = (b[0] + b[1]) / 2, (b[2] + b[3]) / 2
        if by > 0:
            place("sofa", (b[0] - 15.0, b[1] + 0.0, by - 95.0, by + 95.0, 0.0, 90.0), yaw=(0.0 - SOFA) % 360.0,
                  collide=True)
            for ty in (b[2] + 22.0, b[3] - 22.0):
                K.cyl(bx - 5.0, ty, 0.0, 58.0, 5.0, PANEL)
                K.cyl(bx - 5.0, ty, 0.0, 2.0, 30.0, PANEL)
                K.cyl(bx - 5.0, ty, 58.0, 61.0, 50.0, PANEL, collide=True, shadow=True)
                K.cyl(bx - 5.0, ty, 61.0, 64.0, 13.0, TRIM)                                 # a table lamp
                K.cyl(bx - 5.0, ty, 64.0, 92.0, 2.0, TRIM)
                K.cyl(bx - 5.0, ty, 88.0, 106.0, 30.0, GLOW, dy=30.0)
        else:
            for y in (b[2] + 60.0, b[3] - 60.0):
                place("front_armchair", (b[0] - 10.0, b[1], y - 50.0, y + 50.0, 0.0, 92.0), yaw=(0.0 - ARM) % 360.0,
                      collide=True)
            K.cyl(bx - 5.0, by, 0.0, 55.0, 5.0, PANEL)
            K.cyl(bx - 5.0, by, 0.0, 2.0, 30.0, PANEL)
            K.cyl(bx - 5.0, by, 55.0, 58.0, 55.0, MARBLE, collide=True, shadow=True)
            K.cyl(bx + 5.0, by - 6.0, 58.0, 80.0, 14.0, GLASS)                               # a vase of lilies
            for k in range(5):
                a = math.radians(k * 72.0)
                K.ball((bx + 5.0 + 7.0 * math.cos(a), by - 6.0 + 7.0 * math.sin(a), 88.0), 9.0, LINEN, dz=6.0)
    say("seating: %d benches dressed" % len(benches))
    # The balcony's two settees (import_props) came in side-on and small (its sofa faced +X from a mesh that faces +Y):
    # this area's instances go, and the same sofa stands in each box facing the atrium.
    gone = [(c, i, x, y) for (c, i, x, y, z0, z1) in instances("Prop_Sofa_Instances")
            if LX0 <= x <= LX1 and abs(y) <= LY1 and z0 > F2 - 50.0]
    for (c, i, x, y) in sorted(gone, key=lambda g: -g[1]):
        c.remove_instance(i)
    for b in (C.bounds(a) for a in C.find(r"Sofa_\d+") if LX0 <= loc_of(a)[0] <= LX1 and loc_of(a)[2] > F2 - 50.0):
        place("sofa", (b[0], b[1] + 10.0, b[2], b[3], F2, F2 + 88.0), yaw=(0.0 - SOFA) % 360.0, collide=True)
    say("balcony settees: %d replaced" % len(gone))

# --- palms: the lobby's planters (both floors) and the south door ---------------------------------------------------------
if ON["palms"]:
    plants = hide_in([r"Plant_\d+", r"Planter_\d+"], LOBBY)
    spots = sorted(set((round(loc_of(a)[0]), round(loc_of(a)[1]), 0.0 if loc_of(a)[2] < F2 - 10 else F2)
                       for a in plants if a.get_actor_label().startswith("Plant_")))
    scatter_fit("front_palm", [((x - 95.0, x + 95.0, y - 95.0, y + 95.0, z, z + (175.0 if abs(y) > 1000 else 200.0)),
                                rnd.uniform(0.0, 360.0)) for (x, y, z) in spots], shadow=True)
    say("palms: %d" % len(spots))

# --- the grand stair: a runner held by rods, balusters and a handrail each side, newel lamps at the foot -----------------------
if ON["stair"]:
    d, h = STAIR_RUN / STAIR_N, F2 / STAIR_N
    for i in range(STAIR_N):
        x0, top = STAIR_X0 + i * d, (i + 1) * h
        K.box((x0, x0 + d, -125.0, 125.0, top, top + 0.8), VELVET)                 # the tread
        K.box((x0 - 0.8, x0, -125.0, 125.0, i * h, top + 0.8), VELVET)             # the riser
        K.seg((x0 - 1.4, -134.0, i * h + 1.4), (x0 - 1.4, 134.0, i * h + 1.4), 2.4, TRIM)   # the stair rod
    rail_z = lambda x: h + 92.0 + (x - (STAIR_X0 + d / 2)) * h / d                 # noqa: E731
    for s in (-1, 1):
        y = s * (STAIR_HALF - 7.0)
        for i in range(0, STAIR_N, 2):
            x = STAIR_X0 + i * d + d / 2
            K.cyl(x, y, (i + 1) * h, rail_z(x), 3.2, TRIM)
        xa, xb = STAIR_X0 - 18.0, STAIR_X0 + STAIR_RUN - d / 2
        K.seg((xa, y, rail_z(STAIR_X0 + d / 2) + 2.0), (xb, y, rail_z(xb) + 2.0), 6.0, PANEL)
        nx, ny = STAIR_X0 - 18.0, s * (STAIR_HALF - 7.0)                           # the newel and its lamp
        if kept_clear(nx, ny, why="newel"):
            K.box((nx - 16, nx + 16, ny - 16, ny + 16, 0.0, 112.0), MARBLE, collide=True, shadow=True)
            K.box((nx - 19, nx + 19, ny - 19, ny + 19, 112.0, 118.0), TRIM)
            K.cyl(nx, ny, 118.0, 140.0, 4.0, TRIM)
            # --- W3 lights --- the 32 cm glow ball read as a flat tan disc on a post: a brass gallery and a small
            # globe (lit from in front by lights.py's Newel_* lights, so it shades round and glows)
            K.cyl(nx, ny, 140.0, 146.0, 13.0, TRIM)
            K.ball((nx, ny, 156.0), 19.0, GLOW, dz=21.0)
            K.cyl(nx, ny, 165.5, 168.0, 5.0, TRIM)

# --- the balcony rail: velvet swags on the atrium side, tassels at the posts, globe lamps at the four corners --------------
if ON["balcony"]:
    zt = F2 + RAIL_H - 10.0
    runs = [((VOID_X0, -VOID_Y), (VOID_X0, VOID_Y), (1, 0)), ((VOID_X0, VOID_Y), (VOID_X1, VOID_Y), (0, -1)),
            ((VOID_X0, -VOID_Y), (VOID_X1, -VOID_Y), (0, 1)), ((VOID_X1, -VOID_Y), (VOID_X1, -200.0), (-1, 0)),
            ((VOID_X1, 200.0), (VOID_X1, VOID_Y), (-1, 0))]
    for (a, b, (ox, oy)) in runs:
        L = math.hypot(b[0] - a[0], b[1] - a[1])
        n = max(1, int(round(L / 300.0)))
        posts = [(a[0] + (b[0] - a[0]) * k / n + ox * 7.0, a[1] + (b[1] - a[1]) * k / n + oy * 7.0) for k in range(n + 1)]
        for p, q in zip(posts, posts[1:]):
            m, sag = 10, 38.0
            pts = [(p[0] + (q[0] - p[0]) * k / m, p[1] + (q[1] - p[1]) * k / m, zt - 4.0 * sag * (k / m) * (1 - k / m))
                   for k in range(m + 1)]
            for u, v in zip(pts, pts[1:]):
                K.seg(u, v, 7.0, VELVET, extend=4.0)
        for (px, py) in posts:
            K.seg((px, py, zt - 24.0), (px, py, zt - 4.0), 5.0, TRIM, m=CONE)
    for (px, py) in ((VOID_X0, -VOID_Y), (VOID_X0, VOID_Y), (VOID_X1, -VOID_Y), (VOID_X1, VOID_Y)):
        K.cyl(px, py, F2 + RAIL_H, F2 + RAIL_H + 12.0, 4.0, TRIM)
        K.ball((px, py, F2 + RAIL_H + 24.0), 24.0, GLOW)

# --- the walls: no longer one frame repeated -------------------------------------------------------------------------------
FRAME = mesh("frame")


def frame(axis, face, inward, c, z0, w, h, picture, depth=7.0):
    """A Rodin frame (stretched like import_props' pictures) round a picture box on a wall face."""
    if axis == "x":                                              # the wall runs along X, its face at y = face
        b = (c - w / 2, c + w / 2) + tuple(sorted((face, face + inward * depth))) + (z0, z0 + h)
        pb = (c - w / 2 + 9, c + w / 2 - 9) + tuple(sorted((face + inward * 1.0, face + inward * 2.5))) + (z0 + 9, z0 + h - 9)
        yaw = 90.0 if inward > 0 else 270.0
    else:
        b = tuple(sorted((face, face + inward * depth))) + (c - w / 2, c + w / 2, z0, z0 + h)
        pb = tuple(sorted((face + inward * 1.0, face + inward * 2.5))) + (c - w / 2 + 9, c + w / 2 - 9, z0 + 9, z0 + h - 9)
        yaw = 0.0 if inward > 0 else 180.0
    if C.eal.does_asset_exist(FRAME):
        m = C.eal.load_asset(FRAME)
        r = C.ip()["best_turn"](m, list(b), (yaw, (yaw + 180.0) % 360.0))
        loc, sc = C.ip()["fit"](m, list(b), r, "stretch")
        frames.append((loc, r, sc))
    if picture is not None:
        K.box(pb, picture)


def sconce(axis, face, inward, c, z, mt=GLOW):
    if axis == "x":
        K.box((c - 6, c + 6) + tuple(sorted((face, face + inward * 2.0))) + (z - 10, z + 10), TRIM)
        K.seg((c, face + inward * 2.0, z - 4.0), (c, face + inward * 14.0, z + 2.0), 2.0, TRIM)
        K.cyl(c, face + inward * 14.0, z + 2.0, z + 18.0, 11.0, mt, dy=11.0)
    else:
        K.box(tuple(sorted((face, face + inward * 2.0))) + (c - 6, c + 6, z - 10, z + 10), TRIM)
        K.seg((face + inward * 2.0, c, z - 4.0), (face + inward * 14.0, c, z + 2.0), 2.0, TRIM)
        K.cyl(face + inward * 14.0, c, z + 2.0, z + 18.0, 11.0, mt, dy=11.0)


frames = []
if ON["walls"]:
    for cx in (-2950.0, -2320.0, -1500.0):                       # north wall, ground floor: a sconce each side of each picture
        for s in (-1, 1):
            sconce("x", LY1, -1, cx + s * 128.0, 222.0)
    for cx in (-2950.0, -2320.0, -1500.0):                       # south wall, the balcony: sconces there instead
        for s in (-1, 1):
            sconce("x", LY0, 1, cx + s * 128.0, F2 + 205.0)
    # the pier glass starts above the 90 cm wainscot (it hung from 55 and the panel cut through its foot, Nov 15)
    frame("x", LY0, 1, -1750.0, 100.0, 110.0, 205.0, MIRROR)     # south wall: a tall pier glass and a portrait
    frame("x", LY0, 1, -1225.0, 110.0, 105.0, 145.0, ART[3])
    frame("y", LX0, 1, 1000.0, 135.0, 150.0, 210.0, LINEN)       # the front wall, north: the film's one-sheet
    for (t, s, z) in (("ONE NIGHT ONLY", 9.0, 318.0), ("THE", 10.0, 296.0), ("DIRECTOR'S", 17.0, 279.0),
                      ("CUT", 30.0, 252.0), ("1941", 12.0, 222.0), ("THE LOST PICTURE, RESTORED", 6.5, 200.0),
                      ("THE ONLY PRINT  \u00b7  LOT 1", 6.5, 186.0)):
        text(t, LX0 + 3.0, 1000.0, z, 0.0, s, (0.40, 0.07, 0.09))
    frame("y", LX0, 1, -1000.0, 120.0, 175.0, 220.0, MIRROR)     # the front wall, south: a big gilt mirror
    frame("y", HALL_FACE, -1, 640.0, 105.0, 190.0, 170.0, PANEL)  # the hall wall: tonight's lots
    for (t, s, z) in (("TONIGHT'S SALE", 11.0, 252.0), ("LOT 1  .  THE DIRECTOR'S CUT (1941)", 5.5, 228.0),
                      ("the only surviving print", 4.5, 218.0), ("LOT 2  .  A STUDIO FOLIO, 1939", 5.0, 202.0),
                      ("LOT 3  .  THE ORPHEUS SCORE", 5.0, 188.0), ("LOT 4  .  PORTRAIT OF A STAR", 5.0, 174.0),
                      ("BIDDING FROM 9 O'CLOCK", 5.0, 148.0)):
        text(t, HALL_FACE - 3.0, 640.0, z, 180.0, s, (0.80, 0.72, 0.52))
    # The clock over the hall doors' side of the atrium, on the west wall's upper storey.
    ck = (LX0 + 3.0, -150.0, F2 + 245.0)
    K.add(CYL, TRIM, (ck[0], ck[1], ck[2]), (0.0, 90.0, 0.0), (0.80, 0.80, 0.03))
    K.add(CYL, LINEN, (ck[0] + 2.5, ck[1], ck[2]), (0.0, 90.0, 0.0), (0.70, 0.70, 0.02))
    for k in range(12):
        a = math.radians(k * 30.0)
        K.cube((ck[0] + 3.2, ck[1] + 30.0 * math.sin(a), ck[2] + 30.0 * math.cos(a)), (0.6, 1.6, 5.0 if k % 3 == 0 else 3.0),
               DARK, (k * 30.0, 0.0, 0.0))
    for (ang, ln) in ((95.0, 17.0), (30.0, 25.0)):               # five to nine (seen from the room)
        a = math.radians(ang)
        K.cube((ck[0] + 3.6, ck[1] + ln / 2 * math.sin(a), ck[2] + ln / 2 * math.cos(a)), (0.6, 1.8, ln), DARK,
               (ang, 0.0, 0.0))
    if frames and C.eal.does_asset_exist(FRAME):
        C.scatter(FRAME, frames, tag=TAG, shadow=False)

# --- signs: the cloakroom, the restrooms, the hall -----------------------------------------------------------------------
if ON["signs"]:
    for (t, y, yaw) in (("CLOAKROOM", LY1 - 2.0, 270.0), ("LADIES  \u00b7  GENTLEMEN", LY0 + 2.0, 90.0)):
        K.box((-2650 - 70, -2650 + 70) + tuple(sorted((y, y + (1.5 if yaw == 90.0 else -1.5)))) + (272.0, 298.0), DARK)
        text(t, -2650.0, y + (2.0 if yaw == 90.0 else -2.0), 285.0, yaw, 9.0, (0.80, 0.72, 0.52))
    text("THE GRAND HALL", HALL_FACE - 2.0, 0.0, 415.0, 180.0, 16.0, (0.82, 0.72, 0.50))

placed = K.flush()
C.save_level()
"front: %d kit pieces; %s" % (placed, "; ".join(LOG))
