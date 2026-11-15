# The reel swap's insert cameras on the placed S7_ReelSwap (same values as AScene7Swap's constructor), without a rebuild.
# Run inside the editor, PIE stopped, under the editor lock, from the repo root:
#   "$PY" -I tools/ue_mcp.py py tools/script/scene7_swap_shots.py
import unreal

V = unreal.Vector
# name: (From, To, Aim, Focal, Aperture), actor space (floor under the projector, +X the lens, operator on -Y).
SHOTS = {
    "Door": (V(-12, -78, 240), V(-8, -72, 236), V(-4, -10, 176), 24.0, 2.8),
    "Reel": (V(6, -82, 246), V(4, -74, 240), V(2, -4, 172), 26.0, 2.8),
    "Spindle": (V(6, -82, 246), V(5, -76, 242), V(2, -4, 176), 30.0, 2.8),
}


def setp(s, names, v):
    for n in names:
        try:
            s.set_editor_property(n, v)
            return
        except Exception:
            pass
    raise RuntimeError("no property " + "/".join(names))


eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actor = next(a for a in eas.get_all_level_actors() if a.get_class().get_name() == "Scene7Swap")
shots = actor.get_editor_property("shots")
out = {}
for k, v in shots.items():
    out[str(k)] = v
for name, (f, t, aim, focal, ap) in SHOTS.items():
    s = out[name]
    setp(s, ["from_", "from"], f)
    setp(s, ["to"], t)
    setp(s, ["aim"], aim)
    setp(s, ["focal"], focal)
    setp(s, ["aperture"], ap)
    out[name] = s
m = unreal.Map(unreal.Name, type(out["Door"]))
for k, v in out.items():
    m[unreal.Name(k)] = v
actor.set_editor_property("shots", m)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
d = actor.get_editor_property("shots")[unreal.Name("Door")]
print("Door now", d.get_editor_property("aim"), d.get_editor_property("focal"))
