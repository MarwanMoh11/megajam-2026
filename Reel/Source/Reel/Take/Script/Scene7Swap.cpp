#include "Script/Scene7Swap.h"
#include "Script/Scene7SwapMesh.h"
#include "Script/Scene7SwapAnim.h"
#include "Script/ScriptScene.h"
#include "TakeCharacter.h"
#include "TakeHudKit.h"
#include "TakeStyle.h"
#include "ProceduralMeshComponent.h"
#include "CineCameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Animation/AnimSequence.h"
#include "Sound/SoundBase.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Engine.h"
#include "Misc/App.h"


namespace Scene7SwapLocal
{
	using namespace Scene7SwapMesh;
	const TCHAR* SurfacePath = TEXT("/Game/Take/Props/Swap/M_Swap_Surface.M_Swap_Surface");
	const TCHAR* BeamPath = TEXT("/Game/Take/Props/Swap/M_Swap_Beam.M_Swap_Beam");
	const TCHAR* FallbackPath = TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");

	bool GSwapAuto = false;	// Reel.Swap.Auto: presses itself when a beat ends (PIE captures)

	float Seg(float T, float A, float B) { return FMath::Clamp((T - A) / FMath::Max(B - A, 1e-3f), 0.f, 1.f); }
	float Ease(float A) { return FMath::InterpEaseInOut(0.f, 1.f, FMath::Clamp(A, 0.f, 1.f), 2.f); }
	float ES(float T, float A, float B) { return Ease(Seg(T, A, B)); }

	/** A hand's goal in the actor's space. */
	struct FHand
	{
		float Alpha = 0.f;
		FVector Palm = FVector::ZeroVector;
		FVector Fingers = FVector::UpVector;
		FVector Facing = FVector::ForwardVector;
		float Grip = 0.f;
	};

	FHand LerpHand(const FHand& A, const FHand& B, float W)
	{
		FHand H;
		H.Alpha = FMath::Lerp(A.Alpha, B.Alpha, W);
		// From "no goal" the hand starts at the goal it is heading to (the IK alpha does the travel).
		const FHand& AA = A.Alpha <= 0.f ? B : A;
		const FHand& BB = B.Alpha <= 0.f ? A : B;
		H.Palm = FMath::Lerp(AA.Palm, BB.Palm, W);
		H.Fingers = FMath::Lerp(AA.Fingers, BB.Fingers, W).GetSafeNormal();
		H.Facing = FMath::Lerp(AA.Facing, BB.Facing, W).GetSafeNormal();
		if (H.Fingers.IsNearlyZero()) { H.Fingers = BB.Fingers; }
		if (H.Facing.IsNearlyZero()) { H.Facing = BB.Facing; }
		H.Grip = FMath::Lerp(A.Grip, B.Grip, W);
		return H;
	}

	FHand On(const FTransform& T, const FVector& P, const FVector& Fingers, const FVector& Facing, float Grip)
	{
		FHand H;
		H.Alpha = 1.f;
		H.Palm = T.TransformPosition(P);
		H.Fingers = T.TransformVectorNoScale(Fingers).GetSafeNormal();
		H.Facing = T.TransformVectorNoScale(Facing).GetSafeNormal();
		H.Grip = Grip;
		return H;
	}

	FHand Off() { return FHand(); }

	/** A prop's move from A to B with a lift in the middle. */
	FTransform Carry(const FTransform& A, const FTransform& B, float W, float Arc)
	{
		FTransform T;
		T.SetLocation(FMath::Lerp(A.GetLocation(), B.GetLocation(), W) + FVector(0, 0, Arc * FMath::Sin(PI * W)));
		T.SetRotation(FQuat::Slerp(A.GetRotation(), B.GetRotation(), W));
		return T;
	}

	/** A reel lying flat, in the bench's (or can's) space: the axle up, its +X toward the worker's left. */
	const FQuat Flat0 = FQuat(FVector::UpVector, PI) * FQuat(FVector::ForwardVector, -PI * 0.5f);
	/** A reel held upright in front of him (body space): the axle along his forward, its +X to his left. */
	const FQuat Upright0 = FQuat(FVector::UpVector, -PI * 0.5f);
}

// ---- console (PIE checks) ------------------------------------------------------------------------------------------

static FAutoConsoleCommandWithWorldAndArgs GSwapScrubCmd(TEXT("Reel.Swap.Scrub"),
	TEXT("Reel.Swap.Scrub <beat 1-4> <seconds>: hold the reel swap at that moment (double, props, camera)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		using namespace Scene7SwapLocal;
		if (AScene7Swap* S = AScene7Swap::Find(World))
		{
			S->Scrub(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 1, Args.Num() > 1 ? FCString::Atof(*Args[1]) : 0.f);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GSwapPlayCmd(TEXT("Reel.Swap.Play"),
	TEXT("Reel.Swap.Play [auto 0/1]: begin the reel swap with the player's character (auto presses by itself)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		using namespace Scene7SwapLocal;
		GSwapAuto = Args.Num() > 0 && FCString::Atoi(*Args[0]) != 0;
		AScene7Swap* S = AScene7Swap::Find(World);
		ATakeCharacter* V = Cast<ATakeCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
		if (S && V) { S->ResetSwap(); S->BeginSwap(V, true); }
	}));

static FAutoConsoleCommandWithWorldAndArgs GSwapResetCmd(TEXT("Reel.Swap.Reset"),
	TEXT("Reel.Swap.Reset: the reel swap back to its start."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		using namespace Scene7SwapLocal;
		if (AScene7Swap* S = AScene7Swap::Find(World)) { S->ResetSwap(); }
	}));

// ---- construction --------------------------------------------------------------------------------------------------

AScene7Swap::AScene7Swap()
{
	using namespace Scene7SwapLocal;	// unity builds: no file-scope usings (they leak into the next .cpp)
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	auto PM = [this](const TCHAR* Name, USceneComponent* Parent)
	{
		UProceduralMeshComponent* M = CreateDefaultSubobject<UProceduralMeshComponent>(Name);
		M->SetupAttachment(Parent);
		M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		M->SetCanEverAffectNavigation(false);
		return M;
	};
	Projector = PM(TEXT("Projector"), Root);
	Projector->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Projector->SetCollisionProfileName(TEXT("BlockAll"));
	Projector->bUseComplexAsSimpleCollision = false;
	DoorHinge = CreateDefaultSubobject<USceneComponent>(TEXT("DoorHinge"));
	DoorHinge->SetupAttachment(Root);
	DoorHinge->SetRelativeLocation(Scene7SwapMesh::DoorHinge);
	Door = PM(TEXT("Door"), DoorHinge);
	DoorLatch = PM(TEXT("DoorLatch"), Door);
	DoorLatch->SetRelativeLocation(DoorLatchLocal);
	MasterReel = PM(TEXT("MasterReel"), Root);
	DummyReel = PM(TEXT("DummyReel"), Root);
	FilmStrand = PM(TEXT("FilmStrand"), Root);
	Bench = PM(TEXT("Bench"), Root);
	Bench->SetRelativeLocation(FVector(-20.f, -125.f, 0.f));
	Bench->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Bench->SetCollisionProfileName(TEXT("BlockAll"));
	Bench->bUseComplexAsSimpleCollision = false;
	CanRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CanRoot"));
	CanRoot->SetupAttachment(Root);
	Can = PM(TEXT("Can"), CanRoot);
	CanLatch = PM(TEXT("CanLatch"), CanRoot);
	CanLatch->SetRelativeLocation(FVector(CanRadius + 0.6f, 0.f, 1.6f));
	CanLid = PM(TEXT("CanLid"), Root);
	Beam = PM(TEXT("Beam"), Root);
	Beam->SetRelativeLocation(LensFront);
	Beam->SetCastShadow(false);

	LensLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("LensLight"));
	LensLight->SetupAttachment(Root);
	LensLight->SetRelativeLocation(LensFront + FVector(1.f, 0.f, 0.f));
	LensLight->SetIntensityUnits(ELightUnits::Candelas);
	LensLight->SetIntensity(900.f);
	LensLight->SetLightColor(FLinearColor(0.85f, 0.9f, 1.f));
	LensLight->SetInnerConeAngle(2.f);
	LensLight->SetOuterConeAngle(6.f);
	LensLight->SetAttenuationRadius(3000.f);
	LensLight->SetCastShadows(false);

	MagLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MagLight"));
	MagLight->SetupAttachment(Root);
	MagLight->SetRelativeLocation(FVector(-30.f, -70.f, 200.f));
	MagLight->SetIntensityUnits(ELightUnits::Candelas);
	MagLight->SetIntensity(6.f);
	MagLight->SetLightColor(FLinearColor(1.f, 0.78f, 0.55f));
	MagLight->SetAttenuationRadius(260.f);
	MagLight->SetSourceRadius(8.f);

	BenchLamp = CreateDefaultSubobject<UPointLightComponent>(TEXT("BenchLamp"));
	BenchLamp->SetupAttachment(Bench);
	BenchLamp->SetRelativeLocation(FVector(10.f, -10.f, BenchTop + 75.f));
	BenchLamp->SetIntensityUnits(ELightUnits::Candelas);
	BenchLamp->SetIntensity(10.f);
	BenchLamp->SetLightColor(FLinearColor(1.f, 0.72f, 0.45f));
	BenchLamp->SetAttenuationRadius(300.f);
	BenchLamp->SetSourceRadius(4.f);

	Double = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Double"));
	Double->SetupAttachment(Root);
	Double->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Double->SetHiddenInGame(true);
	Double->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	Cam = CreateDefaultSubobject<UCineCameraComponent>(TEXT("Cam"));
	Cam->SetupAttachment(Root);
	Cam->SetRelativeLocation(FVector(170.f, -200.f, 110.f));
	Cam->FocusSettings.FocusMethod = ECameraFocusMethod::Manual;

	Hum = CreateDefaultSubobject<UAudioComponent>(TEXT("Hum"));
	Hum->SetupAttachment(Root);
	Hum->SetRelativeLocation(FVector(-20.f, 0.f, 130.f));
	Hum->bAutoActivate = false;
	Hum->bOverrideAttenuation = true;
	Hum->AttenuationOverrides.FalloffDistance = 1400.f;
	Hum->AttenuationOverrides.AttenuationShapeExtents = FVector(150.f);

	static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleF(TEXT("/Game/Take/Anim/Mocap/StandIdle_B.StandIdle_B"));
	if (IdleF.Succeeded()) { IdleClip = IdleF.Object; }
	static const TCHAR* SoundNames[] = { TEXT("sfx_s7_mag_open"), TEXT("sfx_s7_reel_swap"), TEXT("sfx_s7_canister_latch"),
		TEXT("sfx_s7_door_click"), TEXT("sfx_s7_clang"), TEXT("sfx_s7_projector_hum") };
	for (const TCHAR* N : SoundNames)
	{
		ConstructorHelpers::FObjectFinder<USoundBase> F(*FString::Printf(TEXT("/Game/Take/Audio/Script/S7/%s.%s"), N, N));
		if (F.Succeeded()) { Sounds.Add(N, F.Object); }
	}

	BeatSeconds = { 1.8f, 3.0f, 2.8f, 3.6f };
	PromptWords = { TEXT("OPEN THE MAGAZINE"), TEXT("LIFT THE MASTER PRINT"), TEXT("LOAD THE DUMMY"), TEXT("SEAL THE CAN") };

	// Paint, Iron, Chrome, Glass, Film, LabelRed, LabelPale, Wood, Interior, Beam.
	SlotColors = {
		FLinearColor(0.030f, 0.055f, 0.042f), FLinearColor(0.012f, 0.012f, 0.013f), FLinearColor(0.80f, 0.80f, 0.78f),
		FLinearColor(0.01f, 0.015f, 0.02f), FLinearColor(0.030f, 0.016f, 0.008f), FLinearColor(0.45f, 0.025f, 0.015f),
		FLinearColor(0.62f, 0.58f, 0.47f), FLinearColor(0.10f, 0.05f, 0.022f), FLinearColor(0.20f, 0.20f, 0.18f),
		FLinearColor(1.0f, 0.93f, 0.80f) };
	SlotFinish = {
		FVector2D(0.15f, 0.38f), FVector2D(0.6f, 0.55f), FVector2D(1.f, 0.18f), FVector2D(0.4f, 0.04f), FVector2D(0.f, 0.28f),
		FVector2D(0.f, 0.6f), FVector2D(0.f, 0.7f), FVector2D(0.f, 0.55f), FVector2D(0.3f, 0.5f), FVector2D(0.f, 1.f) };

	// Coverage (see the header for the spaces).
	auto Add = [this](const TCHAR* Name, FVector From, FVector To, FVector Aim, float Focal, float Ap, float Move, bool bBody = false, bool bBench = false, FName Bone = NAME_None, float FocalTo = 0.f)
	{
		FScene7SwapShot S;
		S.From = From; S.To = To; S.Aim = Aim; S.Focal = Focal; S.Aperture = Ap; S.Move = Move;
		S.bBody = bBody; S.bBench = bBench; S.AimBone = Bone; S.FocalTo = FocalTo;
		Shots.Add(Name, S);
	};
	Add(TEXT("Wide"), FVector(-250.f, -300.f, 150.f), FVector(-235.f, -284.f, 150.f), FVector(-10.f, -45.f, 140.f), 21.f, 4.f, 3.f);
	Add(TEXT("Door"), FVector(-2.f, -88.f, 158.f), FVector(-4.f, -82.f, 160.f), FVector(-6.f, -12.f, 180.f), 32.f, 2.f, 2.f);
	Add(TEXT("Reel"), FVector(-62.f, -84.f, 170.f), FVector(-56.f, -76.f, 172.f), FVector(2.f, -4.f, 172.f), 35.f, 2.2f, 1.6f);
	Add(TEXT("Turn"), FVector(70.f, -125.f, 135.f), FVector(60.f, -130.f, 135.f), FVector(0.f, 0.f, 8.f), 26.f, 2.8f, 2.f, false, false, TEXT("spine_05"));
	Add(TEXT("Bench"), FVector(-6.f, -58.f, BenchTop + 82.f), FVector(-6.f, -54.f, BenchTop + 78.f), FVector(0.f, 8.f, BenchTop + 6.f), 26.f, 2.8f, 2.f, false, true);
	Add(TEXT("Spindle"), FVector(-50.f, -28.f, 152.f), FVector(-46.f, -30.f, 154.f), FVector(2.f, -6.f, 178.f), 30.f, 2.f, 1.6f);
	Add(TEXT("Can"), FVector(-24.f, -34.f, BenchTop + 62.f), FVector(-24.f, -30.f, BenchTop + 54.f), FVector(-24.f, 4.f, BenchTop + 2.f), 30.f, 2.8f, 1.4f, false, true);
	Add(TEXT("Face"), FVector(125.f, -55.f, 158.f), FVector(62.f, -26.f, 160.f), FVector(0.f, 0.f, 3.f), 45.f, 1.8f, 2.1f, true, false, TEXT("head"), 50.f);
}

FTransform AScene7Swap::BenchT() const { return Bench ? Bench->GetRelativeTransform() : FTransform::Identity; }
FTransform AScene7Swap::BodyT(const FVector& Spot, float Yaw) const { return FTransform(FRotator(0.f, Yaw, 0.f), FVector(Spot.X, Spot.Y, 0.f)); }
FVector AScene7Swap::BenchSpotActor() const { FVector P = BenchT().TransformPosition(BenchSpot); P.Z = 0.f; return P; }
float AScene7Swap::BenchYawActor() const { return BenchT().Rotator().Yaw - 90.f; }
FVector AScene7Swap::ReelOnSpindle() const { return FVector(UpperCenter.X, ReelY, UpperCenter.Z); }
FQuat AScene7Swap::FlatOnBench() const { return BenchT().GetRotation() * Flat0; }

AScene7Swap* AScene7Swap::Find(const UObject* WorldContextObject)
{
	using namespace Scene7SwapLocal;	// unity builds: no file-scope usings (they leak into the next .cpp)
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World) { return nullptr; }
	for (TActorIterator<AScene7Swap> It(World); It; ++It) { return *It; }
	return nullptr;
}

void AScene7Swap::BuildMeshes()
{
	using namespace Scene7SwapLocal;	// unity builds: no file-scope usings (they leak into the next .cpp)
	{ FBuilder B; BuildProjector(B); B.Commit(Projector); }
	// The projector's and the bench's collision: their own sections, rebuilt with collision on.
	{ FBuilder B; BuildMagazineDoor(B); B.Commit(Door); }
	{ FBuilder B; BuildDoorLatch(B); B.Commit(DoorLatch); }
	{ FBuilder B; BuildReel(B, true, 17.5f); B.Commit(MasterReel); }
	{ FBuilder B; BuildReel(B, false, 17.0f); B.Commit(DummyReel); }
	{ FBuilder B; BuildFilmStrand(B); B.Commit(FilmStrand); }
	{ FBuilder B; BuildBench(B); B.Commit(Bench); }
	{ FBuilder B; BuildCan(B); B.Commit(Can); }
	{ FBuilder B; BuildCanLid(B); B.Commit(CanLid); }
	{ FBuilder B; BuildCanLatch(B); B.Commit(CanLatch); }
	{ FBuilder B; BuildBeam(B, BeamLength); B.Commit(Beam); }
	// Simple blocking volumes for walking into them: the procedural sections are visual only.
	Projector->ClearCollisionConvexMeshes();
	Projector->AddCollisionConvexMesh({ FVector(-90, -33, 0), FVector(60, -33, 0), FVector(60, 33, 0), FVector(-90, 33, 0),
		FVector(-90, -33, 210), FVector(60, -33, 210), FVector(60, 33, 210), FVector(-90, 33, 210) });
	Bench->ClearCollisionConvexMeshes();
	Bench->AddCollisionConvexMesh({ FVector(-56, -27, 0), FVector(56, -27, 0), FVector(56, 27, 0), FVector(-56, 27, 0),
		FVector(-56, -27, BenchTop), FVector(56, -27, BenchTop), FVector(56, 27, BenchTop), FVector(-56, 27, BenchTop) });
}

void AScene7Swap::ApplyLook()
{
	using namespace Scene7SwapLocal;	// unity builds: no file-scope usings (they leak into the next .cpp)
	if (!SurfaceMaterial) { SurfaceMaterial = LoadObject<UMaterialInterface>(nullptr, SurfacePath, nullptr, LOAD_NoWarn | LOAD_Quiet); }
	if (!BeamMaterial) { BeamMaterial = LoadObject<UMaterialInterface>(nullptr, BeamPath, nullptr, LOAD_NoWarn | LOAD_Quiet); }
	UMaterialInterface* Surface = SurfaceMaterial ? SurfaceMaterial.Get() : LoadObject<UMaterialInterface>(nullptr, FallbackPath);
	Mids.Reset();
	for (int32 I = 0; I < Count; ++I)
	{
		UMaterialInterface* Parent = (I == Scene7SwapMesh::Beam && BeamMaterial) ? BeamMaterial.Get() : Surface;
		UMaterialInstanceDynamic* M = Parent ? UMaterialInstanceDynamic::Create(Parent, this) : nullptr;
		if (M)
		{
			const FLinearColor C = SlotColors.IsValidIndex(I) ? SlotColors[I] : FLinearColor::Gray;
			const FVector2D F = SlotFinish.IsValidIndex(I) ? SlotFinish[I] : FVector2D(0.f, 0.5f);
			M->SetVectorParameterValue(TEXT("Color"), C);
			M->SetScalarParameterValue(TEXT("Metallic"), F.X);
			M->SetScalarParameterValue(TEXT("Roughness"), F.Y);
			M->SetScalarParameterValue(TEXT("Glow"), BeamGlow);
		}
		Mids.Add(M);
	}
	for (UProceduralMeshComponent* P : { Projector.Get(), Door.Get(), DoorLatch.Get(), MasterReel.Get(), DummyReel.Get(), FilmStrand.Get(),
		Bench.Get(), Can.Get(), CanLid.Get(), CanLatch.Get(), Beam.Get() })
	{
		if (!P) { continue; }
		for (int32 I = 0; I < Count; ++I) { if (Mids[I]) { P->SetMaterial(I, Mids[I]); } }
	}
	if (!BeamMaterial) { Beam->SetHiddenInGame(true); Beam->SetVisibility(false); }
}

void AScene7Swap::OnConstruction(const FTransform& Transform)
{
	using namespace Scene7SwapLocal;	// unity builds: no file-scope usings (they leak into the next .cpp)
	Super::OnConstruction(Transform);
	BuildMeshes();
	ApplyLook();
	// The editor shows the rest pose.
	const int32 KeepBeat = Beat;
	Beat = 0;
	Pose(0.f);
	Beat = KeepBeat;
}

void AScene7Swap::BeginPlay()
{
	using namespace Scene7SwapLocal;	// unity builds: no file-scope usings (they leak into the next .cpp)
	Super::BeginPlay();
	BuildMeshes();
	ApplyLook();
	for (USceneComponent* C : { (USceneComponent*)MasterReel.Get(), (USceneComponent*)DummyReel.Get(), (USceneComponent*)FilmStrand.Get(),
		(USceneComponent*)CanRoot.Get(), (USceneComponent*)CanLid.Get(), (USceneComponent*)Double.Get() })
	{
		Rest.Add(C, TPair<USceneComponent*, FTransform>(C->GetAttachParent(), C->GetRelativeTransform()));
	}
	ResetSwap();
	SetProjectorRunning(true);
}

void AScene7Swap::SetProjectorRunning(bool bRun)
{
	using namespace Scene7SwapLocal;	// unity builds: no file-scope usings (they leak into the next .cpp)
	bRunning = bRun;
	if (Hum)
	{
		if (bRun && !Hum->IsPlaying())
		{
			if (TObjectPtr<USoundBase>* S = Sounds.Find(TEXT("sfx_s7_projector_hum"))) { Hum->SetSound(*S); Hum->Play(); }
		}
		else if (!bRun) { Hum->Stop(); }
	}
	if (BeamMaterial) { Beam->SetHiddenInGame(!bRun); }
	LensLight->SetVisibility(bRun);
}

void AScene7Swap::ResetSwap()
{
	using namespace Scene7SwapLocal;	// unity builds: no file-scope usings (they leak into the next .cpp)
	for (TPair<USceneComponent*, TPair<USceneComponent*, FTransform>>& R : Rest)
	{
		if (!R.Key) { continue; }
		if (R.Key->GetAttachParent() != R.Value.Key)
		{
			R.Key->AttachToComponent(R.Value.Key, FAttachmentTransformRules::KeepRelativeTransform);
		}
		R.Key->SetRelativeTransform(R.Value.Value);
		// The booth hides the swap's can (with the lid and print riding it) when it hands its own canister over.
		if (R.Key != Double) { R.Key->SetHiddenInGame(false, true); }
	}
	if (Vance && Vance->IsHidden())
	{
		Vance->SetActorHiddenInGame(false);
		if (UCharacterMovementComponent* CM = Vance->GetCharacterMovement()) { CM->SetMovementMode(MOVE_Walking); }
	}
	Beat = 0;
	BeatT = PrevT = 0.f;
	Rate = 1.f;
	bQueued = bDone = bScrub = false;
	SpinRate = 1.f;
	CurrentShot = NAME_None;
	Double->SetHiddenInGame(true);
	Pose(0.f);
}

FVector AScene7Swap::GetUseSpot() const { return GetActorTransform().TransformPosition(FVector(OpenSpot.X, OpenSpot.Y, 0.f)); }
float AScene7Swap::GetUseYaw() const { return GetActorRotation().Yaw + OpenYaw; }

// ---- the swap ------------------------------------------------------------------------------------------------------

void AScene7Swap::BeginSwap(ATakeCharacter* InVance, bool bStartNow)
{
	using namespace Scene7SwapLocal;	// unity builds: no file-scope usings (they leak into the next .cpp)
	Vance = InVance;
	bDone = false;
	// The body you can see (chars-locomotion): his own mesh, or his cast member's act body while an act plays.
	USkeletalMeshComponent* M = Vance ? Vance->GetVisibleBody() : nullptr;
	if (Vance && M)
	{
		Double->SetSkeletalMeshAsset(M->GetSkeletalMeshAsset());
		for (int32 I = 0; I < M->GetNumMaterials(); ++I) { Double->SetMaterial(I, M->GetMaterial(I)); }
		Double->SetRelativeScale3D(M->GetComponentScale() / GetActorScale3D());
		Double->SetAnimInstanceClass(UScene7SwapAnimInstance::StaticClass());
		if (UScene7SwapAnimInstance* A = Cast<UScene7SwapAnimInstance>(Double->GetAnimInstance()))
		{
			A->Base = IdleClip;
			A->PalmOffset = PalmOffset;
			A->GripDegrees = GripDegrees;
		}
		Double->SetHiddenInGame(false);
		Vance->SetActorHiddenInGame(true);
		if (UCharacterMovementComponent* CM = Vance->GetCharacterMovement())
		{
			CM->StopMovementImmediately();
			CM->DisableMovement();
		}
	}
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0)) { PC->SetViewTarget(this); }
	if (bStartNow) { StartBeat(1); }
	else { Beat = 1; BeatT = PrevT = 0.f; Rate = 0.f; Pose(0.f); UpdateCamera(0.f); }
}

void AScene7Swap::StartSwap(AScriptScene* Scene, bool bStartNow)
{
	using namespace Scene7SwapLocal;	// unity builds: no file-scope usings (they leak into the next .cpp)
	if (Scene) { BeginSwap(Scene->GetPovCharacter(), bStartNow); }
}

void AScene7Swap::StartBeat(int32 NewBeat)
{
	using namespace Scene7SwapLocal;	// unity builds: no file-scope usings (they leak into the next .cpp)
	Beat = NewBeat;
	BeatT = PrevT = 0.f;
	Rate = 1.f;
	bQueued = false;
	PressFlash = 0.f;
	CurrentShot = NAME_None;
	Pose(0.f);
	UpdateCamera(0.f);
}

bool AScene7Swap::HandleSwapKey(const FKey& Key, bool bPressed)
{
	using namespace Scene7SwapLocal;	// unity builds: no file-scope usings (they leak into the next .cpp)
	if (!IsRunning()) { return false; }
	const bool bUse = Key == EKeys::E || Key == EKeys::LeftMouseButton || Key == EKeys::Gamepad_FaceButton_Left || Key == EKeys::Gamepad_FaceButton_Bottom;
	if (!bUse) { return false; }
	if (!bPressed) { return true; }
	PressFlash = 0.f;
	const float Len = BeatSeconds.IsValidIndex(Beat - 1) ? BeatSeconds[Beat - 1] : 2.f;
	if (Rate == 0.f) { StartBeat(Beat); return true; }		// begun without a start: this press is beat 1
	if (BeatT >= Len - KINDA_SMALL_NUMBER)
	{
		if (Beat < 4) { StartBeat(Beat + 1); }
	}
	else
	{
		bQueued = true;
		Rate = HurryRate;
	}
	return true;
}

void AScene7Swap::Scrub(int32 InBeat, float T)
{
	using namespace Scene7SwapLocal;	// unity builds: no file-scope usings (they leak into the next .cpp)
	if (!Vance) { BeginSwap(Cast<ATakeCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)), false); }
	bScrub = true;
	bDone = false;
	Beat = FMath::Clamp(InBeat, 1, 4);
	PrevT = BeatT = T;
	CurrentShot = NAME_None;
	Double->SetHiddenInGame(false);
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0)) { PC->SetViewTarget(this); }
	Pose(0.f);
	UpdateCamera(0.f);
}

void AScene7Swap::Tick(float DeltaSeconds)
{
	using namespace Scene7SwapLocal;	// unity builds: no file-scope usings (they leak into the next .cpp)
	Super::Tick(DeltaSeconds);
	PressFlash += FApp::GetDeltaTime();
	SpinAngle = FMath::Fmod(SpinAngle + (bRunning ? ReelDegPerSec * SpinRate * DeltaSeconds : 0.f), 360.f);

	if (bScrub || Beat == 0 || bDone)
	{
		// Before the swap the master turns on its spindle; held by a scrub, everything stays.
		if (!bDone) { Pose(0.f); }
		if (bScrub) { UpdateCamera(DeltaSeconds); }
		return;
	}

	PrevT = BeatT;
	BeatT += DeltaSeconds * Rate;
	const float Len = BeatSeconds.IsValidIndex(Beat - 1) ? BeatSeconds[Beat - 1] : 2.f;
	if (BeatT >= Len)
	{
		BeatT = Len;
		Pose(DeltaSeconds);
		if (Beat >= 4) { EndSwap(); return; }
		if (bQueued || GSwapAuto) { StartBeat(Beat + 1); return; }
		Rate = 1.f;
	}
	else
	{
		Pose(DeltaSeconds);
	}
	UpdateCamera(DeltaSeconds);
}

void AScene7Swap::EndSwap()
{
	using namespace Scene7SwapLocal;	// unity builds: no file-scope usings (they leak into the next .cpp)
	bDone = true;
	Double->SetHiddenInGame(true);
	if (Vance)
	{
		const FTransform DT = Double->GetComponentTransform();
		const float Half = Vance->GetCapsuleComponent() ? Vance->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
		const float Yaw = DT.Rotator().Yaw + 90.f;
		Vance->SetActorLocationAndRotation(DT.GetLocation() + FVector(0.f, 0.f, Half), FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		Vance->SetLook(Yaw, -10.f);
		Vance->SetActorHiddenInGame(false);
		if (UCharacterMovementComponent* CM = Vance->GetCharacterMovement()) { CM->SetMovementMode(MOVE_Walking); }
		AttachCanisterTo(Vance);
		Vance->SnapFollowCamera();
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0)) { PC->SetViewTarget(Vance); }
	}
	CurrentShot = NAME_None;
}

void AScene7Swap::AttachCanisterTo(ATakeCharacter* Character)
{
	using namespace Scene7SwapLocal;	// unity builds: no file-scope usings (they leak into the next .cpp)
	USkeletalMeshComponent* M = Character ? Character->GetVisibleBody() : nullptr;
	if (!M) { return; }
	// The lid and the print travel with the can.
	CanLid->AttachToComponent(CanRoot, FAttachmentTransformRules::KeepWorldTransform);
	MasterReel->AttachToComponent(CanRoot, FAttachmentTransformRules::KeepWorldTransform);
	const FName BoneName = M->GetBoneIndex(CanCarryBone) != INDEX_NONE ? CanCarryBone : NAME_None;
	if (Character == Vance && Double->GetBoneIndex(CanCarryBone) != INDEX_NONE)
	{
		// Where the double held it, relative to the same bone (the real Vance now stands exactly there).
		const FTransform Rel = CanRoot->GetComponentTransform().GetRelativeTransform(Double->GetSocketTransform(CanCarryBone));
		CanRoot->AttachToComponent(M, FAttachmentTransformRules::SnapToTargetNotIncludingScale, BoneName);
		CanRoot->SetRelativeTransform(Rel);
	}
	else
	{
		CanRoot->AttachToComponent(M, FAttachmentTransformRules::KeepWorldTransform, BoneName);
	}
}

void AScene7Swap::Cue(FName Name, float Volume)
{
	using namespace Scene7SwapLocal;	// unity builds: no file-scope usings (they leak into the next .cpp)
	if (TObjectPtr<USoundBase>* S = Sounds.Find(Name))
	{
		if (*S) { UGameplayStatics::PlaySound2D(this, *S, Volume); }
	}
}

// ---- one frame of the beat -----------------------------------------------------------------------------------------

void AScene7Swap::Pose(float Dt)
{
	using namespace Scene7SwapLocal;	// unity builds: no file-scope usings (they leak into the next .cpp)
	const float T = BeatT;
	const FTransform Tb = BenchT();
	const FVector RS = ReelOnSpindle();
	const float RR = ReelRadius;
	const FQuat Spin(FVector::RightVector, FMath::DegreesToRadians(SpinAngle));

	// Rest places.
	const FTransform CanBench = FTransform(FVector(CanOnBench.X, CanOnBench.Y, BenchTop)) * Tb;
	const FTransform LidBench = FTransform(FVector(LidOnBench.X, LidOnBench.Y, BenchTop + 1.5f)) * Tb;
	const FTransform DummyBench = FTransform(Flat0, FVector(LidOnBench.X, LidOnBench.Y, BenchTop + 1.5f + 1.25f + ReelWidth * 0.5f)) * Tb;
	const FTransform OnSpindle(FQuat::Identity, RS);
	const FTransform OffSpindle(FQuat::Identity, RS + FVector(0.f, -9.f, 0.f));
	const FVector SB = BenchSpotActor();
	const float YB = BenchYawActor();

	// What this frame sets.
	FVector BodyPos = OpenSpot;
	float BodyYaw = OpenYaw;
	float Lean = 0.f;
	FVector Look = RS;
	float LookAlpha = 0.f;
	FHand L, R;
	float DoorDeg = 0.f, LatchPitch = 0.f, CanLatchPitch = -55.f;
	FTransform MasterBase = OnSpindle;		// the master's place (no spin)
	bool bMasterSpins = true;
	bool bMasterInCan = false;
	FVector StrandTop = RS + FVector(4.f, 0.f, -17.f), StrandBottom = Throat;
	bool bStrand = true;
	FTransform DummyT = DummyBench, LidT = LidBench, CanT = CanBench;
	bool bLidOnCan = false;
	FName Shot = NAME_None;
	float ShotStart = 0.f;

	auto ReelL = [&](const FTransform& X) { return On(X, FVector(RR + 1.6f, -0.6f, 3.f), FVector(0, 0, 1), FVector(-1, 0, 0), 0.6f); };
	auto ReelR = [&](const FTransform& X) { return On(X, FVector(-(RR + 1.6f), -0.6f, 3.f), FVector(0, 0, 1), FVector(1, 0, 0), 0.6f); };
	auto CarryAt = [&](const FVector& P, float Y) { return FTransform(Upright0, ReelCarry) * BodyT(P, Y); };
	auto DoorT = [&](float Deg) { return FTransform(FRotator(0.f, Deg, 0.f), Scene7SwapMesh::DoorHinge); };
	auto Move = [&](const FVector& A, float YA, const FVector& B, float YBb, float W)
	{
		BodyPos = FMath::Lerp(A, B, W);
		BodyYaw = YA + FMath::UnwindDegrees(YBb - YA) * W;
	};
	auto ShotAt = [&](const TCHAR* Name, float Start) { if (T >= Start) { Shot = Name; ShotStart = Start; } };

	switch (Beat)
	{
	case 0:
		break;

	case 1:		// OPEN: the latch, the door swings, a look inside.
	{
		ShotAt(TEXT("Wide"), 0.f);
		ShotAt(TEXT("Door"), 0.72f);
		Lean = 6.f * ES(T, 0.f, 0.5f);
		LookAlpha = 0.85f * ES(T, 0.f, 0.3f);
		DoorDeg = 40.f * ES(T, 0.55f, 1.1f) + (DoorOpenDegrees - 40.f) * ES(T, 1.1f, 1.5f);
		LatchPitch = -80.f * ES(T, 0.42f, 0.55f);
		const FTransform D = DoorT(DoorDeg);
		const FTransform LatchT = FTransform(FRotator(LatchPitch, 0.f, 0.f), DoorLatchLocal) * D;
		const FHand OnLatch = On(LatchT, FVector(0.f, -3.4f, 4.5f), FVector(-0.25f, 0.f, 1.f), FVector(0, 1, 0), 0.75f);
		const float Reach = ES(T, 0.05f, 0.45f) * (1.f - ES(T, 1.1f, 1.55f));
		L = LerpHand(Off(), OnLatch, Reach);
		L.Grip = OnLatch.Grip * ES(T, 0.3f, 0.45f) * (1.f - ES(T, 1.05f, 1.2f));
		Look = T < 0.6f ? LatchT.GetLocation() : FMath::Lerp(LatchT.GetLocation(), RS, ES(T, 0.6f, 1.0f));
		if (Crossed(0.47f)) { Cue(TEXT("sfx_s7_door_click"), 0.8f); }
		if (Crossed(0.6f)) { Cue(TEXT("sfx_s7_mag_open")); }
		break;
	}

	case 2:		// LIFT: stop the reel, pinch off the film, lift the print off the spindle, into the can.
	{
		ShotAt(TEXT("Reel"), 0.f);
		ShotAt(TEXT("Turn"), 1.4f);
		DoorDeg = DoorOpenDegrees;
		LatchPitch = -80.f;
		BodyPos = ReelSpot; BodyYaw = ReelYaw;
		if (T > 1.45f) { Move(ReelSpot, ReelYaw, SB, YB, ES(T, 1.45f, 2.15f)); }
		Lean = FMath::Lerp(8.f, 4.f, ES(T, 1.4f, 1.8f)) + (BenchLean - 4.f) * ES(T, 2.15f, 2.6f);
		LookAlpha = 0.85f;
		Look = T < 1.4f ? RS : CanBench.GetLocation() + FVector(0, 0, 5);
		SpinRate = 1.f - ES(T, 0.2f, 0.45f);

		// The master: off the spindle, carried, laid in the can.
		bMasterSpins = false;
		const FTransform Carry0 = CarryAt(ReelSpot, ReelYaw);
		const FTransform InCan = FTransform(Flat0, FVector(0, 0, 0.6f + ReelWidth * 0.5f)) * CanBench;
		if (T < 1.0f) { MasterBase = OnSpindle; }
		else if (T < 1.3f) { MasterBase = Carry(OnSpindle, OffSpindle, ES(T, 1.0f, 1.3f), 0.f); }
		else if (T < 1.5f) { MasterBase = Carry(OffSpindle, Carry0, ES(T, 1.3f, 1.5f), 0.f); }
		else if (T < 2.15f) { MasterBase = CarryAt(BodyPos, BodyYaw); }
		else { MasterBase = Carry(CarryAt(SB, YB), InCan, ES(T, 2.15f, 2.7f), 4.f); }

		// The right hand clamps the turning reel, then both take it by the rim.
		const FHand Clamp = On(OnSpindle, FVector(-12.f, -ReelWidth * 0.5f - 1.2f, 10.f), FVector(-0.3f, 0.f, 1.f), FVector(0, 1, 0), 0.25f);
		// The left pinches the film at the throat and tugs it free.
		const FVector Pinch = FMath::Lerp(StrandTop, StrandBottom, 0.45f) + FVector(0.f, -1.5f, 0.f);
		const FVector Tug = FVector(0.f, -7.f, -3.f) * ES(T, 0.4f, 0.6f);
		FHand PinchH = On(FTransform::Identity, Pinch + Tug, FVector(0.2f, 0.3f, -1.f), FVector(0, 1, 0), 0.85f);
		if (T < 0.75f)
		{
			R = LerpHand(Off(), Clamp, ES(T, 0.f, 0.3f));
			L = LerpHand(Off(), PinchH, ES(T, 0.1f, 0.4f));
		}
		else if (T < 1.0f)
		{
			const float W = ES(T, 0.75f, 1.0f);
			R = LerpHand(Clamp, ReelR(MasterBase), W);
			L = LerpHand(PinchH, ReelL(MasterBase), W);
		}
		else
		{
			R = ReelR(MasterBase);
			L = ReelL(MasterBase);
			const float Let = ES(T, 2.72f, 3.0f);
			R = LerpHand(R, Off(), Let);
			L = LerpHand(L, Off(), Let);
		}
		// The strand: follows the pinch, then it's gone.
		if (T < 0.45f) { bStrand = true; }
		else if (T < 0.78f) { StrandBottom = Pinch + Tug + FVector(0, 0, -1.5f); }
		else { bStrand = false; }
		bMasterInCan = T >= 2.7f;

		if (Crossed(0.22f)) { Cue(TEXT("sfx_s7_door_click"), 0.35f); }
		if (Crossed(0.55f)) { Cue(TEXT("sfx_s7_reel_swap"), 0.5f); }
		if (Crossed(1.05f)) { Cue(TEXT("sfx_s7_reel_swap")); }
		if (Crossed(2.62f)) { Cue(TEXT("sfx_s7_clang"), 0.3f); }
		break;
	}

	case 3:		// DUMMY: up off the lid, onto the spindle, the door shut.
	{
		ShotAt(TEXT("Bench"), 0.f);
		ShotAt(TEXT("Spindle"), 1.2f);
		bMasterSpins = false; bMasterInCan = true; bStrand = false; SpinRate = 0.f;
		BodyPos = SB; BodyYaw = YB;
		if (T > 0.75f) { Move(SB, YB, ReelSpot, ReelYaw, ES(T, 0.75f, 1.25f)); }
		Lean = FMath::Lerp(BenchLean, 8.f, ES(T, 0.55f, 1.0f));
		LookAlpha = 0.85f;
		Look = T < 0.75f ? DummyBench.GetLocation() : RS;
		DoorDeg = DoorOpenDegrees * (1.f - ES(T, 2.0f, 2.35f));
		LatchPitch = -80.f * (1.f - ES(T, 2.35f, 2.45f));

		if (T < 0.35f) { DummyT = DummyBench; }
		else if (T < 0.75f) { DummyT = Carry(DummyBench, CarryAt(SB, YB), ES(T, 0.35f, 0.75f), 6.f); }
		else if (T < 1.25f) { DummyT = CarryAt(BodyPos, BodyYaw); }
		else if (T < 1.6f) { DummyT = Carry(CarryAt(ReelSpot, ReelYaw), OffSpindle, ES(T, 1.25f, 1.6f), 0.f); }
		else { DummyT = Carry(OffSpindle, OnSpindle, ES(T, 1.6f, 1.8f), 0.f); }

		const FTransform D = DoorT(DoorDeg);
		const FHand Push = On(D, FVector(-24.f, 2.6f, -2.f), FVector(-0.2f, 0.f, 1.f), FVector(0, -1, 0), 0.1f);
		if (T < 1.8f)
		{
			const float Grab = ES(T, 0.f, 0.35f);
			R = LerpHand(Off(), ReelR(DummyT), Grab);
			L = LerpHand(Off(), ReelL(DummyT), Grab);
		}
		else
		{
			R = LerpHand(ReelR(DummyT), Off(), ES(T, 1.8f, 2.05f));
			L = LerpHand(ReelL(DummyT), Push, ES(T, 1.8f, 2.0f));
			L = LerpHand(L, Off(), ES(T, 2.28f, 2.6f));
		}
		if (Crossed(0.4f)) { Cue(TEXT("sfx_s7_reel_swap"), 0.6f); }
		if (Crossed(1.75f)) { Cue(TEXT("sfx_s7_clang"), 0.35f); }
		if (Crossed(2.33f)) { Cue(TEXT("sfx_s7_mag_open"), 0.9f); }
		if (Crossed(2.42f)) { Cue(TEXT("sfx_s7_door_click")); }
		break;
	}

	case 4:		// SEAL: lid on, latch, the can under his arm, a look at the door.
	{
		ShotAt(TEXT("Can"), 0.f);
		ShotAt(TEXT("Face"), 1.45f);
		bMasterSpins = false; bMasterInCan = true; bStrand = false; SpinRate = 0.f;
		DummyT = OnSpindle;
		BodyPos = SB; BodyYaw = YB + 25.f * ES(T, 2.6f, 3.2f);
		Lean = BenchLean * (1.f - ES(T, 1.45f, 2.0f));
		const FTransform LidOn = FTransform(FVector(0, 0, CanDepth)) * CanBench;
		if (T < 0.35f) { LidT = LidBench; }
		else if (T < 0.95f) { LidT = Carry(LidBench, LidOn, ES(T, 0.35f, 0.95f), 9.f); }
		else { bLidOnCan = true; }
		CanLatchPitch = -55.f * (1.f - ES(T, 1.12f, 1.28f));
		const FTransform ArmT = FTransform(FQuat(FVector::ForwardVector, PI * 0.5f), CanUnderArm) * BodyT(BodyPos, BodyYaw);
		CanT = T < 1.6f ? CanBench : Carry(CanBench, ArmT, ES(T, 1.6f, 2.3f), 8.f);
		if (bLidOnCan) { LidT = FTransform(FVector(0, 0, CanDepth)) * CanT; }

		auto LidL = [&](const FTransform& X) { return On(X, FVector(-(CanRadius + 1.8f), 0.f, 0.4f), FVector(0, -1, -0.5f), FVector(1, 0, 0), 0.5f); };
		auto LidR = [&](const FTransform& X) { return On(X, FVector(CanRadius + 1.8f, 0.f, 0.4f), FVector(0, -1, -0.5f), FVector(-1, 0, 0), 0.5f); };
		auto CanL = [&](const FTransform& X) { return On(X, FVector(-(CanRadius + 1.4f), 2.f, 3.f), FVector(0, -1, -0.6f), FVector(1, 0, 0), 0.6f); };
		auto CanR = [&](const FTransform& X) { return On(X, FVector(CanRadius + 1.4f, 2.f, 3.f), FVector(0, -1, -0.6f), FVector(-1, 0, 0), 0.6f); };
		const FTransform LatchW = FTransform(FRotator(CanLatchPitch, 0.f, 0.f), FVector(CanRadius + 0.6f, 0.f, 1.6f)) * CanT;
		const FHand Snap = On(LatchW, FVector(1.6f, 0.f, 5.f), FVector(-0.4f, 0.f, 1.f), FVector(-1, 0, 0), 0.3f);
		// Under the arm: the left hand cups the can's front edge.
		const FHand Cup = On(BodyT(BodyPos, BodyYaw), FVector(16.f, -29.f, 90.f), FVector(1.f, 0.f, 0.25f), FVector(-0.2f, 0.f, 1.f), 0.55f);

		if (T < 1.05f)
		{
			const float Grab = ES(T, 0.f, 0.35f);
			L = LerpHand(Off(), LidL(LidT), Grab);
			R = LerpHand(Off(), LidR(LidT), Grab);
			if (T > 0.95f) { L.Palm.Z -= 0.6f; R.Palm.Z -= 0.6f; }
		}
		else if (T < 1.35f)
		{
			L = LidL(LidT);
			R = LerpHand(LidR(LidT), Snap, ES(T, 1.02f, 1.14f));
		}
		else
		{
			const float W = ES(T, 1.35f, 1.6f);
			L = LerpHand(LidL(LidT), CanL(CanT), W);
			R = LerpHand(Snap, CanR(CanT), W);
			if (T > 2.0f)
			{
				R = LerpHand(R, Off(), ES(T, 2.0f, 2.4f));
				L = LerpHand(CanL(CanT), Cup, ES(T, 2.0f, 2.35f));
			}
		}
		LookAlpha = 0.85f;
		Look = T < 2.4f ? CanT.GetLocation() : FMath::Lerp(CanT.GetLocation(), GlanceAt, ES(T, 2.4f, 2.9f));
		if (Crossed(0.98f)) { Cue(TEXT("sfx_s7_clang"), 0.45f); }
		if (Crossed(1.22f)) { Cue(TEXT("sfx_s7_canister_latch")); }
		break;
	}
	default:
		break;
	}

	if (Beat >= 3) { DummyT = Beat == 4 ? OnSpindle : DummyT; }
	if (Beat == 4) { bMasterSpins = false; }

	// ---- the props ----
	DoorHinge->SetRelativeRotation(FRotator(0.f, DoorDeg, 0.f));
	DoorLatch->SetRelativeRotation(FRotator(LatchPitch, 0.f, 0.f));
	CanLatch->SetRelativeRotation(FRotator(CanLatchPitch, 0.f, 0.f));
	if (CanRoot->GetAttachParent() == Root) { CanRoot->SetRelativeTransform(CanT); }
	if (CanLid->GetAttachParent() == Root) { CanLid->SetRelativeTransform(bLidOnCan ? FTransform(FVector(0, 0, CanDepth)) * CanT : LidT); }
	if (MasterReel->GetAttachParent() == Root)
	{
		const FTransform InCan = FTransform(Flat0, FVector(0, 0, 0.6f + ReelWidth * 0.5f)) * CanT;
		const FTransform Base = bMasterInCan ? InCan : MasterBase;
		MasterReel->SetRelativeTransform(FTransform(Spin) * Base);
	}
	DummyReel->SetRelativeTransform(DummyT);
	FilmStrand->SetHiddenInGame(!bStrand);
	if (bStrand)
	{
		const FVector D = StrandBottom - StrandTop;
		const float Len = FMath::Max(D.Size(), 0.1f);
		const FQuat Q = FQuat::FindBetweenNormals(FVector(0, 0, -1), D / Len);
		FilmStrand->SetRelativeTransform(FTransform(Q, StrandTop, FVector(1.f, 1.f, Len)));
	}

	// ---- the double ----
	if (Beat == 0 || !Double->GetSkeletalMeshAsset()) { return; }
	const FTransform Body = BodyT(BodyPos, BodyYaw);
	const FTransform MeshT(FRotator(0.f, BodyYaw - 90.f, 0.f), FVector(BodyPos.X, BodyPos.Y, 0.f), Double->GetRelativeScale3D());
	Double->SetRelativeTransform(MeshT);
	UScene7SwapAnimInstance* A = Cast<UScene7SwapAnimInstance>(Double->GetAnimInstance());
	if (!A) { return; }
	A->PalmOffset = PalmOffset;
	A->GripDegrees = GripDegrees;
	FScene7SwapPose& P = A->Pose;
	auto CSP = [&](const FVector& X) { return MeshT.InverseTransformPosition(X); };
	auto CSV = [&](const FVector& X) { return MeshT.InverseTransformVectorNoScale(X).GetSafeNormal(); };
	auto Fill = [&](FScene7SwapHand& Out, const FHand& In, float Side)
	{
		Out.Alpha = In.Alpha;
		Out.Palm = CSP(In.Palm);
		Out.Fingers = CSV(In.Fingers);
		Out.PalmFacing = CSV(In.Facing);
		Out.Grip = In.Grip;
		Out.Pole = CSP(Body.TransformPosition(FVector(-20.f, Side * 50.f, ShoulderZ - 45.f)));
	};
	Fill(P.Left, L, -1.f);
	Fill(P.Right, R, 1.f);
	P.Lean = Lean;
	P.LeanAxis = CSV(Body.TransformVectorNoScale(FVector::RightVector));
	P.LookAt = CSP(Look);
	P.LookAlpha = LookAlpha;

	if (Shot != CurrentShot) { SetShot(Shot); ShotT = T - ShotStart; }
	else { ShotT = T - ShotStart; }
}

void AScene7Swap::SetShot(FName Name)
{
	using namespace Scene7SwapLocal;	// unity builds: no file-scope usings (they leak into the next .cpp)
	CurrentShot = Name;
	ShotT = 0.f;
}

void AScene7Swap::UpdateCamera(float Dt)
{
	using namespace Scene7SwapLocal;	// unity builds: no file-scope usings (they leak into the next .cpp)
	const FScene7SwapShot* S = Shots.Find(CurrentShot);
	if (!S) { return; }
	const FTransform Actor = GetActorTransform();
	const FTransform MeshRel = Double->GetRelativeTransform();
	const FTransform Body = FTransform(FRotator(0.f, MeshRel.Rotator().Yaw + 90.f, 0.f), MeshRel.GetLocation()) * Actor;
	const FTransform Space = S->bBody ? Body : (S->bBench ? BenchT() * Actor : Actor);
	const float W = Ease(ShotT / FMath::Max(S->Move, 0.01f));
	FVector From = Space.TransformPosition(FMath::Lerp(S->From, S->To, W));
	FVector Aim;
	if (!S->AimBone.IsNone() && Double->GetBoneIndex(S->AimBone) != INDEX_NONE)
	{
		Aim = Double->GetSocketLocation(S->AimBone) + Actor.TransformVectorNoScale(S->Aim);
	}
	else
	{
		Aim = Space.TransformPosition(S->Aim);
	}
	// A hand-held breath.
	const double Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
	From += FVector(FMath::Sin(Now * 1.3) * 0.6 + FMath::Sin(Now * 2.9) * 0.25, FMath::Sin(Now * 1.1 + 1.0) * 0.5, FMath::Sin(Now * 1.7 + 2.0) * 0.4) * S->Drift;
	Cam->SetWorldLocationAndRotation(From, (Aim - From).Rotation());
	Cam->SetCurrentFocalLength(S->FocalTo > 0.f ? FMath::Lerp(S->Focal, S->FocalTo, W) : S->Focal);
	Cam->CurrentAperture = S->Aperture;
	Cam->FocusSettings.FocusMethod = ECameraFocusMethod::Manual;
	Cam->FocusSettings.ManualFocusDistance = FVector::Dist(From, Aim);
}

// ---- the prompt ----------------------------------------------------------------------------------------------------

void AScene7Swap::DrawSwapHud(UCanvas* Canvas)
{
	using namespace Scene7SwapLocal;	// unity builds: no file-scope usings (they leak into the next .cpp)
	if (!Canvas || bDone) { return; }
	FTakeHudKit Kit;
	Kit.Begin(Canvas, FPlatformTime::Seconds(), 0.f);
	const float CX = Kit.CX();
	const float Y = Kit.H * 0.84f;

	if (Beat == 0)
	{
		Kit.Text(TakeStyle::EType::Goal, Goal, CX, Y, TakeStyle::Bone, ETakeAlign::Centre);
		return;
	}
	const float Len = BeatSeconds.IsValidIndex(Beat - 1) ? BeatSeconds[Beat - 1] : 2.f;
	const bool bWaiting = Rate == 0.f || (BeatT >= Len - KINDA_SMALL_NUMBER && Beat < 4);
	if (!bWaiting) { return; }
	const int32 Next = Rate == 0.f ? 0 : Beat;
	if (!PromptWords.IsValidIndex(Next)) { return; }
	const FString Words = PromptWords[Next];
	const float TW = Kit.Measure(TakeStyle::EType::Line, Words);
	const float KW = Kit.KeyWidth(TEXT("E"), false);
	const float Gap = Kit.Px(16.f);
	const float X0 = CX - (TW + KW + Gap) * 0.5f;
	Kit.Rect(0.f, Y - Kit.Px(34.f), Kit.W, Kit.Px(68.f), TakeStyle::Ink * FLinearColor(1, 1, 1, 0.45f));
	Kit.Key(TEXT("E"), X0, Y, FTakeInk::Of(TakeStyle::Paper, TakeStyle::Julian));
	Kit.Text(TakeStyle::EType::Line, Words, X0 + KW + Gap, Y, TakeStyle::Paper);
	Kit.Text(TakeStyle::EType::Label, FString::Printf(TEXT("%d / 4"), Next + 1), CX, Y - Kit.Px(50.f), TakeStyle::Ash, ETakeAlign::Centre);
}
