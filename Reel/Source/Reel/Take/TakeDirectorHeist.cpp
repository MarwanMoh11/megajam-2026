// ATakeDirector slice: the swap in the dark (QA pass, Nov 14: Marwan's note 6). Owner: heist. The trick is described
// at the top of HouseDimmer.h; the members are the "QA heist" block in TakeDirector.h.
//
// The flow with bHeistTrick on (and a house dimmer and the case mark in the level):
//   Keycard: Vex's teach line; the goal line asks for the house lights.
//   Maya fires the dimmer: the house is dark for its PreviewSeconds and every hall watcher looks at the screen.
//   Julian on the case mark: after its 1.2 s it fires "Swap" (no longer "Case"), and it saves no checkpoint itself.
//     - house dark: the dummy is in the case; the house comes up LightsUpAfterSwapSeconds later. If the preview
//       would end with him half way through, Maya holds the house until he is done.
//     - house up: once the swap is SeenAtCaseAlpha along (or if it ever finishes) the room sees him: the blooper
//       SeenAtCase, back to the keycard's checkpoint.
//   The house comes up with the swap done: DiscoverLine, FireTrigger(Case) (the alarm and the getaway exactly as
//     before) and SaveCheckpoint(the case mark, "THE CASE"), the checkpoint the case mark used to save itself.
//   The house comes up without a swap: NoSwapLine; the dimmer is free and can run the preview again.
// Off (bHeistTrick false, or no dimmer or case mark): nothing here runs and the case mark is left as the map made it.

#include "TakeDirector.h"

#include "HouseDimmer.h"
#include "Julian.h"
#include "Mark.h"
#include "TakeCharacter.h"
#include "TakeGameInstance.h"
#include "WinchDevice.h"

namespace TakeHeistLocal
{
	/** The case mark's trigger while the trick is on: the reel swapped for the dummy (no alarm yet). */
	inline const TCHAR* const SwapTrigger = TEXT("Swap");

	/** The checkpoint's name if the case mark had none. */
	inline const TCHAR* const CaseCheckpoint = TEXT("THE CASE");

	/** Maya holds the house this much longer than the swap still needs, s. */
	constexpr float HoldMargin = 0.3f;

	/** Vex counts as posted at the winch within its use ring plus this, cm. */
	constexpr float PostedSlack = 50.f;
}

bool ATakeDirector::IsHeistOn() const
{
	return bHeistTrick && HeistDimmer != nullptr && HeistCaseMark != nullptr;
}

void ATakeDirector::BeginHeist()
{
	HeistDimmer = nullptr;
	HeistCaseMark = nullptr;
	HeistCaseCheckpointName.Reset();
	HeistLoggedObjective.Reset();
	bHeistPrimed = false;
	bHeistTaught = false;
	bHeistSwapHandled = false;
	bHeistWasDark = false;
	bHeistReaching = false;

	for (ADevice* EachDevice : Devices)
	{
		if (Cast<AHouseDimmer>(EachDevice))
		{
			HeistDimmer = EachDevice;
			break;
		}
	}
	const FName CaseName(TakeTriggers::Case);
	const FName SwapName(TakeHeistLocal::SwapTrigger);
	for (AMark* EachMark : Marks)
	{
		if (EachMark && EachMark->IsActionMark()
			&& (EachMark->ActionTrigger == CaseName || EachMark->ActionTrigger == SwapName))
		{
			HeistCaseMark = EachMark;
			break;
		}
	}

	if (!IsHeistOn())
	{
		TAKE_LOG(TEXT("HEIST off (%s): the case trips the alarm at once"),
			!bHeistTrick ? TEXT("the switch") : (!HeistDimmer ? TEXT("no house dimmer") : TEXT("no case mark")));
		return;
	}

	// The case mark swaps the reel instead of tripping the alarm, and the case's checkpoint waits for the alarm.
	const AHouseDimmer* const Dimmer = CastChecked<AHouseDimmer>(HeistDimmer.Get());
	AMark* const CaseMark = HeistCaseMark.Get();
	HeistCaseCheckpointName = CaseMark->ActionCheckpointName.IsEmpty()
		? FString(TakeHeistLocal::CaseCheckpoint) : CaseMark->ActionCheckpointName;
	CaseMark->ActionTrigger = SwapName;
	CaseMark->ActionCheckpointName.Reset();
	if (!Dimmer->SwapVerb.IsEmpty())
	{
		CaseMark->ActionVerb = Dimmer->SwapVerb;
	}

	// The crew hint at the keycard's checkpoint names the dimmer (the case's checkpoint keeps the winch's hint).
	const UTakeGameInstance* const GI = GetTakeGameInstance();
	if (GI && GI->CheckpointTriggers.Contains(FName(TakeTriggers::Keycard)) && !GI->CheckpointTriggers.Contains(CaseName)
		&& SceneHints.IsValidIndex(3) && !Dimmer->HintLine.IsEmpty())
	{
		SceneHints[3] = Dimmer->HintLine;
	}
	TAKE_LOG(TEXT("HEIST on: %s swaps the reel, %s takes the house down"), *CaseMark->MarkName, *Dimmer->DeviceName);
}

void ATakeDirector::OnHeistBlooper()
{
	if (AHouseDimmer* const Dimmer = Cast<AHouseDimmer>(HeistDimmer.Get()))
	{
		Dimmer->RestoreHouseNow();
	}
}

void ATakeDirector::StepHeist(float DeltaSeconds)
{
	if (Phase != ETakePhase::Rolling || !IsHeistOn())
	{
		return;
	}
	AHouseDimmer* const Dimmer = Cast<AHouseDimmer>(HeistDimmer.Get());
	AMark* const CaseMark = HeistCaseMark.Get();
	if (!Dimmer || !CaseMark)
	{
		return;
	}

	const FName CaseName(TakeTriggers::Case);
	const FName SwapName(TakeHeistLocal::SwapTrigger);
	const bool bKeycard = HasFired(FName(TakeTriggers::Keycard));
	const bool bDark = Dimmer->IsHouseDark();

	auto SaveCaseCheckpoint = [this, CaseMark]()
	{
		if (Phase == ETakePhase::Rolling)
		{
			AMark* const At = CaseMark->ActionCheckpointMark ? CaseMark->ActionCheckpointMark.Get() : CaseMark;
			SaveCheckpoint(At, HeistCaseCheckpointName);
		}
	};
	auto RoomSawHim = [this, Dimmer](const TCHAR* Why)
	{
		if (!Dimmer->SeenLine.IsEmpty())
		{
			Flash(Dimmer->SeenLine, ETakeFlashKind::Line, 3.f);
		}
		TAKE_LOG(TEXT("HEIST the room saw him at the case (%s)"), Why);
		RequestBlooper(EBlooperReason::SeenAtCase);
	};

	// The first rolling frame (after any restore): what was already done is not said again.
	if (!bHeistPrimed)
	{
		bHeistPrimed = true;
		bHeistTaught = bKeycard;
		bHeistSwapHandled = HasFired(SwapName);
		bHeistWasDark = bDark;
		if (HasFired(SwapName) && !HasFired(CaseName))
		{
			// Never saved like this; if it ever is, the house is up and Vance finds the dummy now.
			FireTrigger(CaseName);
			SaveCaseCheckpoint();
			return;
		}
	}

	// 1. The spoken half of the lesson, as the keycard lands.
	if (bKeycard && !bHeistTaught)
	{
		bHeistTaught = true;
		if (!HasFired(CaseName) && !Dimmer->TeachLine.IsEmpty())
		{
			Flash(Dimmer->TeachLine, ETakeFlashKind::Line, 6.f);
		}
	}

	// 2. The house goes down: Vance introduces the preview (after Maya's own call, flashed as she fired).
	if (bDark && !bHeistWasDark && !Dimmer->PreviewLine.IsEmpty())
	{
		Flash(Dimmer->PreviewLine, ETakeFlashKind::Line, 4.f);
	}

	// 3. Julian at the case, before the swap: his hands go to it; in the light the room sees him.
	if (bKeycard && !HasFired(SwapName))
	{
		const float Alpha = CaseMark->GetActionAlpha();
		const bool bReaching = Alpha > 0.f;
		if (bReaching && !bHeistReaching && Julian)
		{
			Julian->PlayGesture(FName(TEXT("Lift")));	// the cast contract (package cast)
		}
		bHeistReaching = bReaching;

		if (!bDark && Alpha >= Dimmer->SeenAtCaseAlpha)
		{
			RoomSawHim(TEXT("the house lights were up"));
			return;
		}
		if (bDark && bReaching)
		{
			// Maya sees him at the case on her monitor and holds the house until the swap is done.
			const float StillNeeds = (1.f - Alpha) * CaseMark->ActionSeconds + TakeHeistLocal::HoldMargin;
			if (Dimmer->GetDarkSecondsLeft() < StillNeeds)
			{
				Dimmer->HoldPreviewFor(StillNeeds);
			}
		}
	}

	// 4. The swap (the case mark fired Swap this frame, in step 6b).
	if (HasFired(SwapName) && !bHeistSwapHandled)
	{
		bHeistSwapHandled = true;
		if (!bDark)
		{
			RoomSawHim(TEXT("the swap finished in the light"));
			return;
		}
		if (!Dimmer->SwapLine.IsEmpty())
		{
			Flash(Dimmer->SwapLine, ETakeFlashKind::Line, 3.f);
		}
		Dimmer->EndPreviewIn(Dimmer->LightsUpAfterSwapSeconds);
		TAKE_LOG(TEXT("HEIST swap done in the dark: the dummy is in the case"));
	}

	// 5. The house comes up. With the dummy in the case, Vance presents Lot 41, opens the can, and the old Case
	// trigger starts the alarm and the getaway; the case's checkpoint is saved as the case mark used to save it.
	const bool bDarkNow = Dimmer->IsHouseDark();	// the swap may have ended the dark on this frame
	if (bHeistWasDark && !bDarkNow)
	{
		if (HasFired(SwapName) && !HasFired(CaseName))
		{
			if (!Dimmer->DiscoverLine.IsEmpty())
			{
				Flash(Dimmer->DiscoverLine, ETakeFlashKind::Line, 3.f);
			}
			TAKE_LOG(TEXT("HEIST the house is up: Vance opens the dummy"));
			FireTrigger(CaseName);
			SaveCaseCheckpoint();
		}
		else if (!HasFired(SwapName))
		{
			if (!Dimmer->NoSwapLine.IsEmpty())
			{
				Flash(Dimmer->NoSwapLine, ETakeFlashKind::Line, 4.f);
			}
			TAKE_LOG(TEXT("HEIST the preview ended with no swap"));
		}
	}
	bHeistWasDark = bDarkNow;

	// 6. The goal line, over UpdateObjective's, from the keycard until the case is open.
	if (!bKeycard || HasFired(CaseName) || Phase != ETakePhase::Rolling)
	{
		HeistLoggedObjective.Reset();
		return;
	}
	FString Line;
	FString LogKey;
	if (HasFired(SwapName))
	{
		Line = TEXT("Clean swap. The house is coming up: get ready to run");
	}
	else if (Dimmer->IsHouseDark())
	{
		const int32 Left = FMath::CeilToInt(Dimmer->GetDarkSecondsLeft());
		LogKey = CurrentCamera == ETakeCamera::A ? FString(TEXT("Lights down: swap the reel at the case"))
			: FString(TEXT("[1] JULIAN: swap the reel at the case while it is dark"));
		Line = FString::Printf(TEXT("%s.   %d s"), *LogKey, Left);
	}
	else if (CurrentCamera == ETakeCamera::A)
	{
		if (!bJulianExposed)
		{
			// Vex at the winch first, as before: the alarm follows the swap by a second.
			bool bVexPosted = true;
			const ATakeCharacter* const Vex = GetCharacter(ETakeCamera::C);
			for (ADevice* EachDevice : Devices)
			{
				if (const AWinchDevice* const Winch = Cast<AWinchDevice>(EachDevice))
				{
					if (Vex && Winch->UsesLeft > 0)
					{
						bVexPosted = FVector::Dist2D(Vex->GetFeetLocation(), Winch->GetUsePoint())
							<= Winch->UseRadius + TakeHeistLocal::PostedSlack;
					}
					break;
				}
			}
			Line = bVexPosted
				? FString(TEXT("[2] MAYA: dim the HOUSE LIGHTS. In the dark, JULIAN swaps the reel at the case"))
				: FString(TEXT("Post [3] VEX at the CURTAIN WINCH. Then [2] MAYA dims the HOUSE LIGHTS for the swap"));
		}
	}
	else if (CurrentCamera == ETakeCamera::B)
	{
		if (CurrentUse.Device.Get() == static_cast<ADevice*>(Dimmer))
		{
			Line = CurrentUse.bCanFire
				? FString(TEXT("[E] Dim the HOUSE LIGHTS: every eye in the hall goes to the screen"))
				: CurrentUse.Prompt;
		}
		else
		{
			Line = TEXT("Walk MAYA to the HOUSE DIMMER, at the west rail of her platform");
		}
	}

	if (Line.IsEmpty())
	{
		HeistLoggedObjective.Reset();	// UpdateObjective's own line stands
		return;
	}
	Objective = Line;
	if (LogKey.IsEmpty())
	{
		LogKey = Line;
	}
	if (LogKey != HeistLoggedObjective)
	{
		HeistLoggedObjective = LogKey;
		TAKE_LOG(TEXT("OBJECTIVE (heist) %s"), *Line);
	}
}
