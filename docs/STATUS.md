# Status

Last updated: 2026-11-13, about 14:30 Cairo. Theme: **Director's Cut**.

**The game: a heist shot in one take** (working title *Print It*). Marwan wrote the pitch on Fri Nov 13; it is kept
word for word in `docs/ideas/heist-pitch.md` and replaces K. Judge agents scored the pitch round by round, and this
spec answers every round (`docs/ideas/heist-judging.md`). From round 3, Marwan asked for the design to be scored on
its own, with execution set aside ("I will do the wonders"). So the scope here is the full design, and the build order
ends with a cut order in case Saturday slips.

K's spec is archived in `docs/ideas/k-meanwhile-spec.md`, and nothing in `prototypes/k-3d/NOTES.md` applies any more.
`docs/ART_STYLE.md` was written for K: only the parts named under "Look" apply, and where it differs from this file,
this file wins.

State: the Unreal project `Reel` exists on the PC (Third Person C++ template). Stage 3 proved the pipeline: a Windows
package in 3.6 min, a 343 MB zip, and a Web build that boots. There is no game code yet. Next: build order step 1.

## The pitch

Tonight the Aurelia Auction House sells the only print of a lost film: the **Director's Cut**. A film crew is going to
steal it, and you are their director, shooting the heist live, in one take, on three cameras:

| Camera | Who | Where |
|---|---|---|
| **A** | **Julian**, the lead, in a white dinner jacket | The gala floor. He can only walk |
| **B** | **Maya**, the gaffer (the lighting chief) | Her post on the catwalk above the hall |
| **C** | **Vex**, the foley artist and prop master | His post backstage |

You do what a director does: you **cue** Julian to his marks (blocking), **cut** between the cameras, and **fire the
set**, meaning the spotlights, banners, carts, the band's PA, the pyro and the curtain. The take never stops. Julian
walks his blocking whether you're watching or not, and the whole room keeps time to the band on the floor. The skill
is landing the right effect on the beat he arrives.

- **Caught:** a blooper. The frame burns, the slate comes up, and you pick up from the last checkpoint.
- **Out with the reel:** the clapper snaps and the screen says **PRINT IT**. The game then plays **your director's
  cut**: your take as one unbroken film, cut where you cut. You can re-cut it live.

**How to play.** The prompts follow the scenes:
- Scene 1 shows **"1 2 3: cut. E: fire the set."**
- Scene 2 adds **"Q: send Julian to a mark. Past the red tape, stay out of their frames."**

**The theme lands four ways:**
1. You direct: you cue the actor, call the cuts, fire the effects, and edit the result.
2. The heist is a take, with bloopers, pick-ups and a long take.
3. The thing you steal is the Director's Cut.
4. Each scene hides a **deleted scene** to restore. Only a take with all four restored earns the grade DIRECTOR'S CUT.

## Rules

### 1. Three cameras: the film and the making-of

Keys 1, 2 and 3 cut to that camera on the same frame: a hard cut, a switcher click, and the change of look. The world
never pauses, except for the 1 s blooper freeze.

| Camera | Lens | Look |
|---|---|---|
| **A, "the film"** | Low follow shot behind Julian, FOV 50 | 2.39:1 letterbox. Noir: high-contrast monochrome, every figure rim-lit into a silhouette, and only red and gold in colour. The cast wear their costumes (top hats, gowns, guards' caps), and the set shows its dressed fronts. Grain. The HUD lives in the letterbox bars |
| **B, "the making-of"** | High and wide from the catwalk, FOV 75 | A 16:9 video tap (the on-set monitor feed): washed-out colour, a timecode burned into the picture, faint scanlines. The same bodies as bare stand-in mannequins with tape name labels ("GUARD 2"), and the raw backs of the flats, light stands and cables |
| **C, "the making-of"** | Handheld, FOV 60, with a slight idle camera shake | The same video-tap look as B |

Every cut flips between the movie and the making of it. The costumes and the tape labels are the same actors' parts:
costumes are seen only from camera A, labels only from B and C.

### 2. The band's clock

- A jazz quintet on the floor plays at a fixed 120 BPM, so 1 bar is 2 s.
- The world keeps time to it:
  - Patrols turn on bar lines.
  - The drone passes each zone every 8 bars.
  - Vance speaks on the band's breaks.
  - The faulty infrared beam flickers off on one beat in every 4 bars.
- **The clock is visible:**
  - Every watcher's frame pulses on each beat, and shows a ghost of its next turn one bar ahead.
  - Julian's path shows his arrival count ("on his mark in 6").
- **The clock never stops.**
  - The feedback knocks the band out for one bar, and they come back in on the next.
  - The alarm switches to the heist cue at the same tempo.
- **Pick-ups:** every patrol, drone pass and speech is a function of the bar count. A pick-up sets the clock back to
  the bar saved at the checkpoint and seeks the music there, so the world is exactly where it was.

### 3. Blocking

- **Q**, with the crosshair on a mark, **cues** Julian there. It works from any camera.
- He walks the shortest path at 250 cm/s on his own, whether or not you're on camera A, and stops on the mark.
- On B and C his path is a dotted gold line with the beat count.
- A new cue replaces the old one.
- On camera A, WASD also walks him directly, and E interacts: lift, open the case, bow, answer a bluff.
- He never runs, crouches or jumps.

### 4. Maya and Vex never walk

- Each stands at one post with every device within reach.
- The mouse aims, and whatever is under the crosshair highlights, with its range ring and a marker over each watcher
  it would catch.
- **E** fires it.
- A crew cut takes about 2 s: cut, aim, E.

### 5. Composure (0-100)

The only fail meter. On camera A it is a thin gold line in the bottom letterbox bar; on B and C it sits under
Julian's inset.

Julian is **out of character** when:
- he stands past the **red tape** (taped on the floor: past the rope, on the dais, behind Vance);
- he holds the keycard or the reel;
- he crosses a live infrared beam.

| Case | Change | Label |
|---|---|---|
| A watcher has him in frame while he's out of character | 0.5 s grace, then -30 a second | **SEEN** |
| The drone has him in frame while he's **off a mark**, anywhere on the floor (on the broadcast, an actor off his mark is a mistake) | The same | **ON AIR** |
| He crosses a live infrared beam | -50 at once, and the nearest guard turns | **BEAM** |
| He's off a mark, not walking a cue, and you're on another camera | -2 a second | **OFF MARK** |
| He's on a mark (a gold X: by the champagne, a painting, a pillar, the band) | No drain. +15 a second only while you're on camera A | **ON MARK** |
| A clean lift | +25 | |
| Composure reaches 0 | Blooper | |

Crossing the gala means hopping from mark to mark between the drone's passes. As composure falls, camera A's picture
burns in from the edges.

### 6. Watchers

- **Sight:** a watcher sees with a **camera frame**: thin red frustum lines (the pyramid of what a camera sees), bold
  on B and C and faint on A. A line trace lets walls, banners and the curtain block it.
- **Alert:** every watcher carries a small red **tally light**, like a camera's. It goes live the moment they frame
  Julian. On A, the tally light and the frame turning solid are the only alert. On B and C, the watcher's tape label
  shows its state ("GUARD 2 - LURED").
- **No reaction animations:** frames dim, swing and turn instead.

| Watcher | Behaviour |
|---|---|
| Guards and bodyguards | Plain C++ state machines on waypoints, turning on bar lines |
| The iris scanner | Fails Julian if it is live when he reaches it |
| The broadcast drone | A spline loop, one pass per zone every 8 bars |
| Vance, the host | Watches only his own belt. He notices a lift unless he is speaking, lured or blinded |

### 7. Devices: the cue sheet

The set's equipment. Each device has one verb and a fixed number of uses for the whole take, shown on the **cue sheet**
on B and C. A verb works on any watcher in range.

| Verb | Effect |
|---|---|
| **Blind** | The frame goes dark for the duration |
| **Lure** | The watcher walks to the device's point, looks at it, and walks back |
| **Mask** | Solid geometry (a matte) that stops frames and infrared beams |

When one firing catches two watchers it stamps **TWO-SHOT**; three or more stamp **WIDE SHOT**.

**Spending uses costs the grade and the deleted scenes, never the ending.**
- Every scene keeps an answer outside the budget (marked "free" in the scene table).
- The pyro is locked until the alarm.
- The reversal never takes the last Blind.

### 8. Cuts that count

- **CUT ON ACTION:** you cut to camera A within 1.5 s of a crew device landing, while the effect is inside Julian's
  frame (on screen and unhidden). The next still gets a gold frame.
- **ON THE BEAT:** a cut that lands within 0.12 s of the downbeat (editors cut on the beat). It earns a small tick.

### 9. The bluff

The first time in a scene that a guard frames Julian out of character for 0.5 s:

1. The drain stops, and the guard walks up and asks a question, voiced and subtitled.
2. The answer is written on a prop at Maya's or Vex's post: the seating plan ("Which table?") or the guest list ("Who's
   your host?"). The prop glows while the bluff is live.
3. You have 8 s. Julian can stall once ("One moment...") for 4 s more.
4. Three plausible answers appear on camera A. The one you've read on a crew camera carries a camera icon. The mouse
   moves the highlight and E picks; this works on camera A only.

| Answer | Result |
|---|---|
| Right | "Stay here." The guard walks off to radio it in, and you get a 12 s window |
| Wrong, or none | -25, and he keeps watching |

You never have to guess: the skill is cutting to read the answer in time.

### 10. Pick-ups and takes

**A blooper:** composure at 0, the scanner failing Julian, or the finale clock running out.

1. The frame freezes for 1 s and the film burns through from the centre.
2. The music tape-stops and the clapper clacks.
3. A red slate reads **"CUT! - PICK-UP"** with the scene, and the director says "Cut!".
4. A rewind whirr plays while the level reloads at the last checkpoint, and the director says "Action!".

**Checkpoints:** the start of each scene, and the rope once Julian is past it.

**What survives the reload:** the GameInstance, Unreal's object that outlives a level load, keeps:
- the checkpoint;
- its bar;
- the cue sheet's counts;
- what's done so far;
- the recording.

**Hints:** after two pick-ups at one checkpoint, the crew member who holds the answer hints at it over the earpiece.

**R** starts a new **take** from the opening, and the slate reads "TAKE N".

### 11. Deleted scenes

Each is a harder, optional answer. Restoring one stamps "DELETED SCENE RESTORED", grabs its own still, and the director
says "Keep that in!":

| Scene | Deleted scene | How |
|---|---|---|
| 1 | **The Wrong Eye** | Before Julian reaches the scanner, Vex swaps the registry photo at the backstage terminal. The live scanner says "Welcome, Lord Sterling", and the Chief salutes |
| 2 | **The Long Take** | Julian crosses the rope with no device fired since the scene began. He threads the patrol's bars, the drone's passes and the beam's beat |
| 2 | **The Two-Shot** | The lift, made while both bodyguards are caught by one device firing |
| 3 | **The Bow** | At either exit, E: Julian bows to the blinded drone |

### 12. The finish

1. **The close:** a snap to black, the clapper, the **PRINT IT** slate, and the sting.
2. **YOUR DIRECTOR'S CUT** (about 60 s; Space skips).
   - **One unbroken take:** the recording, with everything before each pick-up's restore point dropped. It plays as a
     single take, and the bloopers go to their own reel.
   - **Your edit:** cut on every cut you called. Holds longer than 4 s are trimmed, and every trim and cut lands on a
     bar line.
   - **The real score:** the band, then the heist cue.
   - **All in the film look,** with B's and C's angles re-rendered in it.
   - **Restored deleted scenes** carry a "RESTORED" tag.
   - **Recording:** every moving actor's transform at 20 Hz, plus device events, cues and the cut list.
3. **The edit suite.** While it plays, 1, 2, 3 and 4 (4 is the drone's broadcast angle) take over the cut live, like
   a vision mixer. Your live cut replaces the automatic one from that moment.
4. **The contact sheet:** up to 12 stills grabbed at your cuts. Each is a SceneCapture2D set to Final Color (LDR),
   rendering into one of 12 render targets at 256x144.
5. **The stats:** PICK-UPS, CUTS, CUTS ON ACTION, ON THE BEAT, TWO-SHOTS, DELETED SCENES n/4, TIME.
6. **The grade:**

   | Cut | When |
   |---|---|
   | **DIRECTOR'S CUT** (gold) | No pick-ups and all 4 deleted scenes |
   | **FINAL CUT** | 2 pick-ups or fewer |
   | **ROUGH CUT** | Finished |

7. **The blooper reel under the credits:** every pick-up's freeze frame with its slate.

R shoots a new take; ESC quits.

## The take

One continuous take, about 2.5 minutes when clean. Scenes are checkpoints in one continuous space, not gates. The gold
case with the reel is on the dais in the very first shot.

| Scene | Julian's goal | Watchers | Answers | What it adds |
|---|---|---|---|---|
| **1. The Door** (about 30 s) | Get through the iris scanner. The queue is a moving mark: it carries him there in 25 s whether you're watching or not. If the scanner is live when he arrives: "NO MATCH", a blooper | The scanner, the Security Chief | **Free:** Vex's scanner fuse. **Blind** the scanner: Maya's spot or the entrance breaker. **Deleted scene:** The Wrong Eye | **Cutting.** Maya's post faces the scanner. At 0:02 "[2] MAYA" flashes; E lands the spot inside 10 s and the Chief yells "Who turned that on?!" Then "[1] JULIAN", and cutting back lands the first CUT ON ACTION |
| **2. The Floor** (about 90 s) | Cross the floor, past the rope onto the dais (a checkpoint), then take the keycard from Vance's belt: hold E for 1.5 s from behind him | The drone, a patrol guard, the rope's infrared beam, two bodyguards facing outward, Vance | **The rope:** **free:** the bluff, or the side route (Vex unlocks the side door; a passage along the wall skips the rope and comes out inside the bodyguards' frames). The beam: Maya's banner (Mask) or Vex's beam fuse. The guard: a cart (Lure) or the spot (Blind). **The lift:** Vance must be speaking. He speaks for 5 bars on every band break (every 16 bars), and the spot on him starts a speech at once. During a speech, blind the bodyguards with feedback, or halve their frames with the dais breaker and come in from the side. **Deleted scenes:** The Long Take, The Two-Shot | **Blocking.** It opens with a guided win inside 60 s: "[Q] cue Julian to the champagne", then "[2] spot the guard as he passes", then CUT ON ACTION. **The reversal:** when Julian first crosses onto the dais, the device you've fired most breaks for the rest of the take ("Bulb's blown!", "Cart's jammed!"). It is struck through on the cue sheet, which forces an answer you skipped |
| **3. The Getaway** (16 s: the last 8 bars) | Open the case and take the reel. The alarm stops the band dead, the shutters fall, four guards walk in from both sides, and the drone locks on. Reach an exit | Everyone | **The hatch** under the stage (10 m): **free:** Vex's winch (the curtain drops for good, Masks both flanks and opens the hatch) and Maya's pyro (Blinds everything for 6 s). **The side passage** past the band: needs the side door open, plus a Lure or a Blind from what you saved for the two guards on that flank. **Deleted scene:** The Bow | **Camera A bleeds to full colour** for the last 8 bars: the alarm's red, the pyro's gold, every colour at once |

## Devices: the cue sheet

| Device | Who | Verb | Range, duration | Uses per take |
|---|---|---|---|---|
| Follow spot: snaps to the nearest watcher within 10° of the crosshair. On Vance it also starts a speech | Maya | Blind one watcher | 35 m, 4 s | 3 (bulbs) |
| Banner drops | Maya | Mask | 10 s, then it winches up | 2 (one each) |
| Breakers: entrance, floor, dais | Maya | Halve the frames in that zone | 6 s | 3 (one each) |
| Pyro (locked until the alarm) | Maya | Blind everything in the hall, with sparks and smoke | 6 s | 1, guaranteed |
| Champagne carts at 2 service doors: one mid-floor, one by the dais | Vex | Lure watchers within 12 m to the crash | 6 s | 2 (one each) |
| Fuse box: the scanner circuit | Vex | Scanner off | 8 s | Free |
| Fuse box: the beam circuit | Vex | Infrared beam off | 8 s | 2 |
| Sound desk: feedback through the band's PA | Vex | Blind everyone in the hall | 4 s | 2 |
| Side door lock | Vex | Opens the side route and the side passage | Permanent | Free, once |
| Registry terminal | Vex | Swaps the registry photo (The Wrong Eye) | Until the scanner | 1 |
| Counterweight winch (finale only) | Vex | Mask: the curtain drops for good and the hatch opens | Permanent | 1, guaranteed |

All values are `UPROPERTY(EditAnywhere)` on the device classes. A pick-up restores the counts saved at the checkpoint.

## Controls

| Key | What |
|---|---|
| 1, 2, 3 | Cut to Julian, Maya, Vex (and 4, the drone's angle, in the edit suite) |
| Q | Cue Julian to the mark under the crosshair (any camera) |
| WASD | Julian walks (camera A) |
| Mouse | Look and aim; during a bluff on camera A, move the highlight |
| E | Fire the device under the crosshair. On A: lift, open the case, bow, pick a bluff answer |
| Space | Skip the replay |
| R | A new take from the opening |
| M | Mute |
| ESC | Quit |

## Camera and HUD

- **Cameras:** each character has a spring-arm camera with the lens from rule 1. A cut swaps possession (which
  character the player controls) on the same frame: no camera flight and no slow motion.
- **Exposure is manual** on every camera.
- **Per-camera visibility:** costume props are set to "only owner see" and tape labels to "owner no see", owned by
  Julian's pawn, so only camera A shows costumes and only B and C show labels.
- **Camera A:** everything sits in the letterbox bars.
  - Top left: "A - JULIAN" and the timecode.
  - Top right: a red REC dot.
  - Bottom: the gold composure line with its label.
- **Cameras B and C:** the video-tap overlay carries:
  - "B - MAYA" or "C - VEX", the burned-in timecode, and the cue sheet;
  - Julian's feed: a corner inset of camera A at 480x270. It is a SceneCapture2D at 15 fps set to Final Color (LDR),
    so it keeps the film look, with the composure line under it.

## The set

One level and one hall, about 30 x 20 m, built as a film set. Three walls are flats, and the fourth side opens onto a
dark studio with light stands and cables. Camera A sees the dressed fronts (gilt, velvet, chandeliers). B and C see
the bracing, sandbags, cables and stencilled labels behind them. Three layers, all in view of each other:

1. **The floor** (camera A):
   - The entrance and the iris scanner at one end.
   - Posed guests in the middle.
   - **The band:** a jazz quintet on a riser with its PA. The band and the guests bob on the beat, using a Transform
     (Modify) Bone node in the animation Blueprint driven by the bar clock.
   - The champagne bar, paintings and pillars, each with a gold mark.
   - The side door, with its passage along the wall to the dais and on past the band to a side exit.
   - At the far end, the dais: Vance's podium, the gold glass case with the reel, the velvet rope and its infrared
     beam, red tape, and the hatch under the stage.
2. **The catwalk** (camera B):
   - About 6 m up, along one long side and across the stage end.
   - It looks down on every frame and every mark, so this is where you block Julian from.
   - Maya's post has the follow spot, both banner lines, the breakers, the pyro switch and the seating plan.
3. **Backstage** (camera C):
   - A corridor behind the long wall and the stage.
   - It sees the floor through two service doors and a gauze scrim.
   - Vex's post has the cart releases, the fuse box, the sound desk, the side door lock, the registry terminal, the
     winch and the guest list.

The broadcast drone flies its spline loop over the floor with its tally light on.

## Look

The film and the making-of, cut against each other.

- **Camera A, "the film":**
  - Noir: monochrome, with strong rim and back lights so every figure reads as a silhouette in costume.
  - Only red (danger: frames, tape, tally lights) and gold (Julian's marks and blocking, the reel, the stamps, the
    DIRECTOR'S CUT grade) keep their colour. Those objects are marked with the custom-depth stencil, a per-object tag
    the post-process material reads.
  - Hard spotlights through haze, the letterbox and grain.
  - At the alarm the picture bleeds to full colour for the last 8 bars.
- **Cameras B and C, "the making-of":** washed-out video-tap colour, 16:9, the timecode and scanlines. Bare stand-ins
  with tape labels, the raw backs of the set, work lights.
- **Value:** Julian's white jacket is the lightest thing in the film's frame, and the guards are the darkest.
- **From ART_STYLE.md, still in force:**
  - §1 rule 1 (value) and rule 2 (red and gold only for meaning).
  - §1 rules 4 and 5 (no Lumen, no Nanite).
  - §8, the film overlay as a UI widget, with a 2.39:1 letterbox in place of the 4:3 gate.
  - §14, the checks.

  Rule 3's location tones are replaced by the film/making-of split.
- **Lighting:**
  - Movable spot lights with volumetric fog. These work without Lumen.
  - At most about 4 shadow-casting spots on screen.
  - A dim skylight, because there's no Lumen bounce light.
- **Characters:** template mannequins, tinted by role. On camera A they wear simple costume props (top hats, gown
  shells, caps, bow ties, instruments). Generated models are for props only.

## Audio

- **The band is the score and the clock** (rule 2).
  - The per-camera mix has a real source: close on the floor, from above on the catwalk, through the flats backstage.
  - The feedback squeals through their PA and knocks them out for a bar.
  - The alarm stops them dead, and the heist cue crashes in at the same tempo. Its last bar is the deadline, the sting
    is the escape, and a tape-stop is failure.
- **Composure:** a heartbeat and a high ringing tone as it falls. Low-pass is only for backstage.
- **Voice (ElevenLabs):** about 80 short lines, all subtitled.
  - The director: "Action!", "Cut!", "Print it!", "Keep that in!"
  - Julian's quips and stalls.
  - Maya and Vex on the earpiece: device lines, hints, the reversal.
  - Vance's speeches.
  - The Chief and the guards: 4 bluff questions and both reactions.
- **Sound effects (ElevenLabs):**
  - A switcher click on every cut.
  - The clapper only at a take's start, a pick-up and PRINT IT.
  - The blooper: the film-burn crackle, the buzzer, the tape-stop, the rewind whirr.
  - The feedback, breaking glass, the banner drop, the curtain thud, sparks, the bulb blowing, the alarm, the
    shutters.
- **Music:** WeLoveIndies or ElevenLabs music, at a fixed and known 120 BPM. The game's bar clock comes from it.
- **The mix:** music ducks under every voice line.
- **Credits and the key:** every tool and library goes in `docs/CREDITS.md`. Marwan sets `ELEVENLABS_API_KEY` on the
  PC themself (README, "The game"); agents never type it or print it.
- **Until step 6:** subtitles, engine sounds and a click track stand in.

## Tick order

The rules live in one director actor, `ATakeDirector`, which ticks after the player controller:

1. **The bar clock.** Beat and bar events for this frame.
2. **Presses.** A cut first: swap possession, grab a still, open the cut-on-action check, test the beat. Then Q, E and
   the mouse.
3. **Devices.** A device that fires this frame applies its verb to every watcher in range this frame, counts the
   two-shot, and records when it landed.
4. **Movement and sight.** Julian's blocking walk, then the watchers' sight tests against him: frame, range, line
   trace.
5. **Composure.** Rule 5's drains and gains, and the bluff trigger.
6. **The scene.** Goals, checkpoints, deleted scenes, the reversal, the finale clock.
7. **End of frame** (late tick): the blooper check, the finish check, then the recorder's sample.

## Events and feedback (all on the frame of the event)

| Event | Feedback |
|---|---|
| Cut | Hard cut, switcher click, the film or making-of look, the label, a still |
| Cut on the beat | A small tick by the timecode |
| Beat | Every frame pulses; ghosts show the next turns |
| Cue (Q) | A gold ring on the mark, the dotted path with its beat count, Julian turns and walks |
| Crosshair on a device | Range ring, a marker on each watcher it would catch, the uses left |
| Device fires | The crew's line, its sound, the effect starting in the hall, a use struck off the cue sheet |
| Two-shot or wide shot | The stamp |
| Cut on action | "CUT ON ACTION" stamp, a gold frame on the inset |
| A watcher frames Julian | The tally light goes live, the frame turns solid red, the heartbeat starts |
| Composure drains | The gold line flashes red with its cause, the film burns at the edges, the ringing rises |
| On a mark | A gold X under Julian, ON MARK |
| Lure, Blind, Mask land | The frame swings toward the point / the frame goes dark / a thud with dust |
| Bluff | The guard walks up, the question is voiced and subtitled, the answer's prop glows, 3 answers on an 8 s bar |
| Reversal | The crew's line, the device struck through, a spark burst |
| Lift | A 1.5 s ring, the keycard chime, +25 |
| Deleted scene restored | Stamp, its own still, "Keep that in!" |
| Case opened | The band stops dead, the alarm, the bleed to full colour, shutters falling, the heist cue, the 8-bar clock |
| Blooper | Freeze, film burn, tape-stop, clapper, red slate, buzzer, "Cut!", the rewind whirr, "Action!" |
| Finish | Snap to black, clapper, PRINT IT, the sting, your director's cut |

## Kill list (final)

- **A live AI interrogation.** A key in the build gets extracted, offline judges get silence, and the latency breaks
  the same-frame rule. The bluff is pre-written and pre-voiced, and it is answered from what the crew's cameras see.
- **Camera tricks:** the character wheel, camera flights between characters, slow motion.
- **Crew movement:** Maya and Vex stay at their posts.
- **More space:** a separate lobby (the entrance is one end of the hall), more than one level, more than one hall.
- **Animation and physics:**
  - Crowd AI and guests that walk.
  - Tackles.
  - Physics destruction: the carts and banners move on timelines.
  - Reaction animations: frames, tally lights and labels do that job.
  - "?" and "!" icons.
- **AI tooling:** behaviour trees and AI perception. Watchers are plain C++ with frames and line traces.
- **Rewind and editing extras:** a true rewind (a pick-up is a level reload at a checkpoint) and a timeline editor.
  The edit suite is a live cut over the replay.
- **Imported characters:** generated or retargeted character models.
- **More dialogue:** anything beyond the bluffs, hints, device lines, Vance's speeches and the director.
- **Menus and systems:** saves, menus beyond the title card and the end sequence, settings, difficulty modes.
- **Lumen and Nanite.**

## Build order

Core loop in a test box first, then the set with its look, then the scenes (the ending first), then dressing,
characters, audio and polish. Every step ends playable, and the Windows package is rebuilt every night (Fri and Sat).

| When (Cairo) | Step | Done when |
|---|---|---|
| Fri, by 11:45 | **0. Pipeline** | Done |
| Fri 13:00-21:30 | **1. Core loop in a test box.** Start with one header pass that declares every new class and `UPROPERTY`, so the editor restarts happen together. Three characters and the cut (possession, label, lens). The bar clock on a metronome, with pulsing frames and next-turn ghosts. Julian: Q cues with the beat count, the blocking walk, marks, red tape, composure. Maya's post with crosshair aiming. One patrol guard whose frame is a flat mesh that ignores walls for now, with Lure and Blind. Three devices with use counts: the spot (auto-target), a cart, a banner. The rope's infrared beam. A blooper that reloads the level | In play-in-editor, Julian gets past the guard and the beam two ways, and a cue plus a device timed to his arrival works |
| Fri about 22:00 | **Gate 1.** Marwan plays for 5 minutes. If they don't want another go, change a rule (drain rates, windows, uses, the cost of cutting away), not the art | A yes, or the rule changed |
| Fri night to Sat morning | **2. The set, its look, Scenes 1 and 2.** The hall's blockout on three layers, fronts and backs. The film and the making-of (noir monochrome with stencilled red and gold, rim lights, costume and label visibility, the video tap), manual exposure, spots with haze, the skylight. Frames with line traces, tally lights and tape labels, Julian's inset, still grabs, checkpoints with the bar restore, the recorder. The queue and the scanner, the breakers, the fuse box, the registry terminal, the side door, the drone. The bluff with its props (placeholder text). Vance's speeches, the bodyguards, the sound desk, the lift, the reversal. The Wrong Eye, The Long Take, The Two-Shot. Package | Scenes 1-2 play in the package, and screenshots show the film and the making-of |
| Sat (midterm 12:30) | **3. Scene 3 and the ends.** The pyro, the winch, the curtain, the hatch, the side passage, the shutters, the 8-bar clock, the colour bleed, The Bow. The title card, PRINT IT, the assembled replay with the edit suite, the contact sheet, the stats and the grade, the blooper reel, R. Package | Title card to blooper reel in the package |
| Sat evening | **Gate 2.** Marwan plays a full take cold | They want another go |
| Sat evening and night | **4. Look.** Dressing the fronts (gilt, velvet, chandeliers) and the backs (bracing, cables, stencils). The film burn, the HUD in the letterbox, the stamps | Screenshots pass ART_STYLE §14 |
| Sat night to Sun 10:00 | **5. Characters and props.** Tinted mannequins, costume props, the band's instruments, the bob on the beat, the bow. Fab free packs, or generated props (Tripo or Rodin) if the codes arrive. Niagara (Unreal's particle system) sparks, glass and smoke | Credited in docs/CREDITS.md |
| Fri night to Sun 10:00, in parallel | **6. Audio.** A separate session, outside the editor. Write the lines Fri night, generate them with ElevenLabs once Marwan has set the key, and pick the band's tune and the heist cue at 120 BPM. The editor session imports everything and locks the bar clock to the tune by Sun 10:00 | Content lock |
| Sun 10:00-18:00 | **7. Polish.** A mix pass, a bug bash, the 30-60 s video, the itch page, the dev log. The video opens on the first cut on action (the spot slams the scanner, cut, Julian walks through), then a cue landing on the beat, the finale's bleed to colour, and a frame of the replay | Nothing new after 18:00 |
| **Sun 22:00** | **Upload and submit.** Then one try at the Web build (proven on the template, 23 min to package) as a second zip that plays in the browser | Downloaded and played once |

**If Saturday slips,** cut in this order, so the ending is always reachable:
1. The edit suite, keeping the automatic replay.
2. The replay itself, keeping the contact sheet.
3. The Wrong Eye, The Long Take and The Two-Shot, keeping The Bow.
4. The side passage exit.
5. The ON THE BEAT ticks.
6. The reversal.

## Decided Nov 13

1. The heist replaces K (Marwan's pitch). From round 3 the full design is the target, and Marwan carries the
   execution.
2. The bluff is pre-written and pre-voiced, and answered from what the crew's cameras see. There is no AI at runtime.
3. Hard cuts on 1, 2 and 3. The world pauses only for the blooper freeze.
4. Julian is cued to marks and walks on his own. Maya and Vex stay at their posts.
5. One hall on three layers. Three scenes, which are checkpoints in one continuous space.
6. Camera A is the film: noir, costumed, red and gold. B and C are the making-of: stand-ins with tape labels.
7. The band is the score and the world's clock, at 120 BPM.
8. Spending uses costs the grade, never the ending.
9. ElevenLabs for voice and sound effects.
10. Solo, with Claude agents (the Solo Driver prize).

## Not decided yet

- **Game and team name.** The zip must be `TeamName_GameName.zip`. Suggested: *Print It*.
- **The band's tune, the heist cue and their source,** before step 6: WeLoveIndies or ElevenLabs music, at 120 BPM.
- **Student category:** the Official Rules are still unread.
- **Perk codes:** none arrived. Retry Tripo before Nov 16; ask the organisers for late-joiner codes if wanted.
