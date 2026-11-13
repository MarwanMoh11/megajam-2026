// Owner: (d). Complete (constructor only).

#include "PatrolPoint.h"

#include "Components/StaticMeshComponent.h"
#include "TakeShapes.h"

APatrolPoint::APatrolPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// An editor-only arrow along +X (the facing the guard holds here).
	Arrow = FTakeShapes::CreateShape(this, TEXT("Arrow"), ETakeShape::Cube, Root, FVector(30.f, 0.f, 2.f), FVector(0.6f, 0.1f, 0.03f), FRotator::ZeroRotator, false);
	Arrow->SetHiddenInGame(true);
}
