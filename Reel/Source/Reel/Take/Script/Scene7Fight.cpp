// Scene 7's fist fight. Owner: the s7-fight session. See Scene7Fight.h for the beats and the hand-offs.
#include "Script/Scene7Fight.h"
#include "Script/Scene7Booth.h"
#include "Script/ScriptScene.h"
#include "GameFramework/HUD.h"
#include "TakeCharacter.h"
#include "TakeDirector.h"
#include "TakeHudKit.h"
#include "TakeStyle.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
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

DEFINE_LOG_CATEGORY_STATIC(LogScene7Fight, Log, All);

// Test hook. Reel.S7Fight.Auto 1: every prompt answered right 0.5 s in; 2: the first go of each beat misses (to shoot
// the CUT card and the retake), the retake is answered right. 0 (default): the player fights. Reel.S7Fight.Start
// starts the fight now, wherever Vance is (for shots without the route).
static int32 GFightAuto = 0;
static FAutoConsoleVariableRef GFightAutoVar(TEXT("Reel.S7Fight.Auto"), GFightAuto,
	TEXT("1: Scene 7's fight answers every prompt itself. 2: it misses each beat once first. 0: the player fights."));

static FAutoConsoleCommandWithWorld GFightStartCmd(TEXT("Reel.S7Fight.Start"),
	TEXT("Start Scene 7's fist fight now, in the security room."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		// The console hands us the editor world in play-in-editor: start the fight in the game's world.
		UWorld* Play = World;
		if (GEngine)
		{
			for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
			{
				if ((Ctx.WorldType == EWorldType::PIE || Ctx.WorldType == EWorldType::Game) && Ctx.World()) { Play = Ctx.World(); break; }
			}
		}
		if (AScene7Fight* F = AScene7Fight::Find(Play)) { F->StartFight(nullptr); }
	}));

namespace
{
	const FName FightSlowPrompt(TEXT("S7FightPrompt"));
	const FName FightSlowStop(TEXT("S7FightHitStop"));
	const FName FightSlowMo(TEXT("S7FightSlowMo"));
	const FName FightSlowCut(TEXT("S7FightCut"));

	USkeletalMeshComponent* MakeFightPuppet(AActor* Owner, USceneComponent* Parent, FName Name)
	{
		USkeletalMeshComponent* M = Owner->CreateDefaultSubobject<USkeletalMeshComponent>(Name);
		M->SetupAttachment(Parent);
		M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		M->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		M->SetHiddenInGame(true);
		M->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		return M;
	}

	FFightShot MakeFightShot(const FVector& From, EFightSubject Subject, FName Bone, float Fov, float EndFov = 0.f,
		float ZoomSeconds = 0.5f, const FVector& Drift = FVector::ZeroVector, float Handheld = 0.4f,
		const FVector& LookOffset = FVector::ZeroVector)
	{
		FFightShot S;
		S.From = From;
		S.Subject = Subject;
		S.Bone = Bone;
		S.Fov = Fov;
		S.EndFov = EndFov;
		S.ZoomSeconds = ZoomSeconds;
		S.Drift = Drift;
		S.Handheld = Handheld;
		S.LookOffset = LookOffset;
		return S;
	}

	AScene7Booth* FindFightBooth(UWorld* World)
	{
		if (!World) { return nullptr; }
		for (TActorIterator<AScene7Booth> It(World); It; ++It) { return *It; }
		return nullptr;
	}

	void FreezeFightVance(ATakeCharacter* V, bool bFrozen)
	{
		UCharacterMovementComponent* M = V ? V->GetCharacterMovement() : nullptr;
		if (!M) { return; }
		if (bFrozen) { M->StopMovementImmediately(); M->DisableMovement(); }
		else if (M->MovementMode == MOVE_None) { M->SetMovementMode(MOVE_Walking); }
	}
}

AScene7Fight::AScene7Fight()
{
	PrimaryActorTick.bCanEverTick = true;
	// After the player controller has read this frame's keys and the driving scene has ticked.
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Guard = MakeFightPuppet(this, Root, TEXT("Guard"));
	Pendleton = MakeFightPuppet(this, Root, TEXT("Pendleton"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeF(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TrimF(TEXT("/Game/Take/Materials/MI_Box_Trim.MI_Box_Trim"));
	Stiletto = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Stiletto"));
	Stiletto->SetupAttachment(Root);
	Stiletto->SetStaticMesh(CubeF.Object);
	Stiletto->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Stiletto->SetCanEverAffectNavigation(false);
	Stiletto->SetHiddenInGame(true);
	if (TrimF.Succeeded()) { Stiletto->SetMaterial(0, TrimF.Object); }
	StilettoInHand = FTransform(FRotator::ZeroRotator, FVector(16.f, 2.f, 0.f), FVector(0.24f, 0.012f, 0.028f));
	Stiletto->SetRelativeScale3D(StilettoInHand.GetScale3D());

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> GuardF(TEXT("/Game/Take/Cast/NPCs/Bodyguard/SKM_NPC_Bodyguard.SKM_NPC_Bodyguard"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> PendF(TEXT("/Game/Take/Cast/NPCs/Vance/SKM_NPC_Vance.SKM_NPC_Vance"));
	GuardMesh = GuardF.Object;
	PendletonMesh = PendF.Object;

	struct FPath { const TCHAR* Name; const TCHAR* Path; };
	static const FPath AnimPaths[] = {
		{ TEXT("Idle"), TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle") },
		{ TEXT("Walk"), TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd.MF_Unarmed_Jog_Fwd") },
		{ TEXT("Jab"), TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01") },
		{ TEXT("Slash"), TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02.MM_Attack_02") },
		{ TEXT("Sweep"), TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03.MM_Attack_03") },
		{ TEXT("Hook"), TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack.MM_ChargedAttack") },
		{ TEXT("HitHeavy"), TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Hvy_01.MM_HitReact_Front_Hvy_01") },
		{ TEXT("HitMed"), TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Med_01.MM_HitReact_Front_Med_01") },
		{ TEXT("HitLight"), TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_02.MM_HitReact_Front_Lgt_02") },
		{ TEXT("HitBack"), TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Back_Med_01.MM_HitReact_Back_Med_01") },
		{ TEXT("FallBack"), TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01.MM_Death_Back_01") },
		{ TEXT("FallFront"), TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01.MM_Death_Front_01") },
		{ TEXT("Stand"), TEXT("/Game/Take/Anim/Mocap/StandIdle_B.StandIdle_B") },
	};
	for (const FPath& P : AnimPaths)
	{
		ConstructorHelpers::FObjectFinder<UAnimationAsset> F(P.Path);
		if (F.Succeeded()) { Anims.Add(P.Name, F.Object); }
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> SparksF(TEXT("/Game/Take/FX/NS_Take_Sparks.NS_Take_Sparks"));
	SparksFX = SparksF.Object;

	static const FPath SoundPaths[] = {
		{ TEXT("Punch"), TEXT("/Game/Take/Audio/Script/S7/sfx_s7_punch.sfx_s7_punch") },
		{ TEXT("Clang"), TEXT("/Game/Take/Audio/Script/S7/sfx_s7_clang.sfx_s7_clang") },
		{ TEXT("Slam"), TEXT("/Game/Take/Audio/Script/S7/sfx_s7_head_slam.sfx_s7_head_slam") },
		{ TEXT("Fall"), TEXT("/Game/Take/Audio/Script/S7/sfx_s7_body_fall.sfx_s7_body_fall") },
		{ TEXT("Crash"), TEXT("/Game/Take/Audio/Script/S7/sfx_s7_window_crash.sfx_s7_window_crash") },
		{ TEXT("Grunt1"), TEXT("/Game/Take/Audio/Script/S7/vo_s7_vance_grunt_1.vo_s7_vance_grunt_1") },
		{ TEXT("Grunt2"), TEXT("/Game/Take/Audio/Script/S7/vo_s7_vance_grunt_2.vo_s7_vance_grunt_2") },
	};
	for (const FPath& P : SoundPaths)
	{
		ConstructorHelpers::FObjectFinder<USoundBase> F(P.Path);
		if (F.Succeeded()) { Sounds.Add(P.Name, F.Object); }
	}

	PromptKeys = { EKeys::Q, EKeys::E, EKeys::F };
	PromptWords = { TEXT("PARRY THE HOOK"), TEXT("BLOCK WITH THE CANISTER"), TEXT("SWEEP HIS LEGS") };
	MissWords = { TEXT("Vance walked into the hook."), TEXT("The blade got through."), TEXT("Pendleton bowled him over.") };
	WindupSeconds = { 0.55f, 1.0f, 0.45f };
	StrikeSeconds = { 2.7f, 1.9f, 1.9f };

	// Coverage, fight space: Vance at the origin facing +X (north); the room runs X -320..280, Y -550..550; the
	// monitor bank is behind him at X -295. Wides for the geography, close-ups on the contacts.
	typedef EFightSubject S;
	Shots.Add(TEXT("Wide"), MakeFightShot(FVector(40.f, -470.f, 185.f), S::VanceGuard, TEXT("spine_03"), 64.f, 54.f, 2.4f, FVector(0.f, 60.f, -10.f), 0.25f));
	Shots.Add(TEXT("VanceCU"), MakeFightShot(FVector(150.f, -115.f, 160.f), S::Vance, TEXT("head"), 40.f, 16.f, 0.35f, FVector::ZeroVector, 0.35f));
	Shots.Add(TEXT("OTSGuard"), MakeFightShot(FVector(-75.f, 38.f, 172.f), S::Guard, TEXT("head"), 46.f, 40.f, 0.6f, FVector(15.f, 0.f, 0.f), 0.7f));
	Shots.Add(TEXT("DuckLow"), MakeFightShot(FVector(40.f, -180.f, 50.f), S::VanceGuard, TEXT("spine_03"), 60.f, 0.f, 0.5f, FVector(0.f, 20.f, 0.f), 0.9f));
	Shots.Add(TEXT("SlamCU"), MakeFightShot(FVector(-120.f, -150.f, 150.f), S::Guard, TEXT("head"), 38.f, 30.f, 0.4f, FVector::ZeroVector, 1.1f));
	Shots.Add(TEXT("GuardDown"), MakeFightShot(FVector(-40.f, -90.f, 250.f), S::Guard, TEXT("pelvis"), 52.f, 0.f, 0.5f, FVector(0.f, 0.f, 15.f), 0.3f));
	Shots.Add(TEXT("BladeInsert"), MakeFightShot(FVector(60.f, 40.f, 110.f), S::Stiletto, NAME_None, 30.f, 22.f, 0.5f, FVector::ZeroVector, 0.5f));
	Shots.Add(TEXT("Profile"), MakeFightShot(FVector(55.f, -250.f, 150.f), S::VancePendleton, TEXT("neck_01"), 44.f, 38.f, 1.0f, FVector(0.f, 25.f, 0.f), 0.6f));
	Shots.Add(TEXT("BlockCU"), MakeFightShot(FVector(45.f, -85.f, 165.f), S::Canister, NAME_None, 34.f, 0.f, 0.5f, FVector::ZeroVector, 1.2f));
	Shots.Add(TEXT("LungeWide"), MakeFightShot(FVector(-170.f, -120.f, 150.f), S::Pendleton, TEXT("spine_03"), 50.f, 36.f, 0.45f, FVector::ZeroVector, 0.8f));
	Shots.Add(TEXT("SweepLow"), MakeFightShot(FVector(90.f, -210.f, 22.f), S::VancePendleton, TEXT("pelvis"), 64.f, 0.f, 0.5f, FVector(0.f, 30.f, 0.f), 0.5f, FVector(0.f, 0.f, 10.f)));
	Shots.Add(TEXT("Overhead"), MakeFightShot(FVector(110.f, 25.f, 260.f), S::Pendleton, TEXT("pelvis"), 60.f, 50.f, 1.5f, FVector(0.f, 0.f, 15.f), 0.3f));
	Shots.Add(TEXT("Final"), MakeFightShot(FVector(-230.f, -170.f, 140.f), S::Vance, TEXT("head"), 48.f, 38.f, 2.0f, FVector(40.f, 0.f, 0.f), 0.3f));
	Shots.Add(TEXT("MissGuard"), MakeFightShot(FVector(150.f, 30.f, 168.f), S::Vance, TEXT("head"), 36.f, 0.f, 0.5f, FVector::ZeroVector, 1.6f));
	Shots.Add(TEXT("MissBlade"), MakeFightShot(FVector(140.f, -40.f, 160.f), S::Vance, TEXT("spine_05"), 34.f, 0.f, 0.5f, FVector::ZeroVector, 1.6f));
	Shots.Add(TEXT("MissLunge"), MakeFightShot(FVector(40.f, -230.f, 110.f), S::Vance, TEXT("pelvis"), 52.f, 0.f, 0.5f, FVector::ZeroVector, 1.6f));
}

void AScene7Fight::BeginPlay()
{
	Super::BeginPlay();
	if (GuardMesh) { Guard->SetSkeletalMesh(GuardMesh); }
	if (PendletonMesh) { Pendleton->SetSkeletalMesh(PendletonMesh); }
	Guard->SetRelativeScale3D(FVector(GuardScale));

	for (TObjectPtr<ACameraActor>* Slot : { &CamA, &CamB })
	{
		FActorSpawnParameters P;
		P.Owner = this;
		ACameraActor* Cam = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), GetActorTransform(), P);
		if (Cam)
		{
			Cam->GetCameraComponent()->bConstrainAspectRatio = false;
			*Slot = Cam;
		}
	}
	HudHandle = AHUD::OnHUDPostRender.AddUObject(this, &AScene7Fight::DrawHud);
}

void AScene7Fight::EndPlay(const EEndPlayReason::Type Reason)
{
	AHUD::OnHUDPostRender.Remove(HudHandle);
	Super::EndPlay(Reason);
}

AScene7Fight* AScene7Fight::Find(const UObject* Ctx)
{
	UWorld* World = Ctx ? Ctx->GetWorld() : nullptr;
	if (!World) { return nullptr; }
	for (TActorIterator<AScene7Fight> It(World); It; ++It) { return *It; }
	return nullptr;
}

// ---- the driving scene's calls ------------------------------------------------------------------------------------

void AScene7Fight::StartFight(AScriptScene* Scene)
{
	OwnerScene = Scene;
	BeginFight();
}

void AScene7Fight::ResetFight()
{
	ClearFight(true);
}

bool AScene7Fight::HandleFightKey(const FKey& Key, bool bPressed)
{
	const bool bFightKey = PromptKeys.Contains(Key) || Key == EKeys::SpaceBar || Key == EKeys::LeftMouseButton || Key == EKeys::E;
	if (!IsRunning()) { return false; }
	if (Stage != EFightStage::Prompt || !bPressed || GFightAuto) { return bFightKey; }
	// Movement, look and AnyKey don't count as an answer (AnyKey reaches us first with every press).
	if (!bFightKey || Key == EKeys::AnyKey) { return false; }
	const int32 B = FMath::Clamp(Beat, 0, 2);
	const FKey Want = PromptKeys.IsValidIndex(B) ? PromptKeys[B] : EKeys::E;
	Answer(Key == Want);
	return true;
}

// ---- helpers --------------------------------------------------------------------------------------------------------

ATakeCharacter* AScene7Fight::GetVance() const
{
	if (OwnerScene) { if (ATakeCharacter* V = OwnerScene->GetPovCharacter()) { return V; } }
	// No driving scene (the test hook): the playing scene's POV character, else player 0's pawn.
	for (TActorIterator<AScriptScene> It(GetWorld()); It; ++It)
	{
		if (It->IsActive()) { if (ATakeCharacter* V = It->GetPovCharacter()) { return V; } }
	}
	return Cast<ATakeCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
}

APlayerController* AScene7Fight::GetPC() const
{
	return UGameplayStatics::GetPlayerController(this, 0);
}

UAnimationAsset* AScene7Fight::FindAnim(FName Anim) const
{
	// A role with no clip of its own borrows the nearest one.
	static const TMap<FName, FName> Stand = {
		{ TEXT("Stance"), TEXT("Idle") }, { TEXT("Duck"), TEXT("HitLight") }, { TEXT("Block"), TEXT("HitLight") },
		{ TEXT("Lunge"), TEXT("Jab") }, { TEXT("Stand"), TEXT("Idle") }, { TEXT("Shove"), TEXT("Slash") },
		{ TEXT("Stumble"), TEXT("HitBack") }, { TEXT("SweepFall"), TEXT("FallBack") },
	};
	for (int32 Hop = 0; Hop < 3 && !Anim.IsNone(); ++Hop)
	{
		if (const TObjectPtr<UAnimationAsset>* A = Anims.Find(Anim)) { if (*A) { return *A; } }
		const FName* Next = Stand.Find(Anim);
		Anim = Next ? *Next : NAME_None;
	}
	return nullptr;
}

void AScene7Fight::Act(USkeletalMeshComponent* Mesh, FName Anim, bool bLoop, float Rate)
{
	UAnimationAsset* A = Mesh ? FindAnim(Anim) : nullptr;
	if (!A) { return; }
	if (Mesh->GetAnimationMode() != EAnimationMode::AnimationSingleNode)
	{
		Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	}
	Mesh->PlayAnimation(A, bLoop);
	Mesh->SetPlayRate(Rate);
}

void AScene7Fight::ActEnd(USkeletalMeshComponent* Mesh, FName Anim)
{
	UAnimationAsset* A = Mesh ? FindAnim(Anim) : nullptr;
	if (!A) { return; }
	Act(Mesh, Anim, false, 1.f);
	Mesh->SetPosition(FMath::Max(0.f, A->GetPlayLength() - 0.02f), false);
}

void AScene7Fight::ActVance(FName Anim, bool bLoop, float Rate)
{
	if (ATakeCharacter* V = GetVance()) { Act(V->GetMesh(), Anim, bLoop, Rate); }
}

void AScene7Fight::Slide(FSlide& S, const FVector& From, const FVector& To, float FromYaw, float Yaw, float T0, float T1)
{
	S.From = From;
	S.To = To;
	S.FromYaw = FromYaw;
	S.Yaw = Yaw;
	S.T0 = FightT + T0;
	S.T1 = FightT + FMath::Max(T1, T0 + 0.01f);
	S.bOn = true;
}

void AScene7Fight::StepSlides()
{
	auto Step = [this](FSlide& S, FVector& Pos, float& Yaw)
	{
		if (!S.bOn || FightT < S.T0) { return; }
		const float A = Ease((FightT - S.T0) / (S.T1 - S.T0));
		Pos = FMath::Lerp(S.From, S.To, A);
		// Turn the short way round, done in the first half of the move.
		Yaw = S.FromYaw + FMath::FindDeltaAngleDegrees(S.FromYaw, S.Yaw) * FMath::Clamp(A * 2.f, 0.f, 1.f);
		if (FightT >= S.T1) { S.bOn = false; }
	};
	Step(GuardSlide, GuardPos, GuardYaw);
	Step(PendSlide, PendPos, PendYaw);
	Step(VanceSlide, VancePos, VanceYaw);
}

void AScene7Fight::PlaceAll()
{
	// The mannequin faces +Y in its own space: turn it -90 to face Yaw.
	Guard->SetRelativeLocationAndRotation(GuardPos, FRotator(0.f, GuardYaw - 90.f, 0.f));
	Pendleton->SetRelativeLocationAndRotation(PendPos, FRotator(0.f, PendYaw - 90.f, 0.f));
	if (ATakeCharacter* V = GetVance())
	{
		const float Half = V->GetCapsuleComponent() ? V->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
		const FRotator R(0.f, WYaw(VanceYaw), 0.f);
		V->SetActorLocationAndRotation(W(VancePos) + FVector(0.f, 0.f, Half + 2.f), R, false, nullptr, ETeleportType::TeleportPhysics);
		if (AController* C = V->GetController()) { C->SetControlRotation(R); }
	}
}

FVector AScene7Fight::BoneOr(USkeletalMeshComponent* Mesh, FName Bone, const FVector& Fallback) const
{
	if (Mesh && !Bone.IsNone() && Mesh->GetBoneIndex(Bone) != INDEX_NONE) { return Mesh->GetSocketLocation(Bone); }
	return Mesh ? Mesh->GetComponentLocation() + FVector(0.f, 0.f, 100.f) : Fallback;
}

FVector AScene7Fight::SubjectPoint(EFightSubject Subject, FName Bone) const
{
	ATakeCharacter* V = GetVance();
	USkeletalMeshComponent* VM = V ? V->GetMesh() : nullptr;
	const FVector Home = W(FVector(0.f, 0.f, 110.f));
	switch (Subject)
	{
	case EFightSubject::Vance: return BoneOr(VM, Bone, Home);
	case EFightSubject::Guard: return BoneOr(Guard, Bone, Home);
	case EFightSubject::Pendleton: return BoneOr(Pendleton, Bone, Home);
	case EFightSubject::VanceGuard: return (BoneOr(VM, Bone, Home) + BoneOr(Guard, Bone, Home)) * 0.5f;
	case EFightSubject::VancePendleton: return (BoneOr(VM, Bone, Home) + BoneOr(Pendleton, Bone, Home)) * 0.5f;
	case EFightSubject::Canister:
		if (AScene7Booth* B = FindFightBooth(GetWorld())) { if (B->GetCanister()) { return B->GetCanister()->GetComponentLocation(); } }
		return BoneOr(VM, TEXT("hand_r"), Home);
	case EFightSubject::Stiletto: return Stiletto->GetComponentLocation();
	}
	return Home;
}

void AScene7Fight::Shot(FName Name)
{
	APlayerController* PC = GetPC();
	if (!PC) { return; }
	if (Name.IsNone())
	{
		CurShot = NAME_None;
		if (ATakeCharacter* V = GetVance()) { PC->SetViewTarget(V); }
		return;
	}
	const FFightShot* S = Shots.Find(Name);
	if (!S) { UE_LOG(LogScene7Fight, Warning, TEXT("S7FIGHT no shot %s"), *Name.ToString()); return; }
	// Every change is a hard cut: the other camera is set up, then the view jumps to it this frame.
	bOnA = !bOnA;
	CurShot = Name;
	ShotT = 0.f;
	ShotAim = SubjectPoint(S->Subject, S->Bone) + GetActorTransform().TransformVector(S->LookOffset);
	StepCamera(0.f, 0.f);
	if (ACameraActor* Cam = bOnA ? CamA : CamB) { PC->SetViewTarget(Cam); }
}

void AScene7Fight::StepCamera(float Dt, float RealDt)
{
	Kick *= FMath::Exp(-KickDecay * RealDt);
	ACameraActor* Cam = bOnA ? CamA : CamB;
	const FFightShot* S = CurShot.IsNone() ? nullptr : Shots.Find(CurShot);
	if (!Cam || !S) { return; }
	ShotT += Dt;

	const FVector Loc = W(S->From + S->Drift * Ease(ShotT / 2.f));
	const FVector Live = SubjectPoint(S->Subject, S->Bone) + GetActorTransform().TransformVector(S->LookOffset);
	const FVector Aim = FMath::Lerp(ShotAim, Live, FMath::Clamp(S->Track, 0.f, 1.f));
	FRotator R = (Aim - Loc).Rotation();
	const float Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.f;
	R.Pitch += S->Handheld * FMath::PerlinNoise1D(Now * 1.3f) + Kick * FMath::PerlinNoise1D(Now * 31.f);
	R.Yaw += S->Handheld * FMath::PerlinNoise1D(Now * 1.1f + 7.f) + Kick * FMath::PerlinNoise1D(Now * 27.f + 3.f);
	R.Roll = S->Handheld * 0.5f * FMath::PerlinNoise1D(Now * 0.7f + 11.f);
	float Fov = S->Fov;
	if (S->EndFov > 0.f) { Fov = FMath::Lerp(S->Fov, S->EndFov, Ease(ShotT / FMath::Max(S->ZoomSeconds, 0.01f))); }
	Fov = FMath::Max(8.f, Fov - Kick * 0.9f);
	Cam->SetActorLocationAndRotation(Loc, R);
	Cam->GetCameraComponent()->SetFieldOfView(Fov);
}

void AScene7Fight::Sfx(FName Name, float Volume)
{
	if (const TObjectPtr<USoundBase>* S = Sounds.Find(Name))
	{
		if (*S) { UGameplayStatics::PlaySound2D(this, *S, Volume); }
	}
}

void AScene7Fight::SetSlow(FName Reason, bool bOn, float Scale, float EaseSeconds)
{
	ATakeDirector* D = ATakeDirector::Get(this);
	if (!D) { return; }
	if (bOn) { D->BeginSlow(Reason, Scale, EaseSeconds); }
	else { D->EndSlow(Reason, EaseSeconds); }
}

void AScene7Fight::Impact(const FVector& World, FName Sound, float KickAmount, bool bSparks)
{
	Sfx(Sound);
	Kick = FMath::Max(Kick, KickAmount);
	if (bSparks && SparksFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, SparksFX, World, FRotator::ZeroRotator, FVector(1.f), true);
	}
	// Hit-stop: the frame of contact holds for a beat.
	SetSlow(FightSlowStop, true, 0.04f, 0.f);
	HitStopLeft = HitStopReal;
}

void AScene7Fight::CanisterToHand(bool bInHand)
{
	ATakeCharacter* V = GetVance();
	AScene7Booth* B = FindFightBooth(GetWorld());
	if (!V || !B || !B->GetCanister()) { return; }
	if (!bInHand) { AScene7Booth::AttachCanisterTo(this, V); return; }
	UStaticMeshComponent* C = B->GetCanister();
	C->SetHiddenInGame(false);
	C->AttachToComponent(V->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, B->CanisterBlockBone);
	C->SetRelativeTransform(CanisterBlock.Equals(FTransform::Identity) ? B->CanisterInHand : CanisterBlock);
}

void AScene7Fight::ReleaseVance(bool bToWalk)
{
	ATakeCharacter* V = GetVance();
	if (!V) { return; }
	if (USkeletalMeshComponent* M = V->GetMesh())
	{
		M->SetPlayRate(1.f);
		if (M->GetAnimationMode() != EAnimationMode::AnimationBlueprint) { M->SetAnimationMode(EAnimationMode::AnimationBlueprint); }
		if (bHaveVanceMesh) { M->SetRelativeLocation(VanceMeshRel); }
	}
	if (bToWalk) { FreezeFightVance(V, false); }
}

// ---- the fight ------------------------------------------------------------------------------------------------------

void AScene7Fight::BeginFight()
{
	ATakeCharacter* V = GetVance();
	if (!V) { UE_LOG(LogScene7Fight, Warning, TEXT("S7FIGHT no Vance in %s"), *GetNameSafe(GetWorld())); return; }
	for (FName R : { FightSlowPrompt, FightSlowStop, FightSlowMo, FightSlowCut }) { SetSlow(R, false, 1.f, 0.f); }

	// Hide whatever this fight's cast stands in for.
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (!HideTag.IsNone() && It->ActorHasTag(HideTag) && !It->IsHidden())
		{
			It->SetActorHiddenInGame(true);
			Hidden.AddUnique(*It);
		}
	}
	if (!bHaveVanceMesh && V->GetMesh()) { VanceMeshRel = V->GetMesh()->GetRelativeLocation(); bHaveVanceMesh = true; }
	FreezeFightVance(V, true);

	Take = 1;
	for (bool& B : bFirstGo) { B = true; }
	FightT = 0.f;
	HitStopLeft = Kick = FlashRed = Stamp = 0.f;
	Guard->SetHiddenInGame(false);
	Pendleton->SetHiddenInGame(false);
	BeginBeat(-1);
}

void AScene7Fight::ClearFight(bool bHideCast)
{
	for (FName R : { FightSlowPrompt, FightSlowStop, FightSlowMo, FightSlowCut }) { SetSlow(R, false, 1.f, 0.f); }
	if (Stage != EFightStage::Idle) { ReleaseVance(true); }
	Stage = EFightStage::Idle;
	CurShot = NAME_None;
	bBladeFlying = false;
	if (!bHideCast) { return; }
	Guard->SetHiddenInGame(true);
	Pendleton->SetHiddenInGame(true);
	Stiletto->SetHiddenInGame(true);
	for (AActor* A : Hidden) { if (A) { A->SetActorHiddenInGame(false); } }
	Hidden.Reset();
}

void AScene7Fight::SetStage(EFightStage NewStage)
{
	Stage = NewStage;
	StageT = PrevT = StageReal = 0.f;
	UE_LOG(LogScene7Fight, Log, TEXT("S7FIGHT stage %d beat %d take %d"), (int32)NewStage, Beat, Take);
}

void AScene7Fight::BeginBeat(int32 Index)
{
	Beat = Index;
	GuardSlide.bOn = PendSlide.bOn = VanceSlide.bOn = false;
	bBladeFlying = false;
	BladeT = -1.f;
	Stiletto->AttachToComponent(Pendleton, FAttachmentTransformRules::KeepRelativeTransform, StilettoBone);
	Stiletto->SetRelativeTransform(StilettoInHand);
	Stiletto->SetHiddenInGame(true);
	CanisterToHand(false);

	// Where everyone stands at the top of this beat (a retake puts them back here).
	VancePos = FVector::ZeroVector;
	const bool bGuardUp = Index <= 0;
	GuardPos = Index < 0 ? GuardStart : bGuardUp ? GuardSpot : GuardSlamSpot;
	GuardYaw = bGuardUp ? YawTo(GuardPos, VancePos) : GuardSlamYaw;
	if (bGuardUp) { Act(Guard, TEXT("Stance"), true); }
	else { ActEnd(Guard, TEXT("FallBack")); }
	PendPos = Index <= 1 ? PendletonStart : PendletonAttack + FVector(30.f, 0.f, 0.f);
	PendYaw = YawTo(PendPos, VancePos);
	Act(Pendleton, Index <= 1 ? TEXT("Stand") : TEXT("Stance"), true);
	VanceYaw = Index <= 0 ? 0.f : YawTo(VancePos, PendletonAttack);
	ActVance(TEXT("Stance"), true);
	PlaceAll();

	if (Index < 0)
	{
		SetStage(EFightStage::Intro);
		Shot(TEXT("Wide"));
		return;
	}
	SetStage(EFightStage::Windup);
	switch (Index)
	{
	case 0:
		Shot(TEXT("OTSGuard"));
		Act(Guard, TEXT("Hook"), false, 1.f);
		Slide(GuardSlide, GuardPos, GuardSpot - FVector(18.f, 0.f, 0.f), GuardYaw, 180.f, 0.f, WindupSeconds.IsValidIndex(0) ? WindupSeconds[0] : 0.55f);
		break;
	case 1:
		Shot(TEXT("BladeInsert"));
		Act(Pendleton, TEXT("Walk"), true, 0.8f);
		Slide(PendSlide, PendPos, PendletonAttack, YawTo(PendPos, PendletonAttack), YawTo(PendPos, PendletonAttack), 0.f, 0.6f);
		break;
	default:
		Shot(TEXT("LungeWide"));
		Act(Pendleton, TEXT("Lunge"), false, 1.1f);
		Slide(PendSlide, PendPos, PendletonAttack - FVector(15.f, 0.f, 0.f), PendYaw, PendYaw, 0.f, WindupSeconds.IsValidIndex(2) ? WindupSeconds[2] : 0.45f);
		break;
	}
}

void AScene7Fight::BeginPrompt()
{
	SetStage(EFightStage::Prompt);
	SetSlow(FightSlowPrompt, true, PromptSlowScale, 0.06f);
}

void AScene7Fight::Answer(bool bRight)
{
	SetSlow(FightSlowPrompt, false, 1.f, 0.f);
	bFirstGo[FMath::Clamp(Beat, 0, 2)] = false;
	Stamp = 1.f;
	StampWords = bRight ? (Beat == 0 ? TEXT("PARRIED") : Beat == 1 ? TEXT("CLANG!") : TEXT("DOWN HE GOES")) : TEXT("CUT!");
	if (bRight) { StartStrike(); } else { StartMiss(); }
}

void AScene7Fight::StartStrike()
{
	SetStage(EFightStage::Strike);
	switch (Beat)
	{
	case 0:
		// He slips under the hook: the fist sails over his head and the guard's weight carries him on past.
		Shot(TEXT("DuckLow"));
		ActVance(TEXT("Duck"), false, 1.3f);
		Slide(GuardSlide, GuardPos, GuardStumble, GuardYaw, 180.f, 0.08f, 0.5f);
		Slide(VanceSlide, VancePos, FVector(0.f, -28.f, 0.f), VanceYaw, VanceYaw, 0.f, 0.2f);
		break;
	case 1:
		// The canister comes up as the blade comes down.
		Shot(TEXT("BlockCU"));
		CanisterToHand(true);
		ActVance(TEXT("Block"), false, 1.2f);
		break;
	default:
		// The sweep, in slow motion.
		SetSlow(FightSlowMo, true, SweepSlowScale, 0.05f);
		Shot(TEXT("SweepLow"));
		ActVance(TEXT("Sweep"), false, 1.15f);
		break;
	}
}

void AScene7Fight::StartMiss()
{
	SetStage(EFightStage::Miss);
	static const FName MissShots[] = { TEXT("MissGuard"), TEXT("MissBlade"), TEXT("MissLunge") };
	Shot(MissShots[FMath::Clamp(Beat, 0, 2)]);
	if (Beat == 1) { Act(Pendleton, TEXT("Slash"), false, 1.4f); }
}

void AScene7Fight::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsRunning()) { return; }
	const float RealDt = FApp::GetDeltaTime();
	FightT += DeltaSeconds;
	PrevT = StageT;
	StageT += DeltaSeconds;
	StageReal += RealDt;

	if (HitStopLeft > 0.f)
	{
		HitStopLeft -= RealDt;
		if (HitStopLeft <= 0.f) { SetSlow(FightSlowStop, false, 1.f, 0.04f); }
	}
	FlashRed = FMath::Max(0.f, FlashRed - RealDt * 1.6f);
	Stamp = FMath::Max(0.f, Stamp - RealDt * 1.2f);

	// The switchblade: flicks open, then (after the block) spins away.
	if (BladeT >= 0.f && !bBladeFlying)
	{
		BladeT += DeltaSeconds;
		FVector Sc = StilettoInHand.GetScale3D();
		Sc.X *= FMath::Max(0.05f, FMath::Clamp(BladeT / 0.09f, 0.f, 1.f));
		Stiletto->SetRelativeScale3D(Sc);
	}
	if (bBladeFlying)
	{
		const FVector P = Stiletto->GetComponentLocation() + BladeVel * DeltaSeconds;
		BladeVel.Z -= 980.f * DeltaSeconds;
		const float Floor = GetActorLocation().Z + 2.f;
		if (P.Z <= Floor) { bBladeFlying = false; Stiletto->SetWorldLocation(FVector(P.X, P.Y, Floor)); }
		else
		{
			Stiletto->SetWorldLocation(P);
			Stiletto->AddWorldRotation(FRotator(0.f, 900.f * DeltaSeconds, 1400.f * DeltaSeconds));
		}
	}

	TickStage(DeltaSeconds);
	if (Stage == EFightStage::Idle) { return; }
	StepSlides();
	if (IsRunning()) { PlaceAll(); }
	StepCamera(DeltaSeconds, RealDt);
}

void AScene7Fight::TickStage(float Dt)
{
	ATakeCharacter* V = GetVance();
	USkeletalMeshComponent* VM = V ? V->GetMesh() : nullptr;
	const int32 B = FMath::Clamp(Beat, 0, 2);

	switch (Stage)
	{
	case EFightStage::Intro:
		if (At(0.2f))
		{
			// The bodyguard squares up and steps in.
			Act(Guard, TEXT("Walk"), true, 0.6f);
			Slide(GuardSlide, GuardPos, GuardSpot, GuardYaw, YawTo(GuardSpot, VancePos), 0.f, 0.9f);
		}
		if (At(0.35f) && OwnerScene && !PendletonLine.IsEmpty()) { OwnerScene->Say(TEXT("PENDLETON"), PendletonLine); }
		if (At(1.1f)) { Act(Guard, TEXT("Stance"), true); }
		if (At(1.2f)) { Shot(TEXT("VanceCU")); }
		if (StageT >= IntroSeconds) { BeginBeat(0); }
		break;

	case EFightStage::Windup:
		if (Beat == 1)
		{
			if (At(0.2f))
			{
				// Click: the switchblade opens in his fist.
				Stiletto->SetHiddenInGame(false);
				BladeT = 0.f;
				Sfx(TEXT("Clang"), 0.35f);
			}
			if (At(0.6f))
			{
				Shot(TEXT("Profile"));
				Act(Pendleton, TEXT("Slash"), false, 1.0f);
				PendSlide.bOn = false;
				PendPos = PendletonAttack;
				PendYaw = YawTo(PendPos, VancePos);
			}
		}
		if (StageT >= (WindupSeconds.IsValidIndex(B) ? WindupSeconds[B] : 0.5f)) { BeginPrompt(); }
		break;

	case EFightStage::Prompt:
	{
		// The keys come in through HandleSceneKey, on the frame they are pressed; here only the clock and the test hook.
		const float Limit = PromptRealSeconds + (bFirstGo[B] && Take == 1 ? FirstPromptBonus : 0.f);
		if (GFightAuto && StageReal >= 0.5f) { Answer(!(GFightAuto == 2 && bFirstGo[B])); }
		else if (StageReal >= Limit) { Answer(false); }
		break;
	}

	case EFightStage::Strike:
		if (Beat == 0)
		{
			if (At(0.5f))
			{
				// Vance turns on him as he stumbles past.
				Act(Guard, TEXT("Stumble"), false, 1.f);
				Slide(VanceSlide, VancePos, FVector(-30.f, 0.f, 0.f), VanceYaw, 180.f, 0.f, 0.25f);
			}
			if (At(0.78f))
			{
				// A hand on his collar, and his face goes into the monitor bank.
				Shot(TEXT("SlamCU"));
				ActVance(TEXT("Shove"), false, 1.3f);
				Slide(GuardSlide, GuardPos, GuardSlamSpot, GuardYaw, GuardSlamYaw, 0.f, 0.28f);
				Slide(VanceSlide, VancePos, GuardSlamSpot + FVector(85.f, 0.f, 0.f), VanceYaw, 180.f, 0.f, 0.28f);
			}
			if (At(1.06f))
			{
				Impact(BoneOr(Guard, TEXT("head"), W(GuardSlamSpot)) + GetActorForwardVector() * -25.f, TEXT("Slam"), 3.8f, true);
				Act(Guard, TEXT("FallBack"), false, 1.f);
				Sfx(TEXT("Grunt1"), 0.8f);
			}
			if (At(1.25f))
			{
				// He steps clear of the falling body and turns to Pendleton.
				Slide(VanceSlide, VancePos, FVector(0.f, -60.f, 0.f), VanceYaw, YawTo(FVector(0.f, -60.f, 0.f), PendletonStart), 0.1f, 0.8f);
			}
			if (At(1.7f))
			{
				Shot(TEXT("GuardDown"));
				ActVance(TEXT("Stance"), true);
				Sfx(TEXT("Fall"), 0.9f);
			}
		}
		else if (Beat == 1)
		{
			if (At(0.12f))
			{
				Impact(SubjectPoint(EFightSubject::Canister, NAME_None), TEXT("Clang"), 3.2f, true);
				HitStopLeft = HitStopReal * 1.6f;
				Act(Pendleton, TEXT("HitMed"), false, 1.f);
				Slide(PendSlide, PendPos, PendletonAttack + FVector(30.f, 0.f, 0.f), PendYaw, PendYaw, 0.05f, 0.45f);
				// The blade flies out of his hand.
				Stiletto->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
				BladeVel = GetActorTransform().TransformVector(FVector(160.f, 260.f, 420.f));
				bBladeFlying = true;
			}
			if (At(0.75f)) { Shot(TEXT("Profile")); }
			if (At(1.25f))
			{
				CanisterToHand(false);
				ActVance(TEXT("Stance"), true);
			}
		}
		else
		{
			if (At(0.34f))
			{
				Impact(BoneOr(Pendleton, TEXT("foot_l"), W(PendPos)), TEXT("Punch"), 2.0f, false);
				Act(Pendleton, TEXT("SweepFall"), false, 0.95f);
				Slide(PendSlide, PendPos, PendletonLand, PendYaw, PendYaw, 0.f, 0.6f);
				Sfx(TEXT("Grunt2"), 0.7f);
			}
			if (At(0.95f))
			{
				Impact(BoneOr(Pendleton, TEXT("pelvis"), W(PendletonLand)), TEXT("Fall"), 3.0f, false);
				Sfx(TEXT("Crash"), 0.5f);
			}
			if (At(1.1f))
			{
				SetSlow(FightSlowMo, false, 1.f, 0.25f);
				Shot(TEXT("Overhead"));
			}
		}
		if (StageT >= (StrikeSeconds.IsValidIndex(B) ? StrikeSeconds[B] : 2.f))
		{
			if (Beat < 2) { BeginBeat(Beat + 1); }
			else
			{
				SetStage(EFightStage::Outro);
				Shot(TEXT("Final"));
				ActVance(TEXT("Stand"), true);
			}
		}
		break;

	case EFightStage::Miss:
		if (At(0.1f))
		{
			// The blow lands on Vance.
			Impact(BoneOr(VM, Beat == 1 ? TEXT("spine_05") : TEXT("head"), W(VancePos)), Beat == 1 ? TEXT("Clang") : TEXT("Punch"), 4.f, Beat == 1);
			ActVance(Beat == 1 ? TEXT("HitMed") : TEXT("HitHeavy"), false, 1.f);
			const FVector Back = VancePos - FRotator(0.f, VanceYaw, 0.f).Vector() * MissKnockBack;
			Slide(VanceSlide, VancePos, Back, VanceYaw, VanceYaw, 0.f, 0.35f);
			Sfx(TEXT("Grunt1"));
			FlashRed = 1.f;
		}
		if (StageT >= MissSeconds)
		{
			SetStage(EFightStage::Cut);
			SetSlow(FightSlowCut, true, 0.02f, 0.08f);
		}
		break;

	case EFightStage::Cut:
		if (StageReal >= CutRealSeconds)
		{
			SetSlow(FightSlowCut, false, 1.f, 0.f);
			++Take;
			BeginBeat(Beat);
		}
		break;

	case EFightStage::Outro:
		if (StageT >= OutroSeconds)
		{
			SetStage(EFightStage::Done);
			ReleaseVance(true);
			UE_LOG(LogScene7Fight, Log, TEXT("S7FIGHT done after %d take(s)"), Take);
		}
		break;

	default:
		break;
	}
}

// ---- HUD ------------------------------------------------------------------------------------------------------------

void AScene7Fight::DrawHud(AHUD* Hud, UCanvas* Canvas)
{
	if (!Canvas || !Hud || Hud->GetWorld() != GetWorld() || !IsRunning()) { return; }
	FTakeHudKit Kit;
	Kit.Begin(Canvas, FPlatformTime::Seconds(), 0.f);
	const float CX = Kit.CX();
	const int32 B = FMath::Clamp(Beat, 0, 2);

	if (FlashRed > 0.f) { Kit.Rect(0.f, 0.f, Kit.W, Kit.H, TakeStyle::Danger * FLinearColor(1, 1, 1, 0.35f * FlashRed)); }

	if (Stage == EFightStage::Intro && StageT > 1.2f)
	{
		Kit.Text(TakeStyle::EType::Goal, TEXT("Press the key on screen the moment it shows."), CX, Kit.H * 0.80f, TakeStyle::Bone, ETakeAlign::Centre);
	}

	if (Stage == EFightStage::Prompt && PromptKeys.IsValidIndex(B))
	{
		const float Limit = PromptRealSeconds + (bFirstGo[B] && Take == 1 ? FirstPromptBonus : 0.f);
		const float Left = FMath::Clamp(1.f - StageReal / Limit, 0.f, 1.f);
		const float Pop = FMath::Clamp(StageReal / 0.12f, 0.f, 1.f);
		const float Y = Kit.H * 0.66f;
		const FString KeyText = PromptKeys[B].GetDisplayName().ToString();
		const FString Words = PromptWords.IsValidIndex(B) ? PromptWords[B] : FString();
		Kit.Rect(0.f, Y - Kit.Px(80.f), Kit.W, Kit.Px(170.f), TakeStyle::Ink * FLinearColor(1, 1, 1, 0.55f * Pop));
		Kit.Push(FVector2D(CX, Y), 0.f, 1.7f + 0.7f * (1.f - Pop));
		const float KW = Kit.KeyWidth(KeyText, false);
		Kit.Key(KeyText, CX - KW * 0.5f, Y, FTakeInk::Of(TakeStyle::Paper, TakeStyle::Julian));
		Kit.Pop();
		Kit.Text(TakeStyle::EType::Title, Words, CX, Y + Kit.Px(58.f), TakeStyle::Paper, ETakeAlign::Centre);
		const float BW = Kit.Px(420.f), BH = Kit.Px(6.f);
		Kit.Rect(CX - BW * 0.5f, Y + Kit.Px(84.f), BW * Left, BH, Left < 0.35f ? TakeStyle::Danger : TakeStyle::Julian);
		Kit.Text(TakeStyle::EType::Label, FString::Printf(TEXT("%d / 3"), B + 1), CX, Y - Kit.Px(58.f), TakeStyle::Ash, ETakeAlign::Centre);
	}

	// The press answered: the move's name stamps in on the same frame.
	if (Stage == EFightStage::Strike && Stamp > 0.f)
	{
		const float Y = Kit.H * 0.66f;
		Kit.Push(FVector2D(CX, Y), TakeStyle::StampTilt, 1.f + 0.25f * Stamp);
		Kit.Text(TakeStyle::EType::Stamp, StampWords, CX, Y, TakeStyle::Julian * FLinearColor(1, 1, 1, FMath::Min(1.f, Stamp * 1.5f)), ETakeAlign::Centre);
		Kit.Pop();
	}

	if (Stage == EFightStage::Miss || Stage == EFightStage::Cut)
	{
		const float A = Stage == EFightStage::Cut ? 1.f : FMath::Clamp(StageT / 0.3f, 0.f, 1.f);
		const float Y = Kit.H * 0.42f;
		Kit.Rect(0.f, Y - Kit.Px(90.f), Kit.W, Kit.Px(260.f), TakeStyle::Ink * FLinearColor(1, 1, 1, 0.7f * A));
		Kit.Push(FVector2D(CX, Y), TakeStyle::StampTilt, 1.f);
		Kit.Text(TakeStyle::EType::Stamp, TEXT("CUT!"), CX, Y, TakeStyle::Danger * FLinearColor(1, 1, 1, A), ETakeAlign::Centre);
		Kit.Pop();
		const FString Why = MissWords.IsValidIndex(B) ? MissWords[B] : FString();
		Kit.Text(TakeStyle::EType::Line, Why, CX, Y + Kit.Px(70.f), TakeStyle::Paper * FLinearColor(1, 1, 1, A), ETakeAlign::Centre);
		if (Stage == EFightStage::Cut && PromptKeys.IsValidIndex(B))
		{
			const FString KeyText = PromptKeys[B].GetDisplayName().ToString();
			const FString Hint = FString::Printf(TEXT("Take %d. This time: %s"), Take + 1, PromptWords.IsValidIndex(B) ? *PromptWords[B].ToLower() : TEXT(""));
			const float TW = Kit.Measure(TakeStyle::EType::Goal, Hint);
			const float KW = Kit.KeyWidth(KeyText, false);
			const float X0 = CX - (TW + KW + Kit.Px(14.f)) * 0.5f;
			const float HY = Y + Kit.Px(124.f);
			Kit.Text(TakeStyle::EType::Goal, Hint, X0, HY, TakeStyle::Bone);
			Kit.Key(KeyText, X0 + TW + Kit.Px(14.f), HY, FTakeInk::Of(TakeStyle::Paper, TakeStyle::Julian));
		}
	}
}
