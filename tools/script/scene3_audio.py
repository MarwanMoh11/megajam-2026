"""Scene 3 (the foyer, the critic, the main hall) voice lines and the champagne splash:
ElevenLabs -> C:\\Jam\\Audio\\Script\\S3 -> /Game/Take/Audio/Script/S3 (names = AScene3Foyer's voices, scene3.py).

Outside the editor (the audio venv: C:\\Jam\\venv-audio\\Scripts\\python.exe):
  python tools/script/scene3_audio.py [--dry-run] [--force] [id ...]   generate the missing raw MP3s, then process all
  python tools/script/scene3_audio.py --process-only [id ...]          re-process the raw MP3s into WAVs
Inside the editor (hold the editor lock):
  "$PY" -I tools/ue_mcp.py py tools/script/scene3_audio.py             import every S3 WAV, replacing and saving
Then rerun tools/script/scene3.py so the scene actor picks the sounds up.

Same shape as tools/script/scene2_audio.py (tools/audio read-only). Voices: VANCE = Julian's (Callum), MAX = the voice
s2-van designed (v2: W61sMqsWccdiqyhEKpI7, higher and faster than the doorman, on the earpiece band-pass), the GUARD = the rope guard's (Chris), the CRITIC =
George (an older gentleman, pushed tipsy and theatrical).
"""
import os
import sys

try:
    import unreal  # present only inside the editor
except ImportError:
    unreal = None

OUT = r"C:\Jam\Audio\Script\S3"
RAW = os.path.join(OUT, "raw")
DEST = "/Game/Take/Audio/Script/S3"
REPO = r"C:\Jam\megajam"
MAX_VOICE = "W61sMqsWccdiqyhEKpI7"

SFX = [("S3_Splash", 0.6, "A splash of champagne thrown from a coupe glass onto a dinner jacket: a short wet slosh "
        "and spatter on cloth, a tiny glass clink, close, dry, indoors")]
SFX_CREDITS = 200

# id, voice key in tools/audio/manifest.VOICES (None = MAX), text, (stability, similarity, style, speed), earpiece
VO = [
    ("S3_Critic", "GUEST_OLD", "Mon Dieu! Watch the... wait. I know that jawline. You're the financier from Zurich! "
     "Brilliant piece in Cahiers last month!", (0.25, 0.75, 0.75, 1.0), False),
    ("S3_PlayAlong", "JULIAN", "Keep your voice down, darling. I'm incognito.", (0.5, 0.8, 0.45, 0.95), False),
    ("S3_PlayAlongReply", "GUEST_OLD", "Ooh! My lips are sealed. I'll tell the door you're with me, monsieur!",
     (0.25, 0.75, 0.75, 1.0), False),
    ("S3_BrushOff", "JULIAN", "You have the wrong man.", (0.55, 0.8, 0.3, 0.95), False),
    ("S3_BrushOffReply", "GUEST_OLD", "Hmph! Philistine. Zurich was much better dressed.", (0.3, 0.75, 0.7, 1.0),
     False),
    ("S3_MaxVo", None, "Too many eyes on the velvet rope, Vance. You walk up there without a seat assignment, they'll "
     "tackle you into the shrimp cocktail. Nora, you in position?", (0.4, 0.8, 0.5, 1.12), True),
    ("S3_GuardTackle", "GUARD", "You're not on the list. Down you go!", (0.4, 0.8, 0.4, 1.0), False),
    ("S3_GuardAlibi", "GUARD", "Name? ...Ah, the gentleman from Zurich. Your seat isn't ready, sir. Wait by the bar.",
     (0.5, 0.8, 0.3, 1.0), False),
]


def _audio_tools():
    sys.path.insert(0, os.path.join(REPO, "tools", "audio"))
    import el_api
    import manifest
    import process
    return el_api, manifest, process


def generate(ids, force, dry):
    el_api, M, _ = _audio_tools()
    os.makedirs(RAW, exist_ok=True)
    todo = []
    for (sid, secs, prompt) in SFX:
        todo.append((sid, SFX_CREDITS, lambda p=prompt, d=max(secs, 0.5): el_api.sound_effect(p, d, 0.6, False)))
    for (lid, key, text, (st, sim, style, speed), _ear) in VO:
        vid = MAX_VOICE if key is None else M.VOICES[key]["voice_id"]
        s = {"stability": st, "similarity_boost": sim, "style": style, "use_speaker_boost": True, "speed": speed}
        todo.append((lid, len(text), lambda t=text, v=vid, s=s: el_api.tts(v, t, M.TTS_MODEL, s)))
    todo = [t for t in todo if (not ids or t[0] in ids)
            and (force or not os.path.exists(os.path.join(RAW, t[0] + ".mp3")))]
    print(f"to generate: {len(todo)} items, ~{sum(t[1] for t in todo)} credits")
    if dry:
        return
    for (iid, _est, call) in todo:
        try:
            data = call()
        except Exception as e:  # report and go on
            print(f"FAILED {iid}: {e}")
            continue
        with open(os.path.join(RAW, iid + ".mp3"), "wb") as f:
            f.write(data)
        print(f"ok {iid} ({len(data) // 1024} KB)")


def process_all(ids):
    import numpy as np
    _, M, P = _audio_tools()
    from audio_io import decode_mp3, write_wav
    ear = {v[0] for v in VO if v[4]}
    vo = {v[0] for v in VO}
    length = {s[0]: s[1] for s in SFX}
    for f in sorted(os.listdir(RAW)):
        iid = f[:-4]
        if not f.endswith(".mp3") or (ids and iid not in ids):
            continue
        x, rate = decode_mp3(os.path.join(RAW, f))
        x = P.mono(P.to_rate(x, rate))
        if iid in vo:
            x = P.fade(P.cap_pauses(P.trim(x, -45, 0.02, 0.15)), 0.005, 0.04)
            if iid in ear:
                x = P.earpiece(x)
            target = M.loudness_for("vo_")
        else:
            x = P.trim(x, -50, 0.004, 0.1)
            x = x[: int((length[iid] + 0.05) * P.RATE)]
            x = P.fade(x, 0.002, min(0.05, length[iid] / 4))
            target = M.loudness_for("sfx_")
        x = P.normalise(x, target)
        write_wav(os.path.join(OUT, iid + ".wav"), x)
        print(f"{iid:20s} {len(x) / P.RATE:5.2f} s  peak {P.db(np.max(np.abs(x))):5.1f} dBFS  "
              f"{P.loudness(x):5.1f} LUFS (target {target:.0f})")


def import_to_editor():
    tasks = []
    for f in sorted(os.listdir(OUT)):
        if not f.endswith(".wav"):
            continue
        t = unreal.AssetImportTask()
        t.filename = os.path.join(OUT, f)
        t.destination_path = DEST
        t.destination_name = f[:-4]
        t.replace_existing = True
        t.automated = True
        t.save = True
        tasks.append(t)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    done = [t.destination_name for t in tasks
            if unreal.load_asset(f"{DEST}/{t.destination_name}.{t.destination_name}")]
    print(f"S3_AUDIO imported {len(done)} of {len(tasks)} into {DEST}: {', '.join(done)}")
    return len(done)


if unreal is not None:
    import_to_editor()
elif __name__ == "__main__":
    argv = sys.argv[1:]
    sel = [a for a in argv if not a.startswith("--")]
    if "--process-only" not in argv:
        generate(sel, "--force" in argv, "--dry-run" in argv)
    if "--dry-run" not in argv:
        process_all(sel)
