"""Scene 1 (the marquee) sounds and voice lines -> C:\\Jam\\Audio\\Script\\S1 -> /Game/Take/Audio/Script/S1.

Outside the editor (C:\\Jam\\venv-audio, numpy + miniaudio, as for tools/audio):
  python tools/script/scene1_audio.py [--force] [id ...]      generate the missing raw files, then process all
  python tools/script/scene1_audio.py --process-only [id ...]
Inside the editor (hold the editor lock):
  "$PY" -I tools/ue_mcp.py py tools/script/scene1_audio.py     import every S1 WAV, replacing and saving

Voices from ElevenLabs through tools/audio (read-only): the bouncer reuses the designed DOORMAN voice, the floor
manager the designed BROADCAST voice (harried, clipped), whispering through eleven_v3's audio tag. The beep and the
laser are synthesised here; the rain is an ElevenLabs looping sound effect (synthesised if that fails).
Loudness as tools/audio: vo -16 LUFS, sfx -18 LUFS, the rain bed -28 LUFS (it sits under dialogue); 16-bit 44.1 kHz mono.
"""
import importlib
import os
import sys

# the editor's Python tool allows only top-level import statements, so the rest go through importlib
try:
    unreal = importlib.import_module("unreal")  # present only inside the editor
except ImportError:
    unreal = None

OUT = r"C:\Jam\Audio\Script\S1"
RAW = os.path.join(OUT, "raw")
DEST = "/Game/Take/Audio/Script/S1"
REPO = r"C:\Jam\megajam"
LOOPS = {"amb_s1_rain"}

# id, voice key in tools/audio/manifest.VOICES, text, model (None = manifest TTS_MODEL), (stability, similarity, style)
VO = [
    ("vo_s1_manager_whisper", "BROADCAST",
     "[whispers urgently] Word from Mr. Pendleton. The cards are out. Palms only, tonight.", "eleven_v3",
     (0.5, 0.8, 0.3)),
    ("vo_s1_bouncer_directives", "DOORMAN",
     "Directives from Mr. Pendleton. Magnetic cards are void. Palm verification only.", None, (0.6, 0.8, 0.15)),
    ("vo_s1_bouncer_hand", "DOORMAN", "Hand on the glass.", None, (0.6, 0.8, 0.15)),
    ("vo_s1_bouncer_enjoy", "DOORMAN", "Enjoy the screening, Your Grace.", None, (0.55, 0.8, 0.25)),
]
RAIN_PROMPT = ("Steady gentle night rain falling on wet cobblestones and pattering on a canvas awning above a theatre "
               "entrance, soft drips, no thunder, no traffic, constant, seamless loop")


def _tools():
    sys.path.insert(0, os.path.join(REPO, "tools", "audio"))
    return tuple(importlib.import_module(m) for m in ("el_api", "manifest", "process"))


def generate(ids, force):
    el_api, M, _ = _tools()
    os.makedirs(RAW, exist_ok=True)
    todo = []
    for (lid, key, text, model, (st, sim, style)) in VO:
        s = {"stability": st, "similarity_boost": sim, "style": style, "use_speaker_boost": True}
        vid = M.VOICES[key]["voice_id"]
        todo.append((lid, lambda t=text, v=vid, s=s, m=model: el_api.tts(v, t, m or M.TTS_MODEL, s),
                     # fallback for the v3 whisper: the same voice on the manifest model, quiet and breathy
                     (lambda t=text.split("] ", 1)[-1], v=vid: el_api.tts(
                         v, t, M.TTS_MODEL, {"stability": 0.3, "similarity_boost": 0.8, "style": 0.5,
                                             "use_speaker_boost": True})) if model else None))
    todo.append(("amb_s1_rain", lambda: el_api.sound_effect(RAIN_PROMPT, 22, 0.6, True), None))
    for (iid, call, fallback) in todo:
        if (ids and iid not in ids) or (not force and os.path.exists(os.path.join(RAW, iid + ".mp3"))):
            continue
        try:
            data = call()
        except Exception as e:
            print(f"FAILED {iid}: {e}")
            if not fallback:
                continue
            try:
                data = fallback()
            except Exception as e2:
                print(f"FAILED fallback {iid}: {e2}")
                continue
        with open(os.path.join(RAW, iid + ".mp3"), "wb") as f:
            f.write(data)
        print(f"ok {iid} ({len(data) // 1024} KB)")


def synth_beep(np, R):
    """Accept beep: two clean tones (G6 then C7), a little odd harmonic for the 70s hardware edge, 0.25 s."""
    out = np.zeros(int(0.25 * R))
    for (f, t0, dur) in ((1568.0, 0.0, 0.10), (2093.0, 0.115, 0.135)):
        n = int(dur * R)
        t = np.arange(n) / R
        sig = np.sin(2 * np.pi * f * t) + 0.12 * np.sin(2 * np.pi * 3 * f * t) + 0.05 * np.sin(2 * np.pi * 2 * f * t)
        env = np.minimum(1, t / 0.002) * np.exp(-t / (dur * 0.9))
        env[-int(0.006 * R):] *= np.linspace(1, 0, int(0.006 * R))
        a = int(t0 * R)
        out[a:a + n] += sig * env
    return out


def synth_laser(np, R):
    """Red laser sweep: a mains hum under a buzzy tone that rises, overshoots and settles, 1.0 s."""
    n = int(1.0 * R)
    t = np.arange(n) / R
    rise = np.clip(t / 0.45, 0, 1) ** 1.5
    settle = np.exp(-np.clip(t - 0.45, 0, None) / 0.12) * np.cos(2 * np.pi * 5 * np.clip(t - 0.45, 0, None))
    f = 260 + 640 * rise - 140 * (1 - settle) * (t > 0.45)
    ph = 2 * np.pi * np.cumsum(f) / R
    tone = sum((0.8 ** k) * np.sin(k * ph) for k in range(1, 7))  # bright, saw-like
    tone *= 1 + 0.25 * np.sin(2 * np.pi * 31 * t)                 # electrical flutter
    hum = sum((0.6 ** k) * np.sin(2 * np.pi * 60 * k * t) for k in range(1, 6))
    rng = np.random.default_rng(1)
    hiss = np.convolve(rng.standard_normal(n), np.ones(6) / 6, "same") * 0.06
    env = np.minimum(1, t / 0.06) * np.where(t > 0.82, np.maximum(0, (1.0 - t) / 0.18), 1)
    return (0.55 * tone + 0.5 * hum + hiss) * env


def synth_rain(np, R, secs=22.0):
    """Fallback rain: soft low-passed noise plus random droplet ticks."""
    rng = np.random.default_rng(7)
    n = int(secs * R)
    x = rng.standard_normal(n)
    X = np.fft.rfft(x)
    fr = np.fft.rfftfreq(n, 1 / R)
    X *= 1 / np.sqrt(1 + (fr / 1800) ** 2) * (fr > 80)
    x = np.fft.irfft(X, n) * 0.5
    k = int(0.012 * R)
    tick = rng.standard_normal(k) * np.exp(-np.arange(k) / (0.002 * R))
    for at in rng.integers(0, n - k, int(secs * 40)):
        x[at:at + k] += tick * rng.uniform(0.1, 0.6)
    return x


def process_all(ids):
    np = importlib.import_module("numpy")
    _, M, P = _tools()
    aio = importlib.import_module("audio_io")
    decode_mp3, write_wav = aio.decode_mp3, aio.write_wav
    R = P.RATE
    items = {}
    if os.path.isdir(RAW):
        for f in os.listdir(RAW):
            if f.endswith(".mp3"):
                x, rate = decode_mp3(os.path.join(RAW, f))
                items[f[:-4]] = P.mono(P.to_rate(x, rate))
    items["sfx_s1_beep"] = synth_beep(np, R)
    items["sfx_s1_laser"] = synth_laser(np, R)
    if "amb_s1_rain" not in items:
        print("amb_s1_rain: no ElevenLabs file, synthesising (stand-in)")
        items["amb_s1_rain"] = synth_rain(np, R)
    for iid, x in sorted(items.items()):
        if ids and iid not in ids:
            continue
        if iid.startswith("vo_"):
            x = P.fade(P.cap_pauses(P.trim(x, -50 if "whisper" in iid else -45, 0.02, 0.15)), 0.005, 0.04)
            target = M.loudness_for("vo_")
        elif iid in LOOPS:
            k = int(0.5 * R)
            w = np.linspace(0, 1, k)
            x = np.concatenate([x[:k] * np.sqrt(w) + x[-k:] * np.sqrt(1 - w), x[k:-k]])
            x = x / np.max(np.abs(x))
            x = P.limit(x, 10 ** (-14 / 20), max_cut_db=14.0, release=0.08)  # tame the close awning drips
            target = -28.0
        else:
            x = P.fade(x, 0.001, 0.005)
            target = M.loudness_for("sfx_")
        x = P.normalise(x, target)
        write_wav(os.path.join(OUT, iid + ".wav"), x)
        print(f"{iid:26s} {len(x) / R:5.2f} s  peak {P.db(np.max(np.abs(x))):5.1f} dBFS  "
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
    done = []
    for t in tasks:
        n = t.destination_name
        a = unreal.load_asset(f"{DEST}/{n}.{n}")
        if not a:
            print(f"S1_AUDIO missing after import: {n}")
            continue
        if n in LOOPS:
            a.set_editor_property("looping", True)
            unreal.EditorAssetLibrary.save_loaded_asset(a, only_if_is_dirty=False)
        done.append(f"{n} {a.get_editor_property('duration'):.2f}s")
    print(f"S1_AUDIO imported {len(done)} of {len(tasks)} into {DEST}: {', '.join(done)}")


if unreal is not None:
    import_to_editor()
elif __name__ == "__main__":
    argv = sys.argv[1:]
    sel = [a for a in argv if not a.startswith("--")]
    if "--process-only" not in argv:
        generate(sel, "--force" in argv)
    process_all(sel)
