// Owner: (u). Constructor by the header pass; every "// BODY: (u)" is a stub to fill.

#include "Device.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreGlobals.h"
#include "Materials/MaterialInstanceDynamic.h"
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
	// BODY: (u)
}

FVector ADevice::GetUsePoint() const
{
	return GetActorTransform().TransformPosition(UseOffset);
}

void ADevice::SetInReach(bool bInReach)
{
	// BODY: (u)
}

void ADevice::LayOutUseRing()
{
	// BODY: (u)
}

void ADevice::BeginPlay()
{
	Super::BeginPlay();

	UsesLeft = Uses;
	bRunning = false;
	bLanded = false;
	RunSeconds = 0.f;
	RefreshBodyLook();
}

void ADevice::SetHovered(bool bInHovered)
{
	bHovered = bInHovered;
	RefreshBodyLook();
}

bool ADevice::CanFire(FString& OutWhyNot) const
{
	if (Uses > 0 && UsesLeft <= 0)
	{
		OutWhyNot = DeviceName + TEXT(" - used");
		return false;
	}
	if (bRunning)
	{
		OutWhyNot = DeviceName + TEXT(" - busy");
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
	const FLinearColor Look = bHovered ? BodyColour + (FLinearColor::White - BodyColour) * 0.4f : BodyColour;
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
