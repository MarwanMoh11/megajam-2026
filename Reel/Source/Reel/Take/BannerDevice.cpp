// Owner: (e). Constructor by the header pass; every "// BODY: (e)" is a stub to fill.

#include "BannerDevice.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TakeShapes.h"

ABannerDevice::ABannerDevice()
{
	DeviceName = TEXT("BANNER");
	Effect = EDeviceEffect::Mask;
	Range = 600.f;
	Duration = 10.f;
	Uses = 1;
	FiredFrom = ETakeCamera::B;

	// Place the actor at the batten, up in the flies. The matte (Body) hangs under it, rolled up (Up phase).
	Batten = FTakeShapes::CreateShape(this, TEXT("Batten"), ETakeShape::Cube, Root, FVector::ZeroVector, FVector(0.1f, MatteSize.Y / 100.f + 0.2f, 0.1f));

	Body->SetRelativeLocation(FVector(0.f, 0.f, -MatteSize.Z * 0.5f));
	Body->SetRelativeScale3D(MatteSize / 100.f);

	ClickVolume->SetRelativeLocation(FVector(0.f, 0.f, -MatteSize.Z * 0.5f));
	ClickVolume->InitBoxExtent(FVector(60.f, MatteSize.Y * 0.5f + 20.f, MatteSize.Z * 0.5f + 20.f));
}

void ABannerDevice::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// BODY: (e)
}

void ABannerDevice::BeginPlay()
{
	Super::BeginPlay();
	// BODY: (e)
}

bool ABannerDevice::CanFire(FString& OutWhyNot) const
{
	// BODY: (e)
	return Super::CanFire(OutWhyNot);
}

bool ABannerDevice::Fire()
{
	// BODY: (e)
	return false;
}

void ABannerDevice::StepDevice(float DeltaSeconds)
{
	// BODY: (e)
}

FVector ABannerDevice::GetEffectPoint() const
{
	// BODY: (e)
	return GetActorTransform().TransformPosition(EffectOffset);
}
