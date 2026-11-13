// One stop on a guard's patrol. Owner: (d).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PatrolPoint.generated.h"

class UStaticMeshComponent;

/** Place on the floor. Its yaw is the guard's facing while he holds this point; Bars is how long he holds it. */
UCLASS()
class REEL_API APatrolPoint : public AActor
{
	GENERATED_BODY()

public:
	APatrolPoint();

	/** Bars the guard holds this point (facing its yaw) before moving on to the next, on a bar line. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Patrol", meta = (ClampMin = "1"))
	int32 Bars = 2;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<USceneComponent> Root;

	/** Editor-only arrow (a flat cube pointing along +X), hidden in game. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> Arrow;
};
