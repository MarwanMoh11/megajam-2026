// Owner: (m). Constructor by the header pass; every "// BODY: (m)" is a stub to fill.

#include "Mark.h"

#include "Components/StaticMeshComponent.h"
#include "TakeShapes.h"
#include "TakeTypes.h"

AMark::AMark()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// Collision: every shape here is NoCollision (FTakeShapes::SetupShape): no Pawn or Camera blocking.
	// An 80 cm gold disc, 2 cm thick, sitting on the floor.
	Disc = FTakeShapes::CreateShape(this, TEXT("Disc"), ETakeShape::Cylinder, Root, FVector(0.f, 0.f, 1.f), FVector(0.8f, 0.8f, 0.02f), FRotator::ZeroRotator, false);

	// The gold X under Julian, hidden until he stands here.
	CrossA = FTakeShapes::CreateShape(this, TEXT("CrossA"), ETakeShape::Cube, Root, FVector(0.f, 0.f, 3.f), FVector(1.0f, 0.12f, 0.02f), FRotator(0.f, 45.f, 0.f), false);
	CrossB = FTakeShapes::CreateShape(this, TEXT("CrossB"), ETakeShape::Cube, Root, FVector(0.f, 0.f, 3.f), FVector(1.0f, 0.12f, 0.02f), FRotator(0.f, -45.f, 0.f), false);
	CrossA->SetVisibility(false);
	CrossB->SetVisibility(false);
}

void AMark::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyLook();
}

void AMark::BeginPlay()
{
	Super::BeginPlay();
	// No reset of bIsOccupied (false from construction): Julian's BeginPlay may already have claimed this mark, and
	// actors begin play in no fixed order.
	ApplyLook();
}

void AMark::SetOccupied(bool bOccupied)
{
	bIsOccupied = bOccupied;
	ApplyLook();
}

FVector AMark::GetStandPoint() const
{
	return GetActorLocation();
}

void AMark::ApplyLook()
{
	if (!Disc || !CrossA || !CrossB)
	{
		return;
	}

	// An action running: the disc pulses brighter (two pulses a second, from its progress); done: bright gold.
	FLinearColor DiscColour = TakeColors::Gold;
	if (IsActionMark() && bActionDone)
	{
		DiscColour = TakeColors::GoldBright;
	}
	else if (IsActionMark() && ActionProgress > 0.f)
	{
		const float Pulse = 0.5f + 0.5f * FMath::Cos(ActionProgress * 2.f * UE_PI * 2.f);
		DiscColour = FMath::Lerp(TakeColors::GoldBright, TakeColors::White, 0.2f + 0.5f * Pulse);
	}
	FTakeShapes::SetColour(Disc, DiscColour);
	FTakeShapes::SetColour(CrossA, TakeColors::GoldBright);
	FTakeShapes::SetColour(CrossB, TakeColors::GoldBright);
	CrossA->SetVisibility(bIsOccupied);
	CrossB->SetVisibility(bIsOccupied);
}

// ---------------------------------------------------------------------------------------------------------------------
// Step 2 (header pass stubs; BODY: S)
// ---------------------------------------------------------------------------------------------------------------------

bool AMark::StepAction(float DeltaSeconds, bool bRunning)
{
	if (!IsActionMark() || bActionDone)
	{
		return false;
	}

	if (!bRunning)
	{
		// Off the mark or seen: it starts over (the disc back to plain gold this frame).
		if (ActionProgress > 0.f)
		{
			ActionProgress = 0.f;
			TAKE_LOG(TEXT("ACTION %s broken off"), *MarkName);
			ApplyLook();
		}
		return false;
	}

	if (ActionProgress <= 0.f)
	{
		TAKE_LOG(TEXT("ACTION %s started (%s, %.1f s)"), *MarkName, *ActionVerb, ActionSeconds);
	}
	ActionProgress += FMath::Max(0.f, DeltaSeconds);
	if (ActionProgress >= ActionSeconds)
	{
		ActionProgress = ActionSeconds;
		bActionDone = true;
		ApplyLook();
		return true;
	}
	ApplyLook();
	return false;
}

float AMark::GetActionAlpha() const
{
	if (!IsActionMark())
	{
		return 0.f;
	}
	if (bActionDone)
	{
		return 1.f;
	}
	return FMath::Clamp(ActionProgress / ActionSeconds, 0.f, 1.f);
}
