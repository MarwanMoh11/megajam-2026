# Stage 2: the Unreal project

Stage 1 is done: the engine is installed through the Launcher and Visual Studio has finished. This stage creates the
real jam project inside this repo and gets the editor running with the Unreal MCP server (the local server through
which Claude drives the editor), ready for a new session to connect to.

You are in an SSH session on the PC; Marwan watches its screen from a MacBook through Moonlight. Anything with a
window starts through `tools\on-desktop.ps1`, and there is no clipboard between the Mac and the PC (stage 1 has the
details).

**Done when:** `Reel\Reel.uproject` exists as a C++ project and builds from the command line; the All Toolsets
plugin is on; the editor is open and answering on port 8000; the paths under "Commands" in `CLAUDE.md` are real; and
the project is committed and pushed.

**This runbook has never been run.** Paths, file names and plugin names are best guesses from Epic's documentation:
look on disk before relying on them, and correct this file when something differs. "`C:\Jam`" means the folder that
holds this repo. `Reel` is a working name for the project; the game's displayed title can change later, the project
name can't cheaply.

## 1. Find the engine and check the toolchain

- Engine folder: read `C:\ProgramData\Epic\UnrealEngineLauncher\LauncherInstalled.dat` (JSON with each install's
  location), or look under `C:\Program Files\Epic Games\`. Confirm the version in `Engine\Build\Build.version`.
  Call this folder `UE` below.
- If no 5.9 install is there, stop and tell Marwan: the engine download has to finish first.
- Toolchain: `UE\Engine\Config\Windows\Windows_SDK.json` lists the compiler and Windows SDK versions this engine
  accepts. Compare them with what is installed (`vswhere.exe -all -products * -format json`, and the folders under
  `C:\Program Files (x86)\Windows Kits\10\Include`). Add anything missing with the Visual Studio Installer
  (`setup.exe modify --installPath <path> --add <component id> --passive --norestart`), in the background with a log.
- Web target: list `UE\Engine\Platforms` and note what the browser platform is called in this build (Web, Wasm or
  HTML5) and whether its files are present. Write the name in `docs/MACHINE.md`; stage 3 needs it. If it is absent,
  write that: the game ships on Windows only, and nothing else changes.

## 2. Ask Marwan to create the project (the one step that needs the wizard)

There is no supported command line for creating a project from a template, so give Marwan these clicks, to do
through Moonlight, and end your turn. Carry on when they reply.

1. Epic Games Launcher > Unreal Engine > Library > **Launch** on 5.9 MegaJam Preview.
2. In the project browser: **Games** > **Third Person**. If it offers variants, keep the plain one.
3. Project defaults on the right: **C++** (not Blueprint), Target Platform **Desktop**, Quality Preset **Maximum**,
   **Starter Content off**.
4. Project Location: the folder of this repo (for example `C:\Jam\megajam`). Project Name: `Reel`.
5. **Create**. The first compile takes a few minutes, then the editor opens. Visual Studio may open as well; close
   it. Press Play (Alt+P) once to see the character run, then **close the editor** and reply "created".

If the wizard reports missing components or the compile fails, read the newest log under `Reel\Saved\Logs\`
yourself, or ask Marwan what the dialog says. Fix it (usually a Visual Studio component from section 1) and have
them retry.

## 3. Wire the project up (editor closed)

- Check `Reel\Reel.uproject` and `Reel\Source\` exist. No `Source` folder means a Blueprint project was created:
  ask Marwan to delete `Reel\` and redo step 2 with C++.
- If the wizard wrote `Reel\.vsconfig`, compare it with the installed Visual Studio components and add any missing.
- **Plugins.** Search the engine's `.uplugin` files for the one whose friendly name is "All Toolsets", and for the
  Model Context Protocol server plugin. Add each to the `Plugins` list in `Reel.uproject` as
  `{ "Name": "<file name without .uplugin>", "Enabled": true }` unless it is already enabled by default. All
  Toolsets is required: without it the agent connects but has no tools for levels, actors or Blueprints (a known
  bug in this Preview, UE-412135).
- **`.mcp.json`** is already at the repo root and points Claude Code at `http://127.0.0.1:8000/mcp` under the name
  `unreal-mcp`. The first session that sees it asks Marwan to approve the server; that is expected.
- **Command-line build.** Build the editor target once to prove the path works without the editor, and time it:
  ```
  "<UE>\Engine\Build\BatchFiles\Build.bat" ReelEditor Win64 Development -Project="<repo>\Reel\Reel.uproject" -WaitMutex
  ```
- Fill in the `UE` path under "Commands" in `CLAUDE.md`, and correct any command there that differed from what worked.

## 4. Start the editor and wait for the server

- Start the editor on the desktop, with the `.uproject` path as its argument:
  ```
  powershell -ExecutionPolicy Bypass -File tools\on-desktop.ps1 "<UE>\Engine\Binaries\Win64\UnrealEditor.exe" "C:\Jam\megajam\Reel\Reel.uproject"
  ```
- Wait, in one bounded loop of at most five minutes, for port 8000 on 127.0.0.1 to accept a connection
  (`Test-NetConnection`). The first launch compiles shaders, so it is slow.
- If the port never opens: read the newest log in `Reel\Saved\Logs\` for lines about MCP or Agentic Workflows. Then
  ask Marwan, through Moonlight, to open Edit > Editor Preferences > Agentic Workflows > Model Context Protocol, check that Auto Start
  Server is on and the port is 8000, choose Client = Claude Code and click **Generate Client Config**. That writes
  `Reel\.mcp.json`; if it differs from the one at the repo root, copy it over the root one.

## 5. Record and commit

- `docs/DEVLOG.md`: the time, the engine and Visual Studio versions, how long the first build took, what went wrong.
- `git add -A`, `git commit -m "Stage 2: Unreal project created (Third Person C++ template)"`, `git push`.
  The template's assets go through Git LFS. If the push fails on LFS quota or size, keep the local commit, tell
  Marwan in one line, and move on: local commits are the safety net, the push is only a backup.

## 6. Hand over

Tell Marwan: leave the editor open, start a **new** session on `jam-pc` in this folder (a session only connects to
the Unreal server if the editor was running when the session started), and paste the stage 3 prompt from `README.md`.
