"""The main hall's ambience, one-shots and action sounds, plus the two score loops and two root cues:
ElevenLabs -> C:\\Jam\\Audio\\Script\\Hall (and \\Hall\\root, \\Score) -> /Game/Take/Audio/Hall (/Game/Take/Audio, .../Score).

Outside the editor (the audio venv: C:\\Jam\\venv-audio\\Scripts\\python.exe):
  python tools/audio/hall_audio.py [--dry-run] [--force] [id ...]   generate the missing raw MP3s, then process all
  python tools/audio/hall_audio.py --process-only [id ...]          re-process the raw MP3s into WAVs
Inside the editor (hold the editor lock):
  "$PY" -I tools/ue_mcp.py py tools/audio/hall_audio.py             import every WAV of all three folders, replacing and
                                                                    saving; amb_* and mus_* get looping = True

Same shape as tools/script/scene2_audio.py. Loudness: loops amb -18, music -20, one-shots -20, action -18, root cues -18
(LUFS); -1 dBFS peak ceiling; 16-bit 44.1 kHz mono. Raw MP3s of every group sit in Hall\\raw, except Score's in Score\\raw.
"""
import os
import sys

try:
    import unreal  # present only inside the editor
except ImportError:
    unreal = None

BASE = r"C:\Jam\Audio\Script"
REPO = r"C:\Jam\megajam"
# group -> (wav folder, raw folder, /Game folder)
GROUPS = {
    "hall": (os.path.join(BASE, "Hall"), os.path.join(BASE, "Hall", "raw"), "/Game/Take/Audio/Hall"),
    "root": (os.path.join(BASE, "Hall", "root"), os.path.join(BASE, "Hall", "raw"), "/Game/Take/Audio"),
    "score": (os.path.join(BASE, "Score"), os.path.join(BASE, "Score", "raw"), "/Game/Take/Audio/Score"),
}

HALL_ROOM = "in a grand marble theatre hall, a hint of natural reverb"
# id, group, kind (loop | music | shot), seconds (the WAV is cut to this; loops and music: the asked length), LUFS, prompt
ITEMS = [
    ("amb_hall_murmur", "hall", "loop", 20.0, -18,
     "The murmur of about a hundred elegant guests chatting at a formal gala in a huge reverberant marble hall: a steady "
     "wash of low conversation and polite laughter far and near, no clear words, no music, no single voice standing "
     "out, constant level, seamless loop"),
    ("amb_hall_room", "hall", "loop", 15.0, -18,
     "Room tone recorded inside a huge marble theatre hall: a continuous soft whoosh of air, a low steady HVAC "
     "rumble and hum, a faint far-away echoing clatter now and then, no voices, no music, constant level, seamless loop"),
    ("mus_hall_quartet", "hall", "music", 90.0, -20,
     "A string quartet (two violins, viola, cello) playing an elegant, light, slightly playful baroque-style piece for "
     "a black-tie gala reception, 1940s grand theatre, recorded in a live room, lively but polite, steady tempo, no other "
     "instruments, no percussion, no vocals"),
    ("mus_sneak", "score", "music", 75.0, -20,
     "A 1960s heist-film stealth cue: brushed drums, a walking upright bass line, muted trumpet stabs, vibraphone, "
     "cool and tense, G minor, about 120 BPM, instrumental, tape-era warmth, steady groove that loops"),
    ("mus_chase", "score", "music", 75.0, -20,
     "A 1960s heist-film chase cue: driving drums, urgent brass, bongos, surf guitar, big and fast, G minor, about "
     "150 BPM, instrumental, relentless groove that loops"),
]
for _i, _p in enumerate([
        "Two champagne flutes clinking together once in a toast, crystal ping, dry and close with a hint of hall air",
        "Three champagne flutes clinking together in a toast, a bright cluster of crystal pings and a little ring, "
        "hint of hall reverb",
        "A single quick clink of two crystal champagne glasses, a short high ping, dry, hint of marble hall",
        "Two champagne flutes clinking twice in quick succession, light crystal pings, dry, hint of hall"], start=1):
    ITEMS.append((f"sfx_hall_clink_{_i}", "hall", "shot", 1.6, -20, _p))
for _i, _p in enumerate([
        "One elegant older woman laughing politely at a party a few metres away, a short light chuckle, no words, "
        "slight hall reverb, other guests faintly murmuring far behind",
        "A man giving a short polite gentlemanly laugh at a cocktail party a few metres away, no words, dry-ish with a "
        "hint of marble hall",
        "Two guests laughing politely together at a party a few metres away, a brief tinkle of amused laughter, no words, "
        "hint of hall reverb"], start=1):
    ITEMS.append((f"sfx_hall_laugh_{_i}", "hall", "shot", 2.5, -20, _p))
for _i, _p in enumerate([
        "A guest's leather dress shoes walking five steps across a polished marble floor, passing by, clicking "
        "footsteps, hint of hall reverb",
        "A woman's high heels walking five steps across a marble floor, passing from left to right, crisp clicks, "
        "hint of hall reverb",
        "A man in leather shoes strolling four slow steps across marble, passing by, hint of hall reverb"], start=1):
    ITEMS.append((f"sfx_hall_steps_{_i}", "hall", "shot", 3.0, -20, _p))
for _i, _p in enumerate([
        "A waiter's silver tray with champagne glasses rattling and chinking as it is carried, then set down on a "
        "table with a soft clunk, dry, hint of hall",
        "A silver serving tray of crystal glasses rattling gently, set down on a marble-topped table with a clink, "
        "close, hint of hall"], start=1):
    ITEMS.append((f"sfx_hall_tray_{_i}", "hall", "shot", 2.5, -20, _p))
ITEMS += [
    ("sfx_hall_stumble", "hall", "shot", 2.5, -18,
     "A man tumbling down three or four carpeted marble stairs: a quick sequence of dull body thumps and a final "
     "heavy landing on the floor, no grunts, no voice, dry with a hint of hall"),
    ("sfx_hall_tackle", "hall", "shot", 2.5, -18,
     "A heavy body slammed onto a marble floor with a hard thud, and a tray of glasses crashing and shattering "
     "with it, no voice, hint of hall echo"),
    ("sfx_hall_rope", "hall", "shot", 2.0, -18,
     "A brass velvet-rope stanchion knocked: a metallic clink and a wobbling ring as it rocks on a marble floor, "
     "dry, hint of hall"),
    ("sfx_hall_clamp", "hall", "shot", 3.0, -18,
     "A metal stage-light clamp being loosened: a ratchet squeak and a metallic creak as a heavy lamp swings on "
     "its yoke, close, dry, hint of hall"),
    ("sfx_hall_gasp", "hall", "shot", 1.5, -18,
     "A crowd of elegant guests gasping together in shock, a short sharp collective intake of breath, no words, "
     "hint of hall reverb"),
    ("sfx_spot_on", "root", "shot", 1.2, -18,
     "A big theatre follow-spot slamming on: a heavy carbon-arc lamp thunk and a rising electric hum, close"),
    ("sfx_door_close", "root", "shot", 1.5, -18,
     "A heavy oak door swinging shut with a solid wooden thud and a metal latch click, close, dry"),
]
LOOP_XFADE = {"loop": 0.5, "music": 1.5}   # seconds: tail mixed into head
SFX_CREDITS = 200
MUSIC_CREDITS = 1500   # estimate for the dry-run total only


def _audio_tools():
    sys.path.insert(0, os.path.join(REPO, "tools", "audio"))
    import el_api
    import manifest
    import process
    return el_api, manifest, process


def _raw(group):
    return GROUPS[group][1]


def generate(ids, force, dry):
    el_api, _, _ = _audio_tools()
    todo = []
    for (iid, group, kind, secs, _lufs, prompt) in ITEMS:
        if kind == "music":
            call = lambda p=prompt, s=secs: el_api.music(p, s * 1000)
            est = MUSIC_CREDITS
        else:
            call = lambda p=prompt, d=max(secs, 0.5), l=(kind == "loop"): el_api.sound_effect(p, d, 0.6, l)
            est = SFX_CREDITS
        if (not ids or iid in ids) and (force or not os.path.exists(os.path.join(_raw(group), iid + ".mp3"))):
            todo.append((iid, group, est, call))
    print(f"to generate: {len(todo)} items, ~{sum(t[2] for t in todo)} credits")
    if dry:
        return
    for (iid, group, _est, call) in todo:
        os.makedirs(_raw(group), exist_ok=True)
        try:
            data = call()
        except Exception as e:  # report and go on
            print(f"FAILED {iid}: {e}")
            continue
        with open(os.path.join(_raw(group), iid + ".mp3"), "wb") as f:
            f.write(data)
        print(f"ok {iid} ({len(data) // 1024} KB)")


def _loop(x, P, secs):
    """Seamless loop: the last `secs` fade out under the first `secs` fading in (equal power); the result is shorter."""
    import numpy as np
    k = int(secs * P.RATE)
    w = np.linspace(0, 1, k)
    return np.concatenate([x[:k] * np.sqrt(w) + x[-k:] * np.sqrt(1 - w), x[k:-k]])


def process_all(ids):
    import numpy as np
    _, _, P = _audio_tools()
    from audio_io import decode_mp3, write_wav
    for (iid, group, kind, secs, target, _prompt) in ITEMS:
        if ids and iid not in ids:
            continue
        src = os.path.join(_raw(group), iid + ".mp3")
        if not os.path.exists(src):
            continue
        x, rate = decode_mp3(src)
        x = P.mono(P.to_rate(x, rate))
        if kind in LOOP_XFADE:
            x = _loop(x, P, LOOP_XFADE[kind])
        else:
            x = P.trim(x, -48, 0.01, 0.4)        # the first sound lands within 10 ms of the start
            x = x[: int((secs + 0.05) * P.RATE)]
            x = P.fade(x, 0.002, min(0.08, secs / 4))
        if not np.isfinite(P.loudness(x)):    # the loudness gate sees nothing (a very quiet bed): gain by RMS instead
            x = x * 10 ** ((-34.0 - P.db(np.sqrt(np.mean(x ** 2)))) / 20)
            target = P.loudness(x) if np.isfinite(P.loudness(x)) else target
        x = P.normalise(x, float(target))
        out = GROUPS[group][0]
        os.makedirs(out, exist_ok=True)
        write_wav(os.path.join(out, iid + ".wav"), x)
        print(f"{iid:20s} {len(x) / P.RATE:6.2f} s  peak {P.db(np.max(np.abs(x))):5.1f} dBFS  "
              f"{P.loudness(x):5.1f} LUFS (target {target})")


def import_to_editor():
    loops = {i[0] for i in ITEMS if i[2] in LOOP_XFADE}
    tasks = []
    for (folder, _raw_dir, dest) in GROUPS.values():
        if not os.path.isdir(folder):
            continue
        for f in sorted(os.listdir(folder)):
            if not f.endswith(".wav"):
                continue
            t = unreal.AssetImportTask()
            t.filename = os.path.join(folder, f)
            t.destination_path = dest
            t.destination_name = f[:-4]
            t.replace_existing = True
            t.automated = True
            t.save = True
            tasks.append(t)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    done = []
    for t in tasks:
        n, dest = t.destination_name, t.destination_path
        a = unreal.load_asset(f"{dest}/{n}.{n}")
        if not a:
            print(f"HALL_AUDIO missing after import: {dest}/{n}")
            continue
        if n in loops:
            a.set_editor_property("looping", True)
            unreal.EditorAssetLibrary.save_loaded_asset(a, only_if_is_dirty=False)
        done.append(n)
    print(f"HALL_AUDIO imported {len(done)} of {len(tasks)}: {', '.join(done)}")


if unreal is not None:
    import_to_editor()
elif __name__ == "__main__":
    argv = sys.argv[1:]
    sel = [a for a in argv if not a.startswith("--")]
    if "--process-only" not in argv:
        generate(sel, "--force" in argv, "--dry-run" in argv)
    if "--dry-run" not in argv:
        process_all(sel)
