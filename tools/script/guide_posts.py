# Places the guidance posts (AStageGuide, TakeGuide.h) in Lvl_Auction from the table below, by label, idempotent:
# existing posts with a "Guide_" label not in the table are deleted, the rest are spawned or updated, then saved.
# Engine Python, repo root, under the editor lock, editor running (not in PIE):
#   "$PY" -I tools/script/guide_posts.py
# Rows come from the scene owners (docs: shared notes hud-guidance_GUIDE_API.md). Owner: hud-guidance.
import os, sys, json
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "sets"))
import lights_pie as L

# label: (scene, step, (x, y, z), yaw, {fields})   fields are AStageGuide properties (python names)
POSTS = {
}

SRC = r'''
import unreal, json
posts = json.loads(%r)
sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
have = {a.get_actor_label(): a for a in sub.get_all_level_actors() if a.get_actor_label().startswith("Guide_")}
out = []
for label, a in have.items():
    if label not in posts:
        sub.destroy_actor(a); out.append("deleted " + label)
for label, (scene, step, loc, yaw, fields) in posts.items():
    a = have.get(label)
    if a is None:
        a = sub.spawn_actor_from_class(unreal.StageGuide, unreal.Vector(*loc), unreal.Rotator(0, 0, yaw))
        a.set_actor_label(label); out.append("spawned " + label)
    else:
        a.set_actor_location(unreal.Vector(*loc), False, False); a.set_actor_rotation(unreal.Rotator(0, 0, yaw), False)
        out.append("updated " + label)
    a.set_editor_property("scene_number", scene)
    a.set_editor_property("step", step)
    for k, v in fields.items():
        if k == "highlight":
            t = [x for x in sub.get_all_level_actors() if x.get_actor_label() == v]
            a.set_editor_property("highlight", t[0] if t else None)
            if not t: out.append("  no actor labelled " + v)
        elif k in ("require_flag", "done_flag"):
            a.set_editor_property(k, unreal.Name(v))
        else:
            a.set_editor_property(k, v)
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)  # one file per actor: save the actors too
print("\n".join(out) or "nothing to do")
'''


def main():
    s = L.sid()
    print(L.py(s, SRC % json.dumps(POSTS), 90))


if __name__ == "__main__":
    main()
