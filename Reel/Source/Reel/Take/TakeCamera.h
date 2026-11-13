// A take camera: one of camera A's shots, or B's / C's fixed view. Owner: (b).

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "TakeTypes.h"
#include "TakeCamera.generated.h"

/** Place and aim it like any camera. Aspect ratio is not constrained (the HUD draws A's letterbox). */
UCLASS()
class REEL_API ATakeCamera : public ACameraActor
{
	GENERATED_BODY()

public:
	ATakeCamera();

	/** Which camera this belongs to. Camera A may have several (the shot list); B and C have one each in step 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Camera")
	ETakeCamera CameraId = ETakeCamera::A;

	/** The shot's name for the log and AShotZone::ShotName matching ("A_DOOR", "A_ROPE", "B_CATWALK"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Camera")
	FName ShotName;

	/** Camera A's first angle (the director cuts to it at the start). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Camera")
	bool bInitialShot = false;
};
