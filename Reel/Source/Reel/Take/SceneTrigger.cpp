// Owner: S. Constructor by the header pass; every "// BODY: S" is a stub to fill.

#include "SceneTrigger.h"

#include "Components/BoxComponent.h"
#include "TakeShapes.h"

ASceneTrigger::ASceneTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Area = CreateDefaultSubobject<UBoxComponent>(TEXT("Area"));
	Area->SetupAttachment(Root);
	FTakeShapes::SetupEditorOnlyBox(Area, Extent, FColor(255, 200, 40));
	Area->SetRelativeLocation(FVector(0.f, 0.f, Extent.Z));
}

void ASceneTrigger::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (Area)
	{
		FTakeShapes::SetupEditorOnlyBox(Area, Extent, FColor(255, 200, 40));
		Area->SetRelativeLocation(FVector(0.f, 0.f, Extent.Z));
	}
}

bool ASceneTrigger::Contains(const FVector& WorldPoint) const
{
	// The same test as ARedTapeZone::Contains: the trigger's own frame, yaw only, Extent scaled with the actor.
	const FVector Delta = WorldPoint - GetActorLocation();
	const FVector Local = FRotator(0.f, GetActorRotation().Yaw, 0.f).UnrotateVector(Delta);
	const FVector Scaled = Extent * GetActorScale3D().GetAbs();
	return FMath::Abs(Local.X) <= Scaled.X
		&& FMath::Abs(Local.Y) <= Scaled.Y
		&& Delta.Z >= -50.f
		&& Delta.Z <= Scaled.Z * 2.f;
}
