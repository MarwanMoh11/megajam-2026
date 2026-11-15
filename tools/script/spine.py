# Spine: places the one AScriptSequencer (tag script_spine) that plays the script's scenes in order.
# Owner: the spine session. Re-runnable; run under the editor lock, after the existing pipeline:
#     "<engine python>" -I tools/ue_mcp.py py tools/script/spine.py 60
import sys
import importlib

import unreal

sys.path.insert(0, r"C:\Jam\megajam\tools\sets")
import common as C   # noqa: E402

importlib.reload(C)

TAG = "script_spine"
C.clear(TAG)
out = []
cls = unreal.load_class(None, "/Script/Reel.ScriptSequencer")
if cls is None:
    out.append("AScriptSequencer not compiled: nothing placed")
else:
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    a = eas.spawn_actor_from_class(cls, unreal.Vector(0, 0, 200), unreal.Rotator(0, 0, 0))
    a.set_actor_label("Script_Sequencer")
    a.set_folder_path("Script")
    a.set_editor_property("tags", [unreal.Name(TAG)])
    clap = unreal.load_asset("/Game/Take/Audio/sfx_clapper")
    if clap:
        a.set_editor_property("splice_sound", clap)
    out.append("placed %s splice=%s" % (a.get_actor_label(), bool(clap)))
"; ".join(out)
