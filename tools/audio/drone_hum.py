"""The broadcast drone's hum: a soft, seamless 4 s loop (four detuned motors and a breathy rotor wash), made here.

Run with the audio venv:  /c/Jam/venv-audio/Scripts/python.exe tools/audio/drone_hum.py
Writes C:\\Jam\\Audio\\sfx_drone_hum.wav. Import it into /Game/Take/Audio (looping) with tools/audio/import_drone_hum.py.
Every frequency is a whole number of cycles over the loop, and the noise is built in the frequency domain over the
loop's length, so the end joins the start with no click.
"""
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from audio_io import OUT_DIR, RATE, write_wav  # noqa: E402

SECONDS = 4.0
N = int(RATE * SECONDS)
STEP = 1.0 / SECONDS   # every frequency is a multiple of this (Hz), so it loops clean


def snap(hz):
    return round(hz / STEP) * STEP


def periodic_noise(rng, shape_of_hz):
    """Noise that loops over N samples: random phases, magnitude shaped by shape_of_hz (Hz -> gain)."""
    freqs = np.fft.rfftfreq(N, 1.0 / RATE)
    mag = shape_of_hz(freqs)
    phase = rng.uniform(0, 2 * np.pi, len(freqs))
    x = np.fft.irfft(mag * np.exp(1j * phase), N)
    return x / (np.max(np.abs(x)) + 1e-9)


def main():
    rng = np.random.default_rng(7)
    t = np.arange(N) / RATE
    out = np.zeros(N)

    # Four motors, a little apart: they beat slowly against each other (the "alive" wobble of a hovering drone).
    for base in (116.0, 117.5, 119.25, 120.75):
        f = snap(base)
        for h, g in ((1, 1.0), (2, 0.35), (3, 0.12), (4, 0.05)):
            out += g * np.sin(2 * np.pi * f * h * t + rng.uniform(0, 2 * np.pi)) * 0.12

    # The rotor wash: breathy band noise (soft peak near 400 Hz), fluttering at the blade rate.
    def wash_shape(f):
        return np.exp(-((np.log(np.maximum(f, 1.0)) - np.log(400.0)) ** 2) / (2 * 0.6 ** 2))

    wash = periodic_noise(rng, wash_shape)
    flutter = 0.75 + 0.25 * np.sin(2 * np.pi * snap(23.0) * t)
    out += wash * flutter * 0.35

    # A low body under it (felt more than heard).
    out += 0.18 * np.sin(2 * np.pi * snap(58.0) * t)

    out = out / np.max(np.abs(out)) * 0.6
    path = os.path.join(OUT_DIR, "sfx_drone_hum.wav")
    write_wav(path, out)
    print(f"wrote {path}: {SECONDS} s, loop seam {abs(out[-1] - out[0]):.4f}")


if __name__ == "__main__":
    main()
