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
- **About 15:00-16:10. Engine download started, Visual Studio updated.** With the disk cleared to 147 GB, Claude
  drove the Epic Games Launcher by computer use: Library > + > 5.9.0 Preview (the only 5.9 entry), core, templates
  and engine source, no symbols or extra platforms, 33 GB. Four things fought back. The Launcher's saved install
  folder was on a `D:` that doesn't exist on this boot, so Install silently did nothing until its config was
  edited. The Claude desktop window is always-on-top and invisible to screenshots, so clicks landed on it until the
  Launcher was made topmost. A RivaTuner overlay hid the dialogs mid-screen. And UAC prompts (the Launcher's folder
  step, the Visual Studio installer) cancel themselves after two minutes, so both waited until Marwan watched
  through Moonlight and clicked Yes. Visual Studio went from 17.6 to 17.14.45 with the game workload, one elevated
  script, 17 minutes.
- **The line is slow.** About 10 Mbps to the Launcher (Ethernet negotiates at 100 Mbps, the internet gives 10-18):
  the 13 GB download took about three and a half hours. A Launcher tweak for more parallel chunks made it slower
  and was reverted. The wait itself watched the wrong file (`LauncherInstalled.dat`, which this Launcher leaves
  empty) and sat two hours past the finish at about 19:50; the runbooks now check the Launcher's manifest instead.
- **About 22:00. `Reel` created without the wizard.** Claude copied the Third Person C++ template by hand, following
  its `TemplateDefs.ini` (renames, the three shared content packs, the redirects the wizard writes), turned on the
  All Toolsets plugin (the MCP server plugin is on by default), and registered the editor's MCP server for this
  Windows user. 5.9's compiler list bans MSVC 14.44.0-35210 and the toolset folder says 35207, but the compiler
  inside is 14.44.35230, so it passes. First command-line build of `ReelEditor`: 4.3 minutes, no warnings.
- **About 22:10. Editor up.** `UnrealEditor.exe Reel.uproject` opened straight on `Lvl_ThirdPerson` with no dialogs
  and no load errors; the MCP server answered on port 8000 after 1.2 minutes (server `unreal-mcp`
  5.9.0-60351404, 106 toolsets listed). Ready for the stage 3 session.

## Day 2, Fri Nov 13

- **About 11:30. K wins.** Marwan tried both browser prototypes (the 3D third-person K with the studio's script,
  and L's six-scene Tight Edit) and picked K, Meanwhile..., to be refined before the spec is written. Moonlight on
  the Mac was paired from the command line (`moonlight pair <address> --pin`), set to windowed, and streams the PC.
  The PC had restarted overnight: the editor and the `jam-pc` door were down and stage 3 had not run.
- **11:11-11:45 (the PC clock shows 2 h less). Stage 3: the pipeline, proven on the untouched template.** The
  PC had rebooted overnight, so the editor wasn't running and the new session started without its `unreal-mcp` tools. Claude started the editor
  (MCP up 59 s after launch), found it can't re-dial a project MCP server itself, and wrote `tools/ue_mcp.py`, a small
  client that talks to the editor's MCP server over HTTP. That route worked for everything that followed. Through it:
  five coloured cubes spawned with the editor's Python tool, seen in a viewport capture, then deleted with nothing
  saved; PIE started, `BP_ThirdPersonCharacter` possessed at the player start, seen in a PIE capture. The C++ loop:
  jump input routed through `DoJumpStart` with a log line, Live Coding compiled it in **24 s** (18 s for the revert),
  and the input tool pressed Jump in PIE: the log line appeared and the character left the ground. The slow route
  (exit, `Build.bat`, restart, check in PIE) took **102 s**: exit 6 s, build 11 s for one file, editor start to MCP
  29 s. Windows package, Shipping with prerequisites: **3 min 35 s** (build 80 s, cook 95 s, pak 4 s), 737 MB on
  disk; the `.exe` ran windowed and was alive at 20 s. `Test_Reel.zip` without the symbols file: **343 MB**, 14 s.
  What fought back: `cmd //c` from Git Bash mangles the quoted `.bat` path (PowerShell works); `FocusOnActors` returns
  before the camera moves; the on-screen debug text never showed in captures (the log did); Live Coding patches
  only memory, so the DLL on disk stays stale until the next `Build.bat`; a Windows Firewall prompt for UnrealPak
  popped up during packaging and was left for Marwan. DLSS: no plugin build for 5.9 (8.9.0 stops at 5.8).

- **About 11:50. The spec.** The pick was refined with Marwan into a story-driven cut: one job, three hired guns,
  and "the story only happens where the camera is" (objectives fill only on camera, three links between threads, a
  MATCH CUT for catching one land). Written into docs/STATUS.md with the win rules, tick order, event table, final
  kill list and a build order: core loop first, then the map it needs, then content, look, characters, audio. The
  Web build was dropped. Two play gates: Fri night (two heroes cutting) and Sat evening (a full run).
- **11:34-12:15. The Web build works on the template.** Marwan OK'd installing emscripten 6.0.10, which Epic's 5.9
  Wasm platform needs exactly. Claude cloned emsdk and installed it (694 MB over the slow line, 10 minutes; `emsdk.bat`
  wants `python` on PATH, so the engine's own Python stood in). Epic's guide says AllToolsets must be off for a web
  cook, so it went off for the package and back on after. The Shipping package took **23 min** the first time (Wasm
  compile 11.5 min, cook 6 min, Brotli 4 min) and passed Epic's `check_stage.py`: **223 MB** staged, largest file
  141 MB, **121 MB** zipped. In a Chromium tab with WebGPU it booted to READY in **2.9 s** on the forward tier. The
  attempt ran 34 minutes against a 30-minute clock: the cap fell during the last compression step, so it was let
  finish. The packaged `Play_Reel.bat` couldn't find Python on this PC; `serve.py` run with the engine's Python
  works. The spec, written meanwhile, dropped the Web build; this stays on record in case that changes.
