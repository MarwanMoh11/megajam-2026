// Owner: (c). Constructor by the header pass; every "// BODY: (c)" is a stub to fill.

#include "Mark.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TakeShapes.h"
#include "TakeTypes.h"

AMark::AMark()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// An 80 cm gold disc, 2 cm thick, sitting on the floor.
	Disc = FTakeShapes::CreateShape(this, TEXT("Disc"), ETakeShape::Cylinder, Root, FVector(0.f, 0.f, 1.f), FVector(0.8f, 0.8f, 0.02f), FRotator::ZeroRotator, false);

	// The cued ring: instances (small flat blocks) laid out in BeginPlay; hidden until cued.
	Ring = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Ring"));
	Ring->SetupAttachment(Root);
	FTakeShapes::SetupShape(Ring, ETakeShape::Cube, false);
	Ring->SetVisibility(false);

	// The gold X under Julian, hidden until he stands here.
	CrossA = FTakeShapes::CreateShape(this, TEXT("CrossA"), ETakeShape::Cube, Root, FVector(0.f, 0.f, 3.f), FVector(1.0f, 0.12f, 0.02f), FRotator(0.f, 45.f, 0.f), false);
	CrossB = FTakeShapes::CreateShape(this, TEXT("CrossB"), ETakeShape::Cube, Root, FVector(0.f, 0.f, 3.f), FVector(1.0f, 0.12f, 0.02f), FRotator(0.f, -45.f, 0.f), false);
	CrossA->SetVisibility(false);
	CrossB->SetVisibility(false);

	// Generous click volume over the disc.
	ClickVolume = FTakeShapes::CreateClickVolume(this, TEXT("ClickVolume"), Root, FVector(70.f, 70.f, 40.f), FVector(0.f, 0.f, 40.f));
}

void AMark::BeginPlay()
{
	Super::BeginPlay();
	// BODY: (c)
}

void AMark::SetHovered(bool bHovered)
{
	// BODY: (c)
}

void AMark::SetCued(bool bCued)
{
	// BODY: (c)
}

void AMark::SetOccupied(bool bOccupied)
{
	// BODY: (c)
}

FVector AMark::GetStandPoint() const
{
	// BODY: (c)
	return GetActorLocation();
}
