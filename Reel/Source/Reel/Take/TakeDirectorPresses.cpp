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

namespace TakeSliceBPresses
{
	const TCHAR* ClickKindName(ETakeClickKind Kind)
	{
		switch (Kind)
		{
		case ETakeClickKind::Mark: return TEXT("MARK");
		case ETakeClickKind::Device: return TEXT("DEVICE");
		case ETakeClickKind::Watcher: return TEXT("WATCHER");
		default: return TEXT("NOTHING");
		}
	}

	/** The log / HUD name of a clicked actor: MarkName, DeviceName, Label, else the object name. */
	FString ClickedName(const AActor* ClickedActor)
	{
		if (!ClickedActor)
		{
			return TEXT("-");
		}
		if (const AMark* AsMark = Cast<AMark>(ClickedActor))
		{
			return AsMark->MarkName;
		}
		if (const ADevice* AsDevice = Cast<ADevice>(ClickedActor))
		{
			return AsDevice->DeviceName;
		}
		if (const AWatcher* AsWatcher = Cast<AWatcher>(ClickedActor))
		{
			return AsWatcher->Label;
		}
		return ClickedActor->GetName();
	}
}

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

	// Cuts first (in queue order), then clicks, then a new take.
	for (const FTakePress& QueuedPress : Presses)
	{
		if (QueuedPress.Type == ETakePressType::Cut)
		{
			HandleCut(QueuedPress.Camera, QueuedPress);
		}
	}
	for (const FTakePress& QueuedPress : Presses)
	{
		if (QueuedPress.Type == ETakePressType::Click)
		{
			HandleClick(QueuedPress);
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
	ATakeCamera* CutCamera = FindCamera(Camera);
	if (!CutCamera)
	{
		Flash(FString::Printf(TEXT("No camera %s on this set"), TakeText::CameraLetter(Camera)));
		return;
	}

	CurrentCamera = Camera;
	if (PC)
	{
		PC->CutTo(CutCamera);
	}
	TAKE_LOG(TEXT("CUT -> %s (%s)"), TakeText::CameraLetter(Camera), *Press.Source);
}

void ATakeDirector::HandleClick(const FTakePress& Press)
{
	AActor* ClickedActor = Press.Target.Get();
	const ITakeClickable* Clickable = Cast<ITakeClickable>(ClickedActor);
	const ETakeClickKind Kind = Clickable ? Clickable->GetClickKind() : ETakeClickKind::None;

	TAKE_LOG(TEXT("CLICK on %s: %s %s (%s)"), TakeText::CameraLetter(CurrentCamera),
		TakeSliceBPresses::ClickKindName(Kind), *TakeSliceBPresses::ClickedName(ClickedActor), *Press.Source);

	// The camera that can spot a watcher: the follow spot's (B if the set has no follow spot).
	const ETakeCamera SpotCamera = FollowSpot ? FollowSpot->FiredFrom : ETakeCamera::B;

	if (CurrentCamera == ETakeCamera::A)
	{
		// Rule 3: a click on A always means "go there".
		switch (Kind)
		{
		case ETakeClickKind::Mark:
		{
			AMark* ClickedMark = Cast<AMark>(ClickedActor);
			if (Julian && ClickedMark && !Julian->IsWalking() && Julian->GetStandingMark() == ClickedMark)
			{
				// Already standing on it: no cue, but the press still shows something this frame.
				TAKE_LOG(TEXT("ALREADY ON %s"), *ClickedMark->MarkName);
				Flash(TEXT("He's on that mark"));
			}
			else if (Julian && ClickedMark && Julian->CueTo(ClickedMark))
			{
				TAKE_LOG(TEXT("CUE -> %s"), *ClickedMark->MarkName);
			}
			else
			{
				Flash(TEXT("Julian can't get there"));
			}
			break;
		}
		case ETakeClickKind::Device:
		{
			const ADevice* ClickedDevice = Cast<ADevice>(ClickedActor);
			const ETakeCamera FireCamera = ClickedDevice ? ClickedDevice->FiredFrom : ETakeCamera::B;
			Flash(FString::Printf(TEXT("[%s] to fire it"), TakeText::CameraKey(FireCamera)));
			break;
		}
		case ETakeClickKind::Watcher:
			Flash(FString::Printf(TEXT("[%s] to spot him"), TakeText::CameraKey(SpotCamera)));
			break;
		default:
			Flash(TEXT("Click a gold mark"));
			break;
		}
		return;
	}

	// Rule 4: on B or C a click fires the device or spots the watcher; it never cues Julian.
	switch (Kind)
	{
	case ETakeClickKind::Device:
	{
		ADevice* ClickedDevice = Cast<ADevice>(ClickedActor);
		if (ClickedDevice && ClickedDevice->FiredFrom == CurrentCamera)
		{
			FireDevice(ClickedDevice);
		}
		else
		{
			const ETakeCamera FireCamera = ClickedDevice ? ClickedDevice->FiredFrom : ETakeCamera::C;
			Flash(FString::Printf(TEXT("[%s] to fire it"), TakeText::CameraKey(FireCamera)));
		}
		break;
	}
	case ETakeClickKind::Watcher:
	{
		AWatcher* ClickedWatcher = Cast<AWatcher>(ClickedActor);
		if (ClickedWatcher && FollowSpot && FollowSpot->FiredFrom == CurrentCamera)
		{
			SpotWatcher(ClickedWatcher);
		}
		else
		{
			Flash(FString::Printf(TEXT("[%s] to spot him"), TakeText::CameraKey(SpotCamera)));
		}
		break;
	}
	default:
		// A mark, or nothing clickable.
		Flash(FString::Printf(TEXT("[%s] to cue Julian"), TakeText::CameraKey(ETakeCamera::A)));
		break;
	}
}

void ATakeDirector::StepShotList()
{
	if (!Julian)
	{
		return;
	}

	const FVector Feet = Julian->GetFeetLocation();
	AShotZone* BestZone = nullptr;
	for (AShotZone* Zone : ShotZones)
	{
		if (Zone && Zone->Contains(Feet) && (!BestZone || Zone->Priority > BestZone->Priority))
		{
			BestZone = Zone;
		}
	}
	CurrentShotZone = BestZone;
	if (!BestZone)
	{
		// Outside every zone: camera A keeps its current shot.
		return;
	}

	ATakeCamera* NewShot = BestZone->ResolveShot(Cameras);
	if (!NewShot || NewShot == CurrentShotA)
	{
		return;
	}

	CurrentShotA = NewShot;
	TAKE_LOG(TEXT("SHOT -> %s"), NewShot->ShotName.IsNone() ? *NewShot->GetName() : *NewShot->ShotName.ToString());
	if (CurrentCamera == ETakeCamera::A && PC)
	{
		PC->CutTo(NewShot);
	}
}
