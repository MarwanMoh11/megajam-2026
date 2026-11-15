// Scene 8's getaway car (see Scene8Car.h). Owner: s8-alley.

#include "Script/Scene8Car.h"

#include "Animation/AnimationAsset.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
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
