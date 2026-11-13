// Owner: S. Complete (the header pass): a placed transform, nothing else.

#include "CrewPost.h"

#include "Components/StaticMeshComponent.h"
#include "TakeShapes.h"

ACrewPost::ACrewPost()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Arrow = FTakeShapes::CreateShape(this, TEXT("Arrow"), ETakeShape::Cube, Root, FVector(30.f, 0.f, 2.f),
		FVector(0.6f, 0.15f, 0.02f), FRotator::ZeroRotator, false);
	Arrow->SetHiddenInGame(true);
}
