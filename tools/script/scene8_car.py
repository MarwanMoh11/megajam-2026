"""Scene 8 (s8-alley): the getaway GT car from Hyper3D Rodin (tools/rodin/props_s8.json, prop s8_gtcar).

Two steps, both re-runnable:
  1. outside the editor (engine Python, stdlib only) writes C:\\Jam\\Art\\Rodin\\s8_gtcar\\S8_GTCar.glb: the Rodin GLB with
     its one node turned, scaled and moved so that, imported, the nose points +X, the tyres sit on Z=0 and the pivot is
     on the ground halfway between the axles, about 440 cm long:
       "<engine python>" -I tools/script/scene8_car.py
  2. inside the editor imports that GLB to /Game/Take/Script/S8 (replace_existing and save on purpose: a re-run
     replaces the asset, and the Preview engine can crash), names the mesh SM_S8_GTCar, gives it one box collision:
       "<engine python>" -I tools/ue_mcp.py py tools/script/scene8_car.py
glTF is Y-up with the car's nose on +Z; Interchange maps glTF (x, y, z) to Unreal (x, z, y) and metres to cm.
"""
import json
import math
import os
import struct
import sys

SRC_DIR = r"C:\Jam\Art\Rodin\s8_gtcar"
SRC = os.path.join(SRC_DIR, "base_basic_pbr.glb")
OUT = os.path.join(SRC_DIR, "S8_GTCar.glb")
LENGTH_CM = 440.0
FOLDER = "/Game/Take/Script/S8"
MESH = FOLDER + "/SM_S8_GTCar"


def prep():
    d = open(SRC, "rb").read()
    ln = struct.unpack("<I", d[12:16])[0]
    g = json.loads(d[20:20 + ln])
    rest = d[20 + ln:]                                          # the BIN chunk, unchanged
    a = g["accessors"][g["meshes"][0]["primitives"][0]["attributes"]["POSITION"]]
    bv = g["bufferViews"][a["bufferView"]]
    off = 20 + ln + 8 + bv.get("byteOffset", 0) + a.get("byteOffset", 0)
    st = bv.get("byteStride", 12)
    P = [struct.unpack_from("<3f", d, off + i * st) for i in range(a["count"])]
    lo, hi = a["min"], a["max"]
    s = LENGTH_CM / 100.0 / (hi[2] - lo[2])                    # metres in glTF
    # the axles: tyre contact points (lowest 2 cm) split into rear (z < mid) and front clusters
    ground = [p for p in P if p[1] < lo[1] + 0.02]
    mid = (lo[2] + hi[2]) / 2
    rear = [p[2] for p in ground if p[2] < mid]
    front = [p[2] for p in ground if p[2] >= mid]
    axle_mid = (sum(rear) / len(rear) + sum(front) / len(front)) / 2 if rear and front else mid
    cx = (lo[0] + hi[0]) / 2
    # rotate +90 about glTF Y (x' = z, z' = -x): the nose (+z) goes to glTF +x = Unreal +X; Unreal Y = -glTF x
    q = [0.0, math.sqrt(0.5), 0.0, math.sqrt(0.5)]
    t = [-axle_mid * s, -lo[1] * s, cx * s]                     # after rotate+scale: x' = s z, y' = s y, z' = -s x
    node = g["nodes"][0]
    node.pop("matrix", None)
    node.update({"name": "S8_GTCar", "rotation": q, "scale": [s, s, s], "translation": t})
    g["meshes"][0]["name"] = "S8_GTCar"
    for m in g.get("materials", []):
        m["name"] = "S8_GTCar"
    js = json.dumps(g, separators=(",", ":")).encode("utf-8")
    js += b" " * (-len(js) % 4)
    body = struct.pack("<II", len(js), 0x4E4F534A) + js + rest
    with open(OUT, "wb") as f:
        f.write(struct.pack("<III", 0x46546C67, 2, 12 + len(body)) + body)
    print("wrote %s: scale %.4f, axle mid z %.3f m (bbox mid %.3f), expect %.0f x %.0f x %.0f cm" % (
        OUT, s, axle_mid, mid, (hi[2] - lo[2]) * s * 100, (hi[0] - lo[0]) * s * 100, (hi[1] - lo[1]) * s * 100))


def tris(m):
    try:
        return m.get_num_triangles(0)
    except Exception:                                           # noqa: BLE001
        return -1


def editor_import():
    eal = unreal.EditorAssetLibrary
    before = set(eal.list_assets(FOLDER, recursive=True)) if eal.does_directory_exist(FOLDER) else set()
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", OUT)
    t.set_editor_property("destination_path", FOLDER)
    t.set_editor_property("replace_existing", True)            # on purpose: a re-run replaces the last import
    t.set_editor_property("automated", True)
    t.set_editor_property("save", True)                        # on purpose: saved as it lands
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
    paths = list(t.get_editor_property("imported_object_paths") or [])
    paths = paths or sorted(set(eal.list_assets(FOLDER, recursive=True)) - before)
    found = [eal.load_asset(p) for p in paths]
    meshes = [m for m in found if isinstance(m, unreal.StaticMesh)]
    if not meshes:
        print("no static mesh imported:", paths)
        return
    m = meshes[0]
    if m.get_path_name().split(".")[0] != MESH:
        if eal.does_asset_exist(MESH):
            eal.delete_asset(MESH)
        eal.rename_asset(m.get_path_name().split(".")[0], MESH)
        m = eal.load_asset(MESH)
    try:
        lib = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    except Exception:                                           # noqa: BLE001
        lib = unreal.EditorStaticMeshLibrary
    lib.remove_collisions(m)
    lib.add_simple_collisions(m, unreal.ScriptCollisionShapeType.BOX)
    eal.save_asset(MESH, only_if_is_dirty=False)
    b = m.get_bounding_box()
    bs = m.get_bounds()
    print("imported:", [p.split(".")[0] for p in paths])
    print("%s: min (%.1f, %.1f, %.1f) max (%.1f, %.1f, %.1f); origin (%.1f, %.1f, %.1f) extent (%.1f, %.1f, %.1f); "
          "tris %d; materials %s" % (MESH, b.min.x, b.min.y, b.min.z, b.max.x, b.max.y, b.max.z,
                                      bs.origin.x, bs.origin.y, bs.origin.z, bs.box_extent.x, bs.box_extent.y,
                                      bs.box_extent.z, tris(m),
                                      [s.material_interface.get_path_name() if s.material_interface else None
                                       for s in m.static_materials]))


# Inside the editor `unreal` is already loaded (its runner allows only top-level import statements).
unreal = sys.modules.get("unreal")

if unreal is None:
    prep()
else:
    editor_import()
