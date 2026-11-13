// Owner: (e). Constructor by the header pass; every "// BODY: (e)" is a stub to fill.

#include "FollowSpot.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TakeShapes.h"
#include "Watcher.h"

AFollowSpot::AFollowSpot()
{
	DeviceName = TEXT("SPOT");
	Effect = EDeviceEffect::Blind;
	Range = 3500.f;
	Duration = 4.f;
	Uses = 0;
	FiredFrom = ETakeCamera::B;

	// The lamp: a squat cylinder on its stand height (place the actor where Maya's lamp stands).
	Body->SetStaticMesh(FTakeShapes::GetMesh(ETakeShape::Cylinder));
	Body->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	Body->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.8f));

	// The light beam: a thin cylinder StepDevice stretches from the lamp to the target; hidden until fired.
	BeamShape = FTakeShapes::CreateShape(this, TEXT("BeamShape"), ETakeShape::Cylinder, Root, FVector(0.f, 0.f, 80.f), FVector(0.3f, 0.3f, 1.f), FRotator::ZeroRotator, false);
	BeamShape->SetVisibility(false);
}

void AFollowSpot::BeginPlay()
{
	Super::BeginPlay();
	// BODY: (e)
}

bool AFollowSpot::CanFire(FString& OutWhyNot) const
{
	// BODY: (e)
	OutWhyNot = TEXT("Click a watcher to spot it");
	return false;
}

void AFollowSpot::StepDevice(float DeltaSeconds)
{
	// BODY: (e)
}

void AFollowSpot::GatherTargets(const TArray<TObjectPtr<AWatcher>>& All, TArray<AWatcher*>& Out) const
{
	// BODY: (e)
	Out.Reset();
}

FVector AFollowSpot::GetEffectPoint() const
{
	// BODY: (e)
	return GetActorLocation();
}

FString AFollowSpot::GetPipsText() const
{
	// BODY: (e)
	return DeviceName;
}

bool AFollowSpot::FireAt(AWatcher* Watcher, FString& OutWhyNot)
{
	// BODY: (e)
	OutWhyNot = TEXT("SPOT - not wired yet");
	return false;
}

AWatcher* AFollowSpot::GetTarget() const
{
	// BODY: (e)
	return nullptr;
}

float AFollowSpot::GetBusySecondsLeft() const
{
	// BODY: (e)
	return 0.f;
}
