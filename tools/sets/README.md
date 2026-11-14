# Set overlays (tools/sets)

Each set package dresses its area in `tools/sets/<area>.py` over the level `build_auction.py` (+ dress, cast, Rodin
props) builds. `run_all.py` re-applies every overlay (front, hall, rooms, devices, `tools/build_heroes.py`);
`build_auction.py` runs it last. Every room's box, shots and defect labels: `C:\Jam\Builds\qa\survey\index.md`.

## Running an overlay (always under the editor lock)
```bash
PY="/c/Epic Games/UE_5.9/Engine/Binaries/ThirdParty/Python3/Win64/python.exe"
"$PY" -I tools/editor_lock.py acquire hall --wait 540            # Bash timeout 600000
"$PY" -I tools/ue_mcp.py py tools/sets/hall.py 110               # returns the script's last expression
"$PY" -I tools/editor_lock.py release hall
```
The MCP Python tool wants every `import` at the top level (not in a def or try) and gives up at ~110 s: split long
work. `print` goes to the editor log; end with an expression (a string) to see a result. Be idempotent:
```python
import sys, importlib; sys.path.insert(0, r"C:\Jam\megajam\tools\sets"); import common as C; importlib.reload(C)
TAG = "set_hall"; C.clear(TAG)                                  # deletes this tag's actors, shows what it hid
C.hide([r"Bench_\d+", r"Prop_Chair_Instances"], TAG)   # visibility only: collision, sight, navigation stay
m = C.import_prop("hall_chair", look="chair")  # C:\Jam\Art\Rodin\hall_chair -> /Game/Take/Props/Hall_chair/SM_Hall_chair
C.scatter(m, [((x, 900, 0), 180, 1.0) for x in range(-600, 600, 62)], tag=TAG, collide=True)
C.place(m, (100, 160, 0, 60, C.top_of("Cocktail_Table_3"), 300), yaw=90, tag=TAG)
C.save_level()
```

## Helper API (`common.py`)
- `clear(tag)`; `hide(patterns, tag)` (regexes on the whole label); `unhide(tag)`.
- `spawn(mesh, loc, rot=0, scale=1, tag=, label=, folder=, collide=False, shadow=True, mat=None)`: a StaticMeshActor.
  `rot` is a yaw or `(roll, pitch, yaw)`. `collide=True` blocks the pawn only (no sight line or camera answers it).
  Labels become `Set_<label>`, never `Prop_` (import_props' unplace deletes every `Prop_*` actor). Folder `Sets/<tag>`.
- `scatter(mesh, [(loc, rot, scale)], tag=...)`: repeats on one instanced component (use it for anything repeated).
- `import_prop(name, look=, reimport=False)`: import_props' import and M_Prop material instance; `look` borrows a LOOK
  key (`cocktail`/`bar` wood, `column`/`pedestal` stone, `stanchion`/`frame` steel, `chair`/`sofa` velvet, `piano`).
- `fit(mesh, box, yaw, mode)` -> `(loc, rot, scale)`; `place(mesh, box, yaw, mode, **spawn_kw)`. Box `(x0, x1, y0, y1,
  z0, z1)`, mesh on z0, centred; modes fit, fitxy, stretch, stretchz. Check a Rodin mesh's front (import_props `FRONT`).
- `bounds(actor_or_label)` -> `[x0, x1, y0, y1, z0, z1]` (union of matches); `top_of(label)` (z to stand on);
  `find(pattern)` -> actors; `near(x, y, r, z=None)` -> labels near a point (to find culprits).
- `shot(name, pos, look_at, fov=70, w=1280, h=720, out_dir=, ev=0, unlit=False)`: a SceneCapture to PNG in one call,
  final colour with the level's post-process; the editor viewport is untouched. `ev=3` lifts a dark room.
- `plan(name, x0, x1, y0, y1, z_cut, px=1600, out_dir=, unlit=False)`: orthographic top-down; actors whose bottom is
  above `z_cut` are left out. **Image right = +X, image down = +Y**: pixel (u, v) = world (x0 + u*s, y0 + v*s).
- `save_level()`. Shots default to `C:\Jam\Builds\qa\shots`: use `C:\Jam\Builds\qa\<package>\`. Read the PNG yourself.

## Level conventions (cm; from the top of build_auction.py)
X runs west (street) to east (dais); +Y is north; Z up, ground floor top 0, second floor top `F2` 450 (slab 430-450),
two-storey walls to 900, hall ceiling 1400, walls 20 thick, doorways 140 x 250.
| Area | x0..x1 | y0..y1 | z |
|---|---|---|---|
| Road / pavement (queue) | -5500..-4100 / -4100..-3200 | -2400..2400 | 0 |
| Terrace (Maya, searchlight) | -3450..-3220 | -600..600 | 450 |
| Lobby (atrium -2700..-1500 x -800..800) | -3200..-1000 | -1400..1400 | 0, 450 |
| Cloakroom / office above | -3200..-2100 | 1400..2000 | 0 / 450 |
| Restrooms / security room above | -3200..-2100 | -2000..-1400 | 0 / 450 |
| Great hall (gallery to x -600; dais x 3600+, top 20; band riser 3640..3920 x 1320..1600) | -1000..4400 | -2000..2000 | 0..1400 |
| Preview gallery / stock room | -1000..2300 / 2300..4400 | 2020..2900 | 0 |
| Director's office / projection / archive | -1000..800 / 800..2600 / 2600..4400 | 2020..2900 | 450 |
| Backstage corridor | -1000..4400 | -2600..-2020 | 0 (h 500) |
| Kitchen / prop store / loading dock | -1000..800 / 800..2600 / 2600..4400 | -3300..-2620 | 0 |
| Vault corridor and vault | 4400..5300 | -2000..2000 (vault -1000..1000) | 0 |
Labels: boxes `<Name>_<n>`, Rodin `Prop_*`, dressing `Dress_*`, overlays `Set_*`. Never hide or move gameplay actors.
