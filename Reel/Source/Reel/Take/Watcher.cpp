// Owner: (d). Constructor by the header pass; every "// BODY: (d)" is a stub to fill.

#include "Watcher.h"

#include "BarClockComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Julian.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TakeShapes.h"

AWatcher::AWatcher()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// The frame lies just above the floor; subclasses shape FrameMesh.
	FramePivot = CreateDefaultSubobject<USceneComponent>(TEXT("FramePivot"));
	FramePivot->SetupAttachment(Root);
	FramePivot->SetRelativeLocation(FVector(0.f, 0.f, 2.f));

	FrameMesh = FTakeShapes::CreateShape(this, TEXT("FrameMesh"), ETakeShape::Cube, FramePivot, FVector::ZeroVector, FVector::OneVector, FRotator::ZeroRotator, false);

	Tally = FTakeShapes::CreateShape(this, TEXT("Tally"), ETakeShape::Sphere, Root, FVector(0.f, 0.f, 200.f), FVector(0.15f, 0.15f, 0.15f), FRotator::ZeroRotator, false);

	ClickVolume = FTakeShapes::CreateClickVolume(this, TEXT("ClickVolume"), Root, FVector(70.f, 70.f, 100.f), FVector(0.f, 0.f, 100.f));
}

void AWatcher::BeginPlay()
{
	Super::BeginPlay();
	// BODY: (d)
}

void AWatcher::SetHovered(bool bInHovered)
{
	// BODY: (d)
}

void AWatcher::StepWatcher(float DeltaSeconds, UBarClockComponent* Clock)
{
	// BODY: (d)
}

bool AWatcher::HoldsJulian(const AJulian* Julian) const
{
	// BODY: (d)
	return false;
}

bool AWatcher::ApplyEffect(EDeviceEffect Effect, float Duration, const FVector& Point)
{
	// BODY: (d)
	return false;
}

void AWatcher::SetTally(bool bHolding)
{
	// BODY: (d)
}

float AWatcher::DistanceToPoint(const FVector& Point) const
{
	// BODY: (d)
	return FVector::Dist2D(GetActorLocation(), Point);
}

bool AWatcher::IsLive() const
{
	// BODY: (d)
	return State == EWatcherState::Watching;
}

FString AWatcher::GetStateText() const
{
	// BODY: (d)
	return TakeText::WatcherStateName(State);
}

FVector AWatcher::GetLabelPoint() const
{
	// BODY: (d)
	return GetActorLocation() + FVector(0.f, 0.f, LabelHeight);
}

void AWatcher::RefreshFrameLook()
{
	// BODY: (d)
}
