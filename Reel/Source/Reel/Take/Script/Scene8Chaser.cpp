// Scene 8's chasing guard (see Scene8Chaser.h). Owner: s8-escape.

#include "Script/Scene8Chaser.h"

#include "Animation/AnimationAsset.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "TakeShapes.h"

namespace
{
	const FLinearColor ChaserNavy(0.05f, 0.06f, 0.10f);
	const FLinearColor ChaserSkin(0.55f, 0.42f, 0.34f);
	const FLinearColor ChaserBlack(0.02f, 0.02f, 0.02f);
}

AScene8Chaser::AScene8Chaser()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorEnableCollision(false);
	GuardMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Take/Cast/NPCs/Guard/SKM_NPC_Guard.SKM_NPC_Guard")));
	RunAnim = TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(
		TEXT("/Game/Characters/Mannequins/Anims/Pistol/Jog/MF_Pistol_Jog_Fwd.MF_Pistol_Jog_Fwd")));
	AimAnim = TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(
		TEXT("/Game/Characters/Mannequins/Anims/Pistol/MF_Pistol_Idle_ADS.MF_Pistol_Idle_ADS")));
	GunMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Take/Props/Guns/SM_Pistol.SM_Pistol")));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Box = CreateDefaultSubobject<USceneComponent>(TEXT("Box"));
	Box->SetupAttachment(Root);
	FTakeShapes::CreateShape(this, TEXT("Box_Body"), ETakeShape::Cylinder, Box, FVector(0.f, 0.f, 80.f), FVector(0.5f, 0.5f, 1.6f));
	FTakeShapes::CreateShape(this, TEXT("Box_Head"), ETakeShape::Sphere, Box, FVector(0.f, 0.f, 172.f), FVector(0.26f));
	FTakeShapes::CreateShape(this, TEXT("Box_Arm"), ETakeShape::Cube, Box, FVector(28.f, 16.f, 128.f), FVector(0.56f, 0.1f, 0.1f));

	// The mannequin faces +Y: turn it to face the actor's +X.
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// The pistol: on the grey box's hand; moved into the rigged guard's hand at BeginPlay.
	Gun = FTakeShapes::CreateShape(this, TEXT("Gun"), ETakeShape::Cube, Box, FVector(60.f, 16.f, 134.f), FVector(0.2f, 0.05f, 0.1f));

	MuzzleFlash = FTakeShapes::CreateShape(this, TEXT("MuzzleFlash"), ETakeShape::Sphere, Root, FVector::ZeroVector,
		FVector(0.2f), FRotator::ZeroRotator, false);
	MuzzleFlash->SetHiddenInGame(true);
	Tracer = FTakeShapes::CreateShape(this, TEXT("Tracer"), ETakeShape::Cube, Root, FVector::ZeroVector,
		FVector(1.f, 0.012f, 0.012f), FRotator::ZeroRotator, false);
	Tracer->SetHiddenInGame(true);
	Tracer->SetUsingAbsoluteLocation(true);
	Tracer->SetUsingAbsoluteRotation(true);
	Tracer->SetUsingAbsoluteScale(true);

	MuzzleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleLight"));
	MuzzleLight->SetupAttachment(Root);
	MuzzleLight->SetMobility(EComponentMobility::Movable);
	MuzzleLight->SetIntensityUnits(ELightUnits::Candelas);
	MuzzleLight->SetIntensity(0.f);
	MuzzleLight->SetAttenuationRadius(900.f);
	MuzzleLight->SetLightColor(FLinearColor(1.f, 0.75f, 0.4f));
	MuzzleLight->SetCastShadows(false);
}

void AScene8Chaser::BeginPlay()
{
	Super::BeginPlay();
	TArray<USceneComponent*> Parts;
	Box->GetChildrenComponents(false, Parts);
	for (USceneComponent* K : Parts)
	{
		if (UStaticMeshComponent* M = Cast<UStaticMeshComponent>(K))
		{
			const FString N = M->GetName();
			FTakeShapes::SetColour(M, N.EndsWith(TEXT("_Head")) ? ChaserSkin : N == TEXT("Gun") ? ChaserBlack : ChaserNavy);
		}
	}
	FTakeShapes::SetColour(MuzzleFlash, FLinearColor(1.f, 0.85f, 0.5f));
	FTakeShapes::SetColour(Tracer, FLinearColor(1.f, 0.9f, 0.6f));

	USkeletalMesh* SK = GuardMesh.IsNull() ? nullptr : GuardMesh.LoadSynchronous();
	bRigged = SK != nullptr;
	if (bRigged)
	{
		Mesh->SetSkeletalMeshAsset(SK);
		for (USceneComponent* K : Parts) { if (K != Gun.Get()) { K->SetHiddenInGame(true); } }
		// The pistol in his right hand: ACastMember's grip (grip across the palm, barrel along the fingers).
		Gun->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, HandBone);
		Gun->SetRelativeLocationAndRotation(FVector(-7.5f, 2.5f, -3.f), FRotator(-8.f, 176.f, 1.f));
		if (UStaticMesh* GM = GunMesh.IsNull() ? nullptr : GunMesh.LoadSynchronous())
		{
			Gun->SetStaticMesh(GM);
			Gun->SetRelativeScale3D(FVector(1.f));
			Gun->SetMaterial(0, nullptr);
			bGunMesh = true;
		}
		else
		{
			Gun->SetRelativeScale3D(FVector(0.2f, 0.035f, 0.1f));
		}
	}
	else
	{
		Mesh->SetVisibility(false);
	}
	ApplyAnim();
}

void AScene8Chaser::ApplyAnim()
{
	if (!bRigged) { return; }
	const TSoftObjectPtr<UAnimationAsset>& Want = bRunning ? RunAnim : AimAnim;
	if (UAnimationAsset* A = Want.IsNull() ? nullptr : Want.LoadSynchronous()) { Mesh->PlayAnimation(A, true); }
}

void AScene8Chaser::SetRunning(bool bRun)
{
	if (bRun == bRunning) { return; }
	bRunning = bRun;
	ApplyAnim();
}

FVector AScene8Chaser::GetMuzzle() const
{
	const FVector Fwd = GetActorForwardVector();
	if (bGunMesh)
	{
		return Gun->GetComponentTransform().TransformPosition(GunMuzzle);
	}
	if (bRigged && Mesh->DoesSocketExist(HandBone))
	{
		return Mesh->GetSocketLocation(HandBone) + Fwd * 24.f + FVector(0.f, 0.f, 4.f);
	}
	return GetActorTransform().TransformPosition(FVector(72.f, 16.f, 134.f));
}

void AScene8Chaser::Fire(const FVector& Hit)
{
	const FVector M = GetMuzzle();
	MuzzleFlash->SetWorldLocation(M);
	MuzzleFlash->SetHiddenInGame(false);
	MuzzleLight->SetWorldLocation(M);
	MuzzleLight->SetIntensity(1400.f);
	const FVector D = Hit - M;
	const float Len = D.Size();
	if (Len > 1.f)
	{
		Tracer->SetWorldLocationAndRotation(M + D * 0.5f, D.Rotation());
		Tracer->SetWorldScale3D(FVector(Len / 100.f, 0.012f, 0.012f));
		Tracer->SetHiddenInGame(false);
	}
	FlashAge = 0.f;
}

void AScene8Chaser::Show(bool bShow)
{
	SetActorHiddenInGame(!bShow);
	SetActorTickEnabled(bShow);
	if (!bShow)
	{
		MuzzleFlash->SetHiddenInGame(true);
		Tracer->SetHiddenInGame(true);
		MuzzleLight->SetIntensity(0.f);
	}
}

void AScene8Chaser::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	FlashAge += DeltaSeconds;
	const bool bOn = FlashAge < FlashSeconds;
	MuzzleFlash->SetHiddenInGame(!bOn);
	Tracer->SetHiddenInGame(!bOn);
	MuzzleLight->SetIntensity(bOn ? 1400.f : 0.f);
}
