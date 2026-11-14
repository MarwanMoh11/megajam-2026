// A security shutter (step 3). Owner: W. See Shutter.h for the contract.

#include "Shutter.h"

#include "BarClockComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "TakeDirector.h"
#include "TakeShapes.h"

AShutter::AShutter()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Panel = FTakeShapes::CreateShape(this, TEXT("Panel"), ETakeShape::Cube, Root);
}

namespace TakeShutterLocal
{
	/** The fall's progress (0..1) after Elapsed seconds of a DropSecs fall (0 = at once). */
	float FallAlpha(float Elapsed, float DropSecs)
	{
		return DropSecs > KINDA_SMALL_NUMBER ? FMath::Clamp(Elapsed / DropSecs, 0.f, 1.f) : 1.f;
	}
}

void AShutter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Open in the editor: sized, coloured, hanging OpenHeight up, no collision.
	if (Panel)
	{
		Panel->SetRelativeScale3D(FVector(FMath::Max(Size.X, 1.f) / 100.f, FMath::Max(Size.Y, 1.f) / 100.f, FMath::Max(Size.Z, 1.f) / 100.f));
		Panel->SetRelativeRotation(FRotator::ZeroRotator);
		Panel->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		FTakeShapes::SetColour(Panel, Colour);
	}
	SetDrop(0.f);
}

void AShutter::BeginPlay()
{
	Super::BeginPlay();

	bDropping = false;
	bDown = false;
	DropElapsed = 0.f;

	// OnConstruction does not rerun for placed actors in a package: the same open layout here.
	if (Panel)
	{
		Panel->SetMobility(EComponentMobility::Movable);
		Panel->SetRelativeScale3D(FVector(FMath::Max(Size.X, 1.f) / 100.f, FMath::Max(Size.Y, 1.f) / 100.f, FMath::Max(Size.Z, 1.f) / 100.f));
		Panel->SetRelativeRotation(FRotator::ZeroRotator);
		Panel->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Panel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		FTakeShapes::SetColour(Panel, Colour);
	}
	SetDrop(0.f);
}

void AShutter::StartDrop()
{
	if (bDropping || bDown)
	{
		return;
	}

	bDropping = true;
	DropElapsed = 0.f;

	// From this frame it blocks everything: a walker, a camera, a sight line. Nobody slips under it.
	if (Panel)
	{
		Panel->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Panel->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		if (!bBlocksWalkers)
		{
			// A side-door shutter: it closes the view, but never lands on (or traps) a crew member left in the doorway.
			Panel->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		}
	}

	// The first step shows this frame. The time is counted by StepShutter (so a StepShutter later this same frame
	// does not take the frame twice; if it only runs next frame, the panel holds for that one frame).
	const UWorld* ShutterWorld = GetWorld();
	const float FirstStep = ShutterWorld ? ShutterWorld->GetDeltaSeconds() : 0.f;
	SetDrop(TakeShutterLocal::FallAlpha(FirstStep, DropSeconds));
	TAKE_LOG(TEXT("SHUTTER falling %s"), *GetName());
}

void AShutter::SetDown()
{
	bDropping = false;
	bDown = true;
	DropElapsed = DropSeconds;
	if (Panel)
	{
		Panel->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Panel->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		if (!bBlocksWalkers)
		{
			// A side-door shutter: it closes the view, but never lands on (or traps) a crew member left in the doorway.
			Panel->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		}
	}
	SetDrop(1.f);
	TAKE_LOG(TEXT("SHUTTER down %s (restored)"), *GetName());
}

void AShutter::StepShutter(float DeltaSeconds)
{
	if (!bDropping)
	{
		return;
	}

	DropElapsed += DeltaSeconds;
	const float Alpha = TakeShutterLocal::FallAlpha(DropElapsed, DropSeconds);
	SetDrop(Alpha);
	if (Alpha < 1.f)
	{
		return;
	}

	// Landed: once, with the thud.
	bDropping = false;
	bDown = true;
	const ATakeDirector* Director = ATakeDirector::Get(this);
	if (UBarClockComponent* Clock = Director ? Director->Clock.Get() : nullptr)
	{
		Clock->PlayThud();
	}
	TAKE_LOG(TEXT("SHUTTER down %s"), *GetName());
}

void AShutter::SetDrop(float Alpha)
{
	if (!Panel)
	{
		return;
	}
	// The cube's pivot is its middle: the foot is OpenHeight up when open, on the floor when down.
	const float Fall = FMath::Clamp(Alpha, 0.f, 1.f);
	const float FootHeight = OpenHeight * (1.f - Fall);
	Panel->SetRelativeLocation(FVector(0.f, 0.f, FootHeight + FMath::Max(Size.Z, 1.f) * 0.5f));
}
