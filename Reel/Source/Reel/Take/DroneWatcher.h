// The broadcast drone (rule 6): flies a loop over the floor; off a mark, its cone sees Julian (rule 5). Owner: W.

#pragma once

#include "CoreMinimal.h"
#include "Watcher.h"
#include "DroneWatcher.generated.h"

class APatrolPoint;

/**
 * Its flight is a pure function of the bar clock, so a checkpoint's bar restores it exactly. PathPoints is a closed
 * loop of floor points; each point's Bars is how long the flight from it to the NEXT point takes (the last flies
 * back to the first). The path's time is the bars since its arrival bar (since bar 1 when ArriveOn is None); before
 * it arrives it hovers over the first point, Off.
 *
 * The actor is placed (and moved) on the floor under the drone: the actor location is the centre of its cone on the
 * floor. Body hangs FlyHeight above it. The frame is the cone's footprint: FrameMesh is a flat red disc of
 * ConeRadius on the floor (the "red ellipse"), and the frame lines run from the body down to it.
 *
 * Seeing: HoldsJulian is true while it is live, Julian's feet are within ConeRadius (2D) of the actor location and
 * no more than ConeFloorTolerance above or below it, and HasLineOfSight (the bridge and the walkway hide him).
 * ExposesOffMark is true: the director counts Julian as exposed inside the cone unless he stands on a mark.
 * Blind: dark for the duration (it keeps flying). Lure and Mask: no effect (false).
 */
UCLASS()
class REEL_API ADroneWatcher : public AWatcher
{
	GENERATED_BODY()

public:
	ADroneWatcher();

	/** Sizes the disc from ConeRadius, lifts the body to FlyHeight (editor and runtime). */
	virtual void OnConstruction(const FTransform& Transform) override;	// BODY: W

	/** Super, then the layout, then BuildFrameLines. It starts over its first path point. */
	virtual void BeginPlay() override;	// BODY: W

	/** Moves the actor to EvaluatePath (once arrived; SetActorLocation, no sweep), then the base step. */
	virtual void StepWatcher(float DeltaSeconds, UBarClockComponent* Clock) override;	// BODY: W

	virtual bool HoldsJulian(const AJulian* Julian) const override;	// BODY: W

	/** Blind: the base. Lure, Mask: false. */
	virtual bool ApplyEffect(EDeviceEffect Effect, float Duration, const FVector& Point) override;	// BODY: W

	virtual bool ExposesOffMark() const override { return true; }

	/** It leaves a path point on the bar line that starts Bar (the start of a leg). False before it has arrived. */
	virtual bool ChangesAtBar(int32 Bar, const UBarClockComponent* Clock) const override;	// BODY: W

	/** The body (the follow spot aims here, line traces start here). */
	virtual FVector GetEyePoint() const override;	// BODY: W
	virtual FVector GetAimPoint() const override;	// BODY: W
	virtual FVector GetLabelPoint() const override;	// BODY: W

	/** Four points on the disc's edge (+-X, +-Y at ConeRadius, on the floor), in FramePivot's space. */
	virtual void GetFrameFarCorners(TArray<FVector>& OutLocal) const override;	// BODY: W

	/** Pure function of the clock: the floor point under the drone now. Linear along each leg. No points, or not
	 *  arrived: the first point (or the placed location). */
	FVector EvaluatePath(const UBarClockComponent* Clock) const;	// BODY: W

	/** The loop, in order; it closes back to the first. Each point's Bars = the bars the leg FROM it takes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Drone")
	TArray<TObjectPtr<APatrolPoint>> PathPoints;

	/** Radius of the cone on the floor, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Drone")
	float ConeRadius = 300.f;

	/** The body's height above the floor, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Drone")
	float FlyHeight = 520.f;

	/** Julian's feet count as under the cone within this height of its floor, cm (not on the floor above). */
	UPROPERTY(EditAnywhere, Category = "Take|Drone")
	float ConeFloorTolerance = 150.f;

	/** The drone's body: a flat dark box with the tally under it. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> Body;

private:
	/** Disc size, body height, tally and click volume at the body; colours. From OnConstruction and BeginPlay. */
	void LayoutDrone();
};
