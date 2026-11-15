// Scene 8's getaway car (see Scene8Car.h). Owner: s8-alley.

#include "Script/Scene8Car.h"

#include "Animation/AnimationAsset.h"
#include "Camera/CameraComponent.h"
#include "Containers/Ticker.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "TakeShapes.h"

namespace
{
	const FLinearColor CarSilver(0.62f, 0.63f, 0.65f);
	const FLinearColor CarChrome(0.85f, 0.86f, 0.88f);
	const FLinearColor CarGlass(0.03f, 0.04f, 0.05f);
	const FLinearColor CarTyre(0.02f, 0.02f, 0.02f);
	const FLinearColor CarLamp(1.f, 0.95f, 0.8f);
	const FLinearColor CoatMax(0.20f, 0.16f, 0.12f);	// a brown driving jacket
	const FLinearColor CoatVance(0.92f, 0.90f, 0.85f);	// the white dinner jacket
	const FLinearColor CoatCole(0.08f, 0.08f, 0.09f);	// black stagehand kit
	const FLinearColor Skin(0.62f, 0.45f, 0.36f);

	void SetupCarLight(ULocalLightComponent* L, float Cd, float Radius, const FLinearColor& Colour)
	{
		L->SetMobility(EComponentMobility::Movable);
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(Cd);
		L->SetAttenuationRadius(Radius);
		L->SetLightColor(Colour);
		L->SetCastShadows(false);
	}
}

UStaticMeshComponent* AScene8Car::Part(FName Name, int32 Shape, USceneComponent* Parent, const FVector& Loc,
	const FVector& Scale, const FLinearColor& C, const FRotator& Rot)
{
	UStaticMeshComponent* M = FTakeShapes::CreateShape(this, Name, static_cast<ETakeShape>(Shape), Parent, Loc, Scale,
		Rot, true);
	Paint.Add(TPair<UStaticMeshComponent*, FLinearColor>(M, C));
	return M;
}

USceneComponent* AScene8Car::MakeSitter(FName Name, const FVector& Seat, const FLinearColor& Coat)
{
	USceneComponent* S = CreateDefaultSubobject<USceneComponent>(Name);
	S->SetupAttachment(Chassis);
	S->SetRelativeLocation(Seat);
	const FString N = Name.ToString();
	Part(*(N + TEXT("_Torso")), (int32)ETakeShape::Cylinder, S, FVector(0.f, 0.f, 32.f), FVector(0.42f, 0.42f, 0.62f), Coat);
	Part(*(N + TEXT("_Head")), (int32)ETakeShape::Sphere, S, FVector(4.f, 0.f, 76.f), FVector(0.24f), Skin);
	return S;
}

AScene8Car::AScene8Car()
{
	PrimaryActorTick.bCanEverTick = false;
	BodyMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Take/Script/S8/SM_S8_GTCar.SM_S8_GTCar")));
	// Max as s2-van seats him in the van: the clerk model at 0.9 scale on the Mixamo sitting idle (hip at 57.5 cm).
	MaxMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Take/Cast/NPCs/Clerk/SKM_NPC_Clerk.SKM_NPC_Clerk")));
	MaxAnim = TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(TEXT("/Game/Take/Anim/Mocap/SitIdle_A.SitIdle_A")));
	// The final shot: Vance is Julian's hero model, Cole is Vex's (docs/SCRIPT_ADAPT.md); the S8 materials are
	// tools/script/scene8_cabin.py's.
	VanceMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Take/Cast/Heroes/Julian/SKM_Hero_Julian.SKM_Hero_Julian")));
	ColeMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Take/Cast/Heroes/Vex/SKM_Hero_Vex.SKM_Hero_Vex")));
	SitAnim = TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(TEXT("/Game/Take/Anim/Mocap/SitIdle_B.SitIdle_B")));
	ColeSitAnim = TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(TEXT("/Game/Take/Anim/Mocap/SitIdle_A.SitIdle_A")));
	TieAnim = TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(TEXT("/Game/Take/Anim/Mocap/Headset.Headset")));
	PaintMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Take/Script/S8/M_S8_Paint.M_S8_Paint")));
	GlowMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Take/Script/S8/M_S8_Glow.M_S8_Glow")));
	MirrorMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Take/Script/S8/M_S8_Mirror.M_S8_Mirror")));
	GlassMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Take/Script/S8/M_S8_RainGlass.M_S8_RainGlass")));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Chassis = CreateDefaultSubobject<USceneComponent>(TEXT("Chassis"));
	Chassis->SetupAttachment(Root);

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Chassis);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetCanEverAffectNavigation(false);
	Body->SetVisibility(false);

	// The grey-box coupe: long bonnet, fastback cabin, chrome at both ends, four wheels.
	GreyBox = CreateDefaultSubobject<USceneComponent>(TEXT("GreyBox"));
	GreyBox->SetupAttachment(Chassis);
	const int32 Cube = (int32)ETakeShape::Cube, Sph = (int32)ETakeShape::Sphere, Cyl = (int32)ETakeShape::Cylinder;
	Part(TEXT("GB_Hull"), Cube, GreyBox, FVector(0.f, 0.f, 55.f), FVector(4.4f, 1.75f, 0.58f), CarSilver);
	Part(TEXT("GB_Bonnet"), Cube, GreyBox, FVector(120.f, 0.f, 82.f), FVector(1.9f, 1.6f, 0.08f), CarSilver,
		FRotator(-3.f, 0.f, 0.f));
	Part(TEXT("GB_Cabin"), Cube, GreyBox, FVector(-45.f, 0.f, 104.f), FVector(1.7f, 1.42f, 0.42f), CarGlass);
	Part(TEXT("GB_Roof"), Cube, GreyBox, FVector(-55.f, 0.f, 126.f), FVector(1.35f, 1.4f, 0.05f), CarSilver);
	Part(TEXT("GB_Fastback"), Cube, GreyBox, FVector(-150.f, 0.f, 100.f), FVector(0.9f, 1.45f, 0.06f), CarSilver,
		FRotator(-24.f, 0.f, 0.f));
	Part(TEXT("GB_BumperF"), Cube, GreyBox, FVector(222.f, 0.f, 42.f), FVector(0.08f, 1.7f, 0.1f), CarChrome);
	Part(TEXT("GB_BumperR"), Cube, GreyBox, FVector(-222.f, 0.f, 42.f), FVector(0.08f, 1.7f, 0.1f), CarChrome);
	Part(TEXT("GB_LampL"), Sph, GreyBox, FVector(214.f, -62.f, 68.f), FVector(0.2f), CarLamp);
	Part(TEXT("GB_LampR"), Sph, GreyBox, FVector(214.f, 62.f, 68.f), FVector(0.2f), CarLamp);
	for (int32 i = 0; i < 4; ++i)
	{
		const float X = (i < 2) ? 140.f : -140.f;
		const float Y = (i % 2) ? 80.f : -80.f;
		Part(*FString::Printf(TEXT("GB_Wheel%d"), i), Cyl, GreyBox, FVector(X, Y, 33.f), FVector(0.66f, 0.66f, 0.24f),
			CarTyre, FRotator(0.f, 0.f, 90.f));
	}
	// The passenger door on its front hinge (swings out to +Y).
	DoorHinge = CreateDefaultSubobject<USceneComponent>(TEXT("DoorHinge"));
	DoorHinge->SetupAttachment(GreyBox);
	DoorHinge->SetRelativeLocation(FVector(40.f, 89.f, 0.f));
	Part(TEXT("GB_Door"), Cube, DoorHinge, FVector(-58.f, 0.f, 62.f), FVector(1.12f, 0.05f, 0.62f), CarSilver);

	HeadL = CreateDefaultSubobject<USpotLightComponent>(TEXT("HeadL"));
	HeadL->SetupAttachment(Chassis);
	HeadL->SetRelativeLocationAndRotation(FVector(225.f, -62.f, 68.f), FRotator(-4.f, 0.f, 0.f));
	HeadR = CreateDefaultSubobject<USpotLightComponent>(TEXT("HeadR"));
	HeadR->SetupAttachment(Chassis);
	HeadR->SetRelativeLocationAndRotation(FVector(225.f, 62.f, 68.f), FRotator(-4.f, 0.f, 0.f));
	for (USpotLightComponent* L : { HeadL.Get(), HeadR.Get() })
	{
		SetupCarLight(L, 9000.f, 4500.f, FLinearColor(1.f, 0.93f, 0.78f));
		L->SetInnerConeAngle(12.f);
		L->SetOuterConeAngle(30.f);
	}
	Dash = CreateDefaultSubobject<UPointLightComponent>(TEXT("Dash"));
	Dash->SetupAttachment(Chassis);
	Dash->SetRelativeLocation(FVector(25.f, 10.f, 92.f));
	SetupCarLight(Dash, 6.f, 160.f, FLinearColor(1.f, 0.78f, 0.5f));

	MaxBody = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MaxBody"));
	MaxBody->SetupAttachment(Chassis);
	MaxBody->SetRelativeLocationAndRotation(DriverSeat - FVector(0.f, 0.f, 52.f), FRotator(0.f, -90.f, 0.f));
	MaxBody->SetRelativeScale3D(FVector(0.9f));
	MaxBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MaxBody->SetVisibility(false);
	FinaleCam = CreateDefaultSubobject<UCameraComponent>(TEXT("FinaleCam"));
	FinaleCam->SetupAttachment(Chassis);
	FinaleCam->SetAutoActivate(false);
	FinaleCam->bConstrainAspectRatio = false;
	MirrorCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MirrorCapture"));
	MirrorCapture->SetupAttachment(Chassis);
	MirrorCapture->bCaptureEveryFrame = false;
	MirrorCapture->bCaptureOnMovement = false;

	MaxProxy = MakeSitter(TEXT("MaxProxy"), DriverSeat, CoatMax);
	VanceProxy = MakeSitter(TEXT("VanceProxy"), PassengerSeat, CoatVance);
	ColeProxy = MakeSitter(TEXT("ColeProxy"), FVector(-115.f, 20.f, 30.f), CoatCole);
}

void AScene8Car::BeginPlay()
{
	Super::BeginPlay();
	for (const TPair<UStaticMeshComponent*, FLinearColor>& P : Paint)
	{
		if (P.Key) { FTakeShapes::SetColour(P.Key, P.Value); }
	}
	if (UStaticMesh* M = BodyMesh.IsNull() ? nullptr : BodyMesh.LoadSynchronous())
	{
		Body->SetStaticMesh(M);
		Body->SetVisibility(true);
		GreyBox->SetVisibility(false, true);
		// The generated body is solid: its windows hide the grey-box people, so only Max's real model shows.
	}
	if (USkeletalMesh* SM = MaxMesh.IsNull() ? nullptr : MaxMesh.LoadSynchronous())
	{
		MaxBody->SetSkeletalMeshAsset(SM);
		MaxBody->SetVisibility(true);
		MaxProxy->SetVisibility(false, true);
		if (UAnimationAsset* A = MaxAnim.IsNull() ? nullptr : MaxAnim.LoadSynchronous())
		{
			MaxBody->PlayAnimation(A, true);
		}
	}
	SetFlare(0.f);
	SetDoorOpen(1.f);
	SetRiders(false, false);
}

void AScene8Car::SetFlare(float Alpha)
{
	const float Cd = FMath::Lerp(HeadlightCd, FlareCd, FMath::Clamp(Alpha, 0.f, 1.f));
	HeadL->SetIntensity(Cd);
	HeadR->SetIntensity(Cd);
}

void AScene8Car::SetDoorOpen(float Alpha)
{
	DoorHinge->SetRelativeRotation(FRotator(0.f, -70.f * FMath::Clamp(Alpha, 0.f, 1.f), 0.f));
}

void AScene8Car::SetRiders(bool bVance, bool bCole)
{
	VanceProxy->SetVisibility(bVance, true);
	ColeProxy->SetVisibility(bCole, true);
}

// ---- the final shot (s8-car-mirror) --------------------------------------------------------------------------------
// Car space: nose +X, ground Z = 0, Vance on +Y. The GT shell (tools/script/scene8_car.py) measures: roof 142 cm at
// X -40, windscreen from (X 35, Z 103) up to (X 3, Z 135), rear window from (X -100, Z 135) down to (X -178, Z 97),
// sides at Y +-83. The cabin is built inside that, at BeginFinale.

namespace
{
	const FLinearColor Tan(0.30f, 0.15f, 0.07f);		// the seats: tan leather
	const FLinearColor Oxblood(0.12f, 0.025f, 0.02f);	// the door cards
	const FLinearColor Hide(0.015f, 0.014f, 0.013f);	// black leather: the dash top, the tunnel
	const FLinearColor Walnut(0.16f, 0.06f, 0.02f);
	const FLinearColor CabinChrome(0.9f, 0.9f, 0.92f);
	const FLinearColor Liner(0.30f, 0.27f, 0.22f);
	const FLinearColor Carpet(0.03f, 0.025f, 0.02f);
	const FLinearColor Silver(0.55f, 0.56f, 0.58f);
	const FLinearColor Asphalt(0.012f, 0.012f, 0.013f);
	FVector GS8FinaleShotCam = FVector::ZeroVector;	// the current shot's base camera and look point (SetShot)
	FVector GS8FinaleShotLook = FVector::ForwardVector;

	// The head bone's name differs between rigs ("head", "Head", "mixamorig:Head"): find it rather than guess.
	FVector S8HeadOf(const USkeletalMeshComponent* M)
	{
		if (!M) { return FVector::ZeroVector; }
		for (int32 i = 0; i < M->GetNumBones(); ++i)
		{
			const FString B = M->GetBoneName(i).ToString();
			if (B.EndsWith(TEXT("head"), ESearchCase::IgnoreCase)) { return M->GetBoneLocation(M->GetBoneName(i)); }
		}
		return M->GetComponentLocation() + FVector(0.f, 0.f, 75.f);
	}
	const TCHAR* InsideBodyPath = TEXT("/Game/Take/Script/S8/MI_S8_GTCar_Inside.MI_S8_GTCar_Inside");

	float WrapX(float V, float Span)
	{
		return V - Span * FMath::FloorToFloat((V + 0.5f * Span) / Span);
	}
}

UStaticMeshComponent* AScene8Car::Bit(int32 Shape, USceneComponent* Parent, const FVector& Loc, const FVector& SizeCm,
	const FLinearColor& C, float Rough, float Metal, const FRotator& Rot, bool bShadow)
{
	UStaticMeshComponent* M = FTakeShapes::AddShapeAtRuntime(this, static_cast<ETakeShape>(Shape), Parent, Loc,
		SizeCm / 100.f, C, bShadow);
	if (!M) { return nullptr; }
	M->SetRelativeRotation(Rot);
	if (UMaterialInterface* P = PaintMaterial.IsNull() ? nullptr : PaintMaterial.LoadSynchronous())
	{
		UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(P, this);
		D->SetVectorParameterValue(TEXT("Color"), C);
		D->SetScalarParameterValue(TEXT("Rough"), Rough);
		D->SetScalarParameterValue(TEXT("Metal"), Metal);
		M->SetMaterial(0, D);
	}
	return M;
}

UStaticMeshComponent* AScene8Car::Glow(USceneComponent* Parent, const FVector& Loc, float DiameterCm,
	const FLinearColor& C, float Power)
{
	UStaticMeshComponent* M = FTakeShapes::AddShapeAtRuntime(this, ETakeShape::Sphere, Parent, Loc,
		FVector(DiameterCm / 100.f), C, false);
	if (!M) { return nullptr; }
	if (UMaterialInterface* P = GlowMaterial.IsNull() ? nullptr : GlowMaterial.LoadSynchronous())
	{
		UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(P, this);
		D->SetVectorParameterValue(TEXT("Color"), C);
		D->SetScalarParameterValue(TEXT("Power"), Power);
		M->SetMaterial(0, D);
	}
	return M;
}

UStaticMeshComponent* AScene8Car::Pane(USceneComponent* Parent, const FVector& Loc, const FVector2D& SizeCm,
	const FRotator& Rot, const FVector2D& Pan, float Tiling)
{
	UMaterialInterface* P = GlassMaterial.IsNull() ? nullptr : GlassMaterial.LoadSynchronous();
	if (!P) { return nullptr; }	// no glass asset: open windows rather than grey slabs
	UStaticMeshComponent* M = FTakeShapes::AddShapeAtRuntime(this, ETakeShape::Plane, Parent, Loc,
		FVector(SizeCm.X / 100.f, SizeCm.Y / 100.f, 1.f), FLinearColor::Black, false);
	if (!M) { return nullptr; }
	M->SetRelativeRotation(Rot);
	UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(P, this);
	D->SetVectorParameterValue(TEXT("Tiling"), FLinearColor(SizeCm.X / (0.5f * Tiling), SizeCm.Y / (0.5f * Tiling), 0.f));
	D->SetVectorParameterValue(TEXT("PanSpeed"), FLinearColor(Pan.X, Pan.Y, 0.f));
	D->SetScalarParameterValue(TEXT("DropOpacity"), 0.22f);	// first look: the drops read as opaque wallpaper
	D->SetScalarParameterValue(TEXT("BaseOpacity"), 0.03f);
	M->SetMaterial(0, D);
	M->SetTranslucentSortPriority(1);
	GlassMids.Add(D);
	return M;
}

USkeletalMeshComponent* AScene8Car::Sitter(const TSoftObjectPtr<USkeletalMesh>& Mesh,
	const TSoftObjectPtr<UAnimationAsset>& Anim, const FVector& Hips)
{
	USkeletalMesh* SM = Mesh.IsNull() ? nullptr : Mesh.LoadSynchronous();
	if (!SM) { return nullptr; }
	USkeletalMeshComponent* K = NewObject<USkeletalMeshComponent>(this);
	K->SetupAttachment(Cabin);
	K->SetRelativeLocationAndRotation(Hips - FVector(0.f, 0.f, SitHipHeight), FRotator(0.f, -90.f, 0.f));
	K->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	K->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	K->RegisterComponent();
	AddInstanceComponent(K);
	K->SetSkeletalMeshAsset(SM);
	if (UAnimationAsset* A = Anim.IsNull() ? nullptr : Anim.LoadSynchronous()) { K->PlayAnimation(A, true); }
	return K;
}

void AScene8Car::BuildFinale()
{
	const int32 Cube = (int32)ETakeShape::Cube, Sph = (int32)ETakeShape::Sphere, Cyl = (int32)ETakeShape::Cylinder;
	const FRotator AlongY(0.f, 0.f, 90.f);		// a cylinder lying across the car
	const FRotator FacingBack(90.f, 0.f, 0.f);	// a cylinder's face toward the seats (-X)

	Cabin = NewObject<USceneComponent>(this, TEXT("Cabin"));
	Cabin->SetupAttachment(Chassis);
	Cabin->RegisterComponent();
	AddInstanceComponent(Cabin);
	Street = NewObject<USceneComponent>(this, TEXT("Street"));
	Street->SetupAttachment(Root);
	Street->RegisterComponent();
	AddInstanceComponent(Street);

	// Floor, tunnel, gear lever.
	Bit(Cube, Cabin, FVector(-65.f, 0.f, 12.f), FVector(230.f, 160.f, 4.f), Carpet, 1.f, 0.f);
	Bit(Cube, Cabin, FVector(0.f, 0.f, 24.f), FVector(110.f, 26.f, 22.f), Hide, 0.55f, 0.f);
	Bit(Cyl, Cabin, FVector(-12.f, 0.f, 46.f), FVector(2.f, 2.f, 30.f), CabinChrome, 0.12f, 1.f, FRotator(-12.f, 0.f, 0.f));
	Bit(Sph, Cabin, FVector(-15.f, 0.f, 61.f), FVector(6.f), Walnut, 0.25f, 0.f);

	// The front buckets (tan leather) and the 2+2 bench behind.
	for (const float Y : { -40.f, 40.f })
	{
		Bit(Cube, Cabin, FVector(-40.f, Y, 30.f), FVector(50.f, 48.f, 12.f), Tan, 0.45f, 0.f);
		Bit(Cyl, Cabin, FVector(-15.f, Y, 31.f), FVector(13.f, 48.f, 13.f), Tan, 0.45f, 0.f, AlongY);
		Bit(Cube, Cabin, FVector(-70.f, Y, 62.f), FVector(12.f, 48.f, 62.f), Tan, 0.45f, 0.f, FRotator(14.f, 0.f, 0.f));
		Bit(Cyl, Cabin, FVector(-78.f, Y, 93.f), FVector(15.f, 48.f, 15.f), Tan, 0.45f, 0.f, AlongY);
		for (const float S : { -1.f, 1.f })	// the side bolsters
		{
			Bit(Cube, Cabin, FVector(-71.f, Y + S * 22.f, 60.f), FVector(14.f, 7.f, 58.f), Tan * 0.8f, 0.5f, 0.f,
				FRotator(14.f, 0.f, 0.f));
		}
	}
	Bit(Cube, Cabin, FVector(-128.f, 0.f, 26.f), FVector(44.f, 140.f, 12.f), Tan, 0.45f, 0.f);
	Bit(Cube, Cabin, FVector(-152.f, 0.f, 56.f), FVector(12.f, 140.f, 56.f), Tan, 0.45f, 0.f, FRotator(24.f, 0.f, 0.f));
	Bit(Cube, Cabin, FVector(-186.f, 0.f, 94.f), FVector(32.f, 150.f, 3.f), Hide, 0.6f, 0.f);

	// The dash: walnut fascia, a black leather roll on top, the gauges glowing amber on Max's side, chrome trim.
	Bit(Cube, Cabin, FVector(28.f, 0.f, 82.f), FVector(26.f, 160.f, 30.f), Walnut, 0.22f, 0.f);
	Bit(Cube, Cabin, FVector(36.f, 0.f, 99.f), FVector(34.f, 160.f, 8.f), Hide, 0.5f, 0.f);
	Bit(Cyl, Cabin, FVector(17.f, 0.f, 96.f), FVector(10.f, 160.f, 10.f), Hide, 0.5f, 0.f, AlongY);
	Bit(Cube, Cabin, FVector(34.f, 0.f, 55.f), FVector(20.f, 160.f, 26.f), Hide, 0.7f, 0.f);
	Bit(Cube, Cabin, FVector(40.f, 0.f, 32.f), FVector(4.f, 160.f, 44.f), Carpet, 1.f, 0.f);
	Bit(Cube, Cabin, FVector(14.6f, 0.f, 70.f), FVector(1.f, 156.f, 1.2f), CabinChrome, 0.12f, 1.f);
	const FLinearColor Amber(1.f, 0.5f, 0.18f);
	for (const FVector& G : { FVector(14.f, -52.f, 85.f), FVector(14.f, -29.f, 85.f) })
	{
		Bit(Cyl, Cabin, G + FVector(0.6f, 0.f, 0.f), FVector(13.f, 13.f, 1.f), CabinChrome, 0.12f, 1.f, FacingBack);
		if (UStaticMeshComponent* Face = Glow(Cabin, G, 11.f, Amber, 0.35f))
		{
			Face->SetRelativeScale3D(FVector(0.01f, 0.11f, 0.11f));
		}
	}
	for (const float Y : { -6.f, 6.f })
	{
		if (UStaticMeshComponent* Face = Glow(Cabin, FVector(14.f, Y, 86.f), 6.f, Amber, 0.3f))
		{
			Face->SetRelativeScale3D(FVector(0.01f, 0.06f, 0.06f));
		}
	}
	if (UStaticMeshComponent* Dial = Glow(Cabin, FVector(14.f, 0.f, 77.f), 1.f, Amber, 0.5f))
	{
		Dial->SetRelativeScale3D(FVector(0.01f, 0.16f, 0.025f));
	}
	Bit(Cube, Cabin, FVector(14.6f, 42.f, 80.f), FVector(1.f, 9.f, 1.5f), CabinChrome, 0.12f, 1.f);

	// Max's wheel: a thin walnut rim on three chrome spokes, tilted to him.
	USceneComponent* Wheel = NewObject<USceneComponent>(this, TEXT("Wheel"));
	Wheel->SetupAttachment(Cabin);
	Wheel->SetRelativeLocationAndRotation(FVector(-2.f, -40.f, 86.f), FRotator(-24.f, 0.f, 0.f));
	Wheel->RegisterComponent();
	AddInstanceComponent(Wheel);
	for (int32 i = 0; i < 20; ++i)
	{
		const float A = 2.f * PI * i / 20.f;
		Bit(Cyl, Wheel, FVector(0.f, 19.f * FMath::Cos(A), 19.f * FMath::Sin(A)), FVector(2.6f, 2.6f, 6.4f), Walnut,
			0.25f, 0.f, FRotator(0.f, 0.f, FMath::RadiansToDegrees(A)));
	}
	for (const float Deg : { 0.f, 180.f, 270.f })
	{
		const float A = FMath::DegreesToRadians(Deg);
		Bit(Cube, Wheel, FVector(0.f, 9.f * FMath::Cos(A), 9.f * FMath::Sin(A)), FVector(0.8f, 18.f, 1.6f), CabinChrome,
			0.15f, 1.f, FRotator(0.f, 0.f, Deg));
	}
	Bit(Cyl, Wheel, FVector::ZeroVector, FVector(6.f, 6.f, 3.f), CabinChrome, 0.15f, 1.f, FacingBack);
	Bit(Cyl, Wheel, FVector(15.f, 0.f, 0.f), FVector(4.f, 4.f, 30.f), Hide, 0.5f, 0.f, FRotator(-90.f, 0.f, 0.f));

	// The shell from inside: headliner, pillars, door cards with a walnut cap rail and a chrome handle.
	Bit(Cube, Cabin, FVector(-48.f, 0.f, 134.f), FVector(110.f, 156.f, 3.f), Liner, 0.9f, 0.f);
	for (const float S : { -1.f, 1.f })
	{
		Bit(Cube, Cabin, FVector(19.f, S * 77.f, 119.f), FVector(52.f, 4.f, 5.f), Liner, 0.9f, 0.f, FRotator(-45.f, 0.f, 0.f));
		Bit(Cube, Cabin, FVector(-97.f, S * 79.f, 113.f), FVector(6.f, 4.f, 44.f), Liner, 0.9f, 0.f);
		Bit(Cube, Cabin, FVector(-34.f, S * 81.f, 58.f), FVector(130.f, 4.f, 62.f), Oxblood, 0.5f, 0.f);
		Bit(Cube, Cabin, FVector(-34.f, S * 79.f, 91.f), FVector(128.f, 6.f, 3.f), Walnut, 0.22f, 0.f);
		Bit(Cube, Cabin, FVector(-8.f, S * 77.5f, 74.f), FVector(8.f, 1.f, 1.5f), CabinChrome, 0.12f, 1.f);
	}

	// The glass, all of it wet: the windscreen (drops creeping up it in the wind), the side windows (running back),
	// the rear window.
	Pane(Cabin, FVector(19.f, 0.f, 119.f), FVector2D(46.f, 152.f), FRotator(-45.f, 0.f, 0.f), FVector2D(-0.18f, 0.f), 14.f);
	for (const float S : { -1.f, 1.f })
	{
		Pane(Cabin, FVector(-34.f, S * 80.f, 113.f), FVector2D(124.f, 40.f), FRotator(0.f, 0.f, 90.f),
			FVector2D(0.35f, 0.f), 16.f);
	}
	Pane(Cabin, FVector(-139.f, 0.f, 116.f), FVector2D(86.f, 130.f), FRotator(24.8f, 0.f, 0.f), FVector2D(0.f, 0.f), 16.f);

	// The rear-view mirror: a stem from the roof, a black body, a chrome rim, the glass showing MirrorCapture.
	Bit(Cyl, Cabin, MirrorPoint + FVector(2.f, 0.f, 8.f), FVector(1.5f, 1.5f, 12.f), Hide, 0.4f, 0.f);
	MirrorHead = NewObject<USceneComponent>(this, TEXT("MirrorHead"));
	MirrorHead->SetupAttachment(Cabin);
	MirrorHead->SetRelativeLocation(MirrorPoint);
	MirrorHead->RegisterComponent();
	AddInstanceComponent(MirrorHead);
	UStaticMeshComponent* MirrorBody = Bit(Cube, MirrorHead, FVector(1.8f, 0.f, 0.f), FVector(3.f, 27.f, 8.5f), Hide, 0.4f, 0.f);
	UStaticMeshComponent* MirrorRim = Bit(Cube, MirrorHead, FVector(0.4f, 0.f, 0.f), FVector(0.8f, 27.8f, 9.2f), CabinChrome, 0.12f, 1.f);
	MirrorGlass = FTakeShapes::AddShapeAtRuntime(this, ETakeShape::Plane, MirrorHead, FVector(-0.1f, 0.f, 0.f),
		FVector(0.26f, 0.078f, 1.f), FLinearColor::Black, false);
	MirrorGlass->SetRelativeRotation(FRotationMatrix::MakeFromXY(FVector(0.f, 1.f, 0.f), FVector(0.f, 0.f, -1.f)).Rotator());
	MirrorTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this, 640, 192, RTF_RGBA16f, FLinearColor::Black);
	if (UMaterialInterface* P = MirrorMaterial.IsNull() ? nullptr : MirrorMaterial.LoadSynchronous())
	{
		UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(P, this);
		D->SetTextureParameterValue(TEXT("Capture"), MirrorTarget);
		MirrorGlass->SetMaterial(0, D);
	}
	MirrorCapture->AttachToComponent(MirrorHead, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	MirrorCapture->SetRelativeLocation(FVector(-2.f, 0.f, 0.f));
	MirrorCapture->TextureTarget = MirrorTarget;
	MirrorCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	MirrorCapture->FOVAngle = FMath::Max(MirrorCaptureFov, 72.f);	// the target is 640x192: at 26-34 deg it saw a 12 cm strip of cheek; 72 gives the eyes strip
	MirrorCapture->ShowFlags.SetDepthOfField(false);	// the world's film look defocused the face into a bright blob
	MirrorCapture->ShowFlags.SetMotionBlur(false);
	MirrorCapture->HiddenComponents.Add(MirrorGlass);
	MirrorCapture->HiddenComponents.Add(MirrorBody);
	MirrorCapture->HiddenComponents.Add(MirrorRim);

	// The people: Vance (Julian's model) in the passenger bucket with the film can on his lap, Cole (Vex's) behind.
	VanceBody = Sitter(VanceMesh, SitAnim, PassengerSeat);
	ColeBody = Sitter(ColeMesh, ColeSitAnim, FVector(-124.f, 34.f, 34.f));
	if (ColeBody) { ColeBody->SetRelativeRotation(FRotator(0.f, -98.f, 0.f)); }
	USceneComponent* Can = NewObject<USceneComponent>(this, TEXT("FinaleCanister"));
	Can->SetupAttachment(Cabin);
	Can->SetRelativeLocationAndRotation(PassengerSeat + FVector(16.f, -1.f, 36.f), FRotator(80.f, 0.f, 0.f));	// held upright against him, face to the dash
	Can->RegisterComponent();
	AddInstanceComponent(Can);
	Bit(Cyl, Can, FVector::ZeroVector, FVector(36.f, 36.f, 6.f), Silver, 0.28f, 1.f);
	Bit(Cyl, Can, FVector(0.f, 0.f, 3.2f), FVector(36.6f, 36.6f, 1.2f), CabinChrome, 0.18f, 1.f);
	Bit(Cyl, Can, FVector(0.f, 0.f, 3.9f), FVector(8.f, 8.f, 0.4f), Hide, 0.4f, 0.f);
	Bit(Cube, Can, FVector(-6.f, 0.f, 3.9f), FVector(9.f, 14.f, 0.3f), FLinearColor(0.7f, 0.65f, 0.5f), 0.8f, 0.f);

	// Light inside: the instruments' warm glow on Vance's face, a cool spill from the street through his window.
	FaceKey = NewObject<UPointLightComponent>(this, TEXT("FaceKey"));
	FaceKey->SetupAttachment(Cabin);
	FaceKey->SetRelativeLocation(FVector(6.f, 28.f, 98.f));
	FaceKey->RegisterComponent();
	AddInstanceComponent(FaceKey);
	SetupCarLight(FaceKey, FaceKeyCd, 160.f, FLinearColor(1.f, 0.72f, 0.45f));
	UPointLightComponent* Fill = NewObject<UPointLightComponent>(this, TEXT("WindowFill"));
	Fill->SetupAttachment(Cabin);
	Fill->SetRelativeLocation(FVector(-40.f, 130.f, 125.f));
	Fill->RegisterComponent();
	AddInstanceComponent(Fill);
	SetupCarLight(Fill, 8.f, 260.f, FLinearColor(0.45f, 0.55f, 0.8f));

	// The street stays still under the car and slides past instead: a wet road, a kerb, lane marks, sodium lamps on
	// the passenger side, lit windows far off on both sides, now and then an oncoming car.
	Bit(Cube, Street, FVector(0.f, 0.f, -2.f), FVector(14000.f, 1000.f, 4.f), Asphalt, 0.16f, 0.f);
	Bit(Cube, Street, FVector(0.f, 470.f, 6.f), FVector(14000.f, 30.f, 12.f), FLinearColor(0.08f, 0.08f, 0.08f), 0.6f, 0.f);
	Bit(Cube, Street, FVector(0.f, 700.f, 4.f), FVector(14000.f, 440.f, 16.f), FLinearColor(0.03f, 0.03f, 0.032f), 0.25f, 0.f);
	auto AddMover = [this](USceneComponent* C, float Span)
	{
		Movers.Add(C);
		MoverSpan.Add(Span);
		MoverX0.Add(C->GetRelativeLocation().X);
	};
	auto Group = [this](const FString& Name, const FVector& At)
	{
		USceneComponent* G = NewObject<USceneComponent>(this, *Name);
		G->SetupAttachment(Street);
		G->SetRelativeLocation(At);
		G->RegisterComponent();
		AddInstanceComponent(G);
		return G;
	};
	const float MarkSpan = 900.f * 14.f;
	for (int32 i = 0; i < 14; ++i)
	{
		UStaticMeshComponent* Mark = Bit(Cube, Street, FVector(-MarkSpan * 0.5f + 900.f * i, -170.f, 0.6f),
			FVector(300.f, 12.f, 1.f), FLinearColor(0.5f, 0.48f, 0.4f), 0.5f, 0.f);
		if (Mark) { AddMover(Mark, MarkSpan); }
	}
	const int32 NumLamps = 4;
	const float LampSpan = LampSpacing * NumLamps;
	for (int32 i = 0; i < NumLamps; ++i)
	{
		USceneComponent* L = Group(FString::Printf(TEXT("Lamp%d"), i), FVector(-LampSpan * 0.5f + LampSpacing * i, 520.f, 0.f));
		Bit(Cyl, L, FVector(0.f, 0.f, 330.f), FVector(16.f, 16.f, 660.f), FLinearColor(0.04f, 0.045f, 0.04f), 0.4f, 0.6f);
		Bit(Cube, L, FVector(0.f, -70.f, 655.f), FVector(8.f, 150.f, 7.f), FLinearColor(0.04f, 0.045f, 0.04f), 0.4f, 0.6f);
		Glow(L, FVector(0.f, -140.f, 645.f), 34.f, LampColour, 30.f);
		USpotLightComponent* S = NewObject<USpotLightComponent>(this);
		S->SetupAttachment(L);
		S->SetRelativeLocationAndRotation(FVector(0.f, -140.f, 630.f), FRotator(-90.f, 0.f, 0.f));
		S->RegisterComponent();
		AddInstanceComponent(S);
		SetupCarLight(S, LampCd, 1500.f, LampColour);
		S->SetInnerConeAngle(22.f);
		S->SetOuterConeAngle(48.f);
		LampSpots.Add(S);
		AddMover(L, LampSpan);
	}
	FRandomStream R(1968);
	const float FarSpan = 10000.f;
	const FLinearColor FarColours[] = { FLinearColor(1.f, 0.62f, 0.3f), FLinearColor(1.f, 0.82f, 0.55f),
		FLinearColor(0.35f, 0.6f, 1.f), FLinearColor(1.f, 0.2f, 0.35f), FLinearColor(0.3f, 1.f, 0.75f) };
	for (int32 i = 0; i < 22; ++i)
	{
		const float Side = (i % 2) ? 1.f : -1.f;
		const FVector At(R.FRandRange(-FarSpan * 0.5f, FarSpan * 0.5f), Side * R.FRandRange(1100.f, 3200.f),
			R.FRandRange(120.f, 1100.f));
		const float Size = R.FRandRange(40.f, 140.f);
		const FLinearColor C = FarColours[R.RandRange(0, 4)];
		if (UStaticMeshComponent* G = Glow(Street, At, Size, C, R.FRandRange(4.f, 14.f))) { AddMover(G, FarSpan); }
	}
	USceneComponent* On = Group(TEXT("Oncoming"), FVector(5000.f, -330.f, 0.f));
	for (const float Y : { -65.f, 65.f })
	{
		Glow(On, FVector(0.f, Y, 70.f), 22.f, FLinearColor(1.f, 0.95f, 0.85f), 40.f);
	}
	Oncoming = NewObject<UPointLightComponent>(this, TEXT("OncomingLight"));
	Oncoming->SetupAttachment(On);
	Oncoming->SetRelativeLocation(FVector(-150.f, 0.f, 90.f));
	Oncoming->RegisterComponent();
	AddInstanceComponent(Oncoming);
	SetupCarLight(Oncoming, 2500.f, 2200.f, FLinearColor(1.f, 0.95f, 0.85f));
	AddMover(On, -13000.f);
}

void AScene8Car::SetShot(bool bB)
{
	bShotB = bB;
	FVector Cam = bB ? ShotBCam : ShotACam;
	FVector Look = bB ? MirrorPoint : ShotALook;
	float Fov = bB ? ShotBFov : ShotAFov;
	if (!bB)
	{
		// Shot A, from the dash on the driver's side of centre: his head and chest, the can held upright in front of
		// him. Fixed in car space (measured in play: his seated head bone is at about (-46, 41, 102)); reading the bone
		// here gave the standing reference pose, before the sit clip had posed him, and put the camera in the roof.
		Cam = FVector(14.f, -6.f, 98.f);
		Look = FVector(-34.f, 41.f, 92.f);
		Fov = FMath::Max(Fov, 66.f);
	}
	GS8FinaleShotCam = Cam;
	GS8FinaleShotLook = Look;
	FinaleCam->SetRelativeLocationAndRotation(Cam, (Look - Cam).Rotation());
	FinaleCam->SetFieldOfView(Fov);
	FPostProcessSettings& PP = FinaleCam->PostProcessSettings;
	PP.bOverride_DepthOfFieldFstop = true;
	PP.DepthOfFieldFstop = Fstop;
	PP.bOverride_DepthOfFieldFocalDistance = true;
	PP.DepthOfFieldFocalDistance = (Look - Cam).Size();
	PP.bOverride_MotionBlurAmount = true;
	PP.MotionBlurAmount = 0.6f;
	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = 0.7f;
	PP.bOverride_BloomIntensity = true;
	PP.BloomIntensity = 1.2f;
}

void AScene8Car::BeginFinale()
{
	if (!Cabin) { BuildFinale(); }
	bFinale = true;
	bTie = false;
	SetRiders(false, false);	// the grey-box stand-ins: Vance's sphere head sat in front of Julian's face in the mirror
	PreFinaleLocation = GetActorLocation();
	PreFinaleRotation = GetActorRotation();
	if (!FinaleSpot.IsNearlyZero()) { SetActorLocationAndRotation(FinaleSpot, FRotator::ZeroRotator); }
	else { SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f)); }
	Cabin->SetVisibility(true, true);
	Street->SetVisibility(true, true);
	SetRiders(false, false);
	SetDoorOpen(0.f);
	SetFlare(0.25f);
	// The shell from inside: its one-sided twin (the bonnet ahead shows, the roof and doors look through), else hidden.
	if (Body && Body->GetStaticMesh())
	{
		UMaterialInterface* Inside = LoadObject<UMaterialInterface>(nullptr, InsideBodyPath, nullptr, LOAD_NoWarn);
		if (Inside) { Body->SetMaterial(0, Inside); }
		Body->SetVisibility(Inside != nullptr);
	}
	if (VanceBody)
	{
		VanceBody->SetRelativeLocation(PassengerSeat - FVector(0.f, 0.f, SitHipHeight));
		if (UAnimationAsset* A = SitAnim.IsNull() ? nullptr : SitAnim.LoadSynchronous()) { VanceBody->PlayAnimation(A, true); }
	}
	// The mirror turned to Vance's face (he set it so he could see himself).
	// The glass faces shot B's camera square on (aimed at his head it tilted away from us); the capture behind it
	// is re-aimed at his eyes every tick, so the mirror shows his face whatever the glass's angle.
	const FVector Viewer = GetActorTransform().TransformPosition(ShotBCam);
	MirrorHead->SetWorldRotation((MirrorHead->GetComponentLocation() - Viewer).Rotation());
	MirrorHead->SetRelativeScale3D(FVector(1.f, 1.3f, 1.3f));	// a bigger glass: more of him in the frame
	MirrorCapture->bCaptureEveryFrame = true;
	MirrorCapture->SetActive(true);
	FinaleCam->SetActive(true);
	SetShot(false);
	TickFinale(0.f, 0.f);
}

void AScene8Car::TickFinale(float T, float DeltaSeconds)
{
	if (!bFinale) { return; }
	if (!bShotB && T >= ShotASeconds) { SetShot(true); }
	if (!bTie && T >= TieAt && VanceBody)
	{
		bTie = true;
		// A standing clip for the tie: the body lowered so the hips stay on the seat (only his top half shows).
		if (UAnimationAsset* A = TieAnim.IsNull() ? nullptr : TieAnim.LoadSynchronous())
		{
			VanceBody->SetRelativeLocation(PassengerSeat - FVector(0.f, 0.f, StandHipHeight));
			VanceBody->PlayAnimation(A, false);
		}
	}
	// The road under the car: a slow float, a shiver, a lean in the long bends.
	Chassis->SetRelativeLocationAndRotation(
		FVector(0.f, 0.f, 0.6f * FMath::Sin(T * 7.3f) + 0.25f * FMath::Sin(T * 17.9f)),
		FRotator(0.15f * FMath::Sin(T * 5.1f), 0.f, 0.35f * FMath::Sin(T * 1.3f) + 0.1f * FMath::Sin(T * 11.f)));
	// The camera rides the car, with a hand's drift; each shot creeps in.
	const FVector Cam = GS8FinaleShotCam;
	const FVector Look = GS8FinaleShotLook;
	const float Push = bShotB ? FMath::Clamp((T - ShotASeconds) / FMath::Max(0.1f, FinaleSeconds - ShotASeconds), 0.f, 1.f) * 5.f
		: FMath::Clamp(T / FMath::Max(0.1f, ShotASeconds), 0.f, 1.f) * 9.f;
	const FVector At = Cam + (Look - Cam).GetSafeNormal() * Push;
	FRotator Rot = (Look - At).Rotation();
	Rot.Pitch += 0.25f * FMath::Sin(T * 0.9f);
	Rot.Yaw += 0.3f * FMath::Sin(T * 0.7f + 1.f);
	FinaleCam->SetRelativeLocationAndRotation(At, Rot);
	FinaleCam->PostProcessSettings.DepthOfFieldFocalDistance = (Look - At).Size();
	// Everything outside slides past (the oncoming car faster, coming the other way).
	for (int32 i = 0; i < Movers.Num(); ++i)
	{
		USceneComponent* M = Movers[i];
		if (!M) { continue; }
		const float Span = FMath::Abs(MoverSpan[i]);
		const float Speed = MoverSpan[i] < 0.f ? RoadSpeed * 2.4f : RoadSpeed;
		FVector L = M->GetRelativeLocation();
		L.X = WrapX(MoverX0[i] - Speed * T, Span);
		M->SetRelativeLocation(L);
	}
	// The drops on the glass catch each lamp as it goes over.
	float NearestLamp = 1e9f;
	for (USpotLightComponent* S : LampSpots)
	{
		if (S) { NearestLamp = FMath::Min(NearestLamp, FMath::Abs(S->GetRelativeLocation().X + S->GetAttachParent()->GetRelativeLocation().X)); }
	}
	const float Glint = 3.f * FMath::Exp(-FMath::Square(NearestLamp / 420.f));
	for (UMaterialInstanceDynamic* D : GlassMids)
	{
		if (D) { D->SetScalarParameterValue(TEXT("Glint"), 0.15f + Glint); }
	}
	// The mirror keeps Vance's face in it.
	if (VanceBody && MirrorCapture)
	{
		const FVector Face = S8HeadOf(VanceBody) + GetActorForwardVector() * 10.f + FVector(0.f, 0.f, 9.f);	// his eyes, not the skull base
		MirrorCapture->SetWorldRotation((Face - MirrorCapture->GetComponentLocation()).Rotation());
	}
}

void AScene8Car::EndFinale()
{
	if (!bFinale) { return; }
	bFinale = false;
	bShotB = false;
	if (Cabin) { Cabin->SetVisibility(false, true); }
	if (Street) { Street->SetVisibility(false, true); }
	MirrorCapture->bCaptureEveryFrame = false;
	FinaleCam->SetActive(false);
	Chassis->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	if (Body && Body->GetStaticMesh())
	{
		Body->SetMaterial(0, nullptr);	// back to the mesh's own
		Body->SetVisibility(true);
	}
	SetActorLocationAndRotation(PreFinaleLocation, PreFinaleRotation);
}

// Reel.S8.Finale [T | off]: the final shot on its own, for checking it in play (pause the game first to hold it).
// With T it holds the frame at T seconds; without, it plays from 0 on the wall clock; "off" puts everything back.
namespace
{
	FTSTicker::FDelegateHandle GFinaleTick;

	void StopFinaleCmd(UWorld* World)
	{
		if (GFinaleTick.IsValid()) { FTSTicker::GetCoreTicker().RemoveTicker(GFinaleTick); GFinaleTick.Reset(); }
		if (!World) { return; }
		for (TActorIterator<AScene8Car> It(World); It; ++It) { It->EndFinale(); }
	}

	FAutoConsoleCommandWithWorldAndArgs GFinaleCmd(TEXT("Reel.S8.Finale"),
		TEXT("Reel.S8.Finale [T|off]: Scene 8's final in-car shot on its own (T: hold at T s)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			StopFinaleCmd(World);
			if (!World || (Args.Num() > 0 && Args[0] == TEXT("off"))) { return; }
			AScene8Car* Car = nullptr;
			for (TActorIterator<AScene8Car> It(World); It; ++It) { Car = *It; break; }
			if (!Car) { UE_LOG(LogTemp, Warning, TEXT("Reel.S8.Finale: no AScene8Car in the world")); return; }
			Car->SetActorHiddenInGame(false);
			Car->BeginFinale();
			if (APlayerController* PC = World->GetFirstPlayerController()) { PC->SetViewTarget(Car); }
			const float Hold = Args.Num() > 0 ? FCString::Atof(*Args[0]) : -1.f;
			const double Start = FPlatformTime::Seconds();
			TWeakObjectPtr<AScene8Car> Weak(Car);
			GFinaleTick = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak, Hold, Start](float Dt)
			{
				AScene8Car* C = Weak.Get();
				if (!C || !C->IsInFinale()) { GFinaleTick.Reset(); return false; }
				const float T = Hold >= 0.f ? Hold : static_cast<float>(FPlatformTime::Seconds() - Start);
				C->TickFinale(T, Dt);
				if (APlayerController* PC = C->GetWorld() ? C->GetWorld()->GetFirstPlayerController() : nullptr)
				{
					if (PC->GetViewTarget() != C) { PC->SetViewTarget(C); }
				}
				return true;
			}));
			UE_LOG(LogTemp, Log, TEXT("Reel.S8.Finale: on (hold %.2f)"), Hold);
		}));
}
