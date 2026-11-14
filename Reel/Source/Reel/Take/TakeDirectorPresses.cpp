// ATakeDirector slice (s): presses (step 2). See TakeDirector.h for the contract.

#include "TakeDirector.h"

#include "Device.h"
#include "FollowSpot.h"
#include "Julian.h"
#include "TakeCharacter.h"
#include "TakeGameInstance.h"
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
	// Step 3: Confirm starts the title card and turns the end pages; while the take runs it says how to play.
	for (const FTakePress& QueuedPress : Presses)
	{
		if (QueuedPress.Type == ETakePressType::Confirm)
		{
			Flash(TEXT("WASD walk   E use   1 2 3 cut   R new take"), TakeColors::Grey, 2.f);
			TAKE_LOG(TEXT("CONFIRM (%s): the controls line"), *QueuedPress.Source);
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
	if (!PC)
	{
		return;
	}
	const TCHAR* Letter = TakeText::CameraLetter(Camera);

	if (PC->IsInFlight())
	{
		Flash(TEXT("Mid-cut"), TakeColors::Grey, 0.8f);
		TAKE_LOG(TEXT("CUT -> %s (%s) refused: mid-cut"), Letter, *Press.Source);
		return;
	}

	ATakeCharacter* To = GetCharacter(Camera);
	if (!To)
	{
		Flash(FString::Printf(TEXT("No camera %s on this set"), Letter), TakeColors::Grey);
		TAKE_LOG(TEXT("CUT -> %s (%s) refused: no such character"), Letter, *Press.Source);
		return;
	}
	if (To == PC->GetCurrentCharacter())
	{
		Flash(FString::Printf(TEXT("Already on %s"), To->GetCrewName()), TakeColors::Grey, 1.f);
		TAKE_LOG(TEXT("CUT -> %s (%s) refused: already there"), Letter, *Press.Source);
		return;
	}

	// The view leaves the old character inside this call.
	PC->StartFlight(To);
	int32 CutsThisTake = 0;
	if (UTakeGameInstance* GI = GetTakeGameInstance())
	{
		// Only a flight that really started counts (rule 11's stats).
		if (PC->IsInFlight())
		{
			++GI->Cuts;
		}
		CutsThisTake = GI->Cuts;
	}
	TAKE_LOG(TEXT("CUT -> %s (%s) cuts=%d"), Letter, *Press.Source, CutsThisTake);
}

void ATakeDirector::HandleUse(const FTakePress& Press)
{
	if (!PC)
	{
		return;
	}
	if (PC->IsInFlight())
	{
		Flash(TEXT("Mid-cut"), TakeColors::Grey, 0.8f);
		TAKE_LOG(TEXT("USE (%s) refused: mid-cut"), *Press.Source);
		return;
	}

	const ATakeCharacter* Who = PC->GetCurrentCharacter();
	if (!Who)
	{
		TAKE_LOG(TEXT("USE (%s) refused: no current character"), *Press.Source);
		return;
	}
	const TCHAR* WhoName = Who->GetCrewName();

	if (Who == Julian.Get())
	{
		Flash(TEXT("[2] MAYA or [3] VEX work the set"), TakeColors::Grey);
		TAKE_LOG(TEXT("USE %s: refused, the crew work the set"), WhoName);
		return;
	}

	ADevice* Device = CurrentUse.Device;
	if (!Device)
	{
		Flash(TEXT("Nothing in reach - stand in a device's ring"), TakeColors::Grey);
		TAKE_LOG(TEXT("USE %s: refused, nothing in reach"), WhoName);
		return;
	}
	if (!CurrentUse.bCanFire)
	{
		Flash(CurrentUse.Prompt, TakeColors::Grey);
		TAKE_LOG(TEXT("USE %s: %s -> refused %s"), WhoName, *Device->DeviceName, *CurrentUse.Prompt);
		return;
	}

	bool bFired = false;
	if (AFollowSpot* Spot = Cast<AFollowSpot>(Device))
	{
		FollowSpot = Spot;	// the spot in reach, not just the level's first
		bFired = SpotWatcher(CurrentUse.Watcher);
	}
	else
	{
		bFired = FireDevice(Device);
	}
	TAKE_LOG(TEXT("USE %s: %s -> %s"), WhoName, *Device->DeviceName, bFired ? TEXT("fired") : TEXT("refused"));

	// The device changed inside this call: the prompt, the highlights and a second Use press this frame must see it.
	if (bFired)
	{
		StepUse();
	}
}
