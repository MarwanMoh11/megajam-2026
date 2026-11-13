// ATakeDirector slice (u): the use target and firing devices (step 2), landing their effects (step 3). See
// TakeDirector.h.

#include "TakeDirector.h"

#include "Device.h"
#include "FollowSpot.h"
#include "Julian.h"
#include "TakeCharacter.h"
#include "TakePlayerController.h"
#include "Watcher.h"

FTakeUseTarget ATakeDirector::FindUseTarget(const ATakeCharacter* Who) const
{
	// BODY: (u)
	return FTakeUseTarget();
}

void ATakeDirector::StepUse()
{
	// BODY: (u)
}

bool ATakeDirector::FireDevice(ADevice* Device)
{
	if (!Device)
	{
		return false;
	}

	// Fire spends a use and changes something visible before it returns.
	FString WhyNot;
	if (Device->CanFire(WhyNot) && Device->Fire())
	{
		return true;
	}

	if (WhyNot.IsEmpty())
	{
		WhyNot = Device->DeviceName + TEXT(" - can't fire now");
	}
	Flash(WhyNot);

	FString ShortWhy = WhyNot;
	ShortWhy.RemoveFromStart(Device->DeviceName + TEXT(" - "));
	TAKE_LOG(TEXT("FIRE REFUSED %s: %s"), *Device->DeviceName, *ShortWhy);
	return false;
}

bool ATakeDirector::SpotWatcher(AWatcher* Watcher)
{
	if (!FollowSpot)
	{
		Flash(TEXT("No follow spot on this set"));
		TAKE_LOG(TEXT("SPOT REFUSED: no follow spot"));
		return false;
	}
	if (!Watcher)
	{
		return false;
	}

	FString WhyNot;
	if (!FollowSpot->FireAt(Watcher, WhyNot))
	{
		if (WhyNot.IsEmpty())
		{
			WhyNot = FollowSpot->DeviceName + TEXT(" - can't fire now");
		}
		Flash(WhyNot);
		TAKE_LOG(TEXT("SPOT REFUSED %s: %s"), *Watcher->GetLabel(), *WhyNot);
		return false;
	}
	return true;
}

void ATakeDirector::StepDevices(float DeltaSeconds)
{
	TArray<AWatcher*> Targets;
	for (ADevice* Device : Devices)
	{
		if (!Device)
		{
			continue;
		}

		Device->StepDevice(DeltaSeconds);
		if (!Device->LandedThisFrame())
		{
			continue;
		}

		Device->GatherTargets(Watchers, Targets);
		const FVector EffectPoint = Device->GetEffectPoint();
		int32 Caught = 0;
		FString CaughtNames;
		for (AWatcher* W : Targets)
		{
			if (W && W->ApplyEffect(Device->Effect, Device->Duration, EffectPoint))
			{
				++Caught;
				if (!CaughtNames.IsEmpty())
				{
					CaughtNames += TEXT(", ");
				}
				CaughtNames += W->GetLabel();
			}
		}

		const FString EffectWord = FString(TakeText::EffectName(Device->Effect)).ToLower();
		TAKE_LOG(TEXT("%s LANDED: %s %.1f s, caught %d (%s)"), *Device->DeviceName, *EffectWord, Device->Duration, Caught, *CaughtNames);
	}
}
