# Scene 6's route check (s6-backstage): a top-down plan of the backstage room, the vault anteroom, the corridor and the
# dock, and traces along Cole's route (what blocks a walk at knee and chest height). Run under the editor lock:
#   "$PY" -I tools/ue_mcp.py py tools/script/scene6_plan.py 110
import sys
import unreal
sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C

OUT = r"C:\Jam\Builds\script\s6"
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
OT = [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1, unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY2]   # WorldStatic, WorldDynamic
# Cole's route: the room, out through the round vault door, the anteroom, the corridor west, the dock door, the oak door.
ROUTE = [(5000, 300), (4850, -700), (4850, -1150), (4850, -1700), (4500, -2300), (4200, -2300), (3000, -2450),
         (3000, -2560), (3000, -2700), (3000, -2450), (3700, -2250), (3765, -2110)]
out = []
for (a, b) in zip(ROUTE, ROUTE[1:]):
    for z in (60.0, 130.0):
        hit = unreal.SystemLibrary.line_trace_single_for_objects(
            world, unreal.Vector(a[0], a[1], z), unreal.Vector(b[0], b[1], z), OT, False, [],
            unreal.DrawDebugTrace.NONE, True)
        if hit:
            t = hit.to_tuple()
            actor = t[9] if len(t) > 9 else None
            comp = t[10] if len(t) > 10 else None
            out.append("%s->%s z%d: %s/%s at %s" % (a, b, z, actor.get_actor_label() if actor else "?",
                                                   comp.get_name() if comp else "?", t[4]))
shot = C.plan("s6_route_plan", 2600, 5400, -3400, 1100, 400.0, px=1600, out_dir=OUT)
"plan %s; blocks: %s" % (shot, out or "none")
