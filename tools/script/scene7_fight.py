# Scene 7's fist fight (AScene7Fight, Reel/Source/Reel/Take/Script/Scene7Fight.*). Owner: s7-fight. A tagged,
# re-runnable overlay (tag script_s7fight), run in the editor under the editor lock:
#     "<engine python>" -I tools/ue_mcp.py py tools/script/scene7_fight.py 110
# Places the fight actor on Vance's fight mark in the security room (s7-route's layout: room x -3200..-2100,
# y -2000..-1400, floor z 450; Vance (-2650, -1680) facing north, yaw 90), so fight space +X = world +Y and the
# monitor bank on the south wall sits at fight X -280..-295. s7-route's AScene7Booth drives it
# (AScene7Fight::Find(this)->StartFight(this)). Then saves the level. The report goes to C:\Jam\Builds\script\s7fight\place.txt.
# Switches: S7F_OFF = True only clears. S7F_ANIMS = {role: asset path} overrides clips (the Mixamo fight clips).
import sys, importlib, traceback
sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C
importlib.reload(C)
import unreal

TAG = "script_s7fight"
MARK = (-2650.0, -1680.0, 450.0)
YAW = 90.0
OFF = bool(globals().get("S7F_OFF"))
ANIMS = globals().get("S7F_ANIMS") or {}
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
REPORT = []


def place():
    a = eas.spawn_actor_from_class(unreal.Scene7Fight, unreal.Vector(*MARK), unreal.Rotator(0.0, 0.0, YAW))
    a.set_actor_label("S7_Fight")
    C._set_tags(a, [TAG])
    if ANIMS:
        cur = dict(a.get_editor_property("anims"))
        for role, path in ANIMS.items():
            asset = unreal.load_asset(path)
            if asset:
                cur[unreal.Name(role)] = asset
        a.set_editor_property("anims", cur)
    return "placed %s at %s yaw %.0f" % (a.get_name(), MARK, YAW)


def run():
    gone, _ = C.clear(TAG)
    REPORT.append("cleared %d" % gone)
    if OFF:
        return
    for step in (place,):
        try:
            REPORT.append(step())
        except Exception:                                   # noqa: BLE001
            REPORT.append("%s FAILED: %s" % (step.__name__, traceback.format_exc().splitlines()[-1]))
    REPORT.append("saved %s" % C.save_level())


run()
print("\n".join(str(r) for r in REPORT))
