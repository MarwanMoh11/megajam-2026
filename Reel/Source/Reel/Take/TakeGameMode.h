// The take's game mode: the player is the director, so no pawn. Owner: header pass (shared, complete).
// Set it as the test map's GameMode Override (World Settings).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TakeGameMode.generated.h"

UCLASS()
class REEL_API ATakeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/** ATakePlayerController, ATakeHUD, no default pawn, no spectator pawn. */
	ATakeGameMode();

	/** Spawns an ATakeDirector if the level has none, then starts play (so every actor gets BeginPlay together). */
	virtual void StartPlay() override;

	/** No pawn by design: the player only cuts and clicks. Leaves the controller unpossessed and silent. */
	virtual void RestartPlayer(AController* NewPlayer) override;
};
