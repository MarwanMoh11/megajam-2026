# Spine: the script contract as built (commit 00a71de, Nov 15 13:40)

Headers: `Reel/Source/Reel/Take/Script/ScriptScene.h`, `ScriptSequencer.h`. Include `"Script/ScriptScene.h"`.

## What the sequencer does for you
- `Reel.Script.Jump N` (console): starts scene N now (skips the title card). Also `Reel.Script.Finish` (active scene
  finishes) and `Reel.Script.Fail` (active scene fails -> retake). Play from the start: title card, then scene 1.
- On every scene start (and retake): `ResetScene()`, splice cut (white flash + rolling frame line + clapper,
  0.35 s), slug line (`Slug`, 3.5 s), the POV character teleported to `StartSpot` (yaw from its rotation) and the
  player put on it (`PC->SnapTo`), view on `SceneCamera` if set, then `BeginScene()`.
  - `Pov`: Vance = Julian (camera A), Nora = Maya (B), Cole = Vex (C). **Max has no body**: set `SceneCamera`; the
    player is on no character (WASD does nothing; read keys in `HandleSceneKey`).
  - `bPlayerWalks = false`: the player is taken off the character too (no WASD/mouse look); your scene drives it.
- Look: Vance = letterbox, grain, film. Nora/Cole/Max = no bars, no grain, no viewfinder.
- `TickScene(Dt)` every frame while active (not during a retake burn or the end). `GetSceneSeconds()`.
- `HandleSceneKey(Key, bPressed)`: EVERY key edge (press and release) while your scene is active, before anything
  else. Return true to consume: a consumed E / LMB / R / Enter won't reach the director. Keys 1/2/3 are swallowed
  (no free switching) unless you consume them first.
- `DrawSceneHud(UCanvas*)`: called by ATakeHUD every frame over the picture, under the slug/splice/retake layers.
- `Say(Speaker, Line, Sound)`: subtitle through the HUD's lane ("SPEAKER: line"), plays Sound 2D if given.
- `CutTo(Pov, Camera)`: splice cut inside the scene; does NOT move people (StartSpot is only for the scene's own
  start POV). `CutTo(EScriptPov::Vance)` puts the player back on Vance's follow camera.
- `FailScene(Reason)`: the frame burns, "CUT!" card with a line for the reason, 1.6 s, then the scene restarts
  (`ResetScene`, StartSpot, `BeginScene`). Reasons with canned lines: Seen, Caught, Time, Suspicion, Hit; any other
  FName is shown as written (underscores -> spaces). `GetRetakes()` counts them (hints after 2).
- `FinishScene()`: cut to the next scene by SceneNumber; after the last: "Cut. PRINT IT." then credits over
  sprockets; Enter runs it again.
- R during play = retake the active scene (unless your scene consumes R).
- Flags: `SetFlag/HasFlag/ConsumeFlag` (e.g. VipAlibi).
- `GetPovCharacter()` (ATakeCharacter*, null for Max); `GetSequencer()->GetTune(Name, Default)` reads the placed
  sequencer's `Tunes` map (tune without a restart).

## The old flow while the script runs (`ATakeDirector::bScriptDriven`)
Off: the alarm/warnings, intro lines, composure and its bloopers, scene triggers, mark actions, StepScene,
objectives, the heist, the opening slate. Still on: devices and Use (E), watchers/guards patrolling, sight, the look.

## Placing your scene
Your own `tools/script/sceneN.py` overlay (tag `script_sN`). The sequencer is placed by `tools/script/spine.py`
(tag `script_spine`, label `Script_Sequencer`). Scenes are found by class on BeginPlay, sorted by SceneNumber.
