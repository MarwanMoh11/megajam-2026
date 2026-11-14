// Owner: (m). Constructor by the header pass; every "// BODY: (m)" is a stub to fill.

#include "Mark.h"

#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TakeShapes.h"
#include "TakeStyle.h"
#include "TakeTypes.h"
#include "Watcher.h"

AMark::AMark()
{
	// QA sight: it ticks only to ease its pool of light (ApplyPool).
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// Collision: every shape here is NoCollision (FTakeShapes::SetupShape): no Pawn or Camera blocking.
	// An 80 cm gold disc, 2 cm thick, sitting on the floor.
	Disc = FTakeShapes::CreateShape(this, TEXT("Disc"), ETakeShape::Cylinder, Root, FVector(0.f, 0.f, 1.f), FVector(0.8f, 0.8f, 0.02f), FRotator::ZeroRotator, false);

	// Step 4, pass 3 (§15.8): a solid T plate, CrossA the bar where the toes go (+X), CrossB the stem behind it.
	// The disc under it shows while Julian stands here, and fills with an action's progress.
	CrossA = FTakeShapes::CreateShape(this, TEXT("CrossA"), ETakeShape::Cube, Root, FVector(22.f, 0.f, 3.f), FVector(0.14f, 0.8f, 0.02f), FRotator::ZeroRotator, false);
	CrossB = FTakeShapes::CreateShape(this, TEXT("CrossB"), ETakeShape::Cube, Root, FVector(-12.f, 0.f, 3.f), FVector(0.56f, 0.14f, 0.02f), FRotator::ZeroRotator, false);
	Disc->SetVisibility(false);

	// QA sight: the warm pool from above (off until ApplyPool lights it).
	PoolLight = FTakeShapes::CreateCueSpot(this, TEXT("PoolLight"), Root, FVector(0.f, 0.f, PoolHeight),
		FRotator(-90.f, 0.f, 0.f), PoolHeight * 1.6f);
}

void AMark::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ApplyPool(DeltaSeconds);
}

void AMark::GetPoolTarget(float& OutCandelas, float& OutRadius, FLinearColor& OutColour) const
{
	OutCandelas = PoolCandelas;
	OutRadius = PoolRadius;
	OutColour = PoolColour;
	if (!bMarkLight || !bAppeared || IsHidden() || AWatcher::DrawsSightOverlays(this))
	{
		OutCandelas = 0.f;	// the drawn plate says it instead, or the mark is not there yet
		return;
	}
	const bool bRunning = IsActionMark() && !bActionDone && ActionProgress > 0.f;
	if (IsActionMark() && bActionDone)
	{
		OutCandelas = PoolCandelas * FMath::Clamp(DoneScale, 0.f, 1.f);
	}
	else if (bRunning)
	{
		// Warms and brightens with the action, and draws in a little more on him.
		const float A = GetActionAlpha();
		OutCandelas = FMath::Lerp(OccupiedCandelas, ActionCandelas, A);
		OutRadius = FMath::Lerp(OccupiedRadius, OccupiedRadius * 0.85f, A);
		OutColour = FMath::Lerp(OccupiedColour, ActionColour, A);
	}
	else if (bIsOccupied)
	{
		OutCandelas = OccupiedCandelas;
		OutRadius = OccupiedRadius;
		OutColour = OccupiedColour;
	}
}

void AMark::ApplyPool(float DeltaSeconds)
{
	if (!PoolLight)
	{
		return;
	}
	float Candelas = 0.f;
	float Radius = PoolRadius;
	FLinearColor Colour = PoolColour;
	GetPoolTarget(Candelas, Radius, Colour);

	// Lights up from its radius, not from a point; eases otherwise.
	if (PoolCandelasNow <= 0.5f)
	{
		PoolRadiusNow = Radius;
		PoolColourNow = Colour;
	}
	PoolCandelasNow = FTakeShapes::Ease(PoolCandelasNow, Candelas, PoolEaseRate, DeltaSeconds);
	PoolRadiusNow = FTakeShapes::Ease(PoolRadiusNow, Radius, PoolEaseRate, DeltaSeconds);
	const float ColourAlpha = (PoolEaseRate <= 0.f || DeltaSeconds < 0.f) ? 1.f : 1.f - FMath::Exp(-PoolEaseRate * DeltaSeconds);
	PoolColourNow = FMath::Lerp(PoolColourNow, Colour, ColourAlpha);
	if (FMath::Abs(PoolCandelasNow - Candelas) < 0.5f)
	{
		PoolCandelasNow = Candelas;
	}

	const float Height = FMath::Max(PoolHeight, 50.f);
	const FVector LightLocal(0.f, 0.f, Height);
	if (!PoolLight->GetRelativeLocation().Equals(LightLocal, 1.0))
	{
		PoolLight->SetRelativeLocation(LightLocal);
		PoolLight->SetAttenuationRadius(Height * 1.6f);
	}
	PoolLight->SetWorldRotation(FRotator(-90.f, 0.f, 0.f));
	FTakeShapes::SetCueCone(PoolLight, FMath::RadiansToDegrees(FMath::Atan2(FMath::Max(PoolRadiusNow, 10.f), Height)), 0.45f);
	FTakeShapes::SetCueLight(PoolLight, PoolCandelasNow, PoolColourNow, PoolHaze);
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
	// Step 3: a mark that appears on a trigger starts hidden; the director syncs it on its first tick (after the
	// checkpoint restore), so a restore past the trigger shows it before the first rendered frame.
	PoolCandelasNow = 0.f;
	if (!AppearOn.IsNone())
	{
		SetAppeared(false);
		ApplyPool(-1.f);	// QA sight: dark until it appears
		return;
	}
	ApplyLook();
	ApplyPool(-1.f);	// QA sight: the pool for the state now, snapped
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

	// §15.8: gold never moves and never pulses. The T is always Gold; the disc under it (GoldBright, a value step
	// lighter) shows while Julian stands on the mark, and on an action mark it fills outwards with the progress and
	// stays full when done. Growth once, no oscillation.
	const bool bRunning = IsActionMark() && !bActionDone && ActionProgress > 0.f;
	const bool bDone = IsActionMark() && bActionDone;
	FTakeShapes::SetSignal(Disc, TakeStyle::StencilGold);
	FTakeShapes::SetSignal(CrossA, TakeStyle::StencilGold);
	FTakeShapes::SetSignal(CrossB, TakeStyle::StencilGold);
	FTakeShapes::SetColour(Disc, TakeColors::GoldBright);
	FTakeShapes::SetColour(CrossA, TakeColors::Gold);
	FTakeShapes::SetColour(CrossB, TakeColors::Gold);
	const float S = FMath::Clamp(PlateScale, 0.5f, 2.5f);
	CrossA->SetRelativeLocation(FVector(22.f * S, 0.f, 3.f));
	CrossA->SetRelativeScale3D(FVector(0.14f * S, 0.8f * S, 0.03f));
	CrossB->SetRelativeLocation(FVector(-12.f * S, 0.f, 3.f));
	CrossB->SetRelativeScale3D(FVector(0.56f * S, 0.14f * S, 0.03f));
	const float Fill = bRunning ? FMath::Clamp(GetActionAlpha(), 0.05f, 1.f) : 1.f;
	Disc->SetRelativeScale3D(FVector(0.8f * Fill * S, 0.8f * Fill * S, 0.02f));
	Disc->SetVisibility(bIsOccupied || bRunning || bDone);
	CrossA->SetVisibility(true);
	CrossB->SetVisibility(true);

	// QA sight (Marwan's note 18): no T plates in game unless the drawn overlays are on; the editor still shows them so
	// the marks can be found. The pool of light says it instead (ApplyPool, eased in Tick).
	const bool bHideInGame = !AWatcher::DrawsSightOverlays(this);
	Disc->SetHiddenInGame(bHideInGame);
	CrossA->SetHiddenInGame(bHideInGame);
	CrossB->SetHiddenInGame(bHideInGame);
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

// --- step 3 stubs (header pass): the builder replaces these bodies -------------------------------------------------

void AMark::SetAppeared(bool bInAppeared)
{
	bAppeared = bInAppeared;
	SetActorHiddenInGame(!bInAppeared);
	if (!bInAppeared && bIsOccupied)
	{
		// Hidden under him: the X goes with it (AJulian::StepMarks lets go of it on its next step).
		bIsOccupied = false;
	}
	ApplyLook();
}
