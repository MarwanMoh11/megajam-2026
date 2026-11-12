# Automatic PC setup: one session, start to finish

You are Claude Code on Marwan's Windows PC, started by `go.cmd` from an administrator terminal, with all permissions
and Remote Control on. Marwan started you at the PC's keyboard and is about to go downstairs to a MacBook. From
there they follow this session at claude.ai/code, and see the PC's screen through Moonlight once it is paired.

The game jam is already running (deadline in `CLAUDE.md`). Work for speed. Decide what you can decide; don't end
your turn to ask.

**Done when:** the Unreal editor is open on the `Reel` project with its MCP server answering on port 8000; a new
session is carrying out `setup/win-3-pipeline.md`; a Remote Control server is waiting for new sessions; the work is
committed and pushed; and Marwan has a short list of what passed and what is theirs.

## Rules for this run

- **Never run when written.** Everything here was written on a Mac from documentation. Commands, flags, package ids
  and paths are best guesses: look before relying on them, and correct the runbook you are in when something differs.
- **Never type a password or sign in for Marwan** (Windows, Claude, Epic, GitHub, itch.io, Sunshine). When one is
  needed, say exactly what to do in one or two lines, keep working on whatever doesn't depend on it, and wait only
  when nothing else is left.
- **What you may change on this PC:** install the software these runbooks list (the winget flags
  `--accept-source-agreements --accept-package-agreements` accept each licence for Marwan, who asked for these
  installs), and turn off sleep and hibernation while plugged in. Nothing else: no firewall rules, no remote-login
  services, no account or security settings. If a step seems to need one, tell Marwan and skip it.
- **Marwan reads you on a phone or a laptop.** Say one short line at each milestone: what just finished, what is
  running, and anything they must do. No narration in between.
- **Waiting:** start one background command that exits when the condition is met, and let it wake you. Don't loop
  on `sleep` in the foreground.
- **You run as administrator, and so does everything you start.** That is why installers don't ask for permission.
- If this session was not started in `C:\Jam\megajam`, work there anyway and read `CLAUDE.md` in it first.

## 1. The first five minutes, in this order

Marwan is still at the keyboard for these, so get to them before anything slow.

1. **Epic Games Launcher.** `winget install -e --id EpicGames.EpicGamesLauncher`, then start it. Then tell Marwan,
   without ending your turn:
   > In the Launcher: sign in with the Epic account whose email will go on the submission form. Unreal Engine >
   > Library > the **+** beside "Engine Versions" > the version dropdown > **5.9 MegaJam Preview** > Install. In
   > Options keep Core Components, Templates and Feature Packs, Engine Source, and **Web** if it is listed (it may be
   > called Wasm); untick Editor symbols for debugging, Starter Content and every other platform.

   This download is the longest wait of the setup, so it starts first. Check the target drive has about 120 GB free;
   if it doesn't, say which drive does.
2. **Sunshine**, which streams this PC's screen to the Mac. `winget install -e --id LizardByte.Sunshine`, then check
   its service is running and port 47990 is listening. Find this PC's address on the home network (the adapter with
   a default gateway) and tell Marwan:
   > On the Mac, pair Moonlight: `setup/0-remote.md`. This PC's address is `<address>`.
3. **No sleep:** `powercfg /change standby-timeout-ac 0` and `powercfg /change hibernate-timeout-ac 0`.
4. **The door.** Open a new console window in `C:\Jam\megajam` running:
   ```
   claude remote-control --name jam-pc --permission-mode bypassPermissions
   ```
   It lets Marwan start new sessions on this PC from claude.ai/code, even after this session ends. If the window
   asks to enable Remote Control or to trust the folder, tell Marwan to press `y` in it. If it rejects the
   permission mode, use the closest one it lists and say so. If it won't start at all, carry on: sessions can still
   be started at the PC through Moonlight.

## 2. Tools

Carry out `setup/win-1-tools.md`. Visual Studio is the slow part: start it first, in the background.

## 3. Wait for the engine and Visual Studio

Wait in the background until both are true:

- the Visual Studio installer has exited, and the three workloads are present;
- `C:\ProgramData\Epic\UnrealEngineLauncher\LauncherInstalled.dat` lists a 5.9 install, its
  `Engine\Binaries\Win64\UnrealEditor.exe` exists, and the install folder has stopped growing for three minutes.

Give the wait a ceiling of six hours. If no engine download has started 20 minutes after you asked, say so once:
Marwan may have missed the request, and can do it through Moonlight.

## 4. The project

Carry out `setup/win-2-project.md`. It ends with the editor open on `Reel` and port 8000 answering.

## 5. Hand over to a new session

A session only gets the editor's tools if the editor was running when the session started, so stage 3 needs a new
one. Open a new console window in `C:\Jam\megajam` running:

```
claude "Read setup/win-3-pipeline.md and carry it out." --dangerously-skip-permissions --remote-control jam-pipeline
```

Check that it started and is working (its window, or its entry in `claude --resume`). If it didn't, tell Marwan to
start a session on `jam-pc` from claude.ai/code and paste that sentence.

## 6. Report

Add the run to `docs/DEVLOG.md` (times, what installed cleanly, what fought back), commit and push. Then tell
Marwan, in a short table: what passed, what didn't and why, what is theirs to do, and that `jam-pipeline` is now the
live session.
