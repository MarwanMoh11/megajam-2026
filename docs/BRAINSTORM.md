# Brainstorm: "Director's Cut"

Written 2026-11-11, the evening before the jam (ideas only, nothing built). List this file in `docs/PRE_JAM.md`.
Method carried from Luxel: read the theme several ways, cross out what everyone will build, run the fit test
("In this game, the director's cut is ___"), score with the checklist in luxel/docs/WINNING_JAM_GAMES.md, then
grey-box the riskiest question of the top two before committing.

## 1. Reading the theme (four layers)

**What a director's cut actually is** (the part most entries will skip):
- Two versions of one work: the studio's and the director's. A conflict over who decides.
- It restores what was removed, so it is usually longer.
- It is a second pass, made with hindsight and authority.

**"Cut" has other meanings:** an edit between two shots (match cut, jump cut, cross-cut, smash cut) · the shout that
stops a take · a **share of the money** ("my cut of the take") · a cutscene · cutting it close · a shortcut ·
cut and paste · budget cuts · the cutting-room floor.

**"Director" has other meanings:** film director · a game's AI director (the pacing system in Left 4 Dead) · live-TV
director · company director · the one who gives directions.

**Film logic that only exists because of editing:** plot armor · off-screen time passes differently · scenes are shot
out of story order · continuity errors · "it's fine, it's just a movie" · the long take with no cuts at all.

## 2. Crossed out: what 7,000 jammers and their AIs will build

These are the answers any AI gives first. Expect dozens of each; judges will be tired of them by entry 50.

| # | The crowd idea | Why it's out |
|---|---|---|
| 1 | Film-director sim: place actors and cameras, shout Action | The literal reading. UI-heavy, and the most common entry by far. |
| 2 | Retake time loop: redo the scene, past takes replay as ghosts | "Take 2" maps straight onto the best-known jam mechanic. |
| 3 | Editing puzzle: reorder or trim clips to change the story | Storyteller and Framed already did it; every AI names them. |
| 4 | A narrator-director bosses the player, who disobeys | The Stanley Parable. Meta jokes crowd fast (trait 3). |
| 5 | Deleted scenes: explore glitchy cut content of a fake game | Same meta crowd, and the look is "unfinished" on purpose. |
| 6 | Horror on a film set, a cursed reel | A skin; the theme isn't the mechanic. |
| 7 | Only what's in the camera frame exists | Viewfinder and Superliminal territory; a standard jam trick. |
| 8 | Cut = slash: a sword, scissors, a clapperboard weapon | The pun everyone finds in ten seconds. |
| 9 | Swap between the theatrical world and the director's-cut world | The two-world switch, with double the art to make. |
| 10 | Reverse horror: you are the AI director placing monsters | "Play the villain" was the crowd pick in Roles Reversed too. |

Also crowded, so not pursued: live-TV camera switching (Not For Broadcast), a stuntman hitting marks, studio
notes as roguelike curses, and "kill your darlings" (give up abilities as you go, which is Luxel's trade-off again).

## 3. Survivors

Each has a fit sentence, the familiar game underneath (trait 2), and its riskiest question.

### A. It's Just a Movie (heist comedy, inverted stealth)
- **Fit:** the director's cut is your **share of the loot**. The film shoot is the cover for a real robbery.
- **Rule:** a crime inside your camera's frame is "just a movie" and bystanders applaud. A crime seen outside the
  frame raises the alarm. You carry the camera; its view cone is your crew's only cover.
- **Verbs:** move, aim the camera, "Action!" (crew does the next step of the job), "Cut!" (crew freezes and acts casual).
- **Underneath:** vision-cone stealth, flipped: you are a cone that must cover things, not avoid being seen.
- **Feeling:** funny. Shoot / take / cut all mean two things. Failure is funny too (a guard asks for an autograph mid-drill).
- **Twist:** a second camera on a tripod; then a real film crew turns up to shoot at the same bank.
- **Verifiable by Claude:** yes, if guards walk fixed routes: the level is 2D cone geometry a bot can solve.
- **Riskiest question:** is covering the crew with a cone play, or babysitting?

### B. Plot Armor (action-tactics)
- **Fit:** the director's cut is the cut between characters. Whoever the film is following cannot be hurt; cut away
  and they're mortal.
- **Rule:** you control one hero at a time. Bullets bounce off the one on screen. The others stand where you left
  them, so park them in cover before you cut.
- **Verbs:** move, cut to another hero (one button). Nothing else.
- **Underneath:** The Lost Vikings / character-swap games, with one rule added. Unreal's possess-a-pawn makes the swap nearly free.
- **Feeling:** funny and tense. Everyone knows the trope; enemies whiffing every shot at the star is the joke.
- **Twist:** the villain has plot armor too until the last scene; later the camera cuts on its own on a timer.
- **Payoff:** the end credits bill the cast by screen time. "Starring: the janitor."
- **Verifiable by Claude:** partly (level reachability and line-of-sight); the feel needs you.
- **Riskiest question:** is being invulnerable boring? The danger must move fully onto the ones you left behind.

### C. Match Cut (first-person puzzle, the showpiece)
- **Fit:** the director's cut is how you travel. Frame a shape so it matches a shape in another scene, cut, and you are there.
- **Rule:** line up the desert sun inside the guide ring and cut: it's now a porthole and you're on a spaceship. A
  lamppost becomes a rocket, a spinning coin becomes a moon.
- **Underneath:** Superliminal / Viewfinder-style perspective puzzles, with the most famous edit in cinema as the verb.
- **Feeling:** wonder. Every cut is a trailer moment. The route of cuts you chose replays at the end as a
  20-second montage: your director's cut.
- **Why it suits this jam:** no characters or animation at all, only small lit scenes, which is exactly what the
  3D generators and Unreal's lighting do best. Highest ambition and visuals score of the list.
- **Verifiable by Claude:** yes: scenes and cuts form a graph it can check for dead ends.
- **Riskiest question:** is hunting for matching shapes a delight, or pixel-hunting? Fun and gameplay scores are the weak side.

### D. One Take (camera-operator skill game)
- **Fit:** the director's cut has no cuts. The director wants the whole scene in one unbroken shot and you are the camera.
- **Rule:** keep the star framed while walking backwards through a busy set. Bump a prop or lose the star and it's
  "Cut! Back to one."
- **Payoff:** you watch your own shot afterwards as the finished film (the scene is scripted, so replay is exact and cheap).
- **Risk:** needs a lot of character animation and choreography, the hardest thing for a first Unreal project.
- **Riskiest question:** is framing a moving actor fun for two minutes?

### E. Cut It Close (cross-cutting arcade)
- **Fit:** the director's cut is the cross-cut. Two or three scenes run at once; you only control the one on screen,
  and the score is suspense: the later you cut back, the bigger the bonus.
- **Underneath:** a multitasking game where near-misses pay. The bomb should be defused at 0:01, on purpose.
- **Risk:** three mini-games to build and tune, and split attention is easy to make stressful instead of fun
  (Luxel's trial failed by stacking two loads).

### F. Reshoots (diorama puzzle, played backwards)
- **Fit:** the director's cut restores missing scenes, and films are shot out of order. You play the last scene first;
  each earlier scene must **end exactly how the next one began** (1 heart, 2 bullets, hat missing, building on fire).
- **Underneath:** Hitman GO / Lara Croft GO on miniature film sets, with a Memento structure. You spend resources
  precisely instead of saving them.
- **Verifiable by Claude:** fully: a solver can prove every level and set its par. No feel risk (turn-based).
- **Risk:** lower fun ceiling; puzzle games rarely top a "fun factor" score.

### G. Cut & Paste (diorama puzzle)
- **Fit:** the director's cut is cut-and-paste across films. Cut the T-rex out of the monster movie and paste it into
  the rom-com to chase off the rival.
- **Why it suits this jam:** one-off props across genres are what the 3D generators are for. Figurines hop like board
  pieces, so no animation.
- **Risk:** sits near crowd idea 3 (editing puzzle), and needs a small rule system for how objects react.

### H. CUT! (one-button push-your-luck)
- **Fit:** the director's cut is the only button. The stunt gets more valuable and more dangerous every second you
  keep rolling; shout Cut before it goes wrong.
- **Risk:** shallow, and near crowd idea 1. Strong only as slapstick with Unreal's physics destruction.

### I. The Extra (blend-in game)
- **Fit:** the director's cut is what you're trying to survive. Stay in frame for screen time, but get noticed and you're cut.
- **Risk:** animation-heavy crowds; the theme is "film" more than "director's cut".

### J. Cutting Room Floor (platformer)
- **Fit:** you're a strip of deleted film splicing itself back into the reel.
- **Risk:** a premise more than a mechanic; fails checklist question 1.

## 4. Scores

Checklist questions that drop an idea: Q1 theme is the mechanic, Q2 one-sentence hook that isn't a crowd idea,
Q4 fun as grey boxes (a guess until someone plays it). Then 0-2 each on: understood in 10 s, cheap and funny
failure, second-half twist, variety from places, buildable by a first-time Unreal dev with agents.

| Idea | Q1 | Q2 | Q4 (guess) | 10 s | Failure | Twist | Places | Buildable | Total /10 | Clash risk |
|---|---|---|---|---|---|---|---|---|---|---|
| A. It's Just a Movie | Y | Y | likely | 2 | 2 | 2 | 2 | 1 | **9** | low |
| B. Plot Armor | Y | Y | likely | 2 | 2 | 2 | 1 | 2 | **9** | low |
| C. Match Cut | Y | Y | unsure | 1 | 2 | 1 | 2 | 1 | 7 | very low |
| F. Reshoots | Y | Y | modest | 1 | 2 | 2 | 2 | 2 | 9 (low fun ceiling) | very low |
| E. Cut It Close | Y | Y | unsure | 1 | 1 | 1 | 1 | 1 | 5 | low |
| D. One Take | Y | Y | unsure | 2 | 1 | 1 | 1 | 0 | 5 | medium |
| G. Cut & Paste | Y | weak | modest | 1 | 2 | 1 | 2 | 1 | 7 | medium |
| H. CUT! | Y | weak | likely | 2 | 2 | 0 | 2 | 1 | 7 | high |
| I. The Extra | weak | Y | unsure | 1 | 2 | 1 | 2 | 0 | dropped (Q1) | medium |
| J. Cutting Room Floor | N | Y | | | | | | | dropped (Q1) | medium |

Against the jam's seven judged criteria (guesses, 1-5):

| Idea | Ambition | Theme | Fun | Gameplay | Visuals | Levels | Audio |
|---|---|---|---|---|---|---|---|
| A. It's Just a Movie | 3 | 5 | 4 | 4 | 3 | 4 | 4 |
| B. Plot Armor | 3 | 4 | 5 | 4 | 3 | 4 | 3 |
| C. Match Cut | 5 | 4 | 3 | 3 | 5 | 3 | 3 |
| F. Reshoots | 3 | 4 | 3 | 4 | 4 | 4 | 3 |

## 5. Recommendation (round 1; superseded by the tier list in section 7)

**Grey-box B, then A, on Thursday morning; keep C as the ambitious alternative.**

- **B (Plot Armor)** is the safest route to a game that is fun in the first ten seconds. Two verbs, a trope every
  judge knows, the cheapest build (the Third Person template plus pawn swapping), and failure that is funny.
- **A (It's Just a Movie)** has the best theme answer of the list. A judge who has played forty film-set games will
  remember the one where the director's cut was 40% of a bank.
- **C (Match Cut)** is the entry that could win on ambition and visuals, and it needs no animation. It is also the one
  most likely to end up as a beautiful walk with thin gameplay.

Thursday 07:00-13:00 (the plan's idea-lock window):

| Test | Build (grey boxes only) | Pass if |
|---|---|---|
| B, ~90 min | Two heroes, one swap key, one turret whose shots bounce off the hero on screen and hurt the other | You invent a trick with the swap in the first two minutes, unprompted |
| A, ~2 h | One room, one robber on a 20 s drill, one guard on a loop, a camera cone you aim | Moving the cone feels like a decision every few seconds, not holding a pose |
| C, only if both fail | Two rooms, one sphere each, a guide ring, the cut | The first cut makes you say "oh!" out loud |

Whichever passes, write before building: the win condition, the order rules resolve in, and the event-to-feedback
table (the spec gaps that hit every Luxel builder).

## 6. Standing out when everything is film-themed

Clapperboards, letterbox bars and film grain will be on every entry. Pick one specific era and commit to it:
- A: 1970s heist film (split-screen panels, funk, bold title cards).
- B: 1980s action B-movie (squibs, synth, a VHS cover as the thumbnail).
- C: silent, wide and painterly; the cut sound is the only punctuation.

## 7. Round 2: stress test, new ideas, tier list

### 7.1 Round 1's top three, attacked

- **Plot Armor breaks under its own rule.** If the hero on screen can't be hurt, nothing threatens you while you
  play. It collapses into a ferry puzzle: park, swap, park. Fix: give each hero their own scene and their own horde,
  and score late cuts. That is idea K below, which also absorbs E (Cut It Close).
- **It's Just a Movie is the heaviest build on the list.** Crew, guards, bystanders, suspicion and camera cover
  through walls are five systems, and the joke needs character animation to land. Best theme answer, worst odds of
  being finished by a first-time Unreal dev.
- **Match Cut is the closest to Luxel's trial v1** ("a bland tech demo"): no fail state, no escalation, nothing that
  makes a second run. It needs a source of pressure before it is a game.

### 7.2 What round 2 did differently

- Took familiar games and flipped one rule (trait 2), instead of starting from film.
- Used the two readings round 1 left alone: **two versions** (studio vs director) and **scoring how close you cut it**.
- Asked of each idea whether Claude can prove it works without a human playing (the Luxel solver lesson).

### 7.3 New ideas

**K. Meanwhile... (cross-cutting action; replaces B and E)**
- **Fit:** the director's cut is the cross-cut. Three heroes in three places, each with enemies closing in. The one on
  screen has plot armor and fights; the other two are helpless, with a danger bar on a small monitor.
- **Hook:** cut back at the last second for a "Just in time!" bonus. The later the rescue, the better the film.
- **Underneath:** a horde-survival game (what Luxel was) played in three places at once. Each cut gets a
  "Meanwhile, on the rooftop..." title card.
- **Twist:** the three storylines meet in one room for the finale. A hero who dies is "killed off" and the film goes on.
- **Riskiest question:** does cutting between danger bars feel like directing, or like babysitting timers?

**L. The Tight Edit (timeline puzzle with a stunt payoff)**
- **Fit:** the director's cut is your edit. The bottom of the screen is a video-editing timeline; each trap in the
  3D scene is a clip on it. Slide and trim the clips, press play, and the star runs the gauntlet on a fixed path.
- **Hook:** surviving earns the "Studio Cut" (safe, boring). Every near miss under a quarter second raises the
  score toward the "Director's Cut". The same level asks to be re-edited tighter.
- **Underneath:** Poly Bridge's tweak-and-watch loop. Failure is a ragdoll and "Cut! Medic!".
- **Verifiable by Claude:** fully. The star and traps move on fixed timings, so a solver can find the best possible
  edit and set the medal scores, as Luxel's track bot did.
- **Risks:** a draggable timeline is the hardest interface on this list for a first Unreal project, and judges may
  file it with crowd idea 3 unless the stunt shows in the first five seconds.
- **Riskiest question:** is sliding clips and watching the take fun, or homework?

**M. Off-Screen (brawler; the only idea built on "two versions")**
- **Fit:** the studio demands a PG rating, so the director's cut is everything that happens off-screen. Sumo where
  the ring is the camera frame: shove enemies out of shot and the violence is only a sound effect.
- **Rule:** hits on camera raise the rating meter; a ring-out is free. If you leave the frame, the take is lost.
- **Twist:** the camera operator starts following the action, zooming and panning, so the ring moves and shrinks.
- **Riskiest question:** does shoving feel good without real fight animation? (Ragdolls forgive a lot.)

**N. Cut to the Beat (rhythm)**
- **Fit:** editors cut on the beat. Four cameras on four buttons film a concert or fight; cut on the beat to the
  camera where the action is. Your presses are the finished music video. A wrong cut shows a crew member eating lunch.
- **Risks:** audio sync in a new engine, dance animation, and a rhythm game has little level design to score.

**O. Edit the Lap (time-trial racing)**
- **Fit:** jump-cut the track. After each lap you may cut one section out, and the road rejoins itself.
- **Risk:** the whole game is driving feel, the slowest thing to tune in Unreal.

**P. Texas Switch (one-button runner):** swap the star for the stunt double only while they pass behind a pillar, so
the audience never sees the cut. Tidy, but the theme link is thin.

**Q. Cut the Deck (card roguelike):** the reel is a deck in fixed order and your only move is where to cut it. All
interface, no use of Unreal's 3D, and AIs suggest a card roguelike for every theme.

**R. The Longer Cut (Snake):** the film is the snake, longer is better, and tail you cut off stays as a wall. A clean
flip with no level design and low ambition.

Dropped on the fit test: a Goose-style "ruin every take" blooper sandbox (too much bespoke content), and matching a
storyboard with cheap props (the theme isn't the mechanic).

### 7.4 Tier list

Ranked by three things multiplied together: the likely score across the jam's seven criteria, the odds a first
Unreal project gets it finished in four days, and how few other entries will look like it. Fun is still a guess
for every row until someone plays a grey box.

| Tier | Idea | Why it sits here |
|---|---|---|
| **S** | K. Meanwhile... | Best odds of fun in the first ten seconds; reuses your horde-survival experience; cheapest build; late-cut scoring gives replay |
| **S** | L. The Tight Edit | Strongest theme answer (the medals are named Studio Cut and Director's Cut); Claude can prove every level; a stunt reel for the video. One big risk: the timeline interface |
| **A** | M. Off-Screen | Funniest premise and understood in two seconds; depends on shoving feeling good |
| **A** | A. It's Just a Movie | The most memorable theme pun; held back by five systems and animation |
| **A** | C. Match Cut | Could win on ambition and visuals; could also be a pretty tech demo |
| **B** | N. Cut to the Beat | Rhythm is reliably fun and scores on audio; sync and animation risk, thin level design |
| **B** | F. Reshoots | Fully provable and no feel risk; takes too long to explain to a skimming judge |
| **B** | O. Edit the Lap | Fresh, but lives or dies on driving feel |
| **B** | H. CUT! | Instant to understand, funny failure; shallow and likely to clash |
| **C** | D. One Take | Needs more animation than a first Unreal project can afford |
| **C** | G. Cut & Paste | Sits next to the crowd's editing puzzles |
| **C** | P. Texas Switch | Thin theme link |
| **C** | Q. Cut the Deck | All interface; card roguelikes are every AI's reflex |
| **C** | R. The Longer Cut | Too small for an "ambition" score |
| **C** | I. The Extra | The theme is film, not director's cut; animation-heavy |
| **D** | The ten crowd ideas (section 2) | Dozens of each will be submitted |
| **D** | J. Cutting Room Floor, B. Plot Armor (original), E. Cut It Close (original) | J fails the fit test; B and E are merged into K |

### 7.5 Thursday morning (replaces the table in section 5)

| Test | Build (grey boxes only) | Pass if |
|---|---|---|
| K, ~90 min | Two rooms, one hero each, boxes that walk at the hero, one cut key, a danger bar for the room you left | You start leaving a room late on purpose to chase the bonus |
| L, ~2 h | A capsule on a fixed path past three crushers; three clips you can slide; play; a near-miss readout | After surviving, you re-edit to shave the margins without being asked |
| M, only if both fail | A fixed camera frame, a shove, boxes that ring out | Shoving a box out of shot makes you laugh |

K is a feel idea only you can judge; L is one Claude can verify alone. Testing one of each means a failed fun gate
on Friday still leaves a route that doesn't depend on feel.
