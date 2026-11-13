// Owner: (b). Constructor and OnConstruction by the header pass; every "// BODY: (b)" is a stub to fill.

#include "ShotZone.h"

#include "Components/BoxComponent.h"
#include "TakeCamera.h"
#include "TakeShapes.h"

AShotZone::AShotZone()
{
	PrimaryActorTick.bCanEverTick = false;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	RootComponent = Box;
	FTakeShapes::SetupEditorOnlyBox(Box, Extent, FColor(60, 140, 255));
}

void AShotZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (Box)
	{
		Box->SetBoxExtent(Extent, false);
	}
}

bool AShotZone::Contains(const FVector& WorldPoint) const
{
	// Into the zone's own frame (location, rotation and scale, like the drawn box), then an axis-aligned test.
	const FVector LocalPoint = GetActorTransform().InverseTransformPosition(WorldPoint);
	return FMath::Abs(LocalPoint.X) <= Extent.X
		&& FMath::Abs(LocalPoint.Y) <= Extent.Y
		&& FMath::Abs(LocalPoint.Z) <= Extent.Z;
}

ATakeCamera* AShotZone::ResolveShot(const TArray<TObjectPtr<ATakeCamera>>& Cameras) const
{
	if (Shot)
	{
		return Shot;
	}
	if (ShotName.IsNone())
	{
		return nullptr;
	}
	for (ATakeCamera* Candidate : Cameras)
	{
		if (Candidate && Candidate->CameraId == ETakeCamera::A && Candidate->ShotName == ShotName)
		{
			return Candidate;
		}
	}
	return nullptr;
}
