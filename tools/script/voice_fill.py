"""Voices for every line that had only a subtitle (Nov 16, the voice fill), plus the one missing sound (sfx_beep).

Outside the editor (python with numpy + miniaudio, as for tools/audio):
  C:\\Jam\\venv-audio\\Scripts\\python.exe tools/script/voice_fill.py [--dry-run] [--force] [--process-only]
Inside the editor (hold the editor lock):
  "$PY" -I tools/ue_mcp.py py tools/script/voice_fill_import.py   import every WAV, replacing and saving

Two kinds of line:
- Script lines (AScriptSequencer::OnSay with no sound): /Game/Take/Audio/Script/Fill/vo_fill_<hash>, where hash is
  FNV-1a 32 of the UTF-8 "SPEAKER|Line" (speaker upper-cased), the same key OnSay builds (ReelVoiceFill in
  ScriptSequencer.cpp). A line edited in code loses its voice until this runs again.
- The hall's side lines (SetLife, by name): /Game/Take/Audio/sl_*, where UTakeAudioComponent::FindSound looks.
Voices and settings are the ones the scene sessions picked (tools/audio/manifest.VOICES, scene3/5/7 audio scripts);
Nora and Max on the earpiece band-pass, as in Scenes 3 and 4.
"""
import os
import shutil
import sys
from concurrent.futures import ThreadPoolExecutor

REPO = r"C:\Jam\megajam"
OUT = r"C:\Jam\Audio\script\fill"
RAW = os.path.join(OUT, "raw")
FILL_DEST = "/Game/Take/Audio/Script/Fill"
SL_DEST = "/Game/Take/Audio"

MAX_VOICE = "W61sMqsWccdiqyhEKpI7"         # Scene 2/3's Max (designed)
PENDLETON_VOICE = "onwK4e9ZLuTAKqWW03F9"   # Daniel, Scenes 5 and 7

# speaker (upper case, as OnSay keys it) -> (voice key in manifest.VOICES or a raw id, settings or None, earpiece)
SPEAKERS = {
    "COLE": ("VEX", (0.3, 0.8, 0.6), False),
    "MAX (V.O.)": (MAX_VOICE, (0.4, 0.8, 0.5), True),
    "NORA (V.O.)": ("MAYA", None, True),
    "NORA": ("MAYA", None, True),
    "PENDLETON": (PENDLETON_VOICE, (0.6, 0.8, 0.3), False),
    "VANCE": ("VANCE", None, False),
    "GUARD 1": ("GUARD", (0.4, 0.8, 0.45), False),
    "BOUNCER": ("DOORMAN", None, False),
    "ENFORCER": ("SECURITY", (0.35, 0.8, 0.5), False),
}

# (speaker, line) exactly as the code says them
SCRIPT = [
    ("BOUNCER", "Enjoy the screening, Your Grace."),
    ("GUARD 1", "What the hell? Somebody fix that spot!"),
    ("NORA", "That lamp under the pipe, by the east door. Loosen its clamp and it swings right into his eyes."),
    ("NORA", "The bulb's blown. They're turning back... we lost it."),
    ("MAX (V.O.)", "You're up, Vance. The plate, by the door."),
    ("MAX (V.O.)", "South wall, Vance. The oak door."),
    ("COLE", "Door's yours, Your Grace. Projectionist's stair, west end, through the staff door. Way out's through the prop store."),
    ("NORA (V.O.)", "I see you, Vance. Corridor's clear all the way west. Every guard's watching the film."),
    ("MAX (V.O.)", "Staff door on your right. West end of the hall's dark, the rope's behind you."),
    ("VANCE", "Reel two. Right on cue."),
    ("VANCE", "Le Soleil Noir. Exit, stage left."),
    ("MAX (V.O.)", "Alarm's tripped and your stair's mag-locked! Lobby door, Vance. Go!"),
    ("NORA (V.O.)", "Pendleton and his ape, coming up the grand stair. Get out of sight: the door on your left!"),
    ("PENDLETON", "Going somewhere, Mr. Vance? That reel belongs to me."),
    ("PENDLETON", "Take it off him."),
    ("COLE", "Don't stop! Move!"),
    ("MAX (V.O.)", "Stay on me! Down the back corridor, the car's out back!"),
]
# Scene 6's enforcer barks every GuardBrain line through Say (GuardBrain.h's bark lists).
BARKS = ["Huh?", "Hm? Who's there?", "What was that...?", "What was that?", "Did you hear that?",
         "Something over there...", "Come out, come out...", "I know someone's here.", "Where are you...",
         "Must've been nothing.", "Just the old building settling.", "Rats. Big rats.", "Hey! You there!",
         "Stop right there!", "Intruder!", "Where'd he go?", "Lost him...", "He can't have gone far.",
         "Man down! Man down!", "Eddie? Eddie, wake up!", "What the...?", "Who's messing with that?"]
SCRIPT += [("ENFORCER", b) for b in BARKS]

# The hall's side lines: name -> (voice key, text). Women: GUEST (Bella); men: GUEST_OLD, GUEST_GRUFF, GUEST_QUEUE;
# the waiters: CLERK.
SIDE = {
    "sl_photo_again_1": ("GUEST_QUEUE", "Lovely! One more for the Herald."),
    "sl_photo_again_2": ("GUEST_QUEUE", "Swell! Once more, for the Herald."),
    "sl_photo_beckon_1": ("GUEST_QUEUE", "You there! One for the society page?"),
    "sl_photo_beckon_2": ("GUEST_QUEUE", "Hold it! A picture for the papers?"),
    "sl_photo_blink_1": ("GUEST_OLD", "Ha! I'm seeing stars!"),
    "sl_photo_blink_2": ("GUEST", "Oh! Spots before my eyes!"),
    "sl_photo_next_1": ("GUEST_QUEUE", "Who's next? Smile for the Herald!"),
    "sl_photo_next_2": ("GUEST_QUEUE", "Anybody else? Don't be shy!"),
    "sl_photo_pose_1": ("GUEST_QUEUE", "Closer, now. Chins up!"),
    "sl_photo_pose_2": ("GUEST_QUEUE", "Big smiles. Hold it..."),
    "sl_photo_reply_1": ("GUEST", "Us? Oh, how thrilling!"),
    "sl_photo_reply_2": ("GUEST", "Is my hair all right?"),
    "sl_photo_thanks_1": ("GUEST", "Do send us a print!"),
    "sl_photo_thanks_2": ("GUEST", "Page six, darling, page six!"),
    "sl_reunion_chat_1": ("GUEST", "You look a million dollars!"),
    "sl_reunion_chat_2": ("GUEST", "You still owe me twenty from Saratoga!"),
    "sl_reunion_drink_1": ("GUEST", "Come on, let's find a drink."),
    "sl_reunion_drink_2": ("GUEST", "Inside. You can tell me everything."),
    "sl_reunion_reply_1": ("GUEST_OLD", "Bunny! They let you out of Monte Carlo?"),
    "sl_reunion_reply_2": ("GUEST_OLD", "Good heavens! Not since before the war!"),
    "sl_reunion_spot_1": ("GUEST", "Kit? Kit Harlow, as I live and breathe!"),
    "sl_reunion_spot_2": ("GUEST", "Well, I'll be! Sandy Bell!"),
    "sl_rivals_aside_1": ("GUEST", "Ha! Every season, those two."),
    "sl_rivals_aside_2": ("GUEST", "Here we go again."),
    "sl_rivals_brush_1": ("GUEST_GRUFF", "May the deeper pocket win, old man."),
    "sl_rivals_brush_2": ("GUEST_GRUFF", "Your pockets aren't that deep, old man."),
    "sl_rivals_needle_1": ("GUEST_OLD", "You took the Degas from me, Pemberton."),
    "sl_rivals_needle_2": ("GUEST_OLD", "Not this time, Pemberton. Not this lot."),
    "sl_rivals_shush_1": ("GUEST", "Shh!"),
    "sl_rivals_shush_2": ("GUEST", "Shh! Do you mind?"),
    "sl_rivals_vow_1": ("GUEST_OLD", "I'll go to the last penny. Mark me."),
    "sl_rivals_vow_2": ("GUEST_GRUFF", "We shall see whose pocket runs dry."),
    "sl_spill_sorry_1": ("GUEST_OLD", "Oh! Terribly sorry, old boy."),
    "sl_spill_sorry_2": ("GUEST_OLD", "I say! Frightfully sorry."),
    "sl_spill_tease_1": ("GUEST_GRUFF", "Steady on, Reggie!"),
    "sl_spill_tease_2": ("GUEST_GRUFF", "Ha! Two left feet, as ever."),
    "sl_spill_waiter_1": ("CLERK", "Quite all right, sir."),
    "sl_spill_waiter_2": ("CLERK", "No harm done, sir. I'll fetch another."),
    "sl_toast_call_1": ("GUEST_OLD", "Friends! A moment, if you please."),
    "sl_toast_call_2": ("GUEST_OLD", "I say, a word, everyone!"),
    "sl_toast_laugh_1": ("GUEST", "Ha! Well said!"),
    "sl_toast_laugh_2": ("GUEST", "Ha! Not me, I hope!"),
    "sl_toast_reply_1": ("GUEST_GRUFF", "Hear, hear!"),
    "sl_toast_reply_2": ("GUEST_GRUFF", "Here's how!"),
    "sl_toast_toast_1": ("GUEST_OLD", "To Lot Forty-One, and the fool who pays for it!"),
    "sl_toast_toast_2": ("GUEST_OLD", "To the old girl: may she fetch a king's ransom!"),
    "sl_toast_waiter_1": ("CLERK", "Champagne? Fresh off the ice."),
    "sl_toast_waiter_2": ("CLERK", "A fresh glass, with the house's compliments."),
    "sl_waiter_accept_1": ("GUEST_OLD", "Don't mind if I do."),
    "sl_waiter_accept_2": ("GUEST", "How very kind."),
    "sl_waiter_decline_2": ("GUEST", "Not for me, thank you."),
    "sl_waiter_decline_bentley": ("GUEST_GRUFF", "No thank you, I'm driving the Bentley."),
    "sl_waiter_decline_glass": ("GUEST_OLD", "I've one already, thank you."),
    "sl_waiter_offer_bidding": ("CLERK", "A glass before the bidding?"),
    "sl_waiter_offer_house": ("CLERK", "Compliments of the house."),
    "sl_waiter_offer_madam": ("CLERK", "Champagne, madam?"),
    "sl_waiter_offer_sir": ("CLERK", "Champagne, sir?"),
    "sl_waiter_verygood_madam": ("CLERK", "Very good, madam."),
    "sl_waiter_verygood_sir": ("CLERK", "Very good, sir."),
    "sl_whisper_ashford_1": ("GUEST", "Isn't that the Ashford boy?"),
    "sl_whisper_ashford_2": ("GUEST_OLD", "I heard he lost everything."),
    "sl_whisper_bidder_1": ("GUEST", "Is he one of the bidders, do you think?"),
    "sl_whisper_bidder_2": ("GUEST_OLD", "In that jacket? I rather doubt it."),
    "sl_whisper_jacket_1": ("GUEST", "A white jacket, after dark? How daring."),
    "sl_whisper_jacket_2": ("GUEST_OLD", "Darling, don't stare."),
    "sl_whisper_who_1": ("GUEST", "Who's the fellow in the white jacket?"),
    "sl_whisper_who_2": ("GUEST_OLD", "Never seen him. New money, I'd wager."),
}
BEEP_FROM = r"C:\Jam\Audio\script\s1\sfx_s1_beep.wav"   # the scanner's beep, reused for Scene 7's door panel


def fill_name(speaker, line):
    """vo_fill_<FNV-1a 32 of UTF-8 'SPEAKER|Line'>: the name ReelVoiceFill::Find builds in ScriptSequencer.cpp."""
    h = 2166136261
    for b in (speaker.upper() + "|" + line).encode("utf-8"):
        h = ((h ^ b) * 16777619) & 0xFFFFFFFF
    return "vo_fill_%08x" % h


def items():
    """(asset name, voice key or id, settings or None, earpiece, text, speaker)"""
    out = []
    for speaker, line in SCRIPT:
        key, settings, ear = SPEAKERS[speaker]
        out.append((fill_name(speaker, line), key, settings, ear, line, speaker))
    for name, (key, text) in SIDE.items():
        out.append((name, key, None, False, text, key))
    return out


def _audio_tools():
    sys.path.insert(0, os.path.join(REPO, "tools", "audio"))
    import el_api
    import manifest
    import process
    return el_api, manifest, process


def generate(force, dry):
    el_api, M, _ = _audio_tools()
    os.makedirs(RAW, exist_ok=True)
    todo = [i for i in items() if force or not os.path.exists(os.path.join(RAW, i[0] + ".mp3"))]
    print(f"to generate: {len(todo)} lines, ~{sum(len(i[4]) for i in todo)} credits")
    if dry:
        return

    def one(i):
        name, key, settings, _ear, text, _who = i
        v = M.VOICES.get(key)
        vid = v["voice_id"] if v else key
        st = settings or (v["settings"] if v else (0.5, 0.8, 0.3))
        s = {"stability": st[0], "similarity_boost": st[1], "style": st[2], "use_speaker_boost": True}
        try:
            data = el_api.tts(vid, text, M.TTS_MODEL, s)
        except Exception as e:  # report and go on
            return f"FAILED {name}: {e}"
        with open(os.path.join(RAW, name + ".mp3"), "wb") as f:
            f.write(data)
        return f"ok {name} ({len(data) // 1024} KB) {text[:40]}"

    with ThreadPoolExecutor(3) as pool:
        for msg in pool.map(one, todo):
            print(msg, flush=True)


def process_all():
    _, M, P = _audio_tools()
    from audio_io import decode_mp3, write_wav
    ear = {i[0] for i in items() if i[3]}
    lines = []
    for i in items():
        src = os.path.join(RAW, i[0] + ".mp3")
        if not os.path.exists(src):
            print(f"MISSING raw {i[0]}")
            continue
        x, rate = decode_mp3(src)
        x = P.mono(P.to_rate(x, rate))
        x = P.fade(P.cap_pauses(P.trim(x, -45, 0.02, 0.15)), 0.005, 0.04)
        if i[0] in ear:
            x = P.earpiece(x)
        x = P.normalise(x, M.loudness_for("vo_"))
        write_wav(os.path.join(OUT, i[0] + ".wav"), x)
        lines.append(f"{i[0]}\t{i[5]}\t{i[4]}\t{len(x) / P.RATE:.2f}")
    shutil.copyfile(BEEP_FROM, os.path.join(OUT, "sfx_beep.wav"))
    with open(os.path.join(OUT, "lines.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    print(f"processed {len(lines)} lines + sfx_beep into {OUT}")


if __name__ == "__main__":
    argv = sys.argv[1:]
    if "--process-only" not in argv:
        generate("--force" in argv, "--dry-run" in argv)
    if "--dry-run" not in argv:
        process_all()
