// Owner: (b). Constructor by the header pass; bodies by builder (b).

#include "TakePlayerController.h"

#include "BarClockComponent.h"
#include "CollisionQueryParams.h"
#include "Components/InputComponent.h"
#include "ComposureComponent.h"
#include "Device.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FollowSpot.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Julian.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Mark.h"
#include "Misc/Paths.h"
#include "TakeCamera.h"
#include "TakeClickable.h"
#include "TakeDirector.h"
#include "UnrealClient.h"
#include "Watcher.h"

namespace TakeSliceBPC
{
	/** The name TakeClick matches on each pass: 0 a device's DeviceName, 1 a watcher's Label, 2 a mark's MarkName,
	 *  3 the object name, 4 the editor label (WITH_EDITOR only). Empty when the pass does not apply. */
	FString NameForPass(AActor* Candidate, int32 Pass)
	{
		switch (Pass)
		{
		case 0:
			if (const ADevice* AsDevice = Cast<ADevice>(Candidate))
			{
				return AsDevice->DeviceName;
			}
			break;
		case 1:
			if (const AWatcher* AsWatcher = Cast<AWatcher>(Candidate))
			{
				return AsWatcher->Label;
			}
			break;
		case 2:
			if (const AMark* AsMark = Cast<AMark>(Candidate))
			{
				return AsMark->MarkName;
			}
			break;
		case 3:
			return Candidate->GetName();
		default:
#if WITH_EDITOR
			return Candidate->GetActorLabel();
#else
			break;
#endif
		}
		return FString();
	}

	/** The first ITakeClickable actor whose name matches Wanted (case-insensitive), by the pass order above. */
	AActor* FindClickableByName(UWorld* InWorld, const FString& Wanted)
	{
		if (!InWorld || Wanted.IsEmpty())
		{
			return nullptr;
		}
		for (int32 Pass = 0; Pass < 5; ++Pass)
		{
			for (TActorIterator<AActor> It(InWorld); It; ++It)
			{
				AActor* Candidate = *It;
				if (!Candidate || !Cast<ITakeClickable>(Candidate))
				{
					continue;
				}
				const FString CandidateName = NameForPass(Candidate, Pass);
				if (!CandidateName.IsEmpty() && CandidateName.Equals(Wanted, ESearchCase::IgnoreCase))
				{
					return Candidate;
				}
			}
		}
		return nullptr;
	}
}

ATakePlayerController::ATakePlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	DefaultMouseCursor = EMouseCursor::Default;
	bAutoManageActiveCameraTarget = false;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = false;
}

void ATakePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	// The pointer is always shown and never captured or locked. Game-and-UI uses CaptureDuringMouseDown, so the
	// first mouse-down reaches the game (no "focus" click is eaten).
	SetShowMouseCursor(true);
	FInputModeGameAndUI InputModeData;
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputModeData.SetHideCursorDuringCapture(false);
	SetInputMode(InputModeData);

	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
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
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ATakePlayerController::OnLeftClick);
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ATakePlayerController::OnKeyR);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ATakePlayerController::OnEscape);
}

void ATakePlayerController::PlayerTick(float DeltaTime)
{
	// Input bindings fire inside Super::PlayerTick (they only queue presses).
	Super::PlayerTick(DeltaTime);

	AActor* NewHover = nullptr;
	double MouseX = 0.0;
	double MouseY = 0.0;
	if (GetMousePosition(MouseX, MouseY))
	{
		FHitResult HoverHit;
		TraceClickable(FVector2D(MouseX, MouseY), NewHover, HoverHit);
	}

	AActor* OldHover = HoverTarget.Get();
	if (NewHover != OldHover)
	{
		if (ITakeClickable* OldClickable = Cast<ITakeClickable>(OldHover))
		{
			OldClickable->SetHovered(false);
		}
		if (ITakeClickable* NewClickable = Cast<ITakeClickable>(NewHover))
		{
			NewClickable->SetHovered(true);
		}
		HoverTarget = NewHover;
	}
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
	case ETakePressType::Click:
	{
		const AActor* ClickedActor = Added.Target.Get();
		TAKE_LOG(TEXT("PRESS click %s at %.0f,%.0f (%s)"), ClickedActor ? *ClickedActor->GetName() : TEXT("nothing"),
			Added.ScreenPos.X, Added.ScreenPos.Y, *Added.Source);
		break;
	}
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

void ATakePlayerController::CutTo(ATakeCamera* NewCamera)
{
	if (!NewCamera)
	{
		return;
	}
	// Default transition params: blend time 0, so the view target swaps at once (a hard cut).
	SetViewTarget(NewCamera);
	TAKE_LOG(TEXT("VIEW -> %s %s"), TakeText::CameraLetter(NewCamera->CameraId),
		NewCamera->ShotName.IsNone() ? *NewCamera->GetName() : *NewCamera->ShotName.ToString());
}

ATakeCamera* ATakePlayerController::GetViewCamera() const
{
	return Cast<ATakeCamera>(GetViewTarget());
}

bool ATakePlayerController::TraceClickable(const FVector2D& ScreenPos, AActor*& OutActor, FHitResult& OutHit) const
{
	OutActor = nullptr;
	OutHit = FHitResult();

	const UWorld* TraceWorld = GetWorld();
	if (!TraceWorld)
	{
		return false;
	}

	FVector RayOrigin = FVector::ZeroVector;
	FVector RayDirection = FVector::ForwardVector;
	if (!DeprojectScreenPositionToWorld(static_cast<float>(ScreenPos.X), static_cast<float>(ScreenPos.Y), RayOrigin, RayDirection))
	{
		return false;
	}

	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(TakeClickTrace), false);
	const FVector RayEnd = RayOrigin + RayDirection * ClickTraceDistance;
	if (!TraceWorld->LineTraceSingleByChannel(OutHit, RayOrigin, RayEnd, ECC_Visibility, TraceParams))
	{
		return false;
	}

	AActor* HitActor = OutHit.GetActor();
	if (HitActor && Cast<ITakeClickable>(HitActor))
	{
		OutActor = HitActor;
		return true;
	}
	return false;
}

AActor* ATakePlayerController::GetHoverTarget() const
{
	return HoverTarget.Get();
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

void ATakePlayerController::TakeClick(const FString& ActorNameOrLabel)
{
	const FString Wanted = ActorNameOrLabel.TrimStartAndEnd();
	AActor* Found = TakeSliceBPC::FindClickableByName(GetWorld(), Wanted);
	if (!Found)
	{
		TAKE_LOG(TEXT("TakeClick: nothing clickable named '%s'"), *Wanted);
		return;
	}

	FTakePress Press;
	Press.Type = ETakePressType::Click;
	Press.Target = Found;
	Press.HitLocation = Found->GetActorLocation();
	FVector2D Projected = FVector2D::ZeroVector;
	if (ProjectWorldLocationToScreen(Found->GetActorLocation(), Projected))
	{
		Press.ScreenPos = Projected;
	}
	Press.Source = TEXT("exec");
	QueuePress(Press);
}

void ATakePlayerController::TakeClickAt(float X, float Y)
{
	QueueClickAt(FVector2D(X, Y), TEXT("exec"));
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

	FString Line = FString::Printf(TEXT("DUMP phase=%s cam=%s shot=%s"),
		TakeText::PhaseName(Director->Phase),
		TakeText::CameraLetter(Director->CurrentCamera),
		Director->CurrentShotA ? *Director->CurrentShotA->ShotName.ToString() : TEXT("NONE"));

	if (Director->Composure)
	{
		Line += FString::Printf(TEXT(" composure=%.1f label=%s"),
			Director->Composure->GetComposure(), *Director->Composure->GetLabelText());
	}

	if (AJulian* DumpJulian = Director->Julian)
	{
		const FVector Feet = DumpJulian->GetFeetLocation();
		const AMark* Cued = DumpJulian->GetCuedMark();
		const float Beats = Director->Clock ? DumpJulian->GetBeatsToArrival(Director->Clock->GetSecondsPerBeat()) : 0.f;
		Line += FString::Printf(TEXT(" julian=(%.0f,%.0f,%.0f) cued=%s beats=%.1f walking=%d"),
			Feet.X, Feet.Y, Feet.Z, Cued ? *Cued->MarkName : TEXT("none"), Beats, DumpJulian->IsWalking() ? 1 : 0);
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

void ATakePlayerController::OnLeftClick()
{
	double MouseX = 0.0;
	double MouseY = 0.0;
	if (GetMousePosition(MouseX, MouseY))
	{
		QueueClickAt(FVector2D(MouseX, MouseY), TEXT("mouse"));
		return;
	}
	// No pointer position (outside the viewport): still a press, on nothing, so the director flashes a line.
	FTakePress Press;
	Press.Type = ETakePressType::Click;
	Press.Source = TEXT("mouse");
	QueuePress(Press);
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

void ATakePlayerController::QueueClickAt(const FVector2D& ScreenPos, const TCHAR* PressSource)
{
	AActor* ClickedActor = nullptr;
	FHitResult ClickHit;
	TraceClickable(ScreenPos, ClickedActor, ClickHit);

	FTakePress Press;
	Press.Type = ETakePressType::Click;
	Press.Target = ClickedActor;
	Press.HitLocation = ClickHit.bBlockingHit ? FVector(ClickHit.ImpactPoint) : FVector::ZeroVector;
	Press.ScreenPos = ScreenPos;
	Press.Source = PressSource;
	QueuePress(Press);
}
