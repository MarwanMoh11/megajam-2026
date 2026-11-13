// ATakeDirector core: gathering, the tick order, the late tick, flashes. Owner: (d) (written by the header passes).
// The step functions live in the slice files named in TakeDirector.h.

#include "TakeDirector.h"

#include "BarClockComponent.h"
#include "ComposureComponent.h"
#include "Device.h"
#include "EngineUtils.h"
#include "FollowSpot.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Julian.h"
#include "Mark.h"
#include "RedTapeZone.h"
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
	// A blooper's reload starts at the last checkpoint Julian reached; a new take (R) starts where he was placed.
	const UTakeGameInstance* CheckpointGI = GetTakeGameInstance();
	if (CheckpointGI && Julian && !CheckpointGI->CheckpointMark.IsEmpty())
	{
		for (const TObjectPtr<AMark>& MarkPtr : Marks)
		{
			const AMark* const EachMark = MarkPtr.Get();
			if (EachMark && EachMark->bCheckpoint && EachMark->MarkName == CheckpointGI->CheckpointMark)
			{
				const FVector FeetToCentre = Julian->GetActorLocation() - Julian->GetFeetLocation();
				const float MarkYaw = EachMark->GetActorRotation().Yaw;
				Julian->SetActorLocationAndRotation(EachMark->GetStandPoint() + FeetToCentre, FRotator(0.f, MarkYaw, 0.f),
					false, nullptr, ETeleportType::TeleportPhysics);
				Julian->SetLook(MarkYaw, Julian->StartLookPitch);
				Julian->SnapFollowCamera();
				TAKE_LOG(TEXT("CHECKPOINT start at %s"), *EachMark->MarkName);
				break;
			}
		}
	}

	CurrentCamera = ETakeCamera::A;
	CurrentUse = FTakeUseTarget();
	JulianFeetBeforeWalk = Julian ? Julian->GetFeetLocation() : FVector::ZeroVector;

	EnsurePlayerController();

	const UTakeGameInstance* GI = GetTakeGameInstance();
	TAKE_LOG(TEXT("ROLLING take %d: julian=%s characters=%d marks=%d watchers=%d devices=%d tape=%d"),
		GI ? GI->TakeNumber : 1, Julian ? TEXT("yes") : TEXT("NO"), Characters.Num(), Marks.Num(), Watchers.Num(),
		Devices.Num(), TapeZones.Num());

	Flash(TEXT("ACTION!"), TakeColors::White, 1.5f);
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
	StepFlashes(DeltaSeconds);

	if (Phase == ETakePhase::BlooperFreeze)
	{
		if (PC)
		{
			// No press is silent, even under the slate: R starts the new take at once, anything else says why not.
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
				Flash(TEXT("Cut! Resetting to the start"), TakeColors::White, 1.0f);
			}
		}
		StepFreeze(DeltaSeconds);
		return;
	}

	// 1. The bar clock.
	if (Clock)
	{
		Clock->Advance(DeltaSeconds);
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
	StepUse();
	StepPresses();
	if (Phase == ETakePhase::BlooperFreeze || bReloadRequested)
	{
		return;
	}

	// 3. Devices.
	StepDevices(DeltaSeconds);

	// 4. Julian's marks (he has already walked this frame), then the watchers and their sight tests.
	if (Julian)
	{
		Julian->StepMarks();
	}
	bJulianExposed = IsJulianExposed();
	for (AWatcher* Watcher : Watchers)
	{
		if (Watcher)
		{
			Watcher->StepWatcher(DeltaSeconds, Clock);
		}
	}
	StepSight(DeltaSeconds);

	// 5. Composure.
	StepComposure(DeltaSeconds);

	// 6. The scene.
	StepScene(DeltaSeconds);

	// For next frame's swept beam test.
	if (Julian)
	{
		JulianFeetBeforeWalk = Julian->GetFeetLocation();
	}
}

void ATakeDirector::LateTick(float DeltaSeconds)
{
	// 7. End of frame: the blooper check.
	if (Phase == ETakePhase::BlooperFreeze || bReloadRequested)
	{
		return;
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

void ATakeDirector::Flash(const FString& Text, const FLinearColor& Colour, float Seconds)
{
	FTakeFlash& Line = Flashes.AddDefaulted_GetRef();
	Line.Text = Text;
	Line.Colour = Colour;
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
