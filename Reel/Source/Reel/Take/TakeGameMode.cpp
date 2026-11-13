// Owner: header pass (shared, complete).

#include "TakeGameMode.h"

#include "EngineUtils.h"
#include "TakeDirector.h"
#include "TakeHUD.h"
#include "TakePlayerController.h"
#include "TakeTypes.h"

ATakeGameMode::ATakeGameMode()
{
	PlayerControllerClass = ATakePlayerController::StaticClass();
	HUDClass = ATakeHUD::StaticClass();
	DefaultPawnClass = nullptr;
	SpectatorClass = nullptr;
	bStartPlayersAsSpectators = false;
}

void ATakeGameMode::StartPlay()
{
	UWorld* World = GetWorld();
	if (World && !TActorIterator<ATakeDirector>(World))
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		World->SpawnActor<ATakeDirector>(ATakeDirector::StaticClass(), FTransform::Identity, Params);
		UE_LOG(LogTake, Log, TEXT("TakeGameMode: the level had no TakeDirector; spawned one."));
	}
	Super::StartPlay();
}

void ATakeGameMode::RestartPlayer(AController* NewPlayer)
{
	// No pawn by design.
}
