// Owner: (m). Constructor by the header pass; every "// BODY: (m)" is a stub to fill.

#include "Mark.h"

#include "Components/StaticMeshComponent.h"
#include "TakeShapes.h"
#include "TakeTypes.h"

AMark::AMark()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// Collision: every shape here is NoCollision (FTakeShapes::SetupShape): no Pawn or Camera blocking.
	// An 80 cm gold disc, 2 cm thick, sitting on the floor.
	Disc = FTakeShapes::CreateShape(this, TEXT("Disc"), ETakeShape::Cylinder, Root, FVector(0.f, 0.f, 1.f), FVector(0.8f, 0.8f, 0.02f), FRotator::ZeroRotator, false);

	// The gold X under Julian, hidden until he stands here.
	CrossA = FTakeShapes::CreateShape(this, TEXT("CrossA"), ETakeShape::Cube, Root, FVector(0.f, 0.f, 3.f), FVector(1.0f, 0.12f, 0.02f), FRotator(0.f, 45.f, 0.f), false);
	CrossB = FTakeShapes::CreateShape(this, TEXT("CrossB"), ETakeShape::Cube, Root, FVector(0.f, 0.f, 3.f), FVector(1.0f, 0.12f, 0.02f), FRotator(0.f, -45.f, 0.f), false);
	CrossA->SetVisibility(false);
	CrossB->SetVisibility(false);
}

void AMark::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyLook();
}

void AMark::BeginPlay()
{
	Super::BeginPlay();
	bIsOccupied = false;
	ApplyLook();
}

void AMark::SetOccupied(bool bOccupied)
{
	bIsOccupied = bOccupied;
	ApplyLook();
}

FVector AMark::GetStandPoint() const
{
	return GetActorLocation();
}

void AMark::ApplyLook()
{
	if (!Disc || !CrossA || !CrossB)
	{
		return;
	}

	FTakeShapes::SetColour(Disc, TakeColors::Gold);
	FTakeShapes::SetColour(CrossA, TakeColors::GoldBright);
	FTakeShapes::SetColour(CrossB, TakeColors::GoldBright);
	CrossA->SetVisibility(bIsOccupied);
	CrossB->SetVisibility(bIsOccupied);
}
