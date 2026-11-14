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

	/** Pure function of the clock: where the patrol puts him and which way he faces at Clock's current time.
	 *  Step 2: the patrol's time is the bars since bar 1 PLUS PatrolOffsetBars. */
	void EvaluatePatrol(const UBarClockComponent* Clock, FVector& OutLocation, float& OutYaw) const;	// BODY: (d)

	// --- step 2 (BODY: W) --------------------------------------------------------------------------------------------

	/** The index in PatrolPoints of the point the patrol holds at the START of Bar (1-based bar, PatrolOffsetBars
	 *  applied; -1 with no points). Pure. */
	int32 GetPatrolIndexAtBar(int32 Bar) const;	// BODY: W

	/** The patrol moves on to another point on the bar line that starts Bar (GetPatrolIndexAtBar(Bar) differs from
	 *  GetPatrolIndexAtBar(Bar - 1)). False with fewer than two points, before he has arrived, or while lured. */
	virtual bool ChangesAtBar(int32 Bar, const UBarClockComponent* Clock) const override;	// BODY: W

	/** The four far corners of the wedge in FramePivot's space: at FrameRange along +X, at +-FrameHalfAngle, on the
	 *  floor and at FrameLineHeight. */
	virtual void GetFrameFarCorners(TArray<FVector>& OutLocal) const override;	// BODY: W

	/** Bars added to the patrol's clock: Vance's cycle (crowd 4 bars, lift 4 bars) with offset 1 puts the start of
	 *  each speech on a band break (bars 8, 16, 24...). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Guard")
	int32 PatrolOffsetBars = 0;

	// --- step 3 (BODY: W): the guards who walk in at the alarm -------------------------------------------------------

	/** The patrol's clock starts at his arrival bar (GetArriveBar) and not at bar 1: he is at PatrolPoints[0] on the
	 *  bar he arrives, whatever bar that is. PatrolOffsetBars still adds. Only meaningful with ArriveOn set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Guard")
	bool bPatrolFromArrival = false;

	/** The patrol does not loop: he goes through PatrolPoints once (holding each for its Bars, walking between them)
	 *  and then holds the last point for good. ChangesAtBar is false once he is there. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Guard")
	bool bPatrolOnce = false;

	/** Vance: a Lure only turns him to the point for its duration, he does not walk (rule 6). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Guard")
	bool bTurnsOnly = false;

	/** Height of the frame lines' upper far corners, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Guard")
	float FrameLineHeight = 200.f;

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

	/** The lure's own timer (stacks with a Blind's dark timer): Going/Looking until it ends, then Returning. */
	float LureSecondsLeft = 0.f;

	/** Dark first (base), else LURED while the lure's timer runs. */
	virtual void SyncState() override;

	/** The patrol point held at the last StepWatcher (-1 = none yet): a change flashes the new point's ArriveLine. */
	int32 LastPatrolIndex = -1;

private:
	/** Sizes the wedge from FrameRange / FrameHalfAngle and colours the wedge, body and cap (OnConstruction and BeginPlay). */
	void LayoutGuard();
};
