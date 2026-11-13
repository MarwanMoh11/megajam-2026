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
// Tick order (docs/STATUS.md "Tick order", as reworked). Input and walking happen BEFORE the director in the same
// frame: the player controller ticks first (presses queued, WASD -> Move, mouse -> AddLook), then every character
// and its movement component (they move), then the director (tick prerequisites on the PC and on every character's
// movement component, set in EnsurePlayerController / BeginPlay):
//   1 the bar clock           Clock->Advance
//   2 the flight, use, presses PC->StepFlight, CurrentCamera mirrors PC->GetCurrentCharacter(), StepUse (CurrentUse
//                             and the highlights), then StepPresses (cuts first, then uses, then a new take)
//   3 devices                 StepDevices
//   4 marks and sight         Julian->StepMarks, bJulianExposed = IsJulianExposed(), every Watcher->StepWatcher,
//                             then StepSight
//   5 composure               StepComposure
//   6 the scene               StepScene; then JulianFeetBeforeWalk = Julian's feet (for next frame's swept beam test)
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

	UPROPERTY()
	FLinearColor Colour = FLinearColor::White;

	/** Seconds still to show. */
	UPROPERTY()
	float SecondsLeft = 0.f;

	/** Seconds it was shown for (the HUD may fade over the last part). */
	UPROPERTY()
	float Seconds = 0.f;
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
	void Flash(const FString& Text, const FLinearColor& Colour = FLinearColor::White, float Seconds = 2.f);

	/** The level's character with this Id (the first found), null if the set has none (no Vex in the test box). */
	ATakeCharacter* GetCharacter(ETakeCamera Id) const;

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

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|Level")
	TObjectPtr<ATakePlayerController> PC;

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

	/** The HUD's flash lines, oldest first. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	TArray<FTakeFlash> Flashes;

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
};
