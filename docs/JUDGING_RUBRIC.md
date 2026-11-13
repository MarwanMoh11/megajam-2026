# Judging rubric: the one yardstick for scoring the game

Every score of this game, by an agent or by Marwan, uses this file and nothing else. It copies the jam's own
criteria and adds what the jam leaves out: fixed anchors for each score, who may judge, which changes get made, and
**when to stop**. Results go in `docs/ideas/heist-judging.md`.

## 1. The jam's criteria (the source)

From the Official Rules (p5-6) and the jam page; see `docs/SURVEY.md`. Seven criteria, 1-5 each, total out of 35.
Every entry is first pre-screened for rule compliance. **Ties are re-scored with Fun counted twice.**

| Criterion | What the organisers said |
|---|---|
| Ambition and Innovation | "Something you couldn't usually pull off in four days: a new mechanic, a bold art style, or a bigger scope" |
| Theme (Director's Cut) | "A fresh, clever take" |
| Fun | Not described; the tips say "a finished, fun game beats an ambitious broken one" |
| Gameplay | "Solid, original mechanics that feel good" |
| Visuals | Not described |
| Level design (and game progression) | "Spaces that support the gameplay and guide the player" |
| Audio | Not described |

**How a judge meets the game** (our reading of the rules and the field of 8,570 sign-ups): they watch the 30-60 s
video, then play the Windows build cold for 2-5 minutes. Anything they don't reach or understand in that time
doesn't score.

## 2. The anchors

A score must match its row's words; when it falls between two rows, take the lower. **5 means the top ~1% of
entries**, 4 the top ~10%, 3 a solid, finished jam game.

| | 1 | 2 | 3 | 4 | 5 |
|---|---|---|---|---|---|
| **Ambition** | A template or tutorial with small changes | One familiar idea, done small | A familiar genre with one new twist, or a large scope | A mechanic the judge hasn't played before, or a bold look, and it works | Both a new mechanic and a bold look, at a scope that seems impossible in four days |
| **Theme** | The theme is a title or a label | The theme is decoration or setting only | The theme shapes one mechanic | The theme is the core verb, and the judge says why within a minute | Every layer (mechanic, look, sound, ending) says "director's cut", and the take feels new |
| **Fun** | The judge quits before 2 minutes | It works, but nothing pulls the judge forward | Engaging for one run; no wish to replay | The judge laughs, gasps or leans in at least once, and **chooses** to retry | The judge asks for another go after the 5 minutes are up |
| **Gameplay** | Broken, or the controls fight the player | The controls work; the choices don't matter | Clear rules, responsive input, one real decision at a time | Several working answers to each problem, and every input reacts in the same frame | Deep and readable: few rules, many combinations, and mastery shows on screen |
| **Visuals** | Untextured and unlit | Default look, or mixed styles that clash | One consistent style; the action reads | A distinctive look that serves the play; good stills | Frames the judge would screenshot; the style is a reason to play |
| **Level design** | No direction; the player is lost | The path is clear, but the space only holds the action | The space guides the player, and difficulty climbs | Each space teaches one thing and then tests it; the ending is reached | Spaces and pacing build to a peak; the progression itself is memorable |
| **Audio** | Silent, or audio that annoys | Stock sounds, unrelated to the play | Feedback for every action, plus fitting music | Sound carries the game's information, and the mix is clean | Music and play are one system; the judge plays with the sound on to win |

## 3. Who judges

- **Fresh judges only.** Every round uses new agents (Opus, xhigh) that have never seen an earlier version, score or
  note. Never resume a judge with SendMessage: judges who see their own notes applied inflate the score (the panel
  gave 5.0 where cold readers gave 3.9).
- **Three judges per round,** each with a different persona: a general jam judge, a judge with a film or art
  background, and a hard-to-impress indie designer.
- **They read only** this file and the thing being judged (the spec, or the build's video and screenshots). No
  judging history, no dev log.
- **Each score cites** its anchor row word for word, and names the one concrete thing missing for the next level.
- **Two modes.** Design mode scores the spec as if built well. Build mode scores only what the build shows. Never
  mix them in one table.

## 4. Which changes get made

A judge's request goes into the spec only if it passes **all three** tests:

1. **Agreement or a defect.** At least 2 of 3 judges asked for it, or it fixes a contradiction, soft-lock or
   unreachable state.
2. **The learning budget.** What a cold player must learn stays within the budget below. A change that adds an item
   removes one.
3. **The clock.** It fits the build order in `docs/STATUS.md` without moving the upload (Sun 22:00).

| What a cold player learns | Budget (the spec on Nov 13) |
|---|---|
| Actions | 3 (cut, cue, fire) |
| Effects | 3 (Blind, Lure, Mask) |
| Meters | 1 (composure) |
| Watcher types | 6 |
| Device types | 6 |
| New words on screen | Only those already in the spec |

Requests that conflict between judges are not applied. They go on a list for Marwan, or for gate 1 to settle.

## 5. When to stop

A review ends at the **first** of these. Write which rule ended it in `heist-judging.md`.

1. **Target:** the fresh judges' mean is at least **4.5**, and no criterion scores below 4.
2. **Plateau:** the mean rose by less than **0.2** over the previous round.
3. **Conflict:** the judges' main requests contradict each other (a taste plateau).
4. **Cap:** **3 rounds** in design mode; **1 round** per gate in build mode.
5. **Nothing to apply:** no request passed section 4.

After a stop, nobody re-scores the same version. The next score comes only after the build changes.

## 6. Where we are (Nov 13, 14:00)

**Design mode reopened** at Marwan's request ("refine to the best possible before we proceed"), under this rubric:
fresh judges, at most 3 rounds, stop rule as above. Each round is logged in `docs/ideas/heist-judging.md`.

**Before that, at 13:35:**

- **Design mode is closed.** It ended on the conflict and plateau rules: cold reads of 3.9, 4.0 and 3.9, re-reads at
  4.4 and 4.6, and opposite requests on Fun and Gameplay. The spec is frozen except for defects found while
  building.
- **From here, build mode only:**
  - **Gate 1** (Fri about 22:30). Marwan plays the core loop. Their answer to "do you want another go?" is the Fun
    score that counts. One agent round may then score Gameplay and Level design from a capture.
  - **Gate 2** (Sat evening). Marwan plays a full take cold. Then one round of three fresh judges, on a 60 s capture
    and 12 screenshots, scores all seven criteria.
  - **Before upload** (Sun). No judging round: the build order's last step is downloading the zip and playing it once.
