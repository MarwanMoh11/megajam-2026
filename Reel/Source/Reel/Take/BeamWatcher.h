// The rope's infrared beam: a line-shaped frame between two posts (rule 6). Owner: (d).

#pragma once

#include "CoreMinimal.h"
#include "Watcher.h"
#include "BeamWatcher.generated.h"

class UPointLightComponent;

/**
 * Post A stands at the actor location, post B at EndB (local offset; a widget in the editor). Live from
 * LiveFromBar; drops for one beat in every DropEveryBars bars (beat DropBeatInBar of bars that are multiples of
 * DropEveryBars), on a hi-hat (Clock->PlayAccent on the drop frame). Blind or Mask turns it off for the duration.
 * Touching it while live is a blooper at once.
 */
UCLASS()
class REEL_API ABeamWatcher : public AWatcher
{
	GENERATED_BODY()

public:
	ABeamWatcher();

	/** Places post B and stretches the beam line between the posts at BeamHeight (editor and runtime). */
	virtual void OnConstruction(const FTransform& Transform) override;	// BODY: (d)

	virtual void BeginPlay() override;	// BODY: (d)

	/** Effect countdown; State Off before LiveFromBar, Dropped during the drop beat (beam hidden on the frame the
	 *  beat starts, back on the frame it ends; PlayAccent on the drop), else Watching / Blinded / Masked. */
	virtual void StepWatcher(float DeltaSeconds, UBarClockComponent* Clock) override;	// BODY: (d)

	/** TouchesJulian. */
	virtual bool HoldsJulian(const AJulian* Julian) const override;

	/** Step 6: the beam is a laser in the world: FrameDisplay never hides it. */
	virtual bool UsesFrameDisplay() const override { return false; }	// BODY: (d)

	/** Blind and Mask: off for Duration (line dark this frame), returns true. Lure: no effect, false. */
	virtual bool ApplyEffect(EDeviceEffect Effect, float Duration, const FVector& Point) override;	// BODY: (d)

	virtual bool IsInstantBlooper() const override { return true; }

	/** 2D distance from Point to the segment A-B. */
	virtual float DistanceToPoint(const FVector& Point) const override;	// BODY: (d)

	/** The middle of the beam's segment at beam height (the follow spot aims here). */
	virtual FVector GetAimPoint() const override;	// BODY: (d)

	virtual bool IsLive() const override;	// BODY: (d)

	virtual FString GetStateText() const override;	// BODY: (d)

	/** Live, and the 2D distance from Julian's feet to the segment is within his capsule radius. */
	bool TouchesJulian(const AJulian* Julian) const;	// BODY: (d)

	/** Live, and the 2D distance between the segment PrevFeet -> his feet now and the beam's segment A-B is within
	 *  his capsule radius (no tunnelling on a long frame). PrevFeet == his feet reduces to TouchesJulian. */
	bool TouchesJulianSwept(const AJulian* Julian, const FVector& PrevFeet) const;

	/** World ends of the beam at BeamHeight. */
	FVector GetEndA() const;	// BODY: (d)
	FVector GetEndB() const;	// BODY: (d)

	/** Post B relative to post A (the actor), cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Beam", meta = (MakeEditWidget = true))
	FVector EndB = FVector(0.f, 400.f, 0.f);

	/** Beam height above the floor, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Beam")
	float BeamHeight = 80.f;

	/** Beam line thickness, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Beam")
	float BeamThickness = 4.f;

	/** The first bar it is live (1 = from the start). */
	UPROPERTY(EditAnywhere, Category = "Take|Beam", meta = (ClampMin = "1"))
	int32 LiveFromBar = 1;

	/** It drops in bars that are multiples of this. */
	UPROPERTY(EditAnywhere, Category = "Take|Beam", meta = (ClampMin = "1"))
	int32 DropEveryBars = 4;

	/** The beat of that bar it drops for (1-based). */
	UPROPERTY(EditAnywhere, Category = "Take|Beam", meta = (ClampMin = "1"))
	int32 DropBeatInBar = 4;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> PostA;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> PostB;

	// --- QA sight: an electric eye (Marwan's note 18: no dotted lines) ------------------------------------------------
	// With the drawn overlays off the laser is a thin solid glint (no dashes), and each post carries a lamp at beam
	// height with a small glow in the haze: red while live, dim while it drops or before it is live, green while
	// blinded or masked. Touching it is still an instant blooper, exactly as before.

	/** The laser's thickness with the drawn overlays off, cm (BeamThickness with them on). */
	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0.2"))
	float GlintThickness = 1.5f;

	/** The post lamps' glow (values above 1 bloom): live, dropped or not yet live, blinded or masked. */
	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	FLinearColor LampLiveColour = FLinearColor(4.f, 0.3f, 0.15f);

	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	FLinearColor LampOffColour = FLinearColor(0.25f, 0.02f, 0.01f);

	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	FLinearColor LampDarkColour = FLinearColor(0.3f, 3.2f, 0.8f);

	/** The small light at each post: candelas while live (half while blinded or masked, none while dropped), reach,
	 *  colours and glow in the haze. */
	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0"))
	float PostLightCandelas = 40.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "10"))
	float PostLightRadius = 160.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	FLinearColor PostLightLiveColour = FLinearColor(1.f, 0.2f, 0.1f);

	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	FLinearColor PostLightDarkColour = FLinearColor(0.35f, 1.f, 0.5f);

	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0"))
	float PostLightHaze = 1.2f;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> LampA;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> LampB;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UPointLightComponent> PostLightA;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UPointLightComponent> PostLightB;

protected:
	bool bDropped = false;
	bool bBeforeLive = false;

	/** QA sight: the base (planning lift), then the post lamps and lights, this frame. */
	virtual void UpdateWorldCues(float DeltaSeconds) override;

	/** Dark first (base), else OFF before LiveFromBar, DROPPED on the drop beat, else WATCHING. */
	virtual void SyncState() override;

private:
	/** Places post B, the line (FramePivot at the beam's midpoint) and the click volume; colours them (OnConstruction and BeginPlay). */
	void LayoutBeam();
};
