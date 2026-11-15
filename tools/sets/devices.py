# Package devices (QA Nov 14, notes 10, 17, 18): every thing Maya and Vex use is a period object, not a box.
# Overlay on the built map (see tools/sets/README.md). Idempotent: clears tag set_devices first. Run under the lock:
#     "$PY" -I tools/ue_mcp.py py tools/sets/devices.py 110
# A wrapper that puts  DEVICES_STEP = "<step>"  before this file's text runs one step (an MCP call gives up at ~110 s):
#   import   GLB -> /Game/Take/Props/Devices_<x>/SM_Devices_<x> (C:\Jam\Art\Rodin\devices_<x>), keeping the mesh's own
#            PBR material (no M_Prop: the brief keeps the texture). DEVICES_NAMES limits it.
#   lineup   every device mesh in a row at LINEUP, unturned, and a shot from +X (to read which way each faces)
#   place    (default) the overlay below
# Two modes, picked by whether the device C++ has the model slot (ADevice::ModelMesh, QA Nov 14):
#   slot     sets ModelMesh/ModelOffset/ModelRotation/ModelScale, the pilot lamp, the lever and the rope on each device
#            (the C++ hides the box and the floor ring, keeps the mesh's material and moves the parts on the press)
#   stand-in (before that build) stands each model over its device as a StaticMeshActor (NoCollision) and hides the
#            box body and the floor ring (component visibility only: clear() shows them again)
# Both modes: the brass ticket gate over the scanner (its posts and lintel hidden), the searchlight's stand hidden,
# fresnel lanterns in place of the catwalk's lamp cans, the catwalk and platform edge tape hidden (note 18).
# Nothing placed here collides: no sight line, camera probe or pawn ever meets it.
import importlib
import math
import os
import sys

import unreal

sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C  # noqa: E402

importlib.reload(C)

STEP = globals().get("DEVICES_STEP") or "place"
NAMES = globals().get("DEVICES_NAMES")
TAG = "set_devices"
LINEUP_TAG = "set_devices_lineup"
OUT = r"C:\Jam\Builds\qa\devices"
LINEUP = (-6400.0, -1200.0, 0.0)
eal = unreal.EditorAssetLibrary
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
MESHES = ["devices_searchlight", "devices_followspot", "devices_winch", "devices_pyro", "devices_gate",
          "devices_dimmer", "devices_rail", "devices_fresnel"]
F2 = 450.0

# The yaw that turns each imported mesh to face +X (its front: the lens, the panel, the gate's walkway, the rail's
# lever side). From the lineup shot.
FRONT = {n: 0.0 for n in MESHES}
FRONT.update({"cart": 0.0, "desk": 0.0, "devices_pyro": 270.0, "devices_gate": 90.0, "devices_followspot": 270.0,
              "rooms_projector": 270.0})

# --- W3 spot: positions come from the placed actors and stage's hall_zones.json, never hard-coded hall numbers ---------
ZONES_FILE = r"C:\Jam\megajam\tools\sets\hall_zones.json"


def zones():
    try:
        import json
        with open(ZONES_FILE, encoding="utf-8") as f:
            return json.load(f)
    except Exception:                                               # noqa: BLE001
        return {}


ZONES = zones()
CASE_XY = tuple(ZONES.get("case", (4000.0, 0.0, 0.0))[:2])        # the follow spot rests facing the lot

BRASS = (0.42, 0.27, 0.08)
AMBER = (1.0, 0.52, 0.16)


def mpath(n):
    return C.ip()["mesh_path"](n)


def mesh(n):
    return eal.load_asset(mpath(n))


def mbox(m):
    b = m.get_bounding_box()
    return (b.min.x, b.max.x, b.min.y, b.max.y, b.min.z, b.max.z)


def turn(v, yaw):
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    return (v[0] * c - v[1] * s, v[0] * s + v[1] * c, v[2])


def add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def vec(v):
    return unreal.Vector(float(v[0]), float(v[1]), float(v[2]))


def lin(c):
    return unreal.LinearColor(float(c[0]), float(c[1]), float(c[2]), 1.0)


def yaw_to(a, b):
    return math.degrees(math.atan2(b[1] - a[1], b[0] - a[0]))


def has_slot(dev):
    try:
        dev.get_editor_property("model_mesh")
        return True
    except Exception:                                               # noqa: BLE001
        return False


def device(label):
    for a in C.actors():
        if a.get_actor_label() == label and isinstance(a, unreal.Device):
            return a
    return None


# --- the devices --------------------------------------------------------------------------------------------------------
# World cm. mesh: a MESHES name, or an existing prop ("cart", "desk"). at: the floor point under the model's middle
# (None: the actor's own place; ("actor", dx, dy) or ("use", dx, dy): offset in the actor's own frame from the actor or
# from its use point, on the use point's floor; so the model follows wherever stage moves the device). face: the world yaw the model's front turns to ("use": toward the use point). size:
# the model's fit along its front, across, up (uniform scale: the smallest ratio). Parts, in the model's own frame at
# rest as fractions of its fitted size (forward, across, up), or cm where noted:
#   pilot (+ pilot_size cm, pilot_colour), lens (follow spot: where the beam starts, and lens_face: the lens's yaw
#   relative to face), lever: (pivot, rest (pitch, yaw rel. to face, roll), throw (pitch, yaw, roll), length, spin),
#   rope: (from, to_z world).
SPECS = [
    dict(label="Searchlight", mesh="devices_searchlight", at=("actor", 67.0, 30.0), face=150.0, size=(230, 160, 200),
         lens=(0.5, 0.0, 0.75), pilot=(-0.22, 0.0, 0.42), pilot_size=4.0, hide_body=True,
         shot=((-3990.0, 120.0, 260.0), (-3390.0, 470.0, 540.0))),
    # The follow spot: its operator's handle sticks out the back toward Maya (SPOT_USE 650,-290), beside her.
    dict(label="Spot", mesh="devices_followspot", at=("actor", -90.0, -92.0), face=("toward", CASE_XY),
         size=(240, 110, 170), lens=(0.9, 0.0, 0.8), pilot=(-0.3, 0.35, 0.45), pilot_size=3.5, hide_body=True,
         shot=((470.0, -560.0, 640.0), (680.0, -340.0, 540.0))),
    dict(label="Banner", mesh="devices_winch", at=("use", 68.0, -2.0), face=0.0, size=(55, 85, 62),
         pilot=(0.0, -0.45, 0.95), pilot_size=3.5, hide_body=False,
         lever=((0.0, 0.5, 0.55), (0.0, 90.0, 0.0), (0.0, 0.0, 0.0), 22.0, 540.0),
         rope=((0.15, 0.0, 0.72), F2 + 420.0),
         shot=((470.0, -380.0, 660.0), (720.0, -512.0, 480.0))),
    dict(label="Pyro", mesh="devices_pyro", at=("actor", -11.0, -7.0), face="use", size=(50, 100, 85),
         pilot=(0.52, 0.30, 0.86), pilot_size=4.0, pilot_colour=(1.0, 0.10, 0.04), hide_body=True,
         lever=((0.05, -0.40, 0.98), (20.0, 0.0, 0.0), (-70.0, 0.0, 0.0), 26.0, 0.0),
         shot=((470.0, -930.0, 650.0), (725.0, -750.0, 500.0))),
    dict(label="Cart", mesh="cart", at=None, face="actor", size=(100, 60, 90), slot_only=True, pilot=None,
         shot=((2780.0, -1700.0, 230.0), (3000.0, -1900.0, 60.0))),
    dict(label="Cart_Dais", mesh="cart", at=None, face="actor", size=(100, 60, 90), slot_only=True, pilot=None,
         shot=((3930.0, -1700.0, 230.0), (4150.0, -1900.0, 60.0))),
    dict(label="Sound_Desk", mesh="desk", at=None, face="actor", size=(60, 140, 100), slot_only=True,
         pilot=(0.05, -0.25, 0.93), pilot_size=6.0, pilot_colour=(1.0, 0.62, 0.22),
         lever=((0.30, 0.28, 0.80), (25.0, 0.0, 0.0), (-50.0, 0.0, 0.0), 16.0, 0.0),
         shot=((3330.0, -2180.0, 240.0), (3550.0, -2460.0, 90.0))),
    dict(label="Winch", mesh="devices_rail", at=("actor", -48.0, 0.0), face="use", size=(45, 85, 300),
         pilot=(0.55, -0.35, 0.40), pilot_size=4.0, hide_body=True,
         lever=((0.55, 0.30, 0.36), (-15.0, 0.0, 0.0), (75.0, 0.0, 0.0), 40.0, 0.0),
         rope=((0.0, 0.0, 1.0), 495.0),
         shot=((4080.0, -2230.0, 260.0), (4330.0, -2530.0, 160.0))),
    # The house dimmer (package heist's AHouseDimmer, an ADevice: the slot works): on the platform's west rail, facing
    # its use point. Its projector is dressed by projector() below.
    dict(label="HouseDimmer", mesh="devices_dimmer", at=None, face="use", size=(40, 120, 180), hide_body=True,
         pilot=(0.52, 0.0, 0.86), pilot_size=4.0, optional=True,
         lever=((0.0, 0.56, 0.55), (0.0, 0.0, 0.0), (-80.0, 0.0, 0.0), 34.0, 0.0),
         shot=((560.0, -420.0, 640.0), (470.0, -640.0, 540.0))),
]


def fitted(m, size):
    """Uniform scale that fits the mesh (front +X after FRONT) inside size = (along, across, up)."""
    x0, x1, y0, y1, z0, z1 = mbox(m)
    return min(size[0] / max(x1 - x0, 1e-3), size[1] / max(y1 - y0, 1e-3), size[2] / max(z1 - z0, 1e-3))


def frames(spec, dev, m):
    """(anchor world, model world yaw, scale, actor loc, actor yaw, use world, fitted dims (along, across, up))."""
    loc = dev.get_actor_location()
    aloc = (loc.x, loc.y, loc.z)
    ayaw = dev.get_actor_rotation().yaw
    uo = dev.get_editor_property("use_offset")
    use = add(aloc, turn((uo.x, uo.y, uo.z), ayaw))
    at = spec["at"] or aloc
    if isinstance(at[0], str):                                      # W3 spot: relative to the actor or its use point
        base = aloc if at[0] == "actor" else use
        off = turn((at[1], at[2], 0.0), ayaw)
        at = (base[0] + off[0], base[1] + off[1], use[2])
    f = spec["face"]
    if f == "use":
        yaw = yaw_to(at, use)
    elif f == "actor":
        yaw = ayaw
    elif isinstance(f, tuple):
        yaw = yaw_to(at, f[1])
    else:
        yaw = float(f)
    name = spec["mesh"]
    front = FRONT.get(name, 0.0)
    # The mesh's dims with its front on +X (FRONT turns it), then the uniform fit.
    x0, x1, y0, y1, z0, z1 = mbox(m)
    dx, dy = (x1 - x0, y1 - y0) if int(round(front / 90.0)) % 2 == 0 else (y1 - y0, x1 - x0)
    s = min(spec["size"][0] / max(dx, 1e-3), spec["size"][1] / max(dy, 1e-3), spec["size"][2] / max(z1 - z0, 1e-3))
    return at, yaw, s, aloc, ayaw, use, (dx * s, dy * s, (z1 - z0) * s), front


def part(at, yaw, dims, frac):
    """A point on the model: fractions of its fitted size, forward/across/up from its anchor, turned with it."""
    return add(at, turn((frac[0] * dims[0] * 0.5, frac[1] * dims[1] * 0.5, frac[2] * dims[2]), yaw))


def to_local(p, aloc, ayaw):
    return turn(sub(p, aloc), -ayaw)


def stand_in(spec, dev, m):
    """Before the C++ slot: the model as a StaticMeshActor over the device, the box and the ring hidden."""
    at, yaw, s, aloc, ayaw, use, dims, front = frames(spec, dev, m)
    x0, x1, y0, y1, z0, z1 = mbox(m)
    anchor = ((x0 + x1) * 0.5, (y0 + y1) * 0.5, z0)
    total = yaw + front
    loc = sub(at, turn((anchor[0] * s, anchor[1] * s, anchor[2] * s), total))
    C.spawn(m, loc, total, s, tag=TAG, label="Set_Device_" + spec["label"], shadow=True)
    parts = ["UseRing"] + (["Body"] if spec.get("hide_body") else [])
    hide_parts(dev, parts)
    return "%s: stand-in at (%.0f, %.0f, %.0f) yaw %.0f scale %.2f" % (spec["label"], at[0], at[1], at[2], yaw, s)


def hide_parts(a, names):
    comps = [c for c in a.get_components_by_class(unreal.PrimitiveComponent) if c.get_name() in names and c.is_visible()]
    for c in comps:
        c.set_visibility(False, False)
    if comps:
        ts = C.tags(a)
        had = [t.split("=", 1)[1].split("|") for t in ts if t.startswith(TAG + ":hid=")]
        names = sorted(set(sum(had, [])) | {c.get_name() for c in comps})
        C._set_tags(a, [t for t in ts if not t.startswith(TAG + ":hid=")] + ["%s:hid=%s" % (TAG, "|".join(names))])


def slot(spec, dev, m):
    """With the C++ slot: the model, its pilot lamp, its lever and its rope as the device's own properties."""
    at, yaw, s, aloc, ayaw, use, dims, front = frames(spec, dev, m)
    lyaw = yaw - ayaw
    p = dict(model_mesh=m, use_model=True, model_offset=vec(to_local(at, aloc, ayaw)),
             model_rotation=unreal.Rotator(0.0, 0.0, lyaw + front), model_scale=vec((s, s, s)),
             show_use_ring=False, model_hides_body=bool(spec.get("hide_body", True)))
    if spec.get("pilot"):
        p.update(show_pilot=True, pilot_offset=vec(to_local(part(at, yaw, dims, spec["pilot"]), aloc, ayaw)),
                 pilot_size=float(spec.get("pilot_size", 4.0)), pilot_colour=lin(spec.get("pilot_colour", AMBER)))
    elif spec.get("pilot") is None and "pilot" in spec:
        p.update(show_pilot=True, pilot_size=0.0, pilot_offset=vec(to_local(add(at, (0, 0, 120.0)), aloc, ayaw)))
    lv = spec.get("lever")
    if lv:
        pivot, rest, throw, length, spin = lv
        p.update(show_handle=True, handle_offset=vec(to_local(part(at, yaw, dims, pivot), aloc, ayaw)),
                 handle_rest=unreal.Rotator(float(rest[2]), float(rest[0]), float(lyaw + rest[1])),
                 handle_throw=unreal.Rotator(float(throw[2]), float(throw[0]), float(throw[1])),
                 handle_length=float(length), handle_spin_degrees=float(spin),
                 handle_colour=lin(BRASS))
    else:
        p.update(show_handle=False)
    rp = spec.get("rope")
    if rp:
        a0 = part(at, yaw, dims, rp[0])
        p.update(show_rope=True, rope_from=vec(to_local(a0, aloc, ayaw)),
                 rope_to=vec(to_local((a0[0], a0[1], rp[1]), aloc, ayaw)))
    else:
        p.update(show_rope=False)
    if spec.get("lens"):
        lens = part(at, yaw, dims, spec["lens"])
        p.update(show_beam_shape=False, swing_model=True, lens_yaw=float(lyaw),
                 lens_offset=vec(turn(sub(lens, at), -ayaw)),
                 lens_glow_size=float(spec.get("lens_glow", 0.35 * min(dims[1], dims[2]))))  # W3 spot
    for k, v in p.items():
        try:
            dev.set_editor_property(k, v)
        except Exception as e:                                       # noqa: BLE001
            unreal.log_warning("devices: %s.%s: %s" % (spec["label"], k, e))
    return "%s: slot at (%.0f, %.0f, %.0f) yaw %.0f scale %.2f" % (spec["label"], at[0], at[1], at[2], yaw, s)


def prop_mesh(n):
    if n in ("cart", "desk"):
        return eal.load_asset(C.ip()["mesh_path"](n))
    return mesh(n)


# --- steps ------------------------------------------------------------------------------------------------------------
def do_import():
    P = C.ip()
    out = []
    for n in (NAMES or MESHES):
        if eal.does_asset_exist(mpath(n)) and not globals().get("DEVICES_REIMPORT"):
            out.append("%s: there" % n)
            continue
        P["import_one"](n)
        out.append("%s: %s" % (n, "ok" if eal.does_asset_exist(mpath(n)) else "FAILED " + "; ".join(P["REPORT"][-2:])))
    return " | ".join(out)


def do_lineup():
    C.clear(LINEUP_TAG)
    x, y, z = LINEUP
    for n in MESHES:
        m = mesh(n)
        if m is None:
            continue
        s = 160.0 / max(max(mbox(m)[1] - mbox(m)[0], mbox(m)[3] - mbox(m)[2], mbox(m)[5] - mbox(m)[4]), 1e-3)
        loc, r, sc = C.fit(m, (x - 80, x + 80, y - 80, y + 80, z, z + 160), 0.0, "fit")
        C.spawn(m, loc, r, sc, tag=LINEUP_TAG, label="Set_Lineup_" + n)
        y += 260.0
    a = C.shot("devices_lineup_from_x", (x + 700.0, LINEUP[1] + 900.0, 260.0), (x, LINEUP[1] + 900.0, 80.0), fov=80, unlit=True,
               out_dir=OUT)
    b = C.shot("devices_lineup_from_above", (x + 300.0, LINEUP[1] + 900.0, 900.0), (x, LINEUP[1] + 900.0, 0.0), unlit=True,
               fov=80, out_dir=OUT)
    C.clear(LINEUP_TAG)
    return a + " " + b


def do_place():
    C.clear(TAG)
    out = []
    for spec in SPECS:
        dev = device(spec["label"])
        if dev is None:
            if not spec.get("optional"):
                out.append("%s: NO DEVICE" % spec["label"])
            continue
        m = prop_mesh(spec["mesh"])
        if m is None:
            out.append("%s: no mesh %s" % (spec["label"], spec["mesh"]))
            continue
        if has_slot(dev):
            out.append(slot(spec, dev, m))
        elif not spec.get("slot_only"):
            out.append(stand_in(spec, dev, m))
        else:
            hide_parts(dev, ["UseRing"])
            out.append("%s: ring hidden (model waits for the slot)" % spec["label"])
    # Rings on any other device (the stand-in mode only: the slot's C++ empties them itself).
    for a in C.actors():
        if isinstance(a, unreal.Device) and not has_slot(a):
            hide_parts(a, ["UseRing"])
    out.append(gate())
    out.append(catwalk())
    out.append(projector())
    C.save_level()
    return "\n".join(out)


PROJECTOR_MESH = "rooms_projector"      # W3 spot: rooms' 1940s 35 mm projector on its pedestal (was the follow spot's barrel)
PROJECTOR_HEIGHT = (120.0, 230.0)       # the fitted model's height is kept between these, cm


def projector():
    """The house dimmer's projector (package heist's ProjectorBody box, hidden): a model on the platform floor, its
    front at ProjectorOffset (the lens), turned to the screen. HouseDimmer.cpp places the box once at BeginPlay and
    never moves it, so a standing model holds."""
    dims = [a for a in C.actors() if a.get_actor_label() == "HouseDimmer"]
    if not dims:
        return "projector: no HouseDimmer yet"
    d = dims[0]
    loc, ayaw = d.get_actor_location(), d.get_actor_rotation().yaw
    aloc = (loc.x, loc.y, loc.z)
    po, so = d.get_editor_property("projector_offset"), d.get_editor_property("screen_offset")
    lens = add(aloc, turn((po.x, po.y, po.z), ayaw))
    screen = add(aloc, turn((so.x, so.y, so.z), ayaw))
    yaw = yaw_to(lens, screen)
    m = mesh(PROJECTOR_MESH)
    if m is None:
        return "projector: no mesh " + PROJECTOR_MESH
    front = FRONT.get(PROJECTOR_MESH, 0.0)
    x0, x1, y0, y1, z0, z1 = mbox(m)
    along = (x1 - x0) if int(round(front / 90.0)) % 2 == 0 else (y1 - y0)
    # The floor under the lens: the dimmer's own floor (stage's catwalk_z when the dimmer stands on the catwalk).
    floor = aloc[2]
    cz = ZONES.get("catwalk_z")
    if cz is not None and abs(float(cz) - aloc[2]) < 60.0:
        floor = float(cz)
    tall = min(max((lens[2] - floor) / 0.85, PROJECTOR_HEIGHT[0]), PROJECTOR_HEIGHT[1])   # the lens at ~85 % up
    s = tall / max(z1 - z0, 1e-3)
    anchor = ((x0 + x1) * 0.5, (y0 + y1) * 0.5, z0)
    at = add((lens[0], lens[1], floor), turn((-0.4 * along * s, 0.0, 0.0), yaw))
    total = yaw + front
    C.spawn(m, sub(at, turn((anchor[0] * s, anchor[1] * s, anchor[2] * s), total)), total, s, tag=TAG,
            label="Set_Device_Projector")
    hide_parts(d, ["ProjectorBody"])
    return "projector: at (%.0f, %.0f) yaw %.0f scale %.2f" % (at[0], at[1], yaw, s)


def gate():
    """The scanner's brass ticket gate: its posts and lintel hidden (their collision stays), the model over them."""
    sc = [a for a in C.actors() if a.get_actor_label() == "Scanner"]
    if not sc:
        return "gate: no Scanner"
    p = sc[0].get_actor_location()
    hidden = C.hide([r"Scanner_Post(_\d+)?", r"Scanner_Lintel(_\d+)?", r"Searchlight_Stand(_\d+)?"], TAG)
    m = mesh("devices_gate")
    loc, r, s = C.fit(m, (p.x - 20.0, p.x + 20.0, p.y - 140.0, p.y + 140.0, p.z, p.z + 272.0),
                      FRONT["devices_gate"], "stretch")
    C.spawn(m, loc, r, s, tag=TAG, label="Set_Device_Gate")
    return "gate: hid %s" % ", ".join(hidden)


def catwalk():
    """dress_catwalk's lamp cans become fresnel lanterns on their pipes; the edge tape goes (note 18)."""
    m = mesh("devices_fresnel")
    cans = [a for a in C.find(r"(Dress_)?Lamp_Housing(_\d+)?")]
    tr = []
    if m is not None:
        x0, x1, y0, y1, z0, z1 = mbox(m)
        s = 42.0 / max(z1 - z0, 1e-3)
        anchor = ((x0 + x1) * 0.5, (y0 + y1) * 0.5, z1)          # hung by the clamp on its top
        for a in cans:
            b = C.bounds(a)
            top = ((b[0] + b[1]) * 0.5, (b[2] + b[3]) * 0.5, b[5] + 8.0)
            yaw = FRONT["devices_fresnel"]
            tr.append((sub(top, turn((anchor[0] * s, anchor[1] * s, anchor[2] * s), yaw)), (0.0, 0.0, yaw), s))
        if tr:
            C.scatter(m, tr, tag=TAG, label="Set_Device_Fresnel_Instances", shadow=False)
    hid = C.hide([r"(Dress_)?Lamp_Housing(_\d+)?"], TAG) if tr else []
    tape = []
    for a in C.find(r"(Dress_)?Tape_Edge(_\d+)?"):
        b = C.bounds(a)
        if 400.0 <= b[4] <= 470.0 and -620.0 <= b[0] and b[1] <= 780.0 and -850.0 <= b[2] and b[3] <= -180.0:
            tape.append(a.get_actor_label())
    if tape:
        C.hide([t.replace("(", r"\(") for t in tape], TAG)
    return "catwalk: %d fresnels, %d cans hidden, %d tape strips hidden" % (len(tr), len(hid), len(tape))


def do_shots():
    out = []
    for spec in SPECS:
        if not spec.get("shot") or device(spec["label"]) is None:
            continue
        pos, look = spec["shot"]
        out.append(C.shot("after_%s" % spec["label"].lower(), pos, look, fov=60, out_dir=OUT))
    sc = [a for a in C.actors() if a.get_actor_label() == "Scanner"]
    if sc:
        p = sc[0].get_actor_location()
        out.append(C.shot("after_gate", (p.x - 520.0, p.y - 260.0, 190.0), (p.x, p.y, 140.0), fov=60, out_dir=OUT))
    out.append(C.shot("after_catwalk", (-300.0, -720.0, 640.0), (200.0, -340.0, 520.0), fov=65, out_dir=OUT))
    return " ".join(os.path.basename(o) for o in out)


if STEP == "import":
    RESULT = do_import()
elif STEP == "lineup":
    RESULT = do_lineup()
elif STEP == "shots":
    RESULT = do_shots()
else:
    RESULT = do_place()
RESULT
