// Owner: (u). Constructor by the header pass; every "// BODY: (u)" is a stub to fill.

#include "FollowSpot.h"

#include "Components/BoxComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TakeShapes.h"
#include "Watcher.h"

AFollowSpot::AFollowSpot()
{
	FireSound = TEXT("sfx_lamp_strike");	// step 6: the lamp strikes on (the searchlight and the follow spot)
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
		Light->SetVolumetricScatteringIntensity(FMath::Max(BeamHaze, 0.f));	// QA devices: the shaft in the haze
		Light->SetVisibility(false);
	}

	// QA devices: at rest, facing where the model was set.
	SwingYaw = 0.f;
	SwingAimYaw = 0.f;
}

bool AFollowSpot::CanFire(FString& OutWhyNot) const
{
	// Idle: it can fire (which watcher is FindUseTarget's and FireAt's business). Busy: the countdown.
	if (bRunning)
	{
		OutWhyNot = FString::Printf(TEXT("%s is busy for %.1f s"), *DeviceName, BusySecondsLeft);
		return false;
	}
	return Super::CanFire(OutWhyNot);
}

void AFollowSpot::StepDevice(float DeltaSeconds)
{
	StepLook(DeltaSeconds);		// QA devices: the pilot lamp
	StepSwing(DeltaSeconds);	// QA devices: the lamp head swings onto the target, and home when idle

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
		OutWhyNot = FString::Printf(TEXT("%s is busy for %.1f s"), *DeviceName, BusySecondsLeft);
		return false;
	}
	if (Uses > 0 && UsesLeft <= 0)
	{
		OutWhyNot = DeviceName + TEXT(" is used up");
		return false;
	}
	if (Watcher->DistanceToPoint(GetActorLocation()) > Range)
	{
		OutWhyNot = DeviceName + TEXT(" can't reach that far");
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

	// QA devices: the lamp swings a good part of the way onto the target on this frame (the rest over the next few).
	SwingAimYaw = GetAimSwing();
	SwingYaw = FMath::UnwindDegrees(SwingYaw + FMath::FindDeltaAngleDegrees(SwingYaw, SwingAimYaw)
		* FMath::Clamp(SwingFirstStep, 0.f, 1.f));
	if (HasModel() && bSwingModel)
	{
		PlaceModel(FQuat(FVector::UpVector, FMath::DegreesToRadians(SwingYaw)));
	}
	OnFiredLook();	// the lamp goes bright, Maya reaches for the handle

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

	// QA devices: with the model, the beam starts at its lens (turned with the swing); else at the box lamp.
	const bool bFromModel = HasModel();
	const FVector BeamStart = bFromModel ? GetLensPoint() : Body->GetComponentLocation();
	const FVector BeamEnd = Target->GetActorLocation() + FVector(0.f, 0.f, BeamTargetHeight);
	const FVector Span = BeamEnd - BeamStart;
	const double SpanLength = Span.Size();
	if (SpanLength < 1.0)
	{
		TurnOff();
		return;
	}

	// While the lamp is still swinging on, the beam points where the lens points: it sweeps onto the target.
	FVector BeamDir = Span / SpanLength;
	if (bFromModel && bSwingModel)
	{
		const float Behind = FMath::FindDeltaAngleDegrees(SwingAimYaw, SwingYaw);
		BeamDir = FQuat(FVector::UpVector, FMath::DegreesToRadians(Behind)).RotateVector(BeamDir);
	}
	if (BeamShape)
	{
		BeamShape->SetWorldLocationAndRotation(BeamStart + BeamDir * SpanLength * 0.5, FQuat::FindBetweenNormals(FVector::UpVector, BeamDir));
		BeamShape->SetWorldScale3D(FVector(BeamThickness, BeamThickness, SpanLength / 100.0));
		BeamShape->SetVisibility(bShowBeamShape);	// note 18: off, the light's shaft is the beam
	}

	// The real light rides the beam just clear of the lamp's own body (inside it, the lamp would shadow its own
	// light), aimed at the target: a spot light shines along its X axis.
	if (Light)
	{
		const double Clearance = bFromModel ? FMath::Min(8.0, SpanLength * 0.5)
			: FMath::Min(static_cast<double>(Body->Bounds.SphereRadius) + 5.0, SpanLength * 0.5);
		const FVector LightAt = BeamStart + BeamDir * Clearance;
		Light->SetWorldLocationAndRotation(LightAt, BeamDir.Rotation());
		Light->SetVisibility(true);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// QA devices (Nov 14): the swing
// ---------------------------------------------------------------------------------------------------------------------

float AFollowSpot::GetAimSwing() const
{
	if (!IsValid(Target))
	{
		return SwingYaw;
	}
	const FTransform& Frame = GetActorTransform();
	const FVector Anchor = Frame.TransformPosition(ModelOffset);
	const FVector Local = Frame.InverseTransformVectorNoScale(Target->GetActorLocation() - Anchor);
	if (Local.SizeSquared2D() < 1.0)
	{
		return SwingYaw;
	}
	const float Yaw = FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(Local.Y, Local.X)));
	return FMath::UnwindDegrees(Yaw - LensYaw);
}

void AFollowSpot::StepSwing(float DeltaSeconds)
{
	// Onto the target while busy (it may walk), home to rest when ready again.
	SwingAimYaw = (bRunning && IsValid(Target)) ? GetAimSwing() : 0.f;
	const float Delta = FMath::FindDeltaAngleDegrees(SwingYaw, SwingAimYaw);
	const float MaxStep = FMath::Max(SwingDegreesPerSecond, 1.f) * DeltaSeconds;
	const float Step = FMath::Clamp(Delta, -MaxStep, MaxStep);
	if (FMath::IsNearlyZero(Step, 0.01f))
	{
		return;
	}
	SwingYaw = FMath::UnwindDegrees(SwingYaw + Step);
	if (HasModel() && bSwingModel)
	{
		PlaceModel(FQuat(FVector::UpVector, FMath::DegreesToRadians(SwingYaw)));
	}
}

FVector AFollowSpot::GetLensPoint() const
{
	const FVector Turned = (HasModel() && bSwingModel)
		? FQuat(FVector::UpVector, FMath::DegreesToRadians(SwingYaw)).RotateVector(LensOffset) : LensOffset;
	return GetActorTransform().TransformPosition(ModelOffset + Turned);
}
