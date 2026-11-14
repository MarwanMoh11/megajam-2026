// Owner: W. Constructor by the header pass; bodies by W.

#include "DroneWatcher.h"

#include "BarClockComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Julian.h"
#include "PatrolPoint.h"
#include "TakeCharacter.h"
#include "TakeDirector.h"
#include "TakeShapes.h"

namespace TakeDroneLocal
{
	/** The disc's thickness (cylinder Z scale): 1 cm. */
	constexpr float DiscThicknessScale = 0.01f;

	/** The tally hangs this far under the body, cm. */
	constexpr float TallyBelowBody = 25.f;

	/** The label sits at least this far over the body, cm. */
	constexpr float LabelAboveBody = 80.f;

	/** The click volume round the body (half extents), cm. */
	const FVector BodyClickExtent(90.f, 90.f, 60.f);

	/** The path's live points (null entries skipped) and each leg's bars. Returns the cycle's length in bars. */
	int32 GatherPath(const TArray<TObjectPtr<APatrolPoint>>& PathPoints, TArray<FVector, TInlineAllocator<16>>& OutPoints,
		TArray<int32, TInlineAllocator<16>>& OutLegBars)
	{
		int32 CycleBars = 0;
		for (const TObjectPtr<APatrolPoint>& PointPtr : PathPoints)
		{
			if (const APatrolPoint* PathPoint = PointPtr.Get())
			{
				const int32 LegBars = FMath::Max(1, PathPoint->Bars);
				OutPoints.Add(PathPoint->GetActorLocation());
				OutLegBars.Add(LegBars);
				CycleBars += LegBars;
			}
		}
		return CycleBars;
	}
}

ADroneWatcher::ADroneWatcher()
{
	Label = TEXT("DRONE");
	Instrument = EWatcherInstrument::Trumpet;
	LabelHeight = 600.f;

	// The cone's footprint: a flat disc on the floor (FrameMesh is the base's cube: re-shaped here).
	FTakeShapes::SetupShape(FrameMesh, ETakeShape::Cylinder, false);
	FrameMesh->SetRelativeScale3D(FVector(6.f, 6.f, 0.01f));

	Body = FTakeShapes::CreateShape(this, TEXT("Body"), ETakeShape::Cube, Root, FVector(0.f, 0.f, 520.f),
		FVector(0.9f, 0.9f, 0.18f), FRotator::ZeroRotator, false);
}

void ADroneWatcher::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LayoutDrone();
}

void ADroneWatcher::BeginPlay()
{
	Super::BeginPlay();
	LayoutDrone();

	// It starts over its first path point (Off until it arrives).
	SetActorLocation(EvaluatePath(nullptr));

	BuildFrameLines();
	SyncState();
	RefreshFrameLook();
}

void ADroneWatcher::StepWatcher(float DeltaSeconds, UBarClockComponent* Clock)
{
	// The flight is a pure function of the clock (it hovers over the first point until its arrival bar). Moved
	// before the base step, so the cone is tested this frame where it is drawn.
	if (const AActor* Target = ChaseTarget.Get())
	{
		// Step 3, the lock-on: no path; the floor point closes on the target's feet (2D) at ChaseSpeed, takes the
		// feet's height, and then stays over them. Blinded, it still follows.
		const ATakeCharacter* Character = Cast<const ATakeCharacter>(Target);
		const FVector Feet = Character ? Character->GetFeetLocation() : Target->GetActorLocation();
		FVector Here = GetActorLocation();
		const FVector ToFeet(Feet.X - Here.X, Feet.Y - Here.Y, 0.0);
		const double Remaining = ToFeet.Size();
		const double Step = static_cast<double>(FMath::Max(ChaseSpeed, 0.f)) * static_cast<double>(FMath::Max(DeltaSeconds, 0.f));
		if (Remaining <= Step || Remaining < KINDA_SMALL_NUMBER)
		{
			Here.X = Feet.X;
			Here.Y = Feet.Y;
		}
		else
		{
			Here += ToFeet / Remaining * Step;
		}
		Here.Z = Feet.Z;
		SetActorLocation(Here);
	}
	else
	{
		SetActorLocation(EvaluatePath(Clock));
	}
	Super::StepWatcher(DeltaSeconds, Clock);
}

bool ADroneWatcher::HoldsJulian(const AJulian* Julian) const
{
	if (!Julian || !IsLive())
	{
		return false;
	}

	const FVector Feet = Julian->GetFeetLocation();
	const FVector Here = GetActorLocation();
	if (FVector::Dist2D(Feet, Here) > static_cast<double>(FMath::Max(ConeRadius, 1.f)))
	{
		return false;
	}
	if (FMath::Abs(Feet.Z - Here.Z) > static_cast<double>(ConeFloorTolerance))
	{
		return false;	// on the floor above or below the cone's
	}
	return HasLineOfSight(Julian);
}

bool ADroneWatcher::ApplyEffect(EDeviceEffect Effect, float Duration, const FVector& Point)
{
	// Blind: dark for the duration, it keeps flying (the base). Lure and Mask: nothing.
	if (Effect != EDeviceEffect::Blind)
	{
		return false;
	}
	return Super::ApplyEffect(Effect, Duration, Point);
}

bool ADroneWatcher::ChangesAtBar(int32 Bar, const UBarClockComponent* Clock) const
{
	if (!HasArrived() || IsChasing())
	{
		return false;
	}

	TArray<FVector, TInlineAllocator<16>> Points;
	TArray<int32, TInlineAllocator<16>> LegBars;
	const int32 CycleBars = TakeDroneLocal::GatherPath(PathPoints, Points, LegBars);
	if (Points.Num() < 2 || CycleBars <= 0)
	{
		return false;
	}

	// The path's time starts on its arrival bar (bar 1 when it is there from the start); that first leg is the
	// arrival's own cue, so only later leg starts count.
	const int32 StartBar = ArriveOn.IsNone() ? 1 : ArriveBar;
	const int32 BarsSince = Bar - StartBar;
	if (BarsSince <= 0)
	{
		return false;
	}

	const int32 Phase = BarsSince % CycleBars;
	int32 LegStart = 0;
	for (const int32 Bars : LegBars)
	{
		if (Phase == LegStart)
		{
			return true;
		}
		LegStart += Bars;
	}
	return false;
}

FVector ADroneWatcher::GetEyePoint() const
{
	return GetActorLocation() + FVector(0.f, 0.f, FlyHeight);
}

FVector ADroneWatcher::GetAimPoint() const
{
	return GetActorLocation() + FVector(0.f, 0.f, FlyHeight);
}

FVector ADroneWatcher::GetLabelPoint() const
{
	return GetActorLocation() + FVector(0.f, 0.f, FMath::Max(LabelHeight, FlyHeight + TakeDroneLocal::LabelAboveBody));
}

void ADroneWatcher::GetFrameFarCorners(TArray<FVector>& OutLocal) const
{
	// Four points on the disc's edge, in order round it, on the floor (FramePivot's space).
	const float Radius = FMath::Max(ConeRadius, 1.f);
	OutLocal.Reset();
	OutLocal.Add(FVector(Radius, 0.f, 0.f));
	OutLocal.Add(FVector(0.f, Radius, 0.f));
	OutLocal.Add(FVector(-Radius, 0.f, 0.f));
	OutLocal.Add(FVector(0.f, -Radius, 0.f));
}

FVector ADroneWatcher::EvaluatePath(const UBarClockComponent* Clock) const
{
	TArray<FVector, TInlineAllocator<16>> Points;
	TArray<int32, TInlineAllocator<16>> LegBars;
	const int32 CycleBars = TakeDroneLocal::GatherPath(PathPoints, Points, LegBars);
	if (Points.Num() == 0)
	{
		return HomeLocation;
	}
	if (!Clock || Points.Num() < 2 || CycleBars <= 0)
	{
		return Points[0];
	}

	// Arrived by the clock alone (so the path stays a pure function of it; the base's flag follows this frame).
	const bool bArrivedNow = ArriveOn.IsNone() || (ArriveBar > 0 && Clock->GetBar() >= ArriveBar);
	if (!bArrivedNow)
	{
		return Points[0];
	}

	const int32 StartBar = ArriveOn.IsNone() ? 1 : ArriveBar;
	const double BarsNow = Clock->GetTotalBeats() / static_cast<double>(FMath::Max(1, Clock->BeatsPerBar));
	const double BarsSince = FMath::Max(0.0, BarsNow - static_cast<double>(StartBar - 1));
	const double CyclePhase = FMath::Fmod(BarsSince, static_cast<double>(CycleBars));

	double LegStart = 0.0;
	for (int32 i = 0; i < Points.Num(); ++i)
	{
		const double Bars = static_cast<double>(LegBars[i]);
		if (CyclePhase < LegStart + Bars || i == Points.Num() - 1)
		{
			const double Alpha = FMath::Clamp((CyclePhase - LegStart) / Bars, 0.0, 1.0);
			const FVector& From = Points[i];
			const FVector& To = Points[(i + 1) % Points.Num()];
			return From + (To - From) * Alpha;
		}
		LegStart += Bars;
	}
	return Points[0];
}

void ADroneWatcher::LayoutDrone()
{
	const float Radius = FMath::Max(ConeRadius, 1.f);
	const float Height = FMath::Max(FlyHeight, 1.f);

	// The disc: the cylinder (100 across, pivot at its centre) flattened on FramePivot (2 cm above the floor).
	FramePivot->SetRelativeLocation(FVector(0.f, 0.f, 2.f));
	FrameMesh->SetRelativeLocation(FVector::ZeroVector);
	FrameMesh->SetRelativeRotation(FRotator::ZeroRotator);
	FrameMesh->SetRelativeScale3D(FVector(Radius * 2.f / 100.f, Radius * 2.f / 100.f, TakeDroneLocal::DiscThicknessScale));

	// The body at FlyHeight, the tally under it, the click volume round it.
	Body->SetRelativeLocation(FVector(0.f, 0.f, Height));
	Tally->SetRelativeLocation(FVector(0.f, 0.f, Height - TakeDroneLocal::TallyBelowBody));
	ClickVolume->SetRelativeLocation(FVector(0.f, 0.f, Height));
	ClickVolume->SetBoxExtent(TakeDroneLocal::BodyClickExtent);

	FTakeShapes::SetColour(FrameMesh, TakeColors::Red);
	FTakeShapes::SetColour(Body, BodyTint);
	FTakeShapes::SetColour(Tally, TakeColors::RedDark);
}

// ---------------------------------------------------------------------------------------------------------------------
// Step 3: the lock-on (W)
// ---------------------------------------------------------------------------------------------------------------------

void ADroneWatcher::SetChaseTarget(const AActor* Target)
{
	const bool bNewTarget = Target != nullptr && ChaseTarget.Get() != Target;
	const bool bWasChasing = ChaseTarget.IsValid();
	ChaseTarget = Target;

	if (bNewTarget)
	{
		TAKE_LOG(TEXT("%s locks on"), *Label);
		if (!ChaseLine.IsEmpty())
		{
			if (ATakeDirector* Director = ATakeDirector::Get(this))
			{
				Director->Flash(ChaseLine, ETakeFlashKind::Line, 3.f);
			}
		}
	}
	else if (Target == nullptr && bWasChasing)
	{
		TAKE_LOG(TEXT("%s back to its loop"), *Label);
	}
}
