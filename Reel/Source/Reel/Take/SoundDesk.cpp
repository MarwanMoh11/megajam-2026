// Owner: D. Constructor by the header pass; bodies by D (step 2).

#include "SoundDesk.h"

#include "BarClockComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TakeDirector.h"
#include "TakeShapes.h"

ASoundDesk::ASoundDesk()
{
	DeviceName = TEXT("SOUND DESK");
	Effect = EDeviceEffect::Blind;
	Range = 6000.f;
	Duration = 4.f;
	Uses = 1;
	FiredFrom = ETakeCamera::C;
	UseVerb = TEXT("FEEDBACK");

	// QA devices: the meters' amber glow, jumping while the feedback runs; the master fader is the lever.
	PilotColour = FLinearColor(1.f, 0.62f, 0.22f);
	PilotRunFlickerHz = 9.f;

	// A desk: a low wide box.
	if (Body)
	{
		Body->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
		Body->SetRelativeScale3D(FVector(0.6f, 1.4f, 1.0f));
	}
}

namespace TakeSoundDeskLocal
{
	/** The desk's body while the feedback runs: a steady near-white. */
	FLinearColor FlashColour(const FLinearColor& BodyColour)
	{
		return FMath::Lerp(BodyColour, TakeColors::White, 0.8f);
	}
}

void ASoundDesk::BeginPlay()
{
	Super::BeginPlay();	// UsesLeft, the body's colour, the use ring

	bRunning = false;
	bLanded = false;
	RunSeconds = 0.f;
}

bool ASoundDesk::Fire()
{
	// Spends the use, bRunning, RunSeconds = 0, FiredFrame, "FIRE DESK".
	if (!Super::Fire())
	{
		return false;
	}

	// Lands on this frame, like the follow spot: StepDevices (step 3, same frame) applies the Blind.
	MarkLanded();

	// The body flashes white from this frame (the box only: a model's meters flicker instead, through the pilot).
	if (Body && BodyIsBox())
	{
		BodyMID = FTakeShapes::SetColour(Body, TakeColors::White);
	}

	// The squeal, and the band drops out (not a band break).
	const ATakeDirector* Director = ATakeDirector::Get(this);
	UBarClockComponent* Clock = Director ? Director->Clock.Get() : nullptr;
	if (Clock)
	{
		Clock->PlayFeedback();
		Clock->MuteMetronome(static_cast<float>(FMath::Max(0, MuteBars)));
	}
	TAKE_LOG(TEXT("%s FEEDBACK: blind %.1f s in %.0f cm, band out %d bars"), *DeviceName, Duration, Range, MuteBars);
	return true;
}

void ASoundDesk::StepDevice(float DeltaSeconds)
{
	StepLook(DeltaSeconds);	// QA devices: the meters, the fader

	if (!bRunning)
	{
		return;
	}

	// The fire frame's time is not taken twice (Fire ran in step 2 of this frame).
	if (!FiredThisFrame())
	{
		RunSeconds += DeltaSeconds;
	}

	if (RunSeconds >= Duration)
	{
		bRunning = false;
		RefreshBodyLook();	// back to BodyColour (dimmed: it is spent)
		TAKE_LOG(TEXT("%s quiet"), *DeviceName);
		return;
	}

	if (Body && BodyIsBox())
	{
		BodyMID = FTakeShapes::SetColour(Body, TakeSoundDeskLocal::FlashColour(BodyColour));
	}
}
