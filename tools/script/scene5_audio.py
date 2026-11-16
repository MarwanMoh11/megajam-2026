"""Scene 5 (the private auditorium) voices, score and sound effects: ElevenLabs -> C:\\Jam\\Audio\\script\\s5.

Outside the editor (the audio venv: numpy + miniaudio):
  C:\\Jam\\venv-audio\\Scripts\\python.exe tools/script/scene5_audio.py [--dry-run] [--force] [id ...]
      generate the missing raw MP3s, then process them all into 48 kHz WAVs
  ... tools/script/scene5_audio.py --process-only [id ...]     re-process the raw MP3s only
Import with tools/script/scene5_audio_import.py (inside the editor, holding the editor lock).

Uses tools/audio's API wrapper and DSP read-only: TTS model eleven_multilingual_v2, voices -16 LUFS, effects and the
score -18 LUFS, -1 dBFS peak ceiling, 16-bit. Processing runs at 44.1 kHz (tools/audio's rate) and the result is
resampled to 48 kHz. Pendleton gets a synthetic hall reverb; Vance (cuff mic) stays dry. Hits that the game plays on
a beat (timpani, breaker) are cut sample-accurately to their attack.
"""
import os
import sys

import numpy as np

REPO = r"C:\Jam\megajam"
OUT = r"C:\Jam\Audio\script\s5"
RAW = os.path.join(OUT, "raw")
OUT_RATE = 48000
sys.path.insert(0, os.path.join(REPO, "tools", "audio"))

# id, voice_id, text, (stability, similarity, style)
VO = [
    ("vo_s5_pendleton", "onwK4e9ZLuTAKqWW03F9",
     "Tonight, you witness history. Le Soleil Noir. The only surviving print in existence. Tomorrow, it enters my "
     "private vault, never to be seen by common eyes again.", (0.6, 0.8, 0.3)),
    ("vo_s5_vance_cuff", "N2lVS1w4EtoT3dr4eOWO", "Booth door is mag-locked from the inside. Cole, you're up.",
     (0.4, 0.8, 0.45)),
    # QA Nov 15 evening: the blackout to the wall is a scene of its own now. Vance rises out of the row and says where
    # he is going; Cole (Vex's voice, behind the screen, on the earpiece) answers with the way and the danger.
    ("vo_s5_vance_go", "N2lVS1w4EtoT3dr4eOWO", "Lights are down. I'm moving. Heading for the stage door.",
     (0.4, 0.8, 0.45)),
    ("vo_s5_cole_reply", "IKne3meq5aSn9XLyUdCD", "Copy. Oak door, south wall, right by the stage. It's the only "
     "way up to the booth. Ushers have torches: move while they watch the picture.", (0.4, 0.8, 0.4)),
    ("vo_s5_usher_who", "iP95p4xoKVk53GoZ742B", "Hey. Who's moving back there?", (0.5, 0.8, 0.3)),
    ("vo_s5_usher_alibi", "iP95p4xoKVk53GoZ742B", "Oh. Pardon me, sir. Enjoy the picture.", (0.5, 0.8, 0.3)),
    ("vo_s5_guard_seen", "SOYHLrjzK2X1ezoPC6cr", "You! Stop right there!", (0.3, 0.8, 0.6)),
]
REVERB = {"vo_s5_pendleton"}
EARPIECE = {"vo_s5_cole_reply"}                     # band-passed like the crew's radio (tools/audio/process.earpiece)

# id, seconds, prompt
SFX = [
    ("sfx_s5_timpani", 2.0, "One single huge concert timpani hit, very low and punchy, struck hard once, with a long "
     "big concert hall reverb tail, orchestral, cinematic, no other instruments"),
    ("sfx_s5_lights_cut", 1.0, "A heavy theatre breaker switch thrown: one loud mechanical CHUNK as all the house "
     "lights cut out at once, a low electrical thump, big room"),
    ("sfx_s5_projector", 3.0, "A vintage 35 mm film projector bursting into life: the arc lamp strikes with a "
     "sharp electrical clack and hum, then the motor spins up and the film rattles steadily through the gate"),
    ("sfx_s5_maglock", 0.7, "An electronic magnetic door lock refusing: a hard solenoid click followed by a short "
     "low electrical buzz, access denied, close"),
]
ON_BEAT = {"sfx_s5_timpani", "sfx_s5_lights_cut"}   # attack at t=0, sample-accurate
HALL_TAIL = {"sfx_s5_timpani"}                       # extra synthetic hall on the hit's tail
PEAK_NORM = {"sfx_s5_timpani": -1.0}                 # peak-normalised instead of loudness-normalised

# Nov 16, 02:25 (Marwan: "add sneaky music in scene 5"): was a thunderous orchestral main title; now the sneak's cue.
MUSIC = ("mus_s5_score", 60000,
         "Sneaky, tense instrumental underscore for a 1970s heist thriller, a thief creeping through a dark cinema: "
         "plucked pizzicato strings and a walking muted upright bass on tiptoe, soft brushed snare, low tremolo "
         "strings, a sly muted trumpet answering now and then, quiet vibraphone. Steady medium-slow pulse, "
         "suspenseful and playful, loopable. Instrumental only, no vocals.")
MUSIC_XFADE = 3.0


def generate(ids, force, dry):
    import el_api
    os.makedirs(RAW, exist_ok=True)
    todo = []
    for (iid, vid, text, (st, sim, style)) in VO:
        s = {"stability": st, "similarity_boost": sim, "style": style, "use_speaker_boost": True}
        todo.append((iid, f"{len(text)} chars", lambda v=vid, t=text, s=s: el_api.tts(v, t, "eleven_multilingual_v2", s)))
    for (iid, secs, prompt) in SFX:
        todo.append((iid, f"sfx {secs} s", lambda p=prompt, d=secs: el_api.sound_effect(p, d, 0.6, False)))
    mid, ms, mprompt = MUSIC
    todo.append((mid, f"music {ms // 1000} s", lambda: el_api.music(mprompt, ms)))
    todo = [t for t in todo if (not ids or t[0] in ids)
            and (force or not os.path.exists(os.path.join(RAW, t[0] + ".mp3")))]
    print(f"to generate: {len(todo)}: " + ", ".join(f"{t[0]} ({t[1]})" for t in todo))
    if dry:
        return
    for (iid, _what, call) in todo:
        try:
            data = call()
        except Exception as e:  # report and go on
            print(f"FAILED {iid}: {e}")
            continue
        with open(os.path.join(RAW, iid + ".mp3"), "wb") as f:
            f.write(data)
        print(f"ok {iid} ({len(data) // 1024} KB)")


def hall_reverb(x, rate, rt60=1.7, predelay=0.025, wet=0.22, seed=5):
    """Convolution with a synthetic hall: decaying noise, darker as it decays, a few early reflections."""
    rng = np.random.default_rng(seed)
    n = int(rt60 * 1.2 * rate)
    t = np.arange(n) / rate
    noise = rng.standard_normal(n) * np.exp(-6.91 * t / rt60)
    # darken the tail: a one-pole low-pass whose cutoff falls over time, done in two bands by crossfading
    k = np.exp(-2 * np.pi * 2500 / rate)
    lp = np.empty(n)
    acc = 0.0
    for i, v in enumerate(noise.tolist()):
        acc = (1 - k) * v + k * acc
        lp[i] = acc
    mix = np.clip(t / (rt60 * 0.5), 0, 1)
    ir = noise * (1 - mix) * 0.5 + lp * (0.5 + 0.5 * mix)
    ir = np.concatenate([np.zeros(int(predelay * rate)), ir])
    for d, g in ((0.011, 0.5), (0.019, 0.35), (0.031, 0.3), (0.047, 0.2)):
        i = int((predelay + d) * rate)
        if i < len(ir):
            ir[i] += g * np.max(np.abs(ir))
    ir /= np.sqrt(np.sum(ir ** 2))
    m = len(x) + len(ir) - 1
    nfft = 1 << (m - 1).bit_length()
    w = np.fft.irfft(np.fft.rfft(x, nfft) * np.fft.rfft(ir, nfft), nfft)[:m]
    dry = np.concatenate([x, np.zeros(len(ir) - 1)])
    w *= np.sqrt(np.mean(x ** 2)) / max(np.sqrt(np.mean(w[: len(x)] ** 2)), 1e-9)
    y = (1 - wet * 0.5) * dry + wet * w
    return y


def attack_trim(x, rate, rel_db=-30.0, back=0.001):
    """Cut to the first sample within rel_db of the peak, minus `back` seconds."""
    m = np.abs(x if x.ndim == 1 else x.max(axis=1))
    i = int(np.argmax(m >= np.max(m) * 10 ** (rel_db / 20)))
    return x[max(0, i - int(back * rate)):]


def process_all(ids):
    import process as P
    from audio_io import decode_mp3, write_wav
    R = P.RATE
    ceiling = 10 ** (-1.0 / 20)
    report = []
    for f in sorted(os.listdir(RAW)):
        iid = f[:-4]
        if not f.endswith(".mp3") or (ids and iid not in ids):
            continue
        x, rate = decode_mp3(os.path.join(RAW, f))
        x = P.to_rate(x, rate)
        if iid.startswith("vo_"):
            x = P.mono(x)
            x = P.fade(P.cap_pauses(P.trim(x, -45, 0.02, 0.15)), 0.005, 0.04)
            if iid in EARPIECE:
                x = P.earpiece(x)
            if iid in REVERB:
                x = P.fade(hall_reverb(x, R), 0.0, 0.3)
                x = P.trim(x, -60, 0.0, 0.05)
            target = -16.0
        elif iid.startswith("mus_"):
            # stereo kept; trailing silence and the final ending cut, then the tail crossfaded into the head
            x = P.trim(x, -50, 0.0, 0.0)
            end = int(2.0 * R)
            x = x[: len(x) - end] if len(x) > 30 * R else x
            k = int(MUSIC_XFADE * R)
            w = np.linspace(0, 1, k)[:, None] if x.ndim == 2 else np.linspace(0, 1, k)
            x = np.concatenate([x[:k] * np.sqrt(w) + x[-k:] * np.sqrt(1 - w), x[k:-k]])
            target = -18.0
        else:
            x = P.mono(x)
            if iid in ON_BEAT:
                x = attack_trim(x, R)
                if iid in HALL_TAIL:
                    x = hall_reverb(x / np.max(np.abs(x)), R, rt60=2.0, predelay=0.012, wet=0.35)
                # trailing cut relative to the peak, so the tail rings out
                x = P.trim(x, P.db(np.max(np.abs(x))) - 60, 0.0, 0.05)
                x = P.fade(x, 0.0, 0.15)
            else:
                x = P.fade(P.trim(x, -50, 0.004, 0.1), 0.002, 0.05)
            target = -18.0
        if iid in PEAK_NORM:
            x = x * (10 ** (PEAK_NORM[iid] / 20) / np.max(np.abs(x)))
        else:
            x = P.normalise(x, target)
        y = P.dsp.resample(x, OUT_RATE / R)
        peak = np.max(np.abs(y))
        if peak > ceiling:
            y = y * (ceiling / peak)
        write_wav(os.path.join(OUT, iid + ".wav"), y, OUT_RATE)
        m = np.abs(y if y.ndim == 1 else y.max(axis=1))
        lead_ms = 1000 * int(np.argmax(m > 10 ** (-40 / 20))) / OUT_RATE
        line = (f"{iid:20s} {len(y) / OUT_RATE:6.2f} s  peak {P.db(np.max(np.abs(y))):5.1f} dBFS  "
                f"{P.loudness(x):5.1f} LUFS  lead {lead_ms:4.1f} ms  ch {1 if y.ndim == 1 else y.shape[1]}")
        print(line)
        report.append(line)
    with open(os.path.join(OUT, "checks.txt"), "w") as f:
        f.write("\n".join(report) + "\n")


if __name__ == "__main__":
    argv = sys.argv[1:]
    sel = [a for a in argv if not a.startswith("--")]
    if "--process-only" not in argv:
        generate(sel, "--force" in argv, "--dry-run" in argv)
    if "--dry-run" not in argv:
        process_all(sel)
