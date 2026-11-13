# Epic AI MegaJam 2026: Director's Cut

Private jam repo. Unreal Engine 5.9 MegaJam Preview, C++. Built and packaged on a Windows PC, driven from a MacBook
with Claude agents.

**Hard deadline: Mon Nov 16, 03:59 Cairo. Upload target: Sun Nov 15, 22:00 Cairo.**

| Path | What |
|---|---|
| `CLAUDE.md` | The rules every agent session in this repo follows |
| `docs/` | Plan, the game's spec and build order (`STATUS.md`), brainstorm, survey, art style, dev log |
| `setup/` | One runbook per setup stage, written for an agent to carry out |
| `go.cmd` | Starts the automatic setup on the PC |
| `prototypes/` | Browser grey boxes of the two ideas and the art board, made on the Mac |
| `claude-global/` | Your global Claude rules and agents, copied to the PC in stage 1 |
| `Reel/` | The Unreal project, created during setup (`Reel` is a working name) |

## The machines

| Machine | Job | Repo |
|---|---|---|
| MacBook | Where you sit. Claude app, browser, Moonlight for the PC's screen | `~/Developer/personal/EpicMegaJam` (docs, prototypes) |
| PC, Windows | Everything Unreal: editor, agents, git, the Windows package the rules require | `C:\Jam\megajam` |

Windows, not the PC's Linux Mint boot: the jam accepts Windows builds and not Linux ones, Unreal can't package a
Windows game from Linux, and Unreal's fast C++ reload (Live Coding) only exists on Windows. One boot means one
engine install, no reboot per package, and the game is tuned on the renderer the judges will see.

## Setup: four lines at the PC, then it runs itself

**At the PC, in Windows.** Right-click Start > **Terminal (Admin)** and run:

```powershell
winget install -e --id Git.Git --accept-source-agreements --accept-package-agreements
irm https://claude.ai/install.ps1 | iex
```

Close that window, open **Terminal (Admin)** again so it finds the two new programs, and run:

```powershell
git clone https://github.com/MarwanMoh11/megajam-2026 C:\Jam\megajam
C:\Jam\megajam\go.cmd
```

The clone opens a browser to sign in to GitHub. `go.cmd` starts Claude Code with all permissions and Remote
Control on, and hands it `setup/auto.md`. The first start asks you to sign in to Claude and to confirm the
permission mode. If it then sits at an empty prompt, type `Read setup/auto.md and carry it out.`

**Stay about five minutes.** One thing can't be automated: when the Epic Games Launcher opens, sign in and start
the **5.9 MegaJam Preview** install (the session tells you the clicks). If a second window asks to enable Remote
Control, press `y`. Then go downstairs; the PC stays on with its monitor connected.

**From the Mac** (`setup/0-remote.md`): the sessions are at https://claude.ai/code, and Moonlight shows the PC's
screen once you have paired it with the address the session prints.

**What the session does by itself:**

1. Installs the Epic Games Launcher, Sunshine, Visual Studio with the Unreal workloads, 7-Zip and OBS Studio; turns
   off sleep; records the machine in `docs/MACHINE.md`; copies your global Claude rules.
2. Waits for the engine download and Visual Studio to finish.
3. Creates the `Reel` project from the Third Person C++ template, turns on the All Toolsets plugin, proves the
   command-line build, and opens the editor with its MCP server.
4. Starts a second session, `jam-pipeline`, which drives the editor, times the C++ loop, packages the Windows build
   and zips it, and tries the browser build for 30 minutes at most.

**Know before starting:**

- The session runs as administrator with no permission prompts, so it can install software unattended. The winget
  flags accept each package's licence terms for you.
- It changes no firewall, account or security settings, and opens no remote login. Sunshine's installer adds its
  own firewall rule for the stream.
- **None of it has been run.** Likely places to need you: creating the project (if the automatic route fails, the
  session asks for six clicks in the new-project wizard through Moonlight) and Remote Control (if it won't connect,
  the sessions are still there in their windows on the PC).

The stages are also separate runbooks, if one has to be rerun: `setup/win-1-tools.md`, `setup/win-2-project.md`,
`setup/win-3-pipeline.md`. Each takes the prompt `Read <file> and carry it out.`

## The game

The game is a heist shot in one take (working title *Print It*); its spec and build order are in `docs/STATUS.md`.
For each build step, start a new session on the PC with the editor open (at claude.ai/code, on the `jam-pc` door)
and give it, with the step's number:

```text
Read docs/STATUS.md. Do step 1 of the build order in Unreal, and nothing past it: C++ for the logic with the tunable values exposed, basic shapes, no art, audio or menus. Run it in play-in-editor yourself and fix what breaks. Then tell me in two lines how to play it, and what you could not judge. Commit and push when it runs.
```

Audio (step 6) runs in parallel in a second session, outside the editor, so it never touches `.uasset` files. First,
at the PC, sign in to elevenlabs.io in its browser, make an API key, and set it yourself in PowerShell (agents never
type keys; there is no clipboard from the Mac). Sessions started after this see it:

```powershell
setx ELEVENLABS_API_KEY "paste-your-key-here"
```

```text
Read docs/STATUS.md. Do the audio part of step 6 only, outside the editor: write every voice line, bluff and sound effect the spec lists into docs/AUDIO_SCRIPT.md, generate them with the ElevenLabs API (key in ELEVENLABS_API_KEY; never print it) as WAV files in C:\Jam\Audio, named by event. Keep each voice to one ElevenLabs voice and say which. Log ElevenLabs in docs/CREDITS.md. Don't import into Unreal; the editor session does that. Tell me the credits used and anything that sounds wrong.
```

## By hand, in a browser

Do these while the engine downloads. They need your accounts.

- [ ] The jam page says **Joined**: https://itch.io/jam/2026-epic-ai-megajam
- [ ] Find the Nov 10 partner email (university inbox, then spam). It holds most of the codes below.
  Checked Nov 13: not in the personal Gmail, spam included. The codes went to people who had joined by Nov 10, and
  itch.io was first linked on Nov 12, so ask the organisers for late-joiner codes. Tripo's page said "codes are
  temporarily unavailable" on Nov 13 (try again). WeLoveIndies account made with the personal Gmail (free licence, non-commercial).
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

- **`claude` or `git` isn't recognised in the second terminal:** the window was opened before the installs
  finished. Close it and open Terminal (Admin) again.
- **No sessions at claude.ai/code:** see `setup/0-remote.md`. Until it works, use the session windows on the PC
  through Moonlight.
- **The browser build fails:** drop it. The Windows package is the entry, and the rules require it anyway.
- **A push fails on Git LFS quota:** keep committing locally and carry on. The push is only a backup.
