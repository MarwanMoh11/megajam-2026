"""Scene 6 (backstage) sounds and voice lines: ElevenLabs -> C:\\Jam\\Audio\\Script\\S6 -> /Game/Take/Audio/Script/S6.

Outside the editor (python with numpy + miniaudio, as for tools/audio):
  python tools/script/scene6_audio.py [--dry-run] [--force] [id ...]   generate the missing raw MP3s, then process all
  python tools/script/scene6_audio.py --process-only [id ...]          re-process the raw MP3s into WAVs
Inside the editor (hold the editor lock):
  "$PY" -I tools/ue_mcp.py py tools/script/scene6_audio.py              import every S6 WAV, replacing and saving

Uses tools/audio's API wrapper and DSP (read-only): same TTS model, voices, loudness targets (vo -16 LUFS, sound
effects -18 LUFS), -1 dBFS peak ceiling, 16-bit 44.1 kHz mono. Voices are in-scene (no earpiece band-pass).
Made from tools/script/scene8_audio.py. Owner: s6-backstage. Cole is Vex's voice; the enforcers use SECURITY.
"""
import os
import re
import sys

try:
    import unreal  # present only inside the editor
except ImportError:
    unreal = None

OUT = r"C:\Jam\Audio\Script\S6"
RAW = os.path.join(OUT, "raw")
DEST = "/Game/Take/Audio/Script/S6"
REPO = r"C:\Jam\megajam"

# id, seconds, prompt, loop
SFX = [
    ("sfx_s6_gunfight", 2.6, "A 1940s gangster film gunfight heard from behind a cinema screen: a burst of tommy gun "
     "fire and several pistol shots with ricochets, loud but muffled through the screen fabric, big hall reverb", False),
    ("sfx_s6_explosion", 3.0, "An old movie explosion heard from backstage behind a cinema screen: a huge deep boom, "
     "rumble and falling debris, muffled through the screen, the theatre shaking, cinematic", False),
    ("sfx_s6_choke", 1.2, "A quick choke-hold takedown: a man's strangled gasp, a scuffle of shoes on floorboards, "
     "a heavy body slumping, close and quiet", False),
    ("sfx_s6_lever", 1.0, "A heavy iron counterweight lever on a theatre fly rail thrown hard: metal clank, a rope "
     "running fast through a pulley", False),
    ("sfx_s6_thunder_sheet", 2.5, "A sandbag dropping onto a hanging theatrical thunder sheet: a huge rolling metallic "
     "crash and wobbling thunder rumble ringing out backstage", False),
    ("sfx_s6_maglock", 1.2, "A magnetic door lock releasing: an electric buzz, a heavy clunk of the bolt, a steel "
     "door unlatching", False),
    ("sfx_s6_trunk", 1.0, "A wooden costume trunk lid slammed shut on a heavy load, latches clicking", False),
    # QA Nov 15 evening: the drag, the door-control box, the fire exit
    ("sfx_s6_drag", 3.0, "A heavy unconscious man dragged by the shoulders across dusty wooden stage floorboards: cloth "
     "scraping, shoe heels dragging, a man's effortful breathing, close", False),
    ("sfx_s6_spark", 1.6, "Electrical wires bridged in an open security junction box: sharp crackling sparks, a short "
     "buzz, a pop, close and dry", False),
    ("sfx_s6_alarm_off", 1.0, "A fire door alarm box disarmed with a key: a click, a short descending electronic beep, "
     "then silence", False),
    ("sfx_s6_wedge", 1.4, "A heavy steel fire exit door pushed open a crack with a clunk of its push bar, then a "
     "wooden wedge kicked hard under it, the door creaking to a stop", False),
]

# id, character, voice key in tools/audio/manifest.VOICES, text, (stability, similarity, style)
VO = [
    ("vo_s6_cole_open", "COLE", "VEX", "Two heavies blocking the stairs. Going dark.", (0.45, 0.8, 0.35)),
    ("vo_s6_cole_onedown", "COLE", "VEX", "One down.", (0.45, 0.8, 0.35)),
    ("vo_s6_cole_door", "COLE", "VEX", "Door's open. Make it quick.", (0.45, 0.8, 0.35)),
    ("vo_s6_enforcer_what", "ENFORCER", "SECURITY", "What the hell was that?", (0.35, 0.8, 0.5)),
    ("vo_s6_enforcer_hey", "ENFORCER", "SECURITY", "Hey! You!", (0.3, 0.8, 0.6)),
    # QA Nov 15 evening: one guard, the door rig, the exit
    ("vo_s6_cole_one", "COLE", "VEX", "One heavy on the door controls. Going dark.", (0.45, 0.8, 0.35)),
    ("vo_s6_cole_sleep", "COLE", "VEX", "Sleep tight.", (0.5, 0.8, 0.3)),
    ("vo_s6_cole_rigged", "COLE", "VEX", "Scanner's rigged. Vance, the door's yours.", (0.45, 0.8, 0.35)),
    ("vo_s6_cole_exit", "COLE", "VEX", "Way out's wedged. Max, bring the car round back.", (0.45, 0.8, 0.35)),
]
SFX_CREDITS = 200


def _audio_tools():
    sys.path.insert(0, os.path.join(REPO, "tools", "audio"))
    import el_api
    import manifest
    import process
    return el_api, manifest, process


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
            print(f"S6_AUDIO missing after import: {n}")
            continue
        if n in loops:
            a.set_editor_property("looping", True)
            unreal.EditorAssetLibrary.save_loaded_asset(a, only_if_is_dirty=False)
        done.append(n)
    print(f"S6_AUDIO imported {len(done)} of {len(tasks)} into {DEST}: {', '.join(done)}")


if unreal is not None:
    import_to_editor()
elif __name__ == "__main__":
    argv = sys.argv[1:]
    sel = [a for a in argv if not a.startswith("--")]
    if "--process-only" not in argv:
        generate(sel, "--force" in argv, "--dry-run" in argv)
    if "--dry-run" not in argv:
        process_all(sel)
