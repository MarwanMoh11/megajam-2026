# Set life: the stage manager

Small rehearsed scenes ("pieces") among the guests and staff, so there is always some side business near Julian:
Hitman's overheard party stories, the Boyle party, RDR2's ambient scenarios. Built Nov 15. Code:
`Reel/Source/Reel/Take/SetLife/` (`SetLife.h` is the API, `SetLife.cpp` the system, `SetLifeSpilledDrink.cpp` the
worked example). Nothing outside that folder was changed.

## What it does
- `USetLifeSubsystem` (a world subsystem: one object per world that Unreal creates by itself) starts in every game
  world, PIE and packaged, never in the editor's own world. No level or asset edits. A take restart reloads the level,
  so the system and every piece start clean with it.
- On its first tick it gathers the pool (below). From 4 s into the take it keeps up to 3 pieces running
  (`Reel.SetLife.Max`), one start every 3-7 s, in the zones on show: Julian's zone and the camera's.
- Each piece casts free members, runs its beat sheet, then gives everyone back: the cast walk home (or slide, if they
  only stepped), face the way they faced, and the piece's props are removed. Someone off screen and far from the camera
  is put home at once.
- A piece that ran waits its cooldown (x0.8-1.2) before it can run again. One instance of each piece at a time; no
  member is ever in two pieces (claims). A piece that cannot cast now is retried after 6 s.
- A piece ends early ("abort") if a cast member is destroyed, it runs over 60 s, the director's state says so (below),
  or Julian is more than 40 m away from its anchor and it is off screen.
- Logs (LogTake, `SETLIFE`): `ready pool=N excluded=N pieces=N`, `start <piece> cast=<role>:<actor>,... at=(x,y)
  zone=<zone>`, `say <piece> <actor> "<line>" sound=<id> d=<cm to Julian>`, `end <piece> ok|abort(<why>) t=<s>`.

## Zones (world cm; X runs from the street to the screen wall)
| Zone | Where |
|---|---|
| Street | x < -3220 (the pavement, the queue) |
| Lobby | -3220 <= x < -1000, both floors (desk, palms, stair, balcony) |
| Hall | x >= -1000 (rows x 1900..3150, cross aisle x 2135..2400, front floor 3185..3600, dais 3600..4400, bar x 800..1600 y -1560..-1470 with the customers' side +Y, bartender at (1200,-1700)) |
| Catwalk | z >= 700 (deck z 900; Maya, Vex and the devices only: no pool members up there) |

Positions to anchor on: `tools/sets/hall_zones.json` (rows, aisle, front floor, dais, podium), `tools/sets/hall.py`
(bar), `tools/sets/front.py` (lobby LX0..LX1 = -3200..-1000, LY -1400..1400, the desk, the street furniture),
`tools/build_cast.py` (`STAFF_AT`: bartender (1200,-1700), receptionist (-2050,1100), cloakroom (-2650,1640)).

## The casting pool
- Kits GuestTux, GuestTails, GuestGown, GuestGownHat and Staff only. Never: Julian, Maya, Vex, Vance, the guards and
  any watcher (attached members), the doorman, the band.
- Not free: anyone claimed by a piece, anyone walking that we did not send (the queue drives its guests), anyone
  within 1.7 m of the queue's places, scan point, way in, inside spots, the clerk's spots or the doorman's post, and
  anyone standing on a queue place when the take started. Hidden members and members with `bActing` off are skipped.
- `FSetLifeRole` defaults: standing, near the action (within 25 m of Julian, or on screen within 35 m of the camera),
  never within 2.5 m of Julian, in the piece's zone. Seated members can be cast with `.Seated()` / `.AnySeat()`:
  gestures and looks only (Walk, Shift and Face ignore them; the seat would break).
- Cast members have no collision (nor do props), so a piece can never block Julian, a sight line or a test.

## Director states
| State | Effect |
|---|---|
| `Reel.SetLife 0` | every piece ends at once, nothing starts, no captions |
| the alarm (`bAlarm`, scene 3) | every piece ends; nothing starts for the rest of the take |
| the heist's dark (`AHouseDimmer::IsRoomBlind`: dark or rising) | hall pieces end (people walk home in the dark); none start |
| phase not Rolling/Cleared (title card, blooper freeze, PRINT IT) | held: the piece clock stops, nothing starts |
| slow time (`IsSlowed`) | held |
| the queue's taught beats (`AQueueLine::IsTeaching`) | held |
"Held" stops the beat sheet; a walk already given runs on at the world's (slowed) speed.

## Writing a piece
1. New file `Reel/Source/Reel/Take/SetLife/SetLife<Name>.cpp`. No header, no other file to edit. Put everything in a
   namespace named after the piece (the build glues .cpp files together: two files' helpers must not collide).
2. `#include "SetLife/SetLife.h"`, derive `FSetLifePiece`, implement `Setup` (cast and write the sheet; return false
   to give up), optionally `Tick` and `End`.
3. Register it at file scope: `SETLIFE_PIECE(<Id>, <Namespace>::FPiece, ESetLifeZone::Hall, <cooldown s>, <weight>)`.
   Id is a plain identifier, used in the logs.
4. Build (`Build.bat Reel Win64 Development` with the editor open checks it compiles; a new .cpp needs the editor
   rebuilt, not Live Coding). Check the log for `SETLIFE start <Id>` and `SETLIFE end <Id> ok`.

### The API (all on `FSetLifeStage& S`; roles are ints, INDEX_NONE is ignored everywhere)
| Call | What |
|---|---|
| `S.FindFree(Role, Max)` | free members that fit, best first (near, on screen, a little shuffled) |
| `S.Cast(Name, Role)` / `S.CastMember(Name, Member)` | claim one; INDEX_NONE if none |
| `S.Get / Where / Head / Home(R)` | the member, its feet, head, where it stood when cast |
| `S.SetAnchor(P)`, `S.SetReturnVia(R, {points})` | where it happens; a way home (round the bar) |
| `S.GetJulian()`, `GetCamera()`, `IsInView(P)`, `IsNearFocus(P)`, `FloorAt(P)`, `Rand`, `RandInt`, `Chance`, `Pick` | context |
| `S.At(t)`, `Wait(s)`, `WaitArrived(R, max)`, `WaitGesture(R, max)`, `WaitUntil(fn, max, abortOnTimeout)` | hold the line |
| `S.Walk(R, P, speed)`, `WalkUpTo(R, Other, stopShort)`, `Shift(R, offset, s)` | move (Shift: a step back, no turn) |
| `S.Face(R, yaw)`, `FaceTo(R, Other)`, `FacePoint(R, P)` | turn (eased) |
| `S.Gesture(R, G)`, `GestureAt(R, G, Other)`, `GestureAtPoint(R, G, P)` | Talk, Listen, Laugh, Sip, Nod, Shrug, Point, Wave, Hello, Applaud, CheckWatch, LookAround, Bow, Refuse, Use, Lift; None stops |
| `S.Look(R, Other, s)`, `LookAtPoint(R, P, s)` | the head turns |
| `S.Say(R, line, soundId, s)` | an overheard bark (below) |
| `S.Do(fn)`, `S.Finish()` | anything; end here |
| `S.HoldProp(R, shape, sizeCm, colour, offset, bone)` | a basic shape in the right hand, level, turning with the member; offset in its frame (x fwd, y right, z up) |
| `S.AddProp(shape, P, sizeCm, colour, rot)`, `GetProp`, `DropProp(id, P, rot)`, `RemoveProp` | props in the world |
| `S.Flash(P, candelas, s, colour)` | a point light that flashes and fades (a camera bulb) |
| `S.Abort(why)` | end now |

Beats run top to bottom. Actions take no time (several on one frame); only the At/Wait calls hold. Every beat call
returns the stage, so they chain: `S.Wait(0.5f).Say(Guest, ...).Gesture(Guest, TEXT("Shrug"));`. Positions for beats
are worked out in Setup; use `Do` or `WaitUntil` lambdas for anything that must read the scene when the beat runs
(capture role ints and values, never member pointers).

### The worked example: the spilled drink (`SetLifeSpilledDrink.cpp`, hall, cooldown 55 s)
```cpp
// casting: a gentleman with someone within 2.6 m, near the bar; the bartender (from off screen)
for (ACastMember* G : S.FindFree(FSetLifeRole().Men().NearPoint(BarFront, 1300.f), 10))
{
    const TArray<ACastMember*> Near = S.FindFree(FSetLifeRole().Guests().AnyDistance()
        .NearPoint(G->GetActorLocation(), 260.f).Where([G](const ACastMember& M) { return &M != G; }), 1);
    if (Near.Num() > 0) { Guest = S.CastMember(TEXT("Guest"), G); Partner = S.CastMember(TEXT("Partner"), Near[0]); break; }
}
if (Guest == INDEX_NONE) return false;
Waiter = S.Cast(TEXT("Waiter"), FSetLifeRole().Staff().AnyDistance().NearPoint(BarFront, 1600.f));
// ...geometry: Bump = 70 cm behind the guest's back, Approach = 2.4 m to the side, Way = round the bar's end...
S.HoldProp(Waiter, ETakeShape::Cylinder, FVector(38, 38, 1.5f), Silver, FVector(10, 0, 3));   // the tray
Glass = S.HoldProp(Waiter, ETakeShape::Cylinder, FVector(6, 6, 11), GlassTone, FVector(15, -7, 9.5f));
S.FaceTo(Guest, Partner).FaceTo(Partner, Guest).GestureAt(Guest, TEXT("Talk"), Partner);
for (const FVector& P : Way) S.Walk(Waiter, P, 130.f).WaitArrived(Waiter, 12.f);
S.Walk(Waiter, Approach, 130.f).WaitArrived(Waiter, 14.f);
S.Walk(Waiter, Bump, 100.f);
S.WaitUntil([W, BumpAt](FSetLifeStage& St) { return FVector::Dist2D(St.Where(W), BumpAt) < 90.f; }, 6.f);
S.Shift(Guest, Away * 32.f, 0.3f).Wait(0.28f);
S.Do([this](FSetLifeStage& St) { Spill(St); });          // DropProp(Glass, floor) + a claret pool (grown in Tick)
S.Wait(0.35f).FaceTo(Guest, Waiter);
S.Wait(0.45f).Say(Guest, L1.Text, L1.Sound).Gesture(Guest, TEXT("Shrug"));
S.Wait(0.7f).GestureAt(Partner, TEXT("Laugh"), Guest).Say(Partner, L2.Text, L2.Sound);
S.Wait(1.4f).FaceTo(Waiter, Guest).Gesture(Waiter, TEXT("Nod")).Say(Waiter, L3.Text, L3.Sound);
S.Wait(2.2f).FaceTo(Guest, Partner).GestureAt(Guest, TEXT("Talk"), Partner);
S.Walk(Waiter, Out, 120.f).Wait(0.8f).GestureAt(Partner, TEXT("Nod"), Guest).WaitArrived(Waiter, 14.f);
return true;
// SETLIFE_PIECE(SpilledDrink, SetLifeSpilledDrink::FPiece, ESetLifeZone::Hall, 55.f, 1.f)
```
At the end the stage removes the tray, glasses and pool, slides the guest back his 32 cm, turns the pair back, and
walks the waiter round the bar's end (SetReturnVia) to his place.

### How the cast's own acting interacts (read before choreographing)
Claimed members keep their own acting (`ACastMember` has no "hold" switch, and turning `bActing` off would also kill
looks and built gestures). What that means:
- A gesture you play holds off their conversation turn, idle acts and sips while it lasts (`IsGesturing`). Between
  your gestures they may Talk (if in a group), Nod, Laugh, Listen, LookAround, CheckWatch or Sip by themselves: keep
  the gaps short where it matters, or fill them with Listen/Talk.
- Your Look / GestureAt wins over their own looks for its seconds.
- Group mates still turn to a cast member when it is "their turn" to speak: natural for pieces inside a group.
- Walking: they face the way they walk (a WalkTo of more than 8 cm turns them). For a step without turning use Shift.
  Speeds 60-130 cm/s; there is no pathfinding or collision: route round furniture with points (the bar, the rows).
- Seated members: gestures and looks only. The room watch, the dimmer and Vance's applause may play gestures on cast
  members too; harmless.

## Overheard barks
- `S.Say` shows the line in the HUD's subtitle face (Fell italic, Bone on a half-ink plate, 72% of the subtitle size)
  over the speaker's head, only while Julian is within 7 m of the speaker, the head is on screen and no wall is between
  the camera and it. One line per speaker at a time; lasts 1.8-5 s by length.
- Voice: if `/Game/Take/Audio/<soundId>` exists (`UTakeAudioComponent::FindSound`), it plays at the speaker's head, 3D
  (full to 3 m, gone by 15 m), when Julian is within 15 m. Missing sound: caption only, logged `sound=<id>(none)`.
- Drawn from `AHUD::OnHUDPostRender` with its own `FTakeHudKit`: TakeHUD is untouched.

### The bark list (for batch voicing through tools/audio; nothing is voiced yet)
| Piece | Speaker | Line | Sound id |
|---|---|---|---|
| SpilledDrink | Guest (a gentleman) | Oh! Terribly sorry, old boy. | sl_spill_sorry_1 |
| SpilledDrink | Guest (a gentleman) | I say! Frightfully sorry. | sl_spill_sorry_2 |
| SpilledDrink | Partner (any guest) | Steady on, Reggie! | sl_spill_tease_1 |
| SpilledDrink | Partner (any guest) | Ha! Two left feet, as ever. | sl_spill_tease_2 |
| SpilledDrink | Waiter (the bartender) | Quite all right, sir. | sl_spill_waiter_1 |
| SpilledDrink | Waiter (the bartender) | No harm done, sir. I'll fetch another. | sl_spill_waiter_2 |
| Reunion | Spotter (A, any guest) | Kit? Kit Harlow, as I live and breathe! | sl_reunion_spot_1 |
| Reunion | Spotter (A, any guest) | Well, I'll be! Sandy Bell! | sl_reunion_spot_2 |
| Reunion | OldFriend (B, any guest) | Bunny! They let you out of Monte Carlo? | sl_reunion_reply_1 |
| Reunion | OldFriend (B, any guest) | Good heavens! Not since before the war! | sl_reunion_reply_2 |
| Reunion | Spotter (A) | You look a million dollars! | sl_reunion_chat_1 |
| Reunion | Spotter (A) | You still owe me twenty from Saratoga! | sl_reunion_chat_2 |
| Reunion | OldFriend (B) | Come on, let's find a drink. | sl_reunion_drink_1 |
| Reunion | OldFriend (B) | Inside. You can tell me everything. | sl_reunion_drink_2 |
| RivalBidders | Front | You took the Degas from me, Pemberton. | sl_rivals_needle_1 |
| RivalBidders | Front | Not this time, Pemberton. Not this lot. | sl_rivals_needle_2 |
| RivalBidders | Back | May the deeper pocket win, old man. | sl_rivals_brush_1 |
| RivalBidders | Back | Your pockets aren't that deep, old man. | sl_rivals_brush_2 |
| RivalBidders | Front | I'll go to the last penny. Mark me. | sl_rivals_vow_1 |
| RivalBidders | Front | We shall see whose pocket runs dry. | sl_rivals_vow_2 |
| RivalBidders | Neighbour | Ha! Every season, those two. | sl_rivals_aside_1 |
| RivalBidders | Neighbour | Here we go again. | sl_rivals_aside_2 |
| RivalBidders | Shusher | Shh! | sl_rivals_shush_1 |
| RivalBidders | Shusher | Shh! Do you mind? | sl_rivals_shush_2 |
| Toast | Waiter (the bartender) | Champagne? Fresh off the ice. | sl_toast_waiter_1 |
| Toast | Waiter (the bartender) | A fresh glass, with the house's compliments. | sl_toast_waiter_2 |
| Toast | Host (any guest) | Friends! A moment, if you please. | sl_toast_call_1 |
| Toast | Host (any guest) | I say, a word, everyone! | sl_toast_call_2 |
| Toast | Host | To Lot Forty-One, and the fool who pays for it! | sl_toast_toast_1 |
| Toast | Host | To the old girl: may she fetch a king's ransom! | sl_toast_toast_2 |
| Toast | Guest A | Hear, hear! | sl_toast_reply_1 |
| Toast | Guest A | Here's how! | sl_toast_reply_2 |
| Toast | Guest A (laughing) | Ha! Well said! | sl_toast_laugh_1 |
| Toast | Guest A (laughing) | Ha! Not me, I hope! | sl_toast_laugh_2 |
| WaiterRounds | Waiter (to a lady) | Champagne, madam? | sl_waiter_offer_madam |
| WaiterRounds | Waiter (to a gentleman) | Champagne, sir? | sl_waiter_offer_sir |
| WaiterRounds | Waiter | Compliments of the house. | sl_waiter_offer_house |
| WaiterRounds | Waiter | A glass before the bidding? | sl_waiter_offer_bidding |
| WaiterRounds | Guest (takes one) | Don't mind if I do. | sl_waiter_accept_1 |
| WaiterRounds | Guest (takes one) | How very kind. | sl_waiter_accept_2 |
| WaiterRounds | Guest (declines) | No thank you, I'm driving the Bentley. | sl_waiter_decline_bentley |
| WaiterRounds | Guest (declines) | Not for me, thank you. | sl_waiter_decline_2 |
| WaiterRounds | Guest (holding one) | I've one already, thank you. | sl_waiter_decline_glass |
| WaiterRounds | Waiter (after a no, lady) | Very good, madam. | sl_waiter_verygood_madam |
| WaiterRounds | Waiter (after a no, gentleman) | Very good, sir. | sl_waiter_verygood_sir |
| Whispers | Whisperer | Who's the fellow in the white jacket? | sl_whisper_who_1 |
| Whispers | Partner | Never seen him. New money, I'd wager. | sl_whisper_who_2 |
| Whispers | Whisperer | Isn't that the Ashford boy? | sl_whisper_ashford_1 |
| Whispers | Partner | I heard he lost everything. | sl_whisper_ashford_2 |
| Whispers | Whisperer | A white jacket, after dark? How daring. | sl_whisper_jacket_1 |
| Whispers | Partner | Darling, don't stare. | sl_whisper_jacket_2 |
| Whispers | Whisperer | Is he one of the bidders, do you think? | sl_whisper_bidder_1 |
| Whispers | Partner | In that jacket? I rather doubt it. | sl_whisper_bidder_2 |
| Photographer | Photographer | You there! One for the society page? | sl_photo_beckon_1 |
| Photographer | Photographer | Hold it! A picture for the papers? | sl_photo_beckon_2 |
| Photographer | Subject A | Us? Oh, how thrilling! | sl_photo_reply_1 |
| Photographer | Subject A | Is my hair all right? | sl_photo_reply_2 |
| Photographer | Photographer | Closer, now. Chins up! | sl_photo_pose_1 |
| Photographer | Photographer | Big smiles. Hold it... | sl_photo_pose_2 |
| Photographer | Subject B | Ha! I'm seeing stars! | sl_photo_blink_1 |
| Photographer | Subject B | Oh! Spots before my eyes! | sl_photo_blink_2 |
| Photographer | Photographer | Lovely! One more for the Herald. | sl_photo_again_1 |
| Photographer | Photographer | Swell! Once more, for the Herald. | sl_photo_again_2 |
| Photographer | Subject A | Do send us a print! | sl_photo_thanks_1 |
| Photographer | Subject A | Page six, darling, page six! | sl_photo_thanks_2 |
| Photographer | Photographer | Who's next? Smile for the Herald! | sl_photo_next_1 |
| Photographer | Photographer | Anybody else? Don't be shy! | sl_photo_next_2 |

Add a row per line when you add a piece. Ids: `sl_<piece>_<what>_<n>`, lower case. Keep lines under about 40
characters, 1940s register.

## Turning it off
- Console (PIE or the packaged game's console in Development): `Reel.SetLife 0`. Running pieces end and people walk
  home; `Reel.SetLife 1` turns it back on. `Reel.SetLife.Max <n>` caps the pieces at once (default 3).
- For a scripted test that should not see it, run `Reel.SetLife 0` before the take. Pieces never change gameplay
  (no collision, no input, they avoid casting within 2.5 m of Julian), so the tests should pass either way.
- To drop one piece, delete its .cpp (nothing else refers to it).

## Not done / known limits
- Not seen in play yet: the coordinator does the editor rebuild and the look. The first proof is `SETLIFE start
  SpilledDrink` in the log while Julian is in the hall near the bar.
- The tray rides at the bartender's right hand (no carry pose in the clip set): it is held low, level.
- No pathfinding: a piece's walks must be routed by hand.
