# The great hall's layout in numbers (cm), shared by tools/sets/hall.py (inside the editor) and by anyone who needs the
# seats or the band without the editor. Plain Python, no unreal. Run it alone to (re)write the two contract files:
#     "<engine python>" -I tools/sets/hall_layout.py
# hall_seats.json: [{x, y, z, yaw}] where z is the seat's top and yaw the way the sitter faces (0 = +X, the dais).
# hall_band.json: {member: {x, y, z, yaw, seated}, "props": true}: z is the seat's top when seated, else the floor.
# Package cast seats the guests and places the band from these (and then builds no shape chairs or instruments).
import json
import os
import random

HERE = os.path.dirname(os.path.abspath(__file__))

# --- the audience (W3 stage): two blocks of rows facing the dais either side of the centre aisle, one chair over each
# build_auction Bench box (hidden, it keeps the collision). The reserved block starts 4.5 m off the dais; the general
# block sits behind the cross aisle at the rope. tools/sets/hall_zones.json has the same numbers.
ROWS = (                                                   # (row x centres, chairs a side): build_auction ROWS_*
    tuple(3150.0 - 100.0 * i for i in range(8)),           # reserved: 3150 .. 2450, 14 a side
    tuple(2100.0 - 100.0 * i for i in range(14)),          # general: 2100 .. 800, 12 a side
)
PER_SIDE = (14, 12)
ROW_Y0, PITCH = 180.0, 62.0                                # the aisle edge, 62 cm a chair
OFFER = ((3, 0.95), (5, 0.3), (7, 0.08), (7, 0.3))                # (rows from the front of the room, share of their seats offered)
CHAIR_H = 92.0                                             # the gilt ballroom chair stands 92 cm
SEAT_FRAC = 83.0 / 171.2                                   # its seat's top over its height (measured off the mesh)
SEAT_Z = round(CHAIR_H * SEAT_FRAC, 1)                     # 44.6
LEGS = ()                                                  # the catwalk hangs from the roof now: no legs in the rows
JULIAN_SEAT = (2100.0, -211.0)                             # his bidder's seat: kept free for him (hall_zones julian_place)
PROPS_ON_SEATS = 14                                        # empty seats with a catalogue or a paddle: not offered to cast
# The phone bids (chairs facing the desk, north) and the bar stools (facing the bar, south).
PHONE_SEATS = ((1880.0, -1640.0), (2000.0, -1640.0), (2120.0, -1640.0))
STOOLS = tuple((x, -1420.0) for x in (900.0, 1050.0, 1200.0, 1350.0, 1500.0))
STOOL_Z = 70.0                                             # build_auction stool(): the Rodin stool's top

# --- the bandstand: the old riser (x 3640-3920, y 1320-1600, top 30) grown north and east, facing the house (west) ---
RISER_Z = 30.0
STAND = (3640.0, 3990.0, 1320.0, 1860.0)                   # the whole bandstand's footprint, top at RISER_Z
DRUM_RISER = (3800.0, 3990.0, 1600.0, 1800.0, 55.0)        # x0, x1, y0, y1, top: the drums one step up at the back
PIANO = (3790.0, 1480.0, 180.0)                            # the grand's centre and yaw: lid open to the house, keys south
PIANO_BENCH = (3790.0, 1358.0, 50.0)                       # x, y, seat height over the riser
DRUM_STOOL = (3945.0, 1700.0, 50.0)
BAND = {                                                   # member: x, y, floor or seat top, yaw, seated
    "BAND_PIANO": (PIANO_BENCH[0], PIANO_BENCH[1], RISER_Z + PIANO_BENCH[2], 90.0, True),
    "BAND_DRUMS": (DRUM_STOOL[0], DRUM_STOOL[1], DRUM_RISER[4] + DRUM_STOOL[2], 180.0, True),
    "BAND_BASS": (3760.0, 1795.0, RISER_Z, 200.0, False),
    "BAND_TRUMPET": (3700.0, 1700.0, RISER_Z, 195.0, False),
    "BAND_SINGER": (3690.0, 1598.0, RISER_Z, 205.0, False),
}
HELD = ("BAND_TRUMPET",)                                   # no prop stands for these: cast keeps the held instrument


def chair_seats():
    """[(x, y, yaw)] of every audience chair, front row first."""
    out = []
    for rows, n in zip(ROWS, PER_SIDE):
        for bx in rows:
            for s in (1.0, -1.0):
                for i in range(n):
                    y = s * (ROW_Y0 + PITCH * (i + 0.5))
                    if any(abs(bx - lx) < 40.0 and abs(y - ly) < 36.0 for (lx, ly) in LEGS):
                        continue
                    out.append((bx, y, 0.0))
    return out


def offered(i, x, y):
    """Whether chair i goes to cast: the front rows full, thinning toward the back (frame time: about 170 offered)."""
    if abs(x - JULIAN_SEAT[0]) < 1.0 and abs(y - JULIAN_SEAT[1]) < 1.0:
        return False
    order = sorted({cx for (cx, _cy, _cyaw) in chair_seats()}, reverse=True).index(x)
    share, k = 0.0, 0
    for (n, sh) in OFFER:
        if order < k + n:
            share = sh
            break
        k += n
    return random.Random(i * 7919 + 41).random() < share


def seats_with_props():
    """The empty audience seats that carry a catalogue or a paddle (the same ones on every run)."""
    seats = chair_seats()
    rnd = random.Random(41)
    return sorted(rnd.sample(range(len(seats)), PROPS_ON_SEATS))


def seats_json():
    taken = set(seats_with_props())
    out = [dict(x=x, y=y, z=SEAT_Z, yaw=yaw) for i, (x, y, yaw) in enumerate(chair_seats())
           if i not in taken and offered(i, x, y)]
    out += [dict(x=x, y=y, z=SEAT_Z, yaw=90.0) for (x, y) in PHONE_SEATS]
    out += [dict(x=x, y=y, z=STOOL_Z, yaw=270.0) for (x, y) in STOOLS]
    return out


def band_json():
    out = {k: dict(x=v[0], y=v[1], z=v[2], yaw=v[3], seated=v[4]) for k, v in BAND.items()}
    for k in HELD:
        out[k]["held"] = True          # his own instrument stays in his hands (no prop stands for it)
    out["props"] = True
    return out


def write():
    with open(os.path.join(HERE, "hall_seats.json"), "w", encoding="utf-8") as f:
        json.dump(seats_json(), f, indent=1)
    with open(os.path.join(HERE, "hall_band.json"), "w", encoding="utf-8") as f:
        json.dump(band_json(), f, indent=1)
    return "hall_seats.json %d seats, hall_band.json %d members" % (len(seats_json()), len(BAND))


if __name__ == "__main__":
    print(write())
