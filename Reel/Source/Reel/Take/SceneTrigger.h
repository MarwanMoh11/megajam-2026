// A named box on the floor that fires once when Julian walks into it: scene starts, checkpoints, arrivals. Owner: S.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SceneTrigger.generated.h"

class AMark;
class UBoxComponent;

/**
 * Place it on the floor (actor location = the box's centre at floor height; yaw rotates it), like ARedTapeZone. It
 * has no collision and nothing visible in game. ATakeDirector::StepTriggers tests Julian's feet against every one
 * each frame; the first frame they are inside, the director (once per take):
 *  - FireTrigger(TriggerName): watchers with that ArriveOn are scheduled, the scene script moves on;
 *  - Scene > 0: the director's Scene becomes Scene, the crew move to their posts for it (ACrewPost), and Title is
 *    shown as the big scene title;
 *  - CheckpointMark set: the checkpoint is saved (ATakeDirector::SaveCheckpoint): a blooper restarts Julian on that
 *    mark, at this bar, with the device uses and triggers as they are now. The slate reads "AGAIN FROM
 *    <CheckpointName>".
 * A trigger that has fired stays fired across a blooper's reload (the game instance keeps it), so it does not fire
 * again when Julian restarts inside it. R (a new take) clears them all.
 */
UCLASS()
class REEL_API ASceneTrigger : public AActor
{
	GENERATED_BODY()

public:
	ASceneTrigger();

	/** Sizes the editor outline. */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** True when WorldPoint is inside the box: 2D in the trigger's own frame (yaw honoured), Z within
	 *  [-50, Extent.Z * 2] of the actor's height. Pure. */
	bool Contains(const FVector& WorldPoint) const;	// BODY: S

	/** The name it fires ("Scene2", "PastRope", "Door"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Trigger")
	FName TriggerName;

	/** Half-size of the box in cm (X, Y); Z is the height it counts up to above the floor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Trigger")
	FVector Extent = FVector(300.f, 300.f, 200.f);

	/** > 0: walking in starts this scene. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Trigger")
	int32 Scene = 0;

	/** The big title shown on walking in ("SCENE 2 - THE FLOOR"). Empty: none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Trigger")
	FString Title;

	/** Set: walking in saves a checkpoint, and a blooper restarts Julian standing on this mark, facing its yaw. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Trigger")
	TObjectPtr<AMark> CheckpointMark;

	/** The checkpoint's name on the slate and in the flash ("THE FLOOR"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Trigger")
	FString CheckpointName;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<USceneComponent> Root;

	/** Editor-only outline, no collision. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UBoxComponent> Area;
};
