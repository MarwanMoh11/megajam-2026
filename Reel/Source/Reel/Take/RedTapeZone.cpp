// Owner: (a). Constructor by the header pass; every "// BODY: (a)" is a stub to fill.

#include "RedTapeZone.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TakeShapes.h"
#include "TakeTypes.h"

ARedTapeZone::ARedTapeZone()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Area = CreateDefaultSubobject<UBoxComponent>(TEXT("Area"));
	Area->SetupAttachment(Root);
	FTakeShapes::SetupEditorOnlyBox(Area, Extent, FColor(230, 20, 20));

	// Four flat strips (cubes squashed to tape); OnConstruction lays them along the edges of Extent.
	const FVector Flat(1.f, 1.f, 0.01f);
	TapeNorth = FTakeShapes::CreateShape(this, TEXT("TapeNorth"), ETakeShape::Cube, Root, FVector(Extent.X, 0.f, 1.f), Flat, FRotator::ZeroRotator, false);
	TapeSouth = FTakeShapes::CreateShape(this, TEXT("TapeSouth"), ETakeShape::Cube, Root, FVector(-Extent.X, 0.f, 1.f), Flat, FRotator::ZeroRotator, false);
	TapeEast = FTakeShapes::CreateShape(this, TEXT("TapeEast"), ETakeShape::Cube, Root, FVector(0.f, Extent.Y, 1.f), Flat, FRotator::ZeroRotator, false);
	TapeWest = FTakeShapes::CreateShape(this, TEXT("TapeWest"), ETakeShape::Cube, Root, FVector(0.f, -Extent.Y, 1.f), Flat, FRotator::ZeroRotator, false);
}

void ARedTapeZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// BODY: (a)
}

void ARedTapeZone::BeginPlay()
{
	Super::BeginPlay();
	// BODY: (a)
}

bool ARedTapeZone::Contains(const FVector& WorldPoint) const
{
	// BODY: (a)
	return false;
}
