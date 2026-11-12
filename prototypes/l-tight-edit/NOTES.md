# L. The Tight Edit: browser grey box

Open `prototypes/l-tight-edit/index.html` in any browser (one self-contained file, no network). Solver from the
command line: `node solve.mjs` (Node 22 was used; it runs the rule code straight out of `index.html`).

**What to watch for (the pass condition):** after the first Studio Cut on Take 2 or 3, do you go back and re-edit to
shave the margins without anyone asking? The debug panel (`D`) counts it: takes after the first Studio Cut, and edits
after it.

## Rules as implemented

- **The scene.** A star (capsule) runs right along a fixed path at a fixed speed, past three crushers, to a WRAP line.
- **The edit.** One timeline track, gridded in film frames (24 fps). Each crusher has one clip of fixed length.
  A crusher is down (slammed on the floor) on every frame its clip covers, and up otherwise. You can slide clips; you
  can't trim them.
- **The coupling: one camera.** All clips share one track and can't overlap ("a crusher only drops while it's on
  screen, and there's one camera"). Dragging a clip into another hops it to the nearest free spot; nudging stops at
  the neighbour.
- **Lose:** the star overlaps a down crusher on any frame. Crushed ("CUT! MEDIC!"), score 0.
- **Win (Studio Cut):** the star's back edge passes the WRAP line alive.
- **Near miss:** for each crusher, the margin is the gap in whole frames between the star being under it and it being
  down: behind (dropped `m` frames after he cleared it) or in front (lifted `m` frames before he got under it).
  A margin under `nearMissFrames` (6 frames = 0.25 s) scores `round(100 * (6 - m) / 6)`: 0 frames 100, 1 frame 83,
  2 frames 67, 3 frames 50, 4 frames 33, 5 frames 17. Score = sum over the three crushers, max 300.
- **Director's Cut:** score >= the solver's best possible score for the level minus `dcSlack` (17, one frame on one
  crusher). The card shows the threshold, the best possible score and how far you are from it.

**Tick order** (`simulate()` in the `rules` script, once per film frame):
1. Each crusher takes its state from the edit: down if any clip that drives it covers this frame.
2. The star moves one frame (`starSpeed / fps`). (Only with the rejected `starWaits` rule: it stops at the edge of a
   down crusher.)
3. Collision: overlapping a down crusher = crushed, the take ends. Otherwise record the frames he's under each crusher
   and the last down frame before / first down frame after, for margins.
4. Back edge past `exitX` = cleared. The frame loop continues to the end of the track, so a slam behind him after he
   has cleared still counts.

The game plays back the same `simulate()` result frame by frame, so what you see is exactly what was scored. The
star moves in 24 fps steps on purpose: in slow motion you see the frames you are counting.

**Events and feedback**

| Event | When | Feedback |
|---|---|---|
| Clip starts | its first frame | crusher drops in one frame, dust, low thud, small shake; viewfinder brackets and REC around it while its clip plays |
| Clip ends | the frame after it | crusher snaps up, soft click |
| Near miss | the slam frame (behind) or the frame he gets under (in front) | slow motion x0.25 from 2 frames before to 2 after; white flash and shake scaled by closeness; two-note beep that rises with closeness; big text "SHAVED! 0 FRAMES" / "1 FRAME!" / "n FRAMES" with "right behind him / lifted just in time +pts" |
| Crushed | the overlap frame | 0.3 s freeze, red flash, big shake, falling buzz, star flattens, "CUT! MEDIC!" |
| Cleared | back edge past WRAP | three-note arpeggio, "THAT'S A WRAP" |
| Take ends | 8 frames after the last event (16 after a death) | result card: medal, margin per crusher in frames, closest marked, score, best possible, points to go; frame badges over each clip on the timeline; dashed ghost of the last take's clips while you re-edit |
| First Director's Cut on a level | take end | four-note fanfare, star on the level list |
| Drag or nudge | same frame | clip moves with a tick; a blocked nudge flashes the clip red with a low bump |
| A clip over its own crusher's pass mark | while editing | red overlay and a cross on the overlap |

Controls: drag a clip, `←/→` nudge 1 frame (`Shift` 6), `Tab` or `1-3` pick a clip, `Space` roll or replay,
`Enter` skip to the result, hold `Shift` during a take for x3, `R` reset the edit, `N/P` next/previous level,
`D` (or `` ` ``, F2) debug panel. Every tunable is in `CONFIG` at the top of the `rules` script; the panel edits it
live and has "Solve this take", "Solve all", "Load best edit" and "Load opening edit".

## The coupling: what the solver says

Two tuners stand in for a player who works one clip at a time:
- **Slide**: push one clip toward the star until it nearly touches (never through death, another clip or the track's
  end), keep it if the score rises. This is the chore the brief worried about.
- **Best-response**: move one clip to its best spot anywhere on the track (it can hop clips and cross its own mark),
  keep it if the score rises. A strong single-clip player.

Each starts from random Studio Cuts (150 per level here, 60 per random geometry). "DC" = how often it reaches the
Director's Cut. `node solve.mjs --compare` reproduces this (about 80 s).

| Coupling | Take 1 survive / best | Take 2 survive / best | Take 3 survive / best | Best-response reaches DC (T1/T2/T3) | Slide reaches DC (T1/T2/T3) |
|---|---|---|---|---|---|
| Independent: 3 tracks | 42% / 300 | 37% / 300 | 29% / 300 | 100 / 100 / 100% | 100 / 100 / 100% |
| **One track, no overlap (chosen)** | 47% / 300 | 44% / 300 | 38% / 250 | **90 / 34 / 50%** | **5 / 1 / 5%** |
| One clip drives two crushers (2 clips) | 37% / 200 | 24% / 200 | 29% / 200 | 100 / 100 / 100% | 100 / 100 / 100% |
| Star waits at a closed crusher | 60% / 300 | 76% / 300 | 64% / 300 | 99 / 93 / 91% | 100 / 89 / 93% |
| One track + star waits | 65% / 300 | 78% / 300 | 69% / 250 | 89 / 45 / 26% | 7 / 6 / 4% |

Across 20 random level geometries, the levels where best-response reaches the DC less than half the time:
independent 0/20, **one track 18/20**, shared clip 1/20, star waits 1/20, one track + waits 18/20.

- **Rejected: independent tracks.** Every clip tunes alone: always 100%. The chore.
- **Rejected: one clip drives two crushers.** One clip with two targets just has one sweet spot, and tuning still
  finds it every time. It also caps the score at 200.
- **Rejected: star waits at a closed crusher.** Moving clip 1 shifts every later arrival, but tuning in order 1, 2, 3
  still solves it (91-99%), and it makes surviving too easy (60-78%). Waiting also hands out free "in front" near
  misses unless you special-case it.
- **Chosen: one track, no overlap.** Clips compete for the same stretch of time, so the best edit often needs one clip
  moved to the *other* side of its crusher (front instead of behind) to make room for the next, or the clip order
  changed. Single-clip sliding almost never finds that (1-5%). Adding "star waits" on top adds nothing the solver can
  see except easier survival, so it stays off.

## Levels (all numbers from `node solve.mjs`)

Common: `starSpeed 4 m/s`, star 0.5 m wide, crushers 1 m wide, star starts at x = 0, 24 fps.
Clip starts are in frames. Patterns: F = near miss in front, B = behind, - = none; then the clip order on the track.

| | Take 1: The Corridor | Take 2: The Long Clip | Take 3: Backwards |
|---|---|---|---|
| Crushers x (m) | 4, 9, 14 | 6, 10.5, 14 | 4.5, 7.5, 12 |
| Clip lengths (frames) | 18, 18, 18 | 30, 18, 24 | 18, 30, 24 |
| Track / WRAP | 120 frames / 18 m | 138 / 18 | 120 / 16 |
| Star under each crusher (frames) | 19-27, 49-57, 79-87 | 31-39, 58-66, 79-87 | 22-30, 40-48, 67-75 |
| Opening edit | [0, 46, 96]: near miss on 1 (1 frame), then crushed by 2 | [66, 96, 114]: survives, score 0 | [102, 72, 10]: clips in reverse order, survives, score 0 |
| Edits on the grid / survive | 314,364 / 46.8% | 314,364 / 44.3% | 124,950 / 38.2% |
| Best possible | 300, at [1, 31, 61] (4 edits) | 300, at [1, 40, 88] (2 edits) | 250, at [4, 49, 79] (2 edits) |
| Director's Cut threshold | 283 | 283 | 233 |
| Ways to the DC | 16 edits, 4 patterns (FFF FFB FBB BBB) | 8 edits, 2 patterns (FFB FBB) | 5 edits, 2 patterns (FBB BBB); 11 patterns within 80% of best |
| Slide from the opening | dead (the opening needs a real fix first) | 250, stalls short of the DC | 100 |
| Best-response from the opening | 300 | 300 (needs clip 1 hopped to the front of crusher 1) | 100 |
| From random Studio Cuts: best-response / slide reach DC | 90% / 5% | 33% / 1% | 51% / 5% |

The ramp: Take 1 teaches the rules and gives a free near miss in the first second before a death. Take 2 opens as a
boring Studio Cut; nudging gets 250 and the last 50 needs the insight. Take 3 opens backwards, nothing single-clip
gets past 100, and 300 is impossible (the readout says best possible 250).

Checked in the browser (built-in browser pane, scripted input and the `TE_TEST` hook): each level's solver best
edit, played in real time, scored exactly 300 / 300 / 250 and got the Director's Cut; the opening edits behave as
listed; real mouse drag, arrow nudge, Tab, Space, N and D all work; no console errors. The in-page solver prints the
same numbers as node.

## What won't carry over to a 3D Unreal build

- **The fixed 24 fps rule tick.** Unreal ticks at a variable frame rate. The rules must run on a fixed 1/24 s step
  (an accumulator in C++), with visuals interpolated, or game and solver drift. Port `simulate()` as is: it is 1D
  interval math, no physics.
- **Instant crusher drops.** A 3D crusher needs a fall animation. Keep the rule "down = inside its clip" and make the
  visible drop finish within one frame (anticipate with a shake before the clip starts), or a 1-frame near miss looks
  like a hit.
- **Reading the gap.** In 2D side-on, a frame is 9 px of visible gap. A 3D camera must also be side-on (or cut to a
  side-on replay cam) at the near miss, or the margin can't be seen.
- **The timeline in UMG.** Drag with snapping, hop-over placement, pass marks, ghosts and badges were cheap on a
  canvas. In UMG this is the riskiest piece of the build. Budget for it first.
- **Pass marks** only work because the star's path doesn't depend on the edit (true under the chosen rule).
- **The solver** stays JS: use it offline to design levels (`--search`), then copy the level data into a DataTable.
  Add an automated test in Unreal that plays each level's solver-best edit and checks the score.
- WebAudio beeps and flat shapes are placeholders.

## Open questions for Marwan

1. **Pass marks on or off?** On (default), it's a packing puzzle you can reason about. Off (`showPassMarks` in the
   panel), it becomes trial and error by watching takes. Which one makes you want another go?
2. **Take 3's best is 250, not 300.** The card says so. Satisfying ("found the best possible") or a let-down?
3. **Near-miss window:** 6 frames feels generous? Try 4 in the panel (the solver numbers change; press "Solve all").
4. **Take length:** measured 3.6 s for a death, 4.4 s for a clean run, 6.7 s with three near misses in slow
   motion. Too long between edits? (`Shift` x3 and `Enter` skip exist.)
5. **24 fps stepped motion:** reads as film, or as choppy?
6. **Trimming clips** (changing length) isn't built. It adds a second dial per clip; only worth it if sliding alone
   runs dry.
7. **The card shows the Director's Cut threshold.** It's a goal, not a request, but it does nudge. Hide it to test
   the pass condition more strictly?
