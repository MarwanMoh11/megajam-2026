# Status

Last updated: 2026-11-13, about 13:00 Cairo. Theme: **Director's Cut**.

**The game: a heist shot in one take** (working title *Print It*). Marwan wrote the pitch on Fri Nov 13; it is kept
word for word in `docs/ideas/heist-pitch.md` and replaces K. Three judge agents scored it twice, and this spec answers
both rounds of their notes (`docs/ideas/heist-judging.md`). K's spec is archived in
`docs/ideas/k-meanwhile-spec.md`, and nothing in `prototypes/k-3d/NOTES.md` applies any more. `docs/ART_STYLE.md` was
written for K: only the parts named under "Look" still apply, and where it differs from this file, this file wins.

State: the Unreal project `Reel` exists on the PC (Third Person C++ template). Stage 3 proved the pipeline: a
Windows package in 3.6 min, a 343 MB zip, and a Web build that boots. There is no game code yet. Next: step 1 of the
build order.

## The pitch

Tonight the Aurelia Auction House sells the only print of a lost film: the **Director's Cut**. Three thieves are
going to steal it, and you are the director, shooting the heist live, in one take, on three cameras:

- **Camera A, Julian:** the con man in a white dinner jacket on the gala floor. He can only walk.
- **Camera B, Maya:** the lighting tech at her post on the catwalk above the hall.
- **Camera C, Vex:** props and sound, at his post backstage.

The take never stops. You **cut** between the cameras (keys 1, 2, 3) to fire the set's own equipment: spotlights,
banners, carts, feedback, pyro and the curtain. Each one opens a few seconds for Julian to slip past the guards, lift
the keycard and take the reel.

- **Caught:** it's a **blooper**. A freeze, a slate, and back to the top of the scene in two seconds.
- **Out with the reel:** the clapper snaps, the screen says **PRINT IT**, and the end card shows **your director's
  cut**, a film strip of every cut you called.

**How to play**, on screen for the first 10 s: **"1 2 3: cut between your crew. Aim + E: fire the set. Past the red
tape, stay out of the red cones."**

**The theme lands three ways:**
- You direct and cut the take.
- The heist is a take, with bloopers.
- The thing you steal is the Director's Cut.

## Rules

1. **Three cameras, one take.** Keys 1, 2 and 3 cut to that camera on the same frame.
   - A cut is a hard cut, a switcher click, a slide-in camera label, and a tone change.
   - The character you leave freezes in place.
   - The world never pauses, except for the short black-and-white freezes (the first cut, a blooper, the finish).

   Each camera has its own lens:

   | Camera | Lens |
   |---|---|
   | **A** | Low follow shot behind Julian, FOV 50 |
   | **B** | High and wide from the catwalk, FOV 75 |
   | **C** | Handheld, FOV 60, with a slight idle camera shake |

2. **Julian can only walk:** 250 cm/s, no running, crouching or jumping. **E** interacts: hit a mark, lift, open the
   case, bow.
3. **Maya and Vex never walk.** Each stands at one post with every device within reach.
   - The mouse aims.
   - Whatever is under the crosshair highlights, showing its range ring and a marker over each watcher it would hit.
   - **E** fires it.
   - A crew cut is about 2 s: cut, aim, E.
4. **Composure** (0-100) is the only fail meter.

   Julian is **out of character** when:
   - he stands past the **red tape** (a restricted zone taped on the floor: past the rope, on the dais, behind Vance);
   - he holds the keycard or the reel;
   - he crosses an infrared beam.

   | What happens | Effect | Label on the bar |
   |---|---|---|
   | A watcher sees him while he's out of character | A red "?" for 0.5 s, then -30 a second | **SEEN** |
   | He crosses an infrared beam | -50 at once, and the nearest guard turns | **BEAM** |
   | You cut away while he stands off a mark | -2 a second: an actor frozen on camera | **OFF MARK** |
   | He stands on a **mark** (a taped gold X by the champagne, a painting or a pillar) | +15 a second, and no drain while you're cut away | **ON MARK** |
   | A clean lift | +25 | |
   | Composure reaches 0 | Blooper | |

   The habit the game teaches: hit your mark, then cut.
5. **Watchers** see in a cone drawn on the floor (red, 30% opacity), with a line trace that walls, banners and the
   curtain block.
   - Guards and bodyguards: plain C++ state machines on waypoints.
   - The iris scanner.
   - The broadcast drone, on a spline.
   - Vance only notices a lift when he isn't speaking.

   The mannequins have no reaction animations. Reactions are an icon over the head (eyes shut, ears covered, "?",
   "!") plus the cone fading or turning.
6. **Devices** are the set's equipment, and each has one verb. A verb works on any watcher in range, so every obstacle
   has at least two answers.

   | Verb | What it does to watchers in range |
   |---|---|
   | **Blind** | Their sight goes off for the duration. |
   | **Lure** | They walk to the device's point, look at it, then walk back. |
   | **Block** | Solid geometry that stops sight and infrared beams. |

7. **Match cut.** Cut to Julian within 1.5 s of a crew device landing:
   - a "MATCH CUT" stamp;
   - a gold frame on the next still;
   - one more on the end card's count.

   Fast cross-cutting is the skill the grade rewards.
8. **The bluff.** The first time in a scene that a guard sees Julian out of character for 0.5 s, the drain stops.
   The guard walks up and asks a question, voiced and subtitled. Julian turns to face him, and three answers appear.
   - **Picking:** the mouse moves the highlight and E picks, within 6 s. Keys 1, 2 and 3 still cut, so you can set
     something up while he waits.
   - **The answers:** one is always plainly wrong, and funny. Of the other two, the right one is written on a prop
     next to Maya's or Vex's post (the seating plan, the guest list). Once a crew camera has seen it, that answer gets
     a small camera icon.
   - **Right:** "Stay here." The guard walks off to radio it in, which gives a 12 s window.
   - **Wrong, or no answer:** -25, and the guard keeps watching.
9. **Blooper.**
   - **The trigger:** composure hits 0, or the finale clock runs out.
   - **The sequence:**
     1. The frame freezes in black and white for 1 s. The music tape-stops, the clapper clacks, a red slate reads
        "CUT! BLOOPER - TAKE N", and the director's voice says "Cut!".
     2. A rewind whirr plays over the restore, and the director says "Action!".
   - **The restore** reloads the level at the current scene. The scene index and what's done so far (the scanner
     passed, the keycard taken) live in the GameInstance, Unreal's object that survives a level load. Each scene
     start puts every actor in a known state, so nothing has to remember the past.
   - **After two bloopers in a scene,** the crew member who holds the answer hints at it over the earpiece.
10. **The finish.**
    - A snap to black, the clapper clacks, the **PRINT IT** slate, and the music sting.
    - Then the end card, "YOUR DIRECTOR'S CUT": the strip of stills, then TAKES, CUTS, MATCH CUTS and TIME, then the
      cut you earned:

      | Cut | When |
      |---|---|
      | **DIRECTOR'S CUT** (gold) | 1 take and at least 4 match cuts |
      | **FINAL CUT** | 3 takes or fewer |
      | **ROUGH CUT** | Finished |

    - The stills are grabbed at each cut: a SceneCapture2D set to Final Color (LDR) renders into one of 12 render
      targets of 256x144 each. Match cuts are kept first. The fallback, if this slips, is 6 stills grabbed at fixed
      moments in the scenes.
    - R reshoots from the opening. ESC quits.

## The take

One continuous take of about 4 minutes, in four scenes. Each scene adds one character or one constraint. The gold
case with the reel sits on the dais and is in the very first shot.

| Scene | Julian's goal | Watchers | Answers (at least two) | Teaches |
|---|---|---|---|---|
| **1. The Door** (about 30 s) | Get through the iris scanner. The queue is a mark that moves: it carries Julian to the scanner in 25 s, even while you're cut away, with no drain | The scanner and the Security Chief | Maya's follow spot on the scanner (Blind). Maya's entrance breaker (Blind). Vex's fuse box for the scanner | Cutting. At 0:02, "[2] MAYA" flashes. The first cut gets a 0.6 s black-and-white freeze and the "SCENE 1 - TAKE 1" clapper. Maya's post faces the scanner, so E lands the spot within 10 s of the start and the Chief yells "Who turned that on?!" Then "[1] JULIAN" flashes, and cutting back lands the first MATCH CUT. When the scanner faults, the Chief says "Wave them through!" |
| **2. The Rope** (60-90 s) | Get onto the dais, past a velvet rope with an infrared beam | A patrol guard, and the drone (one pass every 20 s) | **The beam:** Maya's banner (Block), or Vex's fuse box (8 s off). **The guard:** Vex's cart (Lure), Maya's spot (Blind), or the bluff | Two characters, in any order. Marks |
| **4. The Getaway** (15 s clock) | Open the case and take the reel. The alarm sounds, the shutters fall, four guards walk in from both sides, and the drone locks on. Walk 10 m to the hatch under the stage | Everyone | Two things are needed within the 15 s, in either order. **Blind:** Maya's pyro (everything for 6 s), or the spot plus the feedback. **Block:** Vex's winch (the curtain drops, blocks both sides and opens the hatch) | Three cameras under a clock. Built before Scene 3 (see the build order) |
| **3. The Lift** (60-90 s) | Take the keycard from Vance's belt: hold E for 1.5 s from behind him | Two bodyguards facing outward, the drone, and Vance | Vance gives a 10 s speech every 30 s, and the spot on him starts one early. **During a speech:** Vex's feedback (Blind on everyone in the hall for 4 s), then lift. **Or:** the dais breaker (cones at half range for 6 s), plus the spot on one bodyguard | Two devices in order, inside a window |

Scenes play 1, 2, 3, 4. The table lists 4 before 3 because it is built first. At the hatch, E makes Julian **bow**
to the blinded drone. That earns a "THE BOW" stamp and its own still. The bow is faked with a Transform (Modify) Bone
node on the spine in the animation Blueprint, since the mannequin has no bow animation.

## Devices

| Device | Who | Verb | Range, duration | Recharge |
|---|---|---|---|---|
| Follow spot: E with the crosshair on a watcher; it snaps to the nearest one in a 10° cone | Maya | Blind one watcher | 35 m, 4 s | 5 s |
| Banner drops, 2 fixed | Maya | Block | 10 s, then it winches up | 20 s |
| Breaker panel: entrance, floor, dais | Maya | Cones at half range in that zone | 6 s | 15 s |
| Pyro switch (finale only) | Maya | Blind everything in the hall, with sparks and smoke | 6 s | Once |
| Champagne carts at 2 service doors (left, right) | Vex | Lure guards within 12 m to the crash | 6 s | Once per scene |
| Fuse box: scanner or infrared beam | Vex | Off | 8 s | 15 s |
| Sound desk: feedback | Vex | Blind everyone in the hall | 4 s | 15 s |
| Counterweight winch (finale only) | Vex | Block: the curtain drops for good and the hatch opens | Permanent | Once |

All values are `UPROPERTY(EditAnywhere)` on the device classes.

## Controls

| Key | What |
|---|---|
| 1, 2, 3 | Cut to Julian, Maya, Vex |
| WASD | Julian walks |
| Mouse | Julian looks around. Maya and Vex aim. During a bluff, moves the highlight |
| E | Fire the device under the crosshair. Hit a mark, lift, open the case, bow. Pick a bluff answer |
| R | Reshoot from the opening |
| M | Mute |
| ESC | Quit |

## Camera and HUD

- Each character has a spring-arm camera with the lens from rule 1. A cut swaps possession (which character the
  player controls) on the same frame: no camera flight and no slow motion.
- **Exposure is manual** on every camera, so a cut changes the brightness on the same frame with no auto-exposure
  ramp.
- **The viewfinder HUD:**
  - Top left: "CAM A - JULIAN" (or B or C), with the composure bar under it (always Julian's) and the label from
    rule 4.
  - Top right: a red REC dot and the take's timecode.
  - Safe-frame corners, and the device recharge rings.
- **Julian's feed:** on cameras B and C, a corner inset shows camera A at 480x270. It is a SceneCapture2D at 15 fps,
  set to Final Color (LDR), with his composure bar. Julian is always on screen, and cause and effect stay visible.

## The set

One level and one hall, about 30 x 20 m, built as a film set. Three walls are flats; the fourth side opens onto a dark
studio with light stands and cables. The hall has three layers, and each can see the others:

1. **The floor** (camera A, warm amber):
   - The entrance and the iris scanner at one end.
   - Guests in the middle, posed as static mannequins with no AI.
   - The champagne bar, paintings and pillars, each with a gold mark.
   - At the far end, the dais: Vance's podium, the gold glass case with the reel, the velvet rope with its infrared
     beam, red tape, and the hatch under the stage.
2. **The catwalk** (camera B, night blue): about 6 m up, along one long side and across the stage end. It looks down
   on everything, so this is where the player reads the cones. Maya's post has the follow spot, both banner lines,
   the breakers, the pyro switch and the seating plan.
3. **Backstage** (camera C, fluorescent green): a corridor behind the long wall and the stage. It sees the floor
   through two service doors and a gauze scrim, and the backs of the flats show their bracing. Vex's post has the
   cart releases, the fuse box, the sound desk, the winch and the guest list.

The broadcast drone flies a spline loop over the floor with its red tally light on.

## Look

The grey box is the style: **a soundstage in rehearsal**.
- The mannequins are the actors' stand-ins.
- The floor has taped marks and the flats have stencilled labels.
- Hard movable spotlights shine through haze.

It looks deliberate from step 2 on, and every later step only adds to it.

- **From ART_STYLE.md, still in force:**
  - §1 rule 1: Julian's white jacket is the lightest thing on screen, and the guards are the darkest.
  - §1 rule 2: red only for danger (cones, tape, the tally light, the alarm). Gold for Julian's marks, the reel,
    stamps and the DIRECTOR'S CUT grade.
  - §1 rule 3, with this hall's tones: the floor amber, the catwalk blue, backstage green. Each tone comes from a
    per-camera colour grade, and a cut changes it on the same frame.
  - §1 rule 4, and §1 rule 5 (no Lumen, no Nanite).
  - §8: the film overlay as a UI widget, with a 2.39:1 letterbox in place of the 4:3 gate.
  - §14: the checks.
- **Lighting:**
  - Movable spot lights with volumetric fog for visible beams. These work without Lumen.
  - At most about 4 shadow-casting spots on screen at once.
  - A dim skylight, so anything outside a spot doesn't go black. Without Lumen there is no bounce light.
- **Characters:** template mannequins tinted by role, with simple props: a bow tie, a headset, a cap.
- **Generated models:** for props only.

## Audio

- **Voice, ElevenLabs:** about 60 short lines, all subtitled.
  - The director: "Action!", "Cut!", "Print it!"
  - Julian's quips.
  - Maya and Vex on the earpiece when a device fires ("Lights!", "Cart's away!"), plus the hints after two bloopers.
  - Vance's speech.
  - The Chief and the guards: 4 bluff questions with 3 answers each, and both reactions.
- **Sound effects, ElevenLabs:**
  - A soft switcher click on every cut.
  - The clapper: only at the start of a take, a blooper, and PRINT IT.
  - The blooper buzzer, the tape-stop, and the rewind whirr.
  - The feedback screech, breaking glass, the banner drop, the curtain thud and the sparks.
  - The alarm and the shutters.
  - A heartbeat under 30 composure.
- **Music:** one swing or jazz heist track that never restarts across cuts. It carries the single take as sound.
  - Mixed per camera: open on the floor, high and airy on the catwalk, low-passed backstage.
  - Ducked under every voice line.
  - A faster cue crossfades in on the alarm frame, and a sting plays for PRINT IT.
  - Source: WeLoveIndies or ElevenLabs music.
- **Credits:** every tool and library goes in `docs/CREDITS.md`.
- **API key:** Marwan sets `ELEVENLABS_API_KEY` on the PC themself (README, "The game"). Agents never type it or
  print it.
- **Until step 6:** subtitles and engine sounds stand in.

## Tick order

The rules live in one director actor, `ATakeDirector`, which ticks after the player controller:

1. **Presses.** A cut first: swap possession, grab a still, start the match-cut check. Then E and the mouse.
2. **Devices.** A device that fires this frame applies its verb to every watcher in range this frame and records
   when it landed.
3. **Watchers.** Sight tests against Julian: cone, range, then a line trace.
4. **Composure.** The drains and gains from rule 4, and the bluff trigger.
5. **The scene.** Goal checks and the finale clock.
6. **End of frame** (the director's late tick): the blooper check, then the finish check.

## Events and feedback (all on the frame of the event)

| Event | Feedback |
|---|---|
| Cut | Hard cut, switcher click, the camera label slides in, the lens and tone change, a still is grabbed |
| First cut of the take | 0.6 s black-and-white freeze, the "SCENE 1 - TAKE 1" clapper |
| Crosshair on a device | Its range ring, and a marker over each watcher it would hit |
| Device fires | The crew's line, its sound, and the effect starting in the hall |
| Match cut | "MATCH CUT" stamp, and a gold frame on the inset |
| Watcher notices Julian | A red "?" over the watcher, the cone turns solid red, and the heartbeat starts |
| Composure drains | The bar flashes red with its cause (SEEN, BEAM, OFF MARK), and the music low-passes as it falls |
| Hit a mark | A gold X under Julian, and ON MARK on the bar |
| Lure lands | The guards turn their heads, then walk to the point |
| Blind lands | The cone fades out, with an eyes-shut or ears-covered icon |
| Block lands | A thud and dust |
| Bluff | The guard walks up, the question is voiced and subtitled, and three answers appear with a 6 s bar |
| Lift | A 1.5 s ring around Julian, then a keycard chime and +25 |
| Case opened | Alarm, red strobes, the shutters start to fall, the 15 s clock, the faster music cue |
| Blooper | Black-and-white freeze, tape-stop, clapper, red slate, buzzer, "Cut!", the rewind whirr, "Action!" |
| Two bloopers in a scene | The crew's hint over the earpiece |
| Bow | Julian bows, a "THE BOW" stamp, a still |
| Finish | Snap to black, the clapper, PRINT IT, the music sting |
| End card | The strip slides in still by still, the stats count up, and the cut you earned stamps last |

## Kill list (final)

- **A live AI interrogation.**
  - An API key inside the build gets extracted.
  - Offline judges get silence.
  - The latency breaks the same-frame rule.

  The bluff is pre-written and pre-voiced instead.
- **Camera tricks:** the character wheel, camera flights between characters, slow motion on cuts.
- **Crew walking:** Maya and Vex stay at their posts.
- **More space:** a separate lobby (the entrance is one end of the hall), more than one level, more than one hall.
- **Animation and physics:**
  - Crowd AI, guests that move, tackles.
  - Physics destruction: the carts and banners move on timelines.
  - Reaction animations: icons instead.
- **AI tooling:** behaviour trees and AI perception. Watchers are plain C++ with cones and line traces.
- **Replays:** a true rewind (the restore is a level reload with a slate) and a full replay (only the strip of
  stills).
- **Generated or retargeted character models.**
- **More story:** dialogue beyond the 4 bluffs, the hints and the barks. No cutscenes beyond the short freezes and
  the end card.
- **Menus and systems:** saves, any menu beyond the title card and the end card, settings, difficulty modes.
- **Lumen and Nanite.**
- **If time:**
  - A replay of the finale with the player's own cuts. Only if gate 2 passes by Sat 20:00.
  - A smoke machine for Vex (Blind in a zone).
  - A 0.3 s slow motion on a match cut.

## Build order

Core loop in a test box first, then the set with its basic look, then the scenes, the ending first. Every step ends
playable, and the Windows package is rebuilt every night (Fri and Sat).

| When (Cairo) | Step | Done when |
|---|---|---|
| Fri, by 11:45 | **0. Pipeline** | Done |
| Fri 13:00-21:30 | **1. Core loop in a test box.** Start with one header pass that declares every new class and `UPROPERTY`, so the editor restarts happen together. Three characters and the cut: possession, label, lens. Julian's walk, marks, red tape and composure. Maya's post with crosshair aiming. One patrol guard, whose cone is a flat mesh that ignores walls for now, with Lure and Blind states. Three devices: the follow spot (auto-targets the nearest watcher), a cart and a banner. A velvet rope with its infrared beam. A blooper that reloads the level | In play-in-editor, Julian gets past the guard and the beam in two different ways |
| Fri about 22:00 | **Gate 1.** Marwan plays for 5 minutes. If they don't want another go, change a rule (drain rates, windows, the cost of cutting away), not the art | A yes, or the rule changed |
| Fri night to Sat morning | **2. The set, its basic look, Scenes 1 and 2.** The hall's blockout on three layers. Spots with haze, the dim skylight, manual exposure, per-camera grades, and the overlay with letterbox and grain. Cones with line traces. Julian's inset. Still grabs. The restore at the current scene. The queue and the scanner, the breakers and the fuse box, the drone on its spline, and the bluff (UI and placeholder text). Package | Scenes 1-2 play in the package and screenshots look like the soundstage |
| Sat (midterm 12:30) | **3. Scene 4 first, then Scene 3, then the ends.** Pyro, the winch, the curtain, the hatch, the shutters, the clock and the bow. Then Vance and the bodyguards, the sound desk and the lift. The title card, the end card with the strip, and R. If Saturday slips, Scene 3 shrinks to one device and the ending stays reachable. Package | Title card to end card in the package |
| Sat evening | **Gate 2.** Marwan plays a full take cold | They want another go |
| Sat evening and night | **4. Look.** Soundstage dressing (flats, stencils, light stands, cables), the black-and-white freezes, the viewfinder HUD, stamps | Screenshots pass ART_STYLE §14 |
| Sat night to Sun 10:00 | **5. Characters and props.** Tinted mannequins with props, and the bow. Fab free packs, or generated props (Tripo or Rodin) if the codes arrive. Niagara (Unreal's particle system) sparks and glass | Credited in docs/CREDITS.md |
| Fri night to Sun 10:00, in parallel | **6. Audio.** A separate session, outside the editor. Write the lines Fri night, then generate them with ElevenLabs once Marwan has set the key, and pick the music. The editor session imports everything by Sun 10:00 | Content lock |
| Sun 10:00-18:00 | **7. Polish.** The if-time list, a mix pass, a bug bash, the 30-60 s video, the itch page, the dev log. The video opens on the first match cut (the spot slams the scanner, cut, Julian walks through, MATCH CUT), with the finale second | Nothing new after 18:00 |
| **Sun 22:00** | **Upload and submit.** Then one try at the Web build (proven on the template, 23 min to package) as a second zip that plays in the browser | Downloaded and played once |

## Decided Nov 13

1. The heist replaces K (Marwan's pitch).
2. The bluff is pre-written and pre-voiced; there is no AI at runtime.
3. Hard cuts on 1, 2 and 3. The world never pauses except for the short freezes.
4. Maya and Vex stay at their posts.
5. One hall on three layers. The lobby is its entrance.
6. The look is a soundstage in rehearsal, so the grey box is the style.
7. ElevenLabs for voice and sound effects.
8. Solo, with Claude agents (the Solo Driver prize).

## Not decided yet

- **Game and team name.** The zip must be `TeamName_GameName.zip`. Suggested: *Print It*.
- **Music source**, before step 6: WeLoveIndies or ElevenLabs music. Swing or jazz heist either way.
- **Student category:** the Official Rules are still unread.
- **Perk codes:** none arrived. Retry Tripo before Nov 16; ask the organisers for late-joiner codes if wanted.
