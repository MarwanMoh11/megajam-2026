# hud-guidance's PIE checks into C:\Jam\Builds\script\hud\. Engine Python, repo root, under the editor lock:
#   "$PY" -I tools/script/hud_guide_pie.py probe 1 3 5      # per scene: a shot + small visible meshes near Vance
#   "$PY" -I tools/script/hud_guide_pie.py lost 5           # per scene: Reel.Guide.Lost, then shots of the marker
#   "$PY" -I tools/script/hud_guide_pie.py choice           # Scene 3's choice cards (Reel.S3 flow), several sizes
#   "$PY" -I tools/script/hud_guide_pie.py --stop
import os, sys, time, json
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "sets"))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import lights_pie as L

OUT = r"C:\Jam\Builds\script\hud"
WORLD = {"world": {"refPath": "/Game/Take/UEDPIE_0_Lvl_Auction.Lvl_Auction"}, "maxResolution": 1280}

PROBE = r'''
import unreal
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
cm = unreal.GameplayStatics.get_player_camera_manager(w, 0)
cam = cm.get_camera_location()
fwd = unreal.MathLibrary.get_forward_vector(cm.get_camera_rotation())
out = []
pawn = unreal.GameplayStatics.get_player_pawn(w, 0)
out.append("cam %s pawn %s %s" % (cam, pawn.get_name() if pawn else None, pawn.get_actor_location() if pawn else None))
for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Actor):
    for c in a.get_components_by_class(unreal.PrimitiveComponent):
        if not c.is_visible() or c.get_editor_property("hidden_in_game"):
            continue
        o, ext, r = unreal.SystemLibrary.get_component_bounds(c)
        if r > 70 or r < 1:
            continue
        d = o - cam
        ahead = d.dot(fwd)
        if ahead < 30 or ahead > 900:
            continue
        side = (d - fwd * ahead).length()
        if side > ahead * 0.6:
            continue
        mesh = ""
        if isinstance(c, unreal.StaticMeshComponent) and c.static_mesh:
            mesh = c.static_mesh.get_name()
        out.append("%5.0f ahead %4.0f side r%3.0f  %s.%s [%s] %s" % (ahead, side, r, a.get_name(), c.get_name(), c.get_class().get_name(), mesh))
"\n".join(out[:60])
'''


def snap(s, name):
    res = L.tool(s, "SceneCaptureToolset.SceneCaptureToolset.CaptureGameViewport", WORLD)
    path = os.path.join(OUT, "%s.png" % name)
    print(name, L.save_image(res, path) and path, flush=True)


def ensure_pie(s):
    if "true" not in json.dumps(L.tool(s, "EditorToolset.EditorAppToolset.IsPIERunning", {})).lower():
        L.tool(s, "EditorToolset.EditorAppToolset.StartPIE", {})
        time.sleep(4.0)
        L.console(s, "TakeConfirm")
        time.sleep(1.5)


def main():
    a = sys.argv[1:]
    s = L.sid()
    if not a or a[0] == "--stop":
        print(L.tool(s, "EditorToolset.EditorAppToolset.StopPIE", {}))
        return
    os.makedirs(OUT, exist_ok=True)
    ensure_pie(s)
    mode, scenes = a[0], [int(x) for x in a[1:]]
    if mode == "probe":
        for n in scenes:
            L.console(s, "Reel.Script.Jump %d" % n)
            time.sleep(6.0)
            snap(s, "probe_s%d" % n)
            print(L.py(s, PROBE, 60), flush=True)
    elif mode == "lost":
        for n in scenes:
            L.console(s, "Reel.Script.Jump %d" % n)
            time.sleep(6.0)
            L.console(s, "Reel.Guide.Lost")
            time.sleep(2.5)
            snap(s, "lost_s%d" % n)
    elif mode == "choice":
        L.console(s, "Reel.Script.Jump 3")
        time.sleep(float(a[1]) if len(a) > 1 else 9.0)
        snap(s, "choice_s3")


if __name__ == "__main__":
    main()
