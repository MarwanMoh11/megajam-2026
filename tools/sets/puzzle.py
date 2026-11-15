# Package puzzle (Marwan, Nov 15: "what is this banner that drops and how can you phase through it, what are the
# people seated for and watching: redesign the puzzle elements until they make sense"). An overlay on the built level,
# after hall.py and devices.py; run_all.py runs it, or alone under the editor lock:
#     "<engine python>" -I tools/ue_mcp.py py tools/sets/puzzle.py 110
# Idempotent: clears its own tag (set_puzzle) and shows what it hid, then builds again. No collision is added or removed.
#   rope     the reserved bidders' barrier is velvet rope on brass posts, wall to wall, not a black wall (the two Rope_*
#            boxes are hidden, their collision stays); a RESERVED card stands each side of the gap
#            (the gap's beam keeps ABeamWatcher's own posts, lamps and glint: they read as the electric eye)
#   breaker  the banner is gone from the room. The same device (ABannerDevice, label Banner) is now the breaker of the
#            aisle's electric eye on Maya's platform: no matte, it lands at once, the eye's lamps go green for 10 s.
#            The same values are in build_auction.py, so a level rebuild keeps them.
#   sale     what the room is watching: a lantern slide of the lot on the screen in house light (LOT 41, the bid);
#            the preview's film panel covers it while the house is down
# Switches: PUZZLE_PARTS (a tuple of the names above) runs only those; PUZZLE_OFF = True only clears.
import sys, importlib, math, traceback
sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C
importlib.reload(C)
import unreal

TAG = "set_puzzle"
PARTS = globals().get("PUZZLE_PARTS") or ("rope", "breaker", "sale")
OFF = bool(globals().get("PUZZLE_OFF"))
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
HDIR = "/Game/Take/Sets/Hall"
CUBE, CYL, SPH = "/Engine/BasicShapes/Cube", "/Engine/BasicShapes/Cylinder", "/Engine/BasicShapes/Sphere"
STANCHION = "/Game/Take/Props/Stanchion/SM_Stanchion"
REPORT = []
N = [0]

# From build_auction.py (the hall) and hall_zones.json (the screen).
ROPE_X, GAP, HY = 2400.0, 150.0, 2000.0
SCREEN_FACE, SCREEN_HW, SCREEN_Z0, SCREEN_Z1 = 4390.0, 500.0, 235.0, 965.0
BID = "$410,000"


def mi(name, vectors=None, scalars=None):
    """An instance of hall.py's flat master (made by hall.py; this only adds instances)."""
    path = HDIR + "/" + name
    if eal.does_asset_exist(path):
        m = eal.load_asset(path)
    else:
        at = unreal.AssetToolsHelpers.get_asset_tools()
        m = at.create_asset(name, HDIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(m, eal.load_asset(HDIR + "/M_HallFlat"))
    for k, v in (vectors or {}).items():
        mel.set_material_instance_vector_parameter_value(m, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
    for k, v in (scalars or {}).items():
        mel.set_material_instance_scalar_parameter_value(m, k, float(v))
    mel.update_material_instance(m)
    eal.save_asset(path, only_if_is_dirty=False)
    return m


M = {}


def materials():
    for k in ("Brass", "Burgundy", "Walnut", "WalnutDark", "Black", "Cream", "Glass"):
        M[k] = eal.load_asset(HDIR + "/MI_Hall_" + k)
    # The lantern slide: a warm card that glows a little, like a slide thrown from the booth.
    M["Slide"] = mi("MI_Puzzle_Slide", {"Color": (0.10, 0.085, 0.06), "Emissive": (1.0, 0.80, 0.52)},
                    {"Roughness": 0.9, "EmissiveScale": SLIDE_GLOW})


SLIDE_GLOW = float(globals().get("PUZZLE_SLIDE_GLOW", 0.22))


def _lbl(what):
    N[0] += 1
    return "Puzzle_%s_%03d" % (what, N[0])


def box(what, x0, x1, y0, y1, z0, z1, mat, shadow=False):
    return C.spawn(CUBE, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), 0.0,
                   ((x1 - x0) / 100.0, (y1 - y0) / 100.0, (z1 - z0) / 100.0), tag=TAG, label=_lbl(what),
                   shadow=shadow, mat=M[mat])


def cyl(what, x, y, z0, z1, dia, mat, rot=0.0):
    return C.spawn(CYL, (x, y, (z0 + z1) / 2), rot, (dia / 100.0, dia / 100.0, (z1 - z0) / 100.0), tag=TAG,
                   label=_lbl(what), shadow=False, mat=M[mat])


def text(s, x, y, z, yaw, size, rgb):
    N[0] += 1
    a = eas.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(x, y, z), unreal.Rotator(0.0, 0.0, yaw))
    a.set_actor_label("Set_Puzzle_Text_%03d" % N[0])
    a.set_folder_path("Sets/" + TAG)
    c = a.get_component_by_class(unreal.TextRenderComponent)
    try:
        c.set_text(s)
    except Exception:                                           # noqa: BLE001
        c.set_editor_property("text", s)
    for k, v in (("world_size", float(size)),
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


# --- rope ----------------------------------------------------------------------------------------------------------------
POST_H, HOOK = 94.0, 86.0


def rope():
    hidden = C.hide([r"Rope_(South|North)(_\d+)?"], TAG)
    posts, segs, hooks = [], [], []
    n = 9
    for s in (1, -1):
        ys = [s * (GAP + 40.0 + (HY - 40.0 - GAP - 40.0) * i / n) for i in range(n + 1)]   # 190 .. 1960
        posts += [(ROPE_X, y) for y in ys]
        for a, b in zip(ys, ys[1:]):
            y0, y1 = a + s * 4.0, b - s * 4.0
            ln = abs(y1 - y0)
            sag, k = min(26.0, 0.06 * ln + 4.0), 12
            pts = [(ROPE_X, y0 + (y1 - y0) * i / k, HOOK - 4.0 * sag * (i / k) * (1 - i / k)) for i in range(k + 1)]
            for p, q in zip(pts, pts[1:]):
                d = [q[i] - p[i] for i in range(3)]
                seg = math.sqrt(sum(v * v for v in d))
                yaw = math.degrees(math.atan2(d[1], d[0]))
                pitch = math.degrees(math.atan2(d[2], math.hypot(d[0], d[1])))
                segs.append((tuple((p[i] + q[i]) / 2 for i in range(3)), (0.0, pitch - 90.0, yaw),
                             (0.042, 0.042, (seg + 2.0) / 100.0)))
            hooks += [(pts[0], 0.0, 0.05), (pts[-1], 0.0, 0.05)]
    try:
        if not eal.does_asset_exist(STANCHION):
            raise RuntimeError("absent")
        C.scatter(STANCHION, [C.fit(STANCHION, (x - 14.0, x + 14.0, y - 14.0, y + 14.0, 0.0, POST_H), 0.0, "stretchz")
                              for (x, y) in posts], tag=TAG, label="Puzzle_Rope_Posts", mat=M["Brass"])
    except Exception as e:                                      # noqa: BLE001 (no Rodin post: a plain brass one)
        REPORT.append("rope: stanchion mesh (%s): plain posts" % str(e)[:80])
        C.scatter(CYL, [((x, y, POST_H / 2), 0.0, (0.05, 0.05, POST_H / 100.0)) for (x, y) in posts], tag=TAG,
                  label="Puzzle_Rope_Posts", mat=M["Brass"])
        C.scatter(CYL, [((x, y, 1.5), 0.0, (0.28, 0.28, 0.03)) for (x, y) in posts], tag=TAG,
                  label="Puzzle_Rope_Feet", mat=M["Brass"])
    C.scatter(CYL, segs, tag=TAG, label="Puzzle_Rope_Velvet", mat=M["Burgundy"])
    C.scatter(SPH, hooks, tag=TAG, label="Puzzle_Rope_Hooks", mat=M["Brass"])
    # A card on a brass stand each side of the gap, read from the general seats (the west).
    for s in (1, -1):
        x, y = ROPE_X - 34.0, s * (GAP + 95.0)
        cyl("Card_Foot", x, y, 0.0, 2.5, 30.0, "Brass")
        cyl("Card_Stem", x, y, 2.5, 118.0, 3.0, "Brass")
        box("Card_Frame", x - 1.5, x + 1.5, y - 27.0, y + 27.0, 116.0, 152.0, "Brass")
        box("Card", x - 2.2, x - 1.5, y - 24.5, y + 24.5, 118.5, 149.5, "Cream")
        text("RESERVED", x - 2.6, y, 139.0, 180.0, 8.0, (0.06, 0.035, 0.02))
        text("PADDLE HOLDERS ONLY", x - 2.6, y, 127.0, 180.0, 3.6, (0.06, 0.035, 0.02))
    REPORT.append("rope: %d wall boxes hidden, %d posts, %d velvet links" % (len(hidden), len(posts), len(segs)))


# --- breaker -------------------------------------------------------------------------------------------------------------
BREAKER = dict(device_name="BREAKER", use_verb="Pull the breaker on the aisle's electric eye",
               fire_line='MAYA: "Breaker\'s out. The eye is dead for ten seconds."',
               matte_size=unreal.Vector(1.0, 1.0, 1.0), drop_distance=0.0, drop_seconds=0.02, raise_seconds=0.02,
               model_hides_body=True, show_rope=False)


def breaker():
    done = []
    for a in C.find(r"Banner(_\d+)?"):
        for k, v in BREAKER.items():
            try:
                a.set_editor_property(k, v)
                done.append(k)
            except Exception as e:                              # noqa: BLE001
                REPORT.append("breaker: %s: %s" % (k, str(e)[:120]))
    for a in C.find(r"Beam(_\d+)?"):
        try:
            a.set_editor_property("label", "ELECTRIC EYE")
            done.append("eye label")
        except Exception as e:                                  # noqa: BLE001
            REPORT.append("breaker: beam label: %s" % str(e)[:120])
    REPORT.append("breaker: set %s" % ", ".join(done))


# --- sale ----------------------------------------------------------------------------------------------------------------
def sale():
    """The lantern slide on the screen. The preview's film panel is 2 cm in front of the screen's face: this sits
    behind it, so the film covers the slide while the house is down."""
    card_x, ink_x = SCREEN_FACE - 0.6, SCREEN_FACE - 1.0
    ink, gold = (0.07, 0.04, 0.025), (0.24, 0.07, 0.02)
    box("Slide", card_x, SCREEN_FACE - 0.1, -SCREEN_HW, SCREEN_HW, SCREEN_Z0, SCREEN_Z1, "Slide")
    for z0, z1 in ((SCREEN_Z0 + 40.0, SCREEN_Z0 + 44.0), (SCREEN_Z1 - 44.0, SCREEN_Z1 - 40.0)):
        box("Slide_Rule", ink_x, card_x, -SCREEN_HW + 40.0, SCREEN_HW - 40.0, z0, z1, "WalnutDark")
    for y in (-SCREEN_HW + 40.0, SCREEN_HW - 44.0):
        box("Slide_Rule", ink_x, card_x, y, y + 4.0, SCREEN_Z0 + 40.0, SCREEN_Z1 - 40.0, "WalnutDark")
    text("NOW SELLING", ink_x, 0.0, 880.0, 180.0, 34.0, ink)
    text("LOT 41", ink_x, 0.0, 760.0, 180.0, 150.0, ink)
    text("THE DIRECTOR'S CUT", ink_x, 0.0, 625.0, 180.0, 74.0, ink)
    text("the only print in existence", ink_x, 0.0, 545.0, 180.0, 32.0, ink)
    box("Slide_Rule", ink_x, card_x, -230.0, 230.0, 478.0, 481.0, "WalnutDark")
    text("CURRENT BID", ink_x, 0.0, 425.0, 180.0, 32.0, ink)
    text(BID, ink_x, 0.0, 335.0, 180.0, 92.0, gold)
    REPORT.append("sale: lantern slide on the screen (LOT 41, %s), glow %.2f" % (BID, SLIDE_GLOW))


# --- run -----------------------------------------------------------------------------------------------------------------
gone, shown = C.clear(TAG)
REPORT.append("cleared %d, shown %d" % (gone, shown))
if not OFF:
    materials()
    for part in PARTS:
        try:
            globals()[part]()
        except Exception as e:                                  # noqa: BLE001 (one broken part must not stop the rest)
            REPORT.append("FAIL %s: %s" % (part, traceback.format_exc().strip().splitlines()[-1][:300]))
            unreal.log_error("[puzzle] %s\n%s" % (part, traceback.format_exc()))
C.save_level()
"\n".join(REPORT)
