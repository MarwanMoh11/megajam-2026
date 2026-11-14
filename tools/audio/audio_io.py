"""WAV in and out (16-bit PCM, float arrays shaped (frames, channels)) and MP3 decoding (miniaudio)."""
import os
import wave

import numpy as np

OUT_DIR = r"C:\Jam\Audio"
RAW_DIR = os.path.join(OUT_DIR, "raw")
STANDIN_DIR = os.path.join(OUT_DIR, "standin")
RATE = 44100


def write_wav(path, data, rate=RATE):
    data = np.asarray(data, dtype=np.float64)
    if data.ndim == 1:
        data = data[:, None]
    pcm = np.clip(np.round(data * 32767.0), -32768, 32767).astype("<i2")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with wave.open(path, "wb") as w:
        w.setnchannels(data.shape[1])
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(pcm.tobytes())


def read_wav(path):
    with wave.open(path, "rb") as w:
        ch, rate, n = w.getnchannels(), w.getframerate(), w.getnframes()
        raw = np.frombuffer(w.readframes(n), dtype="<i2").astype(np.float64) / 32768.0
    return raw.reshape(-1, ch), rate


def decode_mp3(path):
    import miniaudio
    dec = miniaudio.decode_file(path, output_format=miniaudio.SampleFormat.SIGNED16)
    data = np.frombuffer(dec.samples, dtype="<i2").astype(np.float64) / 32768.0
    return data.reshape(-1, dec.nchannels), dec.sample_rate
