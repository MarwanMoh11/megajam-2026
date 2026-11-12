# Epic AI MegaJam 2026: setup and jam plan

Theme: **Director's Cut**. Jam page: https://itch.io/jam/2026-epic-ai-megajam
Written 2026-11-11, the night before the start. All times below are Cairo (UTC+2).

## The facts that matter

| What | When (Cairo) |
|---|---|
| Building may start | Thu Nov 12, 07:00 (nothing game-related before this) |
| Daily livestream (tips, theme talk) | 21:00 every jam day |
| **Hard deadline** (late = disqualified) | **Mon Nov 16, 03:59** (Sun Nov 15, 5:59 PM PST) |
| Our upload target | Sun Nov 15, 22:00 (6 h buffer: UE packaging and big uploads are slow) |

- Engine: Unreal Engine **or** UEFN, never both. **Decision: Unreal Engine 5.9 MegaJam Preview** (below).
- Solo or up to 5 people; unlimited agents; any AI tools. Assets you didn't make are fine with a licence, listed in the submission.
- Declare anything made before Nov 12 (this plan, CLAUDE.md, agent configs, test scripts). Keep `docs/PRE_JAM.md` as that list.
- No unpackaged projects. UE submission = a packaged build zipped as `TeamName_GameName.zip`:
  - Desktop/mobile: Windows 10/11, macOS, Android, or iOS dev build; zip under 1 GB (bigger: butler).
  - Web (experimental): UE 5.9 MegaJam Preview, Shipping build, `index.html` at the zip's top level, under 500 MB total
    and 200 MB per file, marked "playable in the browser".
- The form also needs: team name, each member's name and **Epic account email**, game name, a 30-60 s gameplay video
  link (gameplay in the first 5 s), dev log link (needed for Glass Box), and "tools you used" (needed for partner prizes).
- Judging: 1-5 on each of ambition, theme, fun, gameplay, visuals, level design and progression, audio. Winners by Jan 29.
- Official Rules PDF (eligibility from Egypt, student proof): https://epicgames.box.com/s/a7xcgl07fqoieeb99sjtd3juhj4agc4z
  **Not read yet**: Box shows it as images. Read it before Thursday.

## Machines and engine

- **PC, Linux Mint boot = where the game is built.** Editor, project, agents, git. Epic ships a Linux build of the
  5.9 MegaJam Preview, and Claude sessions run there over SSH from the MacBook.
- **PC, Windows boot = packaging only.** The Official Rules require a Windows package for Unreal entries (p1), and
  Unreal can't package Windows from Linux. The project is mirrored onto the Windows partition and packaged there:
  first run Friday morning at the latest, then every evening, then the final.
- **MacBook = where Marwan sits.** Claude app, browser work, Moonlight for the PC's screen. No engine on it.
- **What Mint costs:** no Live Coding (C++ changes need a hot reload or an editor restart), two engine installs, a
  reboot per Windows package, and a look tuned under Vulkan but judged under DirectX. The tripwire: if the editor or
  its MCP server won't work on Mint within about an hour, the whole jam moves to the Windows boot.
- **Unreal Engine, not UEFN.** Claude knows UE C++, Blueprints and Python far better than Verse, a packaged build has no
  Fortnite rules or Fortnite look to fit, and the UE prize pool is the same size. UEFN's only exclusive is Walkie-Talkie.
- **C++ project.** Gameplay core in C++: it's text, so agents can diff, review and edit it without touching binary
  assets. Tunable values live in editable properties and data, so most iteration needs no compile.
- **Ship Windows first, Web as a bonus.** Epic's forum post says the Web target currently needs a source build of
  the engine, so it may not be available at all. One 30-minute attempt on the first Windows package, then decide.

## Setup (Thu, from the MacBook)

Setup is prompt-driven: `README.md` lists what to paste, and `setup/` holds one runbook per stage: reaching the PC,
three Mint stages, two Windows stages. The pipeline test runs on the real project, not on a throwaway one.

## Lessons carried from Luxel

| Luxel problem | Rule here |
|---|---|
| Fun judged last: "a bland tech demo"; art and audio built before the core was proven | **Fun gate**: someone who hasn't seen it plays the grey-box build by Sat noon. No art or audio agents start before it passes. |
| Controls waited for a tick, so presses felt dead | Every press does something visible in the same frame. |
| "Too difficult ... I want the judge to finish" | Tune for a judge playing 2-5 minutes cold: easy start, instant retry, the end reachable. |
| 13 ideas scored in 3 minutes; the riskiest question never tested | Pick the top 2 ideas, then grey-box the riskiest question for ~2 h before committing. |
| Spec gaps hit every builder (win rule, tick order, event cues) | Before building, write the win condition, the rule order, and an event-to-feedback table. |
| Parallel worktrees and three agents on one machine: collisions, flaky runs | UE assets are binary and don't merge: **one editor, one asset writer**. Parallel agents only on C++, docs, and asset generation outside the editor. |
| Deadline and rules were never recorded | They are at the top of this file. |
| The look changed mid-jam | Lock the look on Thursday, right after the idea. |
| Tool traps: the preview and file pane only see the session's starting folder | Start every session in the UE project folder on the PC. |
| Headless runs hang with no output | Run packaging and commandlets in the background with a log file; check the log, don't poll. |

## Jam schedule (Cairo)

| When | Goal | Done when |
|---|---|---|
| Thu afternoon | Mint setup stages 0-3 (README.md); browser grey boxes of K and L on the Mac | Claude drives the editor on Mint; you have played both prototypes |
| Thu evening | Pick one; write the spec; start the Unreal grey box of the pick | The pick and the spec are in docs/STATUS.md |
| Thu night | Windows boot: tools and engine download, overnight | The Launcher shows Launch for 5.9 |
| Fri morning | First Windows package of whatever exists | A Windows zip that runs |
| Fri | Core loop in grey boxes, one level start to finish | Playable from title to end |
| **Sat 12:00** | **Fun gate** + packaged builds (Windows, and Web if stage 3 passed) | A cold player wants another go; the Windows zip runs on another PC |
| Sat 12:00 - Sun 10:00 | Content: look, characters (Tripo), props (Rodin), materials (PBRgen), audio (WeLoveIndies), level progression | Content locked Sun 10:00 |
| Sun 10:00-18:00 | Polish: juice, title screen, how-to-play in 10 s, bug bash, the 30-60 s video, itch page, dev log | Nothing new after 18:00 |
| **Sun 22:00** | **Upload and submit** | Submission page shows the entry; download it and play it once |
| Mon 03:59 | Hard deadline | |

The pick is made from the browser grey boxes, so it doesn't wait for Unreal: Fri 10:00 at the latest; the fun gate stays.
The schedule lost Thursday morning to setup, so the first cut, if one is needed, is the Web build.

Package every evening from today on, in the background, so Sunday's build is never the first.

Dev log: the agents append to docs/DEVLOG.md at each milestone; post one short itch devlog entry per day (what was built, one GIF, what the agents did). It costs about
20 minutes a day and is the whole entry for **Glass Box** ($2,500).

## Prize targets

- Best Overall (UE), or **Best Student Project** if you're currently enrolled (same game, better odds).
- **Solo Driver** if solo; **Best Team** if you recruit (up to 5).
- **Glass Box**: the daily dev log above.
- Hyper3D pool: use Rodin for at least one shipped asset and list it.
- NVIDIA RTX award, only if the PC has an RTX card: DLSS 4.5 alone qualifies. Check tonight whether the DLSS plugin
  supports the 5.9 Preview; if not, drop it (NvRTX itself is a separate engine source build, not worth it).
- Out: Walkie-Talkie (UEFN only), Pocket Powerhouse (needs a phone build).

## The idea

Two candidates are locked for Thursday's grey-box tests: **K. Meanwhile...** and **L. The Tight Edit**. They are
described in docs/STATUS.md; the brainstorm, the crossed-out crowd ideas and the tier list are in docs/BRAINSTORM.md.
