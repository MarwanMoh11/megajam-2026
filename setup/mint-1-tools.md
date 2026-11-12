# Mint stage 1: tools and the engine

You are in an SSH session on Marwan's PC, booted into Linux Mint, in the clone of the jam repo (`~/Jam/megajam`).
Marwan is at a MacBook: they reach this PC through SSH (you) and through Sunshine and Moonlight (the screen). The
game is built here on Mint. The same PC also boots Windows, used only to package, because the jam accepts Windows
builds and not Linux ones. The jam is already running, so work for speed.

**Done when:** `docs/MACHINE.md` describes this PC; the engine is unzipped and its editor binary exists; Sunshine is
running; the Windows partition's mount point is in `.winroot`; the global Claude rules are in place; the work is
committed and pushed; and Marwan has the list in section 7.

**This runbook has never been run.** It was written on a Mac from documentation. Package names, file names and
paths are best guesses: look before relying on them, and correct this file when something differs.

**Passwords are Marwan's.** You never type the sudo password. When something needs root, collect it into one
command and give it to Marwan to run from the Mac's Terminal as `ssh -t jam-mint '<command>'`, then end your turn
and carry on when they say it is done. Ask as few times as you can.

## 1. Look at the machine

Collect, then write `docs/MACHINE.md` (a short table, plus the decisions below):

- Mint version and the Ubuntu release it is based on (`/etc/os-release`, `/etc/upstream-release/lsb-release`), kernel
- CPU, RAM in GB, every graphics card and the driver in use (`nvidia-smi`, `inxi -G` or `lspci -k`)
- Whether Vulkan works (`vulkaninfo --summary`)
- Every mounted filesystem with its free GB, and whether the home drive is an SSD
- The desktop session: X11 or Wayland, and whether a desktop session is logged in on display `:0`
- The NTFS partitions (`lsblk -f`), and which one holds Windows

Decisions to write down:

| Finding | Decision |
|---|---|
| Under about 100 GB free where the engine will go | Stop and tell Marwan which filesystem has room. |
| An NVIDIA card running the open `nouveau` driver, or Vulkan not working | Stop. Marwan installs the proprietary driver with Mint's Driver Manager and reboots. |
| A Wayland session | Note it: Sunshine capture and launching the editor on `:0` are written for X11. |
| No desktop session logged in | Ask Marwan to log in (stage 0 turns on automatic login). The editor needs it. |
| An NVIDIA RTX card | Note "RTX award possible (DLSS), on the Windows build only". |
| RAM under 32 GB | Note "one agent at a time; close the editor while packaging". |

## 2. What needs root, in one command

Stage 0 already installed git, git-lfs, gh, curl, rsync, 7-Zip, efibootmgr and vulkan-tools. Check what is missing,
then build one command for Marwan covering only what is needed:

- **Sunshine.** Download the `.deb` for this Mint's Ubuntu base from the newest release at
  https://github.com/LizardByte/Sunshine/releases into `~/Downloads` (no root needed for the download), and add
  `sudo apt install -y ~/Downloads/<file>.deb` to the command.
- **OBS Studio** (`obs-studio`), for recording gameplay.
- Any library the engine's own documentation lists as required on Linux and that is missing.

## 3. What needs no root

- `git lfs install`
- Marwan's global Claude rules: copy `claude-global/CLAUDE.md` to `~/.claude/CLAUDE.md` and
  `claude-global/agents/*.md` into `~/.claude/agents/`. If a target exists and differs, show the difference and ask
  before replacing it. Follow them from now: be concise, keep command output short, no subagents unless asked.
- Start Sunshine for the logged-in user: `systemctl --user enable --now sunshine`, and check it is listening on
  port 47990. Marwan pairs Moonlight with it from the Mac (`setup/0-remote.md`, part C).

## 4. The engine

Marwan started the download in stage 0: a zip of the Linux build of the 5.9 MegaJam Preview in `~/Downloads`.

- If it is still downloading, do sections 5 and 6 first and come back.
- If it isn't there at all, say so: Marwan downloads it in a browser on this PC (it needs their Epic sign-in).
- Check there is room for the zip and its contents at once, then unzip it to `~/UnrealEngine/UE_5.9` in the
  background with a log. Call that folder `UE`.
- Verify: `UE/Engine/Binaries/Linux/UnrealEditor` exists and `UE/Engine/Build/Build.version` says 5.9. Delete the zip
  only after that, and only if space is tight; otherwise leave it.
- Read the Linux notes that ship with the build (a README, or `UE/Engine/Build/BatchFiles/Linux/`) for any setup
  step it expects, and do it.

## 5. The Windows partition

`tools/sync-to-windows.sh` mirrors this repo onto the Windows partition so the Windows boot can package it.

- Mount the Windows partition without root (`udisksctl mount -b /dev/<partition>`) and write its mount point into
  `.winroot` in the repo root (the file is git-ignored).
- Test that you can create and delete a file there. If it is read-only, Windows was last shut down instead of
  restarted: tell Marwan it needs one boot into Windows ending in **Restart**.
- Don't run the sync yet; stage 3 does.

## 6. Record it

- `docs/MACHINE.md`: section 1, plus the versions of what was installed and the engine path.
- `docs/DEVLOG.md`: add to Day 1, with the time (Cairo): what installed cleanly and what fought back.
- `git add` only the files you changed, `git commit -m "Mint stage 1: tools, engine, machine recorded"`, `git push`.

## 7. Tell Marwan what is theirs

1. Pair Moonlight with Sunshine: `setup/0-remote.md`, part C.
2. Anything from the decisions table that needs them.
3. When the engine is unzipped and Moonlight shows the desktop, paste the stage 2 prompt from `README.md`.

**The tripwire:** if the editor or its MCP server can't be made to work on Mint within about an hour of trying in
stages 2 and 3, stop and say so. The fallback is to do the whole jam on the Windows boot, with the runbooks in
`setup/windows-only/`. Sinking the evening into Linux problems would cost more than the switch.
