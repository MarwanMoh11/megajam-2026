# Scene 1's play-in-editor run and shots into C:\Jam\Builds\script\s1\. Engine Python, repo root, under the editor lock:
#   "$PY" -I tools/script/scene1_pie.py [prefix]        # default prefix "pie"
#   "$PY" -I tools/script/scene1_pie.py --stop          # stop PIE (always before releasing the lock)
# Starts PIE (if needed), presses through the title card, Reel.Script.Jump 1, then plays the scene through: the queue,
# the floor manager, the handover, the bolt, the directive, the guests at the plate, Vance walked up (TakeWalkTo) to
# "Hand on the glass." and the cut to Scene 2. Then Scene 2's end from Scene 1's side: PalmAccepted (Python).
import os, sys, time, json
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "sets"))
import lights_pie as L

OUT = r"C:\Jam\Builds\script\s1"
WORLD = {"world": {"refPath": "/Game/Take/UEDPIE_0_Lvl_Auction.Lvl_Auction"}, "maxResolution": 1280}
SCAN = (-3610, 320)


def snap(s, name, prefix):
    res = L.tool(s, "SceneCaptureToolset.SceneCaptureToolset.CaptureGameViewport", WORLD)
    path = os.path.join(OUT, "%s_%s.png" % (prefix, name))
    print(name, L.save_image(res, path) and path, flush=True)


def log(s, pattern, n=40):
    res = L.tool(s, "ConsoleToolset.LogsToolset.GetLogEntries", {"pattern": pattern, "maxEntries": n})
    text = json.dumps(res)
    for part in text.split("\\n"):
        if "S1 " in part or "QUEUE" in part or "SCRIPT" in part:
            print(part[:200])


def main():
    a = sys.argv[1:]
    s = L.sid()
    if a and a[0] == "--stop":
        print(L.tool(s, "EditorToolset.EditorAppToolset.StopPIE", {}))
        return
    prefix = a[0] if a else "pie"
    os.makedirs(OUT, exist_ok=True)
    if "true" not in json.dumps(L.tool(s, "EditorToolset.EditorAppToolset.IsPIERunning", {})).lower():
        L.tool(s, "EditorToolset.EditorAppToolset.StartPIE", {})
        time.sleep(4.0)
        L.console(s, "TakeConfirm")
        time.sleep(1.5)
    L.console(s, "Reel.Script.Jump 1")
    t0 = time.time()

    def at(sec):
        d = t0 + sec - time.time()
        if d > 0:
            time.sleep(d)

    at(1.5); snap(s, "1_queue", prefix)
    at(4.0); snap(s, "2_manager", prefix)
    at(7.0); snap(s, "3_whisper", prefix)
    at(11.5); snap(s, "4_bolted", prefix)
    at(14.0); snap(s, "5_directive", prefix)
    for k in range(8):                                # Vance keeps his place, then steps up to the plate
        at(16.0 + 2.0 * k)
        L.console(s, "TakeWalkTo %d %d" % SCAN)
        if k == 1:
            snap(s, "6_beep", prefix)
        if k == 5:
            snap(s, "7_plate", prefix)
    at(34.0); snap(s, "8_after", prefix)
    log(s, "S1 beat|SCRIPT|QUEUE script", 60)


if __name__ == "__main__":
    main()
