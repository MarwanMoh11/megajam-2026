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
MEN = [(0.025, 0.025, 0.03), (0.07, 0.07, 0.075), (0.04, 0.06, 0.15), (0.13, 0.08, 0.05), (0.04, 0.12, 0.07),
       (0.18, 0.03, 0.045), (0.34, 0.31, 0.24), (0.11, 0.14, 0.20), (0.02, 0.03, 0.08), (0.16, 0.16, 0.16),
       (0.30, 0.21, 0.11), (0.10, 0.04, 0.09), (0.12, 0.12, 0.05)]
WOMEN = [(0.22, 0.03, 0.05), (0.04, 0.15, 0.08), (0.05, 0.07, 0.18), (0.36, 0.28, 0.16), (0.32, 0.16, 0.16),
         (0.03, 0.03, 0.035), (0.14, 0.05, 0.13), (0.12, 0.15, 0.22), (0.36, 0.33, 0.26), (0.03, 0.14, 0.15),
         (0.30, 0.10, 0.03), (0.22, 0.22, 0.24), (0.20, 0.15, 0.26), (0.36, 0.13, 0.10), (0.02, 0.03, 0.09)]
MEN_KITS = ("GUEST_TUX", "GUEST_TAILS")
WOMEN_KITS = ("GUEST_GOWN", "GUEST_GOWN_HAT")
STAFF_TINT = (0.035, 0.035, 0.04)
NEIGHBOUR_RADIUS = 260.0                    # no two guests this close share a kit and colour, cm (and they differ in at
                                            # least NEIGHBOUR_DIFFER of the far-reading traits: colour, skin, hair colour,
                                            # the head's outline, the face or wrap)
NEIGHBOUR_DIFFER = 3
MEN_SCALE = (0.93, 1.08)                    # body scale ranges (Quinn is already smaller)
WOMEN_SCALE = (0.92, 1.05)
MEN_WIDTH = (0.88, 1.18)                    # the build: breadth against height (older men run heavier)
WOMEN_WIDTH = (0.88, 1.10)
GLASS_SHARE = 0.4                           # standing guests holding a coupe (they sip from it)
# --- crowd pass (Nov 15): every guest a different person (FCastLook; the body painted by M_CrowdBody) ---
CROWD_LOOKS = True                          # False: the old seed-drawn variety (skin shell, tinted mannequin)
CROWD_MAT = "/Game/Take/Cast/Crowd/M_CrowdBody"   # tools/crowd_material.py makes it
SKINS = [(0.62, 0.42, 0.32), (0.55, 0.36, 0.27), (0.52, 0.34, 0.22), (0.47, 0.30, 0.20), (0.42, 0.27, 0.16),
         (0.34, 0.20, 0.12), (0.27, 0.15, 0.09), (0.19, 0.10, 0.06), (0.13, 0.07, 0.04), (0.08, 0.045, 0.03)]
HAIRS = [(0.012, 0.010, 0.009), (0.04, 0.025, 0.015), (0.08, 0.045, 0.025), (0.14, 0.05, 0.02), (0.30, 0.10, 0.03),
         (0.22, 0.15, 0.07), (0.38, 0.28, 0.13), (0.50, 0.45, 0.35)]
OLD_HAIRS = [(0.20, 0.19, 0.18), (0.42, 0.41, 0.39), (0.10, 0.10, 0.10)]   # grey, silver, salt and pepper
OLD_SHARE = 0.3                             # older guests: grey or silver hair, more balding, glasses and breadth
ACCENTS = [(0.010, 0.010, 0.012), (0.14, 0.018, 0.025), (0.015, 0.07, 0.035), (0.012, 0.018, 0.06),
           (0.38, 0.34, 0.25), (0.30, 0.20, 0.06), (0.09, 0.025, 0.07), (0.32, 0.16, 0.16), (0.03, 0.12, 0.13)]
SHIRTS = [(0.40, 0.39, 0.36), (0.37, 0.34, 0.27), (0.30, 0.33, 0.38)]
GLOVES = [(0.40, 0.39, 0.36), (0.015, 0.015, 0.018), (0.36, 0.32, 0.24), (0.18, 0.17, 0.16)]
FURS = [(0.40, 0.39, 0.37), (0.11, 0.06, 0.035), (0.22, 0.22, 0.23), (0.02, 0.02, 0.02)]
LIPS = [(0.22, 0.02, 0.03), (0.10, 0.01, 0.02), (0.30, 0.10, 0.10), (0.32, 0.09, 0.05)]
MEN_HATS = [(0.012, 0.012, 0.012), (0.05, 0.04, 0.03), (0.10, 0.10, 0.10), (0.02, 0.03, 0.06)]
MEN_HAT_SHARE = 0.22                        # men in a hat; women: GUEST_GOWN_HAT always, GUEST_GOWN never
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
# --- featured NPCs (Nov 15): everyone Julian meets looks like nobody else. The Rodin people (tools/build_heroes.py NPCS:
# /Game/Take/Cast/NPCs/<Name>/SKM_NPC_<Name>, HeroMesh) go on the watchers by label, the singer, the receptionist and the
# guest ahead of Julian; the rest are hand-kitted below (kit, tint, LookSeed, body scale; seeds picked for hair,
# glasses, moustache, hat and skin). A Rodin mesh missing on disk leaves that person on the mannequin.
NPC_DIR = "/Game/Take/Cast/NPCs"
NPC_BODYGUARD_SCALE = 1.04                  # the Rodin bodyguard is already broad
WATCHER_NPC = {"vance": "Vance", "doorman": "Doorman", "bodyguard": "Bodyguard", "chief": "Chief", "guard": "Guard"}
WATCHER_LOOK = {                            # the alarm's security men by label: tint, LookSeed, body scale
    "security_n_door": ((0.030, 0.040, 0.095), 1039, 1.07),   # navy, tan, bald under the cap, wide moustache
    "security_n_flank": ((0.060, 0.065, 0.070), 1000, 0.96),  # slate, pale, glasses
    "security_s_door": ((0.055, 0.060, 0.030), 1004, 1.02),   # olive drab, brown, clean-shaven
    "security_s_flank": ((0.020, 0.020, 0.025), 1059, 0.99),  # near black, light, thin moustache
}
BAND_LOOK = {                               # the band: tint, LookSeed, body scale (the singer is Rodin's)
    "BAND_PIANO": ((0.020, 0.028, 0.085), 8632, 0.97),        # midnight navy; silver slicked hair, glasses, moustache
    "BAND_DRUMS": ((0.200, 0.120, 0.030), 1038, 1.03),        # mustard; dark skin, a fedora
    "BAND_BASS": ((0.020, 0.075, 0.040), 18149, 1.08),        # bottle green; bald, wide moustache, tall
    "BAND_TRUMPET": ((0.100, 0.025, 0.085), 1040, 0.95),      # plum; olive skin, short auburn hair, slight
}
NPC_BAND = {"BAND_SINGER": "Singer"}
# STAFF_AT order: the bartender, the receptionist (the clerk AQueueLine moves to the desk), the cloakroom attendant
STAFF_LOOK = [
    dict(kit="STAFF", tint=(0.30, 0.27, 0.20), seed=8761, scale=1.04, npc="Bartender"),  # Rodin; else cream, bald, walrus
    dict(kit="STAFF", tint=(0.05, 0.05, 0.06), seed=1508, scale=0.97, npc="Clerk"),
    dict(kit="STAFF", tint=(0.14, 0.02, 0.03), seed=8397, scale=0.95),                 # burgundy; brown skin, grey slicked hair, glasses, thin moustache
]
QUEUE_RADIUS = 60.0                         # a queue guest stands within this of its QueueLine place, cm
QUEUE_LOOK = {                              # by QueueLine place (Julian's is 3): kit, tint, LookSeed, body scale, Rodin,
    # and an explicit CastLook (the crowd pass's painted body; field names as in unreal.CastLook) when it is compiled
    0: dict(kit="GUEST_GOWN_HAT", tint=(0.10, 0.02, 0.12), seed=2195, scale=0.94,       # the dowager in purple
            look=dict(skin=(0.55, 0.36, 0.27), hair=(0.40, 0.39, 0.37), hair_style=16, hat=13, hat_colour=(0.06, 0.01, 0.07),
                      glasses=1, neck=11, wrap=2, wrap_colour=(0.36, 0.33, 0.26), gloves=2, chest=4, accent=(0.30, 0.20, 0.06),
                      lips=(0.12, 0.02, 0.04), earrings=True, width=1.12)),
    1: dict(kit="GUEST_TAILS", tint=(0.02, 0.03, 0.10), seed=2495, scale=1.07,          # the old admiral: top hat, monocle
            look=dict(skin=(0.50, 0.30, 0.24), hair=(0.40, 0.39, 0.37), hair_style=1, facial_hair=2, hat=2,
                      hat_colour=(0.012, 0.012, 0.012), glasses=3, neck=1, gloves=1, chest=1, width=0.94)),
    2: dict(kit="GUEST_TUX", tint=(0.13, 0.08, 0.05), seed=1, scale=1.0, npc="Queue"),  # "Wait your turn, pal" (Rodin)
    4: dict(kit="GUEST_GOWN", tint=(0.30, 0.02, 0.03), seed=4346, scale=0.98,           # scarlet, blonde victory rolls
            look=dict(skin=(0.45, 0.30, 0.20), hair=(0.33, 0.24, 0.11), hair_style=12, hair_flower=True, neck=13,
                      gloves=2, glove_colour=(0.012, 0.012, 0.012), lips=(0.25, 0.01, 0.02), neckline=2, earrings=True,
                      accent=(0.38, 0.34, 0.25), width=0.95)),
    5: dict(kit="GUEST_TUX", tint=(0.13, 0.07, 0.03), seed=4444, scale=0.93,            # short and stout: bald, beard, bowler
            look=dict(skin=(0.55, 0.36, 0.27), hair=(0.08, 0.045, 0.025), hair_style=3, facial_hair=3, glasses=2, hat=4,
                      hat_colour=(0.05, 0.03, 0.015), neck=2, accent=(0.14, 0.018, 0.025), cummerbund=True, chest=2,
                      width=1.25)),
    6: dict(kit="GUEST_GOWN_HAT", tint=(0.02, 0.10, 0.10), seed=2103, scale=1.0,        # teal, a gold turban and a stole
            look=dict(skin=(0.18, 0.10, 0.06), hair=(0.012, 0.010, 0.009), hair_style=14, hat=14,
                      hat_colour=(0.30, 0.20, 0.06), neck=12, wrap=1, wrap_colour=(0.37, 0.34, 0.27), gloves=1,
                      lips=(0.09, 0.025, 0.07), earrings=True, sash=True, accent=(0.30, 0.20, 0.06), width=1.0)),
    7: dict(kit="GUEST_TUX", tint=(0.03, 0.10, 0.05), seed=3090, scale=1.02,            # green, pencil moustache, fedora
            look=dict(skin=(0.28, 0.16, 0.09), hair=(0.012, 0.010, 0.009), hair_style=1, facial_hair=1, hat=1,
                      hat_colour=(0.10, 0.09, 0.08), neck=3, accent=(0.14, 0.018, 0.025), chest=3, width=0.9)),
    8: dict(kit="GUEST_GOWN", tint=(0.36, 0.26, 0.08), seed=2384, scale=0.96,           # gold, a black bob, a wide brim
            look=dict(skin=(0.10, 0.055, 0.035), hair=(0.012, 0.010, 0.009), hair_style=13, hat=12,
                      hat_colour=(0.012, 0.012, 0.012), neck=11, gloves=2, lips=(0.14, 0.018, 0.025), neckline=1,
                      sash=True, accent=(0.012, 0.012, 0.012), width=1.05)),
}
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


def pick(seq, *parts):
    return seq[min(int(h01(*parts) * len(seq)), len(seq) - 1)]


def weighted(pairs, *parts):
    """pairs: (value, weight); a stable weighted choice."""
    r = h01(*parts) * sum(w for _, w in pairs)
    for v, w in pairs:
        r -= w
        if r < 0.0:
            return v
    return pairs[-1][0]


def draw_look(i, salt, k, ci, seated):
    """One guest's whole look as plain values (indices into the palettes): build_cast's uniqueness and neighbour
    checks run on these; cast_look() turns them into an unreal.CastLook."""
    woman = k in WOMEN_KITS
    r = lambda tag: h01("look", tag, i, salt)
    old = r("old") < OLD_SHARE
    L = {"kit": k, "ci": ci, "woman": woman, "old": old,
         "skin": int(r("skin") * len(SKINS)) % len(SKINS),
         "accent": int(r("accent") * len(ACCENTS)) % len(ACCENTS),
         "width": round(r("width"), 3)}
    if old:
        L["hairc"] = len(HAIRS) + int(r("hairc") * len(OLD_HAIRS)) % len(OLD_HAIRS)
    else:
        L["hairc"] = weighted([(0, 4), (1, 4), (2, 3), (3, 2), (4, 1), (5, 1.5), (6, 1), (7, 0.5)], "hairc", i, salt)   # mostly dark: light hair read as bald
    if woman:
        L["style"] = 10 + int(r("style") * 7) % 7
        L["hat"] = (11 + int(r("hat") * 4) % 4) if k == "GUEST_GOWN_HAT" else 0
        L["hatc"] = int(r("hatc") * len(ACCENTS)) % len(ACCENTS)
        L["face"] = 0
        L["glasses"] = weighted([(0, 92), (1, 4), (2, 4)], "glasses", i, salt)
        L["neck"] = weighted([(10, 3), (11, 3), (12, 2), (13, 2)], "neck", i, salt)
        L["neckline"] = int(r("neckline") * 3) % 3
        L["bare"] = r("bare") < ((0.5 if L["neckline"] == 2 else 0.3) if seated else (0.8 if L["neckline"] == 2 else 0.45))   # seated: no skirt shape, so mostly sleeved
        L["gloves"] = weighted([(0, 45), (1, 20), (2, 35)], "gloves", i, salt)
        L["glovec"] = int(r("glovec") * (len(GLOVES) + 1)) % (len(GLOVES) + 1)   # the last: the gown's colour
        L["wrap"] = weighted([(0, 70), (1, 30), (2, 0)], "wrap", i, salt)              # 2, the fur collar, read as a ring
        L["wrapc"] = int(r("wrapc") * (len(FURS) + 1)) % (len(FURS) + 1)        # the last: the accent
        L["earrings"] = r("earrings") < 0.55
        L["chest"] = 4 if r("brooch") < 0.15 else 0
        L["flower"] = L["hat"] == 0 and r("flower") < 0.2
        L["sash"] = r("sash") < 0.25
        L["lower"] = -1 if r("skirt") < 0.85 else 0                              # -1: the gown's colour, 0: black
        L["lips"] = int(r("lips") * len(LIPS)) % len(LIPS)
        L["shirt"] = 0
        L["cummer"] = False
    else:
        styles = [(0, 4), (1, 3), (2, 1), (3, 0.5), (4, 1.5), (5, 2)] if not old else [(0, 2), (1, 2), (2, 4), (3, 2), (4, 3)]
        L["style"] = weighted(styles, "style", i, salt)
        hat = r("hatshare") < MEN_HAT_SHARE
        L["hat"] = 0 if not hat else (weighted([(2, 3), (1, 1), (3, 1)], "hatk", i, salt) if k == "GUEST_TAILS"
                                      else weighted([(1, 3), (3, 2), (4, 2)], "hatk", i, salt))
        L["hatc"] = int(r("hatc") * len(MEN_HATS)) % len(MEN_HATS)
        L["face"] = weighted([(0, 55), (1, 10), (2, 12), (3, 8), (4, 7), (5, 8)], "face", i, salt)
        L["glasses"] = weighted([(0, 60 if old else 80), (1, 10), (2, 8), (3, 2)], "glasses", i, salt)
        L["neck"] = weighted([(0, 5), (1, 3 if k == "GUEST_TAILS" else 1), (2, 1.5), (3, 1)], "neck", i, salt)
        L["neckline"] = 0
        L["bare"] = False
        L["gloves"] = 1 if r("gloves") < 0.08 else 0
        L["glovec"] = 0
        L["wrap"] = 0
        L["wrapc"] = 0
        L["earrings"] = False
        L["chest"] = weighted([(0, 4), (1, 2.5), (2, 1.5), (3, 2)], "chest", i, salt)
        L["flower"] = False
        L["sash"] = False
        L["lower"] = 0 if (k == "GUEST_TAILS" or r("suit") < 0.7) else -1          # black trousers, or the suit's
        L["lips"] = 0
        L["shirt"] = weighted([(0, 6), (1, 3), (2, 1)], "shirt", i, salt)
        L["cummer"] = r("cummer") < 0.25
    if old:
        L["width"] = round(0.35 + 0.65 * L["width"], 3)
    return L


def far_traits(L):
    """What tells two guests apart at 3-10 m: the colour, the skin, the hair's colour, the head's outline, the face or
    wrap, the arms (bare, gloved)."""
    head = ("hat", L["hat"]) if L["hat"] else ("hair", L["style"])
    face = ("fur", L["wrap"]) if L["woman"] else ("face", L["face"], L["glasses"] > 0)
    return ((L["kit"], L["ci"]), L["skin"], L["hairc"], head, face, (L["bare"], L["gloves"]))


def look_key(L):
    """The full look: every discrete choice (two guests with the same key would be twins)."""
    return tuple(sorted((key, v) for key, v in L.items() if key not in ("width", "old")))


def cast_look(L, pal):
    c = colour
    look = unreal.CastLook()
    hairs = HAIRS + OLD_HAIRS
    tint = pal[L["ci"]]
    look.set_editor_property("explicit", True)
    look.set_editor_property("skin", c(SKINS[L["skin"]]))
    look.set_editor_property("hair", c(hairs[L["hairc"]]))
    look.set_editor_property("hair_style", L["style"])
    look.set_editor_property("facial_hair", L["face"])
    look.set_editor_property("hat", L["hat"])
    look.set_editor_property("hat_colour", c(ACCENTS[L["hatc"]] if L["woman"] else MEN_HATS[L["hatc"]]))
    look.set_editor_property("glasses", L["glasses"])
    look.set_editor_property("neck", L["neck"])
    look.set_editor_property("wrap", L["wrap"])
    look.set_editor_property("wrap_colour", c(FURS[L["wrapc"]] if L["wrapc"] < len(FURS) else ACCENTS[L["accent"]]))
    look.set_editor_property("gloves", L["gloves"])
    look.set_editor_property("glove_colour", c(GLOVES[L["glovec"]] if L["glovec"] < len(GLOVES) else tint))
    look.set_editor_property("chest", L["chest"])
    look.set_editor_property("accent", c(ACCENTS[L["accent"]]))
    look.set_editor_property("shirt", c(SHIRTS[L["shirt"]]))
    look.set_editor_property("lower", c(tint if L["lower"] < 0 else (0.015, 0.015, 0.018)))
    look.set_editor_property("shoes", c((0.012, 0.012, 0.012)))
    look.set_editor_property("lips", c(LIPS[L["lips"]]))
    look.set_editor_property("neckline", L["neckline"])
    look.set_editor_property("bare_arms", L["bare"])
    look.set_editor_property("cummerbund", L["cummer"])
    look.set_editor_property("earrings", L["earrings"])
    look.set_editor_property("hair_flower", L["flower"])
    look.set_editor_property("sash", L["sash"])
    lo, hi = WOMEN_WIDTH if L["woman"] else MEN_WIDTH
    look.set_editor_property("width", lo + (hi - lo) * L["width"])
    return look


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
crowd_mat = unreal.load_asset(CROWD_MAT) if CROWD_LOOKS and EAL.does_asset_exist(CROWD_MAT) else None
looks = []                                   # (x, y, look) of the guests with an explicit look
taken = set()                                # every full look so far
retries = 0
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
    if crowd_mat and not staff:
        # The first draw that is nobody's twin and differs from every neighbour in NEIGHBOUR_DIFFER far traits.
        near = [lk for (lx, ly, lk) in looks if math.hypot(lx - x, ly - y) < NEIGHBOUR_RADIUS]
        best, best_score = None, -1
        for salt in range(60):
            cand = draw_look(i, salt, k, ci, seated)
            if look_key(cand) in taken:
                continue
            ft = far_traits(cand)
            score = min([sum(a != b for a, b in zip(ft, far_traits(o))) for o in near] or [99])
            if score > best_score:
                best, best_score = cand, score
            if score >= NEIGHBOUR_DIFFER:
                break
            retries += 1
        taken.add(look_key(best))
        looks.append((x, y, best))
        extra["look"] = cast_look(best, WOMEN if k in WOMEN_KITS else MEN)
        extra["look"].set_editor_property("for_seed", i * 7919 + 13)    # re-seeded later (a featured person): dropped
        extra["crowd_body_material"] = crowd_mat
    spawn_member(k, x, y, z, yaw, tint, seated=seated, name="Cast_Guest_%03d" % i,
                 pose_time=(i * 1.37) % 7.0, body_shadow=False, draw_distance=GUEST_DRAW,
                 look_seed=i * 7919 + 13, body_scale=lo + (hi - lo) * h01("scale", i), holds_glass=glass, **extra)
    n_seated += seated
    n_glass += glass
REPORT.append("guests %d (%d seated%s, %d with a glass)" % (len(places), n_seated, " on the hall's chairs" if seats else "", n_glass))
if looks:
    # The guarantee, checked on what was placed: no two guests share a full look; neighbours differ visibly.
    keys = [look_key(lk) for (_, _, lk) in looks]
    assert len(set(keys)) == len(keys), "two guests share a look"
    close = weak = 0
    for a in range(len(looks)):
        for b in range(a + 1, len(looks)):
            (ax, ay, la), (bx, by, lb) = looks[a], looks[b]
            if math.hypot(ax - bx, ay - by) < NEIGHBOUR_RADIUS:
                close += 1
                weak += sum(p != q for p, q in zip(far_traits(la), far_traits(lb))) < NEIGHBOUR_DIFFER
    REPORT.append("guest looks: %d guests, %d distinct full looks (all unique: %s); %d neighbour pairs, %d differ in "
                  "fewer than %d far traits; %d redraws" % (len(looks), len(set(keys)), len(set(keys)) == len(keys),
                                                           close, weak, NEIGHBOUR_DIFFER, retries))
elif CROWD_LOOKS:
    REPORT.append("guest looks: %s missing (run tools/crowd_material.py): the old seed-drawn variety" % CROWD_MAT)

# --- 2. the band -------------------------------------------------------------------------------------------------------
def npc_mesh(name):
    """The featured NPC's Rodin skeletal mesh (tools/build_heroes.py NPCS), or None while it is not built."""
    p = "%s/%s/SKM_NPC_%s" % (NPC_DIR, name, name)
    return EAL.load_asset(p) if name and EAL.does_asset_exist(p) else None


def restyle(m, kit_name=None, tint=None, seed=None, scale=None, npc=None, quinn=None, look=None):
    """Re-dresses a placed CastMember as one featured person; returns what it now wears, for the report. look: an
    explicit CastLook (guest kits, when the crowd pass's C++ is compiled); without one a re-seeded guest drops its old
    explicit look and is drawn from the seed."""
    if look and hasattr(unreal, "CastLook"):
        cl = unreal.CastLook()
        cl.set_editor_property("explicit", True)
        cl.set_editor_property("for_seed", seed if seed is not None else -1)
        for key, val in look.items():
            cl.set_editor_property(key, colour(val) if isinstance(val, tuple) else val)
        m.set_editor_property("look", cl)
    if kit_name:
        m.set_editor_property("kit", kit(kit_name))
    if tint is not None:
        m.set_editor_property("tint", colour(tint))
    if seed is not None:
        m.set_editor_property("look_seed", seed)
    if scale is not None:
        m.set_editor_property("body_scale", scale)
    if quinn is not None:
        m.set_editor_property("quinn", quinn)
    skm = npc_mesh(npc)
    if skm:
        m.set_editor_property("hero_mesh", skm)
    if hasattr(m, "rerun_construction_scripts"):     # set_editor_property already re-runs OnConstruction
        m.rerun_construction_scripts()
    return skm.get_name() if skm else (kit_name or "kit")


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
    bm = spawn_member(k, x, y, z, yaw, tint, seated=seated, name="Cast_" + k.title().replace("_", ""),
                      body_shadow=False, draw_distance=BAND_DRAW, instrument_props=props_there and not held,
                      look_seed=int(h01("band", k) * 100000), **extra)
    if k in BAND_LOOK or k in NPC_BAND:          # --- featured NPCs: each player his own jacket, face and build
        bt, bs, bsc = BAND_LOOK.get(k, (None, None, None))
        REPORT.append("band %s: %s" % (k, restyle(bm, tint=bt, seed=bs, scale=bsc, npc=NPC_BAND.get(k))))
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
    if lb in WATCHER_NPC or lb in WATCHER_LOOK:  # --- featured NPCs
        wt, ws, wsc = WATCHER_LOOK.get(lb, (None, None, NPC_BODYGUARD_SCALE if k == "BODYGUARD" else None))
        REPORT.append("watcher %s: %s" % (label(g), restyle(m, tint=wt, seed=ws, scale=wsc, npc=WATCHER_NPC.get(lb))))
    n_w += 1
REPORT.append("watchers %d" % n_w)

# --- featured NPCs: the staff and the queue (placed by section 1 as guests; re-dressed here, after it) ------------------
for m in [a for a in all_actors() if isinstance(a, unreal.CastMember) and not a.get_attach_parent_actor()]:
    ml = m.get_actor_location()
    for si, (sx, sy) in enumerate(STAFF_AT):
        if si < len(STAFF_LOOK) and math.hypot(ml.x - sx, ml.y - sy) < 30.0:
            d = STAFF_LOOK[si]
            REPORT.append("staff %d %s: %s" % (si, label(m), restyle(m, d["kit"], d["tint"], d["seed"], d["scale"],
                                                                      d.get("npc"), d.get("quinn", False))))
queue = [a for a in all_actors() if isinstance(a, unreal.QueueLine)]
if queue:
    free = [a for a in all_actors() if isinstance(a, unreal.CastMember) and not a.get_attach_parent_actor()
            and not a.get_editor_property("seated")]
    for qi, p in enumerate(queue[0].get_editor_property("places")):
        d = QUEUE_LOOK.get(qi)
        near = sorted(free, key=lambda a: math.hypot(a.get_actor_location().x - p.x, a.get_actor_location().y - p.y))
        if not d or not near or math.hypot(near[0].get_actor_location().x - p.x,
                                           near[0].get_actor_location().y - p.y) > QUEUE_RADIUS:
            continue
        REPORT.append("queue %d %s: %s" % (qi, label(near[0]), restyle(near[0], d["kit"], d["tint"], d["seed"],
                                                                         d["scale"], d.get("npc"), False,
                                                                         d.get("look"))))

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
