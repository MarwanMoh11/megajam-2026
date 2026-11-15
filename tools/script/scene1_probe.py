# s12-exterior probe (read-only but for shots): what the entrance signs and the street cars are, and shots of the
# Scene 1 and Scene 2 street. Run under the editor lock:
#     "<engine python>" -I tools/ue_mcp.py py tools/script/scene1_probe.py 110
import sys, importlib, math
sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C
importlib.reload(C)
import unreal

OUT = r"C:\Jam\Builds\script\s12"
LINES = []
P = lambda *a: LINES.append(" ".join(str(x) for x in a))
WORDS = ("QUEUE", "INVITED", "GUESTS", "PRESS", "CREW")
P("--- texts in the street")
for a in C.actors():
    if isinstance(a, unreal.TextRenderActor):
        v = a.get_actor_location()
        if v.x < -3200 and abs(v.y) < 2600:
            t = str(a.text_render.get_editor_property("text"))
            vis = a.text_render.is_visible()
            P("T %-24s %-28s (%.0f,%.0f,%.0f) yaw %.0f size %.0f vis %s" % (
                a.get_actor_label(), t[:28].replace("\n", "/"), v.x, v.y, v.z, a.get_actor_rotation().yaw,
                a.text_render.get_editor_property("world_size"), vis))
P("--- sign-ish meshes in the street")
for a in C.actors():
    lab = a.get_actor_label()
    if any(k in lab.lower() for k in ("sign", "board", "placard", "lane")):
        o, e = a.get_actor_bounds(False)
        if o.x < -3200 and abs(o.y) < 2600:
            comps = a.get_components_by_class(unreal.PrimitiveComponent)
            P("M %-32s c(%.0f,%.0f,%.0f) e(%.0f,%.0f,%.0f) vis %s" % (lab, o.x, o.y, o.z, e.x, e.y, e.z,
                  [c.is_visible() for c in comps][:3]))
P("--- cars")
for a in C.actors():
    lab = a.get_actor_label()
    if "sedan" in lab.lower() or "car" in lab.lower() and "carpet" not in lab.lower() and "cart" not in lab.lower():
        o, e = a.get_actor_bounds(False)
        if o.x < -3200:
            comps = a.get_components_by_class(unreal.StaticMeshComponent)
            sm = comps[0].static_mesh.get_path_name().split(".")[0] if comps and comps[0].static_mesh else "-"
            P("C %-30s c(%.0f,%.0f,%.0f) e(%.0f,%.0f,%.0f) vis %s %s s %s" % (
                lab, o.x, o.y, o.z, e.x, e.y, e.z, comps[0].is_visible() if comps else "-", sm,
                a.get_actor_scale3d()))
P("--- lorry / van meshes")
for p in unreal.EditorAssetLibrary.list_assets("/Game/Take/Props", recursive=True):
    if any(k in p.lower() for k in ("lorry", "sedan", "van")):
        P("A", p)
for a in C.actors():
    lab = a.get_actor_label()
    if "lorry" in lab.lower():
        o, e = a.get_actor_bounds(False)
        P("L %s c(%.0f,%.0f,%.0f) e(%.0f,%.0f,%.0f)" % (lab, o.x, o.y, o.z, e.x, e.y, e.z))

# shots: the Scene 2 cross (steps -> windshield), the street from the steps, the queue, and the van
S, W = (-3400.0, 250.0, 220.0), (-5380.0, 1080.0, 170.0)
LOOK = (-5380.0, 1330.0, 160.0)
for i, t in enumerate((0.25, 0.5, 0.75)):
    p = tuple(S[k] + (W[k] - S[k]) * t for k in range(3))
    P(C.shot("cross_%d" % i, p, LOOK, 62.0, out_dir=OUT))
P(C.shot("street_from_steps", (-3400.0, 250.0, 220.0), (-5000.0, 900.0, 100.0), 70.0, out_dir=OUT))
P(C.shot("queue_view", (-4250.0, 600.0, 180.0), (-3500.0, 250.0, 150.0), 70.0, out_dir=OUT))
P(C.shot("van_3q", (-4700.0, 800.0, 220.0), (-5380.0, 1550.0, 120.0), 60.0, out_dir=OUT))
P(C.shot("van_side", (-4650.0, 1550.0, 160.0), (-5380.0, 1550.0, 130.0), 70.0, out_dir=OUT))
for a in C.actors():
    lab = a.get_actor_label()
    comps = a.get_components_by_class(unreal.StaticMeshComponent)
    if comps and comps[0].static_mesh and "lorry" in comps[0].static_mesh.get_name().lower():
        o, e = a.get_actor_bounds(False)
        P("LORRY", lab, o, e)
        P(C.shot("lorry", (o.x - 900.0, o.y + 700.0, 350.0), (o.x, o.y, o.z), 55.0, out_dir=OUT))
        break
"\n".join(LINES)
