# Stage 2: the Unreal project

Stage 1 is done: the engine is installed through the Launcher and Visual Studio has finished. This stage creates the
real jam project inside this repo and gets the editor running with the Unreal MCP server (the local server through
which Claude drives the editor), ready for a new session to connect to.

You are Claude Code on the PC itself, running as administrator; `setup/auto.md` has the rules for this run. Marwan
is at a MacBook: they read you at claude.ai/code and see this PC's screen through Moonlight.

**Done when:** `Reel\Reel.uproject` exists as a C++ project and builds from the command line; the All Toolsets
plugin is on; the editor is open and answering on port 8000; the paths under "Commands" in `CLAUDE.md` are real; and
the project is committed and pushed.

**This runbook has never been run.** Paths, file names and plugin names are best guesses from Epic's documentation:
look on disk before relying on them, and correct this file when something differs. "`C:\Jam`" means the folder that
holds this repo. `Reel` is a working name for the project; the game's displayed title can change later, the project
name can't cheaply.

## 1. Find the engine and check the toolchain

- Engine folder: `C:\Epic Games\UE_5.9` on this PC. In general, the `InstallLocation` of the `UE_5.9` entry in
  `C:\ProgramData\Epic\EpicGamesLauncher\Data\Manifests\*.item` (this Launcher leaves `LauncherInstalled.dat`
  empty). Confirm the version in `Engine\Build\Build.version`. Call this folder `UE` below.
- If no 5.9 install is there, stop and tell Marwan: the engine download has to finish first.
- Toolchain: `UE\Engine\Config\Windows\Windows_SDK.json` lists the compiler and Windows SDK versions this engine
  accepts. Compare them with what is installed (`vswhere.exe -all -products * -format json`, and the folders under
  `C:\Program Files (x86)\Windows Kits\10\Include`). Judge the compiler by the version UnrealBuildTool prints
  ("Using Visual Studio 14.44.35230 toolchain"), not the toolset folder name (`14.44.35207`), which lags behind
  and falls inside 5.9's banned range. Add anything missing with the Visual Studio Installer
  (`setup.exe modify --installPath <path> --add <component id> --passive --norestart`), in the background with a log.
- Web target: list `UE\Engine\Platforms` and note what the browser platform is called in this build (Web, Wasm or
  HTML5) and whether its files are present. Write the name in `docs/MACHINE.md`; stage 3 needs it. If it is absent,
  write that: the game ships on Windows only, and nothing else changes.

## 2. Create the project

The supported way to create a project from a template is the editor's new-project wizard, which needs clicks. Try
to do without it first, for **30 minutes at most**:

- Spend up to five minutes looking for a supported route in this engine version (a commandlet, an editor
  command-line switch, an MCP or Python call). Use it if there is one.
- Otherwise do what the wizard does, by hand. The C++ Third Person template is under `<UE>\Templates\` (in recent
  versions `TP_ThirdPerson`), and its `Config\TemplateDefs.ini` lists every step: which folders and files to skip,
  which names to replace in file names and file contents (the template's name becomes `Reel`), and which shared
  content packs to copy in from `<UE>\Templates\TemplateResources\`. Copy it to `C:\Jam\megajam\Reel`, apply those
  steps, and set the `.uproject`'s engine association to this install. If the template offers variants, keep the
  plain one. No Starter Content.
- **Accept it only if all of this holds:** the command-line build in section 3 succeeds; the editor opens
  `Reel.uproject` with no dialog about missing modules or assets; and the editor log shows the third-person map
  loading with no "Failed to load", missing-class or missing-asset errors. Stage 3 then checks play-in-editor.
- If the clock runs out or any check fails, don't debug it: delete `Reel\` and use the wizard.

**What the first run did (2026-11-12):** no supported command-line route turned up, so the hand copy was used and
passed every check above. Beyond `TemplateDefs.ini`'s own steps it needed what the wizard adds on its own: the
shared packs go to `Content\<MountName>\` (from `TemplateResources\High\<MountName>\Content\`); `DefaultEngine.ini`
gets `[/Script/Engine.Engine]` `+ActiveGameNameRedirects` for `TP_ThirdPerson` and `/Script/TP_ThirdPerson` to
`/Script/Reel`, an `+ActiveClassRedirects` line per renamed header (`TP_ThirdPersonCharacter` to `ReelCharacter` and
so on), and the Desktop/Maximum `HardwareTargetingSettings`; `DefaultGame.ini` gets a fresh `ProjectID`; the
`.uproject` is the template's with the module renamed and `"EngineAssociation": "5.9"`. The C++ template carries
the code and content of all its variants (Combat, Platforming, SideScrolling); they stay in.

**The wizard, if it comes to that.** Give Marwan these clicks, to do through Moonlight, and wait for their reply:

1. Epic Games Launcher > Unreal Engine > Library > **Launch** on 5.9 MegaJam Preview.
2. In the project browser: **Games** > **Third Person**. If it offers variants, keep the plain one.
3. Project defaults on the right: **C++** (not Blueprint), Target Platform **Desktop**, Quality Preset **Maximum**,
   **Starter Content off**.
4. Project Location: `C:\Jam\megajam`. Project Name: `Reel`.
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
  bug in this Preview, UE-412135). In 5.9.0 they are `AllToolsets` (off by default, so it is added) and
  `ModelContextProtocol`, "Unreal MCP" (on by default, so it is not).
- **The MCP server entry.** `.mcp.json` at the repo root points Claude Code at `http://127.0.0.1:8000/mcp` under the
  name `unreal-mcp`, but a session asks for approval before using a server from a project file, and the next
  session starts unattended. So register the same server for this Windows user as well:
  `claude mcp add --transport http --scope user unreal-mcp http://127.0.0.1:8000/mcp`. If `claude mcp list` then
  shows it twice, delete the repo's `.mcp.json` and commit that.
- **Command-line build.** Build the editor target once to prove the path works without the editor, and time it:
  ```
  "<UE>\Engine\Build\BatchFiles\Build.bat" ReelEditor Win64 Development -Project="<repo>\Reel\Reel.uproject" -WaitMutex
  ```
- Fill in the `UE` path under "Commands" in `CLAUDE.md`, and correct any command there that differed from what worked.

## 4. Start the editor and wait for the server

- Start `<UE>\Engine\Binaries\Win64\UnrealEditor.exe` with the `.uproject` path as its argument, detached, so it
  outlives this session.
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

Leave the editor open. If `setup/auto.md` sent you here, go back to it: its next step starts the stage 3 session.
Otherwise tell Marwan to start a **new** session in this folder (a session only connects to the Unreal server if
the editor was running when the session started) and give it: `Read setup/win-3-pipeline.md and carry it out.`
