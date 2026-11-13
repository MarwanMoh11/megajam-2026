# Status

Last updated: 2026-11-13. Theme: **Director's Cut**.

**Picked: K, Meanwhile...** (Marwan, Fri Nov 13, about 11:30 Cairo, after trying the 3D prototype
`prototypes/k-3d/` and L's `prototypes/l-final-cut/`). The idea gets refined with Marwan before the spec below is
written; L is no longer a candidate. The Unreal project `Reel` exists on the PC; no game code yet.

## Locked for now: two candidates

One of these becomes the game. The pick is made after you have played both browser grey boxes in `prototypes/`: Thursday evening if they are ready, **Fri Nov 13, 10:00 Cairo** at the latest.
Reasoning and the full tier list: docs/BRAINSTORM.md, section 7.

### K. Meanwhile... (cross-cutting action)
- **Fit:** the director's cut is the cross-cut. Three heroes in three places, each with enemies closing in. The one
  on screen has plot armor and fights; the others are helpless until you cut back.
- **Promise (first 30 s):** "I left him too long. Cut. Just in time!"
- **Grey box (~90 min):** two rooms, one hero each, boxes that walk at the hero, one cut key, a danger bar for the
  room you left, a bonus for cutting back late.
- **Passes if:** you start leaving a room late on purpose to chase the bonus.
- **Kill list (draft):** story and dialogue; more than three heroes or rooms; different abilities per hero; a shop
  (one upgrade pick at most, and only after the fun gate); inventory; saves; settings beyond volume and sensitivity.

### L. The Tight Edit (timeline puzzle with a stunt payoff)
- **Fit:** the director's cut is your edit. Each trap in the scene is a clip on a timeline; slide and trim the
  clips, press play, and the star runs the gauntlet on a fixed path. Surviving earns the Studio Cut; near misses
  earn the Director's Cut.
- **Promise (first 30 s):** "He lived. Now make it closer."
- **Grey box (~2 h):** a capsule on a fixed path past three crushers, three clips you can slide, a play button, a
  near-miss readout.
- **Passes if:** after surviving, you re-edit to shave the margins without being asked.
- **Kill list (draft):** free movement for the star; physics-driven traps (fixed timings only, so a solver can
  prove levels); moving clips between tracks; a level editor; story; more than six levels.

**Fallback:** M. Off-Screen (shove enemies out of the camera frame), tested only if both fail.

## How the pick is made
1. Play K, then L, cold, a few minutes each.
2. Pick the one you want another go at. If it's a tie, K: it is the cheaper build.
3. If neither passes, grey-box M for an hour. If that fails too, reopen tier A in the brainstorm.

## To write on Thursday, after the pick and before building
- The win and lose conditions.
- The order rules resolve in each tick.
- A table of game events to feedback (sound, effect, when each shows).
- The final kill list.
- The look: one film era, locked (brainstorm section 6).

## Not decided yet
- Game name and team name (the zip must be `TeamName_GameName.zip`).
- Solo or team.
- Whether the Web build ships (decided Friday, after the first Web package). Stage 3 (Nov 13) didn't package it:
  it needs emscripten 6.0.10 (emsdk, about 0.5-1 GB, not on any install list: waiting for Marwan's OK), and the
  AllToolsets plugins need `"PlatformDenyList": ["Wasm"]` in `Reel.uproject` or the cook crashes (Epic's
  `web-deploy` skill). Windows ships either way.
- Student category: the Official Rules are still unread.
