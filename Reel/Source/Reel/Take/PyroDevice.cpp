// Maya's pyro (step 3). Owner: W. See PyroDevice.h for the contract.

#include "PyroDevice.h"

#include "BarClockComponent.h"
#include "Components/PointLightComponent.h"
#include "DroneWatcher.h"
#include "Julian.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TakeDirector.h"
#include "TakeShapes.h"

APyroDevice::APyroDevice()
{
	DeviceName = TEXT("PYRO BOARD");
	Effect = EDeviceEffect::Blind;
	Range = 1300.f;
	Duration = 6.f;
	Uses = 1;
	FiredFrom = ETakeCamera::B;
	UseVerb = TEXT("Fire the pyro");
	LockedUntil = FName(TakeTriggers::Case);

	FlareLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FlareLight"));
	FlareLight->SetupAttachment(RootComponent);
	FlareLight->SetMobility(EComponentMobility::Movable);
	FlareLight->SetCastShadows(false);
	FlareLight->SetVisibility(false);
}

namespace TakePyroLocal
{
	/** FlareLight's reach, cm: the whole dais and the wings around it. */
	constexpr float FlareAttenuationRadius = 2500.f;

	/** A flare's height swells by up to this share on the beat (the clock's pulse)... */
	constexpr float FlickerPulse = 0.25f;

	/** ...and wobbles by this share on its own fast sine (each flare a little out of step with the next). */
	constexpr float FlickerWobble = 0.12f;
	constexpr float FlickerSpeed = 23.f;
	constexpr float FlickerStep = 1.7f;

	/** The flare's height scale at RunSeconds for flare Index, with the clock's pulse. */
	float FlareFlicker(float RunSeconds, int32 Index, float Pulse)
	{
		const float Wobble = FMath::Sin(RunSeconds * FlickerSpeed + static_cast<float>(Index) * FlickerStep);
		return FMath::Max(0.2f, 1.f + FlickerPulse * Pulse + FlickerWobble * Wobble);
	}
}

void APyroDevice::BeginPlay()
{
	Super::BeginPlay();	// UsesLeft, the body's colour, the use ring

	bRunning = false;
	bLanded = false;
	RunSeconds = 0.f;

	// The flares: gold cones standing on their offsets, hidden until fired. No shadows, NoCollision.
	for (const TObjectPtr<UStaticMeshComponent>& OldFlare : Flares)
	{
		if (UStaticMeshComponent* Old = OldFlare.Get())
		{
			Old->DestroyComponent();
		}
	}
	Flares.Reset();

	const float Across = FMath::Max(FlareSize.X, 1.f);
	const float Tall = FMath::Max(FlareSize.Y, 1.f);
	for (const FVector& Offset : FlareOffsets)
	{
		// The basic cone is 100 cm tall with its pivot at its middle: lift it half its height so it stands on Offset.
		const FVector Where = Offset + FVector(0.f, 0.f, Tall * 0.5f);
		const FVector Scale(Across / 100.f, Across / 100.f, Tall / 100.f);
		UStaticMeshComponent* Flare = FTakeShapes::AddShapeAtRuntime(this, ETakeShape::Cone, Root, Where, Scale, FlareColour, false);
		if (Flare)
		{
			Flare->SetMobility(EComponentMobility::Movable);
			Flare->SetVisibility(false);
			Flares.Add(Flare);
		}
	}

	// The light: gold, at the effect point, off until fired.
	if (FlareLight)
	{
		FlareLight->SetWorldLocation(GetEffectPoint());
		FlareLight->SetIntensityUnits(ELightUnits::Candelas);
		FlareLight->SetIntensity(FlareCandelas);
		FlareLight->SetLightColor(FlareColour);
		FlareLight->SetAttenuationRadius(TakePyroLocal::FlareAttenuationRadius);
		FlareLight->SetVisibility(false);
	}
}

bool APyroDevice::Fire()
{
	// Spends the use, bRunning, RunSeconds = 0, FiredFrame, "FIRE PYRO".
	if (!Super::Fire())
	{
		return false;
	}

	// Lands on this frame, like the sound desk: StepDevices (step 3, same frame) applies the Blind.
	MarkLanded();

	// The flares stand up, the light burns and the board goes white, this frame.
	const float Tall = FMath::Max(FlareSize.Y, 1.f);
	const float Across = FMath::Max(FlareSize.X, 1.f);
	for (int32 Index = 0; Index < Flares.Num(); ++Index)
	{
		if (UStaticMeshComponent* Flare = Flares[Index].Get())
		{
			const FVector Offset = FlareOffsets.IsValidIndex(Index) ? FlareOffsets[Index] : FVector::ZeroVector;
			Flare->SetRelativeLocation(Offset + FVector(0.f, 0.f, Tall * 0.5f));
			Flare->SetRelativeScale3D(FVector(Across / 100.f, Across / 100.f, Tall / 100.f));
			Flare->SetVisibility(true);
		}
	}
	if (FlareLight)
	{
		FlareLight->SetIntensity(FlareCandelas);
		FlareLight->SetVisibility(true);
	}
	if (Body)
	{
		BodyMID = FTakeShapes::SetColour(Body, TakeColors::White);
	}

	const ATakeDirector* Director = ATakeDirector::Get(this);
	if (UBarClockComponent* Clock = Director ? Director->Clock.Get() : nullptr)
	{
		Clock->PlayCrackle();
	}
	TAKE_LOG(TEXT("%s burns: blind %.1f s in %.0f cm"), *DeviceName, Duration, Range);
	return true;
}

void APyroDevice::StepDevice(float DeltaSeconds)
{
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
		// Out for good: flares and light off, the board back to its (spent, dimmed) colour.
		bRunning = false;
		for (const TObjectPtr<UStaticMeshComponent>& FlarePtr : Flares)
		{
			if (UStaticMeshComponent* Flare = FlarePtr.Get())
			{
				Flare->SetVisibility(false);
			}
		}
		if (FlareLight)
		{
			FlareLight->SetVisibility(false);
		}
		RefreshBodyLook();
		TAKE_LOG(TEXT("%s out"), *DeviceName);
		return;
	}

	// Burning: each flare's height flickers with the beat and its own sine, foot fixed on its offset.
	const ATakeDirector* Director = ATakeDirector::Get(this);
	const UBarClockComponent* Clock = Director ? Director->Clock.Get() : nullptr;
	const float Pulse = Clock ? Clock->GetPulse() : 0.f;
	const float Tall = FMath::Max(FlareSize.Y, 1.f);
	const float Across = FMath::Max(FlareSize.X, 1.f);
	for (int32 Index = 0; Index < Flares.Num(); ++Index)
	{
		UStaticMeshComponent* Flare = Flares[Index].Get();
		if (!Flare)
		{
			continue;
		}
		const float Flicker = TakePyroLocal::FlareFlicker(RunSeconds, Index, Pulse);
		const FVector Offset = FlareOffsets.IsValidIndex(Index) ? FlareOffsets[Index] : FVector::ZeroVector;
		Flare->SetRelativeLocation(Offset + FVector(0.f, 0.f, Tall * Flicker * 0.5f));
		Flare->SetRelativeScale3D(FVector(Across / 100.f, Across / 100.f, Tall * Flicker / 100.f));
	}
	if (FlareLight)
	{
		FlareLight->SetIntensity(FlareCandelas * (0.85f + 0.3f * Pulse));
	}
}

FVector APyroDevice::GetEffectPoint() const
{
	return GetActorTransform().TransformPosition(EffectOffset);
}

void APyroDevice::GatherTargets(const TArray<TObjectPtr<AWatcher>>& All, TArray<AWatcher*>& Out) const
{
	Super::GatherTargets(All, Out);
	if (!bBlindsLockedDrone)
	{
		return;
	}
	// A drone locked on someone on the stage is looking straight into the charges, wherever it is.
	const ATakeDirector* const Director = ATakeDirector::Get(this);
	const AJulian* const JulianActor = Director ? Director->Julian.Get() : nullptr;
	if (!JulianActor || FVector::Dist2D(JulianActor->GetFeetLocation(), GetEffectPoint()) > Range)
	{
		return;
	}
	for (AWatcher* W : All)
	{
		const ADroneWatcher* const Drone = Cast<ADroneWatcher>(W);
		if (Drone && Drone->IsChasing() && !Out.Contains(W))
		{
			Out.Add(W);
		}
	}
}
