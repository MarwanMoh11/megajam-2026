// Maya's follow spot: free, one watcher at a time (rule 7). Owner: (e).

#pragma once

#include "CoreMinimal.h"
#include "Device.h"
#include "FollowSpot.generated.h"

/**
 * Not fired by clicking it: clicking a watcher on its FiredFrom camera calls ATakeDirector::SpotWatcher, which
 * calls FireAt. Blind for Duration (4 s) within Range (35 m) of the lamp; ready again the moment the 4 s end.
 * Clicking the lamp itself: CanFire is false with "Click a watcher to spot it".
 */
UCLASS()
class REEL_API AFollowSpot : public ADevice
{
	GENERATED_BODY()

public:
	AFollowSpot();

	/** Colours the lamp; hides the beam. */
	virtual void BeginPlay() override;	// BODY: (e)

	/** Always false: "Click a watcher to spot it" (or "SPOT - busy 2.1 s" while busy). */
	virtual bool CanFire(FString& OutWhyNot) const override;	// BODY: (e)

	/** Busy countdown; the beam shape follows the target (lamp to target) while busy, hidden when it ends. */
	virtual void StepDevice(float DeltaSeconds) override;	// BODY: (e)

	/** Only Target. */
	virtual void GatherTargets(const TArray<TObjectPtr<AWatcher>>& All, TArray<AWatcher*>& Out) const override;	// BODY: (e)

	/** The target's location (for the log; the spot ignores Range around it). */
	virtual FVector GetEffectPoint() const override;	// BODY: (e)

	virtual FString GetPipsText() const override;	// BODY: (e)

	/** Step 2 (SpotWatcher). Busy -> false, OutWhyNot "SPOT - busy 2.1 s". Target farther than Range from the lamp ->
	 *  false, "SPOT - out of range". Else: Target = Watcher, busy for Duration, the beam shape shows from the lamp to
	 *  the target this frame, LandedThisFrame true (the Blind lands in StepDevices this frame). Logs. */
	bool FireAt(AWatcher* Watcher, FString& OutWhyNot);	// BODY: (e)

	AWatcher* GetTarget() const;	// BODY: (e)
	float GetBusySecondsLeft() const;	// BODY: (e)

	/** The light beam: a long thin cylinder from the lamp to the target, hidden when idle. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> BeamShape;

	/** The beam's thickness (cylinder scale across; 1 = 100 cm). */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	float BeamThickness = 0.3f;

	/** Height above the target's feet the beam aims at, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	float BeamTargetHeight = 120.f;

	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	FLinearColor BeamColour = TakeColors::White;

protected:
	UPROPERTY(Transient)
	TObjectPtr<AWatcher> Target;

	float BusySecondsLeft = 0.f;

private:
	/** Stretches the beam from the lamp to the target and shows it (hides it if there is no target), this frame. */
	void UpdateBeam();
};
