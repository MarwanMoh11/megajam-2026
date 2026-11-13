// Owner: (e). Constructor by the header pass; every "// BODY: (e)" is a stub to fill.

#include "CartDevice.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TakeShapes.h"

ACartDevice::ACartDevice()
{
	DeviceName = TEXT("CART");
	Effect = EDeviceEffect::Lure;
	Range = 1200.f;
	Duration = 6.f;
	Uses = 1;
	FiredFrom = ETakeCamera::B;

	// The cart: a 100 x 60 x 90 block at its door (Body moves along the roll; the actor stays at the door).
	Body->SetRelativeLocation(FVector(0.f, 0.f, 45.f));
	Body->SetRelativeScale3D(FVector(1.0f, 0.6f, 0.9f));

	CrashMarker = FTakeShapes::CreateShape(this, TEXT("CrashMarker"), ETakeShape::Cube, Root, CrashOffset + FVector(0.f, 0.f, 1.f), FVector(0.4f, 0.4f, 0.02f), FRotator(0.f, 45.f, 0.f), false);
	CrashMarker->SetHiddenInGame(true);
}

void ACartDevice::BeginPlay()
{
	Super::BeginPlay();
	// BODY: (e)
}

bool ACartDevice::Fire()
{
	// BODY: (e)
	return false;
}

void ACartDevice::StepDevice(float DeltaSeconds)
{
	// BODY: (e)
}

FVector ACartDevice::GetEffectPoint() const
{
	// BODY: (e)
	return GetActorTransform().TransformPosition(CrashOffset);
}
