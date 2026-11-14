"""Generates the voices, sound effects and music with ElevenLabs into C:\\Jam\\Audio\\raw\\<id>.mp3.

  python tools/audio/generate.py --dry-run [what]      characters and estimated credits, nothing sent
  python tools/audio/generate.py [what] [--force] [--max-credits N]

`what` is any mix of vo, sfx, music and ids (default: everything). An id whose raw file exists is skipped unless
--force (a retake). --max-credits stops before an item would take the run's estimate past N. Run process.py after.
The key comes from the environment inside el_api and is never printed.
"""
import json
import os
import socket
import sys
import time
import urllib.error

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import el_api  # noqa: E402
import manifest as M  # noqa: E402
from audio_io import RAW_DIR  # noqa: E402

SFX_CREDITS = 200            # per generation (pricing page, Nov 2026); the dry run's estimate
MUSIC_CREDITS_PER_MIN = 900


def items():
    """(category, id, estimate, call) for everything in the manifest."""
    out = []
    for (lid, speaker, text, _key, _event, say) in M.LINES:
        words = say or text
        v = M.VOICES[speaker]
        st, sim, style = v["settings"]
        settings = {"stability": st, "similarity_boost": sim, "style": style, "use_speaker_boost": True}
        if "speed" in v:
            settings["speed"] = v["speed"]
        out.append(("vo", lid, len(words),
                    lambda w=words, vid=v["voice_id"], s=settings: el_api.tts(vid, w, M.TTS_MODEL, s)))
    for (sid, _event, secs, prompt, loop) in M.SFX:
        out.append(("sfx", sid, SFX_CREDITS,
                    lambda p=prompt, d=secs, l=loop: el_api.sound_effect(p, d, prompt_influence=0.6, loop=l)))
    for (mid, _event, _bars, gen_s, kind, prompt) in M.MUSIC:
        if kind == "music":
            est = int(round(MUSIC_CREDITS_PER_MIN * gen_s / 60.0))
            out.append(("music", mid, est, lambda p=prompt, g=gen_s: el_api.music(p, int(g * 1000))))
        else:
            out.append(("music", mid, SFX_CREDITS,
                        lambda p=prompt, g=gen_s: el_api.sound_effect(p, g, prompt_influence=0.7)))
    return out


def wanted(argv):
    sel = [a for a in argv if not a.startswith("--") and not a.isdigit()]
    force = "--force" in argv
    cap = None
    if "--max-credits" in argv:
        cap = int(argv[argv.index("--max-credits") + 1])
    picked = []
    for (cat, iid, est, call) in items():
        if sel and cat not in sel and iid not in sel:
            continue
        if not force and os.path.exists(os.path.join(RAW_DIR, iid + ".mp3")):
            continue
        picked.append((cat, iid, est, call))
    return picked, force, cap


def call_with_retry(call):
    for attempt in range(3):
        try:
            return call()
        except urllib.error.URLError as e:
            # Only a connection that never reached the server is retried (nothing was billed).
            if isinstance(e.reason, (socket.gaierror, ConnectionRefusedError, ConnectionResetError)) and attempt < 2:
                time.sleep(5 * (attempt + 1))
                continue
            raise


def main(argv):
    picked, force, cap = wanted(argv)
    total = sum(p[2] for p in picked)
    by_cat = {}
    for (cat, _iid, est, _c) in picked:
        n, c = by_cat.get(cat, (0, 0))
        by_cat[cat] = (n + 1, c + est)
    print("to generate: " + ", ".join(f"{cat} {n} (~{c} credits)" for cat, (n, c) in by_cat.items())
          + f"; total ~{total} credits" + (" (forced)" if force else ""))
    if "--dry-run" in argv or not picked:
        return
    os.makedirs(RAW_DIR, exist_ok=True)
    try:
        before = el_api.subscription()
    except Exception as e:  # the counter is a report, not a gate
        before = {"used": None, "limit": None}
        print(f"credits before: unknown ({e})")
    else:
        print(f"credits used before: {before['used']} of {before['limit']}")
    spent_est = 0
    log = open(os.path.join(RAW_DIR, "log.jsonl"), "a")
    for (cat, iid, est, call) in picked:
        if cap is not None and spent_est + est > cap:
            print(f"stop: {iid} would pass --max-credits {cap}")
            break
        t0 = time.time()
        try:
            data = call_with_retry(call)
        except (el_api.ApiError, urllib.error.URLError, TimeoutError, socket.timeout) as e:
            print(f"FAILED {iid}: {e}")
            log.write(json.dumps({"id": iid, "ok": False, "error": str(e)[:300], "time": time.ctime()}) + "\n")
            continue
        with open(os.path.join(RAW_DIR, iid + ".mp3"), "wb") as f:
            f.write(data)
        spent_est += est
        log.write(json.dumps({"id": iid, "ok": True, "bytes": len(data), "est": est, "secs": round(time.time() - t0, 1),
                              "time": time.ctime()}) + "\n")
        log.flush()
        print(f"ok {iid} ({len(data) // 1024} KB, {time.time() - t0:.1f} s)")
    try:
        after = el_api.subscription()
        run = after["used"] - before["used"] if before["used"] is not None else "?"
        print(f"credits used after: {after['used']} of {after['limit']} (this run: {run}, estimated {spent_est})")
    except Exception as e:
        print(f"credits after: unknown ({e}); estimated this run {spent_est}")


if __name__ == "__main__":
    main(sys.argv[1:])
