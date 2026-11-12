# K "Meanwhile..." as a 3D third-person shooter (three.js prototype)

One file: `index.html`. Double-click it (plain `file://` works, checked in headless Chrome). It **needs internet**:
three.js 0.170.0 and the stand-in robot come from jsDelivr, and the fonts from Google Fonts. If a browser blocks it
from `file://`, run `python3 -m http.server` in this folder and open `http://localhost:8000`.

**Play:** click to roll (it locks the mouse). Mouse aims, LMB shoots, RMB aims down sights, WASD moves, SPACE takes
cover, SHIFT dodges, R reloads, F is gun-fu up close. 1/2/3 cut to a hero, TAB goes back to the studio's script. ESC pauses.
R restarts from the pause or end screen. M mutes. Backtick opens every tuning value.

## The game as built

Three hired guns in three rooms, one camera. **Plot armor goes where the camera goes.**

- **On camera** you play an over-the-shoulder shooter: free mouse aim, aim-down-sights zoom, cover, dodge-roll,
  reload, melee. You take hits (x0.75), but plot armor stops you at 12% health. Health comes back after 2.5 s without
  a hit, at 7%/s, twice as fast in cover.
- **Off camera** the other heroes fight on through a simple AI with no plot armor. Mooks wear them down slowly, and a
  heavy kills them fast. A hero who dies off camera is "written out of the picture".
- **The studio's script** (Marwan's idea from this session) is the intended edit. Each act has a shot list of times,
  heroes and story beats. While you are **on script**, the film cuts itself, with a 3 s countdown ("The script cuts to
  MOSS in 2"). If you press a cut key for anyone else, you are **off script**: a stamp appears, the auto-cuts stop and
  the edit is yours. Cut to the hero the script wants, or press TAB, to go back on script. The script follows story
  beats, not danger: it lingers on Juno's montage while a heavy beats Moss. Scripted cuts never pay a bonus.
- **The ending** replays the studio's cut headless from the same seed and puts it next to yours ("The studio's cut:
  Moss (Act two, 1:57) written out"). The medal depends on how your cut compares:

  | Medal | When |
  |---|---|
  | Studio Cut (silver) | Less than 15 s off script |
  | Director's Cut (gold) | Off script, and more heroes survived than in the studio's cut |
  | Alternate Cut | Off script, same number of survivors |
  | Recut | Off script, fewer survivors |
  | Cancelled | Nobody survived |

  The ending title (Everybody Walks / Bittersweet / Lone Survivor / Cancelled) and 0-5 stars from the score (capped one
  star lower per hero lost) are as before.
- **Acts:**
  - Act One, "Two Names": Vera at the Mill and Moss at the Railway.
  - Act Two, "The Third Name": Juno joins on the Rooftop.
  - Act Three: everyone in the Picture Palace with the Bull, which hunts whoever is off camera.
  - Each act ends once its minimum time has passed and its heavies are down, or at its maximum.
- **Score** is in feet of film:
  - +50 per mook kill on camera, times a chain multiplier up to x4.
  - +50 for a headshot, +50 point blank, +150 gun-fu, +150 close-shave dodge.
  - Heavy 1,500, Bull 4,000, survivors 2,500 each.
  - The just-in-time bonus for cutting to someone hurt pays up to 2,500, x1.5 if a heavy is on them. You only get it
    if they were away at least 2 s and lost at least 8%.
  - Kills off camera pay nothing.

### Gunplay and cover
- **Shots** are hitscan, in two rays. The first runs from the camera through the crosshair to find the aim point; the
  rules and the renderer share `camRig()`, so the crosshair is exact. The second runs from the gun to that point, so
  cover between you and the target stops the shot (sparks).
- **Spread:**

  | Stance | Spread (rad) |
  |---|---|
  | Hip | 0.025 |
  | Moving | 0.045 |
  | Aiming down sights | 0.004 |
  | Blind fire from cover | 0.1 |

  Each shot also kicks the camera 0.01 rad of recoil.
- **Gun:** a 12-round magazine, 1.25 s reload, and it reloads itself when empty.
- **Damage:**
  - A mook dies to 2 body shots or 1 headshot. The head is a sphere fitted to the robot's head.
  - A heavy has 30 HP; headshots do x2, and shots during its recovery do x2.
  - Gun-fu (F) kills a mook within 2.4 m outright, or does 3 damage and a shove to a heavy.
- **Cover:** SPACE within 1.5 m of any box snaps you to the nearest face; move to slide along it.
  - **Low cover** (under 1.5 m): you crouch, and mook bullets hit the box. They are blocked by real geometry, not dice.
    Aiming pops you up and exposes you; firing without aiming is blind fire.
  - **Tall cover:** aiming within 0.9 m of an edge leans you 0.75 m out.
  - Pull away for 0.2 s, press SPACE again, or dodge to leave cover.
- **Mooks:**
  - They claim cover spots on the far side of low boxes from their target, then pop up for 0.55 s to fire, with a red
    glint at the gun.
  - They move to new cover every 7-12 s, or as soon as you flank them.
  - From Act Two, 18% of them are rushers.
  - Off-camera heroes hit a crouched mook half as often.
- **Heavies:** the same pattern as before.
  - Close up they slam: a red disc grows under them.
  - From 10 m or more they charge down a red lane.
  - The body turns stencil red during the wind-up.
  - Afterwards they stand dazed for 1.2-1.4 s and take double damage.
  - The punch animation is timed so it lands on the frame of the slam.

### Tick order (rules, fixed 60 Hz; mouse look and presses apply at the start of the frame they happen)
1. Mouse look.
2. Press: cut, follow, cover, reload, melee, dodge, fire.
3. Card countdown.
4. Wrap-up timer.
5. Clocks advance.
6. **The script**: shot changes, then the auto-cut if on script.
7. Scheduled heavies spawn.
8. Mooks spawn.
9. Heroes update: you, then the AI.
10. Enemies update; their shots and slams resolve here.
11. Separation.
12. On-camera regen.
13. Corpse cleanup.
14. Lone check.
15. Act-end check.

### Events → feedback (all on the frame of the event)
| Event | You see | You hear |
|---|---|---|
| Your shot | Muzzle flash at the robot's hand, Paper tracer, sparks where it lands | Gunshot + low thump |
| Hit / kill | Hit marker (gold for a headshot), Paper impact lines, gold pop "+100 HEADSHOT" | Thud, tink on a head |
| Mook about to fire | Red glint grows at its gun for 0.55 s | — |
| Mook shot at you | Tracer; red damage arc toward the shooter; sparks on your cover if blocked | Shot, whiz or ricochet |
| At the plot-armor floor | "PLOT ARMOR" in gold under the crosshair | Bright ping |
| Cover / dodge / reload | Crouch pose / roll / magazine pips flash | Thud / whoosh / two clicks |
| Heavy wind-up | Body goes stencil red, red disc or lane | Low tremolo stabs |
| Slam / wall hit | Dust, 0.3 shake | Boom / crash |
| Cut | Hard cut, splice flash, the new room's tone, "Meanwhile, at the Railway..." intertitle | Projector clack |
| Off / on script | "OFF SCRIPT" stamp; script box flips; "The script wants MOSS · TAB" | — |
| Scripted cut coming | "The script cuts to VERA in 3" + cameo tag | Tick each second |
| Just in time / close / tense | Gold Limelight word scaling in, "+N feet of film" | Chord + crash |
| Hero killed off | "Vera was written out of the picture." | Low chord, music ducks |
| Last hero | "Nobody left to cut to. Plot armor is gone." | — |

## Look (Nitrate, from `prototypes/style/nitrate.js`, pasted in)
- **Frame:** a 4:3 picture with rounded gate corners. The other two rooms show as round live cameos in the side
  margins, each graded in its own room's tone.
- **Tones:** Mill amber, Railway green, Rooftop night, Picture Palace silver. A cut swaps the tone on the same frame,
  with `film.splice()`.
- **Value ladder:** heroes are the lightest thing on screen (with a little self-light so they stay light on the shadow
  side), mooks are dark, heavies are darkest.
- **Colour:** stencil red only for danger (strike window, discs, lanes, mook glint, damage arcs); gold only for the
  hero's scarf and plot-armor ring, headshots and payoffs.
- **Type:** Limelight, IM Fell English italic and Josefin Sans, Paper on Ink.
- **Film layers:** grain, flicker, dust and scratches at 24 fps.
- **Characters:** one rigged model (RobotExpressive), reskinned per role, each with its own AnimationMixer and
  crossfaded clips:
  - Idle, Walking, Running.
  - Sitting = crouched in cover.
  - Jump = dodge.
  - Punch = gun-fu and the heavy's slam.
  - Death = killed, and knockdown at x2.2.
  - No = the heavy dazed.

  Aiming is procedural on top of the clip: the arm bones turn to point along the aim ray, and the left hand reaches
  for the grip. Heavies reuse the same mesh, scaled 1.45x (Bull 1.7x).
- **Loader:** generic. Any rigged glTF with the same clip names and `UpperArm.R`-style bones drops in; change
  `MODEL_URL` and the `SHADE` table.

**Music** is an original procedural action score in WebAudio: E Phrygian, 126 BPM, four-on-the-floor. Kick, offbeat
bass, hats, claps, pad, stabs and a delayed arp layer in by intensity:
- 0: cards.
- 1: walking.
- 2: gunfight.
- 3: a heavy on camera or a hero dying.

The music ducks when a hero is killed off.

## Bot results (`node bots.js 40`, current values)
| Policy | Ends | Wall time | All 3 survive | Lose 1+ | Mean score | Stars 0..5 |
|---|---|---|---|---|---|---|
| director (breaks script for heroes in trouble) | 100% | 3:09 | 37/40 | 3 | 29.0k | 0/0/0/14/26/0 |
| greedy (breaks script late, for the bonus) | 100% | 3:10 | 29/40 | 11 | 33.3k | 0/0/0/6/23/11 |
| script (never cuts: the studio's edit) | 100% | 3:09 | 1/40 | 39 (Moss 39) | 23.3k | 0/0/0/39/1/0 |
| idle | 100% (Cancelled) | 1:08 | 0 | 40 | 0 | 40/0/0/0/0/0 |

The studio's script loses Moss almost every time. The heavy arrives at 0:26 of Act Two, and the script stays on Juno
until 0:42 and doesn't visit Moss until 1:18. That is the designed trap: breaking script is how you save him.

## What was verified
- The rules run headless in Node (`bots.js`), and all four policies reach an ending every run.
- In real-time headless Chrome, scripted through the DevTools protocol (shots in the session scratchpad):
  - title, act card, cover crouch, aim-down-sights, a heavy's red wind-up with its slam disc
  - a just-in-time cut going off script, the script's countdown cue, a mook's red glint
  - the end card with the medal and the studio's cut, from `http://` and from a real `file://` URL
  - no console errors and no exceptions in any of them
- 60 fps in the built-in browser pane at 1280×720, with the main view plus two graded cameos.
- Test hooks:
  - `index.html#scene=cover|aim|slam|mook|cut|end` sets up each moment and freezes there (press P to unfreeze).
  - `window.KM`: `start`, `press`, `setBot`, `fastForward`, `theatrical`, `glints`, `headCheck`.

## Not verified (only Marwan can judge)
- **Real mouse play.** Pointer lock, mouse sensitivity, and the feel of aiming, cover and dodge with hands on the
  controls. Every check above was driven by bots.
- **The sound.** The music and effects compile and schedule, but the headless runs had no audio output.
- **Whether the script auto-cut is fun or annoying** mid-gunfight.
- **The robot stand-in.** Its big head crowds the shoulder camera, especially when aiming down sights. A human-shaped
  model will frame better.

## Changes from the brief and from the art direction, and why
- **Mouse-aim shoulder camera, cover and on-camera damage (Marwan asked for an Uncharted-style shooter).** On-camera
  hits now cost health, floored by plot armor, so cover matters. Wounds you take on camera stay with that hero once
  you cut away. The old auto-aim and lock-on camera are gone.
- **Mooks take two body shots** (the brief said one), so aiming and headshots matter.
- **ART_STYLE says K's camera never moves during play.** It moves here because Marwan asked for a shooter. The Unreal
  build needs one answer.
- **ART_STYLE §11b asks for silent-film piano.** The score here is the John Wick-style electronic Marwan asked for in
  this session. Conflict to settle.
- **Sets renamed** to the Nitrate rooms (Mill, Railway, Rooftop), plus the Picture Palace (silver) for the finale.
- **Credits:** the stand-in robot is "RobotExpressive" by Tomás Laulhé (CC0), from the three.js examples, loaded from
  jsDelivr. The fonts are under the SIL OFL, from Google Fonts. My scope rule kept me out of `docs/`, so this still
  needs a line in `docs/CREDITS.md`.

## What maps to Unreal, and what won't
- **Maps directly:**
  - Shoulder camera: SpringArm + Camera with a socket offset, its probe for wall collision, and an aim blend of arm
    length and FOV.
  - Two-ray hitscan: line trace from the camera, then from the muzzle.
  - Aiming: Aim Offsets replace the procedural bone pointing.
  - Animation: crossfades become an AnimBP locomotion blendspace with crouch, dodge, death and dazed states.
  - Cameos: SceneCapture2D per room into render targets, captured at 24 fps.
  - Tone per room: the post-process LUTs from ART_STYLE §7, swapped on the cut frame.
  - Shot list: a DataTable per act (time, hero, beat).
- **Needs care:**
  - Off-camera rooms must keep simulating. Skeletal meshes need *Always Tick Pose and Refresh Bones*, or off-camera
    anim notifies (the slam frame) stop firing.
  - Mook cover spots: generate CoverPoint actors from tagged low meshes rather than EQS.
- **Won't port as is:**
  - The studio's-cut replay needs a deterministic headless sim. Keep the rules as plain C++ structs (no Actors,
    physics or anim) so they can be stepped offline. Otherwise, precompute or fake the comparison.
  - The procedural WebAudio score: use composed stems layered by intensity, for example with MetaSounds.

## Open questions for Marwan
1. Should the studio's script **auto-cut**, as now, or only cue so the camera never moves without you?
2. Do on-camera wounds (floored at 12%) keep the "plot armor" fantasy, or should the star be untouchable again?
3. Music: John Wick electronic, or the art doc's silent-film piano?
4. Camera for the Unreal build: free shoulder camera, or Nitrate's fixed cine camera per room?
5. Keep the "studio's cut vs your cut" ending? It is the clearest way to show the theme, but in Unreal it needs a
   deterministic sim.
