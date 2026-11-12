# Epic AI MegaJam 2026: Director's Cut

Private jam repo. Unreal Engine 5.9 MegaJam Preview, C++. Built and packaged on a Windows PC, driven from a MacBook
with Claude agents.

**Hard deadline: Mon Nov 16, 03:59 Cairo. Upload target: Sun Nov 15, 22:00 Cairo.**

| Path | What |
|---|---|
| `CLAUDE.md` | The rules every agent session in this repo follows |
| `docs/` | Plan and schedule, the two candidate ideas (`STATUS.md`), brainstorm, survey, art style, dev log |
| `setup/` | One runbook per setup stage, written for an agent to carry out |
| `tools/` | `on-desktop.ps1`: starts a program on the PC's desktop from an SSH session |
| `prototypes/` | Browser grey boxes of the two ideas and the art board, made on the Mac |
| `claude-global/` | Your global Claude rules and agents, copied to the PC in stage 1 |
| `Reel/` | The Unreal project, created in stage 2 (`Reel` is a working name) |

## The machines

| Machine | Job | Repo |
|---|---|---|
| MacBook | Where you sit. Claude app, browser, Moonlight for the PC's screen | `~/Developer/personal/EpicMegaJam` (docs, prototypes) |
| PC, Windows | Everything Unreal: editor, agents, git, the Windows package the rules require | `C:\Jam\megajam` |

Windows, not the PC's Linux Mint boot: the jam accepts Windows builds and not Linux ones, Unreal can't package a
Windows game from Linux, and Unreal's fast C++ reload (Live Coding) only exists on Windows. One boot means one
engine install, no reboot per package, and the game is tuned on the renderer the judges will see.

## Setup

After stage 0 everything is done from the MacBook. Each prompt goes into a Claude session started on the PC over
SSH (`jam-pc`, folder `C:\Jam\megajam`); each stage ends by telling you what to do next. What stays with you:
sign-ins, the engine install, and the new-project wizard.

### 0. Reach the PC (one trip upstairs, about 15 minutes)

Follow `setup/0-remote.md`. In short: at the PC, paste two blocks into an administrator PowerShell (SSH, the tools,
Visual Studio) and start the engine install in the Epic Games Launcher. On the Mac: send Claude the PC's address
and username, sign the PC in to GitHub, clone this repo onto it, install Moonlight, and add the PC as an SSH
connection in the Claude app.

### 1. Tools

```text
Read setup/win-1-tools.md and carry it out.
```

It checks the PC and writes `docs/MACHINE.md`, finishes any install stage 0 left, proves it can open a window on
the PC's desktop, and copies your global Claude rules.
**Yours:** pair Moonlight (`setup/0-remote.md`, part C), and wait for the engine and Visual Studio to finish.

### 2. The Unreal project

When the Launcher shows **Launch** for 5.9:

```text
The engine is installed. Read setup/win-2-project.md and carry it out.
```

It checks the compiler against the engine, and after you create the project it turns on the All Toolsets plugin,
proves the command-line build and starts the editor with the Unreal MCP server.
**Yours:** the new-project wizard through Moonlight: Games > Third Person > **C++** > no starter content >
location `C:\Jam\megajam` > name `Reel`.

### 3. Prove the pipeline

Leave the editor open and start a **new** session on `jam-pc`. A session connects to the editor only if the editor
was already running when the session started.

```text
Read setup/win-3-pipeline.md and carry it out.
```

It writes down what the editor tools can do (`docs/UNREAL_MCP.md`), drives the editor, times the C++ loop, packages
the Windows build and zips it, and tries the browser build for 30 minutes at most.
**Yours:** rehearse one upload to a Draft itch.io page.

## The game

The pick between the two ideas is made on the Mac, from the browser grey boxes in `prototypes/`. Once it is made
and the spec is in `docs/STATUS.md`, in a session on `jam-pc` with the editor open:

```text
Read docs/STATUS.md and the NOTES.md of the picked prototype under prototypes/. Build that grey box in Unreal, in its own level, copying the prototype's rules and tuned values: C++ for the logic with the tunable values exposed, basic shapes, no art, audio or menus. Run it in play-in-editor yourself and fix what breaks. Then tell me in two lines how to play it, and what you could not judge. Commit when it runs.
```

## By hand, in a browser

Do these while the engine downloads. They need your accounts.

- [ ] The jam page says **Joined**: https://itch.io/jam/2026-epic-ai-megajam
- [ ] Find the Nov 10 partner email (university inbox, then spam). It holds most of the codes below.
- [ ] Claim the perks:

  | Perk | Where | Claim by |
  |---|---|---|
  | Tripo Pro + 3,000 credits (characters) | worlds.tripo3d.ai/epicaimegajam2026promotion, then tripo3d.ai > Upgrade > Redeem Code | Nov 16 |
  | Hyper3D Rodin (props; list it for the $10k pool) | hyper3d.ai > Wallet > Code Redemption, code from the email | Nov 16 |
  | PBRgen (materials) | sign-up link in the email | ends Nov 15 |
  | WeLoveIndies (music, SFX) | weloveindies.com/en/epicmegajam | |
  | fal $30 (concept art) | fal.ai, code from the email | Nov 30 |
  | Rokoko (video mocap) | register through the email | |
  | Anthropic $25 API | claim link in the email; only needed if the game calls Claude while running | Nov 12-27 |

- [ ] Read the Official Rules for eligibility and the student category:
  https://epicgames.box.com/s/a7xcgl07fqoieeb99sjtd3juhj4agc4z

## If something doesn't work

- **Claude sessions over SSH fight back on Windows:** install the Claude desktop app on the PC and use it through
  Moonlight. The runbooks still hold.
- **The editor won't start from a session:** `tools\on-desktop.ps1` is the untested part. Start the editor by hand
  through Moonlight and tell the session to fix the script.
- **The browser build fails:** drop it. The Windows package is the entry, and the rules require it anyway.
- **A push fails on Git LFS quota:** keep committing locally and carry on. The push is only a backup.
