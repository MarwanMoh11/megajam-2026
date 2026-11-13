# Status

Last updated: 2026-11-13, about 18:30 Cairo (gate 1 answered: three rules changed, step 1 is being reworked). Theme:
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

State: **step 1, the core loop in a test box, is built** (Nov 13, about 17:30). The C++ is in
`Reel/Source/Reel/Take/`, the map is `/Game/Take/Lvl_TestBox` (the project starts in it), and
`tools/build_testbox.py` rebuilds the map from its numbers. In play-in-editor and in the Windows package Julian
gets past the guard and the beam two ways (banner plus spot, cart plus spot), and a blooper reloads the level.
Next: **gate 1**, Marwan plays it. Nothing past step 1 is built.

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
  arrives, it plays the phrase solo while that watcher's tally light blinks.
- **On screen:** every watcher's frame pulses on the beat, but nothing on screen warns a bar ahead: only the band
  does. Julian's path shows his arrival count, e.g. "on his mark in 6", and turns red only where danger is right now: the red tape, the beam, a
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
instrument) for the bass, trumpet and drums (the warning instruments) and the piano, so each warning's warning phrase can be triggered. Voice lines
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
| Beat | Every frame pulses |
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
| Fri 17:00-22:00 | **1. Core loop in a test box.** See "Step 1" below | In play-in-editor, Julian gets past the guard and the beam two ways, by a cue plus a device timed to his arrival. **Built Nov 13, 17:30** |
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
- **The band's tune, the heist cue and their source,** before step 6: WeLoveIndies or ElevenLabs music, at 120 BPM,
  with stems.
- **Student category:** the Official Rules are still unread.
- **Perk codes:** none arrived. Retry Tripo before Nov 16; ask the organisers for late-joiner codes if wanted.
