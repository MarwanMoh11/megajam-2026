// Owner: (s). Constructor by the header pass; every "// BODY: (s)" is a stub to fill.

#include "TakePlayerController.h"

#include "Camera/CameraActor.h"
#include "ComposureComponent.h"
#include "Components/InputComponent.h"
#include "Device.h"
#include "Engine/World.h"
#include "FollowSpot.h"
#include "InputCoreTypes.h"
#include "Julian.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Mark.h"
#include "Misc/Paths.h"
#include "TakeCharacter.h"
#include "TakeDirector.h"
#include "UnrealClient.h"
#include "Watcher.h"

ATakePlayerController::ATakePlayerController()
{
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	bAutoManageActiveCameraTarget = false;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = false;
}

void ATakePlayerController::BeginPlay()
{
	Super::BeginPlay();
	// BODY: (s)
}

void ATakePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent)
	{
		return;
	}
	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &ATakePlayerController::OnKeyOne);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ATakePlayerController::OnKeyTwo);
	InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ATakePlayerController::OnKeyThree);
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ATakePlayerController::OnKeyR);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ATakePlayerController::OnEscape);
	// BODY: (s) E and LeftMouseButton -> OnUse
}

void ATakePlayerController::PlayerTick(float DeltaTime)
{
	// Input bindings fire inside Super::PlayerTick (they only queue presses).
	Super::PlayerTick(DeltaTime);
	// BODY: (s) walk, look, the test walk
}

void ATakePlayerController::QueuePress(const FTakePress& Press)
{
	FTakePress& Added = PendingPresses.Add_GetRef(Press);
	Added.Frame = GFrameCounter;

	switch (Added.Type)
	{
	case ETakePressType::Cut:
		TAKE_LOG(TEXT("PRESS cut %s (%s)"), TakeText::CameraLetter(Added.Camera), *Added.Source);
		break;
	case ETakePressType::Use:
		TAKE_LOG(TEXT("PRESS use (%s)"), *Added.Source);
		break;
	case ETakePressType::NewTake:
		TAKE_LOG(TEXT("PRESS new take (%s)"), *Added.Source);
		break;
	}
}

void ATakePlayerController::TakePresses(TArray<FTakePress>& Out)
{
	Out = MoveTemp(PendingPresses);
	PendingPresses.Reset();
}

void ATakePlayerController::SnapTo(ATakeCharacter* To)
{
	// BODY: (s)
}

void ATakePlayerController::StartFlight(ATakeCharacter* To)
{
	// BODY: (s)
}

void ATakePlayerController::StepFlight(float Dt)
{
	// BODY: (s)
}

float ATakePlayerController::GetFlightAlpha() const
{
	// BODY: (s)
	return 0.f;
}

void ATakePlayerController::TakeCut(const FString& CameraName)
{
	const FString Wanted = CameraName.TrimStartAndEnd().ToUpper();
	if (Wanted == TEXT("A") || Wanted == TEXT("1"))
	{
		QueueCut(ETakeCamera::A, TEXT("exec"));
	}
	else if (Wanted == TEXT("B") || Wanted == TEXT("2"))
	{
		QueueCut(ETakeCamera::B, TEXT("exec"));
	}
	else if (Wanted == TEXT("C") || Wanted == TEXT("3"))
	{
		QueueCut(ETakeCamera::C, TEXT("exec"));
	}
	else
	{
		TAKE_LOG(TEXT("TakeCut: unknown camera '%s' (use A/B/C or 1/2/3)"), *CameraName);
	}
}

void ATakePlayerController::TakeUse()
{
	QueueUse(TEXT("exec"));
}

void ATakePlayerController::TakeAim(const FString& ActorNameOrLabel)
{
	// BODY: (s)
}

void ATakePlayerController::TakeLook(float Yaw, float Pitch)
{
	// BODY: (s)
}

void ATakePlayerController::TakeWalkTo(float X, float Y)
{
	// BODY: (s)
}

void ATakePlayerController::TakeStop()
{
	// BODY: (s)
}

void ATakePlayerController::TakeNewTake()
{
	FTakePress Press;
	Press.Type = ETakePressType::NewTake;
	Press.Source = TEXT("exec");
	QueuePress(Press);
}

void ATakePlayerController::TakeDump()
{
	ATakeDirector* Director = ATakeDirector::Get(this);
	if (!Director)
	{
		TAKE_LOG(TEXT("DUMP no director"));
		return;
	}

	// BODY: (s) the rest of the header's list (current character, flight, every character, use target)
	FString Line = FString::Printf(TEXT("DUMP phase=%s cam=%s"),
		TakeText::PhaseName(Director->Phase),
		TakeText::CameraLetter(Director->CurrentCamera));

	if (Director->Composure)
	{
		Line += FString::Printf(TEXT(" composure=%.1f label=%s"),
			Director->Composure->GetComposure(), *Director->Composure->GetLabelText());
	}

	if (AJulian* DumpJulian = Director->Julian)
	{
		const FVector Feet = DumpJulian->GetFeetLocation();
		const AMark* Standing = DumpJulian->GetStandingMark();
		Line += FString::Printf(TEXT(" julian=(%.0f,%.0f,%.0f) mark=%s"),
			Feet.X, Feet.Y, Feet.Z, Standing ? *Standing->MarkName : TEXT("none"));
	}

	for (AWatcher* DumpWatcher : Director->Watchers)
	{
		if (DumpWatcher)
		{
			Line += FString::Printf(TEXT(" [%s %s%s]"), *DumpWatcher->GetLabel(), *DumpWatcher->GetStateText(),
				DumpWatcher->HasTally() ? TEXT(" TALLY") : TEXT(""));
		}
	}

	for (ADevice* DumpDevice : Director->Devices)
	{
		if (DumpDevice)
		{
			Line += FString::Printf(TEXT(" [%s uses=%d %s"), *DumpDevice->DeviceName, DumpDevice->UsesLeft,
				*DumpDevice->GetPipsText());
			if (const AFollowSpot* DumpSpot = Cast<AFollowSpot>(DumpDevice))
			{
				Line += FString::Printf(TEXT(" busy=%.1f"), DumpSpot->GetBusySecondsLeft());
			}
			else
			{
				Line += DumpDevice->IsRunning() ? TEXT(" running") : TEXT("");
			}
			Line += TEXT("]");
		}
	}

	TAKE_LOG(TEXT("%s"), *Line);
}

void ATakePlayerController::TakeShot(const FString& ShotFileName)
{
	FScreenshotRequest::RequestScreenshot(ShotFileName, true, false);
	TAKE_LOG(TEXT("SCREENSHOT %s"), *FPaths::ConvertRelativePathToFull(FScreenshotRequest::GetFilename()));
}

void ATakePlayerController::OnKeyOne()
{
	QueueCut(ETakeCamera::A, TEXT("key"));
}

void ATakePlayerController::OnKeyTwo()
{
	QueueCut(ETakeCamera::B, TEXT("key"));
}

void ATakePlayerController::OnKeyThree()
{
	QueueCut(ETakeCamera::C, TEXT("key"));
}

void ATakePlayerController::OnUse()
{
	// BODY: (s) QueueUse("key") for E, "mouse" for the left button (two handlers or one, the builder's call)
}

void ATakePlayerController::OnKeyR()
{
	FTakePress Press;
	Press.Type = ETakePressType::NewTake;
	Press.Source = TEXT("key");
	QueuePress(Press);
}

void ATakePlayerController::OnEscape()
{
	TAKE_LOG(TEXT("PRESS quit (key)"));
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void ATakePlayerController::QueueCut(ETakeCamera CutCamera, const TCHAR* PressSource)
{
	FTakePress Press;
	Press.Type = ETakePressType::Cut;
	Press.Camera = CutCamera;
	Press.Source = PressSource;
	QueuePress(Press);
}

void ATakePlayerController::QueueUse(const TCHAR* PressSource)
{
	// BODY: (s)
}
