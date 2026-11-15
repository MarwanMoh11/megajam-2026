"""Package heroes (QA note 21): Julian, Maya and Vex as Rodin-generated characters skinned to the UE5 mannequin's own
skeleton (SK_Mannequin), so every template animation plays on them with no retargeting.

Per hero (skipped when its skeletal mesh was already built from the same .glb by the same VERSION of this script):
  1. import C:\\Jam\\Art\\Rodin\\heroes_<name>\\base_basic_pbr.glb -> /Game/Take/Cast/Heroes/<Name>/SM_HeroSrc_<Name>
  2. copy it into a dynamic mesh; turn it to face the mannequin's front, scale it to the mannequin's height, feet on
     the ground, centred on the legs
  3. measure both meshes' arm angle (A against T pose); pose the mannequin's arms to the generated pose with its own
     skin weights, transfer the bone weights (closest point on surface), then pose the generated arms back to the
     mannequin's reference pose with the weights they just got
  3b. (Nov 15, hands) each forearm turned about the elbow onto the mannequin's (its bind pose bends the elbow forward)
     and its wrist set on the mannequin's (FOREARM_ALIGN); the sleeves weighted again along the arm bones
     (ARM_REBONE); the generated hands (stiff splayed slabs) cut off at the wrist and the mannequin's own rigged hands
     put in, coloured from the generated wrist skin (REAL_HANDS), so every clip's finger curls play
  4. create /Game/Take/Cast/Heroes/<Name>/SKM_Hero_<Name> on SK_Mannequin, with the source mannequin's bone
     proportions (Quinn's for Maya) and an instance of M_Hero carrying the Rodin colour
Then the hook-up: sets ATakeCharacter.HeroMesh (package cast's property) on the level's A, B and C characters when the
property exists, and skips quietly when it does not.

Run inside the editor, under the editor lock:
  "<engine python>" -I tools/ue_mcp.py py tools/build_heroes.py 110
NPCS (Nov 15) are built the same way into /Game/Take/Cast/NPCs/<Name>/SKM_NPC_<Name> from C:\\Jam\\Art\\Rodin\\npc_<name>;
they get no hook-up here (tools/build_cast.py sets their HeroMesh), and one without a .glb yet is skipped.
A wrapper may set HEROES_ONLY = ["Julian"] (build only those), HEROES_FORCE = True (rebuild) or HEROES_NO_HOOKUP = True
(meshes updated in place: no level edit, no level save) and exec() this file.
tools/sets/run_all.py runs it after the set overlays (it saves the level itself).
"""
import math
import os
import traceback

import unreal

VERSION = "6"                 # bump to rebuild every hero on the next run
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
REAL_HANDS = True                   # the generated hands (stiff, splayed slabs) cut off at the wrist and the source
                                    # mannequin's own hands put in their place, with their finger weights, so every
                                    # clip's finger curls play; they take the skin colour from the generated hand
HAND_CUT = 0.5                      # a generated vertex with at least this weight on hand_l/hand_r is hand
HAND_PLANE = 1.5                    # ...and so is an arm vertex past a plane this many cm short of the wrist joint,
HAND_SKIN_BACK = 0.0                # ...and the bare wrist skin up to this many cm short of the joint,
HAND_SKIN_RADIUS = 4.3              # within this many cm of the forearm's axis (the cuff stands wider)
HAND_RADIUS = 22.0                  # within this many cm of it (the generated hand's skin that the forearm carried)
HAND_KEEP = 0.5                     # a mannequin vertex with at least this weight on the hand and fingers is hand
BIND_TO_MESH = True                 # VERSION 6: the arm bones fitted to the generated mesh, which is never bent (the
                                    # swings, FOREARM_ALIGN and ARM_REBONE below bent it, and the sleeves crumpled and
                                    # the forearms broke away at the elbow); False: the VERSION 5 path
FOREARM_ALIGN = True                # the generated forearms turned about the elbow onto the mannequin's forearm (its
                                    # reference pose bends the elbow forward; the generated arm hangs straight), so a
                                    # bent elbow carries the sleeve along the bone and the hand meets its cuff
ARM_REBONE = True                   # the sleeves weighted again along the shoulder-elbow-wrist bones: the transfer
                                    # smeared forearm weight onto the upper arm, so a bent elbow left the sleeve
                                    # halfway and the hand out of its cuff (Use, Clap)
ARM_ELBOW_BLEND = 6.0               # cm either side of the elbow's bisecting plane over which upper arm hands over
ARM_SHOULDER_KEEP = (0.25, 0.5)     # along the upper arm: the transferred weights kept before the first, the new ones
                                    # from the second, a blend between (no seam at the shoulder)
ARM_RADIUS = 20.0                   # cm from the arm's bones within which a vertex counts as sleeve
HAND_WRIST_BACK = 2.0               # cm of the mannequin's forearm kept above its wrist joint, inside the sleeve
HAND_UV_SPREAD = 0.004              # the mannequin hand's UVs squeezed to this patch around the skin point

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

    if BIND_TO_MESH:
        # VERSION 6: the generated mesh is never bent. Weights from the mannequin posed by its arm angle give each arm
        # vertex its bone; the arm bones are then turned onto the generated arm's own axes; the mannequin is skinned
        # to those bones and the weights transferred again, so the elbow's weights sit at the fitted elbow.
        transfer(name, S, T, len(tp), infos, by_name, full=False)
        R = ref_xforms(infos)
        X = dict(R)
        for side, piv, out in (("l", ua_l, out_l), ("r", ua_r, out_r)):
            d = ang[side] if abs(ang[side]) >= ARM_MIN_DEG else 0.0
            if d:
                k = _unit(_cross((out[0], out[1], 0.0), (0.0, 0.0, 1.0)))
                turn_subtree(X, subtree(infos, by_name["upperarm_" + side].index), qaxis(k, math.radians(d)), piv)
        W0 = read_weights(T, len(tp))
        if _G.get("HEROES_DUMP"):                               # the fit's inputs, for working on it outside the editor
            import json
            with open(os.path.join(_G["HEROES_DUMP"], name + "_fit.json"), "w") as f:
                json.dump({"tp": tp, "W": [{str(k): v for k, v in w.items()} for w in W0], "X": {str(k): v for k, v in X.items()},
                           "bones": {k: b.index for k, b in by_name.items()},
                           "parent": {str(b.index): b.parent_index for b in infos}}, f)
        fit_arms(name, tp, W0, infos, by_name, X)
        set_positions(S, skin_points(sp, read_weights(S, len(sp)), R, X))
        recompute_normals(S)
        transfer(name, S, T, len(tp), infos, by_name, full=True)
        if REAL_HANDS:
            swap_hands(name, T, src_sk, fwd, X, R)
        return finish(hero, name, path, stamp, T, src_sk, folder, infos, by_name, X, yaw, s, ang)

    # (VERSION 5 and before) weights: posed mannequin -> generated mesh, with the mannequin's bones
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

    if FOREARM_ALIGN:
        align_forearms(name, T, src_sk)
    if ARM_REBONE:
        rebone_arms(name, T)
    if REAL_HANDS:
        swap_hands(name, T, src_sk, fwd)

    return finish(hero, name, path, stamp, T, src_sk, folder, infos, by_name, None, yaw, s, ang)


def finish(hero, name, path, stamp, T, src_sk, folder, infos, by_name, X, yaw, s, ang):
    """The skeletal mesh: updated in place when it exists (the level's actors and the scenes keep their reference),
    else created; with X, its arm bones set onto the generated arms."""
    mi = instance(hero, folder)
    skm = EAL.load_asset(path) if EAL.does_asset_exist(path) else None
    if skm:
        co = unreal.GeometryScriptCopyMeshToAssetOptions()
        co.set_editor_property("enable_recompute_tangents", True)
        co.set_editor_property("replace_materials", True)
        co.set_editor_property("new_materials", [mi])
        co.set_editor_property("new_material_slot_names", ["Hero"])
        GA.copy_mesh_to_skeletal_mesh(T, skm, co, unreal.GeometryScriptMeshWriteLOD())
    else:
        co = unreal.GeometryScriptCreateNewSkeletalMeshAssetOptions()
        co.set_editor_property("use_mesh_bone_proportions", True)
        co.set_editor_property("enable_recompute_tangents", True)
        co.set_editor_property("materials", {"Hero": mi})
        skm = first(NA.create_new_skeletal_mesh_asset_from_mesh(T, EAL.load_asset(SKELETON), path, co),
                    unreal.SkeletalMesh)
    if not skm:
        raise RuntimeError("%s: no skeletal mesh written" % name)
    if X is not None:
        set_arm_bones(name, skm, infos, by_name, X)
    try:
        skm.set_editor_property("physics_asset", src_sk.get_editor_property("physics_asset"))
    except Exception:                                           # noqa: BLE001
        pass
    EAL.set_metadata_tag(skm, "HeroSource", stamp)
    EAL.save_asset(path, only_if_is_dirty=False)
    b = skm.get_bounds()
    log("%s: %s, %d verts, yaw %.0f, scale %.3f, arms %+.1f / %+.1f deg, %.0f cm tall" % (
        name, path, len(positions(T)), yaw, s, ang["l"], ang["r"], b.box_extent.z * 2))
    return skm


# --- bind to the mesh (VERSION 6) -------------------------------------------------------------------------------------------
# Quaternions are (w, x, y, z) tuples; a bone's world transform is (t, q) with unit scale (the mannequins' bones are).
def qmul(a, b):
    return (a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3],
            a[0] * b[1] + a[1] * b[0] + a[2] * b[3] - a[3] * b[2],
            a[0] * b[2] - a[1] * b[3] + a[2] * b[0] + a[3] * b[1],
            a[0] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[0])


def qconj(q):
    return (q[0], -q[1], -q[2], -q[3])


def qrot(q, v):
    r = qmul(qmul(q, (0.0, v[0], v[1], v[2])), qconj(q))
    return (r[1], r[2], r[3])


def qaxis(k, ang):
    s = math.sin(ang / 2)
    return (math.cos(ang / 2), k[0] * s, k[1] * s, k[2] * s)


def _sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def _dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def _cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def _unit(a):
    n = math.sqrt(_dot(a, a)) or 1.0
    return (a[0] / n, a[1] / n, a[2] / n)


def qfromto(a, b):
    """The shortest rotation taking direction a onto direction b, and its angle in degrees."""
    a, b = _unit(a), _unit(b)
    k = _cross(a, b)
    kl = math.sqrt(_dot(k, k))
    ang = math.atan2(kl, _dot(a, b))
    if kl < 1e-9:
        return (1.0, 0.0, 0.0, 0.0), 0.0
    return qaxis((k[0] / kl, k[1] / kl, k[2] / kl), ang), math.degrees(ang)


def ref_xforms(infos):
    out = {}
    for b in infos:
        t, r = b.world_transform.translation, b.world_transform.rotation
        out[b.index] = ((t.x, t.y, t.z), (r.w, r.x, r.y, r.z))
    return out


def turn_subtree(X, idx, q, pivot):
    """Every bone in idx turned by q about pivot (world space), in place."""
    for i in idx:
        t, r = X[i]
        X[i] = (tuple(p + d for p, d in zip(pivot, qrot(q, _sub(t, pivot)))), qmul(q, r))


def _dist(a, b):
    d = _sub(a, b)
    return math.sqrt(_dot(d, d))


def medial(pts, origin, axis, radius=18.0, step=2.0, start=6.0):
    """The arm's centre line: centroids of 2 cm slices across axis from origin (points within radius of the axis)."""
    bins = {}
    for p in pts:
        d = _sub(p, origin)
        t = _dot(d, axis)
        if t >= start and _dot(d, d) - t * t <= radius * radius:
            bins.setdefault(int(t // step), []).append(p)
    return [tuple(sum(p[i] for p in v) / len(v) for i in range(3)) for _k, v in sorted(bins.items()) if len(v) >= 8]


def along(poly, first_i, length, origin):
    """The point on the centre line (from index first_i) at length from origin (straight on past its end), its index."""
    prev = origin
    for i in range(first_i, len(poly)):
        if _dist(poly[i], origin) >= length:
            a, b = (poly[i - 1] if i > first_i else prev), poly[i]
            da, db = _dist(a, origin), _dist(b, origin)
            t = max(0.0, min(1.0, (length - da) / ((db - da) or 1.0)))
            return tuple(a[j] + (b[j] - a[j]) * t for j in range(3)), i
    last = poly[-1]
    d = _unit(_sub(last, poly[max(0, len(poly) - 4)]))
    ext = max(0.0, length - _dist(last, origin))
    return tuple(last[j] + d[j] * ext for j in range(3)), len(poly)


def fit_arms(name, tp, W, infos, by_name, X):
    """The upper arm and the forearm bones (and everything below them) turned about their joints onto the generated
    arm's centre line: the elbow joint goes to the point on it the upper arm's length from the shoulder joint, the
    wrist the forearm's length on from there (the bones keep the mannequin's lengths, which every clip carries), in
    place on X. The generated arms are not straight (Rodin bends the elbows a little), so the line is followed."""
    for side in ("l", "r"):
        iu, il, ih = (by_name["%s_%s" % (b, side)].index for b in ("upperarm", "lowerarm", "hand"))
        up_sub, lo_sub = subtree(infos, iu), subtree(infos, il)
        sh, el, wr = X[iu][0], X[il][0], X[ih][0]
        lu, lf = _dist(sh, el), _dist(el, wr)
        arm = [p for p, w in zip(tp, W) if sum(x for k, x in w.items() if k in up_sub) >= 0.5]
        axis = _unit(_sub(wr, sh))
        poly = []
        for _ in range(3):
            poly = medial(arm, sh, axis)
            if len(poly) < 6:
                break
            axis = _unit(_sub(poly[-1], sh))
        if len(poly) < 6:
            log("%s arm %s: %d slices on the arm: left alone" % (name, side, len(poly)))
            continue
        e2, i = along(poly, 0, lu, sh)
        w2, _i = along(poly, i, lf, e2)
        q1, d1 = qfromto(_sub(el, sh), _sub(e2, sh))
        turn_subtree(X, up_sub, q1, sh)
        el, wr = X[il][0], X[ih][0]
        q2, d2 = qfromto(_sub(wr, el), _sub(w2, el))
        turn_subtree(X, lo_sub, q2, el)
        log("%s arm %s: upper arm turned %.1f deg, forearm %.1f deg onto the mesh's centre line (reaches %.0f cm, "
            "bones %.0f cm)" % (name, side, d1, d2, _dist(poly[-1], sh), lu + lf))


def skin_points(pts, W, ref, new):
    """Linear blend skinning: pts (bound to ref) moved to the bones in new."""
    D = {}
    for i in new:
        (tr, qr), (tn, qn) = ref[i], new[i]
        if tr == tn and qr == qn:
            continue
        qd = qmul(qn, qconj(qr))
        rt = qrot(qd, tr)
        D[i] = (qd, (tn[0] - rt[0], tn[1] - rt[1], tn[2] - rt[2]))
    out = []
    for p, w in zip(pts, W):
        if not any(k in D for k in w):
            out.append(p)
            continue
        acc, tw = [0.0, 0.0, 0.0], 0.0
        for k, x in w.items():
            if k in D:
                qd, td = D[k]
                r = qrot(qd, p)
                q = (r[0] + td[0], r[1] + td[1], r[2] + td[2])
            else:
                q = p
            acc = [acc[i] + x * q[i] for i in range(3)]
            tw += x
        out.append(tuple(c / (tw or 1.0) for c in acc))
    return out


def transfer(name, S, T, n, infos, by_name, full=True):
    """Bone weights S -> T (closest point; full: also the hybrid inpaint, smoothing and rigid fingers)."""
    o = unreal.GeometryScriptTransferBoneWeightsOptions()
    o.set_editor_property("output_target_mesh_bones", unreal.OutputTargetMeshBones.SOURCE_BONES)
    o.set_editor_property("transfer_method", unreal.TransferBoneWeightsMethod.CLOSEST_POINT_ON_SURFACE)
    BW.transfer_bone_weights_from_mesh(S, T, o, unreal.GeometryScriptMeshSelection())
    if not full:
        return
    W = None
    if TRANSFER == "hybrid":
        Wc = read_weights(T, n)
        o.set_editor_property("transfer_method", unreal.TransferBoneWeightsMethod.INPAINT_WEIGHTS)
        o.set_editor_property("radius_percentage", INPAINT_RADIUS)
        o.set_editor_property("normal_threshold", INPAINT_NORMAL)
        o.set_editor_property("num_smoothing_iterations", 6)
        o.set_editor_property("smoothing_strength", 0.4)
        try:
            BW.transfer_bone_weights_from_mesh(S, T, o, unreal.GeometryScriptMeshSelection())
            W, kept = merge(Wc, read_weights(T, n), {b.index: b.parent_index for b in infos})
            log("%s: inpainted weights kept on %d of %d vertices" % (name, kept, n))
        except Exception as e:                                  # noqa: BLE001
            W = Wc
            log("%s: inpaint failed (%s): closest point only" % (name, e))
    if SMOOTH_ITERS or W is not None:
        smooth_weights(T, n, SMOOTH_ITERS, SMOOTH_STRENGTH, W)
    if RIGID_FINGERS:
        fingers = [unreal.Name(k) for k in by_name if any(f in k for f in ("thumb_", "index_", "middle_", "ring_",
                                                                         "pinky_"))]
        BW.prune_bone_weights(T, fingers, unreal.GeometryScriptPruneBoneWeightsOptions(),
                              unreal.GeometryScriptBoneWeightProfile())


def _ue_xf(t, q):
    return unreal.Transform(unreal.Vector(*t), unreal.Quat(q[1], q[2], q[3], q[0]).rotator(), unreal.Vector(1, 1, 1))


def _local(X, parent, child):
    (tp_, qp), (tc, qc) = X[parent], X[child]
    ic = qconj(qp)
    return qrot(ic, _sub(tc, tp_)), qmul(ic, qc)


def set_arm_bones(name, skm, infos, by_name, X):
    """The skeletal mesh's reference pose: each upper arm and forearm as fitted (their children follow), the mesh left
    where it is (the bind moves, not the skin)."""
    mod = unreal.new_object(unreal.SkeletonModifier)
    if not mod.set_skeletal_mesh(skm):
        raise RuntimeError("%s: SkeletonModifier could not load the mesh" % name)
    parent = {b.index: b.parent_index for b in infos}
    for side in ("l", "r"):
        for b in ("upperarm", "lowerarm"):
            i = by_name["%s_%s" % (b, side)].index
            t, q = _local(X, parent[i], i)
            if not mod.set_bone_transform(unreal.Name("%s_%s" % (b, side)), _ue_xf(t, q), True):
                raise RuntimeError("%s: could not set %s_%s" % (name, b, side))
    mod.commit_skeleton_to_skeletal_mesh()                      # False ("no modification needed") when already so
    # read back from the mesh: every fitted joint where it was meant to be
    mod = unreal.new_object(unreal.SkeletonModifier)
    mod.set_skeletal_mesh(skm)
    worst = 0.0
    for side in ("l", "r"):
        for b in ("lowerarm", "hand"):
            g = mod.get_bone_transform(unreal.Name("%s_%s" % (b, side)), True).translation
            worst = max(worst, math.sqrt(sum(c * c for c in _sub((g.x, g.y, g.z), X[by_name["%s_%s" % (b, side)].index][0]))))
    log("%s: arm bones set on the mesh's reference pose (joints within %.2f cm of the fit)" % (name, worst))
    if worst > 0.5:
        raise RuntimeError("%s: arm bones not set (%.1f cm off the fit)" % (name, worst))


# --- real hands ------------------------------------------------------------------------------------------------------------
def tri_ids(dm):
    r = Q.get_all_triangle_i_ds(dm)
    il = first(r, unreal.GeometryScriptIndexList)
    return list(LS.convert_index_list_to_array(il)) if il is not None else []


def tri(dm, t):
    v = first(Q.get_triangle_indices(dm, t), unreal.IntVector)
    return v.x, v.y, v.z


def delete_tris(dm, tids):
    if not tids:
        return
    il = first(LS.convert_array_to_index_list(list(tids), unreal.GeometryScriptIndexType.TRIANGLE),
               unreal.GeometryScriptIndexList)
    ED.delete_triangles_from_mesh(dm, il)
    unreal.GeometryScript_MeshRepair.compact_mesh(dm)           # vertex ids dense again (positions() indexes by id)


def _seg(p, a, b):
    """(t along a->b clamped to [0, 1], squared distance to the segment)."""
    ab = (b[0] - a[0], b[1] - a[1], b[2] - a[2])
    ap = (p[0] - a[0], p[1] - a[1], p[2] - a[2])
    ll = ab[0] ** 2 + ab[1] ** 2 + ab[2] ** 2
    t = max(0.0, min(1.0, (ap[0] * ab[0] + ap[1] * ab[1] + ap[2] * ab[2]) / ll))
    c = (a[0] + ab[0] * t - p[0], a[1] + ab[1] * t - p[1], a[2] + ab[2] * t - p[2])
    return t, c[0] ** 2 + c[1] ** 2 + c[2] ** 2


def _ends(pts, W, sub, hand, elbow, near):
    """Centroids of a forearm: its elbow end (forearm-weighted points within near cm of the elbow) and its hand."""
    a, b = [], []
    for p, w in zip(pts, W):
        if sum(x for k, x in w.items() if k in hand) >= 0.5:
            b.append(p)
        elif sum(x for k, x in w.items() if k in sub) >= 0.5 and sum((p[i] - elbow[i]) ** 2 for i in range(3)) < near * near:
            a.append(p)
    if not a or not b:
        return None
    return tuple(mean([p[i] for p in a]) for i in range(3)), tuple(mean([p[i] for p in b]) for i in range(3))


def _slice(pts, W, sub, hand, wrist, ax, a=-11.0, b=-3.0, r=12.0):
    """Centroid of the forearm points (not the hand's) between a and b cm along ax from the wrist joint."""
    out = []
    for p, w in zip(pts, W):
        if sum(x for k, x in w.items() if k in sub and k not in hand) < 0.5:
            continue
        d = [p[i] - wrist[i] for i in range(3)]
        t = sum(d[i] * ax[i] for i in range(3))
        if a <= t <= b and sum(c * c for c in d) - t * t < r * r:
            out.append(p)
    return tuple(mean([p[i] for p in out]) for i in range(3)) if out else None


def _rot(p, o, k, ang):
    """p turned about the axis k (unit) through o by ang radians (Rodrigues)."""
    v = (p[0] - o[0], p[1] - o[1], p[2] - o[2])
    c, s_ = math.cos(ang), math.sin(ang)
    kv = (k[1] * v[2] - k[2] * v[1], k[2] * v[0] - k[0] * v[2], k[0] * v[1] - k[1] * v[0])
    kd = k[0] * v[0] + k[1] * v[1] + k[2] * v[2]
    return tuple(o[i] + v[i] * c + kv[i] * s_ + k[i] * kd * (1 - c) for i in range(3))


def align_forearms(name, T, src_sk):
    """Per side: the generated forearm's direction (elbow-end centroid to hand centroid) against the mannequin's, the
    generated forearm and hand turned about the mannequin's elbow joint by the angle between, per vertex times its
    forearm-subtree weight (a smooth elbow)."""
    by_name, infos = bones(T)
    S = unreal.DynamicMesh()
    GA.copy_mesh_from_skeletal_mesh(src_sk, S, unreal.GeometryScriptCopyMeshFromAssetOptions(),
                                    unreal.GeometryScriptMeshReadLOD())
    sp = positions(S)
    SW = read_weights(S, len(sp))
    tp = positions(T)
    W = read_weights(T, len(tp))
    for side in ("l", "r"):
        e = by_name["lowerarm_" + side].world_transform.translation
        el = (e.x, e.y, e.z)
        sub = subtree(infos, by_name["lowerarm_" + side].index)
        hand = subtree(infos, by_name["hand_" + side].index)
        m = _ends(sp, SW, sub, hand, el, 12.0)
        g = _ends(tp, W, sub, hand, el, 12.0)
        if not m or not g:
            log("%s forearm %s: no ends found: left alone" % (name, side))
            continue
        dm = [m[1][i] - m[0][i] for i in range(3)]
        dg = [g[1][i] - g[0][i] for i in range(3)]
        lm, lg = math.sqrt(sum(c * c for c in dm)), math.sqrt(sum(c * c for c in dg))
        dm, dg = [c / lm for c in dm], [c / lg for c in dg]
        k = (dg[1] * dm[2] - dg[2] * dm[1], dg[2] * dm[0] - dg[0] * dm[2], dg[0] * dm[1] - dg[1] * dm[0])
        kl = math.sqrt(sum(c * c for c in k))
        ang = math.atan2(kl, sum(dg[i] * dm[i] for i in range(3)))
        log("%s forearm %s: %.1f deg off the mannequin's (length %.1f against %.1f cm)" % (
            name, side, math.degrees(ang), lg, lm))
        if math.degrees(ang) >= 1.0:
            k = tuple(c / kl for c in k)
            tp = [_rot(p, el, k, ang * min(1.0, sum(x for b, x in w.items() if b in sub)))
                  if any(b in sub for b in w) else p for p, w in zip(tp, W)]
        # then the hand onto the mannequin's hand: a shift, per vertex times its forearm weight (the elbow stretches)
        g2 = _ends(tp, W, sub, hand, el, 12.0)
        sh = [m[1][i] - g2[1][i] for i in range(3)]
        # across the arm, the sleeve's centre a little above the wrist onto the mannequin forearm's (the old hand's
        # centroid sits wherever its splayed fingers were); along the arm, the hand's
        h = by_name["hand_" + side].world_transform.translation
        hw = (h.x, h.y, h.z)
        ax = [m[1][i] - m[0][i] for i in range(3)]
        al = math.sqrt(sum(c * c for c in ax))
        ax = [c / al for c in ax]
        cs, cm = _slice(tp, W, sub, hand, hw, ax), _slice(sp, SW, sub, hand, hw, ax)
        if cs and cm:
            along = sum(sh[i] * ax[i] for i in range(3))
            lat = [cm[i] - cs[i] for i in range(3)]
            la = sum(lat[i] * ax[i] for i in range(3))
            sh = [lat[i] - la * ax[i] + along * ax[i] for i in range(3)]
            log("%s forearm %s: sleeve centre %.1f cm across off the mannequin forearm's (%s / %s)" % (
                name, side, math.sqrt(max(0.0, sum(c * c for c in lat) - la * la)),
                ",".join("%.1f" % c for c in cs), ",".join("%.1f" % c for c in cm)))
        else:
            log("%s forearm %s: no sleeve slice (%s, %s)" % (name, side, bool(cs), bool(cm)))
        log("%s forearm %s: hand moved %.1f cm onto the mannequin's" % (name, side, math.sqrt(sum(c * c for c in sh))))
        tp = [tuple(p[i] + sh[i] * min(1.0, sum(x for b, x in w.items() if b in sub)) for i in range(3))
              if any(b in sub for b in w) else p for p, w in zip(tp, W)]
    set_positions(T, tp)
    recompute_normals(T)


def rebone_arms(name, T):
    """Each arm vertex (mostly weighted to that upperarm's subtree, near the bones) weighted again by where it lies:
    upper arm before the elbow's bisecting plane, forearm after it, a smooth hand-over across ARM_ELBOW_BLEND; near the
    shoulder the transferred weights stay (ARM_SHOULDER_KEEP). Hands are left to swap_hands. T in the reference pose."""
    by_name, infos = bones(T)
    tp = positions(T)
    n = len(tp)
    W = read_weights(T, n)
    prof = unreal.GeometryScriptBoneWeightProfile()
    changed = 0
    for side in ("l", "r"):
        P = lambda b: (lambda v: (v.x, v.y, v.z))(by_name[b + "_" + side].world_transform.translation)  # noqa: E731
        sh, el, wr = P("upperarm"), P("lowerarm"), P("hand")
        ui, li = by_name["upperarm_" + side].index, by_name["lowerarm_" + side].index
        arm = subtree(infos, ui)
        da = [el[k] - sh[k] for k in range(3)]
        db = [wr[k] - el[k] for k in range(3)]
        la, lb = math.sqrt(sum(c * c for c in da)), math.sqrt(sum(c * c for c in db))
        nb = [da[k] / la + db[k] / lb for k in range(3)]
        ln = math.sqrt(sum(c * c for c in nb))
        nb = [c / ln for c in nb]
        r2 = ARM_RADIUS ** 2
        for v in range(n):
            if sum(w for b, w in W[v].items() if b in arm) < 0.5:
                continue
            p = tp[v]
            ta, d2a = _seg(p, sh, el)
            tb, d2b = _seg(p, el, wr)
            if min(d2a, d2b) > r2 or (tb >= 1.0 and d2b < d2a):
                continue                                        # off the arm, or past the wrist (the hand's)
            d = sum((p[k] - el[k]) * nb[k] for k in range(3))
            x = max(0.0, min(1.0, (d + ARM_ELBOW_BLEND) / (2 * ARM_ELBOW_BLEND)))
            lo = x * x * (3 - 2 * x)
            new = {ui: 1.0 - lo, li: lo}
            k0, k1 = ARM_SHOULDER_KEEP
            if lo < 0.5 and ta < k1:
                y = max(0.0, min(1.0, (ta - k0) / (k1 - k0)))
                keep = 1.0 - y * y * (3 - 2 * y)
                mix = {}
                for b, w in W[v].items():
                    mix[b] = mix.get(b, 0.0) + w * keep
                for b, w in new.items():
                    mix[b] = mix.get(b, 0.0) + w * (1.0 - keep)
                new = mix
            new = {b: w for b, w in new.items() if w > 0.002}
            tot = sum(new.values())
            BW.set_vertex_bone_weights(T, v, [unreal.GeometryScriptBoneWeight(bone_index=b, weight=w / tot)
                                              for b, w in new.items()], prof)
            changed += 1
    log("%s arms: %d sleeve vertices weighted again along the arm bones" % (name, changed))


def swap_hands(name, T, src_sk, fwd=(0.0, 1.0), X=None, R=None):
    """The generated hands cut off where their weight is mostly hand_l/hand_r (T is in the mannequin's reference pose by
    now), the source mannequin's hands (reference pose, own finger weights) appended in their place, their UVs squeezed
    onto a skin texel of the generated hand so they take its colour, their material slot the generated mesh's."""
    by_name, _infos = bones(T)
    hand = {by_name[b].index for b in ("hand_l", "hand_r")}
    fingers = {b.index for n, b in by_name.items() if any(k in n for k in ("thumb_", "index_", "middle_", "ring_",
                                                                           "pinky_"))}
    # the generated hand: the cut (hand-weighted; past a plane just short of the wrist; and the bare wrist skin, close
    # to the forearm's axis, back to HAND_SKIN_BACK, which the cuff stands clear of), and the skin point (the generated
    # skin nearest the wrist joint: the colour the cuff meets)
    # X: the fitted bones (bind to the mesh) where the generated hand is cut and the mannequin's hand goes; R: the
    # mannequin's reference pose, where its hand is taken from. Without them both are the bones' own.
    def at(Xs, b):
        if Xs is None:
            return by_name[b].world_transform.translation
        return unreal.Vector(*Xs[by_name[b].index][0])
    tp = positions(T)
    n = len(tp)
    W = read_weights(T, n)
    is_hand = [sum(w for b, w in W[v].items() if b in hand) >= HAND_CUT for v in range(n)]
    wrists = []
    for side in ("l", "r"):
        arm = subtree(_infos, by_name["lowerarm_" + side].index)
        h = at(X, "hand_" + side)
        e = at(X, "lowerarm_" + side)
        wrists.append((h.x, h.y, h.z))
        d = (h.x - e.x, h.y - e.y, h.z - e.z)
        dl = math.sqrt(sum(c * c for c in d))
        d = tuple(c / dl for c in d)
        for v in range(n):
            if is_hand[v] or sum(w for b, w in W[v].items() if b in arm) < 0.5:
                continue
            p = tp[v]
            r = (p[0] - h.x, p[1] - h.y, p[2] - h.z)
            along = r[0] * d[0] + r[1] * d[1] + r[2] * d[2]
            r2 = r[0] * r[0] + r[1] * r[1] + r[2] * r[2]
            radial = math.sqrt(max(0.0, r2 - along * along))
            if (along > -HAND_PLANE and r2 < HAND_RADIUS * HAND_RADIUS) or                     (-HAND_SKIN_BACK < along <= -HAND_PLANE and radial < HAND_SKIN_RADIUS):
                is_hand[v] = True
    cut, skin_t, best = [], None, None
    for t in tri_ids(T):
        vs = tri(T, t)
        if any(is_hand[v] for v in vs):
            cut.append(t)
            c = [sum(tp[v][k] for v in vs) / 3 for k in range(3)]
            dd = min(sum((c[k] - wj[k]) ** 2 for k in range(3)) for wj in wrists)
            if best is None or dd < best:
                best, skin_t = dd, t
    skin_uv = (0.5, 0.5)
    if skin_t is not None:
        u = [x for x in Q.get_triangle_u_vs(T, 0, skin_t) if isinstance(x, unreal.Vector2D)]
        if len(u) == 3:
            skin_uv = (sum(x.x for x in u) / 3, sum(x.y for x in u) / 3)
    mat_id = 0                                                  # the generated mesh has one material
    delete_tris(T, cut)

    # the mannequin's hands, in its reference pose, with its weights
    H = unreal.DynamicMesh()
    GA.copy_mesh_from_skeletal_mesh(src_sk, H, unreal.GeometryScriptCopyMeshFromAssetOptions(),
                                    unreal.GeometryScriptMeshReadLOD())
    hn = len(positions(H))
    HW = read_weights(H, hn)
    keep = [sum(w for b, w in HW[v].items() if b in hand or b in fingers) >= HAND_KEEP for v in range(hn)]
    hpos = positions(H)
    for side in ("l", "r"):                                     # and the wrist, up into the sleeve (no gap at the cuff)
        arm = subtree(_infos, by_name["lowerarm_" + side].index)
        h = at(R, "hand_" + side)
        e = at(R, "lowerarm_" + side)
        d = (h.x - e.x, h.y - e.y, h.z - e.z)
        dl = math.sqrt(sum(c * c for c in d))
        for v in range(hn):
            if keep[v] or sum(w for b, w in HW[v].items() if b in arm) < 0.5:
                continue
            p = hpos[v]
            if ((p[0] - h.x) * d[0] + (p[1] - h.y) * d[1] + (p[2] - h.z) * d[2]) / dl > -HAND_WRIST_BACK:
                keep[v] = True
    drop = []
    for t in tri_ids(H):
        a, b, c = tri(H, t)
        if not (keep[a] or keep[b] or keep[c]):
            drop.append(t)
    delete_tris(H, drop)
    if X is not None:                                           # each hand onto its fitted hand bone, rigidly
        hpos, HWk = positions(H), read_weights(H, len(positions(H)))
        moves = {}
        for side in ("l", "r"):
            i = by_name["hand_" + side].index
            (tr, qr), (tn, qn) = R[i], X[i]
            qd = qmul(qn, qconj(qr))
            rt = qrot(qd, tr)
            moves[side] = (subtree(_infos, by_name["lowerarm_" + side].index), qd,
                           (tn[0] - rt[0], tn[1] - rt[1], tn[2] - rt[2]))
        out = []
        for p, w in zip(hpos, HWk):
            side = max(moves, key=lambda sd: sum(x for k, x in w.items() if k in moves[sd][0]))
            _s, qd, td = moves[side]
            r = qrot(qd, p)
            out.append((r[0] + td[0], r[1] + td[1], r[2] + td[2]))
        set_positions(H, out)
    # skin colour: every UV onto a small patch around the skin point; one material slot
    U = unreal.GeometryScript_UVs
    U.scale_mesh_u_vs(H, 0, unreal.Vector2D(HAND_UV_SPREAD, HAND_UV_SPREAD), unreal.Vector2D(0, 0),
                      unreal.GeometryScriptMeshSelection())
    U.translate_mesh_u_vs(H, 0, unreal.Vector2D(*skin_uv), unreal.GeometryScriptMeshSelection())
    for m in range(0, 8):
        if m != mat_id:
            unreal.GeometryScript_Materials.remap_material_i_ds(H, m, mat_id)
    before = len(positions(T))
    ED.append_mesh(T, H, unreal.Transform(), False, unreal.GeometryScriptAppendMeshOptions())
    # the appended vertices' weights, set again from the mannequin by position (append may not carry them)
    prof = unreal.GeometryScriptBoneWeightProfile()
    hp = {tuple(round(c, 3) for c in p): v for v, p in enumerate(positions(H))}
    tp = positions(T)
    HW2 = read_weights(H, len(positions(H)))
    fixed = 0
    for v in range(before, len(tp)):
        src = hp.get(tuple(round(c, 3) for c in tp[v]))
        if src is None or not HW2[src]:
            continue
        BW.set_vertex_bone_weights(T, v, [unreal.GeometryScriptBoneWeight(bone_index=b, weight=w)
                                          for b, w in HW2[src].items()], prof)
        fixed += 1
    recompute_normals(T)
    log("%s hands: cut %d generated triangles, put in %d mannequin hand vertices (%d weights set), skin uv %.3f,%.3f"
        % (name, len(cut), len(tp) - before, fixed, skin_uv[0], skin_uv[1]))


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
    if built and not _G.get("HEROES_NO_HOOKUP") and hook_up(built) and _G.get("__name__") != "set_overlay":
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    return "\n".join(REPORT)


RESULT = main()
RESULT
