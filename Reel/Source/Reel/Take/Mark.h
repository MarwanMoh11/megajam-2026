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

private:
	/** Disc and X colours, X visibility from the occupied state. This frame. */
	void ApplyLook();
};
