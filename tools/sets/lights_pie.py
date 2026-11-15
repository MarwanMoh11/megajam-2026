# QA lights (W3): play-in-editor shots through the game camera, for the lighting pass (tools/sets/lights.py).
# Run with the engine's Python from the repo root, under the editor lock:
#   "$PY" -I tools/sets/lights_pie.py <prefix> [place ...]      # prefix: before / after; no places = all
# Starts PIE if it is not running, presses through the title card, then for each place: warps Julian, sets the look,
# waits, and saves the game viewport to C:\Jam\Builds\qa\lights\<prefix>_<place>.png. It leaves PIE running: stop it
# with "--stop" as the only argument (always before releasing the lock).
import base64, json, os, sys, time
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import ue_mcp as U

OUT = r"C:\Jam\Builds\qa\lights"
# place: (feet x, y, z, look yaw, look pitch, extra console commands run first)
PLACES = {
    "street_n": (-3700.0, 1300.0, 0.0, 200.0, -12.0),       # the north pavement, the kerb lane's sedans
    "street_s": (-3650.0, -500.0, 0.0, 230.0, -12.0),       # the south pavement, newsstand, hydrant, sedans
    "lobby_desk": (-2780.0, 380.0, 0.0, 20.0, -8.0),        # the reception desk, its clerk, the newel lamps (W3: desk moved)
    "lobby_seat": (-1700.0, 600.0, 0.0, 45.0, -10.0),       # the seating group by the hall doors
    "hall_bar": (600.0, -900.0, 0.0, -60.0, -8.0),          # the bar on the south flank
    "hall_north": (600.0, 600.0, 0.0, 45.0, -8.0),          # the north flank, the phone desk
    "hall_band": (2800.0, 800.0, 0.0, 30.0, -6.0),          # the band
    "kitchen": (-700.0, -2900.0, 0.0, 0.0, -10.0),          # a side room
    "gallery": (0.0, 2450.0, 0.0, 0.0, -10.0),              # the preview gallery
    "office": (-500.0, 2450.0, 450.0, 0.0, -10.0),          # the director's office upstairs
    "cue_doorman": (-3750.0, -420.0, 0.0, 70.0, -25.0),     # the doorman's torch down the carpet (red tape beside)
    "cue_guard": (2450.0, -1000.0, 0.0, 50.0, -22.0),       # the hall guard's torch at the front of the house
    "cue_chief": (-2930.0, -420.0, 0.0, 90.0, -28.0),       # the chief's torch at the foot of the stair
}
PIE_WORLD = None


def sid():
    return U.session()


def py(s, src, timeout=60):
    r = U.toolset_call(s, "editor_toolset.toolsets.python_scripting.PythonScriptingTools.execute_python",
                       {"script": {"content": src, "language": "Python"}}, timeout)
    return "".join(c.get("text", "") for c in r.get("result", {}).get("content", []))


def console(s, *cmds):
    src = ("import unreal\nw = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()\n"
           + "".join("unreal.SystemLibrary.execute_console_command(w, %r)\n" % c for c in cmds)
           + "str(w.get_path_name() if w else None)")
    return py(s, src)


def tool(s, name, args, timeout=60):
    return U.toolset_call(s, name, args, timeout)


def save_image(res, path):
    for c in res.get("result", {}).get("content", []):
        if c.get("type") == "image":
            with open(path, "wb") as f:
                f.write(base64.b64decode(c["data"]))
            return True
    print("no image:", json.dumps(res)[:300])
    return False


def main():
    a = sys.argv[1:]
    s = sid()
    if a and a[0] == "--stop":
        print(tool(s, "EditorToolset.EditorAppToolset.StopPIE", {}))
        return
    prefix = a[0]
    names = a[1:] or list(PLACES)
    os.makedirs(OUT, exist_ok=True)
    running = json.dumps(tool(s, "EditorToolset.EditorAppToolset.IsPIERunning", {}))
    if "true" not in running.lower():
        tool(s, "EditorToolset.EditorAppToolset.StartPIE", {})
        time.sleep(3.0)
        console(s, "TakeConfirm")
        time.sleep(1.5)
    world = console(s, "stat unit") .strip()
    print("world", world)
    for n in names:
        p = PLACES[n]
        console(s, "TakeWarp %.0f %.0f %.0f" % p[:3], "TakeLook %.1f %.1f" % p[3:5])
        time.sleep(2.0)
        path = os.path.join(OUT, "%s_%s.png" % (prefix, n))
        res = tool(s, "SceneCaptureToolset.SceneCaptureToolset.CaptureGameViewport", {"world": {"refPath": "/Game/Take/UEDPIE_0_Lvl_Auction.Lvl_Auction"}, "maxResolution": 1280})
        print(n, save_image(res, path) and path)


if __name__ == "__main__":
    main()
