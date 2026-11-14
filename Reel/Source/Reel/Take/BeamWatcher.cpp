// Owner: (d). Constructor by the header pass; bodies by (d).

#include "BeamWatcher.h"

#include "BarClockComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Julian.h"
#include "TakeShapes.h"
#include "TakeStyle.h"

namespace TakeSliceDBeam
{
	/** 2D distance (cm) from P to the segment A-B, heights ignored. */
	double DistToSegment2D(const FVector& P, const FVector& A, const FVector& B)
	{
		const FVector2D P2(P.X, P.Y);
		const FVector2D A2(A.X, A.Y);
		const FVector2D AB(B.X - A.X, B.Y - A.Y);
		const double LenSq = AB.SizeSquared();
		double Along = 0.0;
		if (LenSq > UE_KINDA_SMALL_NUMBER)
		{
			Along = FMath::Clamp(FVector2D::DotProduct(P2 - A2, AB) / LenSq, 0.0, 1.0);
		}
		return FVector2D::Distance(P2, A2 + AB * Along);
	}

	/** 2D cross product of (A - O) and (B - O). */
	double Cross2D(const FVector& O, const FVector& A, const FVector& B)
	{
		return (A.X - O.X) * (B.Y - O.Y) - (A.Y - O.Y) * (B.X - O.X);
	}

	/** 2D distance between the segments P0-P1 and Q0-Q1 (0 when they cross). P0 == P1 gives the point test. */
	double SegSegDist2D(const FVector& P0, const FVector& P1, const FVector& Q0, const FVector& Q1)
	{
		const double D1 = Cross2D(Q0, Q1, P0);
		const double D2 = Cross2D(Q0, Q1, P1);
		const double D3 = Cross2D(P0, P1, Q0);
		const double D4 = Cross2D(P0, P1, Q1);
		const bool bPStraddles = (D1 > 0.0 && D2 < 0.0) || (D1 < 0.0 && D2 > 0.0);
		const bool bQStraddles = (D3 > 0.0 && D4 < 0.0) || (D3 < 0.0 && D4 > 0.0);
		if (bPStraddles && bQStraddles)
		{
			return 0.0;
		}
		const double ToQ = FMath::Min(DistToSegment2D(P0, Q0, Q1), DistToSegment2D(P1, Q0, Q1));
		const double ToP = FMath::Min(DistToSegment2D(Q0, P0, P1), DistToSegment2D(Q1, P0, P1));
		return FMath::Min(ToQ, ToP);
	}
}

ABeamWatcher::ABeamWatcher()
{
	FramePattern = TakeStyle::PatternDash;	// the beam is a line from post A: dashed, not banded
	Label = TEXT("BEAM");

	// Two 120 cm posts; post B at EndB. OnConstruction re-places post B, the line and the click volume.
	PostA = FTakeShapes::CreateShape(this, TEXT("PostA"), ETakeShape::Cylinder, Root, FVector(0.f, 0.f, 60.f), FVector(0.15f, 0.15f, 1.2f));
	PostB = FTakeShapes::CreateShape(this, TEXT("PostB"), ETakeShape::Cylinder, Root, EndB + FVector(0.f, 0.f, 60.f), FVector(0.15f, 0.15f, 1.2f));

	// The line: a thin cube from post A to post B at BeamHeight (FramePivot sits 2 cm above the floor).
	const float Length = EndB.Size2D();
	FrameMesh->SetRelativeLocation(EndB * 0.5f + FVector(0.f, 0.f, BeamHeight - 2.f));
	FrameMesh->SetRelativeRotation(FRotator(0.f, EndB.Rotation().Yaw, 0.f));
	FrameMesh->SetRelativeScale3D(FVector(Length / 100.f, BeamThickness / 100.f, BeamThickness / 100.f));

	Tally->SetRelativeLocation(FVector(0.f, 0.f, 135.f));

	ClickVolume->SetRelativeLocation(EndB * 0.5f + FVector(0.f, 0.f, BeamHeight));
	ClickVolume->InitBoxExtent(FVector(Length * 0.5f + 20.f, 40.f, 80.f));
	ClickVolume->SetRelativeRotation(FRotator(0.f, EndB.Rotation().Yaw, 0.f));
}

void ABeamWatcher::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LayoutBeam();
}

void ABeamWatcher::BeginPlay()
{
	Super::BeginPlay();
	LayoutBeam();

	// The first StepWatcher reads the clock and corrects this on the first frame.
	bDropped = false;
	bBeforeLive = LiveFromBar > 1;
	SyncState();
	RefreshFrameLook();
}

void ABeamWatcher::SyncState()
{
	Super::SyncState();
	if (!HasArrived())
	{
		return;	// not arrived: Off first (the base), whatever the clock says
	}
	if (DarkSecondsLeft <= 0.f)
	{
		if (bBeforeLive)
		{
			State = EWatcherState::Off;
		}
		else if (bDropped)
		{
			State = EWatcherState::Dropped;
		}
	}
}

void ABeamWatcher::LayoutBeam()
{
	const float Length = FMath::Max(static_cast<float>(EndB.Size2D()), 1.f);
	const float Yaw = static_cast<float>(FVector(EndB.X, EndB.Y, 0.0).Rotation().Yaw);
	const FVector Middle = FVector(EndB.X * 0.5, EndB.Y * 0.5, EndB.Z * 0.5 + BeamHeight);

	PostB->SetRelativeLocation(EndB + FVector(0.f, 0.f, 60.f));

	// FramePivot at the beam's midpoint so the beat pulse swells the line about its centre.
	FramePivot->SetRelativeLocation(Middle);
	FrameMesh->SetRelativeLocation(FVector::ZeroVector);
	FrameMesh->SetRelativeRotation(FRotator(0.f, Yaw, 0.f));
	FrameMesh->SetRelativeScale3D(FVector(Length / 100.f, BeamThickness / 100.f, BeamThickness / 100.f));

	ClickVolume->SetRelativeLocation(Middle);
	ClickVolume->SetRelativeRotation(FRotator(0.f, Yaw, 0.f));
	ClickVolume->SetBoxExtent(FVector(Length * 0.5f + 20.f, 40.f, 80.f));

	FTakeShapes::SetColour(PostA, TakeColors::Grey);
	FTakeShapes::SetColour(PostB, TakeColors::Grey);
	FTakeShapes::SetColour(FrameMesh, TakeColors::Red);
	FTakeShapes::SetColour(Tally, TakeColors::DarkGrey);
}

void ABeamWatcher::StepWatcher(float DeltaSeconds, UBarClockComponent* Clock)
{
	if (Clock)
	{
		const int32 CurBar = Clock->GetBar();
		const int32 CurBeat = Clock->GetBeatInBar();
		const bool bNowBeforeLive = CurBar < LiveFromBar;
		const bool bNowDropped = !bNowBeforeLive && (CurBar % FMath::Max(1, DropEveryBars)) == 0 && CurBeat == DropBeatInBar;

		// The hi-hat marks the drop beat whether or not a Blind / Mask has the beam off already.
		if (bNowDropped && !bDropped)
		{
			Clock->PlayAccent();
		}
		bDropped = bNowDropped;
		bBeforeLive = bNowBeforeLive;
	}

	// Dark countdown, then the state (dark first, else OFF / DROPPED / WATCHING via SyncState), the state-change
	// log ("BEAM DROPPED", "BEAM LIVE"), the frame look (hidden / dark / red) and the beat pulse, this frame.
	Super::StepWatcher(DeltaSeconds, Clock);
}

bool ABeamWatcher::HoldsJulian(const AJulian* Julian) const
{
	return TouchesJulian(Julian);
}

bool ABeamWatcher::ApplyEffect(EDeviceEffect Effect, float Duration, const FVector& Point)
{
	if (Effect == EDeviceEffect::Blind)
	{
		return Super::ApplyEffect(Effect, Duration, Point);
	}
	if (Effect == EDeviceEffect::Mask)
	{
		// Stacks with a blind: max(dark time left, Duration), reported MASKED.
		AddDark(EWatcherState::Masked, Duration);
		TAKE_LOG(TEXT("%s MASKED %.1f s"), *Label, DarkSecondsLeft);
		return true;
	}
	return false;	// a lure means nothing to a beam
}

float ABeamWatcher::DistanceToPoint(const FVector& Point) const
{
	return static_cast<float>(TakeSliceDBeam::DistToSegment2D(Point, GetEndA(), GetEndB()));
}

bool ABeamWatcher::IsLive() const
{
	return DarkSecondsLeft <= 0.f && State == EWatcherState::Watching;
}

bool ABeamWatcher::TouchesJulianSwept(const AJulian* Julian, const FVector& PrevFeet) const
{
	if (!Julian || !IsLive())
	{
		return false;
	}
	return TakeSliceDBeam::SegSegDist2D(PrevFeet, Julian->GetFeetLocation(), GetEndA(), GetEndB()) <= Julian->GetCapsuleRadius();
}

FString ABeamWatcher::GetStateText() const
{
	if (State == EWatcherState::Watching)
	{
		return TEXT("LIVE");
	}
	return Super::GetStateText();
}

bool ABeamWatcher::TouchesJulian(const AJulian* Julian) const
{
	if (!Julian || !IsLive())
	{
		return false;
	}
	return TakeSliceDBeam::DistToSegment2D(Julian->GetFeetLocation(), GetEndA(), GetEndB()) <= Julian->GetCapsuleRadius();
}

FVector ABeamWatcher::GetEndA() const
{
	return GetActorTransform().TransformPosition(FVector(0.f, 0.f, BeamHeight));
}

FVector ABeamWatcher::GetEndB() const
{
	return GetActorTransform().TransformPosition(EndB + FVector(0.f, 0.f, BeamHeight));
}

FVector ABeamWatcher::GetAimPoint() const
{
	// The middle of the beam's segment; both ends are already at BeamHeight.
	return (GetEndA() + GetEndB()) * 0.5;
}
