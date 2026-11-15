// See Scene7Booth.h. Owner: s7-route (was s7-booth).
#include "Script/Scene7Booth.h"
#include "Script/ScriptSequencer.h"
#include "TakeCharacter.h"
#include "TakeDirector.h"
#include "TakeAudio.h"
#include "TakeHudKit.h"
#include "TakeStyle.h"
#include "HouseDimmer.h"
#include "TakeGuide.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "Animation/AnimationAsset.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "HAL/IConsoleManager.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogScene7, Log, All);

// Test hook: Reel.S7.Auto 1 and the scene plays itself (walks to the magazine, holds the swap, answers every prompt
// 0.6 s in). For shots and the coordinator's full runs; 0 (default) is the game.
static int32 GS7Auto = 0;
static FAutoConsoleVariableRef GS7AutoVar(TEXT("Reel.S7.Auto"), GS7Auto,
	TEXT("1: Scene 7 plays itself (swap, then every fight prompt answered). 0: the player plays it."));

namespace
{
	const FName SlowPrompt(TEXT("S7Prompt"));
	const FName SlowFall(TEXT("S7Fall"));

	UStaticMeshComponent* MakeMesh(AActor* Owner, USceneComponent* Parent, FName Name, UStaticMesh* Mesh,
		const FVector& Loc, const FRotator& Rot, const FVector& Scale, UMaterialInterface* Mat = nullptr)
	{
		UStaticMeshComponent* C = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(Parent);
		C->SetStaticMesh(Mesh);
		C->SetRelativeLocation(Loc);
		C->SetRelativeRotation(Rot);
		C->SetRelativeScale3D(Scale);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCanEverAffectNavigation(false);
		if (Mat) { C->SetMaterial(0, Mat); }
		return C;
	}

	UPointLightComponent* MakeLight(AActor* Owner, USceneComponent* Parent, FName Name, const FVector& Loc,
		const FColor& Colour, float Candela, float Radius, bool bShadows)
	{
		UPointLightComponent* L = Owner->CreateDefaultSubobject<UPointLightComponent>(Name);
		L->SetupAttachment(Parent);
		L->SetRelativeLocation(Loc);
		L->IntensityUnits = ELightUnits::Candelas;
		L->Intensity = Candela;
		L->LightColor = Colour;
		L->AttenuationRadius = Radius;
		L->SetCastShadows(bShadows);
		return L;
	}

	USkeletalMeshComponent* MakePuppet(AActor* Owner, USceneComponent* Parent, FName Name)
	{
		USkeletalMeshComponent* M = Owner->CreateDefaultSubobject<USkeletalMeshComponent>(Name);
		M->SetupAttachment(Parent);
		M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		M->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		M->SetHiddenInGame(true);
		M->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		return M;
	}

	/** The player walks Vance only in the approach; from the swap on the scene drives him. */
	void FreezeVance(ATakeCharacter* V, bool bFrozen)
	{
		UCharacterMovementComponent* M = V ? V->GetCharacterMovement() : nullptr;
		if (!M) { return; }
		if (bFrozen) { M->StopMovementImmediately(); M->DisableMovement(); }
		else if (M->MovementMode == MOVE_None) { M->SetMovementMode(MOVE_Walking); }
	}

	FScene7Shot MakeShot(const FVector& From, const FVector& To, float Fov)
	{
		FScene7Shot S;
		S.Location = From;
		S.Rotation = (To - From).Rotation();
		S.Fov = Fov;
		return S;
	}
}

AScene7Booth::AScene7Booth()
{
	SceneNumber = 7;
	Pov = EScriptPov::Vance;
	Slug = FText::FromString(TEXT("INT. PROJECTION BOOTH - CONTINUOUS"));
	Tags.Add(TEXT("Part=2"));	// after AScene7Route (0) and s7-reelswap's swap (1); s7-fight's fight is 3
	bPlayerWalks = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeF(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylF(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereF(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ProjF(TEXT("/Game/Take/Props/Rooms_projector/SM_Rooms_projector.SM_Rooms_projector"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ReelF(TEXT("/Game/Take/Props/Reel/SM_Reel.SM_Reel"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RedF(TEXT("/Game/Take/Materials/MI_Box_Red.MI_Box_Red"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TrimF(TEXT("/Game/Take/Materials/MI_Box_Trim.MI_Box_Trim"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DoorF(TEXT("/Game/Take/Materials/MI_Box_Door.MI_Box_Door"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WindowF(TEXT("/Game/Take/Materials/MI_Box_Window.MI_Box_Window"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GoldF(TEXT("/Game/Take/Materials/MI_Box_Gold.MI_Box_Gold"));
	UStaticMesh* Cube = CubeF.Object;
	UStaticMesh* Cyl = CylF.Object;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// The projector: the rooms' 35 mm model, lens to +X; the upper magazine (a drum standing on edge, its face to -Y)
	// with a lid on a hinge; the master print on the drum's spindle; the take-up reel below.
	Projector = MakeMesh(this, Root, TEXT("Projector"), ProjF.Object, FVector(40.f, -150.f, 0.f), FRotator::ZeroRotator, FVector(1.f));
	Projector->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Projector->SetCollisionProfileName(TEXT("BlockAll"));
	Magazine = MakeMesh(this, Root, TEXT("Magazine"), Cyl, FVector(30.f, -150.f, 205.f), FRotator(0.f, 0.f, 90.f), FVector(0.9f, 0.9f, 0.22f), TrimF.Object);
	MagazineHinge = CreateDefaultSubobject<USceneComponent>(TEXT("MagazineHinge"));
	MagazineHinge->SetupAttachment(Root);
	MagazineHinge->SetRelativeLocation(FVector(-15.f, -162.f, 205.f));
	MagazineLid = MakeMesh(this, MagazineHinge, TEXT("MagazineLid"), Cyl, FVector(45.f, 0.f, 0.f), FRotator(0.f, 0.f, 90.f), FVector(0.9f, 0.9f, 0.03f), TrimF.Object);
	MasterReel = MakeMesh(this, Root, TEXT("MasterReel"), ReelF.Object, FVector(30.f, -150.f, 205.f), FRotator::ZeroRotator, FVector(1.f));
	DummyReel = MakeMesh(this, Root, TEXT("DummyReel"), ReelF.Object, FVector(-70.f, -330.f, 92.f), FRotator::ZeroRotator, FVector(1.f));
	LowerReel = MakeMesh(this, Root, TEXT("LowerReel"), ReelF.Object, FVector(30.f, -150.f, 40.f), FRotator::ZeroRotator, FVector(1.f));

	// The silver canister lies open on the bench; its lid stands beside it until it latches.
	Canister = MakeMesh(this, Root, TEXT("Canister"), Cyl, FVector(-70.f, -260.f, 90.f), FRotator::ZeroRotator, FVector(0.42f, 0.42f, 0.07f), TrimF.Object);
	CanisterLid = MakeMesh(this, Canister, TEXT("CanisterLid"), Cyl, FVector(0.f, 120.f, -45.f), FRotator(0.f, 0.f, 0.f), FVector(1.02f, 1.02f, 0.25f), TrimF.Object);
	CanisterLid->SetUsingAbsoluteScale(false);

	// The red manual fire alarm by the north door.
	AlarmBox = MakeMesh(this, Root, TEXT("AlarmBox"), Cube, FVector(-110.f, 492.f, 140.f), FRotator::ZeroRotator, FVector(0.16f, 0.06f, 0.24f), RedF.Object);
	AlarmLever = MakeMesh(this, AlarmBox, TEXT("AlarmLever"), Cube, FVector(0.f, -70.f, 15.f), FRotator::ZeroRotator, FVector(0.5f, 0.6f, 0.15f), GoldF.Object);

	// The viewing glass in the front wall (the overlay cuts the opening and frames it).
	Glass = MakeMesh(this, Root, TEXT("Glass"), Cube, FVector(190.f, 150.f, 210.f), FRotator::ZeroRotator, FVector(0.02f, 2.6f, 2.0f), WindowF.Object);

	// Two doors, hinged at their west jamb: north (Pendleton) and south (the stairwell: Cole).
	DoorNorthHinge = CreateDefaultSubobject<USceneComponent>(TEXT("DoorNorthHinge"));
	DoorNorthHinge->SetupAttachment(Root);
	DoorNorthHinge->SetRelativeLocation(FVector(60.f, 500.f, 0.f));
	DoorNorth = MakeMesh(this, DoorNorthHinge, TEXT("DoorNorth"), Cube, FVector(50.f, 0.f, 110.f), FRotator::ZeroRotator, FVector(1.0f, 0.06f, 2.2f), DoorF.Object);
	DoorSouthHinge = CreateDefaultSubobject<USceneComponent>(TEXT("DoorSouthHinge"));
	DoorSouthHinge->SetupAttachment(Root);
	DoorSouthHinge->SetRelativeLocation(FVector(60.f, -500.f, 0.f));
	DoorSouth = MakeMesh(this, DoorSouthHinge, TEXT("DoorSouth"), Cube, FVector(50.f, 0.f, 110.f), FRotator::ZeroRotator, FVector(1.0f, 0.06f, 2.2f), DoorF.Object);

	Stiletto = MakeMesh(this, Root, TEXT("Stiletto"), Cube, FVector::ZeroVector, FRotator::ZeroRotator, FVector(0.25f, 0.012f, 0.03f), GoldF.Object);
	Stiletto->SetHiddenInGame(true);
	Grenade = MakeMesh(this, Root, TEXT("Grenade"), Cyl, FVector::ZeroVector, FRotator::ZeroRotator, FVector(0.07f, 0.07f, 0.12f), TrimF.Object);
	Grenade->SetHiddenInGame(true);
	CurtainHeap = MakeMesh(this, Root, TEXT("CurtainHeap"), SphereF.Object, FVector(330.f, 150.f, -450.f), FRotator::ZeroRotator, FVector(3.2f, 2.2f, 0.45f), RedF.Object);
	CurtainHeap->SetHiddenInGame(true);

	for (int32 i = 0; i < 12; ++i)
	{
		UStaticMeshComponent* S = MakeMesh(this, Root, *FString::Printf(TEXT("Shard%d"), i), Cube, FVector(190.f, 150.f, 210.f),
			FRotator::ZeroRotator, FVector(0.01f, 0.18f + 0.04f * (i % 3), 0.14f + 0.05f * (i % 4)), WindowF.Object);
		S->SetHiddenInGame(true);
		Shards.Add(S);
	}
	for (int32 i = 0; i < 7; ++i)
	{
		UStaticMeshComponent* P = MakeMesh(this, Root, *FString::Printf(TEXT("Smoke%d"), i), SphereF.Object, FVector::ZeroVector,
			FRotator::ZeroRotator, FVector(0.2f));
		P->SetHiddenInGame(true);
		P->SetCastShadow(false);
		SmokePuffs.Add(P);
	}

	LampSpill = MakeLight(this, Root, TEXT("LampSpill"), FVector(-10.f, -150.f, 150.f), FColor(255, 196, 130), LampCandela, 650.f, true);
	KeyLight = MakeLight(this, Root, TEXT("KeyLight"), FVector(-60.f, 250.f, 280.f), FColor(200, 210, 255), 12.f, 700.f, false);
	StrobeA = MakeLight(this, Root, TEXT("StrobeA"), FVector(-150.f, 470.f, 270.f), FColor(255, 20, 10), 0.f, 900.f, true);
	StrobeB = MakeLight(this, Root, TEXT("StrobeB"), FVector(-150.f, -420.f, 270.f), FColor(255, 20, 10), 0.f, 900.f, false);
	FireLight = MakeLight(this, Root, TEXT("FireLight"), FVector(10.f, -150.f, 230.f), FColor(255, 120, 40), 0.f, 500.f, false);

	Pendleton = MakePuppet(this, Root, TEXT("Pendleton"));
	Bodyguard = MakePuppet(this, Root, TEXT("Bodyguard"));
	Bodyguard->SetRelativeScale3D(FVector(1.12f));
	Cole = MakePuppet(this, Root, TEXT("Cole"));

	HumAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("HumAudio"));
	HumAudio->SetupAttachment(Projector);
	HumAudio->bAutoActivate = false;
	AlarmAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("AlarmAudio"));
	AlarmAudio->SetupAttachment(Root);
	AlarmAudio->bAutoActivate = false;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> GuardF(TEXT("/Game/Take/Cast/NPCs/Bodyguard/SKM_NPC_Bodyguard.SKM_NPC_Bodyguard"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> ChiefF(TEXT("/Game/Take/Cast/NPCs/Vance/SKM_NPC_Vance.SKM_NPC_Vance"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> VexF(TEXT("/Game/Take/Cast/Heroes/Vex/SKM_Hero_Vex.SKM_Hero_Vex"));
	BodyguardMesh = GuardF.Object;
	PendletonMesh = ChiefF.Object;
	ColeMesh = VexF.Object;

	struct FAnimPath { const TCHAR* Name; const TCHAR* Path; };
	static const FAnimPath AnimPaths[] = {
		{ TEXT("Idle"), TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle") },
		{ TEXT("Walk"), TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd.MF_Unarmed_Jog_Fwd") },
		{ TEXT("Punch"), TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01") },
		{ TEXT("Slash"), TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02.MM_Attack_02") },
		{ TEXT("Sweep"), TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03.MM_Attack_03") },
		{ TEXT("Hook"), TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack.MM_ChargedAttack") },
		{ TEXT("HitHeavy"), TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Hvy_01.MM_HitReact_Front_Hvy_01") },
		{ TEXT("HitMed"), TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Med_01.MM_HitReact_Front_Med_01") },
		{ TEXT("FallBack"), TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01.MM_Death_Back_01") },
		{ TEXT("FallFront"), TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01.MM_Death_Front_01") },
		{ TEXT("Use"), TEXT("/Game/Take/Anim/Mocap/Use.Use") },
		{ TEXT("Lift"), TEXT("/Game/Take/Anim/Mocap/Lift.Lift") },
		{ TEXT("Refuse"), TEXT("/Game/Take/Anim/Mocap/Refuse.Refuse") },
		{ TEXT("Talk"), TEXT("/Game/Take/Anim/Mocap/Talk_A.Talk_A") },
		{ TEXT("Stand"), TEXT("/Game/Take/Anim/Mocap/StandIdle_A.StandIdle_A") },
	};
	for (const FAnimPath& P : AnimPaths)
	{
		ConstructorHelpers::FObjectFinder<UAnimationAsset> F(P.Path);
		if (F.Succeeded()) { Anims.Add(P.Name, F.Object); }
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> SparksF(TEXT("/Game/Take/FX/NS_Take_Sparks.NS_Take_Sparks"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> PyroF(TEXT("/Game/Take/FX/NS_Take_Pyro.NS_Take_Pyro"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> GlassF(TEXT("/Game/Take/FX/NS_Take_Glass.NS_Take_Glass"));
	SparksFX = SparksF.Object;
	FireFX = PyroF.Object;
	GlassFX = GlassF.Object;

	CueFallbacks.Add(TEXT("sfx_s7_window_crash"), TEXT("sfx_glass_smash"));
	CueFallbacks.Add(TEXT("sfx_s7_head_slam"), TEXT("sfx_pyro"));
	CueFallbacks.Add(TEXT("sfx_s7_clang"), TEXT("sfx_winch_release"));
	CueFallbacks.Add(TEXT("sfx_s7_body_fall"), TEXT("sfx_thud"));
	CueFallbacks.Add(TEXT("sfx_s7_door_kick"), TEXT("sfx_thud"));
	CueFallbacks.Add(TEXT("sfx_s7_canister_latch"), TEXT("sfx_chime"));
	CueFallbacks.Add(TEXT("sfx_s7_fire_whoosh"), TEXT("sfx_film_burn"));
	CueFallbacks.Add(TEXT("sfx_s7_smoke"), TEXT("sfx_pyro"));
	CueFallbacks.Add(TEXT("sfx_s7_punch"), TEXT("sfx_thud"));

	PromptKeys = { EKeys::Q, EKeys::E, EKeys::F };
	PromptWords = { TEXT("PARRY"), TEXT("BLOCK WITH THE CANISTER"), TEXT("COUNTER") };

	CanisterInArm = FTransform(FRotator(0.f, 0.f, 90.f), FVector(12.f, 6.f, -14.f), FVector(0.42f, 0.42f, 0.07f));
	CanisterInHand = FTransform(FRotator(0.f, 90.f, 0.f), FVector(4.f, 10.f, 0.f), FVector(0.42f, 0.42f, 0.07f));
	StilettoInHand = FTransform(FRotator(0.f, 0.f, 0.f), FVector(14.f, 0.f, 0.f), FVector(0.25f, 0.012f, 0.03f));

	// Cameras, booth space (+X to the glass, +Y north to Pendleton's door, floor at z 0).
	Shots.Add(TEXT("Swap"), MakeShot(FVector(-30.f, -340.f, 210.f), FVector(-30.f, -160.f, 185.f), 45.f));
	Shots.Add(TEXT("Door"), MakeShot(FVector(-150.f, -100.f, 170.f), FVector(110.f, 500.f, 110.f), 45.f));
	Shots.Add(TEXT("Insert"), MakeShot(FVector(-130.f, 100.f, 125.f), FVector(-50.f, 40.f, 100.f), 30.f));
	Shots.Add(TEXT("Line"), MakeShot(FVector(-140.f, -10.f, 175.f), FVector(60.f, 300.f, 165.f), 35.f));
	Shots.Add(TEXT("Alarm"), MakeShot(FVector(40.f, 330.f, 150.f), FVector(-110.f, 470.f, 140.f), 45.f));
	Shots.Add(TEXT("Screen"), MakeShot(FVector(700.f, 100.f, 120.f), FVector(5188.f, -300.f, 150.f), 30.f));
	Shots.Add(TEXT("Parry"), MakeShot(FVector(150.f, -200.f, 90.f), FVector(-20.f, 150.f, 140.f), 50.f));
	Shots.Add(TEXT("ParryHit"), MakeShot(FVector(160.f, 60.f, 140.f), FVector(10.f, -60.f, 140.f), 50.f));
	Shots.Add(TEXT("Block"), MakeShot(FVector(60.f, -40.f, 100.f), FVector(0.f, 170.f, 160.f), 40.f));
	Shots.Add(TEXT("BlockHit"), MakeShot(FVector(60.f, 120.f, 165.f), FVector(-30.f, 60.f, 160.f), 30.f));
	Shots.Add(TEXT("Throw"), MakeShot(FVector(-150.f, 280.f, 120.f), FVector(60.f, 160.f, 100.f), 45.f));
	Shots.Add(TEXT("Fall"), MakeShot(FVector(700.f, 350.f, -400.f), FVector(250.f, 150.f, 0.f), 50.f));
	Shots.Add(TEXT("Cole"), MakeShot(FVector(170.f, -90.f, 175.f), FVector(110.f, -450.f, 120.f), 55.f));
	// World cameras for the venue cut, the stair and the security room.
	WorldShots.Add(TEXT("Venue"), MakeShot(FVector(2250.f, 900.f, 300.f), FVector(4300.f, -250.f, 480.f), 55.f));
	WorldShots.Add(TEXT("Pendleton"), MakeShot(FVector(3480.f, -260.f, 230.f), FVector(3890.f, -480.f, 185.f), 32.f));
	WorldShots.Add(TEXT("Stair"), MakeShot(FVector(-1350.f, 420.f, 700.f), FVector(-2350.f, 0.f, 230.f), 60.f));
	WorldShots.Add(TEXT("Caught"), MakeShot(FVector(-2900.f, -1930.f, 650.f), FVector(-2620.f, -1420.f, 580.f), 70.f));
	StrobesVenue = { FVector(3000.f, 1200.f, 900.f), FVector(1500.f, -1200.f, 900.f) };
	StrobesEscape = { FVector(-1300.f, -500.f, 820.f), FVector(-2650.f, -1700.f, 820.f) };
	{
		FScene7RouteStep S;
		S.At = FVector(-1180.f, 0.f, 450.f); S.Radius = 170.f; S.Objective = TEXT("Out through the lobby door");
		EscapeSteps.Add(S);
		S = FScene7RouteStep(); S.At = FVector(-1350.f, -1050.f, 450.f); S.Radius = 260.f;
		S.Objective = TEXT("Along the balcony: the security room"); S.Speaker = TEXT("NORA (V.O.)");
		S.Line = TEXT("Pendleton and his ape, coming up the grand stair. Get out of sight: the door on your left!");
		EscapeSteps.Add(S);
		S = FScene7RouteStep(); S.At = FVector(-2650.f, -1300.f, 450.f); S.Radius = 160.f; S.Objective = TEXT("Into the security room");
		EscapeSteps.Add(S);
	}
	GuardPath = { FVector(-2750.f, 60.f, 0.f), FVector(-2560.f, 60.f, 0.f), FVector(-1500.f, 60.f, 450.f),
		FVector(-1380.f, -250.f, 450.f) };
	Shots.Add(TEXT("Miss"), MakeShot(FVector(60.f, 150.f, 170.f), FVector(-30.f, 40.f, 160.f), 35.f));
}

void AScene7Booth::BeginPlay()
{
	Super::BeginPlay();

	if (PendletonMesh) { Pendleton->SetSkeletalMesh(PendletonMesh); }
	if (BodyguardMesh) { Bodyguard->SetSkeletalMesh(BodyguardMesh); }
	if (ColeMesh) { Cole->SetSkeletalMesh(ColeMesh); }

	// One camera actor per shot, attached to the booth.
	for (const TPair<FName, FScene7Shot>& S : Shots)
	{
		FActorSpawnParameters P;
		P.Owner = this;
		ACameraActor* Cam = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), GetActorTransform(), P);
		if (!Cam) { continue; }
		Cam->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		Cam->SetActorRelativeLocation(S.Value.Location);
		Cam->SetActorRelativeRotation(S.Value.Rotation);
		Cam->GetCameraComponent()->SetFieldOfView(S.Value.Fov);
		Cam->GetCameraComponent()->bConstrainAspectRatio = false;
		Cams.Add(S.Key, Cam);
	}

	for (const TPair<FName, FScene7Shot>& S : WorldShots)
	{
		FActorSpawnParameters P;
		P.Owner = this;
		ACameraActor* Cam = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(),
			FTransform(S.Value.Rotation, S.Value.Location), P);
		if (!Cam) { continue; }
		Cam->GetCameraComponent()->SetFieldOfView(S.Value.Fov);
		Cam->GetCameraComponent()->bConstrainAspectRatio = false;
		Cams.Add(S.Key, Cam);
	}
	// The stair door (south) stands open: part 0 walks Vance in through it. The alarm shuts it (mag-lock).
	DoorSouthHinge->SetRelativeRotation(FRotator(0.f, DoorOpenDegrees, 0.f));

	// The rest pose of everything that moves.
	TArray<USceneComponent*> Movers = { Magazine, MagazineHinge, MasterReel, DummyReel, LowerReel, Canister, CanisterLid,
		AlarmLever, Glass, DoorNorthHinge, DoorSouthHinge, Stiletto, Grenade, CurtainHeap, Pendleton, Bodyguard, Cole };
	for (UStaticMeshComponent* S : Shards) { Movers.Add(S); }
	for (UStaticMeshComponent* S : SmokePuffs) { Movers.Add(S); }
	for (USceneComponent* C : Movers)
	{
		if (C) { Rest.Add(C, C->GetRelativeTransform()); }
	}
	if (SmokeMaterial)
	{
		for (UStaticMeshComponent* S : SmokePuffs) { S->SetMaterial(0, SmokeMaterial); }
	}

	// The projector runs in every scene (the house can hear it from the booth).
	StrobeA->SetIntensity(0.f);
	StrobeB->SetIntensity(0.f);
	FireLight->SetIntensity(0.f);
}

// ---- helpers ------------------------------------------------------------------------------------------------------

APlayerController* AScene7Booth::GetPC() const
{
	return UGameplayStatics::GetPlayerController(this, 0);
}

void AScene7Booth::Shot(FName Name)
{
	APlayerController* PC = GetPC();
	if (!PC) { return; }
	if (Name.IsNone())
	{
		if (ATakeCharacter* V = GetPovCharacter()) { PC->SetViewTarget(V); }
		CurrentShot = NAME_None;
		return;
	}
	if (TObjectPtr<ACameraActor>* Cam = Cams.Find(Name))
	{
		PC->SetViewTarget(*Cam);
		CurrentShot = Name;
	}
}

USoundBase* AScene7Booth::FindS7Sound(FName Name)
{
	if (TObjectPtr<USoundBase>* Found = SoundCache.Find(Name)) { return *Found; }
	const FString Path = FString::Printf(TEXT("/Game/Take/Audio/Script/S7/%s.%s"), *Name.ToString(), *Name.ToString());
	USoundBase* S = LoadObject<USoundBase>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	SoundCache.Add(Name, S);
	return S;
}

void AScene7Booth::Cue(FName Name, float Volume)
{
	if (USoundBase* S = FindS7Sound(Name))
	{
		UGameplayStatics::PlaySound2D(this, S, Volume);
		return;
	}
	if (UTakeAudioComponent* A = UTakeAudioComponent::Get(this))
	{
		const FName* Fallback = CueFallbacks.Find(Name);
		A->PlayCue(Fallback ? *Fallback : Name, Volume);
	}
}

void AScene7Booth::Act(USkeletalMeshComponent* Mesh, FName Anim, bool bLoop, float Rate)
{
	if (!Mesh) { return; }
	TObjectPtr<UAnimationAsset>* A = Anims.Find(Anim);
	if (!A || !*A) { return; }
	if (Mesh->GetAnimationMode() != EAnimationMode::AnimationSingleNode)
	{
		Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	}
	Mesh->PlayAnimation(*A, bLoop);
	Mesh->SetPlayRate(Rate);
}

void AScene7Booth::ActVance(FName Anim, bool bLoop, float Rate)
{
	if (ATakeCharacter* V = GetPovCharacter()) { Act(V->GetMesh(), Anim, bLoop, Rate); }
}

void AScene7Booth::VanceBackToBlueprint()
{
	ATakeCharacter* V = GetPovCharacter();
	if (!V || !V->GetMesh()) { return; }
	USkeletalMeshComponent* M = V->GetMesh();
	if (M->GetAnimationMode() != EAnimationMode::AnimationBlueprint)
	{
		M->SetPlayRate(1.f);
		M->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	}
	if (bHaveVanceMesh) { M->SetRelativeLocation(VanceMeshRel); }
}

void AScene7Booth::PlaceVance(const FVector& Local, float Yaw)
{
	ATakeCharacter* V = GetPovCharacter();
	if (!V) { return; }
	const float Half = V->GetCapsuleComponent() ? V->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	const FRotator R(0.f, GetActorRotation().Yaw + Yaw, 0.f);
	V->SetActorLocationAndRotation(W(Local) + FVector(0.f, 0.f, Half + 2.f), R, false, nullptr, ETeleportType::TeleportPhysics);
	if (V->GetCharacterMovement()) { V->GetCharacterMovement()->StopMovementImmediately(); }
	if (AController* C = V->GetController()) { C->SetControlRotation(R); }
}

void AScene7Booth::FaceVance(float Yaw)
{
	if (ATakeCharacter* V = GetPovCharacter())
	{
		V->SetActorRotation(FRotator(0.f, GetActorRotation().Yaw + Yaw, 0.f));
	}
}

void AScene7Booth::Puppet(USkeletalMeshComponent* Mesh, const FVector& Local, float FaceYaw)
{
	// The mannequin faces +Y in its own space: turn it -90 to face FaceYaw.
	Mesh->SetRelativeLocationAndRotation(Local, FRotator(0.f, FaceYaw - 90.f, 0.f));
}

void AScene7Booth::PutCanisterOn(ATakeCharacter* Char, bool bInHand)
{
	if (!Char || !Char->GetMesh()) { return; }
	Canister->SetHiddenInGame(false);
	CanisterLid->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	Canister->AttachToComponent(Char->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform,
		bInHand ? CanisterBlockBone : CanisterBone);
	Canister->SetRelativeTransform(bInHand ? CanisterInHand : CanisterInArm);
}

bool AScene7Booth::AttachCanisterTo(const UObject* WorldContextObject, ATakeCharacter* Character)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World || !Character) { return false; }
	for (TActorIterator<AScene7Booth> It(World); It; ++It)
	{
		It->PutCanisterOn(Character, false);
		return true;
	}
	return false;
}

void AScene7Booth::BurstAt(UNiagaraSystem* System, const FVector& World, float Scale)
{
	if (System)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, System, World, FRotator::ZeroRotator, FVector(Scale), true);
	}
}

void AScene7Booth::SetAlarm(bool bOn)
{
	bAlarmOn = bOn;
	if (bOn)
	{
		if (USoundBase* S = FindS7Sound(TEXT("sfx_s7_alarm")))
		{
			AlarmAudio->SetSound(S);
		}
		else if (UTakeAudioComponent* A = UTakeAudioComponent::Get(this))
		{
			AlarmAudio->SetSound(A->FindSound(TEXT("sfx_alarm")));
		}
		AlarmAudio->Play();
	}
	else
	{
		AlarmAudio->Stop();
		StrobeA->SetIntensity(0.f);
		StrobeB->SetIntensity(0.f);
	}
}

void AScene7Booth::BreakGlass()
{
	if (bGlassBroken) { return; }
	bGlassBroken = true;
	Glass->SetHiddenInGame(true);
	Cue(TEXT("sfx_s7_window_crash"));
	BurstAt(GlassFX, W(GlassPoint), 1.2f);
	ShardT = 0.f;
	ShardVel.SetNum(Shards.Num());
	for (int32 i = 0; i < Shards.Num(); ++i)
	{
		const float Y = FMath::FRandRange(-120.f, 120.f), Z = FMath::FRandRange(-80.f, 90.f);
		Shards[i]->SetRelativeLocation(GlassPoint + FVector(5.f, Y, Z));
		Shards[i]->SetRelativeRotation(FRotator(FMath::FRandRange(-40.f, 40.f), 0.f, FMath::FRandRange(-60.f, 60.f)));
		Shards[i]->SetHiddenInGame(false);
		ShardVel[i] = FVector(FMath::FRandRange(250.f, 520.f), Y * 1.5f, FMath::FRandRange(50.f, 260.f));
	}
}

void AScene7Booth::ApplySwap(float A)
{
	// 0..0.2 the lid opens; 0.2..0.45 the master print comes off the spindle into the canister; 0.45..0.7 the dummy
	// goes on; 0.7..0.85 the lid shuts; 0.85..1 the canister's lid latches.
	const FTransform* MR = Rest.Find(MasterReel);
	const FTransform* DR = Rest.Find(DummyReel);
	const FTransform* CR = Rest.Find(Canister);
	const FTransform* LR = Rest.Find(CanisterLid);
	if (!MR || !DR || !CR || !LR) { return; }
	const float Open = Ease(A / 0.2f) * (1.f - Ease((A - 0.7f) / 0.15f));
	MagazineHinge->SetRelativeRotation(FRotator(0.f, LidOpenDegrees * Open, 0.f));

	const FVector Spindle = MR->GetLocation();
	const FVector InCan = CR->GetLocation() + FVector(0.f, 0.f, 6.f);
	const float M = Ease((A - 0.2f) / 0.25f);
	FVector MP = FMath::Lerp(Spindle, InCan, M) + FVector(0.f, -30.f * FMath::Sin(M * PI), 35.f * FMath::Sin(M * PI));
	MasterReel->SetRelativeLocation(MP);
	MasterReel->SetRelativeRotation(FQuat::Slerp(MR->GetRotation(), DR->GetRotation(), M));
	MasterReel->SetHiddenInGame(A >= 0.95f);

	const float D = Ease((A - 0.45f) / 0.25f);
	FVector DP = FMath::Lerp(DR->GetLocation(), Spindle, D) + FVector(0.f, -30.f * FMath::Sin(D * PI), 35.f * FMath::Sin(D * PI));
	DummyReel->SetRelativeLocation(DP);
	DummyReel->SetRelativeRotation(FQuat::Slerp(DR->GetRotation(), MR->GetRotation(), D));

	const float L = Ease((A - 0.85f) / 0.15f);
	CanisterLid->SetRelativeLocation(FMath::Lerp(LR->GetLocation(), FVector(0.f, 0.f, 50.f), L));

	// Sounds as the hold crosses each step.
	auto Cross = [&](float T) { return LastSwapA < T && A >= T; };
	if (Cross(0.01f)) { Cue(TEXT("sfx_s7_mag_open")); }
	if (Cross(0.22f) || Cross(0.47f)) { Cue(TEXT("sfx_s7_reel_swap")); }
	if (Cross(0.88f)) { Cue(TEXT("sfx_s7_canister_latch")); }
	LastSwapA = A;
}

void AScene7Booth::PutCanisterOnVance(bool bInHand)
{
	PutCanisterOn(GetPovCharacter(), bInHand);
}

// ---- the venue, the escape: helpers ---------------------------------------------------------------------------------

void AScene7Booth::PuppetWorld(USkeletalMeshComponent* Mesh, const FVector& World, float FaceYaw)
{
	// The mannequin faces +Y in its own space: turn it -90 to face FaceYaw.
	Mesh->SetWorldLocationAndRotation(World, FRotator(0.f, FaceYaw - 90.f, 0.f));
}

FVector AScene7Booth::PathPoint(float Dist, FVector* OutDir) const
{
	if (GuardPath.Num() == 0) { return GetActorLocation(); }
	if (GuardPath.Num() == 1) { if (OutDir) { *OutDir = FVector::ForwardVector; } return GuardPath[0]; }
	float Left = FMath::Max(0.f, Dist);
	for (int32 i = 0; i + 1 < GuardPath.Num(); ++i)
	{
		const FVector Seg = GuardPath[i + 1] - GuardPath[i];
		const float Len = Seg.Size();
		if (Left <= Len || i + 2 == GuardPath.Num())
		{
			if (OutDir) { *OutDir = Seg.GetSafeNormal2D(); }
			return GuardPath[i] + Seg * FMath::Clamp(Len > 0.f ? Left / Len : 1.f, 0.f, 1.f);
		}
		Left -= Len;
	}
	return GuardPath.Last();
}

void AScene7Booth::PlaceStrobes(const TArray<FVector>& At)
{
	if (At.IsValidIndex(0)) { StrobeA->SetWorldLocation(At[0]); }
	if (At.IsValidIndex(1)) { StrobeB->SetWorldLocation(At[1]); }
	StrobeA->SetAttenuationRadius(2600.f);
	StrobeB->SetAttenuationRadius(2600.f);
}

void AScene7Booth::SetVenueHidden(bool bHide)
{
	if (bHide)
	{
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (It->ActorHasTag(VenueHideTag) && !It->IsHidden())
			{
				It->SetActorHiddenInGame(true);
				VenueHidden.Add(*It);
			}
		}
	}
	else
	{
		for (AActor* A : VenueHidden) { if (A) { A->SetActorHiddenInGame(false); } }
		VenueHidden.Reset();
	}
}

FVector AScene7Booth::VanceFeet() const
{
	const ATakeCharacter* V = GetPovCharacter();
	if (!V) { return FVector::ZeroVector; }
	const float Half = V->GetCapsuleComponent() ? V->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	return V->GetActorLocation() - FVector(0.f, 0.f, Half);
}

void AScene7Booth::PointGuide()
{
	UTakeGuideSubsystem* G = UTakeGuideSubsystem::Get(this);
	if (!G) { return; }
	if (Stage == EScene7Stage::Escape && EscapeSteps.IsValidIndex(EscapeStep))
	{
		G->SetGoal(EscapeSteps[EscapeStep].At, EscapeSteps[EscapeStep].Objective);
	}
	else { G->ClearGoal(); }
}

// ---- the scene ----------------------------------------------------------------------------------------------------

void AScene7Booth::BeginScene()
{
	Super::BeginScene();

	// The old box's stairs, landing and front wall would stand inside the booth (the overlay also hides them for good).
	Hidden.Reset();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(HideTag) && !It->IsHidden())
		{
			It->SetActorHiddenInGame(true);
			It->SetActorEnableCollision(false);
			Hidden.Add(*It);
		}
	}
	ATakeCharacter* V = GetPovCharacter();
	if (V && !bHaveVanceMesh && V->GetMesh())
	{
		VanceMeshRel = V->GetMesh()->GetRelativeLocation();
		bHaveVanceMesh = true;
	}
	if (V && V->GetCharacterMovement()) { SavedWalkSpeed = V->GetCharacterMovement()->MaxWalkSpeed; }

	LampSpill->SetIntensity(LampCandela);
	if (USoundBase* Hum = FindS7Sound(TEXT("sfx_s7_projector_hum")))
	{
		HumAudio->SetSound(Hum);
		HumAudio->Play();
	}
	// The film still runs from Scene 5 (a cold jump: start it, the house dark), until the reel comes off.
	for (TActorIterator<AHouseDimmer> It(GetWorld()); It; ++It) { if (!It->IsScreening()) { It->BeginScreening(0.f); } }

	// s7-reelswap's part 1 played the swap (it sets Canister): skip ours, the canister goes under his arm.
	bSwapActor = HasFlag(TEXT("Canister"));
	bPlayerWalks = false;
	FreezeVance(V, true);
	SetStage(EScene7Stage::Approach);
	UE_LOG(LogScene7, Log, TEXT("S7 booth: begin (swap already played %d)"), bSwapActor ? 1 : 0);
}

void AScene7Booth::StartAtCheckpoint()
{
}

void AScene7Booth::EndScene()
{
	for (AActor* A : Hidden)
	{
		if (A) { A->SetActorHiddenInGame(false); A->SetActorEnableCollision(true); }
	}
	Hidden.Reset();
	SetVenueHidden(false);
	Pendleton->SetHiddenInGame(true);
	Bodyguard->SetHiddenInGame(true);
	HumAudio->Stop();
	SetAlarm(false);
	VanceBackToBlueprint();
	if (ATakeCharacter* V = GetPovCharacter())
	{
		if (UCharacterMovementComponent* M = V->GetCharacterMovement())
		{
			M->MaxWalkSpeed = SavedWalkSpeed > 0.f ? SavedWalkSpeed : V->WalkSpeed;
		}
	}
	if (UTakeGuideSubsystem* G = UTakeGuideSubsystem::Get(this)) { G->ClearGoal(); }
	bPlayerWalks = true;
	FreezeVance(GetPovCharacter(), false);
	Super::EndScene();
}

void AScene7Booth::ResetScene()
{
	// The canister back on the bench (it may be under an arm).
	if (Canister->GetAttachParent() != Root)
	{
		Canister->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
	}
	for (const TPair<USceneComponent*, FTransform>& R : Rest)
	{
		if (R.Key) { R.Key->SetRelativeTransform(R.Value); }
	}
	DoorSouth->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MasterReel->SetHiddenInGame(false);
	for (USkeletalMeshComponent* P : { Pendleton.Get(), Bodyguard.Get(), Cole.Get() })
	{
		P->SetHiddenInGame(true);
		P->Stop();
	}
	SetAlarm(false);
	SetVenueHidden(false);
	FireLight->SetIntensity(0.f);
	bSwapDone = false;
	bUseDown = false;
	SwapHeld = 0.f;
	LastSwapA = 0.f;
	EscapeStep = 0;
	GuardDist = -1.f;
	bStairShot = false;
	bLeftMark = false;
	VanceBackToBlueprint();
	bPlayerWalks = true;
	FreezeVance(GetPovCharacter(), false);
	Stage = EScene7Stage::Approach;
	StageT = PrevT = StageReal = 0.f;
}

void AScene7Booth::SetStage(EScene7Stage NewStage)
{
	Stage = NewStage;
	StageT = PrevT = StageReal = 0.f;
	UE_LOG(LogScene7, Log, TEXT("S7 booth stage %d"), (int32)NewStage);
	PointGuide();
}

void AScene7Booth::BeginPrompt(int32 Index) {}
void AScene7Booth::PromptHit() {}
void AScene7Booth::PromptMissed() {}

bool AScene7Booth::HandleSceneKey(const FKey& Key, bool bPressed)
{
	if (Stage == EScene7Stage::Swap)
	{
		if (Key == EKeys::E || Key == EKeys::LeftMouseButton)
		{
			bUseDown = bPressed;
			if (bPressed && SwapHeld <= 0.f) { ActVance(TEXT("Use"), true); }
			return true;
		}
		return false;
	}
	// Use is the film's own, not the director's.
	return Key == EKeys::E || Key == EKeys::LeftMouseButton || Key == EKeys::Q || Key == EKeys::F;
}

void AScene7Booth::TickScene(float DeltaSeconds)
{
	const float RealDt = FApp::GetDeltaTime();
	PrevT = StageT;
	StageT += DeltaSeconds;
	StageReal += RealDt;
	ATakeCharacter* V = GetPovCharacter();
	StepFX(DeltaSeconds, RealDt);

	// Pendleton and the bodyguard on their path (once they've started up the grand stair).
	if (GuardDist >= 0.f && Stage == EScene7Stage::Escape)
	{
		float PathLen = 0.f;
		for (int32 i = 0; i + 1 < GuardPath.Num(); ++i) { PathLen += FVector::Dist(GuardPath[i], GuardPath[i + 1]); }
		const bool bWasWalking = GuardDist < PathLen;
		GuardDist = FMath::Min(GuardDist + GuardSpeed * DeltaSeconds, PathLen);
		FVector DirP = FVector::ForwardVector, DirB = FVector::ForwardVector;
		const FVector P = PathPoint(GuardDist, &DirP);
		const FVector B = PathPoint(GuardDist - GuardGap, &DirB);
		const bool bWalking = GuardDist < PathLen;
		// At the top they stop and look along the balcony for him.
		const float YawP = bWalking ? DirP.Rotation().Yaw : (VanceFeet() - P).Rotation().Yaw;
		const float YawB = bWalking ? DirB.Rotation().Yaw : (VanceFeet() - B).Rotation().Yaw;
		PuppetWorld(Pendleton, P, YawP);
		PuppetWorld(Bodyguard, B, YawB);
		if (bWasWalking && !bWalking)
		{
			Act(Pendleton, TEXT("Stand"), true);
			Act(Bodyguard, TEXT("Idle"), true);
		}
	}

	switch (Stage)
	{
	case EScene7Stage::Approach:
	{
		// Part 0 walked him onto the mark (a cold jump: StartSpot). Square him up to the magazine.
		PlaceVance(UseSpot, UseYaw);
		if (bSwapActor)
		{
			// The swap played in part 1: the booth's props show it done, the canister under his arm.
			ApplySwap(1.f);
			LastSwapA = 1.f;
			bSwapDone = true;
			MasterReel->SetHiddenInGame(true);
			PutCanisterOnVance(false);
			HumAudio->Stop();
			LampSpill->SetIntensity(0.f);
			SetStage(EScene7Stage::Swapped);
			StageT = PrevT = SwappedSeconds;	// straight to the cut
			break;
		}
		SetStage(EScene7Stage::Swap);
		break;
	}
	case EScene7Stage::Swap:
	{
		if (GS7Auto && StageT > 0.8f) { bUseDown = true; }
		if (bUseDown) { SwapHeld += DeltaSeconds; }
		else if (SwapHeld > 0.f && SwapHeld < SwapHoldSeconds)
		{
			// Let go early: nothing half done.
			SwapHeld = 0.f;
			ApplySwap(0.f);
			LastSwapA = 0.f;
			VanceBackToBlueprint();
			Shot(NAME_None);
		}
		if (SwapHeld > 0.f && CurrentShot != TEXT("Swap")) { Shot(TEXT("Swap")); }
		ApplySwap(FMath::Clamp(SwapHeld / SwapHoldSeconds, 0.f, 1.f));
		if (SwapHeld >= SwapHoldSeconds)
		{
			bSwapDone = true;
			bUseDown = false;
			MasterReel->SetHiddenInGame(true);
			PutCanisterOnVance(false);
			SetFlag(TEXT("Canister"));
			// The projector runs on with nothing in the gate: the film is gone.
			HumAudio->Stop();
			LampSpill->SetIntensity(0.f);
			ActVance(TEXT("Stand"), true);
			Say(TEXT("VANCE"), SwappedLine);
			SetStage(EScene7Stage::Swapped);
		}
		break;
	}
	case EScene7Stage::Swapped:
	{
		if (StageT >= SwappedSeconds + 0.6f)
		{
			// THE CUT: the auditorium. The film dies on the screen; Pendleton at the podium.
			SetVenueHidden(true);
			Pendleton->SetHiddenInGame(false);
			PuppetWorld(Pendleton, PodiumSpot, PodiumYaw);
			Act(Pendleton, TEXT("Stand"), true);
			for (TActorIterator<AHouseDimmer> It(GetWorld()); It; ++It) { It->FlashScreen(1.f); It->EndScreening(); }
			Cue(TEXT("sfx_film_burn"));
			Shot(TEXT("Venue"));
			SetStage(EScene7Stage::Venue);
		}
		break;
	}
	case EScene7Stage::Venue:
	{
		if (At(1.1f)) { Shot(TEXT("Pendleton")); Act(Pendleton, TEXT("Refuse"), false); }
		if (At(1.4f)) { Say(TEXT("PENDLETON"), VenueLine, FindS7Sound(TEXT("vo_s7_pendleton_lock"))); }
		if (At(3.6f))
		{
			PlaceStrobes(StrobesVenue);
			SetAlarm(true);
			Cue(TEXT("sfx_s7_crowd_scream"));
			Shot(TEXT("Venue"));
		}
		if (StageT >= VenueSeconds)
		{
			// Back on Vance in the booth: the stair door has mag-locked; the lobby door is the only way out.
			SetVenueHidden(false);
			PlaceStrobes(StrobesEscape);
			DoorSouthHinge->SetRelativeRotation(FRotator::ZeroRotator);
			DoorSouth->SetCollisionProfileName(TEXT("BlockAll"));
			DoorSouth->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Cue(TEXT("sfx_s7_door_click"), 0.8f);
			// Pendleton and his bodyguard wait at the grand stair's foot (off screen) until the stair cut.
			if (GuardPath.Num() > 0)
			{
				PuppetWorld(Pendleton, GuardPath[0], 0.f);
				PuppetWorld(Bodyguard, GuardPath[0] - FVector(GuardGap, 0.f, 0.f), 0.f);
			}
			Bodyguard->SetHiddenInGame(false);
			Act(Pendleton, TEXT("Stand"), true);
			Act(Bodyguard, TEXT("Idle"), true);
			GuardDist = -1.f;
			VanceBackToBlueprint();
			Shot(NAME_None);
			if (V && V->GetCharacterMovement()) { V->GetCharacterMovement()->MaxWalkSpeed = EscapeSpeed; }
			FreezeVance(V, false);
			bPlayerWalks = true;
			EscapeStep = 0;
			bStairShot = false;
			Say(TEXT("MAX (V.O.)"), TEXT("Alarm's tripped and your stair's mag-locked! Lobby door, Vance. Go!"));
			SetStage(EScene7Stage::Escape);
		}
		break;
	}
	case EScene7Stage::Escape:
	{
		const FVector Feet = VanceFeet();
		for (int32 i = EscapeSteps.Num() - 1; i >= EscapeStep; --i)
		{
			const FScene7RouteStep& S = EscapeSteps[i];
			if (FVector::Dist2D(Feet, S.At) <= S.Radius && FMath::Abs(Feet.Z - S.At.Z) < 150.f)
			{
				for (int32 j = EscapeStep; j <= i; ++j)
				{
					if (!EscapeSteps[j].Line.IsEmpty()) { Say(EscapeSteps[j].Speaker, EscapeSteps[j].Line); }
				}
				// Out on the balcony: cut to them starting up the grand stair.
				if (!bStairShot)
				{
					bStairShot = true;
					StairShotT = 0.f;
					GuardDist = 0.f;
					Act(Pendleton, TEXT("Walk"), true, 0.75f);
					Act(Bodyguard, TEXT("Walk"), true, 0.75f);
					Shot(TEXT("Stair"));
					FreezeVance(V, true);
					bPlayerWalks = false;
				}
				EscapeStep = i + 1;
				PointGuide();
				break;
			}
		}
		if (bStairShot && StairShotT >= 0.f)
		{
			StairShotT += DeltaSeconds;
			if (StairShotT >= 1.8f)
			{
				StairShotT = -1.f;
				Shot(NAME_None);
				FreezeVance(V, false);
				bPlayerWalks = true;
			}
		}
		// Into the security room: caught.
		if (StairShotT < 0.f && Feet.X >= RoomMin.X && Feet.X <= RoomMax.X && Feet.Y >= RoomMin.Y && Feet.Y <= RoomMax.Y
			&& FMath::Abs(Feet.Z - RoomMin.Z) < 150.f)
		{
			bPlayerWalks = false;
			if (UTakeGuideSubsystem* G = UTakeGuideSubsystem::Get(this)) { G->ClearGoal(); }
			// They come in by the door behind him.
			Pendleton->SetHiddenInGame(false);
			Bodyguard->SetHiddenInGame(false);
			PuppetWorld(Pendleton, RoomDoorOutside + FVector(40.f, 60.f, 0.f), -90.f);
			PuppetWorld(Bodyguard, RoomDoorOutside + FVector(-60.f, 120.f, 0.f), -90.f);
			Act(Pendleton, TEXT("Walk"), true, 0.6f);
			Act(Bodyguard, TEXT("Walk"), true, 0.6f);
			PendletonFrom = Pendleton->GetComponentLocation();
			BodyguardFrom = Bodyguard->GetComponentLocation();
			GuardDist = -1.f;
			if (V && V->GetCharacterMovement())
			{
				V->GetCharacterMovement()->MaxWalkSpeed = SavedWalkSpeed > 0.f ? SavedWalkSpeed : V->WalkSpeed;
			}
			Cue(TEXT("sfx_s7_door_kick"), 0.7f);
			Shot(TEXT("Caught"));
			SetStage(EScene7Stage::Caught);
		}
		break;
	}
	case EScene7Stage::Caught:
	{
		// Vance takes the last steps to his mark and turns to face them; they walk to theirs.
		if (V)
		{
			if (FVector::Dist2D(VanceFeet(), VanceFightMark) > 15.f && StageT < 1.4f) { V->MoveToward(VanceFightMark); }
			const float Yaw = FMath::FixedTurn(V->GetActorRotation().Yaw, VanceFightYaw, 300.f * DeltaSeconds);
			V->SetActorRotation(FRotator(0.f, Yaw, 0.f));
		}
		const float A = Ease(StageT / 1.3f);
		PuppetWorld(Pendleton, FMath::Lerp(PendletonFrom, PendletonFightMark, A), VanceFightYaw + 180.f);
		PuppetWorld(Bodyguard, FMath::Lerp(BodyguardFrom, BodyguardFightMark, A), VanceFightYaw + 180.f);
		if (At(1.3f))
		{
			Act(Pendleton, TEXT("Talk"), true);
			Act(Bodyguard, TEXT("Idle"), true);
			Say(TEXT("PENDLETON"), CaughtLine);
		}
		if (StageT >= CaughtSeconds)
		{
			if (V && V->GetCharacterMovement()) { V->GetCharacterMovement()->StopMovementImmediately(); }
			if (V) { V->SetActorRotation(FRotator(0.f, VanceFightYaw, 0.f)); }
			// s7-fight's part 3 takes it from here, on the same marks (our puppets hide at EndScene).
			SetStage(EScene7Stage::Done);
			FinishScene();
		}
		break;
	}
	default:
		break;
	}
}

void AScene7Booth::StepFX(float Dt, float RealDt)
{
	// The reels turn while the projector runs (until the master print comes off).
	if (!bSwapDone)
	{
		LowerReel->AddLocalRotation(ReelSpin * Dt);
		if (SwapHeld <= 0.f) { MasterReel->AddLocalRotation(ReelSpin * Dt); }
		LampSpill->SetIntensity(LampCandela * (0.85f + 0.15f * FMath::Sin(GetWorld()->GetTimeSeconds() * 151.f)));
	}
	if (bAlarmOn)
	{
		const double Now = FPlatformTime::Seconds();
		const float Phase = FMath::Frac(static_cast<float>(Now) * StrobeHz);
		StrobeA->SetIntensity(Phase < 0.35f ? StrobeIntensity : 0.f);
		StrobeB->SetIntensity((Phase >= 0.5f && Phase < 0.85f) ? StrobeIntensity * 0.7f : 0.f);
		if (!AlarmAudio->IsPlaying()) { AlarmAudio->Play(); }
	}
}

void AScene7Booth::DrawSceneHud(UCanvas* Canvas)
{
	if (!Canvas || Stage != EScene7Stage::Swap || bSwapActor) { return; }
	FTakeHudKit Kit;
	Kit.Begin(Canvas, FPlatformTime::Seconds(), 0.f);
	const float CX = Kit.CX();
	const float Y = Kit.H * 0.80f;
	const float A = FMath::Clamp(SwapHeld / SwapHoldSeconds, 0.f, 1.f);
	const FString Words = A > 0.f ? TEXT("SWAPPING THE REEL") : TEXT("HOLD: SWAP THE REEL");
	const float TW = Kit.Measure(TakeStyle::EType::Line, Words);
	const float KW = Kit.KeyWidth(TEXT("E"), false);
	const float X0 = CX - (TW + KW + Kit.Px(16.f)) * 0.5f;
	Kit.Key(TEXT("E"), X0, Y, FTakeInk::Of(TakeStyle::Paper, TakeStyle::Julian));
	Kit.Text(TakeStyle::EType::Line, Words, X0 + KW + Kit.Px(16.f), Y, TakeStyle::Paper);
	const float BW = Kit.Px(320.f), BH = Kit.Px(5.f);
	Kit.Rect(CX - BW * 0.5f, Y + Kit.Px(30.f), BW, BH, TakeStyle::Ink * FLinearColor(1, 1, 1, 0.6f));
	Kit.Rect(CX - BW * 0.5f, Y + Kit.Px(30.f), BW * A, BH, TakeStyle::Julian);
	Kit.Text(TakeStyle::EType::Goal, TEXT("The master print is in the projector's upper magazine."), CX, Y - Kit.Px(44.f),
		TakeStyle::Bone, ETakeAlign::Centre);
}
