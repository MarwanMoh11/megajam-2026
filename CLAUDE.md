# Epic AI MegaJam 2026 entry (theme: Director's Cut)

Unreal Engine 5.9 MegaJam Preview, C++ project, built and packaged on a Windows PC. Solo dev (Marwan) with Claude agents. This is
Marwan's first Unreal project: explain Unreal-specific terms in a few words the first time they come up.

**Hard deadline: Mon Nov 16, 03:59 Cairo. Upload target: Sun Nov 15, 22:00.** Late entries are disqualified.

Read `docs/STATUS.md` first (what the game is, the kill list, what's open). Rules, schedule and prize targets are
in `docs/JAM_PLAN.md`. Setup is `README.md` plus the runbooks in `setup/`.

## Machines and layout
- **PC, Windows: everything Unreal happens here.** Repo at `C:\Jam\megajam`; the Unreal project is `Reel\`
  (`Reel\Reel.uproject`, C++ in `Reel\Source\`; `Reel` is a working name). Start every session in the repo root.
  Builds and logs go to `C:\Jam\Builds`, outside the repo.
- **MacBook: where Marwan sits.** Sessions run on the PC and Marwan reads and steers them from the Mac through
  Remote Control (claude.ai/code); the PC's screen comes through Sunshine and Moonlight. The Mac's clone holds docs
  and the browser prototypes. It has no engine: never build Unreal there, and never commit binary files from it (it
  has no Git LFS).
- **Marwan is rarely looking at the PC's screen.** Check your own work with screenshots and logs. When only they
  can judge something, put it on a short list for them instead of waiting on an answer.
- **You never type a password** (Windows, Claude, Epic, itch.io, GitHub). Say what Marwan has to do through
  Moonlight, carry on with whatever doesn't depend on it, and wait only when nothing else is left.
- **Leave the PC's settings alone.** Setup installs the listed software and turns off sleep. No firewall, account
  or security changes, and no remote-login services, without asking Marwan.
- **No clipboard between the Mac and the PC.** Don't ask Marwan to paste on the PC: do it yourself.

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
- Function bodies: compile with Live Coding, Unreal's in-editor fast reload (`LiveCodingToolset.CompileLiveCoding`,
  or Ctrl+Alt+F11 in the editor). The editor stays open. **18-24 s** on this PC.
- New classes, new `UPROPERTY`/`UFUNCTION`, or changed headers: `ExitEditor`, `Build.bat` from PowerShell, start the
  editor, reconnect. **About 100 s** for a one-file change (build 11 s; clean build 4.3 min). Steps and examples are in
  `docs/UNREAL_MCP.md`.
- Live Coding patches memory only: the next editor start loads the DLL from the last `Build.bat`. Rebuild (editor
  closed) before restarting on Live-Coded changes.
- After an editor restart this session's `unreal-mcp` tools stay dead: Marwan can reconnect them with `/mcp`, or use
  `tools/ue_mcp.py` over HTTP, which always works.
- Live Coding does not reliably re-apply changed constructor defaults to Blueprint children that already exist. Put
  tunable values in `UPROPERTY(EditAnywhere)` fields and change them in the editor instead.

## Known traps in the 5.9 Preview (from Epic's forum post)
- Swapping a Blueprint's root component from Python can crash the editor. Give the C++ base class the root instead.
- Incremental packaging with `-NoCleanStage` can ship stale content. Don't use that switch.
- Web builds: Shipping only, under 500 MB total and 200 MB per file, under 2 GB of memory, no Bink video, and leave
  the Wasm SIMD and allocator settings at their defaults. Open a Web build with its `Play_*.bat`, never `index.html`.

## Commands (worked on this PC in stage 3, 2026-11-13)
Engine `C:\Epic Games\UE_5.9` (5.9.0-60351404; the Launcher doesn't list it in LauncherInstalled.dat). Run the `.bat`
files from **PowerShell**: Git Bash's `cmd //c` mangles the quoted paths. Times are for the untouched template.
```powershell
# build the editor target (editor closed): 11 s for one changed .cpp, 4.3 min clean
& "C:\Epic Games\UE_5.9\Engine\Build\BatchFiles\Build.bat" ReelEditor Win64 Development "-Project=C:\Jam\megajam\Reel\Reel.uproject" -WaitMutex > C:\Jam\Builds\build-editor.log

# start the editor, detached: MCP answers on port 8000 about 30 s later (59 s on the first start after a reboot)
Start-Process "C:\Epic Games\UE_5.9\Engine\Binaries\Win64\UnrealEditor.exe" '"C:\Jam\megajam\Reel\Reel.uproject"'

# package for Windows, in the background: 3.6 min (build 80 s, cook 95 s); output in C:\Jam\Builds\Windows
& "C:\Epic Games\UE_5.9\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun "-project=C:\Jam\megajam\Reel\Reel.uproject" -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -prereqs -archive "-archivedirectory=C:\Jam\Builds" -nop4 -utf8output > C:\Jam\Builds\package-win.log

# zip without the 249 MB .pdb: 14 s, 343 MB
& "C:\Program Files\7-Zip\7z.exe" a -tzip C:\Jam\Builds\TeamName_GameName.zip C:\Jam\Builds\Windows "-xr!*.pdb"
```
- Close the editor before packaging, so Live Coding can't block the build and the editor DLL on disk is fresh.
- `-prereqs` stages `Engine\Extras\Redist\en-us\vc_redist.x64.exe` (5.9 has no `UEPrereqSetup`). Shipping writes no log.
- Driving the editor from a shell: `"/c/Epic Games/UE_5.9/Engine/Binaries/ThirdParty/Python3/Win64/python.exe" -I tools/ue_mcp.py tool <Toolset.Tool> '<json>'`.

The submission zip is `TeamName_GameName.zip`, under 1 GB, made with 7-Zip from the packaged folder.
