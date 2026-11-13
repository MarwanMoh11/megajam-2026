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

	DarkSecondsLeft = 0.f;
	DarkKind = EWatcherState::Blinded;
	bTally = false;
	LastPulse = 0.f;

	FrameMID = FTakeShapes::SetColour(FrameMesh, TakeColors::Red);
	TallyMID = FTakeShapes::SetColour(Tally, TakeColors::DarkGrey);

	SyncState();
	RefreshFrameLook();
	ApplyFrameScale();
}

void AWatcher::SetHovered(bool bInHovered)
{
	bHovered = bInHovered;
	RefreshFrameLook();
}

void AWatcher::StepWatcher(float DeltaSeconds, UBarClockComponent* Clock)
{
	// Subclasses update their own timers / clock flags before calling this.
	const EWatcherState OldState = State;
	if (DarkSecondsLeft > 0.f)
	{
		DarkSecondsLeft = FMath::Max(DarkSecondsLeft - DeltaSeconds, 0.f);
	}

	LastPulse = Clock ? Clock->GetPulse() : 0.f;
	SyncState();
	if (State != OldState)
	{
		TAKE_LOG(TEXT("%s %s"), *Label, *GetStateText());
	}

	// Every frame: the beat pulse is a brightness pulse on the frame's colour.
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
	if (Effect != EDeviceEffect::Blind)
	{
		return false;
	}

	// Stacks: a shorter blind never cuts a longer dark; a lure (subclass timer) is untouched.
	AddDark(EWatcherState::Blinded, Duration);
	TAKE_LOG(TEXT("%s BLINDED %.1f s"), *Label, DarkSecondsLeft);
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

	// Off and Dropped (the beam): no frame at all.
	const bool bShowFrame = State != EWatcherState::Off && State != EWatcherState::Dropped;
	FrameMesh->SetVisibility(bShowFrame);

	FLinearColor Colour = TakeColors::RedDark;
	if (IsLive())
	{
		Colour = bTally ? HoldingColour : TakeColors::Red;
		// The beat: brighter on the beat, decaying with the clock's pulse. The footprint never changes.
		const float BeatLift = FMath::Clamp(PulseBrightness * LastPulse, 0.f, 1.f);
		Colour = Colour + (TakeColors::White - Colour) * BeatLift;
	}
	if (bHovered)
	{
		const float HoverLift = FMath::Clamp(HoverBrightness, 0.f, 1.f);
		Colour = Colour + (TakeColors::White - Colour) * HoverLift;
	}
	FrameMID = FTakeShapes::SetColour(FrameMesh, Colour);
}

void AWatcher::ApplyFrameScale()
{
	// The drawn frame must equal the tested frame: FramePivot always stays at scale 1 (PulseScale / HoverScale
	// are no longer used; the pulse and hover are brightness changes in RefreshFrameLook).
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
