# Stage 0: reach the PC from the MacBook

One trip upstairs, about 15 minutes at the PC's keyboard in Windows. After it, everything is done from the Mac: SSH
carries the Claude sessions, Sunshine (on the PC) and Moonlight (on the Mac) carry the screen.

**None of this has been run.** If a line fails, carry on with the next one and tell Claude on the Mac what it said.

## A. At the PC, in Windows

Open this page in the PC's browser (github.com > `megajam-2026` > `setup/0-remote.md`), so the blocks can be copied.
Then right-click Start > **Terminal (Admin)** or **Windows PowerShell (Admin)**.

**Block 1: SSH, the small tools, and the two facts the Mac needs (about 5 minutes).** It installs Windows' own SSH
server, lets in one key (the MacBook's, made for this jam), opens port 22 on the home network, stops the PC from
sleeping, and installs the Epic Games Launcher, Git, the GitHub CLI, Sunshine, 7-Zip and OBS Studio. The winget
flags accept each package's licence terms for you.

```powershell
Add-WindowsCapability -Online -Name OpenSSH.Server~~~~0.0.1.0
Set-Service sshd -StartupType Automatic; Start-Service sshd
$key = 'ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIPTGjksrzOrUuK6woAF0/alCcZOPxASnjgApIxaZRRJP jam-mac'
Add-Content C:\ProgramData\ssh\administrators_authorized_keys $key -Encoding ascii
icacls C:\ProgramData\ssh\administrators_authorized_keys /inheritance:r /grant "*S-1-5-32-544:F" /grant "*S-1-5-18:F"
New-Item -ItemType Directory -Force "$env:USERPROFILE\.ssh" | Out-Null
Add-Content "$env:USERPROFILE\.ssh\authorized_keys" $key -Encoding ascii
New-NetFirewallRule -Name Jam-SSH -DisplayName "Jam SSH" -Direction Inbound -Protocol TCP -LocalPort 22 -Action Allow -ErrorAction SilentlyContinue | Out-Null
powercfg /change standby-timeout-ac 0; powercfg /change hibernate-timeout-ac 0
foreach ($id in 'EpicGames.EpicGamesLauncher','Git.Git','GitHub.cli','LizardByte.Sunshine','7zip.7zip','OBSProject.OBSStudio') { winget install -e --id $id --accept-source-agreements --accept-package-agreements }
Restart-Service sshd
"ADDRESS: " + (Get-NetIPConfiguration | Where-Object { $_.IPv4DefaultGateway }).IPv4Address.IPAddress; "USER: $env:USERNAME"
```

Write down the **ADDRESS** and **USER** it prints (a phone photo will do).

**Block 2: Visual Studio with the Unreal workloads (20 to 40 minutes, unattended).** Paste it and leave the window
open.

```powershell
winget install -e --id Microsoft.VisualStudio.2022.Community --accept-source-agreements --accept-package-agreements --override "--passive --wait --norestart --includeRecommended --add Microsoft.VisualStudio.Workload.NativeGame --add Microsoft.VisualStudio.Workload.NativeDesktop --add Microsoft.VisualStudio.Workload.ManagedDesktop"
```

**The engine (the longest download, so start it now).** Open the Epic Games Launcher from the Start menu and sign in
with the Epic account whose email will go on the submission form. Unreal Engine > Library > the **+** beside "Engine
Versions" > the version dropdown > **5.9 MegaJam Preview** > Install. In Options keep Core Components, Templates and
Feature Packs, Engine Source, and **Web** if it is listed (it may be called Wasm); untick Editor symbols for
debugging, Starter Content and every other platform. It needs about 120 GB free.

**Before leaving:** the PC stays on, signed in to Windows, with its monitor connected (Sunshine streams a real
display).

## B. On the MacBook

1. Send Claude the ADDRESS and USER. Claude writes the `jam-pc` entry in `~/.ssh/config` (the key is already made)
   and checks that `ssh jam-pc` gets in without a password.
2. Sign the PC in to GitHub. It prints a code: type it at https://github.com/login/device on the Mac. The token is
   stored in a plain file on the PC, because Windows' credential store can't be read from an SSH session.

   ```bash
   ssh -t jam-pc "gh auth login -p https -w --insecure-storage"
   ```

3. Clone the repo on the PC:

   ```bash
   ssh jam-pc "gh auth setup-git && git lfs install && gh repo clone MarwanMoh11/megajam-2026 C:\Jam\megajam"
   ```

4. Install Moonlight:

   ```bash
   brew install --cask moonlight
   ```

5. In the Claude app: the environment selector under the prompt box > New environment > **SSH** > Add SSH
   connection: name `jam-pc`, SSH host `jam-pc`, folder `C:\Jam\megajam`. The app installs Claude Code on the PC the
   first time it connects.
6. Start a session on `jam-pc` and paste the stage 1 prompt from `README.md`.

## C. The screen: pair Moonlight with Sunshine (from the Mac)

1. In the Mac's browser open `https://<ADDRESS>:47990`. The browser warns about the certificate, because Sunshine
   signs its own: continue. Create a Sunshine username and password of your choice.
2. Open Moonlight. The PC appears by itself, or add it by its ADDRESS. Click it: Moonlight shows a four-digit PIN.
3. In the Sunshine page, **PIN** tab: enter the PIN and any device name.
4. In Moonlight, click the PC > **Desktop**. Quit the stream with Ctrl+Option+Shift+Q.

Sunshine runs as a Windows service, so the stream works at the sign-in screen and shows Windows' permission prompts.
There is no shared clipboard: when something long has to be typed on the PC, ask the Claude session to do it or to
write it into a file there.

## If it doesn't work

- **`ssh jam-pc` asks for a password or is refused:** in the admin PowerShell on the PC, `Get-Service sshd` should
  say Running, and `Get-Content C:\ProgramData\ssh\administrators_authorized_keys` should show the key line.
- **Claude sessions over SSH fight back on Windows:** install the Claude desktop app on the PC and use it through
  Moonlight. Clunkier, but everything else in the runbooks still holds.
- **Moonlight shows a black screen:** the monitor is off or unplugged. Turn it on, or ask the Claude session to
  check Sunshine's log.
