// A zone that names one camera-A shot: while Julian is inside, that is camera A's current shot (the film's own
// cut, not the player's). Owner: (b).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShotZone.generated.h"

class UBoxComponent;
class ATakeCamera;

/** Tested geometrically by the director (Contains), not by overlap events. */
UCLASS()
class REEL_API AShotZone : public AActor
{
	GENERATED_BODY()

public:
	AShotZone();

	/** Applies Extent to the editor box. */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Half-size of the zone, cm (actor location is the centre; yaw rotates it). */
	UPROPERTY(EditAnywhere, Category = "Take|Shot")
	FVector Extent = FVector(500.f, 500.f, 300.f);

	/** The shot this zone selects (a direct reference: set it from Python or the details panel). */
	UPROPERTY(EditAnywhere, Category = "Take|Shot")
	TObjectPtr<ATakeCamera> Shot;

	/** Used when Shot is empty: the camera-A ATakeCamera with this ShotName. */
	UPROPERTY(EditAnywhere, Category = "Take|Shot")
	FName ShotName;

	/** Where zones overlap, the higher priority wins. */
	UPROPERTY(EditAnywhere, Category = "Take|Shot")
	int32 Priority = 0;

	/** True when WorldPoint is inside the box (zone's own frame, yaw honoured). Pure query. */
	bool Contains(const FVector& WorldPoint) const;	// BODY: (b)

	/** Shot, or else the camera in Cameras with CameraId A and ShotName == ShotName; null if neither. */
	ATakeCamera* ResolveShot(const TArray<TObjectPtr<ATakeCamera>>& Cameras) const;	// BODY: (b)

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UBoxComponent> Box;
};
