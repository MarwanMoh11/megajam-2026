# Rig probe: plain Manny beside the heroes and featured NPCs, every one on the same Mixamo clip at the same frame, shot
# from the front and the side by a scene capture far under the level (the game's view is left alone). Shots go to
# C:\Jam\Builds\qa\rig\<tag>_<clip>_<view>.png. Engine Python, repo root, under the editor lock:
#   "$PY" -I tools/rig_probe.py [tag] [Clip@sec ...]   # starts PIE if needed; default clips below
#   "$PY" -I tools/rig_probe.py --stop                  # stop PIE and remove the probe from the editor level
#                                                       (always before releasing the lock; never save the level between)
# ROW: who stands in the row, left to right (Manny first, the reference the clips were retargeted to).
import os, sys, time, json
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "sets"))
import lights_pie as L

OUT = r"C:\Jam\Builds\qa\rig"
STUDIO = (0.0, 0.0, -60000.0)
GAP = 110.0
ROW = ["/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"] + \
      ["/Game/Take/Cast/Heroes/%s/SKM_Hero_%s" % (n, n) for n in ("Julian", "Maya", "Vex")] + \
      ["/Game/Take/Cast/NPCs/%s/SKM_NPC_%s" % (n, n) for n in ("Vance", "Bodyguard", "Doorman")]
CLEAN = r'''
import unreal
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
k = 0
for a in unreal.GameplayStatics.get_all_actors_with_tag(w, "RigProbe"):
    a.destroy_actor()
    k += 1
"removed %d" % k
'''
if os.environ.get("RIG_NPCS"):                         # RIG_NPCS=Clerk,Chief,...: Manny and those NPCs instead
    ROW = ROW[:1] + ["/Game/Take/Cast/NPCs/%s/SKM_NPC_%s" % (n, n) for n in os.environ["RIG_NPCS"].split(",")]
CLIPS = ["StandIdle_C@1.0", "Talk_B@1.6", "Clap@0.6", "Use@1.2"]

SETUP = r'''
import unreal
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
for a in unreal.GameplayStatics.get_all_actors_with_tag(w, "RigProbe"):
    a.destroy_actor()
S, GAP, ROW = %r, %r, %r
def spawn(cls, loc, rot=unreal.Rotator(0, 0, 0)):
    a = EAS.spawn_actor_from_class(cls, unreal.Vector(*loc), rot, False)   # not transient: PIE copies only saved-able actors
    a.tags = [unreal.Name("RigProbe")]
    return a
n = len(ROW)
for i, path in enumerate(ROW):
    m = unreal.load_asset(path)
    x = S[0] + (i - (n - 1) / 2.0) * GAP
    a = spawn(unreal.SkeletalMeshActor, (x, S[1], S[2]), unreal.Rotator(0, 0, 180))   # mesh front +Y turned to face -Y, the camera
    c = a.skeletal_mesh_component
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    c.set_skeletal_mesh_asset(m)
    c.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
    c.set_editor_property("visibility_based_anim_tick_option", unreal.VisibilityBasedAnimTickOption.ALWAYS_TICK_POSE_AND_REFRESH_BONES)
floor = spawn(unreal.StaticMeshActor, (S[0], S[1], S[2] - 1))
floor.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
floor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Plane"))
floor.set_actor_scale3d(unreal.Vector(20, 20, 1))
for loc, inten in (((S[0] - 300, S[1] - 600, S[2] + 400), 120.0), ((S[0] + 500, S[1] - 300, S[2] + 200), 45.0),
                   ((S[0], S[1] + 500, S[2] + 300), 45.0), ((S[0] - 600, S[1] - 100, S[2] + 150), 30.0)):
    l = spawn(unreal.PointLight, loc)
    lc = l.point_light_component
    lc.set_mobility(unreal.ComponentMobility.MOVABLE)
    lc.set_intensity(inten)
    lc.set_attenuation_radius(3000.0)
    lc.set_cast_shadows(False)
for k in range(2):
    cam = spawn(unreal.SceneCapture2D, (S[0], S[1] - 900, S[2] + 100))
    cam.tags = [unreal.Name("RigProbe"), unreal.Name("RigProbeCam%%d" %% k)]
    cam.capture_component2d.set_editor_property("capture_every_frame", False)
    cam.capture_component2d.set_editor_property("capture_on_movement", False)
"spawned %%d" %% n
'''

POSE = r'''
import unreal
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
anim = unreal.load_asset(%r)
t = %r
k = 0
for a in unreal.GameplayStatics.get_all_actors_with_tag(w, "RigProbe"):
    if isinstance(a, unreal.SkeletalMeshActor):
        c = a.skeletal_mesh_component
        c.set_tickable_when_paused(True)
        a.set_tickable_when_paused(True)
        c.play_animation(anim, True)
        c.set_play_rate(0.0)
        c.set_position(t, False)
        k += 1
"posed %%d (paused %%s, dilation %%.2f)" %% (k, unreal.GameplayStatics.is_game_paused(w), unreal.GameplayStatics.get_global_time_dilation(w))
'''

SHOOT = r'''
import unreal
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
S, n, GAP = %r, %r, %r
rt = unreal.RenderingLibrary.create_render_target2d(w, %d, %d, unreal.TextureRenderTargetFormat.RTF_RGBA8, unreal.LinearColor(0, 0, 0, 1), False)
shots = %r
cam = unreal.GameplayStatics.get_all_actors_with_tag(w, "RigProbeCam0")[0]
for name, pos, look, fov in shots:
    r = unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*pos), unreal.Vector(*look))
    cam.set_actor_location_and_rotation(unreal.Vector(*pos), r, False, True)
    c = cam.capture_component2d
    c.set_editor_property("texture_target", rt)
    c.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    c.set_editor_property("capture_every_frame", False)
    c.set_editor_property("fov_angle", float(fov))
    pp = c.get_editor_property("post_process_settings")
    pp.set_editor_property("override_auto_exposure_method", True)
    pp.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
    pp.set_editor_property("override_auto_exposure_bias", True)
    pp.set_editor_property("auto_exposure_bias", %r)
    pp.set_editor_property("override_auto_exposure_apply_physical_camera_exposure", True)
    pp.set_editor_property("auto_exposure_apply_physical_camera_exposure", False)
    c.set_editor_property("post_process_settings", pp)
    c.set_editor_property("post_process_blend_weight", 1.0)
    c.capture_scene()
    unreal.RenderingLibrary.export_render_target(w, rt, %r, name + ".png")
"shot %%d" %% len(shots)
'''


def main():
    a = sys.argv[1:]
    s = L.sid()
    if a and a[0] == "--stop":                          # stop PIE, then take the probe out of the editor level
        print(L.tool(s, "EditorToolset.EditorAppToolset.StopPIE", {}))
        time.sleep(2.0)
        print(L.py(s, CLEAN))
        return
    tag = a[0] if a else "probe"
    clips = a[1:] or CLIPS
    exposure = float(os.environ.get("RIG_EV", "-3"))
    os.makedirs(OUT, exist_ok=True)
    if "true" in json.dumps(L.tool(s, "EditorToolset.EditorAppToolset.IsPIERunning", {})).lower():
        print(L.tool(s, "EditorToolset.EditorAppToolset.StopPIE", {}))
        time.sleep(2.0)
    print(L.py(s, SETUP % (STUDIO, GAP, ROW)))         # in the editor level: PIE copies it
    L.tool(s, "EditorToolset.EditorAppToolset.StartPIE", {})
    time.sleep(5.0)
    n = len(ROW)
    half = (n - 1) / 2.0 * GAP
    cx, cy, cz = STUDIO
    shots_for = lambda clip: [                                  # noqa: E731
        ("%s_%s_front" % (tag, clip), (cx, cy - 900, cz + 110), (cx, cy, cz + 100), 50.0),
        ("%s_%s_side" % (tag, clip), (cx - half - 330, cy - 120, cz + 120), (cx - half + GAP * 1.5, cy, cz + 100), 40.0),
        ("%s_%s_close" % (tag, clip), (cx - half + GAP, cy - 300, cz + 130), (cx - half + GAP, cy, cz + 115), 50.0),
    ]
    for spec in clips:
        clip, t = spec.split("@")
        print(L.py(s, POSE % ("/Game/Take/Anim/Mocap/" + clip, float(t))))
        time.sleep(0.6)
        print(clip, L.py(s, SHOOT % (STUDIO, n, GAP, 2400, 900, shots_for(clip)[:1], exposure, OUT)))
        print(clip, L.py(s, SHOOT % (STUDIO, n, GAP, 1200, 900, shots_for(clip)[1:], exposure, OUT)))


if __name__ == "__main__":
    main()
