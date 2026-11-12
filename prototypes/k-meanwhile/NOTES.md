# K. Meanwhile... browser grey box

Open `prototypes/k-meanwhile/index.html` in any browser (double-click; works from disk, no network).
Files: `index.html` (the game; the rules sit in their own `<script id="rules">` block), `bots.js` (headless bots
that load that same block: `node bots.js [runs] '{"configKey": value}'`).

**How to play:** WASD/arrows walk the hero on screen into the boxes (they can't be hurt). SPACE cuts to the other
room, whose hero is helpless; cut back as late as you dare, because the fuller their danger bar, the bigger the bonus.

**The question:** K's pass condition. Do you start leaving a room late on purpose to chase the bonus?

## Rules as implemented

- **Two rooms** (Rooftop, Vault), one hero each. You see one room full-size; the other is on a small monitor (top
  right) with its danger bar, the live "CUT NOW: +N" payout, and how many boxes are grabbing the hero.
- **Boxes** spawn at random points on the room's edges in both rooms all the time, walk at that room's hero and stop
  when they reach them. Spawn rate and walk speed ramp up over the reel.
- **Hero on screen:** plot armor (boxes can't hurt them). Auto-swings at boxes within reach on a short cooldown,
  killing up to 3 per swing. You only steer.
- **Hero off screen:** helpless. Each box grabbing them adds danger per second (capped at 6 boxes). Danger in the
  room on screen drains.
- **Cut (SPACE):** swaps rooms instantly. The bonus depends on the danger in the room you cut *into*:
  `bonus = bonusMax × ((d − 0.5) / 0.5) ^ 2` for d ≥ 50%, nothing below. Tiers: under 50% "Safe cut", 50% "Tense!",
  75% "Close call!", 90% "JUST IN TIME!". A 0.4 s cooldown stops ping-ponging.
- **Lose:** the off-screen hero reaches 100% danger, is "killed off", and the run ends. You keep your score
  but lose the wrap bonus and whatever the rest of the reel would have paid.
- **Win:** survive the 2:00 reel: "That's a wrap", +1000.
- **Score:** 10 per kill, plus cut-back bonuses, plus the wrap bonus. Best score is remembered in the browser.
- **Restart:** R at any time; SPACE on the end screen after 0.8 s (so a panicked cut doesn't skip the results).

### Order rules resolve in each tick (fixed 60 Hz, `step()` in index.html)
1. Cut: if SPACE was pressed since the last tick and the cooldown is clear, swap rooms, score the bonus (variations
   apply here). The press resolves before anything moves, so it is on screen in the next drawn frame.
2. Clock advances; spawn interval and box speed are interpolated over the reel.
3. Spawn boxes in both rooms.
4. The hero on screen moves.
5. Boxes in both rooms walk at their hero, push apart, and the grabbers per room are counted.
6. The hero on screen auto-swings.
7. Danger: rises in the room off screen (grabbers × rate), drains in the room on screen.
8. Killed-off check (off-screen danger ≥ 100%): run ends.
9. End-of-reel check: run ends with the wrap bonus.

### Game events to feedback
| Event | Feedback |
|---|---|
| Cut accepted | Hard cut (view swaps in the same frame), 90 ms white flash, clapper sound (noise burst + click), title card "Meanwhile, in the vault..." for 1.3 s |
| Cut under 50% | Grey "Safe cut, no bonus" |
| Cut 50–75% | Yellow "Tense!" + bonus, light shake, one tone |
| Cut 75–90% | Orange "Close call!", medium shake, two tones |
| Cut 90%+ | Big red "JUST IN TIME!" that scales in, heavy shake, 60 ms hit-stop, four-note arpeggio, particle burst on the rescued hero |
| Cut pressed during cooldown | CUT button flashes red, low buzz |
| Swing that hits | White ring at swing reach, particles per box, high blip |
| Box grabbing the off-screen hero | Box turns red on the monitor; hero blinks red/white and jitters harder as danger rises |
| Off-screen danger ≥ 60% | Monitor and main-view borders pulse orange; heartbeat thump, speeding up from 0.65 s to 0.16 s as danger nears 100% |
| Off-screen danger ≥ 85% | Borders pulse red; each thump adds a high beep |
| First run, before the first cut, danger ≥ 30% | Flashing "SPACE: cut to the vault!" hint |
| Hero killed off | "CUT!" end screen with the cause and time, heavy shake, falling saw tone + noise |
| Reel ends | "THAT'S A WRAP!" end screen, rising arpeggio |
| Last 10 s | Reel clock blinks red |
| Streak lost (variation) | Grey "streak lost", falling tone |
| Rescue blast (variation) | Big white ring, shake, low thud |

## Tuned values (the defaults in `CONFIG`)

The tuning panel (backtick key `` ` ``) changes every value live and has "copy JSON" to keep what you find.

| Value | Default | Note |
|---|---|---|
| reelSeconds | 120 | |
| heroSpeed | 270 px/s | room is 800 × 500 px |
| swingRadius / swingCooldown / swingMaxHits | 64 px / 0.26 s / 3 | |
| boxSpeed start → end | 65 → 95 px/s | |
| spawnInterval start → end | 1.1 → 0.5 s per room | ±30% random |
| dangerPerBox | 2.5 %/s per grabbing box | |
| dangerContactCap | 6 boxes | so top speed is 15 %/s: 90 → 100% takes 0.67 s |
| dangerDecay | 35 %/s while on screen | |
| cutCooldown | 0.4 s | |
| bonusMin / tierClose / tierJIT | 50% / 75% / 90% | |
| bonusMax / bonusPow | 400 / 2 | 90% pays 256, 95% pays 324 |
| killPoints / wrapBonus / deathScoreMult | 10 / 1000 / 1 | |

The first tuning (top speed 45 %/s) let the 90→100% stretch pass in 0.2 s, under human reaction time: the last-moment
bot died in 100% of runs. Lowering the cap and the per-box rate fixed that.

## What the bots showed

Bot model: steers toward the nearest box (or back to the middle). It checks the monitor every 0.3–0.9 s, or every
0.12–0.4 s once danger passes 60%. When it sees danger at or above its threshold it presses SPACE after
0.32 ± 0.1 s. 200 seeded runs per row, same rule code as the browser.

| Policy (press at) | Mean | Median | Death % | Cuts | Danger when the cut lands | s per cut |
|---|---|---|---|---|---|---|
| never cuts | 128 | 130 | 100% (at ~15 s) | 0 | – | – |
| safe (35%) | 3998 | 4000 | 0% | 14.7 | 44% | 8.2 |
| 50% | 4213 | 4215 | 0% | 12.9 | 60% | 9.3 |
| 75% | 5799 | 5804 | 0% | 11.0 | 82% | 11.0 |
| 85% | **6832** | 6810 | 0% | 10.1 | 92% | 11.9 |
| **last moment (90%)** | 5158 | **7372** | **47%** (avg at 59 s) | 7.1 | 97% | 14.5 |
| 93% | 734 | 689 | 100% | 1.0 | 99% | 19.5 |

- **The brief's bar holds.** Last-moment play beats safe play (mean +29%, median +84%) and is killed off in about half
  its runs. Never cutting loses within 15 s.
- **The best expected score is just short of greedy.** Pressing at ~85% lands at ~92% (reaction time adds ~7 points)
  and takes "Just in time" almost every cut with no deaths. Pressing at 90% gains ~60 points a cut and costs a ~10%
  death risk per cut. So the right play is "as late as you can and no later", which is the behaviour the pass
  condition looks for.
- **The risk is a cliff, not a slope.** Once 6 boxes grab, the bar climbs at a fixed 15%/s, so the risk comes only
  from the player's own glance and reaction timing. It is very sensitive to that: with a 0.45 s reaction, pressing at
  90% dies in 98% of runs (85% still dies 0%). Humans are less consistent than the bot, which smears the cliff into a
  gamble. A careful player may still learn "press at the 85 mark" and the cut turns into rote; the open questions
  below cover that.
- Kills barely differ between policies (~300 when the run survives), so the score gap is all cut-back bonus.

### Rule variations (toggles in the tuning panel, all off by default)
| Variation | Bot effect (press at 85% / 90%) | What it is for |
|---|---|---|
| **Streak** (`vStreak`): each paying cut in a row multiplies the bonus +0.5×, up to ×3; a safe cut resets it | 11159 / 8626 mean (death unchanged) | Makes greed compound and one timid cut hurt. The strongest pull toward late cuts |
| **Rescue blast** (`vRescueBlast`): a paying cut-back blasts every box within 150 px off the rescued hero | no change (6834 / 5014) | Feel only: the late cut lands as an event in the room, "the hero breaks free" |
| **Clean exit** (`vCleanExit`): bonus ×2 if no box is within 160 px of the hero you leave | 8683 / 6799 | Gives the on-screen room a job (clear it, *then* cut late). The sweeping bot gets it ~64% of the time without trying, so at 160 px it asks little; raise `cleanRadius` to ~250 to make it a real demand |

**What I would keep:** I can't judge feel, so this is a guess from the rules. Play the base game first. If it reads as
timer-watching, turn on **Streak + Rescue blast**: Streak is the most direct pull toward the pass condition, and
Rescue blast makes the "Just in time!" moment physical, which a 3D build will need anyway. Clean exit is the
fix if your eyes never leave the monitor.

## What won't carry over to a 3D third-person Unreal build

Carries over directly: the tick order, all timings and rates (seconds, %/s), the bonus curve and tiers, the score,
the monitor + danger bar + "CUT NOW" readout (a UMG widget, Unreal's UI system), the hard cut.

Won't carry over as-is:
- **Seeing the room.** Top-down shows the whole room; a third-person camera doesn't. That's harmless on screen
  (plot armor), but the monitor is how you read danger. Give it a fixed high "security camera" view (a SceneCapture2D,
  a camera that renders into a texture), not the other hero's chase camera. It costs a second scene render every
  frame: capture at low resolution and maybe 15–30 fps, and test it in the Web build early.
- **Units.** Pixels to centimetres: at 1 px = 2 cm the room is 16 × 10 m, the hero walks 5.4 m/s, boxes 1.3–1.9 m/s,
  swing reach 1.3 m. Third-person movement has acceleration and turning, so walking feel needs its own pass.
- **Crowd cost.** Up to 60 boxes per room × 2 rooms. Don't make them Characters (Unreal's heavy walking-pawn class);
  move simple actors or instanced meshes from one C++ manager, which also keeps the rule order in one place.
- **The room off screen must keep running.** Put both rooms in one level, far apart, and check that nothing stops
  updating when it isn't rendered (skeletal mesh animation does this by default).
- **The swing.** A circle in 2D; in 3D it needs an animation and a facing. Keep it automatic or instant-hit, so no
  press waits on a wind-up.
- **The cut.** Switch the view target with zero blend and route input to the other hero on the key-down frame.
- **The pile-up.** Boxes ringing a hero read as "grabbing" in 2D; in 3D they need to cling visibly or the danger is
  unreadable on a small monitor.
- **Two rooms vs three.** The spec says three heroes. With three, a cut becomes "which room", a new decision this grey
  box doesn't test.

## Open questions for Marwan
1. The pass condition: by your third run, were you deliberately waiting into the 75–90% band?
2. Did the on-screen room hold your eyes, or did you stare at the monitor? If the monitor, that's the
   "babysitting timers" risk from the brainstorm: try Clean exit.
3. When you died, did it feel like your fault ("I left him too long") or like the bar jumped?
4. Rhythm: roughly 10–12 s per room. Right, or too slow? Levers: `spawnInterval*`, `boxSpeed*`, `dangerPerBox`.
5. Does ending the run on a death suit a cold judge, or should a death cost points and the film go on?
6. Did one variation make you want another go more than the base game?
7. Two rooms or three for the real build?
