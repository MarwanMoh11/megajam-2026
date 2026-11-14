// ATakeDirector slice (e): the blooper (step 7), the freeze, the reload, R. See TakeDirector.h for the contract.

#include "TakeDirector.h"

#include "BarClockComponent.h"
#include "ComposureComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TakeCharacter.h"
#include "TakeGameInstance.h"
#include "Watcher.h"

void ATakeDirector::RequestBlooper(EBlooperReason Reason)
{
	if (Reason == EBlooperReason::None || Phase != ETakePhase::Rolling || bReloadRequested)
	{
		return;
	}
	// The first reason in a frame wins.
	if (PendingBlooper == EBlooperReason::None)
	{
		PendingBlooper = Reason;
	}
}

void ATakeDirector::StepBlooperCheck(float DeltaSeconds)
{
	if (Phase != ETakePhase::Rolling || bReloadRequested)
	{
		PendingBlooper = EBlooperReason::None;
		return;
	}

	EBlooperReason Reason = PendingBlooper;
	PendingBlooper = EBlooperReason::None;
	if (Reason == EBlooperReason::None && Composure && Composure->IsEmpty())
	{
		Reason = EBlooperReason::ComposureZero;
	}
	if (Reason == EBlooperReason::None)
	{
		return;
	}

	// The blooper: frozen from this frame, the slate shows (the HUD draws after the late tick).
	LastBlooperReason = Reason;
	SetPhase(ETakePhase::BlooperFreeze);
	FreezeSecondsLeft = BlooperFreezeSeconds;
	// Everyone stops where they stand: nobody slides on through the beam under the slate (the level reloads next).
	for (ATakeCharacter* Character : Characters)
	{
		if (UCharacterMovementComponent* Move = Character ? Character->GetCharacterMovement() : nullptr)
		{
			Move->StopMovementImmediately();
			Move->DisableMovement();
		}
	}
	SaveCrewPlaces();
	if (UTakeGameInstance* GI = GetTakeGameInstance())
	{
		// The blooper reel (rule 11): recorded before NoteBlooper, so it names the checkpoint it goes again from.
		FTakeBlooperRecord Record;
		Record.Take = GI->TakeNumber;
		Record.Bar = Clock ? Clock->GetBar() : 1;
		Record.Scene = Scene;
		Record.Reason = Reason;
		Record.CheckpointName = GI->CheckpointName;
		if (Reason == EBlooperReason::ComposureZero)
		{
			for (const AWatcher* EachWatcher : Watchers)
			{
				if (EachWatcher && EachWatcher->HasTally())
				{
					Record.CaughtBy = EachWatcher->GetLabel();
					break;
				}
			}
		}
		GI->BlooperReel.Add(Record);
		GI->NoteBlooper(Reason);
	}
	TAKE_LOG(TEXT("BLOOPER %s"), TakeText::BlooperReasonName(Reason));
}

void ATakeDirector::StepFreeze(float DeltaSeconds)
{
	if (bReloadRequested)
	{
		return;
	}
	FreezeSecondsLeft -= DeltaSeconds;
	if (FreezeSecondsLeft <= 0.f)
	{
		FreezeSecondsLeft = 0.f;
		ReloadLevel();
	}
}

void ATakeDirector::ReloadLevel()
{
	if (bReloadRequested)
	{
		return;
	}
	bReloadRequested = true;

	// GetCurrentLevelName strips the play-in-editor prefix; the game instance survives the load, the rest restarts.
	const FString ReloadName = UGameplayStatics::GetCurrentLevelName(this, true);
	TAKE_LOG(TEXT("RELOAD %s"), *ReloadName);
	UGameplayStatics::OpenLevel(this, FName(*ReloadName));
}

void ATakeDirector::NewTake()
{
	if (bReloadRequested)
	{
		return;
	}

	int32 NewTakeNumber = 1;
	if (UTakeGameInstance* GI = GetTakeGameInstance())
	{
		GI->NoteNewTake();
		NewTakeNumber = GI->TakeNumber;
	}
	Flash(FString::Printf(TEXT("TAKE %d"), NewTakeNumber), TakeColors::White, 1.5f);
	TAKE_LOG(TEXT("NEW TAKE %d"), NewTakeNumber);
	ReloadLevel();
}
