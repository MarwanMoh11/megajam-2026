// Owner: (b). Constructor by the header pass; every "// BODY: (b)" is a stub to fill.

#include "TakePlayerController.h"

#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TakeCamera.h"
#include "TakeClickable.h"
#include "TakeDirector.h"
#include "UnrealClient.h"

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
	// BODY: (b)
}

void ATakePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	// BODY: (b)
}

void ATakePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	// BODY: (b)
}

void ATakePlayerController::QueuePress(const FTakePress& Press)
{
	// BODY: (b)
}

void ATakePlayerController::TakePresses(TArray<FTakePress>& Out)
{
	// BODY: (b)
	Out.Reset();
}

void ATakePlayerController::CutTo(ATakeCamera* Camera)
{
	// BODY: (b)
}

ATakeCamera* ATakePlayerController::GetViewCamera() const
{
	// BODY: (b)
	return nullptr;
}

bool ATakePlayerController::TraceClickable(const FVector2D& ScreenPos, AActor*& OutActor, FHitResult& OutHit) const
{
	// BODY: (b)
	OutActor = nullptr;
	return false;
}

AActor* ATakePlayerController::GetHoverTarget() const
{
	// BODY: (b)
	return nullptr;
}

void ATakePlayerController::TakeCut(const FString& Camera)
{
	// BODY: (b)
}

void ATakePlayerController::TakeClick(const FString& ActorNameOrLabel)
{
	// BODY: (b)
}

void ATakePlayerController::TakeClickAt(float X, float Y)
{
	// BODY: (b)
}

void ATakePlayerController::TakeNewTake()
{
	// BODY: (b)
}

void ATakePlayerController::TakeDump()
{
	// BODY: (b)
}

void ATakePlayerController::TakeShot(const FString& Name)
{
	// BODY: (b)
}

void ATakePlayerController::OnKeyOne()
{
	// BODY: (b)
}

void ATakePlayerController::OnKeyTwo()
{
	// BODY: (b)
}

void ATakePlayerController::OnKeyThree()
{
	// BODY: (b)
}

void ATakePlayerController::OnLeftClick()
{
	// BODY: (b)
}

void ATakePlayerController::OnKeyR()
{
	// BODY: (b)
}

void ATakePlayerController::OnEscape()
{
	// BODY: (b)
}

void ATakePlayerController::QueueCut(ETakeCamera Camera, const TCHAR* Source)
{
	// BODY: (b)
}

void ATakePlayerController::QueueClickAt(const FVector2D& ScreenPos, const TCHAR* Source)
{
	// BODY: (b)
}
