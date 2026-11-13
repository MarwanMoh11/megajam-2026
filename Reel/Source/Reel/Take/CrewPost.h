// Where Maya or Vex stands for a scene: the crew move to a new post between scenes (rule 1). Owner: S.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TakeTypes.h"
#include "CrewPost.generated.h"

class UStaticMeshComponent;

/**
 * Place it on the floor: the actor location is where the character's feet go, its yaw is the way they face and
 * look. ATakeDirector::PlaceCrewForScene(N) moves each crew character to its post with the highest Scene <= N (no
 * such post: it stays where the map put it). That runs when a scene starts (ASceneTrigger::Scene) and when a
 * checkpoint is restored. Julian only walks while the player is on him, so the crew move while the player is
 * looking at Julian.
 */
UCLASS()
class REEL_API ACrewPost : public AActor
{
	GENERATED_BODY()

public:
	ACrewPost();

	/** Whose post: B Maya, C Vex. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Post")
	ETakeCamera Who = ETakeCamera::B;

	/** The scene it is for. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Post")
	int32 Scene = 2;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<USceneComponent> Root;

	/** Editor-only arrow (a flat cube pointing along +X), hidden in game. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> Arrow;
};
