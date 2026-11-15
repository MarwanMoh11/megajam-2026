// Owner: (u). W3 spot (Nov 15, item 3): the lamp swings onto its target with weight (a spring: it eases in, overshoots a
// little and settles), tilts about its trunnion, carries its lever, and its beam is traced: the shaft and the light end
// at the first solid thing, the light casts shadows, and a soft glow lands where it hits.

#include "FollowSpot.h"

#include "CollisionQueryParams.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TakeCharacter.h"
#include "TakeDirector.h"
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

	// The old drawn beam (bShowBeamShape): a thin cylinder from the lamp to the beam's end; hidden by default.
	BeamShape = FTakeShapes::CreateShape(this, TEXT("BeamShape"), ETakeShape::Cylinder, Root, FVector(0.f, 0.f, 80.f), FVector(0.3f, 0.3f, 1.f), FRotator::ZeroRotator, false);
	BeamShape->SetVisibility(false);

	// The real light, off until the spot is fired (UpdateBeam aims and switches it). A hero light: it casts shadows.
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

	// W3 spot: the glow where the beam lands (small, no shadows, no haze).
	HitGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("HitGlow"));
	HitGlow->SetupAttachment(Root);
	HitGlow->SetMobility(EComponentMobility::Movable);
	HitGlow->SetCastShadows(false);
	HitGlow->SetIntensityUnits(ELightUnits::Candelas);
	HitGlow->SetIntensity(0.f);
	HitGlow->SetAttenuationRadius(260.f);
	HitGlow->SetVolumetricScatteringIntensity(0.f);
	HitGlow->SetVisibility(false);

	// W3 spot: the glowing lens (unlit disc; BeginPlay gives it the signal material).
	LensDisc = FTakeShapes::CreateShape(this, TEXT("LensDisc"), ETakeShape::Cylinder, Root, FVector(0.f, 0.f, 80.f),
		FVector(0.3f, 0.3f, 0.01f), FRotator::ZeroRotator, false);
	LensDisc->SetCastShadow(false);
	LensDisc->SetVisibility(false);
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

	// The real light from the tunables, off until the spot is fired.
	if (Light)
	{
		const float Cone = FMath::Clamp(LightConeDegrees, 1.f, 80.f);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(FMath::Max(0.f, LightCandelas));
		Light->SetOuterConeAngle(Cone);
		Light->SetInnerConeAngle(Cone * 0.6f);
		Light->SetAttenuationRadius(FMath::Max(1.f, LightRadius));
		Light->SetLightColor(BeamColour);
		Light->SetVolumetricScatteringIntensity(FMath::Max(BeamHaze, 0.f));	// the shaft in the haze
		Light->SetCastShadows(bBeamShadows);
		Light->SetCastVolumetricShadow(bBeamShadows);	// the shaft stops at walls too
		Light->SetVisibility(false);
	}
	if (HitGlow)
	{
		HitGlow->SetLightColor(BeamColour);
		HitGlow->SetIntensity(FMath::Max(HitGlowCandelas, 0.f));
		HitGlow->SetAttenuationRadius(FMath::Max(HitGlowRadius, 10.f));
		HitGlow->SetVisibility(false);
	}

	if (LensDisc)
	{
		LensDisc->SetVisibility(false);
		if (UMaterialInterface* const Signal = FTakeShapes::GetSignalMaterial())
		{
			UMaterialInstanceDynamic* const Glow = UMaterialInstanceDynamic::Create(Signal, this);
			Glow->SetVectorParameterValue(FTakeShapes::ColorParam, BeamColour * FMath::Max(LensGlow, 0.f));
			LensDisc->SetMaterial(0, Glow);
		}
		else
		{
			FTakeShapes::SetColour(LensDisc, BeamColour);
		}
	}
	if (FogApexBias > 0.f)
	{
		if (IConsoleVariable* const Bias = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog.InverseSquaredLightDistanceBiasScale")))
		{
			Bias->Set(FogApexBias, ECVF_SetByCode);
		}
	}

	// At rest, facing where the model was set.
	SwingYaw = SwingAimYaw = SwingPitch = SwingAimPitch = 0.f;
	SwingYawSpeed = SwingPitchSpeed = 0.f;
	bBeamOn = false;
	BeamHitActor.Reset();
	BeamEndPoint = GetActorLocation();
	PoseSwing();
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
	StepLook(DeltaSeconds);		// the pilot lamp (and the lever at its rest pose: StepSwing carries it)
	StepSwing(DeltaSeconds);	// the lamp head swings onto the target, and home when idle

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
		UpdateBeam();	// idle: the beam and the lights go off this frame
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

	// The shove: the lamp is already moving toward the target this frame (StepDevice takes this frame's step); the
	// spring carries it on, a little past, and back.
	SwingAimYaw = GetAimSwing();
	SwingAimPitch = GetAimPitch();
	const float Top = FMath::Max(SwingDegreesPerSecond, 1.f);
	const float Kick = FMath::Clamp(SwingFirstStep, 0.f, 1.f) * Top;
	auto Shove = [Kick](float Now, float Aim, float& Speed, bool bWrap)
	{
		const float Delta = bWrap ? FMath::FindDeltaAngleDegrees(Now, Aim) : Aim - Now;
		const float Want = FMath::Sign(Delta) * Kick * FMath::Min(1.f, FMath::Abs(Delta) / 30.f);
		if (FMath::Abs(Want) > FMath::Abs(Speed) || FMath::Sign(Speed) != FMath::Sign(Want))
		{
			Speed = Want;
		}
	};
	Shove(SwingYaw, SwingAimYaw, SwingYawSpeed, true);
	Shove(SwingPitch, SwingAimPitch, SwingPitchSpeed, false);
	OnFiredLook();	// the pilot goes bright, Maya reaches for the handle
	PoseSwing();

	UpdateBeam();	// the beam strikes on this frame, along the lamp
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

FVector AFollowSpot::GetBeamEnd() const
{
	return bBeamOn ? BeamEndPoint : GetLensPoint();
}

AActor* AFollowSpot::GetBeamHitActor() const
{
	return bBeamOn ? BeamHitActor.Get() : nullptr;
}

void AFollowSpot::UpdateBeam()
{
	// Off (idle, no target): the beam shape and both lights are hidden this frame.
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
		if (HitGlow)
		{
			HitGlow->SetVisibility(false);
		}
		if (LensDisc)
		{
			LensDisc->SetVisibility(false);
		}
		bBeamOn = false;
		BeamHitActor.Reset();
	};

	if (!bRunning || !IsValid(Target) || !Body)
	{
		TurnOff();
		return;
	}

	// With the model, the beam starts at its lens (turned with the swing); else at the box lamp.
	const bool bFromModel = HasModel();
	const FVector BeamStart = bFromModel ? GetLensPoint() : Body->GetComponentLocation();
	const FVector AimPoint = Target->GetActorLocation() + FVector(0.f, 0.f, BeamTargetHeight);
	const FVector ToAim = AimPoint - BeamStart;
	const double AimLength = ToAim.Size();
	if (AimLength < 1.0)
	{
		TurnOff();
		return;
	}

	// The beam points where the lamp points: while it is still swinging it sweeps onto the target (and a little past).
	FVector BeamDir = ToAim / AimLength;
	if (bFromModel && bSwingModel)
	{
		const FRotator Exact = BeamDir.Rotation();
		const float ErrYaw = FMath::FindDeltaAngleDegrees(SwingAimYaw, SwingYaw);
		const float ErrPitch = SwingPitch - SwingAimPitch;
		BeamDir = FRotator(FMath::Clamp(Exact.Pitch + ErrPitch, -89.f, 89.f), Exact.Yaw + ErrYaw, 0.f).Vector();
	}
	const bool bOnTarget = FVector::DotProduct(BeamDir, ToAim / AimLength) > FMath::Cos(FMath::DegreesToRadians(2.5f));

	// The trace: the first solid thing along the beam. Hidden collision (click volumes, ghost shapes) is passed
	// through, except a watcher's: a person stops the beam. The user and the lamp itself never do.
	FVector End = BeamStart + BeamDir * FMath::Max(LightRadius, 100.f);
	AActor* Hit = nullptr;
	if (bBeamStopsAtHit)
	{
		UWorld* const World = GetWorld();
		FCollisionQueryParams Params(SCENE_QUERY_STAT(FollowSpotBeam), false, this);
		const ATakeDirector* const Director = ATakeDirector::Get(this);
		if (const ATakeCharacter* const User = Director ? Director->GetCharacter(FiredFrom) : nullptr)
		{
			Params.AddIgnoredActor(User);
		}
		const FVector From = BeamStart + BeamDir * FMath::Clamp(static_cast<double>(BeamNearSkip), 10.0, AimLength * 0.5);
		for (int32 Try = 0; World && Try < 6; ++Try)
		{
			FHitResult Result;
			if (!World->LineTraceSingleByChannel(Result, From, End, ECC_Visibility, Params))
			{
				break;
			}
			AActor* const What = Result.GetActor();
			UPrimitiveComponent* const Part = Result.GetComponent();
			const bool bHiddenPart = Part && (Part->bHiddenInGame || !Part->IsVisible());
			if (bHiddenPart && !Cast<AWatcher>(What))
			{
				Params.AddIgnoredComponent(Part);
				continue;
			}
			End = Result.ImpactPoint;
			Hit = What;
			break;
		}
		// Settled on the target and nothing hit before it (its body has no collision there): the beam lands on it.
		const double EndDistance = (End - BeamStart).Size();
		if (bOnTarget && (Hit == nullptr || EndDistance > AimLength + 60.0))
		{
			End = BeamStart + BeamDir * AimLength;
			Hit = Target.Get();
		}
	}
	else
	{
		End = AimPoint;
		Hit = Target.Get();
	}
	const double Length = FMath::Max((End - BeamStart).Size(), 1.0);

	if (Hit != BeamHitActor.Get())
	{
		TAKE_LOG(TEXT("SPOT beam lands on %s"), Hit ? *Hit->GetName() : TEXT("nothing"));
	}
	bBeamOn = true;
	BeamEndPoint = End;
	BeamHitActor = Hit;

	if (BeamShape)
	{
		BeamShape->SetWorldLocationAndRotation(BeamStart + BeamDir * Length * 0.5, FQuat::FindBetweenNormals(FVector::UpVector, BeamDir));
		BeamShape->SetWorldScale3D(FVector(BeamThickness, BeamThickness, Length / 100.0));
		BeamShape->SetVisibility(bShowBeamShape);	// note 18: off, the light's shaft is the beam
	}

	// The real light rides the beam just clear of the lamp's own body (inside it, the lamp would shadow its own
	// light), aimed along it: a spot light shines along its X axis. It reaches just past the end (the shaft stops).
	if (Light)
	{
		const double Clearance = bFromModel ? FMath::Min(static_cast<double>(FMath::Max(LightClearance, 0.f)), Length * 0.5)
			: FMath::Min(static_cast<double>(Body->Bounds.SphereRadius) + 5.0, Length * 0.5);
		Light->SetWorldLocationAndRotation(BeamStart + BeamDir * Clearance, BeamDir.Rotation());
		const float Reach = bBeamStopsAtHit ? static_cast<float>(Length) + FMath::Max(BeamSpill, 0.f) : LightRadius;
		Light->SetAttenuationRadius(FMath::Clamp(Reach, 50.f, FMath::Max(LightRadius, 50.f)));
		Light->SetVisibility(true);
	}
	if (LensDisc)
	{
		const bool bDisc = bFromModel && LensGlowSize > 0.f;
		if (bDisc)
		{
			const float Wide = LensGlowSize / 100.f;
			LensDisc->SetWorldLocationAndRotation(BeamStart, FQuat::FindBetweenNormals(FVector::UpVector, BeamDir));
			LensDisc->SetWorldScale3D(FVector(Wide, Wide, 0.01f));
		}
		LensDisc->SetVisibility(bDisc);
	}
	if (HitGlow)
	{
		const bool bGlow = HitGlowCandelas > 0.f && Hit != nullptr;
		if (bGlow)
		{
			HitGlow->SetWorldLocation(End - BeamDir * FMath::Min(60.0, Length * 0.5));
		}
		HitGlow->SetVisibility(bGlow);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// The swing: a spring on yaw and pitch
// ---------------------------------------------------------------------------------------------------------------------

FVector AFollowSpot::GetTrunnionLocal() const
{
	const float Height = TrunnionHeight >= 0.f ? TrunnionHeight : LensOffset.Z;
	return ModelOffset + FVector(0.f, 0.f, Height);
}

float AFollowSpot::GetAimSwing() const
{
	if (!IsValid(Target))
	{
		return SwingYaw;
	}
	const FTransform& Frame = GetActorTransform();
	const FVector Pivot = Frame.TransformPosition(GetTrunnionLocal());
	const FVector Local = Frame.InverseTransformVectorNoScale(Target->GetActorLocation() - Pivot);
	if (Local.SizeSquared2D() < 1.0)
	{
		return SwingYaw;
	}
	const float Yaw = FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(Local.Y, Local.X)));
	return FMath::UnwindDegrees(Yaw - LensYaw);
}

float AFollowSpot::GetAimPitch() const
{
	if (!IsValid(Target))
	{
		return SwingPitch;
	}
	const FTransform& Frame = GetActorTransform();
	const FVector Pivot = Frame.TransformPosition(GetTrunnionLocal());
	const FVector Local = Frame.InverseTransformVectorNoScale(Target->GetActorLocation() + FVector(0.f, 0.f, BeamTargetHeight) - Pivot);
	const double Across = Local.Size2D();
	if (Across < 1.0)
	{
		return SwingPitch;
	}
	return FMath::Clamp(FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(Local.Z, Across))), -85.f, 85.f);
}

void AFollowSpot::StepSwing(float DeltaSeconds)
{
	// Onto the target while busy (it may walk), home to rest when ready again.
	const bool bAiming = bRunning && IsValid(Target);
	SwingAimYaw = bAiming ? GetAimSwing() : 0.f;
	SwingAimPitch = bAiming ? GetAimPitch() : 0.f;

	const float Omega = UE_TWO_PI * FMath::Max(SwingFrequencyHz, 0.05f);
	const float Zeta = FMath::Clamp(SwingDampingRatio, 0.05f, 3.f);
	const float Top = FMath::Max(SwingDegreesPerSecond, 1.f);
	const float Dt = FMath::Clamp(DeltaSeconds, 0.f, 0.25f);
	const int32 Steps = FMath::Clamp(FMath::CeilToInt(Dt * 120.f), 1, 30);
	const float H = Dt / static_cast<float>(Steps);
	for (int32 Step = 0; Step < Steps && H > 0.f; ++Step)
	{
		const float DeltaYaw = FMath::FindDeltaAngleDegrees(SwingYaw, SwingAimYaw);
		SwingYawSpeed = FMath::Clamp(SwingYawSpeed + (Omega * Omega * DeltaYaw - 2.f * Zeta * Omega * SwingYawSpeed) * H, -Top, Top);
		SwingYaw = FMath::UnwindDegrees(SwingYaw + SwingYawSpeed * H);

		const float DeltaPitch = SwingAimPitch - SwingPitch;
		SwingPitchSpeed = FMath::Clamp(SwingPitchSpeed + (Omega * Omega * DeltaPitch - 2.f * Zeta * Omega * SwingPitchSpeed) * H, -Top, Top);
		SwingPitch = FMath::Clamp(SwingPitch + SwingPitchSpeed * H, -89.f, 89.f);
	}
	// Settled: snap (no endless creep).
	if (FMath::Abs(FMath::FindDeltaAngleDegrees(SwingYaw, SwingAimYaw)) < 0.05f && FMath::Abs(SwingYawSpeed) < 0.5f)
	{
		SwingYaw = SwingAimYaw;
		SwingYawSpeed = 0.f;
	}
	if (FMath::Abs(SwingAimPitch - SwingPitch) < 0.05f && FMath::Abs(SwingPitchSpeed) < 0.5f)
	{
		SwingPitch = SwingAimPitch;
		SwingPitchSpeed = 0.f;
	}
	PoseSwing();
}

FQuat AFollowSpot::GetSwingTurn() const
{
	if (!(HasModel() && bSwingModel))
	{
		return FQuat::Identity;
	}
	const float Tilt = FMath::Clamp(SwingPitch, -FMath::Abs(MaxModelPitch), FMath::Abs(MaxModelPitch));
	// Yaw about the actor's up; the tilt about the lens's own side axis.
	return FRotator(Tilt, SwingYaw + LensYaw, 0.f).Quaternion() * FRotator(0.f, -LensYaw, 0.f).Quaternion();
}

void AFollowSpot::PoseSwing()
{
	const FQuat Turn = GetSwingTurn();
	const FVector Pivot = GetTrunnionLocal();
	if (HasModel() && bSwingModel)
	{
		// About the trunnion: the anchor moves by (pivot + turn(anchor - pivot)) - anchor.
		const FVector Move = Pivot + Turn.RotateVector(ModelOffset - Pivot) - ModelOffset;
		PlaceModel(Turn, Move);
	}
	// The operator's lever rides the lamp (its rest pose and throw are the base's; this carries it with the swing).
	if (HandlePivot && bShowHandle)
	{
		const FRotator Thrown(HandleThrow.Pitch * HandleAlpha, HandleThrow.Yaw * HandleAlpha, HandleThrow.Roll * HandleAlpha);
		const FQuat Pose = HandleRest.Quaternion() * Thrown.Quaternion() * FQuat(FVector::XAxisVector, FMath::DegreesToRadians(HandleSpin));
		HandlePivot->SetRelativeLocationAndRotation(Pivot + Turn.RotateVector(HandleOffset - Pivot), Turn * Pose);
	}
}

FVector AFollowSpot::GetLensPoint() const
{
	const FVector Pivot = GetTrunnionLocal();
	const FVector Lens = Pivot + GetSwingTurn().RotateVector(ModelOffset + LensOffset - Pivot);
	return GetActorTransform().TransformPosition(Lens);
}
