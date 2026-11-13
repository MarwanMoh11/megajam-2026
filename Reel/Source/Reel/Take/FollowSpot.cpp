// Owner: (u). Constructor by the header pass; every "// BODY: (u)" is a stub to fill.

#include "FollowSpot.h"

#include "Components/BoxComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TakeShapes.h"
#include "Watcher.h"

AFollowSpot::AFollowSpot()
{
	DeviceName = TEXT("SPOT");
	UseVerb = TEXT("Spot");
	Effect = EDeviceEffect::Blind;
	Range = 3500.f;
	Duration = 4.f;
	Uses = 0;
	FiredFrom = ETakeCamera::B;
	BodyColour = TakeColors::WorkBlue;

	// The lamp: a squat cylinder on its stand height (place the actor where Maya's lamp stands).
	Body->SetStaticMesh(FTakeShapes::GetMesh(ETakeShape::Cylinder));
	Body->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	Body->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.8f));

	// The light beam: a thin cylinder StepDevice stretches from the lamp to the target; hidden until fired.
	BeamShape = FTakeShapes::CreateShape(this, TEXT("BeamShape"), ETakeShape::Cylinder, Root, FVector(0.f, 0.f, 80.f), FVector(0.3f, 0.3f, 1.f), FRotator::ZeroRotator, false);
	BeamShape->SetVisibility(false);

	// Step 2, the look: the real light, off until the spot is fired (UpdateBeam aims and switches it).
	Light = CreateDefaultSubobject<USpotLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(4000.f);
	Light->SetOuterConeAngle(7.f);
	Light->SetInnerConeAngle(4.f);
	Light->SetAttenuationRadius(4500.f);
	Light->SetCastShadows(true);
	Light->SetVisibility(false);
}

void AFollowSpot::BeginPlay()
{
	Super::BeginPlay();

	Target = nullptr;
	BusySecondsLeft = 0.f;
	if (BeamShape)
	{
		FTakeShapes::SetColour(BeamShape, BeamColour);
		BeamShape->SetVisibility(false);
	}

	// Step 2, the look: the real light from the tunables, off until the spot is fired.
	if (Light)
	{
		const float Cone = FMath::Clamp(LightConeDegrees, 1.f, 80.f);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(FMath::Max(0.f, LightCandelas));
		Light->SetOuterConeAngle(Cone);
		Light->SetInnerConeAngle(Cone * 0.6f);
		Light->SetAttenuationRadius(FMath::Max(1.f, LightRadius));
		Light->SetLightColor(BeamColour);
		Light->SetVisibility(false);
	}
}

bool AFollowSpot::CanFire(FString& OutWhyNot) const
{
	// Idle: it can fire (which watcher is FindUseTarget's and FireAt's business). Busy: the countdown.
	if (bRunning)
	{
		OutWhyNot = FString::Printf(TEXT("%s - busy %.1f s"), *DeviceName, BusySecondsLeft);
		return false;
	}
	return Super::CanFire(OutWhyNot);
}

void AFollowSpot::StepDevice(float DeltaSeconds)
{
	if (!bRunning)
	{
		return;
	}

	RunSeconds += DeltaSeconds;
	BusySecondsLeft -= DeltaSeconds;
	if (BusySecondsLeft <= 0.f || !IsValid(Target))
	{
		BusySecondsLeft = 0.f;
		bRunning = false;
		UpdateBeam();	// idle: the beam shape and the light go off this frame
		TAKE_LOG(TEXT("SPOT READY"));
		return;
	}

	UpdateBeam();
}

void AFollowSpot::GatherTargets(const TArray<TObjectPtr<AWatcher>>& All, TArray<AWatcher*>& Out) const
{
	Out.Reset();
	if (IsValid(Target))
	{
		Out.Add(Target.Get());
	}
}

FVector AFollowSpot::GetEffectPoint() const
{
	return IsValid(Target) ? Target->GetActorLocation() : GetActorLocation();
}

FString AFollowSpot::GetPipsText() const
{
	if (bRunning)
	{
		return FString::Printf(TEXT("%s  BUSY %.1f"), *DeviceName, BusySecondsLeft);
	}
	if (Uses > 0 && UsesLeft <= 0)
	{
		return DeviceName + TEXT("  -");
	}
	return DeviceName + TEXT("  READY");
}

bool AFollowSpot::FireAt(AWatcher* Watcher, FString& OutWhyNot)
{
	if (!IsValid(Watcher))
	{
		OutWhyNot = TEXT("Look at a watcher");
		return false;
	}
	if (bRunning)
	{
		OutWhyNot = FString::Printf(TEXT("%s - busy %.1f s"), *DeviceName, BusySecondsLeft);
		return false;
	}
	if (Uses > 0 && UsesLeft <= 0)
	{
		OutWhyNot = DeviceName + TEXT(" - used");
		return false;
	}
	if (Watcher->DistanceToPoint(GetActorLocation()) > Range)
	{
		OutWhyNot = DeviceName + TEXT(" - out of range");
		return false;
	}

	if (Uses > 0)
	{
		UsesLeft = FMath::Max(UsesLeft - 1, 0);
	}
	OutWhyNot.Reset();
	Target = Watcher;
	BusySecondsLeft = Duration;
	bRunning = true;
	RunSeconds = 0.f;
	UpdateBeam();	// the beam shows from the lamp to the target this frame
	MarkLanded();	// the Blind lands in StepDevices this frame

	TAKE_LOG(TEXT("SPOT -> %s"), *Watcher->GetLabel());
	return true;
}

AWatcher* AFollowSpot::GetTarget() const
{
	return bRunning ? Target.Get() : nullptr;
}

float AFollowSpot::GetBusySecondsLeft() const
{
	return bRunning ? BusySecondsLeft : 0.f;
}

void AFollowSpot::UpdateBeam()
{
	// Off (idle, no target, or nothing to stretch): the beam shape and the light are both hidden this frame.
	auto TurnOff = [this]()
	{
		if (BeamShape)
		{
			BeamShape->SetVisibility(false);
		}
		if (Light)
		{
			Light->SetVisibility(false);
		}
	};

	if (!bRunning || !IsValid(Target) || !Body)
	{
		TurnOff();
		return;
	}

	const FVector BeamStart = Body->GetComponentLocation();
	const FVector BeamEnd = Target->GetActorLocation() + FVector(0.f, 0.f, BeamTargetHeight);
	const FVector Span = BeamEnd - BeamStart;
	const double SpanLength = Span.Size();
	if (SpanLength < 1.0)
	{
		TurnOff();
		return;
	}

	const FVector BeamDir = Span / SpanLength;
	if (BeamShape)
	{
		BeamShape->SetWorldLocationAndRotation(BeamStart + Span * 0.5, FQuat::FindBetweenNormals(FVector::UpVector, BeamDir));
		BeamShape->SetWorldScale3D(FVector(BeamThickness, BeamThickness, SpanLength / 100.0));
		BeamShape->SetVisibility(true);
	}

	// The real light rides the beam just clear of the lamp's own body (inside it, the lamp would shadow its own
	// light), aimed at the target: a spot light shines along its X axis.
	if (Light)
	{
		const double Clearance = FMath::Min(static_cast<double>(Body->Bounds.SphereRadius) + 5.0, SpanLength * 0.5);
		const FVector LightAt = BeamStart + BeamDir * Clearance;
		Light->SetWorldLocationAndRotation(LightAt, BeamDir.Rotation());
		Light->SetVisibility(true);
	}
}
