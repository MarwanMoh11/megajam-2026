// Owner: (d). Constructor by the header pass; every "// BODY: (d)" is a stub to fill.

#include "BeamWatcher.h"

#include "BarClockComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Julian.h"
#include "TakeShapes.h"

ABeamWatcher::ABeamWatcher()
{
	Label = TEXT("BEAM");

	// Two 120 cm posts; post B at EndB. OnConstruction re-places post B, the line and the click volume.
	PostA = FTakeShapes::CreateShape(this, TEXT("PostA"), ETakeShape::Cylinder, Root, FVector(0.f, 0.f, 60.f), FVector(0.15f, 0.15f, 1.2f));
	PostB = FTakeShapes::CreateShape(this, TEXT("PostB"), ETakeShape::Cylinder, Root, EndB + FVector(0.f, 0.f, 60.f), FVector(0.15f, 0.15f, 1.2f));

	// The line: a thin cube from post A to post B at BeamHeight (FramePivot sits 2 cm above the floor).
	const float Length = EndB.Size2D();
	FrameMesh->SetRelativeLocation(EndB * 0.5f + FVector(0.f, 0.f, BeamHeight - 2.f));
	FrameMesh->SetRelativeRotation(FRotator(0.f, EndB.Rotation().Yaw, 0.f));
	FrameMesh->SetRelativeScale3D(FVector(Length / 100.f, BeamThickness / 100.f, BeamThickness / 100.f));

	Tally->SetRelativeLocation(FVector(0.f, 0.f, 135.f));

	ClickVolume->SetRelativeLocation(EndB * 0.5f + FVector(0.f, 0.f, BeamHeight));
	ClickVolume->InitBoxExtent(FVector(Length * 0.5f + 20.f, 40.f, 80.f));
	ClickVolume->SetRelativeRotation(FRotator(0.f, EndB.Rotation().Yaw, 0.f));
}

void ABeamWatcher::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// BODY: (d)
}

void ABeamWatcher::BeginPlay()
{
	Super::BeginPlay();
	// BODY: (d)
}

void ABeamWatcher::StepWatcher(float DeltaSeconds, UBarClockComponent* Clock)
{
	// BODY: (d)
}

bool ABeamWatcher::HoldsJulian(const AJulian* Julian) const
{
	// BODY: (d)
	return false;
}

bool ABeamWatcher::ApplyEffect(EDeviceEffect Effect, float Duration, const FVector& Point)
{
	// BODY: (d)
	return false;
}

float ABeamWatcher::DistanceToPoint(const FVector& Point) const
{
	// BODY: (d)
	return Super::DistanceToPoint(Point);
}

bool ABeamWatcher::IsLive() const
{
	// BODY: (d)
	return false;
}

FString ABeamWatcher::GetStateText() const
{
	// BODY: (d)
	return Super::GetStateText();
}

bool ABeamWatcher::TouchesJulian(const AJulian* Julian) const
{
	// BODY: (d)
	return false;
}

FVector ABeamWatcher::GetEndA() const
{
	// BODY: (d)
	return GetActorLocation();
}

FVector ABeamWatcher::GetEndB() const
{
	// BODY: (d)
	return GetActorLocation();
}
