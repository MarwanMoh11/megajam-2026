// Julian, the actor: he only moves by cues (rule 3). Owner: (c).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Julian.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class AMark;

/**
 * Not possessed by anyone (no controller, no movement component): the director steps him. He walks kinematically
 * along a navmesh path at WalkSpeed and stops exactly on the mark, so arrival counts are exact. Place him on the
 * floor (actor location = capsule centre). The capsule has no collision; marks under him stay clickable.
 */
UCLASS()
class REEL_API AJulian : public APawn
{
	GENERATED_BODY()

public:
	AJulian();

	/** Colours the body white and the path dots gold; snaps him onto the navmesh if he is slightly off it. */
	virtual void BeginPlay() override;	// BODY: (c)

	/** Same look as BeginPlay, in the editor (OnConstruction does not rerun for placed actors in play). */
	virtual void OnConstruction(const FTransform& Transform) override;

	/**
	 * Step 2 (from ATakeDirector::HandleClick, camera A only). Finds the navmesh path from his feet to the mark's
	 * stand point synchronously (UNavigationSystemV1::FindPathToLocationSynchronously). No path: returns false and
	 * nothing changes. Otherwise, on this frame: replaces any old cue (old mark SetCued(false), SetOccupied(false)),
	 * new mark SetCued(true) (its gold ring shows), stores the path, turns him to face the first leg, rebuilds the
	 * dotted gold path, TAKE_LOG("CUE <mark> path <n> pts <len> cm, in <beats> beats"). Cueing the mark he already
	 * stands on is a valid cue of length 0.
	 */
	bool CueTo(AMark* Mark);	// BODY: (c)

	/**
	 * Step 4 (director). Moves him along the path by WalkSpeed * DeltaSeconds (across path corners in one step),
	 * facing his direction of travel; trims the dots he has passed. On reaching the last point: snaps exactly onto
	 * it, stops, mark->SetOccupied(true) (gold X), mark->SetCued(false), clears the dots, TAKE_LOG("ARRIVE <mark>").
	 */
	void StepWalk(float DeltaSeconds);	// BODY: (c)

	bool IsWalking() const;	// BODY: (c)

	/** Remaining path length, cm (0 when stopped). */
	float GetRemainingPathLength() const;	// BODY: (c)

	/** Remaining path length / WalkSpeed (0 when stopped). */
	float GetSecondsToArrival() const;	// BODY: (c)

	/** Seconds to arrival in beats; the HUD rounds up for "in 6". */
	float GetBeatsToArrival(float SecondsPerBeat) const;	// BODY: (c)

	/** The path's remaining points, from his position to the mark (empty when stopped). */
	const TArray<FVector>& GetPathPoints() const;	// BODY: (c)

	/** The mark he is walking to, or standing on after arriving (null before the first cue). */
	AMark* GetCuedMark() const;	// BODY: (c)

	/** The mark he is standing on: non-null only when stopped on it. */
	AMark* GetStandingMark() const;	// BODY: (c)

	/** His feet: actor location minus the capsule half-height. Tape, shot zones and frames test this point. */
	FVector GetFeetLocation() const;	// BODY: (c)

	float GetCapsuleRadius() const;	// BODY: (c)

	/** cm/s. He never runs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Julian")
	float WalkSpeed = 250.f;

	/** Gap between the gold dots of his path, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Julian")
	float DotSpacing = 40.f;

	/** Size of one dot, cm (flat cubes on the floor). */
	UPROPERTY(EditAnywhere, Category = "Take|Julian")
	float DotSize = 10.f;

	/** How far a path end may be from the mark and still count as arriving on it, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Julian")
	float ArriveTolerance = 5.f;

	/** Half-size of the box used to project his feet and the mark onto the navmesh, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Julian")
	FVector NavProjectExtent = FVector(150.f, 150.f, 300.f);

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UCapsuleComponent> Capsule;

	/** White body (cylinder). */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> Body;

	/** White head (sphere). */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> Head;

	/** A small block on the front of the head so his facing reads from above. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> Nose;

	/** The dotted gold path: one instance per dot, in WORLD space (bWorldSpace = true in AddInstance), visible on
	 *  every camera until he arrives. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UInstancedStaticMeshComponent> PathDots;

protected:
	/** Rebuilds the dots along the remaining path (call after a cue and whenever dots must be trimmed). */
	void RebuildPathDots();	// BODY: (c)

	TArray<FVector> PathPoints;
	int32 NextPointIndex = 0;
	bool bWalking = false;

	UPROPERTY(Transient)
	TObjectPtr<AMark> CuedMark;

	UPROPERTY(Transient)
	TObjectPtr<AMark> StandingMark;

private:
	/** Colours body, head, nose and path dots (BeginPlay and OnConstruction). */
	void ApplyLook();

	/** Lays the dots along the whole new path (after a cue): DotTransforms, DotDistances, WalkedDistance = 0. */
	void LayOutDots();

	/** Every dot of the current cue, world space, and how far along the path each sits, cm. */
	TArray<FTransform> DotTransforms;
	TArray<float> DotDistances;

	/** First dot not yet passed; dots before it are trimmed. */
	int32 FirstDotIndex = 0;

	/** Distance walked since the cue, cm. */
	float WalkedDistance = 0.f;
};
