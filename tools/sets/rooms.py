# Package rooms (QA pass, Nov 14; Marwan's notes 11, 14, 20): every side room dressed for what it is, so a player
# reads it in three seconds. Overlay on the grey box: idempotent, tag "set_rooms", one function per room. Run it inside
# the editor (under the editor lock):
#     "<engine python>" -I tools/ue_mcp.py py tools/sets/rooms.py 110
# A wrapper that sets ROOMS_STEPS / ROOMS_ONLY / ROOMS_NAMES and execs this file runs part of it:
#     ROOMS_STEPS = ("import",)      import the Rodin props (tools/rodin/props_rooms.json; C:\Jam\Art\Rodin\rooms_*)
#     ROOMS_STEPS = ("lineup",)      every prop this file places in a row west of the street, facing +X, and a shot
#     ROOMS_STEPS = ("rooms",)       the dressing (the default); ROOMS_ONLY = ("kitchen",) redoes just those rooms
#
# What it hides (visibility only: collision, sight lines and navigation stay as the grey box had them) it replaces in
# the same footprint, so no invisible wall stands in an empty spot. Every new piece is NoCollision unless it stands on
# open floor (collide=True: blocks the pawn only, never a sight line or the camera, as import_props does). Nothing new
# within 1.2 m of a mark, a device, a crew post or Vex's walk (dock -> corridor -> hatches -> winch).
import math
import random
import re
import sys
import importlib

import unreal

sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C   # noqa: E402
importlib.reload(C)

TAG = "set_rooms"
STEPS = globals().get("ROOMS_STEPS") or ("rooms",)
ONLY = globals().get("ROOMS_ONLY")
NAMES = globals().get("ROOMS_NAMES")
OUT = r"C:\Jam\Builds\qa\rooms"
F2 = 450.0
eal = unreal.EditorAssetLibrary
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LOG = []

# --- materials: the set's own instances (dress_auction.py LIT/GLOW); never MI_Gold or MI_Red (the signals) ----------
D = "/Game/Take/Dress/"
M = {"wood": D + "MI_Front_Panel", "parquet": D + "MI_Front_Parquet", "pewter": D + "MI_Front_Trim",
     "linen": D + "MI_Front_Linen", "stone": D + "MI_Front_Plinth", "velvet": D + "MI_Front_Velvet",
     "ply": D + "MI_Back_Ply", "batten": D + "MI_Back_Batten", "steel": D + "MI_Back_Steel", "tape": D + "MI_Back_Tape",
     "black": D + "MI_Back_Cable", "canvas": D + "MI_Canvas_Light", "canvas_dk": D + "MI_Canvas_Dark",
     "glass": D + "MI_Front_Glass", "glow": D + "MI_Front_Sconce", "screen": D + "MI_Front_Screen",
     "cloth_a": D + "MI_Person_A", "cloth_b": D + "MI_Person_B", "cloth_c": D + "MI_Person_C", "grey": D + "MI_Car_Grey"}
for _k in list(M):
    if not eal.does_asset_exist(M[_k]):
        LOG.append("no material %s: pewter instead" % M[_k])
        M[_k] = D + "MI_Front_Trim"
CUBE, CYL, SPH = "/Engine/BasicShapes/Cube", "/Engine/BasicShapes/Cylinder", "/Engine/BasicShapes/Sphere"

# --- the Rodin props: this package's (props_rooms.json) and the shared ones it reuses ---------------------------------
LOOKS = {   # import_props LOOK key (or a LOOK-shaped tuple) per prop: the value band of its surface
    "rooms_range": "piano", "rooms_projector": "roadcase", "rooms_filing": "stanchion", "rooms_desk": "bar",
    "rooms_switchboard": "cocktail", "rooms_filmshelf": "stanchion", "rooms_shelving": "crate",
    "rooms_lockers": ((0.05, 0.26), (0.95, 0.96, 1.0), 0.5), "rooms_vaultdoor": ((0.06, 0.40), (0.95, 0.96, 1.0), 0.3),
    "rooms_handtruck": "roadcase", "rooms_sink": "pedestal", "rooms_lorry": "piano"}
SHARED = ("coatrack", "crate", "chair", "sofa", "bust", "vase", "pedestal", "column", "camera", "cart")
# The yaw that turns each mesh to face +X (set from the lineup shot). Rodin's front arrives along +Y after
# Interchange's glTF conversion (import_props FRONT["piano"] = 270), so 270 unless the lineup says otherwise.
FRONT = {}


def front(n):
    return FRONT.get(n, 270.0)


def mesh(n):
    c = n[:1].upper() + n[1:]
    return "/Game/Take/Props/%s/SM_%s" % (c, c)


# --- kit: basic shapes gathered per room, material and collision, then one instanced actor each ---------------------
KIT = {}
PROPS = {}
ROOM = ["misc"]
rng = random.Random(14)


def _add(m, mat, loc, r, sc, collide=False):
    KIT.setdefault((ROOM[0], m, mat, collide), []).append((tuple(loc), tuple(r), tuple(sc)))


class At:
    """A local frame: x, y, z of its origin and a yaw. Pieces are given in local cm (front = +X)."""

    def __init__(self, x, y, z=0.0, yaw=0.0):
        self.x, self.y, self.z, self.yaw = float(x), float(y), float(z), float(yaw)
        self.c, self.s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))

    def w(self, lx, ly, lz):
        return (self.x + lx * self.c - ly * self.s, self.y + lx * self.s + ly * self.c, self.z + lz)

    def box(self, x0, x1, y0, y1, z0, z1, mat, r=(0.0, 0.0, 0.0), collide=False):
        """A box from local x0..x1, y0..y1, z0..z1, turned by r (roll, pitch, yaw) about its own centre."""
        _add(CUBE, mat, self.w((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (r[0], r[1], r[2] + self.yaw),
             (abs(x1 - x0) / 100, abs(y1 - y0) / 100, abs(z1 - z0) / 100), collide)

    def cyl(self, x, y, z0, d, h, mat, r=(0.0, 0.0, 0.0), dy=None, collide=False):
        """A standing cylinder, d across (dy across y if given), from z0 up h (turned by r about its centre)."""
        _add(CYL, mat, self.w(x, y, z0 + h / 2), (r[0], r[1], r[2] + self.yaw), (d / 100, (dy or d) / 100, h / 100),
             collide)

    def rod(self, x, y, z, d, length, mat, yaw=0.0):
        """A lying cylinder centred on (x, y, z), its axis horizontal along `yaw` (a reel on edge, a rolled cloth)."""
        _add(CYL, mat, self.w(x, y, z), (0.0, 90.0, yaw + self.yaw), (d / 100, d / 100, length / 100))

    def sph(self, x, y, z, sx, sy, sz, mat):
        _add(SPH, mat, self.w(x, y, z), (0.0, 0.0, self.yaw), (sx / 100, sy / 100, sz / 100))

    def bar(self, a, b, wd, th, mat):
        """A board from local point a to b (its length), wd wide (horizontal, across) and th thick."""
        pa, pb = self.w(*a), self.w(*b)
        dx, dy, dz = pb[0] - pa[0], pb[1] - pa[1], pb[2] - pa[2]
        hz = math.hypot(dx, dy)
        ln = math.sqrt(dx * dx + dy * dy + dz * dz)
        _add(CUBE, mat, ((pa[0] + pb[0]) / 2, (pa[1] + pb[1]) / 2, (pa[2] + pb[2]) / 2),
             (0.0, math.degrees(math.atan2(dz, hz)), math.degrees(math.atan2(dy, dx)) if hz > 0.01 else self.yaw),
             (ln / 100, wd / 100, th / 100))

    def prop(self, n, lx, ly, lz, d, w, h, face=0.0, **kw):
        x, y, z = self.w(lx, ly, lz)
        return prop(n, x, y, z, d, w, h, face + self.yaw, **kw)


W = At(0, 0, 0, 0)
_MESH = {}


def _fitted(n, r, sc):
    m = _MESH.get(n) or eal.load_asset(mesh(n))
    _MESH[n] = m
    bb = m.get_bounding_box()
    mn, mx = (bb.min.x, bb.min.y, bb.min.z), (bb.max.x, bb.max.y, bb.max.z)
    rot = C.ip()["rotate"]
    cs = [rot((x * sc[0], y * sc[1], z * sc[2]), r) for x in (mn[0], mx[0]) for y in (mn[1], mx[1])
          for z in (mn[2], mx[2])]
    return tuple(max(c[i] for c in cs) - min(c[i] for c in cs) for i in range(3))


def prop(n, x, y, z0, d, w, h, face=0.0, mode="fit", collide=False, mat=None):
    """A Rodin prop standing on z0 at (x, y), fitted into d (along the way it faces) by w (across) by h, facing
    `face` (0 +X, 90 +Y, 180 -X, 270 -Y, or anything between). Returns its fitted size (d, w, h)."""
    if not eal.does_asset_exist(mesh(n)):
        LOG.append("no mesh " + n)
        return (0.0, 0.0, 0.0)
    loc, r, sc = C.fit(mesh(n), (-d / 2, d / 2, -w / 2, w / 2, 0.0, h), front(n), mode)
    c, s = math.cos(math.radians(face)), math.sin(math.radians(face))
    wl = (x + loc[0] * c - loc[1] * s, y + loc[0] * s + loc[1] * c, z0 + loc[2])
    PROPS.setdefault((ROOM[0], n, collide, mat), []).append((wl, (r[0], r[1], r[2] + face), tuple(sc)))
    return _fitted(n, r, sc)


def flush():
    n = 0
    for (room, m, mat, col), tr in sorted(KIT.items(), key=lambda kv: str(kv[0])):
        what = m.split("/")[-1]
        C.scatter(m, tr, tag=TAG, label="Rooms_%s_%s_%s%s" % (room, what, mat, "_Solid" if col else ""),
                  folder="%s/%s" % (TAG, room), collide=col, shadow=False, mat=M[mat])
        n += len(tr)
    for (room, p, col, mat), tr in sorted(PROPS.items(), key=lambda kv: str(kv[0])):
        C.scatter(mesh(p), tr, tag=TAG, label="Rooms_%s_%s" % (room, p), folder="%s/%s" % (TAG, room),
                  collide=col, shadow=False, mat=M[mat] if mat else None)
        n += len(tr)
    KIT.clear()
    PROPS.clear()
    return n


# --- hiding the grey boxes an overlay replaces (only inside a room's box) --------------------------------------------
_CENTRES = []


def _centres():
    if not _CENTRES:
        for a in C.actors():
            o, e = a.get_actor_bounds(False)
            _CENTRES.append((a.get_actor_label(), o.x, o.y, o.z))
    return _CENTRES


def hide(patterns, box):
    """Hides every actor whose label matches a pattern and whose bounds centre lies in box (x0, x1, y0, y1[, z0, z1])."""
    rx = re.compile(r"^(%s)$" % "|".join(patterns))
    z0, z1 = (box[4], box[5]) if len(box) > 4 else (-100.0, 1000.0)
    labels = [lb for (lb, x, y, z) in _centres() if rx.match(lb) and box[0] <= x <= box[1] and box[2] <= y <= box[3]
              and z0 <= z <= z1]
    if labels:
        C.hide([re.escape(lb) for lb in labels], TAG)
    return labels


# --- small furniture -------------------------------------------------------------------------------------------------
def hat(at, x, y, z, mat="black", yaw=0.0):
    at.cyl(x, y, z, 34, 1.5, mat, r=(0, 0, yaw), dy=30)
    at.cyl(x, y, z + 1.5, 19, 12, mat, r=(0, 0, yaw), dy=16)
    at.cyl(x, y, z + 1.5, 19.6, 2.5, "black", r=(0, 0, yaw), dy=16.6)


def books(at, x, y, z, n, yaw=0.0):
    mats = ("cloth_a", "velvet", "cloth_c", "linen", "wood")
    for i in range(n):
        at.box(x - 12, x + 12, y - 16, y + 16, z + i * 4, z + i * 4 + 3.8, mats[(i + int(x)) % 5],
               r=(0, 0, yaw + rng.uniform(-9, 9)))


def phone(at, x, y, z):
    at.box(x - 9, x + 9, y - 7, y + 7, z, z + 7, "black")
    at.box(x - 7, x + 7, y - 2, y + 2, z + 7, z + 10, "black")
    at.rod(x, y, z + 12, 4.5, 21, "black", yaw=0)


def desk_lamp(at, x, y, z):
    at.cyl(x, y, z, 14, 2, "pewter")
    at.bar((x, y, z + 2), (x + 8, y, z + 34), 2, 2, "pewter")
    at.cyl(x + 12, y, z + 26, 17, 9, "steel", r=(0, -30, 0))


def stool(at, x, y, z=0.0, h=60.0):
    at.cyl(x, y, z + h - 4, 32, 4, "wood")
    for a in (90, 210, 330):
        c, s = math.cos(math.radians(a)), math.sin(math.radians(a))
        at.bar((x + 9 * c, y + 9 * s, z + h - 4), (x + 15 * c, y + 15 * s, z), 3, 3, "wood")


def coupe(at, x, y, z):
    at.cyl(x, y, z, 4, 0.6, "pewter")
    at.cyl(x, y, z + 0.6, 1, 7, "pewter")
    at.cyl(x, y, z + 7.6, 9, 2.5, "pewter")


def wall_frame(cx, wy, z0, w, h, inward, mat="canvas"):
    """A framed picture on a wall face along x at y = wy (inward: +1 the room is at +y, -1 at -y)."""
    a, b = sorted((wy, wy + inward * 4))
    c0, c1 = sorted((wy, wy + inward * 5))
    W.box(cx - w / 2, cx + w / 2, a, b, z0, z0 + h, "pewter")
    W.box(cx - w / 2 + 5, cx + w / 2 - 5, c0, c1, z0 + 5, z0 + h - 5, mat)


def counter(x0, x1, y0, y1, front=1, h=92.0, body="linen", top="pewter"):
    """A kitchen counter from x0 to x1 against a wall; its front at y1 (front=1) or y0 (front=-1)."""
    fy = y1 if front > 0 else y0
    W.box(x0, x1, y0, y1, 8, h - 4, body)
    W.box(x0 + 2, x1 - 2, min(y0, y1) + 5, max(y0, y1) - 5, 0, 8, "black")
    W.box(x0 - 1, x1 + 1, y0 - (0 if front > 0 else 2), y1 + (2 if front > 0 else 0), h - 4, h, top)
    x = x0 + 50.0
    while x < x1 - 20:
        W.box(x - 0.4, x + 0.4, fy - 0.3, fy + 0.6 * front, 14, h - 12, "black")
        x += 50.0


# =====================================================================================================================
# The rooms. Each starts with one line of intent: the story a player reads in three seconds.
# =====================================================================================================================
def corridor():
    """Backstage between lots: the quick-change mirror and costume rails, the props table laid out for the next lot,
    a wardrobe trunk left open, crates and a rope coil against the wall, the ghost light at the far end."""
    hide([r"Costume_Rail_\d+", r"Costume_Rail_Post_\d+", r"Costume_\d+", r"Back_Crate_\d+", r"Prop_Orb_\d+",
          r"Prop_Box_\d+", r"Makeup_Lights_\d+"], (-1000, 2700, -2600, -2020, 0, 500))
    prop("coatrack", 430, -2540, 0, 70, 220, 176, face=90)
    prop("coatrack", 765, -2552, 0, 70, 180, 170, face=97)
    t = At(60, -2522, 0, 4)                                     # the wardrobe trunk, lid up, a costume spilling out
    t.box(-55, 55, -32, 32, 0, 62, "ply")
    t.box(-56, 56, -33, 33, 29, 33, "pewter")
    for (a, b) in ((-56, -52), (52, 56)):
        t.box(a, b, -33, 33, 0, 62, "pewter")
    t.box(-52, 52, -29, 29, 56, 58.5, "canvas")
    t.box(-55, 55, -38, -33, 62, 122, "ply")
    t.box(-28, 14, 26, 37, 22, 66, "velvet")
    W.cyl(165, -2548, 0, 42, 26, "linen")
    W.cyl(170, -2543, 26, 32, 20, "cloth_b")
    prop("crate", 1550, -2522, 0, 104, 112, 92, face=97, mode="stretch")      # crates, a rope coil
    W.cyl(1545, -2520, 92, 44, 9, "batten")
    W.cyl(1545, -2520, 92.2, 16, 9, "black")
    prop("crate", 1440, -2562, 0, 52, 60, 44, face=71, mode="stretch")
    prop("crate", 2660, -2524, 0, 116, 120, 104, face=93, mode="stretch")
    prop("crate", 2652, -2531, 104, 46, 54, 40, face=121, mode="stretch")
    for x in [-810 + 40 * i for i in range(9)]:                 # bulbs round the quick-change mirror
        W.sph(x, -2026, 178, 7, 7, 7, "glow")
    for z in (104, 127, 150, 173):
        W.sph(-829, -2026, z, 7, 7, 7, "glow")
        W.sph(-471, -2026, z, 7, 7, 7, "glow")
    W.cyl(-800, -2060, 75, 9, 4, "linen")                       # the dressing table
    W.cyl(-786, -2073, 75, 7, 3, "pewter")
    W.cyl(-560, -2055, 75, 12, 2, "wood")
    W.cyl(-560, -2055, 77, 3, 22, "wood")
    W.sph(-560, -2055, 106, 17, 17, 20, "linen")
    W.sph(-560, -2052, 111, 19, 19, 15, "cloth_c")
    W.cyl(-700, -2045, 75, 7, 10, "glass")
    for i in range(3):
        W.box(-703 + i * 2, -702 + i * 2, -2046, -2045, 80, 95, "black", r=(rng.uniform(-9, 9), 0, 0))
    W.cyl(-640, -2072, 75, 14, 1, "pewter")
    W.box(-640, -625, -2074, -2070, 75, 76, "pewter")
    ends = [(-985.0, -2392.0)]                                  # the quick-change screen, three leaves
    for yaw in (75.0, 15.0, 80.0):
        x, y = ends[-1]
        At(x, y, 0, yaw).box(0, 60, -2, 2, 2, 175, "wood")
        At(x, y, 0, yaw).box(3, 57, -2.6, 2.6, 10, 166, "linen")
        ends.append((x + 60 * math.cos(math.radians(yaw)), y + 60 * math.sin(math.radians(yaw))))
    At(ends[1][0], ends[1][1], 0, 15).box(6, 44, -5, 5, 110, 178, "velvet")
    W.cyl(-950, -2560, 0, 40, 6, "black")                       # the ghost light
    W.cyl(-950, -2560, 6, 4, 150, "pewter")
    W.sph(-950, -2560, 162, 10, 10, 13, "glow")
    W.box(-951, -949, -2561, -2559, 156, 172, "black")
    for (x0, x1, y0, y1) in ((1015, 1050, -2075, -2045), (1080, 1120, -2080, -2040), (1160, 1200, -2075, -2045),
                             (1225, 1275, -2080, -2040)):   # the props table: tape outlines, each prop on its spot
        W.box(x0, x1, y0, y0 + 1, 80, 80.3, "tape")
        W.box(x0, x1, y1 - 1, y1, 80, 80.3, "tape")
        W.box(x0, x0 + 1, y0, y1, 80, 80.3, "tape")
        W.box(x1 - 1, x1, y0, y1, 80, 80.3, "tape")
    W.rod(1032, -2060, 84, 7, 14, "wood", yaw=90)
    W.rod(1046, -2060, 83, 2.5, 26, "wood", yaw=0)
    W.box(1085, 1115, -2075, -2045, 80, 87, "velvet")
    W.sph(1100, -2060, 93, 9, 9, 12, "pewter")
    W.cyl(1180, -2060, 80, 12, 2, "pewter")
    W.cyl(1180, -2060, 82, 2.5, 30, "pewter")
    W.box(1166, 1194, -2061, -2059, 104, 106, "pewter")
    for (x, z) in ((1167, 106), (1180, 112), (1193, 106)):
        W.cyl(x, -2060, z, 2, 12, "linen")
    for i in range(3):
        W.cyl(1240 + i * 4, -2064 + i * 3, 80 + i * 0.6, 20, 0.5, "linen")
        W.box(1240 + i * 4 - 1, 1240 + i * 4 + 1, -2054 + i * 3, -2038 + i * 3, 80, 81, "wood")
    phone(W, 1285, -2052, 80)
    W.box(1800, 1890, -2600, -2597, 140, 200, "batten")         # the call sheet by the props door
    for (x, z) in ((1806, 172), (1832, 168), (1858, 174), (1815, 146)):
        W.box(x, x + 20, -2597, -2596.5, z, z + 25, "linen", r=(rng.uniform(-4, 4), 0, 0))
    W.rod(2000, -2026, 300, 46, 8, "batten", yaw=90)            # a rope coil hung on the flats
    W.rod(2000, -2025, 300, 18, 9, "black", yaw=90)


def kitchen():
    """Service for the gala: two ranges going, champagne on ice in the trolleys by the door, the cake on the prep
    table, crates of bottles stacked by the door, dry stores on the shelves."""
    box = (-1000, 800, -3300, -2620, 0, 500)
    hide([r"Kitchen_Counter_\d+", r"Kitchen_Island_\d+", r"Fridge_\d+", r"Kitchen_Shelf_\d+", r"Pot_\d+", r"Tray_\d+",
          r"Glass_\d+", r"Plates_\d+"], box)
    sz = prop("rooms_range", -455, -3246, 0, 106, 290, 150, face=90)
    hw = sz[1] / 2 if sz[1] else 90.0
    counter(-990, -455 - hw - 2, -3300, -3214)
    counter(-455 + hw + 2, -145 - hw - 2, -3300, -3214)
    prop("rooms_range", -145, -3246, 0, 106, 290, 150, face=90)
    counter(-145 + hw + 2, 600, -3300, -3214)
    W.box(-990, 600, -3300, -3298, 92, 172, "linen")           # tiled splashback
    W.box(-340, -260, -3300, -3240, 232, 500, "steel")          # the hood's flue
    W.box(220, 330, -3292, -3236, 86, 92.5, "steel")            # the sink in the east counter, its taps
    for x in (255, 295):
        W.cyl(x, -3290, 92, 3, 22, "pewter")
        W.box(x - 1, x + 1, -3290, -3276, 112, 114, "pewter")
    W.box(100, 420, -3300, -3282, 150, 152, "wood")             # the plate rack
    for i in range(44):
        W.rod(108 + i * 7, -3291, 163, 22, 1.2, "linen", yaw=0)
    W.box(-985, -600, -3299, -3296, 165, 168, "pewter")         # pans on the rail over the west counter
    for (x, d) in ((-950, 30), (-895, 24), (-845, 18), (-790, 28), (-730, 22), (-670, 26)):
        W.rod(x, -3292, 165 - d / 2 - 6, d, 4, "steel", yaw=90)
        W.box(x - 1, x + 1, -3293, -3291, 165 - 8, 168, "steel")
    W.box(622, 778, -3282, -3182, 10, 198, "linen")             # the icebox
    W.box(624, 776, -3280, -3184, 0, 10, "black")
    W.box(624, 776, -3182, -3181, 119, 121, "black")
    for (z0, z1) in ((140, 165), (70, 95)):
        W.box(752, 762, -3182, -3175, z0, z1, "pewter")
    W.box(-505, 305, -3004, -2896, 88, 94, "pewter")            # the prep table (the island's footprint)
    for x in (-495, -100, 295):
        for y in (-2995, -2905):
            W.cyl(x, y, 0, 5, 88, "pewter")
    W.box(-500, 300, -2998, -2902, 18, 21, "pewter")
    for (i, d) in enumerate((32, 27, 22)):
        W.cyl(-400 + i * 3, -2950 + i * 2, 21 + i * 17, d, 17, "steel")
    W.sph(150, -2950, 36, 52, 36, 30, "batten")
    W.box(-470, -400, -2985, -2945, 94, 96.5, "wood")
    for (d, h, z) in ((42, 14, 94), (31, 12, 108), (20, 10, 120)):   # tonight's cake
        W.cyl(-200, -2950, z, d, h, "linen")
    W.sph(-200, -2950, 134, 6, 6, 8, "pewter")
    for (tx, ty) in ((40, -2938), (180, -2958)):                 # trays of coupes
        W.box(tx - 26, tx + 26, ty - 18, ty + 18, 94, 95, "pewter")
        for i in range(6):
            coupe(W, tx - 16 + (i % 3) * 16, ty - 8 + (i // 3) * 16, 95)
    prop("rooms_shelving", -963, -3100, 0, 56, 185, 205, face=0)   # dry stores on the west wall
    prop("rooms_shelving", -963, -2895, 0, 56, 185, 205, face=0)
    for (x, y, z, f) in ((470, -2672, 0, 90), (476, -2669, 33, 96), (548, -2694, 0, 81)):   # champagne crates
        prop("crate", x, y, z, 50, 72, 32, face=f, mode="stretch")
    for i in range(6):
        x, y = 462 + (i % 3) * 18, -2680 + (i // 3) * 20
        W.cyl(x, y, 62, 3.2, 11, "black")
        W.cyl(x, y, 73, 3.4, 3, "linen")
    W.cyl(690, -2700, 0, 62, 70, "pewter")                      # the ice bin
    for (x, y) in ((675, -2705), (695, -2690), (708, -2712), (683, -2685), (700, -2720)):
        W.cyl(x, y, 66, 3.2, 12, "black")
        W.cyl(x, y, 78, 3.4, 3, "linen")
    for (x, y) in ((-405, -2732), (-352, -2724)):              # buckets on the side table
        W.cyl(x, y, 80, 22, 24, "pewter")
        W.cyl(x + 3, y, 96, 6, 18, "black")
        W.cyl(x + 3, y, 114, 3.4, 4, "linen")
    prop("cart", -660, -2722, 0, 100, 62, 100, face=0, collide=True)    # trolleys ready to go out
    prop("cart", 262, -2735, 0, 100, 62, 100, face=196, collide=True)


def propstore():
    """Last season's productions: painted flats in a rack, the throne and a Roman column, furniture under dust
    sheets, a costume rail, rolled backdrops, a bust on its pedestal."""
    hide([r"Bust_\d+", r"Fake_Column_\d+", r"Chair_Stack_\d+", r"Bust_Plinth_\d+", r"Prop_Crate_\d+",
          r"Prop_Shelf_\d+"], (820, 2600, -3300, -2620, 0, 500))
    for y in (-3270, -3000, -2730):                             # the flats rack
        W.box(905, 911, y - 3, y + 3, 0, 300, "batten")
    W.box(905, 911, -3276, -2724, 280, 286, "batten")
    for (x, y0, y1, h, mat) in ((882, -3270, -2960, 330, "canvas"), (893, -3080, -2790, 262, "canvas_dk"),
                                (900, -2930, -2700, 288, "ply")):
        yc = (y0 + y1) / 2
        f = At(x, yc, 0, 0)
        f.box(-2, 2, -(y1 - y0) / 2, (y1 - y0) / 2, 0, h, mat, r=(0, 4, 0))
        if mat == "canvas_dk":                                  # a painted window on the dark flat
            f.box(2, 3, -50, 50, 115, 215, "linen", r=(0, 4, 0))
            f.box(2.5, 3.5, -3, 3, 115, 215, "black", r=(0, 4, 0))
        if mat == "ply":                                        # the back of a flat: its rails
            for z in (60, 150, 240):
                f.box(2, 5, -(y1 - y0) / 2 + 4, (y1 - y0) / 2 - 4, z - 3, z + 3, "batten", r=(0, 4, 0))
    W.box(1220, 1380, -3240, -3120, 0, 15, "wood", collide=True)    # the throne on its rostrum
    th = At(1300, -3182, 15, 90)
    th.box(-32, 32, -37, 37, 0, 30, "wood")
    th.box(-30, 30, -35, 35, 30, 48, "velvet")
    th.box(-36, -26, -37, 37, 48, 190, "velvet")
    for s in (-1, 1):
        th.box(-37, -25, s * 36 - 2, s * 36 + 2, 30, 196, "pewter")
        th.box(-30, 28, s * 38.5 - 3.5, s * 38.5 + 3.5, 30, 72, "wood")
        th.sph(28, s * 38.5, 74, 8, 8, 8, "pewter")
    th.sph(-31, 0, 202, 24, 8, 24, "pewter")
    prop("column", 1750, -3200, 0, 72, 72, 262, face=90)        # the Roman column and a broken drum
    W.rod(1640, -3060, 28, 56, 70, "stone", yaw=20)
    prop("pedestal", 1870, -3150, 0, 50, 50, 100, face=90, mode="stretchz")
    prop("bust", 1870, -3150, 100, 45, 45, 55, face=90)
    prop("sofa", 2250, -3205, 0, 85, 205, 92, face=95, mat="linen", collide=True)   # under dust sheets
    prop("chair", 2010, -3120, 0, 55, 55, 92, face=62, mat="linen", collide=True)
    prop("chair", 1252, -2748, 0, 52, 52, 90, face=250)
    prop("coatrack", 2470, -2930, 0, 70, 190, 176, face=180, collide=True)
    W.rod(1300, -3275, 15, 30, 480, "canvas", yaw=0)            # rolled backdrops
    W.rod(1290, -3272, 43, 26, 420, "canvas_dk", yaw=2)
    prop("rooms_shelving", 2563, -3150, 0, 56, 220, 210, face=180)
    W.cyl(1000, -2725, 0, 30, 4, "wood")                        # a globe on its stand
    W.cyl(1000, -2725, 4, 4, 72, "wood")
    W.sph(1000, -2725, 100, 46, 46, 46, "parquet")
    W.box(1435, 1465, -2670, -2620, 0, 212, "wood")             # a long-case clock
    W.rod(1450, -2672, 178, 26, 2, "linen", yaw=90)
    W.box(1441, 1459, -2672, -2669, 70, 140, "glass")
    prop("crate", 1030, -2985, 0, 90, 100, 80, face=95, mode="stretch")   # a crate of spears and a banner
    for (x, y, rl) in ((1012, -2992, 7), (1030, -2970, -5)):
        W.cyl(x, y, 40, 3, 190, "wood", r=(rl, 4, 0))
        W.sph(x, y, 236, 4, 4, 12, "pewter")
    W.cyl(1050, -2990, 40, 3, 200, "wood")
    W.box(1051, 1053, -2990, -2950, 205, 238, "velvet")
    cv = At(1110, -2770, 0, 300)                                # a cheval glass
    cv.box(-3, 3, -34, 34, 26, 172, "pewter")
    cv.box(-2, 4, -30, 30, 30, 168, "glass")
    cv.box(-20, 20, -34, -30, 0, 4, "wood")
    cv.box(-20, 20, 30, 34, 0, 4, "wood")


def dock():
    """The crew's way in: the lorry backed up to the shutter, a pallet half unloaded, the hand truck left mid-job,
    the punch clock and the drivers' cards by the door."""
    hide([r"Dock_Crate_\d+", r"Hand_Truck_\d+", r"Hazard_Line_\d+"], (2620, 4400, -3300, -2620, 0, 500))
    hide([r"Lorry_Trailer_\d+", r"Lorry_Cab_\d+"], (3300, 4000, -4400, -3320, 0, 500))
    prop("rooms_lorry", 3650, -3745, 0, 820, 500, 360, face=270)
    for i in range(18):                                         # the line at the shutter: tape and black blocks
        x = 3310 + i * 38.5
        W.box(x, x + 38.5, -3240, -3220, 0, 0.6, "tape" if i % 2 else "black")
    prop("crate", 2840, -3060, 0, 168, 176, 112, face=93, mode="stretch")
    prop("crate", 2815, -3075, 112, 60, 72, 46, face=108, mode="stretch")
    tp = At(2692, -2925, 0, 10)                                 # a tarpaulin over a stack
    tp.box(-46, 46, -40, 40, 0, 68, "canvas_dk")
    tp.box(-50, 50, -44, 44, 60, 72, "canvas_dk", r=(0, 0, 3))
    prop("crate", 4240, -3060, 0, 150, 160, 120, face=87, mode="stretch")
    for i in range(3):                                          # empty pallets
        W.box(4252 + i * 3, 4372 + i * 3, -3292, -3192, i * 13, i * 13 + 4, "batten")
        for x in (4258, 4307, 4356):
            W.box(x + i * 3, x + 10 + i * 3, -3292, -3192, i * 13 + 4, i * 13 + 13, "batten")
    prop("rooms_handtruck", 3205, -3238, 0, 55, 58, 125, face=90)
    prop("crate", 3150, -3165, 0, 46, 50, 42, face=104, mode="stretch")
    prop("crate", 3448, -3052, 12, 95, 100, 82, face=92, mode="stretch")      # pallet one, half unloaded
    prop("crate", 3436, -3043, 94, 62, 70, 48, face=75, mode="stretch")
    prop("crate", 3672, -2862, 12, 60, 62, 55, face=100, mode="stretch")      # pallet two
    prop("crate", 3738, -2832, 12, 48, 52, 44, face=84, mode="stretch")
    W.sph(3700, -2808, 28, 48, 34, 32, "batten")
    W.box(2700, 2752, -2626, -2620, 110, 172, "wood")           # the drivers' cards and the punch clock
    for r_ in range(3):
        for c in range(6):
            if rng.random() < 0.75:
                W.box(2704 + c * 8, 2709 + c * 8, -2628, -2626, 116 + r_ * 18, 128 + r_ * 18, "linen")
    W.box(2775, 2805, -2638, -2620, 128, 172, "steel")
    W.rod(2790, -2639, 160, 14, 1.5, "linen", yaw=90)
    W.box(3245, 3268, -3300, -3298, 140, 172, "batten")         # the manifest on its nail
    W.box(3247, 3266, -3298, -3297.4, 142, 168, "linen")
    W.box(3252, 3261, -3298.5, -3296.5, 166, 171, "pewter")


def cloakroom():
    """Coat check for three hundred: rails of coats and hats, the numbered tags on their board, the attendant's
    counter with the bell and the tip saucer, an umbrella stand by the door."""
    box = (-3200, -2100, 1420, 2000, 0, 430)
    hide([r"Coat_\d+", r"Cloak_Counter_\d+"], box)
    C.hide([r"Prop_Coatrack_Instances"], TAG)
    W.box(-2900, -2400, 1505, 1560, 8, 95, "wood")              # the counter
    W.box(-2908, -2392, 1497, 1566, 95, 100, "parquet")
    W.box(-2895, -2405, 1508, 1556, 0, 8, "black")
    for i in range(6):
        x0 = -2890 + i * 82
        W.box(x0 + 6, x0 + 76, 1503, 1505, 18, 82, "parquet")
    W.cyl(-2470, 1530, 100, 9, 2, "pewter")
    W.sph(-2470, 1530, 104, 7, 7, 5, "pewter")
    W.box(-2640, -2600, 1520, 1545, 100, 102, "wood")
    for i in range(5):
        W.rod(-2634 + i * 7, 1532, 103.5, 5, 1, "pewter", yaw=0)
    W.box(-2740, -2700, 1515, 1545, 100, 101.5, "linen")
    W.box(-2721, -2719, 1515, 1545, 101.5, 102, "black")
    for (x, yw) in ((-2830, 20), (-2812, 35)):
        W.box(x - 9, x + 9, 1520, 1528, 100, 102, "black", r=(0, 0, yw))
    W.cyl(-2560, 1530, 100, 12, 1.2, "linen")
    W.cyl(-2558, 1529, 101.2, 2.4, 0.4, "pewter")
    rails = ((-3085, 185, 175, 270, 1912), (-2885, 180, 170, 90, 1918), (-2690, 190, 178, 272, 1910),
             (-2495, 175, 172, 88, 1916), (-2295, 185, 176, 268, 1913))
    for (x, w, h, f, y) in rails:                               # the rails along the back wall, turned off each other
        prop("coatrack", x, y, 0, 70, w, h, face=f)
    prop("coatrack", -3108, 1702, 0, 70, 180, 172, face=0, collide=True)
    prop("coatrack", -2238, 1722, 0, 70, 175, 176, face=180, collide=True)
    W.box(-3195, -2105, 1962, 2000, 196, 200, "wood")           # the hat shelf
    for x in (-3150, -2650, -2150):
        W.box(x - 2, x + 2, 1975, 2000, 180, 196, "wood")
    for x in range(-3170, -2120, 68):
        if rng.random() < 0.25:
            continue
        hat(W, x + rng.uniform(-12, 12), 1982 + rng.uniform(-5, 5), 200,
            mat=rng.choice(("black", "cloth_a", "cloth_b", "cloth_c")), yaw=rng.uniform(0, 360))
    W.box(-2148, -2100, 1440, 1610, 0, 190, "wood")            # pigeonholes for bags and parcels
    for c in range(4):
        for r_ in range(6):
            y0, z0 = 1446 + c * 41, 8 + r_ * 30
            W.box(-2150, -2146, y0, y0 + 35, z0, z0 + 25, "black")
            if rng.random() < 0.4:
                W.box(-2153, -2149, y0 + 10, y0 + 18, z0 + 2, z0 + 14, "linen")
    W.box(-3200, -3196, 1600, 1820, 115, 200, "wood")          # the tag board: gaps are coats checked in
    for c in range(12):
        for z in (180, 155, 130):
            y = 1612 + c * 17.5
            W.box(-3196, -3193, y - 0.5, y + 0.5, z + 4, z + 6, "pewter")
            if rng.random() < 0.65:
                W.rod(-3194, y, z, 6, 1, "pewter", yaw=0)
    W.cyl(-2770, 1462, 0, 30, 52, "pewter")                     # the umbrella stand
    for (dx, dy, rl, pt) in ((-5, -4, 6, 4), (5, 3, -5, 7), (2, -6, 3, -6)):
        W.cyl(-2770 + dx, 1462 + dy, 10, 5, 82, "black", r=(rl, pt, 0))
        W.sph(-2770 + dx * 2.4, 1462 + dy * 2.4, 93, 6, 6, 6, "wood")
    W.box(-3175, -3145, 1620, 1700, 45, 52, "cloth_b")         # a coat and a hat box left on the bench
    W.cyl(-3160, 1760, 45, 32, 20, "linen")


def restrooms():
    """The gentlemen's: a row of basins under their mirrors, eight stalls with doors, someone's shoes under the door
    of the fourth, the attendant's table with towels and the tip saucer."""
    box = (-3200, -2100, -2000, -1420, 0, 430)
    hide([r"Stall_\d+", r"Toilet_\d+", r"Sink_Counter_\d+", r"Mirror_\d+"], box)
    xs = [-2900.0 + 100 * i for i in range(8)]
    for x in xs:                                                # partitions on pewter feet
        W.box(x - 2.5, x + 2.5, -2000, -1760, 15, 205, "wood")
        W.box(x - 3, x + 3, -1768, -1758, 0, 15, "pewter")
    W.box(-2903, -2100, -1764, -1756, 200, 205, "pewter")
    for i, x in enumerate(xs):
        nxt = xs[i + 1] if i < 7 else -2100.0
        ang = (0, 40, 75, 0, 20, 90, 0, 55)[i]
        At(x + 3, -1760, 0, -ang).box(0, 70, -1.5, 1.5, 15, 190, "wood")
        W.box(x + 73, nxt - 2.5, -1761.5, -1758.5, 15, 190, "wood")
        cx = x + 50
        W.cyl(cx, -1950, 0, 24, 30, "linen", dy=30)
        W.cyl(cx, -1940, 30, 38, 10, "linen", dy=48)
        W.cyl(cx, -1938, 40, 40, 2, "wood", dy=50)
        W.box(cx - 22, cx + 22, -2000, -1982, 188, 214, "black")
        W.box(cx - 2, cx + 2, -1996, -1992, 42, 188, "pewter")
        W.box(cx + 15, cx + 16, -1984, -1983, 140, 188, "pewter")
        W.cyl(cx + 15.5, -1983.5, 132, 3, 8, "black")
    for (x0, y0) in ((-2560, -1812), (-2535, -1814)):           # stall four is occupied
        W.box(x0, x0 + 10, y0, y0 + 26, 0, 8, "black")
        W.box(x0 - 1, x0 + 11, y0, y0 + 12, 8, 15, "cloth_a")
    W.cyl(-3050, -1950, 0, 50, 55, "batten")                    # the laundry basket
    W.box(-3065, -3035, -1960, -1940, 55, 60, "linen")
    for y in (-1830.0, -1690.0, -1550.0):                       # basins and mirrors
        prop("rooms_sink", -3170, y, 0, 58, 62, 92, face=0)
        W.box(-3200, -3197, y - 34, y + 34, 120, 205, "pewter")
        W.box(-3197, -3196, y - 30, y + 30, 124, 201, "glass")
        W.box(-3200, -3186, y - 28, y + 28, 112, 114, "pewter")
        if y < -1600:
            W.box(-3198, -3193, y + 53, y + 87, 121, 123, "pewter")
            W.box(-3197, -3194, y + 55, y + 85, 84, 121, "linen")
    W.box(-2270, -2160, -1492, -1440, 74, 78, "wood")           # the attendant's table
    for x in (-2266, -2168):
        for y in (-1488, -1448):
            W.box(x, x + 4, y, y + 4, 0, 74, "wood")
    for i in range(3):
        W.box(-2254 + i * 2, -2226 + i * 2, -1476 + i, -1456 + i, 78 + i * 4, 82 + i * 4, "linen")
    W.cyl(-2195, -1470, 78, 12, 1.2, "linen")
    W.cyl(-2193, -1469, 79.2, 2.4, 0.4, "pewter")
    W.box(-2215, -2200, -1458, -1452, 78, 82, "wood")
    for x in (-2182, -2172):
        W.cyl(x, -1452, 78, 6, 14, "glass")
        W.sph(x, -1452, 94, 3, 3, 4, "glass")
    stool(W, -2215, -1540)
    W.cyl(-2140, -1680, 0, 34, 30, "pewter")                    # the mop and bucket
    W.bar((-2140, -1680, 20), (-2112, -1690, 150), 3, 3, "wood")


def office():
    """The house manager's office: the seating plan pinned up, ledgers and the typewriter on the desk, the filing,
    and the picture over the wall safe swung open."""
    Z = F2
    hide([r"Office_Desk_\d+", r"Office_Screen_\d+", r"Office_Cabinet_\d+"], (-3200, -2100, 1420, 2000, 430, 900))
    sz = prop("rooms_desk", -2700, 1850, Z, 100, 205, 110, face=90)
    hw = (sz[1] / 2 if sz[1] else 100.0) + 3
    for (x0, x1, side) in ((-2900, -2700 - hw, -1), (-2700 + hw, -2500, 1)):
        if x1 - x0 < 25:
            continue
        W.box(x0, x1, 1805, 1895, Z, Z + 70, "wood")
        W.box(x0 - 1, x1 + 1, 1803, 1897, Z + 70, Z + 73, "parquet")
        for z in (25, 48):
            W.box(x0 + 3, x1 - 3, 1804, 1805, Z + z, Z + z + 0.8, "black")
        cx = (x0 + x1) / 2
        if side < 0:
            W.box(cx - 15, cx + 15, 1838, 1862, Z + 73, Z + 79, "pewter")
            W.box(cx - 13, cx + 13, 1840, 1860, Z + 79, Z + 81, "linen")
        else:
            books(W, cx, 1850, Z + 73, 3, yaw=8)
    for y in (1525.0, 1573.0, 1621.0):                          # the filing, one drawer out
        prop("rooms_filing", -3167, y, Z, 64, 46, 132, face=0)
    W.box(-3135, -3092, 1555, 1591, Z + 88, Z + 112, "grey")
    for i in range(6):
        W.box(-3130 + i * 6, -3128 + i * 6, 1558, 1588, Z + 100, Z + 118, "linen")
    books(W, -3165, 1525, Z + 133, 4, yaw=90)
    W.cyl(-3165, 1621, Z + 132, 14, 3, "black")                 # the desk fan
    W.cyl(-3165, 1621, Z + 135, 3, 14, "black")
    W.sph(-3160, 1621, Z + 158, 10, 26, 26, "pewter")
    W.box(-3045, -2965, 1993, 2000, Z + 118, Z + 198, "steel")  # the wall safe, the picture swung off it
    W.box(-3038, -2972, 1990, 1993, Z + 125, Z + 191, "grey")
    W.rod(-3005, 1988.5, Z + 165, 13, 3, "pewter", yaw=90)
    W.box(-2990, -2975, 1987, 1990, Z + 148, Z + 152, "pewter")
    At(-3055, 1996, Z, -72).box(0, 100, -2.5, 2.5, 110, 200, "wood")
    At(-3055, 1996, Z, -72).box(5, 95, -3, 3, 115, 195, "canvas")
    W.box(-2104, -2100, 1445, 1610, Z + 105, Z + 195, "batten")   # tonight's seating plan
    W.box(-2105, -2104, 1460, 1595, Z + 182, Z + 190, "linen")
    for c in range(5):
        for r_ in range(3):
            W.rod(-2105, 1466 + c * 32 + rng.uniform(-4, 4), Z + 122 + r_ * 22 + rng.uniform(-3, 3), 9, 1,
                  "linen", yaw=0)
    W.cyl(-3160, 1760, Z, 30, 3, "wood")                        # the hat tree, coat and hat on it
    W.cyl(-3160, 1760, Z + 3, 4, 180, "wood")
    W.box(-3172, -3150, 1745, 1785, Z + 85, Z + 170, "cloth_c")
    hat(W, -3160, 1760, Z + 183, "cloth_b")
    W.box(-3195, -3125, 1918, 1992, Z, Z + 95, "wood")          # the radio
    W.box(-3125, -3124, 1930, 1980, Z + 45, Z + 85, "linen")
    W.rod(-3124, 1955, Z + 30, 10, 1.5, "pewter", yaw=0)
    W.box(-2960, -2440, 1640, 1975, Z, Z + 1, "velvet")         # the rug
    hat(W, -2800, 1730, Z + 46, "black")


def security():
    """The house detective's post, 1940s: the duty desk under the door-lamp board, the key cabinet open, the
    switchboard, the lockers, a thermos and half a sandwich."""
    Z = F2
    hide([r"Security_Monitor_\d+", r"Server_Lights_\d+", r"Server_Rack_\d+", r"Lockers_\d+", r"Security_Desk_\d+"],
         (-3200, -2100, -2000, -1420, 430, 900))
    W.box(-3005, -2295, -1965, -1875, Z + 76, Z + 80, "wood")   # the duty desk
    W.box(-3000, -2300, -1965, -1960, Z + 10, Z + 76, "wood")
    W.box(-3000, -2930, -1960, -1880, Z, Z + 76, "wood")
    W.box(-2370, -2300, -1960, -1880, Z, Z + 76, "wood")
    for z in (25, 50):
        W.box(-2995, -2935, -1880, -1879, Z + z, Z + z + 0.8, "black")
        W.box(-2365, -2305, -1880, -1879, Z + z, Z + z + 0.8, "black")
    W.box(-2870, -2830, -1915, -1885, Z + 80, Z + 81.5, "linen")
    W.box(-2825, -2824, -1915, -1900, Z + 80.5, Z + 81.5, "black")
    W.cyl(-2760, -1925, Z + 80, 9, 26, "grey")
    W.cyl(-2760, -1925, Z + 106, 7, 5, "black")
    W.cyl(-2742, -1910, Z + 80, 7, 7, "linen")
    W.box(-2722, -2700, -1936, -1914, Z + 80, Z + 80.4, "linen")
    W.box(-2718, -2704, -1932, -1918, Z + 80.4, Z + 83, "batten")
    W.box(-2700, -2691, -1930, -1921, Z + 80.4, Z + 83, "batten", r=(0, 0, 20))
    phone(W, -2600, -1925, Z + 80)
    desk_lamp(W, -2965, -1930, Z + 80)
    W.cyl(-2650, -1900, Z + 80, 12, 2, "pewter")
    W.box(-2652, -2646, -1901, -1900, Z + 82, Z + 83, "linen")
    W.box(-2450, -2400, -1945, -1905, Z + 80, Z + 81, "linen", r=(0, 0, 12))
    hat(W, -2340, -1910, Z + 80, "cloth_b")
    W.box(-2960, -2340, -2000, -1996, Z + 100, Z + 235, "wood")   # the door-lamp board: the building, a lamp a door
    W.box(-2945, -2355, -1996, -1995, Z + 110, Z + 225, "canvas_dk")
    for (x0, x1, z0, z1) in ((-2910, -2390, 128, 129.5), (-2910, -2390, 214, 215.5), (-2910, -2908.5, 128, 215.5),
                             (-2391.5, -2390, 128, 215.5), (-2700, -2698.5, 128, 215.5), (-2550, -2548.5, 128, 180),
                             (-2910, -2700, 175, 176.5)):
        W.box(x0, x1, -1995, -1994.5, Z + z0, Z + z1, "linen")
    lit = set(rng.sample(range(39), 5))
    for r_, z in enumerate((140, 165, 190)):
        for c in range(13):
            W.sph(-2890 + c * 40, -1994, Z + z, 4.5, 3, 4.5, "glow" if (r_ * 13 + c) in lit else "steel")
    prop("rooms_switchboard", -2160, -1912, Z, 70, 150, 170, face=180)   # the switchboard and its stool
    stool(W, -2232, -1912, Z, h=55)
    W.box(-2112, -2100, -1610, -1460, Z + 110, Z + 205, "wood")   # the key cabinet, one door open
    W.box(-2113, -2112, -1604, -1466, Z + 116, Z + 199, "canvas_dk")
    for c in range(8):
        for z in (188, 163, 138):
            y = -1596 + c * 17
            W.box(-2116, -2112, y - 0.4, y + 0.4, Z + z, Z + z + 1, "pewter")
            if rng.random() < 0.8:
                W.box(-2115, -2113, y - 0.5, y + 0.5, Z + z - 6, Z + z, "pewter")
                W.box(-2116, -2114, y - 1.8, y + 1.8, Z + z - 12, Z + z - 6, "linen")
    At(-2112, -1460, Z, 160).box(0, 75, -1.5, 1.5, 110, 205, "wood")
    prop("rooms_lockers", -3162, -1725, Z, 62, 150, 192, face=0)
    prop("rooms_lockers", -3162, -1575, Z, 62, 150, 192, face=0)
    W.box(-3200, -3196, -1500, -1430, Z + 165, Z + 172, "wood")   # a trench coat on the hook
    W.box(-3196, -3180, -1490, -1450, Z + 80, Z + 165, "cloth_c")
    W.rod(-2650, -1423, Z + 275, 30, 4, "linen", yaw=90)        # the clock over the door
    W.rod(-2650, -1422, Z + 275, 33, 3, "black", yaw=90)
    W.box(-2651, -2649, -1426, -1425, Z + 275, Z + 287, "black")
    W.box(-2650, -2642, -1426, -1425, Z + 274, Z + 276, "black")
    W.box(-3100, -2860, -1424, -1420, Z + 120, Z + 200, "batten")   # the noticeboard: wanted, rota, memos
    for (x, z, w, h) in ((-3090, 140, 40, 52), (-3040, 150, 26, 34), (-3005, 135, 30, 40), (-2965, 148, 24, 32),
                         (-2925, 130, 36, 48), (-2885, 152, 18, 26)):
        W.box(x, x + w, -1425, -1424.5, Z + z, Z + z + h, "linen", r=(rng.uniform(-3, 3), 0, 0))


def stairs():
    """The fire stairs, kept for staff: a hose reel on the north one, a mop, a broom and fire buckets on the south."""
    W.box(-2080, -2073, 1695, 1705, 120, 180, "steel")
    W.rod(-2068, 1700, 150, 56, 12, "steel", yaw=0)
    W.rod(-2062, 1700, 150, 48, 10, "black", yaw=0)
    W.cyl(-2060, -1700, 0, 34, 30, "pewter")
    W.bar((-2052, -1692, 25), (-2076, -1712, 150), 3, 3, "wood")
    W.bar((-2075, -1640, 10), (-2078, -1650, 140), 3, 3, "wood")
    W.box(-2079, -2070, -1662, -1618, 0, 12, "black")
    W.box(-2080, -2077, -1600, -1480, 150, 160, "wood")
    for y in (-1580, -1540, -1500):
        W.cyl(-2066, y, 118, 22, 26, "steel")
        W.box(-2078, -2066, y - 1, y + 1, 144, 152, "steel")


def gallery():
    """Tonight's lots on view: each on its pedestal with its card, roped off with a rope that sags, a salon hang of
    smaller pictures above, the star lot on an easel, jewellery in a vitrine, catalogues on the lectern and benches."""
    box = (-1000, 2300, 2020, 2900, 0, 430)
    hide([r"Preview_Plinth_\d+", r"Preview_Bench_\d+", r"Rope_\d+"], box)
    for x in (-600.0, 0.0, 600.0, 1200.0, 1800.0):
        prop("pedestal", x, 2745, 0, 90, 90, 100, face=270, mode="stretchz")
        W.box(x - 14, x + 14, 2696, 2698.5, 62, 80, "linen")
        W.box(x - 15, x + 15, 2698, 2699.5, 61, 81, "pewter")
    posts = (-800.0, -300.0, 300.0, 900.0, 1500.0, 2000.0)
    for (a, b) in zip(posts, posts[1:]):                        # ropes that hang from the posts
        m = (a + b) / 2
        W.bar((a + 6, 2640, 60), (m, 2640, 47), 3.5, 3.5, "velvet")
        W.bar((m, 2640, 47), (b - 6, 2640, 60), 3.5, 3.5, "velvet")
    for x in posts:
        W.sph(x, 2640, 61, 5, 5, 5, "pewter")
    for (x0, x1) in ((-200, 230), (330, 870), (970, 1400)):     # banquettes and the catalogue tables between
        W.box(x0 + 2, x1 - 2, 2382, 2438, 38, 48, "velvet")
        W.box(x0 + 6, x1 - 6, 2386, 2434, 30, 38, "wood")
        for x in (x0 + 12, x1 - 16, (x0 + x1) / 2):
            W.box(x, x + 4, 2388, 2392, 0, 30, "wood")
            W.box(x, x + 4, 2428, 2432, 0, 30, "wood")
    for (x0, x1) in ((230, 330), (870, 970)):
        W.box(x0 + 5, x1 - 5, 2385, 2435, 42, 46, "wood")
        for x in (x0 + 8, x1 - 12):
            W.box(x, x + 4, 2388, 2392, 0, 42, "wood")
            W.box(x, x + 4, 2428, 2432, 0, 42, "wood")
        books(W, (x0 + x1) / 2, 2410, 46, 3)
    for (cx, w, h, z0, mat) in ((-600, 60, 80, 318, "canvas"), (0, 90, 60, 334, "canvas_dk"), (600, 50, 50, 342, "canvas"),
                                (1200, 80, 100, 312, "canvas_dk"), (1800, 70, 50, 340, "canvas")):
        wall_frame(cx, 2900, z0, w, h, -1, mat)                  # the salon hang above the lots
        W.box(cx - 0.5, cx + 0.5, 2897, 2898, z0 + h, 420, "black")
    W.box(-1000, 2300, 2896, 2900, 418, 422, "pewter")          # the picture rail and the big pictures' wires
    for c in (-900.0, -300.0, 300.0, 900.0, 1500.0, 2100.0):
        for s in (-1, 1):
            W.box(c + s * 40 - 0.5, c + s * 40 + 0.5, 2897, 2898, 298, 418, "black")
        W.box(c + 100, c + 113, 2897.5, 2900, 150, 158, "linen")
    W.box(-790, -610, 2180, 2260, 0, 82, "wood", collide=True)  # the jewellery vitrine
    W.box(-785, -615, 2185, 2255, 82, 84, "velvet")
    for i in range(9):
        a = math.pi * i / 8
        W.sph(-740 + 22 * math.cos(a), 2215 + 14 * math.sin(a), 85.5, 3.4, 3.4, 3.4, "linen")
    for (x, y) in ((-670, 2205), (-655, 2225), (-640, 2210)):
        W.cyl(x, y, 84, 3, 1, "pewter")
    W.sph(-690, 2240, 85, 5, 5, 3, "pewter")
    W.box(-628, -612, 2190, 2200, 84, 92, "linen", r=(0, 0, 15))
    ea = At(2140, 2690, 0, 205)                                 # the star lot on its easel
    for s in (-1, 1):
        ea.box(-3, 3, s * 35 - 2.5, s * 35 + 2.5, 0, 185, "wood", r=(0, 8, 0))
    ea.box(-40, -35, -2, 2, 0, 172, "wood", r=(0, -14, 0))
    ea.box(0, 12, -48, 48, 66, 70, "wood")
    ea.box(0, 8, -45, 45, 72, 142, "canvas_dk", r=(0, 8, 0))
    ea.box(2, 6, -50, 50, 67, 147, "pewter", r=(0, 8, 0))
    lc = At(1650, 2095, 0, 90)                                  # the catalogue lectern by the hall door
    lc.box(-22, 22, -22, 22, 0, 5, "wood")
    lc.box(-11, 11, -11, 11, 5, 100, "wood", collide=True)
    lc.box(-20, 20, -28, 28, 98, 102, "wood", r=(0, -15, 0))
    lc.box(-15, 15, -21, 21, 102, 103.5, "linen", r=(0, -15, 0))
    prop("chair", -905, 2150, 0, 55, 55, 92, face=10, collide=True)   # the attendant's chair


def stock():
    """Lots waiting their turn: crated, one being unpacked in its straw, a crated painting on edge, the porter's
    trolley, the packing bench, shelves of small lots, canvases in the rack."""
    hide([r"Stock_Shelf_\d+"], (2320, 4400, 2020, 2900, 0, 430))
    prop("crate", 2700, 2752, 0, 165, 190, 118, face=94, mode="stretch")
    prop("crate", 2672, 2738, 118, 62, 78, 52, face=112, mode="stretch")
    prop("crate", 2525, 2815, 0, 52, 60, 44, face=71, mode="stretch")
    oc = At(3100, 2752, 0, 3)                                   # the crate being unpacked, its lid leaning on it
    for (x0, x1, y0, y1) in ((-88, 88, -76, -70), (-88, 88, 70, 76), (-88, -82, -70, 70), (82, 88, -70, 70)):
        oc.box(x0, x1, y0, y1, 0, 105, "batten")
    oc.box(-82, 82, -70, 70, 0, 8, "batten")
    for z in (0, 93):
        oc.box(-88, 88, -78, -76, z, z + 12, "batten")
    oc.box(-80, 80, -78.5, -76.5, 45, 56, "batten", r=(0, 30, 0))
    oc.box(100, 106, -88, 88, 0, 132, "batten", r=(0, 15, 0))
    for (x, y, z, sx, sy, sz) in ((-40, -20, 98, 76, 62, 26), (32, 25, 96, 80, 66, 24), (0, -42, 100, 62, 40, 20),
                                  (-125, 40, 0, 84, 62, 18), (118, -98, 0, 62, 50, 14)):
        oc.sph(x, y, z, sx, sy, sz, "tape")
    W.sph(2955, 2640, 0, 70, 40, 12, "tape")
    prop("bust", 3110, 2760, 68, 45, 45, 55, face=270)
    W.box(2990, 3062, 2652, 2656, 1, 4, "black", r=(0, 0, 25))
    prop("crate", 3455, 2760, 0, 110, 105, 100, face=88, mode="stretch")   # a cube crate and a picture crate on edge
    prop("crate", 3585, 2765, 0, 36, 190, 150, face=0, mode="stretch")
    prop("crate", 3950, 2742, 0, 118, 112, 96, face=91, mode="stretch")
    W.cyl(3940, 2735, 96, 48, 58, "batten")
    for z in (104, 140):
        W.cyl(3940, 2735, z, 49, 3, "black")
    prop("crate", 4200, 2200, 0, 98, 100, 88, face=118, mode="stretch")
    for (x, w) in ((2650.0, 270.0), (3200.0, 280.0), (3722.0, 255.0)):     # shelves of small lots
        prop("rooms_shelving", x, 2058, 0, 62, w, 215, face=90)
    for x in (2885.0, 2930.0):                                  # a ladder against the wall
        W.bar((x, 2088, 0), (x, 2030, 224), 5, 5, "wood")
    for z in range(25, 222, 30):
        y = 2088 - 58 * z / 224
        W.box(2885, 2930, y - 2, y + 2, z - 2, z + 2, "wood")
    W.cyl(3010, 2050, 0, 34, 200, "canvas_dk", r=(0, 4, 0))     # rolled carpets on end
    W.cyl(3048, 2058, 0, 26, 178, "velvet", r=(3, -3, 0))
    for (x, w, h, k) in ((3400, 100, 140, 0), (3470, 80, 110, 1), (3530, 60, 80, 2)):   # empty frames leaning
        y0 = 2078 + k * 5
        W.bar((x, y0, 0), (x, y0 - 34, h), w, 3, "pewter")
        W.bar((x, y0 + 2, 6), (x, y0 - 30, h - 6), w - 12, 3, "ply")
    for y in (2560, 2725, 2890):                                # the canvas rack
        W.box(2330, 2460, y - 3, y + 3, 0, 6, "wood")
        W.box(2455, 2461, y - 3, y + 3, 0, 200, "wood")
    W.box(2455, 2461, 2557, 2893, 194, 200, "wood")
    for (x, y0, y1, h, mat) in ((2360, 2575, 2800, 165, "linen"), (2380, 2600, 2870, 140, "ply"),
                                (2403, 2580, 2740, 120, "canvas_dk")):
        W.box(x, x + 6, y0, y1, 6, 6 + h, mat)
    t = At(3420, 2190, 0, 4)                                    # the porter's trolley with a crate on it
    t.box(-75, 75, -35, 35, 14, 19, "wood", collide=True)
    for s in (-1, 1):
        t.box(-78, -74, s * 31 - 2, s * 31 + 2, 19, 100, "pewter")
    t.box(-78, -74, -33, 33, 96, 100, "pewter")
    for sx in (-1, 1):
        for sy in (-1, 1):
            t.cyl(sx * 65, sy * 28, 3.5, 12, 5, "black", r=(90, 0, 0))
    prop("crate", 3425, 2192, 19, 70, 62, 52, face=96, mode="stretch")
    W.box(4285, 4390, 2600, 2880, 85, 90, "wood")               # the packing bench
    for x in (4290, 4380):
        for y in (2605, 2869):
            W.box(x, x + 6, y, y + 6, 0, 85, "wood")
    W.box(4290, 4386, 2605, 2875, 20, 23, "wood")
    W.rod(4330, 2740, 31, 16, 120, "batten", yaw=90)
    W.rod(4358, 2745, 30, 14, 110, "ply", yaw=90)
    W.box(4310, 4313, 2650, 2680, 90, 92, "wood")
    W.box(4306, 4318, 2678, 2682, 90, 94, "black")
    W.sph(4330, 2720, 95, 10, 10, 10, "batten")
    W.box(4340, 4375, 2760, 2800, 90, 90.6, "linen")
    for i in range(4):
        W.box(4300 + i * 9, 4306 + i * 9, 2820 + i * 4, 2830 + i * 4, 90, 90.4, "linen", r=(0, 0, rng.uniform(-30, 30)))


def director():
    """Mr Vance's office over the hall: the big desk facing the window, posters of his pictures, the drinks
    cabinet, a movie camera kept as a trophy, the blinds half down."""
    Z = F2
    hide([r"Director_Desk_\d+", r"Desk_Screen_\d+", r"Award_\d+"], (-1000, 800, 2020, 2900, 430, 900))
    sz = prop("rooms_desk", -100, 2705, Z, 110, 240, 118, face=90)
    hw = (sz[1] / 2 if sz[1] else 110.0) + 3
    W.cyl(-310, 2705, Z, 40, 3, "wood")                         # the globe bar
    for a in (90, 210, 330):
        c, s = math.cos(math.radians(a)), math.sin(math.radians(a))
        W.bar((-310 + 18 * c, 2705 + 18 * s, Z + 3), (-310 + 10 * c, 2705 + 10 * s, Z + 58), 3, 3, "wood")
    W.sph(-310, 2705, Z + 82, 56, 56, 56, "parquet")
    W.cyl(-310, 2705, Z + 81, 60, 3, "pewter")
    x0 = -100 + hw
    if 200 - x0 > 40:                                           # the credenza, his statuette on it
        W.box(x0, 200, 2662, 2748, Z, Z + 72, "wood")
        W.box(x0 - 1, 201, 2660, 2750, Z + 72, Z + 75, "parquet")
        cx = (x0 + 200) / 2
        W.box(cx + 20, cx + 36, 2697, 2713, Z + 75, Z + 80, "black")
        W.cyl(cx + 28, 2705, Z + 80, 5, 22, "pewter")
        W.sph(cx + 28, 2705, Z + 105, 6, 6, 7, "pewter")
        W.cyl(cx - 20, 2700, Z + 75, 36, 4, "steel")
        W.box(cx - 8, cx + 7, 2725, 2727, Z + 75, Z + 93, "pewter", r=(0, 0, -15))
    W.box(-570, -130, 2020, 2026, Z + 92, Z + 100, "wood")      # the window onto the hall, blinds half down
    W.box(-570, -130, 2020, 2024, Z + 300, Z + 308, "wood")
    for (a, b) in ((-570, -560), (-140, -130), (-354, -346)):
        W.box(a, b, 2020, 2024, Z + 100, Z + 300, "wood")
    W.box(-560, -140, 2020, 2021, Z + 100, Z + 300, "glass")
    for z in range(int(Z) + 192, int(Z) + 298, 6):
        W.box(-556, -144, 2023, 2027, z, z + 0.8, "linen", r=(25, 0, 0))
    for x in (-500, -200):
        W.box(x, x + 0.5, 2026, 2026.5, Z + 150, Z + 300, "linen")
        W.cyl(x, 2026, Z + 145, 1.5, 5, "wood")
    wall_frame(-700, 2900, Z + 110, 100, 150, -1, "canvas_dk")  # posters and photographs on the north wall
    wall_frame(-250, 2900, Z + 150, 80, 110, -1, "canvas")
    for (cx, z0, w, h) in ((-110, Z + 200, 30, 40), (-60, Z + 210, 26, 22), (-75, Z + 165, 34, 26)):
        wall_frame(cx, 2900, z0, w, h, -1, "canvas_dk")
    W.box(745, 800, 2620, 2790, Z, Z + 100, "wood")             # the drinks cabinet
    W.box(743, 800, 2618, 2792, Z + 100, Z + 103, "parquet")
    for (y, h) in ((2650, 20), (2680, 24), (2712, 18)):
        W.cyl(770, y, Z + 103, 10, h, "glass")
        W.sph(770, y, Z + 107 + h, 5, 5, 6, "glass")
    for i in range(3):
        W.cyl(765, 2740 + i * 12, Z + 103, 6, 8, "pewter")
    W.cyl(770, 2775, Z + 103, 9, 26, "grey")
    W.box(765, 776, 2771, 2779, Z + 129, Z + 136, "pewter")
    prop("camera", -870, 2790, Z, 80, 70, 175, face=315)        # a camera from his first picture
    W.cyl(-950, 2660, Z, 30, 3, "pewter")                       # the floor lamp by the sofa
    W.cyl(-950, 2660, Z + 3, 3, 150, "pewter")
    W.cyl(-950, 2660, Z + 150, 40, 28, "linen")
    W.box(-800, -775, 2390, 2410, Z + 40, Z + 46, "wood")       # cigars and a script on the low table
    W.cyl(-760, 2415, Z + 40, 13, 2, "glass")
    books(W, -790, 2375, Z + 40, 2, yaw=15)


def projection():
    """The booth: two 35 mm projectors at their ports for the changeover, lamp-house chimneys to the roof, the
    rewind bench with its reels, film cans shelved and stacked by the machines, a bucket of sand."""
    Z = F2
    box = (820, 2600, 2020, 2900, 430, 900)
    hide([r"Projector_\d+", r"Projector_Stand_\d+", r"Film_Can_\d+", r"Reel_\d+", r"Edit_Screen_\d+", r"Reel_Shelf_\d+"],
         box)
    hide([r"Table_\d+", r"Table_Foot_\d+"], (2150, 2350, 2300, 2400, 430, 900))
    for x in (1490.0, 1910.0):
        prop("rooms_projector", x, 2128, Z, 120, 90, 195, face=270)
        W.cyl(x, 2140, Z + 185, 14, 900 - (Z + 185), "pewter")
        W.box(x - 26, x + 26, 2020, 2022, Z + 124, Z + 172, "steel")
        W.box(x - 22, x + 22, 2020, 2023, Z + 128, Z + 168, "glass")
        W.box(x - 28, x + 28, 2020, 2024, Z + 176, Z + 212, "steel")
        for s in (-1, 1):
            W.box(x + s * 29.5 - 1.5, x + s * 29.5 + 1.5, 2020, 2024, Z + 120, Z + 215, "pewter")
        W.box(x + 48, x + 68, 2020, 2023, Z + 148, Z + 164, "glass")
    W.cyl(1700, 2090, Z, 30, 34, "steel")                       # sand for a film fire
    W.cyl(1700, 2090, Z + 32, 28, 2.5, "tape")
    W.cyl(1340, 2090, Z, 40, 9, "steel")                        # cans by the machines: the next reels up
    W.cyl(1346, 2084, Z + 9, 38, 9, "grey")
    W.rod(1300, 2122, Z + 20, 40, 9, "steel", yaw=30)
    W.cyl(2060, 2100, Z, 40, 9, "steel")
    W.cyl(2062, 2102, Z + 9, 36, 3, "pewter")
    W.cyl(2062, 2102, Z + 12, 8, 1, "black")
    for x in (1150.0, 1505.0):                                  # the can shelves on the north wall
        prop("rooms_filmshelf", x, 2862, Z, 60, 280, 205, face=270)
    W.box(1700, 2390, 2852, 2895, Z, Z + 95, "wood")            # the changeover cabinet, reels racked on top
    W.box(1698, 2392, 2850, 2896, Z + 95, Z + 98, "parquet")
    W.box(1950, 2150, 2860, 2890, Z + 98, Z + 100, "steel")
    for i in range(8):
        W.rod(1962 + i * 24, 2875, Z + 118, 36, 3, "pewter", yaw=0)
    for (x, y, z, d) in ((1760, 2872, 98, 38), (1765, 2869, 106, 34), (1852, 2876, 98, 30), (2230, 2870, 98, 40)):
        W.cyl(x, y, Z + z, d, 8, "steel" if d > 33 else "grey")
    W.box(820, 824, 2150, 2320, Z + 120, Z + 200, "black")      # the changeover chart in chalk
    for (y0, ln, z) in ((2160, 110, 186), (2160, 80, 172), (2170, 130, 158), (2160, 60, 144), (2175, 95, 130)):
        W.box(824, 824.5, y0, y0 + ln, Z + z, Z + z + 1.5, "linen")
    W.box(2160, 2340, 2310, 2390, Z + 78, Z + 82, "wood")       # the rewind bench
    for x in (2165, 2331):
        for y in (2315, 2381):
            W.box(x, x + 4, y, y + 4, Z, Z + 78, "wood")
    for x in (2181, 2319):
        W.box(x - 3, x + 3, 2347, 2353, Z + 82, Z + 112, "pewter")
        W.rod(x, 2350, Z + 112, 38, 3, "pewter", yaw=90)
        W.box(x - 3, x + 3, 2353, 2368, Z + 110, Z + 113, "black")
    W.box(2192, 2308, 2348.5, 2351.5, Z + 94, Z + 94.3, "black")
    W.box(2240, 2260, 2345, 2355, Z + 82, Z + 88, "black")
    W.box(2215, 2285, 2365, 2385, Z + 82, Z + 84, "screen")


def archive():
    """The film archive: rows of can shelving, the library ladder on a row, the can trolley in an aisle, the card
    catalogue by the archivist's desk, the filing along the east wall."""
    Z = F2
    hide([r"Archive_Shelf_\d+", r"Filing_Cabinets_\d+"], (2620, 4400, 2020, 2900, 430, 900))
    for ri, cx in enumerate((2930.0, 3330.0, 3730.0, 4130.0)):
        for ui, yc in enumerate((2391.0, 2575.0, 2759.0)):
            if ri == 3 and ui == 1:                             # one bay is an open rack of cans on edge
                for (sx, sy) in ((-26, -88), (26, -88), (-26, 88), (26, 88)):
                    W.box(cx + sx - 2, cx + sx + 2, yc + sy - 2, yc + sy + 2, Z, Z + 215, "pewter")
                for z in (6, 72, 138, 204):
                    W.box(cx - 29, cx + 29, yc - 91, yc + 91, Z + z, Z + z + 2, "steel")
                    if z < 200:
                        y = yc - 80
                        while y < yc + 80:
                            if rng.random() < 0.8:
                                W.rod(cx, y, Z + z + 2 + 19, 38, 4, "steel" if rng.random() < 0.6 else "grey", yaw=90)
                            y += rng.uniform(6, 14)
                continue
            prop("rooms_filmshelf", cx, yc, Z, 58, 180, 215, face=0 if (ri + ui) % 2 == 0 else 180)
    for y in (2560.0, 2602.0):                                  # the library ladder on row two
        W.bar((3226, y, Z), (3297, y, Z + 218), 5, 5, "wood")
    for z in range(25, 214, 30):
        x = 3226 + 71 * z / 218
        W.box(x - 2, x + 2, 2560, 2602, Z + z - 2, Z + z + 2, "wood")
    W.box(3294, 3302, 2556, 2606, Z + 214, Z + 218, "pewter")
    tr = At(3130, 2700, Z, 90)                                  # the can trolley in the aisle
    for z in (18, 70):
        tr.box(-45, 45, -28, 28, z, z + 3, "steel")
    for sx in (-1, 1):
        for sy in (-1, 1):
            tr.box(sx * 43 - 2, sx * 43 + 2, sy * 26 - 2, sy * 26 + 2, 6, 92, "pewter")
            tr.cyl(sx * 40, sy * 22, 0, 8, 6, "black")
    for (x, y, z, d) in ((-20, 0, 73, 38), (-17, 3, 81, 36), (20, -2, 73, 38), (0, 0, 21, 40)):
        tr.cyl(x, y, z, d, 8, "steel")
    W.box(2625, 2685, 2560, 2760, Z, Z + 125, "wood")           # the card catalogue, one drawer out
    for c in range(7):
        for r_ in range(9):
            y0, z0 = 2564 + c * 27.5, Z + 8 + r_ * 12.8
            W.box(2685, 2686.5, y0, y0 + 26, z0, z0 + 11, "parquet")
            W.box(2686.5, 2688, y0 + 11, y0 + 15, z0 + 5, z0 + 7, "pewter")
    W.box(2685, 2728, 2619, 2645, Z + 84, Z + 95, "parquet")
    for i in range(6):
        W.box(2690 + i * 6, 2691 + i * 6, 2621, 2643, Z + 88, Z + 96, "linen")
    desk_lamp(W, 2722, 2172, Z + 75)
    books(W, 2800, 2160, Z + 75, 2, yaw=20)
    for y in (2130.0, 2178.0, 2226.0, 2274.0, 2560.0, 2608.0):  # filing along the east wall
        prop("rooms_filing", 4368, y, Z, 58, 46, 132, face=180)
    W.box(4300, 4342, 2206, 2246, Z + 88, Z + 112, "grey")
    for i in range(6):
        W.box(4305 + i * 6, 4307 + i * 6, 2209, 2243, Z + 100, Z + 118, "linen")
    W.box(4340, 4385, 2400, 2440, Z, Z + 20, "wood")            # a step stool
    W.box(4362, 4385, 2400, 2440, Z + 20, Z + 40, "wood")


def vaultcor():
    """The vault's anteroom: the round door swung open in its steel surround, the guard's desk with the sign-in
    book, a bench for depositors, an opened strongbox on the viewing table."""
    hide([r"Vault_Desk_Screen_\d+", r"Keypad_\d+", r"Vault_Door_\d+", r"Vault_Desk_\d+"], (4420, 5300, -2600, -990, 0, 500))
    prop("rooms_vaultdoor", 5012, -1128, 0, 75, 255, 262, face=0)
    W.box(4715, 4752, -1018, -1000, 0, 292, "steel")            # the surround
    W.box(4968, 5000, -1018, -1000, 0, 292, "steel")
    W.box(4715, 5000, -1018, -1000, 260, 292, "steel")
    W.box(4700, 4740, -1004, -1000, 115, 160, "pewter")         # the time lock and its dial
    W.rod(4720, -1006, 140, 16, 3, "black", yaw=90)
    W.rod(4720, -1005, 124, 10, 2, "linen", yaw=90)
    W.box(4522, 4598, -1698, -1402, 0, 96, "wood")              # the guard's desk
    W.box(4515, 4605, -1705, -1395, 96, 100, "parquet")
    for y in (-1690, -1592, -1494):
        W.box(4598, 4600, y + 6, y + 86, 16, 84, "parquet")
    W.box(4545, 4580, -1580, -1530, 100, 101.5, "linen")
    W.box(4562, 4564, -1580, -1530, 101.5, 102, "black")
    W.cyl(4570, -1500, 100, 6, 3, "pewter")
    W.box(4569, 4571, -1501, -1499, 103, 115, "black", r=(10, 0, 0))
    desk_lamp(W, 4540, -1650, 100)
    phone(W, 4548, -1440, 100)
    W.cyl(4590, -1560, 100, 8, 2, "pewter")
    W.sph(4590, -1560, 103, 6, 6, 4, "pewter")
    W.box(5235, 5290, -1640, -1440, 40, 48, "velvet")           # the depositors' bench
    W.box(5240, 5290, -1635, -1445, 32, 40, "wood", collide=True)
    W.box(5285, 5292, -1640, -1440, 48, 95, "wood")
    for y in (-1630, -1452):
        for x in (5242, 5282):
            W.box(x, x + 5, y, y + 5, 0, 32, "wood")
    prop("crate", 5185, -1300, 0, 92, 90, 78, face=96, mode="stretch")
    W.box(5165, 5205, -1315, -1285, 78, 100, "steel")
    W.box(5180, 5190, -1316, -1284, 100, 103, "pewter")
    W.box(5070, 5120, -2215, -2185, 80, 92, "steel")            # an opened strongbox on the viewing table
    W.box(5070, 5120, -2187, -2184, 92, 122, "steel")
    W.box(5125, 5150, -2220, -2190, 80, 80.5, "linen", r=(0, 0, 10))
    W.cyl(5140, -2172, 80, 8, 1, "glass")
    W.box(5140, 5160, -2173, -2171, 80, 81.5, "black")


def vault():
    """The strongroom: walls of deposit boxes, one drawer pulled out on the viewing table, the house safe, tonight's
    top lots on steel shelves in their cases."""
    hide([r"Gold_Bar_\d+", r"Safe_Handle_\d+", r"Statue_\d+", r"Statue_Plinth_\d+", r"Vault_Lot_\d+", r"Vault_Shelf_\d+",
          r"Safe_\d+"], (4420, 5300, -990, 1000, 0, 500))
    for (y0, y1) in ((-900, -200), (200, 900)):                 # the deposit boxes' doors
        n = int((y1 - y0) // 35)
        for c in range(n):
            y = y0 + c * 35
            for r_ in range(9):
                z = 10 + r_ * 22
                if (y0, c, r_) == (-900, 12, 4):
                    W.box(4471, 4520, y + 2, y + 33, z + 2, z + 18, "steel")      # pulled out
                    W.box(4471, 4502, y + 1, y + 2, z + 2, z + 20, "pewter")
                    continue
                W.box(4470, 4471.5, y + 2, y + 33, z + 2, z + 20, "pewter")
                W.box(4471.5, 4472, y + 16, y + 18, z + 8, z + 12, "black")
    W.box(4702, 4898, 852, 990, 8, 198, "steel")                # the house safe
    W.box(4705, 4895, 855, 985, 0, 8, "black")
    W.box(4720, 4880, 848, 852, 25, 185, "grey")
    W.box(4799, 4801, 847, 848, 25, 185, "black")
    for x in (4770, 4830):
        W.rod(x, 846, 130, 18, 3, "pewter", yaw=90)
    for x in (4785, 4810):
        W.box(x, x + 5, 842, 846, 90, 120, "pewter")
    W.box(4770, 4830, 847, 848, 165, 175, "linen")
    for sy in (-700.0, -300.0, 100.0, 500.0):                   # steel shelves for the lots
        for (x, y) in ((5222, sy + 2), (5294, sy + 2), (5222, sy + 244), (5294, sy + 244)):
            W.box(x, x + 4, y, y + 4, 0, 200, "pewter")
        for z in (5, 68, 132, 196):
            W.box(5220, 5300, sy, sy + 250, z, z + 2, "steel")
    for z in (7, 70, 134):                                      # jewellery cases
        y = -690.0
        while y < -470:
            w = rng.uniform(18, 30)
            W.box(5235, 5235 + rng.uniform(25, 40), y, y + w, z, z + rng.uniform(6, 11),
                  rng.choice(("velvet", "black", "wood")))
            y += w + rng.uniform(6, 16)
    prop("bust", 5262, -235, 70, 45, 45, 58, face=180)
    W.box(5232, 5290, -150, -90, 7, 45, "wood")
    W.box(5235, 5285, -262, -200, 134, 164, "black")
    prop("vase", 5262, 165, 70, 40, 40, 58, face=180)
    W.box(5228, 5292, 120, 330, 7, 15, "linen")
    W.box(5236, 5286, 260, 330, 70, 102, "wood")
    for (x0, x1, y0, y1, z0, z1, mat) in ((5230, 5290, 520, 600, 7, 40, "steel"), (5235, 5280, 640, 700, 7, 30, "grey"),
                                          (5232, 5288, 530, 590, 70, 100, "steel"), (5240, 5285, 680, 730, 134, 160, "grey"),
                                          (5238, 5282, 620, 660, 70, 92, "linen")):
        W.box(x0, x1, y0, y1, z0, z1, mat)
    prop("pedestal", 4850, 500, 0, 50, 50, 100, face=270, mode="stretchz")
    prop("vase", 4850, 500, 100, 48, 48, 70, face=270)
    W.box(4640, 4720, -40, 30, 90, 92, "velvet")                # the viewing table
    for i in range(9):
        a = math.pi * i / 8
        W.sph(4680 + 22 * math.cos(a), -5 + 14 * math.sin(a), 93.5, 3.4, 3.4, 3.4, "linen")
    W.cyl(4730, 40, 90, 3, 4, "black")
    for yw in (20, 35):
        W.box(4610, 4630, 50, 62, 90, 92, "linen", r=(0, 0, yw))
    desk_lamp(W, 4615, -80, 90)
    W.box(4690, 4740, 50, 80, 90, 102, "steel")
    prop("crate", 5080, -520, 0, 118, 112, 100, face=84, mode="stretch")
    W.box(5025, 5140, -580, -460, 100, 104, "batten", r=(0, 0, 12))


ROOMS = (("corridor", corridor), ("kitchen", kitchen), ("propstore", propstore), ("dock", dock),
         ("cloakroom", cloakroom), ("restrooms", restrooms), ("gallery", gallery), ("stock", stock),
         ("office", office), ("security", security), ("stairs", stairs), ("director", director),
         ("projection", projection), ("archive", archive), ("vaultcor", vaultcor), ("vault", vault))


# --- steps -----------------------------------------------------------------------------------------------------------
def do_import():
    for n in (NAMES or sorted(LOOKS)):
        C.import_prop(n, look=LOOKS[n])
        LOG.append("imported " + n)


def lineup():
    """Every prop this file places, in a row at x -6200 (outside the street), turned by FRONT: each should face +X,
    where the white bar points. Two shots from the east."""
    C.clear("set_rooms_lineup")
    names = sorted(LOOKS) + list(SHARED)
    for i, n in enumerate(names):
        if not eal.does_asset_exist(mesh(n)):
            continue
        y = -2400.0 + i * 330
        loc, r, sc = C.fit(mesh(n), (-6300, -6100, y - 100, y + 100, 0, 200), front(n), "fit")
        C.spawn(mesh(n), loc, r, sc, tag="set_rooms_lineup", shadow=False)
        C.spawn(CUBE, (-6040, y, 3), 0.0, (1.0, 0.06, 0.06), tag="set_rooms_lineup", mat=M["linen"], shadow=False)
    n1 = len(LOOKS)
    mid1 = -2400.0 + (n1 - 1) * 165
    mid2 = -2400.0 + (n1 + (len(names) - 1)) * 165
    LOG.append(C.shot("lineup_rooms", (-4700, mid1, 420), (-6200, mid1, 90), fov=80, out_dir=OUT, unlit=True))
    LOG.append(C.shot("lineup_shared", (-4900, mid2, 420), (-6200, mid2, 90), fov=80, out_dir=OUT, unlit=True))


def clear_rooms(names):
    """Removes this tag's actors in the named rooms only (hides stay: hide() is idempotent)."""
    n = 0
    for a in C.actors():
        if TAG in C.tags(a) and str(a.get_folder_path()).split("/")[-1] in names:
            eas.destroy_actor(a)
            n += 1
    return n


def rooms():
    if ONLY:
        LOG.append("cleared %d" % clear_rooms(ONLY))
    else:
        C.clear(TAG)
    C.hide([r"Prop_Crate_Instances"], TAG)          # every crate of import_props' is in these rooms: re-placed here
    n = 0
    for name, fn in ROOMS:
        if ONLY and name not in ONLY:
            continue
        ROOM[0] = name
        fn()
        n += flush()
    LOG.append("rooms: %d pieces" % n)
    C.save_level()


if "import" in STEPS:
    do_import()
if "lineup" in STEPS:
    lineup()
if "rooms" in STEPS:
    rooms()
RESULT = "; ".join(LOG[-40:])
RESULT
