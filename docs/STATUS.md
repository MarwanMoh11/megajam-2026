# Status

Last updated: 2026-11-14, about 22:40 Cairo. **Gate 1 is passed:** Marwan played the rework and the street-start
build on Nov 13 and said to start step 2. **Steps 1 to 4 are built** (Nov 13-14), without the parts written for
fixed cameras and hard cuts: a whole take plays from the street to PRINT IT, with the look (see "Step 4 as built",
"Step 3 as built" and "Step 2 as built" below). On Nov 14 Marwan moved step 4 (the look) in front of gate 2, so his
first full take is played on the build with the look. Next: **gate 2**, Marwan plays a full take cold. Theme:
**Director's Cut**.

## Gate 1 answer, Nov 13 about 18:15: Marwan changed three rules

Marwan asked for three changes after the test box. **Where this section differs from the text below, this section
wins.** The rest of the spec stands until Marwan has played the rework (gate 1 again).

1. **You walk the characters yourself.** Third person, WASD and mouse-look, a slow weighted walk (Uncharted is the
   reference). No clicking marks to cue. This replaces rule 3 and lifts "WASD and mouse-look" from the kill list.
2. **Cuts are flights.** 1, 2, 3 switch to Julian, Maya or Vex with a camera flight up, across and down (GTA V's
   switch is the reference). This replaces rule 1's hard cut and fixed cameras, and lifts "camera flights between
   characters" from the kill list. Still no wheel and no slow motion.
3. **The set is a large auction building,** not one 30 x 20 m hall: an entrance lobby, the great hall, a catwalk and
   backstage. This lifts "a separate lobby" and "more than one hall" from the kill list. Still one level.

**Second note, Nov 13 about 18:50:** a bigger building with a second floor and fire exits, "like a Hitman level",
scoped to 15 minutes at most. Read as: a full run of the level takes at most 15 minutes, so it stays one compact
venue. Built: about 85 x 62 m on two floors. West, a two-storey lobby with an atrium and a grand stair, cloakroom,
restrooms, an office, the security room, and a fire stair at each end. Centre, the great hall with a second-floor
gallery, walkway and bridge. North, the preview gallery and stock room under the director's office, projection room
and archive. South, the backstage corridor, kitchen, prop store and loading dock. East, the vault. Everything west
of the rope connects and all three characters can walk it; the only way to the dais is the gap and the aisle. The
new rooms are empty of rules: one encounter (the guard and the beam) is all that is built.

**Third note, Nov 13 about 22:30, after Marwan played the rework:** "shouldnt I start outside of the building not
inside ? the line is outside", and "add more props placeholders to make the place feel alive". Built:
- **The take starts on the street.** Julian stands at the foot of a red carpet in front of the open front doors,
  beside the queue, and walks in through the lobby. A scanner arch stands at the doors as a placeholder for
  Scene 1; it has no rule yet.
- **The hall mark is a checkpoint** (rule 9, the smallest form): once Julian comes within 12 m of it, a blooper
  starts the next take there ("AGAIN FROM THE HALL"). R goes back to the street. The clock still restarts at bar 1.
- **Placeholder props and people in every room and on the street.** People are a cylinder and a sphere. None is a
  watcher, none carries a rule, and none blocks a sight line, the aim or a camera.

**Filled in by Claude, the smallest that makes those three work (Marwan can overrule any of them):**
- You move only the character you are on. The other two stand where you left them. The take never stops: watchers,
  devices and the clock keep running during a flight and while you are on someone else.
- Each character has a follow camera, and that is "their camera": A follows Julian, B Maya, C Vex.
- **Use (E or left click):** Maya and Vex fire a device by standing within its reach and pressing Use. The follow
  spot: stand at the lamp, look at a watcher (it highlights), press Use. Julian has nothing to use.
- **Marks** stay as places. Standing still on one is MARK (+15 a second), and the goal mark clears the test. They
  are no longer clicked, and there is no path or arrival count.
- A flight takes under 2 s (a tunable). Presses during a flight are dropped with a flash.
- Everyone walks at 250 cm/s and nobody runs.
- Effects, composure, SEEN, the beam, the red tape, bloopers and the bar clock are unchanged.
- The characters are the engine's template mannequins, which "Look" already names; they come in now because the
  walk needs their animation.

**Not rethought yet, waiting for the next gate:** good cuts (rule 8), the shot list and the film and making-of split
(rule 1), the monitor strip, "a crew cut takes about 2 s" (rule 4), The Long Take, the replay "cut where you cut"
(rule 11), and the 1:45 clean run in a larger building.

## Step 6, audio: phase 1 as built, Nov 15 (outside the editor, branch `step6-audio`)

Built in a parallel session outside the editor; nothing is imported yet (phase 2, when the editor is free). The
script is `docs/AUDIO_SCRIPT.md`; the tools are `tools/audio/`; the files are in `C:\Jam\Audio`.

**Filled in by the orchestrator (Marwan can overrule any of it):**
- The band is a noir jazz quintet at exactly 120 BPM, 4/4, from ElevenLabs Music (Starter includes it, with
  commercial use). Stems are not used: the band is a looping bed (16 bars) plus one-shot warning phrases (the upright
  bass for the guard, the muted trumpet for the drone, a drum roll for Vance's speech, the hi-hat splash on the
  beam's drop), the heist cue (10 bars, its last bar the deadline), the sting, and a tape-stop on failure. Every
  generated loop is measured for tempo and cut or resampled to exactly 120 BPM and a whole number of bars.
- The feedback knocks the bed out for its 2 bars and it comes back on the next bar line, in phase. Voice ducks the
  bed, never the warning phrases. There is no click track (the metronome is gone, Nov 15); the engine stand-ins go
  once the bed is imported.
- **The bar clock follows the music.** The bed starts at the clock's place in its loop (bar N plays loop bar
  (N - 1) mod 16), so a blooper, a checkpoint retry or R restarts the music with the clock wherever the clock
  restarts. Each frame the clock steers toward the music's reported playback position (at most 10% fast or slow); a
  gap of more than half a beat seeks the music back onto the clock instead (after four in a take it stops following).
- Every press is still answered in its frame: the cut's switcher click plays on the press frame.

**Filled in by Claude (phase 1):**
- **Voices play when their subtitle comes on screen** in the HUD's queue, so voice and words never drift apart, and
  a voiced subtitle holds at least as long as its voice. The table that ties words to files is generated from
  `tools/audio/manifest.py`; existing lines are matched by their words, so no level data changed.
- **Variants:** Vance's speech cue in the level is one line, flashed on every band break; the table gives him five
  speeches that play in turn, each with its own subtitle (the first is the existing words). The doorman, the guard,
  the drone's producer, the alarm guards and the follow spot have a second line for repeats.
- **New lines:** Julian's six quips on the scene triggers (through the doors, onto the floor, past the rope, the
  keycard, the reel, The Bow), and Maya's and Vex's lines for the devices that had none (searchlight, follow spot,
  banner, both carts, sound desk). They come from the table by event, so the level needs no new data.
- The band's break bar drops the bed to 20% rather than silence; an arriving watcher's solo bar takes it out.
- The alarm rings a bell, stops the bed dead and rolls the shutters; the heist cue starts on its own first bar
  (the case's bar) or at once from the matching point.
- Camera C (Vex, backstage) hears the band low-passed (900 Hz). Composure: a heartbeat while a watcher holds him,
  a ringing that rises below 60 composure.
- Devices have a `FireSound` (lamp strike, banner drop, glass smash, winch release); the pyro's crackle, the desk's
  feedback, the clapper, the chime, the thud and the sting keep their existing calls on the clock, which now play
  the imported sound or, if it is missing, the old synthesised one.
- Earpiece lines (Maya, Vex, the broadcast producer) are band-passed like a radio.
- **Designed voices (Marwan picked from two or three ElevenLabs Voice Design candidates each):** the chief, Vance,
  the broadcast producer and the alarm guards. Vance's designed voice is slow and theatrical, so it reads 10% fast
  with steadier settings, and any pause over 0.6 s inside a line is cut to 0.4 s (all voices): his longest speech is
  7.45 s, inside his 4 bars at the mic. The doorman too (candidate 1 of 2).
- **Warning phrases retaken:** the first bass and trumpet takes were single held notes. The bass is now a one-bar
  walking figure (four notes on the beats and a pickup, from a 10 s Music take); the trumpet is three short muted
  notes from the sound-effects endpoint (the music model only sustains it).
- **Nothing on screen keeps the beat (Marwan, Nov 15: "like a stealth hitman game, the band as warnings").** The
  bar clock and its beat and bar events still run every timing (arrivals, the beam's drop, the alarm's length) and
  the band still warns a bar ahead, but nothing pulses or blinks: the watchers' frames are steady red (steady
  emissive white at the alarm, `AlarmEmissive` 2.5), an arriving watcher's tally is lit steady while its warning is
  up, the beam's frame, the sound desk (steady near-white while live), the danger burn at the picture's edges
  (steady at the composure's level), the danger glyphs and the title card's keycap are all steady. The HUD's beat
  squares, BAR number and BREAK are gone: it shows the take's running time (TIME m:ss). Counts are in seconds
  (a beat is 0.5 s, rounded up): "GUARD arrives in 4 s", "a 16-second alarm", the alarm as a 16 s countdown with
  a draining bar (tenths in the last 2 s), the blooper reel's "TAKE 3  AT 0:24". The metronome click is removed.
  Left in place: the clock's `GetPulse` (only PyroDevice's flare reads it; step 5 removes that) and Vex's hint
  "...for two bars" (its voice line needs a retake to say "four seconds": waiting on Marwan's go).
- Copy note: Vex's existing hint "...Then Maya's pyro. Then run." breaks the copy deck's "nobody runs"; it is voiced
  as written because the game flashes those words.

**Only Marwan can judge (step 6):** the voices picked (`docs/AUDIO_SCRIPT.md`, Voices), the bed loop and its seam,
the mix levels (bed 0.8, duck 45%, break 20%), whether the warning phrases cut through the bed and sit in its
key (`C:\Jam\Audio\review\band_warnings_over_bed.wav`), Julian's quips.

## Step 4 as built, Nov 14 (the look)

Step 4 is the look: the film grade, the overlay, the HUD, every word on screen, and the set's materials, dressing,
lights and tints. On Nov 14 Marwan moved it in front of gate 2 (his words: "start with the look", "I want to test
with step 4 the look done its much better to get a feel", and "make sure to use /design skill and actually put
thought into the design like my previous game luxel"), so his first full take is played on the build with the look.
The design is `docs/ART_STYLE.md` §15, written before the build with the design plugins' skills (design-system,
frontend-design, ux-copy, accessibility-review); this section says what was built from it and where the build
differs, and does not repeat it. Two design critiques were then run on real screenshots, one of the HUD and one of
the world picture, and each was followed by a fix pass. Built on the same terms as steps 2 and 3: nothing written for
fixed cameras, hard cuts or stills. **Everything here that the spec and §15 did not settle was filled in by Claude:
§15.12 has the long list, and the ones a player would notice are marked *Filled in* below. Marwan can overrule any
of it.** Commits: `1af9f34` (the design), `c50b86b` (pass 1: grade, stencil, overlay), `e639b77` (pass 2: HUD,
stamps, cards, fonts, copy), `1f97c3a` (pass 3: the set), `4870bbc` (the HUD's fix). The world's fix is in the files
and is being built and checked in the final run; where this section names it, it describes the plan.

**The film and the making-of.**
- Julian's camera is the film: an Ink-to-Paper monochrome print where only red (danger) and gold (Julian) keep their
  colour. Everything that carries meaning is drawn in an unlit signal colour and tagged with a custom-depth stencil
  (a per-object number a post-process material can read): 1 red, 2 gold, 3 the alarm's white. One post-process
  material, `MI_FilmGrade` (built from text by `tools/build_look.py`), maps a tagged pixel onto its colour token and
  grades the rest to monochrome (contrast 0.6, vignette 0.30). *Filled in:* a tagged pixel keeps its colour only
  where the tagged surface is the one seen, so a stanchion in front of a red frame stays grey.
- Maya's and Vex's cameras are the making-of: steel-blue video at 90% strength, no vignette, and still no hue but
  red and gold. The picture swaps on the frame of the cut press (to the flight's target, not where the camera is).
- Two switches on the director: `bMakingOfSplit` off makes all three cameras film (§15's `bAllFilm`), and `bFilmLook`
  off is the master switch, the picture before step 4. The step 3 saturation stand-in is gone: `FilmSaturation` and
  `AlarmSaturation` are no longer on the director.

**The alarm.** When the finale goes live the picture bleeds to full colour over `BleedSeconds` (2 s, on a
smoothstep), and the live frames turn white (emissive x4), full on the beat and down to 35% within it. Tally lights
and tape stay red. A blooper snaps the picture back. *Filled in:* the grade's alarm saturation `AlarmSat` is 1.0, not
§15.12's 1.15, from the world fix: at 1.15 under the warm keys the dais went amber (14% of the frame in the gold hue),
so gold stopped meaning the way out. The warm keys go whiter and the reel's key loses its gold gel with it.

**The letterbox and the overlay** (drawn on Canvas, the HUD's 2D drawing layer, by `TakeFilmOverlay`). The same
2.39:1 ink bars on all three cameras, so a cut changes only the picture. On the film: grain, flicker, dust and
scratches at 24 fps, louder after a cut and on cards; at each cut a paper flash and a frame line rolling through
(the splice). On every camera: a danger burn at the picture's edges once composure is under 50, beating with the
band. Before the blooper slate, a burn and a paper flash; at PRINT IT, an ink frame. On the crew's cameras a
viewfinder instead of film: corner marks, scan lines, and a reticle that turns gold while Use is up. *Filled in:*
grain 0.07 in play (§8 said 0.10); the making-of as a viewfinder, not film; the burn on every camera.

**The HUD in the bars** (`TakeHUD`, drawn with the kit in `TakeHudKit`). The bars hold every lane and nothing moves
between cameras. Top: the slate strip (scene, name, take); the key tabs, the active one filled, with device uses as
pips under MAYA and VEX and a line that grows during a flight; the bar clock, or at the alarm ALARM with the bars
left and eight red squares; the goal line, with a rule that draws out when it changes. Bottom: the Use prompt, or
for a dead press why not (the pressed tab shakes); Julian's mark action with its gold bar, and the composure line
(SEEN square, MARK T); the controls row. *Filled in:* top bar for where and what next, bottom bar for hands and
Julian; flashes take the goal line's place; the REC dot, the timecode and "CUT -> MAYA" are dropped. **The HUD's
fix:** under Windows display scaling the game gets a canvas smaller than its window, and the HUD drew 1.2x, so the
controls row and the grade stamp could leave the screen. The HUD now has its own scale (`HudScale`, floor 0.6),
bars at least 138 px tall (an ultra-wide screen crops the picture instead), and bottom rows that lift clear of the
edge. The controls show once (Enter lights the bottom row).

**Stamps and cards.** One family: ink plates, paper type, inset rules. Stamps land at 1.30 scale with a tilt (X2,
CLEARED, DELETED SCENE RESTORED, the grades: ROUGH CUT has an outer border only, DIRECTOR'S CUT is gold with an inner
rule and diamonds). Slates for the opening, the blooper and PRINT IT; cards for the title card, the stats and the
blooper reel. Three fonts under the SIL Open Font License, each in one role: Limelight, IM Fell English (roman and
italic) and Josefin Sans, imported as font faces in `/Game/Take/Fonts` by `tools/build_fonts.py`. *Filled in:* the
slate is the one bold piece; the opening slate sits in the picture's lower left and Julian can already walk.

**The label rule.** On the crew's cameras, watchers and devices carry camera-tape labels (`bone` tape, `ink` text):
at most four plus the one being aimed at, by priority, stacked so none overlap, never in the bars, 48 px in from the
screen's edge, a device's uses as pips under its name. On the film camera, only countdown tags (at most three).
*Filled in:* this replaces step 2's "labels dimmer on Julian's camera": rule 1 (labels only on set) wins, except the
countdown tags, because the look must not hide a rule. Since the HUD's fix there is one target label at a time, and
Maya's platform view shows five.

**The copy** (§15.7: 93 patches in pass 2, 22 in the fix). One name per thing (CHAMPAGNE CART, SOUND DESK, PYRO
BOARD, CURTAIN WINCH); the first checkpoint is THE STREET; nobody runs; R is "New take". The goal line says what to
achieve, never repeats the Use prompt, and fits one line; the bars use sentence case with no final full stop. The
crew's intros are one line each in the director's voice ("Maya, our gaffer, up on the lights. Crew pass: nobody
watches her."). *Filled in:* the device names, "terrace" for "canopy", the reworded blooper reasons, the intros.

**Danger against Julian without hue** (§15.8, for colour-blind players). Red is broken and moving, gold is solid and
still: a watcher's frame lines and the beam are dashed from the eye, a frame's fill is banded outwards from the
watcher, red tape is hatched at 45 degrees, tally lights and the HUD's SEEN and alarm glyphs are squares, and red
pulses. Julian's marks are solid gold T plates that never move, and the disc under a mark fills with an action's
progress instead of pulsing. The world fix adds: frame segments within 2 to 3 m of the lens fade out (near the lens
they crossed Julian as thick slabs); mark plates 1.4x larger (2x at the scanner and the registration desk, where 1.4x was still a sliver), visual only (the street's and lobby's marks were
slivers at play distance; the trigger and timings do not move); the tally's off state a dark red square (it read
violet); the dais footlights as two low glow strips instead of 50 boxes, which read as a dotted line.

**The set: materials and dressing** (`tools/dress_auction.py`, run at the end of `tools/build_auction.py` or alone).
Three master materials in `/Game/Take/Dress` (lit set, unlit practicals, and `M_SignalPattern`, the signal material
with dashes, bands and hatching) and 40 instances. Every set mesh (1,391) is re-surfaced by what it was built as, its
label and its zone: the fronts (street, lobby, hall, dais, the north rooms) as an old auction house, the backs
(backstage and the east) as the raw back of a film set. 312 small dressing pieces with no collision: kerb, queue
stanchions, press wall, bay trees, wainscot and cornice, chandelier, pilasters with sconces, side-aisle runners, the
dais lip and footlights, the pyro charges, the flats' battens, braces and weights, cables, work tubes, road cases,
spike marks, catwalk tape, truss and lamp housings. None stands within 1.2 m of a mark, watcher, device, crew post or
character. Red and gold left everything that carries no meaning: picture frames, stanchions, bottles, instruments,
banners, the rope, the curtain (velvet now), extinguishers, cars, the device rings. *Filled in:* crew kit is light
steel, not work blue; the pyro's flare is warm white, not gold.

**Lighting.** Retuned per zone, with a new `Back_Dais` rim behind Vance and the bodyguard. *Filled in, beyond
§15.9:* Julian carries his own key light (`Dress_Julian_Key`) on a lighting channel (a light only touches meshes on
its channel) that only his mesh is on, so it lights him alone and he stays the lightest thing in frame; two house
lights over the seats (`Dress_House_Fill`), because the night skylight fills nothing (it captures a black sky). The
world fix, as planned: the key hangs off the film camera's boom instead of off Julian, so it lights the side the
camera sees (it falls back to Julian if that does not survive into play); the dais deck and the `Back_Dais` rim go
lighter so Vance, the bodyguard and the mark behind him read; a cool wash up the street's facade
(`Dress_Facade_Wash`) so the opening frame is not half black; the warm keys whiter (`WARM` 1.0, 0.90, 0.80); the
banner and the aisle cases at the rope darker, so nothing but Julian is above 160; LOT 41 on the dais legible.

**Tints.** Value carries the cast: Julian alone at the top (a light warm grey), the crew and guests in the middle,
the band dark, the heavies (guards, the bodyguard, Vance, the drone) near black through `BodyTint`. They are still
the template mannequins (step 5).

**What changed from the spec and §15, and why.**
- The grade runs before TSR (Temporal Super Resolution, Unreal's anti-aliasing), not after tonemapping as §15.3
  says: after TSR the stencil is jittered against the resolved picture and the kept edges crawl. The volume's own
  tone curve is off and the material applies a filmic curve itself.
- Its numbers sit on the volume's material instance, not in a parameter collection: `Bleed` is 1 minus §15's
  `GradeStrength`, `MakingOf` is its `GradeMode`, and `bMakingOfSplit` is the opposite of `bAllFilm`.
- Grain after a cut is capped at 0.14 (`GrainMax`); §15.4 says 0.16.
- After the grade the engine brightens the picture by a constant (linear x1.65). It was found with a test pattern
  the material draws (`DebugFlat`) and divided out (`OutputScale` 0.6); it is measured, not explained.
- Gold cannot glow: the grade clamps every kept pixel at its token, so the world critique's brighter gold was
  declined and the mark plates grew instead.
- Trim is pewter, never the gilt or brass an auction house would have: gold is Julian's.
- The device rings stay on the lit material: the signal material on an instanced mesh (many copies drawn as one)
  crashed the GPU at every editor start, for want of a usage flag.
- `AlarmSat` 1.0 for 1.15, Julian's own key and the house fills (above).

**Held or left out.**
- Still waiting for gate 2: the shot list and fixed angles, the monitor strip, stills, good cuts and their replay,
  the edit suite, the contact sheet, the recorder, The Cold Open, The Long Take, The Insert. The film and making-of
  split that steps 2 and 3 left for gate 2 is built now as a look; the rules written around it still wait.
- Step 5, characters and props: tinted template mannequins only; no costume props, instruments, bob on the beat,
  bow or particle effects.
- Step 6, audio: the clicks, the warning pitches, the alarm tone, the sting and the clapper are still stand-ins.
- Declined from the HUD critique: #3 (the stats page and the reel disagree) is not a bug: the two shots came from two
  different test runs, and both pages read the same results. #5's wording: the intros are one line each, but in the
  director's voice, not the critic's.
- Not in the HUD's fix: #6 (a tape label shows through the subtitle plate), #8 (DELETED SCENES drawn two ways,
  "0 of 1" reading as "o of ı"), #9 (the blooper reel's scroll masks run past the card's border). The HUD's builder
  reports all three were fixed before pass 2 was committed, after the shots the critic saw were taken. Not checked
  again on a packaged screenshot.
- Declined from the world critique: #2's brighter gold (it cannot glow, above); moving the queue-lane stanchion that
  cuts the street's mark in two (it collides). #7 needed nothing: tape never takes the alarm's white, and the white
  "tape" in the shot was a frame's banded fill.

**Proof.** Proven in the earlier passes: each pass ran the four scripted play-in-editor tests to their ends (the
whole take, ROUGH CUT by the trap; Scene 3 by the trap; the keycard blooper, the clock running out, and the side exit
with The Bow for DIRECTOR'S CUT; the carpet and beam bloopers with both routes), logs `C:\Jam\Builds\step4_t_*.log`,
`step4p2_t_*.log`, `step4p3_t_*.log`. Pass 1's package with real keys: a whole take to PRINT IT in 3:02, 17 cuts, 1
blooper, out by the trap (`docs/shots/step4_p1_*.png`), red tape (199, 47, 31) against `#D3301F` (211, 48, 31) and a
gold mark (229, 182, 73) against `#E9B949`. Pass 2's package with real keys: the title card, the queue, a cut to
Maya, a refused cut and a blooper at the scanner's tape, with the fonts and the HUD in Shipping
(`docs/shots/step4_p2_*.png`). Pass 3 and the two fix passes were first proven in the editor
(`step4_p3_pie_*.png`, `step4_fix_*.png`; in a standalone run at 1920 x 1080 on this PC's 125% display the composure
line is 800 px and the controls' lowest pixel is at y 1065, `step4_fix_scale_1080.png`) and are in the final package
below. Still editor only: the blooper's burn frames, the composure burn, and full screen at 1920 x 1080 from the
packaged exe (the packaged run was windowed at 1600 x 900). The final run:
- The four scripted tests in the normal viewport (logs `C:\Jam\Builds\step4final_t_*.log`) end as they did after
  pass 3, with the same deliberate bloopers: t_full ROUGH CUT by the trap, t_s3 ROUGH CUT, t_s3b DIRECTOR'S CUT,
  t_fail both failures and both routes.
- One run of the packaged `.exe` with real keys and mouse (`C:\Jam\Builds\shots\play_final.ps1`, windowed
  1600 x 900 on the 125% display, 3:25 on the wall clock): the title card, the whole take to PRINT IT in 3:02 on the
  band's clock, 17 cuts, 1 blooper (the beam, on purpose), out by the stage trap, ROUGH CUT, the stats. The HUD sits
  inside the window; the fonts, the dashes, the hatching and the T plates are in the Shipping picture.
- Its screenshots: `docs/shots/step4_01_title_card.png`, `02_queue_julian`, `03_scanner_red`, `04_maya_making_of`,
  `05_scene2_floor`, `06_cut_frame`, `07_blooper_slate`, `08_alarm_bleed`, `10_print_it`, `11_stats_grade`,
  `step4_thumb_315x250.png`, and `step4_after_{queue,maya,floor}.png` beside `step4_before_*`.
- **Not caught from the final package: the pyro's flash** (there is no `step4_09`; the run's frame at that moment
  was the same as the alarm's). The pyro is on screen in pass 1's packaged run (`step4_p1_09_bleed_pyro.png`, with
  the old HUD) and in the editor with the new one (`step4_p2_pie_13_pyro.png`). A retry was stopped when Marwan
  asked to play the build.
- The §15.11 checks on the final build: the last column of the first table below. The hall's floor, the heavies
  and the alarm's hue share are sampled in play-in-editor, because the Shipping exe cannot project a point.
- The frame time after the final run: the last column of the second table below.
- The zip `C:\Jam\Builds\TeamName_GameName.zip`, 326 MB, made with 7-Zip without the `.pdb` (pass 2's is kept as
  `TeamName_GameName_step4p2.zip`).

| §15.11 check | Pass | Pass 3, editor, before the world fix | Final run, package |
|---|---|---|---|
| 1 Hero: Julian's back | 200 or more | 217 | (229, 222, 208), 220: pass (package) |
| 1 Heavies: the nearest guard's torso | 45 or less | 31 | 31: pass (editor) |
| 1 Lit floor, in an aisle key | 95-130 | 111-132 | 128: pass (editor; `KEY_CD` 2400 to 2300 after the whiter keys gave 131) |
| 1 Floor in shadow, between bench rows | 45-70 | 52-83 (house fills at 1600 cd, since 1200) | 49 and 65: pass (editor) |
| 1 Black point | within 5 of the bars' (13, 12, 11) | 16-17 | 17: a point or two over (editor) |
| 2 Stencil red: the scanner's red tape | within 15 of `#D3301F` (211, 48, 31) a channel | (198, 45, 30) | a frame line (211, 48, 31): pass (package). The carpet (187, 49, 35) is darker: 24 under on red, outside the 15 |
| 2 Stencil gold: a gold mark | within 15 of `#E9B949` (233, 185, 73) a channel | (213, 169, 67) seat mark, (232, 184, 72) case | (231, 183, 71): pass (package) |
| 3 Making-of: a wall on camera 2 | B at least R + 25 | B - R 25-30 | B - R 29: pass (package) |
| 4 Thumbnail, 315 x 250 | the letterbox, Julian lightest, one red, one gold mark | the gold T hidden under Julian at the floor mark | partial: the letterbox and Julian as the lightest shape read, the red is faint, and the only gold is the composure line in the bar (no mark in that frame) |
| 6 Labels on Maya's platform | 5 or fewer, none overlapping | 6 (5 since the HUD's fix) | 5, none overlapping, none in the bars: pass (package, `step4_04_maya_making_of.png`) |
| 7 Alarm | full colour, frames white, gold hue under 6% of the frame | 14% | 3.5%: pass (editor); full colour and white dashed frames in the package (`step4_08_alarm_bleed.png`) |

| Frame / Game / Draw / GPU, ms (editor, `stat unit`) | Before step 4 | After pass 3 | After the final run |
|---|---|---|---|
| Queue (camera 1, the start) | 22.85 / 22.83 / 3.90 / 8.53 | 23.71 / 23.69 / 4.88 / 9.09 | 23.26 / 23.27 / 4.94 / 9.23 |
| Maya (camera 2, the terrace) | 22.81 / 22.78 / 4.03 / 8.75 | 23.78 / 23.78 / 5.13 / 9.23 | 23.53 / 23.50 / 5.17 / 9.45 |
| Floor (camera 1, the hall) | 22.65 / 22.66 / 4.17 / 8.96 | 23.55 / 23.56 / 5.02 / 9.54 | 23.53 / 23.50 / 5.04 / 10.23 |

The game thread's 22.8 ms is the editor and the test script; Draw and GPU are the columns step 4 moves (the
custom-depth pass and the dressing's draws).

**Only Marwan can judge (step 4):**
- The split against `bMakingOfSplit` off: does the cut to steel-blue video read as "the making-of", or as a
  different game?
- Grain, flicker and dust, the burn's strength as composure falls, the vignette.
- The bleed at the alarm and the white pulse; whether the alarm without amber still feels warm enough.
- The mood: how dark the hall and backstage are, the street's opening frame, the house lights over the seats
  against the noir pools, the footlights and sconces.
- The three fonts and their sizes; the 16 px controls row through Moonlight on the Mac.
- The words: the director's intros and lines, "Walked into the beam", the title card's two rule lines.
- Hatched tape and dashed frames: still clearly "red tape" and "camera frames"? The T marks; the red hatch's
  loudness at the rope in motion.
- The opening slate in the lower left while Julian can already walk: charming or in the way?
- Tape labels on Maya's platform, and whether the film camera is fair without them.

**Step 4 tunables, all `EditAnywhere` unless they are a script's numbers.**
- On the director (`TakeDirector`): `bFilmLook`, `bMakingOfSplit`, `BleedSeconds`, `FilmGradeMaterial`, and
  `ForceBleed` (for checks; -1 is off).
- On the HUD class (`TakeHUD`): the label rule (`bLabelsOnFilmCamera`, `MaxLabels`, `MaxCountdownTags`,
  `LabelMaxDistance`, `OffLabelMaxDistance`, `LabelMergeDistance`, `LabelMaxLifts`, `LabelLiftPx`); the timings
  (`FlashHoldSeconds`, `RefusalHoldSeconds`, `SubtitleBaseSeconds`, `SubtitlePerCharSeconds`, `SubtitleQueueMax`,
  `StampHoldFrames`, `StampTitleHoldFrames`, `GoalRuleFrames`, `StatsRowSeconds`, `GradeDelayFrames`); the words
  (`ControlsHint`, `TitleControls`, `TitlePitch`, `TitleCast`, `TitleRules`, `TitlePrompt`, `DirectorWord`,
  `SceneNames`, `EndKeys`, `EndKeysLast`). The overlay is under `Film`: a switch per effect (`bLetterbox`, `bGrain`,
  `bFlicker`, `bDust`, `bScratch`, `bSplice`, `bViewfinder`, `bBurn`, `bBlooperBurn`, `bPrintItSnap`),
  `LetterboxAspect`, the grain (`GrainOpacity` 0.07, `GrainCutOpacity`, `GrainCardOpacity`, `GrainMax` 0.14), the
  flicker (`FlickerMax` and its cut, card and low-composure versions), dust and scratches, the splice
  (`SpliceFlashOpacity`, `SpliceBandPx`, `SpliceBandFrames`), the viewfinder (`CornerArmPx`, `CornerAlpha`,
  `ScanLineAlpha`), the burn (`BurnBelow` 50, `BurnAlpha`) and the blooper burn
  (`BlooperBurnFrames`, `BlooperPaperFrames`).
- On the actors: watchers' `AlarmEmissive` (2.5, steady since Nov 15), `BodyTint`, `FramePattern`; marks'
  `PlateScale` (1.4, and 2 on the scanner's and the desk's marks; the world fix); devices' `BodyColour`, `RingReachColour`, `RingIdleColour`. The map is rebuilt
  from text, so change them in the scripts below to keep them.
- In `Reel/Source/Reel/Take/TakeStyle.h` (a recompile): the colour tokens, the stencil numbers (`StencilRed`,
  `StencilGold`, `StencilWhite`), the patterns, `LetterboxAspect`, `FilmFps`, `HudScale`, and every HUD size and
  timing (the rows `RowT1` to `RowB3Down`, `MinBar` 138, `BottomSafe`, glyphs, keycaps, stamps, slates and cards,
  `FramesBeat`, `FramesHold`).
- In `MI_FilmGrade`, through the numbers in `tools/build_look.py` (run in the editor, not in play): `ColourExposure`
  0.45, `FilmBlack`, `FilmWhite`, `FilmContrast` 0.6, `Vignette` 0.30, `MoBlack`, `MoRange`, `MoKeep` 0.9,
  `StencilSat`, `KeptGain` 1.7, `RedFloor` 0.20, `GoldFloor` 0.45, `AlarmSat` 1.0, `OutputScale` 0.6, `DebugFlat`
  (the test pattern). The director drives `Bleed` and `MakingOf`.
- At the top of `tools/dress_auction.py`: the material tables (`LIT`, `GLOW`, `SIGNAL`, `SURFACES`), `HATCH_W`,
  `KEEP_OUT`, `TAPE_MARGIN`, `FOOT_STEP`, the per-zone light retune (`RETUNE`, `WORK_HAZE`), `BACK_DAIS`,
  `HOUSE_FILL`, `JULIAN_KEY`, `FACADE_WASH`, the tints (`TINTS`, `BODY_TINTS`, `HEAVY`, `DEVICE_BODY`, `BANNER_BODY`,
  `RING_IDLE`, `CURTAIN`, `FLARE`), and the near fade (`NEAR_FADE`, `NEAR_FADE_LEN`).
- At the top of `tools/build_auction.py`: `EXPOSURE_EV`, `SKY_FILL`, `KEY_CD`, `RIM_CD`, `WARM`.

**Known issues carried forward.**
- The engine's x1.65 after the grade is measured, not explained: rerun the `DebugFlat` check if the exposure or the
  post-process volume changes.
- `M_SignalPattern` is cached per editor session: after rebuilding it, restart the editor before judging the dashes.
- A material put at runtime on an instanced mesh without its usage flag kills the GPU on the editor's first frame:
  if the editor dies on start, look for "usage flag" in `Reel.log`.
- A file-scope `using namespace` in a `.cpp` leaks into the next file of Unreal's unity build (error C4459): keep
  them inside functions.
- The 1600 x 900 floating play window runs at about 12 fps: shoot views there, run the tests in the normal
  viewport. The editor's wide viewport has almost no letterbox.
- 150% Windows scaling is untried (this PC runs 125%).
- The CLEARED stamp never fires in the built take (the keycard shows the checkpoint and SCENE 3 instead), and the
  FINAL CUT stamp has not been seen on screen.
- The blooper burn has not been caught on a packaged screenshot.
- Julian's key rides on the camera's boom in the editor. In the package it was not checked on its own (Julian
  samples 220 at the queue there); if the attachment fails it falls back to hanging off Julian.
- The lobby's floor samples 30, below the critic's 45 to 70 (Julian is still the lightest thing there).
- The pyro's flash has no screenshot from the final package (Proof, above).
- The 315 x 250 thumbnail of the queue frame shows no gold mark, only the gold composure line.
- The near fade drops frame segments within 2 to 3 m of the lens; Julian stands 3.6 m out, so frames at his feet
  stay.
- On Maya's platform the FOLLOW SPOT, VANCE SPEAKING and BODYGUARD labels stack on the red frame box.
- Only four hall pilasters fit between the paintings and the openings.
- HUD critique #6, #8 and #9 are reported fixed but not checked again on a packaged screenshot (above).

## Step 3 as built, Nov 14 (Scene 3 and the ends, for the walking game)

Built on the same terms as step 2: everything written for fixed cameras, hard cuts or stills still waits for gate 2
(the replay and its edit suite, good cuts, the contact sheet, the film and making-of split, The Cold Open, The Long
Take, The Insert). **Everything in this section that the spec did not settle was filled in by Claude with the
smallest thing that works; Marwan can overrule any of it.** Marwan's Nov 14 direction is applied the same way as in
step 2: the crew's places make sense in the story, nobody teleports, Julian has no straight clear line out, and the
immersion is text only.

**The story of the finale.** The stage has a house curtain on a counterweight winch, in the wings at the east end
of Vex's corridor. Maya's pyro board is on her lighting platform beside the follow spot: the gala's own finale
charges, standing along the front of the dais. The crew's lorry is backed up to the loading dock where Vex started.
So the free way out is the stage's own machinery: the curtain comes down, the pyro goes off, and Julian drops
through the stage trap to the lorry. The paid way out is the gold side exit past the band, to the street.

**Scene 3, The Getaway.**
- **The keycard starts Scene 3** (title, and a checkpoint: a blooper before the case puts Julian back on the dais
  mark, still holding the keycard). The gold mark at the case appears. The goal line says to post the crew first:
  Vex walks east along his corridor to the winch (about 7 s from his cart), Maya four metres along her platform to
  the pyro board. The north half of the dais is safe ground until the alarm.
- **The case:** stand on its mark for 1.2 s. Julian has the reel, **the alarm** sounds, and **the case is Scene 3's
  checkpoint**. The alarm tone replaces the band and keeps the beat. Shutters fall across the aisle and the hall's
  side doors.
- **The hold:** the rest of that bar and one more bar. Nothing sees him. *Filled in:* the spec holds "that first
  bar"; a case opened late in a bar would have left almost no hold, so it is the rest of the bar plus one.
- **Live:** the whole floor is red tape, four guards come in through the two doors in the end wall (two hold the
  doors, two walk out along the flanks), the drone leaves the aisle and follows Julian, the frames pulse white on
  the beat, the picture bleeds to full colour, and the HUD counts **8 bars**. At the end of the eighth the clock
  runs out: a blooper, "THE CLOCK RAN OUT", again from the case with the clock back on the bar the case was opened.
- **The free exit, the stage trap** (10 m from the case, round the lots table that stands in the straight line):
  Vex's **winch** drops the curtain across the front of the dais and down both its sides, for good: the guards on
  both flanks lose the dais, and the trap opens (its gold mark appears). Maya's **pyro** blinds everything on the
  stage for 6 s, the drone included once it has locked on. Julian stands on the trap's mark and is out. *Filled
  in:* the spec's "hatch under the stage" is called the stage trap on screen, because "hatch" already means the
  service hatches Vex kicks his carts through.
- **The paid exit, the gold side exit** (14 m, round the lots table and the band's PA stacks): two of the guards
  hold that door, so the follow spot cannot take both. The PA stacks are the only cover from them. The answer is
  Vex's **sound desk**, if it was saved: everything in the hall is blind. *Filled in:* neither cart reaches that
  door and the one banner hangs over the rope, so of the spec's three saved answers only the desk works here.
- **The Bow** (the one deleted scene that is built): the gold mark in the lane before the gold door. Stand on it
  while the drone is blinded: "DELETED SCENE RESTORED: THE BOW", "Keep that in!".
- *Filled in:* **the sound desk blinds for 6 s, not the spec's 4** (the band still drops out for 2 bars). A flight
  back from Vex costs 1.6 s of it, and 2.4 s was not enough to reach the door; it is the same change the arrivals
  searchlight got in step 2. It also makes the paid lift in Scene 2 easier.
- *Filled in:* an exit works even while he is seen (he is leaving anyway), except the gold door, which needs the
  two guards off him for a third of a second: that is what makes it cost the desk.
- A blooper in Scene 3 keeps the crew where the player walked them, as in Scene 2.

**The ends.**
- **The title card,** once a session, before the first take: the name (*Print It*, still the working title), the
  pitch in two lines, the controls, the goal. Enter, Space or E starts the take; nothing moves behind it.
- **PRINT IT:** a snap to black, the clapper and a sting (synthesised stand-ins), the slate, and one line for how he
  left. Then **the stats** (take, time on the band's clock with the bloopers spliced out, cuts, bloopers, the exit,
  deleted scenes) and **the grade**. Then **the blooper reel** rolling over the credits. Enter turns a page, R
  shoots a new take, ESC quits.
- *Filled in:* the blooper reel is text (take, bar, where it went again from, why, who caught him): the freeze
  frames need stills, which wait for gate 2.
- *Filled in:* **the grade.** Only The Bow can be restored in this build, so the stats count and name only the
  built scenes ("DELETED SCENES 0 / 1", "MISSED THE BOW: at the gold side exit, bow to the blinded drone").
  ROUGH CUT: out by the trap. FINAL CUT: out by the gold side exit. DIRECTOR'S CUT: every built deleted scene
  restored. When the other three are built, `bSideExitIsFinalCut` goes off and the spec's rule (2 or 3 restored) is
  what is left.
- *Filled in:* **the colour bleed.** The noir film look waits for gate 2, so there was nothing to bleed from. The
  picture now runs at 60% saturation until the alarm goes live and rises to 125% over two seconds. Red and gold
  still read before it. `FilmSaturation = 1` on the director puts the old look back.
- GOOD CUTS is not in the stats (good cuts wait for gate 2).

**Proof, Nov 14.** Scripted play-in-editor runs: the whole take from the back of the queue to PRINT IT (163.6 s on
the band's clock, 16 cuts, no bloopers, ROUGH CUT); a blooper after the keycard (back on the dais mark with the
keycard); the clock running out (again from the case, the clock back from bar 30 to bar 20, the crew still posted);
the gold side exit with the desk and The Bow (DIRECTOR'S CUT); and the step 2 tests again. One run of the packaged
`.exe` with real keys and mouse (`C:\Jam\Builds\shots\play2.ps1`, 202 s): the title card, the whole take with
one blooper on purpose at the beam, PRINT IT, the stats, the blooper reel, R. Screenshots: `docs/shots/step3_*.png`
(the editor) and `docs/shots/step3_pk_*.png` (the package).

**How the finale's time adds up** (all tunables): the hold is 2 to 4 s; then 8 bars, 16 s. The drone flies in at
700 cm/s and is over him 4 to 7 s after the alarm goes live. From then, unblinded, it burns his composure out in
3.8 s, so the drone is the real clock unless it is blinded. Winch, pyro and the cut back to Julian take about 5 s
with the crew already posted; the walk to the trap is 4.3 s; the pyro covers 4.4 s of it.

**Only Marwan can judge (step 3):**
- Whether the finale is fair cold: the prompts, the 8 bars, how fast the drone arrives, how hard the pyro's flash is.
- The 60% saturation before the alarm, and the bleed.
- The desk at 6 s, and whether the gold side exit and The Bow feel worth saving it for.
- The alarm tone, the sting and the clapper as stand-ins, and every line of text (the title card, the slate, the
  stats, the credits, the guards).
- The name on the title card and in the credits.
- The guards' frame lines show through the curtain's cloth: does that read as "they can still see him"?

**Step 3 tunables, all `EditAnywhere`.** On the director (`TakeDirector` in the level): `AlarmHoldBars`,
`FinaleBars`, `FilmSaturation`, `AlarmSaturation`, `BleedSeconds`, `EndSlateSeconds`, `EndStatsSeconds`,
`bShowTitleCard`, `bSideExitIsFinalCut`, `FinalCutScenes`, `DeletedScenes`, and every line of the alarm and the
slate. On the HUD class (`TakeHUD`): `GameTitle`, `TitleTagline`, `TitleLines`, `CreditLines`, `AlarmEdgeAlpha`,
`ReelScrollSpeed`. On the actors in the level, and as numbers at the top of `tools/build_auction.py` (the map is
rebuilt from that file, so change them there to keep them): the pyro (`PYRO_RANGE`, `PYRO_CD`, 6 s), the desk
(`DESK_SECONDS`), the winch's curtain (`CURTAIN_H`, `DropSeconds`), the drone's `ChaseSpeed`, the guards' posts and
frames (`SEC_*`, `DOOR_RANGE`, `FLANK_RANGE`), the marks' action times, the lots table, the PA stacks, the trap.

## Step 2 as built, Nov 14 (Scenes 1 and 2 for the walking game)

Marwan said to start step 2 with everything written for fixed cameras and hard cuts left out until gate 2: the shot
list, fronts and backs, the film and making-of split, the monitor strip, good cuts and their replay, stills, the
recorder, and three deleted scenes (The Cold Open, The Long Take, The Insert). What is built is the rest of the
step 2 list. **Everything in this section that the spec did not settle was filled in by Claude with the smallest
thing that works; Marwan can overrule any of it.**

**Marwan's note while step 2 was being built, Nov 14 about 01:00** (his words): "while the step 2 works I imagine
the line with the npcs is outside the venue itself and also it should make sense in the narritive the side
characters postiosn and how they control stuff also I dont want the character to just walk in a straight line it
shouldnt be that easy these are all stuff that maybe are there because its just a greybox idk", and "there should
be clever game design basically and narrative building and immersion". **What Claude made of it, the smallest form
of each inside the rules already built (Marwan can overrule any of it); where it differs from the paragraphs below,
this list wins:**
1. **The Door is played on the street.** Three lanes lead to the doors. The red carpet in the middle is for invited
   guests: it is red tape from the kerb to the doors, the doorman's frame runs straight down it ("Invitation,
   sir?"), and a rope closes it at the doors. The queue lane north of it ends at the scanner. The south lane is
   the crew's, roped off. Julian starts at the back of the queue, which has backed up along the kerb: he walks
   down it and turns into the queue lane, so his first steps bend and the straight way to the doors is the one
   with a red cone on it.
2. **One cover story.** Aurelia hired a film crew to light and broadcast its gala, and that crew is the player's.
   Maya and Vex wear crew passes, so nobody watches them. Julian is an actor playing a bidder with no invitation,
   so every watcher is something that would check him. Each character says this in one line the first time the
   player lands on them.
3. **The crew start where the job would be and walk; nobody teleports.** Maya starts on a terrace over the front
   doors at the arrivals searchlight, which looks straight down on the scanner. After The Door the player walks
   her in through the door behind her, round the lobby balcony, through the gallery door and out along a catwalk
   to a lighting platform over the benches, where the follow spot and the banner's winch are (about 20 s). Vex
   starts on the loading dock by the lorry and walks up the corridor to the champagne cart at the hatch (about
   5 s), then on to the sound desk and the dais cart. A blooper puts the crew back where the player had walked
   them, not at their starts: nobody watches the crew, so their walk is not part of what a blooper costs. Each
   Use prompt says what the hand does ("Swing the searchlight onto the SCANNER").
4. **No straight clear line for Julian.** Each long axis is somebody's sight line: the carpet is the doorman's,
   the carpet inside the lobby is the security chief's (he stands at the foot of the grand stair), the hall's
   centre aisle is the broadcast drone's, and its camera riser blocks that aisle, so Julian slips out between the
   bench rows and goes round by a side aisle. The marks are places a guest would stand: the queue, the scanner,
   the registration desk, an aisle seat, a cocktail table, the rope.
5. **Still for a judge playing cold:** the goal line names each step, The Door takes about 20 s done right, and
   every blooper restarts at a checkpoint. The arrivals searchlight blinds for 6 s, not the follow spot's 4 s:
   with 4 s the flight back left a third of a second to get through the gate.
6. **Words:** an opening slate ("AURELIA AUCTION HOUSE - LOT 41: THE DIRECTOR'S CUT", then the scene and take), a
   line from each watcher when it catches him, and lane signs in the world (QUEUE, INVITED GUESTS, PRESS, CREW
   ONLY, LIVE BROADCAST).

**Scene 1, The Door.**
- The scanner arch at the front doors is a watcher with a gate. While the scanner is live the gate is shut and a red
  bar crosses it, and its frame (red tape on the ground in front of it) sees Julian. Blinded, the gate opens and the
  bar turns gold. *Filled in:* in the click-to-cue spec Julian waited on the scanner mark by himself; now the player
  walks him, so a gate is what makes him wait. The gold mark sits 90 cm outside the tape, so standing on it is safe.
- *Filled in, the open point about Maya:* she starts on the terrace over the doors with the arrivals searchlight
  (item 3 above). The first version put her on the street behind Julian and moved her to the bridge when Scene 2
  started; Marwan's note replaced that.
- The loop it teaches, one line at a time in a goal line at the top of the screen: walk Julian to the gold mark,
  [2] cut to Maya, look at the scanner and press [E], [1] cut back, walk through. That goal line stays for the whole
  game and always names the next thing to do.

**Scene 2, The Floor.**
- Walking onto the hall floor starts the scene (a title, a checkpoint).
- **The drone** arrives on the next 4-bar line and flies down the centre aisle and back every 8 bars. Inside its red
  circle Julian is exposed unless he stands on a gold mark (rule 5). The mark at the hall's door is under it; from
  there he has to leave the aisle between the bench rows (item 4 above). It turns back short of the rope, so the
  rope encounter plays as before.
- **The guard and the beam** at the rope are live from the start of the take, not on arrival lines: they are 34 m
  from the hall's door, and after a checkpoint restart an arrival line would have let Julian slip past before they
  came on. Both old routes work (banner plus spot, cart plus spot).
- **Past the rope:** stepping into the aisle brings on **the bodyguard** and **Vance** (the next 4-bar line; the
  walk from there to the end of a lift takes longer than that). Vance's frame sits on the lift mark behind him and
  turns to the crowd while he speaks: 4 bars from every band break (bars 8, 16, 24...). The bodyguard's frame
  covers the lift mark and its near approach.
- **The lift:** stand still on the mark behind Vance, unseen, for 1 s. It clears the take for now ("Scene 3
  comes next"). Free: wait at the edge of the bodyguard's frame, spot him while Vance speaks, step in. Paid: Vex's
  **sound desk** (everything in the hall blind for 4 s, the band out for 2 bars) or the **dais cart** (Vance and the
  bodyguard lured for 6 s, an x2 stamp, +25 composure). *Filled in:* the spec's lift takes 1.5 s. With a 1.6 s
  flight back from Maya, 2.4 s of the spot's 4 s are left, and 1.5 s missed by a quarter second in the scripted
  run, so the mark is set to 1 s. If Marwan picks a 6 s blind or a 1 s flight for route 1, the lift can go back to
  1.5 s. Being seen on the mark starts the lift over.
- Vex's three devices are along the backstage corridor: the mid-floor cart (moved one hatch west so it reaches the
  guard only), the sound desk, the dais cart at a new third hatch.

**Watchers.** Every frame ends in a line trace: walls, floors and a dropped banner block it; the placeholder props
and people do not. Each frame has thin lines from its eye to its far corners, a tally light, and a label with its
state. *Filled in:* labels show on all three characters' cameras (dimmer on Julian's), because the film and
making-of split that hides them on camera A waits for gate 2.

**Checkpoints (rule 9).** The street start, the hall floor (Scene 2's start), the rope mark, and the mark past the
rope on the dais. *Filled in:* the rope mark is a fourth checkpoint, so a blooper at the rope does not cost the
walk across the floor again. A blooper puts Julian back on the checkpoint's mark with the clock at the bar saved
there, the device uses saved there, and the same watchers arrived or still to come. After two bloopers at one
checkpoint Maya or Vex gives a hint. R still starts a new take from the street. The band-break checkpoints of
rule 9 are not built.

**Warnings (rule 2).** One bar before a watcher changes, a click pitch plays: a low pluck for the guard, two mid
notes for the drone, a roll for Vance. An arriving watcher gets its pitch solo the bar before, with the metronome
out. The metronome also rests on every 8th bar (the band's break). These stand in until the audio step.

**The look (lighting only).** Night: a cool moon, a dim sky fill, haze, warm key spots down the centre aisle and on
the scanner, the stair and the podium, cool rim lights from the dais end, red at the scanner and the rope, gold on
the reel's case, cyan-white work light backstage and on the catwalk. The follow spot is a real light while it
burns. Lumen is off for the level. Exposure is fixed. The dressing pass is step 4.

**Small things filled in at the end.** Mouse smoothing is off (`bEnableMouseSmoothing=False` in
`Reel/Config/DefaultInput.ini`): the look is the raw mouse, and a scripted key-and-mouse run repeats exactly. After
SCENE 2 CLEARED the composure bar stays where the take ended. A cut key and E pressed on the same frame do the cut
and refuse the use ("Mid-cut"). A blooper keeps the take number; only R starts a new take.

**Proof, Nov 14.** Scripted play-in-editor runs: the whole take from the back of the queue; the carpet blooper; the
beam blooper back to the rope with the banner use given back and the crew still posted; route 1 (banner then spot)
and route 2 (cart then spot); the free lift, the sound desk lift and the dais cart lift. One run of the packaged
`.exe` with real keys and mouse (`C:\Jam\Builds\shots\play2.ps1`, 164 s): The Door, both crew walks, the floor, a
beam blooper back to the rope with the clock back from bar 62 to bar 60, route 2, the dais cart (x2), the keycard.
Its screenshots are `docs/shots/step2_*.png`.

**Only Marwan can judge:**
- Route 1 (banner then spot) reaches the dais with about 20 composure left: too tight or right?
- The lift takes 1 s (the spec says 1.5 s) and the arrivals searchlight blinds for 6 s (the follow spot 4 s).
- Whether the three warning pitches can be told apart by ear, and whether the drone feels fair.
- The night look: exposure, the haze, how dark the hall and the backstage are.
- Maya's walk to the follow spot (about 22 s) and Vex's (about 6 s): too long before the hall?
- Label clutter where several watchers and devices line up on Maya's camera, and the wording of every line.

**Scoring:** any score of this game, by an agent or by Marwan, follows `docs/JUDGING_RUBRIC.md` (the jam's criteria,
fixed anchors, and when to stop). Design review is closed; the next scores come at gates 1 and 2.

**The game: a heist shot in one take** (working title *Print It*). Marwan wrote the pitch on Fri Nov 13; it is kept
word for word in `docs/ideas/heist-pitch.md` and replaces K. Judge agents scored the design round by round
(`docs/ideas/heist-judging.md`). From round 3, at Marwan's request, they scored the design alone, with execution set
aside. Round 7's panel gave all 5s. A fresh judge who had seen none of the earlier rounds then scored it 3.9: too many
rules for a judge playing 3 minutes cold. This version is the **subtraction pass** after that, revised once more with two fresh judges' notes:
- one way to act on every camera;
- three effects;
- one meter;
- a clean run of under two minutes.

K's spec is archived in `docs/ideas/k-meanwhile-spec.md`, and nothing in `prototypes/k-3d/NOTES.md` applies any more.
`docs/ART_STYLE.md` was written for K: only the parts named under "Look" apply, and where it differs from this file,
this file wins.

State: **steps 1 to 4 are built** (step 4, the look, Nov 14, about 22:00). The C++ is in `Reel/Source/Reel/Take/`,
the map is `/Game/Take/Lvl_Auction` (the project starts in it), and `tools/build_auction.py` rebuilds the map from its
numbers, then dresses it with `tools/dress_auction.py`; `tools/build_look.py` builds the film grade (`Lvl_TestBox`
and `tools/build_testbox.py` are the old step 1 and no longer load cleanly). From the street start the take plays
through Scene 1 (The Door), Scene 2 (The Floor) and Scene 3 (The Getaway) to PRINT IT, the stats and grade and the
blooper reel, with a checkpoint at each scene, at the rope, on the dais and at the case, on the film and making-of
look with the HUD in the letterbox bars. Next: **gate 2**: on Nov 14 Marwan moved step 4 ahead of it, so his first
full take is on the build with the look. Steps 5 and 6 are not started.

## The pitch

Tonight the Aurelia Auction House sells the only print of a lost film: the **Director's Cut**. A film crew is going
to steal it, and you are their director, shooting the heist as one live take on three cameras. As on a multi-camera
shoot, all three cameras roll on one continuous take, and you call the cuts between them as it happens, the way live
television is directed:

- **Camera A, Julian:** the lead, in a white dinner jacket, on the gala floor.
- **Camera B, Maya:** the gaffer (the lighting chief), on the catwalk above the hall.
- **Camera C, Vex:** props and sound, backstage.

Each camera does one job, so every move needs a cut:
- **1, 2, 3** cut between the cameras.
- **On camera A, click a mark** and Julian walks to it.
- **On camera B or C, click a watcher or a piece of the set** (the spotlight, a banner, a cart, the band's PA) and it
  fires.

The take never stops. Julian walks whether you're watching or not, and the room keeps time to the jazz band on the
floor. The skill is landing the right effect on the beat Julian arrives.

- **If he's caught:** a blooper. The frame burns, and you go again from the last checkpoint.
- **If he gets out with the reel:** the clapper snaps and the screen says **PRINT IT**. The game plays back **your
  director's cut**: your take with the bloopers spliced out, cut where you cut. You can re-cut it live.

**What a player learns:**
- three actions: **cut** (1 2 3), **cue** (click a mark, on A), **fire** (click the set, on B or C);
- three effects: **Blind**, **Lure**, **Mask**;
- one meter: **composure**.

Scene 1 teaches the whole loop in 15 seconds: cue, cut, fire, cut back.

**The theme lands four ways:**
1. You direct: cue the actor, fire the effects, call the cuts, edit the result.
2. The heist is a take, with bloopers and a long take.
3. The thing you steal is the Director's Cut.
4. Four **deleted scenes** are hidden across the take. Only a take with all four restored earns the grade
   DIRECTOR'S CUT.

## Rules

### 1. The film and the making-of

Keys 1, 2 and 3 cut to that camera on the same frame, with a hard cut, a switcher click, and the change of look. The
world never pauses, except for the 1 s blooper freeze. Every camera is composed: either fixed or moving on a set
track, and never steered by the player. The mouse is a pointer on every one.

**Camera A, "the film":** a **shot list** of composed fixed angles, one or two per zone:
- a high angle over the queue onto the scanner, which Maya's first catwalk view reverses;
- a slow tracking shot on a long lens, from the exhibit down the floor to the rope: one angle for that whole stretch;
- a two-shot on the podium;
- a high angle on the hatch.

Camera A changes angle by itself as Julian enters a zone. Those are the film's own cuts: they don't count as yours.
- **Frame and grade:** a 2.39:1 letterbox. Noir: high-contrast monochrome, with Julian key-lit (his white jacket the
  lightest thing in frame) and everyone else rim-lit into a silhouette in costume (top hats, gowns, guards' caps).
  Only red and gold keep their colour. Grain.
- **The set:** its dressed fronts.

**Cameras B and C, "the making-of":**
- **Views:** Maya and Vex move to a new post between scenes, so B and C have one fixed view per scene.
  - Each view frames that crew member's devices and the watchers they can reach, not the whole hall.
  - B looks down from the catwalk.
  - C is handheld behind the flats, with a slight idle shake.
- **Matching framing:** in every scene, one crew view is composed as the reverse of camera A's main shot: the same
  frame, seen from behind the flats. The cut flips one composition from front to back.
- **Palette:** a designed work-light palette. Steel-blue shadow, cyan-white fluorescent highlights, and nothing warm
  except the red frames and Julian's gold path.
- **What's in frame:** the same bodies as bare stand-in mannequins with tape name labels ("GUARD 2"), and the raw
  backs of the flats.
- **Layout:** the crew view fills the top three quarters. The bottom quarter is a **monitor strip**: the film feed
  (camera A's current shot, live, 2.39:1), the device pips and the timecode. The noir frame is on screen all game.

**Costumes only in the film, labels only on set.** Camera A, the strip's feed, the stills and the replay all show
costumes and hide labels, because they all view as Julian's pawn: costume props are "only owner see", tape labels
"owner no see", both owned by Julian's pawn. The feed and still captures are SceneCapture2D components on Julian's
pawn, moved each frame to the shot being captured; the replay camera is spawned with Julian's pawn as its owner.

### 2. The band's clock

A jazz quintet on the floor plays at a fixed 120 BPM, so 1 bar is 2 s. They take a one-bar break every 8 bars.
- **The world keeps time:**
  - The guard turns on bar lines.
  - The drone passes each zone every 8 bars.
  - The beam drops for one beat in every 4 bars, on a hi-hat splash.
  - Once Vance has taken the mic (Scene 2), he speaks for 4 bars on every band break.
- **The band warns you.** Each watcher type has its own instrument, which plays a short phrase one bar before that
  watcher turns or passes:

  | Watcher | Instrument |
  |---|---|
  | The guard | The upright bass |
  | The drone | The muted trumpet |
  | Vance's speech | A drum roll |

  You can hear a threat coming before you see it. Each instrument is taught on its own: the first time its watcher
  arrives, it plays the phrase solo while that watcher's tally light is lit.
- **On screen:** nothing moves with the beat (Nov 15), and nothing on screen warns a bar ahead: only the band
  does. Any count on screen is in seconds. Julian's path turns red only where danger is right now: the red tape, the beam, a
  live frame, the drone's cone. It never forecasts a turn.
- **It never stops.** The feedback knocks the band out for its 2 bars, and they come back in on the next bar line. That
  drop isn't a band break: Vance doesn't speak, and no checkpoint saves. The alarm switches to the heist cue at the same tempo.
- **Blooper restores:** every patrol, pass and speech is a function of the bar count. A blooper sets the clock back to
  the bar saved at the checkpoint and seeks the music there.

### 3. Cue: click a mark

- **On camera A, a click on a gold mark** cues Julian there. Marks can only be cued from camera A, because the film is
  where the actor is directed.
- He walks the shortest path at 250 cm/s on his own, whether you're watching or not, and stops on the mark.
- His path shows as a dotted gold line with the beat count. The line stays visible on B and C.
- **A new click replaces the old cue.**
- **Julian only moves by cues.** There is no WASD, and he never runs.
- **Action marks** do their action on arrival:
  - the mark at the scanner is a gate: Julian waits on it until the scanner's frame is dark, then walks through;
  - the mark behind Vance lifts the keycard (1.5 s);
  - the mark at the case opens it (it appears once Julian has the keycard);
  - the mark at the side exit makes Julian bow.

  A click on A always means "go there".

### 4. Fire: click the set

- **The posts.** Maya and Vex stand still at their post for the scene. Every device they can fire is in their
  camera's view. Devices fire only from B and C.
- **Hover:** shows the device's range ring, a marker over every watcher it would catch, and its pips.
- **Click a device:** it fires.
- **Click a watcher on Maya's camera:** the follow spot blinds it.
- **Anything else** (a mark, or a watcher on Vex's camera) fires nothing and flashes the camera that can act on it:
  "[1] to cue Julian", "[2] to spot him".

A click on B or C never cues Julian.
- **Speed:** a crew cut takes about 2 s (cut, click, cut back).

### 5. Composure (0-100)

The only meter: a gold line in camera A's bottom letterbox bar, and under the strip's film feed.

**Julian is exposed while:**
- he is inside the **red tape** (around the scanner, around the exhibit, past the rope, on the dais; at the alarm the
  whole floor turns red);
- he is **off a mark** inside the **drone's cone**, which is drawn on the floor as a red ellipse. On a mark he is in
  character, and the broadcast shoots straight past him.

Those are the only two rules, and both are visible on the floor. Every watcher sees the same way: its frame catches
an exposed Julian.

| Case | Change | Label |
|---|---|---|
| A watcher's frame holds an exposed Julian | 0.5 s grace, then -30 a second | **SEEN** |
| He stands on a mark and isn't exposed | +15 a second | **MARK** |
| Style: a GOOD CUT, or one firing catching extra watchers | +15 for a GOOD CUT; +25 for each extra watcher (x2, x3) | |
| He touches a live infrared beam | Blooper at once | |
| Composure reaches 0 | Blooper | |

As composure falls, the film burns in from the edges.

### 6. Watchers

Each sees with a **camera frame**: thin red frustum lines (the four-sided pyramid of what a camera sees), bold on B and
C, faint on A. A line trace means walls, banners and the curtain block it. Each carries a red **tally light** that goes
live when it frames an exposed Julian. On B and C its tape label also shows its state ("GUARD 2 - LURED").

| Watcher | Behaviour |
|---|---|
| The iris scanner | A frame on the scanner mark, inside its red tape. Julian waits on the mark until the frame is dark (rule 3), and is SEEN while it is live |
| The patrol guard | Waypoints, turning on bar lines |
| The broadcast drone | A spline loop, one pass per zone every 8 bars |
| The infrared beam | A line-shaped frame across the rope. It drops for one beat in every 4 bars, on a hi-hat splash. Blind or Mask turns it off. Live 4 bars into Scene 2, before Julian can reach it |
| The bodyguard | Posted beside the podium, facing outward at an angle |
| Vance, the host | A frame over the lift mark behind him. While he **speaks** it turns to the crowd, a **Lure** turns it to the cart, a **Blind** darkens it. He never leaves the podium. If it catches Julian he barks "Security!", and Julian is SEEN as anywhere else |

### 7. Devices: three effects

Every device does exactly one of three things to every watcher in its range:

| Effect | What it does |
|---|---|
| **Blind** | The frame goes dark for the duration |
| **Lure** | The watcher walks to the device's point, looks at it, and walks back (Vance only turns his head) |
| **Mask** | Solid geometry, a matte, that blocks frames and the beam |

| Device | Who | Effect | Range, duration | Uses |
|---|---|---|---|---|
| Follow spot: click a watcher on Maya's camera | Maya | Blind one watcher | 35 m, 4 s | **Free**, one watcher at a time: ready again the moment its 4 s ends |
| Banner drops, two | Maya | Mask | 10 s, then they winch back up | 1 each |
| Pyro (locked until the alarm) | Maya | Blind everything at the stage end, with sparks and smoke | 6 s | 1, guaranteed |
| Champagne carts at two service doors (mid-floor, and by the dais) | Vex | Lure everything within 12 m to the crash | 6 s | 1 each |
| Sound desk: feedback through the band's PA | Vex | Blind everything in the hall | 4 s (2 bars) | 1 |
| Counterweight winch (finale only) | Vex | Mask: the curtain drops for good and the hatch opens | Permanent | 1, guaranteed |

- **Stamps:** when one firing catches two watchers it stamps **x2**, three stamps **x3**, and each extra watcher refills 25 composure (rule 5).
- **The budget:** the follow spot is the free answer in every scene. The one-use devices are the budget, and the
  budget buys only the side exit, The Bow and the top grade. It never decides whether the ending is reachable.
- **Side exit:** it glows gold on camera A from the first shot.
- **Tuning:** all values are `UPROPERTY(EditAnywhere)` on the device classes. A blooper restores the uses saved at the
  checkpoint.

### 8. Good cuts

**GOOD CUT:** you cut to camera A within 2 s (one bar) of a crew device landing, while the effect is inside camera A's
current shot.
- It gives +15 composure, so cutting back to watch the payoff also keeps Julian steady.
- A cut that lands on a downbeat plays a brass stab, so a clean take sounds composed. It isn't scored.
- The next still gets a gold frame.
- At each checkpoint, your last GOOD CUT replays for 4 s in a gold frame at the right of the top letterbox bar. Play
  doesn't pause.

### 9. Bloopers and takes

**What triggers one:** composure reaches 0, Julian touches a live beam, or the finale clock runs out.

**What happens:**
1. A 1 s freeze, with the film burning through from the centre.
2. The music tape-stops and the clapper clacks.
3. A red slate reads **"BLOOPER - AGAIN FROM THE ROPE"**, naming whichever checkpoint, and the director says "Cut!"
4. A rewind whirr plays while the level reloads at the checkpoint, and then the director says "Action!"

**Checkpoints:** the start of each scene, the mark just past the rope (which no frame covers, the bodyguard's post included), and every band
break on which Julian stands on a mark, unexposed, with at least 50 composure. A failed attempt replays 16 s at most.
A reload clears every effect in play, refills composure to 100 and frees the follow spot.

**What carries across the reload:** the GameInstance (Unreal's object that survives a level load) keeps:
- the checkpoint and its bar;
- the uses left;
- what's done so far;
- the recording.

**Hints:** after two bloopers at one checkpoint, Maya or Vex hints at an answer over the earpiece.

**A new take:** **R** starts one from the opening, and the slate counts up: "TAKE 2", "TAKE 3".

### 10. Deleted scenes

There are four, each a harder optional flourish built from the same three actions. Each is announced by a small
slate at its scene's start ("DELETED SCENE: THE LONG TAKE. Cross without cutting."). Restoring one stamps "DELETED
SCENE RESTORED", grabs its own still, and the director says "Keep that in!"

| Scene | Deleted scene | How |
|---|---|---|
| 1 | **The Cold Open** | The spot lands on the exact beat Julian reaches the scanner |
| 2 | **The Long Take** | From the exhibit mark over the rope with no cut: about 20 s held on camera A's tracking shot, a real oner (one unbroken shot). With no cut there are no devices, so Julian threads the guard's turns (he's exposed inside the exhibit's tape), the drone's pass and the beam's flicker on cues and timing alone |
| 2 | **The Insert** | One cart catches both Vance and the bodyguard (x2), and you are on camera A as the lift lands. The film cuts to an insert: a close-up of the keycard leaving the belt |
| 3 | **The Bow** | At the side exit, cue Julian to the bow mark while the drone is blinded: he bows to it |

### 11. The finish

1. A snap to black, the clapper, the **PRINT IT** slate, and the sting.
2. **YOUR DIRECTOR'S CUT** (32 s, two 8-bar phrases; Space skips). Your take with every blooper spliced out, cut
   where you cut, trimmed in whole 8-bar phrases (the band's cycle) so the music never jumps. It keeps two
   phrases: the exit, and before it your best phrase (a restored deleted scene first, its last 8 bars if longer;
   otherwise the phrase with the most GOOD CUTs). A cold first take and its replay end inside about
   4 minutes, so the grade and "R: another take" land inside a judge's window.
   - It is the film throughout: camera A's shots, in costume. Where you cut to B or C, the replay covers that moment
     with the drone's broadcast angle, which faces the dressed fronts. The real score runs under it (the band, then
     the heist cue).
   - Restored deleted scenes carry a "RESTORED" tag.
   - The recording holds every moving actor's transform at 20 Hz, plus device events, cues and the cut list.
3. **The edit suite.** While the replay plays, 1 (camera A) and 4 (the drone's broadcast angle) take over the cut live.
   Your live cut replaces the automatic one from that moment.
4. **The contact sheet.** Up to 12 stills grabbed at your cuts. Each is a SceneCapture2D set to Final Color (LDR),
   rendering into one of 12 render targets at 256x144.
5. **The stats.** CUTS, GOOD CUTS, BLOOPERS, TIME, and DELETED SCENES n/4, with each missed one named by its slate
   line ("THE LONG TAKE: cross without cutting"), so the next take has a target.
6. **The grade.**

   | Cut | When |
   |---|---|
   | **DIRECTOR'S CUT** (gold) | All 4 deleted scenes restored in one take. Bloopers don't count: they are spliced out |
   | **FINAL CUT** | 2 or 3 restored |
   | **ROUGH CUT** | Finished |

7. **The blooper reel** rolls under the credits: every blooper's freeze frame with its slate.

R shoots a new take. ESC quits.

## The take

One continuous take, about 1:45 when clean, in three scenes that are checkpoints in one space. The gold case with
the reel sits on the dais, and the gold side exit glows at the far end, both in the very first shot.

### Scene 1. The Door (about 15 s): teaches the whole loop

1. On camera A, a high angle over the queue: "Click the mark at the scanner." Julian walks there, and his path shows
   "at the scanner in 12". Counts are always in beats, half a second each.
2. "[2] MAYA" flashes. Her catwalk view is the reverse of that shot: the same queue and scanner, from behind the
   flat. "Click the scanner as Julian arrives."
3. The moment the spot lands, the Chief yells "Who turned that on?!" and "[1] JULIAN" flashes. Cutting back within the
   bar lands the first GOOD CUT. With the scanner's frame dark, Julian walks through onto the floor.
4. If the frame is live when he arrives, he waits on the mark, SEEN, and the Chief says "No match. Hold there, sir."
   The spot still frees him; at 0 composure it's a blooper.

**Answers:** the spot (free).

**Deleted scene:** The Cold Open.

### Scene 2. The Floor (about 70 s)

**Julian's goal:** cross the floor, pass the rope onto the dais (a checkpoint), then lift the keycard from Vance's belt.

**Watchers arrive one at a time,** each on the next 4-bar line after its trigger. The band drops out under its
instrument's solo phrase for that bar, so no wait is longer than 8 s:

| Trigger | Arrives | Its warning |
|---|---|---|
| The scene's start | The marks, and the patrol guard | The bass |
| 4 bars in | The rope's beam goes live, before Julian can reach it | The hi-hat splash |
| The guided win | The drone. Before it can catch anyone, the marks on its path pulse gold and "Off a mark, the drone sees him" shows | The trumpet |
| Julian crosses the rope | The bodyguard takes his post, facing out, and Vance takes the mic; from then on Vance speaks for 4 bars on every band break | The drum roll |

**The guided win** (first 15 s): "Click the mark by the exhibit". It is inside the exhibit's red tape, so the guard
can catch Julian there, and camera A's angle on it frames the guard's approach. Then "[2] spot the guard as his frame
swings onto Julian", and a GOOD CUT.

**The rope:**
- **Free:** spot the beam on the bars when the guard faces away (the bass warns a bar before he turns), and cue Julian
  over.
- **Paid, for a wider window:** a banner on the beam (Mask, 10 s), leaving the spot for the guard; or the mid-floor cart on
  the guard (Lure, 6 s), leaving the spot for the beam.

**The lift:**
- **Vance's frame** has to be off the lift mark: he is speaking (free: 4 bars on every band break), lured, or blinded.
- **The bodyguard:**
  - **Free:** during a speech, spot him and cue Julian to the lift mark behind Vance.
  - **Paid:** the feedback (Blind, the whole hall).
  - **Paid:** the dais cart (Lure: Vance and the bodyguard, x2). If you're on camera A as the lift lands, that's The
    Insert.

**Deleted scenes:** The Long Take, The Insert.

### Scene 3. The Getaway (16 s: the last 8 bars)

Cue Julian to the case mark (it appears once he has the keycard) and he takes the reel; the case is Scene 3's
checkpoint. On the first take, the prompts return as the alarm sounds: "[3] VEX: the winch" and "[2] MAYA: the pyro". The alarm stops the band dead, the shutters fall, four guards walk in from both
sides, the whole floor turns to red tape, and the drone locks on and follows Julian. That first bar is
held: nothing sees him until the next bar line. Camera A bleeds to full colour for the last 8 bars: the alarm's red, the pyro's gold,
every colour at once.

**The exits:**
- **The hatch under the stage, 10 m away (free):** Vex's winch drops the curtain for good, masks both flanks and opens
  the hatch. Maya's pyro blinds the stage end for 6 s.
- **The gold side exit past the band,** 14 m from the case (about 11 beats): the pyro doesn't reach it. Two of the
  four guards come in through it and need a Lure, Blind or Mask you saved (a cart, the feedback or a banner); the
  spot takes the drone.

**Deleted scene:** The Bow, at the side exit only.

## Controls

| Input | What |
|---|---|
| 1, 2, 3 | Cut to Julian, Maya, Vex (and 4, the drone's angle, in the edit suite) |
| Mouse | Point (a cursor on every camera) |
| Left click | On camera A: cue Julian to a mark (action marks act on arrival). On B or C: fire the device, or spot the watcher, you click |
| Space | Skip the replay |
| R | New take from the opening |
| M | Mute |
| ESC | Quit |

## Camera and HUD

- **Cameras:**
  - **Camera A** is the shot list: CineCameraActors (Unreal cameras with real-lens settings), fixed or on a set track,
    switched by zone triggers. Every angle frames the next marks Julian can be cued to.
  - **Hovering a mark on A** draws his path and beat count, red where the tape, the beam, a live frame or the drone's
    cone is right now. It never forecasts a turn: the band does that.
  - **B and C** have one fixed camera per scene.
  - **A cut** swaps the player controller's view target on the same frame: no camera flight, no slow motion.
  - **Exposure** is manual on every camera.
- **Camera A's HUD,** all in the letterbox bars:
  - top left: "A - JULIAN" and the timecode;
  - top right: a red REC dot, and the replayed GOOD CUT at checkpoints;
  - bottom: the gold composure line and its label.
- **B and C:** the monitor strip from rule 1. The film feed is a SceneCapture2D on Julian's pawn, copying camera A's
  current shot at 15 fps, set to Final Color (LDR), with the composure line under it.

## The set

One level and one hall, about 30 x 20 m, built as a film set. Three walls are flats, and the fourth side opens onto a
dark studio with light stands and cables. Camera A sees the dressed fronts: gilt, velvet, chandeliers. B and C see the
bracing, sandbags, cables and stencilled labels behind them.

The hall has three layers, each in view of the others:

1. **The floor (camera A)**
   - The queue and the iris scanner at one end.
   - Posed guests.
   - **The band:** a jazz quintet on a riser with its PA. The band and the guests bob on the beat, through a Transform
     (Modify) Bone node in the animation Blueprint driven by the bar clock.
   - Gold marks by the champagne bar, the paintings, the exhibit and the pillars, and one each side of the rope (the
     near one outside the tape).
   - The rope and its beam.
   - **The dais:** Vance's podium with the lift mark behind it, the gold case, and the hatch under the stage.
   - The gold side exit past the band.
2. **The catwalk (camera B)**
   - About 6 m up, along one long side and across the stage end.
   - Each scene's view looks down on Maya's devices and the watchers they can reach.
   - Maya's devices are in view: the follow spot, both banner lines and the pyro switch.
3. **Backstage (camera C)**
   - A corridor behind the long wall and the stage.
   - It sees the floor through two service doors and a gauze scrim.
   - Vex's devices are in view: the cart releases, the sound desk and the winch.

The broadcast drone flies its spline loop over the floor with its tally light on.

## Look

The film and the making-of, cut against each other (rule 1).

**Camera A:** noir monochrome in composed angles, fixed or tracking. Julian is key-lit; everyone else is rim- and back-lit into
silhouettes. Hard spotlights shine through haze, inside the letterbox, with grain.
- **Colour that survives:** only red and gold keep their colour. Those objects are marked with the custom-depth
  stencil (a per-object tag the post-process reads):
  - red is danger: frames, tape, tally lights;
  - gold is Julian: his marks and path, the reel, the side exit, the stamps, the DIRECTOR'S CUT grade.
- **The alarm:** at the alarm the picture bleeds to full colour, and the watchers' frames switch to steady emissive
  white so they still read.

**B and C:** the designed work-light palette: steel-blue shadow, cyan-white fluorescent highlights, and nothing warm
except the red frames and the gold path. Bare
stand-ins with tape labels, and the raw backs of the set. The monitor strip keeps the noir feed beside them.

**From ART_STYLE.md, still in force:**
- §1 rule 1 (value) and rule 2 (red and gold only for meaning).
- §1 rules 4 and 5: no Lumen, no Nanite.
- §8: the film overlay as a UI widget, with a 2.39:1 letterbox in place of the 4:3 gate.
- §14: the checks.

Rule 3's location tones are replaced by the film and making-of split.

**Lighting:** movable spot lights with volumetric fog (these work without Lumen), at most about 4 shadow-casting spots
on screen, and a dim skylight for the missing bounce light.

**Characters:** template mannequins tinted by role. On camera A they wear full-body costume shells (a tuxedo, gowns,
guard uniforms, the band's suits) with top hats, caps, bow ties and instruments, so the stock mannequin never shows in
the film. Generated models are for props only.

## Audio

**The band** is the score, the clock and the warning system (rule 2). It is heard, not watched: nothing on screen
moves with the beat (Nov 15).
- The mix has a real source for each camera: close on the floor, from above on the catwalk, through the flats
  backstage.
- The feedback squeals through their PA, and they drop out for 2 bars.
- The alarm stops them dead. The heist cue crashes in at the same tempo; its last bar is the deadline, the sting is
  the escape, and a tape-stop is failure.

**Composure** is heard as a heartbeat and a high ringing that rise as it falls. Low-pass filtering is only for
backstage.

**Voice** (ElevenLabs): about 50 short lines, all subtitled.
- The director: "Action!", "Cut!", "Print it!", "Keep that in!"
- Julian's quips.
- Maya and Vex on the earpiece: device lines and hints.
- Vance's speeches.
- The Chief and the guards' barks.

**Sound effects** (ElevenLabs):
- **Cuts:** a switcher click on every cut. The clapper only at a take's start, a blooper and PRINT IT.
- **Bloopers:** the film-burn crackle, the buzzer, the tape-stop, the rewind whirr.
- **Devices:** the feedback, breaking glass, the banner drop, the curtain thud, sparks.
- **The finale:** the alarm, the shutters.

**Music source** (*filled in Nov 15: ElevenLabs Music, a looping bed plus one-shot phrases, no stems; see "Step 6, audio: phase 1 as built"; the original text follows*): WeLoveIndies or ElevenLabs music, at a fixed and known 120 BPM, with separate stems (one track per
instrument) for the bass, trumpet and drums (the warning instruments) and the piano, so each warning's warning phrase can be triggered. Voice lines
duck the band's bed, never the warning phrases.

**Credits and the key:** every tool and library goes in `docs/CREDITS.md`. Marwan sets `ELEVENLABS_API_KEY` on the PC
themself (README, "The game"). Agents never type it or print it.

**Until step 6:** subtitles and engine sounds stand in (the click track was removed on Nov 15).

## Tick order

The rules live in one director actor, `ATakeDirector`, which ticks after the player controller:

1. **The bar clock:** beat and bar events, watcher arrivals, warning phrases a bar ahead.
2. **Presses:** a cut first (swap the view, grab a still, open the GOOD CUT check), then clicks.
3. **Devices:** a device that fires this frame applies its effect to every watcher in range, counts x2, and records
   when it landed.
4. **Julian's cued walk,** then the watchers' sight tests against him: frame, range, line trace.
5. **Composure:** rule 5.
6. **The scene:** goals, checkpoints and the good-cut replay, deleted scenes, the finale clock.
7. **End of frame** (late tick): the blooper check, the finish check, the recorder's sample.

## Events and feedback (all on the frame of the event)

| Event | Feedback |
|---|---|
| Cut | Hard cut, switcher click, the film or making-of look, a still |
| Beat | Nothing on screen (the band alone keeps time) |
| A bar before a turn or pass | That watcher's instrument phrase |
| A watcher arrives | "<LABEL> arrives in N s"; its phrase solo the bar before; its tally lit, steady, while the warning is up |
| Click a mark | A gold ring, the dotted path with its count, Julian turns and walks |
| Hover a device | Range ring, markers on the watchers it would catch, pips |
| Device fires | The crew's line, its sound, the effect starting in the hall, a pip goes out |
| Two or more caught | x2 or x3, and the composure refill |
| GOOD CUT | Stamp, +15 composure, a gold frame on the next still |
| Checkpoint | Your last GOOD CUT replays for 4 s in the top letterbox bar |
| A watcher frames an exposed Julian | Tally light live, the frame solid red, the heartbeat starts |
| Composure drains | The gold line flashes red with SEEN, the film burns at the edges, the ringing rises |
| On a mark | A gold X under Julian, MARK |
| Lure, Blind, Mask land | The frame swings toward the point / goes dark / a thud with dust |
| Lift | A 1.5 s ring, the keycard chime |
| Deleted scene restored | Stamp, its own still, "Keep that in!" |
| Case opened | The band stops dead, the alarm, the bleed to full colour, shutters, the heist cue, the 8-bar clock |
| Blooper | Freeze, film burn, tape-stop, clapper, red slate, buzzer, "Cut!", rewind whirr, "Action!" |
| Finish | Snap to black, clapper, PRINT IT, the sting, your director's cut |

## Kill list (final)

**Cut in the subtraction pass:**
- The bluff: it was Marwan's "live AI interrogation". It would come back pre-voiced if wanted.
- The reversal.
- The breakers, the fuse boxes, the side door, the registry terminal, the table lamps.
- OFF MARK.
- WASD and mouse-look.
- Beat-tick scoring.
- The second side route.

**Also out:**
- **Runtime AI:** any AI at runtime. A key in the build gets extracted, and offline judges get silence.
- **Camera tricks:** the character wheel, camera flights between characters, slow motion.
- **Space:** a separate lobby, more than one level, more than one hall.
- **Characters and physics:** crowd AI, guests that walk, tackles, physics destruction (the carts and banners move on
  timelines), reaction animations, "?" or "!" icons.
- **Engine features:** behaviour trees and AI perception (watchers are plain C++), a true rewind, a timeline editor.
- **Assets:** generated or retargeted character models.
- **Dialogue and UI:** any dialogue beyond the barks, hints, device lines, Vance and the director. Saves, settings,
  difficulty modes, menus beyond the title card and the end sequence.
- **Rendering:** Lumen and Nanite.

## Build order

Core loop in a test box first, then the set with its look, then the scenes (the ending first), then dressing,
characters, audio and polish. Every step ends playable, and the Windows package is rebuilt every night (Fri and Sat).

| When (Cairo) | Step | Done when |
|---|---|---|
| Fri, by 11:45 | **0. Pipeline** | Done |
| Fri 17:00-22:00 | **1. Core loop in a test box.** See "Step 1" below | In play-in-editor, Julian gets past the guard and the beam two ways, by a cue plus a device timed to his arrival. **Built Nov 13, 17:30. Rebuilt to the gate 1 answer (walk, flights, the two-floor auction building) Nov 13, 19:40** |
| Fri about 22:30 | **Gate 1.** Marwan plays for 5 minutes. If they don't want another go, change a rule (windows, drain, uses), not the art | A yes, or the rule changed. **First answer Nov 13, 18:15: three rules changed (top of this file). Passed Nov 13, about 23:45: Marwan played the rework and the street-start build and said to start step 2** |
| Fri night to Sat morning | **2. The set, its look, Scenes 1 and 2.** See "Step 2" below. Package | Scenes 1-2 play in the package, and screenshots show the film and the making-of. **Built Nov 14, about 04:00, for the walking game: see "Step 2 as built". The film and making-of split waits for gate 2** |
| Sat (midterm 12:30) | **3. Scene 3 and the ends.** See "Step 3" below. Package | Title card to blooper reel in the package. **Built Nov 14, about 05:00, for the walking game: see "Step 3 as built". The replay, the edit suite and the contact sheet wait for gate 2** |
| Sat, moved ahead of gate 2 on Nov 14 | **4. Look.** Dressing the fronts and backs, the film burn, the letterbox HUD, the stamps | Screenshots pass ART_STYLE §14. **Marwan moved this step in front of gate 2 on Nov 14. Built Nov 14, about 22:00: see "Step 4 as built" (the design is ART_STYLE §15; its checks are §15.11)** |
| Sat evening, after step 4 | **Gate 2.** Marwan plays a full take cold | They want another go. **Played on the build with the look (step 4 moved ahead of it on Nov 14)** |
| Sat night to Sun 10:00 | **5. Characters and props.** Tinted mannequins, costume props, the band's instruments, the bob on the beat, the bow. Fab free packs, or generated props if the codes arrive. Niagara (Unreal's particle system) sparks, glass and smoke | Credited in docs/CREDITS.md |
| Fri night to Sun 10:00, in parallel | **6. Audio.** A separate session, outside the editor. Write the lines, generate them with ElevenLabs once Marwan has set the key, and get the band's tune and the heist cue at 120 BPM, with stems. The editor session imports everything and locks the bar clock to the tune by Sun 10:00 | Content lock **Phase 1 built Nov 15 (script, tools, C++, generation) on branch `step6-audio`: see "Step 6, audio: phase 1 as built". Phase 2 (merge, import, play-in-editor, package) waits for the editor** |
| Sun 10:00-18:00 | **7. Polish.** See "Step 7" below | Nothing new after 18:00 |
| **Sun 22:00** | **Upload and submit.** Then one try at the Web build (proven on the template, 23 min to package) as a second zip that plays in the browser | Downloaded and played once |

**Step 1, the core loop in a test box:**
- Start with one header pass that declares every new class and `UPROPERTY`, so the editor restarts happen together.
- Three characters and the cut. Camera A is two fixed angles with a zone trigger; B is one fixed camera.
- The mouse pointer: on A, click a mark to cue Julian (navmesh walk, path, count); on B, click a device or a watcher
  to fire.
- The bar clock on a metronome, with pulsing frames.
- Composure and the red tape.
- One patrol guard, whose frame is a flat mesh that ignores walls for now.
- The spot (free, one watcher at a time), a cart and a banner.
- The rope's beam.
- A blooper that reloads the level.

**Step 1, smallest fixes made while building (no rule changed):**
- The test box has no camera C, so its cart is fired from B (`FiredFrom` on the device). Key 3 flashes "No camera C
  on this set".
- The follow spot is fired by clicking a watcher on B. Clicking the lamp itself flashes "Click a watcher to spot it".
- The test box's goal mark sits just outside the red tape, and once Julian stands on it the take is CLEARED: no
  blooper can fire after that, and R starts a new take.
- Numbers the spec doesn't give are tunables with a first guess: the cart rolls for 1.0 s, the banner drops in 0.5 s
  and reaches 6 m, the guard's frame is 9 m long and 30 degrees either side, and he walks at 2 m/s.

**Step 2, the set, its look, Scenes 1 and 2:**
- The hall on three layers, fronts and backs, and the shot list.
- The noir film with stencilled red and gold, key and rim lights.
- The fluorescent making-of with its monitor strip, and costume and label visibility.
- Manual exposure, haze, and the skylight.
- Frames with line traces, tally lights, labels.
- Stills, checkpoints with the bar restore, the good-cut replay, the recorder.
- The queue and scanner, the drone, the staggered arrivals and the instrument warnings (click-track pitches until step
  6).
- Vance's speeches, the bodyguard, the sound desk, the lift.
- The Cold Open, The Long Take, The Insert.

**Step 3, Scene 3 and the ends:**
- The pyro, winch, curtain and hatch; the side exit; the shutters, the 8-bar clock, the colour bleed; The Bow.
- The title card, PRINT IT, the replay with the edit suite, the contact sheet, the stats and grade, the blooper reel,
  R.

**Step 7, polish:**
- A mix pass, a bug bash, the itch page, the dev log.
- **The 30-60 s video:**
  1. Open on the signature shot: a front/back flip (the film's frame, cut to the same frame from behind the flats),
     then Scene 1's GOOD CUT: the spot slams the scanner on the beat, cut, Julian walks through.
  2. A cue landing on the trumpet's warning.
  3. The finale's bleed to colour.
  4. A frame of the replay.

**If Saturday slips,** cut in this order, so the ending is always reachable:
1. The edit suite (keep the automatic replay).
2. The replay itself (keep the contact sheet).
3. The Long Take and The Insert (keep The Cold Open and The Bow).
4. The side exit.
5. The good-cut replay at checkpoints.

## Decided Nov 13

1. The heist replaces K (Marwan's pitch). The full design is the target, and Marwan carries the execution.
2. Each camera does one job: cue on A, fire on B and C, and 1 2 3 to cut. Every move needs a cut.
3. Three effects (Blind, Lure, Mask) and one meter. The follow spot is the free answer, and the budget buys only the
   side exit and the grade.
4. One hall on three layers, three scenes, a clean run of about 1:45.
5. **Camera A is the film:** a shot list, noir, costumed, red and gold. **B and C are the making-of:** fluorescent,
   stand-ins, a monitor strip.
6. The band is the score, the clock and the warning system, at 120 BPM.
7. No AI at runtime. ElevenLabs for voice and sound effects.
8. Solo, with Claude agents (the Solo Driver prize).

## Not decided yet

- **Game and team name.** The zip must be `TeamName_GameName.zip`. Suggested: *Print It*.
- ~~The band's tune, the heist cue and their source~~ *Filled in Nov 15:* ElevenLabs Music at 120 BPM, a looping bed
  and one-shot phrases instead of stems (see "Step 6, audio: phase 1 as built").
- **Student category:** the Official Rules are still unread.
- **Perk codes:** none arrived. Retry Tripo before Nov 16; ask the organisers for late-joiner codes if wanted.
