// Owner: (s). Constructor by the header pass; bodies by builder (s).

#include "TakePlayerController.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "ComposureComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Device.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FollowSpot.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputCoreTypes.h"
#include "Julian.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Mark.h"
#include "Misc/Paths.h"
#include "TakeCharacter.h"
#include "TakeDirector.h"
#include "TakeGameInstance.h"
#include "UnrealClient.h"
#include "Watcher.h"

// Unity builds merge .cpp files: file-local helpers live in a uniquely named namespace.
namespace TakePlayerControllerLocal
{
	/**
	 * One stepped zoom: Steps quick eased jumps, each followed by a hold. A in [0, 1] -> [0, 1].
	 * stepped(a) = (floor(a * N) + ease(frac(a * N))) / N, where ease does the whole jump in the first JumpShare of
	 * the step with a sharp ease-out (quartic) and then holds: it reads as jump cuts.
	 */
	float SteppedZoom(float A, int32 Steps, float JumpShare)
	{
		if (A <= 0.f)
		{
			return 0.f;
		}
		if (A >= 1.f)
		{
			return 1.f;
		}
		const int32 N = FMath::Max(1, Steps);
		const float Scaled = A * N;
		const float Index = FMath::FloorToFloat(Scaled);
		const float Frac = Scaled - Index;
		const float T = FMath::Clamp(Frac / FMath::Max(JumpShare, KINDA_SMALL_NUMBER), 0.f, 1.f);
		const float Eased = 1.f - FMath::Pow(1.f - T, 4.f);
		return FMath::Min((Index + Eased) / N, 1.f);
	}

	/** Smooth ease in-out (smoothstep) on [0, 1]. */
	float EaseInOut(float A)
	{
		const float X = FMath::Clamp(A, 0.f, 1.f);
		return X * X * (3.f - 2.f * X);
	}

	/** Looking straight down, with Yaw as the screen's "up". */
	FQuat LookDown(double Yaw)
	{
		return FRotator(-90.0, Yaw, 0.0).Quaternion();
	}
}

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

	if (IsLocalController())
	{
		// Game-only input: the viewport gets focus and captures the mouse, no cursor.
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		SetShowMouseCursor(false);
	}
	EnsureFlightCamera();
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
	InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ATakePlayerController::OnUse);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ATakePlayerController::OnUseMouse);
	// Step 3: Enter or Space confirms (the title card's ACTION, the end sequence's next page).
	InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ATakePlayerController::OnConfirm);
	InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ATakePlayerController::OnConfirm);
}

void ATakePlayerController::PlayerTick(float DeltaTime)
{
	// Input bindings fire inside Super::PlayerTick (they only queue presses).
	Super::PlayerTick(DeltaTime);

	if (!Current || IsInFlight())
	{
		return;
	}

	// Nothing steps under the blooper slate, the title card or the end screens: no walking, looking or test walk.
	if (const ATakeDirector* Director = ATakeDirector::Get(this))
	{
		if (Director->Phase == ETakePhase::BlooperFreeze || Director->Phase == ETakePhase::Title
			|| Director->Phase == ETakePhase::Printed)
		{
			return;
		}
	}

	// Walk: (W - S, D - A). The movement component ticks after this controller, so he moves this frame.
	const float Forward = (IsInputKeyDown(EKeys::W) ? 1.f : 0.f) - (IsInputKeyDown(EKeys::S) ? 1.f : 0.f);
	const float Right = (IsInputKeyDown(EKeys::D) ? 1.f : 0.f) - (IsInputKeyDown(EKeys::A) ? 1.f : 0.f);
	const FVector2D Axes(Forward, Right);

	// Look: mouse up looks up (unless inverted).
	float MouseX = 0.f;
	float MouseY = 0.f;
	GetInputMouseDelta(MouseX, MouseY);

	// QA scene1: the glance and the look pan run on real time and give way to the player's own input on its frame.
	const float RealDt = GetRealDeltaSeconds();
	if (bGlance)
	{
		GlanceMouse += FMath::Abs(MouseX) + FMath::Abs(MouseY);
		if (!Axes.IsZero() || GlanceMouse > GlanceMouseBreak)
		{
			TAKE_LOG(TEXT("GLANCE handed back (%s)"), Axes.IsZero() ? TEXT("mouse") : TEXT("WASD"));
			EndGlance();
		}
		else
		{
			// Holding on the glance: he stands, the look waits.
			StepGlance(RealDt);
			return;
		}
	}
	if (bLookPan)
	{
		LookPanMouse += FMath::Abs(MouseX) + FMath::Abs(MouseY);
		if ((!Axes.IsZero() && !bLookPanThroughWalk) || LookPanMouse > GlanceMouseBreak)
		{
			bLookPan = false;
			TAKE_LOG(TEXT("LOOKPAN handed back (%s)"), Axes.IsZero() ? TEXT("mouse") : TEXT("WASD"));
		}
		else
		{
			StepLookPan(RealDt);
			MouseX = 0.f;	// the pan has the look this frame
			MouseY = 0.f;
		}
	}

	if (!Axes.IsZero())
	{
		if (bWalkTo)
		{
			bWalkTo = false;
			TAKE_LOG(TEXT("WALKTO cancelled (WASD)"));
		}
		Current->Move(Axes);
	}

	if (MouseX != 0.f || MouseY != 0.f)
	{
		Current->AddLook(MouseX * LookSensitivity, MouseY * LookSensitivity * (bInvertLookY ? -1.f : 1.f));
	}

	// The test walk (TakeWalkTo).
	if (bWalkTo)
	{
		const FVector Feet = Current->GetFeetLocation();
		if (FVector::Dist2D(Feet, WalkToPoint) <= WalkToTolerance)
		{
			bWalkTo = false;
			TAKE_LOG(TEXT("WALKTO arrived %.0f %.0f"), Feet.X, Feet.Y);
		}
		else
		{
			Current->MoveToward(WalkToPoint);
		}
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
	case ETakePressType::Use:
		TAKE_LOG(TEXT("PRESS use (%s)"), *Added.Source);
		break;
	case ETakePressType::NewTake:
		TAKE_LOG(TEXT("PRESS new take (%s)"), *Added.Source);
		break;
	case ETakePressType::Confirm:
		TAKE_LOG(TEXT("PRESS confirm (%s)"), *Added.Source);
		break;
	default:
		break;
	}
}

void ATakePlayerController::TakePresses(TArray<FTakePress>& Out)
{
	// W3 round C: an E pressed mid-cut is not dead: it waits (one of it) and fires on the landing frame, and a cue
	// says so on the frame it is pressed.
	if (!IsInFlight())
	{
		bUseHeld = false;
		Out = MoveTemp(PendingPresses);
		PendingPresses.Reset();
		return;
	}
	Out.Reset();
	TArray<FTakePress> Held;
	for (const FTakePress& Press : PendingPresses)
	{
		if (Press.Type != ETakePressType::Use)
		{
			Out.Add(Press);
		}
		else if (Held.Num() == 0)
		{
			Held.Add(Press);
		}
	}
	PendingPresses = MoveTemp(Held);
	if (PendingPresses.Num() > 0 && !bUseHeld)
	{
		bUseHeld = true;
		if (ATakeDirector* const Director = ATakeDirector::Get(this))
		{
			Director->Flash(TEXT("[E] as the cut lands"), ETakeFlashKind::Note, 1.2f);
		}
		TAKE_LOG(TEXT("USE held: mid-cut, fires on landing"));
	}
}

void ATakePlayerController::SnapTo(ATakeCharacter* To)
{
	if (!To)
	{
		return;
	}

	if (bWalkTo)
	{
		bWalkTo = false;
		TAKE_LOG(TEXT("WALKTO cancelled (snap)"));
	}
	FlightTo = nullptr;
	FlightElapsed = 0.f;
	bGlance = false;	// QA scene1
	bLookPan = false;
	Current = To;
	SetViewTarget(To);	// default transition params: no blend, the view changes this frame
	TAKE_LOG(TEXT("VIEW -> %s (snap)"), TakeText::CameraLetter(To->Id));
}

void ATakePlayerController::StartFlight(ATakeCharacter* To)
{
	if (!To)
	{
		TAKE_LOG(TEXT("FLIGHT refused: no destination"));
		return;
	}
	if (To == Current)
	{
		TAKE_LOG(TEXT("FLIGHT refused: already on %s"), TakeText::CameraLetter(To->Id));
		return;
	}
	if (!Current || !EnsureFlightCamera())
	{
		// Nothing to fly from (or nothing to fly with): land at once.
		TAKE_LOG(TEXT("FLIGHT -> %s has no start view: snapping"), TakeText::CameraLetter(To->Id));
		SnapTo(To);
		return;
	}

	if (bWalkTo)
	{
		bWalkTo = false;
		TAKE_LOG(TEXT("WALKTO cancelled (flight)"));
	}
	FlightTo = To;
	FlightElapsed = 0.f;
	bLookPan = false;	// QA scene1
	if (bGlance && FlightCamera)
	{
		// QA scene1: from a glance, the flight rises from the glance's view (no jump back to the follow view first).
		bGlance = false;
		FlightStartLocation = FlightCamera->GetActorLocation();
		FlightStartRotation = FlightCamera->GetActorRotation();
		const UCameraComponent* GlanceLens = FlightCamera->GetCameraComponent();
		FlightStartFOV = GlanceLens ? GlanceLens->FieldOfView : GlanceFOV;
	}
	else
	{
		bGlance = false;
		Current->GetFollowView(FlightStartLocation, FlightStartRotation, FlightStartFOV);
	}
	PlaceFlightCamera(FlightStartLocation, FlightStartRotation.Quaternion(), FlightStartFOV);
	SetViewTarget(FlightCamera.Get());	// no blend: the flight camera is the view this frame
	TAKE_LOG(TEXT("FLIGHT %s -> %s"), TakeText::CameraLetter(Current->Id), TakeText::CameraLetter(To->Id));

	// The press frame already shows the start of the first jump outward (every press shows something on its frame).
	const UWorld* World = GetWorld();
	StepFlight(World ? World->GetDeltaSeconds() : 0.f);
}

void ATakePlayerController::StepFlight(float Dt)
{
	using namespace TakePlayerControllerLocal;

	if (!IsInFlight())
	{
		return;
	}
	if (!IsValid(FlightTo))
	{
		// The destination went away mid-flight: back to the character the player is on.
		FlightTo = nullptr;
		FlightElapsed = 0.f;
		if (Current)
		{
			SetViewTarget(Current.Get());
			TAKE_LOG(TEXT("FLIGHT lost its destination: VIEW -> %s"), TakeText::CameraLetter(Current->Id));
		}
		return;
	}
	if (!IsValid(Current) || !FlightCamera)
	{
		LandFlight();
		return;
	}

	// QA scene1: on real time, so the taught switch's slowed world does not stretch the flight (a touch slower while
	// slowed, so the switch is shown under the line).
	float StepDt = Dt;
	if (bFlightOnRealTime)
	{
		StepDt = GetRealDeltaSeconds();
		const ATakeDirector* const Director = ATakeDirector::Get(this);
		if (Director && Director->IsSlowed())
		{
			StepDt *= FMath::Clamp(SlowedFlightRate, 0.1f, 1.f);
		}
	}
	FlightElapsed += FMath::Max(StepDt, 0.f);
	if (FlightSeconds <= 0.f || FlightElapsed >= FlightSeconds)
	{
		LandFlight();
		return;
	}

	const float Alpha = FlightElapsed / FlightSeconds;
	const float Rise = FMath::Clamp(FlightRiseShare, 0.f, 1.f);
	const float Pan = FMath::Clamp(FlightPanShare, 0.f, 1.f - Rise);
	const float Drop = 1.f - Rise - Pan;

	// The destination's follow view, read live every frame (the drop lands exactly on it).
	FVector EndLocation = FVector::ZeroVector;
	FRotator EndRotation = FRotator::ZeroRotator;
	float EndFOV = FlightStartFOV;
	FlightTo->GetFollowView(EndLocation, EndRotation, EndFOV);

	// The top of the flight: FlightHeight above the higher of the two characters.
	const FVector OldBody = Current->GetActorLocation();
	const FVector NewBody = FlightTo->GetActorLocation();
	const double TopZ = FMath::Max(OldBody.Z, NewBody.Z) + FlightHeight;
	const FVector AboveOld(OldBody.X, OldBody.Y, TopZ);
	const FVector AboveNew(NewBody.X, NewBody.Y, TopZ);

	// Straight down, keeping the start view's yaw on the way up and turning to the destination's yaw in the pan,
	// so neither the rise nor the drop spins.
	const FQuat DownStart = LookDown(FlightStartRotation.Yaw);
	const FQuat DownEnd = LookDown(EndRotation.Yaw);

	FVector Location;
	FQuat Rotation;
	float FOV;
	if (Alpha < Rise)
	{
		// 1 rise: stepped zooms outward from the start view to straight above the old character.
		const float S = SteppedZoom(Alpha / Rise, FlightSteps, FlightStepJumpShare);
		Location = FMath::Lerp(FlightStartLocation, AboveOld, S);
		Rotation = FQuat::Slerp(FlightStartRotation.Quaternion(), DownStart, S);
		FOV = FlightStartFOV;
	}
	else if (Alpha < Rise + Pan)
	{
		// 2 pan: smooth ease in-out across at the top; the FOV moves between the two follow views here.
		const float E = EaseInOut((Alpha - Rise) / Pan);
		Location = FMath::Lerp(AboveOld, AboveNew, E);
		Rotation = FQuat::Slerp(DownStart, DownEnd, E);
		FOV = FMath::Lerp(FlightStartFOV, EndFOV, E);
	}
	else
	{
		// 3 drop: stepped zooms down into the destination's follow view.
		const float A = Drop > KINDA_SMALL_NUMBER ? (Alpha - Rise - Pan) / Drop : 1.f;
		const float S = SteppedZoom(A, FlightSteps, FlightStepJumpShare);
		Location = FMath::Lerp(AboveNew, EndLocation, S);
		Rotation = FQuat::Slerp(DownEnd, EndRotation.Quaternion(), S);
		FOV = EndFOV;
	}
	PlaceFlightCamera(Location, Rotation, FOV);
}

float ATakePlayerController::GetFlightAlpha() const
{
	if (!IsInFlight() || FlightSeconds <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(FlightElapsed / FlightSeconds, 0.f, 1.f);
}

ACameraActor* ATakePlayerController::EnsureFlightCamera()
{
	if (!FlightCamera)
	{
		if (UWorld* World = GetWorld())
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParams.ObjectFlags |= RF_Transient;
			FlightCamera = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform::Identity, SpawnParams);
			if (FlightCamera)
			{
				if (UCameraComponent* FlightLens = FlightCamera->GetCameraComponent())
				{
					FlightLens->SetConstraintAspectRatio(false);
				}
			}
		}
	}
	return FlightCamera;
}

void ATakePlayerController::PlaceFlightCamera(const FVector& Location, const FQuat& Rotation, float FOV)
{
	if (!FlightCamera)
	{
		return;
	}
	FlightCamera->SetActorLocationAndRotation(Location, Rotation);
	if (UCameraComponent* FlightLens = FlightCamera->GetCameraComponent())
	{
		FlightLens->SetFieldOfView(FOV);
	}
}

void ATakePlayerController::LandFlight()
{
	ATakeCharacter* To = FlightTo;
	FlightTo = nullptr;
	FlightElapsed = 0.f;
	if (!To)
	{
		return;
	}

	Current = To;
	SetViewTarget(To);	// no blend: the follow camera is the view this frame
	TAKE_LOG(TEXT("VIEW -> %s"), TakeText::CameraLetter(To->Id));
}

// ---------------------------------------------------------------------------------------------------------------------
// QA scene1: the director's glance (the camera pans by itself to show who to cut to) and the look pan (QA note 10)
// ---------------------------------------------------------------------------------------------------------------------

float ATakePlayerController::GetRealDeltaSeconds() const
{
	const UWorld* World = GetWorld();
	return World ? FMath::Clamp(World->DeltaRealTimeSeconds, 0.f, 0.1f) : 0.f;
}

void ATakePlayerController::StartGlance(const FVector& Target)
{
	if (!Current || IsInFlight() || !EnsureFlightCamera())
	{
		TAKE_LOG(TEXT("GLANCE refused (%s)"), !Current ? TEXT("on no one") : TEXT("mid-flight"));
		return;
	}
	if (bWalkTo)
	{
		bWalkTo = false;
		TAKE_LOG(TEXT("WALKTO cancelled (glance)"));
	}
	bLookPan = false;
	Current->GetFollowView(GlanceFromLocation, GlanceFromRotation, GlanceFromFOV);
	GlanceTarget = Target;
	GlanceElapsed = 0.f;
	GlanceMouse = 0.f;
	bGlance = true;
	PlaceFlightCamera(GlanceFromLocation, GlanceFromRotation.Quaternion(), GlanceFromFOV);
	SetViewTarget(FlightCamera.Get());	// no blend: the turn starts from the follow view itself
	TAKE_LOG(TEXT("GLANCE %s -> (%.0f, %.0f, %.0f)"), TakeText::CameraLetter(Current->Id), Target.X, Target.Y, Target.Z);
}

void ATakePlayerController::EndGlance()
{
	if (!bGlance)
	{
		return;
	}
	bGlance = false;
	if (!IsInFlight() && Current)
	{
		SetViewTarget(Current.Get());	// back on the follow view this frame
	}
	TAKE_LOG(TEXT("GLANCE ended"));
}

void ATakePlayerController::StepGlance(float RealDt)
{
	using namespace TakePlayerControllerLocal;

	if (!bGlance || !FlightCamera)
	{
		return;
	}
	GlanceElapsed += RealDt;
	if (GlanceMaxSeconds > 0.f && GlanceElapsed >= GlanceMaxSeconds)
	{
		TAKE_LOG(TEXT("GLANCE timed out"));
		EndGlance();
		return;
	}
	const float E = EaseInOut(GlanceSeconds > KINDA_SMALL_NUMBER ? GlanceElapsed / GlanceSeconds : 1.f);
	const FVector ToTarget = GlanceTarget - GlanceFromLocation;
	const FQuat EndRotation = ToTarget.IsNearlyZero() ? GlanceFromRotation.Quaternion() : ToTarget.Rotation().Quaternion();
	PlaceFlightCamera(GlanceFromLocation, FQuat::Slerp(GlanceFromRotation.Quaternion(), EndRotation, E),
		FMath::Lerp(GlanceFromFOV, GlanceFOV, E));
}

void ATakePlayerController::StartLookPan(const FVector& Target, float Seconds, bool bThroughWalk)
{
	bLookPanThroughWalk = bThroughWalk;
	if (!Current || IsInFlight())
	{
		return;
	}
	LookPanTarget = Target;
	LookPanDuration = FMath::Max(0.f, Seconds);
	LookPanElapsed = 0.f;
	LookPanMouse = 0.f;
	LookPanFromYaw = Current->GetLookYaw();
	LookPanFromPitch = Current->GetLookPitch();
	bLookPan = true;
	TAKE_LOG(TEXT("LOOKPAN %s -> (%.0f, %.0f, %.0f) %.2f s"), TakeText::CameraLetter(Current->Id), Target.X, Target.Y,
		Target.Z, LookPanDuration);
}

void ATakePlayerController::StepLookPan(float RealDt)
{
	using namespace TakePlayerControllerLocal;

	if (!bLookPan || !Current)
	{
		bLookPan = false;
		return;
	}
	LookPanElapsed += RealDt;
	const float A = EaseInOut(LookPanDuration > KINDA_SMALL_NUMBER ? LookPanElapsed / LookPanDuration : 1.f);
	// The boom swings with the look, so the aim is solved from where the camera is this frame (it settles as it goes).
	const FRotator Want = (LookPanTarget - Current->GetAimOrigin()).Rotation();
	const float Yaw = LookPanFromYaw + FMath::FindDeltaAngleDegrees(LookPanFromYaw, static_cast<float>(Want.Yaw)) * A;
	const float Pitch = FMath::Lerp(LookPanFromPitch, static_cast<float>(Want.Pitch), A);
	Current->SetLook(Yaw, Pitch);
	if (A >= 1.f)
	{
		bLookPan = false;
		TAKE_LOG(TEXT("LOOKPAN done yaw %.1f pitch %.1f"), Current->GetLookYaw(), Current->GetLookPitch());
	}
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
	const FString Wanted = ActorNameOrLabel.TrimStartAndEnd();
	if (!Current)
	{
		TAKE_LOG(TEXT("AIM %s: no current character"), *Wanted);
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		TAKE_LOG(TEXT("AIM %s: no world"), *Wanted);
		return;
	}

	// DeviceName, watcher Label, MarkName, object name, editor label: case-insensitive, in that order.
	AActor* Found = nullptr;
	for (TActorIterator<ADevice> It(World); It && !Found; ++It)
	{
		if (It->DeviceName.Equals(Wanted, ESearchCase::IgnoreCase))
		{
			Found = *It;
		}
	}
	for (TActorIterator<AWatcher> It(World); It && !Found; ++It)
	{
		if (It->GetLabel().Equals(Wanted, ESearchCase::IgnoreCase))
		{
			Found = *It;
		}
	}
	for (TActorIterator<AMark> It(World); It && !Found; ++It)
	{
		if (It->MarkName.Equals(Wanted, ESearchCase::IgnoreCase))
		{
			Found = *It;
		}
	}
	for (TActorIterator<AActor> It(World); It && !Found; ++It)
	{
		if (It->GetName().Equals(Wanted, ESearchCase::IgnoreCase))
		{
			Found = *It;
		}
	}
#if WITH_EDITOR
	for (TActorIterator<AActor> It(World); It && !Found; ++It)
	{
		if (It->GetActorLabel(false).Equals(Wanted, ESearchCase::IgnoreCase))
		{
			Found = *It;
		}
	}
#endif
	if (!Found)
	{
		TAKE_LOG(TEXT("AIM %s: no such actor (device, watcher, mark, name or label)"), *Wanted);
		return;
	}

	FVector Target = FVector::ZeroVector;
	if (const AWatcher* AimWatcher = Cast<AWatcher>(Found))
	{
		Target = AimWatcher->GetAimPoint();
	}
	else
	{
		FVector BoundsExtent = FVector::ZeroVector;
		Found->GetActorBounds(false, Target, BoundsExtent);
	}

	// The boom swings with the look, so the camera (the aim origin) moves: solve again from the new origin.
	for (int32 Pass = 0; Pass < 3; ++Pass)
	{
		const FRotator Aim = (Target - Current->GetAimOrigin()).Rotation();
		Current->SetLook(Aim.Yaw, Aim.Pitch);
	}
	TAKE_LOG(TEXT("AIM %s yaw %.1f pitch %.1f"), *Wanted, Current->GetLookYaw(), Current->GetLookPitch());
}

void ATakePlayerController::TakeLook(float Yaw, float Pitch)
{
	if (!Current)
	{
		TAKE_LOG(TEXT("LOOK: no current character"));
		return;
	}
	Current->SetLook(Yaw, Pitch);
	TAKE_LOG(TEXT("LOOK %.1f %.1f"), Current->GetLookYaw(), Current->GetLookPitch());
}

void ATakePlayerController::TakeWalkTo(float X, float Y)
{
	if (!Current)
	{
		TAKE_LOG(TEXT("WALKTO %.0f %.0f: no current character"), X, Y);
		return;
	}
	if (IsInFlight())
	{
		TAKE_LOG(TEXT("WALKTO %.0f %.0f: refused mid-flight"), X, Y);
		return;
	}
	bWalkTo = true;
	WalkToPoint = FVector(X, Y, 0.f);
	TAKE_LOG(TEXT("WALKTO %.0f %.0f"), X, Y);
}

void ATakePlayerController::TakeStop()
{
	bWalkTo = false;
	TAKE_LOG(TEXT("WALKTO stopped"));
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

	FString Line = FString::Printf(TEXT("DUMP phase=%s cam=%s"),
		TakeText::PhaseName(Director->Phase),
		TakeText::CameraLetter(Director->CurrentCamera));

	// Step 2: the scene, the objective, the fired triggers (with their bars) and the game instance's checkpoint.
	Line += FString::Printf(TEXT(" scene=%d objective=\"%s\""), Director->Scene, *Director->Objective);

	// Step 3: the alarm, and the end page while Printed.
	{
		const TCHAR* AlarmState = !Director->bAlarm ? TEXT("off") : (Director->bAlarmLive ? TEXT("live") : TEXT("hold"));
		Line += FString::Printf(TEXT(" alarm=%s barsLeft=%d secLeft=%.1f"), AlarmState, Director->GetAlarmBarsLeft(),
			Director->GetAlarmSecondsLeft());
	}
	if (Director->Phase == ETakePhase::Printed)
	{
		Line += FString::Printf(TEXT(" end page %d grade %s exit %s"), Director->EndPage,
			TakeText::GradeName(Director->Grade), Director->bSideExit ? TEXT("side") : TEXT("trap"));
	}
	{
		FString Fired;
		for (const TPair<FName, int32>& Fire : Director->FiredTriggers)
		{
			Fired += FString::Printf(TEXT("%s%s@%d"), Fired.IsEmpty() ? TEXT("") : TEXT(","), *Fire.Key.ToString(), Fire.Value);
		}
		Line += FString::Printf(TEXT(" triggers=[%s]"), *Fired);
	}
	if (const UTakeGameInstance* DumpGI = Director->GetTakeGameInstance())
	{
		Line += FString::Printf(TEXT(" checkpoint=[%s mark=%s bar=%d scene=%d bloopers=%d]"), *DumpGI->CheckpointName,
			DumpGI->CheckpointMark.IsEmpty() ? TEXT("none") : *DumpGI->CheckpointMark, DumpGI->CheckpointBar,
			DumpGI->CheckpointScene, DumpGI->BloopersAtCheckpoint);
	}
	else
	{
		Line += TEXT(" checkpoint=none");
	}
	for (const AMark* DumpMark : Director->Marks)
	{
		if (DumpMark && DumpMark->IsActionMark())
		{
			Line += FString::Printf(TEXT(" [action %s %s %.2f%s]"), *DumpMark->MarkName, *DumpMark->ActionTrigger.ToString(),
				DumpMark->GetActionAlpha(), DumpMark->IsActionDone() ? TEXT(" done") : TEXT(""));
		}
	}

	// The character the player is on, and the flight.
	Line += FString::Printf(TEXT(" on=%s"), Current ? Current->GetCrewName() : TEXT("none"));
	if (IsInFlight())
	{
		Line += FString::Printf(TEXT(" flight=%s@%.2f"), FlightTo->GetCrewName(), GetFlightAlpha());
	}
	else
	{
		Line += TEXT(" flight=none");
	}
	if (bWalkTo)
	{
		Line += FString::Printf(TEXT(" walkto=(%.0f,%.0f)"), WalkToPoint.X, WalkToPoint.Y);
	}

	if (Director->Composure)
	{
		Line += FString::Printf(TEXT(" composure=%.1f label=%s"),
			Director->Composure->GetComposure(), *Director->Composure->GetLabelText());
	}

	// Every character: feet, look, standing, speed; Julian's mark.
	for (ATakeCharacter* DumpCharacter : Director->Characters)
	{
		if (!DumpCharacter)
		{
			continue;
		}
		const FVector Feet = DumpCharacter->GetFeetLocation();
		Line += FString::Printf(TEXT(" [%s %s feet=(%.0f,%.0f,%.0f) look=(%.1f,%.1f) %s v=%.0f"),
			TakeText::CameraLetter(DumpCharacter->Id), DumpCharacter->GetCrewName(),
			Feet.X, Feet.Y, Feet.Z,
			DumpCharacter->GetLookYaw(), DumpCharacter->GetLookPitch(),
			DumpCharacter->IsStanding() ? TEXT("standing") : TEXT("moving"),
			DumpCharacter->GetVelocity().Size2D());
		if (const AJulian* DumpJulian = Cast<AJulian>(DumpCharacter))
		{
			const AMark* Standing = DumpJulian->GetStandingMark();
			Line += FString::Printf(TEXT(" mark=%s"), Standing ? *Standing->MarkName : TEXT("none"));
		}
		Line += TEXT("]");
	}

	// The use target and its prompt.
	const FTakeUseTarget& Use = Director->CurrentUse;
	Line += FString::Printf(TEXT(" use=[%s watcher=%s fire=%d prompt=\"%s\"]"),
		Use.Device ? *Use.Device->DeviceName : TEXT("none"),
		Use.Watcher ? *Use.Watcher->GetLabel() : TEXT("none"),
		Use.bCanFire ? 1 : 0,
		*Use.Prompt);

	for (AWatcher* DumpWatcher : Director->Watchers)
	{
		if (DumpWatcher)
		{
			Line += FString::Printf(TEXT(" [%s %s%s"), *DumpWatcher->GetLabel(), *DumpWatcher->GetStateText(),
				DumpWatcher->HasTally() ? TEXT(" TALLY") : TEXT(""));
			if (DumpWatcher->GetEffectSecondsLeft() > 0.f)
			{
				Line += FString::Printf(TEXT(" %.1fs"), DumpWatcher->GetEffectSecondsLeft());
			}
			Line += FString::Printf(TEXT(" arrived=%d arrive=%d"), DumpWatcher->HasArrived() ? 1 : 0,
				DumpWatcher->GetArriveBar());
			Line += TEXT("]");
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
			Line += DumpDevice->IsInReach() ? TEXT(" reach") : TEXT("");
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
	QueueUse(TEXT("key"));
}

void ATakePlayerController::OnUseMouse()
{
	QueueUse(TEXT("mouse"));
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
	FTakePress Press;
	Press.Type = ETakePressType::Use;
	Press.Source = PressSource;
	QueuePress(Press);
}

// ---------------------------------------------------------------------------------------------------------------------
// Step 2 (header pass stubs; BODY: D)
// ---------------------------------------------------------------------------------------------------------------------

void ATakePlayerController::TakeWarp(float X, float Y, float Z)
{
	if (!Current)
	{
		TAKE_LOG(TEXT("WARP %.0f %.0f %.0f: no current character"), X, Y, Z);
		return;
	}

	// The feet go to (X, Y, Z): the actor sits half a capsule above them.
	const UCapsuleComponent* WarpCapsule = Current->GetCapsuleComponent();
	const float HalfHeight = WarpCapsule ? WarpCapsule->GetScaledCapsuleHalfHeight() : 0.f;
	bWalkTo = false;
	Current->SetActorLocation(FVector(X, Y, Z + HalfHeight), false, nullptr, ETeleportType::TeleportPhysics);
	if (UCharacterMovementComponent* WarpMove = Current->GetCharacterMovement())
	{
		WarpMove->StopMovementImmediately();
	}
	Current->SnapFollowCamera();

	const FVector Feet = Current->GetFeetLocation();
	if (ATakeDirector* WarpDirector = ATakeDirector::Get(this))
	{
		if (Current == WarpDirector->Julian.Get())
		{
			WarpDirector->JulianFeetBeforeWalk = Feet;	// or the swept beam test would cross everything in between
		}
	}
	TAKE_LOG(TEXT("WARP %s %.0f %.0f %.0f"), Current->GetCrewName(), Feet.X, Feet.Y, Feet.Z);
}

void ATakePlayerController::TakeTrigger(const FString& Name)
{
	ATakeDirector* Director = ATakeDirector::Get(this);
	if (!Director || Name.IsEmpty())
	{
		TAKE_LOG(TEXT("TAKETRIGGER %s: %s"), *Name, Director ? TEXT("no name") : TEXT("no director"));
		return;
	}
	const FName TriggerName(*Name);
	if (Director->HasFired(TriggerName))
	{
		TAKE_LOG(TEXT("TAKETRIGGER %s: already fired"), *Name);
	}
	else
	{
		TAKE_LOG(TEXT("TAKETRIGGER %s (exec)"), *Name);
	}
	Director->FireTrigger(TriggerName);
}

void ATakePlayerController::TakeBlooper()
{
	ATakeDirector* Director = ATakeDirector::Get(this);
	if (!Director)
	{
		TAKE_LOG(TEXT("TAKEBLOOPER: no director"));
		return;
	}
	TAKE_LOG(TEXT("TAKEBLOOPER (exec)"));
	Director->RequestBlooper(EBlooperReason::ComposureZero);
}

// ---------------------------------------------------------------------------------------------------------------------
// Step 3 (BODY: D)
// ---------------------------------------------------------------------------------------------------------------------

void ATakePlayerController::TakeConfirm()
{
	FTakePress Press;
	Press.Type = ETakePressType::Confirm;
	Press.Source = TEXT("exec");
	QueuePress(Press);
}

void ATakePlayerController::OnConfirm()
{
	FTakePress Press;
	Press.Type = ETakePressType::Confirm;
	Press.Source = TEXT("key");
	QueuePress(Press);
}
