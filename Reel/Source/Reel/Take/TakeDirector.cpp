// ATakeDirector core: gathering, the tick order, the late tick, flashes. Owner: header pass (complete).
// The step functions live in the slice files named in TakeDirector.h.

#include "TakeDirector.h"

#include "BarClockComponent.h"
#include "ComposureComponent.h"
#include "Device.h"
#include "EngineUtils.h"
#include "FollowSpot.h"
#include "Julian.h"
#include "Mark.h"
#include "RedTapeZone.h"
#include "ShotZone.h"
#include "TakeCamera.h"
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

	// Camera A's first angle: the bInitialShot camera, else the first camera-A shot found.
	CurrentShotA = nullptr;
	for (ATakeCamera* Camera : Cameras)
	{
		if (!Camera || Camera->CameraId != ETakeCamera::A)
		{
			continue;
		}
		if (Camera->bInitialShot)
		{
			CurrentShotA = Camera;
			break;
		}
		if (!CurrentShotA)
		{
			CurrentShotA = Camera;
		}
	}
	CurrentCamera = ETakeCamera::A;

	EnsurePlayerController();

	const UTakeGameInstance* GI = GetTakeGameInstance();
	TAKE_LOG(TEXT("ROLLING take %d: julian=%s marks=%d watchers=%d devices=%d tape=%d shotzones=%d cameras=%d shotA=%s"),
		GI ? GI->TakeNumber : 1, Julian ? TEXT("yes") : TEXT("NO"), Marks.Num(), Watchers.Num(), Devices.Num(),
		TapeZones.Num(), ShotZones.Num(), Cameras.Num(), CurrentShotA ? *CurrentShotA->GetName() : TEXT("NONE"));

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
	Julian = nullptr;
	Marks.Reset();
	Watchers.Reset();
	Devices.Reset();
	FollowSpot = nullptr;
	TapeZones.Reset();
	ShotZones.Reset();
	Cameras.Reset();
	if (!World)
	{
		return;
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
	for (TActorIterator<AShotZone> It(World); It; ++It)
	{
		ShotZones.Add(*It);
	}
	for (TActorIterator<ATakeCamera> It(World); It; ++It)
	{
		Cameras.Add(*It);
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
				AddTickPrerequisiteActor(PC);
			}
		}
	}
	if (PC && !bInitialViewSet)
	{
		bInitialViewSet = true;
		if (ATakeCamera* First = FindCamera(ETakeCamera::A))
		{
			PC->CutTo(First);
		}
		else
		{
			TAKE_LOG(TEXT("WARNING: no camera A in the level"));
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
			TArray<FTakePress> Dropped;
			PC->TakePresses(Dropped);
		}
		StepFreeze(DeltaSeconds);
		return;
	}

	// 1. The bar clock.
	if (Clock)
	{
		Clock->Advance(DeltaSeconds);
	}

	// 2. Presses: cuts first, then clicks.
	StepPresses();
	if (Phase == ETakePhase::BlooperFreeze || bReloadRequested)
	{
		return;
	}

	// 3. Devices.
	StepDevices(DeltaSeconds);

	// 4. Julian's cued walk, the film's own cut, then the watchers and their sight tests.
	if (Julian)
	{
		Julian->StepWalk(DeltaSeconds);
	}
	StepShotList();
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

ATakeCamera* ATakeDirector::FindCamera(ETakeCamera Id) const
{
	if (Id == ETakeCamera::A && CurrentShotA)
	{
		return CurrentShotA;
	}
	for (ATakeCamera* Camera : Cameras)
	{
		if (Camera && Camera->CameraId == Id)
		{
			return Camera;
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
