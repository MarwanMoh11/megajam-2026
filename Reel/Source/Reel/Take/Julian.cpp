// Owner: (c). Constructor by the header pass; every "// BODY: (c)" is a stub to fill.

#include "Julian.h"

#include "BarClockComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Mark.h"
#include "NavigationData.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "TakeDirector.h"
#include "TakeShapes.h"
#include "TakeTypes.h"

namespace TakeJulianSliceC
{
	// Dots sit this far above his feet (the floor), so they clear the floor without z-fighting.
	constexpr float DotLift = 2.f;
	// A dot is a flat block: this tall as a fraction of the 100 cm cube.
	constexpr float DotScaleZ = 0.02f;
	// Guards against a zero spacing and absurd paths.
	constexpr float MinDotSpacing = 5.f;
	constexpr int32 MaxDots = 1000;

	/** Yaw (degrees) of the 2D direction From -> To; false if the two points coincide in 2D. */
	bool YawBetween(const FVector& From, const FVector& To, float& OutYaw)
	{
		const FVector Dir(To.X - From.X, To.Y - From.Y, 0.f);
		if (Dir.SizeSquared() < KINDA_SMALL_NUMBER)
		{
			return false;
		}
		OutYaw = static_cast<float>(Dir.Rotation().Yaw);
		return true;
	}
}

AJulian::AJulian()
{
	PrimaryActorTick.bCanEverTick = false;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	AutoPossessAI = EAutoPossessAI::Disabled;
	AIControllerClass = nullptr;

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->InitCapsuleSize(34.f, 90.f);
	Capsule->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Capsule->SetGenerateOverlapEvents(false);
	Capsule->SetCanEverAffectNavigation(false);
	RootComponent = Capsule;

	// Body 140 cm tall from the feet (-90) up; head on top; a nose block facing +X.
	Body = FTakeShapes::CreateShape(this, TEXT("Body"), ETakeShape::Cylinder, Capsule, FVector(0.f, 0.f, -20.f), FVector(0.55f, 0.55f, 1.4f));
	Head = FTakeShapes::CreateShape(this, TEXT("Head"), ETakeShape::Sphere, Capsule, FVector(0.f, 0.f, 70.f), FVector(0.4f, 0.4f, 0.4f));
	Nose = FTakeShapes::CreateShape(this, TEXT("Nose"), ETakeShape::Cube, Capsule, FVector(22.f, 0.f, 70.f), FVector(0.15f, 0.12f, 0.1f));

	// World-space dots: absolute transform at identity, so instances added with bWorldSpace stay put while he walks.
	PathDots = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PathDots"));
	PathDots->SetupAttachment(Capsule);
	PathDots->SetUsingAbsoluteLocation(true);
	PathDots->SetUsingAbsoluteRotation(true);
	PathDots->SetUsingAbsoluteScale(true);
	FTakeShapes::SetupShape(PathDots, ETakeShape::Cube, false);
}

void AJulian::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyLook();
}

void AJulian::BeginPlay()
{
	Super::BeginPlay();
	ApplyLook();

	// He starts the take standing on a mark if one is under his feet (so MARK shows from the first frame).
	StandingMark = nullptr;
	for (TActorIterator<AMark> It(GetWorld()); It; ++It)
	{
		if (FVector::Dist2D(It->GetStandPoint(), GetFeetLocation()) <= StartMarkTolerance)
		{
			StandingMark = *It;
			break;
		}
	}

	PathPoints.Empty();
	NextPointIndex = 0;
	bWalking = false;
	DotTransforms.Empty();
	DotDistances.Empty();
	FirstDotIndex = 0;
	WalkedDistance = 0.f;
	if (PathDots)
	{
		PathDots->ClearInstances();
	}

	// Snap onto the navmesh if he is slightly off it (keeps his placed height). No navmesh yet: leave him.
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	const ANavigationData* NavData = NavSys ? NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate) : nullptr;
	if (NavSys && NavData)
	{
		FNavLocation OnNav;
		if (NavSys->ProjectPointToNavigation(GetFeetLocation(), OnNav, NavProjectExtent, NavData))
		{
			const FVector Here = GetActorLocation();
			if (FVector::Dist2D(Here, OnNav.Location) > 1.0)
			{
				SetActorLocation(FVector(OnNav.Location.X, OnNav.Location.Y, Here.Z));
			}
		}
	}
}

bool AJulian::CueTo(AMark* Mark)
{
	if (!Mark)
	{
		return false;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	const ANavigationData* NavData = NavSys ? NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate) : nullptr;
	if (!NavSys || !NavData)
	{
		TAKE_LOG(TEXT("CUE FAILED %s: no path (no navmesh)"), *Mark->MarkName);
		return false;
	}

	const FVector Feet = GetFeetLocation();
	const FVector Goal = Mark->GetStandPoint();

	FNavLocation StartNav;
	FNavLocation GoalNav;
	if (!NavSys->ProjectPointToNavigation(Feet, StartNav, NavProjectExtent, NavData)
		|| !NavSys->ProjectPointToNavigation(Goal, GoalNav, NavProjectExtent, NavData))
	{
		TAKE_LOG(TEXT("CUE FAILED %s: no path (off the navmesh)"), *Mark->MarkName);
		return false;
	}

	UNavigationPath* NavPath = UNavigationSystemV1::FindPathToLocationSynchronously(this, StartNav.Location, GoalNav.Location, this);
	if (!NavPath || !NavPath->IsValid() || NavPath->IsPartial() || NavPath->PathPoints.Num() == 0)
	{
		TAKE_LOG(TEXT("CUE FAILED %s: no path"), *Mark->MarkName);
		return false;
	}

	const float EndGap = static_cast<float>(FVector::Dist2D(NavPath->PathPoints.Last(), Goal));
	if (EndGap > ArriveTolerance)
	{
		TAKE_LOG(TEXT("CUE FAILED %s: no path (ends %.0f cm from the mark)"), *Mark->MarkName, EndGap);
		return false;
	}

	// The walk: from where he stands to exactly the mark, all at floor height (his feet), so lengths are 2D.
	const double FloorZ = Feet.Z;
	TArray<FVector> NewPoints;
	NewPoints.Reserve(NavPath->PathPoints.Num() + 1);
	NewPoints.Add(FVector(Feet.X, Feet.Y, FloorZ));
	for (int32 PointIndex = 1; PointIndex < NavPath->PathPoints.Num(); ++PointIndex)
	{
		const FVector& Corner = NavPath->PathPoints[PointIndex];
		NewPoints.Add(FVector(Corner.X, Corner.Y, FloorZ));
	}
	if (NewPoints.Num() < 2)
	{
		NewPoints.Add(FVector(Goal.X, Goal.Y, FloorZ));
	}
	else
	{
		NewPoints.Last() = FVector(Goal.X, Goal.Y, FloorZ);
	}

	// Replace the old cue, from wherever he is.
	if (CuedMark)
	{
		CuedMark->SetCued(false);
		CuedMark->SetOccupied(false);
	}
	if (StandingMark && StandingMark != CuedMark)
	{
		StandingMark->SetOccupied(false);
	}

	CuedMark = Mark;
	StandingMark = nullptr;
	PathPoints = MoveTemp(NewPoints);
	NextPointIndex = 1;
	bWalking = true;

	Mark->SetCued(true);

	// Face the first leg that has any length.
	for (int32 PointIndex = 1; PointIndex < PathPoints.Num(); ++PointIndex)
	{
		float Yaw = 0.f;
		if (TakeJulianSliceC::YawBetween(PathPoints[0], PathPoints[PointIndex], Yaw))
		{
			SetActorRotation(FRotator(0.f, Yaw, 0.f));
			break;
		}
	}

	LayOutDots();
	RebuildPathDots();

	float SecondsPerBeat = 0.f;
	if (const ATakeDirector* Director = ATakeDirector::Get(this))
	{
		if (Director->Clock)
		{
			SecondsPerBeat = Director->Clock->GetSecondsPerBeat();
		}
	}
	TAKE_LOG(TEXT("CUE -> %s path %d pts %.0f cm, %.1f s, %.1f beats"), *Mark->MarkName, PathPoints.Num(),
		GetRemainingPathLength(), GetSecondsToArrival(), GetBeatsToArrival(SecondsPerBeat));
	return true;
}

void AJulian::StepWalk(float DeltaSeconds)
{
	if (!bWalking || PathPoints.Num() == 0)
	{
		return;
	}

	const FVector Here = GetActorLocation();
	FVector Pos = PathPoints[0];
	double Budget = FMath::Max(WalkSpeed, 0.f) * FMath::Max(DeltaSeconds, 0.f);
	double Moved = 0.0;
	int32 Next = FMath::Max(NextPointIndex, 1);
	bool bHasYaw = false;
	float Yaw = 0.f;

	// Spend the step's distance along the path, carrying what is left across corners.
	while (Next < PathPoints.Num())
	{
		const FVector& Target = PathPoints[Next];
		const double Dist = FVector::Dist(Pos, Target);
		float LegYaw = 0.f;
		if (TakeJulianSliceC::YawBetween(Pos, Target, LegYaw))
		{
			Yaw = LegYaw;
			bHasYaw = true;
		}
		if (Budget >= Dist)
		{
			Pos = Target;
			Budget -= Dist;
			Moved += Dist;
			++Next;
			continue;
		}
		Pos += (Target - Pos) * (Budget / Dist);
		Moved += Budget;
		break;
	}

	SetActorLocation(FVector(Pos.X, Pos.Y, Here.Z));
	if (bHasYaw)
	{
		SetActorRotation(FRotator(0.f, Yaw, 0.f));
	}

	if (Next >= PathPoints.Num())
	{
		// Arrived: exactly on the last point (the mark), stopped.
		bWalking = false;
		PathPoints.Empty();
		NextPointIndex = 0;
		DotTransforms.Empty();
		DotDistances.Empty();
		FirstDotIndex = 0;
		WalkedDistance = 0.f;
		if (PathDots)
		{
			PathDots->ClearInstances();
		}

		StandingMark = CuedMark;
		if (CuedMark)
		{
			CuedMark->SetOccupied(true);
			CuedMark->SetCued(false);
			TAKE_LOG(TEXT("ARRIVED %s"), *CuedMark->MarkName);
		}
		return;
	}

	// Still walking: drop the corners he passed, and keep PathPoints[0] at his position.
	if (Next > 1)
	{
		PathPoints.RemoveAt(1, Next - 1);
	}
	PathPoints[0] = Pos;
	NextPointIndex = 1;

	// Trim the dots behind him.
	WalkedDistance += static_cast<float>(Moved);
	const int32 OldFirst = FirstDotIndex;
	while (FirstDotIndex < DotDistances.Num() && DotDistances[FirstDotIndex] <= WalkedDistance)
	{
		++FirstDotIndex;
	}
	if (FirstDotIndex != OldFirst)
	{
		RebuildPathDots();
	}
}

bool AJulian::IsWalking() const
{
	return bWalking;
}

float AJulian::GetRemainingPathLength() const
{
	if (!bWalking)
	{
		return 0.f;
	}
	double Total = 0.0;
	for (int32 PointIndex = 1; PointIndex < PathPoints.Num(); ++PointIndex)
	{
		Total += FVector::Dist(PathPoints[PointIndex - 1], PathPoints[PointIndex]);
	}
	return static_cast<float>(Total);
}

float AJulian::GetSecondsToArrival() const
{
	return WalkSpeed > 0.f ? GetRemainingPathLength() / WalkSpeed : 0.f;
}

float AJulian::GetBeatsToArrival(float SecondsPerBeat) const
{
	return SecondsPerBeat > 0.f ? GetSecondsToArrival() / SecondsPerBeat : 0.f;
}

const TArray<FVector>& AJulian::GetPathPoints() const
{
	return PathPoints;
}

AMark* AJulian::GetCuedMark() const
{
	return CuedMark;
}

AMark* AJulian::GetStandingMark() const
{
	return bWalking ? nullptr : StandingMark.Get();
}

FVector AJulian::GetFeetLocation() const
{
	const float HalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 90.f;
	return GetActorLocation() - FVector(0.f, 0.f, HalfHeight);
}

float AJulian::GetCapsuleRadius() const
{
	return Capsule ? Capsule->GetScaledCapsuleRadius() : 34.f;
}

void AJulian::RebuildPathDots()
{
	if (!PathDots)
	{
		return;
	}
	PathDots->ClearInstances();
	if (FirstDotIndex >= DotTransforms.Num())
	{
		return;
	}
	TArray<FTransform> Remaining;
	Remaining.Reserve(DotTransforms.Num() - FirstDotIndex);
	for (int32 DotIndex = FirstDotIndex; DotIndex < DotTransforms.Num(); ++DotIndex)
	{
		Remaining.Add(DotTransforms[DotIndex]);
	}
	PathDots->AddInstances(Remaining, false, true, false);
}

void AJulian::ApplyLook()
{
	if (Body)
	{
		FTakeShapes::SetColour(Body, TakeColors::White);
	}
	if (Head)
	{
		FTakeShapes::SetColour(Head, TakeColors::White);
	}
	if (Nose)
	{
		FTakeShapes::SetColour(Nose, TakeColors::DarkGrey);
	}
	if (PathDots)
	{
		FTakeShapes::SetColour(PathDots, TakeColors::Gold);
	}
}

void AJulian::LayOutDots()
{
	DotTransforms.Reset();
	DotDistances.Reset();
	FirstDotIndex = 0;
	WalkedDistance = 0.f;

	const float Spacing = FMath::Max(DotSpacing, TakeJulianSliceC::MinDotSpacing);
	const FVector DotScale(DotSize / 100.f, DotSize / 100.f, TakeJulianSliceC::DotScaleZ);

	double LegStart = 0.0;	// path distance at the start of this leg
	double NextDot = Spacing;	// path distance of the next dot (the first one is a spacing ahead of him)
	for (int32 PointIndex = 1; PointIndex < PathPoints.Num(); ++PointIndex)
	{
		const FVector& From = PathPoints[PointIndex - 1];
		const FVector& To = PathPoints[PointIndex];
		const double LegLength = FVector::Dist(From, To);
		float LegYaw = 0.f;
		TakeJulianSliceC::YawBetween(From, To, LegYaw);
		while (LegLength > 0.0 && NextDot < LegStart + LegLength && DotTransforms.Num() < TakeJulianSliceC::MaxDots)
		{
			const double Alpha = (NextDot - LegStart) / LegLength;
			FVector DotLocation = From + (To - From) * Alpha;
			DotLocation.Z += TakeJulianSliceC::DotLift;
			DotTransforms.Add(FTransform(FRotator(0.f, LegYaw, 0.f), DotLocation, DotScale));
			DotDistances.Add(static_cast<float>(NextDot));
			NextDot += Spacing;
		}
		LegStart += LegLength;
	}
}
