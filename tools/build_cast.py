# Step 5, characters and props; QA pass (Nov 14), package cast: puts the costumed cast and the devices' particle
# effects into /Game/Take/Lvl_Auction. tools/build_auction.py runs it last, after tools/dress_auction.py; it also runs
# alone on the open map and is safe to re-run (it removes what it placed before):
#     "<engine python>" -I tools/ue_mcp.py py tools/build_cast.py
#
#   1. guests   standing placeholder guests (a grey cylinder and a sphere, Set/Guests) get a CastMember in their place;
#               seated ones sit on the hall's real chairs (tools/sets/hall_seats.json, package hall) when that file
#               exists, else on the old placeholders' spots. No two neighbours share a kit and colour (a muted 1940s
#               palette); height, build, hair, hats, glasses, skin and accents vary per person (ACastMember, LookSeed).
#               Each one acts (mocap clips when they exist, else built from bones: ACastMember / UCastAnimInstance).
#   2. band     the quintet where tools/sets/hall_band.json puts it (real instrument props there: no shape instruments,
#               stools or stands; the trumpet stays in the trumpeter's hands), else on the old riser layout.
#   3. watchers each PatrolGuard (guards, doorman, Vance, bodyguard) gets a CastMember attached.
#   4. crew     Julian, Maya and Vex get a CastMember attached that dresses their own mesh and plays their acts.
#   5. fx       the Niagara systems in /Game/Take/FX and one TakeFX actor with a cue per device event; it also references
#               the mocap clips (tools/mocap_clips.json) so the cook packages them.
import json
import math
import os

import unreal

EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
EAL = unreal.EditorAssetLibrary
REPORT = []
TOOLS = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir() + "../tools/")

# --- tunables --------------------------------------------------------------------------------------------------------
ACTING = True                               # False: step 5's still cast (also: console Reel.Cast.Acting 0)
VARY = True                                 # False: step 5's grey costumes, no variety
# A muted 1940s palette (linear): jackets and gowns. Saturation modest; nothing near Julian's white.
MEN = [(0.03, 0.03, 0.035), (0.09, 0.09, 0.10), (0.04, 0.06, 0.15), (0.13, 0.08, 0.05),
       (0.04, 0.12, 0.07), (0.18, 0.03, 0.045), (0.34, 0.31, 0.24), (0.11, 0.14, 0.20)]
WOMEN = [(0.22, 0.03, 0.05), (0.04, 0.15, 0.08), (0.05, 0.07, 0.18), (0.36, 0.28, 0.16), (0.32, 0.16, 0.16),
         (0.03, 0.03, 0.035), (0.14, 0.05, 0.13), (0.12, 0.15, 0.22), (0.36, 0.33, 0.26)]
MEN_KITS = ("GUEST_TUX", "GUEST_TAILS")
WOMEN_KITS = ("GUEST_GOWN", "GUEST_GOWN_HAT")
STAFF_TINT = (0.035, 0.035, 0.04)
NEIGHBOUR_RADIUS = 260.0                    # no two guests this close share a kit and colour, cm
MEN_SCALE = (0.95, 1.06)                    # body scale ranges (Quinn is already smaller)
WOMEN_SCALE = (0.94, 1.03)
GLASS_SHARE = 0.4                           # standing guests holding a coupe (they sip from it)
SEAT_FILL = 0.6                             # share of the hall's chairs taken (frame time: each guest is a skeletal mesh)
STAFF_AT = ((1200.0, -1700.0), (-2050.0, 1100.0), (-2650.0, 1640.0))   # the bartender, receptionist, cloakroom
IDLE_RATE_SPREAD = 0.1                      # idles at rates 0.9 to 1.1
SEATED_DROP = 86.0                          # the seated fallback's drop when the seat cannot be fitted
SEATED_TIME = 0.0
# --- W3 cast2 ---
SEAT_HIP_FORWARD = -7.5                     # hall chairs: the hip joints this far behind the chair's centre, cm (the
                                            # gilt chair's back is 20.5 cm behind it: the back then rests on it)
PIANO_SIT_FORWARD = 8.0                     # the pianist sits this far toward the keys from the bench's centre, cm
PIANO_KEYS_IN = 8.0                         # the hands this far in from the keyboard's front edge, cm
PIANO_BUILT_REACH = 30.0                    # where the built playing puts the hands (CastMemberAnim.cpp), cm forward
BAND_CLIPS = True                           # False: the band plays built from bones (hands on the real props), not the
                                            # Mixamo instrument clips (their hands sit where the take's instrument was)
BAND_TINT = (0.07, 0.02, 0.025)             # the band in wine-dark jackets
SINGER_TINT = (0.22, 0.025, 0.04)
GUEST_DRAW = 3500.0                         # guests (and their pieces) are not drawn beyond this, cm
BAND_DRAW = 6000.0
# Guests and the band cast no shadow: a body that moves would redraw every shadow-casting key's cached shadow each
# frame (it cost 1.4-3.3 ms of GPU time with shadows on). The watchers and the crew keep theirs.
RISER_Z = 30.0
BAND = {                                    # the old riser layout (no hall_band.json): x, y, z, yaw, seated
    "BAND_SINGER": (3676.0, 1365.0, RISER_Z, -140.0, False),
    "BAND_TRUMPET": (3671.0, 1502.0, RISER_Z, -130.0, False),
    "BAND_BASS": (3693.0, 1571.0, RISER_Z, -150.0, False),
    "BAND_DRUMS": (3811.0, 1561.0, RISER_Z + 50.0, -140.0, True),
    "BAND_PIANO": (3830.0, 1420.0, RISER_Z + 50.0, 0.0, True),
}
BAND_LIGHT_UP, BAND_LIGHT_CD = 590.0, 300.0         # the key: this far over the band's centre, candelas
BAND_RIM = (170.0, 100.0, 430.0, -140.0, -32.0, 250.0)  # dx, dy from the band's centre, z, yaw, pitch, candelas
BODYGUARD_SCALE = 1.1
FX_DIR = "/Game/Take/FX"
FX = {
    "NS_Take_Pyro": "/Niagara/DefaultAssets/Templates/Systems/SimpleExplosion",
    "NS_Take_Sparks": "/Niagara/DefaultAssets/Templates/Systems/DirectionalBurst",
    "NS_Take_Glass": "/Niagara/DefaultAssets/Templates/Systems/RadialBurst",
}
DAIS_H = 20.0
PYRO_CHARGES = [(3670.0, y, DAIS_H) for y in (-900.0, -450.0, 450.0, 900.0)]
PA_TOPS = [(4130.0, 1250.0, 240.0), (4225.0, 1250.0, 240.0)]
FX_SCALE = {"pyro": 0.35, "glass": 0.5, "sparks": 0.5}


def load_json(name):
    path = TOOLS + name
    if not os.path.exists(path):
        return None
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def h01(*parts):
    """A stable number in [0, 1) from the parts (the same in every run)."""
    x = 2166136261
    for c in "|".join(str(p) for p in parts):
        x = ((x ^ ord(c)) * 16777619) & 0xFFFFFFFF
    x ^= x >> 13
    x = (x * 0x5BD1E995) & 0xFFFFFFFF
    x ^= x >> 15
    return (x & 0xFFFFFF) / float(0x1000000)


def kit(name):
    return getattr(unreal.CastKit, name)


def label(a):
    return a.get_actor_label()


def all_actors():
    return EAS.get_all_level_actors()


def hide(a):
    """Hidden in the editor and in play; collision (if any) stays."""
    for c in a.get_components_by_class(unreal.PrimitiveComponent):
        c.set_editor_property("visible", False)


def colour(c):
    return unreal.LinearColor(c[0], c[1], c[2], 1.0)


def spawn_member(k, x, y, z, yaw, tint, seated=False, name=None, **props):
    m = EAS.spawn_actor_from_class(unreal.CastMember, unreal.Vector(x, y, z), unreal.Rotator(0.0, 0.0, yaw))
    m.set_actor_label(name or ("Cast_" + k.title().replace("_", "")))
    m.set_folder_path("Cast")
    m.set_editor_property("kit", kit(k))
    m.set_editor_property("tint", colour(tint) if isinstance(tint, tuple) else unreal.LinearColor(tint, tint, tint, 1.0))
    m.set_editor_property("seated", seated)
    m.set_editor_property("seated_drop", SEATED_DROP)
    m.set_editor_property("seated_time", SEATED_TIME)
    m.set_editor_property("idle_rate_spread", IDLE_RATE_SPREAD)
    m.set_editor_property("acting", ACTING)
    m.set_editor_property("vary", VARY)
    draw = props.pop("draw_distance", 0.0)
    for key, val in props.items():
        m.set_editor_property(key, val)
    if draw:
        m.get_editor_property("body").set_editor_property("ld_max_draw_distance", draw)
    return m


# --- 0. remove what an earlier run placed ------------------------------------------------------------------------------
for a in all_actors():
    if isinstance(a, (unreal.CastMember, unreal.TakeFX)) or label(a) in ("Cast_Band_Light", "Cast_Band_Rim"):
        a.destroy_actor()

# --- 1. guests ---------------------------------------------------------------------------------------------------------
seats = load_json("sets/hall_seats.json")
heads = [a for a in all_actors() if isinstance(a, unreal.StaticMeshActor) and label(a).startswith(("Guest_Head", "Band_Head"))]
bodies = [a for a in all_actors() if isinstance(a, unreal.StaticMeshActor) and label(a).startswith("Guest")
          and not label(a).startswith("Guest_Head")]
bodies.sort(key=lambda a: (round(a.get_actor_location().x), round(a.get_actor_location().y)))

# Who stands where: (x, y, z, yaw, seated, staff). Seated placeholders give way to the hall's chairs when they exist.
places = []
for b in bodies:
    org, ext = b.get_actor_bounds(False)
    x, y, z0 = org.x, org.y, org.z - ext.z
    seated = ext.z * 2.0 < 100.0
    hide(b)
    for hd in heads:
        hl = hd.get_actor_location()
        if abs(hl.x - x) < 5.0 and abs(hl.y - y) < 5.0:
            hide(hd)
    if seated and seats:
        continue
    staff = any(abs(x - sx) < 30.0 and abs(y - sy) < 30.0 for sx, sy in STAFF_AT)
    places.append((x, y, z0, b.get_actor_rotation().yaw, seated, staff))
if seats:
    for s in seats:
        if h01("seat", round(s["x"]), round(s["y"])) < SEAT_FILL:
            places.append((s["x"], s["y"], s["z"], s.get("yaw", 0.0), True, False))

# Kits and colours: a woman or a man by turns of a stable draw; the first combination no neighbour already wears.
assigned = []                                # (x, y, kit, colour index)
n_seated = n_glass = 0
for i, (x, y, z, yaw, seated, staff) in enumerate(places):
    if staff:
        k, tint, ci = "STAFF", STAFF_TINT, -1
    else:
        woman = h01("woman", i) < 0.5
        kits, pal = (WOMEN_KITS, WOMEN) if woman else (MEN_KITS, MEN)
        near = {(ak, aci) for (ax, ay, ak, aci) in assigned if math.hypot(ax - x, ay - y) < NEIGHBOUR_RADIUS}
        start = int(h01("combo", i) * len(kits) * len(pal))
        k, ci = kits[0], 0
        for n in range(len(kits) * len(pal)):
            j = (start + n) % (len(kits) * len(pal))
            cand = (kits[j % len(kits)], j // len(kits))
            if cand not in near:
                k, ci = cand
                break
        tint = pal[ci]
    assigned.append((x, y, k, ci))
    lo, hi = WOMEN_SCALE if k in WOMEN_KITS else MEN_SCALE
    glass = not seated and not staff and h01("glass", i) < GLASS_SHARE
    extra = {"seat_hip_forward": SEAT_HIP_FORWARD} if seated and seats else {}
    spawn_member(k, x, y, z, yaw, tint, seated=seated, name="Cast_Guest_%03d" % i,
                 pose_time=(i * 1.37) % 7.0, body_shadow=False, draw_distance=GUEST_DRAW,
                 look_seed=i * 7919 + 13, body_scale=lo + (hi - lo) * h01("scale", i), holds_glass=glass, **extra)
    n_seated += seated
    n_glass += glass
REPORT.append("guests %d (%d seated%s, %d with a glass)" % (len(places), n_seated, " on the hall's chairs" if seats else "", n_glass))

# --- 2. the band -------------------------------------------------------------------------------------------------------
hall_band = load_json("sets/hall_band.json")
hidden_band = 0
for a in all_actors():
    if isinstance(a, unreal.StaticMeshActor) and label(a).startswith(("Band", "Instrument")) and not label(a).startswith("Band_Riser"):
        hide(a)
        hidden_band += 1
props_there = bool(hall_band and hall_band.get("props"))
band_at = []
for k, (x, y, z, yaw, seated) in BAND.items():
    held = k == "BAND_TRUMPET"
    if hall_band and k in hall_band:
        e = hall_band[k]
        x, y, z, yaw, seated = e["x"], e["y"], e["z"], e.get("yaw", yaw), e.get("seated", seated)
        held = e.get("held", held)
    tint = SINGER_TINT if k == "BAND_SINGER" else BAND_TINT
    extra = {"band_clips": BAND_CLIPS}
    if k == "BAND_PIANO" and props_there:
        # --- W3 cast2 --- the pianist toward the keys, the hands onto them: measured from the placed grand's bounds.
        fx_, fy_ = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
        x, y = x + fx_ * PIANO_SIT_FORWARD, y + fy_ * PIANO_SIT_FORWARD
        grands = [a for a in all_actors() if isinstance(a, unreal.StaticMeshActor) and a.get_actor_label().startswith("Set_Hall_Piano_")
                  and "Bench" not in a.get_actor_label()]
        grands.sort(key=lambda a: math.hypot(a.get_actor_location().x - x, a.get_actor_location().y - y))
        if grands:
            o, e = grands[0].get_actor_bounds(False)
            edge = min((o.x + sx * e.x - x) * fx_ + (o.y + sy * e.y - y) * fy_ for sx in (-1, 1) for sy in (-1, 1))
            reach = max(0.0, min(40.0, edge + PIANO_KEYS_IN - PIANO_BUILT_REACH))
            extra["band_reach"] = unreal.Vector(reach, 0.0, 0.0)
            REPORT.append("piano: keys %.0f cm ahead of the pianist, hands moved %.0f cm" % (edge, reach))
    spawn_member(k, x, y, z, yaw, tint, seated=seated, name="Cast_" + k.title().replace("_", ""),
                 body_shadow=False, draw_distance=BAND_DRAW, instrument_props=props_there and not held,
                 look_seed=int(h01("band", k) * 100000), **extra)
    band_at.append((x, y, z))
cx = sum(p[0] for p in band_at) / len(band_at)
cy = sum(p[1] for p in band_at) / len(band_at)
cz = min(p[2] for p in band_at)
# A dim key over the band, so the dark suits and instruments read as shapes on the film (no shadows: cheap).
bl = EAS.spawn_actor_from_class(unreal.SpotLight, unreal.Vector(cx, cy, cz + BAND_LIGHT_UP), unreal.Rotator(0.0, -90.0, 0.0))
bl.set_actor_label("Cast_Band_Light")
bl.set_folder_path("Cast")
lc = bl.spot_light_component
lc.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
lc.set_intensity(BAND_LIGHT_CD)
lc.set_light_color(unreal.LinearColor(1.0, 0.92, 0.84, 1.0))
lc.set_editor_property("outer_cone_angle", 38.0)
lc.set_editor_property("inner_cone_angle", 20.0)
lc.set_editor_property("attenuation_radius", 900.0)
lc.set_editor_property("cast_shadows", False)
lc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
# A cool rim from behind the band toward the hall: the noir backlight on the dark suits and the instruments.
rl = EAS.spawn_actor_from_class(unreal.SpotLight, unreal.Vector(cx + BAND_RIM[0], cy + BAND_RIM[1], BAND_RIM[2]),
                                unreal.Rotator(0.0, BAND_RIM[4], BAND_RIM[3]))
rl.set_actor_label("Cast_Band_Rim")
rl.set_folder_path("Cast")
rc = rl.spot_light_component
rc.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
rc.set_intensity(BAND_RIM[5])
rc.set_light_color(unreal.LinearColor(0.55, 0.70, 1.0, 1.0))
rc.set_editor_property("outer_cone_angle", 30.0)
rc.set_editor_property("inner_cone_angle", 16.0)
rc.set_editor_property("attenuation_radius", 700.0)
rc.set_editor_property("cast_shadows", False)
rc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
REPORT.append("band 5 (%s; hid %d placeholders)" % ("hall_band.json, real props" if props_there else "old riser", hidden_band))

# --- 3. watchers -------------------------------------------------------------------------------------------------------
n_w = 0
for g in all_actors():
    if not isinstance(g, unreal.PatrolGuard):
        continue
    lb = label(g).lower()
    k = "VANCE" if "vance" in lb else "BODYGUARD" if "bodyguard" in lb else "DOORMAN" if "doorman" in lb else "GUARD"
    t = g.get_editor_property("body_tint")
    loc, rot = g.get_actor_location(), g.get_actor_rotation()
    m = spawn_member(k, loc.x, loc.y, loc.z, rot.yaw, t.r, name="Cast_" + label(g),
                     body_scale=BODYGUARD_SCALE if k == "BODYGUARD" else 1.0)
    m.set_editor_property("tint", t)
    m.attach_to_actor(g, "", unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD,
                      unreal.AttachmentRule.KEEP_WORLD, False)
    n_w += 1
REPORT.append("watchers %d" % n_w)

# --- 4. the crew -------------------------------------------------------------------------------------------------------
for c in all_actors():
    if not isinstance(c, unreal.TakeCharacter):
        continue
    lb = label(c).lower()
    k = "JULIAN" if isinstance(c, unreal.Julian) else "MAYA" if "maya" in lb else "VEX" if "vex" in lb else None
    if not k:
        continue
    loc = c.get_actor_location()
    m = spawn_member(k, loc.x, loc.y, loc.z, c.get_actor_rotation().yaw, 0.5, name="Cast_" + label(c))
    m.attach_to_actor(c, "", unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD,
                      unreal.AttachmentRule.KEEP_WORLD, False)
    REPORT.append("crew %s" % label(c))

# --- 5. effects (and the mocap clips for the cook) -----------------------------------------------------------------------
systems = {}
for name, src in FX.items():
    path = FX_DIR + "/" + name
    if not EAL.does_asset_exist(path):
        EAL.duplicate_asset(src, path)
        EAL.save_asset(path, only_if_is_dirty=False)
    systems[name] = EAL.load_asset(path)


def cue(device, when, system, points=(), at_effect=False, raise_z=0.0, scale=1.0, rot=(0.0, 0.0, 0.0)):
    c = unreal.TakeFXCue()
    c.set_editor_property("device", device)
    c.set_editor_property("when", getattr(unreal.TakeFXWhen, when))
    c.set_editor_property("system", system)
    c.set_editor_property("points", [unreal.Vector(*p) for p in points])
    c.set_editor_property("at_effect_point", at_effect)
    c.set_editor_property("effect_raise", raise_z)
    c.set_editor_property("scale", scale)
    c.set_editor_property("rotation", unreal.Rotator(*rot))
    return c


cues = []
for d in all_actors():
    if isinstance(d, unreal.PyroDevice):
        cues.append(cue(d, "LAND", systems["NS_Take_Pyro"], PYRO_CHARGES, scale=FX_SCALE["pyro"]))
    elif isinstance(d, unreal.CartDevice):
        cues.append(cue(d, "LAND", systems["NS_Take_Glass"], at_effect=True, raise_z=90.0, scale=FX_SCALE["glass"]))
    elif isinstance(d, unreal.SoundDesk):
        cues.append(cue(d, "LAND", systems["NS_Take_Sparks"], PA_TOPS, scale=FX_SCALE["sparks"]))
fx = EAS.spawn_actor_from_class(unreal.TakeFX, unreal.Vector(0.0, 0.0, 0.0))
fx.set_actor_label("TakeFX")
fx.set_folder_path("Take")
fx.set_editor_property("cues", cues)
REPORT.append("fx cues %d" % len(cues))
clips = []
for key, entry in (load_json("mocap_clips.json") or {}).items():
    asset = entry.get("asset") if isinstance(entry, dict) else None
    path = (asset or ("/Game/Take/Anim/Mocap/" + key)).split(".")[0]
    if EAL.does_asset_exist(path):
        clips.append(EAL.load_asset(path))
fx.set_editor_property("cook_clips", clips)
REPORT.append("mocap clips referenced for the cook %d" % len(clips))

LES.save_current_level()
for line in REPORT:
    print("CAST " + line)
"\n".join(REPORT)
