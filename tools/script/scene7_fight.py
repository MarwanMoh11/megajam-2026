# Scene 7's fist fight (AScene7Fight, Reel/Source/Reel/Take/Script/Scene7Fight.*). Owner: s7-fight. A tagged,
# re-runnable overlay (tag script_s7fight), run in the editor under the editor lock:
#     "<engine python>" -I tools/ue_mcp.py py tools/script/scene7_fight.py 110
# Places the fight actor on Vance's fight mark in the security room (s7-route's layout: room x -3200..-2100,
# y -2000..-1400, floor z 450; Vance (-2650, -1680) facing north, yaw 90), so fight space +X = world +Y and the
# monitor bank on the south wall sits at fight X -280..-295. s7-route's AScene7Booth drives it
# (AScene7Fight::Find(this)->StartFight(this)). Then saves the level. The report goes to C:\Jam\Builds\script\s7fight\place.txt.
# Switches: S7F_OFF = True only clears. S7F_ANIMS = {role: asset path} replaces the MIXAMO table below.
import os, sys, importlib, traceback
sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C
importlib.reload(C)
import unreal

TAG = "script_s7fight"
MARK = (-2650.0, -1680.0, 450.0)
YAW = 90.0
OFF = bool(globals().get("S7F_OFF"))
# The Mixamo fight clips (tools/build_mocap.py, Fight_*) by role; a role whose asset isn't imported keeps the template clip.
MIXAMO = {"Hook": "Fight_Hook", "Duck": "Fight_Duck", "Stance": "Fight_Stance", "Block": "Fight_Block",
          "HitHeavy": "Fight_HitHead", "HitMed": "Fight_HitStomach", "Slash": "Fight_Stab", "Lunge": "Fight_Stab",
          "Sweep": "Fight_Sweep", "SweepFall": "Fight_SweepFall", "FallBack": "Fight_FallBack", "Shove": "Fight_Shove",
          "Stumble": "Fight_Stumble"}
ANIMS = globals().get("S7F_ANIMS") or {r: "/Game/Take/Anim/Mocap/%s.%s" % (k, k) for r, k in MIXAMO.items()}
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
REPORT = []


def place():
    a = eas.spawn_actor_from_class(unreal.Scene7Fight, unreal.Vector(*MARK), unreal.Rotator(0.0, 0.0, YAW))
    a.set_actor_label("S7_Fight")
    C._set_tags(a, [TAG])
    mapped = []
    if ANIMS:
        cur = dict(a.get_editor_property("anims"))
        for role, path in ANIMS.items():
            asset = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path.split(".")[0]) else None
            if asset:
                cur[unreal.Name(role)] = asset
                mapped.append(role)
        a.set_editor_property("anims", cur)
    return "placed %s at %s yaw %.0f; mocap roles %s" % (a.get_name(), MARK, YAW, ",".join(mapped) or "none")


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
try:
    os.makedirs(r"C:\Jam\Builds\script\s7fight", exist_ok=True)
    with open(r"C:\Jam\Builds\script\s7fight\place.txt", "w") as _f:
        _f.write("\n".join(str(r) for r in REPORT))
except Exception:                                           # noqa: BLE001
    pass
