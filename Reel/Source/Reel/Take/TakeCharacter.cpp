// Owner: (m). Constructor and trivial getters by the header pass; every "// BODY: (m)" is a stub to fill.

#include "TakeCharacter.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

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
	static ConstructorHelpers::FClassFinder<UAnimInstance> UnarmedAnim(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"));
	if (UnarmedAnim.Succeeded())
	{
		MeshComp->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		MeshComp->SetAnimInstanceClass(UnarmedAnim.Class);
	}
	MeshComp->SetRelativeLocation(FVector(0.f, 0.f, -CapsuleComp->GetUnscaledCapsuleHalfHeight()));
	MeshComp->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	MeshComp->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}

void ATakeCharacter::BeginPlay()
{
	Super::BeginPlay();

	// OnConstruction does not rerun for placed actors in play or in a package: apply the tunables again here.
	ApplyTunables();
	ApplyTint();
	SetLook(static_cast<float>(GetActorRotation().Yaw), StartLookPitch);
	TAKE_LOG(TEXT("%s on set, look %.0f / %.0f"), GetCrewName(), LookYaw, LookPitch);
}

void ATakeCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// The editor shows the tinted mannequin and the boom where the take starts it (behind him).
	ApplyTunables();
	ApplyTint();
	SetLook(static_cast<float>(GetActorRotation().Yaw), StartLookPitch);
}

void ATakeCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
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

void ATakeCharacter::Move(const FVector2D& Axes)
{
	if (Axes.IsNearlyZero())
	{
		return;	// no input: he brakes at WalkBraking
	}

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
	if (!MeshComp || !MeshComp->GetSkeletalMeshAsset())
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

void ATakeCharacter::ApplyLookToBoom()
{
	if (Boom)
	{
		Boom->SetWorldRotation(FRotator(LookPitch, LookYaw, 0.f));
	}
}
