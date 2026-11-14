# QA look (Nov 14), the lighting pass: motivated practicals at the dressing, so the new props read in play (notes 11, 14).
# An idempotent overlay (tools/sets/README.md): it removes what its last run placed (tag set_lights) and places it again.
#     "<engine python>" -I tools/ue_mcp.py py tools/sets/lights.py 110
# Rules it keeps (sight's cue language, Mark.h / RedTapeZone.h / PatrolGuard.h): a warm pool on the floor means a safe
# spot (260-650 cd), cool light forbidden ground (260 cd), a guard's torch pool that he is looking (900 cd). So these are
# weaker and sit on walls, props and faces: small radii, high or tight to the object, no new pool on Julian's routes
# (the queue, the lobby's centre, the hall's centre aisle). No shadows, movable, no haze. The hall's lights are plain
# unattached PointLights inside AHouseDimmer's box (-1000..4400, -2000..2000), so they go down with the house.
# Switches: LIGHTS_AREAS (a wrapper global) runs part of it; SCALE scales every light; an entry's candelas is its own.
import sys, importlib; sys.path.insert(0, r"C:\Jam\megajam\tools\sets"); import common as C; importlib.reload(C)
import re

import unreal

TAG = "set_lights"
AREAS = globals().get("LIGHTS_AREAS") or ("street", "lobby", "hall", "rooms")
SCALE = 1.0                         # every light's candelas x this
F2 = 450.0
PAVE_X0 = -4100.0                   # build_auction's LX0 - 900: the street lamps stand at PAVE_X0 + 40 (front.py)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

TUNG = (1.0, 0.72, 0.45)            # shaded lamps, sconces
BULB = (1.0, 0.80, 0.58)            # bare bulbs
WORK = (1.0, 0.92, 0.80)            # work lamps, the street's lamps on the road
EMBER = (1.0, 0.55, 0.30)           # the ranges, the switchboard's lamps

# area, name, (x, y, z), candelas, radius, colour, and for a spot (pitch, yaw, outer cone). The story it lights.
LIGHTS = [
    # the lobby: the reception desk's banker's lamp, the two seating groups' lamps (off the centre)
    ("lobby", "Desk_Lamp", (-2196.0, 1060.0, 150.0), 60.0, 300.0, TUNG, None),
    ("lobby", "Seating_W", (-3080.0, 1180.0, 165.0), 55.0, 320.0, TUNG, None),
    ("lobby", "Seating_E", (-1290.0, 1110.0, 125.0), 45.0, 260.0, TUNG, None),
    # the hall: the bar's back-bar lamps, the band's stand lamps (north of the dais, away from the aisle)
    ("hall", "Bar_W", (950.0, -1570.0, 200.0), 70.0, 320.0, TUNG, None),
    ("hall", "Bar_E", (1450.0, -1570.0, 200.0), 70.0, 320.0, TUNG, None),
    ("hall", "Band_W", (3800.0, 1520.0, 190.0), 60.0, 300.0, TUNG, None),
    ("hall", "Band_E", (3960.0, 1680.0, 190.0), 60.0, 300.0, TUNG, None),
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
STREET_SPOT = dict(dx=-30.0, z=395.0, cd=240.0, radius=900.0, pitch=-55.0, yaw=180.0, cone=55.0, colour=WORK)
STREET_EXTRA = [("street", "South_Pavement", (-3300.0, -1500.0, 260.0), 60.0, 420.0, TUNG, None)]


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
    c.set_editor_property("intensity", float(cd) * SCALE)
    c.set_editor_property("attenuation_radius", float(radius))
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
for e in todo:
    light(*e)
    placed.append(e[1])
C.save_level()
"lights: removed %d, placed %d (%s)" % (gone, len(placed), ", ".join(placed))
