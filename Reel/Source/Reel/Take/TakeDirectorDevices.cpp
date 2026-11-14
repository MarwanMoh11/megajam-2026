// ATakeDirector slice (u): the use target and firing devices (step 2), landing their effects (step 3). See
// TakeDirector.h.

#include "TakeDirector.h"
#include "TakeAudio.h"

#include "ComposureComponent.h"
#include "Device.h"
#include "FollowSpot.h"
#include "Julian.h"
#include "TakeCharacter.h"
#include "TakePlayerController.h"
#include "Watcher.h"

FTakeUseTarget ATakeDirector::FindUseTarget(const ATakeCharacter* Who) const
{
	FTakeUseTarget Result;
	if (!Who || (PC && PC->IsInFlight()) || Who->Id == ETakeCamera::A || Who->IsA(AJulian::StaticClass()))
	{
		return Result;
	}

	// The nearest of Who's devices whose use ring holds Who's feet (2D, on the same floor).
	const FVector Feet = Who->GetFeetLocation();
	ADevice* Best = nullptr;
	double BestDist = TNumericLimits<double>::Max();
	for (ADevice* Device : Devices)
	{
		if (!IsValid(Device) || Device->FiredFrom != Who->Id)
		{
			continue;
		}
		const FVector UsePoint = Device->GetUsePoint();
		if (FMath::Abs(UsePoint.Z - Feet.Z) > Device->UseHeight)
		{
			continue;	// another floor
		}
		const double Dist = FVector::Dist2D(UsePoint, Feet);
		if (Dist <= Device->UseRadius && Dist < BestDist)
		{
			Best = Device;
			BestDist = Dist;
		}
	}
	if (!Best)
	{
		return Result;
	}

	Result.Device = Best;
	FString WhyNot;
	if (!Best->CanFire(WhyNot))
	{
		Result.bCanFire = false;
		Result.Prompt = WhyNot.IsEmpty() ? Best->DeviceName + TEXT(" can't fire now") : WhyNot;
		return Result;
	}
	Result.bCanFire = true;
	Result.Prompt = TEXT("[E] ") + Best->UseVerb;

	const AFollowSpot* Spot = Cast<AFollowSpot>(Best);
	if (!Spot)
	{
		return Result;
	}

	// The follow spot: the watcher nearest the centre of Who's view, by angle, within the aim assist and the spot's range.
	const FVector AimOrigin = Who->GetAimOrigin();
	const FVector AimDir = Who->GetAimDirection().GetSafeNormal();
	const FVector Lamp = Spot->GetActorLocation();
	const double MaxAngle = FMath::DegreesToRadians(static_cast<double>(FMath::Max(Spot->AimAssistDegrees, 0.f)));
	AWatcher* Picked = nullptr;
	double PickedAngle = TNumericLimits<double>::Max();
	if (!AimDir.IsNearlyZero())
	{
		for (AWatcher* W : Watchers)
		{
			// A watcher that has not arrived yet (step 2: "DRONE - OFF") is not there to be spotted.
			if (!IsValid(W) || !W->HasArrived() || W->DistanceToPoint(Lamp) > Spot->Range)
			{
				continue;
			}
			const FVector ToW = (W->GetAimPoint() - AimOrigin).GetSafeNormal();
			if (ToW.IsNearlyZero())
			{
				continue;
			}
			const double Angle = FMath::Acos(FMath::Clamp(FVector::DotProduct(AimDir, ToW), -1.0, 1.0));
			if (Angle <= MaxAngle && Angle < PickedAngle)
			{
				Picked = W;
				PickedAngle = Angle;
			}
		}
	}

	if (Picked)
	{
		Result.Watcher = Picked;
		Result.Prompt = TEXT("[E] ") + Best->UseVerb + TEXT(" ") + Picked->GetLabel();
	}
	else
	{
		Result.Watcher = nullptr;
		Result.Prompt = TEXT("Look at a watcher to aim the light");
		Result.bCanFire = false;
	}
	return Result;
}

void ATakeDirector::StepUse()
{
	ATakeCharacter* Who = (PC && !PC->IsInFlight()) ? PC->GetCurrentCharacter() : nullptr;
	const FTakeUseTarget NewUse = FindUseTarget(Who);

	ADevice* OldDevice = CurrentUse.Device;
	ADevice* NewDevice = NewUse.Device;
	AWatcher* OldWatcher = CurrentUse.Watcher;
	AWatcher* NewWatcher = NewUse.Watcher;

	// The highlights move on this frame.
	if (OldDevice != NewDevice)
	{
		if (IsValid(OldDevice))
		{
			OldDevice->SetInReach(false);
		}
		if (IsValid(NewDevice))
		{
			NewDevice->SetInReach(true);
		}
	}
	if (OldWatcher != NewWatcher)
	{
		if (IsValid(OldWatcher))
		{
			OldWatcher->SetHovered(false);
		}
		if (IsValid(NewWatcher))
		{
			NewWatcher->SetHovered(true);
		}
	}

	if (OldDevice != NewDevice || OldWatcher != NewWatcher)
	{
		if (!NewDevice)
		{
			TAKE_LOG(TEXT("USE TARGET none"));
		}
		else if (NewWatcher)
		{
			TAKE_LOG(TEXT("USE TARGET %s %s"), *NewDevice->DeviceName, *NewWatcher->GetLabel());
		}
		else
		{
			TAKE_LOG(TEXT("USE TARGET %s"), *NewDevice->DeviceName);
		}
	}

	CurrentUse = NewUse;
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
		// Step 3: the crew's call as it fires ("VEX: Curtain coming down!").
		if (!Device->FireLine.IsEmpty())
		{
			Flash(Device->FireLine, ETakeFlashKind::Line, 3.f);
		}
		if (Audio)
		{
			// Step 6: its sound, and the crew's line from the script if the level gives it none.
			Audio->OnDeviceFired(Device->FireSound, Device->DeviceName, !Device->FireLine.IsEmpty());
		}
		return true;
	}

	if (WhyNot.IsEmpty())
	{
		WhyNot = Device->DeviceName + TEXT(" can't fire now");
	}
	Flash(WhyNot, ETakeFlashKind::Refusal);

	FString ShortWhy = WhyNot;
	ShortWhy.RemoveFromStart(Device->DeviceName + TEXT(" "));
	TAKE_LOG(TEXT("FIRE REFUSED %s: %s"), *Device->DeviceName, *ShortWhy);
	return false;
}

bool ATakeDirector::SpotWatcher(AWatcher* Watcher)
{
	if (!FollowSpot)
	{
		Flash(TEXT("No follow spot on this set"), ETakeFlashKind::Refusal);
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
			WhyNot = FollowSpot->DeviceName + TEXT(" can't fire now");
		}
		Flash(WhyNot, ETakeFlashKind::Refusal);
		TAKE_LOG(TEXT("SPOT REFUSED %s: %s"), *Watcher->GetLabel(), *WhyNot);
		return false;
	}
	if (Audio)
	{
		// Step 6: the lamp, and Maya's line from the script if the level gives the light none.
		Audio->OnDeviceFired(FollowSpot->FireSound, FollowSpot->DeviceName, !FollowSpot->FireLine.IsEmpty());
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

		// Rule 5 and rule 7's stamps: one firing that catches two or more watchers refills composure.
		LastStampCount = Caught;
		if (Caught >= 2)
		{
			const float Refill = ExtraWatcherRefill * static_cast<float>(Caught - 1);
			Flash(FString::Printf(TEXT("x%d"), Caught), ETakeFlashKind::Count, 1.5f);
			if (Composure)
			{
				Composure->Add(Refill);
			}
			TAKE_LOG(TEXT("STAMP x%d (%s): composure +%.0f"), Caught, *Device->DeviceName, Refill);
		}

		// Step 3: a landing that sets something else off (the winch's curtain opens the stage trap).
		if (!Device->LandTrigger.IsNone())
		{
			FireTrigger(Device->LandTrigger);
		}
	}
}

void ATakeDirector::StepFrameCues()
{
	// Planning: the watchers a crew member could catch from where they stand. The frames decide what they draw
	// (AWatcher::RefreshFrameLook); the sight tests never read any of this.
	TArray<AWatcher*> Planned;
	if (FrameDisplay == ETakeFrameDisplay::Planning)
	{
		if (IsValid(CurrentUse.Watcher))
		{
			Planned.Add(CurrentUse.Watcher);	// the follow spot's aim (its own GatherTargets is the last fired one)
		}
		else if (IsValid(CurrentUse.Device) && CurrentUse.bCanFire && !Cast<AFollowSpot>(CurrentUse.Device))
		{
			CurrentUse.Device->GatherTargets(Watchers, Planned);
		}
	}
	for (AWatcher* EachWatcher : Watchers)
	{
		if (EachWatcher)
		{
			EachWatcher->SetFrameCue(FrameDisplay, Planned.Contains(EachWatcher));
		}
	}
}
