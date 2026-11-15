// See Scene7Booth.h. Owner: the s7-booth session.
#include "Script/Scene7Booth.h"
#include "Script/ScriptSequencer.h"
#include "TakeCharacter.h"
#include "TakeDirector.h"
#include "TakeAudio.h"
#include "TakeHudKit.h"
#include "TakeStyle.h"
#include "HouseDimmer.h"
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
	Slug = FText::FromString(TEXT("INT. PROJECTION BOOTH - NIGHT"));
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
		{ TEXT("Use"), TEXT("/Game/Take/Anim/Mixamo/Clips/Use.Use") },
		{ TEXT("Lift"), TEXT("/Game/Take/Anim/Mixamo/Clips/Lift.Lift") },
		{ TEXT("Refuse"), TEXT("/Game/Take/Anim/Mixamo/Clips/Refuse.Refuse") },
		{ TEXT("Talk"), TEXT("/Game/Take/Anim/Mixamo/Clips/Talk_A.Talk_A") },
		{ TEXT("Stand"), TEXT("/Game/Take/Anim/Mixamo/Clips/StandIdle_A.StandIdle_A") },
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
	Shots.Add(TEXT("Swap"), MakeShot(FVector(-90.f, -40.f, 200.f), FVector(30.f, -150.f, 190.f), 40.f));
	Shots.Add(TEXT("Door"), MakeShot(FVector(-150.f, -100.f, 170.f), FVector(110.f, 500.f, 110.f), 45.f));
	Shots.Add(TEXT("Insert"), MakeShot(FVector(-130.f, 100.f, 125.f), FVector(-50.f, 40.f, 100.f), 30.f));
	Shots.Add(TEXT("Line"), MakeShot(FVector(-55.f, 15.f, 172.f), FVector(60.f, 300.f, 160.f), 35.f));
	Shots.Add(TEXT("Alarm"), MakeShot(FVector(40.f, 330.f, 150.f), FVector(-110.f, 470.f, 140.f), 45.f));
	Shots.Add(TEXT("Screen"), MakeShot(FVector(150.f, 150.f, 170.f), FVector(1000.f, 100.f, 60.f), 50.f));
	Shots.Add(TEXT("Parry"), MakeShot(FVector(150.f, -200.f, 90.f), FVector(-20.f, 150.f, 140.f), 50.f));
	Shots.Add(TEXT("ParryHit"), MakeShot(FVector(-110.f, 60.f, 110.f), FVector(30.f, -130.f, 140.f), 40.f));
	Shots.Add(TEXT("Block"), MakeShot(FVector(60.f, -40.f, 100.f), FVector(0.f, 170.f, 160.f), 40.f));
	Shots.Add(TEXT("BlockHit"), MakeShot(FVector(60.f, 120.f, 165.f), FVector(-30.f, 60.f, 160.f), 30.f));
	Shots.Add(TEXT("Throw"), MakeShot(FVector(-150.f, 280.f, 120.f), FVector(60.f, 160.f, 100.f), 45.f));
	Shots.Add(TEXT("Fall"), MakeShot(FVector(700.f, 350.f, -400.f), FVector(250.f, 150.f, 0.f), 50.f));
	Shots.Add(TEXT("Cole"), MakeShot(FVector(-120.f, -150.f, 170.f), FVector(110.f, -480.f, 130.f), 45.f));
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

// ---- the scene ----------------------------------------------------------------------------------------------------

void AScene7Booth::BeginScene()
{
	Super::BeginScene();

	// The old box's stairs, landing and front wall would stand inside the booth: hidden while the scene runs.
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

	if (ATakeCharacter* V = GetPovCharacter())
	{
		if (!bHaveVanceMesh && V->GetMesh())
		{
			VanceMeshRel = V->GetMesh()->GetRelativeLocation();
			bHaveVanceMesh = true;
		}
	}

	if (FirstTry.Num() != 3) { FirstTry = { true, true, true }; }
	LampSpill->SetIntensity(LampCandela);
	if (USoundBase* Hum = FindS7Sound(TEXT("sfx_s7_projector_hum")))
	{
		HumAudio->SetSound(Hum);
		HumAudio->Play();
	}

	if (bFailing && bCheckpointAlarm)
	{
		bFailing = false;
		StartAtCheckpoint();
		return;
	}
	bFailing = false;
	bCheckpointAlarm = false;
	bPlayerWalks = true;
	FreezeVance(GetPovCharacter(), false);
	// The film still runs from Scene 5 (or a direct Jump 7 / retake: start it, the house dark), until the burn-out.
	for (TActorIterator<AHouseDimmer> It(GetWorld()); It; ++It) { if (!It->IsScreening()) { It->BeginScreening(0.f); } }
	SetStage(EScene7Stage::Approach);
}

void AScene7Booth::StartAtCheckpoint()
{
	// The retake rolls from the alarm: the swap done, both of them in, the strobes going.
	bSwapDone = true;
	bPlayerWalks = false;
	FreezeVance(GetPovCharacter(), true);
	ApplySwap(1.f);
	LastSwapA = 1.f;
	MasterReel->SetHiddenInGame(true);
	PlaceVance(FightSpot, FightYaw);
	PutCanisterOn(GetPovCharacter(), false);
	DoorNorthHinge->SetRelativeRotation(FRotator(0.f, DoorOpenDegrees, 0.f));
	Pendleton->SetHiddenInGame(false);
	Bodyguard->SetHiddenInGame(false);
	Puppet(Pendleton, AlarmSpot, -60.f);
	Puppet(Bodyguard, BodyguardSpot, -90.f);
	Act(Pendleton, TEXT("Walk"), true, 0.7f);
	Act(Bodyguard, TEXT("Idle"), true);
	ActVance(TEXT("Idle"), true);
	AlarmLever->SetRelativeLocation(Rest.FindRef(AlarmLever).GetLocation() + FVector(0.f, 0.f, -10.f));
	SetAlarm(true);
	for (TActorIterator<AHouseDimmer> It(GetWorld()); It; ++It) { It->EndScreening(); }
	SetStage(EScene7Stage::Alarm);
	StageT = PrevT = AlarmSeconds - 1.2f;
	Shot(TEXT("Parry"));
}

void AScene7Booth::EndScene()
{
	if (ATakeDirector* D = ATakeDirector::Get(this))
	{
		D->EndSlow(SlowPrompt, 0.1f);
		D->EndSlow(SlowFall, 0.1f);
	}
	for (AActor* A : Hidden)
	{
		if (A)
		{
			A->SetActorHiddenInGame(false);
			A->SetActorEnableCollision(true);
		}
	}
	Hidden.Reset();
	HumAudio->Stop();
	SetAlarm(false);
	VanceBackToBlueprint();
	bPlayerWalks = true;
	FreezeVance(GetPovCharacter(), false);
	Super::EndScene();
}

void AScene7Booth::ResetScene()
{
	if (ATakeDirector* D = ATakeDirector::Get(this))
	{
		D->EndSlow(SlowPrompt, 0.05f);
		D->EndSlow(SlowFall, 0.05f);
	}
	// The canister back on the bench (it may be under an arm).
	if (Canister->GetAttachParent() != Root)
	{
		Canister->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
	}
	for (const TPair<USceneComponent*, FTransform>& R : Rest)
	{
		if (R.Key) { R.Key->SetRelativeTransform(R.Value); }
	}
	MasterReel->SetHiddenInGame(false);
	Glass->SetHiddenInGame(false);
	Stiletto->SetHiddenInGame(true);
	Grenade->SetHiddenInGame(true);
	CurtainHeap->SetHiddenInGame(true);
	for (UStaticMeshComponent* S : Shards) { S->SetHiddenInGame(true); }
	for (UStaticMeshComponent* S : SmokePuffs) { S->SetHiddenInGame(true); }
	for (USkeletalMeshComponent* P : { Pendleton.Get(), Bodyguard.Get(), Cole.Get() })
	{
		P->SetHiddenInGame(true);
		P->Stop();
	}
	if (Stiletto->GetAttachParent() != Root)
	{
		Stiletto->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
	}
	SetAlarm(false);
	FireLight->SetIntensity(0.f);
	bFire = false;
	bGlassBroken = false;
	bSmoke = false;
	SmokeT = ShardT = -1.f;
	bSwapDone = false;
	bUseDown = false;
	SwapHeld = 0.f;
	LastSwapA = 0.f;
	PromptIndex = 0;
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
	UE_LOG(LogScene7, Log, TEXT("S7 stage %d prompt %d"), (int32)NewStage, PromptIndex);
}

void AScene7Booth::BeginPrompt(int32 Index)
{
	PromptIndex = Index;
	SetStage(EScene7Stage::Prompt);
	if (ATakeDirector* D = ATakeDirector::Get(this)) { D->BeginSlow(SlowPrompt, PromptSlowScale, 0.08f); }
}

void AScene7Booth::PromptHit()
{
	if (ATakeDirector* D = ATakeDirector::Get(this)) { D->EndSlow(SlowPrompt, 0.06f); }
	FirstTry[PromptIndex] = false;
	SetStage(EScene7Stage::Strike);
}

void AScene7Booth::PromptMissed()
{
	if (ATakeDirector* D = ATakeDirector::Get(this)) { D->EndSlow(SlowPrompt, 0.06f); }
	FirstTry[PromptIndex] = false;
	SetStage(EScene7Stage::Miss);
}

bool AScene7Booth::HandleSceneKey(const FKey& Key, bool bPressed)
{
	if (Stage == EScene7Stage::Approach)
	{
		if (Key == EKeys::E || Key == EKeys::LeftMouseButton)
		{
			bUseDown = bPressed;
			return true;
		}
		return false;
	}
	if (Stage == EScene7Stage::Prompt)
	{
		if (!bPressed) { return true; }
		// Movement and camera keys don't count as an answer.
		if (Key == EKeys::W || Key == EKeys::A || Key == EKeys::S || Key == EKeys::D || (Key.IsMouseButton() && Key != EKeys::LeftMouseButton) || Key == EKeys::Escape)
		{
			return false;
		}
		const FKey Want = PromptKeys.IsValidIndex(PromptIndex) ? PromptKeys[PromptIndex] : EKeys::E;
		if (Key == Want) { PromptHit(); }
		else { PromptMissed(); }
		return true;
	}
	// Everything else is the film playing: swallow Use so the director doesn't act on it.
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

	switch (Stage)
	{
	case EScene7Stage::Approach:
	{
		if (GS7Auto && !bUseDown && StageT > 1.5f)
		{
			PlaceVance(UseSpot, UseYaw);
			bUseDown = true;
		}
		const bool bNear = V && FVector::Dist2D(V->GetActorLocation(), W(UseSpot)) <= UseRadius;
		if (bNear && bUseDown)
		{
			if (SwapHeld <= 0.f)
			{
				PlaceVance(UseSpot, UseYaw);
				ActVance(TEXT("Use"), true);
				Shot(TEXT("Swap"));
				bPlayerWalks = false;
	FreezeVance(GetPovCharacter(), true);
			}
			SwapHeld += DeltaSeconds;
			ApplySwap(FMath::Clamp(SwapHeld / SwapHoldSeconds, 0.f, 1.f));
			if (SwapHeld >= SwapHoldSeconds)
			{
				bSwapDone = true;
				PutCanisterOn(V, false);
				ActVance(TEXT("Idle"), true);
				SetStage(EScene7Stage::Swapped);
			}
		}
		else if (SwapHeld > 0.f)
		{
			// Let go early: the heist's rule, nothing is half done.
			SwapHeld = 0.f;
			LastSwapA = 0.f;
			ApplySwap(0.f);
			MasterReel->SetHiddenInGame(false);
			VanceBackToBlueprint();
			Shot(NAME_None);
			bPlayerWalks = true;
	FreezeVance(GetPovCharacter(), false);
		}
		break;
	}
	case EScene7Stage::Swapped:
		if (StageT >= SwappedSeconds) { SetStage(EScene7Stage::Door); }
		break;

	case EScene7Stage::Door:
	{
		if (At(0.f) || StageT == DeltaSeconds)
		{
			Cue(TEXT("sfx_s7_door_click"));
			Shot(TEXT("Door"));
			PlaceVance(FightSpot, FightYaw);
			ActVance(TEXT("Idle"), true);
			Pendleton->SetHiddenInGame(false);
			Bodyguard->SetHiddenInGame(false);
			Act(Pendleton, TEXT("Walk"), true, 0.55f);
			Act(Bodyguard, TEXT("Walk"), true, 0.6f);
		}
		DoorNorthHinge->SetRelativeRotation(FRotator(0.f, DoorOpenDegrees * Ease(StageT / 0.5f), 0.f));
		const float P = Ease((StageT - 0.3f) / 1.5f), B = Ease((StageT - 0.1f) / 1.5f);
		Puppet(Pendleton, FMath::Lerp(PendletonDoor, PendletonSpot, P), -90.f);
		Puppet(Bodyguard, FMath::Lerp(BodyguardDoor, BodyguardSpot, B), -100.f);
		if (At(1.7f)) { Act(Bodyguard, TEXT("Idle"), true); }
		if (At(1.9f)) { Act(Pendleton, TEXT("Stand"), true); }
		if (StageT >= DoorSeconds) { SetStage(EScene7Stage::Line); }
		break;
	}
	case EScene7Stage::Line:
		if (StageT == DeltaSeconds) { Shot(TEXT("Insert")); }
		if (At(0.9f))
		{
			Shot(TEXT("Line"));
			Act(Pendleton, TEXT("Refuse"), false);
			Say(TEXT("PENDLETON"), PendletonLine, FindS7Sound(TEXT("vo_s7_pendleton_lock")));
		}
		if (StageT >= LineSeconds) { SetStage(EScene7Stage::Alarm); }
		break;

	case EScene7Stage::Alarm:
	{
		if (StageT == DeltaSeconds)
		{
			Shot(TEXT("Alarm"));
			Act(Pendleton, TEXT("Walk"), true, 0.7f);
			bCheckpointAlarm = true;
		}
		if (StageT < 0.6f) { Puppet(Pendleton, FMath::Lerp(PendletonSpot, AlarmSpot, Ease(StageT / 0.55f)), 135.f); }
		if (At(0.55f)) { Act(Pendleton, TEXT("Punch"), false, 1.3f); }
		if (At(0.85f))
		{
			AlarmLever->SetRelativeLocation(Rest.FindRef(AlarmLever).GetLocation() + FVector(0.f, 0.f, -10.f));
			SetAlarm(true);
		}
		if (At(1.5f))
		{
			Shot(TEXT("Screen"));
			Cue(TEXT("sfx_film_burn"));
			Cue(TEXT("sfx_s7_crowd_scream"), 0.9f);
			for (TActorIterator<AHouseDimmer> It(GetWorld()); It; ++It) { It->EndScreening(); }
		}
		if (At(2.8f))
		{
			Shot(TEXT("Parry"));
			Act(Pendleton, TEXT("Walk"), true, 0.7f);
		}
		if (StageT > 2.8f) { Puppet(Pendleton, FMath::Lerp(AlarmSpot, PendletonSpot, Ease((StageT - 2.8f) / 0.8f)), -60.f); }
		if (At(3.6f)) { Act(Pendleton, TEXT("Stand"), true); }
		if (StageT >= AlarmSeconds)
		{
			Puppet(Pendleton, PendletonSpot, -90.f);
			PromptIndex = 0;
			BodyguardFrom = BodyguardSpot;
			SetStage(EScene7Stage::Windup);
		}
		break;
	}
	case EScene7Stage::Windup:
	{
		if (StageT == DeltaSeconds)
		{
			if (PromptIndex == 0)
			{
				Shot(TEXT("Parry"));
				Act(Bodyguard, TEXT("Hook"), false, 1.1f);
			}
			else if (PromptIndex == 1)
			{
				Shot(TEXT("Block"));
				FaceVance(FightYaw);
				Stiletto->SetHiddenInGame(false);
				Stiletto->AttachToComponent(Pendleton, FAttachmentTransformRules::KeepRelativeTransform, StilettoBone);
				Stiletto->SetRelativeTransform(StilettoInHand);
				Act(Pendleton, TEXT("Slash"), false, 1.0f);
				PendletonFrom = PendletonSpot;
			}
			else
			{
				Shot(TEXT("Throw"));
				Act(Pendleton, TEXT("Slash"), false, 0.9f);
				PendletonFrom = Pendleton->GetRelativeLocation();
			}
		}
		const float A = Ease(StageT / WindupSeconds);
		if (PromptIndex == 0)
		{
			Puppet(Bodyguard, FMath::Lerp(BodyguardFrom, FightSpot + FVector(10.f, 95.f, 0.f), A), -95.f);
		}
		else
		{
			const FVector To = FightSpot + FVector(PromptIndex == 1 ? 15.f : 40.f, 95.f, 0.f);
			Puppet(Pendleton, FMath::Lerp(PendletonFrom, To, A), -90.f);
		}
		if (StageT >= WindupSeconds) { BeginPrompt(PromptIndex); }
		break;
	}
	case EScene7Stage::Prompt:
	{
		const float Limit = PromptRealSeconds + (FirstTry.IsValidIndex(PromptIndex) && FirstTry[PromptIndex] && GetRetakes() == 0 ? FirstPromptBonus : 0.f);
		if (GS7Auto && StageReal >= 0.6f) { PromptHit(); }
		else if (StageReal >= Limit) { PromptMissed(); }
		break;
	}
	case EScene7Stage::Strike:
	{
		if (PromptIndex == 0)
		{
			// Vance ducks the hook; the bodyguard's own weight carries him head first into the hot housing.
			if (V && V->GetMesh() && bHaveVanceMesh)
			{
				const float Dip = StageT < 0.5f ? FMath::Sin(FMath::Clamp(StageT / 0.5f, 0.f, 1.f) * PI) : 0.f;
				V->GetMesh()->SetRelativeLocation(VanceMeshRel + FVector(0.f, 0.f, -38.f * Dip));
			}
			if (StageT == DeltaSeconds) { Cue(TEXT("sfx_s7_punch"), 0.6f); }
			const FVector From = FightSpot + FVector(10.f, 95.f, 0.f);
			const float A = Ease((StageT - 0.1f) / 0.4f);
			Puppet(Bodyguard, FMath::Lerp(From, SlamSpot, A), -100.f);
			if (At(0.35f)) { ActVance(TEXT("Punch"), false, 1.2f); }
			if (At(0.5f))
			{
				Shot(TEXT("ParryHit"));
				Cue(TEXT("sfx_s7_head_slam"));
				BurstAt(SparksFX, W(SlamSpot + FVector(20.f, -40.f, 150.f)), 1.f);
				Act(Bodyguard, TEXT("FallFront"), false, 1.f);
			}
			if (At(0.75f))
			{
				bFire = true;
				FireSpawnIn = 0.f;
				Cue(TEXT("sfx_s7_fire_whoosh"));
			}
			if (StageT >= StrikeSeconds)
			{
				ActVance(TEXT("Idle"), true);
				PromptIndex = 1;
				SetStage(EScene7Stage::Windup);
			}
		}
		else if (PromptIndex == 1)
		{
			if (StageT == DeltaSeconds)
			{
				PutCanisterOn(V, true);
				ActVance(TEXT("Lift"), false, 1.6f);
			}
			if (At(0.15f))
			{
				Shot(TEXT("BlockHit"));
				Cue(TEXT("sfx_s7_clang"));
				BurstAt(SparksFX, Canister->GetComponentLocation(), 0.6f);
			}
			if (At(0.35f)) { Act(Pendleton, TEXT("HitMed"), false, 1.f); }
			if (StageT > 0.35f)
			{
				const FVector From = FightSpot + FVector(15.f, 95.f, 0.f);
				Puppet(Pendleton, FMath::Lerp(From, FightSpot + FVector(60.f, 170.f, 0.f), Ease((StageT - 0.35f) / 0.5f)), -110.f);
			}
			if (At(1.1f))
			{
				PutCanisterOn(V, false);
				ActVance(TEXT("Idle"), true);
			}
			if (StageT >= StrikeSeconds * 0.85f)
			{
				PromptIndex = 2;
				SetStage(EScene7Stage::Windup);
			}
		}
		else
		{
			if (StageT == DeltaSeconds)
			{
				ActVance(TEXT("Sweep"), false, 1.3f);
				Cue(TEXT("sfx_s7_punch"), 0.7f);
			}
			if (At(0.3f))
			{
				Act(Pendleton, TEXT("FallBack"), false, 1.4f);
				PendletonFrom = Pendleton->GetRelativeLocation();
			}
			if (StageT > 0.3f)
			{
				Puppet(Pendleton, FMath::Lerp(PendletonFrom, GlassPoint - FVector(20.f, 0.f, 60.f), Ease((StageT - 0.3f) / 0.35f)), 180.f);
			}
			if (StageT >= 0.65f)
			{
				PendletonFallFrom = Pendleton->GetRelativeLocation();
				SetStage(EScene7Stage::Fall);
			}
		}
		break;
	}
	case EScene7Stage::Miss:
	{
		if (StageT == DeltaSeconds)
		{
			Shot(TEXT("Miss"));
			Cue(TEXT("sfx_s7_punch"));
			ActVance(PromptIndex == 1 ? TEXT("HitMed") : TEXT("HitHeavy"), false, 1.f);
		}
		if (StageT >= MissSeconds)
		{
			bFailing = true;
			static const FName Reasons[] = { TEXT("He missed the parry"), TEXT("He missed the block"), TEXT("He missed the counter") };
			FailScene(Reasons[FMath::Clamp(PromptIndex, 0, 2)]);
			SetStage(EScene7Stage::Done);
		}
		break;
	}
	case EScene7Stage::Fall:
	{
		if (StageT == DeltaSeconds)
		{
			BreakGlass();
			Shot(TEXT("Fall"));
			if (ATakeDirector* D = ATakeDirector::Get(this)) { D->BeginSlow(SlowFall, FallSlowScale, 0.05f); }
			Stiletto->SetHiddenInGame(true);
		}
		const float FallT = 0.8f;
		if (StageT <= FallT)
		{
			const float A = FMath::Clamp(StageT / FallT, 0.f, 1.f);
			// Out through the glass, a short arc, then down onto the curtains.
			FVector P = FMath::Lerp(PendletonFallFrom, LandSpot, A);
			P.Z = FMath::Lerp(PendletonFallFrom.Z, LandSpot.Z, A * A) + 60.f * FMath::Sin(A * PI) * (1.f - A);
			Puppet(Pendleton, P, 180.f);
		}
		if (At(FallT))
		{
			Puppet(Pendleton, LandSpot + FVector(0.f, 0.f, 25.f), 180.f);
			CurtainHeap->SetHiddenInGame(false);
			Cue(TEXT("sfx_s7_body_fall"));
			if (ATakeDirector* D = ATakeDirector::Get(this)) { D->EndSlow(SlowFall, 0.2f); }
		}
		if (StageT >= FallSeconds * FallSlowScale + FallT) { SetStage(EScene7Stage::Cole); }
		break;
	}
	case EScene7Stage::Cole:
	{
		if (StageT == DeltaSeconds)
		{
			Shot(TEXT("Cole"));
			Cue(TEXT("sfx_s7_door_kick"));
			Cole->SetHiddenInGame(false);
			Act(Cole, TEXT("Walk"), true, 0.8f);
			FaceVance(-90.f);
		}
		DoorSouthHinge->SetRelativeRotation(FRotator(0.f, -DoorOpenDegrees * Ease(StageT / 0.18f), 0.f));
		if (StageT < 0.6f) { Puppet(Cole, FMath::Lerp(ColeDoor, ColeSpot, Ease(StageT / 0.6f)), 90.f); }
		if (At(0.6f))
		{
			Puppet(Cole, ColeSpot, -90.f);
			Act(Cole, TEXT("Punch"), false, 1.f);
		}
		if (At(0.85f))
		{
			Grenade->SetHiddenInGame(false);
			Cue(TEXT("sfx_s7_smoke"));
		}
		if (StageT > 0.85f && StageT < 1.45f)
		{
			const float A = (StageT - 0.85f) / 0.6f;
			FVector P = FMath::Lerp(ColeSpot + FVector(0.f, 0.f, 140.f), SmokeSpot, A);
			P.Z += 80.f * FMath::Sin(A * PI);
			Grenade->SetRelativeLocation(P);
		}
		if (At(1.45f))
		{
			Grenade->SetHiddenInGame(true);
			bSmoke = true;
			SmokeT = 0.f;
			BurstAt(FireFX, W(SmokeSpot), 0.5f);
		}
		if (At(1.3f))
		{
			Act(Cole, TEXT("Talk"), true);
			Say(TEXT("COLE"), ColeLine, FindS7Sound(TEXT("vo_s7_cole_move")));
		}
		if (StageT >= ColeSeconds)
		{
			SetStage(EScene7Stage::Done);
			SetFlag(TEXT("Canister"));
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
	// The reels turn while the projector runs (the master print until it comes off, the dummy once it's on).
	if (!bFire)
	{
		LowerReel->AddLocalRotation(ReelSpin * Dt);
		if (SwapHeld <= 0.f && !bSwapDone) { MasterReel->AddLocalRotation(ReelSpin * Dt); }
		if (bSwapDone) { DummyReel->AddLocalRotation(ReelSpin * Dt); }
	}
	// The lamp house flickers with the shutter.
	LampSpill->SetIntensity(LampCandela * (0.85f + 0.15f * FMath::Sin(GetWorld()->GetTimeSeconds() * 151.f)));

	if (bAlarmOn)
	{
		const double Now = FPlatformTime::Seconds();
		const float Phase = FMath::Frac(static_cast<float>(Now) * StrobeHz);
		StrobeA->SetIntensity(Phase < 0.35f ? StrobeIntensity : 0.f);
		StrobeB->SetIntensity((Phase >= 0.5f && Phase < 0.85f) ? StrobeIntensity * 0.7f : 0.f);
		if (!AlarmAudio->IsPlaying()) { AlarmAudio->Play(); }
	}
	if (bFire)
	{
		FireLight->SetIntensity(30.f + 18.f * FMath::PerlinNoise1D(GetWorld()->GetTimeSeconds() * 9.f));
		FireSpawnIn -= Dt;
		if (FireSpawnIn <= 0.f)
		{
			FireSpawnIn = 0.45f;
			BurstAt(FireFX, Magazine->GetComponentLocation() + FVector(0.f, 0.f, 20.f), 0.35f);
		}
	}
	if (ShardT >= 0.f)
	{
		ShardT += Dt;
		for (int32 i = 0; i < Shards.Num() && i < ShardVel.Num(); ++i)
		{
			ShardVel[i].Z -= 980.f * Dt;
			Shards[i]->AddRelativeLocation(ShardVel[i] * Dt);
			Shards[i]->AddRelativeRotation(FRotator(200.f * Dt, 0.f, 310.f * Dt));
			if (Shards[i]->GetRelativeLocation().Z < LandSpot.Z) { Shards[i]->SetHiddenInGame(true); }
		}
		if (ShardT > 3.f) { ShardT = -1.f; }
	}
	if (bSmoke && SmokeT >= 0.f)
	{
		SmokeT += Dt;
		for (int32 i = 0; i < SmokePuffs.Num(); ++i)
		{
			const float A = FMath::Clamp((SmokeT - 0.12f * i) / 2.f, 0.f, 1.f);
			if (A <= 0.f) { continue; }
			UStaticMeshComponent* P = SmokePuffs[i];
			P->SetHiddenInGame(false);
			const float Ang = i * 1.7f;
			P->SetRelativeLocation(SmokeSpot + FVector(FMath::Cos(Ang) * 60.f * A, FMath::Sin(Ang) * 60.f * A, 40.f + 160.f * A + 20.f * i));
			P->SetRelativeScale3D(FVector(0.3f + 2.2f * A));
		}
	}
}

void AScene7Booth::DrawSceneHud(UCanvas* Canvas)
{
	if (!Canvas) { return; }
	FTakeHudKit Kit;
	Kit.Begin(Canvas, FPlatformTime::Seconds(), 0.f);
	const float CX = Kit.CX();

	if (Stage == EScene7Stage::Approach)
	{
		const ATakeCharacter* V = GetPovCharacter();
		const bool bNear = V && FVector::Dist2D(V->GetActorLocation(), W(UseSpot)) <= UseRadius;
		const float Y = Kit.H * 0.80f;
		if (bNear)
		{
			const float A = FMath::Clamp(SwapHeld / SwapHoldSeconds, 0.f, 1.f);
			const FString Words = A > 0.f ? TEXT("SWAPPING THE REEL") : TEXT("HOLD: SWAP THE REEL");
			const float TW = Kit.Measure(TakeStyle::EType::Line, Words);
			const float KW = Kit.KeyWidth(TEXT("E"), false);
			const float X0 = CX - (TW + KW + Kit.Px(16.f)) * 0.5f;
			Kit.Key(TEXT("E"), X0, Y, FTakeInk::Of(TakeStyle::Paper, TakeStyle::Julian));
			Kit.Text(TakeStyle::EType::Line, Words, X0 + KW + Kit.Px(16.f), Y, TakeStyle::Paper);
			// The hold bar under it.
			const float BW = Kit.Px(320.f), BH = Kit.Px(5.f);
			Kit.Rect(CX - BW * 0.5f, Y + Kit.Px(30.f), BW, BH, TakeStyle::Ink * FLinearColor(1, 1, 1, 0.6f));
			Kit.Rect(CX - BW * 0.5f, Y + Kit.Px(30.f), BW * A, BH, TakeStyle::Julian);
		}
		else if (GetSceneSeconds() < 12.f || SwapHeld <= 0.f)
		{
			Kit.Text(TakeStyle::EType::Goal, TEXT("The master print is in the projector's upper magazine."), CX, Y, TakeStyle::Bone, ETakeAlign::Centre);
		}
		return;
	}

	if (Stage == EScene7Stage::Prompt && PromptKeys.IsValidIndex(PromptIndex))
	{
		const float Limit = PromptRealSeconds + (FirstTry.IsValidIndex(PromptIndex) && FirstTry[PromptIndex] && GetRetakes() == 0 ? FirstPromptBonus : 0.f);
		const float Left = FMath::Clamp(1.f - StageReal / Limit, 0.f, 1.f);
		const float Pop = FMath::Clamp(StageReal / 0.12f, 0.f, 1.f);
		const float Y = Kit.H * 0.62f;
		const FString KeyText = PromptKeys[PromptIndex].GetDisplayName().ToString();
		const FString Words = PromptWords.IsValidIndex(PromptIndex) ? PromptWords[PromptIndex] : TEXT("");

		// A dark band, the key big in the centre, the move under it, and the time running out as a bar.
		Kit.Rect(0.f, Y - Kit.Px(80.f), Kit.W, Kit.Px(170.f), TakeStyle::Ink * FLinearColor(1, 1, 1, 0.55f * Pop));
		Kit.Push(FVector2D(CX, Y), 0.f, 1.6f + 0.6f * (1.f - Pop));
		const float KW = Kit.KeyWidth(KeyText, false);
		Kit.Key(KeyText, CX - KW * 0.5f, Y, FTakeInk::Of(TakeStyle::Paper, TakeStyle::Julian));
		Kit.Pop();
		Kit.Text(TakeStyle::EType::Title, Words, CX, Y + Kit.Px(58.f), TakeStyle::Paper, ETakeAlign::Centre);
		const float BW = Kit.Px(420.f), BH = Kit.Px(6.f);
		Kit.Rect(CX - BW * 0.5f, Y + Kit.Px(84.f), BW * Left, BH, Left < 0.35f ? TakeStyle::Danger : TakeStyle::Julian);
		Kit.Text(TakeStyle::EType::Label, FString::Printf(TEXT("%d / 3"), PromptIndex + 1), CX, Y - Kit.Px(58.f), TakeStyle::Ash, ETakeAlign::Centre);
	}
}
