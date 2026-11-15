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

	/** W3 spot: the swing's top speed, degrees a second (a heavy lamp: it never whips round). */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	float SwingDegreesPerSecond = 240.f;

	/** W3 spot: the shove on the press frame, as a share of the top speed: the lamp is already moving this frame,
	 *  then the spring carries it the rest of the way. */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	float SwingFirstStep = 0.35f;

	// --- W3 spot (Nov 15, item 3): weight, a beam that stops at what it hits -----------------------------------------

	/** The swing is a spring: how quickly it settles, Hz (lower: heavier). */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	float SwingFrequencyHz = 1.1f;

	/** ...and its damping (1: no overshoot; 0.6: a slight overshoot and settle, about 8 %). */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	float SwingDampingRatio = 0.62f;

	/** The model tilts up or down toward the target about its trunnion, at most this much, degrees (the beam itself
	 *  always reaches the target; the stand cannot tilt far). 0: the model only turns. */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	float MaxModelPitch = 10.f;

	/** The trunnion's height over the model's anchor, cm (the tilt's pivot). Below 0: the lens's height. */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	float TrunnionHeight = -1.f;

	/** Switch: the beam is traced from the lens; the shaft and the light end at the first solid thing (the light also
	 *  casts shadows, so nothing glows through a wall). Off: the old straight beam to the target. */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	bool bBeamStopsAtHit = true;

	/** Switch: the beam light casts shadows and volumetric shadows (a hero light: the only shadowed device light). */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	bool bBeamShadows = true;

	/** How far past the beam's end the light still reaches, cm (the floor round and behind the lit person). */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	float BeamSpill = 300.f;

	/** A soft glow where the beam lands (no shadows, no haze), so the lit person or wall reads lit. 0: none. */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	float HitGlowCandelas = 60.f;

	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	float HitGlowRadius = 260.f;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<class UPointLightComponent> HitGlow;

	/** The lens glows while the beam burns: an unlit disc over the lens, this wide, cm (0: none). */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	float LensGlowSize = 30.f;

	/** ...this bright (emissive multiplier on BeamColour). */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	float LensGlow = 6.f;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> LensDisc;

	/** The light starts this far in front of the lens, cm (inside the lamp, the lamp would shadow its own light). */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	float LightClearance = 4.f;

	/** The haze's bright blob at the beam's apex (inverse-square spike in the volumetric fog): sets
	 *  r.VolumetricFog.InverseSquaredLightDistanceBiasScale to this at BeginPlay (engine default 1; higher: no blob,
	 *  the shaft unchanged further out). 0: leave the setting alone. */
	UPROPERTY(EditAnywhere, Category = "Take|Spot")
	float FogApexBias = 8.f;

	/** Where the beam ends now: the first solid thing it meets, or the target (the lamp's lens while it is dark). */
	FVector GetBeamEnd() const;

	/** What the beam lands on now: the first solid actor it meets (the target watcher when nothing is in the way);
	 *  null while it is dark. */
	AActor* GetBeamHitActor() const;

	/** Is the beam burning now? */
	bool IsBeamOn() const { return bBeamOn; }

protected:
	UPROPERTY(Transient)
	TObjectPtr<AWatcher> Target;

	float BusySecondsLeft = 0.f;

	/** The swing now (degrees from rest: yaw about the actor's up, pitch of the beam) and its speed. */
	float SwingYaw = 0.f;
	float SwingAimYaw = 0.f;
	float SwingPitch = 0.f;
	float SwingAimPitch = 0.f;
	float SwingYawSpeed = 0.f;
	float SwingPitchSpeed = 0.f;

	/** W3 spot: the beam as last traced. */
	bool bBeamOn = false;
	FVector BeamEndPoint = FVector::ZeroVector;
	TWeakObjectPtr<AActor> BeamHitActor;

private:
	/** Aims the beam (along the lamp, sweeping onto the target), traces it, lights what it hits, this frame. Hides it
	 *  all when idle. */
	void UpdateBeam();

	/** The swing (yaw from rest, and the beam's pitch) that points the lens at the target; the current one if none. */
	float GetAimSwing() const;
	float GetAimPitch() const;

	/** The spring toward the target (or back to rest when idle), then turns the model and carries the handle. */
	void StepSwing(float DeltaSeconds);

	/** Turns the model (yaw, and pitch up to MaxModelPitch about the trunnion) and carries the lever with it. */
	void PoseSwing();

	/** The swing as a turn local to the actor, about the trunnion, with the model's pitch clamp. */
	FQuat GetSwingTurn() const;

	/** The trunnion (the tilt's pivot), local to the actor. */
	FVector GetTrunnionLocal() const;

	/** The lens in the world, turned with the swing. */
	FVector GetLensPoint() const;
};
