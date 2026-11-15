// The broadcast drone (rule 6): flies a loop over the floor; off a mark, its cone sees Julian (rule 5). Owner: W.

#pragma once

#include "CoreMinimal.h"
#include "Watcher.h"
#include "DroneWatcher.generated.h"

class APatrolPoint;
class USpotLightComponent;

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

	// --- step 3 (BODY: W): the lock-on -------------------------------------------------------------------------------

	/** The alarm (ATakeDirector, when it goes live): the drone leaves its loop and follows Target (Julian): every
	 *  StepWatcher its floor point moves toward Target's feet (2D, at the feet's height) at ChaseSpeed and stays over
	 *  them. Null: back to the loop. On the frame it is set to a target: Flash(ChaseLine, red) through the director.
	 *  Blinded, it still follows. ChangesAtBar is false while it chases. */
	void SetChaseTarget(const AActor* Target);

	bool IsChasing() const { return ChaseTarget.IsValid(); }

	/** How fast it closes on its target, cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Drone")
	float ChaseSpeed = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Drone")
	FString ChaseLine = TEXT("BROADCAST: \"Stay on the man in white!\"");

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

	/** Step 6: its searchlight, a soft pool on the floor where it looks (the sight frame's cone: ConeRadius at
	 *  FlyHeight). No shadows. On while it is live; off before it arrives and while blinded. Look only. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<USpotLightComponent> SearchLight;

	/** The pool's strength at its centre, candelas (§15.11: a lit floor reads 95-130). */
	UPROPERTY(EditAnywhere, Category = "Take|Drone", meta = (ClampMin = "0"))
	float SearchLightCandelas = 4000.f;	// --- W3 lights --- was 600 (under the hall keys), 1800 still faint in play; 4000 seen reading

	/** The beam's glow in the haze (0 = none). QA sight: 0.3 -> 1.0, so the beam reads on its own now the cone is gone. */
	UPROPERTY(EditAnywhere, Category = "Take|Drone", meta = (ClampMin = "0"))
	float SearchLightHaze = 3.f;	// --- W3 lights --- was 1: the shaft reads at 3

	// --- QA sight: the searchlight is the cue (Marwan's note 19: no cone) ---------------------------------------------

	/** The pool's soft edge: inner cone = outer * this (higher: a crisper edge, closer to the true ConeRadius). */
	UPROPERTY(EditAnywhere, Category = "Take|Drone", meta = (ClampMin = "0", ClampMax = "0.95"))
	float SearchLightInnerRatio = 0.8f;

	/** The searchlight's colour at rest: a cool broadcast white. */
	UPROPERTY(EditAnywhere, Category = "Take|Drone")
	FLinearColor SearchLightColour = FLinearColor(0.92f, 0.95f, 1.f);

	/** When it has Julian the pool snaps onto him: its radius on him (cm), candelas and colour. */
	UPROPERTY(EditAnywhere, Category = "Take|Drone", meta = (ClampMin = "20"))
	float HoldPoolRadius = 90.f;

	UPROPERTY(EditAnywhere, Category = "Take|Drone", meta = (ClampMin = "0"))
	float HoldLightCandelas = 7000.f;	// --- W3 lights --- was 1600: brighter than the new rest searchlight

	UPROPERTY(EditAnywhere, Category = "Take|Drone")
	FLinearColor HoldLightColour = FLinearColor(1.f, 0.9f, 0.78f);

	/** How fast the pool lets go of him and widens again, per second (it snaps onto him; 0 = snaps back too). */
	UPROPERTY(EditAnywhere, Category = "Take|Drone", meta = (ClampMin = "0"))
	float HoldReleaseRate = 6.f;

	/** The lens's glint (glow colour, values above 1 bloom) at rest, and while it has him (a broadcast tally red). */
	UPROPERTY(EditAnywhere, Category = "Take|Drone")
	FLinearColor LensColour = FLinearColor(2.4f, 2.2f, 1.8f);

	UPROPERTY(EditAnywhere, Category = "Take|Drone")
	FLinearColor LensHoldColour = FLinearColor(4.f, 0.45f, 0.25f);

	/** The lens under the body (a small sphere that glints; dark while blinded or off). */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> Lens;

protected:
	TWeakObjectPtr<const AActor> ChaseTarget;

	/** QA sight: the base (planning lift), then the searchlight (rest pool, the snap onto Julian) and the lens. */
	virtual void UpdateWorldCues(float DeltaSeconds) override;

	/** 1 while it has Julian (snapped), easing to 0 after. */
	float HoldAlpha = 0.f;

private:
	/** Disc size, body height, tally and click volume at the body; colours. From OnConstruction and BeginPlay. */
	void LayoutDrone();
};
