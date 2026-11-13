// Julian, the actor: the player walks him like Maya and Vex (the Nov 13 rework). Owner: (m).

#pragma once

#include "CoreMinimal.h"
#include "TakeCharacter.h"
#include "Julian.generated.h"

class AMark;

/**
 * An ATakeCharacter with Id A and a white tint. The only character the rules watch: the watchers' frames, the tape,
 * the beam and the marks all test his feet (GetFeetLocation, inherited) and his capsule radius (GetCapsuleRadius).
 * Place him with the actor location at the capsule centre (floor + 96 cm).
 */
UCLASS()
class REEL_API AJulian : public ATakeCharacter
{
	GENERATED_BODY()

public:
	/** Header pass: Id = A, Tint = white. */
	AJulian();

	/** Super, then: he starts the take standing on a mark if one is within StartMarkTolerance (2D) of his feet, so
	 *  MARK shows from the first frame. */
	virtual void BeginPlay() override;	// BODY: (m)

	/**
	 * Step 4 (the director, after the devices). While IsStanding() and his feet are within a mark's StandRadius (2D,
	 * the nearest such mark wins), that mark is his standing mark; otherwise none. On a change: old mark
	 * SetOccupied(false), new mark SetOccupied(true) (the gold X shows this frame), TAKE_LOG("OFF MARK <old>") and
	 * TAKE_LOG("ON MARK <new>") (MarkName). Marks come from the director (ATakeDirector::Get(this)->Marks).
	 */
	void StepMarks();	// BODY: (m)

	/** The mark he is standing still on (StepMarks), else null. Composure's MARK and the goal test read this. */
	AMark* GetStandingMark() const;	// BODY: (m)

	/** At the start of a take, a mark this close to his feet counts as the one he stands on, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Julian")
	float StartMarkTolerance = 40.f;

protected:
	UPROPERTY(Transient)
	TObjectPtr<AMark> StandingMark;
};
