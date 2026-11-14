"""Synthesised stand-ins for every sound effect and music cue, named like the final files.

Writes C:\\Jam\\Audio\\<id>.wav for each id in the manifest's SFX and MUSIC that has no file yet (`--force`: all of
them, but never over a generated one), and records it as "standin" in C:\\Jam\\Audio\\sources.json. Generated
files (process.py) replace them file for file. Voices get no stand-in: the subtitle carries the line.

  python tools/audio/standins.py [--force] [id ...]
"""
import json
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import manifest as M  # noqa: E402
from audio_io import OUT_DIR, RATE, write_wav  # noqa: E402

SPB = 60.0 / M.TEMPO_BPM          # seconds per beat
BAR = SPB * M.BEATS_PER_BAR
rng = np.random.default_rng(7)


def t_of(seconds):
    return np.arange(int(round(seconds * RATE))) / RATE


def env(n, attack=0.005, decay=0.2):
    t = np.arange(n) / RATE
    return np.minimum(1.0, t / max(attack, 1e-4)) * np.exp(-t / max(decay, 1e-4))


def tone(freq, seconds, harmonics=(1.0,), attack=0.005, decay=0.3):
    t = t_of(seconds)
    s = sum(a * np.sin(2 * np.pi * freq * (k + 1) * t) for k, a in enumerate(harmonics))
    return s * env(len(t), attack, decay)


def noise(seconds, decay=0.1, attack=0.001, hp=0.0):
    n = int(round(seconds * RATE))
    x = rng.standard_normal(n)
    if hp:
        x = x - np.convolve(x, np.ones(8) / 8, mode="same") * hp
    return x * env(n, attack, decay)


def mix_into(buf, sig, at):
    i = int(round(at * RATE))
    j = min(len(buf), i + len(sig))
    if i < len(buf):
        buf[i:j] += sig[: j - i]


def midi(n):
    return 440.0 * 2 ** ((n - 69) / 12)


def padd(*sigs):
    out = np.zeros(max(len(x) for x in sigs))
    for x in sigs:
        out[: len(x)] += x
    return out


def norm(x, peak=0.8):
    m = np.max(np.abs(x)) or 1.0
    return x / m * peak


# --- the band ----------------------------------------------------------------------------------------------------
# A minor blues-ish noir loop, 2 bars a chord: Dm7, Dm7, Gm7, A7, Dm7, Bb7, Em7b5, A7 (x2 = 16 bars).
CHORDS = [(50, [53, 57, 60]), (50, [53, 57, 60]), (55, [58, 62, 65]), (57, [61, 64, 67]),
          (50, [53, 57, 60]), (58, [62, 65, 68]), (52, [55, 58, 62]), (57, [61, 64, 67])]


def bass_note(n, seconds=SPB * 0.95):
    return tone(midi(n - 12), seconds, harmonics=(1.0, 0.5, 0.25, 0.1), attack=0.004, decay=0.35)


def ride(seconds=SPB):
    return noise(seconds, decay=0.25, hp=1.0) * 0.12


def brush(seconds=SPB):
    return noise(seconds, decay=0.08, attack=0.01) * 0.1


def piano(notes, seconds=SPB * 0.9):
    return sum(tone(midi(n), seconds, harmonics=(1.0, 0.3, 0.1), attack=0.003, decay=0.25) for n in notes) * 0.18


def bed(bars=16):
    buf = np.zeros(int(round(bars * BAR * RATE)))
    for bar in range(bars):
        root, notes = CHORDS[(bar // 2) % len(CHORDS)]
        t0 = bar * BAR
        walk = [root, root + 3, root + 5, root + 7] if bar % 2 == 0 else [root + 7, root + 5, root + 3, root + 1]
        for beat in range(4):
            mix_into(buf, bass_note(walk[beat]) * 0.5, t0 + beat * SPB)
            mix_into(buf, ride() * (1.3 if beat % 2 else 1.0), t0 + beat * SPB)
            mix_into(buf, ride() * 0.6, t0 + beat * SPB + SPB * 2 / 3)          # the swing's skip note
            if beat % 2 == 1:
                mix_into(buf, brush(), t0 + beat * SPB)
                mix_into(buf, piano(notes), t0 + beat * SPB + SPB * 2 / 3)
    return norm(buf, 0.6)


def heist(bars=10):
    buf = np.zeros(int(round(bars * BAR * RATE)))
    for bar in range(bars):
        root = [50, 50, 53, 50, 55, 53, 57, 57, 50, 50][bar % 10]
        t0 = bar * BAR
        for e in range(8):
            mix_into(buf, bass_note(root + (7 if e % 4 == 3 else 0), SPB * 0.45) * 0.5, t0 + e * SPB / 2)
            mix_into(buf, noise(0.1, decay=0.04, hp=1.0) * 0.12, t0 + e * SPB / 2)
        for beat in (1, 3):
            mix_into(buf, noise(0.2, decay=0.07) * 0.3, t0 + beat * SPB)              # snare
        mix_into(buf, tone(55, 0.3, decay=0.12) * 0.6, t0)                             # kick on the one
        if bar >= 2:
            stab = sum(tone(midi(n), 0.25, harmonics=(1, 0.7, 0.5, 0.3), decay=0.12) for n in (62, 65, 69)) * 0.12
            mix_into(buf, stab, t0 + SPB * 1.5)
            mix_into(buf, stab, t0 + SPB * 3.5)
    end = bars * BAR - BAR
    mix_into(buf, sum(tone(midi(n), 1.8, harmonics=(1, 0.7, 0.5, 0.3), decay=0.6) for n in (62, 66, 69, 74)) * 0.15, end)
    return norm(buf, 0.75)


def sting():
    buf = np.zeros(int(3.0 * RATE))
    for i, n in enumerate((62, 66, 69, 74)):
        mix_into(buf, tone(midi(n), 2.6 - i * 0.15, harmonics=(1, 0.6, 0.4, 0.2), attack=0.01, decay=0.9) * 0.2, i * 0.12)
    mix_into(buf, noise(2.0, decay=0.8, hp=1.0) * 0.1, 0.36)
    return norm(buf, 0.8)


def band_bass():
    buf = np.zeros(int(BAR * RATE))
    for beat, n in enumerate((38, 38, 41, 37)):
        mix_into(buf, bass_note(n + 12) * 0.8, beat * SPB)
    return norm(buf)


def band_trumpet():
    buf = np.zeros(int(BAR * RATE))
    for beat, n in enumerate((72, 75)):
        mix_into(buf, tone(midi(n), SPB * 0.9, harmonics=(1, 0.8, 0.6, 0.5, 0.3, 0.2), attack=0.02, decay=0.5), beat * SPB)
    return norm(buf)


def band_drumroll():
    buf = np.zeros(int((BAR + 0.4) * RATE))
    hits = 32
    for i in range(hits):
        mix_into(buf, noise(0.06, decay=0.03) * (0.2 + 0.8 * i / hits), i * BAR / hits)
    mix_into(buf, noise(0.5, decay=0.3, hp=1.0), BAR)
    return norm(buf)


# --- sound effects -----------------------------------------------------------------------------------------------
def sweep(f0, f1, seconds, amp=0.5):
    t = t_of(seconds)
    f = np.linspace(f0, f1, len(t))
    return np.sin(2 * np.pi * np.cumsum(f) / RATE) * amp


def sfx(sid, secs):
    n = int(secs * RATE)
    if sid == "sfx_switcher_click":
        return norm(noise(0.03, decay=0.006) + tone(2400, 0.03, decay=0.008) * 0.3)
    if sid == "sfx_clapper":
        return norm(noise(0.12, decay=0.02, hp=0.5))
    if sid == "sfx_film_burn":
        buf = np.zeros(n)
        for _ in range(60):
            mix_into(buf, noise(0.02, decay=0.006) * rng.uniform(0.2, 1), rng.uniform(0, secs - 0.05))
        return norm(buf + noise(secs, decay=secs) * 0.05)
    if sid == "sfx_buzzer":
        t = t_of(secs)
        return norm(np.sign(np.sin(2 * np.pi * 110 * t)) * env(len(t), 0.005, secs * 2) * (t < secs - 0.05), 0.6)
    if sid == "sfx_tape_stop":
        t = t_of(secs)
        f = 220 * (1 - t / secs) ** 2 + 20
        return norm(np.sin(2 * np.pi * np.cumsum(f) / RATE) * (1 - t / secs), 0.6)
    if sid == "sfx_rewind":
        return norm(sweep(800, 2400, secs, 0.3) * (1 + 0.5 * np.sin(2 * np.pi * 30 * t_of(secs))) + noise(secs, decay=secs) * 0.1, 0.5)
    if sid == "sfx_lamp_strike":
        return norm(padd(noise(0.15, decay=0.04), tone(100, secs, decay=0.6) * 0.3))
    if sid == "sfx_banner_drop":
        x = noise(secs, decay=secs / 2, attack=0.3)
        return norm(np.convolve(x, np.ones(30) / 30, mode="same"), 0.6)
    if sid == "sfx_glass_smash":
        buf = np.zeros(n)
        for _ in range(40):
            mix_into(buf, tone(rng.uniform(2500, 6000), 0.2, decay=0.05) * rng.uniform(0.1, 0.5), rng.uniform(0.3, secs - 0.3))
        mix_into(buf, noise(0.4, decay=0.1), 0.3)
        return norm(buf)
    if sid == "sfx_feedback":
        t = t_of(secs)
        return norm(np.sin(2 * np.pi * np.cumsum(np.linspace(1800, 3200, len(t))) / RATE) * np.minimum(1, t / 0.4) * (t < secs - 0.1), 0.5)
    if sid == "sfx_winch_release":
        buf = np.zeros(n)
        mix_into(buf, noise(0.1, decay=0.03), 0)
        mix_into(buf, noise(secs - 0.2, decay=secs) * 0.3, 0.15)
        return norm(buf)
    if sid == "sfx_thud":
        return norm(padd(tone(60, secs, decay=0.18), noise(0.1, decay=0.03) * 0.2))
    if sid == "sfx_pyro":
        buf = np.zeros(n)
        mix_into(buf, tone(80, 0.4, decay=0.15), 0)
        for _ in range(120):
            mix_into(buf, noise(0.01, decay=0.004) * rng.uniform(0.1, 0.6), rng.uniform(0, secs - 0.02))
        return norm(buf)
    if sid == "sfx_alarm":
        t = t_of(secs)
        bell = np.sign(np.sin(2 * np.pi * 900 * t)) * 0.3 * (0.6 + 0.4 * np.sign(np.sin(2 * np.pi * 18 * t)))
        return norm(bell * np.minimum(1, (secs - t) / 0.3), 0.6)
    if sid == "sfx_shutters":
        buf = np.zeros(n)
        for i in range(int(secs * 30)):
            mix_into(buf, noise(0.02, decay=0.01) * 0.3, i / 30)
        mix_into(buf, padd(tone(70, 0.4, decay=0.15), noise(0.1, decay=0.03)), secs - 0.45)
        return norm(buf)
    if sid == "sfx_chime":
        buf = np.zeros(n)
        mix_into(buf, tone(1047, 0.6, decay=0.25), 0)
        mix_into(buf, tone(1568, 0.6, decay=0.3), 0.12)
        return norm(buf, 0.6)
    if sid == "sfx_heartbeat":
        buf = np.zeros(n)
        for k in range(int(secs * 2)):
            mix_into(buf, tone(50, 0.15, decay=0.05), k * 0.5)
            mix_into(buf, tone(45, 0.15, decay=0.05) * 0.7, k * 0.5 + 0.16)
        return norm(buf)
    if sid == "sfx_ringing":
        t = t_of(secs)
        return norm(np.sin(2 * np.pi * 6000 * t) * (1 + 0.05 * np.sin(2 * np.pi * 0.5 * t)), 0.3)
    return norm(noise(min(secs, 0.3), decay=0.1))


def build(mid):
    for (sid, _e, secs, _p, _l) in M.SFX:
        if sid == mid:
            return sfx(sid, secs)
    return {"mus_bed": bed, "mus_heist": heist, "mus_sting": sting, "band_bass": band_bass,
            "band_trumpet": band_trumpet, "band_drumroll": band_drumroll,
            "band_hihat": lambda: norm(noise(0.6, decay=0.15, hp=1.0))}[mid]()


def main(argv):
    force = "--force" in argv
    only = [a for a in argv if not a.startswith("--")]
    sources_path = os.path.join(OUT_DIR, "sources.json")
    sources = json.load(open(sources_path)) if os.path.exists(sources_path) else {}
    ids = [s[0] for s in M.SFX] + [m[0] for m in M.MUSIC]
    made = 0
    for sid in ids:
        if only and sid not in only:
            continue
        path = os.path.join(OUT_DIR, sid + ".wav")
        if sources.get(sid) == "elevenlabs" or (os.path.exists(path) and not force):
            continue
        data = build(sid)
        if sid.startswith("mus_bed") or sid.startswith("mus_heist"):
            data = np.stack([data, data], axis=1)
        write_wav(path, data)
        sources[sid] = "standin"
        made += 1
    os.makedirs(OUT_DIR, exist_ok=True)
    json.dump(sources, open(sources_path, "w"), indent=1, sort_keys=True)
    print(f"stand-ins written: {made}")


if __name__ == "__main__":
    main(sys.argv[1:])
