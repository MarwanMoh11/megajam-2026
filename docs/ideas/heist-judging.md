# Judging the heist pitch (Nov 13, 12:15-13:00 Cairo)

Three Claude Opus agents played jam judges, each with a persona:

- **J1, gameplay designer:** fun, gameplay and level design; plays 3 minutes cold.
- **J2, cinematographer turned Unreal artist:** theme, visuals and audio; watches the video first.
- **J3, jam veteran:** what a solo first-timer actually ships; also judges the Solo Driver and Student prizes.

Each scored the jam's seven criteria from 1 to 5, twice: once if built exactly as described, once as likely shipped
by Sunday.

- **Round 1** scored Marwan's pitch (`heist-pitch.md`).
- **Round 2** scored the first refinement.

The second round's notes are folded into `docs/STATUS.md`, which has not been scored again.

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
