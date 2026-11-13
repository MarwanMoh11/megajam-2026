// ATakeDirector slice: the scene (step 6), triggers, checkpoints, the lift, the objective. Owner: S (step 2).
// See TakeDirector.h for the contract.

#include "TakeDirector.h"

#include "BarClockComponent.h"
#include "CrewPost.h"
#include "Device.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Julian.h"
#include "Mark.h"
#include "SceneTrigger.h"
#include "ScannerWatcher.h"
#include "TakeCharacter.h"
#include "TakeGameInstance.h"
#include "TakePlayerController.h"
#include "Watcher.h"

namespace TakeSceneLocal
{
	/** Scene 2: the objective names the rope once Julian's feet are this close to the beam (2D, cm). */
	constexpr float RopePromptDistance = 700.f;

	/** The scanner's countdown line: its fixed start (the log treats the ticking seconds as one line). */
	const TCHAR* const ScannerBlindPrefix = TEXT("The scanner is blind: walk JULIAN through");

	/** Teleports Character so its feet stand on FeetPoint facing Yaw, looking level-ish, the follow camera snapped
	 *  there and its velocity zeroed. This frame. */
	void PlaceCharacter(ATakeCharacter* Character, const FVector& FeetPoint, float Yaw)
	{
		if (!Character)
		{
			return;
		}
		const FVector FeetToCentre = Character->GetActorLocation() - Character->GetFeetLocation();
		Character->SetActorLocationAndRotation(FeetPoint + FeetToCentre, FRotator(0.f, Yaw, 0.f), false, nullptr,
			ETeleportType::TeleportPhysics);
		if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
		{
			Move->StopMovementImmediately();
		}
		Character->SetLook(Yaw, Character->StartLookPitch);
		Character->SnapFollowCamera();
	}
}

void ATakeDirector::StepScene(float /*DeltaSeconds*/)
{
	if (!Julian)
	{
		return;
	}

	// Marks' occupied state follows where Julian stands (AJulian sets it on arrival and on a cue; this keeps it true).
	AMark* const Standing = Julian->GetStandingMark();
	for (const TObjectPtr<AMark>& MarkPtr : Marks)
	{
		AMark* const EachMark = MarkPtr.Get();
		if (!EachMark)
		{
			continue;
		}
		const bool bShouldBeOccupied = (EachMark == Standing);
		if (EachMark->IsOccupied() != bShouldBeOccupied)
		{
			EachMark->SetOccupied(bShouldBeOccupied);
		}
	}

	// Checkpoints come from ASceneTrigger boxes (StepTriggers), no longer from marks.

	// The goal mark reached: the take is cleared (the HUD's CLEAR stamp shows from this frame).
	if (Phase == ETakePhase::Rolling && Standing && Standing->bGoal)
	{
		SetPhase(ETakePhase::Cleared);
		Flash(TEXT("CLEAR - that's the one"), TakeColors::GoldBright, 3.f);
		TAKE_LOG(TEXT("CLEARED on %s"), *Standing->MarkName);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Triggers and arrivals
// ---------------------------------------------------------------------------------------------------------------------

void ATakeDirector::FireTrigger(FName Name)
{
	if (Name.IsNone() || FiredTriggers.Contains(Name))
	{
		return;
	}

	const int32 Bar = Clock ? Clock->GetBar() : 1;
	FiredTriggers.Add(Name, Bar);
	TAKE_LOG(TEXT("TRIGGER %s bar %d"), *Name.ToString(), Bar);

	// Watchers that arrive on it: on the next arrival line (plus their own extra bars).
	for (AWatcher* EachWatcher : Watchers)
	{
		if (EachWatcher && EachWatcher->ArriveOn == Name)
		{
			const int32 ArriveBar = GetArrivalLine(Bar) + FMath::Max(0, EachWatcher->ArriveExtraBars);
			EachWatcher->ScheduleArrival(ArriveBar);
			Flash(FString::Printf(TEXT("%s - arrives on bar %d"), *EachWatcher->GetLabel(), ArriveBar), TakeColors::Red, 3.f);
		}
	}

	// The scene script's own reactions.
	if (Name == FName(TakeTriggers::Keycard))
	{
		Flash(TEXT("KEYCARD LIFTED"), TakeColors::Gold, 3.f);
		if (Clock)
		{
			Clock->PlayChime();
		}
		if (Phase != ETakePhase::Cleared)
		{
			SetPhase(ETakePhase::Cleared);
		}
		ShowTitle(TEXT("SCENE 2 CLEARED"));
		TAKE_LOG(TEXT("CLEARED keycard"));
	}
}

bool ATakeDirector::HasFired(FName Name) const
{
	return FiredTriggers.Contains(Name);
}

int32 ATakeDirector::GetArrivalLine(int32 Bar) const
{
	// Lines start bars 1, 1 + N, 1 + 2N...: the first one after Bar.
	const int32 N = FMath::Max(1, ArrivalLineBars);
	const int32 SafeBar = FMath::Max(1, Bar);
	return ((SafeBar - 1) / N + 1) * N + 1;
}

void ATakeDirector::StepWarnings()
{
	if (!Clock || !Clock->BarCrossedThisFrame())
	{
		return;
	}

	const int32 B = Clock->GetBar();
	uint32 PlayedInstruments = 0;	// one bit per EWatcherInstrument: at most one warning per instrument per bar line
	bool bChimed = false;

	const auto PlayOnce = [this, &PlayedInstruments](EWatcherInstrument Instrument)
	{
		if (Instrument == EWatcherInstrument::None)
		{
			return;
		}
		const uint32 Bit = 1u << static_cast<uint32>(Instrument);
		if ((PlayedInstruments & Bit) == 0)
		{
			PlayedInstruments |= Bit;
			Clock->PlayWarning(Instrument);
		}
	};

	for (AWatcher* EachWatcher : Watchers)
	{
		if (!EachWatcher)
		{
			continue;
		}
		const int32 ArriveBar = EachWatcher->GetArriveBar();

		if (ArriveBar > 0 && ArriveBar == B + 1)
		{
			// It arrives on the next bar line: its instrument plays solo for one bar.
			PlayOnce(EachWatcher->Instrument);
			Clock->MuteMetronome(1.f);
			Flash(FString::Printf(TEXT("%s arrives on the next bar"), *EachWatcher->GetLabel()), TakeColors::Red, 2.f);
			if (EachWatcher->Instrument != EWatcherInstrument::None)
			{
				TAKE_LOG(TEXT("WARNING %s %s: arrives on bar %d"), *EachWatcher->GetLabel(),
					TakeText::InstrumentName(EachWatcher->Instrument), ArriveBar);
			}
		}
		else if (ArriveBar > 0 && ArriveBar == B)
		{
			// The downbeat it arrives on.
			if (!bChimed)
			{
				bChimed = true;
				Clock->PlayChime();
			}
			TAKE_LOG(TEXT("ARRIVES %s on bar %d"), *EachWatcher->GetLabel(), B);
		}

		if (EachWatcher->HasArrived() && EachWatcher->Instrument != EWatcherInstrument::None
			&& EachWatcher->ChangesAtBar(B + 1, Clock))
		{
			// Only within earshot of Julian: the guard in the hall is not heard from the street.
			const bool bInEarshot = WarningEarshot <= 0.f || !Julian
				|| EachWatcher->DistanceToPoint(Julian->GetFeetLocation()) <= WarningEarshot;
			if (bInEarshot)
			{
				PlayOnce(EachWatcher->Instrument);
				TAKE_LOG(TEXT("WARNING %s %s: changes on bar %d"), *EachWatcher->GetLabel(),
					TakeText::InstrumentName(EachWatcher->Instrument), B + 1);
			}
		}
	}
}

void ATakeDirector::StepTriggers()
{
	if (Phase != ETakePhase::Rolling || !Julian)
	{
		return;
	}

	const FVector Feet = Julian->GetFeetLocation();
	for (ASceneTrigger* Trigger : SceneTriggers)
	{
		if (!Trigger || Trigger->TriggerName.IsNone() || HasFired(Trigger->TriggerName) || !Trigger->Contains(Feet))
		{
			continue;
		}

		FireTrigger(Trigger->TriggerName);

		if (Trigger->Scene > 0 && Trigger->Scene != Scene)
		{
			TAKE_LOG(TEXT("SCENE %d -> %d (%s)"), Scene, Trigger->Scene, *Trigger->TriggerName.ToString());
			Scene = Trigger->Scene;
			PlaceCrewForScene(Scene);
			if (!Trigger->Title.IsEmpty())
			{
				ShowTitle(Trigger->Title);
			}
		}

		if (AMark* const CheckpointAt = Trigger->CheckpointMark.Get())
		{
			const FString Name = Trigger->CheckpointName.IsEmpty() ? CheckpointAt->MarkName : Trigger->CheckpointName;
			SaveCheckpoint(CheckpointAt, Name);
		}
	}
}

void ATakeDirector::StepMarkActions(float DeltaSeconds)
{
	if (Phase != ETakePhase::Rolling)
	{
		return;
	}

	const AMark* const Standing = Julian ? Julian->GetStandingMark() : nullptr;
	for (AMark* EachMark : Marks)
	{
		if (!EachMark || !EachMark->IsActionMark() || EachMark->IsActionDone())
		{
			continue;
		}
		const bool bRunning = (Standing == EachMark) && !bJulianSeen;
		if (EachMark->StepAction(DeltaSeconds, bRunning))
		{
			TAKE_LOG(TEXT("ACTION done on %s -> %s"), *EachMark->MarkName, *EachMark->ActionTrigger.ToString());
			FireTrigger(EachMark->ActionTrigger);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// The objective line and the title
// ---------------------------------------------------------------------------------------------------------------------

void ATakeDirector::UpdateObjective()
{
	// A flight keeps the line it started with.
	if (PC && PC->IsInFlight())
	{
		return;
	}

	FString NewObjective;
	bool bCountdownLine = false;

	if (Phase == ETakePhase::Cleared)
	{
		NewObjective = TEXT("CLEARED. Scene 3, the getaway, comes next.   [R] new take");
	}
	else if (Scene <= 1)
	{
		AScannerWatcher* Scanner = nullptr;
		for (AWatcher* EachWatcher : Watchers)
		{
			Scanner = Cast<AScannerWatcher>(EachWatcher);
			if (Scanner)
			{
				break;
			}
		}

		if (Scanner && Julian && !HasFired(FName(TakeTriggers::Door)) && !Scanner->HasPassed(Julian.Get()))
		{
			const bool bLive = Scanner->IsLive();
			if (CurrentCamera == ETakeCamera::A)
			{
				if (!bLive)
				{
					NewObjective = FString::Printf(TEXT("%s   (%.1f s)"), TakeSceneLocal::ScannerBlindPrefix,
						Scanner->GetEffectSecondsLeft());
					bCountdownLine = true;
				}
				else if (FVector::Dist2D(Julian->GetFeetLocation(), Scanner->GetActorLocation()) > DoorPromptDistance)
				{
					NewObjective = TEXT("No invitation: keep off the red carpet. Walk JULIAN up the queue lane to the gold mark   (WASD + mouse)");
				}
				else
				{
					NewObjective = TEXT("[2]  Cut to MAYA on the canopy. She has the searchlight.");
				}
			}
			else if (CurrentCamera == ETakeCamera::B)
			{
				if (!bLive)
				{
					NewObjective = TEXT("[1]  Cut back to JULIAN and walk him through");
				}
				else if (!CurrentUse.Device)
				{
					NewObjective = TEXT("Stand in the searchlight's gold ring");
				}
				else if (CurrentUse.Watcher.Get() == Scanner)
				{
					NewObjective = TEXT("[E]  Swing the searchlight onto the SCANNER");
				}
				else
				{
					NewObjective = TEXT("Look down at the SCANNER's red eye, then [E]");
				}
			}
			else
			{
				NewObjective = TEXT("[2] MAYA has the searchlight for the door.   [1] JULIAN");
			}
		}
		else
		{
			// Inside. The crew walk to their posts for the hall (no teleports): the line says who still has to. A crew
			// member is "at a post" while a device other than the searchlight (the one nearest the scanner) is in reach.
			const ADevice* Searchlight = nullptr;
			if (Scanner)
			{
				float BestDistance = TNumericLimits<float>::Max();
				for (const ADevice* EachDevice : Devices)
				{
					if (EachDevice && EachDevice->FiredFrom == ETakeCamera::B)
					{
						const float Distance = static_cast<float>(FVector::Dist2D(EachDevice->GetActorLocation(), Scanner->GetActorLocation()));
						if (Distance < BestDistance)
						{
							BestDistance = Distance;
							Searchlight = EachDevice;
						}
					}
				}
			}
			const bool bAtPost = CurrentUse.Device != nullptr && CurrentUse.Device.Get() != Searchlight;
			if (CurrentCamera == ETakeCamera::B)
			{
				NewObjective = bAtPost
					? TEXT("MAYA is at her post over the hall.   [3] VEX   [1] JULIAN")
					: TEXT("Walk MAYA in through the door behind her, round the balcony and out along the catwalk to the follow spot");
			}
			else if (CurrentCamera == ETakeCamera::C)
			{
				NewObjective = bAtPost
					? TEXT("VEX is at his cart.   [2] MAYA   [1] JULIAN")
					: TEXT("Walk VEX out of the loading dock and up the corridor to the champagne cart at the hatch");
			}
			else
			{
				NewObjective = TEXT("He is in. Post the crew first: [2] MAYA to the follow spot, [3] VEX to his cart. Then walk JULIAN round the stair into the hall.");
			}
		}
	}
	else if (HasFired(FName(TakeTriggers::PastRope)))
	{
		const AMark* const Standing = Julian ? Julian->GetStandingMark() : nullptr;
		if (Standing && Standing->IsActionMark() && !Standing->IsActionDone())
		{
			NewObjective = bJulianSeen
				? TEXT("SEEN. Blind or lure whoever is watching the mark")
				: TEXT("Hold still: lifting the keycard");
		}
		else
		{
			NewObjective = TEXT("Lift VANCE's keycard: stand on the mark behind him while no frame is on it. He turns away to speak.");
		}
	}
	else
	{
		const AWatcher* Beam = nullptr;
		const AWatcher* Drone = nullptr;
		for (const AWatcher* EachWatcher : Watchers)
		{
			if (!EachWatcher)
			{
				continue;
			}
			if (!Beam && EachWatcher->IsInstantBlooper())
			{
				Beam = EachWatcher;
			}
			if (!Drone && EachWatcher->ExposesOffMark())
			{
				Drone = EachWatcher;
			}
		}

		if (Beam && Julian && Beam->DistanceToPoint(Julian->GetFeetLocation()) <= TakeSceneLocal::RopePromptDistance)
		{
			NewObjective = TEXT("The rope. Blind or mask the BEAM and the GUARD:   [2] MAYA spot, banner   [3] VEX cart");
		}
		else if (Drone && Drone->HasArrived())
		{
			NewObjective = TEXT("Cross the floor to the rope. Off a gold mark, the DRONE's red circle sees him.");
		}
		else
		{
			NewObjective = TEXT("Cross the floor to the rope at the far end");
		}
	}

	Objective = NewObjective;

	// Log changes only; the scanner's ticking seconds count as one line.
	const FString LogKey = bCountdownLine ? FString(TakeSceneLocal::ScannerBlindPrefix) : NewObjective;
	if (LogKey != LastLoggedObjective)
	{
		LastLoggedObjective = LogKey;
		TAKE_LOG(TEXT("OBJECTIVE %s"), *NewObjective);
	}
}

void ATakeDirector::ShowTitle(const FString& Text, float Seconds)
{
	Title = Text;
	TitleSeconds = FMath::Max(0.1f, Seconds);
	TitleSecondsLeft = TitleSeconds;
	TAKE_LOG(TEXT("TITLE \"%s\" %.1f s"), *Text, TitleSeconds);
}

// ---------------------------------------------------------------------------------------------------------------------
// Checkpoints and the crew's posts
// ---------------------------------------------------------------------------------------------------------------------

void ATakeDirector::SaveCheckpoint(AMark* Mark, const FString& Name)
{
	UTakeGameInstance* GI = GetTakeGameInstance();
	if (!GI || !Mark)
	{
		return;
	}

	const int32 Bar = Clock ? Clock->GetBar() : 1;
	GI->CheckpointMark = Mark->MarkName;
	GI->CheckpointName = Name;
	GI->CheckpointBar = Bar;
	GI->CheckpointScene = Scene;
	GI->CheckpointUses.Reset();
	for (const ADevice* EachDevice : Devices)
	{
		if (EachDevice)
		{
			GI->CheckpointUses.Add(EachDevice->GetName(), EachDevice->UsesLeft);
		}
	}
	GI->CheckpointTriggers = FiredTriggers;
	GI->BloopersAtCheckpoint = 0;

	Flash(FString::Printf(TEXT("CHECKPOINT - %s"), *Name), TakeColors::Gold, 3.f);
	if (Clock)
	{
		Clock->PlayChime();
	}
	TAKE_LOG(TEXT("CHECKPOINT saved %s bar %d scene %d (mark %s, %d triggers, %d devices)"), *Name, Bar, Scene,
		*Mark->MarkName, FiredTriggers.Num(), GI->CheckpointUses.Num());
}

void ATakeDirector::RestoreCheckpoint()
{
	const UTakeGameInstance* GI = GetTakeGameInstance();
	if (!GI || GI->CheckpointMark.IsEmpty())
	{
		return;
	}

	// The scene and the triggers; watchers that arrived on a fired trigger are scheduled again from its bar.
	Scene = FMath::Max(1, GI->CheckpointScene);
	FiredTriggers = GI->CheckpointTriggers;
	for (AWatcher* EachWatcher : Watchers)
	{
		if (!EachWatcher || EachWatcher->ArriveOn.IsNone())
		{
			continue;
		}
		if (const int32* FiredBar = FiredTriggers.Find(EachWatcher->ArriveOn))
		{
			EachWatcher->ScheduleArrival(GetArrivalLine(*FiredBar) + FMath::Max(0, EachWatcher->ArriveExtraBars));
		}
	}

	// The device uses as they were.
	for (ADevice* EachDevice : Devices)
	{
		if (!EachDevice)
		{
			continue;
		}
		if (const int32* UsesLeft = GI->CheckpointUses.Find(EachDevice->GetName()))
		{
			EachDevice->RestoreUses(*UsesLeft);
		}
	}

	// An action whose trigger had fired is done (no second lift).
	for (AMark* EachMark : Marks)
	{
		if (EachMark && EachMark->IsActionMark() && !EachMark->ActionTrigger.IsNone()
			&& FiredTriggers.Contains(EachMark->ActionTrigger))
		{
			EachMark->SetActionDone();
		}
	}

	// Julian on the checkpoint mark, facing its yaw.
	bool bPlaced = false;
	if (Julian)
	{
		for (AMark* EachMark : Marks)
		{
			if (EachMark && EachMark->MarkName == GI->CheckpointMark)
			{
				TakeSceneLocal::PlaceCharacter(Julian.Get(), EachMark->GetStandPoint(), EachMark->GetActorRotation().Yaw);
				bPlaced = true;
				break;
			}
		}
	}
	if (!bPlaced)
	{
		TAKE_LOG(TEXT("WARNING: checkpoint mark %s not found (or no Julian): he starts where the map put him"),
			*GI->CheckpointMark);
	}

	// The crew at their posts for this scene, even the one the view is on.
	PlaceCrewForScene(Scene, true);

	TAKE_LOG(TEXT("CHECKPOINT restore %s bar %d scene %d (%d triggers)"), *GI->CheckpointMark, GI->CheckpointBar, Scene,
		FiredTriggers.Num());

	// The crew hint over the earpiece after enough bloopers here.
	if (GI->BloopersAtCheckpoint >= HintAfterBloopers)
	{
		const int32 HintIndex = HasFired(FName(TakeTriggers::PastRope)) ? 2 : Scene - 1;
		if (SceneHints.IsValidIndex(HintIndex) && !SceneHints[HintIndex].IsEmpty())
		{
			Flash(SceneHints[HintIndex], TakeColors::White, 6.f);
		}
	}
}

void ATakeDirector::PlaceCrewForScene(int32 InScene, bool bEvenIfCurrent)
{
	for (ATakeCharacter* Character : Characters)
	{
		if (!Character || Character->Id == ETakeCamera::A || Character == Julian.Get())
		{
			continue;
		}

		// Its post: the one with its Id and the highest Scene <= InScene.
		const ACrewPost* Best = nullptr;
		for (const ACrewPost* Post : CrewPosts)
		{
			if (Post && Post->Who == Character->Id && Post->Scene <= InScene && (!Best || Post->Scene > Best->Scene))
			{
				Best = Post;
			}
		}
		if (!Best)
		{
			continue;
		}

		if (!bEvenIfCurrent && PC && (PC->GetCurrentCharacter() == Character || PC->GetFlightTarget() == Character))
		{
			TAKE_LOG(TEXT("POST %s stays: the view is on her"), Character->GetCrewName());
			continue;
		}

		const FVector PostPoint = Best->GetActorLocation();
		TakeSceneLocal::PlaceCharacter(Character, PostPoint, Best->GetActorRotation().Yaw);
		TAKE_LOG(TEXT("POST %s -> scene %d (%.0f, %.0f, %.0f)"), Character->GetCrewName(), Best->Scene, PostPoint.X,
			PostPoint.Y, PostPoint.Z);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// The crew across a blooper, and the cover-story lines (Nov 14, after Marwan's note: no teleports, the crew walk)
// ---------------------------------------------------------------------------------------------------------------------

void ATakeDirector::SaveCrewPlaces()
{
	UTakeGameInstance* const GI = GetTakeGameInstance();
	if (!GI)
	{
		return;
	}
	GI->CrewPlaces.Reset();
	for (const TObjectPtr<ATakeCharacter>& CharacterPtr : Characters)
	{
		const ATakeCharacter* const Crew = CharacterPtr.Get();
		if (!Crew || Crew == Julian.Get())
		{
			continue;
		}
		FTakeCrewPlace Place;
		Place.Location = Crew->GetActorLocation();
		Place.Yaw = static_cast<float>(Crew->GetActorRotation().Yaw);
		Place.LookYaw = Crew->GetLookYaw();
		Place.LookPitch = Crew->GetLookPitch();
		GI->CrewPlaces.Add(Crew->Id, Place);
	}
	TAKE_LOG(TEXT("CREW places saved (%d)"), GI->CrewPlaces.Num());
}

void ATakeDirector::RestoreCrewPlaces()
{
	const UTakeGameInstance* const GI = GetTakeGameInstance();
	if (!GI || GI->CrewPlaces.Num() == 0)
	{
		return;
	}
	for (const TObjectPtr<ATakeCharacter>& CharacterPtr : Characters)
	{
		ATakeCharacter* const Crew = CharacterPtr.Get();
		const FTakeCrewPlace* const Place = Crew ? GI->CrewPlaces.Find(Crew->Id) : nullptr;
		if (!Place || Crew == Julian.Get())
		{
			continue;
		}
		Crew->SetActorLocationAndRotation(Place->Location, FRotator(0.f, Place->Yaw, 0.f), false, nullptr,
			ETeleportType::TeleportPhysics);
		if (UCharacterMovementComponent* Move = Crew->GetCharacterMovement())
		{
			Move->StopMovementImmediately();
		}
		Crew->SetLook(Place->LookYaw, Place->LookPitch);
		Crew->SnapFollowCamera();
		TAKE_LOG(TEXT("CREW %s back at (%.0f, %.0f, %.0f)"), Crew->GetCrewName(), Place->Location.X, Place->Location.Y,
			Place->Location.Z);
	}
}

void ATakeDirector::StepIntroLines()
{
	UTakeGameInstance* const GI = GetTakeGameInstance();
	if (!GI || !PC || PC->IsInFlight())
	{
		return;
	}
	const ATakeCharacter* const On = PC->GetCurrentCharacter();
	if (!On || On->IntroLine.IsEmpty())
	{
		return;
	}
	const uint8 Bit = static_cast<uint8>(1u << static_cast<uint32>(On->Id));
	if ((GI->IntroShownMask & Bit) != 0)
	{
		return;
	}
	GI->IntroShownMask |= Bit;
	Flash(On->IntroLine, TakeColors::White, IntroLineSeconds);
	TAKE_LOG(TEXT("INTRO %s"), On->GetCrewName());
}
