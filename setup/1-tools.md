# Stage 1: tools and downloads

You are on Marwan's Windows PC, in the clone of the jam repo. The jam is already running (it opened Thu Nov 12,
07:00 Cairo), so every hour of setup comes out of the game. Work for speed: start the long installs first, run them
in the background with a log file, and verify afterwards.

**Done when:** Visual Studio with the Unreal workloads, the Epic Games Launcher, Git LFS, the recording tools and
the global Claude rules are in place; `docs/MACHINE.md` describes this PC; the work is committed and pushed; and
Marwan has the list of steps only they can do (section 6).

**This runbook has never been run.** It was written on a Mac from Epic's and Microsoft's documentation. Package ids,
versions and paths are best guesses: check them (`winget search`, look on disk) before relying on them, and correct
this file when something differs. "`C:\Jam`" below means the folder that holds this repo, whatever drive it is on.

Marwan pasted the prompt that started this stage, which asks for these installs. winget's
`--accept-source-agreements --accept-package-agreements` flags accept each package's licence terms for them; the
README says so next to the prompt.

## 1. Look at the machine

Collect, then write `docs/MACHINE.md` (a short table, plus the decisions below):

- Windows edition and build, CPU, RAM in GB
- Every graphics card and its memory
- Every drive: free GB, and whether it is an SSD (`Get-PhysicalDisk`)
- Whether this repo sits inside a OneDrive-synced folder
- What is already installed: Visual Studio (`vswhere.exe`), Epic Games Launcher, any Unreal Engine version

Decisions to write down:

| Finding | Decision |
|---|---|
| Under about 120 GB free on the drive that will hold the engine | Stop and tell Marwan. The engine and `C:\Jam` can each go on another drive; say which drive has room. |
| The repo is inside OneDrive, or its path has spaces or is long | Stop. It has to move to a short path such as `C:\Jam\megajam` first. |
| An NVIDIA RTX card | Note "RTX award possible (DLSS)". Nothing to install now. |
| RAM under 32 GB | Note "close the editor while packaging; one agent at a time". |
| RAM 32 GB or more | Note "the editor can stay open while packaging". |

## 2. Start the long installs

Check what is already there first and skip it. Run each long install in the background, writing a log under
`C:\Jam\logs\`. Installers raise a Windows permission prompt: tell Marwan to expect them and click Yes.

**Visual Studio (the long one, 10-20 GB).** Spend at most five minutes finding which version the 5.9 MegaJam
Preview wants: Epic's page "Setting Up Visual Studio Development Environment for C++ Projects in Unreal Engine" and
the 5.9 MegaJam Preview forum post linked from https://itch.io/jam/2026-epic-ai-megajam . If that doesn't settle it,
install Visual Studio 2022 Community; stage 2 checks the toolchain against the installed engine and adds what is
missing.

```powershell
winget install -e --id Microsoft.VisualStudio.2022.Community --accept-source-agreements --accept-package-agreements --override "--passive --wait --norestart --includeRecommended --add Microsoft.VisualStudio.Workload.NativeGame --add Microsoft.VisualStudio.Workload.NativeDesktop --add Microsoft.VisualStudio.Workload.ManagedDesktop"
```

If Visual Studio is already installed, add the three workloads with the Visual Studio Installer's `setup.exe modify`
instead of installing a second copy.

**Epic Games Launcher.**

```powershell
winget install -e --id EpicGames.EpicGamesLauncher --accept-source-agreements --accept-package-agreements
```

As soon as the Launcher is installed, give Marwan the steps in section 6 so the engine download runs alongside the
Visual Studio install. Don't wait for Visual Studio to finish first.

**Small tools**, while the two above run:

```powershell
winget install -e --id OBSProject.OBSStudio --accept-source-agreements --accept-package-agreements
winget install -e --id NickeManarin.ScreenToGif --accept-source-agreements --accept-package-agreements
winget install -e --id 7zip.7zip --accept-source-agreements --accept-package-agreements
```

OBS records the 30-60 s gameplay video, ScreenToGif makes the daily dev log GIF, and 7-Zip builds the submission zip.

## 3. Git

```powershell
git lfs install
git config --global core.longpaths true
```

`git lfs version` must print a version (Git for Windows includes it). Unreal's binary assets go through LFS; the
rules are already in `.gitattributes`.

## 4. Marwan's global Claude rules and agents

Copy `claude-global\CLAUDE.md` to `%USERPROFILE%\.claude\CLAUDE.md` and `claude-global\agents\*.md` into
`%USERPROFILE%\.claude\agents\`. They are the same rules and agents Marwan uses on the Mac. If a target file already
exists and differs, show the difference and ask before replacing it. They take effect in the next session, but
follow them from now: be concise, keep command output short, and use subagents only when Marwan asks.

## 5. Record it

- `docs/MACHINE.md`: the table and decisions from section 1, plus the version of everything installed.
- `docs/DEVLOG.md`: add a Day 1 entry with the time (Cairo), what installed cleanly and what fought back.
- Commit and push: `git add -A`, `git commit -m "Stage 1: tools installed, machine recorded"`, `git push`.

## 6. Tell Marwan what only they can do

End with this list, adjusted for what you found:

1. **Epic Games Launcher: install the engine.** Sign in with the Epic account whose email will go on the submission
   form. Then Unreal Engine > Library > the **+** beside "Engine Versions" > open the version dropdown on the new
   slot > **5.9 MegaJam Preview** > Install. In the Options dialog:
   - keep: Core Components, Templates and Feature Packs, Engine Source, and **Web** (it may be listed as Wasm)
   - untick: Editor symbols for debugging (tens of GB), Starter Content, Android, iOS, Linux and any other platform
   - if section 1 chose another drive, set the install location there
2. **While it downloads, in a browser:** the list under "By hand, in a browser" in `README.md` (the jam page says
   Joined, the partner email, the perks, the Official Rules).
3. **Then:** when the Launcher shows a Launch button for 5.9 and this session has reported that Visual Studio
   finished, paste the stage 2 prompt from `README.md`.

Report the Visual Studio result (exit code and the last lines of its log) when the background install completes.
