// The iris scanner at the front doors (rule 6, Scene 1): a frame in front of a gate. Owner: W.

#pragma once

#include "CoreMinimal.h"
#include "Watcher.h"
#include "ScannerWatcher.generated.h"

class UBoxComponent;
class USpotLightComponent;

/**
 * Place it on the ground at the middle of the gate, +X pointing the way in (through the doors). In its own space:
 *  - the gate is the plane X = 0, GateHalfWidth either side of the centre. While the scanner is live the gate is
 *    SHUT: GateBlock (a box) blocks the Pawn channel only, so Julian cannot walk through, and GateBar (a red bar at
 *    waist height) shows across it. While it is not live (blinded, masked, or not arrived) the gate is OPEN:
 *    GateBlock has no collision and the bar turns gold and drops to the floor. Both change on the frame the state
 *    changes. The gate never shuts on top of a character: while any ATakeCharacter's capsule overlaps the gate's
 *    slab it stays open, and shuts on the first frame it is clear.
 *  - the frame is the box in front of the gate: X from -FrameDepth to 0, Y within GateHalfWidth. FrameMesh is a flat
 *    red rectangle on the floor filling it (the map lays red tape round the same box). HoldsJulian: live, his feet
 *    inside that box (2D, within 150 cm of the floor), and HasLineOfSight.
 *  - the eye (GetEyePoint, GetAimPoint: the follow spot aims here) is EyeOffset, on the arch's lintel.
 * Blind and Mask: dark for the duration (the gate opens). Lure: no effect.
 * Spec note: in the click-to-cue design Julian waited on the scanner mark by himself; now the player walks him, so
 * the gate is what makes him wait.
 */
UCLASS()
class REEL_API AScannerWatcher : public AWatcher
{
	GENERATED_BODY()

public:
	AScannerWatcher();

	virtual void OnConstruction(const FTransform& Transform) override;	// BODY: W
	virtual void BeginPlay() override;	// BODY: W

	/** The base step, then the gate: open or shut for the state (see the class comment), this frame. */
	virtual void StepWatcher(float DeltaSeconds, UBarClockComponent* Clock) override;	// BODY: W

	virtual bool HoldsJulian(const AJulian* Julian) const override;	// BODY: W

	/** Blind and Mask: AddDark, true (the gate opens this frame). Lure: false. */
	virtual bool ApplyEffect(EDeviceEffect Effect, float Duration, const FVector& Point) override;	// BODY: W

	virtual FVector GetEyePoint() const override;	// BODY: W
	virtual FVector GetAimPoint() const override;	// BODY: W
	virtual FVector GetLabelPoint() const override;	// BODY: W

	/** The four floor corners of the frame box, in order round it, in FramePivot's space. */
	virtual void GetFrameFarCorners(TArray<FVector>& OutLocal) const override;	// BODY: W

	/** The gate is open right now (not live). */
	bool IsGateOpen() const { return bGateOpen; }

	/** Julian's feet are past the gate plane (local X > PassedDepth): he is through. */
	bool HasPassed(const AJulian* Julian) const;	// BODY: W

	/** Half the gate's width (between the arch's posts), cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Scanner")
	float GateHalfWidth = 160.f;

	/** How far the frame reaches out in front of the gate, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Scanner")
	float FrameDepth = 180.f;

	/** The gate block's thickness along X, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Scanner")
	float GateThickness = 30.f;

	/** The gate block's and the bar's height, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Scanner")
	float GateHeight = 240.f;

	/** The bar's height above the ground while shut, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Scanner")
	float BarHeight = 100.f;

	/** The eye on the lintel, local, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Scanner", meta = (MakeEditWidget = true))
	FVector EyeOffset = FVector(-14.f, 0.f, 251.f);

	/** Julian counts as through this far past the gate plane, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Scanner")
	float PassedDepth = 60.f;

	/** Height of the frame lines' upper corners, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Scanner")
	float FrameLineHeight = 200.f;

	/** Blocks the Pawn channel while the gate is shut; no collision while open. Hidden. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UBoxComponent> GateBlock;

	/** The bar across the gate: red at BarHeight while shut, gold on the floor while open. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> GateBar;

	/** The red eye on the lintel (the tally sits beside it). */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> Eye;

	// --- QA sight: a thing in the world with a state (Marwan's note 18) ----------------------------------------------
	// Its eye and a lamp on each post glow red while it scans and green while it is blinded or masked (the gate open);
	// a soft red light from the lintel washes the ground it reads (the frame box), green while it is open. The bar is a
	// velvet rope. Look only: HoldsJulian and the gate are as before.

	/** The lamps' and the eye's glow (values above 1 bloom): scanning, and open. */
	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	FLinearColor LampLiveColour = FLinearColor(4.f, 0.3f, 0.15f);

	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	FLinearColor LampOpenColour = FLinearColor(0.3f, 3.2f, 0.8f);

	/** The lamps on the posts: height above the ground, how far in front of the gate (local X), how far in from the
	 *  posts' line (GateHalfWidth), cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	float LampHeight = 205.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	float LampForward = -18.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	float LampInset = 4.f;

	/** The light from the lintel on the ground it reads: candelas while scanning, the share left while open, the
	 *  colours, and its glow in the haze. */
	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0"))
	float ScanLightCandelas = 220.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0", ClampMax = "1"))
	float OpenLightScale = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	FLinearColor ScanLightLiveColour = FLinearColor(1.f, 0.22f, 0.12f);

	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	FLinearColor ScanLightOpenColour = FLinearColor(0.35f, 1.f, 0.5f);

	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0"))
	float ScanLightHaze = 0.8f;

	/** The bar as a velvet rope (lit, not glowing), when the drawn overlays are off. */
	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	FLinearColor RopeColour = FLinearColor(0.28f, 0.03f, 0.05f);

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> LampLeft;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> LampRight;

	/** The light from the lintel (a spot light, no shadows). */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<USpotLightComponent> ScanLight;

protected:
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	bool bGateOpen = false;

	/** QA sight: the base (planning lift), then the lamps, the eye and the scan light, this frame. */
	virtual void UpdateWorldCues(float DeltaSeconds) override;

	/** The bar's look for the gate's state: the old red / gold signal with the drawn overlays, else the rope. */
	void ApplyBarLook();

private:
	/** Sizes the frame rectangle, the gate block, the bar, the eye, the tally and the click volume (at the eye).
	 *  From OnConstruction and BeginPlay. */
	void LayoutScanner();

	/** Opens or shuts the gate (collision, the bar's place and colour), this frame; logs a change. */
	void SetGateOpen(bool bOpen);
};
