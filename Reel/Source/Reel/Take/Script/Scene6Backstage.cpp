// See Scene6Backstage.h. Owner: the s6-backstage session.
#include "Script/Scene6Backstage.h"

#include "CastMember.h"
#include "TakeCharacter.h"
#include "Script/Scene5Auditorium.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "CanvasItem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogScene6, Log, All);

namespace
{
	const FName TagLid(TEXT("s6_trunk_lid"));
	const FName TagLever(TEXT("s6_lever_handle"));
	const FName TagSandbag(TEXT("s6_sandbag"));
	const FName TagSheet(TEXT("s6_sheet"));
	const FName TagPanel(TEXT("s6_panel_cover"));
	const FName TagExit(TEXT("s6_exit_leaf"));
	const FName TagWedge(TEXT("s6_exit_wedge"));
	const FVector Z(0.f, 0.f, 1.f);

	float Ease(float T) { T = FMath::Clamp(T, 0.f, 1.f); return T * T * (3.f - 2.f * T); }
	float Yaw2D(const FVector& V) { return static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(V.Y, V.X))); }
	FVector Flat(const FVector& V) { return FVector(V.X, V.Y, 0.f); }
	FVector RightOf(const FVector& Fwd) { return FVector::CrossProduct(Z, Fwd).GetSafeNormal(); }
	/** Tilts a frame forward (its top toward its front) by Deg. */
	FS6Frame Tilt(const FS6Frame& F, float Deg)
	{
		const float R = FMath::DegreesToRadians(Deg);
		const FVector Fw = F.Fwd.GetSafeNormal(), Up = F.Up.GetSafeNormal();
		return FS6Frame(Fw * FMath::Cos(R) - Up * FMath::Sin(R), Up * FMath::Cos(R) + Fw * FMath::Sin(R));
	}
	/** Rolls a frame about its front by Deg. */
	FS6Frame Roll(const FS6Frame& F, float Deg)
	{
		const FVector Fw = F.Fwd.GetSafeNormal();
		return FS6Frame(Fw, FQuat(Fw, FMath::DegreesToRadians(Deg)).RotateVector(F.Up));
	}

	AScene6Backstage* ActiveS6(UWorld* World)
	{
		if (!World) { return nullptr; }
		for (TActorIterator<AScene6Backstage> It(World); It; ++It)
		{
			if (It->IsActive()) { return *It; }
		}
		return nullptr;
	}
}

// Test hooks (console). Reel.S6.Use: press Use. Reel.S6.Crouch 0|1. Reel.S6.Behind: Cole 110 cm behind the guard.
// Reel.S6.Goto lever|panel|exit|body|trunk: Cole to that use point.
static FAutoConsoleCommandWithWorldAndArgs GS6UseCmd(
	TEXT("Reel.S6.Use"), TEXT("Reel.S6.Use: press Use for Cole in the active Scene 6 (test)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (AScene6Backstage* S = ActiveS6(World)) { S->TestUse(); }
	}));
static FAutoConsoleCommandWithWorldAndArgs GS6CrouchCmd(
	TEXT("Reel.S6.Crouch"), TEXT("Reel.S6.Crouch 0|1: Cole crouches (test)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (AScene6Backstage* S = ActiveS6(World)) { S->TestCrouch(Args.Num() == 0 || Args[0] != TEXT("0")); }
	}));
static FAutoConsoleCommandWithWorldAndArgs GS6BehindCmd(
	TEXT("Reel.S6.Behind"), TEXT("Reel.S6.Behind: Cole 110 cm behind the guard (test)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (AScene6Backstage* S = ActiveS6(World))
		{
			const float Yaw = S->TestGuardYaw();
			const FVector Fwd = FRotator(0.f, Yaw, 0.f).Vector();
			S->TestPlaceCole(S->TestGuardFeet() - Fwd * 110.f, Yaw);
		}
	}));
static FAutoConsoleCommandWithWorldAndArgs GS6GotoCmd(
	TEXT("Reel.S6.Goto"), TEXT("Reel.S6.Goto lever|panel|exit|trunk: Cole to that use point (test)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		AScene6Backstage* S = ActiveS6(World);
		if (!S || Args.Num() < 1) { return; }
		const FString& W = Args[0];
		const FVector At = W == TEXT("lever") ? S->LeverPoint + FVector(60.f, 0.f, 0.f)
			: W == TEXT("panel") ? S->PanelStand
			: W == TEXT("exit") ? S->ExitStand
			: S->TrunkPoint + FVector(-120.f, 0.f, 0.f);
		const FVector Look = W == TEXT("panel") ? S->PanelPoint : W == TEXT("exit") ? S->ExitAlarmPoint
			: W == TEXT("lever") ? S->LeverPoint : S->TrunkPoint;
		S->TestPlaceCole(At, Yaw2D(Look - At));
	}));

AScene6Backstage::AScene6Backstage()
{
	SceneNumber = 6;
	Pov = EScriptPov::Cole;
	Slug = FText::FromString(TEXT("INT. BACKSTAGE - CONTINUOUS"));
	bPlayerWalks = true;
	bAllowCrouch = true;	// C / Ctrl: ATakeCharacter's crouch (chars-locomotion)

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// The back of the screen: an engine plane stood up to face +X (into the room), 10 m x 4.8 m, mirrored.
	ScreenBack = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScreenBack"));
	ScreenBack->SetupAttachment(Root);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded())
	{
		ScreenBack->SetStaticMesh(PlaneMesh.Object);
	}
	ScreenBack->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	ScreenBack->SetRelativeScale3D(FVector(4.8f, -10.f, 1.f));
	ScreenBack->SetCollisionProfileName(TEXT("NoCollision"));
	ScreenBack->SetCastShadow(false);

	FlickerLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FlickerLight"));
	FlickerLight->SetupAttachment(Root);
	FlickerLight->SetRelativeLocation(FVector(140.f, 0.f, 0.f));
	FlickerLight->SetIntensityUnits(ELightUnits::Candelas);
	FlickerLight->SetIntensity(FlickerCandelas);
	FlickerLight->SetAttenuationRadius(1600.f);
	FlickerLight->SetLightColor(FLinearColor(0.72f, 0.80f, 1.f));
	FlickerLight->SetCastShadows(true);

	auto Lamp = [&](const TCHAR* Name, float Cd, float Radius, const FLinearColor& C)
	{
		UPointLightComponent* L = CreateDefaultSubobject<UPointLightComponent>(Name);
		L->SetupAttachment(Root);
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(Cd);
		L->SetAttenuationRadius(Radius);
		L->SetLightColor(C);
		L->SetCastShadows(false);
		return L;
	};
	LockLamp = Lamp(TEXT("LockLamp"), 60.f, 260.f, FLinearColor(1.f, 0.05f, 0.03f));
	AlarmLamp = Lamp(TEXT("AlarmLamp"), 60.f, 260.f, FLinearColor(1.f, 0.05f, 0.03f));
	SparkLight = Lamp(TEXT("SparkLight"), 0.f, 420.f, FLinearColor(0.75f, 0.85f, 1.f));

	auto Stunt = [&](const TCHAR* Name)
	{
		UPoseableMeshComponent* P = CreateDefaultSubobject<UPoseableMeshComponent>(Name);
		P->SetupAttachment(Root);
		P->SetHiddenInGame(true);
		P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		P->SetCastShadow(false);
		return P;
	};
	ColePose = Stunt(TEXT("ColePose"));
	GuardPose = Stunt(TEXT("GuardPose"));
}

void AScene6Backstage::BeginPlay()
{
	Super::BeginPlay();
	// The lamps sit just off their wall, toward where Cole stands.
	const FVector PanelOut = (PanelStand - PanelPoint).GetSafeNormal2D();
	const FVector AlarmOut = (ExitStand - ExitAlarmPoint).GetSafeNormal2D();
	LockLamp->SetWorldLocation(PanelPoint + PanelOut * 20.f + FVector(0.f, 0.f, 30.f));
	AlarmLamp->SetWorldLocation(ExitAlarmPoint + AlarmOut * 20.f + FVector(0.f, 0.f, 20.f));
	SparkLight->SetWorldLocation(PanelPoint + PanelOut * 30.f);
	if (UMaterialInterface* Film = FilmMaterial.LoadSynchronous())
	{
		FilmMID = UMaterialInstanceDynamic::Create(Film, this);
		ScreenBack->SetMaterial(0, FilmMID);
		FilmMID->SetScalarParameterValue(TEXT("Play"), 1.f);
		FilmMID->SetScalarParameterValue(TEXT("Brightness"), FilmBrightness);
		FilmMID->SetScalarParameterValue(TEXT("T0"), GetWorld()->GetTimeSeconds());
		FilmMID->SetScalarParameterValue(TEXT("Aspect"), 1000.f / 480.f);
	}
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	StuntCam = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), GetActorTransform(), SP);
	InsertCam = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(),
		FTransform((InsertLookAt - InsertCamAt).Rotation(), InsertCamAt), SP);
	for (ACameraActor* C : { StuntCam.Get(), InsertCam.Get() })
	{
		if (C && C->GetCameraComponent())
		{
			C->GetCameraComponent()->SetConstraintAspectRatio(false);
			C->GetCameraComponent()->SetFieldOfView(C == InsertCam ? 45.f : 58.f);
		}
	}
	FindProps();
	SpawnCast();
	ResetScene();
	SetExitOpen(1.f);	// open outside Scene 6 (the escape runs through it); BeginScene shuts it
}

void AScene6Backstage::SetExitOpen(float Alpha)
{
	TurnAboutEdge(ExitLeaf, ExitHome, FVector(-50.f, 0.f, 0.f), FVector(0.f, 0.f, 1.f), ExitOpenDegrees * Alpha);
	if (ExitWedge)
	{
		ExitWedge->SetActorTransform(WedgeHome);
		ExitWedge->SetActorHiddenInGame(Alpha < 1.f);
	}
}

// ---- the set -------------------------------------------------------------------------------------------------------

void AScene6Backstage::SpawnCast()
{
	if (bSpawned)
	{
		return;
	}
	bSpawned = true;
	G.Name = TEXT("ENFORCER");
	G.Home = GuardPost;
	G.HomeYaw = GuardYaw;
	const FTransform Xf(FRotator(0.f, GuardYaw, 0.f), GuardPost);
	ACastMember* M = GetWorld()->SpawnActorDeferred<ACastMember>(ACastMember::StaticClass(), Xf, this, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (M)
	{
		M->Kit = ECastKit::Bodyguard;
		M->Tint = FLinearColor(0.035f, 0.035f, 0.04f);
		M->Tags.Add(TEXT("script_s6"));
		M->Tags.Add(TEXT("S6_Enforcer"));
#if WITH_EDITOR
		M->SetActorLabel(TEXT("S6_Enforcer"));
#endif
		M->FinishSpawning(Xf);
	}
	G.Body = M;
	UE_LOG(LogScene6, Log, TEXT("S6 cast: guard %s"), M ? TEXT("ok") : TEXT("none"));
}

void AScene6Backstage::FindProps()
{
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Act = *It;
		if (Act->ActorHasTag(TagLid)) { TrunkLid = Act; LidHome = Act->GetActorTransform(); }
		else if (Act->ActorHasTag(TagLever)) { LeverHandle = Act; LeverHome = Act->GetActorTransform(); }
		else if (Act->ActorHasTag(TagSandbag)) { Sandbag = Act; SandbagHome = Act->GetActorTransform(); }
		else if (Act->ActorHasTag(TagSheet)) { Sheet = Act; SheetHome = Act->GetActorTransform(); }
		else if (Act->ActorHasTag(TagPanel)) { PanelCover = Act; PanelHome = Act->GetActorTransform(); }
		else if (Act->ActorHasTag(TagExit)) { ExitLeaf = Act; ExitHome = Act->GetActorTransform(); }
		else if (Act->ActorHasTag(TagWedge)) { ExitWedge = Act; WedgeHome = Act->GetActorTransform(); }
	}
	for (AActor* Act : { TrunkLid.Get(), LeverHandle.Get(), Sandbag.Get(), Sheet.Get(), PanelCover.Get(), ExitLeaf.Get(), ExitWedge.Get() })
	{
		if (Act)
		{
			if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Act->GetRootComponent()))
			{
				Prim->SetMobility(EComponentMobility::Movable);
			}
		}
	}
	UE_LOG(LogScene6, Log, TEXT("S6 props: lid %d lever %d sandbag %d sheet %d panel %d exit %d wedge %d"), TrunkLid != nullptr,
		LeverHandle != nullptr, Sandbag != nullptr, Sheet != nullptr, PanelCover != nullptr, ExitLeaf != nullptr, ExitWedge != nullptr);
}

void AScene6Backstage::PlaceGuardHome()
{
	G.Alert = 0.f;
	G.bDown = false;
	G.Home = GuardPost;
	G.HomeYaw = GuardYaw;
	GuardState = EGuard::Post;
	GuardAge = 0.f;
	GuardRig.Detach();
	if (ACastMember* M = G.Body)
	{
		M->StopWalking();
		M->SetActorHiddenInGame(false);
		M->SetActorLocationAndRotation(G.Home, FRotator(0.f, G.HomeYaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		M->FaceYaw(G.HomeYaw);
		M->PlayGesture(NAME_None);
	}
}

void AScene6Backstage::SetupRigs()
{
	if (ATakeCharacter* Cole = GetPovCharacter())
	{
		if (!ColeRig.IsReady() && Cole->GetMesh())
		{
			ColeRig.Init(ColePose, Cole->GetMesh());
		}
	}
	if (!GuardRig.IsReady() && G.Body && G.Body->Body)
	{
		GuardRig.Init(GuardPose, G.Body->Body);
	}
}

void AScene6Backstage::ResetScene()
{
	Step = EStep::Sneak;
	StepAge = 0.f;
	ColeRig.Detach();
	PlaceGuardHome();
	CaughtReason = NAME_None;
	CaughtAge = 0.f;
	LeverAge = CrashAge = LidAge = PanelAge = ExitAge = -1.f;
	bLidOpen = false;
	bLeverArmed = true;
	FlashLeft = 0.f;
	LastLoudCycle = -1;
	ToastLeft = 0.f;
	GaitPhase = 0.f;
	RouteIndex = 0;
	DragPath.Reset();
	if (TrunkLid) TrunkLid->SetActorTransform(LidHome);
	if (LeverHandle) LeverHandle->SetActorTransform(LeverHome);
	if (Sandbag) Sandbag->SetActorTransform(SandbagHome);
	if (Sheet) Sheet->SetActorTransform(SheetHome);
	if (PanelCover) PanelCover->SetActorTransform(PanelHome);
	LockLamp->SetLightColor(FLinearColor(1.f, 0.05f, 0.03f));
	AlarmLamp->SetIntensity(60.f);
	SparkLight->SetIntensity(0.f);
	if (IsActive())
	{
		SetCrouch(false);
		FreezeCole(false);
		ViewStunt(false);
	}
	Objective = TEXT("Pull the counterweight lever on the west wall to draw the guard off. Crouch [C] to move silently.");
}

void AScene6Backstage::BeginScene()
{
	Super::BeginScene();
	ResetScene();
	SetExitOpen(0.f);	// shut until Cole wedges it (open in every other scene)
	SetupRigs();
	for (TActorIterator<AScene5Auditorium> It(GetWorld()); It; ++It) { It->SetDoorUnlocked(false); }
	SayCue(TEXT("COLE"), TEXT("One heavy on the door controls. Going dark."), ColeOpenLine);
	if (GetRetakes() >= 2)
	{
		Flash(TEXT("Crouch [C]. Throw the lever, then come at him from behind while he looks at the crash."), 5.f);
	}
	UE_LOG(LogScene6, Log, TEXT("S6 begin (retake %d)"), GetRetakes());
}

void AScene6Backstage::EndScene()
{
	Super::EndScene();
	SetExitOpen(1.f);	// whatever happened here, the escape finds it open
	if (G.Body) { G.Body->StopWalking(); }
	ColeRig.Detach();
	SetCrouch(false);
}

// ---- Cole ----------------------------------------------------------------------------------------------------------

void AScene6Backstage::SetCrouch(bool bOn)
{
	if (ATakeCharacter* Cole = GetPovCharacter())
	{
		Cole->SetCrouching(bOn);
	}
	bColeCrouched = bOn;
}

void AScene6Backstage::FreezeCole(bool bFreeze)
{
	bPlayerWalks = !bFreeze;
	if (ATakeCharacter* Cole = GetPovCharacter())
	{
		if (bFreeze) { Cole->GetCharacterMovement()->StopMovementImmediately(); }
	}
}

void AScene6Backstage::ViewStunt(bool bOn)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}
	if (bOn && StuntCam) { PC->SetViewTarget(StuntCam); }
	else if (ATakeCharacter* Cole = GetPovCharacter()) { PC->SetViewTarget(Cole); }
}

FVector AScene6Backstage::ColeLocation() const
{
	const ACharacter* Cole = GetPovCharacter();
	if (!Cole)
	{
		Cole = UGameplayStatics::GetPlayerCharacter(this, 0);
	}
	if (!Cole)
	{
		return FVector(1e7f);
	}
	const float Half = Cole->GetCapsuleComponent() ? Cole->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	return Cole->GetActorLocation() - FVector(0.f, 0.f, Half);	// the feet
}

bool AScene6Backstage::ColeMovingUpright() const
{
	const ATakeCharacter* Cole = GetPovCharacter();
	return Cole && !bColeCrouched && Cole->GetVelocity().Size2D() > 60.f;
}

void AScene6Backstage::TestCrouch(bool bOn) { SetCrouch(bOn); }

void AScene6Backstage::TestPlaceCole(const FVector& Feet, float Yaw)
{
	if (ATakeCharacter* Cole = GetPovCharacter())
	{
		const float Half = Cole->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Cole->SetActorLocationAndRotation(Feet + FVector(0.f, 0.f, Half + 2.f), FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		Cole->SetLook(Yaw, -12.f);
	}
}

FVector AScene6Backstage::TestGuardFeet() const { return G.Body ? G.Body->GetActorLocation() : GuardPost; }
float AScene6Backstage::TestGuardYaw() const { return G.Body ? static_cast<float>(G.Body->GetActorRotation().Yaw) : GuardYaw; }

// ---- the film (look and sound only) --------------------------------------------------------------------------------

void AScene6Backstage::StepFilm(float Dt)
{
	const float Period = FMath::Max(FilmPeriod, 1.f);
	const float T = GetSceneSeconds() + 1.f;
	const int32 Cycle = FMath::FloorToInt(T / Period);
	const bool bLoud = FMath::Fmod(T, Period) >= LoudStart;
	const bool bBoom = (Cycle % 2) == 1;
	if (bLoud && Cycle != LastLoudCycle)
	{
		LastLoudCycle = Cycle;
		Play(bBoom ? ExplosionCues : GunfightCues, nullptr, 0.7f);
		FlashColor = bBoom ? FLinearColor(1.f, 0.55f, 0.2f) : FLinearColor(1.f, 0.95f, 0.85f);
		FlashLeft = Period - LoudStart;
	}
	FlashLeft = FMath::Max(0.f, FlashLeft - Dt);
	const float Now = GetSceneSeconds();
	float Flash = 0.f;
	if (FlashLeft > 0.f)
	{
		Flash = bBoom ? 4.f * FMath::Exp(-2.2f * ((Period - LoudStart) - FlashLeft)) + 0.6f
			: ((FMath::Fmod(Now * 11.f, 1.f) < 0.35f) ? 2.6f : 0.25f);
	}
	const float Noise = 0.75f + 0.15f * FMath::Sin(Now * 23.f) + 0.1f * FMath::Sin(Now * 61.f + 1.3f);
	FlickerLight->SetIntensity(FlickerCandelas * (Noise + Flash));
	FlickerLight->SetLightColor(Flash > 0.5f ? FlashColor : FLinearColor(0.72f, 0.8f, 1.f));
	if (FilmMID)
	{
		FilmMID->SetScalarParameterValue(TEXT("Brightness"), FilmBrightness * (Noise + 0.6f * Flash));
	}
}

// ---- the guard -----------------------------------------------------------------------------------------------------

void AScene6Backstage::Lure()
{
	ACastMember* M = G.Body;
	if (!M || G.bDown)
	{
		return;
	}
	GuardState = EGuard::ToLook;
	GuardAge = 0.f;
	M->PlayGesture(NAME_None);
	M->WalkTo(LookoutPoint, LureSpeed);
	SayCue(TEXT("ENFORCER"), TEXT("What the hell was that?"), EnforcerHuhLine);
	Objective = TEXT("He's looking at the crash. Sneak up behind him and press [E].");
}

void AScene6Backstage::StepGuard(float Dt)
{
	ACastMember* M = G.Body;
	if (!M || G.bDown)
	{
		return;
	}
	GuardAge += Dt;
	switch (GuardState)
	{
	case EGuard::Post:
	{
		const int32 Beat = FMath::FloorToInt(GuardAge / FMath::Max(ScanSeconds, 0.5f));
		const float Want = G.HomeYaw + ((Beat % 2) == 0 ? -ScanDegrees : ScanDegrees);
		M->FaceYaw(Want);
		break;
	}
	case EGuard::ToLook:
		if (!M->IsWalking())
		{
			GuardState = EGuard::Looking;
			GuardAge = 0.f;
			M->FaceYaw(Yaw2D(SheetPoint - M->GetActorLocation()));
			M->LookAtPoint(SheetPoint + FVector(0.f, 0.f, 150.f), LureLookSeconds);
			M->PlayGesture(TEXT("LookAround"));
		}
		break;
	case EGuard::Looking:
		if (GuardAge >= LureLookSeconds)
		{
			GuardState = EGuard::Back;
			GuardAge = 0.f;
			M->WalkTo(GuardPost, LureSpeed * 0.7f);
			// The lever re-arms while he walks back (a stagehand's job, off screen): Cole can lure him again.
			bLeverArmed = true;
			LeverAge = CrashAge = -1.f;
			if (LeverHandle) LeverHandle->SetActorTransform(LeverHome);
			if (Sandbag) Sandbag->SetActorTransform(SandbagHome);
			if (Step == EStep::Sneak)
			{
				Objective = TEXT("He's going back. Pull the lever again.");
			}
		}
		break;
	case EGuard::Back:
		if (!M->IsWalking())
		{
			GuardState = EGuard::Post;
			GuardAge = 0.f;
			M->FaceYaw(G.HomeYaw);
		}
		break;
	}
}

bool AScene6Backstage::IsBehind(const FVector& ColeAt) const
{
	if (!G.Body)
	{
		return false;
	}
	FVector To = ColeAt - G.Body->GetActorLocation();
	To.Z = 0.f;
	const FVector Fwd = G.Body->GetActorForwardVector().GetSafeNormal2D();
	return FVector::DotProduct(Fwd, To.GetSafeNormal()) < BehindDot;
}

bool AScene6Backstage::CanSee(const FVector& ColeAt) const
{
	const ACastMember* M = G.Body;
	if (!M || G.bDown)
	{
		return false;
	}
	const FVector At = M->GetActorLocation();
	FVector To = ColeAt - At;
	if (FMath::Abs(To.Z) > 170.f)
	{
		return false;
	}
	To.Z = 0.f;
	const float Dist = static_cast<float>(To.Size());
	if (Dist > G.Range * (bColeCrouched ? CrouchSightScale : 1.f))
	{
		return false;
	}
	if (Dist > 60.f)
	{
		const float Cos = static_cast<float>(FVector::DotProduct(M->GetActorForwardVector().GetSafeNormal2D(), To / Dist));
		if (Cos < FMath::Cos(FMath::DegreesToRadians(G.HalfAngle)))
		{
			return false;
		}
	}
	FCollisionQueryParams Q(SCENE_QUERY_STAT(S6Sight), false);
	Q.AddIgnoredActor(this);
	Q.AddIgnoredActor(M);
	if (const AActor* Cole = GetPovCharacter()) Q.AddIgnoredActor(Cole);
	FHitResult Hit;
	const float HeadZ = bColeCrouched ? 105.f : 150.f;
	return !GetWorld()->LineTraceSingleByChannel(Hit, At + FVector(0.f, 0.f, 165.f), ColeAt + FVector(0.f, 0.f, HeadZ), ECC_Visibility, Q);
}

void AScene6Backstage::StepSight(float Dt)
{
	if (G.bDown || !G.Body)
	{
		return;
	}
	const FVector Cole = ColeLocation();
	const float Dist = static_cast<float>(FVector::Dist2D(Cole, G.Body->GetActorLocation()));
	const float Hear = GuardState == EGuard::Looking ? LuredHearRadius : HearRadius;
	if (CanSee(Cole))
	{
		const float Speed = FMath::Lerp(1.8f, 0.7f, FMath::Clamp(Dist / G.Range, 0.f, 1.f));
		G.Alert += Dt * Speed / FMath::Max(SpotSeconds, 0.1f);
		if (G.Alert >= 1.f)
		{
			Catch(TEXT("SPOTTED BACKSTAGE"));
		}
	}
	else if (ColeMovingUpright() && Dist < Hear)
	{
		// Footsteps: he hears, turns to the sound, and then his eyes do the rest.
		G.Alert += Dt / 0.8f;
		if (G.Alert > 0.35f)
		{
			G.Body->FaceYaw(Yaw2D(Cole - G.Body->GetActorLocation()));
		}
		if (G.Alert >= 1.f)
		{
			Catch(TEXT("HE HEARD YOUR STEPS"));
		}
	}
	else
	{
		G.Alert = FMath::Max(0.f, G.Alert - Dt * 0.6f);
	}
}

void AScene6Backstage::Catch(FName Reason)
{
	if (Step == EStep::Caught || Step == EStep::Done)
	{
		return;
	}
	Step = EStep::Caught;
	CaughtReason = Reason;
	CaughtAge = 0.f;
	G.Alert = 1.f;
	if (ACastMember* M = G.Body)
	{
		M->StopWalking();
		const FVector Cole = ColeLocation();
		const float Yaw = Yaw2D(Cole - M->GetActorLocation());
		M->FaceYaw(Yaw);
		M->SetActorRotation(FRotator(0.f, Yaw, 0.f));
		M->PlayGestureAt(TEXT("Point"), Cole + FVector(0.f, 0.f, 120.f));
	}
	SayCue(G.Name, TEXT("Hey! You!"), TArray<FName>{ TEXT("vo_s6_enforcer_hey") });
	UE_LOG(LogScene6, Log, TEXT("S6 caught: %s"), *Reason.ToString());
}

// ---- poses ---------------------------------------------------------------------------------------------------------

void AScene6Backstage::LegsWalk(FS6Pose& P, const FS6Rig& Rig, const FVector& Feet, const FVector& Fwd, float Phase, float Amp, float Spread) const
{
	const FVector F = Fwd.GetSafeNormal2D();
	const FVector R = RightOf(F);
	for (int32 S = 0; S < 2; ++S)
	{
		const float Side = S == 0 ? -1.f : 1.f;
		const float Swing = S == 0 ? FMath::Sin(Phase) : -FMath::Sin(Phase);
		const float Lift = FMath::Max(0.f, S == 0 ? FMath::Cos(Phase) : -FMath::Cos(Phase));
		P.Ankle[S] = Feet + R * (Side * (Rig.HipHalf + Spread)) + F * (Amp * Swing) + Z * (Rig.AnkleH + 0.4f * Amp * Lift);
		P.Knee[S] = P.Ankle[S] + F * 100.f + Z * 40.f + R * (Side * 15.f);
		P.Foot[S] = FS6Frame::Lean(F, 0.f);
	}
}

FS6Pose AScene6Backstage::ColeCrouchWalk(const FVector& Feet, const FVector& Fwd, float Speed, float Drop) const
{
	const FVector F = Fwd.GetSafeNormal2D();
	const FVector R = RightOf(F);
	FS6Pose P = ColeRig.Stand(Feet, F, Drop);
	const float Amp = 26.f * FMath::Clamp(Speed / 150.f, 0.f, 1.f);
	P.Pelvis += -F * 6.f + Z * (-2.f * FMath::Abs(FMath::Cos(GaitPhase)) * (Amp / 26.f));
	P.Hips = FS6Frame::Lean(F, 14.f);
	P.Chest = FS6Frame::Lean(F, 30.f, 3.f * FMath::Sin(GaitPhase) * (Amp / 26.f));
	P.Head = FS6Frame::Lean(F, 2.f);
	const FVector ChestTop = P.Pelvis + P.Chest.Up.GetSafeNormal() * (ColeRig.ShoulderH - ColeRig.PelvisH);
	for (int32 S = 0; S < 2; ++S)
	{
		const float Side = S == 0 ? -1.f : 1.f;
		const float Swing = (S == 0 ? -1.f : 1.f) * FMath::Sin(GaitPhase) * Amp * 0.4f;
		const FVector Shoulder = ChestTop + R * (Side * ColeRig.ShoulderHalf);
		P.Hand[S] = Shoulder + F * (32.f + Swing) - Z * 42.f + R * (Side * 4.f);
		P.Elbow[S] = Shoulder + R * (Side * 45.f) - F * 10.f - Z * 20.f;
	}
	LegsWalk(P, ColeRig, Feet, F, GaitPhase, Amp, 6.f);
	return P;
}

FS6Pose AScene6Backstage::GuardLying(float Lolled) const
{
	const FVector H = BodyHeadDir.GetSafeNormal2D();
	FS6Pose P;
	P.Pelvis = BodyAt;
	P.Hips = FS6Frame(Z, H);
	P.Chest = FS6Frame(Z, H);
	const FVector Rb = FVector::CrossProduct(P.Hips.Up, P.Hips.Fwd).GetSafeNormal();	// his right
	P.Head = FS6Frame((Z + Rb * Lolled).GetSafeNormal(), H);
	const float Floor = static_cast<float>(BodyAt.Z) - 13.f;
	for (int32 S = 0; S < 2; ++S)
	{
		const float Side = S == 0 ? -1.f : 1.f;
		P.Hand[S] = FVector(BodyAt.X, BodyAt.Y, Floor + 5.f) + Rb * (Side * 32.f) + H * 12.f;
		P.Elbow[S] = P.Hand[S] + Rb * (Side * 40.f) + H * 30.f;
		P.Ankle[S] = FVector(BodyAt.X, BodyAt.Y, Floor + GuardRig.AnkleH) - H * (GuardRig.Leg1 + GuardRig.Leg2 - 6.f)
			+ Rb * (Side * 20.f);
		P.Knee[S] = P.Ankle[S] + Z * 100.f + H * 30.f;
		P.Foot[S] = FS6Frame((Z + Rb * (Side * 0.5f)).GetSafeNormal(), H);
	}
	return P;
}

// ---- the takedown --------------------------------------------------------------------------------------------------

void AScene6Backstage::StartChoke()
{
	ATakeCharacter* Cole = GetPovCharacter();
	ACastMember* M = G.Body;
	SetupRigs();
	if (!Cole || !M || !ColeRig.IsReady() || !GuardRig.IsReady())
	{
		// No rig (no mesh): the old way, he just drops.
		G.bDown = true;
		if (M) { M->StopWalking(); M->SetActorHiddenInGame(true); }
		Step = EStep::ToPanel;
		Objective = TEXT("Vance's way in: out to the backstage corridor and rig the oak door's scanner [E].");
		return;
	}
	M->StopWalking();
	M->PlayGesture(NAME_None);
	G.bDown = true;
	G.Alert = 0.f;
	ChokeAt = Flat(M->GetActorLocation()) + Z * ColeLocation().Z;
	ChokeAt.Z = M->GetActorLocation().Z;
	ChokeFwd = M->GetActorForwardVector().GetSafeNormal2D();
	// Cole: off the crouch, frozen, his rig takes over from the pose he is in.
	const bool bWasCrouched = Cole->IsCrouching();
	FreezeCole(true);
	ColeRig.Attach();
	ColeFrom = Cole->GetMesh()->GetComponentLocation();
	ColeFromFwd = Cole->GetActorForwardVector().GetSafeNormal2D();
	ColeStartPose = bWasCrouched ? ColeCrouchWalk(ColeFrom, ColeFromFwd, 0.f, ColeRig.PelvisH * 0.3f) : ColeRig.Stand(ColeFrom, ColeFromFwd);
	GuardRig.Attach();
	Step = EStep::Choke;
	StepAge = 0.f;
	Objective = TEXT("");
	Play(TakedownCues, &ChokeAt, 0.9f);
	// The side camera: whichever side of him has more room, a little in front, at chest height.
	const FVector R = RightOf(ChokeFwd);
	const FVector Look = ChokeAt + Z * 115.f - ChokeFwd * 20.f;
	FVector Best = Look + R * 270.f + ChokeFwd * 90.f + Z * 25.f;
	float BestRoom = -1.f;
	for (float Side : { 1.f, -1.f })
	{
		const FVector Want = Look + R * (Side * 270.f) + ChokeFwd * 90.f + Z * 25.f;
		FHitResult Hit;
		FCollisionQueryParams Q(SCENE_QUERY_STAT(S6Cam), false);
		Q.AddIgnoredActor(this); Q.AddIgnoredActor(M); Q.AddIgnoredActor(Cole);
		const float Room = GetWorld()->LineTraceSingleByChannel(Hit, Look, Want, ECC_Camera, Q) ? Hit.Distance : 10000.f;
		if (Room > BestRoom) { BestRoom = Room; Best = Room < 10000.f ? Look + (Want - Look).GetSafeNormal() * FMath::Max(140.f, Room - 30.f) : Want; }
	}
	if (StuntCam)
	{
		StuntCam->SetActorLocationAndRotation(Best, (Look - Best).Rotation());
	}
	ViewStunt(true);
	UE_LOG(LogScene6, Log, TEXT("S6 choke at %s"), *ChokeAt.ToString());
}

void AScene6Backstage::StepChoke(float Dt)
{
	ATakeCharacter* Cole = GetPovCharacter();
	const float T = StepAge;
	const FVector F = ChokeFwd;
	const FVector R = RightOf(F);
	const FVector G0 = ChokeAt;
	const FS6Rig& GR = GuardRig;
	const FS6Rig& CR = ColeRig;
	BodyHeadDir = -F;
	BodyAt = G0 - F * 40.f + Z * 13.f;

	// The struggle's strength: full at the grab, fading to nothing as he goes under.
	const float A = T < 0.45f ? 1.f : FMath::Clamp(1.f - (T - 0.45f) / 2.8f, 0.f, 1.f);
	const float W1 = FMath::Sin(T * 9.f), W2 = FMath::Sin(T * 13.f + 1.1f), W3 = FMath::Sin(T * 7.f + 2.3f);

	// ---- the guard ----
	FS6Pose GStand = GR.Stand(G0, F);
	FS6Pose GChoke = GStand;
	GChoke.Pelvis = G0 + Z * (GR.PelvisH - 5.f) - F * (8.f + 5.f * A * W3);
	GChoke.Hips = FS6Frame::Lean(F, -8.f + 4.f * A * W1, 5.f * A * W2);
	GChoke.Chest = FS6Frame::Lean(F, -20.f + 6.f * A * W2, 9.f * A * W3);
	GChoke.Head = FS6Frame::Lean(F, -32.f + 8.f * A * W1, 12.f * A * W2);
	const FVector ColeWrist = CR.IsAttached() ? CR.Bone(TEXT("hand_r")) : G0 + Z * 140.f;
	const FVector ColeForearm = CR.IsAttached() ? CR.Bone(TEXT("lowerarm_r")) : G0 + Z * 140.f;
	for (int32 S = 0; S < 2; ++S)
	{
		const float Side = S == 0 ? -1.f : 1.f;
		const FVector Grip = S == 0 ? FMath::Lerp(ColeWrist, ColeForearm, 0.25f) : FMath::Lerp(ColeWrist, ColeForearm, 0.75f);
		GChoke.Hand[S] = Grip + F * 4.f + Z * (3.f * A * (S == 0 ? W1 : W2));
		GChoke.Elbow[S] = G0 + Z * 80.f + R * (Side * 60.f) + F * 30.f;
		const float Kick = A * FMath::Max(0.f, FMath::Sin(T * 8.f + Side * 1.7f));
		GChoke.Ankle[S] = G0 + R * (Side * (GR.HipHalf + 4.f)) + F * (12.f * A * FMath::Sin(T * 11.f + Side)) + Z * (GR.AnkleH + 16.f * Kick);
		GChoke.Knee[S] = GChoke.Ankle[S] + F * 100.f + Z * 40.f;
	}
	// Sitting slumped on the floor against Cole's legs, arms limp, head down.
	FS6Pose GSit = GChoke;
	GSit.Pelvis = G0 - F * 38.f + Z * 16.f;
	GSit.Hips = FS6Frame::Lean(F, -30.f);
	GSit.Chest = FS6Frame::Lean(F, -38.f, 6.f);
	GSit.Head = Roll(FS6Frame::Lean(F, -10.f), 25.f);
	for (int32 S = 0; S < 2; ++S)
	{
		const float Side = S == 0 ? -1.f : 1.f;
		GSit.Hand[S] = Flat(GSit.Pelvis) + Z * (G0.Z + 6.f) + R * (Side * 30.f) + F * 10.f;
		GSit.Elbow[S] = GSit.Hand[S] + R * (Side * 30.f) - F * 20.f + Z * 20.f;
		GSit.Ankle[S] = G0 + F * 42.f + R * (Side * 18.f) + Z * GR.AnkleH;
		GSit.Knee[S] = GSit.Ankle[S] + Z * 100.f - F * 10.f + R * (Side * 20.f);
		GSit.Foot[S] = FS6Frame::Lean(F, -60.f);
	}
	const FS6Pose GLie = GuardLying(0.8f);

	FS6Pose GP;
	if (T < 0.45f) { GP = FS6Pose::Lerp(GStand, GChoke, Ease((T - 0.08f) / 0.37f)); }
	else if (T < 3.0f) { GP = GChoke; }
	else if (T < 4.0f) { GP = FS6Pose::Lerp(GChoke, GSit, Ease((T - 3.f) / 1.f)); }
	else { GP = FS6Pose::Lerp(GSit, GLie, Ease((T - 4.f) / (FMath::Max(ChokeSeconds, 4.5f) - 4.f))); }
	GuardRig.Apply(GP);

	// ---- Cole ----
	const FVector Neck = GR.Bone(TEXT("neck_02"));
	const FVector Head = GR.Bone(TEXT("head"));
	const FVector C0 = G0 - F * 30.f;
	FS6Pose CChoke = CR.Stand(C0, F);
	CChoke.Pelvis = C0 + Z * (CR.PelvisH - 10.f) - F * (6.f + 3.f * A * W3);
	CChoke.Hips = FS6Frame::Lean(F, -6.f + 2.f * A * W1);
	CChoke.Chest = FS6Frame::Lean(F, -4.f + 3.f * A * W2, -4.f);
	CChoke.Head = Roll(FS6Frame::Lean(F, 18.f), -14.f);
	CChoke.Hand[1] = Neck - R * 12.f + F * 3.f - Z * 5.f;		// his right arm round the throat, hand at the far side
	CChoke.Elbow[1] = Neck + F * 60.f - Z * 10.f;				// the elbow under the chin
	CChoke.Hand[0] = Head - F * 13.f - R * 2.f + Z * 2.f;		// the left hand behind the head
	CChoke.Elbow[0] = C0 + Z * 140.f - R * 70.f - F * 10.f;
	for (int32 S = 0; S < 2; ++S)
	{
		const float Side = S == 0 ? -1.f : 1.f;
		CChoke.Ankle[S] = C0 + R * (Side * (CR.HipHalf + 12.f)) - F * (S == 1 ? 22.f : -4.f) + Z * CR.AnkleH;
		CChoke.Knee[S] = CChoke.Ankle[S] + F * 100.f + Z * 40.f + R * (Side * 15.f);
	}
	// Going down with him: a deep squat, still holding.
	FS6Pose CSquat = CChoke;
	CSquat.Pelvis = G0 - F * 78.f + Z * (CR.PelvisH * 0.55f);
	CSquat.Hips = FS6Frame::Lean(F, 25.f);
	CSquat.Chest = FS6Frame::Lean(F, 38.f);
	CSquat.Head = FS6Frame::Lean(F, 28.f);
	for (int32 S = 0; S < 2; ++S)
	{
		const float Side = S == 0 ? -1.f : 1.f;
		CSquat.Ankle[S] = G0 - F * 70.f + R * (Side * (CR.HipHalf + 18.f)) + Z * CR.AnkleH;
		CSquat.Knee[S] = CSquat.Ankle[S] + F * 100.f + Z * 30.f + R * (Side * 30.f);
	}
	// Up again beside him, looking down at the body.
	const FVector CEndFeet = G0 - F * 95.f + R * 62.f;
	const FVector CEndFwd = (Flat(BodyAt) - Flat(CEndFeet)).GetSafeNormal();
	FS6Pose CEnd = CR.Stand(CEndFeet + Z * (G0.Z - CEndFeet.Z), CEndFwd);
	CEnd.Head = FS6Frame::Lean(CEndFwd, 25.f);

	FS6Pose CP;
	if (T < 0.45f) { CP = FS6Pose::Lerp(ColeStartPose, CChoke, Ease(T / 0.45f)); }
	else if (T < 3.0f) { CP = CChoke; }
	else if (T < 4.0f) { CP = FS6Pose::Lerp(CChoke, CSquat, Ease((T - 3.f) / 1.f)); }
	else if (T < 4.5f) { CP = CSquat; }
	else { CP = FS6Pose::Lerp(CSquat, CEnd, Ease((T - 4.5f) / FMath::Max(ChokeSeconds - 4.5f, 0.2f))); }
	if (T >= 3.0f && T < 4.5f)
	{
		// The hands stay on him while he sinks.
		CP.Hand[1] = Neck - R * 12.f + F * 3.f - Z * 5.f;
		CP.Hand[0] = Head - F * 13.f + Z * 2.f;
	}
	// Cole's capsule follows his feet (the camera and the next beat start where he ends).
	if (Cole)
	{
		const FVector Feet = T < 0.45f ? FMath::Lerp(ColeFrom, C0, Ease(T / 0.45f))
			: T < 4.5f ? C0 : FMath::Lerp(C0, CEndFeet + Z * (G0.Z - CEndFeet.Z), Ease((T - 4.5f) / FMath::Max(ChokeSeconds - 4.5f, 0.2f)));
		const FVector Face = T < 4.5f ? F : CEndFwd;
		const float Half = Cole->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Cole->SetActorLocationAndRotation(FVector(Feet.X, Feet.Y, G0.Z + Half + 1.f), FRotator(0.f, Yaw2D(Face), 0.f),
			false, nullptr, ETeleportType::TeleportPhysics);
	}
	ColeRig.Apply(CP);

	if (T >= 3.1f && T - Dt < 3.1f)
	{
		const FVector At = G0;
		Play(TArray<FName>{ TEXT("sfx_thud") }, &At, 0.5f);
	}
	if (T >= ChokeSeconds)
	{
		Step = EStep::Down;
		StepAge = 0.f;
		ColeRig.Detach();
		SetCrouch(false);
		FreezeCole(false);
		ViewStunt(false);
		if (Cole) { Cole->SetLook(Yaw2D(CEndFwd) + 25.f, -25.f); }
		if (G.Body)
		{
			// The actor sits on his pelvis now (the stunt pose keeps him lying where he fell).
			G.Body->SetActorLocation(FVector(BodyAt.X, BodyAt.Y, G0.Z), false, nullptr, ETeleportType::TeleportPhysics);
		}
		SayCue(TEXT("COLE"), TEXT("Sleep tight."), ColeSleepLine);
		Objective = TEXT("Hide the body: [E] by him to drag him into the costume trunk.");
	}
}

void AScene6Backstage::StepDown(float Dt)
{
	if (GuardRig.IsAttached())
	{
		GuardRig.Apply(GuardLying(0.8f));
	}
}

void AScene6Backstage::StartDrag()
{
	ATakeCharacter* Cole = GetPovCharacter();
	if (!Cole)
	{
		return;
	}
	ColeRig.Detach();
	SetCrouch(false);
	FreezeCole(true);
	ColeRig.Attach();
	Step = EStep::Drag;
	StepAge = 0.f;
	Objective = TEXT("");
	// The path: his pelvis slides headfirst to the trunk's side, stopping short so his shoulders reach it.
	DragPath.Reset();
	const FVector From = Flat(BodyAt);
	const FVector To = Flat(TrunkPoint);
	const FVector Dir = (To - From).GetSafeNormal();
	const FVector Stop = To - Dir * 95.f;
	DragPath.Add(From);
	DragPath.Add(Stop);
	DragLen = static_cast<float>(FVector::Dist(From, Stop));
	GaitPhase = 0.f;
	bLidOpen = true;
	LidAge = 0.f;
	Play(DragCues, &BodyAt, 0.8f);
	// A wide shot from the room's side of the path.
	const FVector Mid = (From + To) * 0.5f + Z * BodyAt.Z;
	FVector Perp = RightOf(Dir);
	if (FVector::DotProduct(Perp, FVector(4860.f, 0.f, 0.f) - Mid) < 0.f) { Perp = -Perp; }
	if (StuntCam)
	{
		const FVector CamAt = Mid + Perp * 420.f + Z * 230.f - Dir * 60.f;
		StuntCam->SetActorLocationAndRotation(CamAt, ((Mid + Z * 40.f) - CamAt).Rotation());
	}
	ViewStunt(true);
}

void AScene6Backstage::StepDrag(float Dt)
{
	ATakeCharacter* Cole = GetPovCharacter();
	if (DragPath.Num() < 2)
	{
		Step = EStep::ToPanel;
		return;
	}
	const float T = StepAge;
	const float Turn = 0.6f;			// he swings the body round to head for the trunk
	const float MoveT = DragLen / FMath::Max(DragSpeed, 10.f);
	const float Heave = 1.1f;			// up and into the trunk
	const FVector Dir = (DragPath[1] - DragPath[0]).GetSafeNormal();
	const float Floor = static_cast<float>(BodyAt.Z) - 13.f;

	float K = 0.f;		// 0..1 along the path
	float Lift = 0.f;	// the torso's lift off the floor, 0..1
	FVector HeadDir = Dir;
	if (T < Turn)
	{
		const float U = Ease(T / Turn);
		HeadDir = FS6Frame::Lerp(FS6Frame(BodyHeadDir, Z), FS6Frame(Dir, Z), U).Fwd.GetSafeNormal2D();
		Lift = U;
	}
	else
	{
		K = FMath::Clamp((T - Turn) / FMath::Max(MoveT, 0.2f), 0.f, 1.f);
		Lift = 1.f;
	}
	const float HeaveK = T > Turn + MoveT ? Ease((T - Turn - MoveT) / Heave) : 0.f;
	FVector Pelvis = FMath::Lerp(DragPath[0], DragPath[1], K) + Z * (Floor + 13.f);

	// ---- the body: back on the floor, his shoulders lifted, heels trailing ----
	const float LiftDeg = 35.f * Lift;
	const FS6Frame Flat0(Z, HeadDir);
	FS6Pose GP;
	GP.Pelvis = Pelvis;
	GP.Hips = Tilt(Flat0, LiftDeg * 0.5f);
	GP.Chest = Tilt(Flat0, LiftDeg);
	GP.Head = Roll(Tilt(GP.Chest, 30.f), 20.f);
	const FVector Rb = FVector::CrossProduct(GP.Hips.Up, GP.Hips.Fwd).GetSafeNormal();
	for (int32 S = 0; S < 2; ++S)
	{
		const float Side = S == 0 ? -1.f : 1.f;
		GP.Hand[S] = Flat(Pelvis) + Z * (Floor + 6.f) + Rb * (Side * 30.f) + HeadDir * 22.f;
		GP.Elbow[S] = GP.Hand[S] + Rb * (Side * 40.f) + Z * 10.f;
		GP.Ankle[S] = Flat(Pelvis) + Z * (Floor + GuardRig.AnkleH) - HeadDir * (GuardRig.Leg1 + GuardRig.Leg2 - 8.f) + Rb * (Side * 16.f);
		GP.Knee[S] = GP.Ankle[S] + Z * 100.f + HeadDir * 40.f;
		GP.Foot[S] = FS6Frame((Z + Rb * (Side * 0.4f)).GetSafeNormal(), HeadDir);
	}
	if (HeaveK > 0.f)
	{
		// Over the rim: lifted, swung into the trunk, folded.
		FS6Pose In = GP;
		const FVector Into = Flat(TrunkPoint) + Z * (Floor + 30.f);
		In.Pelvis = Into + Z * (55.f * FMath::Sin(UE_PI * HeaveK));
		In.Hips = FS6Frame(Z, HeadDir);
		In.Chest = Tilt(FS6Frame(Z, HeadDir), 50.f);
		In.Head = Roll(Tilt(In.Chest, 40.f), 25.f);
		for (int32 S = 0; S < 2; ++S)
		{
			const float Side = S == 0 ? -1.f : 1.f;
			In.Ankle[S] = Into - HeadDir * 30.f + Z * 45.f + Rb * (Side * 14.f);
			In.Knee[S] = In.Ankle[S] + Z * 100.f;
			In.Hand[S] = Into + Rb * (Side * 25.f) + HeadDir * 10.f;
		}
		GP = FS6Pose::Lerp(GP, In, HeaveK);
	}
	GuardRig.Apply(GP);
	if (G.Body)
	{
		G.Body->SetActorLocation(FVector(GP.Pelvis.X, GP.Pelvis.Y, Floor), false, nullptr, ETeleportType::TeleportPhysics);
	}

	// ---- Cole: bent over him, hands under his arms, walking backwards ----
	const FVector ShL = GuardRig.Bone(TEXT("upperarm_l")), ShR = GuardRig.Bone(TEXT("upperarm_r"));
	const FVector ShMid = (ShL + ShR) * 0.5f;
	const FVector Face = -HeadDir;
	FVector CFeet = Flat(ShMid) + HeadDir * 52.f + Z * Floor;
	if (HeaveK > 0.f)
	{
		CFeet = Flat(DragPath[1]) + HeadDir * 70.f + Z * Floor;
	}
	FS6Pose CP = ColeRig.Stand(CFeet, Face, 18.f + 10.f * (1.f - HeaveK));
	CP.Hips = FS6Frame::Lean(Face, 22.f - 12.f * HeaveK);
	CP.Chest = FS6Frame::Lean(Face, 52.f - 25.f * HeaveK);
	CP.Head = FS6Frame::Lean(Face, 30.f);
	CP.Pelvis += HeadDir * 10.f;
	const FVector Cr = RightOf(Face);
	for (int32 S = 0; S < 2; ++S)
	{
		const float Side = S == 0 ? -1.f : 1.f;
		// They face each other but his right is the guard's right (the guard lies face up): left hand, left armpit.
		const FVector Pit = (S == 0 ? ShL : ShR) - Z * 6.f - HeadDir * 6.f;
		CP.Hand[S] = Pit;
		CP.Elbow[S] = CFeet + Z * 110.f + Cr * (Side * 70.f);
	}
	const float Moved = static_cast<float>(FVector::Dist2D(CFeet, LastColeFeet));
	LastColeFeet = CFeet;
	if (T > Turn) { GaitPhase -= FMath::Min(Moved, 40.f) / 40.f * UE_PI; }
	LegsWalk(CP, ColeRig, CFeet, Face, GaitPhase, (T > Turn && HeaveK <= 0.f) ? 20.f : 0.f, 10.f);
	if (Cole)
	{
		const float Half = Cole->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Cole->SetActorLocationAndRotation(FVector(CFeet.X, CFeet.Y, Floor + Half + 1.f), FRotator(0.f, Yaw2D(Face), 0.f),
			false, nullptr, ETeleportType::TeleportPhysics);
	}
	ColeRig.Apply(CP);

	if (T >= Turn + MoveT + Heave)
	{
		GuardRig.Detach();
		if (G.Body) { G.Body->SetActorHiddenInGame(true); }
		bLidOpen = false;
		LidAge = 0.f;
		Play(TArray<FName>{ TEXT("sfx_s6_trunk"), TEXT("sfx_thud") }, &TrunkPoint, 0.8f);
		ColeRig.Detach();
		FreezeCole(false);
		ViewStunt(false);
		Step = EStep::ToPanel;
		StepAge = 0.f;
		RouteIndex = 0;
		if (Cole) { Cole->SetLook(Yaw2D(ObjectivePoint() - Cole->GetActorLocation()), -12.f); }
		Objective = TEXT("Vance's way in: out to the backstage corridor and rig the oak door's scanner [E].");
	}
}

// ---- the door rig and the exit -------------------------------------------------------------------------------------

void AScene6Backstage::StepRig(float Dt)
{
	const float T = StepAge;
	const float Prev = T - Dt;
	ATakeCharacter* Cole = GetPovCharacter();
	auto At = [&](float S) { return T >= S && Prev < S; };
	if (At(0.05f))
	{
		PanelAge = 0.f;
		if (Cole) { Cole->PlayGestureAt(TEXT("Use"), PanelPoint); }
	}
	if (At(0.6f) || At(1.3f))
	{
		Play(SparkCues, &PanelPoint, 0.9f);
		if (Cole) { Cole->PlayGestureAt(TEXT("Use"), PanelPoint + FVector(0.f, 0.f, -15.f)); }
	}
	// Sparks while the wires are bridged.
	const bool bSparking = T > 0.6f && T < 1.9f;
	SparkLight->SetIntensity(bSparking && FMath::FRand() < 0.55f ? FMath::FRandRange(300.f, 1400.f) : 0.f);
	if (At(1.9f))
	{
		LockLamp->SetLightColor(FLinearColor(0.1f, 1.f, 0.2f));
		Play(UnlockCues, &PanelPoint, 1.f);
	}
	if (At(2.4f))
	{
		// The insert: Vance's door in the hall, its scanner lamp going green.
		for (TActorIterator<AScene5Auditorium> It(GetWorld()); It; ++It) { It->SetDoorUnlocked(true); }
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			if (InsertCam) { PC->SetViewTarget(InsertCam); }
		}
		SetFlag(TEXT("S6DoorRigged"));
		SayCue(TEXT("COLE"), TEXT("Scanner's rigged. Vance, the door's yours."), ColeRiggedLine);
	}
	if (At(4.6f))
	{
		ViewStunt(false);
		FreezeCole(false);
		if (Cole) { Cole->SetLook(Yaw2D(ExitStand - Cole->GetActorLocation()), -10.f); }
		Step = EStep::ToExit;
		StepAge = 0.f;
		Objective = TEXT("Now the way out: wedge the prop store door open, west down the corridor [E].");
	}
}

void AScene6Backstage::StepWedge(float Dt)
{
	const float T = StepAge;
	const float Prev = T - Dt;
	ATakeCharacter* Cole = GetPovCharacter();
	auto At = [&](float S) { return T >= S && Prev < S; };
	if (At(0.05f) && Cole)
	{
		Cole->PlayGestureAt(TEXT("Use"), ExitAlarmPoint);
	}
	if (At(0.6f))
	{
		AlarmLamp->SetIntensity(0.f);
		Play(AlarmOffCues, &ExitAlarmPoint, 0.9f);
		Flash(TEXT("ALARM OFF"), 1.2f);
	}
	if (At(1.2f))
	{
		ExitAge = 0.f;
		if (Cole) { Cole->PlayGesture(TEXT("Lift")); }
	}
	if (At(2.0f))
	{
		if (ExitWedge) { ExitWedge->SetActorHiddenInGame(false); }
		Play(WedgeCues, &ExitStand, 0.9f);
	}
	if (ExitWedge && T > 2.0f)
	{
		const float K = Ease((T - 2.f) / 0.25f);
		ExitWedge->SetActorLocation(WedgeHome.GetLocation() + FVector(0.f, 0.f, 25.f * (1.f - K)));
	}
	if (At(2.4f))
	{
		SetFlag(TEXT("S6ExitReady"));
		SayCue(TEXT("COLE"), TEXT("Way out's wedged. Max, bring the car round back."), ColeExitLine);
	}
	if (At(3.2f))
	{
		Step = EStep::Done;
		StepAge = 1.4f;	// Done finishes at 3.2 s
	}
}

// ---- use -----------------------------------------------------------------------------------------------------------

bool AScene6Backstage::TryUse()
{
	const FVector Cole = ColeLocation();
	auto Near = [&](const FVector& At, float Reach)
	{
		return FVector::Dist2D(Cole, At) <= Reach && FMath::Abs(Cole.Z - At.Z) < 150.f;
	};
	if (Step == EStep::Sneak)
	{
		if (!G.bDown && G.Body && Near(G.Body->GetActorLocation(), TakedownReach))
		{
			if (!IsBehind(Cole))
			{
				Catch(TEXT("FACE TO FACE"));
			}
			else if (G.Alert >= 0.7f)
			{
				Catch(TEXT("HE WAS READY FOR YOU"));
			}
			else
			{
				StartChoke();
			}
			return true;
		}
		if (Near(LeverPoint, UseReach))
		{
			if (!bLeverArmed || GuardState == EGuard::ToLook || GuardState == EGuard::Looking)
			{
				Flash(TEXT("Not yet - he's still looking."), 1.5f);
				return true;
			}
			bLeverArmed = false;
			LeverAge = 0.f;
			Play(LeverCues, &LeverPoint, 0.9f);
			if (ATakeCharacter* C = GetPovCharacter()) { C->PlayGestureAt(TEXT("Use"), LeverPoint + FVector(40.f, 0.f, 150.f)); }
			return true;
		}
		return false;
	}
	if (Step == EStep::Down)
	{
		if (Near(BodyAt, 200.f))
		{
			StartDrag();
			return true;
		}
		return false;
	}
	if (Step == EStep::ToPanel && Near(PanelStand, UseReach))
	{
		Step = EStep::Rigging;
		StepAge = 0.f;
		SetCrouch(false);
		FreezeCole(true);
		if (ATakeCharacter* C = GetPovCharacter()) { C->SetActorRotation(FRotator(0.f, Yaw2D(PanelPoint - C->GetActorLocation()), 0.f)); }
		Objective = TEXT("");
		return true;
	}
	if (Step == EStep::ToExit && Near(ExitStand, UseReach))
	{
		Step = EStep::Wedging;
		StepAge = 0.f;
		SetCrouch(false);
		FreezeCole(true);
		if (ATakeCharacter* C = GetPovCharacter()) { C->SetActorRotation(FRotator(0.f, Yaw2D(ExitAlarmPoint - C->GetActorLocation()), 0.f)); }
		Objective = TEXT("");
		return true;
	}
	return false;
}

bool AScene6Backstage::HandleSceneKey(const FKey& Key, bool bPressed)
{
	const bool bScripted = Step == EStep::Caught || Step == EStep::Done || Step == EStep::Choke || Step == EStep::Drag
		|| Step == EStep::Rigging || Step == EStep::Wedging;
	const bool bUse = Key == EKeys::E || Key == EKeys::LeftMouseButton;
	if (bScripted)
	{
		return bPressed && bUse;
	}
	if (bPressed && bUse)
	{
		return TryUse();
	}
	return false;
}

// ---- props ---------------------------------------------------------------------------------------------------------

void AScene6Backstage::TurnAboutEdge(AActor* Actor, const FTransform& Home, const FVector& HingeLocal, const FVector& AxisLocal, float Degrees)
{
	if (!Actor)
	{
		return;
	}
	const FVector Scaled = HingeLocal * Home.GetScale3D();
	const FQuat HomeQ = Home.GetRotation();
	const FVector Hinge = Home.GetLocation() + HomeQ.RotateVector(Scaled);
	const FQuat Q = HomeQ * FQuat(AxisLocal.GetSafeNormal(), FMath::DegreesToRadians(Degrees));
	Actor->SetActorLocationAndRotation(Hinge - Q.RotateVector(Scaled), Q, false, nullptr, ETeleportType::TeleportPhysics);
}

void AScene6Backstage::StepProps(float Dt)
{
	// The trunk lid: opens as the drag starts, shuts when he's in.
	if (LidAge >= 0.f)
	{
		LidAge += Dt;
		const float Open = bLidOpen ? Ease(LidAge / 0.4f) : 1.f - Ease(LidAge / 0.3f);
		TurnAboutEdge(TrunkLid, LidHome, FVector(0.f, 50.f, -50.f), FVector(1.f, 0.f, 0.f), LidOpenDegrees * Open);
	}
	// The lever: thrown, then the sandbag drops 0.45 s later onto the sheet.
	if (LeverAge >= 0.f)
	{
		LeverAge += Dt;
		TurnAboutEdge(LeverHandle, LeverHome, FVector(0.f, 0.f, -50.f), FVector(0.f, 1.f, 0.f), LeverPullDegrees * Ease(LeverAge / 0.25f));
		if (LeverAge >= 0.45f && CrashAge < 0.f && Sandbag)
		{
			const float Fall = FMath::Min(1.f, (LeverAge - 0.45f) / 0.4f);
			FVector L = SandbagHome.GetLocation();
			L.Z -= SandbagDrop * Fall * Fall;
			Sandbag->SetActorLocation(L);
			if (Fall >= 1.f) { CrashAge = 0.f; }
		}
		else if (LeverAge >= 0.85f && CrashAge < 0.f && !Sandbag)
		{
			CrashAge = 0.f;
		}
	}
	if (CrashAge >= 0.f)
	{
		const bool bFirst = CrashAge == 0.f;
		CrashAge += Dt;
		if (bFirst)
		{
			Play(CrashCues, &SheetPoint, 1.f);
			Flash(TEXT("CRASH!"), 1.2f);
			if (Step == EStep::Sneak && !G.bDown)
			{
				Lure();
			}
		}
		if (Sheet)
		{
			const float Shake = FMath::Exp(-2.5f * CrashAge) * FMath::Sin(CrashAge * 48.f);
			const FQuat Q = SheetHome.GetRotation() * FQuat(FVector(0.f, 1.f, 0.f), FMath::DegreesToRadians(6.f * Shake));
			Sheet->SetActorLocationAndRotation(SheetHome.GetLocation() + SheetHome.GetRotation().RotateVector(FVector(8.f * Shake, 0.f, 0.f)), Q);
		}
	}
	// The control box's cover swings open on its left edge.
	if (PanelAge >= 0.f)
	{
		PanelAge += Dt;
		TurnAboutEdge(PanelCover, PanelHome, FVector(-50.f, 0.f, 0.f), FVector(0.f, 0.f, 1.f), PanelOpenDegrees * Ease(PanelAge / 0.35f));
	}
	// The prop store door swings wide open.
	if (ExitAge >= 0.f)
	{
		ExitAge += Dt;
		TurnAboutEdge(ExitLeaf, ExitHome, FVector(-50.f, 0.f, 0.f), FVector(0.f, 0.f, 1.f), ExitOpenDegrees * Ease(ExitAge / 0.8f));
	}
}

// ---- tick ----------------------------------------------------------------------------------------------------------

void AScene6Backstage::TickScene(float DeltaSeconds)
{
	const float Dt = FMath::Min(DeltaSeconds, 0.1f);
	StepAge += Dt;
	if (const ATakeCharacter* Cole = GetPovCharacter()) { bColeCrouched = Cole->IsCrouching(); }
	ToastLeft = FMath::Max(0.f, ToastLeft - Dt);
	StepFilm(Dt);
	StepProps(Dt);
	switch (Step)
	{
	case EStep::Caught:
		CaughtAge += Dt;
		if (CaughtAge >= 1.0f)
		{
			const FName Reason = CaughtReason;
			CaughtAge = -100.f;
			ColeRig.Detach();
			FailScene(Reason);
		}
		return;
	case EStep::Done:
		if (StepAge >= 3.2f)
		{
			StepAge = -1000.f;
			// Off camera (the cut to Vance): Cole back on s7-route's mark by the oak door, where part 0 picks him up.
			TestPlaceCole(EndMark, EndYaw);
			FinishScene();
		}
		return;
	case EStep::Choke:
		StepChoke(Dt);
		return;
	case EStep::Drag:
		StepDrag(Dt);
		return;
	case EStep::Rigging:
		StepRig(Dt);
		return;
	case EStep::Wedging:
		StepWedge(Dt);
		return;
	case EStep::Down:
		StepDown(Dt);
			return;
	case EStep::Sneak:
		StepGuard(Dt);
		StepSight(Dt);
			return;
	case EStep::ToPanel:
		// The marker walks the route: the next waypoint once he's near this one.
		if (RouteToExit.IsValidIndex(RouteIndex) && FVector::Dist2D(ColeLocation(), RouteToExit[RouteIndex]) < 260.f)
		{
			++RouteIndex;
		}
		return;
	default:	// ToExit
		return;
	}
}

// ---- sound ---------------------------------------------------------------------------------------------------------

USoundBase* AScene6Backstage::FindCue(const TArray<FName>& Cues)
{
	for (const FName& N : Cues)
	{
		if (TObjectPtr<USoundBase>* Hit = CueCache.Find(N))
		{
			return *Hit;
		}
		if (CueMissing.Contains(N))
		{
			continue;
		}
		const FString S = N.ToString();
		for (const FString& Dir : { FString(TEXT("/Game/Take/Audio/Script/S6/")), FString(TEXT("/Game/Take/Audio/")) })
		{
			const FString Path = Dir + S + TEXT(".") + S;
			if (USoundBase* Snd = LoadObject<USoundBase>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet))
			{
				CueCache.Add(N, Snd);
				return Snd;
			}
		}
		CueMissing.Add(N);
	}
	return nullptr;
}

void AScene6Backstage::Play(const TArray<FName>& Cues, const FVector* At, float Volume)
{
	if (USoundBase* Snd = FindCue(Cues))
	{
		if (At)
		{
			UGameplayStatics::PlaySoundAtLocation(this, Snd, *At, Volume);
		}
		else
		{
			UGameplayStatics::PlaySound2D(this, Snd, Volume);
		}
	}
}

void AScene6Backstage::SayCue(const FString& Speaker, const FString& Line, const TArray<FName>& Cues)
{
	Say(Speaker, Line, FindCue(Cues));
}

// ---- HUD -----------------------------------------------------------------------------------------------------------

FVector AScene6Backstage::ObjectivePoint() const
{
	switch (Step)
	{
	case EStep::Sneak:
		return (GuardState == EGuard::Post || GuardState == EGuard::Back) && bLeverArmed ? LeverPoint + FVector(0.f, 0.f, 150.f)
			: (G.Body ? G.Body->GetActorLocation() + FVector(0.f, 0.f, 120.f) : FVector::ZeroVector);
	case EStep::Down: return BodyAt + FVector(0.f, 0.f, 60.f);
	case EStep::ToPanel: return RouteToExit.IsValidIndex(RouteIndex) ? RouteToExit[RouteIndex] + FVector(0.f, 0.f, 120.f) : PanelPoint;
	case EStep::ToExit: return ExitAlarmPoint;
	default: return FVector::ZeroVector;
	}
}

void AScene6Backstage::DrawSceneHud(UCanvas* Canvas)
{
	if (!Canvas || !GEngine)
	{
		return;
	}
	UFont* Font = GEngine->GetMediumFont();	// the large engine font draws boxes here
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const float S = FMath::Max(0.6f, H / 1080.f);
	auto Text = [&](const FString& Str, float X, float Y, const FLinearColor& C, float Scale, bool bCentre)
	{
		FCanvasTextItem It(FVector2D(X, Y), FText::FromString(Str), Font, C);
		It.Scale = FVector2D(Scale, Scale);
		It.bCentreX = bCentre;
		It.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f));
		Canvas->DrawItem(It);
	};
	auto Box = [&](float X, float Y, float BW, float BH, const FLinearColor& C)
	{
		FCanvasTileItem T(FVector2D(X, Y), FVector2D(BW, BH), C);
		T.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(T);
	};
	const FLinearColor Amber(1.f, 0.62f, 0.12f), Grey(0.75f, 0.75f, 0.75f), Red(1.f, 0.18f, 0.12f), Green(0.4f, 1.f, 0.5f);
	const bool bPlayable = Step == EStep::Sneak || Step == EStep::Down || Step == EStep::ToPanel || Step == EStep::ToExit;
	const float X0 = 48.f * S, Y0 = 130.f * S;

	if (!Objective.IsEmpty())
	{
		Text(Objective, X0, Y0, FLinearColor(0.92f, 0.92f, 0.92f), 1.35f * S, false);
	}
	if (bPlayable)
	{
		// The stance: crouched is silent.
		const FString Stance = bColeCrouched ? TEXT("CROUCHED - silent   [C] stand") : TEXT("STANDING - he can hear your steps   [C] crouch");
		Text(Stance, X0, Y0 + 30.f * S, bColeCrouched ? Green : Amber, 1.2f * S, false);
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	// Who's onto him: ? then ! over the head.
	if (PC && G.Body && !G.bDown && G.Alert >= 0.04f)
	{
		FVector2D Scr;
		if (PC->ProjectWorldLocationToScreen(G.Body->GetActorLocation() + FVector(0.f, 0.f, 215.f), Scr, true))
		{
			const bool bBang = G.Alert >= 0.6f;
			Text(bBang ? TEXT("!") : TEXT("?"), Scr.X, Scr.Y - 20.f * S, bBang ? Red : FLinearColor(1.f, 0.85f, 0.2f), (2.2f + 2.4f * G.Alert) * S, true);
		}
	}
	// Where to go: a small marker on the next thing to use (clamped to the screen's edge when it's off screen).
	if (PC && bPlayable)
	{
		const FVector P = ObjectivePoint();
		FVector2D Scr;
		if (!P.IsZero() && PC->ProjectWorldLocationToScreen(P, Scr, true))
		{
			const float M = 40.f * S;
			const bool bOff = Scr.X < M || Scr.X > W - M || Scr.Y < M || Scr.Y > H - M;
			Scr.X = FMath::Clamp(Scr.X, M, W - M);
			Scr.Y = FMath::Clamp(Scr.Y, M, H - M);
			const float D = 14.f * S;
			Box(Scr.X - D * 0.5f, Scr.Y - D * 0.5f, D, D, FLinearColor(1.f, 0.85f, 0.3f, bOff ? 0.5f : 0.9f));
			const float Metres = static_cast<float>(FVector::Dist2D(P, ColeLocation())) / 100.f;
			Text(FString::Printf(TEXT("%.0f m"), Metres), Scr.X, Scr.Y + 10.f * S, FLinearColor(1.f, 0.9f, 0.6f, 0.9f), 1.0f * S, true);
		}
	}

	// The prompt (bottom centre).
	FString Prompt;
	FLinearColor PromptC = FLinearColor::White;
	if (bPlayable)
	{
		const FVector Cole = ColeLocation();
		auto Near = [&](const FVector& At, float Reach) { return FVector::Dist2D(Cole, At) <= Reach && FMath::Abs(Cole.Z - At.Z) < 150.f; };
		if (Step == EStep::Sneak && !G.bDown && G.Body && Near(G.Body->GetActorLocation(), TakedownReach + 60.f))
		{
			if (!IsBehind(Cole)) { Prompt = TEXT("He's facing you"); PromptC = Red; }
			else { Prompt = TEXT("[E]  Choke-hold from behind"); PromptC = Amber; }
		}
		else if (Step == EStep::Sneak && Near(LeverPoint, UseReach))
		{
			const bool bReady = bLeverArmed && GuardState != EGuard::ToLook && GuardState != EGuard::Looking;
			Prompt = bReady ? TEXT("[E]  Throw the counterweight lever") : TEXT("Counterweight lever - he's still looking");
			PromptC = bReady ? FLinearColor::White : Grey;
		}
		else if (Step == EStep::Down && Near(BodyAt, 200.f))
		{
			Prompt = TEXT("[E]  Drag him into the costume trunk");
		}
		else if (Step == EStep::ToPanel && Near(PanelStand, UseReach))
		{
			Prompt = TEXT("[E]  Rig the door scanner");
		}
		else if (Step == EStep::ToExit && Near(ExitStand, UseReach))
		{
			Prompt = TEXT("[E]  Kill the alarm, wedge the door open");
		}
	}
	if (!Prompt.IsEmpty())
	{
		Text(Prompt, W * 0.5f, H * 0.74f, PromptC, 2.0f * S, true);
	}
	if (ToastLeft > 0.f && !Toast.IsEmpty())
	{
		const float Alpha = FMath::Clamp(ToastLeft / 0.4f, 0.f, 1.f);
		Text(Toast, W * 0.5f, H * 0.36f, FLinearColor(1.f, 0.9f, 0.7f, Alpha), 2.4f * S, true);
	}
}
