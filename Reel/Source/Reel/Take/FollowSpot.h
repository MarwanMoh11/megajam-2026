// Maya's follow spot: free, one watcher at a time (rule 7). Owner: (u).

#pragma once

#include "CoreMinimal.h"
#include "Device.h"
#include "FollowSpot.generated.h"

/**
 * Maya stands in its use ring, looks at a watcher (the aim assist in ATakeDirector::FindUseTarget picks it within
 * AimAssistDegrees, and it highlights), presses Use: ATakeDirector::SpotWatcher calls FireAt. Blind for Duration
 * (4 s) within Range (35 m) of the lamp; ready again the moment the 4 s end.
 * Collision: the lamp and BeamShape are FTakeShapes (NoCollision): they never block a character or the camera.
 */
UCLASS()
class REEL_API AFollowSpot : public ADevice
{
	GENERATED_BODY()

public:
	AFollowSpot();

	/** Colours the lamp; hides the beam. */
	virtual void BeginPlay() override;	// BODY: (u)

	/** The rework changes this: true when idle (the watcher is FindUseTarget's and FireAt's business); false only
	 *  while busy, "SPOT - busy 2.1 s". (Today's body still returns false always, "Click a watcher to spot it".) */
	virtual bool CanFire(FString& OutWhyNot) const override;	// BODY: (u)

	/** Busy countdown; the beam shape follows the target (lamp to target) while busy, hidden when it ends. */
	virtual void StepDevice(float DeltaSeconds) override;	// BODY: (u)

	/** Only Target. */
	virtual void GatherTargets(const TArray<TObjectPtr<AWatcher>>& All, TArray<AWatcher*>& Out) const override;	// BODY: (u)

	/** The target's location (for the log; the spot ignores Range around it). */
	virtual FVector GetEffectPoint() const override;	// BODY: (u)

	virtual FString GetPipsText() const override;	// BODY: (u)

	/** Step 2 (SpotWatcher). Busy -> false, OutWhyNot "SPOT - busy 2.1 s". Target farther than Range from the lamp ->
	 *  false, "SPOT - out of range". Else: Target = Watcher, busy for Duration, the beam shape shows from the lamp to
	 *  the target this frame, LandedThisFrame true (the Blind lands in StepDevices this frame). Logs. */
	bool FireAt(AWatcher* Watcher, FString& OutWhyNot);	// BODY: (u)

	AWatcher* GetTarget() const;	// BODY: (u)
	float GetBusySecondsLeft() const;	// BODY: (u)

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

	/** The aim assist: a watcher whose aim point is within this angle of the user's aim direction can be spotted,
	 *  degrees. The smallest angle wins. */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	float AimAssistDegrees = 10.f;

	/** Step 2, the look: a real spot light at the lamp, movable, casting shadows, on only while the spot is busy and
	 *  aimed at the target every frame (UpdateBeam), so the beam cuts through the haze. Off and invisible when idle.
	 *  BODY: D (the constructor makes it; UpdateBeam drives it from the three tunables below). */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<class USpotLightComponent> Light;

	/** The light's intensity while busy, candelas. */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	float LightCandelas = 4000.f;

	/** The light's outer cone half-angle, degrees. */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	float LightConeDegrees = 7.f;

	/** The light's reach, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	float LightRadius = 4500.f;

	// --- QA devices (Nov 14, notes 10, 17, 18) ----------------------------------------------------------------------

	/** Switch: the old drawn beam (a white cylinder from the lamp to the target). Off: the real light's shaft through
	 *  the hall's haze is the beam. */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	bool bShowBeamShape = false;

	/** The light's volumetric scattering while it burns: how strongly its shaft shows in the haze. */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	float BeamHaze = 4.f;

	/** The model swings (turns about its anchor) onto its target on the press, and back to rest when it is ready. */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	bool bSwingModel = true;

	/** Which way the model's lens faces at rest, a yaw local to the actor, degrees. */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	float LensYaw = 0.f;

	/** The lens at rest, local to the actor and relative to ModelOffset (it turns with the swing): the beam starts
	 *  here when the model shows. */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	FVector LensOffset = FVector(40.f, 0.f, 150.f);

	/** How fast it swings, degrees a second. */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	float SwingDegreesPerSecond = 320.f;

	/** The share of the swing taken on the press frame itself. */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	float SwingFirstStep = 0.4f;

protected:
	UPROPERTY(Transient)
	TObjectPtr<AWatcher> Target;

	float BusySecondsLeft = 0.f;

	/** The swing now and the swing that faces the target, degrees from rest. */
	float SwingYaw = 0.f;
	float SwingAimYaw = 0.f;

private:
	/** Stretches the beam from the lamp to the target and shows it (hides it if there is no target), this frame. */
	void UpdateBeam();

	/** The swing (degrees from rest) that points the lens at the target; the current swing if there is none. */
	float GetAimSwing() const;

	/** Eases the swing toward the target (or back to rest when idle) and turns the model, this frame. */
	void StepSwing(float DeltaSeconds);

	/** The lens in the world, turned with the swing. */
	FVector GetLensPoint() const;
};
