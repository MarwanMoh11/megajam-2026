# Audio script (step 6)

Every voice line, sound effect and music cue in *Print It*, with the event that plays it, its file and its subtitle.
The source of truth is `tools/audio/manifest.py`; the tables below are written from it by
`tools/audio/make_tables.py`, which also writes the C++ voice table `Reel/Source/Reel/Take/TakeAudioLines.inl`.
Change a line there, not here.

## How it plays

- **The band** is a noir jazz quintet at exactly 120 BPM in 4/4 (1 bar = 2 s): a looping bed (`mus_bed`) plus
  one-shot warning phrases a bar ahead of a watcher (rule 2): the upright bass for the guard, the muted trumpet for
  the drone, a drum roll for Vance's speech, the hi-hat splash on the beam's drop. The phrases are never ducked.
- **QA, Nov 14 (Marwan's note 9): the warnings tell the player something.** A turn warning plays only when its
  watcher is within range of Julian (guard 16 m, drone 22 m, Vance 18 m); the first turn after he comes into range
  plays in full, later turns wait out a cooldown (14 s, or 5 s within 9 m) and play at 70%. Arrivals always play.
  Each phrase has two variants (`band_*_2`, `band_*_3`, tuned to the bed's G minor): never the same sample twice
  running, with a little level, pitch and late-timing variation. The beam's hi-hat is a quiet sound at the beam,
  attenuated, not a 2D splash. `bSmartWarnings` off on the clock restores the old behaviour.
- **QA, Nov 14 (note 2): slow time** (`ATakeDirector::BeginSlow`/`EndSlow`): the world and the clock slow together;
  the music holds under a low-pass at 30% and comes back in where the clock is, on a beat line. Voices run at
  normal speed.
- **The clock follows the music** (`UBarClockComponent`). The bed starts on the take's first frame at the clock's place
  in its loop: bar N plays loop bar ((N - 1) mod 16), so a blooper, a checkpoint or R restarts the music together
  with the clock, in phase, wherever the clock restarts. Each frame the clock reads the music's playback position
  and takes up drift by running at most 10% fast or slow; a gap of more than half a beat moves the music back onto
  the clock instead (logged; after four in a take the clock stops following and runs on game time).
- **The band's break:** every 8th bar the bed drops to 20% (Vance speaks over it once he has the mic). An arriving
  watcher's first phrase plays solo: the bed is out for that bar.
- **The sound desk's feedback** squeals and knocks the bed out for 2 bars; it comes back on the next bar line, in
  phase (the music keeps running silently under it).
- **The alarm** stops the band dead and rings the bell; the heist cue (`mus_heist`, 10 bars) plays so that its last
  bar ends on the deadline: it starts at its first bar, or at once from the matching point if that bar has passed
  (the alarm lands in the cue's first bar, then the hold bar, then the 8 live bars). **PRINT IT** stops it under the
  clapper and the sting. A blooper (and so the clock running out) stops the music under the tape-stop.
- **Voices** play as their subtitle comes on screen through the HUD's subtitle queue, so voice and subtitle always
  match; a voiced subtitle holds for at least its voice. A voice ducks the bed to 45%. A line with variants plays
  them in turn and shows each one's own words. "Cut!" and "Print it!" are drawn by the blooper slate and the end
  slate and are spoken without a queue.
- **Every press is answered in its frame:** the cut's switcher click plays on the press frame, never quantised.
- **Composure:** a heartbeat while a watcher holds Julian, and a ringing that rises as composure falls below 60.
- **Backstage:** on Vex's camera (C) the band is low-passed, as if through the flats. Nothing else is filtered.
- **Robust to gaps:** a sound that is not imported is skipped, and the synthesised stand-in from steps 1-3 plays
  where there is one (the warning pitches, the alarm tone). There is no click track (Nov 15).
- **Heard, not watched (Nov 15):** nothing on screen moves with the beat. The band is the clock and the warning;
  the HUD counts in seconds.

All tunables are `EditAnywhere`: the band's on the director's `Clock` (category Take|Band), the rest on its `Audio`
component (Take|Audio), and each device's `FireSound`.

## The pipeline

Everything runs from `C:\Jam\megajam` with the audio venv `C:\Jam\venv-audio` (the engine's Python 3.11 plus numpy,
scipy, miniaudio and pyloudnorm). Files land in `C:\Jam\Audio`.

| Step | Command | What it does |
|---|---|---|
| Tables | `python tools/audio/make_tables.py` | This file's tables and `TakeAudioLines.inl` from the manifest |
| Stand-ins | `python tools/audio/standins.py` | Synthesises every SFX and music file that is not there yet (`--force` redoes them) |
| Dry run | `python tools/audio/generate.py --dry-run` | Characters and estimated credits for what is not generated yet |
| Generate | `python tools/audio/generate.py [vo\|sfx\|music\|<id>...] [--force]` | ElevenLabs into `C:\Jam\Audio\raw\<id>.mp3`; skips ids already there |
| Process | `python tools/audio/process.py [<id>...]` | Decode, trim, earpiece, tempo fit (music), loop seam, loudness; writes `<id>.wav` and `checks.txt` |
| Import | `ue_mcp.py py tools/audio/import_audio.py` (editor, phase 2) | Every WAV into `/Game/Take/Audio`, replace and save, loops flagged |

The key is read from the User environment (`ELEVENLABS_API_KEY`) inside the script and never printed.

<!-- tables: begin (tools/audio/make_tables.py writes this part) -->

### Voices

| Character | ElevenLabs voice | Why |
|---|---|---|
| DIRECTOR | Bill (`pqHfZKP75CvOlQylNhV4`) | old, crisp, American: a studio-era director |
| JULIAN | Callum (`N2lVS1w4EtoT3dr4eOWO`) | husky trickster: the actor in white, amused by everything |
| MAYA | Lily (`pFZP5JQG7iQjIQuC4Bku`) | velvety, British, unhurried: the gaffer on the earpiece |
| VEX | Charlie (`IKne3meq5aSn9XLyUdCD`) | young, Australian, quick: props and sound on the earpiece |
| VANCE | Print It - Vance (Voice Design, candidate 1 of 2, picked by Marwan) (`oDQy4IsGdMzP2p0Mj1UU`) | smooth, theatrical English showman with a hint of menace: the auctioneer at the mic |
| CHIEF | Print It - Chief (Voice Design, candidate 2 of 3, picked by Marwan) (`miMOxTjoFH6ibPnOrTEa`) | gruff, older, working-class, unhurried and contemptuous: the security chief (Adam was too recognisable) |
| DOORMAN | Print It - Doorman (Voice Design, candidate 1 of 2, picked by Marwan) (`QqoZ07rA2Lp7reDiPFFj`) | stiff, icily polite, old-fashioned East Coast: the doorman on the carpet |
| GUARD | Chris (`iP95p4xoKVk53GoZ742B`) | down-to-earth: the guard at the rope |
| BODYGUARD | Harry (`SOYHLrjzK2X1ezoPC6cr`) | rough, fierce: Vance's bodyguard |
| BROADCAST | Print It - Broadcast (Voice Design, candidate 1 of 2, picked by Marwan) (`zp7wscOG54kLZi18OhAE`) | harried 1950s live-TV producer, fast and clipped: the drone's producer on talkback |
| SECURITY | Print It - Security (Voice Design, candidate 2 of 2, picked by Marwan) (`BKCBzTejPWQ1UuUOlatr`) | young, burly, breathless Brooklyn guard: the guards who come in at the alarm |

Model `eleven_multilingual_v2`. Earpiece lines (BROADCAST, MAYA, VEX) are band-passed like a radio in `process.py`.

### Lines

| File | Speaker | Subtitle (and what is said) | Event | How the game asks | Length |
|---|---|---|---|---|---|
| `vo_director_action` | DIRECTOR | Action! | Every take starts (after the clapper; the title card's Enter) | its words | ~0.7 s |
| `vo_director_cut` | DIRECTOR | Cut! | A blooper (the blooper slate draws the subtitle) | `@card_cut` | ~0.7 s |
| `vo_director_print` | DIRECTOR | Print it! | PRINT IT (the end slate draws the subtitle) | `@card_print` | ~1.1 s |
| `vo_director_keep` | DIRECTOR | Keep that in! | A deleted scene restored (The Bow) | its words | ~1.5 s |
| `vo_director_thatsone` | DIRECTOR | That's the one. | A goal mark cleared | its words | ~1.5 s |
| `vo_director_intro_julian` | DIRECTOR | Julian, our bidder with no invitation. Every guard is watching him. | First time on Julian | its words | ~4.5 s |
| `vo_director_intro_maya` | DIRECTOR | Maya, our gaffer, up on the lights. Crew pass: nobody watches her. | First cut to Maya | its words | ~4.9 s |
| `vo_director_intro_vex` | DIRECTOR | Vex, props and sound. Crew pass: backstage is his. | First cut to Vex | its words | ~3.8 s |
| `vo_julian_door` | JULIAN | Good evening. I'm on the list. Somewhere. | Through the scanner, inside the doors | `@trigger_Door` | ~3.0 s |
| `vo_julian_floor` | JULIAN | Champagne, chandeliers and forty guards. My kind of room. | Onto the hall floor (Scene 2) | `@trigger_Scene2` | ~3.8 s |
| `vo_julian_rope` | JULIAN | Velvet rope. How quaint. | Past the rope, on the dais | `@trigger_PastRope` | ~1.8 s |
| `vo_julian_keycard` | JULIAN | Thank you, Mr Vance. Lovely speech. | Keycard lifted (Scene 3) | `@trigger_Keycard` | ~2.6 s |
| `vo_julian_reel` | JULIAN | Lot 41. Sold, to the man in white. (said: *Lot forty-one. Sold, to the man in white.*) | The case opened: he has the reel | `@trigger_Case` | ~3.4 s |
| `vo_julian_bow` | JULIAN | Thank you. You're too kind. | The Bow, before the director's line | `@trigger_Bow` | ~2.2 s |
| `vo_maya_hint_door` | MAYA | Cut to me with 2. My searchlight blinds the scanner. (said: *Cut to me with two. My searchlight blinds the scanner.*) | Hint after two bloopers at the street | its words | ~4.1 s |
| `vo_maya_hint_rope` | MAYA | Banner on the beam, my spot on the guard. Or Vex's cart pulls him away. | Hint after two bloopers at the rope | its words | ~6.1 s |
| `vo_maya_searchlight` | MAYA | Searchlight's on the scanner. Walk him through. | SEARCHLIGHT fires | `@fire_SEARCHLIGHT` | ~3.0 s |
| `vo_maya_spot` | MAYA | Spot's on him. He can't see a thing. | FOLLOW SPOT fires | `@fire_FOLLOW SPOT` | ~3.4 s |
| `vo_maya_spot_2` | MAYA | Got him. Go. | FOLLOW SPOT fires (second and later) | `@fire_FOLLOW SPOT` | ~1.5 s |
| `vo_maya_banner` | MAYA | Banner's down. The beam's behind it. | BANNER fires | `@fire_BANNER` | ~2.6 s |
| `vo_maya_pyro` | MAYA | Pyro! Eyes shut! | PYRO BOARD fires | its words | ~1.5 s |
| `vo_vex_hint_desk` | VEX | My desk blinds the whole room for two bars. Then lift it. | Hint after two bloopers on the dais | its words | ~4.9 s |
| `vo_vex_hint_winch` | VEX | Winch first: the curtain hides him from the guards and opens the trap. Then Maya's pyro. Then run. | Hint after two bloopers at the case | its words | ~7.2 s |
| `vo_vex_cart` | VEX | Champagne cart, rolling. Mind the glasses. | CHAMPAGNE CART fires | `@fire_CHAMPAGNE CART` | ~2.6 s |
| `vo_vex_daiscart` | VEX | Dais cart. That'll turn some heads. | DAIS CART fires | `@fire_DAIS CART` | ~2.6 s |
| `vo_vex_desk` | VEX | Sound desk. Sorry, everyone. | SOUND DESK fires (the feedback) | `@fire_SOUND DESK` | ~1.8 s |
| `vo_vex_winch` | VEX | Curtain coming down. The trap is yours. | CURTAIN WINCH fires | its words | ~3.0 s |
| `vo_vance_speech_1` | VANCE | Ladies and gentlemen: the Director's Cut. | Vance's speech, first break | its words | ~2.6 s |
| `vo_vance_speech_2` | VANCE | Lot 41 has never been screened. One reel, one print, and tonight it finds a home. (said: *Lot forty-one has never been screened. One reel, one print, and tonight it finds a home.*) | Vance's speech, second break | variant of: Ladies and gentlemen: the Director's Cut. | ~6.5 s |
| `vo_vance_speech_3` | VANCE | Shot in a single take in 1953, and locked in a vault ever since. (said: *Shot in a single take in nineteen fifty-three, and locked in a vault ever since.*) | Vance's speech, third break | variant of: Ladies and gentlemen: the Director's Cut. | ~6.1 s |
| `vo_vance_speech_4` | VANCE | The director burned every other copy. This is the cut he kept. | Vance's speech, fourth break | variant of: Ladies and gentlemen: the Director's Cut. | ~4.9 s |
| `vo_vance_speech_5` | VANCE | The bidding opens at two million. Do I hear two million? | Vance's speech, fifth break (then from the top) | variant of: Ladies and gentlemen: the Director's Cut. | ~4.5 s |
| `vo_vance_security` | VANCE | Security! | Vance sees him | its words | ~0.7 s |
| `vo_vance_alarm` | VANCE | The reel! Seal the hall! | The case opened: the alarm | its words | ~2.2 s |
| `vo_chief_nomatch` | CHIEF | No match. Hold there, sir. | The scanner sees him | its words | ~2.2 s |
| `vo_chief_light` | CHIEF | Who turned that light on?! | The scanner blinded | its words | ~2.2 s |
| `vo_chief_carpet` | CHIEF | You. That carpet is for invited guests. | The chief sees him on the lobby carpet | its words | ~3.0 s |
| `vo_doorman_invite` | DOORMAN | Invitation, sir? | The doorman sees him on the red carpet | its words | ~1.1 s |
| `vo_doorman_invite_2` | DOORMAN | Guests only on the carpet, sir. | The doorman, second and later | variant of: Invitation, sir? | ~2.6 s |
| `vo_guard_aisle` | GUARD | Sir? That aisle is closed. | The guard sees him | its words | ~2.2 s |
| `vo_guard_aisle_2` | GUARD | Back to your seat, sir. | The guard, second and later | variant of: Sir? That aisle is closed. | ~2.2 s |
| `vo_bodyguard_podium` | BODYGUARD | Step away from the podium. | The bodyguard sees him | its words | ~2.2 s |
| `vo_broadcast_who` | BROADCAST | Who is the gentleman in white? | The drone sees him | its words | ~2.6 s |
| `vo_broadcast_who_2` | BROADCAST | Camera one, find me that gentleman in white. | The drone, second and later | variant of: Who is the gentleman in white? | ~3.4 s |
| `vo_broadcast_chase` | BROADCAST | Stay on the man in white! | The alarm: the drone follows him | its words | ~2.6 s |
| `vo_security_nobody` | SECURITY | Nobody leaves! | The guards come in at the alarm | its words | ~1.1 s |
| `vo_security_there` | SECURITY | There! The man in white! | A guard sees him in the finale | its words | ~2.2 s |
| `vo_security_there_2` | SECURITY | Stop that man! | The guards, second and later | variant of: There! The man in white! | ~1.5 s |

### Sound effects

| File | Event | Length | Loop | Prompt |
|---|---|---|---|---|
| `sfx_switcher_click` | Every cut, on the press frame | 0.5 s |  | A single crisp click of a vintage broadcast video switcher button, dry, close, very short |
| `sfx_clapper` | A take starts, PRINT IT | 0.6 s |  | A single film clapperboard clap, sharp wooden clack, dry studio, very short |
| `sfx_film_burn` | A blooper: the film burns | 1.5 s |  | Film projector reel jamming and the celluloid burning: crackle and flutter, short |
| `sfx_buzzer` | A blooper: the red slate | 0.8 s |  | Short harsh game show wrong-answer buzzer, one blast |
| `sfx_tape_stop` | A blooper, the clock running out: the music stops | 1.0 s |  | Tape stop effect: a jazz band recording slowing down and grinding to a halt, pitch dropping |
| `sfx_rewind` | After a blooper, through the reload | 1.2 s |  | Fast analog tape rewind whirr with squeaky high-pitched chatter, short |
| `sfx_lamp_strike` | SEARCHLIGHT, FOLLOW SPOT fire | 1.0 s |  | A big theatre follow spot lamp switching on: heavy metal shutter clunk and electric arc hum |
| `sfx_banner_drop` | BANNER fires | 1.5 s |  | A large heavy fabric banner unfurling and dropping from a height, cloth whoosh and a soft flap |
| `sfx_glass_smash` | CHAMPAGNE CART, DAIS CART fire | 1.8 s |  | A serving cart of champagne glasses rolling and crashing over, many glasses shattering |
| `sfx_feedback` | SOUND DESK fires: the band is knocked out | 1.8 s |  | Loud PA microphone feedback squeal in a big hall, piercing, rising, then cut off |
| `sfx_winch_release` | CURTAIN WINCH fires | 1.5 s |  | Theatre counterweight rope lock released: a metal clank and a rope running fast through a pulley |
| `sfx_thud` | The curtain or a shutter lands | 0.8 s |  | A heavy stage curtain landing on a wooden stage: deep soft thud with dust |
| `sfx_pyro` | PYRO BOARD fires | 1.8 s |  | Stage pyrotechnic gerbs firing: bright sparks fountain hiss and crackle with a whoomp |
| `sfx_alarm` | The case opens: the alarm | 4.0 s |  | A loud old-fashioned electric alarm bell ringing in a marble auction hall |
| `sfx_shutters` | The alarm: the shutters fall | 2.0 s |  | Heavy steel security shutters rattling down fast and slamming shut |
| `sfx_chime` | Keycard lifted, a checkpoint, an arrival | 0.8 s |  | A soft bright two-note vibraphone chime, short and pleasant |
| `sfx_heartbeat` | Loop: a watcher holds him | 2.0 s | yes | Tense heartbeat, two heavy thumps per second, close and muffled, seamless loop |
| `sfx_ringing` | Loop: composure falling | 4.0 s | yes | High-pitched tinnitus ear ringing tone, steady and thin, seamless loop |

### The band and the score

| File | Event | Shipped length | Source | Prompt |
|---|---|---|---|---|
| `mus_bed` | The band's bed, looped from the take's start; the clock follows it | 16 bars (32 s) | ElevenLabs Music, 40 s generated | Noir jazz quintet, exactly 120 BPM in 4/4, steady medium swing from the first beat: upright bass walking in quarter notes, brushed snare and ride cymbal, piano comping, muted trumpet and tenor saxophone trading short smoky phrases. A 1950s heist film, a black-tie auction hall at night, sly and understated. No intro, no ending, no tempo changes, no breaks. |
| `mus_heist` | The alarm: crashes in at the same tempo, its last bar ends on the deadline | 10 bars (20 s) | ElevenLabs Music, 24 s generated | Urgent noir jazz heist chase, exactly 120 BPM in 4/4, the same quintet at full tilt from the first beat: driving walking bass, hard-swinging drums with crashes, stabbing brass hits, tense piano. Builds for twenty seconds to a final big brass hit. No tempo changes. |
| `mus_sting` | PRINT IT: the escape | one-shot, trimmed | ElevenLabs Music, 4 s generated | A short triumphant noir jazz sting: the quintet's brass fanfare and a final ringing cymbal, three seconds. |
| `band_bass` | Warning, a bar before the guard turns | one-shot, trimmed | ElevenLabs Music, 10 s generated | Solo upright jazz bass, nothing else: no drums, no piano, no horns. Dry studio, exactly 120 BPM in 4/4. A sly, ominous noir figure: four low plucked quarter notes, one on each beat, walking down, then a bar of rest; the same figure again and again. |
| `band_trumpet` | Warning, a bar before the drone passes | one-shot, trimmed | ElevenLabs SFX, 1.5 s generated | Two short staccato notes on a muted jazz trumpet, falling minor third, bah-dah, then silence, dry studio, solo |
| `band_drumroll` | Warning, a bar before Vance speaks | one-shot, trimmed | ElevenLabs SFX, 2.2 s generated | Jazz snare drum roll, one bar long, crescendo from soft to loud, ending on a short cymbal hit, dry studio |
| `band_hihat` | The beam drops (one beat in every 4 bars) | one-shot, trimmed | ElevenLabs SFX, 0.8 s generated | A single open hi-hat cymbal splash on a jazz drum kit, dry, short |
| `band_bass_2` | Warning, a bar before the guard turns (variant) | one-shot, trimmed | ElevenLabs Music, 10 s generated | Solo upright jazz bass in G minor, nothing else: no drums, no piano, no horns. Dry studio, exactly 120 BPM in 4/4. A sly noir figure: four low plucked quarter notes walking down G, F, E-flat, D, one on each beat, then a bar of rest; the same figure again and again. |
| `band_bass_3` | Warning, a bar before the guard turns (variant) | one-shot, trimmed | ElevenLabs Music, 10 s generated | Solo upright jazz bass in G minor, nothing else: no drums, no piano, no horns. Dry studio, exactly 120 BPM in 4/4. A creeping noir figure: four low plucked quarter notes, D, E-flat, F-sharp, G, one on each beat, then a bar of rest; the same figure again and again. |
| `band_trumpet_2` | Warning, a bar before the drone passes (variant) | one-shot, trimmed | ElevenLabs SFX, 1.5 s generated | Two short staccato notes on a harmon-muted jazz trumpet, G falling to D, bah-dah, soft, then silence, dry studio, solo |
| `band_trumpet_3` | Warning, a bar before the drone passes (variant) | one-shot, trimmed | ElevenLabs SFX, 1.5 s generated | Three short soft staccato notes on a muted jazz trumpet, B-flat, A, G, swung, then silence, dry studio, solo |
| `band_drumroll_2` | Warning, a bar before Vance speaks (variant) | one-shot, trimmed | ElevenLabs SFX, 2.2 s generated | Jazz snare drum roll with wire brushes, one bar long, swelling from soft to loud, ending on a short cymbal choke, dry studio |
| `band_drumroll_3` | Warning, a bar before Vance speaks (variant) | one-shot, trimmed | ElevenLabs SFX, 2.2 s generated | Jazz snare drum press roll with sticks, one bar long, crescendo from soft to medium, ending on a single rim click, dry studio |
| `band_hihat_2` | The beam drops, at the beam (variant) | one-shot, trimmed | ElevenLabs SFX, 0.8 s generated | A single soft hi-hat chick opening into a short splash on a jazz drum kit, quiet, dry, short |
| `band_hihat_3` | The beam drops, at the beam (variant) | one-shot, trimmed | ElevenLabs SFX, 0.8 s generated | A single quiet hi-hat foot splash, a soft metallic chick with a brief shimmer, jazz drum kit, dry, short |

### Loudness

| Prefix | Integrated LUFS |
|---|---|
| `vo_` | -16.0 |
| `sfx_` | -18.0 |
| `mus_bed` | -21.0 |
| `mus_heist` | -18.0 |
| `mus_sting` | -15.0 |
| `band_` | -17.0 |

Peaks at most -1.0 dBFS.

<!-- tables: end -->
