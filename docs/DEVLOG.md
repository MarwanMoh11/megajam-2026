# Dev log

Raw notes, newest at the bottom. One entry per milestone: time (Cairo), what changed, what the agents did, what
went wrong. Each evening these become one short itch.io devlog post with a GIF. Entry for the Glass Box prize.

## Day 0, Wed Nov 11 (before the jam; setup only)

- **Theme announced: Director's Cut.** My second jam. The first was Luxel, a 2D Godot game; this is my first
  Unreal project.
- **Plan.** Claude read the jam page and Epic's 5.9 Preview forum post and wrote the plan. Two findings changed it:
  UEFN doesn't run on a Mac, and a Mac can't package Windows builds. Moved the build to my Windows PC.
- **Lessons carried from Luxel.** The biggest: fun was judged last, and the first playtest felt like "a bland tech
  demo". This time a grey-box build gets played before any art or audio is made.
- **Brainstorm, two rounds.** Round 1 crossed out the ten ideas every AI gives first (director sim, retake time
  loop, editing puzzle...) and kept ten. Round 2 attacked round 1's favourites: the top pick, Plot Armor, broke
  under its own rule (an invulnerable hero has nothing to fear). Fixing that produced Meanwhile...
- **Tier list.** Two ideas locked for grey-box tests on Thursday morning: Meanwhile... (cross-cutting action) and
  The Tight Edit (a timeline puzzle with a stunt payoff). One needs a human to judge feel; the other Claude can
  prove with a solver.

## Day 1, Thu Nov 12

- **12:25. Setup starts late.** The build window opened at 07:00 and nothing was installed on the PC yet. To claw
  time back, Claude turned the setup checklist into three runbooks that an agent on the PC carries out (tools,
  project, pipeline), put everything in a private repo to clone there, and dropped the throwaway test project: the
  pipeline test runs on the real project instead. The pick between the two ideas moved to Friday 10:00.
- **13:00. The PC runs Linux Mint.** It dual-boots, and Mint is the boot Marwan wants to work in. Two facts shaped
  the plan: the jam accepts Windows builds and not Linux ones, and Unreal can't package a Windows game from Linux.
  So the game is built on Mint and the Windows boot only packages. Everything is driven from the MacBook: SSH
  carries the Claude sessions, Sunshine and Moonlight carry the screen. Claude rewrote the runbooks for that route.
- **13:00. Four Opus sessions on the Mac while the PC is set up:** browser grey boxes of both ideas, so the pick
  doesn't wait for Unreal; an art direction board; and a survey of the rules, other entrants, prior art and past
  winners. The survey came back first: both ideas clear the rules, one other entry overlaps The Tight Edit, and
  the rules require a Windows package.
- **13:25. Back to Windows only.** Marwan weighed what Mint would cost (no Live Coding, a second engine install, a
  reboot for every Windows package, a look tuned under Vulkan and judged under DirectX) and chose the Windows boot
  for the whole jam. The MacBook still drives everything: SSH for the Claude sessions, Sunshine and Moonlight for
  the screen. Claude rewrote the runbooks a second time, so that one paste at the PC's keyboard sets up SSH, the
  tools and Visual Studio while the engine downloads.
- **14:00. Setup becomes one command.** Marwan wanted to go upstairs once, start Claude Code with all permissions
  and walk away. So the Windows runbooks are now chained by `setup/auto.md`: four lines typed at the PC start a
  session that installs the tools, waits for the engine, creates the project, opens the editor and starts the next
  session itself. Claude Code runs on the PC and is steered from the Mac through Remote Control, which replaced the
  SSH plan before it was ever used. Signing in stays Marwan's: Claude, GitHub and the Epic Games Launcher.
- **Afternoon. Setup stalls on a full disk.** The `setup/auto.md` run installed the Launcher, Git, 7-Zip, OBS,
  Sunshine and Claude Code, turned off sleep, and recorded the PC in `docs/MACHINE.md`. Then it stopped: `C:` is the
  only Windows drive and had 18 GB free (15.7 GB by the next check), while the engine needs roughly 60-120 GB.
  Visual Studio 2022 is there, but without the Game development with C++ workload. No engine, no project, no editor
  yet. Marwan signed in to the Launcher. A separate Claude session now surveys the disk and clears only what Marwan
  approves, plus Windows bloat. Two snags: computer use is off in the desktop app, so the Launcher clicks wait for
  it; and the auto-mode safety check refused to start the Remote Control "door", so Marwan starts that by hand. The
  PC clock also runs about two hours behind (dual boot with Mint).
