// Owner: (d). Constructor by the header pass; bodies by (d).

#include "PatrolGuard.h"

#include "BarClockComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Julian.h"
#include "PatrolPoint.h"
#include "TakeShapes.h"

namespace TakeSliceDGuard
{
	/** The wedge's thickness (cone scale on its local X, which the pitch turns to world Z): 2 cm. */
	constexpr float WedgeThicknessScale = 0.02f;

	/** Yaw Elapsed seconds into a turn from FromYaw to ToYaw that takes TurnSecs (0 = snap). */
	float TurnOver(float FromYaw, float ToYaw, float Elapsed, float TurnSecs)
	{
		if (TurnSecs <= 0.f || Elapsed >= TurnSecs)
		{
			return ToYaw;
		}
		const float Alpha = FMath::Clamp(Elapsed / TurnSecs, 0.f, 1.f);
		return FromYaw + FMath::FindDeltaAngleDegrees(FromYaw, ToYaw) * Alpha;
	}

	/** Turns Yaw toward TargetYaw by at most MaxStep degrees. */
	float TurnToward(float Yaw, float TargetYaw, float MaxStep)
	{
		const float Delta = FMath::FindDeltaAngleDegrees(Yaw, TargetYaw);
		if (FMath::Abs(Delta) <= MaxStep)
		{
			return TargetYaw;
		}
		return Yaw + FMath::Sign(Delta) * MaxStep;
	}

	/** Free turns (lure, return) run at half a turn per TurnSecs. */
	float MaxTurnStep(float TurnSecs, float DeltaSeconds)
	{
		return TurnSecs > 0.f ? 180.f / TurnSecs * DeltaSeconds : 360.f;
	}
}

APatrolGuard::APatrolGuard()
{
	Label = TEXT("GUARD");

	// Grey stand-in, 180 cm, feet at the actor location; a cap block in front shows his facing (+X).
	Body = FTakeShapes::CreateShape(this, TEXT("Body"), ETakeShape::Cylinder, Root, FVector(0.f, 0.f, 90.f), FVector(0.6f, 0.6f, 1.8f));
	Cap = FTakeShapes::CreateShape(this, TEXT("Cap"), ETakeShape::Cube, Root, FVector(25.f, 0.f, 165.f), FVector(0.25f, 0.4f, 0.12f));

	// The frame: the cone pitched +90 so its tip points back at him (-X) and its base lies at FrameRange,
	// squashed flat. Local Z (cone height) -> world X length; local Y -> width; local X -> thickness.
	// OnConstruction recomputes this from FrameRange / FrameHalfAngle.
	const float Width = 2.f * FrameRange * FMath::Tan(FMath::DegreesToRadians(FrameHalfAngle));
	FrameMesh->SetStaticMesh(FTakeShapes::GetMesh(ETakeShape::Cone));
	FrameMesh->SetRelativeLocation(FVector(FrameRange * 0.5f, 0.f, 0.f));
	FrameMesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	FrameMesh->SetRelativeScale3D(FVector(0.02f, Width / 100.f, FrameRange / 100.f));

	Tally->SetRelativeLocation(FVector(0.f, 0.f, 200.f));
}

void APatrolGuard::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LayoutGuard();
}

void APatrolGuard::BeginPlay()
{
	Super::BeginPlay();
	LayoutGuard();
	LurePhase = EGuardLurePhase::None;
	LureTime = 0.f;
	RefreshFrameLook();
}

void APatrolGuard::LayoutGuard()
{
	// Cone mesh: 100 tall along local +Z (tip at +50), base radius 50. Pitch +90 maps local +Z to -X, local Y to Y,
	// local X to Z. Placed at FrameRange/2 ahead of FramePivot (2 cm above his feet), the tip sits at his feet and
	// the base at FrameRange; the base's half-width is FrameRange * tan(FrameHalfAngle).
	const float Range = FMath::Max(FrameRange, 1.f);
	const float HalfAngle = FMath::Clamp(FrameHalfAngle, 1.f, 85.f);
	const float Width = 2.f * Range * FMath::Tan(FMath::DegreesToRadians(HalfAngle));

	FramePivot->SetRelativeLocation(FVector(0.f, 0.f, 2.f));
	FrameMesh->SetStaticMesh(FTakeShapes::GetMesh(ETakeShape::Cone));
	FrameMesh->SetRelativeLocation(FVector(Range * 0.5f, 0.f, 0.f));
	FrameMesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	FrameMesh->SetRelativeScale3D(FVector(TakeSliceDGuard::WedgeThicknessScale, Width / 100.f, Range / 100.f));

	FTakeShapes::SetColour(FrameMesh, TakeColors::Red);
	FTakeShapes::SetColour(Body, TakeColors::Grey);
	FTakeShapes::SetColour(Cap, TakeColors::DarkGrey);
	FTakeShapes::SetColour(Tally, TakeColors::DarkGrey);
}

void APatrolGuard::StepWatcher(float DeltaSeconds, UBarClockComponent* Clock)
{
	// The base counts the effect down (a lure ending sets Watching) and pulses the frame.
	const bool bWasLured = State == EWatcherState::Lured;
	Super::StepWatcher(DeltaSeconds, Clock);
	if (bWasLured && State != EWatcherState::Lured
		&& (LurePhase == EGuardLurePhase::Going || LurePhase == EGuardLurePhase::Looking))
	{
		LurePhase = EGuardLurePhase::Returning;
	}

	FVector PatrolLocation = FVector::ZeroVector;
	float PatrolYaw = 0.f;
	EvaluatePatrol(Clock, PatrolLocation, PatrolYaw);

	if (LurePhase == EGuardLurePhase::None)
	{
		SetActorLocationAndRotation(PatrolLocation, FRotator(0.f, PatrolYaw, 0.f));
		return;
	}

	// The lure (go, look, return) moves him frame by frame; a reload clears it, so the patrol stays exact.
	LureTime += DeltaSeconds;
	const float WalkStep = FMath::Max(WalkSpeed, 1.f) * DeltaSeconds;
	const float TurnStep = TakeSliceDGuard::MaxTurnStep(TurnSeconds, DeltaSeconds);
	FVector Here = GetActorLocation();
	float Yaw = static_cast<float>(GetActorRotation().Yaw);

	if (LurePhase == EGuardLurePhase::Going)
	{
		// He stops LureStandOff short of the point, on the line from where the lure found him.
		const FVector ToPoint(LurePoint.X - LureStart.X, LurePoint.Y - LureStart.Y, 0.0);
		const double PointDist = ToPoint.Size();
		FVector Stand = LureStart;
		if (PointDist > LureStandOff)
		{
			Stand = LureStart + ToPoint / PointDist * (PointDist - LureStandOff);
		}

		const FVector ToStand(Stand.X - Here.X, Stand.Y - Here.Y, 0.0);
		const double Remaining = ToStand.Size();
		if (Remaining <= WalkStep)
		{
			Here.X = Stand.X;
			Here.Y = Stand.Y;
			LurePhase = EGuardLurePhase::Looking;
		}
		else
		{
			Here += ToStand / Remaining * WalkStep;
		}
	}

	if (LurePhase == EGuardLurePhase::Going || LurePhase == EGuardLurePhase::Looking)
	{
		// Facing the lure point the whole time: his frame looks at it.
		const FVector Look(LurePoint.X - Here.X, LurePoint.Y - Here.Y, 0.0);
		if (!Look.IsNearlyZero())
		{
			Yaw = static_cast<float>(Look.Rotation().Yaw);
		}
	}
	else if (LurePhase == EGuardLurePhase::Returning)
	{
		// Back to where the patrol says he should be now, then turn to the patrol's facing.
		const FVector ToPatrol(PatrolLocation.X - Here.X, PatrolLocation.Y - Here.Y, 0.0);
		const double Remaining = ToPatrol.Size();
		if (Remaining <= WalkStep)
		{
			Here = PatrolLocation;
			Yaw = TakeSliceDGuard::TurnToward(Yaw, PatrolYaw, TurnStep);
			if (FMath::Abs(FMath::FindDeltaAngleDegrees(Yaw, PatrolYaw)) < 0.5f)
			{
				Yaw = PatrolYaw;
				LurePhase = EGuardLurePhase::None;
			}
		}
		else
		{
			Here += ToPatrol / Remaining * WalkStep;
			Here.Z = PatrolLocation.Z;
			Yaw = TakeSliceDGuard::TurnToward(Yaw, static_cast<float>(ToPatrol.Rotation().Yaw), TurnStep);
		}
	}

	SetActorLocationAndRotation(Here, FRotator(0.f, Yaw, 0.f));
}

bool APatrolGuard::HoldsJulian(const AJulian* Julian) const
{
	if (!Julian || !IsLive())
	{
		return false;
	}

	// Exactly the drawn wedge: a flat triangle with its tip at his feet, FrameRange deep along his facing and
	// FrameHalfAngle either side. Walls ignored in step 1.
	const float Range = FMath::Max(FrameRange, 1.f);
	const float HalfAngle = FMath::Clamp(FrameHalfAngle, 1.f, 85.f);

	const FVector Feet = Julian->GetFeetLocation();
	const FVector Here = GetActorLocation();
	const FVector Offset(Feet.X - Here.X, Feet.Y - Here.Y, 0.0);
	const float Yaw = FMath::DegreesToRadians(static_cast<float>(GetActorRotation().Yaw));
	const double Ahead = Offset.X * FMath::Cos(Yaw) + Offset.Y * FMath::Sin(Yaw);
	const double Side = -Offset.X * FMath::Sin(Yaw) + Offset.Y * FMath::Cos(Yaw);

	if (Ahead <= 0.0 || Ahead > Range)
	{
		return false;
	}
	return FMath::Abs(Side) <= Ahead * FMath::Tan(FMath::DegreesToRadians(HalfAngle));
}

bool APatrolGuard::ApplyEffect(EDeviceEffect Effect, float Duration, const FVector& Point)
{
	if (Effect == EDeviceEffect::Mask)
	{
		return false;	// step 1: the guard ignores a matte
	}

	if (Effect == EDeviceEffect::Lure)
	{
		State = EWatcherState::Lured;
		EffectSecondsLeft = FMath::Max(Duration, UE_KINDA_SMALL_NUMBER);
		LurePoint = Point;
		LureStart = GetActorLocation();
		LurePhase = EGuardLurePhase::Going;
		LureTime = 0.f;

		// He turns to the point on this frame; StepWatcher (step 4, same frame) starts the walk.
		const FVector Look(Point.X - LureStart.X, Point.Y - LureStart.Y, 0.0);
		if (!Look.IsNearlyZero())
		{
			SetActorRotation(FRotator(0.f, Look.Rotation().Yaw, 0.f));
		}
		RefreshFrameLook();
		TAKE_LOG(TEXT("%s LURED %.1f s -> (%.0f,%.0f)"), *Label, Duration, Point.X, Point.Y);
		return true;
	}

	// Blind: the base darkens the frame. A blind ends a lure: he walks back to his patrol.
	const bool bCaught = Super::ApplyEffect(Effect, Duration, Point);
	if (bCaught && (LurePhase == EGuardLurePhase::Going || LurePhase == EGuardLurePhase::Looking))
	{
		LurePhase = EGuardLurePhase::Returning;
	}
	return bCaught;
}

FString APatrolGuard::GetStateText() const
{
	if (State == EWatcherState::Watching && LurePhase == EGuardLurePhase::Returning)
	{
		return TEXT("RETURNING");
	}
	return Super::GetStateText();
}

void APatrolGuard::EvaluatePatrol(const UBarClockComponent* Clock, FVector& OutLocation, float& OutYaw) const
{
	OutLocation = HomeLocation;
	OutYaw = HomeYaw;

	TArray<const APatrolPoint*, TInlineAllocator<16>> Points;
	int32 CycleBars = 0;
	for (const TObjectPtr<APatrolPoint>& PointPtr : PatrolPoints)
	{
		if (const APatrolPoint* PatrolPoint = PointPtr.Get())
		{
			Points.Add(PatrolPoint);
			CycleBars += FMath::Max(1, PatrolPoint->Bars);
		}
	}
	if (!Clock || Points.Num() == 0 || CycleBars <= 0)
	{
		return;
	}

	// Bars since bar 1 beat 1; the loop repeats every CycleBars. Each point owns a slot of its Bars, starting on a
	// bar line: he walks in from the previous point (from his placed pose on the very first slot), turns to the
	// point's yaw, and holds it until the slot ends.
	const double SecondsPerBar = FMath::Max(static_cast<double>(Clock->GetSecondsPerBar()), 0.001);
	const double BarsElapsed = FMath::Max(0.0, Clock->GetTotalBeats() / static_cast<double>(FMath::Max(1, Clock->BeatsPerBar)));
	const int64 Cycle = static_cast<int64>(BarsElapsed / static_cast<double>(CycleBars));
	const double CyclePhase = BarsElapsed - static_cast<double>(Cycle) * static_cast<double>(CycleBars);

	int32 Index = 0;
	double SlotStart = 0.0;
	for (int32 i = 0; i < Points.Num(); ++i)
	{
		Index = i;
		const double SlotBars = static_cast<double>(FMath::Max(1, Points[i]->Bars));
		if (CyclePhase < SlotStart + SlotBars || i == Points.Num() - 1)
		{
			break;
		}
		SlotStart += SlotBars;
	}

	const double SlotSeconds = static_cast<double>(FMath::Max(1, Points[Index]->Bars)) * SecondsPerBar;
	const double IntoSlot = FMath::Clamp((CyclePhase - SlotStart) * SecondsPerBar, 0.0, SlotSeconds);

	FVector FromLocation = HomeLocation;
	float FromYaw = HomeYaw;
	if (Index > 0 || Cycle > 0)
	{
		const APatrolPoint* Prev = Points[(Index + Points.Num() - 1) % Points.Num()];
		FromLocation = Prev->GetActorLocation();
		FromYaw = static_cast<float>(Prev->GetActorRotation().Yaw);
	}
	const FVector ToLocation = Points[Index]->GetActorLocation();
	const float ToYaw = static_cast<float>(Points[Index]->GetActorRotation().Yaw);

	const FVector Walk(ToLocation.X - FromLocation.X, ToLocation.Y - FromLocation.Y, 0.0);
	const double WalkDist = Walk.Size();
	double WalkSeconds = 0.0;
	float WalkYaw = FromYaw;
	if (WalkDist > 1.0)
	{
		WalkSeconds = FMath::Min(WalkDist / static_cast<double>(FMath::Max(WalkSpeed, 1.f)), SlotSeconds);
		WalkYaw = static_cast<float>(Walk.Rotation().Yaw);
	}

	if (IntoSlot < WalkSeconds)
	{
		// Walking: turns to the walk direction over TurnSeconds as he sets off.
		const double Alpha = IntoSlot / WalkSeconds;
		OutLocation = FromLocation + (ToLocation - FromLocation) * Alpha;
		OutYaw = TakeSliceDGuard::TurnOver(FromYaw, WalkYaw, static_cast<float>(IntoSlot), TurnSeconds);
	}
	else
	{
		// On the point: turns to its yaw over TurnSeconds, then holds.
		OutLocation = ToLocation;
		const float StartYaw = WalkSeconds > 0.0 ? WalkYaw : FromYaw;
		OutYaw = TakeSliceDGuard::TurnOver(StartYaw, ToYaw, static_cast<float>(IntoSlot - WalkSeconds), TurnSeconds);
	}
}
