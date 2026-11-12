# Windows stage 2: package the Windows build

Run this every time a Windows build is wanted: the first time to prove the Windows side, then each evening from
Friday, then for the final on Sunday. You are in an SSH session on the PC's Windows boot; Marwan watches the screen
from a MacBook through Moonlight.

`C:\Jam\megajam` is a mirror written from Linux Mint, where the game is built. Never edit or commit in it.
`C:\Jam\megajam\SYNCED.txt` says which commit it holds and when it was synced.

**Done when:** `C:\Jam\Builds\` holds a packaged Windows build that Marwan has seen running and a zip under 1 GB,
and `C:\Jam\Builds\RESULT.md` says what was built, how long it took and what went wrong.

**This runbook has never been run.** Commands are best guesses from Epic's documentation: correct this file's copy
of them in `RESULT.md` so the Mint side can fix the repo.

## 1. First run only: the toolchain

- Engine folder: read `C:\ProgramData\Epic\UnrealEngineLauncher\LauncherInstalled.dat`, or look under
  `C:\Program Files\Epic Games\`. Confirm 5.9 in `Engine\Build\Build.version`. Call it `UE`. It must be the same
  engine version as on Mint (`docs\MACHINE.md` in the mirror says which).
- `UE\Engine\Config\Windows\Windows_SDK.json` lists the compiler and Windows SDK versions this engine accepts.
  Compare with what is installed (`vswhere.exe -all -products * -format json`, and the folders under
  `C:\Program Files (x86)\Windows Kits\10\Include`). Add anything missing with the Visual Studio Installer
  (`setup.exe modify --installPath <path> --add <component id> --passive --norestart`).

## 2. Package

Read `SYNCED.txt` and say which commit you are building. Run in the background, with the log at
`C:\Jam\Builds\package-win.log`, and time it:

```
"<UE>\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="C:\Jam\megajam\Reel\Reel.uproject" -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -prereqs -archive -archivedirectory="C:\Jam\Builds" -nop4 -utf8output
```

- `-prereqs` bundles the Visual C++ runtime installer, so the game starts on a judge's PC that lacks it.
- Don't add `-NoCleanStage`: in this Preview it can ship stale content.
- Once it works, save the exact command as `C:\Jam\package-win.bat`, so later runs are one command.

## 3. Test it

Start the packaged `.exe` on the desktop Marwan is watching, check the process is still alive after 20 seconds, and
ask Marwan to play it for a minute. Two questions for them: does it play right, and does it look the same as on
Mint? The Mint editor renders with Vulkan and this build with DirectX, so lighting and effects can differ, and the
judges see this one.

## 4. Zip it

With 7-Zip, zip the packaged folder as `C:\Jam\Builds\<TeamName>_<GameName>.zip` (until the names are decided,
`Test_Reel.zip`). Report the size; the jam's limit is 1 GB.

## 5. First run only: the browser build (stop after 30 minutes)

A browser build is a bonus that gets an entry more plays. Epic's forum post says it currently needs a source build
of the engine, so the Launcher build may not offer it at all. Look for a Web, Wasm or HTML5 folder under
`UE\Engine\Platforms`. If it is there, package with that platform name, Shipping, and its own log; start the
result with its `Play_*.bat` and ask Marwan whether it plays in Chrome; check it is under 500 MB in total and 200 MB
per file with `index.html` at the top of the zip. If it isn't there, or the clock runs out, write "Web dropped" and
why. Windows is the entry either way: the Official Rules require a Windows package.

## 6. First run only, with an RTX card (10 minutes at most)

Check whether NVIDIA's DLSS plugin has a build for this engine version. Write the answer and the link in
`RESULT.md`. Install nothing; it is a Saturday decision.

## 7. Write the result

`C:\Jam\Builds\RESULT.md`, replaced on each run: the commit built, package time, zip size, whether it ran, what
Marwan said about how it plays and looks, every error with its fix, and any command that differed from this file.

**Problems in the project itself:** don't fix them here, because the next sync overwrites this folder. Write the
exact fix in `RESULT.md` for the Mint side. The one exception is Sunday night with no time left to switch: fix it
here, and list every file you changed.

## 8. Hand over

Tell Marwan in a short table what passed. On the first run, add the upload rehearsal: on itch.io create a new
project and leave it as a **Draft**, upload the zip, open the draft page, then delete the test upload and keep the
draft. Then: **Restart** (not Shut down) to return to Mint.
