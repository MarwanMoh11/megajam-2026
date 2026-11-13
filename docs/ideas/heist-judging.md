# Judging the heist pitch (Nov 13, 12:15-13:00 Cairo)

Three Claude Opus agents played jam judges, each with a persona:

- **J1, gameplay designer:** fun, gameplay and level design; plays 3 minutes cold.
- **J2, cinematographer turned Unreal artist:** theme, visuals and audio; watches the video first.
- **J3, jam veteran:** what a solo first-timer actually ships; also judges the Solo Driver and Student prizes.

Each scored the jam's seven criteria from 1 to 5, twice: once if built exactly as described, once as likely shipped
by Sunday.

- **Round 1** scored Marwan's pitch (`heist-pitch.md`).
- **Round 2** scored the first refinement.

The second round's notes are folded into `docs/STATUS.md`. From round 3 the scoring changes; see "Rounds 3-7" below.

## Scores (J1 / J2 / J3)

| Criterion | Pitch, as written | Pitch, likely shipped | Spec, as written | Spec, likely shipped |
|---|---|---|---|---|
| Ambition | 5 / 5 / 5 | 3 / 3 / 3 | 4 / 5 / 4 | 3 / 4 / 4 |
| Theme | 4 / 4 / 5 | 4 / 3 / 3 | 5 / 5 / 5 | 4 / 4 / 4 |
| Fun | 3 / 3 / 3 | 2 / 2 / 2 | 4 / 4 / 4 | 3 / 3 / 3 |
| Gameplay | 3 / 3 / 3 | 2 / 2 / 2 | 4 / 5 / 4 | 3 / 4 / 3 |
| Visuals | 4 / 4 / 4 | 2 / 2 / 2 | 4 / 4 / 3 | 3 / 3 / 2 |
| Level design | 3 / 4 / 4 | 2 / 2 / 2 | 4 / 5 / 4 | 3 / 4 / 3 |
| Audio | 4 / 4 / 4 | 3 / 3 / 3 | 5 / 4 / 4 | 4 / 3 / 4 |
| **Mean** | **3.9** | **2.5** | **4.3** | **3.4** |

## Round 1: what the judges agreed on

- **Fun and gameplay were capped at 3.** Each obstacle had one glowing answer, so the player performed the
  screenplay instead of directing it. Maya and Vex were each "a button with a camera on it".
- **Nothing made a cut cost anything.** Cutting away had no price, so the wheel was just a menu.
- **The live AI interrogation had to go.** A key in the build gets extracted, offline judges get silence, and the
  latency breaks the window. As written you picked from a list anyway.
- **The scope didn't fit.** About 12 one-off set pieces, a crowd, guard AI, camera flights and a lobby: the likely
  build had about 1.5 scenes and an ending nobody reaches.
- **The look was promised, not buildable.** Gowns and chandeliers against grey mannequins with Lumen off.
- **What to keep:** three-way cutting inside one take, Julian unable to run, the blooper slate with instant retry, the
  sparks-curtain-bow finale, and PRINT IT.

## Round 1 fixes, as applied

- **Devices with three verbs**, Blind, Lure and Block, that work on any watcher, so every obstacle has 2-3 answers.
- **Marks:** composure drains while you're cut away and Julian stands off a mark, so you hit the mark, then cut.
- **One hall in three layers:** the floor, the catwalk above, backstage behind a scrim. The lobby becomes the
  hall's entrance.
- **Hard cuts on 1/2/3**, Julian's feed as an inset, the bluff pre-voiced, and the restore as a checkpoint.
- **The grey box as the style:** a soundstage in rehearsal, with spotlights in haze and a letterbox.
- **One music track** that never restarts across cuts, filtered per camera.
- **The stolen thing is the Director's Cut,** and the end card is a strip of the player's own cuts.

## Round 2 fixes, as applied

**From J1:**
- Contradictions fixed: the Scene 1 queue now drains nothing, and Scene 3's second answer works through Vance's own
  timed speeches.
- One input per job.
- Every finisher gets PRINT IT, and the grade is the cut earned: Director's, Final or Rough.
- The world pauses only for the freezes.
- The bluff's 1-in-3 guess became a fair 1-in-2.
- The first match cut lands inside 10 s.
- Range rings preview each device.
- The crew hints after two bloopers.
- Music ducks under voice.

**From J2:**
- One lens per camera.
- The bar names the cause of each drain.
- The gold case is in the first shot.
- The clapper sounds only at take start, bloopers and PRINT IT.
- The music tape-stops on a blooper.
- Exposure is manual.
- A dim skylight is added (there is no Lumen bounce light).
- Captures use Final Color (LDR).
- No more than about 4 shadowed spots on screen.
- Icons stand in for missing animations, and the bow is a bone tweak.

**From J3:**
- Maya and Vex stay at their posts and fire with E.
- The basic look moves into step 2.
- Scene 4 is built before Scene 3.
- Step 1 is trimmed to fit gate 1:
  - one header pass;
  - a level reload for the blooper;
  - flat cones;
  - the follow spot auto-targets;
  - the inset moves to step 2.
- The strip has a 6-still fallback.

## The remaining gap

All three judges put the last point down to **execution, not design**:

- whether the drain rates and windows feel right at gate 1;
- whether Saturday's exam leaves time for Scenes 3 and 4 and the look;
- voice quality and the mix.

The build order guards each one:

- gate 1 tunes rules, not art;
- the ending is built first;
- the basic look lands in step 2;
- a mix pass happens on Sunday.

## Rounds 3-7: the design alone, strict

**Marwan's instruction:** "keep doing this recursive loop until you have a theoretical 5 out of 5, and don't worry
about the ambition of the execution."

**From round 3 on:**
- The judges scored only the design, as if built well.
- A 5 means the top ~1% of 8,570 entries.
- No points for effort or for having changed.

That stricter scale is why round 3 starts lower than round 2.

| Criterion | R3 | R4 | R5 | R6 | R7 |
|---|---|---|---|---|---|
| Ambition | 3 / 4 / 3 | 4 / 5 / 4 | 5 / 5 / 5 | 5 / 5 / 5 | 5 / 5 / 5 |
| Theme | 4 / 4 / 4 | 5 / 5 / 5 | 5 / 5 / 5 | 5 / 5 / 5 | 5 / 5 / 5 |
| Fun | 4 / 3 / 4 | 4 / 4 / 4 | 4 / 4 / 4 | 4 / 5 / 5 | 5 / 5 / 5 |
| Gameplay | 4 / 4 / 3 | 4 / 4 / 4 | 4 / 5 / 4 | 4 / 4 / 5 | 5 / 5 / 5 |
| Visuals | 3 / 3 / 3 | 4 / 4 / 4 | 4 / 5 / 4 | 5 / 4 / 4 | 5 / 5 / 5 |
| Level design | 4 / 4 / 4 | 4 / 5 / 4 | 4 / 5 / 4 | 5 / 4 / 5 | 5 / 5 / 5 |
| Audio | 4 / 4 / 4 | 5 / 4 / 5 | 5 / 5 / 5 | 5 / 5 / 5 | 5 / 5 / 5 |
| **Mean** | **3.7** | **4.3** | **4.6** | **4.7** | **5.0** |

What each round changed in the spec:

- **After R3 (all three judges asked):**
  - Julian is **cued** to marks with Q and walks on his own, so you race him.
  - The finish replays **your take with your cuts**.
  - The "match cut" was really a cut on action, so it was renamed.
  - Each device got a **use budget**.
  - A **band on the floor** became the score and the world's clock.
  - **Deleted scenes** to restore, and the DIRECTOR'S CUT grade needs all four.
  - Camera A became **the film** (noir, red and gold), and B and C **the making-of**.
  - The bluff is answered from what the crew's cameras see, not guessed.
- **After R4:**
  - Fixed a finale soft-lock, where the budget could leave no way to blind the guards.
  - Merged the rope and the lift, so PRINT IT lands at about 2.5 minutes.
  - Made the clock visible: frames pulse on the beat, and ghosts show the next turn.
  - Added the arrival count on Julian's path.
  - Added an **edit suite** to re-cut the replay live.
  - Noir silhouettes in costume on camera A; stand-ins with tape labels on B and C.
  - Full colour bleeds in at the alarm.
  - Tally lights replaced "?" icons.
  - The drone now catches Julian off a mark anywhere on the floor.
  - Added a second exit.
- **After R5:**
  - Scene 2's watchers arrive one per band break.
  - The bluff is answered by firing the set.
  - A replay of your last good cut plays at each checkpoint.
  - The Bow is only possible at the side exit, which costs saved uses.
  - Plain words on screen.
  - Fixed six contradictions.
- **After R6:**
  - The drone's rule is taught before it can bite.
  - Uses are scarcer, and the pyro covers only the stage end.
  - The side exit glows gold from the first shot.
  - The crew cameras' light changed from sodium to cold fluorescent, with a full-width view and a monitor strip.
  - **Camera A became a shot list of composed fixed angles.**
  - The tutorial mark moved to the floor side of the rope, and Vance never walks.
- **After R7:**
  - A single tracking angle for The Long Take.
  - The exhibit's red tape.
  - The tutorial spot lands in shot.
  - Replay trims in 8-bar phrases.

In R7 the three judges each said further design rounds wouldn't move the scores, and that the remaining risk is
execution.

**Caveat:** the same three judges scored every round, so they are anchored on their own notes. A fresh judge who had
seen no earlier version scored the final spec as a check; see below.

## Fresh judges: the check on the panel

The panel's 5.0 came from three judges who had seen, and shaped, every version. To check it, three new Opus judges each
read only the spec, with the same strict rules: design only, a 5 means the top ~1%. Each new judge had a different
persona:

| Judge | Persona |
|---|---|
| F1 | A general jam judge |
| F2 | A judge with a film background |
| F3 | A hard-to-impress indie designer |

| Judge | Spec version | Am | Th | Fun | Gp | Vis | Lvl | Aud | Mean |
|---|---|---|---|---|---|---|---|---|---|
| F1, first read | R7 (the panel's 5.0) | 5 | 5 | 3 | 3 | 4 | 3 | 4 | **3.9** |
| F1, second read | The subtraction pass | 5 | 5 | 4 | 4 | 4 | 4 | 5 | **4.4** |
| F2, first read | The subtraction pass | 5 | 4 | 3 | 3 | 4 | 4 | 5 | **4.0** |
| F1, third read | One job per camera | 5 | 5 | 4 | 4 | 4 | 4 | 5 | **4.4** |
| F2, second read | One job per camera | 5 | 5 | 4 | 4 | 5 | 4 | 5 | **4.6** |
| F3, first read | One job per camera | 5 | 4 | 3 | 3 | 4 | 4 | 4 | **3.9** |

**What changed between versions:**
- **The subtraction pass** came after F1's first read:
  - point-and-click on every camera;
  - three effects;
  - one meter;
  - the bluff, the reversal, the breakers, the fuses, OFF MARK and WASD cut;
  - a clean run of about 1:45;
  - instrument warnings.
- **One job per camera** came after F2's first read: cue only on A, fire only on B and C, one bodyguard, crew views
  per scene, a real 20 s oner.
- **The final edits,** after the last reads, take only the fixes no judge disputed. The spec has not been scored since
  them:
  - path previews that turn red where Julian would be caught;
  - arrivals tied to Julian's progress;
  - checkpoints on the band's breaks;
  - finale prompts;
  - two visible exposure rules;
  - costume shells;
  - instruments taught solo;
  - slates for the deleted scenes;
  - the fixed contradictions.

**Conclusion.** Every judge who reads the spec cold lands near 4 (3.9, 4.0, 3.9). Judges who see their own notes
applied rise to 4.4-4.6. Ambition is 5 from everyone. Theme is 5 from most. Fun and Gameplay stick at 3-4, and on
those two the judges now ask for opposite things:
- F2 wants cueing on camera A only; F3 wants it on B too.
- F2 suggested that a GOOD CUT recharge the spot; F3 says that flattens the loop.

That is a taste plateau, so the loop stopped there. What decides Fun and Gameplay from here is gate 1: Marwan playing
the core loop.
