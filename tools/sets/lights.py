# QA look (Nov 14), the lighting pass: motivated practicals at the dressing, so the new props read in play (notes 11, 14).
# An idempotent overlay (tools/sets/README.md): it removes what its last run placed (tag set_lights) and places it again.
#     "<engine python>" -I tools/ue_mcp.py py tools/sets/lights.py 110
# Rules it keeps (sight's cue language, Mark.h / RedTapeZone.h / PatrolGuard.h): a warm pool on the floor means a safe
# spot (260-650 cd), cool light forbidden ground (260 cd), a guard's torch pool that he is looking (900 cd). So these are
# weaker and sit on walls, props and faces: small radii, high or tight to the object, no new pool on Julian's routes
# (the queue, the lobby's centre, the hall's centre aisle). No shadows, movable, no haze. The hall's lights are plain
# unattached PointLights inside AHouseDimmer's box (-1000..4400, -2000..2000), so they go down with the house.
# Switches: LIGHTS_AREAS (a wrapper global) runs part of it; SCALE scales every light; an entry's candelas is its own.
# W3 lights (Nov 15): finished by agent "lights". AREA_SCALE lifts an area (the side rooms read near-black in play);
# the street's four build_auction lamps move from the old box lamp heads to front.py's Rodin globes (MOVE_LAMPS);
# moonlight on the far lane's sedans, the newsstand's bulb; the lobby's lamps brighter; the hall is placed from
# hall_zones.json and the placed props at run time (HALL below), never from fixed coordinates. Sight cues stay the
# brightest thing on any floor: guard torch 3000 cd (pale, a shaft), drone 1800, marks 260 cd from 3.2 m.
import sys, importlib; sys.path.insert(0, r"C:\Jam\megajam\tools\sets"); import common as C; importlib.reload(C)
import math
import re

import unreal

TAG = "set_lights"
AREAS = globals().get("LIGHTS_AREAS") or ("street", "lobby", "hall", "rooms")
SCALE = 1.0                         # every light's candelas x this
AREA_SCALE = dict(street=1.0, lobby=1.0, hall=1.0, rooms=1.8)   # W3 lights: rooms x1.8 (near-black in play)
MOVE_LAMPS = dict(z=395.0, cd=57.0)  # W3 lights: build_auction's street lamps into the Rodin globes (False = leave)
F2 = 450.0
PAVE_X0 = -4100.0                   # build_auction's LX0 - 900: the street lamps stand at PAVE_X0 + 40 (front.py)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

TUNG = (1.0, 0.72, 0.45)            # shaded lamps, sconces
BULB = (1.0, 0.80, 0.58)            # bare bulbs
WORK = (1.0, 0.92, 0.80)            # work lamps, the street's lamps on the road
EMBER = (1.0, 0.55, 0.30)           # the ranges, the switchboard's lamps
MOON = (0.70, 0.80, 1.0)            # W3 lights: cool night light from across the road (noir: warm near, cool far)

# area, name, (x, y, z), candelas, radius, colour, and for a spot (pitch, yaw, outer cone). The story it lights.
LIGHTS = [
    # the lobby: the reception desk's banker's lamp, the two seating groups' lamps (off the centre)
    # W3 lights: the desk, the clerk and the paddle board were grey in play: brighter lamps, a picture light on the board
    ("lobby", "Seating_W", (-3080.0, 1180.0, 165.0), 100.0, 360.0, TUNG, None),
    ("lobby", "Seating_E", (-1290.0, 1110.0, 125.0), 110.0, 380.0, TUNG, None),
    ("lobby", "Seating_SW", (-3080.0, -1100.0, 165.0), 90.0, 360.0, TUNG, None),
    # W3 lights: the newel lamps at the stair's foot (front.py's globes), lit from the doors' side so they shade round
    ("lobby", "Newel_S", (-2650.0, -193.0, 160.0), 12.0, 140.0, BULB, None),
    ("lobby", "Newel_N", (-2650.0, 193.0, 160.0), 12.0, 140.0, BULB, None),
    # the hall: placed at run time from hall_zones.json and the placed props (hall_lights below; W3 lights)
    # backstage corridor: the ghost light at the far end
    ("rooms", "Corridor_Ghost", (-950.0, -2560.0, 175.0), 80.0, 280.0, BULB, None),
    # kitchen: two ranges going (a low ember glow), two pendants over the prep line
    ("rooms", "Kitchen_Ranges", (-300.0, -3140.0, 120.0), 90.0, 320.0, EMBER, None),
    ("rooms", "Kitchen_Pendant_W", (-500.0, -2960.0, 330.0), 70.0, 420.0, BULB, None),
    ("rooms", "Kitchen_Pendant_E", (400.0, -2960.0, 330.0), 70.0, 420.0, BULB, None),
    # prop store: two bare work bulbs over last season's productions
    ("rooms", "Propstore_W", (1300.0, -2960.0, 330.0), 70.0, 480.0, BULB, None),
    ("rooms", "Propstore_E", (2200.0, -2960.0, 330.0), 70.0, 480.0, BULB, None),
    # loading dock: the caged bulb over the punch clock, a work lamp at the shutter on the lorry
    ("rooms", "Dock_Clock", (2726.0, -2670.0, 240.0), 70.0, 330.0, BULB, None),
    ("rooms", "Dock_Shutter", (3650.0, -3200.0, 380.0), 220.0, 700.0, WORK, (-50.0, -90.0, 50.0)),
    # cloakroom: the attendant's counter lamp and the rails
    ("rooms", "Cloak_Counter", (-2650.0, 1500.0, 210.0), 60.0, 380.0, TUNG, None),
    ("rooms", "Cloak_Rails", (-2650.0, 1900.0, 240.0), 45.0, 380.0, BULB, None),
    # restrooms: the lights over the basins' mirrors, the stalls
    ("rooms", "Rest_Mirrors", (-3110.0, -1700.0, 205.0), 60.0, 360.0, TUNG, None),
    ("rooms", "Rest_Stalls", (-2400.0, -1700.0, 260.0), 40.0, 360.0, BULB, None),
    # upstairs over the cloakroom: the house manager's desk lamp, the safe behind its picture
    ("rooms", "Office_Desk", (-2700.0, 1790.0, F2 + 110.0), 70.0, 360.0, TUNG, None),
    ("rooms", "Office_Wall", (-2250.0, 1900.0, F2 + 230.0), 35.0, 300.0, TUNG, None),
    # upstairs over the restrooms: the detective's desk lamp, the switchboard's lamps
    ("rooms", "Security_Desk", (-2950.0, -1930.0, F2 + 105.0), 60.0, 320.0, TUNG, None),
    ("rooms", "Security_Board", (-2230.0, -1912.0, F2 + 120.0), 40.0, 220.0, EMBER, None),
    # the fire stairs: one caged bulb on each
    ("rooms", "Stair_N", (-1550.0, 1700.0, 450.0), 50.0, 480.0, BULB, None),
    ("rooms", "Stair_S", (-1550.0, -1700.0, 450.0), 50.0, 480.0, BULB, None),
    # preview gallery: a picture light on the star lot's easel, the salon hang washed from above
    ("rooms", "Gallery_Star", (2140.0, 2540.0, 330.0), 260.0, 450.0, TUNG, (-45.0, 90.0, 22.0)),
    ("rooms", "Gallery_Hang_W", (300.0, 2800.0, 310.0), 45.0, 320.0, TUNG, None),
    ("rooms", "Gallery_Hang_E", (1200.0, 2800.0, 310.0), 45.0, 320.0, TUNG, None),
    # stock room: a bare bulb over the packing bench, another over the crate being unpacked in its straw
    ("rooms", "Stock_Bench", (4300.0, 2740.0, 260.0), 60.0, 360.0, BULB, None),
    ("rooms", "Stock_Crate", (3100.0, 2620.0, 330.0), 70.0, 450.0, BULB, None),
    # the director's office: his desk lamp, a sconce by the door
    ("rooms", "Director_Desk", (-100.0, 2640.0, F2 + 115.0), 80.0, 400.0, TUNG, None),
    ("rooms", "Director_Sconce", (-920.0, 2460.0, F2 + 220.0), 40.0, 300.0, TUNG, None),
    # projection: the projectors' lamp-house spill
    ("rooms", "Projection_Spill", (1700.0, 2200.0, F2 + 170.0), 70.0, 600.0, (1.0, 0.85, 0.65), None),
    # archive: the clerk's desk lamp, a bulb over the stacks
    ("rooms", "Archive_Desk", (2734.0, 2172.0, F2 + 100.0), 70.0, 350.0, TUNG, None),
    ("rooms", "Archive_Stacks", (3500.0, 2460.0, F2 + 330.0), 60.0, 500.0, BULB, None),
    # the vault corridor's guard desk and the vault's own desk lamp
    ("rooms", "Vaultcor_Desk", (4552.0, -1650.0, 125.0), 60.0, 300.0, TUNG, None),
    ("rooms", "Vault_Desk", (4627.0, -80.0, 115.0), 60.0, 300.0, TUNG, None),
]
# The street: under each new globe (front.py's lamps stand where the old Lamp_Post boxes were), a spot aimed out over
# the road and the parked sedans, not back onto the pavement where the queue walks. And a shop-front glow on the south
# pavement's dressing, against the facade.
STREET_SPOT = dict(dx=-30.0, z=395.0, cd=650.0, radius=1000.0, pitch=-38.0, yaw=180.0, cone=50.0, colour=WORK)   # W3: was 240 cd, pitch -55 (over the roofs)
STREET_EXTRA = [
    # W3 lights: the south pavement read black in play: the newsstand's bulb under its awning, a shop-front glow
    ("street", "South_Pavement", (-3300.0, -1500.0, 260.0), 110.0, 520.0, TUNG, None),
    ("street", "Newsstand_Bulb", (-3640.0, -1500.0, 235.0), 120.0, 420.0, BULB, None),
    ("street", "South_Shopfront", (-3330.0, -2050.0, 380.0), 220.0, 900.0, TUNG, (-55.0, 200.0, 45.0)),
    # W3 lights: the far lane's sedans (x -5400) in moonlight from over the road's middle, cool against the lamps
    ("street", "Moon_FarLane_S", (-4750.0, -400.0, 650.0), 260.0, 1100.0, MOON, (-48.0, 180.0, 32.0)),
    ("street", "Moon_FarLane_N", (-4750.0, 600.0, 650.0), 260.0, 1100.0, MOON, (-48.0, 180.0, 32.0)),
]


# --- W3 lights: the great hall, from tools/sets/hall_zones.json (agent stage's) and what hall.py placed, at run time.
# All plain unattached Point/SpotLights inside the house, so AHouseDimmer takes them down with the house lights.
# Nothing lands a pool on the rows, the centre aisle, the front floor or within MARK_CLEAR of a mark: lamps sit on the
# bar, the phone desk and the walls (sconces wash the wallpaper), the band gets a stage spot on the bandstand.
HALL = dict(bar_cd=150.0, bar_r=380.0, phone_cd=130.0, phone_r=330.0, band_cd=650.0, band_cone=34.0, band_r=1300.0,
            tv_cd=260.0, sconce_cd=75.0, sconce_r=330.0, sconce_z=300.0, sconce_every=950.0, mark_clear=320.0)
HX0, HX1, HY0, HY1 = -1000.0, 4400.0, -2000.0, 2000.0   # the hall's shell (build_auction), not its layout


def _bounds(pat):
    try:
        return C.bounds(pat)
    except Exception:                                           # noqa: BLE001
        return None


def hall_lights():
    import json, os
    with open(os.path.join(os.path.dirname(os.path.abspath(C.__file__)), "hall_zones.json"), encoding="utf-8") as f:
        Z = json.load(f)
    fz, h, out = float(Z.get("floor_z", 0.0)), HALL, []
    marks = [(a.get_actor_location().x, a.get_actor_location().y) for a in C.find(r"Mark_\w+")]
    clear = lambda x, y: all((x - mx) ** 2 + (y - my) ** 2 > h["mark_clear"] ** 2 for mx, my in marks)  # noqa: E731
    # the bar: two back-bar lamps over the bar's back edge (the wall side), lighting the bottles, the barman, the top
    b = _bounds(r"Set_Hall_Bar_Body_\d+")
    if b:
        back = b[2] - 35.0 if (b[2] + b[3]) < 0 else b[3] + 35.0
        for i, fx in enumerate((0.25, 0.75)):
            out.append(("hall", "Bar_%d" % i, (b[0] + fx * (b[1] - b[0]), back, b[5] + 120.0), h["bar_cd"], h["bar_r"],
                        TUNG, None))
    # the phone desk: a green-shaded desk lamp's worth over the baize
    b = _bounds(r"Set_Hall_Desk_Baize_\d+")
    if b:
        out.append(("hall", "Phones", ((b[0] + b[1]) / 2, (b[2] + b[3]) / 2, b[5] + 110.0), h["phone_cd"], h["phone_r"],
                    TUNG, None))
    # the band: a stage spot from in front and above, on the members' centroid (hall_band.json)
    try:
        with open(os.path.join(os.path.dirname(os.path.abspath(C.__file__)), "hall_band.json"), encoding="utf-8") as f:
            band = [v for k, v in json.load(f).items() if isinstance(v, dict)]
    except Exception:                                           # noqa: BLE001
        band = []
    if band:
        bx, by = sum(m["x"] for m in band) / len(band), sum(m["y"] for m in band) / len(band)
        bz = sum(m["z"] for m in band) / len(band) + 110.0
        cx, cy = (HX0 + HX1) / 2, 0.0                               # come in from the room's side, 4 m out, 5 m up
        d = max(((cx - bx) ** 2 + (cy - by) ** 2) ** 0.5, 1.0)
        sx, sy, sz = bx + (cx - bx) / d * 400.0, by + (cy - by) / d * 400.0, bz + 450.0
        yaw = math.degrees(math.atan2(by - sy, bx - sx))
        pitch = -math.degrees(math.atan2(sz - bz, ((bx - sx) ** 2 + (by - sy) ** 2) ** 0.5))
        out.append(("hall", "Band_Spot", (sx, sy, sz), h["band_cd"], h["band_r"], TUNG, (pitch, yaw, h["band_cone"])))
    # the TV riser: a cool studio key from above its road case (the broadcast's own light, not the house's)
    b = _bounds(r"Set_Hall_Roadcase_\d+")
    if b:
        out.append(("hall", "TV_Key", ((b[0] + b[1]) / 2, (b[2] + b[3]) / 2, b[5] + 330.0), h["tv_cd"], 520.0,
                    (0.85, 0.9, 1.0), (-90.0, 0.0, 40.0)))
    # sconces on the long walls, from the back of the rows to the front floor, clear of marks
    r = Z.get("rows", {})
    x, x1 = float(r.get("x0", 0.0)) - 1200.0, float(Z.get("front_floor", {}).get("max", [HX1 - 800.0])[0])
    i = 0
    while x <= x1:
        for wy in (HY0 + 30.0, HY1 - 30.0):
            if clear(x, wy):
                out.append(("hall", "Sconce_%d" % i, (x, wy, fz + h["sconce_z"]), h["sconce_cd"], h["sconce_r"], TUNG,
                            None))
                i += 1
        x += h["sconce_every"]
    return out


# --- W3 lights: the reception desk moved onto the way in (opening's QueueLine clerk_point): the clerk is the first
# person to speak to Julian, so he gets a key from above the desk and a banker's lamp on it. From the placed actor.
DESK_KEY = dict(cd=380.0, cone=30.0, radius=520.0, up=330.0, back=110.0)
# --- W3 lights: Maya's board and the projector stay readable with the house down: shielded work lights ATTACHED to the
# house dimmer (AHouseDimmer takes only unattached lights), straight down, tight, no haze: unseen from the floor.
WORK_LIGHTS = dict(board_cd=140.0, board_cone=38.0, board_up=200.0, proj_cd=40.0, proj_radius=200.0)


def desk_lights():
    q = [a for a in eas.get_all_level_actors() if a.get_class().get_name() == "QueueLine"]
    p = q[0].get_editor_property("clerk_point") if q else None
    cx, cy = (p.x, p.y) if p else (-2050.0, 630.0)
    k = DESK_KEY
    sx, sz = cx - k["back"], k["up"]                                # over the desk, in front of him, aimed back at him
    pitch = -math.degrees(math.atan2(sz - 150.0, k["back"]))
    return [("lobby", "Clerk_Key", (sx, cy, sz), k["cd"], k["radius"], TUNG, (pitch, 0.0, k["cone"])),
            ("lobby", "Desk_Lamp", (cx - 150.0, cy + 70.0, 150.0), 110.0, 340.0, TUNG, None)]


def work_lights():
    import json, os
    dims = [a for a in eas.get_all_level_actors() if a.get_class().get_name() == "HouseDimmer"]
    if not dims:
        return []
    with open(os.path.join(os.path.dirname(os.path.abspath(C.__file__)), "hall_zones.json"), encoding="utf-8") as f:
        Z = json.load(f)
    w, d, m, pr = WORK_LIGHTS, Z["dimmer"], Z["maya_post"], Z["projector"]
    out = [light("hall", "Work_Board", ((d[0] + m[0]) / 2, (d[1] + m[1]) / 2, d[2] + w["board_up"]), w["board_cd"],
                 w["board_up"] * 2.0, WORK, (-90.0, 0.0, w["board_cone"])),
           light("hall", "Work_Projector", (pr[0], pr[1], pr[2] + 70.0), w["proj_cd"], w["proj_radius"], WORK, None)]
    for a in out:
        a.attach_to_actor(dims[0], "", unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD,
                          unreal.AttachmentRule.KEEP_WORLD, False)
    return [a.get_actor_label() for a in out]


def light(area, name, p, cd, radius, colour, spot):
    cls = unreal.SpotLight if spot else unreal.PointLight
    r = unreal.Rotator(roll=0.0, pitch=spot[0], yaw=spot[1]) if spot else unreal.Rotator(0.0, 0.0, 0.0)
    a = eas.spawn_actor_from_class(cls, unreal.Vector(*p), r)
    a.set_actor_label("Set_Light_%s_%s" % (area.capitalize(), name))
    a.set_folder_path("Sets/" + TAG)
    a.set_editor_property("tags", [unreal.Name(TAG)])
    c = a.get_component_by_class(unreal.SpotLightComponent if spot else unreal.PointLightComponent)
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    c.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    c.set_editor_property("intensity", float(cd) * SCALE * AREA_SCALE.get(area, 1.0))
    c.set_editor_property("attenuation_radius", float(radius) * (1.2 if AREA_SCALE.get(area, 1.0) > 1.0 else 1.0))
    c.set_editor_property("cast_shadows", False)
    c.set_editor_property("volumetric_scattering_intensity", 0.0)
    c.set_editor_property("light_color", unreal.Color(r=int(colour[0] * 255), g=int(colour[1] * 255),
                                                      b=int(colour[2] * 255), a=255))
    if spot:
        c.set_editor_property("outer_cone_angle", float(spot[2]))
        c.set_editor_property("inner_cone_angle", float(spot[2]) * 0.6)
    return a


gone, _ = C.clear(TAG)
placed = []
todo = [e for e in LIGHTS if e[0] in AREAS]
if "street" in AREAS:
    ys = sorted(set(round(a.get_actor_location().y) for a in C.find(r"Lamp_Post_\d+")))
    s = STREET_SPOT
    todo += [("street", "Lamp_%d" % i, (PAVE_X0 + 40.0 + s["dx"], float(y), s["z"]), s["cd"], s["radius"], s["colour"],
              (s["pitch"], s["yaw"], s["cone"])) for i, y in enumerate(ys)]
    todo += STREET_EXTRA
    if MOVE_LAMPS:   # W3 lights: the old lamp heads hung at PAVE_X0 - 30, z 450; the globes are at PAVE_X0 + 40
        for a in eas.get_all_level_actors():
            if isinstance(a, unreal.PointLight) and re.match(r"Lamp_\d+$", a.get_actor_label()):
                p = a.get_actor_location()
                if PAVE_X0 - 200.0 < p.x < PAVE_X0 + 100.0 and 350.0 < p.z < 480.0 and abs(p.y) > 500.0:
                    a.set_actor_location(unreal.Vector(PAVE_X0 + 40.0, p.y, MOVE_LAMPS["z"]), False, False)
                    a.get_component_by_class(unreal.PointLightComponent).set_editor_property("intensity", MOVE_LAMPS["cd"])
                    placed.append(a.get_actor_label() + " moved")
if "lobby" in AREAS:
    todo += desk_lights()
if "hall" in AREAS:
    todo += hall_lights()
for e in todo:
    light(*e)
    placed.append(e[1])
if "hall" in AREAS:
    placed += work_lights()
C.save_level()
"lights: removed %d, placed %d (%s)" % (gone, len(placed), ", ".join(placed))
