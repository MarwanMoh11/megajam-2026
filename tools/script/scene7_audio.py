"""Scene 7 (the projection booth fight) sounds and voice lines: ElevenLabs -> C:\\Jam\\Audio\\script\\s7 -> /Game/Take/Audio/Script/S7.

Outside the editor (python with numpy + miniaudio, as for tools/audio):
  C:\\Jam\\venv-audio\\Scripts\\python.exe tools/script/scene7_audio.py [--dry-run] [--force] [id ...]
      generate the missing raw MP3s, then process all
  C:\\Jam\\venv-audio\\Scripts\\python.exe tools/script/scene7_audio.py --process-only [id ...]
Inside the editor (hold the editor lock):
  "$PY" -I tools/ue_mcp.py py tools/script/scene7_audio.py              import every S7 WAV, replacing and saving

Uses tools/audio's API wrapper and DSP (read-only): same TTS model, loudness targets (vo -16 LUFS, sound effects
-18 LUFS), -1 dBFS peak ceiling, 16-bit 44.1 kHz mono. Voices are in-scene (no earpiece band-pass).
Pendleton: the voice and settings the Scene 5 session chose (tools/script/scene5_audio.py).
Cole was Vex: Vex's voice (Charlie). Vance's fight grunts are sound effects, not TTS.
"""
import os
import sys

try:
    import unreal  # present only inside the editor
except ImportError:
    unreal = None

OUT = r"C:\Jam\Audio\script\s7"
RAW = os.path.join(OUT, "raw")
DEST = "/Game/Take/Audio/Script/S7"
REPO = r"C:\Jam\megajam"

PENDLETON_VOICE = "oDQy4IsGdMzP2p0Mj1UU"   # as chosen by the Scene 5 session (tools/script/scene5_audio.py)

# asset name, seconds, prompt, loop
SFX = [
    ("sfx_s7_projector_hum", 4.0, "A 1970s 35mm film projector running, close: steady mechanical rattle of the "
     "intermittent movement, film clatter through the gate, low electric motor hum, constant, seamless loop", True),
    ("sfx_s7_mag_open", 1.0, "A metal film magazine door on a 35mm projector unlatching and swinging open, latch "
     "click and a short metal creak, close, dry", False),
    ("sfx_s7_reel_swap", 1.5, "A metal film reel slid off a projector spindle, then another reel clicked onto the "
     "spindle, metallic scrape and a firm click, close, dry", False),
    ("sfx_s7_canister_latch", 0.7, "An aluminium film canister lid pressed shut and latching, one crisp metallic "
     "click, close, dry", False),
    ("sfx_s7_door_click", 1.5, "A heavy door latch clicking open and the old wooden door creaking open, short, "
     "close", False),
    ("sfx_s7_crowd_scream", 4.0, "Continuous sustained panic for four full seconds: a packed theatre audience screaming and scrambling for the exits, seats "
     "banging, heard muffled through the glass of a projection booth window", False),
    ("sfx_s7_punch", 0.8, "A heavy cinematic movie punch: fast whoosh and a meaty impact, 1970s film fight foley, "
     "punchy, dry", False),
    ("sfx_s7_head_slam", 1.2, "A head slammed into a hot metal projector housing: deep metal clang and thud with "
     "an electric spark crackle, cinematic fight", False),
    ("sfx_s7_clang", 1.2, "A stiletto knife blade striking an aluminium film canister: one sharp bright metallic "
     "clang with a ringing tail", False),
    ("sfx_s7_window_crash", 2.5, "A large plate glass window smashed as a man crashes through it, big glass "
     "shatter, then shards falling and tinkling, cinematic", False),
    ("sfx_s7_body_fall", 1.0, "A man landing heavily on a pile of thick velvet curtains: a soft deep muffled "
     "thump, fabric rustle, no glass", False),
    ("sfx_s7_door_kick", 1.2, "A wooden door kicked open hard and banging against the wall, loud, close", False),
    ("sfx_s7_smoke", 3.5, "A smoke grenade pin pulled with a metal ping, a metal can bouncing and clattering down "
     "concrete steps, then a loud continuous smoke hiss", False),
    ("sfx_s7_fire_whoosh", 1.8, "Celluloid film catching fire: a quick whoosh of flame igniting then crackling "
     "burning film", False),
    # Vance's effort grunts: sound effects, so they stay wordless (TTS reads interjections as words)
    ("vo_s7_vance_grunt_1", 0.6, "A suave man's short sharp effort grunt as he throws a punch in a fist fight, "
     "one breathy 'hngh', close, dry, no other sound", False),
    ("vo_s7_vance_grunt_2", 0.6, "A man's short pained grunt as he takes a punch to the stomach in a fist fight, "
     "one 'ugh', close, dry, no other sound", False),
]

# asset name, character, voice key in tools/audio/manifest.VOICES (or a raw voice id), text, (stability, similarity, style)
VO = [
    ("vo_s7_pendleton_lock", "PENDLETON", PENDLETON_VOICE, "Lock the building. Nobody leaves.", (0.6, 0.8, 0.3)),
    ("vo_s7_cole_move", "COLE", "VEX", "Max is out back! Move!", (0.3, 0.8, 0.6)),
]
SFX_CREDITS = 200


def _audio_tools():
    sys.path.insert(0, os.path.join(REPO, "tools", "audio"))
    import el_api
    import manifest
    import process
    return el_api, manifest, process


def voice_for(manifest, key):
    return manifest.VOICES[key]["voice_id"] if key in manifest.VOICES else key


def generate(ids, force, dry):
    el_api, M, _ = _audio_tools()
    os.makedirs(RAW, exist_ok=True)
    todo = []
    for (sid, secs, prompt, loop) in SFX:
        todo.append((sid, SFX_CREDITS, lambda p=prompt, d=secs, l=loop: el_api.sound_effect(p, d, 0.6, l)))
    for (lid, _who, key, text, (st, sim, style)) in VO:
        vid = voice_for(M, key)
        s = {"stability": st, "similarity_boost": sim, "style": style, "use_speaker_boost": True}
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
        print(f"{iid:24s} {len(x) / P.RATE:5.2f} s  peak {P.db(np.max(np.abs(x))):5.1f} dBFS  "
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
            print(f"S7_AUDIO missing after import: {n}")
            continue
        if n in loops:
            a.set_editor_property("looping", True)
            unreal.EditorAssetLibrary.save_loaded_asset(a, only_if_is_dirty=False)
        done.append(f"{n} {a.get_editor_property('duration'):.2f}s{' loop' if a.get_editor_property('looping') else ''}")
    print(f"S7_AUDIO imported {len(done)} of {len(tasks)} into {DEST}: {', '.join(done)}")


if unreal is not None:
    import_to_editor()
elif __name__ == "__main__":
    argv = sys.argv[1:]
    sel = [a for a in argv if not a.startswith("--")]
    if "--process-only" not in argv:
        generate(sel, "--force" in argv, "--dry-run" in argv)
    if "--dry-run" not in argv:
        process_all(sel)
