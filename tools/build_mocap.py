"""Mixamo mocap onto the template mannequin (QA package mocap). Idempotent; run in the editor under the editor lock.

Source files: C:\\Jam\\Art\\Mixamo\\YBot.fbx (Y Bot with skin, T-pose) and <Key>.fbx (each clip without skin, FBX Binary,
30 fps, no keyframe reduction), downloaded from mixamo.com by hand through the site.

What it builds:
  /Game/Take/Anim/Mixamo/            Y Bot skeletal mesh, its own skeleton and materials
  /Game/Take/Anim/Mixamo/Clips/<Key> the raw clips on the Y Bot skeleton
  /Game/Take/Anim/Retarget/          IK_YBot, IK_Manny (auto retarget chains; FBIK on the mannequin) and
                                     RTG_YBot_To_Manny (default op stack, Root Motion op off: clips stay in place)
  /Game/Take/Anim/Mocap/<Key>        every clip retargeted to SK_Mannequin (SKM_Manny_Simple), what the game plays
  tools/mocap_clips.json             {Key: {asset, seconds, loop, hip_height (seated), source}}

The MCP Python call gives up at ~110 s, so every run works to a time budget and says MORE when it stopped early:
run it again until it says DONE. Stage and force flags come from the caller's globals (a wrapper that execs this
file), defaults: all stages, nothing forced.
  MOCAP_STAGE = "all" | "import" | "rig" | "retarget" | "measure"
  MOCAP_FORCE = True   re-import / re-retarget clips that already exist ("rig" always rebuilds the rigs)
  MOCAP_KEYS  = [...]  limit import / retarget to these keys
Run:  "$PY" -I tools/ue_mcp.py py tools/build_mocap.py 110
"""
import json
import os
import time

import unreal

STAGE = globals().pop("MOCAP_STAGE", "all")
FORCE = bool(globals().pop("MOCAP_FORCE", False))
ONLY = globals().pop("MOCAP_KEYS", None)
BUDGET = 85.0                       # seconds of work per run (the MCP call itself stops at ~110 s)
T0 = time.time()

SRC = r"C:\Jam\Art\Mixamo"
CHAR_FBX = "YBot.fbx"
MX_DIR = "/Game/Take/Anim/Mixamo"
CLIP_DIR = "/Game/Take/Anim/Mixamo/Clips"
RIG_DIR = "/Game/Take/Anim/Retarget"
OUT_DIR = "/Game/Take/Anim/Mocap"
MANNY = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
JSON_OUT = r"C:\Jam\megajam\tools\mocap_clips.json"
SRC_IK, TGT_IK, RTG = "IK_YBot", "IK_Manny", "RTG_YBot_To_Manny"

# Key: (loop, seated, name on mixamo.com (its description))
CLIPS = {
    "SitIdle_A": (True, True, "Sitting Idle (Sitting In Chair Hands Resting On Thighs)"),
    "SitIdle_B": (True, True, "Sitting (Sitting Looking Side To Side)"),
    "SitClap": (True, True, "Sitting Clap (Sitting Unethusiastic Clap)"),
    "SitTalk": (True, True, "Sitting Talking (Sitting And Talking To Another Person)"),
    "SitDrink": (False, True, "Sitting Drinking (Having A Seat And Drinking A Canned Beverage)"),
    "StandIdle_A": (True, False, "Idle (Weight Shift Idle)"),
    "StandIdle_B": (True, False, "Breathing Idle"),
    "StandIdle_C": (True, False, "Neutral Idle"),
    "Talk_A": (True, False, "Talking (General Conversation)"),
    "Talk_B": (True, False, "Talking (Asking A Question With Two Hands)"),
    "Talk_C": (True, False, "Talking (Asking A Question With One Hand)"),
    "Listen": (False, False, "Agreeing (Agreeing Yes)"),
    "Clap": (True, False, "Clapping (Clap While Standing)"),
    "Sip": (False, False, "Drinking (Male Drinking)"),
    "Laugh": (True, False, "Talking (Talking Finding Something Funny)"),
    "LookAround": (True, False, "Looking Around (Idle Stand Looking Around)"),
    "Wave": (True, False, "Waving (Waving)"),                  # one 0.5 s wave cycle
    "Refuse": (False, False, "Annoyed Head Shake (Shaking Head No Annoyed)"),
    "Speech_A": (True, False, "Talking (Asking A Question)"),
    "Speech_B": (True, False, "Rallying (Rallying The Crowd To Make Them Cheer)"),
    "Piano": (True, True, "Piano Playing (Playing Multiple Runs On A Piano)"),
    "Drums": (True, True, "Playing Drums (Basic Rock Beat)"),
    "Bass": (True, False, "Guitar Playing (Playing The Bass Guitar)"),
    "Bow": (False, False, "Quick Formal Bow"),
    "Lift": (False, False, "Pick Fruit (Picking Low Hanging Fruit)"),
    "Use": (False, False, "Button Pushing (Pushing A Button With The Right Hand)"),
    "Shrug": (False, False, "Shrugging (Shoulder Shrug)"),
    "Headset": (True, False, "Talking On A Cell Phone (Male Standing While Talking On A Cell Phone)"),
}

EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
SIDE_SRC = unreal.RetargetSourceOrTarget.SOURCE
SIDE_TGT = unreal.RetargetSourceOrTarget.TARGET
log = []


def out_of_time():
    return time.time() - T0 > BUDGET


def keys():
    ks = [k for k in CLIPS if os.path.exists(os.path.join(SRC, k + ".fbx"))]
    return [k for k in ks if ONLY is None or k in ONLY]


def asset_class(path):
    d = EAL.find_asset_data(path)
    return str(d.asset_class_path.asset_name) if d and d.is_valid() else ""


def find_in(folder, cls):
    for p in EAL.list_assets(folder, recursive=False, include_folder=False):
        if asset_class(p) == cls:
            return unreal.load_asset(p)
    return None


def ybot_mesh():
    return find_in(MX_DIR, "SkeletalMesh")


def import_fbx(fbx, dest, name=None, skeleton=None):
    """Legacy FbxImportUI options; AssetTools converts them for Interchange (InterchangeFbxAssetImportDataConverter)."""
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", fbx)
    t.set_editor_property("destination_path", dest)
    if name:
        t.set_editor_property("destination_name", name)
    t.set_editor_property("replace_existing", True)          # on purpose: a re-run replaces the last import
    t.set_editor_property("automated", True)
    t.set_editor_property("save", True)
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_as_skeletal", True)
    ui.set_editor_property("create_physics_asset", False)
    if skeleton is None:                                     # the skinned character, with its own new skeleton
        ui.set_editor_property("import_mesh", True)
        ui.set_editor_property("import_animations", False)
        ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
        ui.set_editor_property("import_materials", True)
        ui.set_editor_property("import_textures", True)
    else:                                                    # a clip onto that skeleton
        ui.set_editor_property("import_mesh", False)
        ui.set_editor_property("import_animations", True)
        ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
        ui.set_editor_property("skeleton", skeleton)
        ui.set_editor_property("import_materials", False)
        ui.set_editor_property("import_textures", False)
        ad = ui.get_editor_property("anim_sequence_import_data")
        ad.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
        ad.set_editor_property("import_bone_tracks", True)
        ad.set_editor_property("remove_redundant_keys", False)
    t.set_editor_property("options", ui)
    AT.import_asset_tasks([t])
    return [str(p) for p in (t.get_editor_property("imported_object_paths") or [])]


def stage_import():
    if ybot_mesh() is None or FORCE and ONLY is None:
        got = import_fbx(os.path.join(SRC, CHAR_FBX), MX_DIR)
        log.append("YBot: %d assets" % len(got))
    mesh = ybot_mesh()
    if mesh is None:
        raise RuntimeError("Y Bot import made no skeletal mesh in " + MX_DIR)
    skel = mesh.get_editor_property("skeleton")
    n = 0
    for k in keys():
        dst = "%s/%s" % (CLIP_DIR, k)
        if EAL.does_asset_exist(dst) and not FORCE:
            continue
        if out_of_time():
            return False
        got = import_fbx(os.path.join(SRC, k + ".fbx"), CLIP_DIR, k, skel)
        anims = [p for p in got if asset_class(p.split(".")[0]) == "AnimSequence"]
        if anims and anims[0].split(".")[0] != dst:              # Interchange may add a take suffix
            if EAL.does_asset_exist(dst):
                EAL.delete_asset(dst)
            EAL.rename_asset(anims[0].split(".")[0], dst)
        if not EAL.does_asset_exist(dst):
            log.append("%s: import made %s" % (k, got))
        n += 1
    log.append("imported %d clips" % n)
    return True


def fresh(name, cls, factory):
    p = "%s/%s" % (RIG_DIR, name)
    if EAL.does_asset_exist(p):
        EAL.delete_asset(p)
    return AT.create_asset(name, RIG_DIR, cls, factory)


def setup_rig(rig, mesh, fbik):
    c = unreal.IKRigController.get_controller(rig)
    c.set_skeletal_mesh(mesh)
    ok = c.apply_auto_generated_retarget_definition()
    if fbik:
        c.apply_auto_fbik()
    return ok, c.get_retarget_root(), len(c.get_retarget_chains()) if hasattr(c, "get_retarget_chains") else -1


def stage_rig():
    ybot, manny = ybot_mesh(), unreal.load_asset(MANNY)
    if EAL.does_asset_exist("%s/%s" % (RIG_DIR, RTG)):          # delete the user first, then what it uses
        EAL.delete_asset("%s/%s" % (RIG_DIR, RTG))
    src = fresh(SRC_IK, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    tgt = fresh(TGT_IK, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    log.append("IK_YBot %s; IK_Manny %s" % (setup_rig(src, ybot, False), setup_rig(tgt, manny, True)))
    rtg = fresh(RTG, unreal.IKRetargeter, unreal.IKRetargetFactory())
    rc = unreal.IKRetargeterController.get_controller(rtg)
    rc.set_ik_rig(SIDE_SRC, src)
    rc.set_ik_rig(SIDE_TGT, tgt)
    rc.set_preview_mesh(SIDE_SRC, ybot)
    rc.set_preview_mesh(SIDE_TGT, manny)
    rc.remove_all_ops()
    rc.add_default_ops()
    rc.assign_ik_rig_to_all_ops(SIDE_SRC, src)
    rc.assign_ik_rig_to_all_ops(SIDE_TGT, tgt)
    rc.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
    # Y Bot is in T-pose, the mannequin in A-pose: align the mannequin's retarget pose to the source's
    rc.auto_align_all_bones(SIDE_TGT)
    ops = []
    for i in range(rc.get_num_retarget_ops()):
        name = str(rc.get_op_name(i))
        if "root motion" in name.lower():                       # Mixamo has no root bone: keep the root still
            rc.set_retarget_op_enabled(i, False)
        ops.append("%s%s" % (name, "" if rc.get_retarget_op_enabled(i) else " (off)"))
    log.append("RTG ops: " + ", ".join(ops))
    for a in (src, tgt, rtg):
        EAL.save_loaded_asset(a, False)
    return True


def stage_retarget():
    rtg = unreal.load_asset("%s/%s" % (RIG_DIR, RTG))
    if rtg is None:
        raise RuntimeError("no retargeter: run the rig stage first")
    ybot, manny = ybot_mesh(), unreal.load_asset(MANNY)
    todo = [k for k in keys() if EAL.does_asset_exist("%s/%s" % (CLIP_DIR, k))
            and (FORCE or not EAL.does_asset_exist("%s/%s" % (OUT_DIR, k)))]
    done = 0
    # The batch op ticks Slate every frame (cancel check): with a realtime level viewport each tick re-renders the
    # level and a clip takes minutes instead of a second. Viewport realtime off for the run, back on after.
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.editor_set_viewport_realtime(False)
    try:
        return retarget_groups(todo, rtg, ybot, manny)
    finally:
        les.editor_set_viewport_realtime(True)


def retarget_groups(todo, rtg, ybot, manny):
    done = 0
    while todo:
        if out_of_time():
            return False
        group, todo = todo[:4], todo[4:]
        inp = unreal.IKRetargetBatchOperationInputs()
        inp.set_editor_property("assets_to_retarget", [EAL.find_asset_data("%s/%s" % (CLIP_DIR, k)) for k in group])
        inp.set_editor_property("source_mesh", ybot)
        inp.set_editor_property("target_mesh", manny)
        inp.set_editor_property("ik_retarget_asset", rtg)
        inp.set_editor_property("target_path", OUT_DIR)
        inp.set_editor_property("use_source_path", False)
        inp.set_editor_property("include_referenced_assets", False)
        inp.set_editor_property("overwrite_existing_files", True)
        made = unreal.IKRetargetBatchOperation.run_batch_retarget(inp)
        for d in made:
            EAL.save_asset(str(d.package_name), False)
        done += len(made)
    log.append("retargeted %d clips" % done)
    return True


def bone_z(pose, bone):
    return unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD).translation.z


def stage_measure():
    opts = unreal.AnimPoseEvaluationOptions()
    data, notes = {}, []
    for k, (loop, seated, source) in CLIPS.items():
        p = "%s/%s" % (OUT_DIR, k)
        if not EAL.does_asset_exist(p):
            continue
        a = unreal.load_asset(p)
        secs = float(a.get_play_length())
        ts = [secs * i / 8.0 for i in range(9)]
        poses = [unreal.AnimPoseExtensions.get_anim_pose_at_time(a, t, opts) for t in ts]
        hips = [bone_z(q, "pelvis") for q in poses]
        feet = [min(bone_z(q, "ball_l"), bone_z(q, "ball_r")) for q in poses]
        ref_ball = unreal.AnimPoseExtensions.get_ref_bone_pose(poses[0], "ball_l", unreal.AnimPoseSpaces.WORLD).translation.z
        e = {"asset": p, "seconds": round(secs, 2), "loop": loop, "source": "Mixamo: " + source}
        if seated:
            late = sorted(hips[4:])
            e["hip_height"] = round(late[len(late) // 2], 1)
            if hips[0] - e["hip_height"] > 20.0:                  # starts standing (sits down first)
                e["start_s"] = round(next(t for t, h in zip(ts, hips) if h - e["hip_height"] < 5.0), 2)
        data[k] = e
        notes.append("%s %.1fs hips %.0f..%.0f feet %+.1f" % (k, secs, min(hips), max(hips), min(feet) - ref_ball))
    with open(JSON_OUT, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=1)
        f.write("\n")
    log.append("json %d clips; " % len(data) + "; ".join(notes))
    return True


steps = {"import": [stage_import], "rig": [stage_rig], "retarget": [stage_retarget], "measure": [stage_measure],
         "all": [stage_import, stage_rig, stage_retarget, stage_measure]}[STAGE]
state = "DONE"
for s in steps:
    if s is stage_rig and STAGE == "all" and EAL.does_asset_exist("%s/%s" % (RIG_DIR, RTG)):
        continue                                               # "all" keeps an existing retargeter; "rig" rebuilds
    if not s():
        state = "MORE (run again)"
        break
RESULT = "%s [%s, %.0fs] %s" % (state, STAGE, time.time() - T0, " | ".join(log))
print("build_mocap: " + RESULT)
RESULT
