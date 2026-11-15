"""Package heroes (QA note 21): Julian, Maya and Vex as Rodin-generated characters skinned to the UE5 mannequin's own
skeleton (SK_Mannequin), so every template animation plays on them with no retargeting.

Per hero (skipped when its skeletal mesh was already built from the same .glb by the same VERSION of this script):
  1. import C:\\Jam\\Art\\Rodin\\heroes_<name>\\base_basic_pbr.glb -> /Game/Take/Cast/Heroes/<Name>/SM_HeroSrc_<Name>
  2. copy it into a dynamic mesh; turn it to face the mannequin's front, scale it to the mannequin's height, feet on
     the ground, centred on the legs
  3. measure both meshes' arm angle (A against T pose); pose the mannequin's arms to the generated pose with its own
     skin weights, transfer the bone weights (closest point on surface), then pose the generated arms back to the
     mannequin's reference pose with the weights they just got
  4. create /Game/Take/Cast/Heroes/<Name>/SKM_Hero_<Name> on SK_Mannequin, with the source mannequin's bone
     proportions (Quinn's for Maya) and an instance of M_Hero carrying the Rodin colour
Then the hook-up: sets ATakeCharacter.HeroMesh (package cast's property) on the level's A, B and C characters when the
property exists, and skips quietly when it does not.

Run inside the editor, under the editor lock:
  "<engine python>" -I tools/ue_mcp.py py tools/build_heroes.py 110
NPCS (Nov 15) are built the same way into /Game/Take/Cast/NPCs/<Name>/SKM_NPC_<Name> from C:\\Jam\\Art\\Rodin\\npc_<name>;
they get no hook-up here (tools/build_cast.py sets their HeroMesh), and one without a .glb yet is skipped.
A wrapper may set HEROES_ONLY = ["Julian"] (build only those) or HEROES_FORCE = True (rebuild) and exec() this file.
tools/sets/run_all.py runs it after the set overlays (it saves the level itself).
"""
import math
import os
import traceback

import unreal

VERSION = "4"                 # bump to rebuild every hero on the next run
SRC = r"C:\Jam\Art\Rodin"
DEST = "/Game/Take/Cast/Heroes"
SKELETON = "/Game/Characters/Mannequins/Meshes/SK_Mannequin"
MANNY = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
QUINN = "/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple"

# name, Rodin folder, the mannequin whose weights and bone proportions it takes, camera id, height against that
# mannequin's, a yaw (degrees) added after the automatic facing when the feet heuristic gets it wrong
HEROES = [
    dict(name="Julian", rodin="heroes_julian", source=MANNY, cam="A", height=1.0, yaw=0.0),
    dict(name="Maya", rodin="heroes_maya", source=QUINN, cam="B", height=1.0, yaw=0.0),
    dict(name="Vex", rodin="heroes_vex", source=MANNY, cam="C", height=0.98, yaw=0.0),
]
# Featured NPCs (Nov 15, prompts in tools/rodin/props_npcs.json): the same build into /Game/Take/Cast/NPCs/<Name>/
# SKM_NPC_<Name>, no hook-up here (cam None): tools/build_cast.py sets their CastMembers' HeroMesh (NPC_MESH there).
NPC_DEST = "/Game/Take/Cast/NPCs"
NPCS = [dict(name=n, rodin="npc_" + n.lower(), source=src, cam=None, height=h, yaw=0.0, dest=NPC_DEST, prefix="NPC")
        for n, src, h in (("Vance", MANNY, 1.04), ("Doorman", MANNY, 1.0), ("Clerk", MANNY, 0.97),
                          ("Bodyguard", MANNY, 1.0), ("Chief", MANNY, 0.98), ("Guard", MANNY, 1.03),
                          ("Queue", MANNY, 0.97), ("Singer", QUINN, 1.0), ("Bartender", MANNY, 1.0))]
ARM_MIN_DEG = 2.0                   # arm-angle differences below this are left alone
ARM_PASSES = 4                      # --- W3 cast2 --- residual passes after the swing back (0: off)
ARM_REACH = 12.0                    # cm outward of the shoulder joint where the arm sample starts
ROUGHNESS = 0.7
TRANSFER = "hybrid"                 # "closest": ClosestPointOnSurface everywhere. "hybrid": also InpaintWeights (copies
                                    # where the surfaces face the same way, solves the rest smooth: no fins at the
                                    # armpits), kept per vertex only where it is a sane weight set near the closest
                                    # point's bone (an island with nothing matched comes back as garbage otherwise)
INPAINT_RADIUS = -1.0               # x the mesh's bounding-box diagonal; -1: distance is no reason to inpaint
INPAINT_NORMAL = 50.0               # degrees
SANE_HOPS = 2                       # hybrid: the inpainted main bone at most this many bones from the closest point's
SMOOTH_ITERS = 4                    # Laplacian passes over the transferred weights (0: off): no seams at the armpits
SMOOTH_STRENGTH = 0.5
MAX_INFLUENCES = 6
RIGID_FINGERS = True                # finger weights folded into the hand: the generated hands' fingers do not lie
                                    # along the mannequin's, and a curling finger bone tore them (Vex, jump)


EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
GA = unreal.GeometryScript_AssetUtils
BW = unreal.GeometryScript_BoneWeights
NA = unreal.GeometryScript_NewAssetUtils
Q = unreal.GeometryScript_MeshQueries
ED = unreal.GeometryScript_MeshEdits
LS = unreal.GeometryScript_List
TR = unreal.GeometryScript_MeshTransforms
NO = unreal.GeometryScript_Normals
REPORT = []
_G = globals()
ONLY = _G.get("HEROES_ONLY")
FORCE = bool(_G.get("HEROES_FORCE", False))


def where(hero):
    return hero.get("dest", DEST)


def log(s):
    REPORT.append(s)
    unreal.log("[heroes] " + s)


def first(r, cls):
    """The first value of type cls in a Python-wrapped UFUNCTION result (a value or a tuple of out params)."""
    for v in (r if isinstance(r, tuple) else (r,)):
        if isinstance(v, cls):
            return v
    return None


# --- mesh data ----------------------------------------------------------------------------------------------------------
def positions(dm):
    r = Q.get_all_vertex_positions(dm, False)
    vl = first(r, unreal.GeometryScriptVectorList)
    arr = first(LS.convert_vector_list_to_array(vl), (list, unreal.Array))
    if arr is None:
        arr = LS.convert_vector_list_to_array(vl)
    return [(v.x, v.y, v.z) for v in arr]


def set_positions(dm, pts):
    vl = LS.convert_array_to_vector_list([unreal.Vector(*p) for p in pts])
    vl = first(vl, unreal.GeometryScriptVectorList) or vl
    ED.set_all_mesh_vertex_positions(dm, vl)


def recompute_normals(dm):
    try:
        NO.recompute_normals(dm, unreal.GeometryScriptCalculateNormalsOptions())
    except Exception as e:                                      # noqa: BLE001
        log("normals: %s" % e)


def bones(dm):
    infos = first(BW.get_all_bones_info(dm), (list, unreal.Array))
    return {str(b.name): b for b in infos}, list(infos)


def subtree(infos, root_index):
    kids = {}
    for b in infos:
        kids.setdefault(b.parent_index, []).append(b.index)
    out, todo = set(), [root_index]
    while todo:
        i = todo.pop()
        out.add(i)
        todo += kids.get(i, [])
    return out


def side_weights(dm, n, groups):
    """Per vertex, the summed weight on each set of bone indices in groups."""
    prof = unreal.GeometryScriptBoneWeightProfile()
    out = [[0.0] * n for _ in groups]
    for vid in range(n):
        r = BW.get_vertex_bone_weights(dm, vid, prof)
        ws = first(r, (list, unreal.Array)) or []
        for bw in ws:
            for g, s in enumerate(groups):
                if bw.bone_index in s:
                    out[g][vid] += bw.weight
    return out


def read_weights(dm, n):
    prof = unreal.GeometryScriptBoneWeightProfile()
    W = []
    for vid in range(n):
        ws = first(BW.get_vertex_bone_weights(dm, vid, prof), (list, unreal.Array)) or []
        W.append({bw.bone_index: bw.weight for bw in ws})
    return W


def hops(parents, a, b):
    """Bones between a and b in the hierarchy."""
    up, d, i = {}, 0, a
    while i is not None and i >= 0:
        up[i] = d
        i, d = parents.get(i), d + 1
    d, i = 0, b
    while i is not None and i >= 0:
        if i in up:
            return up[i] + d
        i, d = parents.get(i), d + 1
    return 99


def merge(Wc, Wi, parents):
    """Per vertex the inpainted weights when sane (a distribution whose main bone is near the closest point's)."""
    out, kept = [], 0
    for wc, wi in zip(Wc, Wi):
        s = sum(wi.values())
        ok = wi and wc and 0.95 < s < 1.05 and all(-1e-3 < x < 1.001 for x in wi.values()) and \
            hops(parents, max(wi, key=wi.get), max(wc, key=wc.get)) <= SANE_HOPS
        out.append(wi if ok else wc)
        kept += 1 if ok else 0
    return out, kept


def smooth_weights(dm, n, iters, strength, W=None):
    """Laplacian smoothing of every vertex's bone weights over its mesh neighbours, pruned to MAX_INFLUENCES, written
    back to dm (W: the weights to start from, else dm's own)."""
    prof = unreal.GeometryScriptBoneWeightProfile()
    W = W if W is not None else read_weights(dm, n)
    tl = first(Q.get_all_triangle_indices(dm, False), unreal.GeometryScriptTriangleList)
    nb = [set() for _ in range(n)]
    for t in LS.convert_triangle_list_to_array(tl):
        a, b, c = t.x, t.y, t.z
        if max(a, b, c) < n:
            nb[a].update((b, c))
            nb[b].update((a, c))
            nb[c].update((a, b))
    for _ in range(iters):
        new = []
        for v in range(n):
            if not nb[v] or not W[v]:
                new.append(W[v])
                continue
            acc = {}
            for u in nb[v]:
                for k, x in W[u].items():
                    acc[k] = acc.get(k, 0.0) + x
            k_n = 1.0 / len(nb[v])
            d = {k: (1 - strength) * x for k, x in W[v].items()}
            for k, x in acc.items():
                d[k] = d.get(k, 0.0) + strength * x * k_n
            new.append(d)
        W = new
    for v in range(n):
        top = sorted(W[v].items(), key=lambda kv: -kv[1])[:MAX_INFLUENCES]
        top = [(k, x) for k, x in top if x > 0.01] or top[:1]
        tot = sum(x for _, x in top) or 1.0
        arr = [unreal.GeometryScriptBoneWeight(bone_index=k, weight=x / tot) for k, x in top]
        BW.set_vertex_bone_weights(dm, v, arr, prof)


def bbox(pts):
    return [min(p[i] for p in pts) for i in range(3)], [max(p[i] for p in pts) for i in range(3)]


def mean(vals):
    return sum(vals) / max(1, len(vals))


def median(vals):
    v = sorted(vals)
    return v[len(v) // 2] if v else 0.0


# --- frames ---------------------------------------------------------------------------------------------------------------
def front_of(pts):
    """The generated mesh's front, (x, y) along a world axis: arms make the wide axis the lateral one; the toes stick
    out past the shins on the front."""
    lo, hi = bbox(pts)
    h = hi[2] - lo[2]
    depth = 1 if (hi[0] - lo[0]) >= (hi[1] - lo[1]) else 0     # the narrow horizontal axis
    feet = [p[depth] for p in pts if p[2] < lo[2] + 0.04 * h]
    shins = [p[depth] for p in pts if lo[2] + 0.12 * h < p[2] < lo[2] + 0.2 * h]
    s = 1.0 if mean(feet) - mean(shins) >= 0 else -1.0
    return (s, 0.0) if depth == 0 else (0.0, s)


def rot_z(pts, deg):
    c, s = math.cos(math.radians(deg)), math.sin(math.radians(deg))
    return [(x * c - y * s, x * s + y * c, z) for (x, y, z) in pts]


def arm_angle(pts, pivot, out, zmin, h):
    """Degrees above the outward horizontal of the arm's centroid seen from the shoulder joint (frontal plane)."""
    a_s, b_s, n = 0.0, 0.0, 0
    for p in pts:
        q = (p[0] - pivot[0], p[1] - pivot[1], p[2] - pivot[2])
        a = q[0] * out[0] + q[1] * out[1]
        if a > ARM_REACH and p[2] > zmin + 0.4 * h:
            a_s += a
            b_s += q[2]
            n += 1
    return math.degrees(math.atan2(b_s / max(n, 1), a_s / max(n, 1))), n


def swing(pts, weights, pivot, out, deg):
    """Each point turned about the shoulder joint, in the frontal plane, by deg (positive raises), times its weight."""
    if abs(deg) < 1e-3:
        return pts
    c, s = math.cos(math.radians(deg)), math.sin(math.radians(deg))
    res = []
    for p, w in zip(pts, weights):
        if w <= 1e-4:
            res.append(p)
            continue
        q = (p[0] - pivot[0], p[1] - pivot[1], p[2] - pivot[2])
        a = q[0] * out[0] + q[1] * out[1]
        b = q[2]
        a2, b2 = a * c - b * s, a * s + b * c
        n = (pivot[0] + q[0] + (a2 - a) * out[0], pivot[1] + q[1] + (a2 - a) * out[1], pivot[2] + b2)
        res.append((p[0] + w * (n[0] - p[0]), p[1] + w * (n[1] - p[1]), p[2] + w * (n[2] - p[2])))
    return res


# --- assets ---------------------------------------------------------------------------------------------------------------
def import_glb(hero):
    name, src = hero["name"], os.path.join(SRC, hero["rodin"], "base_basic_pbr.glb")
    folder = "%s/%s/Src" % (where(hero), name)
    target = "%s/SM_HeroSrc_%s" % (folder, name)
    before = set(EAL.list_assets(folder, recursive=True)) if EAL.does_directory_exist(folder) else set()
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", src)
    t.set_editor_property("destination_path", folder)
    t.set_editor_property("replace_existing", True)            # on purpose: a retake replaces the last import
    t.set_editor_property("automated", True)
    t.set_editor_property("save", True)                        # on purpose: saved as it lands
    AT.import_asset_tasks([t])
    paths = list(t.get_editor_property("imported_object_paths") or []) or \
        sorted(set(EAL.list_assets(folder, recursive=True)) - before) or EAL.list_assets(folder, recursive=True)
    meshes = [m for m in (EAL.load_asset(p) for p in paths) if isinstance(m, unreal.StaticMesh)]
    if not meshes:
        raise RuntimeError("%s: the import made no static mesh" % name)
    m = meshes[0]
    if m.get_path_name().split(".")[0] != target:
        if EAL.does_asset_exist(target):
            EAL.delete_asset(target)
        EAL.rename_asset(m.get_path_name().split(".")[0], target)
        m = EAL.load_asset(target)
    return m, folder


def textures(folder):
    base = normal = None
    for p in EAL.list_assets(folder, recursive=True):
        t = EAL.load_asset(p)
        if not isinstance(t, unreal.Texture2D):
            continue
        nm = t.get_name().lower()
        if t.get_editor_property("compression_settings") == unreal.TextureCompressionSettings.TC_NORMALMAP \
                or "normal" in nm:
            normal = normal or t
        elif t.get_editor_property("srgb") and not any(k in nm for k in ("rough", "metal", "orm", "occlu")):
            base = base or t
    return base, normal


def param(m, cls, name, x, y, value=None):
    e = MEL.create_material_expression(m, cls, x, y)
    e.set_editor_property("parameter_name", name)
    if value is not None:
        e.set_editor_property("default_value", value)
    return e


def master():
    """M_Hero: the Rodin base colour times Tint, its normal map, one roughness. Built once."""
    path = DEST + "/M_Hero"
    if EAL.does_asset_exist(path):
        return EAL.load_asset(path)
    m = AT.create_asset("M_Hero", DEST, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("used_with_skeletal_mesh", True)
    bt = param(m, unreal.MaterialExpressionTextureSampleParameter2D, "BaseTex", -700, -200)
    bt.set_editor_property("texture", EAL.load_asset("/Engine/EngineResources/DefaultTexture"))
    nt = param(m, unreal.MaterialExpressionTextureSampleParameter2D, "NormalTex", -700, 250)
    nt.set_editor_property("texture", EAL.load_asset("/Engine/EngineMaterials/FlatNormal"))
    nt.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    tint = param(m, unreal.MaterialExpressionVectorParameter, "Tint", -700, 50, unreal.LinearColor(1, 1, 1, 1))
    rough = param(m, unreal.MaterialExpressionScalarParameter, "Roughness", -400, 150, ROUGHNESS)
    mul = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -350, -100)
    MEL.connect_material_expressions(bt, "RGB", mul, "A")
    MEL.connect_material_expressions(tint, "", mul, "B")
    MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(nt, "RGB", unreal.MaterialProperty.MP_NORMAL)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.recompile_material(m)
    EAL.save_asset(path, only_if_is_dirty=False)
    return m


def instance(hero, folder):
    name = hero["name"]
    path = "%s/%s/MI_%s_%s" % (where(hero), name, hero.get("prefix", "Hero"), name)
    mi = EAL.load_asset(path) if EAL.does_asset_exist(path) else AT.create_asset(
        path.split("/")[-1], "%s/%s" % (where(hero), name), unreal.MaterialInstanceConstant,
        unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, master())
    base, normal = textures(folder)
    if base:
        MEL.set_material_instance_texture_parameter_value(mi, "BaseTex", base)
    else:
        log("%s: no base colour texture found" % name)
    if normal:
        MEL.set_material_instance_texture_parameter_value(mi, "NormalTex", normal)
    MEL.update_material_instance(mi)
    EAL.save_asset(path, only_if_is_dirty=False)
    return mi


# --- one hero -------------------------------------------------------------------------------------------------------------
def build(hero):
    name = hero["name"]
    glb = os.path.join(SRC, hero["rodin"], "base_basic_pbr.glb")
    if not os.path.exists(glb):
        log("%s: no %s yet: skipped" % (name, glb))
        return None
    path = "%s/%s/SKM_%s_%s" % (where(hero), name, hero.get("prefix", "Hero"), name)
    stamp = "%d|%s" % (int(os.path.getmtime(glb)), VERSION)
    if not FORCE and EAL.does_asset_exist(path):
        skm = EAL.load_asset(path)
        if EAL.get_metadata_tag(skm, "HeroSource") == stamp:
            log("%s: up to date" % name)
            return skm

    # the source mannequin: mesh, weights, bones
    src_sk = EAL.load_asset(hero["source"])
    S = unreal.DynamicMesh()
    GA.copy_mesh_from_skeletal_mesh(src_sk, S, unreal.GeometryScriptCopyMeshFromAssetOptions(),
                                    unreal.GeometryScriptMeshReadLOD())
    by_name, infos = bones(S)
    w = lambda b: (b.world_transform.translation.x, b.world_transform.translation.y, b.world_transform.translation.z)  # noqa: E731
    sp = positions(S)
    slo, shi = bbox(sp)
    sh = shi[2] - slo[2]
    ua_l, ua_r = w(by_name["upperarm_l"]), w(by_name["upperarm_r"])
    lat = (ua_l[0] - ua_r[0], ua_l[1] - ua_r[1])
    ln = math.hypot(*lat)
    lat = (lat[0] / ln, lat[1] / ln)                            # toward the mannequin's left
    toe = (w(by_name["ball_l"])[0] - w(by_name["foot_l"])[0], w(by_name["ball_l"])[1] - w(by_name["foot_l"])[1])
    fwd = (1.0 if toe[0] > 0 else -1.0, 0.0) if abs(toe[0]) > abs(toe[1]) else (0.0, 1.0 if toe[1] > 0 else -1.0)

    # the generated mesh: import, face the mannequin's front, height, feet down, centred on the legs
    sm, folder = import_glb(hero)
    T = unreal.DynamicMesh()
    GA.copy_mesh_from_static_mesh_v2(sm, T, unreal.GeometryScriptCopyMeshFromAssetOptions(),
                                     unreal.GeometryScriptMeshReadLOD(), True)
    tp = positions(T)
    f = front_of(tp)
    yaw = math.degrees(math.atan2(fwd[1], fwd[0]) - math.atan2(f[1], f[0])) + hero["yaw"]
    TR.rotate_mesh(T, unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw), unreal.Vector(0, 0, 0))
    tp = positions(T)
    lo, hi = bbox(tp)
    s = sh * hero["height"] / (hi[2] - lo[2])
    TR.scale_mesh(T, unreal.Vector(s, s, s), unreal.Vector(0, 0, 0))
    tp = positions(T)
    lo, hi = bbox(tp)
    th = hi[2] - lo[2]

    def lateral_mid(lo_, hi_):
        return (lo_[0] + hi_[0]) / 2 * abs(lat[0]) + (lo_[1] + hi_[1]) / 2 * abs(lat[1])

    def leg_depth(pts, z0, hh):
        return median([p[0] * abs(fwd[0]) + p[1] * abs(fwd[1]) for p in pts if z0 + 0.25 * hh < p[2] < z0 + 0.45 * hh])
    dl = lateral_mid(slo, shi) - lateral_mid(lo, hi)
    dd = leg_depth(sp, slo[2], sh) - leg_depth(tp, lo[2], th)
    move = (dl * abs(lat[0]) + dd * abs(fwd[0]), dl * abs(lat[1]) + dd * abs(fwd[1]), slo[2] - lo[2])
    TR.translate_mesh(T, unreal.Vector(*move))
    tp = positions(T)
    lo, hi = bbox(tp)

    # arms: the generated pose against the mannequin's reference pose
    out_l, out_r = lat, (-lat[0], -lat[1])
    ang = {}
    for side, piv, out in (("l", ua_l, out_l), ("r", ua_r, out_r)):
        a_src, n_src = arm_angle(sp, piv, out, slo[2], sh)
        a_gen, n_gen = arm_angle(tp, piv, out, lo[2], th)
        ang[side] = a_gen - a_src
        log("%s arm %s: mannequin %.1f deg (%d verts), generated %.1f deg (%d verts)" % (name, side, a_src, n_src,
                                                                                      a_gen, n_gen))
    groups = [subtree(infos, by_name["upperarm_l"].index), subtree(infos, by_name["upperarm_r"].index)]
    posed = any(abs(d) >= ARM_MIN_DEG for d in ang.values())
    if posed:
        wl, wr = side_weights(S, len(sp), groups)
        p2 = swing(sp, wl, ua_l, out_l, ang["l"] if abs(ang["l"]) >= ARM_MIN_DEG else 0.0)
        p2 = swing(p2, wr, ua_r, out_r, ang["r"] if abs(ang["r"]) >= ARM_MIN_DEG else 0.0)
        set_positions(S, p2)
        recompute_normals(S)

    # weights: posed mannequin -> generated mesh, with the mannequin's bones
    o = unreal.GeometryScriptTransferBoneWeightsOptions()
    o.set_editor_property("output_target_mesh_bones", unreal.OutputTargetMeshBones.SOURCE_BONES)
    o.set_editor_property("transfer_method", unreal.TransferBoneWeightsMethod.CLOSEST_POINT_ON_SURFACE)
    BW.transfer_bone_weights_from_mesh(S, T, o, unreal.GeometryScriptMeshSelection())
    W = None
    if TRANSFER == "hybrid":
        Wc = read_weights(T, len(tp))
        o.set_editor_property("transfer_method", unreal.TransferBoneWeightsMethod.INPAINT_WEIGHTS)
        o.set_editor_property("radius_percentage", INPAINT_RADIUS)
        o.set_editor_property("normal_threshold", INPAINT_NORMAL)
        o.set_editor_property("num_smoothing_iterations", 6)
        o.set_editor_property("smoothing_strength", 0.4)
        try:
            BW.transfer_bone_weights_from_mesh(S, T, o, unreal.GeometryScriptMeshSelection())
            W, kept = merge(Wc, read_weights(T, len(tp)), {b.index: b.parent_index for b in infos})
            log("%s: inpainted weights kept on %d of %d vertices" % (name, kept, len(tp)))
        except Exception as e:                                  # noqa: BLE001
            W = Wc
            log("%s: inpaint failed (%s): closest point only" % (name, e))
    if SMOOTH_ITERS or W is not None:
        smooth_weights(T, len(tp), SMOOTH_ITERS, SMOOTH_STRENGTH, W)
    if RIGID_FINGERS:
        fingers = [unreal.Name(n) for n in by_name if any(k in n for k in ("thumb_", "index_", "middle_", "ring_",
                                                                          "pinky_"))]
        BW.prune_bone_weights(T, fingers, unreal.GeometryScriptPruneBoneWeightsOptions(),
                              unreal.GeometryScriptBoneWeightProfile())

    # the generated arms back to the reference pose, with their new weights
    if posed:
        wl, wr = side_weights(T, len(tp), groups)
        p2 = swing(tp, wl, ua_l, out_l, -ang["l"] if abs(ang["l"]) >= ARM_MIN_DEG else 0.0)
        p2 = swing(p2, wr, ua_r, out_r, -ang["r"] if abs(ang["r"]) >= ARM_MIN_DEG else 0.0)
        set_positions(T, p2)
        recompute_normals(T)

    # --- W3 cast2 --- close the residual: the weighted swing back lands short (vertices with part of the arm's weight
    # turn only part of the way), which left the arms held out in the bind pose and so out in every animation (Marwan:
    # "fix his arms"). Measure again against the mannequin and swing the rest, a few passes.
    if posed:
        for _pass in range(ARM_PASSES):
            tp = positions(T)
            lo, hi = bbox(tp)
            res = {}
            for side, piv, out in (("l", ua_l, out_l), ("r", ua_r, out_r)):
                res[side] = arm_angle(tp, piv, out, lo[2], hi[2] - lo[2])[0] - arm_angle(sp, piv, out, slo[2], sh)[0]
            log("%s arms pass %d: residual %+.1f / %+.1f deg" % (name, _pass, res["l"], res["r"]))
            if all(abs(d) < ARM_MIN_DEG * 0.5 for d in res.values()):
                break
            wl, wr = side_weights(T, len(tp), groups)
            p2 = swing(tp, wl, ua_l, out_l, -res["l"])
            p2 = swing(p2, wr, ua_r, out_r, -res["r"])
            set_positions(T, p2)
        recompute_normals(T)


    # the skeletal mesh
    mi = instance(hero, folder)
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    co = unreal.GeometryScriptCreateNewSkeletalMeshAssetOptions()
    co.set_editor_property("use_mesh_bone_proportions", True)
    co.set_editor_property("enable_recompute_tangents", True)
    co.set_editor_property("materials", {"Hero": mi})
    skm = first(NA.create_new_skeletal_mesh_asset_from_mesh(T, EAL.load_asset(SKELETON), path, co),
                unreal.SkeletalMesh)
    if not skm:
        raise RuntimeError("%s: CreateNewSkeletalMeshAssetFromMesh failed" % name)
    try:
        skm.set_editor_property("physics_asset", src_sk.get_editor_property("physics_asset"))
    except Exception:                                           # noqa: BLE001
        pass
    EAL.set_metadata_tag(skm, "HeroSource", stamp)
    EAL.save_asset(path, only_if_is_dirty=False)
    b = skm.get_bounds()
    log("%s: %s, %d verts, yaw %.0f, scale %.3f, arms %+.1f / %+.1f deg, %.0f cm tall" % (
        name, path, len(tp), yaw, s, ang["l"], ang["r"], b.box_extent.z * 2))
    return skm


# --- hook-up --------------------------------------------------------------------------------------------------------------
def hook_up(built):
    """HeroMesh on each lead: on the ATakeCharacter (the contract) and on the CastMember build_cast.py attaches to it
    (where package cast declared it), whichever has the property; nothing when neither is compiled yet."""
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    chars = [a for a in eas.get_all_level_actors() if isinstance(a, unreal.TakeCharacter)]
    changed = 0
    for a in chars:
        cid = a.get_editor_property("id")
        cid = getattr(cid, "name", str(cid)).split(".")[-1]
        skm = built.get(cid)
        if not skm:
            continue
        for t in [a] + list(a.get_attached_actors() or []):
            try:
                t.get_editor_property("hero_mesh")
            except Exception:                                   # noqa: BLE001 (no such property: skip quietly)
                continue
            t.set_editor_property("hero_mesh", skm)
            changed += 1
            log("hook-up: %s HeroMesh = %s" % (t.get_actor_label(), skm.get_name()))
    if not changed:
        log("hook-up: no HeroMesh property on the leads or their cast members yet: skipped")
    return changed


def main():
    built = {}
    for hero in HEROES + NPCS:
        if ONLY and hero["name"] not in ONLY:
            continue
        try:
            skm = build(hero)
            if skm and hero["cam"]:
                built[hero["cam"]] = skm
        except Exception:                                       # noqa: BLE001 (one hero must not stop the others)
            log("%s FAILED: %s" % (hero["name"], traceback.format_exc().strip().splitlines()[-1][:300]))
            unreal.log_error(traceback.format_exc())
    if built and hook_up(built) and _G.get("__name__") != "set_overlay":
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    return "\n".join(REPORT)


RESULT = main()
RESULT
