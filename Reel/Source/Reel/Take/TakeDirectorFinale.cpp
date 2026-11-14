// ATakeDirector slice: Scene 3 (the alarm), the ends, the title card (step 3). Owner: S.
// See TakeDirector.h for the contract.

#include "TakeDirector.h"

#include "BarClockComponent.h"
#include "DroneWatcher.h"
#include "Engine/PostProcessVolume.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Julian.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Mark.h"
#include "Shutter.h"
#include "TakeCharacter.h"
#include "TakeGameInstance.h"
#include "TakePlayerController.h"
#include "Watcher.h"

namespace TakeFinaleLocal
{
	/** Built = the deleted scenes that can be restored in this build; Restored = those of them restored this take. */
	void CountDeletedScenes(const TArray<FTakeDeletedScene>& Scenes, const UTakeGameInstance* GI, int32& OutBuilt,
		int32& OutRestored)
	{
		OutBuilt = 0;
		OutRestored = 0;
		for (const FTakeDeletedScene& Deleted : Scenes)
		{
			if (!Deleted.bBuilt)
			{
				continue;
			}
			++OutBuilt;
			if (GI && GI->RestoredScenes.Contains(Deleted.Id))
			{
				++OutRestored;
			}
		}
	}

	/** Every character stops where it stands and cannot walk (the blooper's freeze, PRINT IT, the title card). */
	void StopCharacters(const TArray<TObjectPtr<ATakeCharacter>>& InCharacters)
	{
		for (ATakeCharacter* Character : InCharacters)
		{
			UCharacterMovementComponent* Move = Character ? Character->GetCharacterMovement() : nullptr;
			if (Move && Move->MovementMode.GetValue() != MOVE_None)
			{
				Move->StopMovementImmediately();
				Move->DisableMovement();
			}
		}
	}

	/** Lets every character walk again (the title card's dismissal). */
	void ReleaseCharacters(const TArray<TObjectPtr<ATakeCharacter>>& InCharacters)
	{
		for (ATakeCharacter* Character : InCharacters)
		{
			UCharacterMovementComponent* Move = Character ? Character->GetCharacterMovement() : nullptr;
			if (Move && Move->MovementMode.GetValue() == MOVE_None)
			{
				Move->SetMovementMode(MOVE_Walking);
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Marks that appear
// ---------------------------------------------------------------------------------------------------------------------

AMark* ATakeDirector::FindMark(const FString& MarkName) const
{
	for (AMark* EachMark : Marks)
	{
		if (EachMark && EachMark->MarkName == MarkName)
		{
			return EachMark;
		}
	}
	return nullptr;
}

void ATakeDirector::SyncMarksAppeared()
{
	for (AMark* EachMark : Marks)
	{
		if (!EachMark || EachMark->AppearOn.IsNone())
		{
			continue;
		}
		const bool bShould = HasFired(EachMark->AppearOn);
		if (bShould != EachMark->HasAppeared())
		{
			EachMark->SetAppeared(bShould);
			TAKE_LOG(TEXT("MARK %s %s (%s)"), *EachMark->MarkName, bShould ? TEXT("appears") : TEXT("hidden"),
				*EachMark->AppearOn.ToString());
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// The alarm
// ---------------------------------------------------------------------------------------------------------------------

int32 ATakeDirector::GetAlarmLiveBar(int32 CaseBar) const
{
	return CaseBar + 1 + FMath::Max(0, AlarmHoldBars);
}

int32 ATakeDirector::GetArrivalBarFor(FName Trigger, int32 FiredBar) const
{
	if (Trigger == FName(TakeTriggers::Case))
	{
		// The guards walk in as the alarm goes live.
		return GetAlarmLiveBar(FiredBar);
	}
	return GetArrivalLine(FiredBar);
}

void ATakeDirector::StartAlarm(int32 CaseBar, bool bFromRestore)
{
	bAlarm = true;
	bAlarmLive = false;
	AlarmLiveBar = GetAlarmLiveBar(CaseBar);
	AlarmDeadlineBar = AlarmLiveBar + FMath::Max(1, FinaleBars);

	if (Clock)
	{
		Clock->SetAlarm(true);
	}

	const FName CaseName(TakeTriggers::Case);
	int32 ShuttersMoved = 0;
	for (AShutter* EachShutter : Shutters)
	{
		if (!EachShutter || EachShutter->DropOn != CaseName)
		{
			continue;
		}
		if (bFromRestore)
		{
			EachShutter->SetDown();
		}
		else
		{
			EachShutter->StartDrop();
		}
		++ShuttersMoved;
	}

	if (!bFromRestore)
	{
		if (!AlarmTitle.IsEmpty())
		{
			ShowTitle(AlarmTitle);
		}
		Flash(AlarmLine, TakeColors::Red, 3.f);
	}
	TAKE_LOG(TEXT("ALARM case bar %d live %d deadline %d (shutters %d%s)"), CaseBar, AlarmLiveBar, AlarmDeadlineBar,
		ShuttersMoved, bFromRestore ? TEXT(", restore") : TEXT(""));
}

void ATakeDirector::StepAlarm()
{
	if (Phase != ETakePhase::Rolling || !bAlarm || !Clock)
	{
		return;
	}

	const int32 Bar = Clock->GetBar();

	// >= every frame: a restore mid-alarm lands right; bAlarmLive keeps it to once.
	if (Bar >= AlarmLiveBar && !bAlarmLive)
	{
		bAlarmLive = true;
		for (AWatcher* EachWatcher : Watchers)
		{
			if (!EachWatcher)
			{
				continue;
			}
			EachWatcher->SetAlarmLook(true);
			if (ADroneWatcher* Drone = Cast<ADroneWatcher>(EachWatcher))
			{
				Drone->SetChaseTarget(Julian.Get());
			}
		}
		Flash(AlarmLiveLine, TakeColors::Red, 3.f);
		TAKE_LOG(TEXT("ALARM LIVE bar %d (deadline %d)"), Bar, AlarmDeadlineBar);
	}

	if (Bar >= AlarmDeadlineBar)
	{
		// The blooper fires in this frame's late tick (the phase leaves Rolling, so this runs once).
		if (PendingBlooper != EBlooperReason::ClockRanOut)
		{
			TAKE_LOG(TEXT("ALARM clock ran out (bar %d)"), Bar);
		}
		RequestBlooper(EBlooperReason::ClockRanOut);
	}
}

int32 ATakeDirector::GetAlarmBarsLeft() const
{
	if (!bAlarm)
	{
		return 0;
	}
	const int32 Bar = Clock ? Clock->GetBar() : 1;
	if (!bAlarmLive || Bar < AlarmLiveBar)
	{
		return FMath::Max(1, FinaleBars);
	}
	return FMath::Max(0, AlarmDeadlineBar - Bar);
}

float ATakeDirector::GetAlarmSecondsLeft() const
{
	if (!bAlarm || !Clock)
	{
		return 0.f;
	}
	// Bar N starts at (N - 1) * BeatsPerBar beats.
	const double DeadlineBeats = static_cast<double>(AlarmDeadlineBar - 1) * static_cast<double>(FMath::Max(1, Clock->BeatsPerBar));
	const double BeatsLeft = DeadlineBeats - Clock->GetTotalBeats();
	return FMath::Max(0.f, static_cast<float>(BeatsLeft) * Clock->GetSecondsPerBeat());
}

bool ATakeDirector::IsDroneBlinded() const
{
	for (const AWatcher* EachWatcher : Watchers)
	{
		if (EachWatcher && EachWatcher->ExposesOffMark() && EachWatcher->HasArrived()
			&& EachWatcher->GetState() == EWatcherState::Blinded)
		{
			return true;
		}
	}
	return false;
}

// ---------------------------------------------------------------------------------------------------------------------
// Deleted scenes and the grade
// ---------------------------------------------------------------------------------------------------------------------

void ATakeDirector::RestoreDeletedScene(FName Id)
{
	UTakeGameInstance* GI = GetTakeGameInstance();
	const FTakeDeletedScene* Found = DeletedScenes.FindByPredicate([Id](const FTakeDeletedScene& Deleted)
	{
		return Deleted.Id == Id;
	});
	if (!GI || !Found || !Found->bBuilt || GI->RestoredScenes.Contains(Id))
	{
		TAKE_LOG(TEXT("DELETED SCENE %s not restored (%s)"), *Id.ToString(),
			!GI ? TEXT("no game instance") : (!Found ? TEXT("unknown") : (!Found->bBuilt ? TEXT("not built") : TEXT("already restored"))));
		return;
	}

	GI->RestoredScenes.Add(Id);
	Flash(FString::Printf(TEXT("DELETED SCENE RESTORED: %s"), *Found->Name), TakeColors::GoldBright, 4.f);
	Flash(KeepThatInLine, TakeColors::White);
	if (Clock)
	{
		Clock->PlayChime();
	}
	TAKE_LOG(TEXT("DELETED SCENE RESTORED %s"), *Id.ToString());
}

ETakeGrade ATakeDirector::ComputeGrade(bool bBySideExit) const
{
	int32 Built = 0;
	int32 Restored = 0;
	TakeFinaleLocal::CountDeletedScenes(DeletedScenes, GetTakeGameInstance(), Built, Restored);

	if (Built > 0 && Restored == Built)
	{
		return ETakeGrade::DirectorsCut;
	}
	if (Restored >= FinalCutScenes || (bBySideExit && bSideExitIsFinalCut))
	{
		return ETakeGrade::FinalCut;
	}
	return ETakeGrade::RoughCut;
}

// ---------------------------------------------------------------------------------------------------------------------
// PRINT IT and the end pages
// ---------------------------------------------------------------------------------------------------------------------

void ATakeDirector::PrintIt(bool bBySideExit)
{
	if (Phase != ETakePhase::Rolling)
	{
		return;
	}

	bSideExit = bBySideExit;
	Grade = ComputeGrade(bBySideExit);
	FinishSeconds = Clock ? static_cast<float>(Clock->GetTotalBeats()) * Clock->GetSecondsPerBeat() : 0.f;
	FinishBar = Clock ? Clock->GetBar() : 1;
	PrintLine = bBySideExit ? PrintLineSide : PrintLineTrap;
	SetPhase(ETakePhase::Printed);
	// The take's last lines do not carry over onto the slate.
	Flashes.Reset();
	EndPage = 0;
	EndPageSeconds = 0.f;

	if (Clock)
	{
		Clock->SetAlarm(false);
		Clock->PlayClap();
		Clock->PlaySting();
	}

	// Everyone stops where they stand (as the blooper does).
	TakeFinaleLocal::StopCharacters(Characters);

	const UTakeGameInstance* GI = GetTakeGameInstance();
	int32 Built = 0;
	int32 Restored = 0;
	TakeFinaleLocal::CountDeletedScenes(DeletedScenes, GI, Built, Restored);
	TAKE_LOG(TEXT("PRINT IT exit=%s grade=%s time=%.1f cuts=%d bloopers=%d restored=%d/%d"),
		bBySideExit ? TEXT("SIDE") : TEXT("TRAP"), TakeText::GradeName(Grade), FinishSeconds,
		GI ? GI->Cuts : 0, GI ? GI->BlooperReel.Num() : 0, Restored, Built);
}

void ATakeDirector::StepEnd(float DeltaSeconds)
{
	EndPageSeconds += DeltaSeconds;

	const auto TurnPage = [this]()
	{
		if (EndPage >= 2)
		{
			return;
		}
		++EndPage;
		EndPageSeconds = 0.f;
		TAKE_LOG(TEXT("END PAGE %d"), EndPage);
	};

	// Presses: every one shows something this frame.
	if (PC)
	{
		TArray<FTakePress> Presses;
		PC->TakePresses(Presses);
		for (const FTakePress& Press : Presses)
		{
			if (Press.Type == ETakePressType::NewTake)
			{
				TAKE_LOG(TEXT("PRESS on the end page %d (%s): new take"), EndPage, *Press.Source);
				NewTake();
				return;
			}
			if (Press.Type == ETakePressType::Confirm || Press.Type == ETakePressType::Use)
			{
				if (EndPage >= 2)
				{
					Flash(TEXT("[R] another take"), TakeColors::White, 2.f);
					TAKE_LOG(TEXT("PRESS on the last end page (%s): [R] another take"), *Press.Source);
				}
				else
				{
					TAKE_LOG(TEXT("PRESS on the end page %d (%s): next page"), EndPage, *Press.Source);
					TurnPage();
				}
			}
			else
			{
				Flash(TEXT("That is a print.  [R] another take"), TakeColors::White, 2.f);
				TAKE_LOG(TEXT("PRESS on the end page %d (%s): cut refused, that is a print"), EndPage, *Press.Source);
			}
		}
	}

	// The pages turn by themselves: the slate, then the stats; the blooper reel stays.
	if (EndPage == 0 && EndPageSeconds >= EndSlateSeconds)
	{
		TurnPage();
	}
	else if (EndPage == 1 && EndPageSeconds >= EndStatsSeconds)
	{
		TurnPage();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// The title card
// ---------------------------------------------------------------------------------------------------------------------

void ATakeDirector::StepTitleCard(float DeltaSeconds)
{
	// Nothing moves under the card: the walkers are let go when it is dismissed.
	TakeFinaleLocal::StopCharacters(Characters);

	if (!PC)
	{
		return;
	}

	TArray<FTakePress> Presses;
	PC->TakePresses(Presses);
	for (const FTakePress& Press : Presses)
	{
		if (Phase != ETakePhase::Title)
		{
			// The card went earlier in this queue: the take is rolling from this frame.
			TAKE_LOG(TEXT("PRESS after the title card this frame (%s): dropped"), *Press.Source);
			continue;
		}

		if (Press.Type == ETakePressType::NewTake)
		{
			TAKE_LOG(TEXT("PRESS on the title card (%s): new take"), *Press.Source);
			NewTake();
			return;
		}

		if (Press.Type == ETakePressType::Confirm || Press.Type == ETakePressType::Use)
		{
			SetPhase(ETakePhase::Rolling);
			TakeFinaleLocal::ReleaseCharacters(Characters);

			// The opening slate, as a fresh take's first tick shows it.
			const UTakeGameInstance* GI = GetTakeGameInstance();
			ShowTitle(VenueLine, 4.f);
			Flash(FString::Printf(TEXT("%s   TAKE %d"), *Scene1Title, GI ? GI->TakeNumber : 1), TakeColors::GoldBright, 4.f);
			Flash(TEXT("ACTION!"), TakeColors::White, 1.5f);
			if (Clock)
			{
				Clock->PlayClap();
			}
			TAKE_LOG(TEXT("TITLE CARD dismissed (%s)"), *Press.Source);
			continue;
		}

		// A cut key on the card: say how to start.
		Flash(TitleCardPrompt, TakeColors::White, 2.f);
		TAKE_LOG(TEXT("PRESS on the title card (%s): %s"), *Press.Source, *TitleCardPrompt);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// The look
// ---------------------------------------------------------------------------------------------------------------------

ETakeCamera ATakeDirector::GetLookCamera() const
{
	if (PC)
	{
		if (const ATakeCharacter* To = PC->GetFlightTarget())
		{
			return To->Id;
		}
		if (const ATakeCharacter* On = PC->GetCurrentCharacter())
		{
			return On->Id;
		}
	}
	return CurrentCamera;
}

bool ATakeDirector::IsFilmPicture() const
{
	return bFilmLook && (!bMakingOfSplit || GetLookCamera() == ETakeCamera::A);
}

void ATakeDirector::SetupLook()
{
	GradeMID = nullptr;
	AppliedBleed = -1.f;
	AppliedMakingOf = -1.f;
	AppliedFilmLook = -1;
	if (!LookVolume)
	{
		TAKE_LOG(TEXT("LOOK no unbound post-process volume: no grade"));
		return;
	}

	// The map's volume carries MI_FilmGrade (tools/build_auction.py); a dynamic instance takes its slot.
	TArray<FWeightedBlendable>& Blendables = LookVolume->Settings.WeightedBlendables.Array;
	UMaterialInterface* Base = nullptr;
	int32 Slot = INDEX_NONE;
	for (int32 Index = 0; Index < Blendables.Num(); ++Index)
	{
		UMaterialInterface* const Candidate = Cast<UMaterialInterface>(Blendables[Index].Object);
		const UMaterial* const CandidateBase = Candidate ? Candidate->GetMaterial() : nullptr;
		if (CandidateBase && CandidateBase->MaterialDomain == MD_PostProcess)
		{
			const UMaterialInstanceDynamic* const AsMID = Cast<UMaterialInstanceDynamic>(Candidate);
			Base = AsMID && AsMID->Parent ? AsMID->Parent.Get() : Candidate;
			Slot = Index;
			break;
		}
	}
	if (!Base)
	{
		Base = FilmGradeMaterial.LoadSynchronous();
	}
	if (!Base)
	{
		TAKE_LOG(TEXT("LOOK no grade material (run tools/build_look.py): no grade"));
		return;
	}

	GradeMID = UMaterialInstanceDynamic::Create(Base, this);
	if (Slot == INDEX_NONE)
	{
		Blendables.Add(FWeightedBlendable(1.f, GradeMID));
	}
	else
	{
		Blendables[Slot].Object = GradeMID;
	}
	TAKE_LOG(TEXT("LOOK grade %s"), *Base->GetName());
}

void ATakeDirector::StepLook(float DeltaSeconds, bool bSnap)
{
	if (!LookVolume || !GradeMID)
	{
		return;
	}

	// The master switch: the grade's weight, and the volume's own tone curve (the material does the tone).
	const int32 FilmLookNow = bFilmLook ? 1 : 0;
	if (FilmLookNow != AppliedFilmLook)
	{
		AppliedFilmLook = FilmLookNow;
		FPostProcessSettings& Settings = LookVolume->Settings;
		for (FWeightedBlendable& Blendable : Settings.WeightedBlendables.Array)
		{
			if (Blendable.Object == GradeMID)
			{
				Blendable.Weight = bFilmLook ? 1.f : 0.f;
			}
		}
		Settings.bOverride_ToneCurveAmount = bFilmLook;
		Settings.ToneCurveAmount = 0.f;
		Settings.bOverride_ExpandGamut = bFilmLook;
		Settings.ExpandGamut = 0.f;
		Settings.bOverride_BlueCorrection = bFilmLook;
		Settings.BlueCorrection = 0.f;
		TAKE_LOG(TEXT("LOOK film look %s"), bFilmLook ? TEXT("on") : TEXT("off"));
	}

	// The bleed: 0 until the alarm is live, then to 1 over BleedSeconds; a snap lands it on the frame.
	const float Target = bAlarmLive ? 1.f : 0.f;
	if (bSnap || BleedSeconds <= KINDA_SMALL_NUMBER)
	{
		CurrentBleed = Target;
	}
	else
	{
		CurrentBleed = FMath::FInterpConstantTo(CurrentBleed, Target, DeltaSeconds, 1.f / BleedSeconds);
	}
	float Bleed = FMath::SmoothStep(0.f, 1.f, CurrentBleed);
	if (ForceBleed >= 0.f)
	{
		Bleed = FMath::Clamp(ForceBleed, 0.f, 1.f);
	}
	if (Bleed != AppliedBleed)
	{
		GradeMID->SetScalarParameterValue(TEXT("Bleed"), Bleed);
		AppliedBleed = Bleed;
	}

	// The making-of grade on B and C, from the frame of the cut press (the flight is seen in the look it lands in).
	const float MakingOf = (bMakingOfSplit && GetLookCamera() != ETakeCamera::A) ? 1.f : 0.f;
	if (MakingOf != AppliedMakingOf)
	{
		GradeMID->SetScalarParameterValue(TEXT("MakingOf"), MakingOf);
		AppliedMakingOf = MakingOf;
		TAKE_LOG(TEXT("LOOK grade %s"), MakingOf > 0.5f ? TEXT("making-of") : TEXT("film"));
	}
	if (bSnap)
	{
		TAKE_LOG(TEXT("LOOK bleed %.2f"), Bleed);
	}
}
