// A red-tape area on the floor: Julian is exposed while inside one (rule 5). Owner: (a).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RedTapeZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

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
	virtual void OnConstruction(const FTransform& Transform) override;	// BODY: (a)

	virtual void BeginPlay() override;	// BODY: (a) (colour the strips if OnConstruction did not)

	/** Half-size of the taped area in cm (X, Y); Z is the height the area counts up to above the floor. */
	UPROPERTY(EditAnywhere, Category = "Take|Tape")
	FVector Extent = FVector(300.f, 300.f, 200.f);

	/** Width of the tape strips, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Tape")
	float TapeWidth = 12.f;

	/** True when WorldPoint is inside the area: 2D test in the zone's own frame (yaw honoured), Z within
	 *  [-50, Extent.Z * 2] of the actor's height. Pure query. */
	bool Contains(const FVector& WorldPoint) const;	// BODY: (a)

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

private:
	/** Sizes the Area outline and the four strips to Extent and colours the strips red. From OnConstruction and BeginPlay. */
	void LayoutTape();
};
