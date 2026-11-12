# L. The Tight Edit: Final Cut (full prototype, v2, Nitrate look)

Double-click `prototypes/l-final-cut/index.html`. One 1.7 MB file that works offline from `file://`: everything is
inlined (a vendor bundle of three 0.160.0, its GLTFLoader and SkeletonUtils, and `prototypes/style/nitrate.js`, built
with esbuild into one plain script; the stand-in star model as base64; the three fonts as base64). Solver:
`node solve.mjs` (Node 22; it runs the rule code straight out of `index.html`). `index.html` is the source: after the
one-line vendor bundle and the base64 model come the `rules` script (CONFIG, level data, `simulate`, solver) and the
game script (audio, stage, timeline, flow), both unminified; edit them in place (grep for the section banners, don't
open the file whole).

**How to play:** each trap only works while its clip is on screen. Drag the clips on the timeline (or `←/→`), press
`Space` to roll camera: he must survive (Studio Cut), and near misses under 6 frames earn the Director's Cut.

**What to watch for (L's pass condition):** after the first Studio Cut on a scene, does he go back and re-edit to
shave the margins without being asked? Scenes 2, 4 and 6 are where it shows: tuning one clip at a time stalls at
200-250, and the Director's Cut needs a different shape of edit.

## The look: Nitrate (docs/ART_STYLE.md, locked Thu Nov 12)

- **The shared look, not a copy of it.** The grade pass (silver tone), the value-ladder materials, the stencil red and
  gold, the single hard key light and the DOM film overlay (grain, flicker, dust, iris) all come from
  `prototypes/style/nitrate.js`, pasted into the bundle unchanged. Set pieces use the `set` and `floor` roles over a
  procedural wood texture, traps are `heavy` (the darkest values), the backdrop is painted in the backdrop band.
- **Frame:** a 4:3 picture with rounded gate corners in the top 72% of the screen, the edit bench full width below on
  Ink. Left margin: TAKE and SCENE. Right margin: the two targets ("The Studio Cut: survive the take", "The Director's
  Cut" with the score needed and your count in Limelight, gold once reached). A fixed 50-degree camera (a 20 mm lens
  on a 4:3 sensor) frames the whole route, pitched down slightly; it never moves except a 0.15 s shake on a crusher
  impact.
- **Red means danger now:** 0.6 s before a trap arms, a red disc appears under it (on the floor for crushers and saws,
  behind the zone for pistons and sandbags) and its front face turns red; both stay red while it is armed. Nothing
  else is saturated.
- **Gold:** the star's scarf, the close-shave numbers, the Director's Cut medal and counts.
- **Type:** Limelight for titles and numbers, IM Fell English italic for intertitle cards, Josefin Sans for the HUD;
  Paper on Ink. Cards are Ink with two inset Paper borders and corner diamonds.
- **The star is a rigged, skinned model** driven by an `AnimationMixer` with crossfaded clips (about 0.12 s blends).
  The stand-in is RobotExpressive from the three.js examples (CC0), re-skinned by `applyNitrate(model, 'hero')` so he
  is the lightest thing on screen. Clips are matched to roles by name: idle (before the take), run (driven by distance
  along the route, so OVERCRANK slows his stride too; rotated onto the wall for wall-runs), jump (scrubbed across each
  jump, a falling pose on drops), death (from the hit frame; a crusher also squashes him, a piston flattens him),
  cheer (ThumbsUp at the wrap, turned to the audience), plus a procedural flinch (duck and lean away) on every close
  shave. The loader is generic: **drag any rigged `.glb` onto the page** (or Debug, "Load star .glb") to recast him;
  the picture says which roles it found. A rig with none of these clip names still moves along the route in its bind
  pose.
- **Traps move like machines:** in the 0.6 s wind-up a crusher draws back up and trembles, a saw spins up and peeks
  out of its slot, a piston pulls back, a sandbag hoists and sways; on the arm frame it lands within one film frame
  with a squash, dust puffs, and (crusher) the camera shake.
- **Sound:** WebAudio only: a stride-piano rag under each take, the projector's 24 Hz shutter whirr (it stops dead on
  CUT!), a rising rope creak through each wind-up, thud and bell on a crusher impact, cymbal and piano run on a close
  shave, a record scratch on a hit, a short chord for a Studio Cut, a bigger one for a Director's Cut.
- **Ending = the premiere:** full-picture intertitle cards between scenes, each scene played from your best saved take
  (close shaves in slow motion, iris-out on the star), then THE END and a credits card.

## Rules as implemented

- **The star** runs a fixed route through a single-screen room (run, wall-climb, drop, jump) at 4 m/s. The rules only
  see his distance along the route; the room is drawing. Internally distance is an integer (1 unit = 1/96 m at
  24 fps), so the game and the solver can't drift.
- **Traps** (crusher from above, piston from the side, saw from below, sandbag on a rope over a jump) each block a 1 m
  stretch of the route, and are **armed only while their clip is on screen**.
- **Tracks.** `CAM`: trap clips; clips on one track can't overlap (one camera). `TOP` (scenes 5-6): a second video
  track; where a TOP clip covers a CAM clip, the audience sees the TOP clip, so the CAM clip's trap is off. At most
  one trap is armed on any frame. `STAR` (scenes 3, 4, 6): the SLOW clip (OVERCRANK); while it plays he runs at half
  speed, so every trap after it, he reaches later.
- **Lose:** he is inside the armed trap's stretch on any frame (hit, "CUT! MEDIC!"), or the film ends before he reaches
  the WRAP (out of film). Score 0.
- **Win (Studio Cut):** his back edge passes the WRAP alive.
- **Near misses,** per trap and per side, in whole frames: FRONT = the trap was disarmed m frames before he got inside
  it; BEHIND = it armed m frames after he left. m < 6 scores `round(100 * (6 - m) / 6)`: 0 frames 100, 1 frame 83,
  2: 67, 3: 50, 4: 33, 5: 17. A trap can score both sides (a **double**, up to 200) only if it is armed twice, which
  takes the TOP track.
- **Director's Cut:** score >= the solver's best for the scene minus 17 (one frame on one near miss). The card shows
  the threshold, the best possible and how far you are.

**Tick order** (`simulate()` in the `rules` script, once per film frame):
1. The trap whose clip is on screen this frame is armed (TOP over CAM; at most one).
2. The star moves one frame at this frame's crank rate (normal, or half under SLOW).
3. Collision: inside the armed trap = hit, the take ends. Otherwise record per trap the frames he is inside it, the
   last armed frame before and the first armed frame after.
4. Back edge past the exit = cleared. The film runs on to the end, so a trap that arms behind him after he cleared
   still scores.

The game plays back the same `simulate()` result frame by frame, so what you see is what was scored (traps and
scoring step per film frame; the star is drawn between frames for smooth motion, `smoothStar` in CONFIG). The solver uses
an equivalent interval-math fast path; `node solve.mjs --check` compares the two on 45,000 random edits (0
mismatches).

**Events and feedback** (ART_STYLE section 11 for L, plus the editing events)

| Event | When | Feedback |
|---|---|---|
| Drag or nudge a clip | same frame | clip moves with a tick; the picture jumps to the frame that matters for that clip (its slam behind him, its lift in front, or the hit), with the star posed where he is then and any wind-up shown in red |
| Clip over its own trap's pass | while editing | red frames on the clip, a white cross, a red warning in the hint line |
| TOP clip covers a CAM clip | while editing | the covered part of the CAM clip goes dark and hatched, OFF SCREEN |
| Move the SLOW clip | same frame | the white grease-pencil pass marks on the STAR row slide to his new arrival times |
| Blocked nudge or track change | same frame | clip flashes red, low bump |
| Roll camera | same frame | clapper card (Scene n, Take m) for two film frames, then ROLLING with a blinking red dot; clapper snap, projector whirr, piano |
| Trap wind-up | 0.6 s before it arms | red disc under it, its front face red, it draws back and trembles; rising rope creak |
| Trap arms | its first frame | lands within a frame, squash, dust; crusher: 0.15 s shake, thud and bell |
| Close shave (gap under 0.25 s) | the frame he dashes under a just-lifted trap, or it arms behind him | slow motion x0.22 for 3 frames either side; gold "0.04 s!" over the trap, rising and fading over 1.1 s, CLOSE SHAVE under it; the star flinches; the Director's Cut count in the margin updates; cymbal and piano run |
| Hit | the overlap frame | 0.35 s freeze, record scratch, the piano and projector stop dead, death clip (squashed by a crusher), "CUT! MEDIC!" |
| Out of film | last frame | "OUT OF FILM" |
| Take ends | 10 frames after the last event (12 after a hit) | iris-out on the star (0.45 s), then the result intertitle: medal (gold DC disc, or the bone Studio Cut disc), front and behind gaps in seconds per trap (gold when they score), score of a possible best, points to the Director's Cut; the bench shows red grease-pencil brackets with the gaps beside the pass marks and a dashed ghost of the take's clips |
| Survived | take end | short chord; a bigger one and "Print it!" on a first Director's Cut |

**Controls:** drag clips (drag up or down to change track in scenes 5-6), `←/→` nudge 1 frame (`Shift` 6), `↑/↓`
TOP/CAM, `Tab` next clip, `1-5` pick a clip, click the ruler to scrub, `Space` roll (again: skip to the result), hold
`Shift` during a take for x3, `Enter` skip, `R` reset the edit, `N/P` next/previous scene, `Esc` scenes, `D` (or
`` ` ``) debug panel, drop a `.glb` on the page to recast the star. Every tunable is in `CONFIG` at the top of the
`rules` script (rules, feel, and the star: `starHeight`, `runMetresPerCycle`, `windupSeconds`, `smoothStar`); the
panel edits it live and has "Solve this scene", "Load best edit", "Load opening edit", "Force premiere", "Clear
saves" and "Load star .glb". Medals, best scores and
the best edit per scene are saved in `localStorage` (every call in try/catch; a private window just doesn't save).

**Test hook** `window.FC_TEST`: `levels()`, `load(i)`, `getEdit()`, `setEdit(e)`, `play({skip})` (a promise of the
result), `skip()`, `result()`, `state()`, `solve(i)`, `premiere(force)`, `screen()`, `starRoles()`, `save()`, `clearSave()`.

## The tools, and what the solver says

"Best without" removes only the scene's new tool. "One-at-a-time" moves one clip to its best spot anywhere (any track
it may use), keeps it if the score rises, repeats (a strong single-clip player). "Sliding" pushes one clip along its
track until something blocks it (the chore). Both start from 150 random Studio Cuts. "Ways to get close" = distinct
patterns (front/behind/double per trap, the order clips reach the screen, where the SLOW clip sits) within 80% of
the best. All from `node solve.mjs`.

| Scene | New tool | Best | Best without the tool | Studio Cut survive % | One-at-a-time reaches DC | Sliding reaches DC | DC patterns | Ways to get close |
|---|---|---|---|---|---|---|---|---|
| 1 The Corridor | (the cut) | 200 | n/a | 46.9% | 100% | 24% | BB, FB, FF | 3 |
| 2 Up the Wall | (the cut, twisted) | 300 | n/a | 44.3% | 35% | 0% | FBB, FFB | 4 |
| 3 Overcrank | SLOW | 200 | 150 | 31.9% | 35% | 4% | BB/S1, BB/S2, FB/S1, FF/S1 | 4 |
| 4 The Long Way Down | SLOW, developed | 350 | 250 | 28.4% | 1% | 0% | BFBB/S3, BFBB/S4 | 18 |
| 5 Superimpose | TOP track | 300 | 200 | 24.9% | 3% | 11% | DF (trap 1 double) | 1 |
| 6 The Final Cut | SLOW + TOP | 350 | 300 without SLOW, 200 without TOP | 29.6% | 1% | 1% | FBD (trap 3 double) | 10 |

From each scene's opening edit (all six open with a hit that one drag fixes; see the level data): one-at-a-time
tuning reaches 200 / 250 / 200 / 200 / 200 / 200. It reaches the Director's Cut only on scenes 1 and 3, the intros.

**Shipped:**
- **The cut (one CAM track, no overlap):** v1's finding holds. Scene 2 is v1's Take 2: sliding reaches 250 and the
  last 50 needs clip 1 moved to the front of its trap.
- **OVERCRANK (SLOW clip on the STAR track):** shifts every later arrival, so it decides which shots fit. Scene 3
  opens with the clips right and the actor early: slide SLOW to the start and it is a Director's Cut. Scene 4 needs
  SLOW in the middle (before trap 3 or 4) and the clips reordered 2-1-3-4: +100 over no SLOW, and single-clip tuning
  reaches the DC 1% of the time.
- **SUPERIMPOSE (the TOP track):** a TOP clip punches a hole in a CAM clip, so that trap fires twice: the double. In
  scene 5 the 9-frame sandbag clip sits on TOP exactly over the 9 frames he is under the crusher, so the crusher
  lifts as he enters and slams as he leaves (+200) and the sandbag lifts as he reaches it (+100). +100 over no TOP.
  Scene 6 needs both: SLOW at the start, the saw's CAM clip cut by the crusher's TOP clip (a double on the saw).

**Rejected (`node solve.mjs --tools`):**

| Tool | Result | Why it's out |
|---|---|---|
| UNDERCRANK (FAST x2 on the STAR track) | In place of SLOW: 183 vs 200 (scene 3), 283 vs 350 (scene 4), 350 vs 350 (scene 6). Added to scenes 1-2: +0. On scene 3 one-at-a-time reaches the DC 92%. | Weaker than SLOW everywhere and adds nothing alone. SLOW also reads better: slow motion is what film does at a near miss. |
| JUMP CUT (1-frame clip cutting 12 frames of his run) | Scene 2: 300 -> 300. Scene 5: 200 -> 200. Scene 4: 250 -> 300, but all 4 DC patterns get there by skipping a trap. Survival rises 2-6 points. | Mostly an escape valve: it deletes a trap from the film instead of making a closer shave, and "he skipped the trap" doesn't read as a stunt. |
| RAZOR (one cut splits a clip into two free pieces) | Strong: 200 -> 300 (scene 1), 300 -> 400 (scene 2), 150 -> 300 (scene 3), 200 -> 300 (scene 5); one-at-a-time reaches the DC 13-80%; 1-2 ways to get close. | It makes the same payoff as TOP (a trap armed twice: the double) with a second UI (cutting and piece management). TOP couples two traps (the top clip must be another trap's shot, so its timing is pinned) and is the "vertical" move Marwan asked for. **Razor is the backup if TOP doesn't read.** |
| Trimming clip length | not built | A second dial per clip; the double already gives a reason to shorten what's on screen. |

## Level data (`LEVELS` in the rules script)

Common: 4 m/s, star 0.5 m wide, traps 1 m, 24 fps, star starts at route distance 0. Clip starts in frames. "Inside" =
the frames he is inside each trap with no SLOW.

| | 1 The Corridor | 2 Up the Wall | 3 Overcrank | 4 The Long Way Down | 5 Superimpose | 6 The Final Cut |
|---|---|---|---|---|---|---|
| Route | run 13 m | run, wall 4 m, run | run, jump up, run | run, wall, run, drop, run | run, hop, run | run, wall, run |
| Traps (kind @ m) | crusher 4, saw 8 | piston 6 (wall), crusher 10.5, saw 14 | crusher 3, sandbag 5 (jump) | crusher 4, piston 6 (wall), saw 8.75, piston 11.5 (drop) | crusher 4, sandbag 5.5 (hop) | piston 4 (wall), crusher 6.5, saw 8 |
| Clip lengths | 18, 18 | 30, 18, 24 | 18, 12 + SLOW 12 | 24, 18, 24, 12 + SLOW 24 | 30, 9 | 30, 12, 30 + SLOW 24 |
| Film / exit | 84 frames / 10.5 m | 138 / 18 | 56 / 6.5 | 101 / 13.5 | 70 / 7.5 | 91 / 10.5 |
| Inside (frames) | 19-27, 43-51 | 31-39, 58-66, 79-87 | 13-21, 25-33 | 19-27, 31-39, 48-55, 64-72 | 19-27, 28-36 | 19-27, 34-42, 43-51 |
| Opening edit | [28, 46]: +100 on 1, then 2 hits him | [1, 34, 83]: 3 hits him | [1, 19], SLOW parked at 44: 1 hits him | [28, 74, 2, 53], SLOW at 77: 4 hits him | [0 CAM, 40 CAM]: 1 hits him | [1, 46, 59], SLOW at 67: 1 hits him |
| Best edit | [1, 25] = 200 | [1, 40, 88] = 300 | [1, 19], SLOW at 0 = 200 | [28, 10, 61, 85], SLOW at 52 = 350 | [0 CAM, 19 TOP] = 300 | [1 CAM, 55 TOP, 38 CAM], SLOW at 0 = 350 |
| DC threshold | 183 | 283 | 183 | 333 | 283 | 333 |

## Verified

- `node solve.mjs --check`: every stored best equals the solver's; fast path vs `simulate()` 0 mismatches.
- From `file://` in Chrome (headless, own temp profile, driven over DevTools), on the final Nitrate build: the page
  loads offline with WebGL and the embedded fonts and model; the star's clips map to idle, run, walk, jump, death and
  cheer; every scene's solver-best edit played through the game scores exactly the solver's best (200 / 300 / 200 /
  350 / 300 / 350, all Director's Cuts); a real-time take on scene 3 scored 200; saves persist; no console entries.
- In the app's built-in browser (over a local http server, because the pane won't open a large `file://` page), on the
  Nitrate build: title, the 4:3 gate with the bench below, the margins, the red wind-up disc and red face, the gold
  "0.04 s!" close shave with the live count, the star's run, flinch and death, "CUT! MEDIC!", the iris-out into the
  result card with the DC medal, the red seconds brackets on the bench, the premiere's full-picture cards and scene
  playback; scene 1 (200) and scene 5 (300, the double) in real time; no console errors.
- Earlier, on the pre-Nitrate build (same game code paths for input): real mouse clicks through title, reels and the
  intro card; a real mouse drag in scene 1 then `Space`: 183; a real drag of the sandbag clip up onto TOP then
  `Space`: 300; `Esc` to the credits and a click back to the reels.

**Not verified, Marwan's call:** feel, look, sound and pacing; how the robot stand-in reads at this camera distance
(he is about a tenth of the picture's height in the widest rooms); whether the TOP track and the double read on a
first try; whether a cold player finds SLOW without the hint; Safari (only Chrome-family browsers were used); a real
Tripo or Rodin `.glb` (the drop-in path was written for one but only exercised with the robot).

## What won't carry over to Unreal

- **The fixed 24 fps tick.** Run `simulate()` on a fixed 1/24 s accumulator in C++ with integer route units; render
  interpolated (or stepped, for the film look). Port it as is: it is integer interval math, no physics.
- **Traps that snap in one frame.** A 3D crusher needs anticipation (shake before its clip starts) and a drop that
  finishes inside one frame, or a 1-frame near miss reads as a hit.
- **The near-miss read** needs a side-on camera at the moment; keep the side-on 2.5D stage or cut to a side-on
  replay cam.
- **The timeline in UMG** (drag with snapping, track change, hop to the nearest free spot, pass marks, hidden parts,
  red overlaps, ghosts, margin badges, the moving STAR pass marks) is still the riskiest piece. Build it first.
- **The look** maps onto ART_STYLE as written: the grade is the silver LUT, the film overlay is the `WBP_FilmGate`
  widget, the wind-up red is a material swap plus a decal.
- **Animation driven by distance.** The run cycle's time comes from distance along the route, so it stays locked to
  the 24 fps rule steps and to OVERCRANK; in Unreal, drive the AnimBP's play rate (or a pose-by-distance node) from
  the rule step, not from velocity. Jumps are scrubbed by progress across the jump, the same way.
- **The solver stays JS.** Design levels with it offline, copy the data into a DataTable, and add an automated test
  in Unreal that plays each scene's best edit and checks the score.
- WebAudio synth and procedural textures are placeholders.

## Open questions for Marwan

1. **Does the double land?** It is the "wow" candidate: the crusher lifts as he enters and slams as he leaves. Scene 5
   is its intro. If it doesn't read, razor is the measured backup.
2. **Kill list conflict:** the draft kill list says "moving clips between tracks". The TOP track is exactly that. Keep
   TOP (and update the kill list), or fix each clip's track per scene?
3. **Scene 5 is narrow:** one way to the Director's Cut. Fine for an intro, or add a third trap?
4. **Openings fail on purpose** so a judge can't press Space through the game. Too punishing for a cold start?
5. **Pass marks on** make it a packing puzzle you can reason about; off (`showPassMarks` in the panel) it is trial
   and error. Which one makes you want another go?
6. **Takes are 2.5-6 s** plus slow motion. `Shift` x3 and `Enter` skip exist. Too long between edits?
7. **Six scenes:** two per tool is thin Nintendo-style ramping (intro, develop/combine). More scenes would break the
   kill list; more traps per scene is the cheaper way to deepen it.
8. **The stand-in star** is a robot at about a tenth of the picture's height. Does the side-on 4:3 framing need the
   rooms smaller (or the camera closer) once real character models arrive?
