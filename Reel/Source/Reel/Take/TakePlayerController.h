// The player: on one character at a time (WASD, mouse-look, Use), keys 1/2/3 switch with a flight. Owner: (s).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TakeTypes.h"
#include "TakePlayerController.generated.h"

class ACameraActor;
class ATakeCharacter;

/**
 * No pawn is possessed (ATakeGameMode has no default pawn). The controller feeds the character it is on (Current)
 * directly: walking and looking every frame in PlayerTick, presses through the queue.
 *
 * Input: legacy key bindings made in C++ (UInputComponent::BindKey, processed by UEnhancedPlayerInput through
 * Super::EvaluateInputComponentDelegates), so no input assets are needed:
 *   One/Two/Three -> QueueCut(A/B/C); E and LeftMouseButton -> QueueUse; R -> NewTake press; Escape -> quit.
 *   W/A/S/D are read with IsInputKeyDown and the mouse with GetInputMouseDelta in PlayerTick (not queued: they are
 *   continuous, not presses).
 * Presses are only queued here (input runs in this controller's tick, TG_PrePhysics); ATakeDirector ticks after
 * this controller and every character's movement component, and consumes the queue in the same frame. Exec test
 * commands that are presses (TakeCut, TakeUse, TakeNewTake) go through the same queue.
 *
 * The view: the view target is always either Current (its follow camera, found through
 * bFindCameraComponentWhenViewTarget) or FlightCamera during a flight. bAutoManageActiveCameraTarget = false.
 */
UCLASS()
class REEL_API ATakePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATakePlayerController();

	/** Local only: input mode game-only (FInputModeGameOnly), cursor hidden (SetShowMouseCursor(false)), focus to the
	 *  game viewport; spawns FlightCamera (an ACameraActor, bConstrainAspectRatio false). */
	virtual void BeginPlay() override;	// BODY: (s)

	/** Binds One, Two, Three, E, LeftMouseButton, R and Escape with InputComponent->BindKey (IE_Pressed). */
	virtual void SetupInputComponent() override;	// BODY: (s)

	/**
	 * Super first (the bindings fire in there and only queue presses). Then, when Current is set and !IsInFlight():
	 *  - walk: Axes = (W - S, D - A) from IsInputKeyDown; non-zero -> Current->Move(Axes) and cancel any test walk;
	 *  - look: GetInputMouseDelta(DX, DY) -> Current->AddLook(DX * LookSensitivity, DY * LookSensitivity * (bInvertLookY
	 *    ? -1 : 1)) (mouse up looks up);
	 *  - then the test walk, if one is active: within WalkToTolerance (2D, feet) of WalkToPoint -> stop and TAKE_LOG
	 *    ("WALKTO arrived x y"); else Current->MoveToward(WalkToPoint).
	 * The movement component ticks after this controller, so the move happens this frame.
	 */
	virtual void PlayerTick(float DeltaTime) override;	// BODY: (s)

	// --- the press queue ------------------------------------------------------------------------------------------

	/** Appends a press (stamps Frame = GFrameCounter) and TAKE_LOGs it ("PRESS cut B (key)", "PRESS use (mouse)",
	 *  "PRESS new take (key)"). */
	void QueuePress(const FTakePress& Press);	// BODY: (s)

	/** Moves every queued press into Out (oldest first) and empties the queue. Called by the director in step 2. */
	void TakePresses(TArray<FTakePress>& Out);	// BODY: (s)

	// --- the character and the view -------------------------------------------------------------------------------

	/** The character the player is on (null before the director's first SnapTo). During a flight it is still the
	 *  OLD character; it becomes the new one on the flight's last frame. */
	ATakeCharacter* GetCurrentCharacter() const { return Current; }

	/** No flight (the take's start, called by ATakeDirector::EnsurePlayerController with Julian): cancels any flight
	 *  and test walk, Current = To, SetViewTarget(To) with no blend (this frame), TAKE_LOG("VIEW -> A (snap)").
	 *  Null does nothing. */
	void SnapTo(ATakeCharacter* To);	// BODY: (s)

	/**
	 * The switch (GTA V's is the reference). Called by the director in step 2 on a valid cut press (never during a
	 * flight, never to Current). On this frame: FlightTo = To, FlightElapsed = 0, the start view = Current's follow
	 * view (GetFollowView), FlightCamera placed there with its FOV, SetViewTarget(FlightCamera) with no blend, the
	 * test walk cancelled, TAKE_LOG("FLIGHT A -> B"). WASD and look are ignored until it lands.
	 */
	void StartFlight(ATakeCharacter* To);	// BODY: (s)

	/**
	 * Called by the director every frame at the very start of step 2 (before the presses), in a flight or not (does
	 * nothing when not). Advances FlightElapsed by Dt and places FlightCamera on this frame's point of the path:
	 *  1 rise (the first FlightRiseShare of FlightSeconds): from the start view up to FlightHeight straight above the
	 *    old character, looking straight down (pitch -90), in FlightSteps stepped zooms (each step a quick ease, then
	 *    a short hold, like a jump cut outward);
	 *  2 pan (the next FlightPanShare): at FlightHeight, from above the old character to above the new one, smooth
	 *    ease in-out, still looking down;
	 *  3 drop (the rest): FlightSteps stepped zooms down into the new character's follow view (location, rotation
	 *    and FOV read from FlightTo->GetFollowView each frame).
	 * On the frame FlightElapsed reaches FlightSeconds: Current = FlightTo, FlightTo = null, SetViewTarget(Current)
	 * with no blend, TAKE_LOG("VIEW -> B").
	 */
	void StepFlight(float Dt);	// BODY: (s)

	/** A flight is running (FlightTo is set). */
	bool IsInFlight() const { return FlightTo != nullptr; }

	/** Where the running flight lands (the HUD's "CUT -> MAYA"); null when not in a flight. */
	ATakeCharacter* GetFlightTarget() const { return FlightTo; }

	/** 0..1 through the running flight (FlightElapsed / FlightSeconds); 0 when not in a flight. */
	float GetFlightAlpha() const;	// BODY: (s)

	// --- exec test commands (console, or unreal.SystemLibrary.execute_console_command in PIE) ----------------------

	/** "TakeCut B" / "TakeCut 2": queues a cut exactly like the key. */
	UFUNCTION(Exec)
	void TakeCut(const FString& Camera);	// BODY: (s)

	/** "TakeUse": queues a Use press exactly like E. */
	UFUNCTION(Exec)
	void TakeUse();	// BODY: (s)

	/** "TakeAim GUARD": not a press, acts at once. Finds the actor whose DeviceName, watcher Label, MarkName, object
	 *  name or editor label (WITH_EDITOR) matches (case-insensitive, in that order) and SetLooks Current so the
	 *  follow camera faces it: yaw and pitch of (target - Current->GetAimOrigin()), the target being a watcher's
	 *  GetAimPoint(), else the actor's bounds centre (GetActorBounds). The boom swings with the look, so repeat the
	 *  solve 3 times from the new origin (the boom's lag: set it with the boom snapped, or accept ~1 deg).
	 *  TAKE_LOG("AIM <name> yaw p").
	 *  Unknown name or no Current: logs and does nothing. */
	UFUNCTION(Exec)
	void TakeAim(const FString& ActorNameOrLabel);	// BODY: (s)

	/** "TakeLook 90 -10": Current->SetLook(Yaw, Pitch) at once. TAKE_LOG("LOOK yaw pitch"). */
	UFUNCTION(Exec)
	void TakeLook(float Yaw, float Pitch);	// BODY: (s)

	/** "TakeWalkTo 120 -340": Current walks straight at (X, Y) with MoveToward every PlayerTick until its feet are
	 *  within WalkToTolerance (2D). Cancelled by WASD input, a flight (StartFlight / SnapTo) or TakeStop.
	 *  TAKE_LOG("WALKTO x y"). */
	UFUNCTION(Exec)
	void TakeWalkTo(float X, float Y);	// BODY: (s)

	/** "TakeStop": cancels the test walk (TAKE_LOG("WALKTO stopped")). */
	UFUNCTION(Exec)
	void TakeStop();	// BODY: (s)

	/** "TakeNewTake": queues R. */
	UFUNCTION(Exec)
	void TakeNewTake();	// BODY: (s)

	/** "TakeDump": one TAKE_LOG line: phase, current character and flight, composure and label, every character's
	 *  feet / look / standing, Julian's standing mark, the use target and prompt, each watcher's label/state/tally,
	 *  each device's uses left / busy time / in reach. */
	UFUNCTION(Exec)
	void TakeDump();	// BODY: (s)

	/** "TakeShot name": FScreenshotRequest::RequestScreenshot(name, bShowUI = true, no suffix), so the HUD is in the
	 *  PNG (Saved/Screenshots/...). Logs the path. */
	UFUNCTION(Exec)
	void TakeShot(const FString& Name);	// BODY: (s)

	// --- step 2 test commands (BODY: D) -------------------------------------------------------------------------------

	/** "TakeWarp 900 0 0": test only. Puts the feet of the character the player is on at (X, Y, Z) at once (teleport,
	 *  velocity zeroed, follow camera snapped, the test walk cancelled). TAKE_LOG("WARP <who> x y z"). */
	UFUNCTION(Exec)
	void TakeWarp(float X, float Y, float Z);	// BODY: D

	/** "TakeTrigger Scene2": test only. ATakeDirector::FireTrigger(Name) at once. */
	UFUNCTION(Exec)
	void TakeTrigger(const FString& Name);	// BODY: D

	/** "TakeBlooper": test only. ATakeDirector::RequestBlooper(ComposureZero): the blooper fires at the end of this
	 *  frame, so a checkpoint restore can be tested without dying. */
	UFUNCTION(Exec)
	void TakeBlooper();	// BODY: D

	// --- step 3 (BODY: D) ------------------------------------------------------------------------------------------

	/** "TakeConfirm": queues a Confirm press exactly like Enter or Space (starts the take from the title card, turns
	 *  the end sequence's page). */
	UFUNCTION(Exec)
	void TakeConfirm();

	// --- tunables ---------------------------------------------------------------------------------------------------

	/** The whole flight, s (under 2 s: STATUS.md). */
	UPROPERTY(EditAnywhere, Category = "Take|Flight")
	float FlightSeconds = 1.6f;

	/** The flight's top height above the higher of the two characters, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Flight")
	float FlightHeight = 3500.f;

	/** Stepped zooms on the way up, and again on the way down. */
	UPROPERTY(EditAnywhere, Category = "Take|Flight")
	int32 FlightSteps = 3;

	/** Share of FlightSeconds spent rising (0..1). */
	UPROPERTY(EditAnywhere, Category = "Take|Flight", meta = (ClampMin = "0", ClampMax = "1"))
	float FlightRiseShare = 0.35f;

	/** Share of FlightSeconds spent panning across at the top (0..1); the drop takes what is left. */
	UPROPERTY(EditAnywhere, Category = "Take|Flight", meta = (ClampMin = "0", ClampMax = "1"))
	float FlightPanShare = 0.3f;

	/** Degrees of look per unit of mouse delta. GetInputMouseDelta is already scaled by the engine (BaseInput.ini:
	 *  MouseX/MouseY Sensitivity 0.07 per count), so 2.5 is about 0.175 deg per mouse count, the UE template's feel. */
	UPROPERTY(EditAnywhere, Category = "Take|Look")
	float LookSensitivity = 2.5f;

	/** Mouse up looks down. */
	UPROPERTY(EditAnywhere, Category = "Take|Look")
	bool bInvertLookY = false;

	/** TakeWalkTo stops this close to its point (2D, feet), cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Test")
	float WalkToTolerance = 10.f;

	/** The camera the flight flies (spawned in BeginPlay). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|Flight")
	TObjectPtr<ACameraActor> FlightCamera;

	// --- QA scene1: the director's glance, the look pan, the flight on real time (QA notes 2, 10) -------------------

	/** The camera leaves the follow view of the character the player is on and turns (zooming to GlanceFOV) to frame
	 *  Target over GlanceSeconds of real time, then holds there. It hands the view back at once on WASD (that frame's
	 *  walk still happens), on mouse movement past GlanceMouseBreak, on [1] for the character it is on (the
	 *  director), on EndGlance or SnapTo, or after GlanceMaxSeconds; a cut's flight starts from the glance's view.
	 *  Does nothing in a flight or on no one. Flies FlightCamera. */
	void StartGlance(const FVector& Target);
	void EndGlance();
	bool IsGlancing() const { return bGlance; }

	/** Eases the look of the character the player is on onto Target over Seconds (real time). WASD or the mouse past
	 *  GlanceMouseBreak hands the look back on that frame; a flight or SnapTo ends it. */
	void StartLookPan(const FVector& Target, float Seconds);
	bool IsLookPanning() const { return bLookPan; }

	/** This frame's real (undilated) seconds, clamped to 0..0.1: what runs on it is not slowed by a slowed world. */
	float GetRealDeltaSeconds() const;

	/** The glance's turn, s (real time). */
	UPROPERTY(EditAnywhere, Category = "Take|Glance", meta = (ClampMin = "0"))
	float GlanceSeconds = 1.1f;

	/** The glance zooms to this FOV, degrees. */
	UPROPERTY(EditAnywhere, Category = "Take|Glance", meta = (ClampMin = "5", ClampMax = "120"))
	float GlanceFOV = 40.f;

	/** The glance hands the view back by itself after this long, s (real time). */
	UPROPERTY(EditAnywhere, Category = "Take|Glance", meta = (ClampMin = "0"))
	float GlanceMaxSeconds = 12.f;

	/** Mouse movement (summed GetInputMouseDelta units) that takes the camera back from a glance or a look pan. */
	UPROPERTY(EditAnywhere, Category = "Take|Glance", meta = (ClampMin = "0"))
	float GlanceMouseBreak = 6.f;

	/** The flight runs on real time, so a slowed world (the taught first switch) does not stretch it. Off: world
	 *  time, as before the QA pass. */
	UPROPERTY(EditAnywhere, Category = "Take|Flight")
	bool bFlightOnRealTime = true;

	/** While the world is slowed (ATakeDirector::IsSlowed), the flight runs at this share of real time: a touch slower,
	 *  so the switch is shown while the line plays. */
	UPROPERTY(EditAnywhere, Category = "Take|Flight", meta = (ClampMin = "0.1", ClampMax = "1"))
	float SlowedFlightRate = 0.8f;

	// --- end QA scene1 ---

protected:
	void OnKeyOne();	// BODY: (s)
	void OnKeyTwo();	// BODY: (s)
	void OnKeyThree();	// BODY: (s)

	/** E or the left mouse button: QueueUse. */
	void OnUse();	// BODY: (s)

	/** The left mouse button: QueueUse("mouse") (OnUse is E, "key"). Added by (s). */
	void OnUseMouse();

	/** FlightCamera, spawned (transient, aspect ratio unconstrained) if it does not exist yet. Added by (s). */
	ACameraActor* EnsureFlightCamera();

	/** Puts FlightCamera at Location / Rotation with FOV (this frame). Added by (s). */
	void PlaceFlightCamera(const FVector& Location, const FQuat& Rotation, float FOV);

	/** The flight's last frame: Current = FlightTo, FlightTo = null, SetViewTarget(Current) with no blend,
	 *  TAKE_LOG("VIEW -> B"). Added by (s). */
	void LandFlight();

	/** Share of each stepped zoom spent on its quick eased jump (0..1); the rest is the hold before the next.
	 *  Added by (s). */
	UPROPERTY(EditAnywhere, Category = "Take|Flight", meta = (ClampMin = "0.05", ClampMax = "1"))
	float FlightStepJumpShare = 0.4f;
	void OnKeyR();	// BODY: (s)
	void OnEscape();	// BODY: (s)

	/** Enter or the space bar: queues a Confirm press ("key"). Bound in SetupInputComponent (step 3, D). */
	void OnConfirm();

	/** Builds and queues a Cut press. */
	void QueueCut(ETakeCamera Camera, const TCHAR* Source);	// BODY: (s)

	/** Builds and queues a Use press. */
	void QueueUse(const TCHAR* Source);	// BODY: (s)

	UPROPERTY(Transient)
	TArray<FTakePress> PendingPresses;

	/** The character the player is on. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	TObjectPtr<ATakeCharacter> Current;

	/** The running flight's destination (null = no flight). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	TObjectPtr<ATakeCharacter> FlightTo;

	/** Seconds into the running flight. */
	float FlightElapsed = 0.f;

	/** The view the flight started from (Current's follow view on the StartFlight frame). */
	FVector FlightStartLocation = FVector::ZeroVector;
	FRotator FlightStartRotation = FRotator::ZeroRotator;
	float FlightStartFOV = 90.f;

	/** The test walk (TakeWalkTo). */
	bool bWalkTo = false;
	FVector WalkToPoint = FVector::ZeroVector;

	// --- QA scene1: the glance and the look pan ---
	void StepGlance(float RealDt);
	void StepLookPan(float RealDt);

	bool bGlance = false;
	float GlanceElapsed = 0.f;
	float GlanceMouse = 0.f;
	FVector GlanceTarget = FVector::ZeroVector;
	FVector GlanceFromLocation = FVector::ZeroVector;
	FRotator GlanceFromRotation = FRotator::ZeroRotator;
	float GlanceFromFOV = 90.f;

	bool bLookPan = false;
	float LookPanElapsed = 0.f;
	float LookPanDuration = 0.f;
	float LookPanMouse = 0.f;
	FVector LookPanTarget = FVector::ZeroVector;
	float LookPanFromYaw = 0.f;
	float LookPanFromPitch = 0.f;
	// --- end QA scene1 ---
};
