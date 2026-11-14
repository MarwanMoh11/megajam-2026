# Dresses /Game/Take/Lvl_Auction for the film (step 4, pass 3; docs/ART_STYLE.md §15.9 and §15.10).
# tools/build_auction.py runs it last, with its own numbers (so the map stays rebuildable from text):
#     _dress = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir() + "../tools/dress_auction.py")
#     exec(open(_dress).read(), dict(globals(), __name__="dress_auction"))
# It also runs alone on the open map, and is safe to re-run (the defaults below mirror build_auction.py's numbers):
#     "<engine python>" -I tools/ue_mcp.py py tools/dress_auction.py
#
#   1. materials  M_Dress (lit: Color, Roughness, Specular, Emissive x EmissiveScale), M_DressGlow (unlit: Color x
#                 Scale), M_SignalPattern (unlit, masked: Color, and Pattern 0 solid / 1 dashes along a line from its
#                 start / 2 bands outwards from the actor / 3 hatch at 45 degrees). The C++ shapes use M_SignalPattern
#                 for everything that carries a stencil (CPP.md), so red and gold keep pass 1's unlit signal look.
#   2. surfaces   a post-pass: every StaticMeshActor gets its §15.9 instance from the material it was built with
#                 (MI_Box_*, remembered in a "dress:" tag), its label and the zone its centre is in. No line of
#                 build_auction.py is edited, so the pass survives that file moving.
#   3. dressing   §15.9's small boxes, all NoCollision: no walker, sight line, aim or camera trace can touch them.
#                 A piece is skipped where it would cover a gameplay actor, a tape strip, an opening or a prop.
#   4. lights     §15.9's retune and the new Back_Dais; prints every shadow caster.
#   5. tints      §15.10 on Julian, Maya, Vex, the watchers' bodies, the guests, the band and the devices.
import math

import unreal

ONLY = globals().get("DRESS_ONLY")   # e.g. ("materials",) or ("surfaces", "dressing"): run part of it (each step
                                     # is re-runnable); set DRESS_ONLY in a wrapper that execs this file

# --- numbers from build_auction.py (its globals win when it runs this file) --------------------------------------------
G = globals()


def k(name, default):
    v = G.get(name)
    return default if v is None else v


HX0, HX1, HY0, HY1 = k("HX0", -1000.0), k("HX1", 4400.0), k("HY0", -2000.0), k("HY1", 2000.0)
HALL_H, F2, SLAB, T = k("HALL_H", 1400.0), k("F2", 450.0), k("SLAB", 20.0), k("T", 20.0)
LX0, LX1, LY0, LY1 = k("LX0", -3200.0), k("LX1", -1000.0), k("LY0", -1400.0), k("LY1", 1400.0)
VOID_X0, VOID_X1 = k("VOID_X0", -2700.0), k("VOID_X1", -1500.0)
BY0, BACK_H, SY0, NY1, EX1 = k("BY0", -2600.0), k("BACK_H", 500.0), k("SY0", -3300.0), k("NY1", 2900.0), k("EX1", 5300.0)
PAVE_X0 = k("PAVE_X0", LX0 - 900.0)
FRONT = k("FRONT", LX0 - T)
FRONT_DOOR_HALF, LANE_ROPE = k("FRONT_DOOR_HALF", 450.0), k("LANE_ROPE", 200.0)
LANE_Y1 = k("LANE_Y1", 450.0)
START = k("START", (PAVE_X0 + 130.0, 980.0))
DAIS_X0, DAIS_H, GAP = k("DAIS_X0", 3600.0), k("DAIS_H", 20.0), k("GAP", 150.0)
VANCE = k("VANCE", (3890.0, -480.0))
PYRO_FLARES = k("PYRO_FLARES", ((3640.0, -900.0), (3640.0, -450.0), (3640.0, 0.0), (3640.0, 450.0), (3640.0, 900.0)))
PLAT = k("PLAT", (450.0, 760.0, -830.0, -200.0))
CAT_Y0, CAT_Y1, GALLERY_X1 = k("CAT_Y0", -420.0), k("CAT_Y1", -260.0), k("GALLERY_X1", -600.0)
RAIL_H = k("RAIL_H", 100.0)
WARM, COOL = k("WARM", (1.0, 0.82, 0.60)), k("COOL", (0.55, 0.70, 1.0))

# --- §15.2 tokens (linear) --------------------------------------------------------------------------------------------
BONE = (0.5841, 0.5395, 0.4564)
PAPER = (0.8796, 0.8228, 0.7084)
MO_HIGH = (0.7379, 0.9047, 0.9131)
SIG_RED = (0.85, 0.03, 0.02)        # TakeColors::Red: pass 1's grade calibrates stencil 1 from this to #D3301F
SIG_GOLD = (1.0, 0.62, 0.08)        # TakeColors::Gold: ...and stencil 2 to #E9B949

# --- 1. materials (§15.9's table; linear base colour, roughness) ------------------------------------------------------
DRESS = "/Game/Take/Dress"
DASH_ON, DASH_OFF = 20.0, 10.0      # frames: 20 cm on, 10 cm off, from the eye outwards (§15.8)
HATCH_W = 20.0                      # tape: 20 cm stripes at 45 degrees
NEAR_FADE, NEAR_FADE_LEN = 200.0, 100.0   # frames (patterns 1, 2) drop out from 3 m to 2 m from the lens
LIT = {                             # instance: (base colour, roughness)
    "MI_Front_Street": ((0.025, 0.025, 0.027), 0.35),
    "MI_Front_Pave": ((0.11, 0.105, 0.10), 0.8),
    "MI_Front_Facade": ((0.28, 0.27, 0.255), 0.7),
    "MI_Front_Canopy": ((0.035, 0.033, 0.03), 0.5),
    "MI_Front_Glass": ((0.02, 0.025, 0.03), 0.1),
    "MI_Front_Marble": ((0.13, 0.125, 0.12), 0.25),
    "MI_Front_Wall": ((0.16, 0.15, 0.14), 0.85),
    "MI_Front_Damask": ((0.16, 0.15, 0.14), 0.85),      # the front walls: MI_Front_Wall with the damask (PATTERNS)
    "MI_Front_Runner": ((0.06, 0.035, 0.045), 1.0),     # the side-aisle runners: velvet with the runner pattern
    "MI_Front_Panel": ((0.06, 0.05, 0.045), 0.6),
    "MI_Front_Parquet": ((0.10, 0.09, 0.08), 0.45),
    "MI_Front_Trim": ((0.34, 0.33, 0.31), 0.35),      # pewter, never brass
    "MI_Front_Velvet": ((0.06, 0.035, 0.045), 1.0),
    "MI_Front_Linen": ((0.45, 0.43, 0.40), 0.9),
    "MI_Front_Plinth": ((0.40, 0.39, 0.37), 0.6),
    "MI_Front_Stage": ((0.13, 0.12, 0.11), 0.5),        # was 0.05: the heavies need a mid set to be darkest against
    "MI_Front_Case": ((0.15, 0.15, 0.15), 0.6),         # the display cases walling the rope aisle (were plinth 0.40)
    "MI_Front_Foliage": ((0.05, 0.07, 0.05), 0.9),
    "MI_Back_Ply": ((0.20, 0.17, 0.13), 0.9),
    "MI_Back_Batten": ((0.28, 0.24, 0.18), 0.9),
    "MI_Back_Concrete": ((0.12, 0.12, 0.12), 0.9),
    "MI_Back_Steel": ((0.08, 0.08, 0.085), 0.4),
    "MI_Back_Tape": ((0.65, 0.63, 0.58), 0.9),
    "MI_Back_Cable": ((0.012, 0.012, 0.012), 0.5),
    # §15.10, the stand-ins (placeholder people): three body values in rotation, heads, the band
    "MI_Person_A": ((0.20, 0.20, 0.20), 0.8),
    "MI_Person_B": ((0.28, 0.28, 0.28), 0.8),
    "MI_Person_C": ((0.35, 0.35, 0.35), 0.8),
    "MI_Person_Head": ((0.45, 0.43, 0.40), 0.8),
    "MI_Person_Band": ((0.04, 0.04, 0.04), 0.8),
    # not in §15.9's table: the canvases and the cars lose their colour too (they had red, blue and amber)
    "MI_Canvas_Light": ((0.45, 0.43, 0.40), 0.9),
    "MI_Canvas_Dark": ((0.12, 0.115, 0.11), 0.9),
    "MI_Car_Black": ((0.015, 0.015, 0.018), 0.3),
    "MI_Car_Grey": ((0.09, 0.095, 0.11), 0.3),
    "MI_Car_Silver": ((0.30, 0.30, 0.31), 0.3),
}
GLOW = {                            # unlit: colour x scale
    "MI_Back_Tube": (MO_HIGH, 6.0),       # fluorescent work tubes (no light of their own)
    "MI_Crew_Ring": (MO_HIGH, 2.0),       # the device rings (C++ sets the same colour on its own MID: CPP.md)
    "MI_Exit_Sign": (BONE, 1.5),
    "MI_Front_Sconce": (BONE, 1.0),       # sconces, the chandelier's spheres
    "MI_Front_Footlight": (BONE, 0.5),    # the dais's footlights: a low glow, not a row of paper dots
    "MI_Front_Screen": (BONE, 0.4),       # desk screens and phones in the fronts (were saturated blue)
}
SIGNAL = {                          # unlit, masked, stencil set by the mesh: colour, pattern
    "MI_Red": (SIG_RED, 1.0),             # frames: dashed
    "MI_Red_Hatch": (SIG_RED, 3.0),       # tape: hatched
    "MI_Red_Carpet": ((0.33, 0.03, 0.04), 0.0),   # the red carpet (it is red tape, STATUS step 2): darker than tape
    "MI_Gold": (SIG_GOLD, 0.0),           # solid, never patterned
}

# --- 2. surfaces: (built with, zones or None, label prefixes or None, becomes); the first match wins -----------------
# Zones: street, lobby, hall, dais, north, back (corridor and service rooms), east (vault corridor).
# Surface patterns in M_Dress: value patterns made of material nodes, no textures (Nov 14, Marwan: PBRgen skipped for
# its personal-use licence). Each multiplies the instance's colour by (1 + amp x q), q a zero-mean pattern in about
# -1..1 (SURFACE_HLSL subtracts each mask's measured area), so every surface keeps its average value and §15.11's
# floor numbers hold; rough moves the roughness by -rough x q. Kept low so the signal colours (marks, frames, tape)
# stay the loudest things in frame. scale: the repeat's size x this.
PATTERNS = {                        # instance: (pattern, amp, rough, scale)
    "MI_Front_Parquet": (1, 0.30, 0.10, 1.0),     # strips 12 x 90 cm along x, a value per strip, grain, seams
    "MI_Front_Marble": (2, 0.35, 0.05, 1.0),      # clouds, wandering light veins, faint 60 cm tile joints
    "MI_Front_Damask": (3, 0.10, 0.25, 1.0),      # half-drop medallions, 48 x 64 cm, glossier than the ground
    "MI_Front_Runner": (4, 0.35, 0.0, 1.0),       # a dark edge, a light band 10 cm in, diamonds (in the box's frame)
    "MI_Back_Ply": (5, 0.30, 0.0, 1.0),           # 122 x 244 cm sheets, a value each, vertical grain, seams
    "MI_Back_Concrete": (6, 0.25, 0.0, 1.0),      # mottling, speckle, a few stains, 3 m joints
}
PX_ANGLE = 0.0009                   # radians per pixel at 1080 lines (about 52 degrees tall): the patterns' LOD

FRONTS = ("street", "lobby", "hall", "dais", "north")
BACKS = ("back", "east")
SURFACES = [
    ("MI_Box_Wall", ("street",), None, "MI_Front_Facade"),
    ("MI_Box_Wall", ("lobby", "hall", "dais", "north"), None, "MI_Front_Damask"),
    ("MI_Box_Wall", None, None, "MI_Back_Ply"),
    ("MI_Box_Floor", ("lobby",), ("Floor",), "MI_Front_Marble"),
    ("MI_Box_Floor", BACKS, ("Floor",), "MI_Back_Concrete"),
    ("MI_Box_Floor", None, ("Floor",), "MI_Front_Parquet"),
    ("MI_Box_Floor", None, ("Stage_Trap",), "MI_Front_Stage"),
    ("MI_Box_Floor", None, ("Camera", "Screen", "PA_Stack"), "MI_Back_Cable"),
    ("MI_Box_Floor", None, ("Flight_Case",), "MI_Back_Steel"),
    ("MI_Box_Floor", None, None, "MI_Front_Panel"),                   # sign boards
    ("MI_Box_Deck", ("lobby",), None, "MI_Front_Marble"),             # the lobby's balcony
    ("MI_Box_Deck", None, ("Floor",), "MI_Back_Concrete"),
    ("MI_Box_Deck", None, None, "MI_Back_Steel"),                     # gallery, walkway, bridge, catwalk, platform
    ("MI_Box_Trim", BACKS, ("Back_Crate", "Prop_Crate", "Dock_Crate", "Crate"), "MI_Back_Batten"),
    ("MI_Box_Trim", BACKS, None, "MI_Back_Steel"),
    ("MI_Box_Trim", None, ("Gallery_Bench", "Bench"), "MI_Front_Velvet"),
    ("MI_Box_Trim", None, ("Lots_Table", "Table"), "MI_Front_Linen"),
    ("MI_Box_Trim", None, ("Plinth", "Podium", "Case_Plinth"), "MI_Front_Plinth"),
    ("MI_Box_Trim", None, ("Lobby_Desk", "Office_Desk", "Security_Desk", "Director_Desk", "Vault_Desk", "Bar",
                           "Phone_Desk", "Chair", "Stool", "Planter", "Easel"), "MI_Front_Panel"),
    ("MI_Box_Trim", None, ("Column",), "MI_Front_Wall"),
    ("MI_Box_Trim", None, ("Truss", "Catwalk_Leg", "Bridge_Leg", "Walkway_Leg", "Gallery_Leg", "Shutter_", "Tripod",
                           "Broadcast_Riser", "PA_Grille", "Lorry", "Crate"), "MI_Back_Steel"),
    ("MI_Box_Trim", ("hall", "dais"), ("Rail", "Post"), "MI_Back_Steel"),
    ("MI_Box_Trim", ("street",), ("Terrace_Post",), "MI_Front_Trim"),
    ("MI_Box_Trim", ("street",), ("Terrace", "Bollard", "Lamp_Post", "Lamp_Arm"), "MI_Front_Canopy"),
    ("MI_Box_Trim", None, ("Grand_Stair", "Fire_Stair"), "MI_Front_Marble"),   # the stair was pewter (77)
    ("MI_Box_Trim", None, None, "MI_Front_Trim"),
    ("MI_Box_Gold", None, ("Bottle", "Bar_Bottles"), "MI_Front_Glass"),
    ("MI_Box_Gold", None, None, "MI_Front_Trim"),     # stanchions, frames, glasses, instruments, fascia: never brass
    ("MI_Box_Rope", None, ("Bottle",), "MI_Front_Glass"),
    ("MI_Box_Rope", None, None, "MI_Front_Velvet"),   # the rope, both rope walls (red was a false danger)
    ("MI_Box_Plant", None, ("Bottle", "Bar_Bottles"), "MI_Front_Glass"),
    ("MI_Box_Plant", None, None, "MI_Front_Foliage"),
    ("MI_Box_Carpet", None, None, "MI_Front_Velvet"), # the column banners (the carpet itself is a signal, below)
    ("MI_Box_Sofa", None, None, "MI_Front_Velvet"),
    ("MI_Box_Dais", None, None, "MI_Front_Stage"),
    ("MI_Box_Case", BACKS, None, "MI_Back_Steel"),
    ("MI_Box_Case", None, ("Table_Foot", "Bar_Shelf"), "MI_Front_Panel"),
    ("MI_Box_Case", None, ("Cases_",), "MI_Front_Case"),
    ("MI_Box_Case", None, ("Lot",), "MI_Front_Linen"),
    ("MI_Box_Case", None, None, "MI_Front_Plinth"),   # display cases, the scanner's arch
    ("MI_Box_Door", BACKS, None, "MI_Back_Steel"),
    ("MI_Box_Door", None, None, "MI_Front_Canopy"),
    ("MI_Box_Window", None, None, "MI_Front_Glass"),
    ("MI_Box_Street", None, None, "MI_Front_Street"),
    ("MI_Box_Pave", None, None, "MI_Front_Pave"),
    ("MI_Box_Trunk", None, None, "MI_Front_Panel"),
    ("MI_Box_Tire", None, None, "MI_Back_Cable"),
    ("MI_Box_Red", None, None, "MI_Back_Cable"),      # extinguishers and alarm points: red that meant nothing
    ("MI_Box_Head", None, ("Guest_Head", "Band_Head"), "MI_Person_Head"),
    ("MI_Box_Head", None, None, "MI_Front_Linen"),
    ("MI_Box_Glow", BACKS, None, "MI_Back_Tube"),
    ("MI_Box_Glow", None, None, "MI_Front_Screen"),
    ("MI_Box_Exit", None, None, "MI_Exit_Sign"),
    ("MI_Guest_", None, ("Band",), "MI_Person_Band"),
    ("MI_Guest_", None, None, "@person"),
    ("MI_Art_", None, None, "@canvas"),
    ("MI_Car_", None, None, "@car"),
    ("MI_Signal_Red", None, ("Carpet",), "MI_Red_Carpet"),
]
ROTATE = {"@person": ("MI_Person_A", "MI_Person_B", "MI_Person_C"),
          "@canvas": ("MI_Canvas_Light", "MI_Canvas_Dark"),
          "@car": ("MI_Car_Black", "MI_Car_Grey", "MI_Car_Silver")}
SKIP_FOLDERS = ("Set/Blockers",)

# --- 3. dressing (cm) ----------------------------------------------------------------------------------------------------
KEEP_OUT = 120.0            # no dressing within this of a mark, watcher, device, crew post or character
TAPE_MARGIN = 15.0          # ...nor within this of a red tape strip
KERB_W = 15.0
QUEUE_POSTS = (400.0, 880.0, 120.0)        # stanchions along the queue: y from, to, step (road side, x PAVE_X0 + 12;
                                           # 50 cm clear of the bollards at y 350 and 700)
PRESS_WALL = (-4000.0, -3600.0, -440.0)    # x0, x1, y: a 4 x 2.4 m linen flat behind the press
BAY_Y = FRONT_DOOR_HALF + 70.0             # the potted bay trees, either side of the doors
WAINSCOT_H, WAINSCOT_PROUD = 90.0, 4.0
CORNICE_Z, CORNICE_PROUD = 360.0, 6.0
CHANDELIER = ((VOID_X0 + VOID_X1) / 2.0, 0.0, 790.0, 70.0, 26.0)   # x, y, z, ring radius, sphere size
PILASTER_STEP, PILASTER_W, PILASTER_D = 600.0, 60.0, 30.0
PILASTER_TOP = F2 - SLAB                   # under the walkway's slab
SCONCE_Z = 240.0
RUNNER_Y, RUNNER_W = 1150.0, 120.0         # velvet runners along the side aisles (never the centre aisle)
RUNNER_X = (HX0 + 100.0, k("ROPE_X", 2400.0) - 100.0)
FOOT_STEP = 40.0                           # footlights on the dais's front face
CHARGE_DX = 30.0                           # the pyro charges stand this far upstage of the flare points
BATTEN_STEP, BRACE_OUT, BRACE_TOP = 240.0, 70.0, 260.0
TUBE_STEP, TUBE_Z, TUBE_L, TUBE_H = 600.0, 220.0, 120.0, 8.0
ROAD_CASES = (400.0, 1150.0, 2250.0, 3850.0)   # x of the stacks against the corridor's back wall
TRUSS_Z = F2 + 420.0
HOUSING_STEP = 200.0
BACK_SIGNS = (("PROPS", 1700.0, 290.0), ("STAGE RIGHT", 3880.0, 300.0))   # text, x, z on the corridor's back wall

# --- 4. lights -------------------------------------------------------------------------------------------------------------
RETUNE = {   # label prefix: properties (intensities stay in build_auction.py: KEY_CD, RIM_CD, EXPOSURE_EV)
    "Key_Scanner": dict(cast_shadows=True, volumetric_scattering_intensity=1.5),
    "Key_Stair": dict(cast_shadows=True, volumetric_scattering_intensity=0.5),
    "Key_Aisle": dict(cast_shadows=False, volumetric_scattering_intensity=2.0),
    "Key_Tape": dict(cast_shadows=False, volumetric_scattering_intensity=2.0),
    "Key_Rope": dict(cast_shadows=True, volumetric_scattering_intensity=2.0),
    "Key_Podium": dict(cast_shadows=True, volumetric_scattering_intensity=1.2),
    "Key_Reel": dict(cast_shadows=True, volumetric_scattering_intensity=0.5,   # gold is MI_Gold's, not the light's:
                     light_color=unreal.Color(r=255, g=242, b=230, a=255)),   # (1.0, 0.95, 0.90), was GOLDL   # its gold haze washed the end wall amber at the alarm
    "Fill_Dais": dict(cast_shadows=False),
    "Rim_Hall": dict(cast_shadows=False, volumetric_scattering_intensity=2.5),
}
WORK_HAZE = 0.0             # backstage and catwalk work lamps: no haze (video is clear)
BACK_DAIS = dict(x=VANCE[0] + 360.0, y=VANCE[1], z=DAIS_H + 400.0, yaw=180.0, pitch=-15.0,
                 cd=2000.0, cone=40.0, radius=3000.0, haze=2.0)   # cone as build_auction's spot(): the outer angle
# Julian's own key (STATUS "Look": Julian is key-lit): a point light riding with him, behind and above (the follow
# camera's side), on lighting channel 1 only, which only his mesh joins: it lights nothing else and casts no shadow.
# The house lights at half over the seats (§15.11 check 1: floor between bench rows 45-70 while the aisle pools stay
# 95-130): two wide top-down spots, no shadows, no haze. The night skylight captures a black sky, so it fills nothing.
# A cool wash from across the road over the queue and up the facade (the opening frame was 45% black).
FACADE_WASH = dict(x=PAVE_X0 - 1300.0, y=300.0, z=300.0, yaw=0.0, pitch=2.0, cd=1200.0, cone=40.0, radius=3500.0)
HOUSE_FILL = dict(cd=1200.0, outer=32.0, inner=20.0, radius=3000.0, x=100.0, ys=(620.0, -620.0))
# It rides on the follow camera's boom end (socket SpringEndpoint, boom_dz above it), so it always lights the side the
# camera sees; if the boom is not found it hangs off Julian at dx, dy, dz as before.
JULIAN_KEY = dict(dx=-220.0, dy=60.0, dz=230.0, boom_dz=60.0, cd=240.0, radius=700.0)

# --- 5. tints (§15.10, linear) -----------------------------------------------------------------------------------------
# Marks the default follow camera sees at a grazing angle past a stanchion or across the lobby: bigger plates (visual
# only, AMark::PlateScale; standing on the mark is the director's test, not the plate).
MARK_PLATES = {"Mark_Scanner": 2.0, "Mark_Desk": 2.0}
TINTS = {"Julian": (0.80, 0.78, 0.72), "Maya": (0.38, 0.40, 0.42), "Vex": (0.16, 0.17, 0.18)}
BODY_TINTS = {"Bodyguard": (0.012, 0.012, 0.012), "Vance": (0.03, 0.028, 0.026), "Drone": (0.02, 0.02, 0.02)}
HEAVY = (0.02, 0.02, 0.022)          # every other watcher (Watcher.h's default after CPP.md; set here too)
DEVICE_BODY = (0.30, 0.31, 0.33)     # crew kit: light steel, no hue (was TakeColors::WorkBlue)
BANNER_BODY = (0.15, 0.15, 0.16)     # the banner over the rope: it was the largest light shape after Julian
RING_IDLE = (0.05, 0.09, 0.13)       # device rings out of reach: mo.shadow side, so only the ring in reach lights
CURTAIN = (0.06, 0.035, 0.045)       # the house curtain in velvet (was red: a false danger)
FLARE = (1.0, 0.92, 0.80)            # the pyro's flare light: warm white, not gold

# --- QA look -------------------------------------------------------------------------------------------------------------
# Note 8 (Nov 14): colour is allowed. A muted 1940s palette laid over the greys above, each at about the luminance of
# the grey it replaces (Y = 0.2126 R + 0.7152 G + 0.0722 B), so the value hierarchy and §15.11's floor numbers hold:
# Julian (Y 0.78) the lightest, the heavies near black. By zone: the lobby bottle green over cream marble, the hall and
# the dais burgundy over oak, the north rooms navy; walnut furniture, burgundy and bottle-green velvet, brass and aged
# gilt trim (gold is no longer Julian's), cream linen and plinths, warm tungsten practicals and keys. PALETTE False
# keeps the greys. Instances only, no master recompiled (M_SignalPattern's cache untouched):
#     DRESS_ONLY = ("palette", "zones", "lights", "tints")   in a wrapper that execs this file
PALETTE = True
if PALETTE:
    LIT.update({
        "MI_Front_Street": ((0.022, 0.025, 0.030), 0.35),        # wet asphalt, a touch cool
        "MI_Front_Pave": ((0.118, 0.105, 0.090), 0.8),           # warm grey flags
        "MI_Front_Facade": ((0.32, 0.27, 0.20), 0.7),            # limestone
        "MI_Front_Canopy": ((0.020, 0.036, 0.027), 0.5),         # bottle-green ironwork: canopy, lamp posts, doors
        "MI_Front_Glass": ((0.018, 0.026, 0.032), 0.1),
        "MI_Front_Marble": ((0.150, 0.124, 0.098), 0.25),        # cream marble, veined (PATTERNS 2)
        "MI_Front_Wall": ((0.19, 0.148, 0.108), 0.85),           # cream plaster: the columns
        "MI_Front_Damask": ((0.26, 0.095, 0.09), 0.85),          # the hall and the dais: burgundy damask
        "MI_Front_Damask_Lobby": ((0.075, 0.175, 0.11), 0.85),   # the lobby: bottle-green damask
        "MI_Front_Damask_North": ((0.08, 0.105, 0.19), 0.85),    # the north rooms: navy damask
        "MI_Front_Runner": ((0.10, 0.028, 0.032), 1.0),          # burgundy runners
        "MI_Front_Panel": ((0.078, 0.046, 0.028), 0.55),         # walnut: desks, the bar, chairs, stools, easels
        "MI_Front_Parquet": ((0.135, 0.086, 0.050), 0.45),       # oak
        "MI_Front_Trim": ((0.37, 0.31, 0.20), 0.35),             # aged gilt over ivory: cornice, pilasters, dais lip
        "MI_Front_Brass": ((0.40, 0.29, 0.12), 0.3),             # brass: stanchions, frames, instruments, fascia
        "MI_Front_Velvet": ((0.105, 0.026, 0.032), 1.0),         # burgundy velvet: the rope, the banners
        "MI_Front_Velvet_Green": ((0.024, 0.058, 0.038), 1.0),   # bottle-green velvet: benches, sofas
        "MI_Front_Linen": ((0.48, 0.43, 0.34), 0.9),             # cream linen
        "MI_Front_Plinth": ((0.43, 0.39, 0.32), 0.6),            # cream marble plinths and cases (PATTERNS 2)
        "MI_Front_Stage": ((0.16, 0.115, 0.075), 0.5),           # dark oak stage boards (PATTERNS 1, wider)
        "MI_Front_Case": ((0.20, 0.14, 0.095), 0.5),             # walnut cases along the rope aisle
        "MI_Front_Foliage": ((0.04, 0.078, 0.035), 0.9),
        "MI_Back_Ply": ((0.24, 0.165, 0.10), 0.9),               # raw plywood
        "MI_Back_Batten": ((0.32, 0.235, 0.145), 0.9),           # pine
        "MI_Back_Concrete": ((0.118, 0.12, 0.124), 0.9),
        "MI_Back_Steel": ((0.074, 0.08, 0.092), 0.4),
        "MI_Back_Tape": ((0.66, 0.63, 0.55), 0.9),
        "MI_Person_A": ((0.16, 0.19, 0.27), 0.8),                # navy
        "MI_Person_B": ((0.37, 0.26, 0.17), 0.8),                # brown tweed
        "MI_Person_C": ((0.33, 0.36, 0.30), 0.8),                # dove grey-green
        "MI_Person_Head": ((0.56, 0.40, 0.30), 0.8),
        "MI_Person_Band": ((0.045, 0.038, 0.035), 0.8),
        "MI_Canvas_Light": ((0.52, 0.42, 0.26), 0.9),            # ochre landscapes
        "MI_Canvas_Dark": ((0.15, 0.11, 0.065), 0.9),            # old-master browns
        "MI_Car_Grey": ((0.05, 0.11, 0.075), 0.3),               # bottle green
        "MI_Car_Silver": ((0.34, 0.30, 0.235), 0.3),             # cream
    })
    TUNGSTEN = (0.80, 0.50, 0.24)                                # Y 0.545, as BONE was
    GLOW.update({
        "MI_Front_Sconce": (TUNGSTEN, 1.0),
        "MI_Front_Footlight": (TUNGSTEN, 0.5),
        "MI_Front_Screen": ((0.72, 0.54, 0.34), 0.4),
    })
    PATTERNS.update({
        "MI_Front_Damask_Lobby": PATTERNS["MI_Front_Damask"],
        "MI_Front_Damask_North": PATTERNS["MI_Front_Damask"],
        "MI_Front_Plinth": (2, 0.20, 0.05, 0.6),
        "MI_Front_Stage": (1, 0.22, 0.08, 1.6),
    })
    SURFACES[0:0] = [                                            # first match wins: these go in front
        ("MI_Box_Wall", ("lobby",), None, "MI_Front_Damask_Lobby"),
        ("MI_Box_Wall", ("north",), None, "MI_Front_Damask_North"),
        ("MI_Box_Trim", FRONTS, ("Gallery_Bench", "Bench"), "MI_Front_Velvet_Green"),
        ("MI_Box_Sofa", None, None, "MI_Front_Velvet_Green"),
        ("MI_Box_Gold", None, ("Bottle", "Bar_Bottles"), "MI_Front_Glass"),
        ("MI_Box_Gold", None, None, "MI_Front_Brass"),
    ]
    WARM_KEY = unreal.Color(r=255, g=219, b=179, a=255)        # (1.0, 0.86, 0.70): build_auction's WARM, a little warmer
    for _kk in ("Key_Scanner", "Key_Stair", "Key_Aisle", "Key_Rope", "Key_Tape", "Key_Podium", "Fill_Dais"):
        RETUNE[_kk] = dict(RETUNE.get(_kk, {}), light_color=WARM_KEY)
    COOL = (0.78, 0.86, 1.0)                                     # the rims, Back_Dais, the facade wash: a paler blue
    RETUNE["Rim_Hall"] = dict(RETUNE.get("Rim_Hall", {}),        # (0.55, 0.70, 1.0) turned the hall and its haze
                              light_color=unreal.Color(r=199, g=219, b=255, a=255))   # into a blue night
    TINTS.update({"Julian": (0.82, 0.77, 0.66),                 # an ivory dinner jacket, still the lightest (Y 0.77)
                  "Maya": (0.34, 0.40, 0.46),                   # slate blue
                  "Vex": (0.19, 0.165, 0.13)})                  # olive brown
    BANNER_BODY = (0.27, 0.10, 0.09)                             # burgundy, darker than before (Y 0.135)
    CURTAIN = (0.105, 0.026, 0.032)                              # burgundy velvet
# --- end QA look

# =========================================================================================================================
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
CUBE = eal.load_asset("/Engine/BasicShapes/Cube")
SPHERE = eal.load_asset("/Engine/BasicShapes/Sphere")
CYL = eal.load_asset("/Engine/BasicShapes/Cylinder")
REPORT = []


def step(name):
    return ONLY is None or name in ONLY


def warn(s):
    unreal.log_warning("dress: " + s)


WARNED = set()


def try_set(obj, **props):
    for kk, v in props.items():
        try:
            obj.set_editor_property(kk, v)
        except Exception as e:
            key = (obj.get_class().get_name(), kk)
            if key not in WARNED:                      # once per class and property, not once per piece
                WARNED.add(key)
                warn("%s.%s: %s" % (key[0], kk, e))


def enum_member(enum_cls, *names):
    for n in names:
        if hasattr(enum_cls, n):
            return getattr(enum_cls, n)
    raise RuntimeError("no %s in %s" % (names, enum_cls))


def lc(rgb, a=1.0):
    return unreal.LinearColor(rgb[0], rgb[1], rgb[2], a)


def fc(rgb):
    """Linear 0-1 to an sRGB FColor (TextRender colours are FColor; the text shows the hex of §15.2)."""
    s =[int(round(255.0 * (12.92 * v if v <= 0.0031308 else 1.055 * v ** (1 / 2.4) - 0.055))) for v in rgb]
    return unreal.Color(r=max(0, min(255, s[0])), g=max(0, min(255, s[1])), b=max(0, min(255, s[2])), a=255)


# --- 1. materials ---------------------------------------------------------------------------------------------------------
def fresh_material(name):
    path = DRESS + "/" + name
    if eal.does_asset_exist(path):
        m = eal.load_asset(path)
        mel.delete_all_material_expressions(m)
    else:
        m = at.create_asset(name, DRESS, unreal.Material, unreal.MaterialFactoryNew())
    return m


def pv(m, name, rgb, x, y):
    e = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", lc(rgb))
    return e


def ps(m, name, v, x, y):
    e = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", float(v))
    return e


def link(a, out, b, ins):
    for i in (ins if isinstance(ins, tuple) else (ins,)):
        if mel.connect_material_expressions(a, out, b, i):
            return
    raise RuntimeError("could not link %s -> %s.%s" % (a.get_name(), b.get_name(), ins))


def expr(m, names, x, y):
    for n in names:
        cls = getattr(unreal, n, None)
        if cls is not None:
            return mel.create_material_expression(m, cls, x, y)
    raise RuntimeError("no material expression among %s" % (names,))


def save(path):
    eal.save_asset(path, only_if_is_dirty=False)


PATTERN_HLSL = r"""
float period = max(DashOn + DashOff, 1.0);
float m = 1.0;
if (Pattern > 0.5 && Pattern < 1.5)
{
    // 1: dashes along a line (a cube stretched on X), from its start: for a frame line, the eye.
    float along = (LP.x + 50.0) * length(AX);
    m = (frac(along / period) * period < DashOn) ? 1.0 : 0.0;
    m *= saturate((Depth - NearFade) / max(NearFadeLen, 1.0));
}
else if (Pattern > 1.5 && Pattern < 2.5)
{
    // 2: bands outwards from the watcher (its actor), on the floor: a frame's fill.
    float d = length(Rel.xy);
    m = (frac(d / period) * period < DashOn) ? 1.0 : 0.0;
    m *= saturate((Depth - NearFade) / max(NearFadeLen, 1.0));
}
else if (Pattern > 2.5)
{
    // 3: hatch at 45 degrees in world X-Y: red stripes, the gaps show the floor (graded like the set).
    float u = (WP.x + WP.y) * 0.70710678;
    m = (frac(u / max(2.0 * HatchWidth, 1.0)) < 0.5) ? 1.0 : 0.0;
}
return m;
"""


SURFACE_HLSL = r"""
struct SurfaceNoise
{
    float h(float2 p)
    {
        float3 p3 = frac(float3(p.x, p.y, p.x) * 0.1031);
        p3 += dot(p3, p3.yzx + 33.33);
        return frac((p3.x + p3.y) * p3.z);
    }
    float vn(float2 p)
    {
        float2 i = floor(p);
        float2 f = frac(p);
        f = f * f * (3.0 - 2.0 * f);
        return lerp(lerp(h(i), h(i + float2(1.0, 0.0)), f.x), lerp(h(i + float2(0.0, 1.0)), h(i + float2(1.0, 1.0)), f.x), f.y);
    }
    float fbm(float2 p)
    {
        float s = 0.0;
        float a = 0.5;
        for (int k = 0; k < 4; k++)
        {
            s += a * vn(p);
            p = p * 2.03 + float2(17.0, 31.0);
            a *= 0.5;
        }
        return s / 0.9375;
    }
    float dist(float v, float period) { return abs(frac(v / period + 0.5) - 0.5) * period; }
    // A line w cm wide, d cm from its middle: never thinner than a pixel (fw cm), dimmed as it widens, so its
    // average stays w / period at any distance (no shimmer, no drift in value).
    float stroke(float d, float w, float fw)
    {
        float ww = max(w, fw);
        return (1.0 - smoothstep(0.5 * ww, 0.5 * ww + fw, d)) * (w / ww);
    }
};
SurfaceNoise S;
if (Pattern < 0.5)
    return 0.0;
float3 an = abs(N);
float2 uv = (an.z >= an.x && an.z >= an.y) ? WP.xy : ((an.x >= an.y) ? WP.yz : WP.xz);   // floors x, y; walls along, z
float sc = max(Scale, 0.01);
uv /= sc;
// cm per pixel here, from depth and the pixel's angle, longer where the surface is seen edge-on (no ddx: some passes
// have no derivatives)
float fw = max(Depth * PxAngle / max(abs(dot(N, V)), 0.2) / sc, 0.02);
float fine = saturate(1.5 - fw);
float q = 0.0;
if (Pattern < 1.5)
{
    // 1 parquet: strips 12 x 90 cm along x, staggered; a value per strip, grain along it, seams
    float row = floor(uv.y / 12.0);
    float x = uv.x + S.h(float2(row, 7.0)) * 90.0;
    float id = S.h(float2(row, floor(x / 90.0))) - 0.5;
    float grain = (S.vn(float2(x * 0.15, uv.y * 0.9 + row * 13.0)) - 0.5) * fine;
    float seam = max(S.stroke(S.dist(uv.y, 12.0), 0.8, fw), S.stroke(S.dist(x, 90.0), 0.8, fw));
    q = id * 1.1 + grain * 0.6 - (seam - 0.0787) * 0.9;
}
else if (Pattern < 2.5)
{
    // 2 marble: soft clouds, wandering light veins, faint 60 cm tile joints
    float cloud = S.fbm(uv / 140.0) - 0.5;
    float w1 = uv.x / 110.0 + uv.y / 170.0 + S.fbm(uv / 70.0 + 5.0) * 2.5;
    float w2 = uv.x / 37.0 - uv.y / 53.0 + S.fbm(uv / 30.0) * 1.5;
    float vein = S.stroke(abs(frac(w1) - 0.5) * 80.0, 1.2, fw);
    float vein2 = S.stroke(abs(frac(w2) - 0.5) * 25.0, 0.5, fw);
    float tile = max(S.stroke(S.dist(uv.x, 60.0), 0.4, fw), S.stroke(S.dist(uv.y, 60.0), 0.4, fw));
    q = cloud * 1.4 + (vein - 0.0154) * 1.2 + (vein2 - 0.0216) * 0.6 - (tile - 0.0147) * 0.5;
}
else if (Pattern < 3.5)
{
    // 3 damask: half-drop medallions, 48 x 64 cm, tone on tone (the roughness carries most of it)
    float col = floor(uv.x / 48.0);
    float2 c = float2(frac(uv.x / 48.0), frac((uv.y + fmod(abs(col), 2.0) * 32.0) / 64.0)) - 0.5;
    c.x = abs(c.x);
    float r = length(c * float2(2.0, 1.45));
    float ang = atan2(c.y, c.x);
    float petal = r - 0.07 * cos(ang * 6.0) - 0.05 * cos(ang * 2.0);
    float aa = fw / 48.0 + 0.01;
    float medal = 1.0 - smoothstep(0.42, 0.42 + aa, petal);
    float eye = 1.0 - smoothstep(0.12, 0.12 + aa, r);
    float dc = abs(c.x - 0.5) * 2.0 + abs(abs(c.y) - 0.5) * 1.4;
    float corner = 1.0 - smoothstep(0.22, 0.22 + aa, dc);
    float motif = saturate(medal - eye * 0.7 + corner);
    q = (motif - 0.2169) * 2.0 * saturate(2.0 - fw / 4.0);
}
else if (Pattern < 4.5)
{
    // 4 runner, in the box's own frame: a dark edge, a light band 10 cm in, diamonds down the middle
    float2 lc = float2((LP.x + 50.0) * length(AX), (LP.y + 50.0) * length(AY));
    float2 sz = float2(100.0 * length(AX), 100.0 * length(AY));
    if (sz.y > sz.x)
    {
        lc = lc.yx;
        sz = sz.yx;
    }
    float b = min(lc.y, sz.y - lc.y);
    float border = S.stroke(abs(b - 10.0), 6.0, fw);
    float edge = 1.0 - smoothstep(3.0, 3.0 + fw, b);
    float2 dd = abs(frac(float2(lc.x / 44.0, (lc.y - sz.y * 0.5) / 30.0 + 0.5)) - 0.5) * 2.0;
    float field = (1.0 - smoothstep(0.55, 0.55 + fw / 15.0 + 0.01, dd.x + dd.y)) * step(18.0, b);
    q = (field - 0.1156) * 0.9 + (border - 0.1006) * 1.2 - (edge - 0.0507) * 0.8;
}
else if (Pattern < 5.5)
{
    // 5 plywood flats: 122 x 244 cm sheets with a value each, vertical grain that wanders, seams
    float sheet = S.h(float2(floor(uv.x / 122.0), floor(uv.y / 244.0)) + 3.0) - 0.5;
    float figure = (S.vn(float2(uv.x / 1.6 + S.fbm(uv / 45.0) * 7.0, uv.y / 70.0)) - 0.5) * saturate(2.5 - fw);
    float streak = (S.vn(float2(uv.x / 0.7, uv.y / 25.0)) - 0.5) * fine;
    float seam = max(S.stroke(S.dist(uv.x, 122.0), 0.6, fw), S.stroke(S.dist(uv.y, 244.0), 0.6, fw));
    q = sheet * 0.7 + figure * 0.9 + streak * 0.4 - (seam - 0.0080) * 0.9;
}
else
{
    // 6 concrete: mottling, speckle, a few stains, 3 m joints
    float mott = S.fbm(uv / 90.0) - 0.5;
    float speck = (S.vn(uv / 2.5) - 0.5) * fine;
    float stain = smoothstep(0.62, 0.80, S.fbm(uv / 260.0 + 11.0));
    float joint = max(S.stroke(S.dist(uv.x, 300.0), 1.0, fw), S.stroke(S.dist(uv.y, 300.0), 1.0, fw));
    q = mott * 1.3 + speck * 0.4 - (stain - 0.0662) * 0.7 - (joint - 0.0072) * 0.8;
}
return q;
"""


def surface_pattern(m):
    """M_Dress's SurfacePattern node (SURFACE_HLSL) and its inputs; returns the node (its output is q)."""
    cu = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -700, 760)
    cu.set_editor_property("code", SURFACE_HLSL)
    cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    cu.set_editor_property("description", "SurfacePattern")
    names = ["WP", "N", "V", "Depth", "LP", "AX", "AY", "Pattern", "Scale", "PxAngle"]
    inputs = []
    for n in names:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        inputs.append(ci)
    cu.set_editor_property("inputs", inputs)
    wp = mel.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -1400, 760)
    link(wp, "", cu, "WP")
    link(expr(m, ("MaterialExpressionVertexNormalWS",), -1400, 840), "", cu, "N")
    link(expr(m, ("MaterialExpressionCameraVectorWS",), -1400, 900), "", cu, "V")
    link(mel.create_material_expression(m, unreal.MaterialExpressionPixelDepth, -1400, 960), "", cu, "Depth")
    if hasattr(unreal, "MaterialExpressionLocalPosition"):           # as M_SignalPattern does
        lp = mel.create_material_expression(m, unreal.MaterialExpressionLocalPosition, -1100, 1020)
    else:
        lp = mel.create_material_expression(m, unreal.MaterialExpressionTransformPosition, -1100, 1020)
        try_set(lp, transform_source_type=enum_member(unreal.MaterialPositionTransformSource,
                                                      "TRANSFORMPOSSOURCE_WORLD", "WORLD"),
                transform_type=enum_member(unreal.MaterialPositionTransformSource,
                                           "TRANSFORMPOSSOURCE_LOCAL", "LOCAL"))
        link(wp, "", lp, ("", "Input"))
    link(lp, "", cu, "LP")
    for i, (n, v) in enumerate((("AX", (1.0, 0.0, 0.0)), ("AY", (0.0, 1.0, 0.0)))):
        k3 = mel.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -1400, 1100 + i * 120)
        k3.set_editor_property("constant", unreal.LinearColor(v[0], v[1], v[2], 0.0))
        tv = mel.create_material_expression(m, unreal.MaterialExpressionTransform, -1100, 1100 + i * 120)
        try_set(tv, transform_source_type=enum_member(unreal.MaterialVectorCoordTransformSource,
                                                      "TRANSFORMSOURCE_LOCAL", "LOCAL"),
                transform_type=enum_member(unreal.MaterialVectorCoordTransform, "TRANSFORM_WORLD", "WORLD"))
        link(k3, "", tv, ("", "Input"))
        link(tv, "", cu, n)
    link(ps(m, "Pattern", 0.0, -1100, 1360), "", cu, "Pattern")
    link(ps(m, "PatternScale", 1.0, -1100, 1420), "", cu, "Scale")
    link(ps(m, "PxAngle", PX_ANGLE, -1100, 1480), "", cu, "PxAngle")
    return cu


def build_materials():
    # M_Dress: the set (lit).
    m = fresh_material("M_Dress")
    col = pv(m, "Color", (0.18, 0.18, 0.18), -700, -200)
    rough = ps(m, "Roughness", 0.7, -700, 0)
    spec = ps(m, "Specular", 0.5, -700, 80)
    emc = pv(m, "Emissive", (0.0, 0.0, 0.0), -700, 160)
    ems = ps(m, "EmissiveScale", 0.0, -700, 340)
    mul = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -350, 200)
    link(emc, "", mul, "A")
    link(ems, "", mul, "B")
    # The surface patterns (PATTERNS): base colour = Color x (1 + PatternAmp x q), roughness = Roughness -
    # PatternRough x q. Pattern 0 (every other instance) returns q = 0 first thing.
    q = surface_pattern(m)
    aq = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -450, 560)
    link(ps(m, "PatternAmp", 0.0, -700, 560), "", aq, "A")
    link(q, "", aq, "B")
    one_plus = mel.create_material_expression(m, unreal.MaterialExpressionAdd, -300, 560)
    link(aq, "", one_plus, "A")
    try_set(one_plus, const_b=1.0)
    patterned = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -150, -200)
    link(col, "", patterned, "A")
    link(one_plus, "", patterned, "B")
    rq = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -450, 640)
    link(ps(m, "PatternRough", 0.0, -700, 640), "", rq, "A")
    link(q, "", rq, "B")
    rsub = mel.create_material_expression(m, unreal.MaterialExpressionSubtract, -300, 40)
    link(rough, "", rsub, "A")
    link(rq, "", rsub, "B")
    rsat = mel.create_material_expression(m, unreal.MaterialExpressionSaturate, -150, 40)
    link(rsub, "", rsat, ("", "Input"))
    mel.connect_material_property(patterned, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(rsat, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m)
    save(DRESS + "/M_Dress")

    # M_DressGlow: practicals (unlit).
    g = fresh_material("M_DressGlow")
    g.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    gc = pv(g, "Color", BONE, -500, -100)
    gs = ps(g, "Scale", 1.0, -500, 100)
    gm = mel.create_material_expression(g, unreal.MaterialExpressionMultiply, -250, 0)
    link(gc, "", gm, "A")
    link(gs, "", gm, "B")
    mel.connect_material_property(gm, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(g)
    save(DRESS + "/M_DressGlow")

    # M_SignalPattern: what carries meaning (unlit like pass 1's M_TakeSignal, masked for the patterns).
    s = fresh_material("M_SignalPattern")
    s.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    s.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    # Usage flags set here, never at runtime: a missing flag set on the editor's first frame crashed the GPU (pass 3).
    try_set(s, opacity_mask_clip_value=0.5, used_with_instanced_static_meshes=True)
    sc = pv(s, "Color", SIG_RED, -900, -300)
    mel.connect_material_property(sc, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    cu = mel.create_material_expression(s, unreal.MaterialExpressionCustom, -250, 100)
    cu.set_editor_property("code", PATTERN_HLSL)
    cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    cu.set_editor_property("description", "SignalPattern")
    names = ["LP", "AX", "Rel", "WP", "Depth", "Pattern", "DashOn", "DashOff", "HatchWidth", "NearFade", "NearFadeLen"]
    inputs = []
    for n in names:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        inputs.append(ci)
    cu.set_editor_property("inputs", inputs)
    wp = mel.create_material_expression(s, unreal.MaterialExpressionWorldPosition, -1200, 0)
    # LP: the mesh-space position (the engine cube is -50..50 on each axis).
    lp = None
    if hasattr(unreal, "MaterialExpressionLocalPosition"):
        lp = mel.create_material_expression(s, unreal.MaterialExpressionLocalPosition, -900, -100)
    else:
        lp = mel.create_material_expression(s, unreal.MaterialExpressionTransformPosition, -900, -100)
        try_set(lp, transform_source_type=enum_member(unreal.MaterialPositionTransformSource,
                                                      "TRANSFORMPOSSOURCE_WORLD", "WORLD"),
                transform_type=enum_member(unreal.MaterialPositionTransformSource,
                                           "TRANSFORMPOSSOURCE_LOCAL", "LOCAL"))
        link(wp, "", lp, ("", "Input"))
    # AX: the mesh's X axis in world space; its length is the line's world length per mesh unit.
    one = mel.create_material_expression(s, unreal.MaterialExpressionConstant3Vector, -1200, 120)
    one.set_editor_property("constant", unreal.LinearColor(1.0, 0.0, 0.0, 0.0))
    ax = mel.create_material_expression(s, unreal.MaterialExpressionTransform, -900, 120)
    try_set(ax, transform_source_type=enum_member(unreal.MaterialVectorCoordTransformSource,
                                                  "TRANSFORMSOURCE_LOCAL", "LOCAL"),
            transform_type=enum_member(unreal.MaterialVectorCoordTransform, "TRANSFORM_WORLD", "WORLD"))
    link(one, "", ax, ("", "Input"))
    # Rel: the pixel relative to the actor (bands move with the watcher, so they never crawl).
    ap = expr(s, ("MaterialExpressionActorPositionWS", "MaterialExpressionObjectPositionWS"), -1200, 260)
    rel = mel.create_material_expression(s, unreal.MaterialExpressionSubtract, -900, 260)
    link(wp, "", rel, "A")
    link(ap, "", rel, "B")
    link(lp, "", cu, "LP")
    link(ax, "", cu, "AX")
    link(rel, "", cu, "Rel")
    link(wp, "", cu, "WP")
    link(mel.create_material_expression(s, unreal.MaterialExpressionPixelDepth, -1200, 380), "", cu, "Depth")
    y = 400
    for n, v in (("Pattern", 0.0), ("DashOn", DASH_ON), ("DashOff", DASH_OFF), ("HatchWidth", HATCH_W),
                 ("NearFade", NEAR_FADE), ("NearFadeLen", NEAR_FADE_LEN)):
        link(ps(s, n, v, -900, y), "", cu, n)
        y += 80
    mel.connect_material_property(cu, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    mel.recompile_material(s)
    save(DRESS + "/M_SignalPattern")
    return m, g, s


def instance(name, parent, scalars=None, vectors=None):
    path = DRESS + "/" + name
    if eal.does_asset_exist(path):
        mi = eal.load_asset(path)
    else:
        mi = at.create_asset(name, DRESS, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    for kk, v in (scalars or {}).items():
        mel.set_material_instance_scalar_parameter_value(mi, kk, float(v))
    for kk, v in (vectors or {}).items():
        mel.set_material_instance_vector_parameter_value(mi, kk, lc(v))
    mel.update_material_instance(mi)
    save(path)
    return mi


MI = {}


def build_instances():
    m, g, s = build_materials()
    for n, (rgb, r) in LIT.items():
        pat, amp, prough, pscale = PATTERNS.get(n, (0, 0.0, 0.0, 1.0))
        MI[n] = instance(n, m, {"Roughness": r, "Pattern": pat, "PatternAmp": amp, "PatternRough": prough,
                                "PatternScale": pscale}, {"Color": rgb})
    for n, (rgb, sc) in GLOW.items():
        MI[n] = instance(n, g, {"Scale": sc}, {"Color": rgb})
    for n, (rgb, pat) in SIGNAL.items():
        MI[n] = instance(n, s, {"Pattern": pat}, {"Color": rgb})
    REPORT.append("materials: 3 masters, %d instances in %s" % (len(MI), DRESS))


def load_instances():
    for n in list(LIT) + list(GLOW) + list(SIGNAL):
        p = DRESS + "/" + n
        if eal.does_asset_exist(p):
            MI[n] = eal.load_asset(p)


# --- QA look ---
def repalette():
    """The LIT and GLOW instances' numbers on the masters as they stand, making any new instance: no master is
    recompiled (so M_SignalPattern's per-session cache is untouched). Step "palette", run only when asked for."""
    m, g = eal.load_asset(DRESS + "/M_Dress"), eal.load_asset(DRESS + "/M_DressGlow")
    for n, (rgb, r) in LIT.items():
        pat, amp, prough, pscale = PATTERNS.get(n, (0, 0.0, 0.0, 1.0))
        MI[n] = instance(n, m, {"Roughness": r, "Pattern": pat, "PatternAmp": amp, "PatternRough": prough,
                                "PatternScale": pscale}, {"Color": rgb})
    for n, (rgb, sc) in GLOW.items():
        MI[n] = instance(n, g, {"Scale": sc}, {"Color": rgb})
    REPORT.append("palette: %d instances re-numbered (PALETTE %s)" % (len(LIT) + len(GLOW), PALETTE))
# --- end QA look


# --- zones and the level's boxes --------------------------------------------------------------------------------------
def zone(x, y, z=0.0):
    if x < FRONT - 1.0 or y < SY0 - T - 1.0 or y > NY1 + T + 1.0 or x > EX1 + T + 1.0:
        return "street"
    if x < HX0 - T:
        return "lobby"
    if y < HY0 - T:
        return "back"
    if x > HX1 + T:
        return "east"
    if y > HY1 + T:
        return "north"
    return "dais" if x >= DAIS_X0 else "hall"


def label(a):
    return a.get_actor_label()


def folder(a):
    return str(a.get_folder_path())


def bounds(a):
    o, e = a.get_actor_bounds(False)
    return (o.x - e.x, o.x + e.x, o.y - e.y, o.y + e.y, o.z - e.z, o.z + e.z)


def centre(b):
    return ((b[0] + b[1]) / 2, (b[2] + b[3]) / 2, (b[4] + b[5]) / 2)


def overlaps(a, b, pad=0.0):
    return (a[0] < b[1] + pad and b[0] < a[1] + pad and a[2] < b[3] + pad and b[2] < a[3] + pad
            and a[4] < b[5] + pad and b[4] < a[5] + pad)


def inside(p, b):
    return b[0] <= p[0] <= b[1] and b[2] <= p[1] <= b[3] and b[4] <= p[2] <= b[5]


def built_with(a):
    """The MI_Box_* (or MI_Guest_*, ...) the actor was built with: remembered in a tag on the first pass."""
    tags = [str(t) for t in a.get_editor_property("tags")]
    for t in tags:
        if t.startswith("dress:"):
            return t[6:]
    c = a.static_mesh_component
    mat = c.get_material(0)
    name = mat.get_name() if mat else ""
    a.set_editor_property("tags", list(a.get_editor_property("tags")) + [unreal.Name("dress:" + name)])
    return name


def set_actors():
    out = []
    for a in eas.get_all_level_actors():
        if isinstance(a, unreal.StaticMeshActor) and not label(a).startswith("Dress_") \
                and not folder(a).startswith(SKIP_FOLDERS):
            out.append(a)
    out.sort(key=label)
    return out


# --- 2. surfaces ------------------------------------------------------------------------------------------------------------
def resurface():
    counts, missing, turn = {}, set(), {}
    for a in set_actors():
        old = built_with(a)
        lab = label(a)
        cx, cy, cz = centre(bounds(a))
        zn = zone(cx, cy, cz)
        if old == "MI_Box_Wall" and bounds(a)[0] <= FRONT + 1.0 and bounds(a)[1] <= LX0 + 1.0:
            zn = "street"                                  # the lobby's west wall is the facade
        new = None
        for (mat, zones, prefixes, target) in SURFACES:
            if not old.startswith(mat):
                continue
            if zones and zn not in zones:
                continue
            if prefixes and not lab.startswith(prefixes):
                continue
            new = target
            break
        if new is None:
            continue
        if new in ROTATE:
            pool = ROTATE[new]
            turn[new] = turn.get(new, -1) + 1
            new = pool[turn[new] % len(pool)]
        mi = MI.get(new)
        if mi is None:
            missing.add(new)
            continue
        a.static_mesh_component.set_material(0, mi)
        counts[new] = counts.get(new, 0) + 1
    REPORT.append("surfaces: %d actors re-surfaced (%s)" % (sum(counts.values()),
                  ", ".join("%s %d" % kv for kv in sorted(counts.items()))))
    if missing:
        warn("instances missing (run the materials step): %s" % sorted(missing))


def resign():
    """TextRender signs: bone, except the gold side exit and the trap (gold, stencil 2) and LIVE BROADCAST (red, 1)."""
    special = {"EXIT": (SIG_GOLD, 2), "STAGE TRAP": (SIG_GOLD, 2), "LIVE BROADCAST": (SIG_RED, 1),
               "LOT 41: THE DIRECTOR'S CUT": (PAPER, 0)}   # the scene's subject: paper, legible in the film
    n = 0
    for a in eas.get_all_level_actors():
        if not isinstance(a, unreal.TextRenderActor) or label(a).startswith("Dress_"):
            continue
        c = a.get_component_by_class(unreal.TextRenderComponent)
        text = str(c.get_editor_property("text")).strip().upper()
        rgb, stencil = special.get(text, (BONE, 0))
        try_set(c, text_render_color=fc(rgb), render_custom_depth=stencil != 0, custom_depth_stencil_value=stencil)
        n += 1
    REPORT.append("signs: %d recoloured" % n)


# --- 3. dressing --------------------------------------------------------------------------------------------------------------
NN = [0]
PLACED = []
SKIPPED = {}
OBST = []        # props: (box)
KEEP = []        # gameplay keep-outs: (box)
WALLS = []       # visible wall boxes
FLOOR_LABELS = ("Floor", "Street", "Pavement", "Carpet", "Dais", "Walkway", "Gallery_West", "Bridge", "Catwalk",
                "Light_Platform", "Balcony", "Terrace", "Stage_Trap")


def survey():
    """Reads the level: walls (for skins and the wall-behind test), props (overlap test), gameplay keep-outs."""
    del OBST[:], KEEP[:], WALLS[:]
    for a in set_actors():
        b = bounds(a)
        if built_with(a) == "MI_Box_Wall":
            WALLS.append(b)
        elif not label(a).startswith(FLOOR_LABELS):
            OBST.append(b)
    for a in eas.get_all_level_actors():
        if folder(a) != "Take":
            continue
        cls = a.get_class().get_name()
        if cls in ("SceneTrigger", "ShotZone", "TakeDirector"):
            continue
        p = a.get_actor_location()
        if cls == "RedTapeZone":
            try:
                e = a.get_editor_property("extent")
                sc = a.get_actor_scale3d()
                ex, ey = e.x * abs(sc.x), e.y * abs(sc.y)
            except Exception:
                ex, ey = 100.0, 100.0
            m = TAPE_MARGIN + 10.0
            yaw = a.get_actor_rotation().yaw
            if abs(math.sin(math.radians(yaw))) > 0.7:
                ex, ey = ey, ex
            for (x0, x1, y0, y1) in ((p.x - ex - m, p.x - ex + m, p.y - ey - m, p.y + ey + m),
                                     (p.x + ex - m, p.x + ex + m, p.y - ey - m, p.y + ey + m),
                                     (p.x - ex - m, p.x + ex + m, p.y - ey - m, p.y - ey + m),
                                     (p.x - ex - m, p.x + ex + m, p.y + ey - m, p.y + ey + m)):
                KEEP.append((x0, x1, y0, y1, p.z - 10.0, p.z + 15.0))
        else:
            KEEP.append((p.x - KEEP_OUT, p.x + KEEP_OUT, p.y - KEEP_OUT, p.y + KEEP_OUT, p.z - 120.0, p.z + 250.0))


def wall_behind(x, y, z, nx, ny):
    """True if wall geometry stands just behind a point on a wall face (normal nx, ny): not an opening."""
    p = (x - nx * T * 0.5, y - ny * T * 0.5, z)
    return any(inside(p, w) for w in WALLS)


def free(b, why, check_props=True):
    for kb in KEEP:
        if overlaps(b, kb):
            SKIPPED[why] = SKIPPED.get(why, 0) + 1
            return False
    if check_props:
        for ob in OBST:
            if overlaps(b, ob, -0.5):
                SKIPPED[why] = SKIPPED.get(why, 0) + 1
                return False
    return True


def piece(what, b, mat, mesh=None, yaw=0.0, pitch=0.0, size=None, shadow=False, zone_name=None):
    """One dressing mesh: NoCollision, no navigation, no decals; b is (x0, x1, y0, y1, z0, z1) before turning."""
    NN[0] += 1
    cx, cy, cz = centre(b)
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(cx, cy, cz),
                                   unreal.Rotator(roll=0.0, pitch=pitch, yaw=yaw))
    a.set_actor_label("Dress_%s_%03d" % (what, NN[0]))
    a.set_folder_path("Dress/" + (zone_name or zone(cx, cy, cz)))
    c = a.static_mesh_component
    c.set_static_mesh(mesh or CUBE)
    c.set_material(0, MI[mat])
    c.set_collision_profile_name("NoCollision")
    try_set(c, cast_shadow=shadow, receives_decals=False, can_ever_affect_navigation=False)
    sx, sy, sz = size or (b[1] - b[0], b[3] - b[2], b[5] - b[4])
    a.set_actor_scale3d(unreal.Vector(max(sx, 0.2) / 100.0, max(sy, 0.2) / 100.0, max(sz, 0.2) / 100.0))
    PLACED.append(what)
    return a


def wall_faces(w, want):
    """The faces of a thin wall box whose outside lies in one of the zones `want`: (axis, face, normal, a0, a1, z0, z1)."""
    x0, x1, y0, y1, z0, z1 = w
    out = []
    if x1 - x0 <= 32.0 and y1 - y0 > 30.0:
        for (f, n) in ((x0, -1), (x1, 1)):
            if zone(f + n * 5.0, (y0 + y1) / 2, (z0 + z1) / 2) in want:
                out.append(("y", f, n, y0, y1, z0, z1))
    elif y1 - y0 <= 32.0 and x1 - x0 > 30.0:
        for (f, n) in ((y0, -1), (y1, 1)):
            if zone((x0 + x1) / 2, f + n * 5.0, (z0 + z1) / 2) in want:
                out.append(("x", f, n, x0, x1, z0, z1))
    return out


def skin(what, face, z0, z1, proud, mat, inset=0.0):
    """A strip on a wall face from z0 to z1 (clipped to the wall), `proud` thick, along its whole length."""
    axis, f, n, a0, a1, wz0, wz1 = face
    z0, z1 = max(z0, wz0), min(z1, wz1)
    if z1 - z0 < 1.0 or a1 - a0 - 2 * inset < 10.0:
        return None
    lo, hi = sorted((f, f + n * proud))
    if axis == "y":
        return piece(what, (lo, hi, a0 + inset, a1 - inset, z0, z1), mat)
    return piece(what, (a0 + inset, a1 - inset, lo, hi, z0, z1), mat)


def on_face(face, along0, along1, z0, z1, proud, out=0.0):
    """A box on a wall face: from `along0` to `along1` along it, `out` to `out + proud` from it."""
    axis, f, n = face[0], face[1], face[2]
    lo, hi = sorted((f + n * out, f + n * (out + proud)))
    if axis == "y":
        return (lo, hi, along0, along1, z0, z1)
    return (along0, along1, lo, hi, z0, z1)


def wall_ok(face, along0, along1, zs):
    axis, f, n = face[0], face[1], face[2]
    for al in (along0, (along0 + along1) / 2, along1):
        for z in zs:
            x, y = (f, al) if axis == "y" else (al, f)
            nx, ny = (n, 0) if axis == "y" else (0, n)
            if not wall_behind(x, y, z, nx, ny):
                return False
    return True


def sign(text, x, y, z, yaw, size=22.0):
    NN[0] += 1
    a = eas.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(x, y, z), unreal.Rotator(roll=0.0, pitch=0.0,
                                                                                                   yaw=yaw))
    a.set_actor_label("Dress_Sign_%03d" % NN[0])
    a.set_folder_path("Dress/back")
    c = a.get_component_by_class(unreal.TextRenderComponent)
    try:
        c.set_text(text)
    except Exception:
        c.set_editor_property("text", text)
    try_set(c, world_size=size, text_render_color=fc(BONE),
            horizontal_alignment=unreal.HorizTextAligment.EHTA_CENTER,
            vertical_alignment=unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)
    r = math.radians(yaw)
    w = len(text) * size * 0.62 + 30.0
    bx, by = x - 4.0 * math.cos(r), y - 4.0 * math.sin(r)
    piece("Sign_Board", (bx - 2, bx + 2, by - w / 2, by + w / 2, z - size * 0.85, z + size * 0.85), "MI_Front_Panel",
          yaw=yaw, size=(4.0, w, size * 1.7), zone_name="back")
    PLACED.append("Sign")


def dress_street():
    # The kerb along the road.
    piece("Kerb", (PAVE_X0, PAVE_X0 + KERB_W, HY0 - 400, HY1 + 400, -0.5, 0.6), "MI_Front_Facade")
    # Pewter stanchions along the queue (road side, so Julian never walks through them), velvet ropes between.
    posts = []
    y = QUEUE_POSTS[0]
    while y <= QUEUE_POSTS[1] + 0.1:
        x = PAVE_X0 + 12.0
        b = (x - 4, x + 4, y - 4, y + 4, 0, 94)
        if free(b, "queue stanchion"):
            piece("Stanchion", b, "MI_Front_Trim", mesh=CYL, size=(7, 7, 94))
            piece("Stanchion_Foot", (x - 14, x + 14, y - 14, y + 14, 0, 3), "MI_Front_Trim", mesh=CYL)
            posts.append(y)
        y += QUEUE_POSTS[2]
    # --- QA front (note 13): the velvet sags from hooks under the posts' tops (94 cm), a chain along a catenary
    for y0, y1 in zip(posts, posts[1:]):
        x, n, sag = PAVE_X0 + 12.0, 8, min(28.0, 0.06 * (y1 - y0) + 4.0)
        pts = [(y0 + 4 + (y1 - y0 - 8) * i / n, 88.0 - 4.0 * sag * (i / n) * (1 - i / n)) for i in range(n + 1)]
        for (pa, za), (pb, zb) in zip(pts, pts[1:]):
            ln, my, mz = math.hypot(pb - pa, zb - za), (pa + pb) / 2, (za + zb) / 2
            piece("Queue_Sag", (x - 1, x + 1, my - 1, my + 1, mz - 1, mz + 1), "MI_Front_Velvet", mesh=CYL, yaw=90.0,
                  pitch=math.degrees(math.atan2(zb - za, pb - pa)) - 90.0, size=(3.6, 3.6, ln + 2.2))
    # --- end QA front
    # The press wall.
    x0, x1, py = PRESS_WALL
    b = (x0, x1, py - 4, py, 0, 240)
    if free(b, "press wall"):
        piece("Press_Wall", b, "MI_Front_Linen", shadow=True)
    # Two potted bay trees at the doors.
    for s in (1, -1):
        x, y = FRONT - 45.0, s * BAY_Y
        b = (x - 25, x + 25, y - 25, y + 25, 0, 210)
        if free(b, "bay tree"):
            piece("Bay_Pot", (x - 25, x + 25, y - 25, y + 25, 0, 55), "MI_Front_Panel", mesh=CYL, shadow=True)
            piece("Bay_Stem", (x - 3, x + 3, y - 3, y + 3, 55, 130), "MI_Front_Panel", mesh=CYL)
            piece("Bay_Crown", (x - 40, x + 40, y - 40, y + 40, 120, 205), "MI_Front_Foliage", mesh=SPHERE, shadow=True)


def dress_lobby():
    for w in WALLS:
        for face in wall_faces(w, ("lobby",)):
            z0, z1 = face[5], face[6]
            if z0 < 10.0 or abs(z0 - (F2 - SLAB)) < 2.0:      # a storey's own wall, not a lintel over a door
                base = 0.0 if z0 < 10.0 else F2
                skin("Wainscot", face, base, base + WAINSCOT_H, WAINSCOT_PROUD, "MI_Front_Panel")
            if z0 <= CORNICE_Z and z1 >= CORNICE_Z + 12.0:     # runs on over the doors' lintels
                skin("Cornice", face, CORNICE_Z, CORNICE_Z + 12.0, CORNICE_PROUD, "MI_Front_Trim")
    x, y, z, r, d = CHANDELIER
    piece("Chandelier_Rod", (x - 1, x + 1, y - 1, y + 1, z, k("TOP", 900.0)), "MI_Back_Cable")
    piece("Chandelier", (x - d / 2, x + d / 2, y - d / 2, y + d / 2, z - 30 - d / 2, z - 30 + d / 2), "MI_Front_Sconce",
          mesh=SPHERE)
    for i in range(4):
        a = math.radians(45.0 + 90.0 * i)
        px, py = x + r * math.cos(a), y + r * math.sin(a)
        piece("Chandelier", (px - d / 2, px + d / 2, py - d / 2, py + d / 2, z - d / 2, z + d / 2), "MI_Front_Sconce",
              mesh=SPHERE)


def dress_hall():
    for w in WALLS:
        for face in wall_faces(w, ("hall",)):
            axis, f, n, a0, a1, z0, z1 = face
            if axis != "x" or z0 > 10.0:
                continue                                     # the long walls, ground storey only
            al = math.ceil((a0 + PILASTER_W) / PILASTER_STEP) * PILASTER_STEP
            while al + PILASTER_W / 2 <= min(a1, DAIS_X0) - 10.0:
                lo, hi = al - PILASTER_W / 2, al + PILASTER_W / 2
                b = on_face(face, lo, hi, 0.0, PILASTER_TOP, PILASTER_D)
                if wall_ok(face, lo, hi, (40.0, 215.0, PILASTER_TOP - 20.0)) and free(b, "pilaster"):
                    piece("Pilaster", b, "MI_Front_Wall", shadow=True)
                    piece("Pilaster_Cap", on_face(face, lo - 5, hi + 5, PILASTER_TOP - 14, PILASTER_TOP,
                                                  PILASTER_D + 5), "MI_Front_Trim")
                    piece("Pilaster_Base", on_face(face, lo - 3, hi + 3, 0, 12, PILASTER_D + 3), "MI_Front_Trim")
                    piece("Sconce", on_face(face, al - 7, al + 7, SCONCE_Z - 10, SCONCE_Z + 10, 6, out=PILASTER_D),
                          "MI_Front_Sconce")
                al += PILASTER_STEP
    for s in (1, -1):
        y0, y1 = sorted((s * (RUNNER_Y - RUNNER_W / 2), s * (RUNNER_Y + RUNNER_W / 2)))
        piece("Runner", (RUNNER_X[0], RUNNER_X[1], y0, y1, 0.0, 0.6), "MI_Front_Runner")


def dress_dais():
    # Pewter lip on the front edge and the two sides; footlights on the front face (never on the deck: the tape).
    piece("Dais_Lip", (DAIS_X0 - 1.5, DAIS_X0, -1200, 1200, DAIS_H - 4, DAIS_H), "MI_Front_Trim")
    for s in (1, -1):
        y0, y1 = sorted((s * 1200.0, s * 1201.5))
        piece("Dais_Lip", (DAIS_X0, HX1, y0, y1, DAIS_H - 4, DAIS_H), "MI_Front_Trim")
    for s in (1, -1):                                       # one continuous strip each side of the aisle
        y0, y1 = sorted((s * (GAP + 60.0), s * 1180.0))
        piece("Footlight", (DAIS_X0 - 2.0, DAIS_X0, y0, y1, DAIS_H * 0.3, DAIS_H * 0.6), "MI_Front_Footlight")
    for (fx, fy) in PYRO_FLARES:
        if abs(fy) <= GAP + 60.0:
            continue                                          # Julian's way onto the dais
        x = fx + CHARGE_DX
        b = (x - 6, x + 6, fy - 6, fy + 6, DAIS_H, DAIS_H + 18)
        if free(b, "pyro charge"):
            piece("Pyro_Charge", b, "MI_Back_Cable", mesh=CYL)


def dress_back():
    for w in WALLS:
        # The raw backs of the flats: ply on the corridor face of the hall's south wall, battens, braces, tubes.
        for face in wall_faces(w, ("back",)):
            axis, f, n, a0, a1, z0, z1 = face
            if not (HY0 - T - 2.0 <= f <= HY0 + 2.0):
                continue
            skin("Flat_Ply", face, 0.0, BACK_H, 1.0, "MI_Back_Ply")
            full = z0 < 10.0 and z1 >= BACK_H - 10.0
            if full:
                skin("Flat_Rail", (axis, f + n * 1.0, n, a0, a1, z0, z1), 300.0, 306.0, 3.0, "MI_Back_Batten")
                skin("Cable_Run", (axis, f + n * 4.0, n, a0, a1, z0, z1), 0.0, 4.0, 3.0, "MI_Back_Cable", inset=5.0)
            al = math.ceil(a0 / BATTEN_STEP) * BATTEN_STEP
            bay = 0
            while al <= a1 - 3.0:
                if full and wall_ok(face, al - 3, al + 3, (60.0, 250.0, BACK_H - 40.0)):
                    piece("Batten", on_face(face, al - 3, al + 3, 0.0, BACK_H - 10.0, 3.0, out=1.0), "MI_Back_Batten")
                    mid = al + BATTEN_STEP / 2
                    if bay % 2 == 0 and mid < a1 - 20 and wall_ok(face, mid - 3, mid + 3, (60.0, BRACE_TOP)):
                        foot = on_face(face, mid - 15, mid + 15, 0.0, 12.0, 22.0, out=BRACE_OUT)
                        span = on_face(face, mid - 4, mid + 4, 0.0, BRACE_TOP, BRACE_OUT + 22.0, out=1.0)
                        if free(span, "brace"):
                            length = math.hypot(BRACE_OUT, BRACE_TOP)
                            pitch = math.degrees(math.atan2(BRACE_TOP, BRACE_OUT))
                            cx = mid if axis == "x" else f + n * (1.0 + BRACE_OUT / 2)
                            cy = f + n * (1.0 + BRACE_OUT / 2) if axis == "x" else mid
                            yaw = (-90.0 if n > 0 else 90.0) if axis == "x" else (180.0 if n > 0 else 0.0)
                            piece("Brace", (cx - 3, cx + 3, cy - 3, cy + 3, BRACE_TOP / 2 - 3, BRACE_TOP / 2 + 3),
                                  "MI_Back_Batten", yaw=yaw, pitch=pitch, size=(length, 6.0, 6.0))
                            piece("Stage_Weight", foot, "MI_Back_Cable")
                    bay += 1
                al += BATTEN_STEP
            al = math.ceil((a0 + TUBE_L) / TUBE_STEP) * TUBE_STEP
            while al + TUBE_L / 2 <= a1:
                lo, hi = al - TUBE_L / 2, al + TUBE_L / 2
                b = on_face(face, lo, hi, TUBE_Z, TUBE_Z + TUBE_H, 5.0, out=4.0)
                if wall_ok(face, lo, hi, (TUBE_Z,)) and free(b, "tube"):
                    piece("Work_Tube", b, "MI_Back_Tube")
                al += TUBE_STEP
    # Road cases in twos against the corridor's back wall, pewter edge strips, a spike mark in front of each stack.
    for x in ROAD_CASES:
        y0 = BY0
        b = (x - 35, x + 35, y0 + 1, y0 + 61, 0, 105)
        if not (wall_behind(x - 30, y0, 50.0, 0, 1) and wall_behind(x + 30, y0, 50.0, 0, 1)):
            SKIPPED["road case (doorway)"] = SKIPPED.get("road case (doorway)", 0) + 1
            continue
        if not free(b, "road case"):
            continue
        piece("Road_Case", (x - 35, x + 35, y0 + 1, y0 + 61, 0, 58), "MI_Back_Steel", shadow=True)
        piece("Road_Case", (x - 30, x + 30, y0 + 6, y0 + 56, 58, 105), "MI_Back_Steel", shadow=True)
        piece("Case_Edge", (x - 36, x + 36, y0 + 60, y0 + 62, 54, 58), "MI_Front_Trim")
        piece("Case_Edge", (x - 31, x + 31, y0 + 55, y0 + 57, 101, 105), "MI_Front_Trim")
        sy = y0 + 110.0
        for yaw in (45.0, -45.0):
            piece("Spike", (x - 15, x + 15, sy - 2, sy + 2, 0.0, 0.4), "MI_Back_Tape", yaw=yaw, size=(30.0, 4.0, 0.4))
    for (text, x, z) in BACK_SIGNS:
        if wall_behind(x, BY0, z, 0, 1):
            sign(text, x, BY0 + 6.0, z, 90.0)


def dress_catwalk():
    z = F2
    # Camera tape along both edges of the catwalk and round the platform (flat: under everything, never in the way).
    x0, x1 = GALLERY_X1, PLAT[0]
    for (y0, y1) in ((CAT_Y0 + 1, CAT_Y0 + 6), (CAT_Y1 - 6, CAT_Y1 - 1)):
        piece("Tape_Edge", (x0, x1, y0, y1, z, z + 0.4), "MI_Back_Tape", zone_name="catwalk")
    px0, px1, py0, py1 = PLAT
    for b in ((px0 + 1, px1 - 1, py0 + 1, py0 + 6), (px0 + 1, px1 - 1, py1 - 6, py1 - 1),
              (px0 + 1, px0 + 6, py0 + 1, py1 - 1), (px1 - 6, px1 - 1, py0 + 1, py1 - 1)):
        piece("Tape_Edge", b + (z, z + 0.4), "MI_Back_Tape", zone_name="catwalk")
    # Truss overhead: along the catwalk, across the platform.
    cy = (CAT_Y0 + CAT_Y1) / 2
    piece("Truss", (GALLERY_X1, px1, cy - 15, cy + 15, TRUSS_Z, TRUSS_Z + 30), "MI_Back_Steel", zone_name="catwalk")
    cx = (px0 + px1) / 2
    piece("Truss", (cx - 15, cx + 15, py0, py1, TRUSS_Z, TRUSS_Z + 30), "MI_Back_Steel", zone_name="catwalk")
    # Black lamp housings hung outside both catwalk rails, every 2 m.
    x = GALLERY_X1 + 100.0
    while x <= px0 - 50.0:
        for (ry, s) in ((CAT_Y0, -1), (CAT_Y1, 1)):
            y = ry + s * 16.0
            b = (x - 10, x + 10, y - 10, y + 10, z + RAIL_H - 40, z + RAIL_H - 8)
            if free(b, "lamp housing"):
                piece("Lamp_Housing", b, "MI_Back_Cable", mesh=CYL, zone_name="catwalk")
        x += HOUSING_STEP
    # Tape wrapped on the rails at the posts; a few cable loops hanging under the catwalk's edge.
    for a in set_actors():
        if not label(a).startswith("Post"):
            continue
        b = bounds(a)
        if b[4] < z - 5 or not (GALLERY_X1 - 15 <= b[0] and b[1] <= px1 + 15 and py0 - 15 <= b[2] and b[3] <= CAT_Y1 + 15):
            continue
        px, py = (b[0] + b[1]) / 2, (b[2] + b[3]) / 2
        piece("Rail_Tape", (px - 6, px + 6, py - 6, py + 6, z + 60, z + 66), "MI_Back_Tape", zone_name="catwalk")
    for x in (GALLERY_X1 + 250.0, GALLERY_X1 + 650.0, GALLERY_X1 + 900.0):
        b = (x - 1, x + 1, CAT_Y0 - 1, CAT_Y0 + 1, z - SLAB - 80, z - SLAB)
        if free(b, "cable loop"):
            piece("Cable_Loop", b, "MI_Back_Cable", zone_name="catwalk")


def dress():
    for a in eas.get_all_level_actors():
        if label(a).startswith("Dress_"):
            eas.destroy_actor(a)
    survey()
    for fn in (dress_street, dress_lobby, dress_hall, dress_dais, dress_back, dress_catwalk):
        try:
            fn()
        except Exception as e:
            warn("%s failed: %s" % (fn.__name__, e))
    tally = {}
    for w in PLACED:
        tally[w] = tally.get(w, 0) + 1
    REPORT.append("dressing: %d pieces (%s)" % (len(PLACED), ", ".join("%s %d" % kv for kv in sorted(tally.items()))))
    if SKIPPED:
        REPORT.append("dressing skipped: %s" % ", ".join("%s %d" % kv for kv in sorted(SKIPPED.items())))


# --- 4. lights ------------------------------------------------------------------------------------------------------------
def relight():
    for a in eas.get_all_level_actors():
        if label(a).startswith(("Dress_Back_Dais", "Dress_House_Fill", "Dress_Facade_Wash")):
            eas.destroy_actor(a)
    n = 0
    for a in eas.get_all_level_actors():
        lab = label(a)
        comp = a.get_component_by_class(unreal.LightComponent) if isinstance(a, unreal.Light) else None
        if comp is None:
            continue
        for prefix, props in RETUNE.items():
            if lab.startswith(prefix):
                try_set(comp, **props)
                n += 1
        if lab.startswith("Lamp"):
            p = a.get_actor_location()
            if zone(p.x, p.y, p.z) in BACKS or (zone(p.x, p.y, p.z) in ("hall", "dais") and p.z > F2 + 100.0):
                try_set(comp, volumetric_scattering_intensity=WORK_HAZE)
                n += 1
    bd = BACK_DAIS
    a = eas.spawn_actor_from_class(unreal.SpotLight, unreal.Vector(bd["x"], bd["y"], bd["z"]),
                                   unreal.Rotator(roll=0.0, pitch=bd["pitch"], yaw=bd["yaw"]))
    a.set_actor_label("Dress_Back_Dais")
    a.set_folder_path("Light/Spots")
    c = a.get_component_by_class(unreal.SpotLightComponent)
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    try_set(c, intensity_units=unreal.LightUnits.CANDELAS)
    try_set(c, intensity=bd["cd"], attenuation_radius=bd["radius"], outer_cone_angle=bd["cone"],
            inner_cone_angle=bd["cone"] * 0.6, cast_shadows=False, volumetric_scattering_intensity=bd["haze"],
            light_color=unreal.Color(r=int(COOL[0] * 255), g=int(COOL[1] * 255), b=int(COOL[2] * 255), a=255))
    for y in HOUSE_FILL["ys"]:
        hf = eas.spawn_actor_from_class(unreal.SpotLight, unreal.Vector(HOUSE_FILL["x"], y, HALL_H - 60.0),
                                        unreal.Rotator(roll=0.0, pitch=-90.0, yaw=0.0))
        hf.set_actor_label("Dress_House_Fill")
        hf.set_folder_path("Light/Spots")
        hc = hf.get_component_by_class(unreal.SpotLightComponent)
        hc.set_mobility(unreal.ComponentMobility.MOVABLE)
        try_set(hc, intensity_units=unreal.LightUnits.CANDELAS)
        try_set(hc, intensity=HOUSE_FILL["cd"], attenuation_radius=HOUSE_FILL["radius"], cast_shadows=False,
                outer_cone_angle=HOUSE_FILL["outer"], inner_cone_angle=HOUSE_FILL["inner"],
                volumetric_scattering_intensity=0.0, light_color=unreal.Color(r=255, g=240, b=220, a=255))
    fw = FACADE_WASH
    wa = eas.spawn_actor_from_class(unreal.SpotLight, unreal.Vector(fw["x"], fw["y"], fw["z"]),
                                    unreal.Rotator(roll=0.0, pitch=fw["pitch"], yaw=fw["yaw"]))
    wa.set_actor_label("Dress_Facade_Wash")
    wa.set_folder_path("Light/Spots")
    wc = wa.get_component_by_class(unreal.SpotLightComponent)
    wc.set_mobility(unreal.ComponentMobility.MOVABLE)
    try_set(wc, intensity_units=unreal.LightUnits.CANDELAS)
    try_set(wc, intensity=fw["cd"], attenuation_radius=fw["radius"], outer_cone_angle=fw["cone"],
            inner_cone_angle=fw["cone"] * 0.6, cast_shadows=False, volumetric_scattering_intensity=0.0,
            light_color=unreal.Color(r=int(COOL[0] * 255), g=int(COOL[1] * 255), b=int(COOL[2] * 255), a=255))
    julian = next((x for x in eas.get_all_level_actors() if label(x) == "Julian"), None)
    if julian is not None:
        for old in [x for x in eas.get_all_level_actors() if label(x).startswith("Dress_Julian_Key")]:
            eas.destroy_actor(old)
        jk = JULIAN_KEY
        p = julian.get_actor_transform().transform_location(unreal.Vector(jk["dx"], jk["dy"], jk["dz"]))
        k_ = eas.spawn_actor_from_class(unreal.PointLight, p, unreal.Rotator(0.0, 0.0, 0.0))
        k_.set_actor_label("Dress_Julian_Key")
        k_.set_folder_path("Light/Spots")
        kc = k_.get_component_by_class(unreal.PointLightComponent)
        kc.set_mobility(unreal.ComponentMobility.MOVABLE)
        try_set(kc, intensity_units=unreal.LightUnits.CANDELAS)
        try_set(kc, intensity=jk["cd"], attenuation_radius=jk["radius"], cast_shadows=False,
                volumetric_scattering_intensity=0.0, light_color=unreal.Color(r=255, g=240, b=220, a=255),
                lighting_channels=unreal.LightingChannels(channel0=False, channel1=True, channel2=False))
        attached = False
        booms = julian.get_components_by_class(unreal.SpringArmComponent)
        if booms:
            try:
                k_.attach_to_component(booms[0], unreal.Name("SpringEndpoint"), unreal.AttachmentRule.SNAP_TO_TARGET,
                                       unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, False)
                k_.get_editor_property("root_component").set_relative_location(unreal.Vector(0.0, 0.0, jk["boom_dz"]), False, True)
                attached = True
            except Exception as e:
                warn("julian key on the boom: %s" % e)
        if not attached:
            k_.attach_to_actor(julian, unreal.Name(), unreal.AttachmentRule.KEEP_WORLD,
                               unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, False)
        for mc in julian.get_components_by_class(unreal.PrimitiveComponent):
            if isinstance(mc, (unreal.SkeletalMeshComponent, unreal.StaticMeshComponent)):
                try_set(mc, lighting_channels=unreal.LightingChannels(channel0=True, channel1=True, channel2=False))
    casters = []
    for l in eas.get_all_level_actors():
        if isinstance(l, (unreal.SpotLight, unreal.PointLight, unreal.DirectionalLight)):
            lc_ = l.get_component_by_class(unreal.LightComponent)
            if lc_ and lc_.get_editor_property("cast_shadows"):
                p = l.get_actor_location()
                casters.append("%s(%s)" % (label(l), zone(p.x, p.y, p.z)))
    REPORT.append("lights: %d retuned, Back_Dais added; shadow casters: %s" % (n, ", ".join(sorted(casters))))


# --- 5. tints ---------------------------------------------------------------------------------------------------------------
def retint():
    done = []
    for a in eas.get_all_level_actors():
        lab = label(a)
        cls = a.get_class().get_name()
        if lab in MARK_PLATES:
            try_set(a, plate_scale=MARK_PLATES[lab])
            done.append("%s.plate_scale" % lab)
        if lab in TINTS:
            try_set(a, tint=lc(TINTS[lab]))
            done.append(lab)
        if folder(a) != "Take":
            continue
        if any(cls.endswith(w) for w in ("Watcher", "PatrolGuard")):
            rgb = next((v for kk, v in BODY_TINTS.items() if lab.startswith(kk)), HEAVY)
            try:
                a.set_editor_property("body_tint", lc(rgb))
                done.append(lab)
            except Exception:
                pass                                         # before CPP.md's Watcher.h edit: C++ colours stand
        body = BANNER_BODY if lab.startswith("Banner") else DEVICE_BODY
        for prop, rgb in (("body_colour", body), ("curtain_colour", CURTAIN), ("flare_colour", FLARE),
                          ("ring_idle_colour", RING_IDLE)):
            try:
                a.get_editor_property(prop)
            except Exception:
                continue
            try_set(a, **{prop: lc(rgb)})
            done.append("%s.%s" % (lab, prop))
    REPORT.append("tints: %d set (%s)" % (len(done), ", ".join(done)))


# =========================================================================================================================
if step("materials"):
    build_instances()
else:
    load_instances()
if ONLY is not None and "palette" in ONLY:      # --- QA look: instances only (a full run builds them above)
    repalette()
if step("surfaces"):
    resurface()
    resign()
elif ONLY is not None and "zones" in ONLY:      # --- QA look: the surfaces by zone, without the signs
    resurface()
if step("dressing"):
    dress()
if step("lights"):
    relight()
if step("tints"):
    retint()
les.save_current_level()
for line in REPORT:
    print("DRESS " + line)
"\n".join(REPORT)
