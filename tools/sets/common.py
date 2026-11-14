# Editor-side helpers for the set overlays (tools/sets/<area>.py). Runs inside the editor; see tools/sets/README.md.
# An overlay starts with:
#     import sys, importlib; sys.path.insert(0, r"C:\Jam\megajam\tools\sets"); import common as C; importlib.reload(C)
#     C.clear("set_hall")
# Units are cm. rot is a yaw in degrees, or (roll, pitch, yaw) as unreal.Rotator takes them.
# Box = (x0, x1, y0, y1, z0, z1). Overlay actors are labelled Set_*, never Prop_* (import_props' "unplace" deletes those).
import math
import os
import re

import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
ROOT = r"C:\Jam\megajam"
SHOTS = r"C:\Jam\Builds\qa\shots"
_IP = None          # import_props.py's namespace, loaded once per reload
_RT = {}            # (w, h) -> render target
_N = [0]


def ip():
    """tools/rodin/import_props.py's functions and tables (LOOK, fit, import_one, textures...), its steps not run."""
    global _IP
    if _IP is None:
        ns = {"__name__": "import_props_lib", "PROPS_ONLY": ("none",)}
        p = os.path.join(ROOT, "tools", "rodin", "import_props.py")
        exec(compile(open(p, encoding="utf-8").read(), p, "exec"), ns)
        _IP = ns
    return _IP


def world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def rot(r):
    if isinstance(r, (tuple, list)):
        return unreal.Rotator(float(r[0]), float(r[1]), float(r[2]))
    return unreal.Rotator(0.0, 0.0, float(r))


def tags(a):
    return [str(t) for t in a.get_editor_property("tags")]


def _set_tags(a, ts):
    a.set_editor_property("tags", [unreal.Name(t) for t in ts])


def actors():
    return eas.get_all_level_actors()


def find(pat):
    """Actors whose label matches pat (a regex, whole label), or an exact label."""
    rx = re.compile(r"^(%s)$" % pat)
    return [a for a in actors() if rx.match(a.get_actor_label()) or a.get_actor_label() == pat]


def _one(x):
    return x if isinstance(x, unreal.Actor) else None


# --- clear / hide ---------------------------------------------------------------------------------------------------
def clear(tag):
    """Removes what the last run with this tag placed, and shows what it hid. Returns (removed, shown)."""
    gone = shown = 0
    for a in actors():
        ts = tags(a)
        if tag in ts:
            eas.destroy_actor(a)
            gone += 1
            continue
        hid = [t for t in ts if t.startswith(tag + ":hid=")]
        if hid:
            names = set(hid[0].split("=", 1)[1].split("|"))
            for c in a.get_components_by_class(unreal.PrimitiveComponent):
                if c.get_name() in names:
                    c.set_visibility(True, False)
            _set_tags(a, [t for t in ts if not t.startswith(tag + ":hid=")])
            shown += 1
    return gone, shown


def hide(patterns, tag):
    """Hides (visibility only: collision, sight lines and navigation stay) every actor whose label matches one of the
    patterns. clear(tag) or unhide(tag) shows them again. Returns the labels hidden."""
    if isinstance(patterns, str):
        patterns = [patterns]
    rx = re.compile(r"^(%s)$" % "|".join(patterns))
    out = []
    for a in actors():
        if not rx.match(a.get_actor_label()) or tag in tags(a):
            continue
        comps = [c for c in a.get_components_by_class(unreal.PrimitiveComponent) if c.is_visible()]
        if not comps:
            continue
        for c in comps:
            c.set_visibility(False, False)
        _set_tags(a, tags(a) + ["%s:hid=%s" % (tag, "|".join(c.get_name() for c in comps))])
        out.append(a.get_actor_label())
    return out


def unhide(tag):
    """Shows what hide(..., tag) hid, leaving the tag's spawned actors in place."""
    n = 0
    for a in actors():
        ts = tags(a)
        hid = [t for t in ts if t.startswith(tag + ":hid=")]
        if hid:
            names = set(hid[0].split("=", 1)[1].split("|"))
            for c in a.get_components_by_class(unreal.PrimitiveComponent):
                if c.get_name() in names:
                    c.set_visibility(True, False)
            _set_tags(a, [t for t in ts if not t.startswith(tag + ":hid=")])
            n += 1
    return n


# --- spawn ----------------------------------------------------------------------------------------------------------
def _mesh(m):
    return eal.load_asset(m) if isinstance(m, str) else m


def _mat(m):
    return eal.load_asset(m) if isinstance(m, str) else m


def _collide(c, collide):
    if not collide:
        c.set_collision_profile_name("NoCollision")
        return
    Ch, R = unreal.CollisionChannel, unreal.CollisionResponseType      # as import_props: blocks the pawn only, so
    c.set_collision_enabled(unreal.CollisionEnabled.QUERY_ONLY)        # no sight line or camera probe answers it
    c.set_collision_object_type(Ch.ECC_WORLD_STATIC)
    c.set_collision_response_to_all_channels(R.ECR_IGNORE)
    c.set_collision_response_to_channel(Ch.ECC_PAWN, R.ECR_BLOCK)


def _dress(c, mat, collide, shadow):
    if mat is not None:
        for i, m in enumerate(mat if isinstance(mat, (list, tuple)) else [mat]):
            c.set_material(i, _mat(m))
    _collide(c, collide)
    for k, v in (("cast_shadow", bool(shadow)), ("receives_decals", False), ("can_ever_affect_navigation", False)):
        try:
            c.set_editor_property(k, v)
        except Exception:                                           # noqa: BLE001
            pass


def _label(tag, label, mesh):
    if not label:
        _N[0] += 1
        label = "%s_%s_%03d" % (tag, mesh.get_name().replace("SM_", ""), _N[0])
    if label.startswith("Prop_"):
        label = "Set_" + label[5:]
    if not label.startswith("Set_"):
        label = "Set_" + label
    return label


def spawn(mesh_path, loc, rot_=0.0, scale=(1, 1, 1), tag="set_misc", label=None, folder=None, collide=False,
          shadow=True, mat=None):
    """One StaticMeshActor. scale may be a number. mat: a material (path) or a list per slot; default the mesh's."""
    m = _mesh(mesh_path)
    if m is None:
        raise ValueError("no mesh " + str(mesh_path))
    sc = (scale, scale, scale) if isinstance(scale, (int, float)) else scale
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*loc), rot(rot_))
    a.set_actor_label(_label(tag, label, m))
    a.set_folder_path("Sets/" + (folder or tag))
    a.set_actor_scale3d(unreal.Vector(*sc))
    c = a.static_mesh_component
    c.set_static_mesh(m)
    _dress(c, mat, collide, shadow)
    _set_tags(a, [tag])
    return a


def scatter(mesh_path, transforms, tag="set_misc", label=None, folder=None, collide=False, shadow=False, mat=None):
    """Repeats on one InstancedStaticMeshComponent. transforms: [(loc, rot, scale)], rot and scale as spawn()."""
    m = _mesh(mesh_path)
    a = eas.spawn_actor_from_class(unreal.Actor, unreal.Vector(0.0, 0.0, 0.0))
    a.set_actor_label(_label(tag, label or "%s_%s_Instances" % (tag, m.get_name().replace("SM_", "")), m))
    a.set_folder_path("Sets/" + (folder or tag))
    sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    root = sds.k2_gather_subobject_data_for_instance(a)[0]
    h, fail = sds.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=root,
                                                                 new_class=unreal.InstancedStaticMeshComponent))
    if str(fail).strip():
        eas.destroy_actor(a)
        raise RuntimeError("scatter: " + str(fail))
    lib = unreal.SubobjectDataBlueprintFunctionLibrary
    c = lib.get_object(lib.get_data(h))
    c.set_static_mesh(m)
    _dress(c, mat, collide, shadow)
    for (loc, r, sc) in transforms:
        sc = (sc, sc, sc) if isinstance(sc, (int, float)) else sc
        c.add_instance(unreal.Transform(unreal.Vector(*loc), rot(r), unreal.Vector(*sc)), True)
    _set_tags(a, [tag])
    return a


# --- props ------------------------------------------------------------------------------------------------------------
def import_prop(name, look=None, reimport=False):
    """C:\\Jam\\Art\\Rodin\\<name>\\base_basic_pbr.glb -> /Game/Take/Props/<Name>/SM_<Name> with an MI_Prop_<Name> on
    the project's M_Prop (import_props' import and material). look: an import_props LOOK key to borrow ("cocktail"
    wood, "column" stone, "stanchion" steel, "chair" velvet, "piano" black...) or a LOOK-shaped tuple
    ((dark, light), tint, roughness, ...). Skips the import if the mesh exists, unless reimport. Returns the mesh path."""
    P = ip()
    path = P["mesh_path"](name)
    if reimport or not eal.does_asset_exist(path):
        P["import_one"](name)
        if not eal.does_asset_exist(path):
            raise RuntimeError("import_prop %s: %s" % (name, "; ".join(P["REPORT"][-3:])))
    lk = P["LOOK"].get(look, look) if isinstance(look, str) else look
    lk = lk or P["LOOK"].get(name) or P["LOOK"]["cocktail"]
    (dk, lt), tint, rough = lk[0], lk[1], lk[2]
    master = eal.load_asset(P["DEST"] + "/M_Prop")
    cap = P["cap"](name)
    mip = "%s/%s/MI_Prop_%s" % (P["DEST"], cap, cap)
    at = unreal.AssetToolsHelpers.get_asset_tools()
    mi = eal.load_asset(mip) if eal.does_asset_exist(mip) else at.create_asset(
        "MI_Prop_" + cap, "%s/%s" % (P["DEST"], cap), unreal.MaterialInstanceConstant,
        unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, master)
    base, normal = P["textures"](name)
    if base:
        mel.set_material_instance_texture_parameter_value(mi, "BaseTex", base)
    if normal:
        mel.set_material_instance_texture_parameter_value(mi, "NormalTex", normal)
    mel.set_material_instance_vector_parameter_value(mi, "Dark", P["lc"]([dk * c for c in tint]))
    mel.set_material_instance_vector_parameter_value(mi, "Light", P["lc"]([lt * c for c in tint]))
    mel.set_material_instance_scalar_parameter_value(mi, "Roughness", float(rough))
    mel.update_material_instance(mi)
    eal.save_asset(mip, only_if_is_dirty=False)
    m = eal.load_asset(path)
    for i in range(max(1, m.get_num_sections(0))):
        m.set_material(i, mi)
    eal.save_asset(path, only_if_is_dirty=False)
    return path


def fit(mesh, box, yaw=0.0, mode="fit"):
    """(loc, rot, scale) that stand mesh, turned by yaw, inside box, centred, on its floor: pass to spawn(*...).
    mode: fit (uniform, inside), fitxy (uniform from the footprint), stretch (per axis), stretchz."""
    m = _mesh(mesh)
    r = tuple(yaw) if isinstance(yaw, (tuple, list)) else (0.0, 0.0, float(yaw))
    loc, sc = ip()["fit"](m, box, r, mode)
    return loc, r, sc


def place(mesh, box, yaw=0.0, mode="fit", **kw):
    """spawn() the mesh fitted into box."""
    loc, r, sc = fit(mesh, box, yaw, mode)
    return spawn(mesh, loc, r, sc, **kw)


def bounds(x):
    """[x0, x1, y0, y1, z0, z1] of an actor, or the union of every actor matching a label (regex or exact)."""
    acts = [x] if isinstance(x, unreal.Actor) else find(x)
    if not acts:
        raise ValueError("bounds: no actor " + str(x))
    bs = []
    for a in acts:
        o, e = a.get_actor_bounds(False)
        bs.append([o.x - e.x, o.x + e.x, o.y - e.y, o.y + e.y, o.z - e.z, o.z + e.z])
    return [min(b[0] for b in bs), max(b[1] for b in bs), min(b[2] for b in bs), max(b[3] for b in bs),
            min(b[4] for b in bs), max(b[5] for b in bs)]


def top_of(x):
    """The z to stand something on (the top of its bounds)."""
    return bounds(x)[5]


def near(x, y, r=200.0, z=None, dz=300.0):
    """Labels of actors whose bounds come within r of (x, y) (and within dz of z if given): for finding culprits."""
    out = []
    for a in actors():
        o, e = a.get_actor_bounds(False)
        if e.x > 3000 or e.y > 3000:
            continue
        if abs(o.x - x) <= e.x + r and abs(o.y - y) <= e.y + r and (z is None or abs(o.z - z) <= e.z + dz):
            out.append(a.get_actor_label())
    return sorted(out)


# --- shots ------------------------------------------------------------------------------------------------------------
def _rt(w, h):
    if (w, h) not in _RT:
        _RT[(w, h)] = unreal.RenderingLibrary.create_render_target2d(
            world(), w, h, unreal.TextureRenderTargetFormat.RTF_RGBA8, unreal.LinearColor(0, 0, 0, 1), False)
    return _RT[(w, h)]


def _capture(name, out_dir, w, h, setup, hidden=(), unlit=False, ev=0.0):
    out_dir = out_dir or SHOTS
    os.makedirs(out_dir, exist_ok=True)
    name = name if name.endswith(".png") else name + ".png"
    cam = eas.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(0, 0, 0))
    try:
        c = cam.capture_component2d
        rt = _rt(w, h)
        c.set_editor_property("texture_target", rt)
        c.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
        c.set_editor_property("capture_every_frame", False)
        c.set_editor_property("capture_on_movement", False)
        if unlit:                                               # base colour only: for unlit corners
            c.set_editor_property("show_flag_settings", [unreal.EngineShowFlagsSetting("Lighting", False)])
        if ev:                                                  # exposure bias in stops: for dark rooms
            pp = c.get_editor_property("post_process_settings")
            pp.set_editor_property("override_auto_exposure_bias", True)
            pp.set_editor_property("auto_exposure_bias", float(ev))
            c.set_editor_property("post_process_settings", pp)
            c.set_editor_property("post_process_blend_weight", 1.0)
        for a in hidden:
            c.hide_actor_components(a, False)
        setup(cam, c)
        c.capture_scene()
        unreal.RenderingLibrary.export_render_target(world(), rt, out_dir, name)
    finally:
        eas.destroy_actor(cam)
    return os.path.join(out_dir, name)


def shot(name, pos, look_at, fov=70.0, w=1280, h=720, out_dir=None, unlit=False, ev=0.0):
    """A perspective PNG from pos looking at look_at, final colour with the level's post-process. The editor
    viewport is not used or moved. ev: exposure bias in stops (+2 to +4 shows a dark room; the game is darker).
    unlit: no lighting at all. Returns the path."""
    r = unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*pos), unreal.Vector(*look_at))

    def setup(cam, c):
        cam.set_actor_location_and_rotation(unreal.Vector(*pos), r, False, False)
        c.set_editor_property("fov_angle", float(fov))
    return _capture(name, out_dir, w, h, setup, unlit=unlit, ev=ev)


def plan(name, x0, x1, y0, y1, z_cut, px=1600, out_dir=None, unlit=False):
    """An orthographic top-down PNG of the box, with every actor that starts above z_cut left out (ceilings, upper
    floors). Image right = +X (east), image DOWN = +Y (north): pixel (u, v) is world (x0 + u*s, y0 + v*s), s printed
    in the return value as cm per pixel."""
    sx, sy = x1 - x0, y1 - y0
    w = px if sx >= sy else max(64, int(px * sx / sy))
    h = max(64, int(w * sy / sx))
    s = sx / w
    hidden = [a for a in actors() if a.get_actor_bounds(False)[0].z - a.get_actor_bounds(False)[1].z >= z_cut - 1.0]

    def setup(cam, c):
        cam.set_actor_location_and_rotation(unreal.Vector((x0 + x1) / 2, (y0 + y1) / 2, z_cut + 2000.0),
                                            unreal.Rotator(0.0, -90.0, -90.0), False, False)
        c.set_editor_property("projection_type", unreal.CameraProjectionMode.ORTHOGRAPHIC)
        c.set_editor_property("ortho_width", float(sx))
    p = _capture(name, out_dir, w, h, setup, hidden, unlit)
    return "%s  (%dx%d, %.2f cm/px, u->x from %.0f, v->y from %.0f)" % (p, w, h, s, x0, y0)


def save_level():
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    return "saved"
