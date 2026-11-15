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
  `CartDevice.*`; S5 `HouseDimmer.*`, `BarClockComponent.*`, `SoundDesk.*`; S6 `WinchDevice.*`, `PyroDevice.*`,
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
