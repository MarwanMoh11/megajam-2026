// ATakeDirector slice (s): presses (step 2). See TakeDirector.h for the contract.

#include "TakeDirector.h"

#include "Device.h"
#include "FollowSpot.h"
#include "Julian.h"
#include "TakeCharacter.h"
#include "TakePlayerController.h"
#include "Watcher.h"

void ATakeDirector::StepPresses()
{
	if (!PC)
	{
		return;
	}

	TArray<FTakePress> Presses;
	PC->TakePresses(Presses);
	if (Presses.Num() == 0)
	{
		return;
	}

	// Cuts first (in queue order), then uses, then a new take.
	for (const FTakePress& QueuedPress : Presses)
	{
		if (QueuedPress.Type == ETakePressType::Cut)
		{
			HandleCut(QueuedPress.Camera, QueuedPress);
		}
	}
	for (const FTakePress& QueuedPress : Presses)
	{
		if (QueuedPress.Type == ETakePressType::Use)
		{
			HandleUse(QueuedPress);
		}
	}
	const bool bNewTakeAsked = Presses.ContainsByPredicate([](const FTakePress& QueuedPress)
	{
		return QueuedPress.Type == ETakePressType::NewTake;
	});
	if (bNewTakeAsked)
	{
		NewTake();
	}
}

void ATakeDirector::HandleCut(ETakeCamera Camera, const FTakePress& Press)
{
	// BODY: (s)
}

void ATakeDirector::HandleUse(const FTakePress& Press)
{
	// BODY: (s)
}
