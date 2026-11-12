# Mint stage 2: the Unreal project

Stage 1 is done: the engine is unzipped on this PC (Linux Mint) and Marwan can see its desktop from the MacBook
through Moonlight. This stage creates the real jam project inside this repo and gets the editor running with the
Unreal MCP server (the local server through which Claude drives the editor), ready for a new session to connect.

**Done when:** `Reel/Reel.uproject` exists as a C++ project and builds from the command line; the All Toolsets
plugin is on; the editor is open on the PC's desktop and answering on port 8000; the commands in `CLAUDE.md` are the
real ones; and the project is committed and pushed.

**This runbook has never been run.** Paths, file names and plugin names are best guesses from Epic's documentation:
look on disk before relying on them, and correct this file when something differs. `UE` is the engine folder from
`docs/MACHINE.md`. `Reel` is a working name; the displayed title can change later, the project name can't cheaply.
You never type the sudo password: see stage 1.

## 1. Start the editor once, on the PC's desktop

You are in an SSH session, so a program you start has no screen unless you give it one. Start the editor on the
logged-in desktop, detached, with a log:

```
DISPLAY=:0 XAUTHORITY="$HOME/.Xauthority" nohup "<UE>/Engine/Binaries/Linux/UnrealEditor" > ~/Jam/editor.log 2>&1 &
```

With no project named, it opens the project browser. If it exits at once, read the log: a missing library or a
Vulkan problem shows there. If the log reports the file-watcher limit, give Marwan the one-line `sysctl` fix.

## 2. Ask Marwan to create the project (the one step that needs the wizard)

There is no supported command line for creating a project from a template. Give Marwan these clicks, to do through
Moonlight, and end your turn. Carry on when they reply.

1. In the project browser: **Games** > **Third Person**. If it offers variants, keep the plain one.
2. Project defaults: **C++** (not Blueprint), Target Platform **Desktop**, Quality Preset **Maximum**,
   **Starter Content off**.
3. Project Location: this repo's folder (`~/Jam/megajam`). Project Name: `Reel`.
4. **Create**. The first compile takes a few minutes, then the editor opens. If it complains that no code editor is
   installed, dismiss it: none is needed.
5. Press Play (Alt+P) once to see the character run, then **close the editor** and reply "created".

If the wizard or the compile fails, Marwan pastes the message, or you read `~/Jam/editor.log` and the newest log
under `Reel/Saved/Logs/`. Fix it and have them retry.

## 3. Wire the project up (editor closed)

- Check `Reel/Reel.uproject` and `Reel/Source/` exist. No `Source` folder means a Blueprint project was created:
  ask Marwan to delete `Reel/` and redo step 2 with C++.
- **Plugins.** Search the engine's `.uplugin` files for the one whose friendly name is "All Toolsets", and for the
  Model Context Protocol server plugin. Add each to the `Plugins` list in `Reel.uproject` as
  `{ "Name": "<file name without .uplugin>", "Enabled": true }` unless it is already enabled by default. All
  Toolsets is required: without it the agent connects but has no tools for levels, actors or Blueprints (a known
  bug in this Preview, UE-412135).
- **`.mcp.json`** is already at the repo root and points Claude Code at `http://127.0.0.1:8000/mcp` under the name
  `unreal-mcp`. The first session that sees it asks Marwan to approve the server; that is expected.
- **Command-line build.** Build the editor target once, to prove it works without the editor, and time it:
  ```
  "<UE>/Engine/Build/BatchFiles/Linux/Build.sh" ReelEditor Linux Development -Project="$HOME/Jam/megajam/Reel/Reel.uproject" -WaitMutex
  ```
- Rewrite the "Commands" section of `CLAUDE.md` with the Linux commands that worked: build, start the editor, and
  (after stage 3) package. Keep the Windows package command there too, marked as the Windows boot's.

## 4. Start the editor on the project and wait for the server

- Same command as section 1, with the full path of `Reel.uproject` as the argument.
- Wait, in one bounded loop of at most five minutes, for port 8000 on 127.0.0.1 to accept a connection. The first
  launch compiles shaders, so it is slow.
- If the port never opens: read the newest log in `Reel/Saved/Logs/` for lines about MCP or Agentic Workflows. Then
  ask Marwan, through Moonlight, to open Edit > Editor Preferences > Agentic Workflows > Model Context Protocol,
  check that Auto Start Server is on and the port is 8000, choose Client = Claude Code and click **Generate Client
  Config**. That writes `Reel/.mcp.json`; if it differs from the one at the repo root, copy it over the root one.
- If the server can't be made to work on Linux, that is the tripwire from stage 1: stop and tell Marwan.

## 5. Record and commit

- `docs/DEVLOG.md`: the time, the engine version, how long the first build took, what went wrong.
- `git add` the project and the files you changed, `git commit -m "Mint stage 2: Unreal project created (Third
  Person C++ template)"`, `git push`. The template's assets go through Git LFS. If the push fails on LFS quota or
  size, keep the local commit, tell Marwan in one line and move on: the push is a backup, and Windows gets the
  project through `tools/sync-to-windows.sh`, not through GitHub.

## 6. Hand over

Tell Marwan: leave the editor open, start a **new** session on `jam-mint` in this folder (a session only connects
to the Unreal server if the editor was running when the session started), and paste the stage 3 prompt from
`README.md`.
