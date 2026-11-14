# Re-applies every set overlay present, in order, each in its own try/except, then saves the level. Runs inside the
# editor on /Game/Take/Lvl_Auction, after build_auction.py's chain (build_auction.py runs it last):
#     "<engine python>" -I tools/ue_mcp.py py tools/sets/run_all.py 600
# Every overlay is idempotent (it clears its own tag first), so this is safe to re-run.
import os
import traceback

import unreal

_SETS = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir() + "../tools/sets")
_TOOLS = os.path.dirname(_SETS)
_ORDER = [os.path.join(_SETS, n) for n in ("front.py", "hall.py", "rooms.py", "devices.py")] + \
    [os.path.join(_TOOLS, "build_heroes.py")]
_ORDER.append(os.path.join(_SETS, "lights.py"))   # --- QA look --- the lighting pass, last (it lights the dressing)
_report = []
for _p in _ORDER:
    if not os.path.exists(_p):
        _report.append("skip  %s (absent)" % os.path.basename(_p))
        continue
    try:
        exec(compile(open(_p, encoding="utf-8").read(), _p, "exec"),
             {"__name__": "set_overlay", "__file__": _p})
        _report.append("ok    %s" % os.path.basename(_p))
    except Exception:                                           # noqa: BLE001 (one broken overlay must not stop the rest)
        _report.append("FAIL  %s: %s" % (os.path.basename(_p), traceback.format_exc().strip().splitlines()[-1][:300]))
        unreal.log_error("[run_all] %s\n%s" % (_p, traceback.format_exc()))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("[run_all]\n" + "\n".join(_report))
"\n".join(_report)
