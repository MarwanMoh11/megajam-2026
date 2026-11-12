# Stage 1: tools and the engine

You are in an SSH session on Marwan's Windows PC, in the clone of the jam repo (`C:\Jam\megajam`). Marwan is at a
MacBook: they reach this PC through SSH (you) and through Sunshine and Moonlight (the screen). The whole jam is
built and packaged on this PC. The jam is already running, so work for speed.

Stage 0 (`setup/0-remote.md`) was done by hand at the PC: the SSH server, Git, the GitHub CLI, the Epic Games
Launcher, Sunshine, 7-Zip, OBS Studio, a Visual Studio install that may still be running, and an engine download
that may still be running.

**Done when:** `docs/MACHINE.md` describes this PC; every tool below is installed or has a one-line reason it
isn't; `tools\on-desktop.ps1` is proven; the global Claude rules are in place; the work is committed and pushed; and
Marwan has the list in section 6.

**This runbook has never been run.** It was written on a Mac from documentation. Package ids, versions and paths
are best guesses: look before relying on them, and correct this file when something differs.

**Three things differ from sitting at the PC:**

- **No desktop.** A program you start from this session is invisible and has no display. Start anything with a
  window (the editor, the packaged game, an installer that needs clicks) on Marwan's desktop with
  `powershell -ExecutionPolicy Bypass -File tools\on-desktop.ps1 <exe> "<arguments>"`.
- **No passwords.** You never type a Windows, Epic, itch.io or GitHub password. When something needs one, say what
  to do through Moonlight, end your turn, and carry on when Marwan says it is done.
- **No clipboard between the Mac and the PC.** If Marwan has to run something on the PC's desktop, write it into a
  script under `C:\Jam\` and give them the short path to type, or start it yourself with `on-desktop.ps1`.

## 1. Look at the machine

Collect, then write `docs/MACHINE.md` (a short table, plus the decisions below):

- Windows edition and build; CPU; RAM in GB; every graphics card with its driver version and date
- Every drive with its free GB, and whether it is an SSD
- Whether a user is signed in on the desktop (`quser`)
- What is installed: Visual Studio (`vswhere.exe -all -products * -format json`), the Launcher, any Unreal Engine
  version (`C:\ProgramData\Epic\UnrealEngineLauncher\LauncherInstalled.dat`), Git, `gh`, 7-Zip, OBS, Sunshine

| Finding | Decision |
|---|---|
| Under about 120 GB free on the drive holding the engine | Stop and tell Marwan which drive has room. |
| A graphics driver more than a year old | Tell Marwan to update it from the maker's site; it is their download. |
| An NVIDIA RTX card | Note "RTX award possible (DLSS)". |
| RAM under 32 GB | Note "one agent at a time; close the editor while packaging". |
| Nobody signed in | Ask Marwan to sign in through Moonlight. The editor needs a signed-in desktop. |

## 2. Finish the installs

- **Visual Studio.** If its installer is still running, leave it. When it has finished, check that the three
  workloads are there (Game development with C++, Desktop development with C++, .NET desktop development). If Visual
  Studio is missing entirely, install it, in the background with a log under `C:\Jam\logs\`:
  ```powershell
  winget install -e --id Microsoft.VisualStudio.2022.Community --accept-source-agreements --accept-package-agreements --override "--passive --wait --norestart --includeRecommended --add Microsoft.VisualStudio.Workload.NativeGame --add Microsoft.VisualStudio.Workload.NativeDesktop --add Microsoft.VisualStudio.Workload.ManagedDesktop"
  ```
- **Anything else missing** from stage 0's list: `winget install -e --id <id>` with the same two accept flags
  (Marwan asked for these installs; the flags accept each package's licence terms for them).
- **If winget won't run from an SSH session** (a known limit on some Windows builds): write the commands into
  `C:\Jam\install.ps1` and start it on the desktop with `on-desktop.ps1` and `powershell.exe`, or ask Marwan to run
  it from an administrator PowerShell through Moonlight.
- `git lfs install`.

## 3. Prove the desktop helper and the stream

- Start Notepad with `tools\on-desktop.ps1 C:\Windows\System32\notepad.exe` and check that its process has a
  session id above 0. Ask Marwan, if Moonlight is paired, whether Notepad showed. Close it. If the helper fails, fix
  the script: every later stage starts the editor with it.
- Sunshine: the service is running and port 47990 is listening. If Marwan hasn't paired Moonlight yet, point them
  to `setup/0-remote.md`, part C.

## 4. Marwan's global Claude rules and agents

Copy `claude-global\CLAUDE.md` to `%USERPROFILE%\.claude\CLAUDE.md` and `claude-global\agents\*.md` into
`%USERPROFILE%\.claude\agents\`. If a target exists and differs, show the difference and ask before replacing it.
Follow them from now: be concise, keep command output short, no subagents unless asked.

## 5. Record it

- `docs/MACHINE.md`: section 1, plus the versions of what is installed.
- `docs/DEVLOG.md`: add to Day 1, with the time (Cairo): what installed cleanly and what fought back.
- `git add` only the files you changed, `git commit -m "Stage 1: tools installed, machine recorded"`, `git push`.

## 6. Tell Marwan what is theirs

1. **The engine**, if it isn't downloading yet: the steps are in `setup/0-remote.md`, part A, done through
   Moonlight. Say how far the download is if you can tell (the size of the install folder against about 60 GB).
2. **Moonlight**, if it isn't paired: `setup/0-remote.md`, part C.
3. Anything from the decisions table that needs them.
4. **When the Launcher shows Launch for 5.9 and Visual Studio has finished,** paste the stage 2 prompt from
   `README.md`.

If Visual Studio is still installing when you finish, report its result (exit code and the last lines of its log)
when it completes.
