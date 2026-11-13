# Status

Last updated: 2026-11-13, about 17:30 Cairo. Theme: **Director's Cut**.

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

State: the Unreal project `Reel` exists on the PC (Third Person C++ template). Stage 3 proved the pipeline: a Windows
package in 3.6 min, a 343 MB zip, and a Web build that boots. There is no game code yet. Next: build order step 1.

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
- a low angle on the scanner;
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
  - From the third break, Vance speaks for 4 bars on every break.
- **The band warns you.** Each watcher type has its own instrument, which plays a short phrase one bar before that
  watcher turns or passes:

  | Watcher | Instrument |
  |---|---|
  | The guard | The upright bass |
  | The drone | The muted trumpet |
  | The bodyguard | The piano |
  | Vance's speech | A drum roll |

  You can hear a threat coming before you see it. Each instrument is taught on its own: the first time its watcher
  arrives, it plays the phrase solo while that watcher's tally light blinks.
- **On screen:** every watcher's frame pulses on the beat and shows a ghost of its next turn a bar ahead. Julian's
  path shows his arrival count, e.g. "on his mark in 6".
- **It never stops.** The feedback knocks the band out for its 2 bars, and they come back in on the next bar line. The
  alarm switches to the heist cue at the same tempo.
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
  - the mark behind Vance lifts the keycard (1.5 s);
  - the mark at the case opens it;
  - the mark at the side exit makes Julian bow.

  A click on A always means "go there".

### 4. Fire: click the set

- **The posts.** Maya and Vex stand still at their post for the scene. Every device they can fire is in their
  camera's view. Devices fire only from B and C.
- **Hover:** shows the device's range ring, a marker over every watcher it would catch, and its pips.
- **Clicking a mark** on B or C flashes "[1] to cue Julian".
- **Click a device:** it fires.
- **Click a watcher on Maya's camera:** the follow spot blinds it.

A click on B or C always means "fire".
- **Speed:** a crew cut takes about 2 s (cut, click, cut back).

### 5. Composure (0-100)

The only meter: a gold line in camera A's bottom letterbox bar, and under the strip's film feed.

**Julian is exposed while:**
- he is inside the **red tape** (taped around the exhibit, past the rope, on the dais);
- he is inside the **drone's cone**, which is drawn on the floor as a red ellipse.

Those are the only two rules, and both are visible on the floor.

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
| The iris scanner | It is a watcher. If it is live when Julian reaches it: "NO MATCH" and a blooper |
| The patrol guard | Waypoints, turning on bar lines |
| The broadcast drone | A spline loop, one pass per zone every 8 bars |
| The infrared beam | A line-shaped frame across the rope. It drops for one beat in every 4 bars, on a hi-hat splash. Blind or Mask turns it off |
| The bodyguard | Posted beside the podium, facing outward at an angle |
| Vance, the host | Watches only his belt. He misses a lift while he is **speaking, lured or blinded**. If he sees a lift: "Security!", a blooper. He never leaves the podium: a Lure turns his head |

### 7. Devices: three effects

Every device does exactly one of three things to every watcher in its range:

| Effect | What it does |
|---|---|
| **Blind** | The frame goes dark for the duration |
| **Lure** | The watcher walks to the device's point, looks at it, and walks back (Vance only turns his head) |
| **Mask** | Solid geometry, a matte, that blocks frames and the beam |

| Device | Who | Effect | Range, duration | Uses |
|---|---|---|---|---|
| Follow spot: click a watcher on Maya's camera | Maya | Blind one watcher | 35 m, 4 s | **Free**, recharges in 4 bars |
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

**What triggers one:** composure reaches 0, Julian touches a live beam, the scanner reads him, or the finale clock
runs out.

**What happens:**
1. A 1 s freeze, with the film burning through from the centre.
2. The music tape-stops and the clapper clacks.
3. A red slate reads **"BLOOPER - AGAIN FROM THE ROPE"**, naming whichever checkpoint, and the director says "Cut!"
4. A rewind whirr plays while the level reloads at the checkpoint, and then the director says "Action!"

**Checkpoints:** the start of each scene, the rope once Julian is past it, and every band break on which Julian stands
on a mark, unexposed, with at least 50 composure. A failed attempt replays 16 s at most.

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
2. **YOUR DIRECTOR'S CUT** (about 60 s; Space skips). Your take with every blooper spliced out, cut where you cut,
   tightened by dropping dead holds in whole 8-bar phrases (the band's cycle) so the music never jumps.
   - It is the film throughout: camera A's shots, in costume. Where you cut to B or C, the replay covers that moment
     with the drone's broadcast angle, which faces the dressed fronts. The real score runs under it (the band, then
     the heist cue).
   - Restored deleted scenes carry a "RESTORED" tag.
   - The recording holds every moving actor's transform at 20 Hz, plus device events, cues and the cut list.
3. **The edit suite.** While the replay plays, 1 (camera A) and 4 (the drone's broadcast angle) take over the cut live.
   Your live cut replaces the automatic one from that moment.
4. **The contact sheet.** Up to 12 stills grabbed at your cuts. Each is a SceneCapture2D set to Final Color (LDR),
   rendering into one of 12 render targets at 256x144.
5. **The stats.** CUTS, GOOD CUTS, BLOOPERS, DELETED SCENES n/4, TIME.
6. **The grade.**

   | Cut | When |
   |---|---|
   | **DIRECTOR'S CUT** (gold) | No bloopers, and all 4 deleted scenes |
   | **FINAL CUT** | 2 bloopers or fewer |
   | **ROUGH CUT** | Finished |

7. **The blooper reel** rolls under the credits: every blooper's freeze frame with its slate.

R shoots a new take. ESC quits.

## The take

One continuous take, about 1:45 when clean, in three scenes that are checkpoints in one space. The gold case with
the reel sits on the dais, and the gold side exit glows at the far end, both in the very first shot.

### Scene 1. The Door (about 15 s): teaches the whole loop

1. On camera A: "Click the mark at the scanner." Julian walks there, and his path shows "at the scanner in 12". Counts are always in beats, half a second each.
2. "[2] MAYA" flashes. Her view frames the scanner: "Click the scanner as Julian arrives." The 4 s Blind covers him if
   it lands within the last 8 beats before he arrives.
3. The moment the spot lands, the Chief yells "Who turned that on?!" and "[1] JULIAN" flashes. Cutting back within the
   bar lands the first GOOD CUT as Julian walks through.
4. If the scanner is live when he reaches it: "NO MATCH", a blooper.

**Answers:** the spot (free).

**Deleted scene:** The Cold Open.

### Scene 2. The Floor (about 70 s)

**Julian's goal:** cross the floor, pass the rope onto the dais (a checkpoint), then lift the keycard from Vance's belt.

**Watchers arrive one per band break:**

| When (from the scene's start) | Arrives | Its warning |
|---|---|---|
| At once | The marks, and the patrol guard | The bass |
| The first break after the guided win | The drone. Before it can catch anyone, the marks on its path pulse gold and "On the open floor, the drone wants him on a mark" shows | The trumpet |
| The first break after Julian reaches the floor's middle mark | The rope's beam goes live | |
| The first break after Julian crosses the rope | The bodyguard takes his post, and Vance starts speaking on every break | The piano, then the drum roll |

**The guided win** (first 15 s): "Click the mark by the exhibit". It is inside the exhibit's red tape, so the guard
can catch Julian there, and camera A's angle on it frames the guard's approach. Then "[2] spot the guard as his frame
swings onto Julian", and a GOOD CUT.

**The rope:**
- **Free:** spot the beam on the bars when the guard faces away (the bass warns a bar before he turns), and cue Julian
  over.
- **Paid, for a wider window:** a banner on the beam (Mask, 10 s), or the mid-floor cart on the guard (Lure, 6 s).

**The lift:**
- **Vance** has to be speaking (free: he speaks for 4 bars on every break), lured, or blinded.
- **The bodyguard:**
  - **Free:** during a speech, spot him and cue Julian to the lift mark behind Vance.
  - **Paid:** the feedback (Blind, the whole hall).
  - **Paid:** the dais cart (Lure: Vance and the bodyguard, x2). If you're on camera A as the lift lands, that's The
    Insert.

**Deleted scenes:** The Long Take, The Insert.

### Scene 3. The Getaway (16 s: the last 8 bars)

On the first take, the prompts return: "[3] VEX: the winch" and "[2] MAYA: the pyro". Cue Julian to the case mark
and he takes the reel. The alarm stops the band dead, the shutters fall, four guards walk in from both
sides, and the drone locks on. Camera A bleeds to full colour for the last 8 bars: the alarm's red, the pyro's gold,
every colour at once.

**The exits:**
- **The hatch under the stage, 10 m away (free):** Vex's winch drops the curtain for good, masks both flanks and opens
  the hatch. Maya's pyro blinds the stage end for 6 s.
- **The gold side exit past the band:** the pyro doesn't reach it. Its two guards and the drone over the band need a
  Lure, Blind or Mask you saved: a cart, the feedback, or a banner.

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
  - **Hovering a mark on A** draws his path, red wherever a frame or the drone's cone will catch him on the way.
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
   - Gold marks by the champagne bar, the paintings, the exhibit and the pillars.
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
- **The alarm:** at the alarm the picture bleeds to full colour, and the watchers' frames switch to a white pulse on the
  beat so they still read.

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

**The band** is the score, the clock and the warning system (rule 2).
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

**Music source:** WeLoveIndies or ElevenLabs music, at a fixed and known 120 BPM, with separate stems (one track per
instrument) for the bass, trumpet, piano and drums, so each instrument's warning phrase can be triggered. Voice lines
duck the band's bed, never the warning phrases.

**Credits and the key:** every tool and library goes in `docs/CREDITS.md`. Marwan sets `ELEVENLABS_API_KEY` on the PC
themself (README, "The game"). Agents never type it or print it.

**Until step 6:** subtitles, engine sounds and a click track (a metronome) stand in.

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
| Beat | Every frame pulses; ghosts show the next turns |
| A bar before a turn or pass | That watcher's instrument phrase |
| A watcher arrives on a break | Its tally light blinks on, on the downbeat |
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
| Fri 17:00-22:00 | **1. Core loop in a test box.** See "Step 1" below | In play-in-editor, Julian gets past the guard and the beam two ways, by a cue plus a device timed to his arrival |
| Fri about 22:30 | **Gate 1.** Marwan plays for 5 minutes. If they don't want another go, change a rule (windows, drain, uses), not the art | A yes, or the rule changed |
| Fri night to Sat morning | **2. The set, its look, Scenes 1 and 2.** See "Step 2" below. Package | Scenes 1-2 play in the package, and screenshots show the film and the making-of |
| Sat (midterm 12:30) | **3. Scene 3 and the ends.** See "Step 3" below. Package | Title card to blooper reel in the package |
| Sat evening | **Gate 2.** Marwan plays a full take cold | They want another go |
| Sat evening and night | **4. Look.** Dressing the fronts and backs, the film burn, the letterbox HUD, the stamps | Screenshots pass ART_STYLE §14 |
| Sat night to Sun 10:00 | **5. Characters and props.** Tinted mannequins, costume props, the band's instruments, the bob on the beat, the bow. Fab free packs, or generated props if the codes arrive. Niagara (Unreal's particle system) sparks, glass and smoke | Credited in docs/CREDITS.md |
| Fri night to Sun 10:00, in parallel | **6. Audio.** A separate session, outside the editor. Write the lines, generate them with ElevenLabs once Marwan has set the key, and get the band's tune and the heist cue at 120 BPM, with stems. The editor session imports everything and locks the bar clock to the tune by Sun 10:00 | Content lock |
| Sun 10:00-18:00 | **7. Polish.** See "Step 7" below | Nothing new after 18:00 |
| **Sun 22:00** | **Upload and submit.** Then one try at the Web build (proven on the template, 23 min to package) as a second zip that plays in the browser | Downloaded and played once |

**Step 1, the core loop in a test box:**
- Start with one header pass that declares every new class and `UPROPERTY`, so the editor restarts happen together.
- Three characters and the cut. Camera A is two fixed angles with a zone trigger; B is one fixed camera.
- The mouse pointer: on A, click a mark to cue Julian (navmesh walk, path, count); on B, click a device or a watcher
  to fire.
- The bar clock on a metronome, with pulsing frames and next-turn ghosts.
- Composure and the red tape.
- One patrol guard, whose frame is a flat mesh that ignores walls for now.
- The spot (free, recharging), a cart and a banner.
- The rope's beam.
- A blooper that reloads the level.

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
- **The band's tune, the heist cue and their source,** before step 6: WeLoveIndies or ElevenLabs music, at 120 BPM,
  with stems.
- **Student category:** the Official Rules are still unread.
- **Perk codes:** none arrived. Retry Tripo before Nov 16; ask the organisers for late-joiner codes if wanted.
