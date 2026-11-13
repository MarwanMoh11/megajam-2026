// Owner: (e). Constructor by the header pass; every "// BODY: (e)" is a stub to fill.

#include "Device.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TakeShapes.h"
#include "Watcher.h"

ADevice::ADevice()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// A 1 m block on the floor; subclasses reshape it.
	Body = FTakeShapes::CreateShape(this, TEXT("Body"), ETakeShape::Cube, Root, FVector(0.f, 0.f, 50.f), FVector::OneVector);

	ClickVolume = FTakeShapes::CreateClickVolume(this, TEXT("ClickVolume"), Root, FVector(80.f, 80.f, 100.f), FVector(0.f, 0.f, 100.f));
}

void ADevice::BeginPlay()
{
	Super::BeginPlay();
	// BODY: (e)
	UsesLeft = Uses;
}

void ADevice::SetHovered(bool bInHovered)
{
	// BODY: (e)
}

bool ADevice::CanFire(FString& OutWhyNot) const
{
	// BODY: (e)
	OutWhyNot = DeviceName + TEXT(" - not wired yet");
	return false;
}

bool ADevice::Fire()
{
	// BODY: (e)
	return false;
}

void ADevice::StepDevice(float DeltaSeconds)
{
	// BODY: (e)
}

bool ADevice::LandedThisFrame() const
{
	// BODY: (e)
	return false;
}

void ADevice::GatherTargets(const TArray<TObjectPtr<AWatcher>>& All, TArray<AWatcher*>& Out) const
{
	// BODY: (e)
	Out.Reset();
}

FVector ADevice::GetEffectPoint() const
{
	// BODY: (e)
	return GetActorLocation();
}

FString ADevice::GetPipsText() const
{
	// BODY: (e)
	return DeviceName;
}

FVector ADevice::GetLabelPoint() const
{
	// BODY: (e)
	return GetActorLocation() + FVector(0.f, 0.f, LabelHeight);
}
