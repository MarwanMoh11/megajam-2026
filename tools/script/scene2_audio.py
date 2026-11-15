"""Scene 2 (the florist van, Max's POV) sounds and voice lines: ElevenLabs -> C:\\Jam\\Audio\\Script\\S2 -> /Game/Take/Audio/Script/S2.

Outside the editor (the audio venv C:\\Jam\\venv-audio: python with numpy + miniaudio, as for tools/audio):
  python tools/script/scene2_audio.py --design-max                  Voice Design for MAX: three previews into
                                                                    C:\\Jam\\Audio\\voice_design\\MAX_<n>.mp3, the
                                                                    fastest talker kept as a voice, MAX.json written
  python tools/script/scene2_audio.py [--dry-run] [--force] [id ...] generate the missing raw MP3s, then process all
  python tools/script/scene2_audio.py --process-only [id ...]        re-process the raw MP3s into WAVs
Inside the editor (hold the editor lock):
  "$PY" -I tools/ue_mcp.py py tools/script/scene2_audio.py           import every S2 WAV, replacing and saving

Follows tools/script/scene8_audio.py: tools/audio's API wrapper and DSP (read-only), same TTS model, loudness targets
(vo -16 LUFS, sound effects -18 LUFS), -1 dBFS peak ceiling, 16-bit 44.1 kHz mono. Max is in the van with the player
(in-scene, no band-pass); the doorman's two lines come through Max's CRT security feed (a light monitor-speaker
band-pass, lighter than process.earpiece).
"""
# MAX voice_id: extuKh6lOtuXuEDVJn7l
import json
import os
import re
import sys

try:
    import unreal  # present only inside the editor
except ImportError:
    unreal = None

OUT = r"C:\Jam\Audio\Script\S2"
RAW = os.path.join(OUT, "raw")
DEST = "/Game/Take/Audio/Script/S2"
REPO = r"C:\Jam\megajam"
VD = r"C:\Jam\Audio\voice_design"

# id, seconds (the API's shortest is 0.5 s; the WAV is cut to this length), prompt, loop
SFX = [
    ("s2_dial_click", 0.15, "A single chunky analog radio tuning knob detent click, 1970s hardware, one dry plastic "
     "and metal click, close, no reverb", False),
    ("s2_spark", 0.6, "A bad electrical patch: a sharp electrical arc zap and crackle with a short buzz, sparks from "
     "a cable, close", False),
    ("s2_lock", 1.0, "BZZT-LOCK: a harsh electronic buzz cut off by a solid relay clunk and a low lock tone, "
     "satisfying, 1970s electronics, close", False),
    ("s2_toggle", 0.4, "A heavy metal toggle switch slammed hard, one loud mechanical clack, 1970s control panel, "
     "close, dry", False),
    ("s2_hum", 5.0, "Inside a parked van at night: the idling engine's low rumble, a humming rack of 1970s telecom "
     "gear and a high CRT whine, constant, no events, seamless loop", True),
    ("s2_chime", 1.2, "A retro-futurist 1970s palm scanner accepting: a short laser sweep tick, then a clean bright "
     "two-note rising approval chime", False),
]

# id, character, voice key in tools/audio/manifest.VOICES (None = MAX), text, (stability, similarity, style, speed)
VO = [
    ("vo_s2_max_open", "MAX", None, "Palm reader? Seriously?! Give me three seconds, Vance. Do not touch that glass "
     "yet!", (0.3, 0.8, 0.65, 1.12)),
    ("vo_s2_max_retake", "MAX", None, "Again. Three seconds, Vance. Don't touch that glass.", (0.45, 0.8, 0.45, 1.05)),
    ("vo_s2_max_close", "MAX", None, "Come on, come on... that's it, hold it...", (0.5, 0.8, 0.35, 1.0)),
    ("vo_s2_max_notyet", "MAX", None, "Not yet, not yet! Sync first!", (0.3, 0.8, 0.6, 1.12)),
    ("vo_s2_max_lock", "MAX", None, "You're a French duke now. Drop the hand.", (0.4, 0.8, 0.6, 1.0)),
    ("vo_s2_bouncer_a", "BOUNCER", "DOORMAN", "Sir? The glass.", (0.55, 0.8, 0.2, 1.0)),
    ("vo_s2_bouncer_b", "BOUNCER", "DOORMAN", "Is there a problem, monsieur?", (0.55, 0.8, 0.2, 1.0)),
]
MONITOR = {"vo_s2_bouncer_a", "vo_s2_bouncer_b"}   # heard through the CRT feed's speaker
SFX_CREDITS = 200

MAX_BRIEF = dict(
    name="Print It - Max",
    description=("A sweaty, fast-talking American tech guy around thirty, the wire man of a 1970s heist crew, "
                 "crammed into a surveillance van full of cables and monitors. Quick, nervous energy, slightly "
                 "nasal, wry wisecracking humour, words tumbling out. Close, dry recording, no reverb. Not an "
                 "announcer, not a narrator."),
    text=("Okay, okay, okay. Nobody touch anything. I've got the feed, I've got the line, and I've got maybe ten "
          "seconds before this whole thing melts. Talk to me, Vance."),
)


def _audio_tools():
    sys.path.insert(0, os.path.join(REPO, "tools", "audio"))
    import el_api
    import manifest
    import process
    return el_api, manifest, process


def max_voice():
    """The voice id this file names on its '# MAX voice_id:' line, else MAX.json's."""
    for line in open(os.path.abspath(__file__), encoding="utf-8"):
        m = re.match(r"# MAX voice_id: ([A-Za-z0-9]{20})\b", line)
        if m:
            return m.group(1)
    p = os.path.join(VD, "MAX.json")
    return json.load(open(p)).get("MAX voice_id") if os.path.exists(p) else None


def design_max():
    """Three Voice Design previews; keeps the fastest talker (same text, so the shortest speech), as Max is."""
    import numpy as np
    el_api, _, P = _audio_tools()
    from audio_io import decode_mp3
    os.makedirs(VD, exist_ok=True)
    previews = el_api.design_voice(MAX_BRIEF["description"], MAX_BRIEF["text"])
    state, lengths = {}, {}
    for i, (gid, audio) in enumerate(previews[:3], start=1):
        path = os.path.join(VD, f"MAX_{i}.mp3")
        open(path, "wb").write(audio)
        x, rate = decode_mp3(path)
        x = P.trim(P.mono(P.to_rate(x, rate)), -45, 0.02, 0.15)
        state[str(i)] = gid
        lengths[i] = len(x) / P.RATE
        print(f"candidate {i}: {path}  {lengths[i]:.2f} s speech, peak {P.db(np.max(np.abs(x))):.1f} dBFS")
    pick = min(lengths, key=lengths.get)
    vid = el_api.save_designed_voice(MAX_BRIEF["name"], MAX_BRIEF["description"], state[str(pick)])
    state.update({"picked": pick, "why": "fastest talker of the previews (picked by script; nobody listened)",
                  "MAX voice_id": vid})
    json.dump(state, open(os.path.join(VD, "MAX.json"), "w"), indent=1)
    print(f"MAX candidate {pick} saved as voice_id {vid}")


def generate(ids, force, dry):
    el_api, M, _ = _audio_tools()
    os.makedirs(RAW, exist_ok=True)
    todo = []
    for (sid, secs, prompt, loop) in SFX:
        todo.append((sid, SFX_CREDITS, lambda p=prompt, d=max(secs, 0.5), l=loop: el_api.sound_effect(p, d, 0.6, l)))
    for (lid, who, key, text, (st, sim, style, speed)) in VO:
        vid = max_voice() if key is None else M.VOICES[key]["voice_id"]
        if not vid:
            print(f"skip {lid}: no voice id for {who} yet (run --design-max)")
            continue
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


def monitor_speaker(x):
    """A small CRT monitor speaker: a 300-4500 Hz band with a touch of drive, lighter than process.earpiece."""
    import numpy as np
    import dsp
    band = dsp.spectral_filter(x, dsp.bandpass(300.0, 4500.0))
    return 0.6 * np.tanh(band * 1.5) / np.tanh(1.5) + 0.4 * band


def process_all(ids):
    import numpy as np
    _, M, P = _audio_tools()
    from audio_io import decode_mp3, write_wav
    loops = {s[0] for s in SFX if s[3]}
    length = {s[0]: s[1] for s in SFX}
    for f in sorted(os.listdir(RAW)):
        iid = f[:-4]
        if not f.endswith(".mp3") or (ids and iid not in ids):
            continue
        x, rate = decode_mp3(os.path.join(RAW, f))
        x = P.mono(P.to_rate(x, rate))
        if iid.startswith("vo_"):
            x = P.fade(P.cap_pauses(P.trim(x, -45, 0.02, 0.15)), 0.005, 0.04)
            if iid in MONITOR:
                x = monitor_speaker(x)
            target = M.loudness_for("vo_")
        elif iid in loops:
            k = int(0.08 * P.RATE)
            w = np.linspace(0, 1, k)
            x = np.concatenate([x[:k] * np.sqrt(w) + x[-k:] * np.sqrt(1 - w), x[k:-k]])
            target = M.loudness_for("sfx_")
        else:
            x = P.trim(x, -50, 0.004, 0.1)
            x = x[: int((length[iid] + 0.05) * P.RATE)]   # the asked length: a click stays a click
            x = P.fade(x, 0.002, min(0.05, length[iid] / 4))
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
            print(f"S2_AUDIO missing after import: {n}")
            continue
        if n in loops:
            a.set_editor_property("looping", True)
            unreal.EditorAssetLibrary.save_loaded_asset(a, only_if_is_dirty=False)
        done.append(n)
    print(f"S2_AUDIO imported {len(done)} of {len(tasks)} into {DEST}: {', '.join(done)}")


if unreal is not None:
    import_to_editor()
elif __name__ == "__main__":
    argv = sys.argv[1:]
    sel = [a for a in argv if not a.startswith("--")]
    if "--design-max" in argv:
        design_max()
    else:
        if "--process-only" not in argv:
            generate(sel, "--force" in argv, "--dry-run" in argv)
        if "--dry-run" not in argv:
            process_all(sel)
