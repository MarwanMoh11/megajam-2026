// Owner: (u). Constructor by the header pass; every "// BODY: (u)" is a stub to fill.

#include "Device.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreGlobals.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TakeCharacter.h"
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

	// QA devices: the period model (empty until ApplyLook loads ModelMesh). NoCollision: no sight trace or camera
	// probe ever hits it, and it never affects navigation.
	Model = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Model"));
	Model->SetupAttachment(Root);
	Model->SetMobility(EComponentMobility::Movable);
	Model->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Model->SetCollisionResponseToAllChannels(ECR_Ignore);
	Model->SetGenerateOverlapEvents(false);
	Model->SetCanEverAffectNavigation(false);
	Model->SetVisibility(false);

	// The pilot lamp (a small unlit sphere) and its warm pool (a small point light, no shadows, no haze).
	PilotLamp = FTakeShapes::CreateShape(this, TEXT("PilotLamp"), ETakeShape::Sphere, Root, FVector(0.f, 0.f, 110.f),
		FVector(0.05f), FRotator::ZeroRotator, false);
	PilotPool = CreateDefaultSubobject<UPointLightComponent>(TEXT("PilotPool"));
	PilotPool->SetupAttachment(Root);
	PilotPool->SetMobility(EComponentMobility::Movable);
	PilotPool->SetCastShadows(false);
	PilotPool->SetIntensityUnits(ELightUnits::Candelas);
	PilotPool->SetIntensity(0.f);
	PilotPool->SetAttenuationRadius(260.f);
	PilotPool->SetVolumetricScatteringIntensity(0.f);
	PilotPool->SetVisibility(false);

	// The lever: a pivot, a bar along its +Z and a knob at the bar's end. Hidden unless bShowHandle.
	HandlePivot = CreateDefaultSubobject<USceneComponent>(TEXT("HandlePivot"));
	HandlePivot->SetupAttachment(Root);
	HandleBar = FTakeShapes::CreateShape(this, TEXT("HandleBar"), ETakeShape::Cylinder, HandlePivot, FVector(0.f, 0.f, 20.f),
		FVector(0.03f, 0.03f, 0.4f), FRotator::ZeroRotator, false);
	HandleKnob = FTakeShapes::CreateShape(this, TEXT("HandleKnob"), ETakeShape::Sphere, HandlePivot, FVector(0.f, 0.f, 40.f),
		FVector(0.07f), FRotator::ZeroRotator, false);
	HandleBar->SetVisibility(false);
	HandleKnob->SetVisibility(false);

	// The rope or cable (a thin cylinder from RopeFrom to RopeTo). Hidden unless bShowRope.
	Rope = FTakeShapes::CreateShape(this, TEXT("Rope"), ETakeShape::Cylinder, Root, FVector::ZeroVector,
		FVector(0.025f, 0.025f, 1.f), FRotator::ZeroRotator, false);
	Rope->SetVisibility(false);
}

void ADevice::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LayOutUseRing();
	ApplyLook();
}

FVector ADevice::GetUsePoint() const
{
	return GetActorTransform().TransformPosition(UseOffset);
}

void ADevice::SetInReach(bool bInReach)
{
	bIsInReach = bInReach;
	if (UseRing && bShowUseRing)
	{
		FTakeShapes::SetColour(UseRing, bIsInReach ? RingReachColour : RingIdleColour);
	}
	// The pilot lamp and its pool, this frame.
	SetPilotGlow(PilotGlowNow());
}

void ADevice::LayOutUseRing()
{
	if (!UseRing)
	{
		return;
	}
	// Note 18: no drawn marks on the floor. The ring stays a component (off) so the switch can bring it back.
	UseRing->SetVisibility(bShowUseRing);
	if (!bShowUseRing)
	{
		UseRing->ClearInstances();
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

	// The model, the pilot (dim), the lever at rest, the rope.
	HandleSpin = 0.f;
	PilotClock = 0.f;
	ApplyLook();
	RefreshBodyLook();
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

	OnFiredLook();	// the lever jumps, the lamp goes bright, the user reaches for it: this frame
	TAKE_LOG(TEXT("FIRE %s"), *DeviceName);
	return true;
}

void ADevice::StepDevice(float DeltaSeconds)
{
	StepLook(DeltaSeconds);

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
	// A model, or a generated mesh put on the body (tools/rodin/import_props.py swap_bodies), keeps its own material:
	// recolouring it flat is what lost the carts' and the desk's textures.
	if (!Body || !BodyIsBox())
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
	// QA devices: a restored spent device is dark, its lever thrown, at once.
	HandleAlpha = IsHandleThrown() ? 1.f : 0.f;
	PoseHandle();
	SetPilotGlow(PilotGlowNow());
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

// ---------------------------------------------------------------------------------------------------------------------
// QA devices (Nov 14, notes 10, 17, 18): the period model, the pilot lamp, the lever, the rope
// ---------------------------------------------------------------------------------------------------------------------

void ADevice::ApplyLook()
{
	// The model: ModelMesh on Model with the mesh's own material; the box hides behind it (or stays: the banner).
	UStaticMesh* const Mesh = (bUseModel && !ModelMesh.IsNull()) ? ModelMesh.LoadSynchronous() : nullptr;
	bHasModel = Model && Mesh;
	if (Model)
	{
		Model->SetStaticMesh(Mesh);
		Model->EmptyOverrideMaterials();
		Model->SetVisibility(bHasModel);
		PlaceModel();
	}
	// Interactable props: the old click box (unused by the input, kept for its trace) drew as a big orange wire box
	// over the model in the editor: it draws only when selected now.
	if (ClickVolume && ClickVolume->bDrawOnlyIfSelected != bHasModel)
	{
		ClickVolume->bDrawOnlyIfSelected = bHasModel;
		ClickVolume->MarkRenderStateDirty();
	}
	if (Body)
	{
		Body->SetVisibility(!(bHasModel && bModelHidesBody));
		// A generated mesh put straight on the body keeps its own material, not the box's BasicShapeMaterial.
		if (!BodyIsBox() && !(bHasModel && bModelHidesBody))
		{
			Body->EmptyOverrideMaterials();
		}
	}

	// The pilot lamp: unlit, so its colour times its glow is what shows (and blooms when bright).
	if (PilotLamp)
	{
		PilotLamp->SetRelativeLocation(PilotOffset);
		PilotLamp->SetRelativeScale3D(FVector(FMath::Max(PilotSize, 0.5f) / 100.f));
		PilotLamp->SetVisibility(bShowPilot && PilotSize > 0.f);	// PilotSize 0: the pool alone (the carts)
		if (bShowPilot)
		{
			UMaterialInterface* const Signal = FTakeShapes::GetSignalMaterial();
			PilotMID = Cast<UMaterialInstanceDynamic>(PilotLamp->GetMaterial(0));
			if (Signal && (!PilotMID || PilotMID->Parent != Signal))
			{
				PilotMID = UMaterialInstanceDynamic::Create(Signal, this);
				PilotLamp->SetMaterial(0, PilotMID);
			}
		}
	}
	if (PilotPool)
	{
		PilotPool->SetRelativeLocation(PilotOffset);
		PilotPool->SetIntensityUnits(ELightUnits::Candelas);
		PilotPool->SetAttenuationRadius(FMath::Max(PoolRadius, 10.f));
		PilotPool->SetLightColor(PilotColour);
	}
	PilotGlowShown = -1.f;	// the next SetPilotGlow always applies
	SetPilotGlow(PilotGlowNow());

	// The lever: bar along the pivot's +Z, knob at its end, at rest (or thrown, if spent).
	const float BarLength = FMath::Max(HandleLength, 1.f);
	if (HandleBar)
	{
		const float Thick = FMath::Max(HandleThickness, 0.2f) / 100.f;
		HandleBar->SetRelativeLocation(FVector(0.f, 0.f, BarLength * 0.5f));
		HandleBar->SetRelativeScale3D(FVector(Thick, Thick, BarLength / 100.f));
		HandleBar->SetVisibility(bShowHandle);
		if (bShowHandle)
		{
			FTakeShapes::SetColour(HandleBar, HandleColour);
		}
	}
	if (HandleKnob)
	{
		HandleKnob->SetRelativeLocation(FVector(0.f, 0.f, BarLength));
		HandleKnob->SetRelativeScale3D(FVector(FMath::Max(HandleKnobSize, 0.5f) / 100.f));
		HandleKnob->SetVisibility(bShowHandle);
		if (bShowHandle)
		{
			FTakeShapes::SetColour(HandleKnob, HandleKnobColour);
		}
	}
	HandleAlpha = IsHandleThrown() ? 1.f : 0.f;
	PoseHandle();

	// The rope: a thin cylinder from RopeFrom to RopeTo.
	if (Rope)
	{
		const FVector Span = RopeTo - RopeFrom;
		const double Length = Span.Size();
		const bool bRope = bShowRope && Length > 1.0;
		Rope->SetVisibility(bRope);
		if (bRope)
		{
			const float Thick = FMath::Max(RopeThickness, 0.2f) / 100.f;
			Rope->SetRelativeLocationAndRotation((RopeFrom + RopeTo) * 0.5, FQuat::FindBetweenNormals(FVector::UpVector, Span / Length));
			Rope->SetRelativeScale3D(FVector(Thick, Thick, Length / 100.0));
			FTakeShapes::SetColour(Rope, RopeColour);
		}
	}
}

void ADevice::PlaceModel(const FQuat& ExtraTurn, const FVector& ExtraMove)
{
	const UStaticMesh* const Mesh = (Model && bHasModel) ? Model->GetStaticMesh() : nullptr;
	if (!Mesh)
	{
		return;
	}
	// The anchor is the floor point under the middle of the mesh's footprint, whatever pivot the import gave it.
	const FBox Box = Mesh->GetBoundingBox();
	const FVector Anchor(Box.GetCenter().X, Box.GetCenter().Y, Box.Min.Z);
	const FQuat Turn = ExtraTurn * ModelRotation.Quaternion();
	Model->SetRelativeScale3D(ModelScale);
	Model->SetRelativeLocationAndRotation(ModelOffset + ExtraMove - Turn.RotateVector(Anchor * ModelScale), Turn);
}

void ADevice::StepLook(float DeltaSeconds)
{
	PilotClock += DeltaSeconds;
	SetPilotGlow(PilotGlowNow());

	// The press jolt rocks the model about its foot and settles.
	if (JoltClock >= 0.f)
	{
		JoltClock += DeltaSeconds;
		if (JoltClock >= FMath::Max(PressJoltSeconds, 0.01f))
		{
			JoltClock = -1.f;
		}
		PlaceModel(GetJoltTurn());
	}

	if (!bShowHandle)
	{
		return;
	}
	const float Target = IsHandleThrown() ? 1.f : 0.f;
	HandleAlpha = FMath::FInterpConstantTo(HandleAlpha, Target, DeltaSeconds, 1.f / FMath::Max(HandleThrowSeconds, 0.01f));
	if (HandleSpinDegrees != 0.f && IsHandleTurning())
	{
		HandleSpin = FMath::Fmod(HandleSpin + HandleSpinDegrees * HandleSpinSign() * DeltaSeconds, 360.f);
	}
	PoseHandle();
}

void ADevice::OnFiredLook()
{
	// The lever is already partly thrown (a crank a quarter turn on) on the press frame; the model jolts.
	if (PressJoltDegrees > 0.f && bHasModel)
	{
		JoltClock = 1.f / 60.f;
		PlaceModel(GetJoltTurn());
	}
	if (bShowHandle)
	{
		HandleAlpha = FMath::Max(HandleAlpha, FMath::Clamp(HandleFirstStep, 0.f, 1.f));
		if (HandleSpinDegrees != 0.f)
		{
			HandleSpin = FMath::Fmod(HandleSpin + FMath::Sign(HandleSpinDegrees) * 30.f, 360.f);
		}
		PoseHandle();
	}
	SetPilotGlow(PilotGlowNow());

	// The crew member reaches for it (the cast's gesture; it plays from this frame).
	if (!UseGesture.IsNone())
	{
		const ATakeDirector* const Director = ATakeDirector::Get(this);
		if (ATakeCharacter* const User = Director ? Director->GetCharacter(FiredFrom) : nullptr)
		{
			User->PlayGesture(UseGesture);
		}
	}
}

bool ADevice::IsHandleThrown() const
{
	return bRunning || (bHandleStaysThrownWhenSpent && Uses > 0 && UsesLeft <= 0);
}

bool ADevice::BodyIsBox() const
{
	if (!Body || (bHasModel && bModelHidesBody))
	{
		return false;
	}
	const UStaticMesh* const Mesh = Body->GetStaticMesh();
	return Mesh && (Mesh == FTakeShapes::GetMesh(ETakeShape::Cube) || Mesh == FTakeShapes::GetMesh(ETakeShape::Cylinder)
		|| Mesh == FTakeShapes::GetMesh(ETakeShape::Sphere) || Mesh == FTakeShapes::GetMesh(ETakeShape::Cone)
		|| Mesh == FTakeShapes::GetMesh(ETakeShape::Plane));
}

float ADevice::PilotGlowNow() const
{
	if (!bShowPilot)
	{
		return 0.f;
	}
	const bool bSpent = Uses > 0 && UsesLeft <= 0;
	if ((bSpent && !bRunning) || IsLocked())
	{
		return 0.f;
	}
	if (bRunning)
	{
		if (PilotRunFlickerHz > 0.f)
		{
			const float Wave = FMath::Abs(FMath::Sin(PilotClock * PilotRunFlickerHz * UE_PI));
			return PilotReachGlow * (0.45f + 0.55f * Wave);
		}
		return PilotReachGlow;
	}
	return bIsInReach ? PilotReachGlow : PilotIdleGlow;
}

void ADevice::SetPilotGlow(float Glow)
{
	if (FMath::Abs(Glow - PilotGlowShown) < 0.01f)
	{
		return;
	}
	PilotGlowShown = Glow;

	const FLinearColor Lit(PilotColour.R * Glow, PilotColour.G * Glow, PilotColour.B * Glow, 1.f);
	if (PilotMID)
	{
		PilotMID->SetVectorParameterValue(FTakeShapes::ColorParam, Lit);
	}
	else if (PilotLamp && bShowPilot)
	{
		FTakeShapes::SetColour(PilotLamp, Lit);	// no signal material: the lit basic material, as near as it gets
	}

	if (PilotPool)
	{
		const float Span = FMath::Max(PilotReachGlow - PilotIdleGlow, 0.01f);
		const float Alpha = FMath::Clamp((Glow - PilotIdleGlow) / Span, 0.f, 1.f);
		const float Candelas = Glow > 0.f ? FMath::Lerp(PoolIdleCandelas, PoolReachCandelas, Alpha) : 0.f;
		PilotPool->SetIntensity(Candelas);
		PilotPool->SetVisibility(bShowPilot && Candelas > 0.f);
	}
}

void ADevice::PoseHandle()
{
	if (!HandlePivot)
	{
		return;
	}
	// Interactable props: eased (out-back): fast off the mark, a little past the stop, settles.
	const float A = FMath::Clamp(HandleAlpha, 0.f, 1.f);
	// (Out-back's 1.70158 overshoots by 10 %.) Coming back to rest it eases in and out, no bounce.
	const float C1 = 17.0158f * FMath::Clamp(HandleOvershoot, 0.f, 1.f);
	const float Eased = IsHandleThrown() ? 1.f + (C1 + 1.f) * FMath::Pow(A - 1.f, 3.f) + C1 * FMath::Square(A - 1.f)
		: A * A * (3.f - 2.f * A);
	const FRotator Thrown(HandleThrow.Pitch * Eased, HandleThrow.Yaw * Eased, HandleThrow.Roll * Eased);
	const FQuat Turn = HandleRest.Quaternion() * Thrown.Quaternion()
		* FQuat(FVector::XAxisVector, FMath::DegreesToRadians(HandleSpin));
	HandlePivot->SetRelativeLocationAndRotation(HandleOffset, Turn);
}

FQuat ADevice::GetJoltTurn() const
{
	const float Span = FMath::Max(PressJoltSeconds, 0.01f);
	if (JoltClock < 0.f || JoltClock >= Span || PressJoltDegrees <= 0.f)
	{
		return FQuat::Identity;
	}
	// Two and a half swings, dying away; about the model's own side (its Y), so it rocks toward its user and back.
	const float T = JoltClock / Span;
	const float Angle = PressJoltDegrees * FMath::Sin(T * UE_TWO_PI * 2.5f) * FMath::Square(1.f - T);
	const FVector Side = ModelRotation.Quaternion().GetAxisY();
	return FQuat(Side, FMath::DegreesToRadians(Angle));
}
