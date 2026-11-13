# Status

Last updated: 2026-11-13. Theme: **Director's Cut**.

**The game: K, Meanwhile...** Picked by Marwan on Fri Nov 13 after playing `prototypes/k-3d/` and L's
`prototypes/l-final-cut/`, then refined the same day into the spec below. Where this file differs from
`prototypes/k-3d/NOTES.md` or `docs/ART_STYLE.md`, this file wins. NOTES.md still holds the tuned values (gunplay,
damage, regen, enemies, spawns, score): copy them unless a value here replaces one.

State: the Unreal project `Reel` exists on the PC (Third Person C++ template), and stage 3 proved the pipeline on it
(Windows package 3.6 min, zip 343 MB). No game code yet. Next: build order step 1.

## The pitch

You are the editor of a lost 1926 action film, cutting live between three hired guns on one job.
**The story only happens where the camera is.** The hero on screen has plot armor and moves the plot forward; the two
off screen are mortal and only hold the line. The studio's script cuts the film its own way and kills a hero. Break
from it and make the director's cut.

**Opening card** (5 s, any key skips): "In 1926 the studio recut *Meanwhile...* and killed off its hero. The
director's cut was lost. Until tonight."

Why this shape (docs/SURVEY.md, docs/JAM_PLAN.md): judges skim a huge field, so play starts within 10 s and the
video shows a cut in its first 5 s. The theme is the mechanic, not the decoration. A run takes 3-4 minutes and
invites a second one, and Fun counts double on ties.

## The film

The Bull runs the city and keeps its money in a vault under the Picture Palace. During tonight's midnight premiere,
three hired guns go after it. Someone has sold them out. All story is on silent-film title cards: about 30 s of
reading in a whole run, no voices.

| Act (card) | Hero, area | Objective (moves only on camera) | Link (lands in another area) |
|---|---|---|---|
| One, "Two Names" | Vera, the Mill | Reach the water tower winch and hold it, 5 s | **The flood:** the tower falls and every mook in the yard goes down |
| | Moss, the rail yard | Stoke the stolen engine: hold the cab, 6 s | **The engine:** it runs its fixed track and smashes the Mill gate; every mook at the Mill goes down |
| Two, "The Third Name" | Juno, the rooftop | Shoot out the 3 searchlights over the yard | **The rifle:** her shots reach the yard, so she can kill whatever is on Moss |
| | Moss, the rail yard | Throw the points switch to the Palace: hold, 6 s | |
| | Vera, the Mill | Crack the Bull's safe for the vault plan: hold, 6 s | |
| Three, "The Premiere" | All three, the Picture Palace | Moss: thread the real reel in the projection booth, hold 8 s. Vera: bring down the Bull. Juno: shoot the chandelier chain while the Bull stands in its red floor disc (40% of his health) | Three angles on one fight |

The Juno twist is one line on the end card: "Juno sold them out." If her rifle killed anything on Moss, it goes on:
"...then she covered them."

## Rules

1. **On camera you're John Wick.** The hero you control has plot armor: hits do x0.75 and stop at 12% health, and
   health regenerates (NOTES.md values; no cover bonus, since cover snapping is cut). Aggression pays: gun-fu,
   headshots, close-shave dodges. When only one hero is left alive, plot armor is gone.
2. **The plot only moves on camera.** A hold objective fills only while its hero is on camera and inside its zone;
   Juno's targets fall only to shots you fire. Progress doesn't decay when you cut away. Off camera, heroes are AI:
   they hold their spot, shoot back badly, lose health (mooks slowly, a heavy fast) and can die, "written out of the
   picture".
3. **One job, three angles.** The three links above fire on the frame their objective completes (the rifle: on each
   kill in the yard). Cut to the hero where a link landed within 3 s and you see it land: **MATCH CUT**.
4. **The studio's script.** Each act has a shot list (time, hero, beat). On script, the film cuts itself after a 3 s
   countdown. A cut key for anyone else takes you off script; cut to the hero the script wants, or press TAB, to go
   back. Scripted cuts pay no bonus. The script follows story, not danger: in Act Two a heavy reaches Moss at 0:26,
   and the script stays on Juno until 0:42 and returns to Moss at 1:18. Followed blindly, it loses Moss.
5. **Studio Cut vs Director's Cut.** The end screen puts the studio's cut (fixed text: Moss written out in Act Two,
   Juno the villain, the studio's reel on the screen) beside yours, with the medal.
6. **Big moment (if time).** The chandelier gets a 5 s warning card, "BIG MOMENT: JUNO, 5". Caught on camera, it
   plays in slow motion and pays gold. Until then it is a plain objective.

Score is in feet of film (NOTES.md), plus +1,000 per objective and +1,500 per MATCH CUT. The just-in-time bonus
stays as in NOTES.md.

## Win and lose

- A run: opening card, Act One, Act Two, Act Three, end screen. 3-4 minutes.
- An act ends 4 s after the last living hero's objective is done (so a MATCH CUT can still land), or at its maximum:
  One 80 s, Two 90 s, Three 100 s. A missed objective's link never fires, and the end screen lists it as a missed
  shot ("The tower never fell.").
- Act Three ends when the Bull is down, or at its maximum: the Bull escapes and the studio's reel plays.
- A written-out hero's later objectives are skipped. No game over mid-run, except when all three are dead: the
  run ends at once, Cancelled.
- The act card covers moving each hero to their next start point (a teleport, no travel).
- R on the pause or end screen restarts at the Act One card; the opening card shows only on the first run.

| Medal | When |
|---|---|
| Director's Cut (gold) | Off script, all three survive, and the real reel is threaded |
| Studio Cut (silver) | Less than 15 s off script in the whole run |
| Alternate Cut | Off script, two survive, or three without the reel |
| Recut | Off script, one survives |
| Cancelled | Nobody survives |

Stars 0-5 from the score, capped one lower per hero lost (NOTES.md).

## Heroes, enemies, controls

One hero class and one moveset; the heroes differ only by gun.

| Hero | Gun | Values (new; the pistol is NOTES.md's) |
|---|---|---|
| Vera | Pistol | 12 rounds, 1.25 s reload |
| Moss | Shotgun | 5 shells, 6 pellets, one shot kills a mook within 8 m |
| Juno | Bolt-action rifle | 5 rounds, aim-down-sights zoom x4, one body shot kills a mook, 1 s between shots |

- **Mooks** (NOTES.md: 2 body shots or 1 headshot; a red glint grows at the gun 0.55 s before each shot). They hold
  placed spots behind low boxes and pop up to fire. From Act Two, 18% are rushers.
- **Heavies** (NOTES.md: 30 HP; a slam disc up close, a charge lane from 10 m; stencil red during the wind-up; dazed
  1.2-1.4 s afterwards, taking x2).
- **The Bull:** a heavy at 1.7x scale with 150 HP. He hunts whoever is off camera.
- **Controls:** mouse aims, LMB fires, RMB aims down sights, WASD moves, SHIFT dodges, F is gun-fu up close, R
  reloads, 1/2/3 cut to a hero, TAB goes back on script, ESC pauses, M mutes.
- **Camera:** the template's over-the-shoulder spring-arm camera, zooming to aim. A cut is a hard cut: possession
  switches to the other hero on the same frame, no blend. This replaces ART_STYLE's fixed camera for K (§2, §6).
- **Off camera keeps simulating:** unpossessed heroes get an AI controller, and skeletal meshes need *Always Tick
  Pose and Refresh Bones*.
- **Seeing the others:** the HUD shows the two off-camera heroes' health bars, red and flashing below 40% or while a
  heavy targets them. Step 4 replaces them with two round live cameos (a SceneCapture2D each, 24 fps).

## The map

One level, grey boxes, four areas, built in this order:

1. **The rail yard** (dusk green): the engine on its track, the points switch, low boxes.
2. **The Mill** (lamplight amber): the water tower between the Mill and the yard, visible from both; the gate the
   track runs into; the Bull's safe.
3. **The rooftop** (night blue): about 12 m up, with a clear view of the whole yard (link 3) and the Palace front;
   three searchlights.
4. **The Picture Palace** (silver, interior): stage and screen, stalls, a balcony for Juno, the projection booth,
   the chandelier over the stalls.

Each fight area is about 30 x 30 m with low and tall boxes as cover, and mook spots on the far side.

## Tick order

The rules live in one director actor that ticks after the player controller:

1. **Presses.** A cut switches possession first, so everything after it sees the new hero on camera. Then dodge,
   gun-fu, reload and fire; hitscan resolves here.
2. **The script.** Advance the shot list; on script, when the countdown reaches 0, cut.
3. **Objectives.** The on-camera hero's objective fills if they are in its zone.
4. **Links.** An objective completed this frame fires its link this frame and opens the 3 s MATCH CUT window.
5. **Enemies and off-camera heroes act** in their own ticks. All damage goes through one function that applies plot
   armor at the moment of the hit.
6. **End of frame** (the director's late tick): regen for the on-camera hero, deaths become written out, the act-end
   check, then the run-end check.

## Events and feedback (all on the frame of the event)

Sounds are placeholders until step 6 picks the music.

| Event | You see | You hear |
|---|---|---|
| Cut | Hard cut, one white flash frame, the new area's tone, "Meanwhile, at the Mill..." over the lower third for about 1 s while play continues | Projector clack |
| Your shot, hit, kill | Muzzle flash, tracer, sparks; hit marker, gold on a headshot; "+N" pop | Shot; thud; tink on a head |
| Mook about to fire | Red glint grows at its gun for 0.55 s | |
| At the plot armor floor | "PLOT ARMOR" in gold under the crosshair | Bright ping |
| Heavy wind-up | Body goes stencil red; red disc or lane | Low stabs |
| Objective filling | A ring in the zone and the objective line at the top fill | Rising tick |
| Objective done | Gold line over the picture: "THE TOWER FALLS" | Chord |
| Link lands | The effect plays in the other area on the same frame; that hero's HUD bar flashes gold; "CUT TO MOSS" for 3 s | Distant crash |
| MATCH CUT | "MATCH CUT" in gold scaling in, "+1,500 feet of film" | Chord and cymbal |
| Off-camera hero in trouble | Their HUD bar (later the cameo ring) flashes red | Their music layer turns urgent |
| Off or on script | "OFF SCRIPT" stamp; "The script wants MOSS · TAB" | |
| Scripted cut coming | "The script cuts to VERA in 3" | Tick each second |
| Just in time | "JUST IN TIME!" in gold, "+N feet of film" | Chord and crash |
| Hero written out | "Moss was written out of the picture." | Low chord, music ducks |
| Last hero | "Nobody left to cut to. Plot armor is gone." | |
| Act card | Full-frame title card for 2 s, play paused | Projector run-up |
| End | The studio's cut beside yours, medal, stars, "R to restart" | Theme |

## Kill list (final)

If it isn't in this file, mention it in one line and don't build it.

- Snapping to cover, leaning, climbing, steering any vehicle (the engine runs its track by itself).
- Cutscenes, voices, dialogue, or any moment of watching instead of playing. Story is title cards only.
- More than three heroes, four areas or one level.
- Per-hero abilities: the heroes differ only by gun.
- A simulated replay of the studio's cut: it is fixed text.
- Mook AI that claims and swaps cover: they hold placed spots.
- Shop, upgrades, inventory, ammo pickups, saves, difficulty modes, settings beyond volume and mouse sensitivity.
- The Web build, and with it emsdk (dropped Nov 13: the Windows zip is the entry, and the cameos and gunfire are
  heavy for a browser).
- Lumen and Nanite (ART_STYLE §1 rule 5 stays, for frame rate on judges' laptops).

**If time, in this order:** the big moment's slow motion; a filmstrip of caught moments on the end screen; mooks
that move between cover spots.

## Build order

Core loop first, then the map it needs, then content, look, characters, audio and polish. Every step ends playable,
and the Windows package is rebuilt every night (Fri and Sat).

| When (Cairo) | Step | Done when |
|---|---|---|
| Fri, by 11:45 | **0. Pipeline.** Stage 3 packaged, zipped and ran the template | Done |
| Fri afternoon and evening | **1. Core loop in a test box.** The hero class with the three guns, mooks, plot armor and regen. Then a second hero and area, the cut, off-camera AI and wear, the HUD bars, the Meanwhile card, and one hold objective that fills only on camera | Two heroes, cutting, in play-in-editor |
| Fri about 22:00 | **Gate 1.** Marwan plays it for 5 minutes. If they don't want another go, change a rule (auto-cut, objective times, off-camera wear), not the art | A yes, or the rule changed |
| Fri night to Sat morning | **2. Act One.** Blockout of the yard and the Mill, the tower and the track, both objectives, links 1-2, heavies, the script, the act card. Package | Act One plays start to end in the package |
| Sat (Marwan's midterm 12:30) | **3. Acts Two and Three, and the end.** Rooftop and Palace blockout, Juno and link 3, the Bull, the reel, the chandelier, the end screen, the opening card, restart. Package | Opening card to end screen in the package |
| Sat evening | **Gate 2.** Marwan plays a full run cold | They want another go |
| Sat evening and night | **4. Look.** ART_STYLE §13 step 0 (grade, area tones, film overlay, fonts), then the two cameos | Screenshots pass ART_STYLE §14 checks 1-4 |
| Sat night to Sun 10:00 | **5. Characters and props.** The template mannequins tinted per role (ART_STYLE value ladder) with hat and scarf props. Tripo characters only if one imports and retargets in under 30 min. Rodin props and PBRgen materials need the perk codes, which haven't arrived; without them, use the free tiers, Fab's free packs, or keep the graded boxes | Credited in docs/CREDITS.md |
| Sun morning, by 10:00 | **6. Audio.** Music and foley from WeLoveIndies | Content lock |
| Sun 10:00-18:00 | **7. Polish.** Juice, the if-time list, bug bash, the 30-60 s video, the itch page, the dev log | Nothing new after 18:00 |
| **Sun 22:00** | **Upload and submit** | Downloaded and played once |

## Decided Nov 13 (NOTES.md's open questions)

1. The script auto-cuts with a 3 s countdown. Gate 1 checks it isn't annoying; the fallback is a cue only.
2. On-camera wounds stay, floored at 12%.
3. The camera is the free shoulder camera.
4. The studio's cut on the end screen is fixed text, not a replay.
5. Solo, with Claude agents (Solo Driver prize).

## Not decided yet

- **Music**, before step 6: the prototype's John Wick-style electronic, or ART_STYLE's silent-film piano.
  Recommended: electro swing (1920s piano and brass over a modern beat), which is both.
- **Game and team name.** The zip must be `TeamName_GameName.zip`; the working title is *Meanwhile...*
- **Student category:** the Official Rules are still unread.
- **Perk codes:** none arrived (itch.io was linked after Nov 10). Ask the organisers for late-joiner codes if
  Rodin, PBRgen or fal are wanted; retry Tripo before Nov 16.
