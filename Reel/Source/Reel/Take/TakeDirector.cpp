// ATakeDirector core: gathering, the tick order, the late tick, flashes. Owner: (d) (written by the header passes).
// The step functions live in the slice files named in TakeDirector.h.

#include "TakeDirector.h"

#include "BarClockComponent.h"
#include "ComposureComponent.h"
#include "CrewPost.h"
#include "Device.h"
#include "EngineUtils.h"
#include "FollowSpot.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Julian.h"
#include "Mark.h"
#include "RedTapeZone.h"
#include "SceneTrigger.h"
#include "Shutter.h"
#include "Engine/PostProcessVolume.h"
#include "Materials/MaterialInstanceDynamic.h"   // QA look: FilmColour -> the grade's ColourFilm
#include "TakeAudio.h"
#include "TakeCharacter.h"
#include "TakeGameInstance.h"
#include "TakePlayerController.h"
#include "Watcher.h"

DEFINE_LOG_CATEGORY(LogTake);

namespace TakeDirectorCore
{
	TWeakObjectPtr<ATakeDirector> ActiveDirector;
}

FString TakeLogStamp()
{
	if (ATakeDirector* Director = TakeDirectorCore::ActiveDirector.Get())
	{
		return Director->MakeLogStamp();
	}
	return TEXT("-.-");
}

// ---------------------------------------------------------------------------------------------------------------------
// Late tick function
// ---------------------------------------------------------------------------------------------------------------------

void FTakeDirectorLateTick::ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread,
	const FGraphEventRef& MyCompletionGraphEvent)
{
	if (TickType != LEVELTICK_ViewportsOnly && IsValid(Director))
	{
		Director->LateTick(DeltaTime);
	}
}

FString FTakeDirectorLateTick::DiagnosticMessage()
{
	return Director ? Director->GetFullName() + TEXT("[LateTick]") : FString(TEXT("TakeDirectorLateTick"));
}

// ---------------------------------------------------------------------------------------------------------------------
// Construction and registration
// ---------------------------------------------------------------------------------------------------------------------

ATakeDirector::ATakeDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	LateTickFunction.bCanEverTick = true;
	LateTickFunction.bStartWithTickEnabled = true;
	LateTickFunction.TickGroup = TG_PostUpdateWork;
	LateTickFunction.EndTickGroup = TG_PostUpdateWork;
	LateTickFunction.Director = this;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Clock = CreateDefaultSubobject<UBarClockComponent>(TEXT("Clock"));
	Composure = CreateDefaultSubobject<UComposureComponent>(TEXT("Composure"));
	Audio = CreateDefaultSubobject<UTakeAudioComponent>(TEXT("Audio"));

	SetCanBeDamaged(false);
}

void ATakeDirector::RegisterActorTickFunctions(bool bRegister)
{
	Super::RegisterActorTickFunctions(bRegister);

	if (bRegister)
	{
		if (LateTickFunction.bCanEverTick && !LateTickFunction.IsTickFunctionRegistered())
		{
			LateTickFunction.Director = this;
			LateTickFunction.SetTickFunctionEnable(true);
			LateTickFunction.RegisterTickFunction(GetLevel());
		}
	}
	else if (LateTickFunction.IsTickFunctionRegistered())
	{
		LateTickFunction.UnRegisterTickFunction();
	}
}

ATakeDirector* ATakeDirector::Get(const UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	ATakeDirector* Active = TakeDirectorCore::ActiveDirector.Get();
	if (Active && Active->GetWorld() == World)
	{
		return Active;
	}
	if (World)
	{
		for (TActorIterator<ATakeDirector> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
// Play
// ---------------------------------------------------------------------------------------------------------------------

void ATakeDirector::BeginPlay()
{
	Super::BeginPlay();

	TakeDirectorCore::ActiveDirector = this;

	GatherActors();
	SetPhase(ETakePhase::Rolling);
	PendingBlooper = EBlooperReason::None;
	bReloadRequested = false;

	if (Composure)
	{
		Composure->Refill();
	}

	if (UTakeGameInstance* GI = GetTakeGameInstance())
	{
		if (Clock)
		{
			Clock->SetStartBar(GI->CheckpointBar);
		}
		LastBlooperReason = GI->LastBlooperReason;
	}

	// The rules run after every character has moved this frame.
	for (ATakeCharacter* Character : Characters)
	{
		if (Character && Character->GetCharacterMovement())
		{
			AddTickPrerequisiteComponent(Character->GetCharacterMovement());
		}
	}
	// A blooper's reload starts at the last checkpoint Julian reached: RestoreCheckpoint, at the start of the first
	// Tick (every actor has begun play by then). A new take (R) starts where the map placed him.
	bCheckpointRestored = false;

	// The scene script starts at Scene 1 with nothing fired; the restore refills it from the game instance.
	Scene = 1;
	FiredTriggers.Reset();
	Objective.Reset();
	LastLoggedObjective.Reset();
	Title.Reset();
	TitleSecondsLeft = 0.f;
	TitleSeconds = 0.f;
	if (SceneHints.Num() == 0)
	{
		SceneHints.Add(TEXT("MAYA: \"Cut to me with 2. My searchlight blinds the scanner.\""));
		SceneHints.Add(TEXT("MAYA: \"My breaker kills the electric eye, my spot blinds the guard. Or Vex's cart pulls him away.\""));
		SceneHints.Add(TEXT("VEX: \"My desk blinds the whole room for two bars. Then lift it.\""));
		SceneHints.Add(TEXT("VEX: \"Winch first: the curtain hides him from the guards and opens the trap. Then Maya's pyro. Then run.\""));
	}
	BeginHeist();	// --- QA heist: the dimmer, the case mark swaps the reel (TakeDirectorHeist.cpp)

	// Step 3: the alarm and the ends start over; the restore (first tick) starts the alarm again if the checkpoint
	// holds the case.
	bAlarm = false;
	bAlarmLive = false;
	AlarmLiveBar = 0;
	AlarmDeadlineBar = 0;
	bSideExit = false;
	Grade = ETakeGrade::RoughCut;
	FinishSeconds = 0.f;
	FinishBar = 0;
	PrintLine.Reset();
	EndPage = 0;
	EndPageSeconds = 0.f;
	if (DeletedScenes.Num() == 0)
	{
		auto AddDeletedScene = [this](const TCHAR* Id, const TCHAR* Name, const TCHAR* SlateLine, bool bBuilt)
		{
			FTakeDeletedScene Deleted;
			Deleted.Id = FName(Id);
			Deleted.Name = Name;
			Deleted.SlateLine = SlateLine;
			Deleted.bBuilt = bBuilt;
			DeletedScenes.Add(Deleted);
		};
		AddDeletedScene(TEXT("ColdOpen"), TEXT("THE COLD OPEN"), TEXT("the searchlight lands the moment he reaches the scanner"), false);
		AddDeletedScene(TEXT("LongTake"), TEXT("THE LONG TAKE"), TEXT("cross the floor without cutting"), false);
		AddDeletedScene(TEXT("Insert"), TEXT("THE INSERT"), TEXT("one cart catches Vance and the bodyguard as the lift lands"), false);
		AddDeletedScene(TakeTriggers::Bow, TEXT("THE BOW"), TEXT("at the gold side exit, bow to the blinded drone"), true);
	}

	CurrentCamera = ETakeCamera::A;
	CurrentUse = FTakeUseTarget();
	JulianFeetBeforeWalk = Julian ? Julian->GetFeetLocation() : FVector::ZeroVector;

	EnsurePlayerController();

	const UTakeGameInstance* GI = GetTakeGameInstance();
	TAKE_LOG(TEXT("ROLLING take %d: julian=%s characters=%d marks=%d watchers=%d devices=%d tape=%d triggers=%d posts=%d"),
		GI ? GI->TakeNumber : 1, Julian ? TEXT("yes") : TEXT("NO"), Characters.Num(), Marks.Num(), Watchers.Num(),
		Devices.Num(), TapeZones.Num(), SceneTriggers.Num(), CrewPosts.Num());

	Flash(TEXT("DIRECTOR: \"Action!\""), ETakeFlashKind::Line, 1.5f);
	if (UTakeGameInstance* MutableGI = GetTakeGameInstance())
	{
		MutableGI->bReloadingFromBlooper = false;
	}
}

void ATakeDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (TakeDirectorCore::ActiveDirector.Get() == this)
	{
		TakeDirectorCore::ActiveDirector.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

void ATakeDirector::GatherActors()
{
	UWorld* World = GetWorld();
	Characters.Reset();
	Julian = nullptr;
	Marks.Reset();
	Watchers.Reset();
	Devices.Reset();
	FollowSpot = nullptr;
	TapeZones.Reset();
	SceneTriggers.Reset();
	CrewPosts.Reset();
	SightIgnore.Reset();
	Shutters.Reset();
	LookVolume = nullptr;
	if (!World)
	{
		return;
	}

	for (TActorIterator<ATakeCharacter> It(World); It; ++It)
	{
		Characters.Add(*It);
	}
	for (TActorIterator<AJulian> It(World); It; ++It)
	{
		if (!Julian)
		{
			Julian = *It;
		}
	}
	for (TActorIterator<AMark> It(World); It; ++It)
	{
		Marks.Add(*It);
	}
	for (TActorIterator<AWatcher> It(World); It; ++It)
	{
		Watchers.Add(*It);
	}
	for (TActorIterator<ADevice> It(World); It; ++It)
	{
		Devices.Add(*It);
		if (!FollowSpot)
		{
			FollowSpot = Cast<AFollowSpot>(*It);
		}
	}
	for (TActorIterator<ARedTapeZone> It(World); It; ++It)
	{
		TapeZones.Add(*It);
	}
	for (TActorIterator<ASceneTrigger> It(World); It; ++It)
	{
		SceneTriggers.Add(*It);
	}
	for (TActorIterator<ACrewPost> It(World); It; ++It)
	{
		CrewPosts.Add(*It);
	}
	// Step 3: the shutters (they block sight when down, so they are NOT in SightIgnore) and the look volume.
	for (TActorIterator<AShutter> It(World); It; ++It)
	{
		Shutters.Add(*It);
	}
	for (TActorIterator<APostProcessVolume> It(World); It; ++It)
	{
		APostProcessVolume* const Volume = *It;
		if (Volume && Volume->bUnbound)
		{
			LookVolume = Volume;
			break;
		}
	}
	SetupLook();

	// Every take actor a sight trace ignores (IsSightBlocked).
	for (ATakeCharacter* Character : Characters)
	{
		if (Character)
		{
			SightIgnore.Add(Character);
		}
	}
	for (AWatcher* EachWatcher : Watchers)
	{
		if (EachWatcher)
		{
			SightIgnore.Add(EachWatcher);
		}
	}
	for (ADevice* EachDevice : Devices)
	{
		if (EachDevice)
		{
			SightIgnore.Add(EachDevice);
		}
	}
	for (AMark* EachMark : Marks)
	{
		if (EachMark)
		{
			SightIgnore.Add(EachMark);
		}
	}
	for (ARedTapeZone* EachZone : TapeZones)
	{
		if (EachZone)
		{
			SightIgnore.Add(EachZone);
		}
	}
	for (ASceneTrigger* EachTrigger : SceneTriggers)
	{
		if (EachTrigger)
		{
			SightIgnore.Add(EachTrigger);
		}
	}
}

void ATakeDirector::EnsurePlayerController()
{
	if (!PC)
	{
		if (UWorld* World = GetWorld())
		{
			PC = Cast<ATakePlayerController>(World->GetFirstPlayerController());
			if (PC)
			{
				// Input first, then every character and its movement, then the rules.
				AddTickPrerequisiteActor(PC);
				for (ATakeCharacter* Character : Characters)
				{
					if (Character)
					{
						Character->AddTickPrerequisiteActor(PC);
						if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
						{
							Move->AddTickPrerequisiteActor(PC);
						}
					}
				}
			}
		}
	}
	if (PC && !bInitialViewSet)
	{
		bInitialViewSet = true;
		if (Julian)
		{
			PC->SnapTo(Julian);
		}
		else
		{
			TAKE_LOG(TEXT("WARNING: no Julian in the level"));
		}
	}
}

void ATakeDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	EnsurePlayerController();
	PhaseSeconds += DeltaSeconds;
	TitleSecondsLeft = FMath::Max(0.f, TitleSecondsLeft - DeltaSeconds);
	StepFlashes(DeltaSeconds);

	// The first tick, before anything steps: every actor has begun play, so the checkpoint restore sticks.
	if (!bCheckpointRestored)
	{
		bCheckpointRestored = true;
		UTakeGameInstance* GI = GetTakeGameInstance();
		if (GI && GI->CheckpointMark.IsEmpty() && GI->BloopersAtCheckpoint == 0)
		{
			if (bShowTitleCard && !GI->bTitleShown)
			{
				// The session's first take: the title card first (StepTitleCard shows the opening slate when it goes).
				GI->bTitleShown = true;
				Flashes.Reset();
				SetPhase(ETakePhase::Title);
				TAKE_LOG(TEXT("TITLE CARD"));
			}
			else
			{
				// A fresh take (the first, or R): the opening slate.
				if (!bScriptDriven)	// the script's scenes open on their own slug lines
				{
					ShowTitle(VenueLine, 4.f);
					bTitleIsSlate = true;	// the opening slate shows the scene and the take
				}
			}
		}
		if (GI && !GI->CheckpointMark.IsEmpty())
		{
			RestoreCheckpoint();
		}
		if (Audio && Phase != ETakePhase::Title)
		{
			Audio->OnTakeStart(GI && GI->BloopersAtCheckpoint > 0);	// the clapper (the title card claps as it goes)
		}
		else if (GI && GI->BloopersAtCheckpoint >= HintAfterBloopers && SceneHints.IsValidIndex(0)
			&& !SceneHints[0].IsEmpty())
		{
			// No checkpoint yet (Scene 1 from the start): the crew hint still comes after the bloopers here.
			Flash(SceneHints[0], ETakeFlashKind::Line, 6.f);
		}
		if (GI && GI->CheckpointMark.IsEmpty())
		{
			// A blooper before any checkpoint: its cuts are spliced out too (RestoreCheckpoint does it otherwise).
			GI->Cuts = GI->CheckpointCuts;
		}
		// After a blooper the crew are where the player had walked them (nobody watches the crew).
		RestoreCrewPlaces();
		if (Julian)
		{
			JulianFeetBeforeWalk = Julian->GetFeetLocation();
		}
		// Step 3: the marks that appear on a trigger, and the picture's saturation, from the restored state.
		SyncMarksAppeared();
		StepLook(0.f, true);
	}

	// --- QA look ---
	// The colour film or the monochrome (FilmColour): written when it changes or the grade's instance is new.
	if (GradeMID && (FilmColour != AppliedFilmColour || FilmColourMID.Get() != GradeMID.Get()))
	{
		GradeMID->SetScalarParameterValue(TEXT("ColourFilm"), FMath::Clamp(FilmColour, 0.f, 1.f));
		AppliedFilmColour = FilmColour;
		FilmColourMID = GradeMID.Get();
	}
	// --- end QA look ---

	// Step 3: the title card and the end sequence run in place of steps 1-6.
	if (Phase == ETakePhase::Title)
	{
		StepTitleCard(DeltaSeconds);
		return;
	}
	if (Phase == ETakePhase::Printed)
	{
		StepEnd(DeltaSeconds);
		return;
	}

	if (Phase == ETakePhase::BlooperFreeze)
	{
		if (PC)
		{
			// No press is silent, even under the slate: R starts the new take at once, anything else says why not.
			const UTakeGameInstance* FreezeGI = GetTakeGameInstance();
			const FString AgainFrom = FreezeGI ? FreezeGI->CheckpointName : FString(TEXT("THE STREET"));
			TArray<FTakePress> Dropped;
			PC->TakePresses(Dropped);
			for (const FTakePress& Press : Dropped)
			{
				if (Press.Type == ETakePressType::NewTake)
				{
					TAKE_LOG(TEXT("PRESS during the freeze: new take"));
					NewTake();
					return;
				}
				TAKE_LOG(TEXT("PRESS during the freeze: dropped"));
				Flash(FString::Printf(TEXT("Again from %s.  [R] New take"), *AgainFrom.ToLower()), ETakeFlashKind::Refusal, 1.0f);
			}
		}
		StepFreeze(DeltaSeconds);
		return;
	}

	// 1. The bar clock, then the warnings one bar ahead (only on a bar line).
	if (Clock)
	{
		Clock->Advance(DeltaSeconds);
	}
	// The script runs the take (bScriptDriven): the old flow's alarm, intro lines, composure bloopers and scene
	// triggers stand down; devices, watchers, sight and the look keep stepping for the scenes to use.
	if (!bScriptDriven)
	{
		StepWarnings();
		StepAlarm();
	}

	// 2. The flight, the use target, then presses: cuts first, then uses.
	if (PC)
	{
		PC->StepFlight(DeltaSeconds);
		if (const ATakeCharacter* OnCharacter = PC->GetCurrentCharacter())
		{
			CurrentCamera = OnCharacter->Id;
		}
	}
	if (!bScriptDriven)
	{
		StepIntroLines();
	}
	StepUse();
	StepPresses();
	if (Phase == ETakePhase::BlooperFreeze || bReloadRequested)
	{
		return;
	}

	// 3. Devices.
	StepDevices(DeltaSeconds);
	for (AShutter* EachShutter : Shutters)
	{
		if (EachShutter)
		{
			EachShutter->StepShutter(DeltaSeconds);
		}
	}

	// 4. Julian's marks (he has already walked this frame), the watchers (so a moving frame is tested where it is
	// drawn), the exposure test, then the sight tests.
	if (Julian)
	{
		Julian->StepMarks();
	}
	StepFrameCues();
	for (AWatcher* Watcher : Watchers)
	{
		if (Watcher)
		{
			Watcher->StepWatcher(DeltaSeconds, Clock);
		}
	}
	bJulianExposed = IsJulianExposed();
	StepSight(DeltaSeconds);

	// 5. Composure.
	if (!bScriptDriven)
	{
		StepComposure(DeltaSeconds);
	}

	// 6. The scene: triggers, action marks, the goal mark, the objective line (the old flow only).
	if (!bScriptDriven)
	{
		StepTriggers();
		StepMarkActions(DeltaSeconds);
		StepScene(DeltaSeconds);
		UpdateObjective();
		StepHeist(DeltaSeconds);	// --- QA heist: the swap in the dark, and its goal line over UpdateObjective's
	}
	StepLook(DeltaSeconds);

	// For next frame's swept beam test.
	if (Julian)
	{
		JulianFeetBeforeWalk = Julian->GetFeetLocation();
	}
}

void ATakeDirector::LateTick(float DeltaSeconds)
{
	// 7. End of frame: the blooper check.
	if (Phase == ETakePhase::BlooperFreeze || bReloadRequested || bScriptDriven)
	{
		return;	// the script's scenes fail into their own retakes (AScriptSequencer)
	}
	StepBlooperCheck(DeltaSeconds);
}

// ---------------------------------------------------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------------------------------------------------

void ATakeDirector::SetPhase(ETakePhase NewPhase)
{
	if (Phase != NewPhase)
	{
		TAKE_LOG(TEXT("PHASE %s -> %s"), TakeText::PhaseName(Phase), TakeText::PhaseName(NewPhase));
	}
	Phase = NewPhase;
	PhaseSeconds = 0.f;
}

void ATakeDirector::Flash(const FString& Text, ETakeFlashKind Kind, float Seconds, int32 Tab)
{
	FTakeFlash& Line = Flashes.AddDefaulted_GetRef();
	Line.Text = Text;
	Line.Kind = Kind;
	Line.Serial = ++FlashSerial;
	Line.Tab = Tab;
	Line.Seconds = FMath::Max(0.1f, Seconds);
	Line.SecondsLeft = Line.Seconds;
	while (Flashes.Num() > FMath::Max(1, MaxFlashes))
	{
		Flashes.RemoveAt(0);
	}
	TAKE_LOG(TEXT("FLASH \"%s\""), *Text);
}

void ATakeDirector::StepFlashes(float DeltaSeconds)
{
	for (int32 i = Flashes.Num() - 1; i >= 0; --i)
	{
		Flashes[i].SecondsLeft -= DeltaSeconds;
		if (Flashes[i].SecondsLeft <= 0.f)
		{
			Flashes.RemoveAt(i);
		}
	}
}

ATakeCharacter* ATakeDirector::GetCharacter(ETakeCamera Id) const
{
	for (ATakeCharacter* Character : Characters)
	{
		if (Character && Character->Id == Id)
		{
			return Character;
		}
	}
	return nullptr;
}

UTakeGameInstance* ATakeDirector::GetTakeGameInstance() const
{
	return Cast<UTakeGameInstance>(GetGameInstance());
}

FString ATakeDirector::MakeLogStamp() const
{
	return Clock ? Clock->GetTimecode() : FString(TEXT("-.-"));
}
