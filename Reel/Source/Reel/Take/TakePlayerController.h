// The player is the director: a pointer on every camera, keys 1/2/3 to cut, left click, R, ESC. Owner: (b).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TakeTypes.h"
#include "TakePlayerController.generated.h"

class ATakeCamera;

/**
 * Input: legacy key bindings made in C++ (UInputComponent::BindKey exists in 5.9 and is processed by
 * UEnhancedPlayerInput through Super::EvaluateInputComponentDelegates), so no input assets are needed:
 *   One/Two/Three -> QueueCut(A/B/C), LeftMouseButton -> QueueClickAt(pointer), R -> NewTake press, Escape -> quit.
 * Presses are only queued here (input runs in this controller's tick, TG_PrePhysics); ATakeDirector ticks after
 * this controller and consumes the queue in the same frame. Exec test commands go through the same queue.
 * The mouse pointer is always shown and never captured. No pawn is possessed.
 */
UCLASS()
class REEL_API ATakePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATakePlayerController();

	/** Cursor shown, input mode game-and-UI with the cursor never hidden or locked. */
	virtual void BeginPlay() override;	// BODY: (b)

	/** Binds One, Two, Three, LeftMouseButton, R and Escape with InputComponent->BindKey (IE_Pressed). */
	virtual void SetupInputComponent() override;	// BODY: (b)

	/** Every frame: traces under the pointer (TraceClickable) and moves the hover highlight (ITakeClickable::
	 *  SetHovered false on the old target, true on the new) on this frame. */
	virtual void PlayerTick(float DeltaTime) override;	// BODY: (b)

	// --- the press queue ------------------------------------------------------------------------------------------

	/** Appends a press (stamps Frame = GFrameCounter) and TAKE_LOGs it ("PRESS cut B (key)"). */
	void QueuePress(const FTakePress& Press);	// BODY: (b)

	/** Moves every queued press into Out (oldest first) and empties the queue. Called by the director in step 2. */
	void TakePresses(TArray<FTakePress>& Out);	// BODY: (b)

	// --- the view -------------------------------------------------------------------------------------------------

	/** Hard cut: SetViewTarget(Camera) with no blend. The camera manager updates after TG_PostPhysics, so a call from
	 *  the director's tick renders this frame. Logs "VIEW -> <camera>". Null does nothing. */
	void CutTo(ATakeCamera* Camera);	// BODY: (b)

	/** The take camera currently viewed (null before the first cut). */
	ATakeCamera* GetViewCamera() const;	// BODY: (b)

	// --- the pointer ----------------------------------------------------------------------------------------------

	/** Traces ECC_Visibility from viewport pixel ScreenPos through every clickable actor under the pointer (up to 8,
	 *  stopping at a wall or the floor) and picks the one the current camera acts on: on A the nearest mark, on B
	 *  and C the nearest device or watcher, otherwise the nearest clickable. True with that actor in OutActor;
	 *  OutHit holds its hit. */
	bool TraceClickable(const FVector2D& ScreenPos, AActor*& OutActor, FHitResult& OutHit) const;	// BODY: (b)

	/** The clickable actor under the pointer this frame (null if none). */
	AActor* GetHoverTarget() const;	// BODY: (b)

	// --- exec test commands (console, or unreal.SystemLibrary.execute_console_command in PIE) ----------------------

	/** "TakeCut A" / "TakeCut 2": queues a cut exactly like the key. */
	UFUNCTION(Exec)
	void TakeCut(const FString& Camera);	// BODY: (b)

	/** "TakeClick Mark_Rope": queues a click on the actor whose name or editor label matches (label only WITH_EDITOR),
	 *  as if the pointer were on it. Unknown name: logs and queues nothing. */
	UFUNCTION(Exec)
	void TakeClick(const FString& ActorNameOrLabel);	// BODY: (b)

	/** "TakeClickAt 640 360": queues a click at that viewport pixel (traced like the mouse). */
	UFUNCTION(Exec)
	void TakeClickAt(float X, float Y);	// BODY: (b)

	/** "TakeNewTake": queues R. */
	UFUNCTION(Exec)
	void TakeNewTake();	// BODY: (b)

	/** "TakeDump": one TAKE_LOG line: phase, camera, shot, composure and label, Julian's position, cued mark and beats
	 *  to arrival, each watcher's label/state/tally, each device's uses left / busy time. */
	UFUNCTION(Exec)
	void TakeDump();	// BODY: (b)

	/** "TakeShot name": FScreenshotRequest::RequestScreenshot(name, bShowUI = true, no suffix), so the HUD is in the
	 *  PNG (Saved/Screenshots/...). Logs the path. */
	UFUNCTION(Exec)
	void TakeShot(const FString& Name);	// BODY: (b)

	/** How far the pointer trace reaches, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Pointer")
	float ClickTraceDistance = 100000.f;

protected:
	void OnKeyOne();	// BODY: (b)
	void OnKeyTwo();	// BODY: (b)
	void OnKeyThree();	// BODY: (b)
	void OnLeftClick();	// BODY: (b)
	void OnKeyR();	// BODY: (b)
	void OnEscape();	// BODY: (b)

	/** Builds and queues a Cut press. */
	void QueueCut(ETakeCamera Camera, const TCHAR* Source);	// BODY: (b)

	/** Traces at ScreenPos and queues a Click press with the clickable actor (or none). */
	void QueueClickAt(const FVector2D& ScreenPos, const TCHAR* Source);	// BODY: (b)

	UPROPERTY(Transient)
	TArray<FTakePress> PendingPresses;

	TWeakObjectPtr<AActor> HoverTarget;
};
