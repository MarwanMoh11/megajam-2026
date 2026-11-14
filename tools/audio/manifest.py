"""Every sound in the game, in one place: docs/AUDIO_SCRIPT.md is its script, TakeAudioLines.inl its C++ table.

Edit here, then run `make_tables.py` (rewrites the table and the script's tables), `standins.py` (synth stand-ins for
anything not generated yet), `generate.py` (ElevenLabs) and `process.py` (decode, trim, tempo, loudness).

Names are file names in C:\\Jam\\Audio (<id>.wav) and asset names in /Game/Take/Audio (<id>).
A line's `key` is how the game asks for it:
  "SPEAKER|words"  the game already flashes those words (the line is matched by them; later rows with the same key
                   are variants, played in turn, each with its own subtitle);
  "@event"         the audio layer asks by event: trigger_<TakeTriggers name>, fire_<DEVICE NAME>, or a card.
Text is ASCII only, with the copy deck's rules (ART_STYLE 15.7): one name per thing, sentence case, nobody runs.
"""

TEMPO_BPM = 120
BEATS_PER_BAR = 4

# One ElevenLabs voice per character (premade, or Voice Design for the minor characters; multilingual v2). Settings: stability, similarity, style;
# speed (0.7-1.2) where a voice is too slow for its bars.
VOICES = {
    "DIRECTOR":  dict(voice="Bill",    voice_id="pqHfZKP75CvOlQylNhV4", why="old, crisp, American: a studio-era director", settings=(0.45, 0.8, 0.35)),
    "JULIAN":    dict(voice="Callum",  voice_id="N2lVS1w4EtoT3dr4eOWO", why="husky trickster: the actor in white, amused by everything", settings=(0.4, 0.8, 0.45)),
    "MAYA":      dict(voice="Lily",    voice_id="pFZP5JQG7iQjIQuC4Bku", why="velvety, British, unhurried: the gaffer on the earpiece", settings=(0.55, 0.8, 0.2)),
    "VEX":       dict(voice="Charlie", voice_id="IKne3meq5aSn9XLyUdCD", why="young, Australian, quick: props and sound on the earpiece", settings=(0.4, 0.8, 0.4)),
    "VANCE":     dict(voice="Print It - Vance (Voice Design, candidate 1 of 2, picked by Marwan)", voice_id="oDQy4IsGdMzP2p0Mj1UU", why="smooth, theatrical English showman with a hint of menace: the auctioneer at the mic", settings=(0.6, 0.8, 0.25), speed=1.1),
    "CHIEF":     dict(voice="Print It - Chief (Voice Design, candidate 2 of 3, picked by Marwan)", voice_id="miMOxTjoFH6ibPnOrTEa", why="gruff, older, working-class, unhurried and contemptuous: the security chief (Adam was too recognisable)", settings=(0.5, 0.8, 0.3)),
    "DOORMAN":   dict(voice="Print It - Doorman (Voice Design, candidate 1 of 2, picked by Marwan)", voice_id="QqoZ07rA2Lp7reDiPFFj", why="stiff, icily polite, old-fashioned East Coast: the doorman on the carpet", settings=(0.55, 0.8, 0.2)),
    "GUARD":     dict(voice="Chris",   voice_id="iP95p4xoKVk53GoZ742B", why="down-to-earth: the guard at the rope", settings=(0.5, 0.8, 0.3)),
    "BODYGUARD": dict(voice="Harry",   voice_id="SOYHLrjzK2X1ezoPC6cr", why="rough, fierce: Vance's bodyguard", settings=(0.45, 0.8, 0.4)),
    "BROADCAST": dict(voice="Print It - Broadcast (Voice Design, candidate 1 of 2, picked by Marwan)", voice_id="zp7wscOG54kLZi18OhAE", why="harried 1950s live-TV producer, fast and clipped: the drone's producer on talkback", settings=(0.55, 0.8, 0.25)),
    "SECURITY":  dict(voice="Print It - Security (Voice Design, candidate 2 of 2, picked by Marwan)", voice_id="BKCBzTejPWQ1UuUOlatr", why="young, burly, breathless Brooklyn guard: the guards who come in at the alarm", settings=(0.35, 0.8, 0.5)),
}
TTS_MODEL = "eleven_multilingual_v2"
EARPIECE = {"MAYA", "VEX", "BROADCAST"}   # band-passed like a radio in process.py

# id, speaker, text (the subtitle, word for word), key, event (for the script), say (what the voice reads, if
# different from the text: numbers spelled out, delivery cues in the prompt are not used)
LINES = [
    # The director (the voice of the film)
    ("vo_director_action", "DIRECTOR", "Action!", "DIRECTOR|Action!", "Every take starts (after the clapper; the title card's Enter)", None),
    ("vo_director_cut", "DIRECTOR", "Cut!", "@card_cut", "A blooper (the blooper slate draws the subtitle)", None),
    ("vo_director_print", "DIRECTOR", "Print it!", "@card_print", "PRINT IT (the end slate draws the subtitle)", None),
    ("vo_director_keep", "DIRECTOR", "Keep that in!", "DIRECTOR|Keep that in!", "A deleted scene restored (The Bow)", None),
    ("vo_director_thatsone", "DIRECTOR", "That's the one.", "DIRECTOR|That's the one.", "A goal mark cleared", None),
    ("vo_director_intro_julian", "DIRECTOR", "Julian, our bidder with no invitation. Every guard is watching him.",
     "DIRECTOR|Julian, our bidder with no invitation. Every guard is watching him.", "First time on Julian", None),
    ("vo_director_intro_maya", "DIRECTOR", "Maya, our gaffer, up on the lights. Crew pass: nobody watches her.",
     "DIRECTOR|Maya, our gaffer, up on the lights. Crew pass: nobody watches her.", "First cut to Maya", None),
    ("vo_director_intro_vex", "DIRECTOR", "Vex, props and sound. Crew pass: backstage is his.",
     "DIRECTOR|Vex, props and sound. Crew pass: backstage is his.", "First cut to Vex", None),

    # Julian's quips (new: one per scene beat, said by trigger)
    ("vo_julian_door", "JULIAN", "Good evening. I'm on the list. Somewhere.", "@trigger_Door", "Through the scanner, inside the doors", None),
    ("vo_julian_floor", "JULIAN", "Champagne, chandeliers and forty guards. My kind of room.", "@trigger_Scene2", "Onto the hall floor (Scene 2)", None),
    ("vo_julian_rope", "JULIAN", "Velvet rope. How quaint.", "@trigger_PastRope", "Past the rope, on the dais", None),
    ("vo_julian_keycard", "JULIAN", "Thank you, Mr Vance. Lovely speech.", "@trigger_Keycard", "Keycard lifted (Scene 3)", None),
    ("vo_julian_reel", "JULIAN", "Lot 41. Sold, to the man in white.", "@trigger_Case", "The case opened: he has the reel",
     "Lot forty-one. Sold, to the man in white."),
    ("vo_julian_bow", "JULIAN", "Thank you. You're too kind.", "@trigger_Bow", "The Bow, before the director's line", None),

    # Maya on the earpiece
    ("vo_maya_hint_door", "MAYA", "Cut to me with 2. My searchlight blinds the scanner.",
     "MAYA|Cut to me with 2. My searchlight blinds the scanner.", "Hint after two bloopers at the street", "Cut to me with two. My searchlight blinds the scanner."),
    ("vo_maya_hint_rope", "MAYA", "Banner on the beam, my spot on the guard. Or Vex's cart pulls him away.",
     "MAYA|Banner on the beam, my spot on the guard. Or Vex's cart pulls him away.", "Hint after two bloopers at the rope", None),
    ("vo_maya_searchlight", "MAYA", "Searchlight's on the scanner. Walk him through.", "@fire_SEARCHLIGHT", "SEARCHLIGHT fires", None),
    ("vo_maya_spot", "MAYA", "Spot's on him. He can't see a thing.", "@fire_FOLLOW SPOT", "FOLLOW SPOT fires", None),
    ("vo_maya_spot_2", "MAYA", "Got him. Go.", "@fire_FOLLOW SPOT", "FOLLOW SPOT fires (second and later)", None),
    ("vo_maya_banner", "MAYA", "Banner's down. The beam's behind it.", "@fire_BANNER", "BANNER fires", None),
    ("vo_maya_pyro", "MAYA", "Pyro! Eyes shut!", "MAYA|Pyro! Eyes shut!", "PYRO BOARD fires", None),

    # Vex on the earpiece
    ("vo_vex_hint_desk", "VEX", "My desk blinds the whole room for two bars. Then lift it.",
     "VEX|My desk blinds the whole room for two bars. Then lift it.", "Hint after two bloopers on the dais", None),
    ("vo_vex_hint_winch", "VEX", "Winch first: the curtain hides him from the guards and opens the trap. Then Maya's pyro. Then run.",
     "VEX|Winch first: the curtain hides him from the guards and opens the trap. Then Maya's pyro. Then run.",
     "Hint after two bloopers at the case", None),
    ("vo_vex_cart", "VEX", "Champagne cart, rolling. Mind the glasses.", "@fire_CHAMPAGNE CART", "CHAMPAGNE CART fires", None),
    ("vo_vex_daiscart", "VEX", "Dais cart. That'll turn some heads.", "@fire_DAIS CART", "DAIS CART fires", None),
    ("vo_vex_desk", "VEX", "Sound desk. Sorry, everyone.", "@fire_SOUND DESK", "SOUND DESK fires (the feedback)", None),
    ("vo_vex_winch", "VEX", "Curtain coming down. The trap is yours.", "VEX|Curtain coming down. The trap is yours.", "CURTAIN WINCH fires", None),

    # Vance at the mic: his speech on every band break (4 bars), the same cue in the level, a new speech each time
    ("vo_vance_speech_1", "VANCE", "Ladies and gentlemen: the Director's Cut.", "VANCE|Ladies and gentlemen: the Director's Cut.", "Vance's speech, first break", None),
    ("vo_vance_speech_2", "VANCE", "Lot 41 has never been screened. One reel, one print, and tonight it finds a home.",
     "VANCE|Ladies and gentlemen: the Director's Cut.", "Vance's speech, second break", "Lot forty-one has never been screened. One reel, one print, and tonight it finds a home."),
    ("vo_vance_speech_3", "VANCE", "Shot in a single take in 1953, and locked in a vault ever since.",
     "VANCE|Ladies and gentlemen: the Director's Cut.", "Vance's speech, third break", "Shot in a single take in nineteen fifty-three, and locked in a vault ever since."),
    ("vo_vance_speech_4", "VANCE", "The director burned every other copy. This is the cut he kept.",
     "VANCE|Ladies and gentlemen: the Director's Cut.", "Vance's speech, fourth break", None),
    ("vo_vance_speech_5", "VANCE", "The bidding opens at two million. Do I hear two million?",
     "VANCE|Ladies and gentlemen: the Director's Cut.", "Vance's speech, fifth break (then from the top)", None),
    ("vo_vance_security", "VANCE", "Security!", "VANCE|Security!", "Vance sees him", None),
    ("vo_vance_alarm", "VANCE", "The reel! Seal the hall!", "VANCE|The reel! Seal the hall!", "The case opened: the alarm", None),

    # The watchers' barks
    ("vo_chief_nomatch", "CHIEF", "No match. Hold there, sir.", "CHIEF|No match. Hold there, sir.", "The scanner sees him", None),
    ("vo_chief_light", "CHIEF", "Who turned that light on?!", "CHIEF|Who turned that light on?!", "The scanner blinded", None),
    ("vo_chief_carpet", "CHIEF", "You. That carpet is for invited guests.", "CHIEF|You. That carpet is for invited guests.", "The chief sees him on the lobby carpet", None),
    ("vo_doorman_invite", "DOORMAN", "Invitation, sir?", "DOORMAN|Invitation, sir?", "The doorman sees him on the red carpet", None),
    ("vo_doorman_invite_2", "DOORMAN", "Guests only on the carpet, sir.", "DOORMAN|Invitation, sir?", "The doorman, second and later", None),
    ("vo_guard_aisle", "GUARD", "Sir? That aisle is closed.", "GUARD|Sir? That aisle is closed.", "The guard sees him", None),
    ("vo_guard_aisle_2", "GUARD", "Back to your seat, sir.", "GUARD|Sir? That aisle is closed.", "The guard, second and later", None),
    ("vo_bodyguard_podium", "BODYGUARD", "Step away from the podium.", "BODYGUARD|Step away from the podium.", "The bodyguard sees him", None),
    ("vo_broadcast_who", "BROADCAST", "Who is the gentleman in white?", "BROADCAST|Who is the gentleman in white?", "The drone sees him", None),
    ("vo_broadcast_who_2", "BROADCAST", "Camera one, find me that gentleman in white.", "BROADCAST|Who is the gentleman in white?", "The drone, second and later", None),
    ("vo_broadcast_chase", "BROADCAST", "Stay on the man in white!", "BROADCAST|Stay on the man in white!", "The alarm: the drone follows him", None),
    ("vo_security_nobody", "SECURITY", "Nobody leaves!", "SECURITY|Nobody leaves!", "The guards come in at the alarm", None),
    ("vo_security_there", "SECURITY", "There! The man in white!", "SECURITY|There! The man in white!", "A guard sees him in the finale", None),
    ("vo_security_there_2", "SECURITY", "Stop that man!", "SECURITY|There! The man in white!", "The guards, second and later", None),
]

# id, event, seconds (target), prompt (ElevenLabs sound effects), loop
SFX = [
    ("sfx_switcher_click", "Every cut, on the press frame", 0.5, "A single crisp click of a vintage broadcast video switcher button, dry, close, very short", False),
    ("sfx_clapper", "A take starts, PRINT IT", 0.6, "A single film clapperboard clap, sharp wooden clack, dry studio, very short", False),
    ("sfx_film_burn", "A blooper: the film burns", 1.5, "Film projector reel jamming and the celluloid burning: crackle and flutter, short", False),
    ("sfx_buzzer", "A blooper: the red slate", 0.8, "Short harsh game show wrong-answer buzzer, one blast", False),
    ("sfx_tape_stop", "A blooper, the clock running out: the music stops", 1.0, "Tape stop effect: a jazz band recording slowing down and grinding to a halt, pitch dropping", False),
    ("sfx_rewind", "After a blooper, through the reload", 1.2, "Fast analog tape rewind whirr with squeaky high-pitched chatter, short", False),
    ("sfx_lamp_strike", "SEARCHLIGHT, FOLLOW SPOT fire", 1.0, "A big theatre follow spot lamp switching on: heavy metal shutter clunk and electric arc hum", False),
    ("sfx_banner_drop", "BANNER fires", 1.5, "A large heavy fabric banner unfurling and dropping from a height, cloth whoosh and a soft flap", False),
    ("sfx_glass_smash", "CHAMPAGNE CART, DAIS CART fire", 1.8, "A serving cart of champagne glasses rolling and crashing over, many glasses shattering", False),
    ("sfx_feedback", "SOUND DESK fires: the band is knocked out", 1.8, "Loud PA microphone feedback squeal in a big hall, piercing, rising, then cut off", False),
    ("sfx_winch_release", "CURTAIN WINCH fires", 1.5, "Theatre counterweight rope lock released: a metal clank and a rope running fast through a pulley", False),
    ("sfx_thud", "The curtain or a shutter lands", 0.8, "A heavy stage curtain landing on a wooden stage: deep soft thud with dust", False),
    ("sfx_pyro", "PYRO BOARD fires", 1.8, "Stage pyrotechnic gerbs firing: bright sparks fountain hiss and crackle with a whoomp", False),
    ("sfx_alarm", "The case opens: the alarm", 4.0, "A loud old-fashioned electric alarm bell ringing in a marble auction hall", False),
    ("sfx_shutters", "The alarm: the shutters fall", 2.0, "Heavy steel security shutters rattling down fast and slamming shut", False),
    ("sfx_chime", "Keycard lifted, a checkpoint, an arrival", 0.8, "A soft bright two-note vibraphone chime, short and pleasant", False),
    ("sfx_heartbeat", "Loop: a watcher holds him", 2.0, "Tense heartbeat, two heavy thumps per second, close and muffled, seamless loop", True),
    ("sfx_ringing", "Loop: composure falling", 4.0, "High-pitched tinnitus ear ringing tone, steady and thin, seamless loop", True),
]

# id, event, bars (shipped length at 120 BPM; 0 = one-shot, trimmed), seconds to generate, kind, prompt
# kind: "music" (ElevenLabs Music, instrumental) or "sfx" (sound effects endpoint).
MUSIC = [
    ("mus_bed", "The band's bed, looped from the take's start; the clock follows it", 16, 40, "music",
     "Noir jazz quintet, exactly 120 BPM in 4/4, steady medium swing from the first beat: upright bass walking in "
     "quarter notes, brushed snare and ride cymbal, piano comping, muted trumpet and tenor saxophone trading short "
     "smoky phrases. A 1950s heist film, a black-tie auction hall at night, sly and understated. No intro, no "
     "ending, no tempo changes, no breaks."),
    ("mus_heist", "The alarm: crashes in at the same tempo, its last bar ends on the deadline", 10, 24, "music",
     "Urgent noir jazz heist chase, exactly 120 BPM in 4/4, the same quintet at full tilt from the first beat: "
     "driving walking bass, hard-swinging drums with crashes, stabbing brass hits, tense piano. Builds for twenty "
     "seconds to a final big brass hit. No tempo changes."),
    ("mus_sting", "PRINT IT: the escape", 0, 4, "music",
     "A short triumphant noir jazz sting: the quintet's brass fanfare and a final ringing cymbal, three seconds."),
    # The bass is a one-bar figure the music model repeats (a 5 s "then silence" take came back as one held note):
    # process.py keeps its first bar. The music model only sustains a muted trumpet, so the trumpet comes from the
    # sound-effects endpoint: three short notes on beat 1, beat 2 and the swung "and" of 2.
    ("band_bass", "Warning, a bar before the guard turns", 0, 10, "music",
     "Solo upright jazz bass, nothing else: no drums, no piano, no horns. Dry studio, exactly 120 BPM in 4/4. A sly, "
     "ominous noir figure: four low plucked quarter notes, one on each beat, walking down, then a bar of rest; the "
     "same figure again and again."),
    ("band_trumpet", "Warning, a bar before the drone passes", 0, 1.5, "sfx",
     "Two short staccato notes on a muted jazz trumpet, falling minor third, bah-dah, then silence, dry studio, solo"),
    ("band_drumroll", "Warning, a bar before Vance speaks", 0, 2.2, "sfx",
     "Jazz snare drum roll, one bar long, crescendo from soft to loud, ending on a short cymbal hit, dry studio"),
    ("band_hihat", "The beam drops (one beat in every 4 bars)", 0, 0.8, "sfx",
     "A single open hi-hat cymbal splash on a jazz drum kit, dry, short"),
    # --- QA time (Nov 14): variants, so a repeated warning is never the same sample twice running (the clock picks
    # one at random, never the last). The bed is in G minor (measured): pitched variants ask for it, and process.py
    # tunes them onto the G-minor scale (BAND_KEY) with at most a 1.5-semitone shift.
    ("band_bass_2", "Warning, a bar before the guard turns (variant)", 0, 10, "music",
     "Solo upright jazz bass in G minor, nothing else: no drums, no piano, no horns. Dry studio, exactly 120 BPM in "
     "4/4. A sly noir figure: four low plucked quarter notes walking down G, F, E-flat, D, one on each beat, then a "
     "bar of rest; the same figure again and again."),
    ("band_bass_3", "Warning, a bar before the guard turns (variant)", 0, 10, "music",
     "Solo upright jazz bass in G minor, nothing else: no drums, no piano, no horns. Dry studio, exactly 120 BPM in "
     "4/4. A creeping noir figure: four low plucked quarter notes, D, E-flat, F-sharp, G, one on each beat, then a "
     "bar of rest; the same figure again and again."),
    ("band_trumpet_2", "Warning, a bar before the drone passes (variant)", 0, 1.5, "sfx",
     "Two short staccato notes on a harmon-muted jazz trumpet, G falling to D, bah-dah, soft, then silence, dry "
     "studio, solo"),
    ("band_trumpet_3", "Warning, a bar before the drone passes (variant)", 0, 1.5, "sfx",
     "Three short soft staccato notes on a muted jazz trumpet, B-flat, A, G, swung, then silence, dry studio, solo"),
    ("band_drumroll_2", "Warning, a bar before Vance speaks (variant)", 0, 2.2, "sfx",
     "Jazz snare drum roll with wire brushes, one bar long, swelling from soft to loud, ending on a short cymbal "
     "choke, dry studio"),
    ("band_drumroll_3", "Warning, a bar before Vance speaks (variant)", 0, 2.2, "sfx",
     "Jazz snare drum press roll with sticks, one bar long, crescendo from soft to medium, ending on a single rim "
     "click, dry studio"),
    ("band_hihat_2", "The beam drops, at the beam (variant)", 0, 0.8, "sfx",
     "A single soft hi-hat chick opening into a short splash on a jazz drum kit, quiet, dry, short"),
    ("band_hihat_3", "The beam drops, at the beam (variant)", 0, 0.8, "sfx",
     "A single quiet hi-hat foot splash, a soft metallic chick with a brief shimmer, jazz drum kit, dry, short"),
]

# --- QA time: the band's key (measured from mus_bed: G minor). process.py tunes pitched phrase variants onto it.
BAND_KEY_ROOT = 7            # pitch class of G (C = 0)
BAND_KEY_SCALE = (0, 2, 3, 5, 7, 8, 10)   # natural minor
KEY_FIT_PHRASES = ("band_bass_", "band_trumpet_")   # variant ids only: the shipped originals stay as they are

# Loudness targets (integrated LUFS) and the peak ceiling, by prefix.
LOUDNESS = {"vo_": -16.0, "sfx_": -18.0, "mus_bed": -21.0, "mus_heist": -18.0, "mus_sting": -15.0, "band_": -17.0}
PEAK_DBFS = -1.0


def loudness_for(name):
    best = None
    for prefix, lufs in LOUDNESS.items():
        if name.startswith(prefix) and (best is None or len(prefix) > len(best[0])):
            best = (prefix, lufs)
    return best[1] if best else -18.0


def all_ids():
    return [l[0] for l in LINES] + [s[0] for s in SFX] + [m[0] for m in MUSIC]
