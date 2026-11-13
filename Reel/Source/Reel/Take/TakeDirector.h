// ATakeDirector: the rules. One per level (ATakeGameMode spawns one if the map has none).
//
// Owner of this header and TakeDirector.cpp: header pass (core, complete).
// The step functions are members of this class defined in the slice files:
//   (a) TakeDirectorComposure.cpp  IsJulianExposed, StepComposure
//   (b) TakeDirectorPresses.cpp    StepPresses, HandleCut, HandleClick, StepShotList
//   (c) TakeDirectorScene.cpp      StepScene
//   (d) TakeDirectorSight.cpp      StepSight
//   (e) TakeDirectorDevices.cpp    FireDevice, SpotWatcher, StepDevices
//   (e) TakeDirectorBlooper.cpp    RequestBlooper, StepBlooperCheck, StepFreeze, ReloadLevel, NewTake
//
// Tick order (docs/STATUS.md "Tick order"); the director ticks in TG_PrePhysics AFTER the player controller
// (tick prerequisite), so presses queued during this frame's input handling are consumed this frame:
//   1 the bar clock           Clock->Advance
//   2 presses                 StepPresses (cuts first, then clicks)
//   3 devices                 StepDevices
//   4 Julian's walk           Julian->StepWalk, then StepShotList, then bJulianExposed = IsJulianExposed(),
//                             then every Watcher->StepWatcher, then StepSight
//   5 composure               StepComposure
//   6 the scene               StepScene
//   7 late tick (TG_PostUpdateWork, a second tick function): StepBlooperCheck
// While Phase == BlooperFreeze only StepFreeze runs (presses are drained and dropped). Cleared keeps stepping.
// The camera manager updates after TG_PostPhysics, so a view-target change made in steps 2 or 4 renders this frame.
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
class AShotZone;
class ATakeCamera;
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
	// Core (TakeDirector.cpp, complete)
	// ---------------------------------------------------------------------------------------------------------------

	/** Gathers the level's actors, picks camera A's first shot, cuts to it, refills composure, logs ROLLING. */
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
	 *  MaxFlashes lines are kept; the oldest goes first. Use for prompts ("[1] to cue Julian") and refusals. */
	void Flash(const FString& Text, const FLinearColor& Colour = FLinearColor::White, float Seconds = 2.f);

	/** The camera actor a cut to Id shows: camera A's current shot for A, else the first ATakeCamera with that id.
	 *  Null if the level has none (a cut then flashes a line instead). */
	ATakeCamera* FindCamera(ETakeCamera Id) const;

	/** The game instance that survives reloads (take number, bloopers). Null outside a Take game instance. */
	UTakeGameInstance* GetTakeGameInstance() const;

	/** "bar.beat" for TAKE_LOG. */
	FString MakeLogStamp() const;

	/** Finds every Julian, mark, watcher, device, tape zone, shot zone and take camera in the level (TActorIterator). */
	void GatherActors();

	// ---------------------------------------------------------------------------------------------------------------
	// (a) TakeDirectorComposure.cpp
	// ---------------------------------------------------------------------------------------------------------------

	/** True while Julian stands inside any red tape zone (rule 5; the drone rule is not in step 1). Pure query. */
	bool IsJulianExposed() const;

	/** Step 5. Composure->Step(Dt, bJulianSeen, bOnMarkUnexposed) where bOnMarkUnexposed is Julian standing on a
	 *  mark (Julian->GetStandingMark()) and !bJulianExposed. Logs label changes (SEEN, MARK, none) through TAKE_LOG.
	 *  Does not fire the blooper: StepBlooperCheck reads Composure->IsEmpty() in the late tick. */
	void StepComposure(float DeltaSeconds);

	// ---------------------------------------------------------------------------------------------------------------
	// (b) TakeDirectorPresses.cpp
	// ---------------------------------------------------------------------------------------------------------------

	/** Step 2. Drains PC's press queue (PC->TakePresses). Handles every Cut first (in queue order), then every
	 *  Click, then NewTake (calls NewTake(), which reloads). Each handler changes what is rendered inside the call. */
	void StepPresses();

	/** A cut to Camera: if FindCamera(Camera) is null, Flash("No camera C on this set") and nothing else changes;
	 *  otherwise CurrentCamera = Camera and PC->CutTo(that camera) (hard cut, view target swapped this frame; the
	 *  HUD's look changes with CurrentCamera). TAKE_LOG("CUT -> B"). */
	void HandleCut(ETakeCamera Camera, const FTakePress& Press);

	/** A click, routed by rules 3 and 4 on CurrentCamera:
	 *  - on A, a Mark: Julian->CueTo(mark) (ring, dotted path, count, Julian turns: all this frame); no path ->
	 *    Flash("Julian can't get there").
	 *  - on A, a Device or a Watcher: fires nothing; Flash("[2] to fire it") / Flash("[2] to spot him") naming the
	 *    camera that can act (the device's FiredFrom; the follow spot's FiredFrom for a watcher).
	 *  - on B/C, a Device whose FiredFrom == CurrentCamera: FireDevice(device). Another camera's device: Flash("[3] to fire it").
	 *  - on B/C, a Watcher: if FollowSpot && FollowSpot->FiredFrom == CurrentCamera, SpotWatcher(watcher); else
	 *    Flash("[2] to spot him").
	 *  - on B/C, a Mark: Flash("[1] to cue Julian").
	 *  - nothing clickable: Flash("[1] to cue Julian" on B/C, "Click a gold mark" on A).
	 *  Logs the press and the outcome with TAKE_LOG. */
	void HandleClick(const FTakePress& Press);

	/** Step 4, after Julian's walk: the film's own cut. Finds the AShotZone containing Julian's feet (highest
	 *  Priority wins); if its shot differs from CurrentShotA, CurrentShotA = that shot and, if CurrentCamera == A,
	 *  PC->CutTo(CurrentShotA) at once. Not the player's cut: no flash. TAKE_LOG("SHOT -> <name>"). */
	void StepShotList();

	// ---------------------------------------------------------------------------------------------------------------
	// (c) TakeDirectorScene.cpp
	// ---------------------------------------------------------------------------------------------------------------

	/** Step 6. While Rolling, Julian standing on a bGoal mark: Phase = Cleared, PhaseSeconds = 0 (the HUD shows the
	 *  CLEAR stamp from this frame), Flash("CLEAR - that's the one"), TAKE_LOG("CLEARED on <mark>"). */
	void StepScene(float DeltaSeconds);

	// ---------------------------------------------------------------------------------------------------------------
	// (d) TakeDirectorSight.cpp
	// ---------------------------------------------------------------------------------------------------------------

	/** Step 4, after the watchers stepped. bJulianSeen = false; for each watcher:
	 *  - IsInstantBlooper() (the beam): if HoldsJulian(Julian) -> RequestBlooper(EBlooperReason::BeamTouched).
	 *  - otherwise (the guard): bTally = HoldsJulian(Julian) && bJulianExposed; W->SetTally(bTally) (frame solid red
	 *    and tally light on this frame); bJulianSeen |= bTally.
	 *  Logs tally changes ("GUARD frames Julian" / "GUARD lost him"). */
	void StepSight(float DeltaSeconds);

	// ---------------------------------------------------------------------------------------------------------------
	// (e) TakeDirectorDevices.cpp
	// ---------------------------------------------------------------------------------------------------------------

	/** Step 2 (from HandleClick): fires Device if it can (Device->CanFire), which spends a use and starts its
	 *  timeline with a visible change this frame (pip out, cart starts rolling, banner starts dropping). If it
	 *  can't: Flash the reason ("CART - used", "Click a watcher to spot it") and return false. TAKE_LOG either way. */
	bool FireDevice(ADevice* Device);

	/** Step 2 (from HandleClick): the follow spot on Watcher. FollowSpot->FireAt(Watcher, WhyNot); out of range or
	 *  busy -> Flash(WhyNot) the same frame and return false. On success the beam shape shows this frame and the
	 *  Blind lands in StepDevices this frame. */
	bool SpotWatcher(AWatcher* Watcher);

	/** Step 3. Every device: StepDevice(Dt) (timelines). A device whose LandedThisFrame() is true applies its effect
	 *  to every watcher it GatherTargets() (in range of its effect point; the spot: its one target):
	 *  W->ApplyEffect(Effect, Duration, EffectPoint). Logs "CART lands: lured 1 (GUARD)". */
	void StepDevices(float DeltaSeconds);

	// ---------------------------------------------------------------------------------------------------------------
	// (e) TakeDirectorBlooper.cpp
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

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|Level")
	TObjectPtr<AJulian> Julian;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|Level")
	TArray<TObjectPtr<AMark>> Marks;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|Level")
	TArray<TObjectPtr<AWatcher>> Watchers;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|Level")
	TArray<TObjectPtr<ADevice>> Devices;

	/** The first AFollowSpot among Devices: clicking a watcher on its FiredFrom camera spots it. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|Level")
	TObjectPtr<AFollowSpot> FollowSpot;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|Level")
	TArray<TObjectPtr<ARedTapeZone>> TapeZones;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|Level")
	TArray<TObjectPtr<AShotZone>> ShotZones;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|Level")
	TArray<TObjectPtr<ATakeCamera>> Cameras;

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

	/** The camera the player is on (written by HandleCut). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	ETakeCamera CurrentCamera = ETakeCamera::A;

	/** Camera A's current shot (written by StepShotList; BeginPlay picks the bInitialShot camera). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	TObjectPtr<ATakeCamera> CurrentShotA;

	/** The shot zone Julian is in (StepShotList). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	TObjectPtr<AShotZone> CurrentShotZone;

	/** This frame: Julian is inside red tape (set in step 4 from IsJulianExposed). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	bool bJulianExposed = false;

	/** This frame: a guard frame holds an exposed Julian (StepSight). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	bool bJulianSeen = false;

	/** Julian's feet at the start of this frame's step 4, before StepWalk. StepSight tests the segment from here to
	 *  his feet now against instant-blooper watchers, so a long frame cannot carry him through a live beam. */
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
	/** Finds the local ATakePlayerController the first time it exists, adds it as a tick prerequisite (so the
	 *  director ticks after its input) and cuts to camera A's first shot. Called from BeginPlay and every Tick. */
	void EnsurePlayerController();

	/** Fades and removes flash lines (core, every tick, frozen or not). */
	void StepFlashes(float DeltaSeconds);

	FTakeDirectorLateTick LateTickFunction;

	bool bInitialViewSet = false;
};
