// A red-tape area on the floor: Julian is exposed while inside one (rule 5). Owner: (d).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RedTapeZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class URectLightComponent;

/**
 * Place it on the floor (actor location = the area's centre at floor height; yaw rotates it). The four tape
 * strips are thin red boxes laid out along the edges of Extent by OnConstruction, so they show in the editor and
 * in game. The Area box is an editor-only outline with no collision.
 */
UCLASS()
class REEL_API ARedTapeZone : public AActor
{
	GENERATED_BODY()

public:
	ARedTapeZone();

	/** Lays the four strips out along Extent and colours them red (editor and runtime). */
	virtual void OnConstruction(const FTransform& Transform) override;	// BODY: (d)

	virtual void BeginPlay() override;	// BODY: (d) (colour the strips if OnConstruction did not)

	/** Half-size of the taped area in cm (X, Y); Z is the height the area counts up to above the floor. */
	UPROPERTY(EditAnywhere, Category = "Take|Tape")
	FVector Extent = FVector(300.f, 300.f, 200.f);

	/** Width of the tape strips, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Tape")
	float TapeWidth = 12.f;

	/** True when WorldPoint is inside the area: 2D test in the zone's own frame (yaw honoured), Z within
	 *  [-50, Extent.Z * 2] of the actor's height. Pure query. */
	bool Contains(const FVector& WorldPoint) const;	// BODY: (d)

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UBoxComponent> Area;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> TapeNorth;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> TapeSouth;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> TapeEast;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> TapeWest;

	// --- QA sight: the ground itself says no (Marwan's note 18: no hatched tape) -------------------------------------
	// With the drawn overlays off (ATakeDirector::bDrawnSightOverlays) the hatched red strips go; the edges become a
	// thin brass threshold strip (lit, not glowing) and a cool light from above washes the area, apart from the warm
	// pools of the marks. Look only: Contains is the same box.

	/** A cool light over the area (a rect light the size of the area, pointing down, no shadows). */
	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	bool bCoolLight = true;

	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "50"))
	float CoolLightHeight = 380.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0"))
	float CoolLightCandelas = 260.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	FLinearColor CoolLightColour = FLinearColor(0.6f, 0.74f, 1.f);

	/** The light's barn doors (degrees from straight down, and length, cm): they keep the wash over the area. */
	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0", ClampMax = "90"))
	float CoolLightBarnDoorAngle = 35.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0"))
	float CoolLightBarnDoorLength = 40.f;

	/** The edges as a brass threshold strip: on, width (cm) and colour. */
	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	bool bThresholdStrips = true;

	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "1"))
	float ThresholdWidth = 5.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	FLinearColor ThresholdColour = FLinearColor(0.32f, 0.22f, 0.09f);

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<URectLightComponent> CoolLight;

private:
	/** Sizes the Area outline and the four strips to Extent and colours the strips red. From OnConstruction and BeginPlay. */
	void LayoutTape();
};
