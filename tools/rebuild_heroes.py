# Rebuild heroes / featured NPCs in place (no level edit, no level save): the names in C:\Jam\Builds\qa\rig\only.txt
# (comma separated; empty or missing: all of them). Under the editor lock:
#   "$PY" -I tools/ue_mcp.py py tools/rebuild_heroes.py 110
# One build takes about a minute; give each call one or two names (the MCP call stops waiting at ~110 s).
try:
    HEROES_ONLY = [n.strip() for n in open("C:/Jam/Builds/qa/rig/only.txt").read().split(",") if n.strip()] or None
except OSError:
    HEROES_ONLY = None
HEROES_FORCE = True
HEROES_NO_HOOKUP = True
exec(open(r"C:\Jam\megajam\tools\build_heroes.py", encoding="utf-8").read())
RESULT
