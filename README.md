# Epic AI MegaJam 2026: Director's Cut

Private jam repo. Unreal Engine 5.9 MegaJam Preview, C++, built on Windows with Claude agents.

**Hard deadline: Mon Nov 16, 03:59 Cairo. Upload target: Sun Nov 15, 22:00 Cairo.**

| Path | What |
|---|---|
| `CLAUDE.md` | The rules every agent session in this repo follows |
| `docs/` | Plan and schedule, the two candidate ideas (`STATUS.md`), brainstorm, dev log |
| `setup/` | One runbook per setup stage, written for an agent to carry out |
| `claude-global/` | Your global Claude rules and agents, copied to the PC in stage 1 |
| `Reel/` | The Unreal project, created in stage 2 (`Reel` is a working name) |

## Setting up the Windows PC

Setup is a five-minute bootstrap, then three pasted prompts. A Claude session does the work. What stays with you:
signing in (GitHub, Claude, Epic, itch.io), clicking Yes on Windows permission prompts, and two sets of clicks no
agent can reach: installing the engine in the Launcher and the new-project wizard. Each stage ends by telling you
what to do next.

### 0. Bootstrap, by hand

1. Open PowerShell and install Git:
   ```powershell
   winget install -e --id Git.Git
   ```
2. Install the Claude desktop app from https://claude.ai/download and sign in.
3. Open a **new** PowerShell window, so it can see Git, and clone this repo. A browser window opens for the GitHub
   sign-in, because the repo is private:
   ```powershell
   git clone https://github.com/MarwanMoh11/megajam-2026.git C:\Jam\megajam
   ```
4. In the Claude app, open the Code tab and start a session in the folder `C:\Jam\megajam`.

Keep the path short, without spaces, and outside OneDrive. If C: is short on space, clone to another drive
(`D:\Jam\megajam`); stage 1 checks the space and says so.

### 1. Tools and downloads

```text
Read setup/1-tools.md and carry it out.
```

- **It does:** checks the PC and writes `docs/MACHINE.md`; installs Visual Studio Community with the C++ game
  workloads, the Epic Games Launcher, OBS Studio, ScreenToGif and 7-Zip through winget; sets up Git LFS; copies your
  global Claude rules and agents into `%USERPROFILE%\.claude\`.
- **Know before pasting:** the winget flags it uses accept each package's licence terms for you.
- **Yours:** click Yes on the permission prompts. When the Launcher is in, sign in and install **Unreal Engine 5.9
  MegaJam Preview** (the session gives you the exact clicks and which options to untick). While it downloads, do the
  browser list at the bottom of this page.

### 2. The Unreal project

When the Launcher shows a Launch button for 5.9 and the session says Visual Studio finished:

```text
The engine is installed. Read setup/2-project.md and carry it out.
```

- **It does:** checks the compiler against what the engine wants; after you create the project, turns on the All
  Toolsets plugin, proves the command-line build, fills in the paths in `CLAUDE.md`, starts the editor and waits for
  the Unreal MCP server; commits and pushes.
- **Yours:** the new-project wizard, about six clicks: Games > Third Person > **C++** > no starter content >
  location `C:\Jam\megajam` > name `Reel`. The session lists them and waits for you to say "created".

### 3. Prove the pipeline

Leave the editor open and start a **new** session in `C:\Jam\megajam`. A session connects to the editor only if the
editor was already running when the session started.

```text
Read setup/3-pipeline.md and carry it out.
```

- **It does:** writes down what the editor tools can do (`docs/UNREAL_MCP.md`); spawns cubes and reads a
  screenshot back; runs play-in-editor; makes a C++ change with Live Coding; packages for Windows and builds the
  zip; tries a browser build for 30 minutes at most; records every time and size in the dev log.
- **Yours:** say whether the packaged game and the browser build look right, then rehearse one upload to a
  **Draft** itch.io page (the session gives the steps).

### 4. Grey box K

```text
Read docs/STATUS.md and build the grey box for K, Meanwhile..., in its own level. Build only what its "grey box" line lists: C++ for the logic with the tunable values exposed, basic shapes, no art, audio or menus. Run it in play-in-editor yourself and fix what breaks. Then tell me in two lines how to play it, and what you could not judge. Commit when it runs.
```

### 5. Grey box L

```text
Read docs/STATUS.md and build the grey box for L, The Tight Edit, in its own level. Build only what its "grey box" line lists: C++ for the logic with the tunable values exposed, basic shapes, no art, audio or menus. Run it in play-in-editor yourself and fix what breaks. Then tell me in two lines how to play it, and what you could not judge. Commit when it runs.
```

### 6. The pick

Play both cold. Fill in the two blanks:

```text
I played both grey boxes. I pick ___ because ___. Write the spec into docs/STATUS.md: win and lose conditions, the order rules run in each tick, the event-to-feedback table, the final kill list, and the look. Ask me only what you can't decide yourself, all in one message.
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

- **The Preview engine won't run or keeps crashing:** install the latest stable Unreal release beside it and ask the
  session whether it has the Unreal MCP plugin. Without MCP, Claude still writes the C++ and Python; you drive the editor.
- **The browser build fails:** drop it. Windows is the entry.
- **A push fails on Git LFS quota:** keep committing locally and carry on. The push is a backup, not a requirement.
- **The engine is still downloading:** ask the session to draft the spec for K and for L from `docs/STATUS.md` (win
  condition, rule order, feedback table). That needs no engine.
