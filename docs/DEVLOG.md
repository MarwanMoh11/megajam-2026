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
