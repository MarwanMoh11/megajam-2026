# Epic AI MegaJam 2026: Director's Cut

Private jam repo. Unreal Engine 5.9 MegaJam Preview, C++. Built on Linux Mint, packaged on Windows, driven from a
MacBook with Claude agents.

**Hard deadline: Mon Nov 16, 03:59 Cairo. Upload target: Sun Nov 15, 22:00 Cairo.**

| Path | What |
|---|---|
| `CLAUDE.md` | The rules every agent session in this repo follows |
| `docs/` | Plan and schedule, the two candidate ideas (`STATUS.md`), brainstorm, survey, dev log |
| `setup/` | One runbook per setup stage, written for an agent to carry out |
| `tools/` | `sync-to-windows.sh`: mirrors the project onto the Windows partition |
| `prototypes/` | Browser grey boxes of the two ideas and the art board, made on the Mac |
| `claude-global/` | Your global Claude rules and agents, copied to the PC in stage 1 |
| `Reel/` | The Unreal project, created in Mint stage 2 (`Reel` is a working name) |

## The machines

| Machine | Job | Repo |
|---|---|---|
| MacBook | Where you sit. Claude app, browser, Moonlight for the PC's screen | `~/Developer/personal/EpicMegaJam` (docs, prototypes) |
| PC, Linux Mint boot | Builds the game: editor, agents, git | `~/Jam/megajam` |
| PC, Windows boot | Packages the Windows build the rules require. Nothing is edited here | `C:\Jam\megajam`, a mirror, not a clone |

Why two boots: the jam accepts Windows builds and not Linux ones, and Unreal can't package a Windows game from
Linux. What Mint costs: Unreal's fast C++ reload (Live Coding) is a Windows feature, a second engine install, and a
reboot for every Windows package. Stage 3 measures the first; the tripwire in `setup/mint-1-tools.md` says when to
give up on Mint and do everything on the Windows boot (`setup/windows-only/`).

## Setup

After stage 0 everything is done from the MacBook. Each prompt goes into a Claude session started on the PC over
SSH; each stage ends by telling you what to do next. What stays with you: sign-ins, your sudo password, the
engine downloads, and the new-project wizard.

### 0. Reach the PC (one trip upstairs, 5 minutes)

Follow `setup/0-remote.md`. In short: at the PC, install the SSH server, note its address and username, and start
the engine's Linux download. On the Mac: add the SSH key and config, clone this repo onto the PC, add the PC as an
SSH connection in the Claude app, install Moonlight.

### 1. Mint: tools and the engine

Session on `jam-mint`, folder `~/Jam/megajam`:

```text
Read setup/mint-1-tools.md and carry it out.
```

It checks the PC and writes `docs/MACHINE.md`, installs Sunshine and OBS (giving you one command for the parts that
need your password), unzips the engine, finds the Windows partition, and copies your global Claude rules.
**Yours:** run its one sudo command, then pair Moonlight (`setup/0-remote.md`, part C).

### 2. Mint: the Unreal project

```text
The engine is unzipped. Read setup/mint-2-project.md and carry it out.
```

It opens the editor on the PC's desktop, and after you create the project it turns on the All Toolsets plugin,
proves the command-line build and starts the Unreal MCP server.
**Yours:** the new-project wizard through Moonlight: Games > Third Person > **C++** > no starter content >
location `~/Jam/megajam` > name `Reel`.

### 3. Mint: prove the pipeline

Leave the editor open and start a **new** session on `jam-mint`. A session connects to the editor only if the
editor was already running when the session started.

```text
Read setup/mint-3-pipeline.md and carry it out.
```

It writes down what the editor tools can do (`docs/UNREAL_MCP.md`), drives the editor, measures the C++ loop on
Linux, packages a Linux smoke-test build, and mirrors the project onto the Windows partition.

### 4. Windows: tools (tonight, before sleeping)

The Windows downloads take hours and the PC can't build the game meanwhile, so run them overnight. Switch boots
with `setup/0-remote.md`, part D. The first time, Windows needs five minutes at its keyboard: section 0 of
`setup/win-1-tools.md`. Then, in a session on `jam-win`, folder `C:\Jam\megajam`:

```text
Read setup/win-1-tools.md and carry it out, starting at section 1.
```

**Know before pasting:** the winget flags it uses accept each package's licence terms for you.
**Yours:** in the Launcher, sign in and install Unreal Engine 5.9 MegaJam Preview. Always leave Windows with
**Restart**, never Shut down.

### 5. Windows: package

Every time a Windows build is wanted: on Mint run `tools/sync-to-windows.sh`, switch to Windows, then on `jam-win`:

```text
Read setup/win-2-package.md and carry it out.
```

The first run also proves the Windows toolchain, tries the browser build for 30 minutes at most, and has you
rehearse one upload to a Draft itch.io page. Do the first run Friday morning at the latest.

## The game

The pick between the two ideas is made on the Mac, from the browser grey boxes in `prototypes/`. Once it is made
and the spec is in `docs/STATUS.md`, in a session on `jam-mint` with the editor open:

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

- **The editor or its MCP server won't work on Mint within about an hour:** that is the tripwire. Do the whole jam
  on the Windows boot with `setup/win-1-tools.md` and `setup/windows-only/`.
- **The browser build fails:** drop it. The Windows package is the entry, and the rules require it anyway.
- **A push fails on Git LFS quota:** keep committing locally and carry on. Windows gets the project through the
  sync script, not through GitHub.
- **Mint can't write to the Windows partition:** Windows was shut down instead of restarted. Boot it once and Restart.
