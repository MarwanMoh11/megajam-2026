"""Scene 4 (the catwalk) voice lines, generated and processed with the step-6 audio pipeline (tools/audio).

  C:\\Jam\\venv-audio\\Scripts\\python.exe tools/script/scene4_audio.py [--force] [id ...]

Raw MP3s land in C:\\Jam\\Audio\\script\\s4\\raw, the processed WAVs (mono, trimmed, pauses capped, earpiece band-pass
for Nora's headset, -16 LUFS, peak -1 dBFS, 16-bit 44.1 kHz: exactly as process.py does voices) in
C:\\Jam\\Audio\\script\\s4, with lines.txt (file, speaker, text, seconds). A raw file that exists is reused unless --force.
Nora is Maya's voice (Lily); Guard 1 and the spotting guard use the project's GUARD voice (Chris).
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "audio"))
import el_api  # noqa: E402
import manifest as M  # noqa: E402
import process as P  # noqa: E402
from audio_io import RATE, decode_mp3, write_wav  # noqa: E402

OUT = r"C:\Jam\Audio\script\s4"
RAW = os.path.join(OUT, "raw")

# speaker -> (manifest voice, settings override or None); Guard 1 shouts, so a little less stability, more style
SPEAKERS = {"NORA": ("MAYA", None), "GUARD 1": ("GUARD", (0.4, 0.8, 0.45)), "GUARD": ("GUARD", (0.4, 0.8, 0.45))}
EARPIECE = {"NORA"}   # raw feed on her headset: the same band-pass as Maya's lines

LINES = [
    ("s4_nora_open", "NORA", "I'm in the rafters. Looking down at the whole chessboard. Vance, hold by the bar. "
                             "Let me clear your lane."),
    ("s4_guard1_spot", "GUARD 1", "What the hell, fix that spot! And go help the staff clean that mess before "
                                  "Pendleton sees it!"),
    ("s4_nora_go", "NORA", "Lane's clear. Go, Vance. Now."),
    ("s4_nora_retake", "NORA", "Hold, hold... they saw you. Again."),
    ("s4_guard1_hold", "GUARD 1", "Hold your posts! Nobody leaves the rope."),
    ("s4_guard_spotted", "GUARD", "Hey! You, in the jacket. Stop right there."),
]


def generate(lid, speaker, text, force):
    path = os.path.join(RAW, lid + ".mp3")
    if os.path.exists(path) and not force:
        return False
    vkey, override = SPEAKERS[speaker]
    v = M.VOICES[vkey]
    st, sim, style = override or v["settings"]
    settings = {"stability": st, "similarity_boost": sim, "style": style, "use_speaker_boost": True}
    if "speed" in v:
        settings["speed"] = v["speed"]
    data = el_api.tts(v["voice_id"], text, M.TTS_MODEL, settings)
    with open(path, "wb") as f:
        f.write(data)
    return True


def process(lid, speaker):
    x, rate = decode_mp3(os.path.join(RAW, lid + ".mp3"))
    x = P.to_rate(x, rate)
    x = P.trim(P.mono(x), -45, 0.02, 0.15)
    x = P.cap_pauses(x)
    if speaker in EARPIECE:
        x = P.earpiece(x)
    x = P.fade(x, 0.005, 0.04)
    target = M.loudness_for("vo_")
    x = P.normalise(x, target)
    write_wav(os.path.join(OUT, lid + ".wav"), x)
    return len(x) / RATE, P.loudness(x), P.db(max(abs(x.min()), abs(x.max())))


def main(argv):
    force = "--force" in argv
    sel = [a for a in argv if not a.startswith("--")]
    os.makedirs(RAW, exist_ok=True)
    try:
        before = el_api.subscription()["used"]
    except Exception as e:
        before = None
        print(f"credits before: unknown ({e})")
    rows = []
    for (lid, speaker, text) in LINES:
        if sel and lid not in sel:
            continue
        if generate(lid, speaker, text, force):
            print(f"generated {lid}")
        secs, lufs, peak = process(lid, speaker)
        print(f"{lid:20s} {secs:5.2f} s  {lufs:6.1f} LUFS  peak {peak:5.1f} dBFS")
        rows.append((lid, speaker, text, secs))
    if not sel:
        with open(os.path.join(OUT, "lines.txt"), "w", encoding="utf-8") as f:
            f.write("file\tspeaker\ttext\tseconds\n")
            for (lid, speaker, text, secs) in rows:
                f.write(f"{lid}.wav\t{speaker}\t{text}\t{secs:.2f}\n")
    if before is not None:
        try:
            print(f"credits used this run: {el_api.subscription()['used'] - before}")
        except Exception:
            pass


if __name__ == "__main__":
    main(sys.argv[1:])
