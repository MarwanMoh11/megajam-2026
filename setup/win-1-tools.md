# Stage 1: tools

You are Claude Code on Marwan's Windows PC, in the clone of the jam repo (`C:\Jam\megajam`), running as
administrator. `setup/auto.md` sent you here and has the rules for this run; it has already installed the Epic
Games Launcher and Sunshine.

**Done when:** `docs/MACHINE.md` describes this PC; every tool below is installed, installing in the background, or
has a one-line reason it isn't; the global Claude rules are in place; and the work is committed and pushed.

**This runbook has never been run.** Package ids, versions and paths are best guesses: check them (`winget search`,
look on disk) before relying on them, and correct this file when something differs.

## 1. Start the long install first

**Visual Studio (10-20 GB), in the background with a log under `C:\Jam\logs\`.** Skip it if `vswhere.exe` already
finds Visual Studio 2022 with the three workloads; if Visual Studio is there without them, add them with the Visual
Studio Installer's `setup.exe modify`.

```powershell
winget install -e --id Microsoft.VisualStudio.2022.Community --accept-source-agreements --accept-package-agreements --override "--passive --wait --norestart --includeRecommended --add Microsoft.VisualStudio.Workload.NativeGame --add Microsoft.VisualStudio.Workload.NativeDesktop --add Microsoft.VisualStudio.Workload.ManagedDesktop"
```

Stage 2 checks the compiler against the installed engine, so don't spend time now on which version 5.9 wants.

## 2. The small tools

```powershell
winget install -e --id 7zip.7zip --accept-source-agreements --accept-package-agreements
winget install -e --id OBSProject.OBSStudio --accept-source-agreements --accept-package-agreements
git lfs install
```

7-Zip builds the submission zip. OBS records the gameplay video from the build the judges will play; the rules ask
for 1920x1080 or higher.

## 3. Look at the machine

Write `docs/MACHINE.md` (a short table, plus the decisions below):

- Windows edition and build; CPU; RAM in GB; every graphics card with its driver version and date
- Every drive with its free GB, and whether it is an SSD
- This PC's address on the home network, and the Windows username
- What is installed, with versions: Visual Studio, the Launcher, any Unreal Engine version, Git, 7-Zip, OBS,
  Sunshine, Claude Code

| Finding | Decision |
|---|---|
| Under about 120 GB free on the drive holding the engine | Tell Marwan at once which drive has room. |
| A graphics driver more than a year old | Tell Marwan to update it from the maker's site; it is their download. |
| An NVIDIA RTX card | Note "RTX award possible (DLSS)". |
| RAM under 32 GB | Note "one agent at a time; close the editor while packaging". |

## 4. Marwan's global Claude rules and agents

Copy `claude-global\CLAUDE.md` to `%USERPROFILE%\.claude\CLAUDE.md` and `claude-global\agents\*.md` into
`%USERPROFILE%\.claude\agents\`. If a target exists and differs, keep the existing file, save the repo's copy beside
it as `.new`, and tell Marwan. Follow these rules from now: be concise, keep command output short. Its rule on
subagents stands, with one exception Marwan asked for: `setup/auto.md` starts the next session by itself.

## 5. Record it

- `docs/DEVLOG.md`: add to Day 1, with the time (Cairo): what installed cleanly and what fought back.
- `git add` only the files you changed, `git commit -m "Stage 1: tools installed, machine recorded"`, `git push`.
  If the push asks for a sign-in, a browser window opens on the PC: tell Marwan, and carry on without waiting.
