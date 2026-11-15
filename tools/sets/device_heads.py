# Interactable props (Nov 15): split the searchlight and the follow spot into a fixed base, a part that pans (the yoke)
# and a part that tilts (the drum or barrel), so the lamp head aims at its target while the stand stays put.
# Run in the editor (under the editor lock):  "$PY" -I tools/ue_mcp.py py tools/sets/device_heads.py 110
# Each part is a copy of the imported Rodin mesh (same material, same mesh-local frame, so one transform places all
# three) with the other parts' triangles deleted. Which part a triangle goes to is decided by its centroid, in the
# mesh's local cm (front +X for the searchlight, +Y for the follow spot; see PARTS). Idempotent: re-running rebuilds.
import math

import unreal

eal = unreal.EditorAssetLibrary
ROOT = "/Game/Take/Props"


def sl_part(c):
    """Searchlight: the trailer (chassis, wheels, tow bar: not used on the rail mount); the turntable (the base); the
    yoke (a U under the drum, in the drum's middle slice, and the post); the drum (its axis along X, the trunnion axis
    along Y through (y -30, z 44))."""
    x, y, z = c
    if z < -16.5:
        return "Base" if z > -35.0 and math.hypot(x, y + 30.0) < 49.0 else "Trailer"
    r = math.hypot(y + 30.0, z - 44.0)
    if z < 4.0 or (abs(x) < 12.0 and r > 44.5 and z < 52.0):
        return "Yoke"
    return "Drum"


def fs_part(c):
    """Follow spot: the tripod and post; the bracket at the post's top (it pans); the barrel (lens +Y, tail handle
    -Y) above the trunnion at (y 42, z 32)."""
    if c[2] < 24.0:
        return "Base"
    return "Yoke" if c[2] < 37.0 and abs(c[1] - 42.0) < 14.0 and abs(c[0]) < 14.0 else "Drum"


PARTS = {
    "Devices_searchlight": (sl_part, ("Trailer", "Base", "Yoke", "Drum")),
    "Devices_followspot": (fs_part, ("Base", "Yoke", "Drum")),
}


def split(folder):
    fn, names = PARTS[folder]
    src = "%s/%s/SM_%s" % (ROOT, folder, folder)
    m = eal.load_asset(src)
    md = m.get_static_mesh_description(0)
    n = md.get_polygon_count()
    pos = {}
    owner = []
    for i in range(n):
        pid = unreal.PolygonID(i)
        vs = md.get_polygon_vertices(pid)
        acc = [0.0, 0.0, 0.0]
        for v in vs:
            k = v.id_value
            p = pos.get(k)
            if p is None:
                q = md.get_vertex_position(v)
                p = pos[k] = (q.x, q.y, q.z)
            acc[0] += p[0]
            acc[1] += p[1]
            acc[2] += p[2]
        k = float(max(len(vs), 1))
        owner.append(fn((acc[0] / k, acc[1] / k, acc[2] / k)))
    out = []
    for part in names:
        dst = "%s_%s" % (src, part)
        if eal.does_asset_exist(dst):
            eal.delete_asset(dst)
        eal.duplicate_asset(src, dst)
        pm = eal.load_asset(dst)
        pd = pm.get_static_mesh_description(0)
        gone = 0
        for i in range(n):
            if owner[i] != part:
                pd.delete_polygon(unreal.PolygonID(i))
                gone += 1
        pm.build_from_static_mesh_descriptions([pd], False, False)
        eal.save_asset(dst, only_if_is_dirty=False)
        b = pm.get_bounding_box()
        out.append("%s: %d tris, box (%.0f..%.0f, %.0f..%.0f, %.0f..%.0f)" % (dst.split("/")[-1], n - gone, b.min.x, b.max.x,
                                                                           b.min.y, b.max.y, b.min.z, b.max.z))
    return out


RESULT = "\n".join(sum((split(f) for f in PARTS), []))
RESULT
