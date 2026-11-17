// Owner: (m). Constructor and trivial getters by the header pass; every "// BODY: (m)" is a stub to fill.

#include "TakeCharacter.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "CastMember.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Animation/BlendSpace.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Script/ScriptScene.h"
#include "Script/ScriptSequencer.h"
#include "TakeLocoAnim.h"
#include "TakePlayerController.h"

ATakeCharacter::ATakeCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	// Never possessed: the player controller drives it directly.
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	AutoPossessAI = EAutoPossessAI::Disabled;
	AIControllerClass = nullptr;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Collision: the capsule is the only thing on a character that collides: Pawn profile (blocks walls and floor,
	// ignored by every take prop, see FTakeShapes::SetupShape).
	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	CapsuleComp->InitCapsuleSize(42.f, 96.f);
	CapsuleComp->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bRunPhysicsWithNoController = true;
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, TurnRate, 0.f);
	Move->MaxWalkSpeed = WalkSpeed;
	Move->MinAnalogWalkSpeed = 20.f;
	Move->MaxAcceleration = WalkAcceleration;
	Move->BrakingDecelerationWalking = WalkBraking;
	Move->JumpZVelocity = 0.f;

	Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Boom"));
	Boom->SetupAttachment(RootComponent);
	Boom->SetUsingAbsoluteRotation(true);
	Boom->bUsePawnControlRotation = false;
	Boom->bInheritPitch = false;
	Boom->bInheritYaw = false;
	Boom->bInheritRoll = false;
	Boom->bDoCollisionTest = true;
	Boom->ProbeChannel = ECC_Camera;
	Boom->bEnableCameraLag = true;
	Boom->CameraLagSpeed = CameraLagSpeed;
	Boom->TargetArmLength = BoomLength;
	Boom->SocketOffset = ShoulderOffset;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(Boom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(CameraFOV);

	// The template mannequin and its unarmed animation (left empty if the project lacks them).
	USkeletalMeshComponent* MeshComp = GetMesh();
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMesh(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (MannyMesh.Succeeded())
	{
		MeshComp->SetSkeletalMeshAsset(MannyMesh.Object);
	}
	// chars-locomotion: the C++ body (UTakeLocoAnim: walk by measured speed, run, crouch) instead of ABP_Unarmed.
	static ConstructorHelpers::FObjectFinder<UBlendSpace> LocoSpace(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/BS_Idle_Walk_Run.BS_Idle_Walk_Run"));
	if (LocoSpace.Succeeded())
	{
		MeshComp->AnimationData.AnimToPlay = LocoSpace.Object;
		MeshComp->AnimationData.bSavedLooping = true;
		MeshComp->AnimationData.bSavedPlaying = true;
	}
	MeshComp->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	MeshComp->SetAnimInstanceClass(UTakeLocoAnim::StaticClass());
	MeshComp->SetRelativeLocation(FVector(0.f, 0.f, -CapsuleComp->GetUnscaledCapsuleHalfHeight()));
	MeshComp->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	MeshComp->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}

void ATakeCharacter::BeginPlay()
{
	Super::BeginPlay();

	// OnConstruction does not rerun for placed actors in play or in a package: apply the tunables again here.
	ApplyTunables();
	ApplyHeroMesh();
	// The follow camera's probe passes through people: only walls and props pull it in. Cole's capsule running up behind
	// Vance collapsed the boom into Vance's jacket in the escape (the trailer only turned the probe off while filming,
	// Reel.Trailer.ChaseCam; Marwan, Nov 17: filming fixes go in the game). ACastMember does the same for the cast.
	{
		TArray<UPrimitiveComponent*> Prims;
		GetComponents(Prims);
		for (UPrimitiveComponent* P : Prims)
		{
			P->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		}
	}
	ApplyTint();
	ApplyLocoAnim();
	// The run and crouch keys are read in Tick and Move: Tick first, then the movement component, same frame.
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->PrimaryComponentTick.AddPrerequisite(this, PrimaryActorTick);
	}
	SetLook(static_cast<float>(GetActorRotation().Yaw), StartLookPitch);
	TAKE_LOG(TEXT("%s on set, look %.0f / %.0f"), GetCrewName(), LookYaw, LookPitch);
}

void ATakeCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// The editor shows the tinted mannequin and the boom where the take starts it (behind him).
	ApplyTunables();
	ApplyHeroMesh();
	ApplyTint();
	SetLook(static_cast<float>(GetActorRotation().Yaw), StartLookPitch);
}

// Trailer filming only: with Reel.Trailer.ChaseCam 1 the follow camera sits behind whoever the player is watching and
// swings round with his run, and nothing between them pulls it in (the boom's probe put it inside Vance's jacket when
// a guard ran up behind him in the escape). Off, as shipped: the player's own look and the probing boom.
namespace TakeTrailerCam
{
	TAutoConsoleVariable<int32> CVarOn(TEXT("Reel.Trailer.ChaseCam"), 0, TEXT("1: the follow camera stays behind the watched character (trailer filming). 0: the game's own look."));
	TAutoConsoleVariable<float> CVarArm(TEXT("Reel.Trailer.ChaseArm"), 300.f, TEXT("Chase camera: cm behind him."));
	TAutoConsoleVariable<float> CVarPitch(TEXT("Reel.Trailer.ChasePitch"), -10.f, TEXT("Chase camera: pitch, degrees (negative looks down)."));
	TAutoConsoleVariable<float> CVarUp(TEXT("Reel.Trailer.ChaseUp"), 30.f, TEXT("Chase camera: cm above the boom's end."));
	TAutoConsoleVariable<float> CVarSide(TEXT("Reel.Trailer.ChaseSide"), 0.f, TEXT("Chase camera: cm to his right."));
	TAutoConsoleVariable<float> CVarYaw(TEXT("Reel.Trailer.ChaseYaw"), 0.f, TEXT("Chase camera: degrees round him from straight behind (180 looks at his face)."));
	TAutoConsoleVariable<float> CVarTurn(TEXT("Reel.Trailer.ChaseTurn"), 4.f, TEXT("Chase camera: how fast it swings round behind him (1/s)."));
	TWeakObjectPtr<ATakeCharacter> Holder;
}

void ATakeCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ReadGaitKeys();
	StepScriptWalk();
	ApplyGait();
	if (Boom)
	{
		const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		if (TakeTrailerCam::CVarOn.GetValueOnGameThread() > 0 && PC && PC->GetViewTarget() == this)
		{
			const FVector V = GetVelocity();
			const float Want = static_cast<float>(V.SizeSquared2D() > 3600. ? V.Rotation().Yaw : GetActorRotation().Yaw)
				+ TakeTrailerCam::CVarYaw.GetValueOnGameThread();
			const float A = FMath::Clamp(DeltaSeconds * TakeTrailerCam::CVarTurn.GetValueOnGameThread(), 0.f, 1.f);
			LookYaw = FMath::UnwindDegrees(LookYaw + FMath::FindDeltaAngleDegrees(LookYaw, Want) * A);
			LookPitch = FMath::Lerp(LookPitch, FMath::Clamp(TakeTrailerCam::CVarPitch.GetValueOnGameThread(), MinPitch, MaxPitch), A);
			Boom->bDoCollisionTest = false;
			Boom->TargetArmLength = TakeTrailerCam::CVarArm.GetValueOnGameThread();
			Boom->SocketOffset = FVector(0.f, TakeTrailerCam::CVarSide.GetValueOnGameThread(), TakeTrailerCam::CVarUp.GetValueOnGameThread());
			TakeTrailerCam::Holder = this;
		}
		else if (TakeTrailerCam::Holder.Get() == this)
		{
			TakeTrailerCam::Holder.Reset();
			Boom->bDoCollisionTest = true;
			ApplyTunables();
		}
	}
	ApplyLookToBoom();
}

void ATakeCharacter::AddLook(float YawDeg, float PitchDeg)
{
	LookYaw = FMath::UnwindDegrees(LookYaw + YawDeg);
	LookPitch = FMath::Clamp(LookPitch + PitchDeg, MinPitch, MaxPitch);
	ApplyLookToBoom();
}

void ATakeCharacter::SetLook(float Yaw, float Pitch)
{
	LookYaw = FMath::UnwindDegrees(Yaw);
	LookPitch = FMath::Clamp(Pitch, MinPitch, MaxPitch);
	ApplyLookToBoom();
}

void ATakeCharacter::SnapFollowCamera()
{
	if (!Boom || !Boom->IsRegistered())
	{
		return;
	}
	// The boom lags from where it last was. With the lag off, one zero-length tick puts it on the character.
	const bool bLagWasOn = Boom->bEnableCameraLag;
	Boom->bEnableCameraLag = false;
	ApplyLookToBoom();
	static_cast<UActorComponent*>(Boom.Get())->TickComponent(0.f, LEVELTICK_All, nullptr);
	Boom->bEnableCameraLag = bLagWasOn;
}

void ATakeCharacter::Move(const FVector2D& Axes)
{
	if (Axes.IsNearlyZero())
	{
		return;	// no input: he brakes at WalkBraking
	}
	// The player walks him: a scripted walk gives way. Shift / C are read now so the speed is right this frame.
	if (ScriptPath.Num() > 0)
	{
		StopScriptWalk();
	}
	ReadGaitKeys();
	ApplyGait();

	// Relative to the look's yaw only (pitch ignored), so W walks where the camera faces.
	const FRotationMatrix YawMatrix(FRotator(0.f, LookYaw, 0.f));
	const FVector Forward = YawMatrix.GetUnitAxis(EAxis::X);
	const FVector Right = YawMatrix.GetUnitAxis(EAxis::Y);
	const FVector Input = (Forward * Axes.X + Right * Axes.Y).GetClampedToMaxSize(1.f);
	AddMovementInput(Input, 1.f);
}

void ATakeCharacter::MoveToward(const FVector& WorldPoint)
{
	const FVector Direction = (WorldPoint - GetActorLocation()).GetSafeNormal2D();
	if (Direction.IsNearlyZero())
	{
		return;
	}
	AddMovementInput(Direction, 1.f);
}

bool ATakeCharacter::IsStanding() const
{
	return GetVelocity().Size2D() < StandSpeed;
}

FVector ATakeCharacter::GetFeetLocation() const
{
	const UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	return GetActorLocation() - FVector(0.f, 0.f, CapsuleComp ? CapsuleComp->GetScaledCapsuleHalfHeight() : 0.f);
}

float ATakeCharacter::GetCapsuleRadius() const
{
	const UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	return CapsuleComp ? CapsuleComp->GetScaledCapsuleRadius() : 0.f;
}

void ATakeCharacter::GetFollowView(FVector& OutLocation, FRotator& OutRotation, float& OutFOV) const
{
	OutLocation = FollowCamera ? FollowCamera->GetComponentLocation() : GetActorLocation();
	OutRotation = FollowCamera ? FollowCamera->GetComponentRotation() : GetActorRotation();
	OutFOV = FollowCamera ? FollowCamera->FieldOfView : CameraFOV;
}

FVector ATakeCharacter::GetAimOrigin() const
{
	return FollowCamera ? FollowCamera->GetComponentLocation() : GetActorLocation();
}

FVector ATakeCharacter::GetAimDirection() const
{
	return FollowCamera ? FollowCamera->GetForwardVector() : GetActorForwardVector();
}

void ATakeCharacter::ApplyTunables()
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bRunPhysicsWithNoController = true;
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->MaxWalkSpeed = WalkSpeed;
		MoveComp->MaxAcceleration = WalkAcceleration;
		MoveComp->BrakingDecelerationWalking = WalkBraking;
		// Without this the ground friction (8 x BrakingFrictionFactor 2) stops him dead and WalkBraking does little.
		MoveComp->bUseSeparateBrakingFriction = true;
		MoveComp->BrakingFriction = WalkBrakingFriction;
		MoveComp->RotationRate = FRotator(0.f, TurnRate, 0.f);
	}

	if (Boom)
	{
		Boom->TargetArmLength = BoomLength;
		Boom->SocketOffset = ShoulderOffset;
		Boom->bEnableCameraLag = true;
		Boom->bEnableCameraRotationLag = false;	// the mouse must feel direct
		Boom->CameraLagSpeed = CameraLagSpeed;
	}

	if (FollowCamera)
	{
		FollowCamera->SetFieldOfView(CameraFOV);
	}
}

void ATakeCharacter::ApplyTint()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp || !MeshComp->GetSkeletalMeshAsset() || !HeroMesh.IsNull())
	{
		return;
	}

	// CreateAndSetMaterialInstanceDynamic reuses a slot that already holds a MID, so this creates them once.
	TintMIDs.Reset();
	const int32 NumSlots = MeshComp->GetNumMaterials();
	for (int32 Slot = 0; Slot < NumSlots; ++Slot)
	{
		if (!MeshComp->GetMaterial(Slot))
		{
			continue;
		}
		if (UMaterialInstanceDynamic* MID = MeshComp->CreateAndSetMaterialInstanceDynamic(Slot))
		{
			MID->SetVectorParameterValue(TintParameter, Tint);
			TintMIDs.Add(MID);
		}
	}
}

// --- QA cast: acts and the hero mesh ------------------------------------------------------------------------------------

void ATakeCharacter::PlayGesture(FName Gesture)
{
	PlayGestureAt(Gesture, FVector::ZeroVector);
}

void ATakeCharacter::PlayGestureAt(FName Gesture, const FVector& WorldPoint)
{
	// His cast member (attached by tools/build_cast.py) plays it over his own animation.
	if (ACastMember* Member = ACastMember::FindOn(this))
	{
		Member->PlayGestureAt(Gesture, WorldPoint);
	}
}

void ATakeCharacter::ApplyHeroMesh()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}
	USkeletalMesh* Want = HeroMesh.IsNull() ? nullptr : HeroMesh.LoadSynchronous();
	if (!Want)
	{
		// Cleared (or not found): the class's own mannequin.
		const ATakeCharacter* Default = GetClass()->GetDefaultObject<ATakeCharacter>();
		Want = Default && Default->GetMesh() ? Default->GetMesh()->GetSkeletalMeshAsset() : nullptr;
	}
	if (Want && MeshComp->GetSkeletalMeshAsset() != Want)
	{
		MeshComp->SetSkeletalMeshAsset(Want);
		TintMIDs.Reset();
	}
	// --- W3 cast2 --- the mannequin's tint MIDs (or a Blueprint's override materials) outlive a mesh swap: on the hero
	// mesh they painted the mannequin's texture over the hero's UVs (Julian's grey "camouflage"). The hero wears its own.
	if (!HeroMesh.IsNull() && MeshComp->GetSkeletalMeshAsset() == Want && MeshComp->OverrideMaterials.Num() > 0)
	{
		MeshComp->EmptyOverrideMaterials();
		TintMIDs.Reset();
	}
}

void ATakeCharacter::ApplyLookToBoom()
{
	if (Boom)
	{
		Boom->SetWorldRotation(FRotator(LookPitch, LookYaw, 0.f));
	}
}

// --- chars-locomotion (Nov 15 evening): the body, scripted walks, run and crouch -----------------------------------------

void ATakeCharacter::ApplyLocoAnim()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp || !MeshComp->GetSkeletalMeshAsset())
	{
		return;
	}
	// A placed character (or a Blueprint child) may still carry ABP_Unarmed from before: put the C++ body on it.
	if (MeshComp->GetAnimationMode() == EAnimationMode::AnimationBlueprint && MeshComp->GetAnimClass() != UTakeLocoAnim::StaticClass())
	{
		if (UBlendSpace* Space = UTakeLocoAnim::LocoSpace())
		{
			MeshComp->AnimationData.AnimToPlay = Space;
			MeshComp->AnimationData.bSavedLooping = true;
			MeshComp->AnimationData.bSavedPlaying = true;
		}
		MeshComp->SetAnimInstanceClass(UTakeLocoAnim::StaticClass());
	}
}

bool ATakeCharacter::CanPlayerRun() const
{
	const AScriptSequencer* Seq = AScriptSequencer::Get(this);
	const AScriptScene* Scene = Seq ? Seq->GetActiveScene() : nullptr;
	return Scene && Scene->bAllowRun;
}

bool ATakeCharacter::CanPlayerCrouch() const
{
	const AScriptSequencer* Seq = AScriptSequencer::Get(this);
	const AScriptScene* Scene = Seq ? Seq->GetActiveScene() : nullptr;
	return Scene && Scene->bAllowCrouch;
}

void ATakeCharacter::ReadGaitKeys()
{
	if (GaitFrame == GFrameCounter)
	{
		return;
	}
	GaitFrame = GFrameCounter;
	UWorld* World = GetWorld();
	const ATakePlayerController* PC = World ? Cast<ATakePlayerController>(World->GetFirstPlayerController()) : nullptr;
	if (!PC || PC->GetCurrentCharacter() != this)
	{
		bRunHeld = false;
		return;
	}
	const bool bCrouchOk = CanPlayerCrouch();
	bRunHeld = CanPlayerRun() && (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift));
	if (bCrouchOk && (PC->WasInputKeyJustPressed(EKeys::C) || PC->WasInputKeyJustPressed(EKeys::LeftControl)))
	{
		SetCrouching(!bCrouching);
		bCrouchFromKey = bCrouching;
	}
	// Running stands him up; a scene that does not allow the crouch stands up a crouch the player made.
	if (bCrouching && bCrouchFromKey && (bRunHeld || !bCrouchOk))
	{
		SetCrouching(false);
	}
}

void ATakeCharacter::ApplyGait()
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}
	const bool bScripted = ScriptPath.Num() > 0 && ScriptSpeed > 0.f;
	const uint8 Want = bScripted ? 3 : bCrouching ? 2 : IsRunning() ? 1 : 0;
	if (Want == Gait && Want != 3)
	{
		return;
	}
	if (Gait == 0 && Want != 0)
	{
		// Keep the scene's own walk speed (a stroll, a hurry) to put back afterwards.
		SavedWalkSpeed = MoveComp->MaxWalkSpeed;
		SavedAcceleration = MoveComp->MaxAcceleration;
	}
	switch (Want)
	{
	case 0:
		MoveComp->MaxWalkSpeed = SavedWalkSpeed > 0.f ? SavedWalkSpeed : WalkSpeed;
		MoveComp->MaxAcceleration = SavedAcceleration > 0.f ? SavedAcceleration : WalkAcceleration;
		break;
	case 1:
		MoveComp->MaxWalkSpeed = RunSpeed;
		MoveComp->MaxAcceleration = RunAcceleration;
		break;
	case 2:
		MoveComp->MaxWalkSpeed = CrouchSpeed;
		MoveComp->MaxAcceleration = WalkAcceleration;
		break;
	default:
		MoveComp->MaxWalkSpeed = bCrouching ? FMath::Min(ScriptSpeed, CrouchSpeed) : ScriptSpeed;
		MoveComp->MaxAcceleration = ScriptSpeed >= RunSpeed * 0.9f ? RunAcceleration : WalkAcceleration;
		break;
	}
	Gait = Want;
}

void ATakeCharacter::SetRunning(bool bRun)
{
	bRunForced = bRun;
	if (bRun && bCrouching)
	{
		SetCrouching(false);
	}
	ApplyGait();
}

void ATakeCharacter::SetCrouching(bool bCrouch)
{
	if (bCrouch == bCrouching)
	{
		return;
	}
	bCrouching = bCrouch;
	if (!bCrouch)
	{
		bCrouchFromKey = false;
	}
	ApplyGait();
	TAKE_LOG(TEXT("%s %s"), GetCrewName(), bCrouch ? TEXT("crouches") : TEXT("stands up"));
}

void ATakeCharacter::ScriptWalkTo(const FVector& Target, float Speed, float AcceptRadius)
{
	TArray<FVector> One;
	One.Add(Target);
	ScriptWalkPath(One, Speed, AcceptRadius);
}

void ATakeCharacter::ScriptWalkPath(const TArray<FVector>& Points, float Speed, float AcceptRadius)
{
	ScriptPath = Points;
	ScriptSpeed = FMath::Max(Speed, 0.f);
	ScriptAccept = FMath::Max(AcceptRadius, 5.f);
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		if (MoveComp->MovementMode == MOVE_None)
		{
			MoveComp->SetMovementMode(MOVE_Walking);
		}
	}
	ApplyGait();
	StepScriptWalk();	// the first step this frame
}

void ATakeCharacter::StopScriptWalk()
{
	if (ScriptPath.Num() == 0)
	{
		return;
	}
	ScriptPath.Reset();
	ApplyGait();
}

void ATakeCharacter::StepScriptWalk()
{
	if (ScriptPath.Num() == 0)
	{
		return;
	}
	const FVector Feet = GetActorLocation();
	// Arrived at this point: the next one (the last one within AcceptRadius, the others a little wider so the turn is
	// rounded instead of a stop).
	while (ScriptPath.Num() > 0)
	{
		const float Accept = ScriptPath.Num() == 1 ? ScriptAccept : FMath::Max(ScriptAccept, 45.f);
		if (FVector::Dist2D(Feet, ScriptPath[0]) > Accept)
		{
			break;
		}
		ScriptPath.RemoveAt(0);
	}
	if (ScriptPath.Num() == 0)
	{
		ApplyGait();
		return;
	}
	const FVector To = (ScriptPath[0] - Feet).GetSafeNormal2D();
	if (!To.IsNearlyZero())
	{
		// Slow down into the last point (the braking alone would overshoot it).
		const float Left = static_cast<float>(FVector::Dist2D(Feet, ScriptPath.Last()));
		const float Scale = ScriptPath.Num() == 1 ? FMath::Clamp(Left / 120.f, 0.35f, 1.f) : 1.f;
		AddMovementInput(To, Scale);
	}
}

USkeletalMeshComponent* ATakeCharacter::GetVisibleBody() const
{
	ACastMember* Member = ACastMember::FindOn(this);
	USkeletalMeshComponent* ActBody = Member ? Member->Body.Get() : nullptr;
	if (ActBody && ActBody->IsVisible() && ActBody->GetSkeletalMeshAsset())
	{
		return ActBody;
	}
	return GetMesh();
}
