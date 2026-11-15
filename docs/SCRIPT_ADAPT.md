# Adapting the game to the script (Nov 15, from 13:30 Cairo)

Marwan's call, Nov 15 13:00: the game follows `docs/SCRIPT.md` (8 scenes, 4 crew, Pendleton). **Nothing built is
thrown away:** every scene below is built from what exists. Where this file disagrees with `docs/STATUS.md` or its kill
list, this file wins. Fallback: tag `pre-script-fallback` (6876e0b) and `C:\Jam\Builds\FALLBACK_pre-script.zip`
(the last packaged build). If the new take is not whole by 20:30 Cairo, Marwan submits the fallback.

## Sessions (one per scene, plus Spine; the coordinator is the session that wrote this file)
| Session | Scope | POV |
|---|---|---|
| spine | sequencer, splice cut, POV looks, renames, 4th crew member, retakes, ending, audio registry | all |
| s1-marquee | street queue, floor manager, palm scanner on the podium, bouncer | Vance |
| s2-van | florist van set, Max, CRT feed, signal-intercept minigame, scanner chime | Max |
| s3-foyer | lobby stair, the critic, dialogue choice, VIP Alibi, main hall life, 4 rope guards | Vance |
| s4-catwalk | bird's-eye view, rigged spotlight, champagne cart, the tracking shot past the guards | Nora |
| s5-auditorium | Pendleton at the podium, house to black, film and beam, side-wall stealth on the timpani, oak door | Vance |
| s6-backstage | enforcers, sound-masked takedown, trunk, counterweight sandbag lure, projectionist, mag-lock | Cole |
| s7-booth | reel swap, Pendleton and bodyguard, fire alarm and strobes, 3-prompt fight, smoke grenade | Vance |
| s8-alley | fire exit, rear alley set, the car and Max driving, guards firing, final shot, credits | Vance |

## What each scene reuses (look before building anything new)
- **Vance = Julian** (camera A, white dinner jacket, Rodin hero, letterbox already drawn for A by `TakeFilmOverlay`).
  **Nora = Maya** (camera B, catwalk, devices). **Cole = Vex** (camera C, backstage, winch). **Max** is new.
- **Theater = the auction building.** The Aurelia Auction House is renamed THE GRAND LUMIÈRE. The street, red carpet
  and queue are Scene 1; the lobby and grand stair are Scene 3's foyer; the great hall west of the velvet rope (bar,
  cocktail tables, band, standing guests) is Scene 3's main hall; the rope wall, its gap and the patrolling guards are
  the rope the guards block; the rows, dais, podium, screen and old film east of the rope are Scene 5's auditorium;
  the projection box / projection room is the booth (Scenes 5 and 7); backstage, the winch and the corridor are Scene
  6; the loading dock and fire exits open onto Scene 8's alley.
- **Scene 1:** `AQueueLine` (the line, the hold, the doorman = Bouncer), `AScannerWatcher` (scanner arch, red/green
  lamps) becomes the palm scanner, `tools/sets/front.py` street. Rain and the marquee are new.
- **Scene 2:** the street's parked cars (a van joins them), the pictures pipeline (scene captures) for the CRT feed,
  the composure meter as Vance's "suspicion" timer.
- **Scene 3:** the lobby clerk greeting (camera ease, turn and greet), `SetLife/*` (waiters, toasts, whispers, the
  spilled drink is the critic's champagne), `APatrolGuard`, `ARoomWatch`.
- **Scene 4:** `AFollowSpot` (already blinds a guard 4 s) = the rigged spotlight; `ACartDevice` = the champagne cart;
  director `bDrawnSightOverlays` (the old drawn sight cones) = Nora's patrol cones, shown only in her view; the
  catwalk at deck z 900.
- **Scene 5:** `AHouseDimmer` (house to 8%, film on the screen), `M_OldFilm`, `ABarClockComponent` (beat timing for
  the timpani steps), `ASoundDesk`, the podium.
- **Scene 6:** `APatrolGuard` (enforcers), `AWinchDevice` (counterweight lever), `ASoundDesk`/`APyroDevice` (thunder
  sheet), the electric-eye breaker (`ABannerDevice`, label Banner) can be the mag-lock override.
- **Scene 7:** the heist's lift and swap (keycard and reel for the dummy), the alarm, the shutter, the Variant_Combat
  template's montages and the 28 Mixamo clips for the fight.
- **Scene 8:** the exits (`ExitSide`, `ExitTrap`), the PRINT IT ending and grade card, the dock.

## The contract (C++)
- Every scene is an `AScriptScene` subclass (`Reel/Source/Reel/Take/Script/ScriptScene.h`): `BeginScene`,
  `TickScene`, `EndScene`, `ResetScene`, `HandleSceneKey`, `DrawSceneHud`; it calls `FinishScene`, `FailScene`,
  `CutTo`, `Say`, `SetFlag`/`HasFlag`/`ConsumeFlag`. Include it as `#include "Script/ScriptScene.h"`.
- `AScriptSequencer` (Spine) plays them in `SceneNumber` order. **`Reel.Script.Jump N`** (console) starts scene N:
  every scene session tests its scene with it.
- Flags: `VipAlibi` (Scene 3, spent by a guard check in Scene 4 or 5).

## File ownership (no two sessions edit the same file)
- **Spine:** `Take/Script/ScriptScene.*`, `ScriptSequencer.*`, and every existing shared core file: `TakeDirector*`,
  `TakeHUD*`, `TakeHudKit*`, `TakePlayerController*`, `TakeCharacter*`, `TakeCamera*`, `TakeFilmOverlay*`,
  `TakeGameMode*`, `TakeGameInstance*`, `TakeTypes.h`, `TakeStyle.h`, `TakeAudio*`, `Watcher.*`, `PatrolGuard.*`,
  `CastMember*`, `TakeFX*`, `Reel.Build.cs`, `tools/build_*.py`, `tools/sets/*` (the existing level pipeline),
  `tools/audio/*`.
- **Scene N:** new files only: `Take/Script/SceneN*.h/.cpp`, `tools/script/sceneN*.py` (tag `script_sN`),
  assets under `/Game/Take/Script/SN/`, audio under `/Game/Take/Audio/Script/SN/`, notes in `docs/script/sN.md`.
  Plus these existing files: S1 `QueueLine.*`, `ScannerWatcher.*`; S3 `SetLife/*`, `RoomWatch.*`; S4 `FollowSpot.*`,
  `CartDevice.*`; S5 `HouseDimmer.*`, `BarClockComponent.*`, `SoundDesk.*`, `tools/sets/puzzle.py`; S6 `WinchDevice.*`, `PyroDevice.*`,
  `BannerDevice.*`, `Shutter.*`; S7 none (new files); S8 `SceneTrigger.*`.
- Need a change in a file you do not own: message its owner (SendMessage to their session) with the exact change.
- **Level edits only by a tagged, re-runnable Python overlay** in your `tools/script/` file, run after the existing
  pipeline, so nothing is lost if the level is reverted. It may hide or move existing actors by name or tag. Place your
  `AScriptScene` actor (and its StartSpot/camera) from that overlay.

## The one editor
- One editor runs; MCP at `http://127.0.0.1:8000/mcp`; shell driver `tools/ue_mcp.py`.
- **Hold the lock for any editor work** (scripts, PIE, save) and for Build.bat: `tools/editor_lock.py acquire <session>
  --note "..."`, release when done, at most ~10 minutes a hold. Do C++, scripts and assets outside the editor while
  you wait.
- **Never a lone `.h` in `Source/`.** A header with a UCLASS/USTRUCT/UENUM and no `.cpp` (or a `.cpp` that does not compile) breaks every build in this shared tree, including others' restarts. Write the `.h` and its `.cpp` together and keep both compiling; draft unfinished code outside `Source/` (e.g. `docs/script/drafts/`) until it builds.
- **Snapshot compile checks** (a copy of the tree built elsewhere, e.g. `C:\Jam\Builds\s2snap`) share UBT's machine-wide mutex with the real build. Run them only while the editor is up and nobody holds the lock for a Build.bat (`tools/editor_lock.py status`), one at a time across sessions, and never while a restart is waiting.
- **Live Coding** (function bodies) compiles everyone's files: keep your files compiling at all times; if the compile
  breaks on someone else's file, message them, do not edit it.
- **New UCLASS / UPROPERTY / header changes need an editor restart.** Batch them. Only the lock holder exits the
  editor, runs `Build.bat ReelEditor`, restarts it; log it in `C:\Jam\Builds\script\restarts.log` (time, who, why).
  Avoid more than one restart an hour across all sessions: ask in the coordinator's session if unsure.
- Save the level only while holding the lock. **Only the lock holder commits `Lvl_Auction.umap`**, right after saving.

## Git
- Shared checkout `C:\Jam\megajam` on `main` (no worktrees: one editor, LFS content, 4-min C++ builds per tree).
- Commit only your own paths (`git add <paths>`, never `-A` or `.`). Retry if `index.lock` is busy. Push after each
  commit; if LFS quota fails, keep committing locally.
- Each session appends its own dated entry to `docs/DEVLOG.md` at its milestones (Glass Box prize).
- Credit imported or generated assets in `docs/CREDITS.md` as you go.

## Timeline (Cairo)
- 13:30 sessions start. Spine compiles the contract with the first restart by ~14:00.
- **15:30: every scene playable from `Reel.Script.Jump N` to `FinishScene`, grey boxes allowed.** Committed.
- 15:30-18:30: dress, voice, feel. Voice lines through `tools/audio` (ElevenLabs pipeline; see STATUS "Step 6").
- **18:30: feature freeze.** Fixes only. Each session reports to the coordinator: done, not done, known bugs.
- 18:30-20:00 coordinator: full run 1-8, fixes routed to owners. 20:00 package (editor closed, nobody holds the
  lock), 21:00 zip checked, 22:00 Marwan uploads.

## Done for a scene
From `Reel.Script.Jump N`: the scene opens on its POV and look, the script's beats play in order with subtitles, the
gameplay works and can fail into a retake, and `FinishScene` cuts to the next scene. Shots in
`C:\Jam\Builds\script\sN\`. A short report to the coordinator: what works, what is not done, what only Marwan can judge.

## Session ids (SendMessage `to`)
| Name | Id |
|---|---|
| coordinator | the session that started you (`startedBy` in list_sessions linked) |
| spine | local_66a049fa-d027-41b3-8bd5-6d14596f7375 |
| s1-marquee | local_baae112b-0265-46da-ae9e-01f1964903c0 |
| s2-van | local_df5684a0-459a-41a5-a16a-285a103fb46f |
| s3-foyer | local_6ec31579-2eb4-4c6c-ae0f-b403c910711e |
| s4-catwalk | local_a10928b1-5ffa-4a08-a819-84b7fc721fee |
| s5-auditorium | local_167acbd1-cccd-445c-a4bf-3e5f2d3fb7e2 |
| s6-backstage | local_589af368-a04a-4a85-a202-bb7e17130a8b |
| s7-booth | local_1cc2b37c-5cbf-4699-b1b9-4fe287f870b7 |
| s8-alley | local_6f919fa8-18f0-4981-b2c5-dc328b3f34e3 |
| puzzle (finishing, then stops) | local_6296ffb1-88d3-49e8-aa3e-a664691c9415 |

## Restarts (every C++ change goes in only here; NO Live Coding, from 14:20)
Live Coding crashed the editor at 14:17: it compiles every changed file in the shared tree, including other sessions'
unbuilt headers and new classes. **Nobody uses Live Coding any more** (no `CompileLiveCoding`, no Ctrl+Alt+F11).
Function bodies and headers alike go in at the coordinator's restarts:

**14:40, 15:10, 15:40, 16:10, 16:40, 17:10, 17:40, 18:10** (Cairo). The coordinator runs each one: it takes the lock
(the queue lets a restart jump ahead), saves the level, closes the editor, `Build.bat ReelEditor`, starts the editor,
and broadcasts "editor up" (or the broken file and its owner). About 3-5 minutes.
- To make a restart: your `.h`+`.cpp` committed 5 minutes before, compiling (a snapshot build of the game target with
  `-NoHotReloadFromIDE` while the editor is up, one at a time, never during a restart). Late = the next one.
- A build broken by your file: the coordinator may apply a one-line fix to bring the editor back and tells you;
  otherwise your files are rolled back to your last good commit for that restart, and you are told.
- After each restart, re-run your overlay script if it places actors your new code needs, and save while holding the
  lock. Overlays must be re-runnable: the level can lose anything unsaved at a crash.
