// The patrol guard: waypoints, turning on bar lines (rule 6). Owner: (d).

#pragma once

#include "CoreMinimal.h"
#include "Watcher.h"
#include "PatrolGuard.generated.h"

class APatrolPoint;

/** Where the guard's lure is. */
enum class EGuardLurePhase : uint8
{
	None,
	Going,		// walking to the lure point
	Looking,	// standing, facing the point, until the lure's duration ends
	Returning	// walking back to where the patrol says he should be
};

/**
 * His patrol is a pure function of the bar clock: PatrolPoints cycle, each held for its Bars; at the bar line
 * where a point's hold ends he turns (or walks at WalkSpeed, then turns) to the next. A reload therefore restores
 * him exactly. The frame is a flat red wedge on the floor (a cone pitched to lie along +X and squashed flat),
 * FrameRange long with FrameHalfAngle either side; frames ignore walls in step 1.
 */
UCLASS()
class REEL_API APatrolGuard : public AWatcher
{
	GENERATED_BODY()

public:
	APatrolGuard();

	/** Sizes the wedge from FrameRange / FrameHalfAngle (editor and runtime). */
	virtual void OnConstruction(const FTransform& Transform) override;	// BODY: (d)

	virtual void BeginPlay() override;	// BODY: (d)

	/** Patrol from the clock (EvaluatePatrol), or the lure (go, look, return), then the beat pulse. */
	virtual void StepWatcher(float DeltaSeconds, UBarClockComponent* Clock) override;	// BODY: (d)

	/** Julian's feet within FrameRange (2D) and within FrameHalfAngle of his facing, while live. */
	virtual bool HoldsJulian(const AJulian* Julian) const override;	// BODY: (d)

	/** Blind as the base. Lure: State Lured for Duration, LurePoint = Point, he turns to it and starts walking this
	 *  frame. Mask: nothing in step 1 (returns false). */
	virtual bool ApplyEffect(EDeviceEffect Effect, float Duration, const FVector& Point) override;	// BODY: (d)

	virtual FString GetStateText() const override;	// BODY: (d)

	/** Pure function of the clock: where the patrol puts him and which way he faces at Clock's current time. */
	void EvaluatePatrol(const UBarClockComponent* Clock, FVector& OutLocation, float& OutYaw) const;	// BODY: (d)

	/** The patrol, in order; it loops. Empty: he holds his placed pose. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Guard")
	TArray<TObjectPtr<APatrolPoint>> PatrolPoints;

	/** Frame length, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Guard")
	float FrameRange = 900.f;

	/** Frame half-angle, degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Guard")
	float FrameHalfAngle = 30.f;

	/** Walking speed between patrol points and on a lure, cm/s. */
	UPROPERTY(EditAnywhere, Category = "Take|Guard")
	float WalkSpeed = 200.f;

	/** How long a turn on a bar line takes, s (0 = snap). */
	UPROPERTY(EditAnywhere, Category = "Take|Guard")
	float TurnSeconds = 0.2f;

	/** On a lure he stops this far short of the point, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Guard")
	float LureStandOff = 150.f;

	/** Grey stand-in body (cylinder). */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> Body;

	/** A cap block on the front of the head so his facing reads from above. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> Cap;

protected:
	EGuardLurePhase LurePhase = EGuardLurePhase::None;
	FVector LureStart = FVector::ZeroVector;
	float LureTime = 0.f;

private:
	/** Sizes the wedge from FrameRange / FrameHalfAngle and colours the wedge, body and cap (OnConstruction and BeginPlay). */
	void LayoutGuard();
};
