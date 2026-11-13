// Owner: (d). Constructor by the header pass; bodies by (d).

#include "Watcher.h"

#include "BarClockComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Julian.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TakeShapes.h"

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

	Tally = FTakeShapes::CreateShape(this, TEXT("Tally"), ETakeShape::Sphere, Root, FVector(0.f, 0.f, 200.f), FVector(0.15f, 0.15f, 0.15f), FRotator::ZeroRotator, false);

	ClickVolume = FTakeShapes::CreateClickVolume(this, TEXT("ClickVolume"), Root, FVector(70.f, 70.f, 100.f), FVector(0.f, 0.f, 100.f));
}

void AWatcher::BeginPlay()
{
	Super::BeginPlay();

	HomeLocation = GetActorLocation();
	HomeYaw = static_cast<float>(GetActorRotation().Yaw);

	State = EWatcherState::Watching;
	EffectSecondsLeft = 0.f;
	bTally = false;
	LastPulse = 0.f;

	FrameMID = FTakeShapes::SetColour(FrameMesh, TakeColors::Red);
	TallyMID = FTakeShapes::SetColour(Tally, TakeColors::DarkGrey);

	RefreshFrameLook();
	ApplyFrameScale();
}

void AWatcher::SetHovered(bool bInHovered)
{
	bHovered = bInHovered;
	ApplyFrameScale();
}

void AWatcher::StepWatcher(float DeltaSeconds, UBarClockComponent* Clock)
{
	if (EffectSecondsLeft > 0.f)
	{
		EffectSecondsLeft -= DeltaSeconds;
		if (EffectSecondsLeft <= 0.f)
		{
			EffectSecondsLeft = 0.f;
			if (State == EWatcherState::Blinded || State == EWatcherState::Lured || State == EWatcherState::Masked)
			{
				State = EWatcherState::Watching;
				TAKE_LOG(TEXT("%s WATCHING"), *Label);
			}
			RefreshFrameLook();
		}
	}

	LastPulse = Clock ? Clock->GetPulse() : 0.f;
	ApplyFrameScale();
}

bool AWatcher::HoldsJulian(const AJulian* Julian) const
{
	// The base has no frame shape; subclasses test their own.
	return false;
}

bool AWatcher::ApplyEffect(EDeviceEffect Effect, float Duration, const FVector& Point)
{
	if (Effect != EDeviceEffect::Blind)
	{
		return false;
	}

	State = EWatcherState::Blinded;
	EffectSecondsLeft = FMath::Max(Duration, UE_KINDA_SMALL_NUMBER);
	RefreshFrameLook();
	TAKE_LOG(TEXT("%s BLINDED %.1f s"), *Label, Duration);
	return true;
}

void AWatcher::SetTally(bool bHolding)
{
	if (bHolding == bTally)
	{
		return;
	}
	bTally = bHolding;
	TallyMID = FTakeShapes::SetColour(Tally, bTally ? HoldingColour : TakeColors::DarkGrey);
	RefreshFrameLook();
}

float AWatcher::DistanceToPoint(const FVector& Point) const
{
	return static_cast<float>(FVector::Dist2D(GetActorLocation(), Point));
}

bool AWatcher::IsLive() const
{
	// A lured watcher still sees: its frame goes with it.
	return State == EWatcherState::Watching || State == EWatcherState::Lured;
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

	// Off and Dropped (the beam): no frame at all.
	const bool bShowFrame = State != EWatcherState::Off && State != EWatcherState::Dropped;
	FrameMesh->SetVisibility(bShowFrame);

	FLinearColor Colour = TakeColors::RedDark;
	if (IsLive())
	{
		Colour = bTally ? HoldingColour : TakeColors::Red;
	}
	FrameMID = FTakeShapes::SetColour(FrameMesh, Colour);
}

void AWatcher::ApplyFrameScale()
{
	if (!FramePivot)
	{
		return;
	}
	const float Scale = (1.f + PulseScale * LastPulse) * (bHovered ? HoverScale : 1.f);
	FramePivot->SetRelativeScale3D(FVector(Scale, Scale, Scale));
}
