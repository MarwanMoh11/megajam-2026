# Windows stage 1: the packaging boot

This PC dual-boots. The game is built on Linux Mint; the Windows boot has one job: turning the project into the
Windows build the jam requires, every evening from Friday and for the final on Sunday. Marwan is at a MacBook and
reaches this boot through SSH (you) and through Sunshine and Moonlight (the screen).

`C:\Jam\megajam` is a **mirror** written from Mint by `tools/sync-to-windows.sh`. Never edit or commit anything in
it: the next sync overwrites it, and it is not a git clone. Notes for the Mint side go in `C:\Jam\Builds\`, which
Mint can read.

**Done when:** Visual Studio with the Unreal workloads, the Epic Games Launcher, 7-Zip, OBS Studio and Sunshine are
installed; the engine is installed or downloading; `C:\Jam\Builds\WINDOWS.md` describes this boot; and Marwan has
the list in section 5.

**This runbook has never been run.** Package ids, versions and paths are best guesses: check them (`winget search`,
look on disk) before relying on them. Marwan asked for these installs; winget's `--accept-source-agreements
--accept-package-agreements` flags accept each package's licence terms for them.

## 0. Marwan's part, at the PC's keyboard, before any session can connect (5 minutes)

In PowerShell opened **as administrator**:

```powershell
Add-WindowsCapability -Online -Name OpenSSH.Server~~~~0.0.1.0
Set-Service -Name sshd -StartupType Automatic; Start-Service sshd
Copy-Item C:\Jam\mac_key.pub C:\ProgramData\ssh\administrators_authorized_keys
icacls C:\ProgramData\ssh\administrators_authorized_keys /inheritance:r /grant "Administrators:F" /grant "SYSTEM:F"
winget install -e --id Git.Git
winget install -e --id LizardByte.Sunshine
whoami
```

- `C:\Jam\mac_key.pub` is the Mac's public key, put there from Mint in stage 3. The two key lines let the Mac in
  without a password; they are for an administrator account, which most home accounts are.
- The username `whoami` prints (the part after the backslash) goes into the `jam-win` entry in the Mac's
  `~/.ssh/config`.
- On the Mac, add a second SSH connection in the Claude app, `jam-win`, with the folder `C:\Jam\megajam`, and pair
  Moonlight with this boot's Sunshine the same way as on Mint (`setup/0-remote.md`, part C).
- If SSH into Windows fights back, skip it: install the Claude desktop app on Windows and use it through Moonlight.

## 1. Look at the machine

Write `C:\Jam\Builds\WINDOWS.md`: Windows edition and build, free GB on each drive, what is already installed
(Visual Studio via `vswhere.exe`, the Launcher, any Unreal Engine version).

- Under about 120 GB free on the drive that will hold the engine: stop and tell Marwan which drive has room.
- An NVIDIA RTX card: note "RTX award possible (DLSS)".

## 2. Start the long installs

Skip what is already there. Run each long install in the background with a log under `C:\Jam\logs\`. Installers
raise a Windows permission prompt: tell Marwan to expect them on the Moonlight stream and click Yes.

**Visual Studio (10-20 GB).** Spend at most five minutes finding which version the 5.9 MegaJam Preview wants
(Epic's page "Setting Up Visual Studio Development Environment for C++ Projects in Unreal Engine", and the forum
post https://forums.unrealengine.com/t/unreal-engine-5-9-megajam-preview/2852840 ). If that doesn't settle it,
install Visual Studio 2022 Community; stage 2 checks the toolchain against the installed engine.

```powershell
winget install -e --id Microsoft.VisualStudio.2022.Community --accept-source-agreements --accept-package-agreements --override "--passive --wait --norestart --includeRecommended --add Microsoft.VisualStudio.Workload.NativeGame --add Microsoft.VisualStudio.Workload.NativeDesktop --add Microsoft.VisualStudio.Workload.ManagedDesktop"
```

If Visual Studio is already installed, add the three workloads with the Visual Studio Installer's `setup.exe modify`.

**Epic Games Launcher**, then the small tools:

```powershell
winget install -e --id EpicGames.EpicGamesLauncher --accept-source-agreements --accept-package-agreements
winget install -e --id 7zip.7zip --accept-source-agreements --accept-package-agreements
winget install -e --id OBSProject.OBSStudio --accept-source-agreements --accept-package-agreements
```

7-Zip builds the submission zip. OBS records the gameplay video here, from the build the judges will play; the
rules ask for 1920x1080 or higher.

## 3. Marwan's global Claude rules and agents

Copy `claude-global\CLAUDE.md` to `%USERPROFILE%\.claude\CLAUDE.md` and `claude-global\agents\*.md` into
`%USERPROFILE%\.claude\agents\`. If a target exists and differs, show the difference and ask first.

## 4. Record it

Finish `C:\Jam\Builds\WINDOWS.md` with the versions installed and anything that fought back. There is no git here;
the Mint side copies these notes into the repo.

## 5. Tell Marwan what only they can do

1. **Epic Games Launcher: install the engine**, through Moonlight. Sign in with the Epic account whose email will go
   on the submission form. Unreal Engine > Library > the **+** beside "Engine Versions" > the version dropdown >
   **5.9 MegaJam Preview** > Install. In Options keep Core Components, Templates and Feature Packs, Engine Source
   and **Web** (it may be listed as Wasm); untick Editor symbols for debugging, Starter Content and every other
   platform.
2. **Leave it downloading**, overnight if it is evening. The PC can't build the game while it is in Windows.
3. **When the Launcher shows Launch for 5.9 and Visual Studio has finished,** paste the Windows stage 2 prompt from
   `README.md`. If it is morning and the downloads are done, run stage 2 first, then go back to Mint.
4. **Always leave Windows with Restart, never Shut down.** A shutdown locks the partition and Mint can't sync to it.

Report the Visual Studio result (exit code and the last lines of its log) when the background install completes.
