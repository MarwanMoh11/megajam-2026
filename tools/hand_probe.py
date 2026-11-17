# Hand probe: rig_probe's row (plain Manny, the heroes, featured NPCs) on one clip at one frame, and a close-up of
# each one's hands, framed on the hand bones. Shots: C:\Jam\Builds\qa\hands\<tag>_<clip>_<i>_<side>.png (i = row index,
# 0 Manny). Engine Python, repo root, under the editor lock:
#   "$PY" -I tools/hand_probe.py [tag] [Clip@sec ...]
#   "$PY" -I tools/rig_probe.py --stop          # afterwards: stop PIE, remove the probe (never save the level between)
import os, sys, time, json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rig_probe as RP

L = RP.L
OUT = r"C:\Jam\Builds\qa\hands"

SHOOT = r'''
import unreal
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
rt = unreal.RenderingLibrary.create_render_target2d(w, 520, 520, unreal.TextureRenderTargetFormat.RTF_RGBA8, unreal.LinearColor(0, 0, 0, 1), False)
cam = unreal.GameplayStatics.get_all_actors_with_tag(w, "RigProbeCam0")[0]
row = [a for a in unreal.GameplayStatics.get_all_actors_with_tag(w, "RigProbe") if isinstance(a, unreal.SkeletalMeshActor)]
row.sort(key=lambda a: a.get_actor_location().x)
k = 0
for i, a in enumerate(row):
    c = a.skeletal_mesh_component
    for side in ("l", "r"):
        h = c.get_socket_location("hand_" + side)
        m = c.get_socket_location("middle_02_" + side)
        look = unreal.Vector((h.x + m.x) / 2, (h.y + m.y) / 2, (h.z + m.z) / 2)
        pos = unreal.Vector(look.x, look.y - 55.0, look.z + 12.0)
        cam.set_actor_location_and_rotation(pos, unreal.MathLibrary.find_look_at_rotation(pos, look), False, True)
        cc = cam.capture_component2d
        cc.set_editor_property("texture_target", rt)
        cc.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
        cc.set_editor_property("fov_angle", 40.0)
        pp = cc.get_editor_property("post_process_settings")
        pp.set_editor_property("override_auto_exposure_method", True)
        pp.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
        pp.set_editor_property("override_auto_exposure_bias", True)
        pp.set_editor_property("auto_exposure_bias", %r)
        pp.set_editor_property("override_auto_exposure_apply_physical_camera_exposure", True)
        pp.set_editor_property("auto_exposure_apply_physical_camera_exposure", False)
        cc.set_editor_property("post_process_settings", pp)
        cc.set_editor_property("post_process_blend_weight", 1.0)
        cc.capture_scene()
        unreal.RenderingLibrary.export_render_target(w, rt, %r, "%%s_%%d_%%s.png" %% (%r, i, side))
        k += 1
"shot %%d hands" %% k
'''


def main():
    a = sys.argv[1:]
    s = L.sid()
    tag = a[0] if a else "hands"
    clips = a[1:] or ["Talk_B@1.6", "StandIdle_C@1.0"]
    ev = float(os.environ.get("RIG_EV", "-3"))
    os.makedirs(OUT, exist_ok=True)
    if "true" in json.dumps(L.tool(s, "EditorToolset.EditorAppToolset.IsPIERunning", {})).lower():
        print(L.tool(s, "EditorToolset.EditorAppToolset.StopPIE", {}))
        time.sleep(2.0)
    print(L.py(s, RP.SETUP % (RP.STUDIO, RP.GAP, RP.ROW)))
    L.tool(s, "EditorToolset.EditorAppToolset.StartPIE", {})
    time.sleep(5.0)
    for spec in clips:
        clip, t = spec.split("@")
        print(L.py(s, RP.POSE % ("/Game/Take/Anim/Mocap/" + clip, float(t))))
        time.sleep(0.6)
        name = "%s_%s" % (tag, clip)
        print(clip, L.py(s, SHOOT % (ev, OUT, name)))


if __name__ == "__main__":
    main()
