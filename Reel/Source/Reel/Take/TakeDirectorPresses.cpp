// ATakeDirector slice (b): presses (step 2) and the shot list (step 4). See TakeDirector.h for the contract.

#include "TakeDirector.h"

#include "Device.h"
#include "FollowSpot.h"
#include "Julian.h"
#include "Mark.h"
#include "ShotZone.h"
#include "TakeCamera.h"
#include "TakeClickable.h"
#include "TakePlayerController.h"
#include "Watcher.h"

void ATakeDirector::StepPresses()
{
	// BODY: (b)
	if (PC)
	{
		TArray<FTakePress> Presses;
		PC->TakePresses(Presses);
	}
}

void ATakeDirector::HandleCut(ETakeCamera Camera, const FTakePress& Press)
{
	// BODY: (b)
}

void ATakeDirector::HandleClick(const FTakePress& Press)
{
	// BODY: (b)
}

void ATakeDirector::StepShotList()
{
	// BODY: (b)
}
