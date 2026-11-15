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
- **About 12:00-13:00. The game changed: a heist in one take.** Marwan brought a new pitch, written as a screenplay:
  a con man at a gala, a lighting tech on the catwalk and a props hand backstage, with the player cutting between
  them so the con man gets the loot out in a single take. Three Opus agents judged it as jam judges, each with a
  persona: a gameplay designer, a cinematographer turned Unreal artist, and a jam veteran.
  - **Round 1, the pitch:** a mean of **3.9/5 as written and 2.5/5 as likely shipped.** Each obstacle had one
    glowing answer, nothing made a cut cost anything, the live AI interrogation couldn't ship safely, and the scope
    didn't fit.
  - **The refinement:**
    - Devices with three verbs (Blind, Lure, Block) that work on any guard.
    - Marks Julian must stand on before you cut away.
    - One hall on three layers instead of four areas.
    - Hard cuts.
    - A pre-voiced bluff in place of the runtime LLM.
    - The grey box turned into the style (a soundstage in rehearsal).
    - The stolen thing is the Director's Cut itself, and the end card is a strip of the player's own cuts.
  - **Round 2:** a mean of **4.3/5 as written and 3.4/5 as likely shipped.** Their second notes went in too:
    - Maya and Vex stay at their posts.
    - Every rule contradiction they found is fixed.
    - One lens per camera.
    - Manual exposure, and a skylight for the missing bounce light.
    - The ending is built before Scene 3.
    - Step 1 is trimmed to fit Friday night's play gate.
  - **Files:** the spec is `docs/STATUS.md`; the pitch and both rounds of scores are in `docs/ideas/`, and K's spec
    is archived there.
  - **Audio:** ElevenLabs (Marwan's subscription) will voice about 60 lines and make the sound effects, generated
    outside the editor in a parallel session.
- **About 12:50-13:35. Judging the design until it scored 5 (Marwan's request, design only).**
  - **The panel.** The same three Opus judges went from round 3 to round 7 at 3.7, 4.3, 4.6, 4.7 and then **5.0**.
    Each round folded their notes into `docs/STATUS.md`:
    - blocking Julian to marks;
    - a jazz band as the clock and score;
    - the film against the making-of;
    - deleted scenes;
    - the replay and its edit suite;
    - a shot list for camera A;
    - fixes for soft-locks and contradictions.
  - **Fresh judges.** As a check, new judges who had seen no earlier version read the 5.0 spec and scored it **3.9**:
    too many rules for 3 minutes cold. A subtraction pass followed (point-and-click, three effects, one meter, a clean
    run of about 1:45), then one job per camera (cue on A, fire on B and C). New judges then scored 4.0 and 3.9;
    judges re-reading their own fixes scored 4.4 and 4.6.
  - **Stopped at a taste plateau.** The judges began asking for opposite things, so the loop ended there.
  - **What we learned:** a panel that sees every round anchors on its own notes. Only fresh readers catch bloat.

- **About 13:35-14:30. A rubric, then two blind rounds under it.**
  - Marwan asked for a fixed yardstick so the judging couldn't loop forever. `docs/JUDGING_RUBRIC.md` copies the
    jam's seven criteria and adds anchors for each score, fresh judges only, a rule for which changes get in, and a
    stop rule.
  - Claude wrote the take as a screenplay, with a scorecard for Marwan, on a private page.
  - Round 1, three blind Opus judges: 4.1, 4.0 and 3.9. Applied: sound-only warnings, one way to be seen, a 32 s
    replay, the first cut as the front/back flip, one arrival at a time, and seven contradictions fixed.
  - Round 2, three new judges: 4.1 from all three. The rubric's plateau rule stopped it. Applied: a path that shows
    only danger that exists now, a grade on deleted scenes alone, and twelve defect fixes. Logged round by round in
    `docs/ideas/heist-judging.md`.
  - What went wrong: earlier dev-log times said 17:30 when it was 13:35, and they are corrected.


- **About 14:25-15:25. Step 1 starts: one header pass, reviewed before anyone writes a body.**
  - For this step Claude Fable 5.2 plans, splits and checks, and Opus 5.5 agents build. One agent wrote every header
    for the core loop: 18 classes in `Reel/Source/Reel/Take/`, with every function body left as a marked stub and
    every tunable exposed in the editor. The headers are the contract between the five builders who come next.
  - The design it settled: one director actor holds the rules and steps everything in the spec's tick order, and
    nothing else ticks. Every log line carries the frame number, which is how "same frame" will be checked.
  - Fable read all the headers and the tick skeleton against the spec and asked for no changes.
  - What went wrong: the camera enum and the camera actor got the same name in the editor's Python, so the agent
    renamed one and needed a second build and restart. The PC's clock was 2 hours slow and was corrected mid-run,
    which made the agent look 2 hours late when it wasn't. The session's own editor tools died at the restart, as
    expected; `tools/ue_mcp.py` took over.
  - Smallest fixes, logged in STATUS.md: no camera C in the test box, the spot is fired by clicking a watcher, and
    the goal mark sits outside the tape.

- **About 15:25-16:10. Five builders write the bodies in parallel; the test box is built at the same time.**
  - Five Opus agents each took one slice of the stubs and their own `.cpp` files: (a) clock, composure, red tape
    and HUD; (b) cameras, the cut and the pointer; (c) Julian, marks and the walk; (d) the guard, the beam and being
    seen; (e) the devices and the blooper. None of them could compile: one build at the end, by the orchestrator.
  - While they wrote, Fable built the test box through the editor's Python from one script,
    `tools/build_testbox.py`: floor, three walls, a rope with a gap, three marks, the red tape, the beam, the guard
    and his two patrol points, the cart, the banner, the spot, the film's two angles with their zones, and Maya's
    camera. Re-running the script rebuilds the map from its numbers.
  - Four slices came back in 8 to 9 minutes each and compiled clean on the first build, with no warnings. In
    play-in-editor, driven by console commands that go through the same queue as the keys: the cue found a 25.5 m
    path (10.2 s, 20.4 beats), the cart rolled for a second and lured the guard, the spot blinded him and refused a
    second target while busy, the film cut to its second angle as Julian passed the zone, and the beam touch froze
    the take for a second and reloaded the level with every device restored.
  - What went wrong: the first layout had Julian walking right to left on screen, because a camera looking along +Y
    has +X on its left, so the map was mirrored. Auto exposure washed the dark floor out to grey, so the map got a
    fixed exposure. The guard's frame at 12 m covered half the room and was cut to 9 m. Builder (a) thought for 22
    minutes without writing a file while the others had finished, and needed a nudge to write plain code first.

- **About 16:10-17:00. All five slices compile; both routes and both failures pass in play-in-editor.**
  - Builder (a)'s clock, composure, tape and HUD arrived last. The full build was clean on the first try again, so
    no compile error ever went back to a builder.
  - Fable played the box through console commands on a game-time schedule and read the log, where every line
    carries its frame number:
    - **Route 1, banner plus spot:** cue Julian to the goal, drop the banner over the beam, spot the guard as
      Julian enters the red tape. CLEAR with composure 100.
    - **Route 2, cart plus spot:** cue, spot the beam before he crosses it, crash the cart so the guard walks off
      to look. CLEAR with composure 100.
    - **Failure 1:** no device at all, he touches the beam: blooper on that frame, a 1 s freeze under the red
      slate, reload, devices and composure restored.
    - **Failure 2:** banner only, the guard holds him in the tape: SEEN, composure 100 to 0 in under 4 s, blooper,
      reload.
  - Real keys and real mouse clicks were sent to the play window from PowerShell. Each press and its reaction
    carry the same frame number (press 2, view to B, cut: all frame 5550), and the first click is not eaten.
  - What went wrong: a screenshot in the middle of a run froze the game for seconds and squeezed the schedule, so
    the runs now wait on game time. The film's first angle did not have Julian in it, so the camera moved back and
    the west wall came out. A guard who walks between two points was not dangerous enough (a blind crossing left
    Julian alive), so he now stands and turns on bar lines: three bars watching, one bar away.

- **About 17:00-17:30. A fresh reviewer, its fixes, and the Windows package.**
  - One new Opus agent that had seen none of the work read all the code against the spec's rules and tick order,
    for bugs only. It confirmed the tick order, the reload and the packaged-build paths, and found eight bugs. The
    two that mattered were in how effects stack on a watcher: a cart landing on a blinded guard made his frame
    live again while the spot was still busy on him, and spotting a beam the banner already masked cut the mask
    from 10 s to 4 s.
  - The builders who own those files fixed them (separate timers, the longer effect wins), plus: a swept test so
    a long frame can't carry Julian through the beam, the frame's beat pulse as brightness so the red on the
    floor is exactly what catches him, clicks that reach what the current camera acts on, and no silent presses
    (the mark he already stands on, and presses under the blooper slate, where R now starts the new take).
  - Every fix was replayed in the editor: the stacking cases, both routes, both failures.
  - A trial package while the review ran caught what the editor could not show: at 16:9 the cameras' 60 degree
    lens cropped Julian and the goal mark, and the HUD text was small at 1440p. The three cameras were reframed
    in the map script and the text scale went up. Route 1 was then played in the packaged game with real mouse
    clicks and keys, through to the CLEAR stamp.
  - What went wrong: nothing broke, but the editor window is much wider than a 16:9 screen, so framing checked
    only in the editor was wrong. Check framing in the package.
  - The final package for step 1 (17:20): `C:\Jam\Builds\Windows`, zipped to 320 MB. Started from the `.exe`, it
    opens in the test box, and route 2 (spot on the beam, cart for the guard) was played in it with real clicks
    to the CLEAR stamp. Three screenshots from the package are in `docs/shots/`.

- **About 18:15-19:35. Gate 1 answered: the rework (walk, flights, a real building).**
  - Marwan played step 1 and changed three rules: walk the characters yourself in third person (slow, weighted,
    Uncharted as the reference) instead of clicking marks; switch between Julian, Maya and Vex with a camera
    flight (GTA V as the reference) instead of a hard cut; and a massive auction building instead of the test
    box. All three were on the kill list, and his message lifts them. `docs/STATUS.md` records the change and what
    Claude filled in.
  - Same pattern as the afternoon. One Opus agent did the header pass in 12 minutes (a shared `ATakeCharacter`
    for all three, the flight on the player controller, a "use target" on the director) and it built clean the
    first time. Three Opus builders then wrote the bodies in parallel on their own files: characters and marks;
    controller, flight and presses; use targeting, devices and HUD. The orchestrator (Fable) wrote the map
    script and the play-test scripts meanwhile.
  - At 18:50 Marwan sent a second note: a bigger building with a second floor and fire exits, "like a Hitman
    level", scoped to 15 minutes at most. The map script was rewritten: about 85 x 62 m, two floors, 504 actors.
    A two-storey lobby with an atrium and a grand stair, a cloakroom, restrooms, an office and the security room,
    a fire stair at each end with exit doors and green signs, a gallery and walkway on the hall's second floor,
    a north wing (preview gallery, stock room, director's office, projection room, archive), a service wing
    (backstage corridor, kitchen, prop store, loading dock) and a vault behind the dais. Everything west of the
    rope connects; the only way to the dais is still the gap and the aisle.
  - Checked in play-in-editor with scripted runs and the log's frame numbers. Route 2 (cart for the guard, spot on
    the beam) clears with composure 90. Route 1 (banner on the beam, spot on the guard) clears with composure
    14. Walking into the live beam is a blooper; crossing the tape under the guard's eye drains composure to 0, a
    blooper; both reload as the next take. Use on Julian, a cut to the character you are on, presses during a
    flight and Use out of reach each flash a line. Julian was walked up the grand stair (feet at 450), through
    the second-floor door onto the hall's gallery, and down the north fire stair to the lobby (450, 225, 0).
    Screenshots: each character's view and three frames of a flight.
  - What went wrong:
    - Sending the second note interrupted the session and stopped two of the three builders; a stopped agent
      can't be resumed. They had finished everything except the two press handlers and four HUD elements
      (reticle, use prompt, crew strip, the flight's "CUT -> MAYA"), so the orchestrator wrote those. The build
      was clean the first time.
    - Vex's corridor sat in the hall wall's shadow and was nearly black: lamps along it and a stronger skylight.
    - A "mid-flight" screenshot was taken after the flight had landed. The flight is now captured in slow motion.
    - Maya started 10 cm out of reach of both her devices, so her first Use press was refused. Moved her.
    - A test walk timed out because the scripted path ran Julian into a lobby bench: the script's fault.
    - The flight costs 1.6 s each way and the spec's device times were set for instant cuts, so Route 1 is
      tight. Left as is: it is a tuning call for Marwan after he has played it.

- **About 19:35-20:30. The reviewer's four bugs, the package, and a test that fooled itself.**
  - One fresh Opus reviewer read the rework against the changed rules and the tick order, for bugs only. It
    confirmed the tick order, the flight (start, landing, a blooper or new take mid-flight), the highlights and
    the Shipping paths, and found four bugs, none of which breaks a take:
    - the reach test had no height, so Maya could work the lamp and the winch from the floor under the bridge;
    - the blooper freeze stopped the rules but not the walkers, so Julian slid on through the beam under the
      slate;
    - the use prompt stayed stale for a frame after a fire, and a second Use press in that frame got the wrong
      refusal;
    - a Use press on a follow spot fired the level's first spot, not the one in reach (hidden with one spot).
  - All four were small (40 lines), so the orchestrator fixed them, and each was replayed in the editor: 0.3 s
    into the freeze Julian is at the beam with speed 0, Maya on the floor under the lamp gets "Nothing in
    reach", the prompt changes on the fire frame, and Route 2 still clears (composure 93).
  - The Windows package (95 s) was played from the `.exe` at 16:9 with real key and mouse events: W walks
    Julian, the mouse turns the view, 2 flies to Maya, E at the lamp asks for a watcher, 3 then E releases Vex's
    cart and the guard is lured, R starts take 2.
  - What went wrong: the first packaged runs said the keyboard was dead while the mouse worked. Six runs went
    into the game's input code and three ways of sending keys before a control run against the afternoon's
    build, which was known to take keys, failed the same way. The fault was in the test: PowerShell variable
    names ignore case, so the key table `$K` was hidden by a function parameter `$k` and every key sent was
    empty. Lesson: when a new test disagrees with code that worked, run the test against a known-good build
    before reading the code.
  - The final package for the rework (20:04): `C:\Jam\Builds\Windows`, zipped to 326 MB
    (`C:\Jam\Builds\TeamName_GameName.zip`). It was played once more from the `.exe` after the fixes, with the
    same keys. Five screenshots are in `docs/shots/` (`gate1b_*.png`): the building, Julian, a flight, Maya, Vex.
- **About 22:30-23:35. Marwan played the rework: start on the street, and dress the place.**
  - Marwan played the packaged rework and sent two notes: "shouldnt I start outside of the building not inside ?
    the line is outside", then "also add more props placeholders to make the place feel alive". The spec's queue
    was at one end of the hall; with a whole building it belongs at the front door.
  - The orchestrator did this round itself (one map script, 40 lines of C++), with no subagents.
  - **The start is on the street.** `tools/build_auction.py` now builds a pavement and a road along the front, opens
    the front doors, and puts Julian at the foot of a red carpet. Out there: the queue behind ropes, a doorman, the
    press with tripods, a canopy and a sign, a scanner arch as a placeholder for Scene 1 (no rule yet), windows,
    planters, street lamps, a limousine and parked cars. Round the other sides: trees, staff cars, a lorry at the
    dock shutter, bins. Unseen walls at the street's edge keep everyone on it.
  - **The checkpoint (rule 9) had to come with it.** From the street Julian walks about 22 s to reach the rope, and a
    blooper reloaded the level from his placed start, so every retry would have cost that walk. A mark can now be
    a checkpoint (`bCheckpoint`, `CheckpointRadius`): once Julian comes within 12 m of the hall mark, the game
    instance remembers it and a blooper starts the next take there, with the slate reading "AGAIN FROM THE HALL".
    R (a new take) goes back to the street. The follow camera is snapped to him after the move, or its lag would
    sweep it across the map.
  - **Props and people in every room**, about 830 new pieces (1,336 actors in the map): 36 bidders on the hall
    benches, cocktail tables with guests, a bartender and stools, phone bidders, an auctioneer at the podium,
    banners on the columns, pictures on the walls, sofas and plants on the lobby balcony, film cans and reels in
    the projection room, kitchen, prop store, loading dock and vault dressing, a red extinguisher and alarm point
    by every fire exit. People are a cylinder and a sphere: they are not watchers and carry no rule.
  - A rule for the dressing, so the routes play as before: small props and standing people stop a walker but
    ignore the camera and visibility channels (sight lines, the aim and the follow camera pass through), seated
    people and wall dressing have no collision at all, and nothing new stands east of the rope except the
    auctioneer and two easels on the dais.
  - Checked in play-in-editor from a street start: the walk-in takes 21.7 s, "CHECKPOINT - THE HALL" flashes as
    Julian passes the benches, a beam blooper restarts him on the hall mark, R puts him back on the street. Route
    2 and Route 1 both clear after walking in (composure 100 and 24 at the goal).
  - What went wrong: nothing broke. One false alarm: the editor printed "Video memory has been exhausted" during
    the tests, because Marwan's packaged game was still open beside it; with the game closed the warning is gone.
  - Four screenshots in `docs/shots/` (`gate1c_*.png`): the front from the road, Julian's start, the lobby, the hall.
  - Packaged at 23:40 (65 s; zip 326 MB, `C:\Jam\Builds\TeamName_GameName.zip`) and played from the `.exe` with
    real key and mouse events: W walks Julian up the carpet, through the scanner arch to the open doors with the
    lobby and the grand stair ahead; 2 flies from the street to Maya; E, 3 and R behave as before.

## Day 3, Sat Nov 14

- **About 23:50-01:10. Step 2 starts: Scenes 1 and 2 for the walking game, built by three Opus builders at once.**
  - Marwan played the street-start build and said to start step 2, leaving out everything written for fixed
    cameras and hard cuts until gate 2 (the shot list, the film and making-of split, the monitor strip, good cuts,
    stills, the recorder, three deleted scenes). He chose Opus at its highest effort for every builder and for the
    reviewer. `docs/STATUS.md` has a new section, "Step 2 as built", with what was built and every gap Claude
    filled.
  - Same pattern as step 1, and it held again. The orchestrator (Fable) wrote the header pass itself in about 35
    minutes: five new classes (the scanner with its gate, the drone, a scene trigger box, a crew post, the sound
    desk) and about 40 new functions on the director, the watchers, the devices, the clock and the HUD, each with
    a comment saying exactly what it must do, and a stub body so it built clean. Then three builders wrote the
    bodies in parallel on files nobody else touched: W, watchers and sight (9.5 min); S, scenes, triggers and
    checkpoints (6 min); D, devices, the clock's sounds, the HUD and the test commands (10.5 min). None of them
    could compile. The one build at the end was clean on the first try, with no warnings.
  - While they wrote, the orchestrator rewrote the map script: the scanner arch at the front doors becomes a
    watcher with a gate, Maya starts on the street with a second follow spot, the drone gets a path down the
    centre aisle with gold marks under it, the dais gets Vance, the bodyguard and the lift mark, Vex gets a third
    hatch with a second cart and a sound desk, five trigger boxes start the scenes and save the checkpoints, and
    the sun is replaced by a night look (a cool moon, haze, warm keys, cool rims, red and gold accents; Lumen off).
  - Checked in play-in-editor with scripted runs, reading the log's frame numbers. From the street: the goal line
    teaches walk, [2], [E], [1], walk through; the spot opens the gate; walking onto the hall floor starts Scene 2,
    saves a checkpoint and moves Maya to the bridge; Julian crosses the floor from mark to mark under the drone,
    passes the rope by route 2 (cart and spot), reaches the dais and lifts the keycard with the spot on the
    bodyguard while Vance speaks. CLEARED on bar 42, about 84 s. Also passed: route 1 (banner and spot), the sound
    desk, the dais cart (x2), a walk into the live scanner (the gate holds him, a blooper back to the street), and
    a blooper at the rope after spending the cart (Julian back on the rope mark, the clock back at the saved bar,
    the cart back).
  - What went wrong:
    - The first lighting pass was washed out, and its "warm pool" on the hall floor turned out to be the drone's
      untextured disc, so two rounds of tuning chased the wrong thing. The night sky gives a skylight almost
      nothing to capture, so the fill needed 14 times the daytime number.
    - The lift's 1.5 s missed the follow spot's window by a quarter of a second: a flight back from Maya costs
      1.6 s of the 4 s. The mark is now 1 s, and the spec's number is noted in STATUS.md with the reason.
    - A turning guard's frame swept across Julian for one frame on every bar line and made Vance shout
      "Security!" at nothing. The line now waits for the grace time to run out.
    - The guard's bass warning played on the street, 60 m from the guard. Turn warnings now need the watcher
      within 30 m of Julian.
    - A test that was meant to end in a blooper did not: with the banner down, Julian walked the aisle on the
      guard's away bar and came out with 20 composure. That is the free route the spec allows, not a bug, but it
      meant the next test started from the wrong checkpoint and had to be rewritten.

- **About 01:00-02:40. A reviewer, and a new note from Marwan in the middle of it.**
  - One fresh Opus reviewer read the whole step 2 diff for bugs only (67 tool calls, about an hour of its own
    time, running beside the tests). It found no crash and five things, none fatal: every sound went through one
    queue, so a warning phrase pushed the next click up to 0.4 s late and two warnings on one bar line played one
    after the other; a checkpoint restore announced every arrived watcher again; a checkpoint could be saved on
    the very frame a blooper fired, past the thing that caught him; Maya could be left behind by the scene
    change; and a test command could trip the beam. The orchestrator fixed the four that still applied (the
    warning phrases now have three voices of their own).
  - While that ran, Marwan sent a note through the session that started this one: the queue belongs outside the
    venue, the crew's places and what they control should make sense in the story, Julian should not just walk
    in a straight line, and "there should be clever game design basically and narrative building and immersion".
    The working build was committed and pushed first, then the placement was reworked inside the rules already
    built. No new system, mostly the map script plus about 150 lines of C++:
    - **The street.** Three lanes to the doors: the red carpet for invited guests (red tape, the doorman's frame
      straight down it, a rope at the doors), the queue lane to the scanner, a roped crew lane. Julian starts at
      the back of the queue along the kerb.
    - **The crew walk; nobody teleports.** Maya starts on a terrace over the doors with an arrivals searchlight
      that looks straight down on the scanner, then walks the lobby balcony and a new catwalk to a lighting
      platform over the benches (22 s in the scripted run). Vex starts on the loading dock and walks to his cart
      (6 s). The crew-post teleport written an hour earlier is no longer used.
    - **A blooper keeps the crew where the player walked them.** Without this every retry would have cost the
      walks again; nobody watches the crew, so their place is not part of the risk.
    - **No straight line.** The doorman owns the carpet, the security chief the carpet inside, the broadcast
      drone the hall's centre aisle, whose camera riser now blocks it: Julian goes round by a side aisle.
    - **Words.** A cover-story line per character on first landing, an opening slate, a line from each watcher
      when it really catches him, lane signs in the world, Use prompts that say what the hand does.
  - Checked again in play-in-editor: the full take from the back of the queue clears on bar 66 with both crew
    walks in it; walking up the carpet gets "Invitation, sir?" and a blooper back to the queue; a blooper at the
    beam restarts at the rope with the cart back and Maya and Vex still at their posts.
  - What went wrong:
    - The orchestrator's own fix broke the build once (a missing include): the only compile error of the night.
    - A leftover line in the map script kept the banner's winch on the old bridge after Maya's post had moved, so
      route 1 pressed Use on nothing and walked Julian into the live beam. The scripted test caught it.
    - The first searchlight left 0.35 s to get through the gate after the flight back. It now blinds for 6 s.
    - The carpet's ropes ran to the kerb, which closed the wrong way in so nobody could find out it was wrong.
      Its first metre and a half is open now.


### 02:16, Sat Nov 14: the new layout proven in the editor, not yet packaged
- Scripted play-in-editor runs on the layout after the Nov 14 note all pass: the whole take from the back of the queue
  (cleared on bar 66), the carpet blooper (doorman, back to the queue), the beam blooper (back to the rope at bar 15,
  the banner use given back, the crew still at their posts), route 1 (banner then follow spot, composure 21 on the
  dais), the sound desk lift (x5) and the dais cart (x2).
- What went wrong: a stale BANNER_USE line in the map script put the banner ring on the old spot, so route 1 found
  nothing in reach. Removed.
- Not done yet: the packaged build of this layout and the real-input run of the exe (the session ran out of usage
  here). The key script is ready outside the repo except for two aim numbers for the beam. Known and cosmetic:
  composure still drains after SCENE 2 CLEARED.

### 03:53, Sat Nov 14: step 2 packaged and played from the exe with real keys
- **What changed:** the composure bar now holds after SCENE 2 CLEARED (it used to drain behind the end title). Mouse
  smoothing is off in `Reel/Config/DefaultInput.ini`. Packaged for Windows (Shipping), zipped (326 MB).
- **The proof:** one run of the packaged exe driven by real keyboard and mouse input (a PowerShell script outside
  the repo, `C:\Jam\Builds\shots\play2.ps1`), 164 s from the back of the queue to the keycard: The Door, Vex and
  Maya walked to their posts, the floor round the blocked aisle, a beam blooper on purpose (back on the rope mark
  with the clock back from bar 62 to bar 60 and the crew still posted), route 2, the dais cart (x2), SCENE 2
  CLEARED. Screenshots in `docs/shots/step2_*.png`.
- **What the agents did:** Claude (the orchestrator) did all of this part alone: the editor runs, the package, the
  key script. No subagents ran.
- **What went wrong, and it took four tries of the exe:**
  1. The first run drifted: Maya ended against the gallery rail and Julian clipped a bench. Walking distances were
     exact, the mouse turns were not.
  2. Unreal smooths mouse input by default. With injected mouse moves at a high frame rate the smoothing added up to
     40% to a turn, and not the same amount twice. Turned off: a turn is now exactly 0.136 degrees a count (the
     engine scales the look by the camera's field of view, 70).
  3. Left turns still did nothing. That was the test script, not the game: PowerShell hands a bare `-90` to an
     untyped function parameter as text, so `$deg * 1.29` repeated the string. Typed the parameters.
  4. The scripted editor test walks with a command that steers round props, so it never showed that Vex's straight
     line to the dock door clips a pallet of crates. The key script now goes round it. The same test also sent
     the use and the cut on one frame once (the cut wins, the use is refused), which read as a failed lift.
- **Still open:** route 1 is tight (about 20 composure left on the dais). The band-break checkpoints of rule 9 are
  not built. Labels overlap on Maya's camera when several watchers line up.

### 05:05, Sat Nov 14: step 3 built and proven in the editor (Scene 3 and the ends), not yet packaged
- **What changed:** the take no longer stops at the keycard. Lifting it starts Scene 3, The Getaway: the case, the
  alarm with its hold and its 8-bar clock, shutters, four guards through two doors, the drone locking on, the
  colour bleed; two ways out (the stage trap by Vex's winch and Maya's pyro, the gold side exit by the saved sound
  desk), The Bow, then PRINT IT, the stats and grade, the blooper reel over the credits, and R. A title card opens
  the session. Two new checkpoints (the keycard, the case). `docs/STATUS.md` has a new section, "Step 3 as built",
  with every gap Claude filled.
- **What the agents did:** the same pattern as steps 1 and 2, and it held a third time.
  - The orchestrator (Fable) read the spec and the code, designed Scene 3 for the walking game, and wrote the
    header pass alone: three new classes (the pyro, the winch with its curtain, a shutter), two new phases (the
    title card, PRINT IT), and about 30 new functions and 40 tunables, each with a comment saying what it must do,
    with stub bodies. It built clean in 28 s.
  - Three Opus builders then wrote the bodies at once on files nobody else touched: S, the director's scene logic,
    the alarm and the ends (7.5 min, 71 tool calls); W, watchers, devices and shutters (7.4 min, 66); D, the HUD,
    the synthesised sounds and the input (9.4 min, 50). None of them could compile. The one build at the end was
    clean on the first try, with no warnings, for the third step running.
  - While they wrote, the orchestrator added Scene 3 to the map script (the lots table, the trap, the band and its
    PA stacks, two doors, the guards, the winch, the pyro board, the shutters) and wrote the test scripts.
  - One fresh Opus reviewer read the whole diff for bugs only (73 tool calls, 29 min, beside the tests). No crash
    and no break in Scenes 1 and 2. Two confirmed defects: floating "OFF" labels for the hidden guards, and a
    shutter that would drop onto a crew member parked in a doorway and trap them. Both fixed.
- **The proof (scripted play-in-editor runs, read from the log's frame numbers):**
  - The whole take from the back of the queue to PRINT IT: 163.6 s on the band's clock, 16 cuts, no bloopers,
    ROUGH CUT by the trap, both crew walked to their finale posts.
  - A blooper after the keycard: back on the dais mark with the keycard, Scene 3, bar 18.
  - The clock running out: "THE CLOCK RAN OUT" on bar 30, again from the case with the clock back on bar 20, the
    alarm in its hold again, the crew still posted.
  - The gold side exit: behind the PA stacks unseen, the desk (everything blind, x9), The Bow restored, out:
    DIRECTOR'S CUT.
  - The step 2 tests again (the carpet blooper, the beam blooper back to the rope, route 1, the desk lift): pass.
  - Screenshots: `docs/shots/step3_*.png`.
- **What went wrong:**
  - The first whole-take run died a quarter of a second short of the trap. The pyro had been fired the moment the
    prompt said to, while the drone was still flying in from the aisle, so it missed the drone; the drone then
    arrived unblinded and drained him for the whole walk. A player doing exactly what the goal line says would
    have lost. The pyro now whites out a drone that is locked on him, wherever the drone is.
  - The first pyro was a white screen: 9000 candelas under a fixed exposure. It is 420 now.
  - "ALARM" showed twice (a title and the HUD's clock), and the last line of the take stayed on the PRINT IT slate.
  - The orchestrator's own test assumed the clock would run out if Julian stood still. It does not: the drone
    kills him first, the take restarts, and the test script kept walking a character that no longer existed while
    the game looped 28 bloopers by itself. The test now hides him under the walkway to reach the clock.
  - The spec's sound desk (4 s) left 2.4 s after the flight back, not enough to reach the gold door. It is 6 s
    now, noted in STATUS.md for Marwan.

### 05:20, Sat Nov 14: step 3 packaged and played from the exe with real keys
- **What changed:** packaged for Windows (Shipping), zipped (326 MB; the step 2 zip is kept beside it). The key
  script outside the repo (`C:\Jam\Builds\shots\play2.ps1`) now plays on into Scene 3.
- **The proof:** one run of the packaged exe driven by real keyboard and mouse input, 202 s: the title card, The
  Door, both crew walks, the floor, a beam blooper on purpose (back to the rope), route 2, the dais cart (x2), the
  keycard, Vex walked to the winch and Maya to the pyro board, the case, the alarm, the winch, the pyro, past the
  lots table to the trap, PRINT IT, the stats (take 1, 3:03, 17 cuts, 1 blooper, out by the stage trap), the
  blooper reel, and R: take 2 on the street with no title card. Screenshots: `docs/shots/step3_pk_*.png`.
- **What the agents did:** the orchestrator alone: the package, the key script, the runs. No subagents.
- **What went wrong:** the first exe run died one step from the trap. The game was right and the script was slow:
  its pauses to settle between legs and its screenshots took 7.5 s from the pyro to the trap, and the pyro's 6 s
  plus 3.8 s of composure, less the 1.6 s flight back, is 8.2 s. That run did show the Scene 3 blooper in the
  package (composure zero on bar 93, again from the case with the clock back on bar 86 and the alarm in its hold),
  but the second run's screenshots overwrote those two. The script now walks one straight leg past the table
  with no pause. The number is worth Marwan's eye: a player who hesitates more than about four seconds after the
  pyro loses the take to the drone, not to the 8-bar clock.
- **Still open:** the finale's fairness cold (above). The guards' frame lines show through the curtain's cloth,
  which reads as if they could still see him. Route 1 is still tight. The band-break checkpoints of rule 9 are
  not built. Labels overlap on Maya's camera when several watchers line up.


### 19:10, Sat Nov 14: step 4 pass 1, the look: the film grade, the stencil, the overlay (packaged, played with real keys)
- **What changed:** Julian's camera is now the film: noir monochrome where only the things that carry meaning keep
  their colour, red for danger (watchers' frames and lines, tally lights, red tape and the carpet, the scanner's live
  bar and eye, the beam, the drone's circle) and gold for Julian (his marks, the scanner's open bar, the reel's case,
  the gold side exit, the trap's mark). Maya's and Vex's cameras are the making-of: steel blue video with viewfinder
  corners and scan lines. The grade swaps on the frame of the cut press. At the alarm the picture bleeds to full
  colour over 2 s and the live frames turn white and pulse on the beat; a restore snaps it back. The step 3
  saturation stand-in is gone. On Canvas: 2.39:1 ink bars on every camera, grain, flicker, dust and scratches at
  24 fps on the film, a paper flash and a frame-line roll at each cut, a danger burn at the picture's edges as
  composure falls, a burn and a paper flash before the blooper slate, and an ink frame at PRINT IT.
- **How:** one post-process material (`tools/build_look.py` builds it from text; `build_auction.py` puts it in the
  volume) runs before TSR, so the stencil's edges are anti-aliased with the picture instead of crawling. The numbers
  are the designer's (ART_STYLE §15). Everything meaningful is drawn in an unlit signal colour with a custom-depth
  stencil. Switches on the director: `bFilmLook` (off = the old picture), `bMakingOfSplit` (off = all film); every
  overlay number is on the HUD's `Film`.
- **Measured:** a live red tape pixel on the film camera (199, 47, 31) in the package and (208, 50, 34) in the
  editor against `#D3301F` (211, 48, 31); a gold mark (229, 182, 73) against `#E9B949`. Frame cost in the editor at
  three fixed views: GPU +0.3 to 0.6 ms, draw thread +0.7 ms (`docs/shots/step4_before_*` and `step4_after_*`).
- **Proof:** the four scripted tests in the editor (the whole take, Scene 3 by the trap, the side exit with The Bow,
  the failures), then one run of the packaged exe with real keys: a whole take to PRINT IT (3:02, 17 cuts, 1 blooper,
  out by the stage trap). Screenshots: `docs/shots/step4_p1_*.png`.
- **What the agents did:** one builder session alone: code, material, map, tests, package. The designer agent wrote
  §15 in parallel; no subagents.
- **What went wrong:** three traps, each found on a screenshot. The custom-depth pass ignores what stands in front,
  so a gold stanchion in front of a red frame kept its own colour: the grade now keeps a pixel only where the
  tagged surface is the one seen. After the grade the engine brightened the picture by a constant factor (display
  x1.25), found with a test pattern drawn by the material, and divided out. And a red source tinted by a
  per-channel gain turned the grey tally lights teal: the grade now maps a saturated red or gold onto its token and
  leaves greys alone. The first exe run walked off the keycard mid-lift (the dead-reckoned key script is about a
  second sensitive there); the rerun waits longer on the mark.
- **Needs Marwan's eye:** the grain, flicker and burn levels; how dark the film's hall reads; the making-of blue;
  the bleed into the pyro's orange at the finale; whether the split reads as film against making-of at all.

## Nov 14, about 19:45 Cairo: step 4 pass 2, the HUD and every word on screen

- **What changed:** the HUD is redrawn as one family on the film's own titles (ART_STYLE §15.5-15.7). The letterbox
  bars hold the lanes on every camera: top, the slate strip (scene, name, take), the key tabs with device pips and
  the flight's growing line, the bar clock (four beat squares, BREAK) or the alarm's count (ALARM, bars left, eight
  red squares, seconds in the last bar), and the goal line with a rule that draws out when it changes; bottom, the
  Use prompt or a refused press saying why (the pressed tab shakes), Julian's mark action with its gold bar, the
  composure line (SEEN square, MARK T), the controls row. In the picture: subtitles split into speaker and line,
  camera-tape labels on the crew's cameras under a label rule (priority, at most four plus the one you aim at,
  stacking, never in the bars or under a subtitle), countdown tags only on Julian's camera, the reticle on the
  crew's cameras. Stamps land at 1.30 scale and tilt (X2, CLEARED, DELETED SCENE RESTORED, the grades); slates for
  the opening (lower left, while you walk), the blooper and PRINT IT; cards for the title card, the stats and the
  blooper reel. Three OFL fonts (Limelight, IM Fell English, Josefin Sans) are imported as font faces and drawn
  through runtime fonts built in C++. 93 copy patches: one name per thing (CHAMPAGNE CART, SOUND DESK, PYRO BOARD,
  CURTAIN WINCH), THE STREET, no spaced dashes, nobody runs, "New take".
- **How:** a drawing kit (`TakeHudKit`) holds the type roles, keycaps, glyphs, stamps, cards and slates; every
  number is a token in `TakeStyle.h`; the HUD's tunables (label rule, timings, the words) are on `TakeHUD`. Flashes
  now carry a kind, so the director says where a line goes (goal lane, B1, subtitle, stamp) instead of a colour.
- **What the agents did:** one builder session: phase 1 staged the code and the copy patches while pass 1 still
  held the editor; phase 2 dropped them in, built, checked in play-in-editor screenshots, ran the tests, packaged.
- **What went wrong:** the first PIE run drew every box and no letter: `FCanvasTextItem` draws only when a `UFont`
  stands behind its font info, so each face now gets a transient runtime `UFont`. Unreal's shadow warning treats a
  parameter named like a style token (`Ink`) as an error. The editor's wide viewport has almost no letterbox, so the
  views were shot in a 1600x900 PIE window; that window runs at 12 fps and broke the timing of the gold-door test,
  which passes in the normal viewport. Pass 1's real-keys shots were offset by DPI scaling: the new script makes
  itself DPI-aware before measuring the window.
- **Proof:** play-in-editor screenshots of every view (`docs/shots/step4_p2_pie_*.png`); the four scripted tests
  (whole take, Scene 3 by the trap, the side exit with The Bow for DIRECTOR'S CUT, the failures) reach their ends;
  the Shipping package run with real keys (title card, the queue, a cut to Maya, a refused cut, a blooper at the
  scanner's tape) shows the three fonts and the HUD (`docs/shots/step4_p2_*.png`). Zip: `TeamName_GameName.zip`,
  326 MB; the pass 1 zip is kept as `TeamName_GameName_step4p1.zip`.
- **Needs Marwan's eye:** the fonts and sizes, the opening slate over the street while he walks, the tape labels on
  Maya's platform, every line of the new copy.

## Nov 14, 19:52-20:55 Cairo: step 4 pass 3, the set dressed for the film (not committed)

- **What changed:** the set is dressed for the look in `docs/ART_STYLE.md` §15.9 and §15.10, from text. A new
  `tools/dress_auction.py` runs at the end of `tools/build_auction.py` (and alone): three master materials
  (`/Game/Take/Dress`: lit set, unlit practicals, and `M_SignalPattern`, the unlit signal material with dashes, bands
  and hatching), 40 instances, every set mesh re-surfaced by what it was built with, its label and its zone (1,391
  of them), 312 small NoCollision dressing pieces (kerb, queue stanchions, press wall, bay trees, wainscot and
  cornice, chandelier, pilasters with sconces, side-aisle runners, the dais lip and footlights, pyro charges, the raw
  backs of the flats with battens, braces, weights, cables and work tubes, road cases, spike marks, catwalk tape,
  truss and lamp housings), the lights retuned per zone with a new `Back_Dais` rim, and the §15.10 tints. Gold and red
  left everything that does not carry meaning: picture frames, stanchions, bottles, instruments, banners, the rope,
  the curtain, extinguishers, cars, device rings. In C++: frame lines and the beam are dashed from the eye, frame
  fills banded outwards from the watcher, red tape hatched at 45 degrees, tally lights square, marks solid gold T
  plates (the disc under them fills with an action's progress instead of pulsing), heavies' bodies by `BodyTint`.
- **What the agents did:** one builder session: phase 1 staged the script, the C++ edits and the checks while passes 1
  and 2 held the editor; phase 2 applied them, rebuilt the map, shot every zone in play-in-editor, tuned by the §15.11
  numbers, and re-ran the scripted tests.
- **What went wrong:** the first build crashed the GPU on every editor start (device removed on frame 0): the device
  rings are an instanced mesh, and the unlit signal material had no instanced-mesh usage flag, which the editor tried
  to set while drawing. The rings went back to the lit material. The signal material is cached per editor session,
  so the dashes only appeared after a restart. Julian read dark under the top-down keys, so he now carries his own
  key on a lighting channel only he is on. The night skylight fills nothing (it captures a black sky), so the floor
  between the bench rows got two house lights over the seats. The reel's gold gel washed the end wall amber at the
  alarm: its haze went down.
- **Proof:** `docs/shots/step4_p3_pie_*.png` (street, lobby, floor, rope, dais both ways, backstage on Vex, platform
  and catwalk on Maya, the alarm); §15.11 numbers in the session report; the four scripted tests (same results and bloopers as pass 2); stat unit at pass 1's
  three views.
- **Needs Marwan's eye:** dashes and hatch as "camera frames" and "red tape"; the T marks; the hall's and
  backstage's darkness; the footlights and sconces; the house lights over the seats against the noir pools.

## Nov 14, about 21:45 Cairo: step 4, the HUD fixes after the critique

- **What changed:** the HUD is the same size on every screen. Windows scaling (125% on this PC) hands the game a
  smaller canvas than the window (1280 x 720 for a 1600 x 900 window); the HUD's scale had a 0.8 floor, so on that
  canvas everything drew 1.2x and the controls row and the grade stamp could leave the screen. The scale now has
  no such floor for the HUD, follows the screen's real pixels when the canvas reports a DPI scale, keeps the bars at
  least 138 px tall (ultra-wide screens crop the picture instead), and lifts the bottom rows if the controls would
  touch the edge. One controls legend: pressing Enter lights the bottom row instead of printing a second one. The
  goal line never repeats the Use prompt, every goal fits one line, the cast intros are one line in the director's
  voice, one target label at a time (the label rule's "four plus the target" now holds), pips sit under a device's
  name, labels keep the 48 px margin, slate values align under their captions, and no final full stops in the bars.
- **What went wrong:** a file-scope `using namespace` in the HUD's files leaked into the next file of Unreal's
  unity build and broke `Watcher.cpp` (C4459) once the tree was committed: the using-directives now sit inside each
  function. The first scale theory (the canvas's own DPI scale) was only half the story: the standalone run showed
  DPI 1.0 and a 720-line canvas, so the 0.8 floor was the real cause.
- **Proof:** the standalone game at 1920 x 1080 on this 125% display: composure line 800 px (x 560 to 1359), the
  controls row's lowest pixel at y 1065 of 1080 (`docs/shots/step4_fix_scale_1080.png`); in a 1600 x 900 window the
  line is 667 px from x 468, as at 1080 scaled (`step4_fix_scale_900_windowed.png`). PIE views: `step4_fix_*.png`.

## Nov 14, 21:48-22:30 Cairo: step 4 final run (pass 3 fix after the world critique, package, real keys)

- **What changed:** the world critique's fixes, all from text (`tools/dress_auction.py`, `tools/build_auction.py`,
  `tools/build_look.py`, small C++). The alarm no longer turns the stage amber: WARM keys (1.0, 0.90, 0.80), the
  reel's key white (1.0, 0.95, 0.90), and the alarm's saturation **1.0 instead of §15.3's 1.15** (agreed with the
  lead: gold must own the way out at the alarm); gold-hue share at the alarm 3.5% (case and side exit), was 14%.
  Julian's own key now rides on his camera boom (lighting channel 1, so it lights only him) and he is the lightest
  thing in the lobby too (219); the stair was pewter and is marble now, the marble darker, the chandelier lamp lower.
  The dais reads: stage deck 0.13 (deck 50), `Back_Dais` 2000 cd, `Fill_Dais` 1400. Marks are 1.4x bigger plates
  (`AMark::PlateScale`, visual only), 2x at the scanner and the desk. Tally lights are dark red when off (the grey ones
  turned violet under the red floor). Footlights are two low strips (a dotted line read as danger). The rope aisle's
  cases and the banner are darker, a cool wash lights the facade and the queue, idle device rings are dark blue,
  frames fade out within 2-3 m of the lens, LOT 41 is in paper. `/Game/Take/Dress` is in the cook list.
- **What the agents did:** the builder applied a staged patch script (33 exact patches, dry-run first), rebuilt,
  re-shot every zone, re-ran the checks and the tests, packaged and played the exe with real keys; a critic agent
  ranked the world picture before it; the HUD agent's fixes landed first.
- **What went wrong:** the first rebuild hung Julian's key off him instead of the camera boom (the 5.9 Python
  actor has no `get_root_component`; `root_component` works), caught in the log and fixed. The first package never
  started: a hidden PowerShell launched with nested quotes died silently and 7 minutes went before the old log's date
  gave it away; run directly, the package took 2 minutes. The lighter keys put the aisle pool at 131 (over 130):
  `KEY_CD` 2400 -> 2300. The 1.4x marks still read as slivers at the scanner and across the lobby, so those two
  marks are 2x. The hall floor's numbers cannot be projected in the Shipping exe, so they are from play-in-editor.
- **Proof:** the Shipping package played with real keys from the title card to the stats (3:25 wall clock; 3:02 on
  the band's clock, 17 cuts, 1 blooper at the beam on purpose, out by the stage trap, ROUGH CUT):
  `docs/shots/step4_01_title_card.png` to `step4_11_stats_grade.png`; before and after pairs
  `step4_before_*` / `step4_after_*` (queue, maya, floor); `docs/shots/step4_thumb_315x250.png`. Packaged samples:
  Julian 220, red frame (211, 48, 31) = `#D3301F`, gold mark (231, 183, 71) against `#E9B949`, making-of wall
  B - R 29; in play-in-editor: lit floor 128, floor between rows 49-65, heavies 31, black point 17, alarm gold-hue
  share 3.5% (was 14%). The four scripted tests end as before (ROUGH CUT, ROUGH CUT, DIRECTOR'S CUT, the failures)
  with the same deliberate bloopers. `stat unit` (Frame/Game/Draw/GPU ms) before step 4 -> now: queue
  22.85/22.83/3.90/8.53 -> 23.26/23.27/4.94/9.23, maya 22.81/22.78/4.03/8.75 -> 23.53/23.50/5.17/9.45, floor
  22.65/22.66/4.17/8.96 -> 23.53/23.50/5.04/10.23. Zip `TeamName_GameName.zip` 326 MB (the pass 2 zip is kept as
  `TeamName_GameName_step4p2.zip`).
- **Needs Marwan's eye:** the amber-free alarm (warm enough?), the 2x marks, the hall's and dais's darkness, the
  dashes and hatching in motion, the house lights over the seats.

## Nov 14 23:00 to Nov 15 00:35 Cairo: step 5 milestone, the cast and the effects (characters and props)
- **What changed:** every placeholder person is now a template mannequin in costume. `ACastMember` (new C++) builds
  each costume from the engine's basic shapes on the mannequin's bones: guests in dinner jackets, tails and top hats,
  gowns (Quinn) with buns, stoles and small hats; staff in waistcoats; guards and the doorman in peaked caps and
  epaulettes; Vance in a tall top hat with a cane; the bodyguard broad, bald, in dark glasses. Guests stand frozen in
  the idle at staggered times or sit (the pose bent at the hips and knees on a poseable copy). The band is a quintet
  on the riser: singer at a stand microphone, trumpet (from the pistol aim pose), upright bass, drums and piano, all
  in fedoras, under a dim key of their own. Watchers' mannequins walk on the template's blend space at the guard's
  speed; Julian, Maya and Vex wear a bow tie, headsets, a belt and a beanie. Everyone dips on the band's beat. Julian
  bows at The Bow (about 40 degrees from the waist, 0.9 s, input never taken). `ATakeFX` plays three Niagara systems
  duplicated from engine templates on the frame of the event: the pyro at its five charges, sparks at the PA when the
  sound desk lands, glass at a champagne cart. `tools/build_cast.py` places all of it (run by `build_auction.py`).
- **What the agents did:** one Opus agent wrote the kits, the poses, the bow and the effects, tuned them on screenshots
  and ran the scripted tests; it was stopped mid-run and a second agent resumed: saved, rebuilt the editor DLL from the
  files on disk (the last Live Coding patch was older than `CastMember.cpp`), restarted and re-shot in play.
- **What went wrong:** bodies that bob redrew every cached shadow each frame (1.4-3.3 ms of GPU), so guests and the
  band cast no shadow; posed copies refreshed their bones every frame (about 1 ms for 44 seated guests) until their
  tick was turned off.
- **Proof so far:** play-in-editor shots of the band, Vance and the pyro after the rebuild
  (`Reel/Saved/Screenshots/WindowsEditor/s5m_*.png`). Still open: the bob's tuning, the drums and piano from the
  floor, the pyro's size, the tests, the frame time and the package.

## Nov 14 22:45 to Nov 15 01:00 Cairo: running steps 5 and 6 in parallel, and how Marwan steered them
- **The set-up:** Marwan asked for steps 5 and 6 at once, with Opus 5.5 builders and a lean orchestrator session that
  only briefs, relays and hands over the editor. Step 5 owns the editor and `main`; step 6 builds audio outside the
  editor in its own git worktree (`.wt-audio`, branch `step6-audio`), compiling only the game target so it never
  touches the open editor; a third builder generates props with Rodin in `.wt-rodin`. The editor goes to one of them
  at a time: step 5, then the audio import, then the props.
- **Keys without a clipboard:** the PC and the Mac share no clipboard, and no agent may type a key. The orchestrator
  opened a masked PowerShell prompt on the PC's screen; Marwan typed each key into it from the Mac with Moonlight's
  Ctrl+Alt+Shift+V (it types the Mac's clipboard on the host), and it went straight into the user environment, never
  into the chat or a shell history. Done for ElevenLabs (Starter, checked against its API: 40,000 credits), Rodin
  and Tripo.
- **Marwan as the director, from his phone and Mac:** each builder sends pictures and audio as it goes, and his
  verdicts go back within minutes. The band loop: approved. The chief's stock voice: "the classical AI voice ... it
  ruins immersion", so he got three Voice Design candidates and picked one; then four more stock voices a listener
  would recognise (the doorman, Vance, the broadcast, security) were redesigned and he picked each. The bob on the
  beat: "jolting forward the whole model all together feels unnatural", sent back as upper-body motion, eased, out of
  unison, the band moving most. The goal for the night moved to the set: "make the environment rich rather than how
  barren it is right now"; rigged people only if there's time. His rule for generated assets: one prop first to check
  the quality before anything else.
- **What went wrong:** the builders started as subagents of the orchestrator, and every interruption of its turn
  killed them (four times in two hours; their files and commits survived, the time didn't). From 00:30 they run as
  separate Claude Code sessions, which survive it. Messages to a busy session wait for its turn to end, so urgent
  feedback (the bob, the one-prop rule) needed its turn stopped first. The one-prop rule still arrived late: Rodin's
  API was fast and seven props were already made (0.5 credits each); Marwan judged them good. Rodin's API refused its
  high tier ("no active subscription" for API use), so the props are Gen-2.5-Low.

## Nov 15, 00:35-02:00 Cairo: step 5, the band and the bob, then no bob at all (Marwan's call)
- **What changed:** the band now reads from the hall floor. The drums' and the piano's floor pieces had been placed
  from the seated body's origin, 86 cm under the seat, so the stool, bass drum, snare, bench and most of the piano sat
  inside the riser; floor pieces now stand on the actor, never moving with the body. The quintet is laid out across
  the riser as the floor sees it (drums at the back facing the room, the upright against the east edge with the
  pianist's back to the room), with a cool rim light behind it and lighter keys. The pyro lost its burst at the aisle
  (no charge stands there; it was 30 cm from the dais mark), so no burst covers a mark on Julian's paths.
- **The bob, twice, then gone.** Marwan watched the first version: "all the characters are moving jarringly with the
  beat like jolting forward the whole model all together, feels unnatural and weird". It was rebuilt from the upper
  body only (spine, neck, head on smooth curves, each player by instrument, a third of the guests). Then his call:
  "remove the beat thing its dumb just make it like a stealth hitman game normal and use the band as warnings and
  stuff like that its fine and cool". Built: nothing moves with the beat. Standing guests, staff and the standing band
  play the template idle looped, each from its own start and at its own rate (0.9-1.1), so no two breathe together;
  seated guests and the drums and piano are still; the groove code is deleted. The pyro's flares lost their beat
  swell too (their own wobble stays; the flare light is steady). The band stays the clock and the warnings.
- **What the agents did:** one Opus session rebuilt the band, the groove and then the idle, shooting frame bursts
  across beats and diffing them to show what moved; the orchestrator relayed Marwan's notes.
- **What went wrong:** play-in-editor sessions were torn down a second after starting several times, and the editor
  was quit at 01:33 Cairo, neither through MCP (someone at the PC, or another session); two scripted runs and the
  first t_fail retry were lost to it.
- **Proof so far:** after the idle change the scripted tests end as in step 4: t_full ROUGH CUT by the trap (bar
  82.4), t_s3 ROUGH CUT, t_s3b DIRECTOR'S CUT by the side exit with The Bow (logs `C:\Jam\Builds\step5_t_*.log`);
  t_fail still to re-run. Idle check: `C:\Jam\Builds\s5\idle_band_strip.png`, `idle_queue_close.png`,
  `idle_queue_moved.png`.
## Nov 15, about 00:00 Cairo: step 6, audio, phase 1 (a parallel session outside the editor)

- **What changed:** a branch `step6-audio` in a worktree (`.wt-audio`), so the step 5 session keeps the editor.
  `docs/AUDIO_SCRIPT.md` (48 voice lines, 18 sound effects, 7 music cues, each with its event, file and subtitle),
  generated from `tools/audio/manifest.py`, which also writes the C++ voice table. `tools/audio/`: the ElevenLabs
  generator (resumable, with a dry run that counts credits), a numpy stand-in synth for every sound effect and the
  band, the processing step (decode, trim, earpiece filter, tempo measurement and fit to 120 BPM, loop seam,
  BS.1770 loudness, a peak limiter) and the editor import script for phase 2. C++: `TakeAudio` on the director, the
  bar clock now following the music's playback position, one-line calls at the event sites, `FireSound` on devices.
  The game target compiles in the worktree (no editor build, no assets touched).
- **What the agents did:** one Claude Opus 5.5 subagent wrote the script, the tools and the C++, generated the voices
  first (ElevenLabs multilingual v2, eleven premade voices), sent one line per character and the first band loop to
  Marwan through the orchestrator, then the sound effects. The first generated bed measured 120.01 BPM.
- **What went wrong:** no Python on the PC outside the engine, so a venv was made from the engine's Python 3.11;
  scipy and pyloudnorm would not download (pip read timeouts), so resampling, filtering and loudness were written in
  numpy (a 1 kHz sine at -20 dBFS reads -23.0 LUFS, as BS.1770 says). Three ElevenLabs requests timed out and were
  retried. The account's credit counter lags by minutes, so the run log keeps its own estimate.

## Nov 15, about 00:40 Cairo: step 6, audio, phase 1 resumed (designed voices, warning phrases retaken)

- **What changed:** Marwan picked Voice Design candidates for Vance, the broadcast producer and the alarm guards
  (the chief's was picked earlier); they are saved as voices on the account and their 14 lines regenerated,
  including the chief's "No match" line, which the stopped run had left in the old premade voice. A pause cap in
  `process.py` (pauses over 0.6 s inside a line become 0.4 s) and a per-voice speed in the manifest. The bass and
  trumpet warning phrases were retaken. The doorman's two Voice Design candidates went to Marwan.
- **What the agents did:** one Claude Opus 5.5 session, resuming a stopped one, checked the files by measurement
  rather than by ear: a 10 ms level envelope, onset times against the beat grid and a pitch track for each phrase.
- **What went wrong:** the first bass and trumpet takes passed the loudness checks but were each one held note: the
  checks measured level, not rhythm. A 5 s Music request ignored "then silence"; a 10 s request for a repeating
  figure gave a clean bass bar with attacks every 0.50 s. The music model gave the muted trumpet as held notes three
  times, so the trumpet came from the sound-effects endpoint. Vance's designed voice ran his speeches to 10 s, past
  his 4 bars at the mic; 10% speed, steadier settings and the pause cap brought the longest to 7.45 s.

## Nov 15, about 01:30 Cairo: step 6, nothing on screen keeps the beat

- **What changed:** Marwan's call, relayed by the orchestrator: "make it like a stealth hitman game normal and use
  the band as warnings". Every beat pulse is gone from the screen: the watchers' frames and tallies, the beam, the
  sound desk, the danger burn, the HUD's glyphs, the alarm's blinking last bar and the title card's keycap are
  steady. The HUD's beat squares and bar number became the take's running time; counts are in seconds ("arrives in
  4 s", a 16-second alarm, the blooper reel's "AT 0:24"). The metronome click is removed. The bar clock, its events,
  every gameplay timing and the band's warning phrases are unchanged.
- **What the agents did:** one Claude Opus 5.5 session edited the C++ in the step 6 worktree and compiled the game
  target there; step 5 kept the editor.
- **What went wrong:** a missing include broke the first build. A session-wide interrupt stopped the work midway;
  the retake of Vex's desk hint ("for four seconds") is held until Marwan says go, so the hint still says "two bars".
  An earlier docs edit had turned a Windows path's `\r` and `\b` into control characters in STATUS.md; fixed.

## Nov 15, about 02:00-03:15 Cairo: step 6 phase 2, the audio in the game (merge, import, tests, package)

- **What changed:** `step6-audio` merged into main (the doc conflicts were two additions at the same place: both
  kept). The clock's beat pulse removed now that the pyro's flare no longer reads it. The 73 sounds imported into
  `/Game/Take/Audio` (the bed and heist cue inline, the loops looping). The bed plays from the take's first frame
  and the clock follows it. A clock fix: see below. Packaged (Shipping, 334 MB zip).
- **What the agents did:** one Claude Opus 5.5 session drove the editor through MCP: the import script, a band
  check in play-in-editor, the four scripted tests (twice), `stat unit` at three views, then closed the editor,
  packaged, played the exe with real keys and rebuilt the editor DLL with the fix.
- **What went wrong:** the first test round showed the clock reading the last playback reports of the sound it had
  just stopped (the bed, as the alarm cut to the heist cue or before a restore restarted it) as the new music's
  position: a burst of reseeks within a few frames, then "the clock stops following the music this take", leaving
  the bed out of phase with the warning phrases. A first fix (only the current sound, 0.2 s settle) cured most of
  it; a second (0.5 s settle, and a report must have moved) cured the rest. Each music start still reads about
  0.6 s late once and is corrected in the first bar: for Marwan's ear. Two tests lost a race with the new take
  (Julian not yet on set 2.5 s after it; t_fail had in step 5 too): they now wait for him. Marwan used the PC
  during the real-keys run, so it stopped after Scene 1. Two old docs edits had turned Windows paths' `\v`, `\r`
  and `\b` into control characters (CREDITS, STATUS): fixed.

## Nov 15, about 03:30-04:45 Cairo: step 6, the sight frames on demand (Marwan's call)

- **What changed:** Marwan: the red sight lines of the guards and drones are "too distracting and ugly"; they should be
  "very subtle and indicated smartly from the environment not to ruin immersion or removed". A `FrameDisplay` switch on
  the director (Never / Planning / Always, default Planning). In Planning a frame is hidden at rest, faint and outline
  only while a crew member stands in a device's ring that would catch it (or the follow spot aims at it), and solid red
  while it holds Julian. The beam's laser stays. The drone got a searchlight pool on the floor. Two objectives that
  pointed at frames were reworded. The packaged exe was also played with real keys from the title card to PRINT IT
  (ROUGH CUT, 3:04, 17 cuts, 1 blooper), before this change.
- **What the agents did:** one Claude Opus 5.5 session wrote the C++ and `tools/build_frames.py` (the faint material
  is the pattern material's own lines from `dress_auction.py`, dithered so it still writes the red stencil), rebuilt,
  shot the four views and checked the modes from the components.
- **What went wrong:** the brief assumed mark hovering and a dotted path, gone since the Nov 13 rework (Julian walks):
  "planning" became a crew member in a device's ring. UE 5.9 has no Python class for DitherTemporalAA (it is a material
  function) and returns no input node from a duplicated material, so the faint material is rebuilt from source. The
  pool sits low in camera A's composed view, so it was measured by diffing the same frozen frame with it on and off.
  Marwan asked for less testing: "stop with the obsessive testing, just get work done and I will QA in the end".

## Nov 15, after the sight frames (PC clock 20:00-21:30): the Rodin props and the procedural surfaces, phase 2

- **What changed:** the grey boxes got real furniture. 23 props generated with Hyper3D Rodin (text to 3D, 0.5 credits
  each, 11.5 credits in all) are imported into `/Game/Take/Props` and placed over the boxes they stand for: the podium,
  the case's pedestal and reel, the lobby chandelier, a grand piano on the band's riser, the champagne carts' and the
  sound desk's bodies, and the repeats (144 hall chairs over the 12 benches, 12 fluted columns, 36 picture frames, 27
  stanchions, cocktail tables, the bar, sofas, crates, road cases, cloak rails, film cameras, light stands, busts and
  urns on the plinths). Every box keeps its collision and goes invisible, so gameplay, sight lines and navigation are
  what they were; the props ignore Visibility (the watchers' line traces) and Camera, the heroes and raised pieces have
  no collision, and the floor furniture blocks only the pawn. The set's surfaces got procedural value patterns in the
  M_Dress material (parquet, marble veins, a damask on the front-of-house walls, the aisle runners, plywood, concrete),
  with their averages held so the §15.11 floor numbers stand. Marwan chose procedural over texture packs.
- **What the agents did:** one Claude Opus 5.5 session wrote the Rodin client and prompts outside the editor (a quality
  sample first, Marwan's go, then the rest), the import and placement script (`tools/rodin/import_props.py`: each mesh
  fit to its box, repeats on instanced components, Nanite on the repeats, one material whose texture is remapped into
  the set's value band), and the surface patterns; then in the editor merged the branch, imported, placed, shot the
  rooms before and after with stat unit, and ran the full scripted play test.
- **What went wrong:** the Rodin API only allows its low tier on this account; Tripo's API wallet was empty although the
  web plan had credits, and PBRgen is personal use only, so both were dropped. The lineup row west of the street sits in
  an unlit void and showed nothing: facings were read off the lit street and the bust's face instead (glTF's front comes
  in as +X), except the grand piano, which Rodin drew side-on: a top-down shot showed its keyboard sideways, so it is
  turned 90 degrees. The full scripted play test then ran to PRINTED with no blooper and every walk on time (the queue,
  the gate, the floor past the cocktail tables, the rope, the dais, the lift, the winch, the trap). After the props GPU time read 2-3 ms higher in every room, backstage too; hiding every prop or switching
  the patterns off in the same session changed nothing, so it is the machine warming up between runs, not the props. The
  prop textures were still taken down one mip (VRAM about 0.4 GB more before). The device bodies are recoloured flat at
  runtime, so the carts and the desk show the Rodin shape without its texture. The bar came out as a round ribbed block.

## Sat Nov 14, 20:45 - Sun Nov 15, 01:40 (Cairo): the QA pass

- **What changed:** Marwan played the build and wrote 22 QA notes (the one-line queue, slow time for the tutorial, all
  animations, every person different, no sight cones, a smart trick for the reel, a minimal HUD, colour allowed, the
  repeating trumpet, props that read as real, rooms with a purpose, full models for the characters). The notes were
  split into packages (below); the work is written and committed (local only, nothing pushed) and partly in the level. Nothing new
  has been seen running yet (see "How it was checked").
- **How the agents were used:** an orchestrator session (Claude Fable) split the notes into packages and ran 13 Claude
  Opus agents in parallel plus an integrator. Each agent owned its own files (the table is in
  `C:\Jam\Builds\qa\COMMON.md`), and the packages talked through written contracts (function names and switches the
  next package could rely on). One Unreal editor was shared through a lock file (`tools/editor_lock.py`). Level
  changes were written as re-runnable overlay scripts (`tools/sets/*.py`, run by `tools/sets/run_all.py`) so a rebuild
  could replay them. Each agent kept a progress log, so a stopped agent could be replaced. Most packages got a switch
  to turn their change off.
- **The packages:**
  - **found** (enabler): `tools/sets/common.py` (clear, spawn, scatter, fit, place, shoot) and `run_all.py`; a survey of
    the whole set (20 plans, 50 lit and 50 unlit shots). Found the causes of notes 13 and 15: 22 glasses floating 16 cm
    over the Rodin cocktail tables, rope bars higher than the stanchion tops, the "cone chairs" were the skirt cones on
    seated women, and 22 of 36 guests sat off their chairs.
  - **scene1** (notes 1, 2, 10): a moving queue, Julian's turn at the scanner and its refusal, the first switch taught
    with slow time and a camera glance to Maya (`QueueLine`, `TakePlayerController`, `TakeDirectorScene`). Flight now
    runs on real time.
  - **time** (notes 9, 2): turn warnings only play when their watcher is in range, with a cooldown, 8 new band variants
    (bass, trumpet, drum roll, hi-hat, two each) and small level, pitch and timing jitter, so no phrase repeats twice
    running. The slow-time mechanism holds the music (low-passed, ducked) and rejoins it on a beat. 340 ElevenLabs
    credits used.
  - **sight** (notes 5, 18, 19): the red frames, T marks and tape lines are replaced by light: a torch pool on each
    guard that swings onto Julian when he is held, haze and a lens glint on the drone, red and green lamps on the
    scanner, warm pools on the marks. The old frames stay behind a switch (`bDrawnSightOverlays`).
  - **hud** (note 7): a minimal HUD with one prompt lane and a goal line that fades. The composure meter is gone;
    stress shows as a screen wash, a closing vignette and a pulse, with a side glow toward the watcher who holds him
    (`TakeHUDStress.cpp`).
  - **look** (note 8): the film filter keeps colour (saturation 0.62, a split tone, a muted period palette: walnut,
    burgundy, bottle green, navy, cream, brass) and stays grainy; `FilmColour 0` brings the old monochrome back. Seen
    in the editor viewport.
  - **heist** (note 6): "the preview". After the keycard Maya throws a house dimmer, the hall goes dark, a projector
    lights the screen and Vance announces "a first glimpse of Lot 41". The room looks at it for 12 s while Julian swaps
    the reel for a dummy. Lights up mid-swap is a blooper ("THE ROOM SAW HIM"); Vance finding the newsreel starts the
    old alarm. 8 new lines. `bHeistTrick` turns it off.
  - **cast** (notes 3, 4): a C++ animation instance for the whole cast (mocap clips when present, else the same acts
    built from bones), seated guests fitted to the hall's seat list, a band built from bones, a lap-and-drape for
    seated gowns instead of the cone, and variety (no two neighbours share kit and colour; heights, skin, hair, hats,
    glasses). `Reel.Cast.Acting 0` freezes it as before.
  - **heroes** (note 21): Julian, Maya and Vex as Rodin models skinned to the template mannequin's skeleton with
    Geometry Script, so the template's clips play with no retarget (`tools/build_heroes.py`). Posed through the skin
    weights and looked at from four views.
  - **hall** (notes 12, 15, 16, 22): 141 gilt-and-burgundy chairs facing the dais, a bandstand (grand piano, drums,
    upright bass, mic, trumpet stand), a TV riser with a period studio camera, monitor and ON AIR sign, a rebuilt bar,
    a phone desk and the lots table; the old shape piano and the mid-air paddles are gone.
  - **front** (notes 13, 11): the street (brass stanchions with sagging velvet, seven sedans in six paints, street
    lamps, the marquee, press pen, newsstand, hydrant) and the lobby (reception desk, seating group, ten palms, the
    stair, swags, signs); glasses set on the real table tops.
  - **rooms** (notes 14, 11, 20): all 16 side rooms designed with a one-line story each (about 2,400 pieces in 180
    instanced actors). Note 20's stacked film cans became a working projection booth; the stretched crates became mixed
    sizes. The dock lorry boxes became a Rodin lorry.
  - **devices** (notes 17, 10): every interactive device got a model slot, a pilot lamp and a warm pool, and a brass
    lever, crank or rope that moves on the press; eight Rodin models (searchlight, follow spot, hand winch, pyro board,
    ticket gate, dimmer, rope-lock rail, fresnel); the follow spot's drawn cylinder became a haze shaft. Seen in
    stand-in mode only.
  - **mocap** (note 3): 28 Mixamo clips and one skinned character (Y Bot, used only as the retarget source) were
    downloaded through the Claude in Chrome extension after Marwan signed in to Mixamo. The import script
    (`tools/build_mocap.py`) and the clip list (`tools/mocap_clips.json`) are written; the clips are not imported yet.
- **How it was checked:** the C++ was written blind: with one shared tree nobody could compile while the others were
  editing. Three compile checks of the game target, run with the editor still open, found the errors: cast's missing
  `WalkTo`, `FaceYaw`, `IsWalking` and `PlayGesture` that scene1, sight and heist already called, a name clash in the
  HUD (`Ink` hid a global), and cast's struct `FFrame` clashing with the engine's script frame type. After the fixes
  the game target linked and the editor target built clean in 61 s. State at 01:40: the level rebuild and the first
  look in play are in progress; nothing new has been seen running yet. The look, front, hall, rooms, devices and
  heroes work was seen in editor shots only.
- **What went wrong:**
  - **look:** scene captures skip the film grade, so its shots were taken from the editor viewport. The old cool rim
    lights made the colour hall a blue night.
  - **time:** warnings fired every cycle in 2D because the gate lived in the director's bar-line loop. Two C++ writes
    through a shell heredoc failed on apostrophes.
  - **found:** the exposure override is ignored by scene captures. A `Prop_*` name prefix collision made import_props'
    unplace delete the prop store's contents without a message.
  - **heist:** written blind in wave 1; the scripted tests must now walk Maya to the dimmer first (or set
    `bHeistTrick` off).
  - **scene1:** written against contracts, not seen; the new lines are unvoiced until time's second pass; the guests'
    blockers are untested.
  - **devices:** the outdoor lineup shots came out black (re-shot unlit); the follow spot faced sideways and was sized
    by its handle; the projector prop was refused twice by the permission system.
  - **front:** import_props shrank the Rodin stanchion to 65 cm, under ropes at 76-82 cm; the Rodin sofa came in
    facing +Y; the first rerun was not repeatable (posts read from instances already removed); a 9-minute wait for the
    editor lock ran out; a helper lost an argument mid-run.
  - **hall:** the "upside-down cone chairs" were seated women's skirt cones; the chairs sat sideways because Rodin's
    front is +Y; removing two boxes would have renumbered later labels, so the counter steps on.
  - **heroes:** the editor never ticks animation on placed actors, so test poses were skinned in Python from the
    template clips. Inpainted weights alone left fingertips floating and smeared Vex; fix: keep them only where they
    agree with the closest-point bone.
  - **rooms:** a test light overexposed the lineup shot; lit shots were black, so rooms were judged unlit; the Rodin
    desk came out small because its bounds include a tall lamp; the prop store was empty for the same `Prop_*` reason.
  - **cast:** a mid-task switch to mocap-first; `Skirt()` cones read as chairs; `FFrame` clashed with the engine's.
  - **Dates:** earlier dev-log entries dated "Nov 15" before Sat Nov 14, 21:00 are labelled 7 hours ahead. Git Bash
    prints the PC's time zone as "EST" (Egypt Standard Time); earlier sessions read it as US Eastern and added 7 hours.
    The PC clock was Cairo time all along.
  - **Dark captures:** scene captures taken by agents come out near black because they ignore the game's exposure, so
    several agents judged their work from unlit shots.
  - **Rodin permission:** a Rodin generation for a film projector was refused by the permission system for a subagent
    even after Marwan approved it; solved by reusing the projector the rooms agent had already generated.
  - **Usage limit:** the session hit the account's usage limit at about 21:50 on Saturday and stopped for about four
    hours with the integration half done (editor built, level not rebuilt).
  - **Mixamo:** computer use could not drive Chrome (browsers are read-only for it), so the Claude in Chrome extension
    was used instead.

## Sun Nov 15, 01:35 - 03:40 (Cairo): second feedback, wave 3, four integration rounds

- **Marwan's second feedback (01:35), from the build the set agents had saved before the integration:** (1) seated
  guests hover above the chairs and a grey box pokes out under one; (2) the band ignores the real props: the old box
  piano is still there, the bassist stands off the bass, the pianist is not at the keys, the drummer is not on the kit;
  (3) the follow spot is rigid and its ray goes through solid objects; the player can skip the queue; the line "who
  shined the light" comes too late and off-screen, with nobody standing at the detector; (4) in the lobby it is unclear
  who talks, who to greet and what to do, and the bidding desk sits off to the side; (5) the lobby's picture frames are
  empty flat colours and clip into the wallpaper; (6) the hall has a huge gap between the rows and the screen, and he
  can walk up to the reel because the guards are distracted: the guests should see him too, an old film should play on
  the screen, and the player should have to use the Cut (his reference: the "Lights Out" chapter of Uncharted 4);
  (7) the balconies and terraces are far too low. At 02:25 he added: Julian's arms hang out from his body.
- **What changed:**
  - **The hall is an auction in progress:** rows of chairs from the dais back toward the lobby doors with a centre
    aisle, a dressed dais (podium, the case on a runner, a LOT 41 sign), the bar, the band and a TV riser behind or
    beside the rows, a standing-room rank and cocktail tables at the reception. Everything over the hall became a
    catwalk level hung from the roof at 9 m (open rails, no legs), reached through a closed projection box over the
    lobby doors.
  - **The heist:** in house light the front floor and the dais are in view of the room. After 1 s the nearest guests
    turn and point and a murmur plays; at 2.5 s it is the blooper "THE ROOM SAW HIM". The only way across is the dark:
    Maya, on the catwalk, throws the house dimmer, the old film plays on the screen, the guests look at it, the hall
    goes to 8% for 16 s, and Julian swaps the reel in the dark. Vance finding the dummy starts the alarm; the getaway is
    unchanged. Vex gets one beat, at the winch.
  - **The opening:** Julian waits in the queue and an invisible wall holds him behind the guest ahead ("Wait your
    turn, pal"). A doorman stands by the detector, in shot, and refuses him; the follow-up is the slow-time aside, the
    glance to Maya, the searchlight, and the doorman turning to the light with "Who turned that light on?!".
  - **The lobby:** the desk and paddle board moved onto the way in; a clerk turns, raises a hand and greets; the camera
    eases to him once; one goal line.
  - **Cast:** seated guests fitted to their chairs (77 of 77 checked), the band at the real props, 28 Mixamo clips
    retargeted onto the mannequin for the seated guests and the band, Julian's arms in at his sides.
  - **Pictures:** 16 pictures rendered from our own sets and cast, put into 41 frames, and an old film material for the
    screen. **Lighting:** a pass over the street, lobby, side rooms and hall (about 67 new lights), brighter guard
    torch and drone searchlight. **Follow spot:** it swings on a spring and its beam is traced and stops at what it
    hits. **Voices:** 22 new lines and the room's murmur, voiced and imported.
- **How the agents were run:** the orchestrator (Claude Fable) only planned, relayed and committed. Seven Claude Opus
  agents at high effort each owned their files: stage (hall layout, catwalk), heist2 (the room watch, dimmer, film,
  the Cut), opening (queue, doorman, lobby), cast2 (seats, band, arms), spot (follow spot and devices), pictures,
  and a lighting agent; the earlier time and mocap agents finished their parts. One integrator agent did all the editor
  builds and level rebuilds, in rounds A to D (about 01:55, 02:25, 03:08, 03:32). An agent got into a round by writing
  `ready\<agent>.txt` after a clean game-target compile check (run with the editor open) and a clean re-run of its
  level script; the editor was shared through the lock file. Agents judged their work in play, not in captures.
- **Results:** round D at 03:32, editor target built in 50 s, level rebuilt (223 guests, 132 seated). `t_s3b` (the heist)
  and `t_full` both reach PRINT IT: `t_s3b` "exit=SIDE grade=DIRECTOR'S CUT time=46.7 cuts=11 bloopers=2"; `t_full`
  "exit=TRAP grade=ROUGH CUT time=142.8 cuts=14 bloopers=0" (queue, gate, desk, floor, Vex at the winch, the trap).
  Standing at the front-row mark for 20 s raised no blooper. The follow spot fired at a guard from Maya's post blinded
  him for 4 s. No missing assets in the play log. Nobody has yet listened to the new voices at normal speed.
- **What went wrong:**
  - Scene captures come out near black here, so everything had to be judged in play; the lit interiors and the dark
    dais were first read from unlit shots.
  - `build_auction.py` ended by running the overlay scripts before `import_props` placed the props, which undid the
    front's stanchion removals. The last line was dropped and the order is now written down.
  - Round A crashed the editor in `build_cast.py`: `ACastMember::FitSeat` indexed an empty pose array from
    `OnConstruction`. The integrator made it return early and fit the seat in play.
  - The follow-spot agent's marker came after round B was built, so round B had its code as it stood at 02:11.
  - The follow-spot beam stopped at an invisible collision wall along the catwalk rail (z 900-1060), not at the guard;
    fixed by skipping the first 1.5 m of the trace.
  - The integrator's trimmed heist script skipped Vex's walk to the winch in round C, so the trap never opened and
    PRINT IT was not reached until round D.
  - Julian's "arms out" was not the skeleton (the bind matched the mannequin within 2 degrees): the wide jacket and the
    rigid open hands read as held out. The fix is a rest-arms layer on the hero meshes. The grey camouflage patches on
    his suit were the mannequin's tint materials still painted over the hero mesh.
  - In round C the guard saw Julian at the place the goal line sent him (the front-row place) on bar 9, every time,
    with composure at zero by bar 10. In round D he stood at the front-row mark for 20 s with no blooper (the guard
    spotted him once during the warp in); the cause was not isolated.
  - In round C the dais stayed visibly lit when the house went to 8%. The lighting agent made its dais lights plain
    lights that the dimmer takes, and in round D the dais was dark ("72 house lights to 8%").
  - The old film played sideways on the screen (stills rotated 90 degrees, two copies with a frame line between). It
    was being fixed at the time of writing.
  - After the first re-stage the rear rows were mostly empty chairs; the next pass cut the chairs from 560 to 142
    instead of adding guests, and moved the TV riser off the centre line.
  - Two lines flashed in the same frame in the lobby, a key pressed during a cut flight was dead, and cutting back
    without firing the light moved the tutorial on; opening fixed these in its round.
  - The doorman at first reacted to the beam's position mid-swing, 44 m away; the fix (react to where it settles) waits
    for the next build.
  - The orchestrator's time estimates for the rounds ran ahead of the clock.

## Sun Nov 15, 03:40 - 04:12 (Cairo): rounds E and F, the package

- **First package candidate (03:34-03:38):** a ship agent built the Shipping package from the round D tree: BUILD
  SUCCESSFUL in 210 s, no cook errors or warnings, `C:\Jam\Builds\TeamName_GameName.zip` 562 MiB without the .pdb (the
  old zip kept as `TeamName_GameName_preqa.zip`); the exe stayed alive 25 s in a smoke test. It shipped with the
  sideways film, as a safety copy. The commits were pushed as a backup (8804f4b..3aceae9, LFS 617 MB).
- **Round E (03:50):** both targets built (the editor in 10 s), level rebuilt, saved, no source edits. The film no longer
  sideways or doubled, but upside down. The dais's gold marks no longer show in house light; they appear with the first
  dark. `t_s3b` and `t_full` pass (`t_full`: TRAP, ROUGH CUT, 134.1 s, 0 bloopers).
- **The heist agent got the editor to close its own loop:** it rolled the placed dimmer actor 180 degrees
  (`screen_picture_roll`, also in its `build_auction.py` block) and looked at the leader: the "5" stands upright.
- **Round F (04:08):** both targets built, level rebuilt, saved. After the leader the film is one upright frame with two
  figures the right way up. `t_s3b`: exit SIDE, DIRECTOR'S CUT, 46.6 s, 11 cuts, 2 bloopers. `t_full`: exit TRAP,
  ROUGH CUT, 134.2 s, 14 cuts, 0 bloopers. The opening agent's last fixes (doorman at the detector post, aim height
  130, the clerk's greeting waits for the street's lines) were in.
- **The final package (04:08-04:10):** BUILD SUCCESSFUL, no cook errors or warnings,
  `C:\Jam\Builds\TeamName_GameName.zip`, 562 MiB without the .pdb; the exe was alive after 25 s in a smoke test.
  Nobody has played the packaged build.
- **What went wrong:**
  - The film fix took three tries (sideways, upside down, upright): the first two were written without a look in play;
    only the third, with the editor open, was checked on the screen.
  - The searchlight pool still lands on Julian in the queue, 3.25 m from the doorman (who stands beyond the gate on
    the red carpet), after three aim changes. Left as a known issue for Marwan.
  - `t_s3b`'s second blooper is COMPOSURE_ZERO, not the one the test meant (the drone seeing Julian, which the test
    tried to avoid by hiding him behind the PA); left as it is.
  - The doorman's subtitle is still up when Julian reaches the lobby door; the clerk's greeting follows 1.2 s later.

## Nov 15, 11:20: the drone gets a body and a hum

- **What changed:** the broadcast drone was a flat black box. At BeginPlay it now builds a quadcopter out of basic
  shapes (`DroneWatcher.cpp`, `TakeDroneLocal::BuildSkin`): a rounded hull and canopy, four diagonal arms with motors
  and two-blade rotors, a camera gimbal with the lens in it, skids, and red/green nav lights. Every frame
  (`UpdateSkin`, from `UpdateWorldCues`) the rotors spin (neighbours opposite ways, faster with speed), the body banks
  into its flight and turns to face it (or Julian while it holds him), bobs and sways, and reels while blinded; the
  nav lights blink while it can see. A quiet looping hum (`sfx_drone_hum`, made here by `tools/audio/drone_hum.py`:
  four detuned motor tones and a breathy rotor wash, a seamless 4 s loop) plays from it, spatial, with its pitch
  rising with speed.
- **How:** one Claude session, with three other sessions working in the same editor. The skin lives only in the
  .cpp (its state in a file-local map, its tunables as constants) so it needs no header change. The plan was Live
  Coding, but other sessions had unbuilt header changes in the tree, and Live Coding would have pulled those into the
  shared editor too. So the change was compile-checked in a throwaway worktree instead (`Build.bat ...
  -NoHotReloadFromIDE`: UBT otherwise refuses any build while an editor has Live Coding on). The hum asset was
  imported under the editor lock. It shows in play after the next editor rebuild. The constants should become
  `UPROPERTY`s at the next header change.
- **What went wrong:** nothing was seen in play yet: look, feel and loudness are unverified.

## Sun Nov 15, 11:00-12:45 Cairo: set life, rehearsed ambient events

- **What changed:** the set now has small rehearsed scenes between the guests, in the manner of the party levels
  in Hitman and Dishonored and RDR2's ambient scenarios, so something is happening near Julian most of the time. A
  stage manager (`USetLifeSubsystem`, `Take/SetLife/`) starts itself in every game world. It keeps up to three
  pieces running in Julian's or the camera's zone, claims guests and staff for them, and walks everyone home
  afterwards. It holds during the title card, bloopers, slow time and the teaching beat, and ends in the heist's dark
  and at the alarm. Short overheard lines show as captions near the speaker when Julian is within 7 m. Seven pieces:
  the spilled drink, the toast, waiter rounds, the rival bidders (seated rows), whispers about Julian (lobby and
  hall; flavour only, never a sighting), the reunion (lobby) and the society photographer (street, flashbulbs).
  `Reel.SetLife 0` turns it all off. Design, API and the 66 bark lines with sound ids are in `docs/SET_LIFE.md`.
- **How:** one Claude session as coordinator. One Opus subagent built the framework and the first piece, then six
  Opus subagents wrote one piece each in parallel, each in its own new file, and compile-checked the game target
  only. Five other sessions were working in the same tree, so nothing existing was edited and no level change was
  needed. The editor rebuild was shared with the props session's restart, arranged by messages between sessions
  and the editor lock.
- **Seen in play (one PIE run):** the reunion, rival bidders, toast and waiter rounds each started, spoke their
  lines and ended "ok"; the waiter visibly crossed the hall.
- **What went wrong / open:** the photographer did not start in about 16 s on the street, and the spilled drink and
  whispers did not come up in the run. The reunion ran 58 s against a 60 s cap. The hall has one Staff member (the
  bartender), so three pieces share him. Lines are captions only (nothing voiced). Walks are straight lines with no
  pathfinding.

## Nov 15, 11:00-12:45 Cairo: every NPC gets a face of their own
- **What changed:** Marwan asked for every NPC to look unique, with the most care on the people the player meets.
  Nine featured NPCs are now Rodin characters skinned to the mannequin skeleton with build_heroes.py (new `NPCS` list,
  assets in `/Game/Take/Cast/NPCs`). They are Vance, the doorman, the clerk, the bodyguard, the chief, the aisle guard,
  the queue guest ahead of Julian, the singer and the bartender. The band, the security men, the cloakroom attendant and
  the other seven queue guests got hand-picked kits, tints, faces and scales. The ~220 guests each get an explicit
  look (`FCastLook`, decided in build_cast.py section 1). A new body material (`M_CrowdBody`) paints skin, shirt fronts,
  cuffs, gloves, necklines, hair and faces onto the mannequin. Shape pieces add hats, hair volume, glasses, neckwear and
  jewellery. Report: 220 guests, 220 distinct looks, 0 neighbour pairs differing in fewer than 3 traits readable at a
  distance.
- **What the agents did:** the lead session split the work into two parallel Claude Opus 5.5 (high effort) agents: one
  for the featured NPCs (Rodin prompts, skinning, hook-ups), one for the crowd (C++ looks, material, palettes).
  Five other sessions were also driving the one editor through `tools/editor_lock.py`. The lead session brokered the lock order and the
  shared editor restart between them (props, set life, hand animations, hall puzzle).
- **What went wrong:** Rodin had 5 credits, enough for 9 people, so 0.5 is left. The hand-animation session's REAL_HANDS
  first failed in 5.9 (`get_triangle_material_id` is missing), so the NPCs were built without it; that session is
  rebuilding them with the working version. The crowd agent's material script crashed the editor once at 12:28 (a
  null class in NewObject); it was rebuilt and restarted in 3 min. Two late CastMember fixes went in by Live Coding
  only: run `Build.bat ReelEditor` before the next editor restart. Faces on the crowd are painted on the mannequin head and read
  as stylised up close.

## Nov 15, about 11:00-13:00 Cairo: real hands for the heroes and the featured NPCs
- **What changed:** Marwan's screenshot showed Julian's hands as stiff, splayed slabs. The Rodin models' fingers had
  been folded into the hand bone, so no clip could curl them. `tools/build_heroes.py` (VERSION 5) now cuts each Rodin
  hand off at the wrist and puts in the mannequin's own rigged hand. Its colour comes from the Rodin wrist skin, so
  gloved models keep their gloves: white for Vance, Doorman and Singer, black for Bodyguard, brown work gloves for Maya.
  Two arm fixes came with it. The mannequin's bind pose bends the elbow forward while the Rodin arms hang straight,
  so every bent elbow left the sleeve behind and the hand out of its cuff. Each forearm is now turned onto the
  mannequin's and its wrist set on the mannequin's (`FOREARM_ALIGN`), and the sleeves are weighted again along
  shoulder-elbow-wrist (`ARM_REBONE`). Rebuilt: Julian, Maya, Vex and the nine featured NPCs.
- **What the agents did:** one Claude Opus 5.5 session wrote the passes. It checked each step with a probe that skins
  every pose in Python (bone matrices times weights) into a row of meshes at z -60000 and shoots close-ups. Shots are
  in `C:\Jam\Builds\qa\hands` (j14 Julian, maya1, vex1, npcidle/npcuse).
- **What went wrong:** the editor doesn't tick animation on placed meshes, even with "update in editor" set and the
  pose forced to always tick, so neither a single-node player nor a poseable mesh posed. The Python skinner was the
  way round it. The first skin colour came from a shaded patch of the old hand (too orange), the second from the
  face (hit the brows). The wrist skin matches. Cutting the bare wrist skin by distance from the arm took the cuff too,
  since the sleeves are about as thin as the wrist. A left-running play-in-editor session blocked asset loading for a
  while.
- **Unverified (Marwan):** hands in play. The rest-arms layer (`HeroPalmTurn` 30 deg) was tuned to turn the old slab
  palms inward and may now over-turn real hands. The Bodyguard's hands-in-front pose. A small jagged seam at some
  wrists. The `Wave` clip is only 0.53 s long.

## Nov 15, 12:55: the hall's puzzle pieces make sense (banner out, breaker in)

- **Marwan's note (10:55, with a screenshot of the hall from above):** "what is this banner that drops and how can
  you phase through it, what are the people seated for and watching? Redesign the puzzle elements until they make
  sense, move stuff around, coordinate with the other sessions."
- **What was wrong:** a 4 m burgundy slab hung in mid-air between the seats and the screen, in the room's sight line.
  Fired, it dropped across the gap in the rope to "mask" a light beam, which would trip the beam, and Julian walked
  through it. The reserved seats were fenced by a black wall 40 m long. The screen was dark in house light, so 130
  seated guests faced nothing.
- **What changed:**
  - **Banner -> breaker.** The same device (`ABannerDevice`, label `Banner`, same use point on Maya's platform) is now
    the breaker of the aisle's electric eye: no matte (1 cm, hidden), it lands at once, the eye's lamps go green for
    10 s. Nothing hangs or drops in the room. Device name BREAKER, the beam's label ELECTRIC EYE, a new fire line, and
    the Scene 2 goal line and Maya's hint say so (`TakeDirectorScene.cpp`, `TakeDirector.cpp`: strings only).
  - **The rope.** The two `Rope_*` wall boxes are hidden (collision kept); velvet rope sags between 20 brass posts
    from wall to wall, with a RESERVED / PADDLE HOLDERS ONLY card each side of the gap. The beam keeps its own posts,
    lamps and glint.
  - **The sale.** In house light the screen shows a lantern slide: NOW SELLING, LOT 41, THE DIRECTOR'S CUT, the only
    print in existence, CURRENT BID $410,000. It sits behind the preview's film panel, so the film covers it when Maya
    dims the house.
  - All of it is one new overlay, `tools/sets/puzzle.py` (tag `set_puzzle`, in `run_all.py` before the lights), which
    patches the built level; `build_auction.py` carries the same breaker values for a rebuild.
- **How:** one Claude session (Fable 5.2). Three other sessions were in the same editor (set props, hero hands,
  device models, with their background agents). They were messaged first with the file list this session took, and
  each answered with its own files, hand-made level edits (none) and restart plans. The devices session will swap the
  winch model on the platform for a switch panel and rebuilt the editor with this session's two strings in it. No
  level rebuild and no restart were needed here.
- **What went wrong:**
  - An hour and a half went on waiting for the editor lock: other agents held it for up to 30 minutes at a time, and
    the editor crashed once under another session. The lock has no queue, so a waiter only gets in by polling.
  - A first try at brass "eye" houses on the gap's posts looked like stray black boxes in the shots and was removed.
  - Seen in editor shots only (`C:\Jam\Builds\qa\puzzle`), not in play: the breaker firing, the lamps going green,
    the slide under the film filter. The breaker has no sound (the banner's cloth whoosh is keyed to the old name) and
    Maya's old spoken hint no longer matches the new text, so that hint is subtitle only.

## Sun Nov 15, 11:00-13:15 Cairo: interactable props that move and make sense (one session, no agents)

Marwan, from the editor: "the light here is just a big box that doesn't even go over the edge, and the object has no
animation, you can't move it or change it." He was looking at the searchlight on the marquee balcony.

- **What it was:** the big orange wire box was the device's old click volume (unused since the Nov 13 rework, drawn
  in the editor only). The searchlight's whole trailer turned on the press and the lamp tilted at most 10 degrees,
  while the beam went straight from the lens to the gate 6 m below, through the rail and the balcony's lip. And it
  could not have worked: the gate stands just past the balcony's edge, so no lamp standing behind the rail can see it.
- **What changed:**
  - The searchlight and the follow spot are split into parts (`tools/sets/device_heads.py`, copies of the Rodin
    meshes with the other parts' triangles deleted): the stand stays put, the yoke pans, the drum or barrel tilts on
    its trunnion (`AFollowSpot` "Take|Head": `PanMesh`, `TiltMesh`, pivots, `LensMesh`, tilt limits, `RestTilt`).
    The beam leaves the lens along the drum, wherever it points, and the head swings on the old spring.
  - The searchlight's turntable, yoke and drum sit on a steel outrigger bolted to the balcony rail, the head out over
    the street (the trailer is not used); at rest it looks out and up, and on the press it swings down onto the gate.
  - While Maya is in reach and looking at a watcher, the dark head already follows her aim (`bHeadFollowsAim`).
  - Every lever throw eases out with a small overshoot (`HandleOvershoot`, `HandleFirstStep`); the pyro board, the
    sound desk and the dimmer board also jolt on the press (`PressJoltDegrees`).
  - The click volume draws only when its device is selected.
  - The "Banner" actor's model is now the breaker's knife-switch panel (the dimmer board's mesh, small, a red pilot,
    a handle that throws down and out), to the hall session's spec; `devices.py` leaves `puzzle.py`'s settings alone.
- **How:** one Claude session, sharing the editor with four others through `tools/editor_lock.py`; the header change
  rode on another session's editor restart. Most of the wall time went on waiting for the lock.
- **Checked:** captures of the searchlight at rest and tilted onto the street, the follow spot, the breaker, the pyro
  board, the dimmer board and the sound desk (`C:\Jam\Builds\qa\props`). Not checked in play: the swing's feel, the
  beam landing on the doorman from the new mount, the head following Maya's aim, the lever easing and the jolts; the
  scripted tests were not re-run.

## Nov 15, 13:45: the aisle's shutter is a gate that swings shut

- **Why:** on the alarm a steel shutter fell 8 m out of the dark into the middle of the auction to close the aisle
  behind Julian. After the banner went, Marwan said to fix that too.
- **What changed:** `AShutter` has a gate mode (`bSwingGate`, `OpenSwingDegrees`, `GateFootHeight`, `RailColour`, a
  brass `Rail` on the leaf). The aisle's shutter is now the bidders' gate: two low walnut leaves hinged on newel
  posts at the aisle's front mouth (`Shutter_Aisle`, `Shutter_Aisle_North`). They stand open along the front floor
  and swing shut in 0.35 s on the same trigger, blocking from that frame, with the same thud. The door shutters are
  unchanged. `tools/sets/puzzle.py` part `gate` sets it on the built level; `build_auction.py` has the same.
- **How:** the header change needed an editor restart, taken under the lock and announced to the other sessions:
  `ExitEditor` (it refuses when anything is unsaved), `Build.bat` (38 s), start (MCP in 10 s, level loaded soon
  after), then the overlay. The build also put another session's Live-Coding-only `CastMember.cpp` fixes into the DLL.
- **What went wrong:** the first walnut read as black in the shots and was lightened. Seen open and shut in editor
  shots (`gate_open_dais.png`, `gate_shut.png`, the shut one by setting the swing to 0), not swinging in play.

## Nov 15, 14:20 Cairo: s6-backstage, Scene 6 playable (grey box)
- **What changed:** Scene 6 (Cole, backstage) is `AScene6Backstage` (`Take/Script/Scene6Backstage.*`), set by
  `tools/script/scene6.py` in the vault room behind the screen. There, the film plays mirrored on the screen's back,
  and its light flickers and flashes on the soundtrack's loud moments (gunfight, explosion). The enforcers and the
  projectionist are cast members the scene spawns and walks itself: a patrol, a stair guard, and a projectionist
  who glances round. The scene runs a takedown masked by the film's sound, a drag into a costume trunk, a
  counterweight lever that drops a sandbag on a thunder sheet to lure the stair guard, a knockout and the mag-lock
  door. 7 SFX and 5 voice lines come from ElevenLabs (`tools/script/scene6_audio.py`).
- **Agents:** one session (s6-backstage); compile-checked in its own snapshot tree (`C:\Jam\Builds\s6snap`)
  before the shared restart.
- **What went wrong:** my lock acquire took spine's stale lock while spine's Build.bat was still running; I released
  it at once. A Live Coding compile after the restart hung ("CompileStillActive"), so the HUD font fix and the stair
  guard's downward sight wait for the next Build.bat. The large engine font draws boxes in this build: medium font only.

## Nov 15, 14:25: Scene 5, the private auditorium, plays from Jump 5 to the cut to Cole (s5-auditorium)

- **What changed:** `AScene5Auditorium` (`Take/Script/Scene5Auditorium.*`). The house goes low and Pendleton (the old
  auctioneer, top hat and cane) speaks at the brass podium in one hard spot, in his own close shot. He points at the
  booth and the house lights cut to black. The beam comes out of s7's booth port, the old film and LE SOLEIL NOIR hit
  the screen, the score starts, and the first timpani boom lands. Vance then sneaks along the south wall to the new oak
  door. A timpani hit every 1.6 s flares the screen and the HUD's drum mark. An usher and a guard sweep torches across
  the floor: moving inside a beam between booms fills SEEN, and full is a retake (Scene 3's VIP Alibi saves him
  once). At the door the deadbolt buzzes red, Vance speaks into his cuff, and the scene cuts to Cole.
  `AHouseDimmer` got a screening API (cut to black, held, flares on the beat) so the film runs on into Scenes 6 and 7.
  Set: `tools/script/scene5.py` (placement), `scene5_dress.py` (door, velvet rows, brass podium, signage),
  `tools/sets/puzzle.py` (the screen's slide is now the film's title card).
- **How:** one session plus two opus-high subagents (set dressing, audio). The dresser wrote its overlay but never got
  the editor lock, so the session ran it. Shots in `C:\Jam\Builds\script\s5\`.
- **What went wrong:** the shared editor build failed three times on other scenes' files: shadowing warnings treated as
  errors in Scenes 2, 3 and 4, then a link error, `GWhiteTexture`, which needed RenderCore. Each was fixed by its owner
  or by a one-word rename. A Live Coding compile then crashed the editor while it loaded the patch (other sessions'
  changed files ride along), and the unsaved placement was lost. The fix: restart on the built DLL, re-run the
  placement overlay, save. Lesson: no Live Coding while other sessions have unbuilt changes in Source.
- **Not checked in play:** being seen (the SEEN fill and the retake), the alibi, the torches' sweep against the real
  walk, and the score and voices (the audio was not imported yet).

### Nov 15, 14:30 Cairo: s1-marquee, Scene 1 playable (EXT. THE GRAND LUMIÈRE THEATER - NIGHT)
- `AScene1Marquee` (Take/Script/Scene1Marquee.*) runs the script's beats from `Reel.Script.Jump 1`, checked in PIE
  through `FinishScene`. Vance waits in the existing queue (`AQueueLine`, now with a script mode the scene drives). A
  suited floor manager is spawned at the doors, whispers to the Bouncer (the queue's doorman) and hands him the palm
  scanner. The Bouncer bolts it onto a podium at the door (a thud, the plate lights) and gives the "Magnetic cards are
  void" directive, while Vance's forged VIP card insert gets stamped VOID and drops away on a two-frame composure
  flicker. Guests at the plate: BEEP, green, in. Vance steps up: "Hand on the glass." → the cut to Scene 2.
- Scene 2's end lives here too: `PalmAccepted()` (a red laser sweep, the chime, green, the arch opens, "Enjoy the
  screening, Your Grace", Vance walks in) and `StageAtPlate()`. s2-van calls them by name, so neither scene includes
  the other's header.
- Street dressing by an opus-high helper (tools/script/scene1_dress.py): an Art Deco marquee THE GRAND LUMIÈRE with
  chasing bulbs and a sunburst crown, a vertical blade sign, the posters re-lettered for *Le Soleil Noir*, neon BAR /
  cocktails / HOTEL, procedural wet cobbles with puddles, and rain on 9 translucent planes. The level has no
  SSR/Lumen, so the puddle reflections of the signs are faked in the material against the signs' rectangles.
- What went wrong: the helper waited out a whole editor outage (restarts and a Live Coding crash). Its first run
  failed on a factory property (`initial_parent`), which I fixed on the spot. My C++ was compile-checked in a copy of
  the project (single-file UBT) before any shared restart, so it could not break anyone's build.

## Nov 15, 14:35 Cairo: s2-van, Scene 2 placed: the van's opening move and the signal-intercept minigame (Max)
- `AScene2Van` (Take/Script/Scene2Van.*): Max has no body, so the scene actor is its own camera. The opening flies it
  from the theater steps (on Vance at the palm plate) across the wet street, pushes it through the van's fogged
  windshield (a condensation bloom with drips, drawn on the HUD) and settles on Max. SPACE skips it; a retake skips it.
- The minigame on Max's console: the main CRT is a live SceneCapture of Vance at the plate (green phosphor, scanlines,
  a rolling bar, REC). On the scope, the scanner's amber handshake wave drifts; two dials (A/D frequency, Left/Right
  gain; each press moves the needle on its frame, holding turns it) bring Max's green wave onto it until SYNC fills,
  then SPACE patches the bypass. Patching before sync sparks and costs hesitation. Vance's hesitation bar fills over
  24 s (+5 s a retake, wider windows a retake); the bouncer speaks at 45% and 75%; full = the blooper "Vance hesitated
  at the glass". On the lock: BZZT-LOCK (flash, stamp, the toggle slammed up), "You're a French duke now. Drop the
  hand.", the cut to Vance, Scene 1's `PalmAccepted()` by name, and the finish when Scene 1 sets `S1PalmThrough`.
- Two opus-high helpers: one designed Max's voice with ElevenLabs Voice Design (`extuKh6lOtuXuEDVJn7l`, now also
  used by s3 and s8) and made his five lines, the bouncer's two (a monitor-speaker band) and six sound effects; the
  other builds the florist van set (`tools/script/scene2_van.py`), still waiting on the editor lock.
- What went wrong: my header vanished from disk once (rewritten and committed at once); spine parked the header
  without its .cpp for a build; a local named `Ink` shadowed `TakeStyle::Ink` (C4459 is an error here) and stopped
  s5's restart until renamed; the audio helper never got the editor lock in four tries, so I imported its WAVs with a
  top-level import script (the MCP Python tool rejects imports inside functions).
