// A gold floor mark: a place to stand. Standing still on one is MARK; the goal mark clears the test. Owner: (m).
// Since the Nov 13 rework marks are no longer clicked (no cue ring, no click volume): AJulian::StepMarks finds them.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Mark.generated.h"

class UStaticMeshComponent;

/** Place it on the floor (actor location = the floor point Julian stands on). Its shapes are NoCollision. */
UCLASS()
class REEL_API AMark : public AActor
{
	GENERATED_BODY()

public:
	AMark();

	/** Colours the disc gold and hides the X. */
	virtual void BeginPlay() override;	// BODY: (m)

	/** Same look as BeginPlay, in the editor. */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Called by AJulian::StepMarks: the gold X under him shows while he stands on this mark (MARK). This frame. */
	void SetOccupied(bool bOccupied);	// BODY: (m)

	/** The floor point Julian stands on (the actor location). */
	FVector GetStandPoint() const;	// BODY: (m)

	bool IsOccupied() const { return bIsOccupied; }

	/** The name the HUD and the log use ("ROPE", "PAST THE ROPE"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Mark")
	FString MarkName = TEXT("MARK");

	/** The mark past the rope: Julian standing on it clears the test (StepScene). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Mark")
	bool bGoal = false;

	// Step 2: checkpoints are no longer a mark's own business. An ASceneTrigger box saves one and names the mark
	// Julian restarts on (ASceneTrigger::CheckpointMark).

	// --- step 2: an action mark (rule 3, "the mark behind Vance lifts the keycard"). BODY: S ------------------------

	/** > 0: this is an action mark. Julian standing still on it, not SEEN, for this long does the action. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Mark")
	float ActionSeconds = 0.f;

	/** The trigger the finished action fires (ATakeDirector::FireTrigger), e.g. "Keycard". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Mark")
	FName ActionTrigger;

	/** The HUD's word over the progress bar while it runs ("LIFTING THE KEYCARD"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Mark")
	FString ActionVerb = TEXT("LIFTING");

	/** ATakeDirector::StepMarkActions calls this every frame for every action mark: bRunning is "Julian stands on
	 *  this mark and is not SEEN this frame". Running: ActionProgress += Dt; not running: ActionProgress = 0 (it
	 *  starts over). Returns true on the one frame ActionProgress reaches ActionSeconds (bActionDone becomes true and
	 *  stays). Once done it does nothing more. The disc pulses brighter while it runs (ApplyLook). */
	bool StepAction(float DeltaSeconds, bool bRunning);	// BODY: S

	/** 0..1 through the action (0 when not running or not an action mark, 1 when done). The HUD's bar. */
	float GetActionAlpha() const;	// BODY: S

	bool IsActionMark() const { return ActionSeconds > 0.f; }
	bool IsActionDone() const { return bActionDone; }

	/** The checkpoint restore: an action whose trigger had already fired is done (no second lift). */
	void SetActionDone() { bActionDone = true; ActionProgress = ActionSeconds; }

	/** Julian's feet within this 2D distance of the stand point, standing still, counts as on the mark, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Mark")
	float StandRadius = 60.f;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<USceneComponent> Root;

	/** The gold disc (a flat cylinder). */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> Disc;

	/** The gold X under Julian: two flat bars at +-45 degrees, hidden until occupied. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> CrossA;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> CrossB;

protected:
	bool bIsOccupied = false;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	float ActionProgress = 0.f;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	bool bActionDone = false;

private:
	/** Disc and X colours, X visibility from the occupied state. This frame. */
	void ApplyLook();
};
