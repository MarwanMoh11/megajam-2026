# Scene 2 (the florist van) audio

MAX voice_id: extuKh6lOtuXuEDVJn7l

- Max: ElevenLabs Voice Design "Print It - Max" (sweaty, fast-talking, slightly nasal 1970s heist tech). Three
  previews in `C:\Jam\Audio\voice_design\MAX_1..3.mp3`; candidate 1 kept by script as the fastest talker (nobody has
  listened yet). Candidates and the pick are in `C:\Jam\Audio\voice_design\MAX.json`.
- Script: `tools/script/scene2_audio.py` (generate, process, and the in-editor import). WAVs in
  `C:\Jam\Audio\Script\S2`, assets in `/Game/Take/Audio/Script/S2/` (asset name = id).
- Voices: `vo_s2_max_open`, `vo_s2_max_retake`, `vo_s2_max_close`, `vo_s2_max_notyet`, `vo_s2_max_lock` (MAX, in the
  van, no band-pass); `vo_s2_bouncer_a`, `vo_s2_bouncer_b` (DOORMAN voice, through the CRT feed: a light 300-4500 Hz
  monitor-speaker band-pass). -16 LUFS.
- Sound effects: `s2_dial_click`, `s2_spark`, `s2_lock`, `s2_toggle`, `s2_hum` (looping), `s2_chime`. -18 LUFS.
