// Owner: (d). Constructor by the header pass; every "// BODY: (d)" is a stub to fill.

#include "PatrolGuard.h"

#include "BarClockComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Julian.h"
#include "PatrolPoint.h"
#include "TakeShapes.h"

APatrolGuard::APatrolGuard()
{
	Label = TEXT("GUARD");

	// Grey stand-in, 180 cm, feet at the actor location; a cap block in front shows his facing (+X).
	Body = FTakeShapes::CreateShape(this, TEXT("Body"), ETakeShape::Cylinder, Root, FVector(0.f, 0.f, 90.f), FVector(0.6f, 0.6f, 1.8f));
	Cap = FTakeShapes::CreateShape(this, TEXT("Cap"), ETakeShape::Cube, Root, FVector(25.f, 0.f, 165.f), FVector(0.25f, 0.4f, 0.12f));

	// The frame: the cone pitched +90 so its tip points back at him (-X) and its base lies at FrameRange,
	// squashed flat. Local Z (cone height) -> world X length; local Y -> width; local X -> thickness.
	// OnConstruction recomputes this from FrameRange / FrameHalfAngle.
	const float Width = 2.f * FrameRange * FMath::Tan(FMath::DegreesToRadians(FrameHalfAngle));
	FrameMesh->SetStaticMesh(FTakeShapes::GetMesh(ETakeShape::Cone));
	FrameMesh->SetRelativeLocation(FVector(FrameRange * 0.5f, 0.f, 0.f));
	FrameMesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	FrameMesh->SetRelativeScale3D(FVector(0.02f, Width / 100.f, FrameRange / 100.f));

	Tally->SetRelativeLocation(FVector(0.f, 0.f, 200.f));
}

void APatrolGuard::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// BODY: (d)
}

void APatrolGuard::BeginPlay()
{
	Super::BeginPlay();
	// BODY: (d)
}

void APatrolGuard::StepWatcher(float DeltaSeconds, UBarClockComponent* Clock)
{
	// BODY: (d)
}

bool APatrolGuard::HoldsJulian(const AJulian* Julian) const
{
	// BODY: (d)
	return false;
}

bool APatrolGuard::ApplyEffect(EDeviceEffect Effect, float Duration, const FVector& Point)
{
	// BODY: (d)
	return false;
}

FString APatrolGuard::GetStateText() const
{
	// BODY: (d)
	return Super::GetStateText();
}

void APatrolGuard::EvaluatePatrol(const UBarClockComponent* Clock, FVector& OutLocation, float& OutYaw) const
{
	// BODY: (d)
	OutLocation = GetActorLocation();
	OutYaw = GetActorRotation().Yaw;
}
