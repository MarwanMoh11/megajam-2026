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
	// BODY: (b)
	return false;
}

ATakeCamera* AShotZone::ResolveShot(const TArray<TObjectPtr<ATakeCamera>>& Cameras) const
{
	// BODY: (b)
	return Shot;
}
