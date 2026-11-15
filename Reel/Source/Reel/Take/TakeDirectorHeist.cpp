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
//
// W3 heist2 (Marwan's second feedback, item 6): the room watches. An ARoomWatch in the level (RoomWatch.h) makes the
// seated guests see Julian at the front (the floor before the dais, and the dais) in house light: a glance, a murmur
// at 1 s, the blooper at 2.5 s. The keycard's lift mark is now like the case: half way through it in the light, the
// room sees him. So the dimmer unlocks when Julian is in the hall (Scene2), and the order is: Julian to the front row,
// [2] Maya throws the dimmer on the catwalk, the old film plays on the big screen, [1] Julian goes up in the dark, lifts
// the keycard, swaps the reel; Maya holds the house while his hands are busy; the house comes up and the alarm runs.
// The goal line is this file's from Scene2 whenever Julian is at the front, the house is dark, or he has the keycard.

#include "TakeDirector.h"

#include "HouseDimmer.h"
#include "EngineUtils.h"
#include "Julian.h"
#include "Mark.h"
#include "RoomWatch.h"
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

	/** Where the guests look on Julian: this far above his capsule's middle, cm (about his face). */
	constexpr float HeadAboveCentre = 70.f;
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
	HeistRoom = nullptr;
	HeistLiftMark = nullptr;
	bHeistLiftReaching = false;
	bHeistRoomGrace = false;

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
	const FName KeycardName(TakeTriggers::Keycard);
	for (AMark* EachMark : Marks)
	{
		if (!EachMark || !EachMark->IsActionMark())
		{
			continue;
		}
		if (!HeistCaseMark && (EachMark->ActionTrigger == CaseName || EachMark->ActionTrigger == SwapName))
		{
			HeistCaseMark = EachMark;
		}
		else if (!HeistLiftMark && EachMark->ActionTrigger == KeycardName)
		{
			HeistLiftMark = EachMark;
		}
	}
	if (UWorld* const World = GetWorld())
	{
		for (TActorIterator<ARoomWatch> It(World); It; ++It)
		{
			HeistRoom = *It;
			It->ResetRoom();
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

	// The crew hint on the dais (past the rope) names the dimmer too while the room watches.
	const ARoomWatch* const Room = Cast<ARoomWatch>(HeistRoom.Get());
	if (Room && Room->bRoomWatches && SceneHints.IsValidIndex(2) && !Dimmer->HintLine.IsEmpty())
	{
		SceneHints[2] = Dimmer->HintLine;
	}

	// The crew hint at the keycard's checkpoint names the dimmer (the case's checkpoint keeps the winch's hint).
	const UTakeGameInstance* const GI = GetTakeGameInstance();
	if (GI && GI->CheckpointTriggers.Contains(FName(TakeTriggers::Keycard)) && !GI->CheckpointTriggers.Contains(CaseName)
		&& SceneHints.IsValidIndex(3) && !Dimmer->HintLine.IsEmpty())
	{
		SceneHints[3] = Dimmer->HintLine;
	}
	TAKE_LOG(TEXT("HEIST on: %s swaps the reel, %s takes the house down; the room %s"), *CaseMark->MarkName,
		*Dimmer->DeviceName, !Room ? TEXT("is not in the level") : (Room->bRoomWatches ? TEXT("watches") : TEXT("is off")));
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
	const bool bBlind = Dimmer->IsRoomBlind();	// dark, or still dim on the way up: the room is on the screen
	ARoomWatch* const Room = Cast<ARoomWatch>(HeistRoom.Get());
	const bool bRoomOn = Room && Room->bRoomWatches && !HasFired(CaseName);
	const FVector Feet = Julian ? Julian->GetFeetLocation() : FVector::ZeroVector;

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
		// Restored onto a mark in view (the dais): the room lets him be until he has stepped out of view once.
		bHeistRoomGrace = bRoomOn && Julian && Room->IsInView(Feet);
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
		if (!HasFired(CaseName) && !bRoomOn && !Dimmer->TeachLine.IsEmpty())	// the room's murmur teaches otherwise
		{
			Flash(Dimmer->TeachLine, ETakeFlashKind::Line, 6.f);
		}
	}

	// 2. The house goes down: Vance introduces the preview (after Maya's own call, flashed as she fired).
	if (bDark && !bHeistWasDark && !Dimmer->PreviewLine.IsEmpty())
	{
		Flash(Dimmer->PreviewLine, ETakeFlashKind::Line, 4.f);
	}

	// 2b. The room watches the front (W3 heist2): a glance, a murmur, then the blooper. Not in the dark.
	if (bRoomOn && Julian)
	{
		if (bHeistRoomGrace && !Room->IsInView(Feet))
		{
			bHeistRoomGrace = false;
		}
		if (!bHeistRoomGrace)
		{
			const FVector Head = Julian->GetActorLocation() + FVector(0.f, 0.f, TakeHeistLocal::HeadAboveCentre);
			if (Room->StepRoom(DeltaSeconds, Feet, Head, bBlind, this) == ERoomReaction::Seen)
			{
				TAKE_LOG(TEXT("HEIST the room saw him at the front (house light %.0f%%)"), Dimmer->GetHouseLevel() * 100.f);
				RequestBlooper(EBlooperReason::SeenAtCase);
				return;
			}
		}
	}

	// 2c. The keycard is lifted on the dais in front of the room: like the case, only in the dark (W3 heist2).
	AMark* const LiftMark = HeistLiftMark.Get();
	if (bRoomOn && LiftMark && !bKeycard)
	{
		const float Alpha = LiftMark->GetActionAlpha();
		const bool bReaching = Alpha > 0.f;
		if (bReaching && !bHeistLiftReaching && Julian)
		{
			Julian->PlayGesture(FName(TEXT("Lift")));	// the cast contract (cast2)
		}
		bHeistLiftReaching = bReaching;
		if (!bBlind && Alpha >= Dimmer->SeenAtCaseAlpha)
		{
			if (!Room->SeenLine.IsEmpty())
			{
				Flash(Room->SeenLine, ETakeFlashKind::Line, 3.f);
			}
			TAKE_LOG(TEXT("HEIST the room saw him lift the keycard in the light"));
			RequestBlooper(EBlooperReason::SeenAtCase);
			return;
		}
		if (bDark && bReaching)
		{
			const float StillNeeds = (1.f - Alpha) * LiftMark->ActionSeconds + TakeHeistLocal::HoldMargin;
			if (Dimmer->GetDarkSecondsLeft() < StillNeeds)
			{
				Dimmer->HoldPreviewFor(StillNeeds);
			}
		}
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

		if (!bBlind && Alpha >= Dimmer->SeenAtCaseAlpha)
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
		if (!bBlind)
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

	// 6. The goal line, over UpdateObjective's, until the case is open: from the keycard; with the room watching, from
	// Scene 2 whenever Julian is at the front or the house is dark (the aisle keeps the scene's own lines).
	const bool bOurs = bKeycard || (bRoomOn && HasFired(FName(TakeTriggers::Scene2))
		&& (Dimmer->IsRunning() || Room->IsAtFront(Feet)));
	if (!bOurs || HasFired(CaseName) || Phase != ETakePhase::Rolling)
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
		const bool bLiftFirst = bRoomOn && HeistLiftMark && !bKeycard;
		LogKey = CurrentCamera == ETakeCamera::A
			? FString(bLiftFirst ? TEXT("Lights down: up to the dais, lift VANCE's keycard, swap the reel at the case")
				: TEXT("Lights down: swap the reel at the case"))
			: FString(bLiftFirst ? TEXT("[1] JULIAN: up to the dais while it is dark")
				: TEXT("[1] JULIAN: swap the reel at the case while it is dark"));
		Line = FString::Printf(TEXT("%s.   %d s"), *LogKey, Left);
	}
	else if (CurrentCamera == ETakeCamera::A)
	{
		if (bRoomOn && Room->IsInView(Feet) && !bHeistRoomGrace)
		{
			Line = TEXT("The whole room can see you up here: back to the crowd");
		}
		else if (!bJulianExposed)
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
				? FString(bKeycard ? TEXT("[2] MAYA: dim the HOUSE LIGHTS. In the dark, JULIAN swaps the reel at the case")
					: TEXT("[2] MAYA: dim the HOUSE LIGHTS. In the dark, JULIAN goes up for the keycard and the reel"))
				: FString(TEXT("Post [3] VEX at the CURTAIN WINCH. Then [2] MAYA dims the HOUSE LIGHTS"));
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
			Line = TEXT("Walk MAYA to the HOUSE DIMMER on the catwalk");
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
