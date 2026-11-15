# The sound's play-in-editor pass (QA Nov 15 evening): logs the script take's mix through every scene.
# Engine Python, repo root, under the editor lock:
#   "$PY" -I tools/audio/audio_pie.py [seconds per scene, default 7]
# Starts PIE with Reel.Audio.Log 1 (UTakeAudioComponent logs the mix and every playing audio component once a
# second), plays the title card, jumps through scenes 1..8 (and Scene 7's parts 701-703), fails Scene 3 once (the
# retake's tape-stop and the score back on the roll), finishes Scene 8 into the credits, stops PIE, then prints the
# log's AUDIO and SCRIPT lines from Reel/Saved/Logs/Reel.log into C:\Jam\Builds\qa\audio_pie.log.
import os, sys, time, json
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "sets"))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import lights_pie as L

LOG = r"C:\Jam\megajam\Reel\Saved\Logs\Reel.log"
OUT = r"C:\Jam\Builds\qa\audio_pie.log"


def main():
    per = float(sys.argv[1]) if len(sys.argv) > 1 else 7.0
    s = L.sid()
    start = os.path.getsize(LOG) if os.path.exists(LOG) else 0
    if "true" in json.dumps(L.tool(s, "EditorToolset.EditorAppToolset.IsPIERunning", {})).lower():
        L.tool(s, "EditorToolset.EditorAppToolset.StopPIE", {})
        time.sleep(2.0)
    L.tool(s, "EditorToolset.EditorAppToolset.StartPIE", {})
    time.sleep(4.0)
    L.console(s, "Reel.Audio.Log 1")
    time.sleep(3.0)
    L.console(s, "TakeConfirm")
    time.sleep(2.0)
    try:
        for n in [1, 2, 3, 4, 5, 6, 7, 701, 702, 703, 8]:
            L.console(s, "Reel.Script.Jump %d" % n)
            time.sleep(per)
            if n == 3:
                L.console(s, "Reel.Script.Fail")
                time.sleep(4.0)
        L.console(s, "Reel.Script.Finish")
        time.sleep(6.0)
    finally:
        L.console(s, "Reel.Audio.Log 0")
        print(L.tool(s, "EditorToolset.EditorAppToolset.StopPIE", {}), flush=True)
    time.sleep(1.0)
    with open(LOG, "r", encoding="utf-8", errors="replace") as f:
        f.seek(start)
        lines = [ln for ln in f.read().splitlines()
                 if "AUDIO" in ln or "SCRIPT " in ln or "TAKE band" in ln or "LogAudio" in ln and "Warning" in ln]
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    print("%d lines -> %s" % (len(lines), OUT), flush=True)


if __name__ == "__main__":
    main()
