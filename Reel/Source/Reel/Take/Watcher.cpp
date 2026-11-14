// Owner: (d). Constructor by the header pass; bodies by (d).

#include "Watcher.h"

#include "BarClockComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Julian.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TakeDirector.h"
#include "TakeShapes.h"
#include "TakeStyle.h"

namespace TakeWatcherLocal
{
	/** Line-of-sight targets on Julian, above his feet (Watcher.h: chest and head). */
	constexpr float ChestHeight = 120.f;
	constexpr float HeadHeight = 170.f;

	/** The BlindedLine flash. */
	constexpr float BlindedLineSeconds = 2.5f;

	/** One thin frame line: a cube on Pivot from From to To (Pivot's space), its X axis along the line. */
	UStaticMeshComponent* AddFrameLine(AActor* Owner, USceneComponent* Pivot, const FVector& From, const FVector& To, double Thickness)
	{
		const FVector Dir = To - From;
		const double Length = Dir.Size();
		if (!Owner || !Pivot || Length < 1.0)
		{
			return nullptr;
		}
		const FVector Scale(Length * 0.01, Thickness * 0.01, Thickness * 0.01);
		UStaticMeshComponent* Line = FTakeShapes::AddShapeAtRuntime(Owner, ETakeShape::Cube, Pivot, (From + To) * 0.5, Scale, TakeColors::Red, false);
		if (Line)
		{
			FTakeShapes::SetSignal(Line, TakeStyle::StencilRed);
			FTakeShapes::SetPattern(Line, TakeStyle::PatternDash);	// §15.8: dashed from the eye (From) outwards
			Line->SetRelativeRotation(Dir.Rotation());
		}
		return Line;
	}
}

AWatcher::AWatcher()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// The frame lies just above the floor; subclasses shape FrameMesh.
	FramePivot = CreateDefaultSubobject<USceneComponent>(TEXT("FramePivot"));
	FramePivot->SetupAttachment(Root);
	FramePivot->SetRelativeLocation(FVector(0.f, 0.f, 2.f));

	FrameMesh = FTakeShapes::CreateShape(this, TEXT("FrameMesh"), ETakeShape::Cube, FramePivot, FVector::ZeroVector, FVector::OneVector, FRotator::ZeroRotator, false);

	Tally = FTakeShapes::CreateShape(this, TEXT("Tally"), ETakeShape::Cube, Root, FVector(0.f, 0.f, 200.f), FVector(0.15f, 0.15f, 0.15f), FRotator::ZeroRotator, false);

	ClickVolume = FTakeShapes::CreateClickVolume(this, TEXT("ClickVolume"), Root, FVector(70.f, 70.f, 100.f), FVector(0.f, 0.f, 100.f));

	// QA sight: the planning lift (off at rest; UpdateWorldCues places it over the aim point each frame it shows).
	PlanLight = FTakeShapes::CreateCuePoint(this, TEXT("PlanLight"), Root, FVector(0.f, 0.f, AimHeight) + PlanLightOffset, PlanLightRadius);
}

void AWatcher::BeginPlay()
{
	Super::BeginPlay();

	HomeLocation = GetActorLocation();
	HomeYaw = static_cast<float>(GetActorRotation().Yaw);

	DarkSecondsLeft = 0.f;
	DarkKind = EWatcherState::Blinded;
	bTally = false;

	// Arrival (step 2): there from the start, or Off until its bar. ArriveBar is kept: the director may have
	// scheduled it already.
	bArrived = ArriveOn.IsNone();
	ArrivalBlinkSecondsLeft = 0.f;

	// Step 3: the guards who walk in at the alarm are not on set until they arrive.
	if (!ArriveOn.IsNone() && bHideUntilArrived && !bArrived)
	{
		SetActorHiddenInGame(true);
	}

	FrameMID = FTakeShapes::SetColour(FrameMesh, TakeColors::Red);
	TallyMID = FTakeShapes::SetColour(Tally, TakeColors::RedDark);
	FTakeShapes::SetSignal(FrameMesh, TakeStyle::StencilRed);
	FTakeShapes::SetPattern(FrameMesh, FramePattern);
	FTakeShapes::SetSignal(Tally, TakeStyle::StencilRed);

	SyncState();
	RefreshFrameLook();
	ApplyFrameScale();

	// Subclasses that lay their frame out after this call it again.
	BuildFrameLines();

	// QA sight: the world cues for the state now, snapped (subclasses snap again after their own layout).
	PlanAlpha = 0.f;
	CueDeltaSeconds = 0.f;
	UpdateWorldCues(-1.f);
}

bool AWatcher::DrawsSightOverlays(const UObject* Context)
{
	const ATakeDirector* Director = Context ? ATakeDirector::Get(Context) : nullptr;
	return Director && Director->bDrawnSightOverlays;
}

void AWatcher::UpdateWorldCues(float DeltaSeconds)
{
	// The planning lift: a soft light over a watcher the crew member in reach could catch. World cues only (the drawn
	// overlays have their faint frame for this).
	if (!PlanLight)
	{
		return;
	}
	const bool bLift = !DrawsSightOverlays(this) && bPlanned && IsLive() && !IsHidden();
	PlanAlpha = FTakeShapes::Ease(PlanAlpha, bLift ? 1.f : 0.f, PlanLightEaseRate, DeltaSeconds);
	if (PlanAlpha < 0.002f)
	{
		PlanAlpha = 0.f;
	}
	else
	{
		PlanLight->SetWorldLocation(GetAimPoint() + GetActorRotation().RotateVector(PlanLightOffset));
		if (!FMath::IsNearlyEqual(PlanLight->AttenuationRadius, PlanLightRadius, 1.f))
		{
			PlanLight->SetAttenuationRadius(FMath::Max(PlanLightRadius, 10.f));
		}
	}
	FTakeShapes::SetCueLight(PlanLight, PlanLightCandelas * PlanAlpha, PlanLightColour);
}

void AWatcher::SetHovered(bool bInHovered)
{
	bHovered = bInHovered;
	RefreshFrameLook();
}

void AWatcher::SetFrameCue(ETakeFrameDisplay InMode, bool bInPlanned)
{
	if (InMode == FrameMode && bInPlanned == bPlanned)
	{
		return;
	}
	FrameMode = InMode;
	bPlanned = bInPlanned;
	RefreshFrameLook();
}

void AWatcher::StepWatcher(float DeltaSeconds, UBarClockComponent* Clock)
{
	// Subclasses update their own timers / clock flags before calling this. The arrival first: a watcher that
	// arrives this frame shows its frame and lines this frame (SyncState / RefreshFrameLook below).
	CueDeltaSeconds += FMath::Max(DeltaSeconds, 0.f);	// QA sight: spent by this frame's UpdateWorldCues (SetTally)
	StepArrival(DeltaSeconds, Clock);

	const EWatcherState OldState = State;
	if (DarkSecondsLeft > 0.f)
	{
		DarkSecondsLeft = FMath::Max(DarkSecondsLeft - DeltaSeconds, 0.f);
	}

	SyncState();
	if (State != OldState)
	{
		TAKE_LOG(TEXT("%s %s"), *Label, *GetStateText());
	}

	// Every frame: the look follows the state, the hold and the arrival warning. Nothing moves with the beat.
	RefreshFrameLook();
}

void AWatcher::AddDark(EWatcherState Kind, float Duration)
{
	const bool bWasDark = DarkSecondsLeft > 0.f;
	DarkSecondsLeft = FMath::Max(DarkSecondsLeft, FMath::Max(Duration, UE_KINDA_SMALL_NUMBER));
	if (Kind == EWatcherState::Masked || !bWasDark)
	{
		DarkKind = Kind;
	}
	SyncState();
	RefreshFrameLook();
}

void AWatcher::SyncState()
{
	// Not arrived: Off before anything else (no frame, not live).
	if (!HasArrived())
	{
		State = EWatcherState::Off;
		EffectSecondsLeft = 0.f;
		return;
	}

	if (DarkSecondsLeft > 0.f)
	{
		State = DarkKind;
		EffectSecondsLeft = DarkSecondsLeft;
	}
	else
	{
		State = EWatcherState::Watching;
		EffectSecondsLeft = 0.f;
	}
}

bool AWatcher::HoldsJulian(const AJulian* Julian) const
{
	// The base has no frame shape; subclasses test their own.
	return false;
}

bool AWatcher::ApplyEffect(EDeviceEffect Effect, float Duration, const FVector& Point)
{
	// A watcher that has not arrived is not on set: nothing lands on it.
	if (Effect != EDeviceEffect::Blind || !HasArrived())
	{
		return false;
	}

	// Stacks: a shorter blind never cuts a longer dark; a lure (subclass timer) is untouched.
	AddDark(EWatcherState::Blinded, Duration);
	TAKE_LOG(TEXT("%s BLINDED %.1f s"), *Label, DarkSecondsLeft);

	if (!BlindedLine.IsEmpty())
	{
		if (ATakeDirector* Director = ATakeDirector::Get(this))
		{
			Director->Flash(BlindedLine, ETakeFlashKind::Line, TakeWatcherLocal::BlindedLineSeconds);
		}
	}
	return true;
}

void AWatcher::SetTally(bool bHolding)
{
	if (bHolding != bTally)
	{
		bTally = bHolding;
		// RefreshFrameLook colours the tally (lit while holding, or the arrival blink) this frame.
		RefreshFrameLook();
	}
	// QA sight: StepSight calls this every frame after the sight test: the world cues follow this frame's hold.
	UpdateWorldCues(CueDeltaSeconds);
	CueDeltaSeconds = 0.f;
}

float AWatcher::DistanceToPoint(const FVector& Point) const
{
	return static_cast<float>(FVector::Dist2D(GetActorLocation(), Point));
}

bool AWatcher::IsLive() const
{
	// A lured watcher still sees (its frame goes with it); a dark timer overrides everything.
	return DarkSecondsLeft <= 0.f && (State == EWatcherState::Watching || State == EWatcherState::Lured);
}

FString AWatcher::GetStateText() const
{
	if (State == EWatcherState::Blinded || State == EWatcherState::Lured || State == EWatcherState::Masked)
	{
		return FString::Printf(TEXT("%s %.1f"), TakeText::WatcherStateName(State), EffectSecondsLeft);
	}
	return TakeText::WatcherStateName(State);
}

FVector AWatcher::GetLabelPoint() const
{
	return GetActorLocation() + FVector(0.f, 0.f, LabelHeight);
}

void AWatcher::RefreshFrameLook()
{
	if (!FrameMesh)
	{
		return;
	}

	// QA sight (Marwan's notes 5, 18, 19): with the drawn overlays off, no frame, frame line or tally cube is drawn;
	// the world cues say it (UpdateWorldCues). The beam's laser is a thing in the world: it stays, solid (no dashes).
	const bool bWorldCues = !DrawsSightOverlays(this);
	if (bWorldCues && UsesFrameDisplay())
	{
		FrameMesh->SetVisibility(false);
		for (const TObjectPtr<UStaticMeshComponent>& LinePtr : FrameLines)
		{
			if (UStaticMeshComponent* Line = LinePtr.Get())
			{
				Line->SetVisibility(false);
			}
		}
		if (Tally)
		{
			Tally->SetVisibility(false);
		}
		return;
	}

	// Off and Dropped (the beam): no frame at all.
	bool bShowFrame = State != EWatcherState::Off && State != EWatcherState::Dropped;

	// Step 6 (Marwan, Nov 15): the frame display. Never: no frame. Planning: none at rest, faint and outline only while
	// a crew member plans a device on it, solid red while it holds an exposed Julian. Always: step 4's frames. The
	// beam opts out (UsesFrameDisplay). Only the look changes: the sight tests read none of this.
	const bool bDisplayed = UsesFrameDisplay();
	const bool bSeeing = IsLive() && bTally;
	bool bFaint = false;
	bool bSeenSolid = false;
	if (bShowFrame && bDisplayed && FrameMode == ETakeFrameDisplay::Never)
	{
		bShowFrame = false;
	}
	else if (bShowFrame && bDisplayed && FrameMode == ETakeFrameDisplay::Planning)
	{
		bSeenSolid = bSeeing;
		bFaint = !bSeeing && bPlanned && IsLive();
		bShowFrame = bSeenSolid || bFaint;
	}
	// A faint frame is its outline only (the lines), unless it has none.
	FrameMesh->SetVisibility(bShowFrame && !(bFaint && FrameLines.Num() > 0));

	FLinearColor Colour = TakeColors::RedDark;
	uint8 FrameStencil = TakeStyle::StencilRed;
	if (bFaint)
	{
		Colour = PlanningColour;
	}
	else if (bSeenSolid)
	{
		Colour = HoldingColour;	// the one thing the player must see: solid red until he is out
	}
	else if (IsLive() && bAlarmLook)
	{
		// Step 4 (§15.3): the alarm's frames are steady white and emissive, so they read in full colour.
		Colour = TakeColors::White * AlarmEmissive;
		Colour.A = 1.f;
		FrameStencil = TakeStyle::StencilWhite;
	}
	else if (IsLive())
	{
		Colour = bTally ? HoldingColour : TakeColors::Red;
	}
	if (bHovered)
	{
		const float HoverLift = FMath::Clamp(HoverBrightness, 0.f, 1.f);
		Colour = Colour + (TakeColors::White - Colour) * HoverLift;
	}
	// The material: the solid signal, or the faint one (each switch re-sets the pattern: a new instance forgets it).
	auto Paint = [&](UStaticMeshComponent* Shape, float Pattern)
	{
		if (bFaint && FTakeShapes::GetFaintMaterial())
		{
			FTakeShapes::SetFaint(Shape, FrameStencil, PlanningOpacity);
		}
		else
		{
			FTakeShapes::SetSignal(Shape, FrameStencil);
		}
		FTakeShapes::SetPattern(Shape, bWorldCues ? TakeStyle::PatternSolid : Pattern);
		return FTakeShapes::SetColour(Shape, Colour);
	};
	FrameMID = Paint(FrameMesh, FramePattern);

	// The frame lines go with the frame: same visibility, same colour.
	for (const TObjectPtr<UStaticMeshComponent>& LinePtr : FrameLines)
	{
		if (UStaticMeshComponent* Line = LinePtr.Get())
		{
			Line->SetVisibility(bShowFrame);
			if (bShowFrame)
			{
				Paint(Line, TakeStyle::PatternDash);
			}
		}
	}

	// The tally: hidden before arrival; lit, steady, while the arrival warning is up or while it holds Julian.
	if (Tally)
	{
		Tally->SetVisibility(HasArrived() && !bWorldCues);	// QA sight: the beam's posts carry lamps instead
		const FLinearColor TallyColour = (bTally || IsArrivalBlinking()) ? HoldingColour : TakeColors::RedDark;
		TallyMID = FTakeShapes::SetColour(Tally, TallyColour);
	}
}

void AWatcher::ApplyFrameScale()
{
	// The drawn frame must equal the tested frame: FramePivot always stays at scale 1 (hover is a brightness change
	// in RefreshFrameLook).
	if (FramePivot)
	{
		FramePivot->SetRelativeScale3D(FVector::OneVector);
	}
}

FVector AWatcher::GetAimPoint() const
{
	// BODY: (d) (the beam overrides it)
	return GetActorLocation() + FVector(0.f, 0.f, AimHeight);
}

// ---------------------------------------------------------------------------------------------------------------------
// Step 2: arrivals, line of sight, frame lines (W)
// ---------------------------------------------------------------------------------------------------------------------

void AWatcher::ScheduleArrival(int32 Bar)
{
	ArriveBar = Bar;
	TAKE_LOG(TEXT("%s arrives on bar %d"), *Label, Bar);
}

void AWatcher::StepArrival(float DeltaSeconds, const UBarClockComponent* Clock)
{
	const bool bWasArrived = HasArrived();
	bArrived = ArriveOn.IsNone() || (ArriveBar > 0 && Clock && Clock->GetBar() >= ArriveBar);

	if (bArrived && !bWasArrived)
	{
		// The frame and lines show this frame (StepWatcher syncs the state and the look right after this).
		const float SecondsPerBar = Clock ? Clock->GetSecondsPerBar() : 0.f;
		ArrivalBlinkSecondsLeft = static_cast<float>(FMath::Max(0, ArrivalBlinkBars)) * SecondsPerBar;
		TAKE_LOG(TEXT("%s ARRIVES"), *Label);

		// Step 3: a hidden walker steps on set, and says his line, this frame.
		if (bHideUntilArrived)
		{
			SetActorHiddenInGame(false);
		}
		if (!ArriveLine.IsEmpty())
		{
			if (ATakeDirector* Director = ATakeDirector::Get(this))
			{
				Director->Flash(ArriveLine, ETakeFlashKind::Line, 3.f);
			}
		}
	}
	else if (ArrivalBlinkSecondsLeft > 0.f)
	{
		ArrivalBlinkSecondsLeft = FMath::Max(ArrivalBlinkSecondsLeft - DeltaSeconds, 0.f);
	}
}

FVector AWatcher::GetEyePoint() const
{
	return GetActorLocation() + FVector(0.f, 0.f, EyeHeight);
}

bool AWatcher::HasLineOfSight(const AJulian* Julian) const
{
	if (!bUseLineOfSight)
	{
		return true;
	}
	if (!Julian)
	{
		return false;
	}
	const ATakeDirector* Director = ATakeDirector::Get(this);
	if (!Director)
	{
		return true;
	}

	// Seen if either his chest or his head is in clear view of the eye.
	const FVector Eye = GetEyePoint();
	const FVector Feet = Julian->GetFeetLocation();
	if (!Director->IsSightBlocked(Eye, Feet + FVector(0.f, 0.f, TakeWatcherLocal::ChestHeight)))
	{
		return true;
	}
	return !Director->IsSightBlocked(Eye, Feet + FVector(0.f, 0.f, TakeWatcherLocal::HeadHeight));
}

void AWatcher::BuildFrameLines()
{
	for (const TObjectPtr<UStaticMeshComponent>& LinePtr : FrameLines)
	{
		if (UStaticMeshComponent* Line = LinePtr.Get())
		{
			Line->DestroyComponent();
		}
	}
	FrameLines.Reset();

	TArray<FVector> Corners;
	GetFrameFarCorners(Corners);
	if (!FramePivot || Corners.Num() == 0)
	{
		return;
	}

	USceneComponent* Pivot = FramePivot.Get();
	const FVector EyeLocal = Pivot->GetComponentTransform().InverseTransformPosition(GetEyePoint());
	const double Thickness = static_cast<double>(FMath::Max(FrameLineThickness, 0.1f));

	// One line from the eye to each far corner.
	for (const FVector& Corner : Corners)
	{
		if (UStaticMeshComponent* Line = TakeWatcherLocal::AddFrameLine(this, Pivot, EyeLocal, Corner, Thickness))
		{
			FrameLines.Add(Line);
		}
	}

	// One between each pair of neighbouring far corners (a closed loop from three corners up).
	const int32 NumCorners = Corners.Num();
	const int32 NumEdges = NumCorners >= 3 ? NumCorners : NumCorners - 1;
	for (int32 i = 0; i < NumEdges; ++i)
	{
		if (UStaticMeshComponent* Line = TakeWatcherLocal::AddFrameLine(this, Pivot, Corners[i], Corners[(i + 1) % NumCorners], Thickness))
		{
			FrameLines.Add(Line);
		}
	}

	// Colour and visibility with the frame, this frame.
	RefreshFrameLook();
}

void AWatcher::MarkArrivedIfDue(const UBarClockComponent* Clock)
{
	if (!ArriveOn.IsNone() && ArriveBar > 0 && Clock && Clock->GetBar() >= ArriveBar)
	{
		bArrived = true;
		ArrivalBlinkSecondsLeft = 0.f;
		if (bHideUntilArrived)
		{
			SetActorHiddenInGame(false);
		}
		SyncState();
		RefreshFrameLook();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Step 3: the alarm look (W)
// ---------------------------------------------------------------------------------------------------------------------

void AWatcher::SetAlarmLook(bool bOn)
{
	bAlarmLook = bOn;
	RefreshFrameLook();
}
