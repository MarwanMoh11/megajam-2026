"""Turns the generated MP3s in C:\\Jam\\Audio\\raw into the shipped WAVs in C:\\Jam\\Audio, with listen-free checks.

  python tools/audio/process.py [id ...]          (default: every raw file)

- Voices: mono, silence trimmed, pauses over 0.6 s cut to 0.4 s, the earpiece band-pass for the crew and the broadcast, loudness-normalised.
- Sound effects: mono, trimmed to the onset (a press answers in its frame), tail faded; loops get a crossfaded seam.
- Music: the tempo is measured (onset envelope, comb search over the whole clip), the clip is resampled to exactly
  120 BPM when it is off (a pitch change under 4%; more is flagged for a retake), cut on a downbeat to a whole
  number of bars, and the bed's loop seam is crossfaded with the music that follows it. One-shot phrases are cut to
  their onset and at most a bar.
- Every file: integrated loudness to the manifest's target (ITU-R BS.1770, dsp.py), peak at most -1 dBFS,
  16-bit 44.1 kHz. checks.txt lists length, peak, loudness and tempo for each.
"""
import json
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dsp  # noqa: E402
import manifest as M  # noqa: E402
from audio_io import OUT_DIR, RATE, RAW_DIR, decode_mp3, read_wav, write_wav  # noqa: E402

SPB = 60.0 / M.TEMPO_BPM
BAR = SPB * M.BEATS_PER_BAR
SPEAKER = {l[0]: l[1] for l in M.LINES}
SFX = {s[0]: s for s in M.SFX}
MUSIC = {m[0]: m for m in M.MUSIC}


def db(x):
    return 20 * np.log10(max(float(x), 1e-9))


def mono(x):
    return x.mean(axis=1) if x.ndim == 2 else x


def to_rate(x, rate):
    if rate == RATE:
        return x
    return dsp.resample(x, RATE / rate)


def frame_rms(x, hop=256):
    m = mono(x)
    n = len(m) // hop
    return np.sqrt(np.mean(m[: n * hop].reshape(n, hop) ** 2, axis=1) + 1e-12), hop


def trim(x, thresh_db=-45.0, pre=0.02, post=0.15):
    """Cut leading and trailing silence (relative to an absolute threshold)."""
    rms, hop = frame_rms(x)
    loud = np.where(20 * np.log10(rms) > thresh_db)[0]
    if len(loud) == 0:
        return x
    a = max(0, loud[0] * hop - int(pre * RATE))
    b = min(len(x), (loud[-1] + 1) * hop + int(post * RATE))
    return x[a:b]


def fade(x, fin=0.003, fout=0.03):
    x = x.copy()
    n_in, n_out = min(len(x), int(fin * RATE)), min(len(x), int(fout * RATE))
    shape = (-1,) + (1,) * (x.ndim - 1)
    if n_in:
        x[:n_in] *= np.linspace(0, 1, n_in).reshape(shape)
    if n_out:
        x[-n_out:] *= np.linspace(1, 0, n_out).reshape(shape)
    return x


def cap_pauses(x, longest=0.6, keep=0.4):
    """Shortens any silence inside a line that is longer than `longest` to `keep` seconds: a designed voice's
    theatrical pauses would otherwise run a line past its bars (Vance's speech has 4)."""
    rms, hop = frame_rms(x)
    level = 20 * np.log10(rms)
    quiet = level < level.max() - 35
    pieces, last, i, n = [], 0, 0, len(quiet)
    while i < n:
        if not quiet[i]:
            i += 1
            continue
        j = i
        while j < n and quiet[j]:
            j += 1
        if i > 0 and j < n and (j - i) * hop > longest * RATE:
            pieces.append(x[last:i * hop + int(keep / 2 * RATE)])
            last = j * hop - int(keep / 2 * RATE)
        i = j
    if not pieces:
        return x
    pieces.append(x[last:])
    k = int(0.01 * RATE)
    w = np.linspace(0, 1, k)
    y = pieces[0]
    for p in pieces[1:]:
        y = np.concatenate([y[:-k], y[-k:] * (1 - w) + p[:k] * w, p[k:]])
    return y


def earpiece(x):
    band = dsp.spectral_filter(x, dsp.bandpass(320.0, 3400.0))
    return 0.85 * np.tanh(band * 2.0) / np.tanh(2.0) + 0.15 * band


def loudness(x):
    return dsp.loudness(x)


def limit(x, ceiling, max_cut_db=8.0, release=0.06):
    """A peak limiter: a 3 ms look-ahead, instant attack, exponential release, at most max_cut_db of reduction."""
    a = np.abs(x) if x.ndim == 1 else np.max(np.abs(x), axis=1)
    k = int(0.003 * RATE)
    padded = np.concatenate([a, np.zeros(k)])
    look = np.max(np.lib.stride_tricks.sliding_window_view(padded, k + 1), axis=1)[: len(a)]
    want = np.clip(ceiling / np.maximum(look, 1e-9), 10 ** (-max_cut_db / 20), 1.0)
    g = np.empty_like(want)
    r = np.exp(-1.0 / (release * RATE))
    cur = 1.0
    for i, w in enumerate(want.tolist()):
        cur = w if w < cur else w + (cur - w) * r
        g[i] = cur
    return x * (g if x.ndim == 1 else g[:, None])


def normalise(x, target):
    ceiling = 10 ** (M.PEAK_DBFS / 20)
    for _ in range(2):
        cur = loudness(x)
        if np.isfinite(cur):
            x = x * 10 ** ((target - cur) / 20)
        x = limit(x, ceiling)
    peak = np.max(np.abs(x))
    if peak > ceiling:
        x = x * (ceiling / peak)     # past the limiter's reach: quiet rather than clipped (the checks show it)
    return x


# --- tempo ---------------------------------------------------------------------------------------------------------
HOP = 256


def onset_env(x):
    m = mono(x)
    mag, f = dsp.stft_mag(m, 2048, HOP)
    mag = np.log1p(100 * mag)
    flux = np.maximum(0, np.diff(mag, axis=0)).sum(axis=1)
    flux = np.maximum(flux - dsp.moving_mean(flux, 31), 0)
    low = np.maximum(0, np.diff(mag[:, f < 200], axis=0)).sum(axis=1)
    return flux / (flux.max() or 1), low / (low.max() or 1), RATE / HOP


def comb(env, fps, bpm, phases=48):
    period = 60.0 / bpm * fps
    n = int((len(env) - period) // period)
    if n < 4:
        return 0.0, 0.0
    ph = np.linspace(0, period, phases, endpoint=False)
    idx = ph[:, None] + np.arange(n)[None, :] * period
    vals = np.interp(idx, np.arange(len(env)), env)
    score = vals.mean(axis=1)
    k = int(np.argmax(score))
    return float(score[k]), float(ph[k] / fps)


def measure_tempo(x):
    env, low, fps = onset_env(x)
    coarse = [(comb(env, fps, b)[0], b) for b in np.arange(90, 160.01, 0.5)]
    best = max(coarse)[1]
    fine = [(comb(env, fps, b, 96)[0], b) for b in np.arange(best - 1.0, best + 1.0, 0.01)]
    s, bpm = max(fine)
    return float(bpm), env, low, fps


def downbeat(low, fps, beat_phase):
    """Which of the 4 beats carries the most low-end onsets: taken as beat 1."""
    period = SPB * fps
    best, best_k = -1, 0
    for k in range(4):
        idx = beat_phase * fps + (k + np.arange(0, (len(low) / period - k) // 4) * 4) * period
        idx = idx[idx < len(low) - 1]
        v = np.interp(idx, np.arange(len(low)), low).mean() if len(idx) else 0
        if v > best:
            best, best_k = v, k
    return beat_phase + best_k * SPB


def fit_music(x, mid, bars):
    note = []
    bpm, env, low, fps = measure_tempo(x)
    note.append(f"measured {bpm:.2f} BPM")
    if abs(bpm / M.TEMPO_BPM - 1) > 0.04:
        note.append("OFF BY MORE THAN 4%: retake")
    if abs(bpm - M.TEMPO_BPM) > 0.05:
        # Played at RATE, a clip at bpm must get longer by bpm/120 to sit at 120.
        x = dsp.resample(x, bpm / M.TEMPO_BPM)
        note.append(f"resampled x{bpm / M.TEMPO_BPM:.4f}")
    env, low, fps = onset_env(x)
    _, phase = comb(env, fps, M.TEMPO_BPM, 96)
    first = downbeat(low, fps, phase)
    need = bars * BAR
    xfade = 0.03 if mid == "mus_bed" else 0.0
    # The bed skips a bar of intro when it can; the heist cue starts on its first downbeat (it crashes in).
    starts = [first + k * BAR for k in range(0, 8)]
    if mid == "mus_bed":
        starts = [s for s in starts if s >= 1.0] + starts
    start = next((s for s in starts if (s + need + xfade) * RATE <= len(x)), None)
    if start is None:
        # Not long enough for the bars asked: the most whole bars that fit.
        bars = int((len(x) / RATE - first - xfade) // BAR)
        start, need = first, bars * BAR
        note.append(f"ONLY {bars} BARS FIT")
    a, n = int(round(start * RATE)), int(round(need * RATE))
    seg = x[a:a + n].copy()
    if xfade:
        k = int(xfade * RATE)
        tail = x[a + n:a + n + k]
        w = np.linspace(0, 1, k)[:, None] if seg.ndim == 2 else np.linspace(0, 1, k)
        seg[:k] = seg[:k] * np.sqrt(w) + tail * np.sqrt(1 - w)    # the loop point plays on into the head
    else:
        seg = fade(seg, 0.0, 0.08)
    check_bpm, _, _, _ = measure_tempo(seg)
    note.append(f"cut {bars} bars from {start:.3f} s; result {check_bpm:.2f} BPM")
    return seg, "; ".join(note)


def process(iid):
    raw = os.path.join(RAW_DIR, iid + ".mp3")
    x, rate = decode_mp3(raw)
    x = to_rate(x, rate)
    note = ""
    if iid.startswith("vo_"):
        x = trim(mono(x), -45, 0.02, 0.15)
        x = cap_pauses(x)
        if SPEAKER.get(iid) in M.EARPIECE:
            x = earpiece(x)
        x = fade(x, 0.005, 0.04)
    elif iid in SFX:
        loop = SFX[iid][4]
        x = mono(x)
        if loop:
            k = int(0.08 * RATE)
            if len(x) > 3 * k:
                w = np.linspace(0, 1, k)
                head = x[:k] * np.sqrt(w) + x[-k:] * np.sqrt(1 - w)
                x = np.concatenate([head, x[k:-k]])
        else:
            x = fade(trim(x, -50, 0.004, 0.1), 0.002, 0.05)
    elif iid in MUSIC:
        _mid, _event, bars, _gen, _kind, _prompt = MUSIC[iid]
        if bars:
            x, note = fit_music(x, iid, bars)
        else:
            x = trim(x, -50, 0.003, 0.2)
            # The bass figure repeats every bar: cut just before the next bar's first note, so its pickup leads
            # into the bar the guard turns on.
            longest = {"band_bass": BAR - 0.01, "band_trumpet": BAR + 0.25, "band_drumroll": BAR + 0.4,
                       "band_hihat": 0.9, "mus_sting": 4.0}.get(iid, 4.0)
            x = fade(x[: int(longest * RATE)], 0.002, 0.04 if iid == "band_bass" else 0.12)
            if iid in ("band_bass", "band_trumpet", "band_drumroll", "band_hihat"):
                x = mono(x)
    x = normalise(x, M.loudness_for(iid))
    write_wav(os.path.join(OUT_DIR, iid + ".wav"), x)
    lufs = loudness(x)
    return (f"{iid:28s} {len(x) / RATE:6.2f} s  {('stereo' if x.ndim == 2 else 'mono'):6s}  peak {db(np.max(np.abs(x))):6.1f} dBFS  "
            f"{lufs:6.1f} LUFS (target {M.loudness_for(iid):.0f})  {note}")


def main(argv):
    ids = [a for a in argv if not a.startswith("--")]
    if not ids:
        ids = sorted(f[:-4] for f in os.listdir(RAW_DIR) if f.endswith(".mp3"))
    sources_path = os.path.join(OUT_DIR, "sources.json")
    sources = json.load(open(sources_path)) if os.path.exists(sources_path) else {}
    lines = []
    for iid in ids:
        if iid not in M.all_ids():
            print(f"skip {iid}: not in the manifest")
            continue
        try:
            line = process(iid)
        except Exception as e:  # report and go on: one bad file must not stop the batch
            line = f"{iid:28s} FAILED: {e}"
        else:
            sources[iid] = "elevenlabs"
        print(line)
        lines.append(line)
    json.dump(sources, open(sources_path, "w"), indent=1, sort_keys=True)
    checks = os.path.join(OUT_DIR, "checks.txt")
    old = {}
    if os.path.exists(checks):
        for l in open(checks):
            old[l.split()[0]] = l.rstrip("\n")
    for l in lines:
        old[l.split()[0]] = l
    with open(checks, "w") as f:
        f.write("\n".join(old[k] for k in sorted(old)) + "\n")


if __name__ == "__main__":
    main(sys.argv[1:])
