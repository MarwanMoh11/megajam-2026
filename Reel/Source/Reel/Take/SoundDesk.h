// Vex's sound desk: feedback through the band's PA blinds everything in the hall (rule 7). Owner: D.

#pragma once

#include "CoreMinimal.h"
#include "Device.h"
#include "SoundDesk.generated.h"

/**
 * One use. Fired: it lands on the same frame (like the follow spot): Blind for Duration (4 s = 2 bars) on every
 * watcher within Range of the desk (the map sets Range to cover the whole hall). The feedback squeals
 * (UBarClockComponent::PlayFeedback) and the band drops out for MuteBars bars (UBarClockComponent::MuteMetronome):
 * that drop is not a band break. The body flashes white while the effect runs (bRunning for Duration).
 */
UCLASS()
class REEL_API ASoundDesk : public ADevice
{
	GENERATED_BODY()

public:
	ASoundDesk();

	virtual void BeginPlay() override;	// BODY: D

	/** Spends the use, lands this frame (MarkLanded), runs for Duration, squeal and mute. */
	virtual bool Fire() override;	// BODY: D

	/** Counts the run down; the body goes back to BodyColour when it ends. */
	virtual void StepDevice(float DeltaSeconds) override;	// BODY: D

	/** The band is silent for this many bars after the feedback. */
	UPROPERTY(EditAnywhere, Category = "Take|Desk")
	int32 MuteBars = 2;
};
