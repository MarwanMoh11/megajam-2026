"""numpy-only signal tools for process.py (no scipy: its wheel would not download here).

- resample(x, factor): FFT resampling to factor x the length (the clip plays factor x longer at the same rate).
- spectral_filter(x, gain_of_hz): zero-phase filtering by a magnitude response.
- loudness(x): integrated loudness, ITU-R BS.1770-4 (K-weighting by its 48 kHz reference filters' response,
  400 ms blocks with 75% overlap, absolute gate -70 LUFS, relative gate -10 LU).
- stft_mag(x, n, hop): magnitude spectrogram (frames x bins).
"""
import numpy as np

RATE = 44100


def _per_channel(x, fn):
    if x.ndim == 1:
        return fn(x)
    return np.stack([fn(x[:, c]) for c in range(x.shape[1])], axis=1)


def resample(x, factor):
    n = len(x)
    m = int(round(n * factor))

    def one(v):
        spec = np.fft.rfft(v)
        out_bins = m // 2 + 1
        if out_bins <= len(spec):
            spec = spec[:out_bins]
        else:
            spec = np.concatenate([spec, np.zeros(out_bins - len(spec), dtype=spec.dtype)])
        return np.fft.irfft(spec, m) * (m / n)

    return _per_channel(x, one)


def spectral_filter(x, gain_of_hz):
    n = len(x)
    f = np.fft.rfftfreq(n, 1.0 / RATE)
    g = gain_of_hz(np.maximum(f, 1e-3))
    return _per_channel(x, lambda v: np.fft.irfft(np.fft.rfft(v) * g, n))


def bandpass(lo, hi, order=4):
    return lambda f: 1.0 / np.sqrt(1 + (lo / f) ** (2 * order)) / np.sqrt(1 + (f / hi) ** (2 * order))


def _biquad_mag(b, a, f, fs=48000.0):
    z = np.exp(-2j * np.pi * f / fs)
    return np.abs((b[0] + b[1] * z + b[2] * z * z) / (a[0] + a[1] * z + a[2] * z * z))


def k_weight(f):
    shelf = _biquad_mag((1.53512485958697, -2.69169618940638, 1.19839281085285),
                        (1.0, -1.69065929318241, 0.73248077421585), np.minimum(f, 23999.0))
    hp = _biquad_mag((1.0, -2.0, 1.0), (1.0, -1.99004745483398, 0.99007225036621), np.minimum(f, 23999.0))
    return shelf * hp


def loudness(x):
    x = x if x.ndim == 2 else x[:, None]
    if len(x) < int(0.4 * RATE):
        x = np.concatenate([x, np.zeros((int(0.4 * RATE) - len(x), x.shape[1]))])
    y = spectral_filter(x, k_weight)
    block, hop = int(0.4 * RATE), int(0.1 * RATE)
    starts = range(0, len(y) - block + 1, hop)
    z = np.array([np.mean(y[s:s + block] ** 2, axis=0).sum() for s in starts])
    lj = -0.691 + 10 * np.log10(np.maximum(z, 1e-12))
    keep = lj > -70
    if not keep.any():
        return -np.inf
    rel = -0.691 + 10 * np.log10(z[keep].mean()) - 10
    keep &= lj > rel
    return float(-0.691 + 10 * np.log10(z[keep].mean()))


def stft_mag(m, n=2048, hop=256):
    if len(m) < n:
        m = np.concatenate([m, np.zeros(n - len(m))])
    frames = 1 + (len(m) - n) // hop
    idx = np.arange(n)[None, :] + hop * np.arange(frames)[:, None]
    win = np.hanning(n)
    return np.abs(np.fft.rfft(m[idx] * win, axis=1)), np.fft.rfftfreq(n, 1.0 / RATE)


def moving_mean(v, k):
    return np.convolve(v, np.ones(k) / k, mode="same")
