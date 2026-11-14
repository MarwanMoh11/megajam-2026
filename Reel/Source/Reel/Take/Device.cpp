// Owner: (u). Constructor by the header pass; every "// BODY: (u)" is a stub to fill.

#include "Device.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreGlobals.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TakeDirector.h"
#include "TakeShapes.h"
#include "Watcher.h"

ADevice::ADevice()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// A 1 m block on the floor; subclasses reshape it.
	Body = FTakeShapes::CreateShape(this, TEXT("Body"), ETakeShape::Cube, Root, FVector(0.f, 0.f, 50.f), FVector::OneVector);

	ClickVolume = FTakeShapes::CreateClickVolume(this, TEXT("ClickVolume"), Root, FVector(80.f, 80.f, 100.f), FVector(0.f, 0.f, 100.f));

	// The use ring (dashes laid out by LayOutUseRing). Collision: NoCollision, like every take shape.
	UseRing = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("UseRing"));
	UseRing->SetupAttachment(Root);
	UseRing->SetUsingAbsoluteRotation(true);
	FTakeShapes::SetupShape(UseRing, ETakeShape::Cube, false);
}

void ADevice::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LayOutUseRing();
}

FVector ADevice::GetUsePoint() const
{
	return GetActorTransform().TransformPosition(UseOffset);
}

void ADevice::SetInReach(bool bInReach)
{
	bIsInReach = bInReach;
	if (UseRing)
	{
		FTakeShapes::SetColour(UseRing, bIsInReach ? RingReachColour : RingIdleColour);
	}
}

void ADevice::LayOutUseRing()
{
	if (!UseRing)
	{
		return;
	}

	// Flat and level at the use point's own height (it may be on a raised deck), whatever the actor's rotation/scale.
	UseRing->SetUsingAbsoluteScale(true);
	UseRing->SetWorldLocationAndRotation(GetUsePoint(), FQuat::Identity);
	UseRing->SetWorldScale3D(FVector::OneVector);

	UseRing->ClearInstances();
	const int32 Dots = FMath::Max(UseRingDots, 3);
	const float Radius = FMath::Max(UseRadius, 10.f);
	const float StepRad = UE_TWO_PI / static_cast<float>(Dots);
	const float DashLength = Radius * StepRad * FMath::Clamp(UseRingDashFill, 0.05f, 1.f);
	const FVector DashScale(DashLength / 100.f, UseRingDashWidth / 100.f, UseRingDashHeight / 100.f);

	TArray<FTransform> Dashes;
	Dashes.Reserve(Dots);
	for (int32 Index = 0; Index < Dots; ++Index)
	{
		const float Angle = StepRad * static_cast<float>(Index);
		const FVector Where(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), UseRingDashHeight * 0.5f);
		// The cube's X runs along the circle's tangent.
		const FRotator Along(0.f, FMath::RadiansToDegrees(Angle) + 90.f, 0.f);
		Dashes.Add(FTransform(Along, Where, DashScale));
	}
	UseRing->AddInstances(Dashes, false, false, false);

	FTakeShapes::SetColour(UseRing, bIsInReach ? RingReachColour : RingIdleColour);
}

void ADevice::BeginPlay()
{
	Super::BeginPlay();

	UsesLeft = Uses;
	bRunning = false;
	bLanded = false;
	RunSeconds = 0.f;
	RefreshBodyLook();

	// OnConstruction does not rerun for placed actors in play or in a package: lay the ring out here too, dim.
	bIsInReach = false;
	LayOutUseRing();
}

void ADevice::SetHovered(bool bInHovered)
{
	bHovered = bInHovered;
	RefreshBodyLook();
}

bool ADevice::CanFire(FString& OutWhyNot) const
{
	// Step 3: locked until its trigger (the pyro and the winch: the alarm).
	if (IsLocked())
	{
		OutWhyNot = DeviceName + TEXT(" is ") + LockedWhy;
		return false;
	}
	if (Uses > 0 && UsesLeft <= 0)
	{
		OutWhyNot = DeviceName + TEXT(" is used up");
		return false;
	}
	if (bRunning)
	{
		OutWhyNot = DeviceName + TEXT(" is busy");
		return false;
	}
	OutWhyNot.Reset();
	return true;
}

bool ADevice::Fire()
{
	FString WhyNot;
	if (!CanFire(WhyNot))
	{
		return false;
	}

	if (Uses > 0)
	{
		UsesLeft = FMath::Max(UsesLeft - 1, 0);
	}
	bRunning = true;
	bLanded = false;
	RunSeconds = 0.f;
	FiredFrame = GFrameCounter;

	TAKE_LOG(TEXT("FIRE %s"), *DeviceName);
	return true;
}

void ADevice::StepDevice(float DeltaSeconds)
{
	// Generic timeline: lands on the first step after the fire (step 3 of the fire frame), runs for Duration.
	if (!bRunning)
	{
		return;
	}
	if (!bLanded)
	{
		MarkLanded();
	}
	RunSeconds += DeltaSeconds;
	if (RunSeconds >= Duration)
	{
		bRunning = false;
	}
}

bool ADevice::LandedThisFrame() const
{
	return bLanded && LandedFrame == GFrameCounter;
}

void ADevice::GatherTargets(const TArray<TObjectPtr<AWatcher>>& All, TArray<AWatcher*>& Out) const
{
	Out.Reset();
	const FVector Point = GetEffectPoint();
	for (AWatcher* W : All)
	{
		if (W && W->DistanceToPoint(Point) <= Range)
		{
			Out.Add(W);
		}
	}
}

FVector ADevice::GetEffectPoint() const
{
	return GetActorLocation();
}

FString ADevice::GetPipsText() const
{
	if (Uses <= 0)
	{
		return DeviceName + TEXT("  free");
	}
	FString Pips;
	for (int32 Index = 0; Index < Uses; ++Index)
	{
		Pips.AppendChar(Index < UsesLeft ? TEXT('o') : TEXT('-'));
	}
	return DeviceName + TEXT("  ") + Pips;
}

FVector ADevice::GetLabelPoint() const
{
	return GetActorLocation() + FVector(0.f, 0.f, LabelHeight);
}

void ADevice::RefreshBodyLook()
{
	if (!Body)
	{
		return;
	}
	FLinearColor Look = bHovered ? BodyColour + (FLinearColor::White - BodyColour) * 0.4f : BodyColour;
	if (Uses > 0 && UsesLeft <= 0)
	{
		// Spent: the body is dimmed (a restored spent device reads as spent at once).
		Look = FLinearColor(Look.R * 0.35f, Look.G * 0.35f, Look.B * 0.35f, Look.A);
	}
	BodyMID = FTakeShapes::SetColour(Body, Look);
}

void ADevice::MarkLanded()
{
	bLanded = true;
	LandedFrame = GFrameCounter;
}

bool ADevice::FiredThisFrame() const
{
	return FiredFrame == GFrameCounter;
}

// ---------------------------------------------------------------------------------------------------------------------
// Step 2 (header pass stubs; BODY: D)
// ---------------------------------------------------------------------------------------------------------------------

void ADevice::RestoreUses(int32 InUsesLeft)
{
	// Free devices (Uses 0) have nothing to restore.
	if (Uses <= 0)
	{
		return;
	}
	UsesLeft = FMath::Clamp(InUsesLeft, 0, Uses);
	RefreshBodyLook();	// the pips read UsesLeft; the body dims when spent
	TAKE_LOG(TEXT("RESTORE %s uses=%d"), *DeviceName, UsesLeft);
}

// ---------------------------------------------------------------------------------------------------------------------
// Step 3 (W)
// ---------------------------------------------------------------------------------------------------------------------

bool ADevice::IsLocked() const
{
	if (LockedUntil.IsNone())
	{
		return false;
	}
	const ATakeDirector* Director = ATakeDirector::Get(this);
	return Director && !Director->HasFired(LockedUntil);
}
