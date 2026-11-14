// ATakeDirector: the rules. One per level (ATakeGameMode spawns one if the map has none).
//
// Owners (the Nov 13 rework): this header, TakeDirector.cpp and the (d) slices are (d); the other slices:
//   (d) TakeDirectorComposure.cpp  IsJulianExposed, StepComposure
//   (s) TakeDirectorPresses.cpp    StepPresses, HandleCut, HandleUse
//   (d) TakeDirectorScene.cpp      StepScene
//   (d) TakeDirectorSight.cpp      StepSight
//   (u) TakeDirectorDevices.cpp    FindUseTarget, StepUse, FireDevice, SpotWatcher, StepDevices
//   (d) TakeDirectorBlooper.cpp    RequestBlooper, StepBlooperCheck, StepFreeze, ReloadLevel, NewTake
//
// Step 2 owners (Nov 14): S = TakeDirector.cpp, TakeDirectorScene.cpp, TakeDirectorBlooper.cpp;
//   W = TakeDirectorSight.cpp, TakeDirectorComposure.cpp; D = TakeDirectorDevices.cpp, TakeDirectorPresses.cpp.
//
// Tick order (docs/STATUS.md "Tick order", as reworked; step 2 additions are marked +). Input and walking happen
// BEFORE the director in the same frame: the player controller ticks first (presses queued, WASD -> Move, mouse ->
// AddLook), then every character and its movement component (they move), then the director (tick prerequisites on
// the PC and on every character's movement component, set in EnsurePlayerController / BeginPlay):
//   1 the bar clock           Clock->Advance, + StepWarnings (only does anything on a bar line)
//   2 the flight, use, presses PC->StepFlight, CurrentCamera mirrors PC->GetCurrentCharacter(), StepUse (CurrentUse
//                             and the highlights), then StepPresses (cuts first, then uses, then a new take)
//   3 devices                 StepDevices
//   4 marks and sight         Julian->StepMarks, every Watcher->StepWatcher (+ now BEFORE the exposure test, so the
//                             drone's cone is tested where it is drawn), bJulianExposed = IsJulianExposed(), StepSight
//   5 composure               StepComposure
//   6 the scene               + StepTriggers, + StepMarkActions, StepScene, + UpdateObjective; then
//                             JulianFeetBeforeWalk = Julian's feet (for next frame's swept beam test)
//   7 late tick (TG_PostUpdateWork, a second tick function): StepBlooperCheck
// While Phase == BlooperFreeze only StepFreeze runs (presses are drained and dropped). Cleared keeps stepping.
// The camera manager updates after TG_PostPhysics, so a view-target change made in step 2 renders this frame.
//
// ATakeCamera and AShotZone (the old fixed cameras and camera A's shot list) are no longer used: the director does
// not gather them.
//
// Unity builds merge .cpp files: in slice files, put file-local helpers in a namespace with a unique name
// (e.g. namespace TakePressesLocal { ... }), never a bare static with a common name.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "GameFramework/Actor.h"
#include "TakeTypes.h"
#include "TakeDirector.generated.h"

class AJulian;
class AMark;
class AWatcher;
class ADevice;
class AFollowSpot;
class ARedTapeZone;
class ASceneTrigger;
class ACrewPost;
class AShutter;
class APostProcessVolume;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class ATakeCharacter;
class ATakePlayerController;
class UBarClockComponent;
class UComposureComponent;
class UTakeGameInstance;
class ATakeDirector;

/** The director's second tick function, in TG_PostUpdateWork: step 7 (end of frame). */
struct FTakeDirectorLateTick : public FTickFunction
{
	ATakeDirector* Director = nullptr;

	virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread,
		const FGraphEventRef& MyCompletionGraphEvent) override;
	virtual FString DiagnosticMessage() override;
};

/** One line the HUD flashes (a prompt or a refusal): drawn from the frame Flash() is called. */
USTRUCT()
struct FTakeFlash
{
	GENERATED_BODY()

	UPROPERTY()
	FString Text;

	/** Where and how the HUD shows it. */
	ETakeFlashKind Kind = ETakeFlashKind::Note;

	/** Counts up with every Flash(): the HUD shows each line once, on its own clock. */
	UPROPERTY()
	int32 Serial = 0;

	/** A refused cut: the key tab pressed (1-3); 0 none. */
	UPROPERTY()
	int32 Tab = 0;

	/** Seconds still to show. */
	UPROPERTY()
	float SecondsLeft = 0.f;

	/** Seconds it was shown for (the HUD may fade over the last part). */
	UPROPERTY()
	float Seconds = 0.f;
};

/** One deleted scene (rule 10): what the stats name and the grade counts. */
USTRUCT(BlueprintType)
struct FTakeDeletedScene
{
	GENERATED_BODY()

	/** Its id: the name RestoreDeletedScene is called with ("Bow"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Take")
	FName Id;

	/** "THE BOW". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Take")
	FString Name;

	/** Its slate line, shown in the stats when it was missed: "at the gold side exit, bow to the blinded drone". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Take")
	FString SlateLine;

	/** It can be restored in this build. The stats and the grade count only built scenes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Take")
	bool bBuilt = false;
};

UCLASS()
class REEL_API ATakeDirector : public AActor
{
	GENERATED_BODY()

public:
	ATakeDirector();

	// ---------------------------------------------------------------------------------------------------------------
	// Core (TakeDirector.cpp) (d)
	// ---------------------------------------------------------------------------------------------------------------

	/** Gathers the level's actors, makes this director tick after every character's movement component, refills
	 *  composure, logs ROLLING. The view starts on Julian (EnsurePlayerController: PC->SnapTo(Julian)). */
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Steps 1-6 in the order above. */
	virtual void Tick(float DeltaSeconds) override;

	/** Registers the late tick (TG_PostUpdateWork) next to the primary tick. */
	virtual void RegisterActorTickFunctions(bool bRegister) override;

	/** The running level's director (null if none). Cheap: cached, falls back to an actor search. */
	static ATakeDirector* Get(const UObject* WorldContextObject);

	/** Step 7, called by FTakeDirectorLateTick: StepBlooperCheck unless frozen. */
	void LateTick(float DeltaSeconds);

	/** Shows Text on the HUD from this frame for Seconds, in Colour, and logs it (TAKE_LOG "FLASH ..."). At most
	 *  MaxFlashes lines are kept; the oldest goes first. Use for prompts and refusals. */
	void Flash(const FString& Text, ETakeFlashKind Kind = ETakeFlashKind::Note, float Seconds = 2.f, int32 Tab = 0);

	/** The level's character with this Id (the first found), null if the set has none (no Vex in the test box). */
	ATakeCharacter* GetCharacter(ETakeCamera Id) const;

	/** Step 4: the camera whose look is on screen: the flight's destination from the frame of the cut press, else the
	 *  current character's. A before there is a player controller. */
	ETakeCamera GetLookCamera() const;

	/** Step 4: the picture is the film (the grade's noir and the film overlay): bFilmLook and the look camera is A, or
	 *  bMakingOfSplit is off. False: the making-of (or no look at all when bFilmLook is off). */
	bool IsFilmPicture() const;

	/** The game instance that survives reloads (take number, bloopers). Null outside a Take game instance. */
	UTakeGameInstance* GetTakeGameInstance() const;

	/** "bar.beat" for TAKE_LOG. */
	FString MakeLogStamp() const;

	/** Finds every character (Julian too), Julian, mark, watcher, device and tape zone in the level (TActorIterator). */
	void GatherActors();

	// ---------------------------------------------------------------------------------------------------------------
	// (d) TakeDirectorComposure.cpp
	// ---------------------------------------------------------------------------------------------------------------

	/** True while Julian stands inside any red tape zone (rule 5; the drone rule is not in step 1). Pure query. */
	bool IsJulianExposed() const;

	/** Step 5. Composure->Step(Dt, bJulianSeen, bOnMarkUnexposed) where bOnMarkUnexposed is Julian standing on a
	 *  mark (Julian->GetStandingMark()) and !bJulianExposed. Logs label changes (SEEN, MARK, none) through TAKE_LOG.
	 *  Does not fire the blooper: StepBlooperCheck reads Composure->IsEmpty() in the late tick. */
	void StepComposure(float DeltaSeconds);

	// ---------------------------------------------------------------------------------------------------------------
	// (s) TakeDirectorPresses.cpp
	// ---------------------------------------------------------------------------------------------------------------

	/** Step 2, after StepFlight and StepUse. Drains PC's press queue (PC->TakePresses). Handles every Cut first (in
	 *  queue order), then every Use, then NewTake (calls NewTake(), which reloads). Each handler changes what is
	 *  rendered inside the call. */
	void StepPresses();	// BODY: (s)

	/** A cut press to the character with Id Camera, in this order:
	 *  - PC->IsInFlight(): Flash("Mid-cut"), nothing else;
	 *  - GetCharacter(Camera) null: Flash("No camera C on this set");
	 *  - it is already PC->GetCurrentCharacter(): Flash("Already on MAYA");
	 *  - else PC->StartFlight(it) (the view leaves this frame).
	 *  TAKE_LOG("CUT -> B (key)") or the refusal. */
	void HandleCut(ETakeCamera Camera, const FTakePress& Press);	// BODY: (s)

	/** A Use press, on CurrentUse (computed this frame by StepUse, before the presses):
	 *  - PC->IsInFlight(): Flash("Mid-cut");
	 *  - no current character: nothing but the log;
	 *  - the current character is Julian: Flash("[2] MAYA or [3] VEX work the set");
	 *  - CurrentUse.Device null: Flash("Nothing in reach - stand in a device's ring");
	 *  - !CurrentUse.bCanFire: Flash(CurrentUse.Prompt) (why not: "CART - used", "Look at a watcher");
	 *  - the device is FollowSpot: SpotWatcher(CurrentUse.Watcher); else FireDevice(CurrentUse.Device).
	 *  TAKE_LOG("USE <who>: <device> -> fired / refused <why>"). */
	void HandleUse(const FTakePress& Press);	// BODY: (s)

	// ---------------------------------------------------------------------------------------------------------------
	// (d) TakeDirectorScene.cpp
	// ---------------------------------------------------------------------------------------------------------------

	/** Step 6. While Rolling, Julian standing on a bGoal mark: Phase = Cleared, PhaseSeconds = 0 (the HUD shows the
	 *  CLEAR stamp from this frame), Flash("CLEAR - that's the one"), TAKE_LOG("CLEARED on <mark>"). */
	void StepScene(float DeltaSeconds);

	// ---------------------------------------------------------------------------------------------------------------
	// (d) TakeDirectorSight.cpp
	// ---------------------------------------------------------------------------------------------------------------

	/** Step 4, after the watchers stepped. bJulianSeen = false; for each watcher:
	 *  - IsInstantBlooper() (the beam): if it touches Julian on the segment JulianFeetBeforeWalk -> his feet now ->
	 *    RequestBlooper(EBlooperReason::BeamTouched).
	 *  - otherwise (the guard): bTally = HoldsJulian(Julian) && bJulianExposed; W->SetTally(bTally) (frame solid red
	 *    and tally light on this frame); bJulianSeen |= bTally.
	 *  Logs tally changes ("GUARD frames Julian" / "GUARD lost him"). */
	void StepSight(float DeltaSeconds);

	// ---------------------------------------------------------------------------------------------------------------
	// (u) TakeDirectorDevices.cpp
	// ---------------------------------------------------------------------------------------------------------------

	/**
	 * What a Use press by Who would do right now. Pure query (no highlight, no log). Rules:
	 *  - Who null, PC->IsInFlight(), or Who is Julian: an empty target (Julian's Use press is HandleUse's flash).
	 *  - Otherwise the nearest device with FiredFrom == Who->Id whose GetUsePoint() is within its UseRadius (2D) of
	 *    Who->GetFeetLocation(). None: empty target.
	 *  - That device: Prompt = "[E] " + UseVerb, bCanFire = true; but if !Device->CanFire(WhyNot) (spent, busy,
	 *    running): bCanFire = false, Prompt = WhyNot ("CART - used", "SPOT - busy 2.1 s").
	 *  - If that device is the follow spot (and it can fire): the watcher is picked by angle: among Watchers with
	 *    DistanceToPoint(lamp location) <= the spot's Range, the one with the smallest angle between
	 *    Who->GetAimDirection() and (W->GetAimPoint() - Who->GetAimOrigin()), if that angle <= AimAssistDegrees.
	 *    Picked: Watcher set, Prompt = "[E] " + UseVerb + " " + W->GetLabel() ("[E] Spot GUARD"). None picked:
	 *    Watcher null, Prompt = "Look at a watcher", bCanFire = false.
	 */
	FTakeUseTarget FindUseTarget(const ATakeCharacter* Who) const;	// BODY: (u)

	/** Step 2, after StepFlight, before the presses: NewUse = FindUseTarget(PC current character, or null in a
	 *  flight). Moves the highlights on this frame when they change: old CurrentUse.Device->SetInReach(false), new
	 *  one SetInReach(true); old CurrentUse.Watcher->SetHovered(false), new one SetHovered(true). Then
	 *  CurrentUse = NewUse. Logs a change of device or watcher ("USE TARGET SPOT GUARD" / "USE TARGET none"). */
	void StepUse();	// BODY: (u)

	/** Step 2 (from HandleUse): fires Device if it can (Device->CanFire), which spends a use and starts its
	 *  timeline with a visible change this frame (pip out, cart starts rolling, banner starts dropping). If it
	 *  can't: Flash the reason ("CART - used") and return false. TAKE_LOG either way. */
	bool FireDevice(ADevice* Device);

	/** Step 2 (from HandleUse): the follow spot on Watcher. FollowSpot->FireAt(Watcher, WhyNot); out of range or
	 *  busy -> Flash(WhyNot) the same frame and return false. On success the beam shape shows this frame and the
	 *  Blind lands in StepDevices this frame. */
	bool SpotWatcher(AWatcher* Watcher);

	/** Step 3. Every device: StepDevice(Dt) (timelines). A device whose LandedThisFrame() is true applies its effect
	 *  to every watcher it GatherTargets() (in range of its effect point; the spot: its one target):
	 *  W->ApplyEffect(Effect, Duration, EffectPoint). Logs "CART lands: lured 1 (GUARD)". */
	void StepDevices(float DeltaSeconds);

	// ---------------------------------------------------------------------------------------------------------------
	// (d) TakeDirectorBlooper.cpp
	// ---------------------------------------------------------------------------------------------------------------

	/** Asks for a blooper at the end of this frame (only while Rolling; the first reason in a frame wins). */
	void RequestBlooper(EBlooperReason Reason);

	/** Step 7 (late tick), while Rolling: PendingBlooper set, or Composure->IsEmpty() -> the blooper: Phase =
	 *  BlooperFreeze, FreezeSecondsLeft = BlooperFreezeSeconds, LastBlooperReason set, the game instance's blooper
	 *  count and reason updated, TAKE_LOG("BLOOPER <reason>"). The HUD's red slate shows this frame (it is drawn
	 *  after the late tick). */
	void StepBlooperCheck(float DeltaSeconds);

	/** Runs instead of steps 1-6 while frozen: counts FreezeSecondsLeft down; at 0, ReloadLevel(). */
	void StepFreeze(float DeltaSeconds);

	/** Reloads the current level (works in PIE and packaged, e.g. GetWorld()->ServerTravel("?restart") or
	 *  UGameplayStatics::OpenLevel with the current level name). Logs it. */
	void ReloadLevel();

	/** R: a new take from the opening: the game instance's take number +1, Flash("TAKE <n>"), ReloadLevel(). */
	void NewTake();

	// ---------------------------------------------------------------------------------------------------------------
	// Step 2: scenes, triggers, checkpoints, the lift, the objective (S: TakeDirectorScene.cpp unless said)
	// ---------------------------------------------------------------------------------------------------------------

	/** Fires a named trigger, once per take (a second call with the same name does nothing). On this frame:
	 *  FiredTriggers[Name] = the clock's bar; every watcher whose ArriveOn == Name gets
	 *  ScheduleArrival(GetArrivalLine(bar) + its ArriveExtraBars) and a red Flash("<LABEL> - arrives on bar N");
	 *  TAKE_LOG("TRIGGER <name> bar N"). The scene script's own reaction lives here too:
	 *   - Keycard: Flash("KEYCARD LIFTED", gold), Clock->PlayChime() (step 3: the lift mark's ActionScene starts
	 *     Scene 3; the take is no longer cleared here).
	 *   - Step 3: Case -> StartAlarm; Winch -> the trap's flash; Bow -> RestoreDeletedScene; ExitTrap / ExitSide ->
	 *     PrintIt. Every trigger ends with SyncMarksAppeared. */
	void FireTrigger(FName Name);	// BODY: S

	/** The trigger has fired this take (or before the checkpoint this take restarted from). */
	bool HasFired(FName Name) const;	// BODY: S

	/** The arrival line for something triggered during Bar: the first bar AFTER Bar that starts a group of
	 *  ArrivalLineBars (with 4: bars 1, 5, 9, 13...). Bar 5 gives 9; bar 6 gives 9; bar 8 gives 9. */
	int32 GetArrivalLine(int32 Bar) const;	// BODY: S

	/** Step 1b, right after Clock->Advance. Only on a frame a bar line was crossed (Clock->BarCrossedThisFrame()),
	 *  with B = the bar that just started:
	 *   - a watcher with GetArriveBar() == B + 1 (it arrives on the next bar line): Clock->PlayWarning(its
	 *     Instrument), Clock->MuteMetronome(1) (its solo bar), Flash("<LABEL> arrives on the next bar"), TAKE_LOG.
	 *   - a watcher with GetArriveBar() == B: Clock->PlayChime() (the downbeat it arrives on).
	 *   - an arrived watcher with ChangesAtBar(B + 1, Clock): Clock->PlayWarning(its Instrument),
	 *     TAKE_LOG("WARNING <LABEL> <instrument>: changes on bar N"). At most one PlayWarning per instrument per
	 *     bar line. */
	void StepWarnings();	// BODY: S

	/** Step 6a, only while Rolling. Every ASceneTrigger whose TriggerName has not fired and that Contains Julian's
	 *  feet: FireTrigger(its name); if its Scene > 0 and differs from Scene: Scene = that, PlaceCrewForScene(Scene),
	 *  ShowTitle(its Title); if it has a CheckpointMark: SaveCheckpoint(that mark, its CheckpointName). In that
	 *  order, so the saved checkpoint already holds the trigger and the new scene. */
	void StepTriggers();	// BODY: S

	/** Step 6b, only while Rolling. Every action mark (AMark::IsActionMark) not yet done:
	 *  StepAction(Dt, Julian->GetStandingMark() == it && !bJulianSeen). On the frame one returns true:
	 *  FireTrigger(its ActionTrigger). */
	void StepMarkActions(float DeltaSeconds);	// BODY: S

	/** Step 6d: sets Objective, the HUD's one goal line, from the state right now (so it is right after a restore
	 *  with no script state saved). The table of lines is in the orchestrator's brief to S. Scene 1 teaches walk ->
	 *  [2] -> [E] spot -> [1] -> walk through; Scene 2 names the next thing to do. TAKE_LOG("OBJECTIVE <text>")
	 *  when it changes. */
	void UpdateObjective();	// BODY: S

	/** The big scene title across the upper middle ("SCENE 2 - THE FLOOR"), from this frame, for Seconds. Logs. */
	void ShowTitle(const FString& Text, float Seconds = 3.f);	// BODY: S

	/** Saves the checkpoint in the game instance (rule 9): CheckpointMark = Mark's MarkName, CheckpointName = Name,
	 *  CheckpointBar = the clock's bar now, CheckpointScene = Scene, CheckpointUses = every device's UsesLeft by
	 *  object name (GetName()), CheckpointTriggers = FiredTriggers, BloopersAtCheckpoint = 0.
	 *  Flash("CHECKPOINT - <Name>", gold), Clock->PlayChime(), TAKE_LOG("CHECKPOINT saved <Name> bar N scene N").
	 *  No game instance or no mark: nothing. */
	void SaveCheckpoint(AMark* Mark, const FString& Name);	// BODY: S

	/** BeginPlay, after GatherActors and Clock->SetStartBar(CheckpointBar), when the game instance holds a
	 *  checkpoint (CheckpointMark not empty): Scene = CheckpointScene; FiredTriggers = CheckpointTriggers, and every
	 *  watcher whose ArriveOn is in it gets ScheduleArrival(GetArrivalLine(that bar) + ArriveExtraBars) again;
	 *  every device RestoreUses(CheckpointUses[its name]) when listed; every action mark whose ActionTrigger had
	 *  fired SetActionDone(); Julian is put on the checkpoint mark facing its yaw (teleport, SetLook,
	 *  SnapFollowCamera, as BeginPlay does today); PlaceCrewForScene(Scene, true).
	 *  TAKE_LOG("CHECKPOINT restore <mark> bar N scene N"). With BloopersAtCheckpoint >= HintAfterBloopers and a
	 *  hint for this scene in SceneHints: Flash it (white, 6 s). */
	void RestoreCheckpoint();	// BODY: S

	/** Moves Maya and Vex to their posts for InScene: for each crew character, the ACrewPost with its Id and the
	 *  highest Scene <= InScene (none: it stays where it is). Teleport (feet on the post, facing its yaw),
	 *  SetLook(yaw, StartLookPitch), SnapFollowCamera, velocity zeroed. Skips a character the player is on or
	 *  flying to, unless bEvenIfCurrent (the restore). TAKE_LOG("POST MAYA -> scene 2 (x, y, z)"). */
	void PlaceCrewForScene(int32 InScene, bool bEvenIfCurrent = false);	// BODY: S

	/** The blooper (StepBlooperCheck): remembers where Maya and Vex stand and look, in the game instance. */
	void SaveCrewPlaces();

	/** The first tick after a blooper's reload: puts Maya and Vex back where SaveCrewPlaces found them (teleport,
	 *  look, follow camera snapped). Nothing saved (a first take, a new take): they stay where the map put them. */
	void RestoreCrewPlaces();

	/** Step 2, after CurrentCamera is mirrored: the first time in a session the player is on a character, its
	 *  IntroLine is flashed (white, IntroLineSeconds). Never during a flight. */
	void StepIntroLines();

	/** W (TakeDirectorSight.cpp). Is the straight line From -> To blocked for a watcher's frame (rule 6)? True if
	 *  a line trace on ECC_Visibility hits anything that is not a take actor (SightIgnore: every ATakeCharacter,
	 *  AWatcher, ADevice, AMark, ARedTapeZone and ASceneTrigger is ignored), or if any device's
	 *  BlocksSight(From, To) is true (a dropped banner). The map's dressing ignores ECC_Visibility already. Pure. */
	bool IsSightBlocked(const FVector& From, const FVector& To) const;	// BODY: W

	// ---------------------------------------------------------------------------------------------------------------
	// Step 3: Scene 3 (the alarm), the ends, the title card. BODY: S, in TakeDirectorFinale.cpp. The full contract
	// (who calls what, the order in Tick) is in the orchestrator's brief to S; the comments here are the summary.
	// ---------------------------------------------------------------------------------------------------------------

	/** The mark with this MarkName (the first found), or null. */
	AMark* FindMark(const FString& MarkName) const;

	/** Every mark with AppearOn set: SetAppeared(HasFired(AppearOn)) when that differs from HasAppeared(). Called at
	 *  the end of FireTrigger and in the first tick after the restore. */
	void SyncMarksAppeared();

	/** The bar line the alarm goes live on when the case was opened during CaseBar: CaseBar + 1 + AlarmHoldBars (the
	 *  rest of that bar and AlarmHoldBars whole bars are held: nothing sees him). */
	int32 GetAlarmLiveBar(int32 CaseBar) const;

	/** The bar a watcher with ArriveOn == Trigger arrives on when the trigger fired during FiredBar: for the Case
	 *  trigger GetAlarmLiveBar(FiredBar) (the guards walk in as the alarm goes live), for every other trigger
	 *  GetArrivalLine(FiredBar). The caller adds the watcher's ArriveExtraBars. FireTrigger and RestoreCheckpoint. */
	int32 GetArrivalBarFor(FName Trigger, int32 FiredBar) const;

	/** The case is open (FireTrigger(Case), or the restore of a checkpoint that has it): bAlarm = true,
	 *  AlarmLiveBar = GetAlarmLiveBar(CaseBar), AlarmDeadlineBar = AlarmLiveBar + FinaleBars, bAlarmLive = false,
	 *  Clock->SetAlarm(true), every shutter with DropOn == Case StartDrop (bFromRestore: SetDown). Not from a
	 *  restore: ShowTitle(AlarmTitle) and Flash(AlarmLine, red). TAKE_LOG("ALARM case bar N live N deadline N"). */
	void StartAlarm(int32 CaseBar, bool bFromRestore);

	/** Step 1c, after StepWarnings, while Rolling and bAlarm. The clock's bar >= AlarmLiveBar and !bAlarmLive: the
	 *  alarm goes live (bAlarmLive = true, every watcher SetAlarmLook(true), every ADroneWatcher
	 *  SetChaseTarget(Julian), Flash(AlarmLiveLine, red), TAKE_LOG("ALARM LIVE bar N")). The clock's bar >=
	 *  AlarmDeadlineBar: RequestBlooper(ClockRanOut) (TAKE_LOG("ALARM clock ran out") once). */
	void StepAlarm();

	/** Bars left before the deadline, the running one included (FinaleBars during the hold; 0 with no alarm). */
	int32 GetAlarmBarsLeft() const;

	/** Seconds until the deadline's bar line (0 with no alarm). */
	float GetAlarmSecondsLeft() const;

	/** The broadcast drone (a watcher with ExposesOffMark) has arrived and is Blinded right now. */
	bool IsDroneBlinded() const;

	/** Rule 10: restores the deleted scene Id, once per take (the game instance's RestoredScenes). On this frame:
	 *  Flash("DELETED SCENE RESTORED: " + its Name, GoldBright, 4 s), Flash(KeepThatInLine, white), the chime,
	 *  TAKE_LOG("DELETED SCENE RESTORED <id>"). Unknown or unbuilt id, or already restored: nothing. */
	void RestoreDeletedScene(FName Id);

	/** Rule 11. Built = the DeletedScenes with bBuilt, Restored = those of them in RestoredScenes. DIRECTOR'S CUT:
	 *  Built > 0 and Restored == Built. FINAL CUT: Restored >= FinalCutScenes, or bBySideExit when
	 *  bSideExitIsFinalCut. Else ROUGH CUT. */
	ETakeGrade ComputeGrade(bool bBySideExit) const;

	/** Julian is out with the reel (FireTrigger(ExitTrap / ExitSide)), only while Rolling. On this frame:
	 *  bSideExit, Grade = ComputeGrade, FinishSeconds = the clock's time since bar 1 beat 1, FinishBar, PrintLine =
	 *  PrintLineSide or PrintLineTrap, SetPhase(Printed), EndPage = 0, EndPageSeconds = 0, Clock->SetAlarm(false),
	 *  PlayClap and PlaySting, every character stopped (as the blooper does). TAKE_LOG("PRINT IT exit=TRAP grade=ROUGH
	 *  CUT time=132.0 cuts=14 bloopers=2 restored=0/1"). */
	void PrintIt(bool bBySideExit);

	/** Runs in place of steps 1-6 while Printed. EndPageSeconds += Dt. Pages: 0 the PRINT IT slate, 1 the stats and
	 *  the grade, 2 the blooper reel and the credits (it stays). Page 0 turns by itself after EndSlateSeconds, page 1
	 *  after EndStatsSeconds. Presses (drained here): NewTake -> NewTake(); Confirm or Use -> the next page at once
	 *  (on page 2: Flash("[R] another take")); Cut -> Flash("That is a print.  [R] another take"). A page turn sets
	 *  EndPageSeconds = 0 and TAKE_LOGs "END PAGE n". */
	void StepEnd(float DeltaSeconds);

	/** Runs in place of steps 1-6 while Title (the clock holds on bar 1, nothing moves). Presses (drained here):
	 *  Confirm or Use -> SetPhase(Rolling), the opening slate (ShowTitle(VenueLine, 4 s) and the Scene1Title flash
	 *  with the take number, as the first tick does today), Clock->PlayClap(), TAKE_LOG("TITLE CARD dismissed");
	 *  NewTake -> NewTake(); Cut -> Flash(TitleCardPrompt). */
	void StepTitleCard(float DeltaSeconds);

	/** Step 4, the film grade (docs/ART_STYLE.md §15.3): drives GradeMID, the dynamic instance of the volume's grade
	 *  material. MakingOf = 1 on B and C (bMakingOfSplit) from the frame of the cut press (GetLookCamera); Bleed runs
	 *  0 -> 1 on a smoothstep over BleedSeconds once the alarm is live (bAlarmLive); bSnap (the first tick, a
	 *  restore) lands it on the frame. bFilmLook off: the blendable's weight 0 and the volume's tone curve back.
	 *  Called at the end of step 6 every frame and once in the first tick with bSnap. No LookVolume: nothing. */
	void StepLook(float DeltaSeconds, bool bSnap = false);

	/** Step 4: finds the grade material in LookVolume's blendables (or loads FilmGradeMaterial), puts a dynamic
	 *  instance in its place (GradeMID) and turns the volume's tone curve off (the material does the tone). */
	void SetupLook();

	// ---------------------------------------------------------------------------------------------------------------
	// Components and tunables
	// ---------------------------------------------------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<USceneComponent> Root;

	/** The band's bar clock (step 1). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Take")
	TObjectPtr<UBarClockComponent> Clock;

	/** Julian's composure (step 5). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Take")
	TObjectPtr<UComposureComponent> Composure;

	/** The blooper freeze before the reload (rule 9). */
	UPROPERTY(EditAnywhere, Category = "Take|Blooper")
	float BlooperFreezeSeconds = 1.0f;

	/** How many flash lines the HUD keeps. */
	UPROPERTY(EditAnywhere, Category = "Take|HUD")
	int32 MaxFlashes = 4;

	/** Watchers arrive on the next line of this many bars after their trigger (rule: "the next 4-bar line"). */
	UPROPERTY(EditAnywhere, Category = "Take|Scene", meta = (ClampMin = "1"))
	int32 ArrivalLineBars = 4;

	/** Composure refilled for each extra watcher one firing catches (rule 5: x2, x3). */
	UPROPERTY(EditAnywhere, Category = "Take|Scene")
	float ExtraWatcherRefill = 25.f;

	/** The crew hint over the earpiece comes after this many bloopers at one checkpoint (rule 9). */
	UPROPERTY(EditAnywhere, Category = "Take|Scene")
	int32 HintAfterBloopers = 2;

	/** The hint for each scene (index 0 = Scene 1). */
	UPROPERTY(EditAnywhere, Category = "Take|Scene")
	TArray<FString> SceneHints;

	/** A watcher's turn warning plays only while it is within this distance of Julian (2D, cm): the guard in the
	 *  hall is not heard from the street. 0: always. An arriving watcher's solo always plays. */
	UPROPERTY(EditAnywhere, Category = "Take|Scene")
	float WarningEarshot = 3000.f;

	/** The opening slate, as the big title on a fresh take (a first take or R; not after a blooper). */
	UPROPERTY(EditAnywhere, Category = "Take|Scene")
	FString VenueLine = TEXT("AURELIA AUCTION HOUSE|LOT 41: THE DIRECTOR'S CUT");

	/** Flashed with the take number under the opening slate. */
	UPROPERTY(EditAnywhere, Category = "Take|Scene")
	FString Scene1Title = TEXT("SCENE 1 - THE DOOR");

	/** How long a character's cover-story line shows, s. */
	UPROPERTY(EditAnywhere, Category = "Take|Scene")
	float IntroLineSeconds = 7.f;

	/** Scene 1's objective turns to "[2] MAYA" once Julian's feet are this close to the scanner's gate, cm, whether
	 *  or not he stands on the mark. */
	UPROPERTY(EditAnywhere, Category = "Take|Scene")
	float DoorPromptDistance = 450.f;

	// --- step 3 tunables ---------------------------------------------------------------------------------------------

	/** The title card shows before the session's first take. Off: the take starts at once (tests). */
	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	bool bShowTitleCard = true;

	/** Flashed when a cut key is pressed on the title card. */
	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	FString TitleCardPrompt = TEXT("[ENTER] Action!");

	/** Whole bars held after the bar the case is opened in, before the alarm goes live (rule: "that first bar is
	 *  held: nothing sees him until the next bar line"). */
	UPROPERTY(EditAnywhere, Category = "Take|Finale", meta = (ClampMin = "0"))
	int32 AlarmHoldBars = 1;

	/** The finale's clock: bars from the alarm going live to the blooper (rule 9, Scene 3: "the last 8 bars"). */
	UPROPERTY(EditAnywhere, Category = "Take|Finale", meta = (ClampMin = "1"))
	int32 FinaleBars = 8;

	UPROPERTY(EditAnywhere, Category = "Take|Finale")
	FString AlarmTitle;	// empty: the HUD's alarm clock already says ALARM

	UPROPERTY(EditAnywhere, Category = "Take|Finale")
	FString AlarmLine = TEXT("VANCE: \"The reel! Seal the hall!\"");

	UPROPERTY(EditAnywhere, Category = "Take|Finale")
	FString AlarmLiveLine = TEXT("Shutters down. The whole floor is red tape. Get him out.");

	UPROPERTY(EditAnywhere, Category = "Take|Finale")
	FString KeepThatInLine = TEXT("DIRECTOR: \"Keep that in!\"");

	/** The line under PRINT IT, by exit. */
	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	FString PrintLineTrap = TEXT("Julian drops through the stage trap. Under the stage, the crew's lorry is already rolling.");

	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	FString PrintLineSide = TEXT("Julian walks out past the band through the gold door, the Director's Cut under his arm.");

	/** How long the PRINT IT slate holds before the stats, s. */
	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	float EndSlateSeconds = 3.f;

	/** How long the stats and the grade hold before the blooper reel, s. */
	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	float EndStatsSeconds = 9.f;

	/** The deleted scenes (rule 10). Empty at BeginPlay: filled with the four of the spec, only The Bow built. */
	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	TArray<FTakeDeletedScene> DeletedScenes;

	/** FINAL CUT needs this many deleted scenes restored (rule 11: "2 or 3"). */
	UPROPERTY(EditAnywhere, Category = "Take|Ends", meta = (ClampMin = "1"))
	int32 FinalCutScenes = 2;

	/** Filled in by Claude while three of the four deleted scenes wait for gate 2: leaving by the gold side exit
	 *  earns FINAL CUT by itself. Off: the spec's rule only. */
	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	bool bSideExitIsFinalCut = true;

	/** Step 4, the master switch: off = no grade and no film overlay (the picture as it was before step 4). */
	UPROPERTY(EditAnywhere, Category = "Take|Look")
	bool bFilmLook = true;

	/** Step 4: on = Maya's and Vex's cameras get the making-of grade and viewfinder; off = all three cameras are the
	 *  film (§15.3's bAllFilm, inverted). The HUD, the labels and the stencils do not change. */
	UPROPERTY(EditAnywhere, Category = "Take|Look")
	bool bMakingOfSplit = true;

	/** Step 4: the grade material used when the volume has none (tools/build_look.py makes it). Its numbers are
	 *  parameters of /Game/Take/Look/MI_FilmGrade: tune them there. */
	UPROPERTY(EditAnywhere, Category = "Take|Look")
	TSoftObjectPtr<UMaterialInterface> FilmGradeMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Take/Look/MI_FilmGrade.MI_FilmGrade")));

	/** Step 4, for checking the look: 0..1 holds the bleed there (1 = the full-colour picture); below 0: off. */
	UPROPERTY(EditAnywhere, Category = "Take|Look", meta = (ClampMax = "1"))
	float ForceBleed = -1.f;

	/** How long the alarm's bleed to full colour takes, s (§15.2 motion.bar: 48 film frames). */
	UPROPERTY(EditAnywhere, Category = "Take|Look", meta = (ClampMin = "0"))
	float BleedSeconds = 2.f;

	// ---------------------------------------------------------------------------------------------------------------
	// The level's actors (gathered at BeginPlay)
	// ---------------------------------------------------------------------------------------------------------------

	/** Every ATakeCharacter in the level, Julian included (A Julian, B Maya, C Vex). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|Level")
	TArray<TObjectPtr<ATakeCharacter>> Characters;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|Level")
	TObjectPtr<AJulian> Julian;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|Level")
	TArray<TObjectPtr<AMark>> Marks;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|Level")
	TArray<TObjectPtr<AWatcher>> Watchers;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|Level")
	TArray<TObjectPtr<ADevice>> Devices;

	/** The first AFollowSpot among Devices. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|Level")
	TObjectPtr<AFollowSpot> FollowSpot;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|Level")
	TArray<TObjectPtr<ARedTapeZone>> TapeZones;

	/** Step 2: gathered in GatherActors (S). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|Level")
	TArray<TObjectPtr<ASceneTrigger>> SceneTriggers;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|Level")
	TArray<TObjectPtr<ACrewPost>> CrewPosts;

	/** Every take actor a sight trace ignores (IsSightBlocked): characters, watchers, devices, marks, tape zones,
	 *  scene triggers. Built at the end of GatherActors (S). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|Level")
	TArray<TObjectPtr<AActor>> SightIgnore;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|Level")
	TObjectPtr<ATakePlayerController> PC;

	/** Step 3: gathered in GatherActors (S). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|Level")
	TArray<TObjectPtr<AShutter>> Shutters;

	/** The level's first unbound post-process volume (GatherActors): StepLook writes its saturation. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|Level")
	TObjectPtr<APostProcessVolume> LookVolume;

	// ---------------------------------------------------------------------------------------------------------------
	// State (read by the HUD and TakeDump; written by the steps named)
	// ---------------------------------------------------------------------------------------------------------------

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	ETakePhase Phase = ETakePhase::Rolling;

	/** Seconds since Phase last changed (slate and stamp animation). Core advances it every tick. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	float PhaseSeconds = 0.f;

	/** The Id of the character the player is on (PC->GetCurrentCharacter()), mirrored by Tick right after
	 *  PC->StepFlight in step 2: it changes on a flight's last frame. The HUD's look reads it. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	ETakeCamera CurrentCamera = ETakeCamera::A;

	/** This frame's use target for the character the player is on (StepUse, step 2). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	FTakeUseTarget CurrentUse;

	/** This frame: Julian is inside red tape (set in step 4 from IsJulianExposed). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	bool bJulianExposed = false;

	/** This frame: a guard frame holds an exposed Julian (StepSight). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	bool bJulianSeen = false;

	/** Julian's feet at the end of the PREVIOUS frame (Tick stores it after step 6; BeginPlay seeds it). His
	 *  movement component moves him before the director ticks, so StepSight tests the segment from here to his feet
	 *  now against instant-blooper watchers, and a long frame cannot carry him through a live beam. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	FVector JulianFeetBeforeWalk = FVector::ZeroVector;

	/** Set by RequestBlooper, consumed by StepBlooperCheck. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	EBlooperReason PendingBlooper = EBlooperReason::None;

	/** The blooper being shown (the slate reads it). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	EBlooperReason LastBlooperReason = EBlooperReason::None;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	float FreezeSecondsLeft = 0.f;

	/** True once a reload has been asked for (so it happens once). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	bool bReloadRequested = false;

	/** The scene Julian is in: 1 The Door, 2 The Floor (StepTriggers, RestoreCheckpoint). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	int32 Scene = 1;

	/** The HUD's goal line (UpdateObjective, every frame). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	FString Objective;

	/** The big scene title and how long it still shows (ShowTitle; Tick counts it down, frozen or not). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	FString Title;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	float TitleSecondsLeft = 0.f;

	/** Seconds the title was shown for (the HUD fades it over the last part). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	float TitleSeconds = 0.f;

	/** Every trigger fired this take, with the bar it fired on (FireTrigger; RestoreCheckpoint refills it). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	TMap<FName, int32> FiredTriggers;

	/** How many watchers the last device landing caught (StepDevices sets it; 2 or more flashes "x2", "x3"). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	int32 LastStampCount = 0;

	/** The HUD's flash lines, oldest first. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	TArray<FTakeFlash> Flashes;

	/** The last flash's serial. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	int32 FlashSerial = 0;

	/** The title showing is the opening slate (a fresh take), not a scene title. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	bool bTitleIsSlate = false;

	// --- step 3 state (read by the HUD) -------------------------------------------------------------------------------

	/** The case is open: the alarm sounds (StartAlarm). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	bool bAlarm = false;

	/** The hold is over: the whole floor is red tape, the guards are in, the drone follows him, the clock runs. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	bool bAlarmLive = false;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	int32 AlarmLiveBar = 0;

	/** The bar line that is the blooper. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	int32 AlarmDeadlineBar = 0;

	/** Printed: he left by the gold side exit (false: the stage trap). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	bool bSideExit = false;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	ETakeGrade Grade = ETakeGrade::RoughCut;

	/** Printed: the take's length on the band's clock, s (the bloopers are spliced out of it), and its last bar. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	float FinishSeconds = 0.f;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	int32 FinishBar = 0;

	/** Printed: the line under the slate (PrintLineTrap or PrintLineSide). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	FString PrintLine;

	/** Printed: 0 the PRINT IT slate, 1 the stats and the grade, 2 the blooper reel and the credits. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	int32 EndPage = 0;

	/** Seconds on the current end page. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	float EndPageSeconds = 0.f;

	/** The saturation StepLook last wrote. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	/** Step 4: the bleed's linear progress 0..1 (the material gets its smoothstep), and what GradeMID last got. */
	float CurrentBleed = 0.f;
	float AppliedBleed = -1.f;
	float AppliedMakingOf = -1.f;
	int32 AppliedFilmLook = -1;

	/** Step 4: the dynamic instance of the grade material in LookVolume's blendables (SetupLook). */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GradeMID;

	/** Sets Phase and zeroes PhaseSeconds, logs "PHASE -> X". */
	void SetPhase(ETakePhase NewPhase);

protected:
	/** Finds the local ATakePlayerController the first time it exists: the director ticks after it, and so does every
	 *  character and its movement component (AddTickPrerequisiteActor); then PC->SnapTo(Julian), the take's start
	 *  view. Called from BeginPlay and every Tick. */
	void EnsurePlayerController();

	/** Fades and removes flash lines (core, every tick, frozen or not). */
	void StepFlashes(float DeltaSeconds);

	FTakeDirectorLateTick LateTickFunction;

	bool bInitialViewSet = false;

	/** RestoreCheckpoint has run. It runs at the start of the director's FIRST Tick, not in BeginPlay: actors begin
	 *  play in no fixed order, and a device's or watcher's own BeginPlay would undo a restore made before it. The
	 *  first tick comes before the first rendered frame. */
	bool bCheckpointRestored = false;

	/** The last Objective logged (UpdateObjective logs changes only). */
	FString LastLoggedObjective;
};
