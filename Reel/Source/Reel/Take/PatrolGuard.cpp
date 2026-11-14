// Owner: (d). Constructor by the header pass; bodies by (d).

#include "PatrolGuard.h"

#include "BarClockComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Julian.h"
#include "PatrolPoint.h"
#include "TakeDirector.h"
#include "TakeShapes.h"

namespace TakeSliceDGuard
{
	/** A held point's ArriveLine flash. */
	constexpr float ArriveLineSeconds = 3.f;

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
	BuildFrameLines();
	LurePhase = EGuardLurePhase::None;
	LureTime = 0.f;
	LureSecondsLeft = 0.f;
	LastPatrolIndex = -1;
	SyncState();
	RefreshFrameLook();
}

void APatrolGuard::SyncState()
{
	Super::SyncState();
	if (!HasArrived())
	{
		return;	// not arrived: Off first (the base)
	}
	if (DarkSecondsLeft <= 0.f && LureSecondsLeft > 0.f)
	{
		State = EWatcherState::Lured;
		EffectSecondsLeft = LureSecondsLeft;
	}
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
	FTakeShapes::SetColour(Body, BodyTint);	// §15.10: heavies are the darkest thing on set
	FTakeShapes::SetColour(Cap, TakeColors::DarkGrey);
	FTakeShapes::SetColour(Tally, TakeColors::DarkGrey);
}

void APatrolGuard::StepWatcher(float DeltaSeconds, UBarClockComponent* Clock)
{
	// The lure's own timer (a blind's dark timer runs alongside it in the base): at 0 he walks back.
	if (LureSecondsLeft > 0.f)
	{
		LureSecondsLeft = FMath::Max(LureSecondsLeft - DeltaSeconds, 0.f);
		if (LureSecondsLeft <= 0.f
			&& (LurePhase == EGuardLurePhase::Going || LurePhase == EGuardLurePhase::Looking))
		{
			LurePhase = EGuardLurePhase::Returning;
		}
	}

	// The base steps the arrival, counts the dark timer down, derives the state (off, dark, then LURED) and
	// refreshes the frame's look.
	Super::StepWatcher(DeltaSeconds, Clock);

	// Not arrived yet: he stays where he was placed, Off (a guard who patrols from his arrival waits at his first
	// point, step 3).
	if (!HasArrived())
	{
		if (bPatrolFromArrival && !ArriveOn.IsNone())
		{
			FVector WaitLocation = FVector::ZeroVector;
			float WaitYaw = 0.f;
			EvaluatePatrol(Clock, WaitLocation, WaitYaw);
			SetActorLocationAndRotation(WaitLocation, FRotator(0.f, WaitYaw, 0.f));
		}
		return;
	}

	// The held point (once arrived): a change flashes the new point's ArriveLine this frame.
	if (Clock)
	{
		const int32 HeldIndex = GetPatrolIndexAtBar(Clock->GetBar());
		if (HeldIndex != LastPatrolIndex)
		{
			// The first point seen after BeginPlay (a take's start, a checkpoint restore) is not news: no line.
			const bool bFirstSeen = LastPatrolIndex < 0;
			LastPatrolIndex = HeldIndex;
			const APatrolPoint* Held = PatrolPoints.IsValidIndex(HeldIndex) ? PatrolPoints[HeldIndex].Get() : nullptr;
			if (Held)
			{
				TAKE_LOG(TEXT("%s holds point %d (%s)"), *Label, HeldIndex,
					Held->StateText.IsEmpty() ? TEXT("WATCHING") : *Held->StateText);
				if (!bFirstSeen && !Held->ArriveLine.IsEmpty())
				{
					if (ATakeDirector* Director = ATakeDirector::Get(this))
					{
						Director->Flash(Held->ArriveLine, ETakeFlashKind::Line, TakeSliceDGuard::ArriveLineSeconds);
					}
				}
			}
		}
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
	if (FMath::Abs(Side) > Ahead * FMath::Tan(FMath::DegreesToRadians(HalfAngle)))
	{
		return false;
	}
	// Step 2: walls, floors and a dropped banner hide him (rule 6).
	return HasLineOfSight(Julian);
}

bool APatrolGuard::ApplyEffect(EDeviceEffect Effect, float Duration, const FVector& Point)
{
	if (Effect == EDeviceEffect::Mask || !HasArrived())
	{
		return false;	// the guard ignores a matte; nothing lands on a guard who has not arrived
	}

	if (Effect == EDeviceEffect::Lure)
	{
		// Stacks with a blind: the walk starts now; the frame stays dark until the blind's own time ends.
		// bTurnsOnly (Vance): he only turns to face the point for the duration, then back to the patrol's yaw.
		LureSecondsLeft = FMath::Max(Duration, UE_KINDA_SMALL_NUMBER);
		LurePoint = Point;
		LureStart = GetActorLocation();
		LurePhase = bTurnsOnly ? EGuardLurePhase::Looking : EGuardLurePhase::Going;
		LureTime = 0.f;

		// He turns to the point on this frame; StepWatcher (step 4, same frame) starts the walk.
		const FVector Look(Point.X - LureStart.X, Point.Y - LureStart.Y, 0.0);
		if (!Look.IsNearlyZero())
		{
			SetActorRotation(FRotator(0.f, Look.Rotation().Yaw, 0.f));
		}
		SyncState();
		RefreshFrameLook();
		TAKE_LOG(TEXT("%s LURED %.1f s -> (%.0f,%.0f)"), *Label, Duration, Point.X, Point.Y);
		return true;
	}

	// Blind: the base darkens the frame (max of the dark time left and Duration). A lure in progress carries on.
	return Super::ApplyEffect(Effect, Duration, Point);
}

FString APatrolGuard::GetStateText() const
{
	if (State == EWatcherState::Watching && LurePhase == EGuardLurePhase::Returning)
	{
		return TEXT("RETURNING");
	}
	// Plainly watching on a point with its own state text ("SPEAKING").
	if (State == EWatcherState::Watching && LurePhase == EGuardLurePhase::None && PatrolPoints.IsValidIndex(LastPatrolIndex))
	{
		const APatrolPoint* Held = PatrolPoints[LastPatrolIndex].Get();
		if (Held && !Held->StateText.IsEmpty())
		{
			return Held->StateText;
		}
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
	const double BarsOnClock = Clock->GetTotalBeats() / static_cast<double>(FMath::Max(1, Clock->BeatsPerBar));

	// Step 3: a guard who walks in at the alarm counts his patrol from his arrival bar; before it he stands at the
	// first point (and on the bar he arrives he is there, whatever bar that is).
	const bool bFromArrival = bPatrolFromArrival && !ArriveOn.IsNone();
	double PatrolBars = BarsOnClock;
	if (bFromArrival)
	{
		const int32 StartBar = GetArriveBar();
		if (StartBar <= 0 || BarsOnClock < static_cast<double>(StartBar - 1))
		{
			OutLocation = Points[0]->GetActorLocation();
			OutYaw = static_cast<float>(Points[0]->GetActorRotation().Yaw);
			return;
		}
		PatrolBars = BarsOnClock - static_cast<double>(StartBar - 1);
	}

	const double BarsElapsed = FMath::Max(0.0, PatrolBars + static_cast<double>(PatrolOffsetBars));
	// Step 3: bPatrolOnce never loops: the last point's slot runs on for good (he holds it once he is there).
	const int64 Cycle = bPatrolOnce ? 0 : static_cast<int64>(BarsElapsed / static_cast<double>(CycleBars));
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
	else if (bFromArrival)
	{
		// He arrived standing at the first point: no walk in from his placed pose.
		FromLocation = Points[0]->GetActorLocation();
		FromYaw = static_cast<float>(Points[0]->GetActorRotation().Yaw);
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

// ---------------------------------------------------------------------------------------------------------------------
// Step 2 (W)
// ---------------------------------------------------------------------------------------------------------------------

int32 APatrolGuard::GetPatrolIndexAtBar(int32 Bar) const
{
	// The same slots as EvaluatePatrol (null points skipped), read at the start of Bar.
	TArray<int32, TInlineAllocator<16>> Indices;
	int32 CycleBars = 0;
	for (int32 i = 0; i < PatrolPoints.Num(); ++i)
	{
		if (const APatrolPoint* PatrolPoint = PatrolPoints[i].Get())
		{
			Indices.Add(i);
			CycleBars += FMath::Max(1, PatrolPoint->Bars);
		}
	}
	if (Indices.Num() == 0 || CycleBars <= 0)
	{
		return -1;
	}

	// Step 3: from his arrival bar (before it: the first point), and no loop with bPatrolOnce (past the last point's
	// start the loop below returns the last point for good). Matches EvaluatePatrol.
	int32 BarsElapsed = 0;
	if (bPatrolFromArrival && !ArriveOn.IsNone())
	{
		const int32 StartBar = GetArriveBar();
		if (StartBar <= 0 || Bar < StartBar)
		{
			return Indices[0];
		}
		BarsElapsed = FMath::Max(0, Bar - StartBar + PatrolOffsetBars);
	}
	else
	{
		BarsElapsed = FMath::Max(0, Bar - 1 + PatrolOffsetBars);
	}
	const int32 CyclePhase = bPatrolOnce ? BarsElapsed : BarsElapsed % CycleBars;
	int32 SlotStart = 0;
	for (int32 k = 0; k < Indices.Num(); ++k)
	{
		const int32 SlotBars = FMath::Max(1, PatrolPoints[Indices[k]]->Bars);
		if (CyclePhase < SlotStart + SlotBars || k == Indices.Num() - 1)
		{
			return Indices[k];
		}
		SlotStart += SlotBars;
	}
	return Indices.Last();
}

bool APatrolGuard::ChangesAtBar(int32 Bar, const UBarClockComponent* Clock) const
{
	if (Bar < 2 || !HasArrived() || LureSecondsLeft > 0.f)
	{
		return false;
	}

	int32 NumPoints = 0;
	for (const TObjectPtr<APatrolPoint>& PointPtr : PatrolPoints)
	{
		if (PointPtr.Get())
		{
			++NumPoints;
		}
	}
	if (NumPoints < 2)
	{
		return false;
	}
	return GetPatrolIndexAtBar(Bar) != GetPatrolIndexAtBar(Bar - 1);
}

void APatrolGuard::GetFrameFarCorners(TArray<FVector>& OutLocal) const
{
	// The far edge of the drawn wedge (FramePivot's space: +X ahead, +Y right), on the floor and at FrameLineHeight.
	const float Range = FMath::Max(FrameRange, 1.f);
	const float HalfAngle = FMath::Clamp(FrameHalfAngle, 1.f, 85.f);
	const float HalfWidth = Range * FMath::Tan(FMath::DegreesToRadians(HalfAngle));
	const float PivotZ = FramePivot ? static_cast<float>(FramePivot->GetRelativeLocation().Z) : 0.f;
	const float TopZ = FrameLineHeight - PivotZ;

	OutLocal.Reset();
	OutLocal.Add(FVector(Range, -HalfWidth, 0.f));	// floor-left
	OutLocal.Add(FVector(Range, HalfWidth, 0.f));	// floor-right
	OutLocal.Add(FVector(Range, HalfWidth, TopZ));	// top-right
	OutLocal.Add(FVector(Range, -HalfWidth, TopZ));	// top-left
}
