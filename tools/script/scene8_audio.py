"""Scene 8 (the alley) sounds and voice lines: ElevenLabs -> C:\\Jam\\Audio\\Script\\S8 -> /Game/Take/Audio/Script/S8.

Outside the editor (python with numpy + miniaudio, as for tools/audio):
  python tools/script/scene8_audio.py [--dry-run] [--force] [id ...]   generate the missing raw MP3s, then process all
  python tools/script/scene8_audio.py --process-only [id ...]          re-process the raw MP3s into WAVs
Inside the editor (hold the editor lock):
  "$PY" -I tools/ue_mcp.py py tools/script/scene8_audio.py              import every S8 WAV, replacing and saving

Uses tools/audio's API wrapper and DSP (read-only): same TTS model, voices, loudness targets (vo -16 LUFS, sound
effects -18 LUFS), -1 dBFS peak ceiling, 16-bit 44.1 kHz mono. Voices are in-scene (no earpiece band-pass).
Max (vo_s8_max_getin) uses the voice session s2-van designs; until a voice id for MAX is found it is skipped.
"""
import importlib
import os
import re
import sys

# Inside the editor `unreal` is already loaded (the editor's runner allows only top-level import statements).
unreal = sys.modules.get("unreal")

OUT = r"C:\Jam\Audio\Script\S8"
RAW = os.path.join(OUT, "raw")
DEST = "/Game/Take/Audio/Script/S8"
REPO = r"C:\Jam\megajam"

# id, seconds, prompt, loop
SFX = [
    ("s8_door_kick", 1.4, "A heavy steel fire exit door kicked open with a loud metallic bang, crash bar clatter, the "
     "door slamming against a brick wall outside, 1960s heist film, punchy", False),
    ("s8_gunshot_1", 1.2, "A single pistol gunshot in a narrow brick alley at night, sharp crack with a short slapback "
     "echo off the walls, 1960s crime film, punchy", False),
    ("s8_gunshot_2", 1.2, "One revolver shot fired from a doorway into a wet brick alley, loud bang, brief alley "
     "reverb tail, cinematic", False),
    ("s8_gunshot_3", 1.2, "A single snappy semi-automatic pistol shot outdoors between brick buildings, crack and a "
     "quick echo, 1960s heist movie", False),
    ("s8_ricochet_1", 1.0, "A bullet ricochet off a brick wall: sharp impact crack and a high whining zing flying "
     "away, classic film ricochet", False),
    ("s8_ricochet_2", 1.0, "Bullet hitting brick and sparking, chips of masonry, then a short descending ricochet "
     "whine, cinematic", False),
    ("s8_ricochet_3", 1.0, "A quick bullet ricochet off stone, a ping and a fast whistling whine, 1960s western style "
     "ricochet", False),
    ("s8_car_arrive", 3.0, "A 1960s V8 sports coupe approaching fast down a wet street at night, engine roaring "
     "louder, then a long tyre screech as it drifts round a corner and skids to a stop, cinematic", False),
    ("s8_car_away", 4.0, "A 1960s V8 muscle car revving hard, clutch dumped, rear wheels spinning and squealing on "
     "wet tarmac, then roaring away into the distance, cinematic car chase", False),
    ("s8_car_door_slam", 0.8, "A heavy 1960s car door slammed shut, solid metal thunk with a little rattle, close",
     False),
    ("s8_car_interior", 4.0, "Inside a 1960s V8 sports car driving fast at night: steady deep engine rumble, road "
     "noise and light rain on the roof, constant, no gear changes, seamless loop", True),
]

# id, character, voice key in tools/audio/manifest.VOICES (None = look it up), text, (stability, similarity, style)
VO = [
    ("vo_s8_vance_print", "VANCE", "JULIAN", "Cut. ... Print it.", (0.55, 0.8, 0.4)),
    ("vo_s8_cole_go", "COLE", "VEX", "Go, go, go!", (0.3, 0.8, 0.6)),
    ("vo_s8_guard_stop", "GUARD", "SECURITY", "Stop them!", (0.3, 0.8, 0.6)),
    ("vo_s8_max_getin", "MAX", None, "Get in! Get in!", (0.3, 0.8, 0.6)),
]
SFX_CREDITS = 200


def _audio_tools():
    sys.path.insert(0, os.path.join(REPO, "tools", "audio"))
    return tuple(importlib.import_module(m) for m in ("el_api", "manifest", "process"))


def find_max_voice():
    """Max's voice id, designed by session s2-van: an env var, or a voice id named next to MAX in its files."""
    vid = os.environ.get("MAX_VOICE_ID", "").strip()
    if vid:
        return vid
    cands = [os.path.join(REPO, "docs", "script", "s2.md")]
    sd = os.path.join(REPO, "tools", "script")
    cands += [os.path.join(sd, f) for f in sorted(os.listdir(sd)) if f.startswith("scene2") and f.endswith(".py")]
    vd = r"C:\Jam\Audio\voice_design\MAX.json"
    cands.append(vd)
    for path in cands:
        if not os.path.exists(path):
            continue
        for line in open(path, encoding="utf-8", errors="replace"):
            if re.search(r"\bMAX\b", line, re.I):
                m = re.search(r"voice_id\W{1,4}([A-Za-z0-9]{20})\b", line)
                if m:
                    return m.group(1)
    return None


def voice_for(manifest, key, name):
    if key is None:
        return find_max_voice()
    return manifest.VOICES[key]["voice_id"]


def generate(ids, force, dry):
    el_api, M, _ = _audio_tools()
    os.makedirs(RAW, exist_ok=True)
    todo = []
    for (sid, secs, prompt, loop) in SFX:
        todo.append((sid, SFX_CREDITS, lambda p=prompt, d=secs, l=loop: el_api.sound_effect(p, d, 0.6, l)))
    for (lid, who, key, text, (st, sim, style)) in VO:
        vid = voice_for(M, key, who)
        if not vid:
            print(f"skip {lid}: no voice id for {who} yet (session s2-van designs it)")
            continue
        s = {"stability": st, "similarity_boost": sim, "style": style, "use_speaker_boost": True}
        if key is None:
            s["speed"] = 1.1   # Max talks faster than the doorman
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
    np = importlib.import_module("numpy")
    _, M, P = _audio_tools()
    aio = importlib.import_module("audio_io")
    decode_mp3, write_wav = aio.decode_mp3, aio.write_wav
    loops = {s[0] for s in SFX if s[3]}
    for f in sorted(os.listdir(RAW)):
        iid = f[:-4]
        if not f.endswith(".mp3") or (ids and iid not in ids):
            continue
        x, rate = decode_mp3(os.path.join(RAW, f))
        x = P.mono(P.to_rate(x, rate))
        if iid.startswith("vo_"):
            x = P.fade(P.cap_pauses(P.trim(x, -45, 0.02, 0.15)), 0.005, 0.04)
            target = M.loudness_for("vo_")
        elif iid in loops:
            k = int(0.08 * P.RATE)
            w = np.linspace(0, 1, k)
            x = np.concatenate([x[:k] * np.sqrt(w) + x[-k:] * np.sqrt(1 - w), x[k:-k]])
            target = M.loudness_for("sfx_")
        else:
            x = P.fade(P.trim(x, -50, 0.004, 0.1), 0.002, 0.05)
            target = M.loudness_for("sfx_")
        x = P.normalise(x, target)
        write_wav(os.path.join(OUT, iid + ".wav"), x)
        print(f"{iid:22s} {len(x) / P.RATE:5.2f} s  peak {P.db(np.max(np.abs(x))):5.1f} dBFS  "
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
    loops = {s[0] for s in SFX if s[3]}
    done = []
    for t in tasks:
        n = t.destination_name
        a = unreal.load_asset(f"{DEST}/{n}.{n}")
        if not a:
            print(f"S8_AUDIO missing after import: {n}")
            continue
        if n in loops:
            a.set_editor_property("looping", True)
        # The task's save flag did not write the files in 5.9 (only the explicitly saved loop landed): save each.
        unreal.EditorAssetLibrary.save_loaded_asset(a, only_if_is_dirty=False)
        done.append(n)
    print(f"S8_AUDIO imported {len(done)} of {len(tasks)} into {DEST}: {', '.join(done)}")


if unreal is not None:
    import_to_editor()
elif __name__ == "__main__":
    argv = sys.argv[1:]
    sel = [a for a in argv if not a.startswith("--")]
    if "--process-only" not in argv:
        generate(sel, "--force" in argv, "--dry-run" in argv)
    if "--dry-run" not in argv:
        process_all(sel)
