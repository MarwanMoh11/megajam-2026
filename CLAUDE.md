# Epic AI MegaJam 2026 entry (theme: Director's Cut)

Unreal Engine 5.9 MegaJam Preview, C++ project, built on Windows. Solo dev (Marwan) with Claude agents. This is
Marwan's first Unreal project: explain Unreal-specific terms in a few words the first time they come up.

**Hard deadline: Mon Nov 16, 03:59 Cairo. Upload target: Sun Nov 15, 22:00.** Late entries are disqualified.

Read `docs/STATUS.md` first (what the game is, the kill list, what's open). Rules, schedule and prize targets are
in `docs/JAM_PLAN.md`. Setup is `README.md` plus the runbooks in `setup/`.

## Layout
- Repo root (on the PC: `C:\Jam\megajam`): this file, `docs/`, `setup/`, `.mcp.json`. Start every session here.
- `Reel/`: the Unreal project (`Reel/Reel.uproject`, C++ in `Reel/Source/`). `Reel` is a working name.
- Packaged builds and their logs go to `C:\Jam\Builds`, outside the repo.
- The Mac holds a second clone, for reading the docs and posting the dev log. It has no engine: never build there.

## Working rules
- **Check the kill list in docs/STATUS.md before building anything.** If it isn't in the spec, mention it in one line
  and don't build it.
- **Fun gate first.** Grey boxes only until Marwan has played the core loop and wants another go. No final art,
  audio or menus before that. If it isn't fun as boxes, change the mechanic, not the art.
- **Every press does something visible in the same frame.** Nothing waits for a tick, a timer or an animation.
- **A judge plays for 2-5 minutes, cold.** Easy start, instant retry, the ending reachable, how to play clear in 10 s.
- **One editor, one asset writer.** `.uasset` and `.umap` files are binary and can't be merged. Only one session
  edits assets through the editor at a time. Parallel agents work on `Source/` C++, docs, and asset generation
  outside the editor, each on its own files.
- **Gameplay logic lives in C++** (text: reviewable, diffable). Blueprints are for tuning values, widgets and wiring
  levels. Expose anything tunable as `UPROPERTY(EditAnywhere)` so it changes without a recompile.
- **Check your work in the editor.** After a change, look: a screenshot, a play-in-editor run, or the log. Don't
  report something as working from the code alone. Say plainly what only Marwan can judge (feel, look, sound).
- **Save, commit and push at every working milestone.** The Preview engine can crash and lose unsaved work. If a
  push fails on Git LFS quota, keep committing locally and say so once: the push is only a backup.
- **Keep the dev log.** At each milestone append to `docs/DEVLOG.md`: time, what changed, what the agents did, what
  went wrong. It is the entry for the Glass Box prize, so write it as it happens.
- **Credit as you import.** Any asset not made here goes in `docs/CREDITS.md` with its source and licence, and every
  AI tool used goes in the tools list there. The submission form asks for both.

## Unreal MCP (how Claude drives the editor)
- Server: `http://127.0.0.1:8000/mcp`, started by the editor. The editor must be running before the session starts;
  after an editor restart, reconnect.
- The **All Toolsets** plugin must be enabled, or there are no level, actor, Blueprint or Python tools.
- Tools are found through `list_toolsets`, `describe_toolsets`, `describe_tools` and `call_tool`.
- Claude Code stops waiting for a tool call after about 120 s. Split long work (Live Coding builds, big scripts,
  asset scans) into shorter calls. Run packaging in the background with a log file and read the log.
- Import tools default to `bReplaceExisting=false` and `bSave=false`: set them on purpose.
- To restart the editor: `ExitEditor` in `EditorToolset.EditorAppToolset`, start it again, reconnect.

## C++ changes
- Function bodies: Live Coding (Ctrl+Alt+F11 in the editor, or the Live Coding toolset).
- New classes, new `UPROPERTY`/`UFUNCTION`, or changed headers: close the editor, build, reopen. Live Coding does
  not pick these up reliably.

## Known traps in the 5.9 Preview (from Epic's forum post)
- Swapping a Blueprint's root component from Python can crash the editor. Give the C++ base class the root instead.
- Incremental packaging with `-NoCleanStage` can ship stale content. Don't use that switch.
- Web builds: Shipping only, under 500 MB total and 200 MB per file, under 2 GB of memory, no Bink video, and leave
  the Wasm SIMD and allocator settings at their defaults. Open a Web build with its `Play_*.bat`, never `index.html`.

## Commands (stage 2 fills in `UE`; stage 3 replaces these with the ones that worked)
```
set "UE=C:\Program Files\Epic Games\UE_5.9"
set "PROJ=C:\Jam\megajam\Reel\Reel.uproject"

:: build the editor target (editor closed)
"%UE%\Engine\Build\BatchFiles\Build.bat" ReelEditor Win64 Development -Project="%PROJ%" -WaitMutex

:: package Windows, Shipping (run in the background; read the log)
"%UE%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%PROJ%" -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -prereqs -archive -archivedirectory="C:\Jam\Builds" -nop4 -utf8output > "C:\Jam\Builds\package.log" 2>&1
```
The submission zip is `TeamName_GameName.zip`, under 1 GB.
