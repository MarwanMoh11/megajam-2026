# Step 5, characters and props: puts the costumed cast and the devices' particle effects into /Game/Take/Lvl_Auction.
# tools/build_auction.py runs it last, after tools/dress_auction.py; it also runs alone on the open map and is safe to
# re-run (it removes what it placed before):
#     "<engine python>" -I tools/ue_mcp.py py tools/build_cast.py
#
#   1. guests   every placeholder guest (a grey cylinder and a sphere, Set/Guests) gets a CastMember in its place: a
#               template mannequin frozen in a pose, costumed from basic shapes (C++ kits in CastMember.cpp); some of
#               them move to the band (the groove). The cylinder stays, hidden, so its collision is unchanged.
#   2. band     the five band placeholders and their instrument boxes are hidden; the quintet is placed on the riser
#               with its instruments (singer, trumpet, bass, drums, piano).
#   3. watchers each PatrolGuard (guards, doorman, Vance, bodyguard) gets a CastMember attached: its mannequin walks
#               with the guard and replaces the grey cylinder in play.
#   4. crew     Julian, Maya and Vex get a CastMember attached that dresses their own mesh (Julian's also bows).
#   5. fx       the Niagara systems in /Game/Take/FX (duplicated from the engine's templates) and one TakeFX actor
#               with a cue per device event.
# Values follow docs/ART_STYLE.md §15.10: Julian alone at the top, guests 0.20-0.35, the band dark, heavies near black.
import math

import unreal

EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
EAL = unreal.EditorAssetLibrary
REPORT = []

# --- tunables --------------------------------------------------------------------------------------------------------
GUEST_TINTS = (0.20, 0.28, 0.35)            # §15.10's three guest values, in rotation
GUEST_KITS = ("GUEST_TUX", "GUEST_GOWN", "GUEST_TAILS", "GUEST_GOWN_HAT", "GUEST_TUX", "GUEST_GOWN")
STAFF_AT = ((1200.0, -1700.0), (-2050.0, 1100.0), (-2650.0, 1640.0))   # the bartender, receptionist, cloakroom
# The groove (ACastMember's "Groove" properties): only the upper body moves, on smooth curves locked to the beat
# (deepest on the beat, at rest half a period later), never the whole body. Degrees; periods and phases in beats.
# The band moves the most, each by instrument. Guests: a small head nod or a weight shift, alternately, and only a
# share of them moves at all, each with its own phase (some on the off-beat, some at half time) and amplitude.
GROOVE_BAND = {
    "BAND_BASS": dict(sway_degrees=4.0, sway_beats=2.0, nod_degrees=3.0),                       # sways
    "BAND_DRUMS": dict(nod_degrees=7.0, rock_degrees=1.5, rock_beats=2.0),                      # nods
    "BAND_TRUMPET": dict(rock_degrees=5.0, rock_beats=4.0, nod_degrees=1.5),                    # dips on each bar
    "BAND_PIANO": dict(rock_degrees=3.0, rock_beats=2.0, nod_degrees=2.0, phase_beats=1.0),     # rocks on 2 and 4
    "BAND_SINGER": dict(sway_degrees=3.0, sway_beats=4.0, nod_degrees=1.5, nod_beats=2.0),
}
GUEST_CROWD = dict(move_chance=0.35, offbeat_chance=0.3, half_time_chance=0.5, phase_spread_beats=0.2, amp_spread=0.6)
GUEST_NOD = dict(nod_degrees=3.0, nod_beats=1.0)            # every other guest: a head nod...
GUEST_SHIFT = dict(sway_degrees=1.5, sway_beats=4.0)        # ...or a weight shift over a bar
SEATED_DROP = 86.0                          # a seated body sits this far below the seat's top (hips at the seat)
SEATED_TIME = 0.0                           # the idle frame it is bent from, at the hips and knees (CastMember)
BAND_TINT = 0.04
GUEST_DRAW = 3500.0                         # guests (and their pieces) are not drawn beyond this, cm
BAND_DRAW = 6000.0
# Guests and the band cast no shadow: a body that moves would redraw every shadow-casting key's cached shadow each
# frame (it cost 1.4-3.3 ms of GPU time with shadows on). The watchers and the crew keep theirs.
RISER_Z = 30.0                              # the band riser's top
BAND = (                                    # kit, x, y, z (seat top when seated), yaw, seated
    # Laid out across the riser as the hall floor sees it (from the west-north-west, the side aisles and the rope), so
    # no player stands in another's line of sight: the drums at the back facing the room, the upright piano against
    # the riser's east edge with the pianist's back to the room, the singer, trumpet and bass in front.
    ("BAND_SINGER", 3676.0, 1365.0, RISER_Z, -140.0, False),
    ("BAND_TRUMPET", 3671.0, 1502.0, RISER_Z, -130.0, False),
    ("BAND_BASS", 3693.0, 1571.0, RISER_Z, -150.0, False),
    ("BAND_DRUMS", 3811.0, 1561.0, RISER_Z + 50.0, -140.0, True),
    ("BAND_PIANO", 3830.0, 1420.0, RISER_Z + 50.0, 0.0, True),
)
BAND_LIGHT = (3780.0, 1440.0, 620.0, 300.0)  # x, y, z, candelas
BAND_RIM = (3905.0, 1585.0, 430.0, -140.0, -32.0, 250.0)  # x, y, z, yaw, pitch, candelas: behind the band, at the hall
TRUMPET_POSE = "/Game/Characters/Mannequins/Anims/Pistol/MF_Pistol_Idle_ADS"
BODYGUARD_SCALE = 1.1
FX_DIR = "/Game/Take/FX"
FX = {                                      # our system: the engine template it is duplicated from
    "NS_Take_Pyro": "/Niagara/DefaultAssets/Templates/Systems/SimpleExplosion",
    "NS_Take_Sparks": "/Niagara/DefaultAssets/Templates/Systems/DirectionalBurst",
    "NS_Take_Glass": "/Niagara/DefaultAssets/Templates/Systems/RadialBurst",
}
DAIS_H = 20.0
# The four charges the dressing stands on the dais lip (tools/dress_auction.py): none at the aisle, Julian's way onto
# the dais, 30 cm from the dais mark.
PYRO_CHARGES = [(3670.0, y, DAIS_H) for y in (-900.0, -450.0, 450.0, 900.0)]
PA_TOPS = [(4130.0, 1250.0, 240.0), (4225.0, 1250.0, 240.0)]
FX_SCALE = {"pyro": 0.35, "glass": 0.5, "sparks": 0.5}


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


def spawn_member(k, x, y, z, yaw, tint, seated=False, groove=None, name=None, **props):
    m = EAS.spawn_actor_from_class(unreal.CastMember, unreal.Vector(x, y, z), unreal.Rotator(0.0, 0.0, yaw))
    m.set_actor_label(name or ("Cast_" + k.title().replace("_", "")))
    m.set_folder_path("Cast")
    m.set_editor_property("kit", kit(k))
    m.set_editor_property("tint", unreal.LinearColor(tint, tint, tint, 1.0))
    m.set_editor_property("seated", seated)
    m.set_editor_property("seated_drop", SEATED_DROP)
    m.set_editor_property("seated_time", SEATED_TIME)
    for key, val in (groove or {}).items():
        m.set_editor_property(key, val)
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
heads = [a for a in all_actors() if isinstance(a, unreal.StaticMeshActor) and label(a).startswith(("Guest_Head", "Band_Head"))]
bodies = [a for a in all_actors() if isinstance(a, unreal.StaticMeshActor) and label(a).startswith("Guest")
          and not label(a).startswith("Guest_Head")]
bodies.sort(key=lambda a: (round(a.get_actor_location().x), round(a.get_actor_location().y)))
n_seated = 0
for i, b in enumerate(bodies):
    org, ext = b.get_actor_bounds(False)
    x, y, z0 = org.x, org.y, org.z - ext.z
    seated = ext.z * 2.0 < 100.0
    yaw = b.get_actor_rotation().yaw
    staff = any(abs(x - sx) < 30.0 and abs(y - sy) < 30.0 for sx, sy in STAFF_AT)
    k = "STAFF" if staff else GUEST_KITS[i % len(GUEST_KITS)]
    tint = GUEST_TINTS[(i * 7) % 3]
    groove = {} if staff else dict(GUEST_CROWD, **(GUEST_NOD if i % 2 == 0 else GUEST_SHIFT))
    spawn_member(k, x, y, z0, yaw, tint, seated=seated, groove=groove,
                 name="Cast_Guest_%03d" % i, pose_time=(i * 1.37) % 7.0, body_shadow=False,
                 draw_distance=GUEST_DRAW)
    hide(b)
    for h in heads:
        hl = h.get_actor_location()
        if abs(hl.x - x) < 5.0 and abs(hl.y - y) < 5.0:
            hide(h)
    n_seated += seated
REPORT.append("guests %d (%d seated)" % (len(bodies), n_seated))

# --- 2. the band -------------------------------------------------------------------------------------------------------
hidden_band = 0
for a in all_actors():
    if isinstance(a, unreal.StaticMeshActor) and label(a).startswith(("Band", "Instrument")) and not label(a).startswith("Band_Riser"):
        hide(a)
        hidden_band += 1
trumpet_pose = EAL.load_asset(TRUMPET_POSE)
for (k, x, y, z, yaw, seated) in BAND:
    extra = {"pose_anim": trumpet_pose, "pose_time": 0.0} if k == "BAND_TRUMPET" else {}
    spawn_member(k, x, y, z, yaw, BAND_TINT, seated=seated, groove=GROOVE_BAND[k], name="Cast_" + k.title().replace("_", ""),
                 body_shadow=False, draw_distance=BAND_DRAW, **extra)
# A dim key over the riser, so the dark suits and instruments read as shapes on the film (no shadows: cheap).
bl = EAS.spawn_actor_from_class(unreal.SpotLight, unreal.Vector(BAND_LIGHT[0], BAND_LIGHT[1], BAND_LIGHT[2]),
                                unreal.Rotator(0.0, -90.0, 0.0))
bl.set_actor_label("Cast_Band_Light")
bl.set_folder_path("Cast")
lc = bl.spot_light_component
lc.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
lc.set_intensity(BAND_LIGHT[3])
lc.set_light_color(unreal.LinearColor(1.0, 0.92, 0.84, 1.0))
lc.set_editor_property("outer_cone_angle", 38.0)
lc.set_editor_property("inner_cone_angle", 20.0)
lc.set_editor_property("attenuation_radius", 900.0)
lc.set_editor_property("cast_shadows", False)
lc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
# A cool rim from behind the riser toward the hall: the noir backlight that cuts the dark suits, the drum kit and the
# piano's edges out of the black wall behind them (no shadows: cheap).
rl = EAS.spawn_actor_from_class(unreal.SpotLight, unreal.Vector(BAND_RIM[0], BAND_RIM[1], BAND_RIM[2]),
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
REPORT.append("band 5 (hid %d placeholders)" % hidden_band)

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

# --- 5. effects --------------------------------------------------------------------------------------------------------
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

LES.save_current_level()
for line in REPORT:
    print("CAST " + line)
"\n".join(REPORT)
