// Owner: (b). Complete (constructor only).

#include "TakeCamera.h"

#include "Camera/CameraComponent.h"

ATakeCamera::ATakeCamera()
{
	PrimaryActorTick.bCanEverTick = false;
	if (UCameraComponent* Cam = GetCameraComponent())
	{
		Cam->SetConstraintAspectRatio(false);
		Cam->SetFieldOfView(60.f);
	}
}
