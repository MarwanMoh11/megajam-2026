// See Scene6Backstage.h. Owner: the s6-backstage session.
#include "Script/Scene6Backstage.h"

#include "CastMember.h"
#include "TakeCharacter.h"
#include "GuardBrain.h"
#include "Script/Scene5Auditorium.h"
#include "Script/ScriptHudWidgets.h"
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
	const FName S6TagLid(TEXT("s6_trunk_lid"));
	const FName S6TagLever(TEXT("s6_lever_handle"));
	const FName S6TagSandbag(TEXT("s6_sandbag"));
	const FName S6TagSheet(TEXT("s6_sheet"));
	const FName S6TagPanel(TEXT("s6_panel_cover"));
	const FName S6TagExit(TEXT("s6_exit_leaf"));
	const FName S6TagWedge(TEXT("s6_exit_wedge"));
	const FVector S6Z(0.f, 0.f, 1.f);

	float S6Ease(float T) { T = FMath::Clamp(T, 0.f, 1.f); return T * T * (3.f - 2.f * T); }
	float S6Yaw2D(const FVector& V) { return static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(V.Y, V.X))); }
	FVector S6Flat(const FVector& V) { return FVector(V.X, V.Y, 0.f); }
	FVector S6RightOf(const FVector& Fwd) { return FVector::CrossProduct(S6Z, Fwd).GetSafeNormal(); }
	/** Tilts a frame forward (its top toward its front) by Deg. */
	FS6Frame S6Tilt(const FS6Frame& F, float Deg)
	{
		const float R = FMath::DegreesToRadians(Deg);
		const FVector Fw = F.Fwd.GetSafeNormal(), Up = F.Up.GetSafeNormal();
		return FS6Frame(Fw * FMath::Cos(R) - Up * FMath::Sin(R), Up * FMath::Cos(R) + Fw * FMath::Sin(R));
	}
	/** Rolls a frame about its front by Deg. */
	FS6Frame S6Roll(const FS6Frame& F, float Deg)
	{
		const FVector Fw = F.Fwd.GetSafeNormal();
		return FS6Frame(Fw, FQuat(Fw, FMath::DegreesToRadians(Deg)).RotateVector(F.Up));
	}
	/** Smooth 0..1 across [A, B]. The stunts chain their key poses with overlapping windows, so a move starts before
	 *  the last one has settled (no stop at every key). */
	float S6Win(float T, float A, float B) { return S6Ease((T - A) / FMath::Max(B - A, 0.001f)); }
	/** A soft bump: 1 at C, falling off over about W either side. */
	float S6Bump(float T, float C, float W) { const float X = (T - C) / FMath::Max(W, 0.001f); return FMath::Exp(-X * X); }
	/** Turns a heading about the vertical by Deg. */
	FVector S6Yawed(const FVector& Fwd, float Deg) { return FQuat(S6Z, FMath::DegreesToRadians(Deg)).RotateVector(Fwd); }
	/** One planted foot of a stunt: where it lands and the window it travels in. */
	struct FS6Plant { float T0, T1; FVector At; };
	/** A foot that steps between plants (lifted mid-step, by less for a short shuffle) instead of skating. */
	FVector S6Foot(const FVector& Start, const FS6Plant* Plants, int32 Num, float T, float Lift)
	{
		FVector P = Start;
		for (int32 I = 0; I < Num; ++I)
		{
			const FS6Plant& S = Plants[I];
			if (T <= S.T0) { return P; }
			if (T < S.T1)
			{
				const float U = (T - S.T0) / FMath::Max(S.T1 - S.T0, 0.001f);
				const float L = Lift * FMath::Min(1.f, static_cast<float>(FVector::Dist2D(P, S.At)) / 30.f);
				return FMath::Lerp(P, S.At, S6Ease(U)) + S6Z * (L * FMath::Sin(UE_PI * U));
			}
			P = S.At;
		}
		return P;
	}
	/** A point on the cubic Bezier P[0..3] at U. */
	FVector S6Bez(const TArray<FVector>& P, float U)
	{
		const float V = 1.f - U;
		return P[0] * (V * V * V) + P[1] * (3.f * V * V * U) + P[2] * (3.f * V * U * U) + P[3] * (U * U * U);
	}
	/** The point Dist along the Bezier P[0..3] (flat distance, sampled); with OutLen, its whole length instead. */
	FVector S6BezAlong(const TArray<FVector>& P, float Dist, float* OutLen = nullptr)
	{
		const int32 N = 48;
		FVector Prev = P[0];
		float Acc = 0.f;
		for (int32 I = 1; I <= N; ++I)
		{
			const FVector Cur = S6Bez(P, I / static_cast<float>(N));
			const float Seg = static_cast<float>(FVector::Dist2D(Prev, Cur));
			if (!OutLen && Acc + Seg >= Dist && Seg > KINDA_SMALL_NUMBER)
			{
				return FMath::Lerp(Prev, Cur, (Dist - Acc) / Seg);
			}
			Acc += Seg;
			Prev = Cur;
		}
		if (OutLen) { *OutLen = Acc; }
		return Prev;
	}
	/** The costume trunk stands square against a wall: Out is its way out to the room, Along its long axis, pointing
	 *  to From's side. */
	void S6TrunkAxes(const FVector& Trunk, const FVector& From, FVector& Out, FVector& Along)
	{
		const FVector ToRoom = S6Flat(FVector(4860.f, 0.f, 0.f) - Trunk);
		Out = FMath::Abs(ToRoom.X) >= FMath::Abs(ToRoom.Y) ? FVector(FMath::Sign(ToRoom.X), 0.f, 0.f) : FVector(0.f, FMath::Sign(ToRoom.Y), 0.f);
		Along = S6RightOf(Out);
		if (FVector::DotProduct(Along, S6Flat(From - Trunk)) < 0.f) { Along = -Along; }
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
		S->TestPlaceCole(At, S6Yaw2D(Look - At));
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

// ---- the guard brain (stealth-ai) ---------------------------------------------------------------------------------
// While Cole sneaks and the enforcer is at his post, UGuardBrainComponent is his eyes and ears: a "?" meter that fills
// before he is made (slower in the dark, crouched, far off), head and body turns, barks, and walking over to check
// footsteps. The scene keeps its lure (the brain is off while he walks to the crash and back), the takedown, the hide
// and the catch (the brain's Spotted calls Catch). Reel.S6.GuardBrain 0 = the scene's own cone and hearing, as before.
static TAutoConsoleVariable<int32> CVarS6GuardBrain(TEXT("Reel.S6.GuardBrain"), 1,
	TEXT("1: Scene 6's enforcer senses with the guard brain (meter, turns, investigates steps). 0: the scene's own cone."));

static UGuardBrainComponent* S6GuardBrain(const AActor* Body)
{
	return Body ? Body->FindComponentByClass<UGuardBrainComponent>() : nullptr;
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
		M->bArmed = true;	// SM_Pistol in his right hand (chars-locomotion)
#if WITH_EDITOR
		M->SetActorLabel(TEXT("S6_Enforcer"));
#endif
		M->FinishSpawning(Xf);
	}
	G.Body = M;
	if (M)
	{
		UGuardBrainComponent* Brain = UGuardBrainComponent::AddTo(M);
		Brain->GuardName = G.Name;
		Brain->SightRange = G.Range;
		Brain->InnerHalfAngle = G.HalfAngle;
		Brain->PeripheralHalfAngle = G.HalfAngle + 15.f;
		Brain->GlanceDegrees = ScanDegrees;
		Brain->GlanceSeconds = FMath::Max(ScanSeconds, 0.5f);
		Brain->bChaseWhenAlerted = false;	// the scene catches him (Catch: "Hey! You!", gun up)
		Brain->bWorldBarks = false;			// subtitles through the scene's Say
		Brain->NoticeGesture = NAME_None;	// he is armed: no hand gestures
		Brain->SearchGesture = NAME_None;
		Brain->AlertGesture = NAME_None;
		TArray<FGuardStop> Stops;
		FGuardStop Stop;
		Stop.Location = GuardPost;
		Stop.WaitSeconds = 1000.f;
		Stop.bFaceYaw = true;
		Stop.FaceYaw = GuardYaw;
		Stops.Add(Stop);
		Brain->SetPatrol(Stops);
		Brain->SetBrainActive(false);
		Brain->OnEvent.AddWeakLambda(this, [this](UGuardBrainComponent*, EGuardEvent E, AActor*)
		{
			if (E == EGuardEvent::Spotted && Step == EStep::Sneak)
			{
				Catch(TEXT("SPOTTED BACKSTAGE"));
			}
		});
		Brain->OnBark.AddWeakLambda(this, [this](UGuardBrainComponent*, EGuardEvent, const FString& Line)
		{
			if (Step == EStep::Sneak)
			{
				Say(G.Name, Line, nullptr);
			}
		});
	}
	UE_LOG(LogScene6, Log, TEXT("S6 cast: guard %s"), M ? TEXT("ok") : TEXT("none"));
}

void AScene6Backstage::FindProps()
{
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Act = *It;
		if (Act->ActorHasTag(S6TagLid)) { TrunkLid = Act; LidHome = Act->GetActorTransform(); }
		else if (Act->ActorHasTag(S6TagLever)) { LeverHandle = Act; LeverHome = Act->GetActorTransform(); }
		else if (Act->ActorHasTag(S6TagSandbag)) { Sandbag = Act; SandbagHome = Act->GetActorTransform(); }
		else if (Act->ActorHasTag(S6TagSheet)) { Sheet = Act; SheetHome = Act->GetActorTransform(); }
		else if (Act->ActorHasTag(S6TagPanel)) { PanelCover = Act; PanelHome = Act->GetActorTransform(); }
		else if (Act->ActorHasTag(S6TagExit)) { ExitLeaf = Act; ExitHome = Act->GetActorTransform(); }
		else if (Act->ActorHasTag(S6TagWedge)) { ExitWedge = Act; WedgeHome = Act->GetActorTransform(); }
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
		if (UGuardBrainComponent* Brain = S6GuardBrain(M))
		{
			Brain->SetBrainActive(false);
			Brain->ResetBrain();
		}
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
	if (G.Body) { G.Body->StopAiming(); }
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
	if (UGuardBrainComponent* Brain = S6GuardBrain(G.Body)) { Brain->SetBrainActive(false); }
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
	if (UGuardBrainComponent* Brain = S6GuardBrain(M))
	{
		Brain->SetBrainActive(false);
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
			M->FaceYaw(S6Yaw2D(SheetPoint - M->GetActorLocation()));
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
			G.Body->FaceYaw(S6Yaw2D(Cole - G.Body->GetActorLocation()));
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
	if (UGuardBrainComponent* Brain = S6GuardBrain(G.Body)) { Brain->SetBrainActive(false); }	// Catch aims, not the brain
	CaughtReason = Reason;
	CaughtAge = 0.f;
	G.Alert = 1.f;
	if (ACastMember* M = G.Body)
	{
		M->StopWalking();
		const FVector Cole = ColeLocation();
		const float Yaw = S6Yaw2D(Cole - M->GetActorLocation());
		M->FaceYaw(Yaw);
		M->SetActorRotation(FRotator(0.f, Yaw, 0.f));
		M->AimAt(Cole + FVector(0.f, 0.f, 120.f));	// gun on him
	}
	SayCue(G.Name, TEXT("Hey! You!"), TArray<FName>{ TEXT("vo_s6_enforcer_hey") });
	UE_LOG(LogScene6, Log, TEXT("S6 caught: %s"), *Reason.ToString());
}

// ---- poses ---------------------------------------------------------------------------------------------------------

void AScene6Backstage::LegsWalk(FS6Pose& P, const FS6Rig& Rig, const FVector& Feet, const FVector& Fwd, float Phase, float Amp, float Spread) const
{
	const FVector F = Fwd.GetSafeNormal2D();
	const FVector R = S6RightOf(F);
	for (int32 S = 0; S < 2; ++S)
	{
		const float Side = S == 0 ? -1.f : 1.f;
		const float Swing = S == 0 ? FMath::Sin(Phase) : -FMath::Sin(Phase);
		const float Lift = FMath::Max(0.f, S == 0 ? FMath::Cos(Phase) : -FMath::Cos(Phase));
		P.Ankle[S] = Feet + R * (Side * (Rig.HipHalf + Spread)) + F * (Amp * Swing) + S6Z * (Rig.AnkleH + 0.4f * Amp * Lift);
		P.Knee[S] = P.Ankle[S] + F * 100.f + S6Z * 40.f + R * (Side * 15.f);
		P.Foot[S] = FS6Frame::Lean(F, 0.f);
	}
}

FS6Pose AScene6Backstage::ColeCrouchWalk(const FVector& Feet, const FVector& Fwd, float Speed, float Drop) const
{
	const FVector F = Fwd.GetSafeNormal2D();
	const FVector R = S6RightOf(F);
	FS6Pose P = ColeRig.Stand(Feet, F, Drop);
	const float Amp = 26.f * FMath::Clamp(Speed / 150.f, 0.f, 1.f);
	P.Pelvis += -F * 6.f + S6Z * (-2.f * FMath::Abs(FMath::Cos(GaitPhase)) * (Amp / 26.f));
	P.Hips = FS6Frame::Lean(F, 14.f);
	P.Chest = FS6Frame::Lean(F, 30.f, 3.f * FMath::Sin(GaitPhase) * (Amp / 26.f));
	P.Head = FS6Frame::Lean(F, 2.f);
	const FVector ChestTop = P.Pelvis + P.Chest.Up.GetSafeNormal() * (ColeRig.ShoulderH - ColeRig.PelvisH);
	for (int32 S = 0; S < 2; ++S)
	{
		const float Side = S == 0 ? -1.f : 1.f;
		const float Swing = (S == 0 ? -1.f : 1.f) * FMath::Sin(GaitPhase) * Amp * 0.4f;
		const FVector Shoulder = ChestTop + R * (Side * ColeRig.ShoulderHalf);
		P.Hand[S] = Shoulder + F * (32.f + Swing) - S6Z * 42.f + R * (Side * 4.f);
		P.Elbow[S] = Shoulder + R * (Side * 45.f) - F * 10.f - S6Z * 20.f;
	}
	LegsWalk(P, ColeRig, Feet, F, GaitPhase, Amp, 6.f);
	return P;
}

FS6Pose AScene6Backstage::GuardLying(float Lolled) const
{
	const FVector H = BodyHeadDir.GetSafeNormal2D();
	FS6Pose P;
	P.Pelvis = BodyAt;
	P.Hips = FS6Frame(S6Z, H);
	P.Chest = FS6Frame(S6Z, H);
	const FVector Rb = FVector::CrossProduct(P.Hips.Up, P.Hips.Fwd).GetSafeNormal();	// his right
	P.Head = FS6Frame((S6Z + Rb * Lolled).GetSafeNormal(), H);
	const float Floor = static_cast<float>(BodyAt.Z) - 13.f;
	for (int32 S = 0; S < 2; ++S)
	{
		const float Side = S == 0 ? -1.f : 1.f;
		P.Hand[S] = FVector(BodyAt.X, BodyAt.Y, Floor + 5.f) + Rb * (Side * 32.f) + H * 12.f;
		P.Elbow[S] = P.Hand[S] + Rb * (Side * 40.f) + H * 30.f;
		P.Ankle[S] = FVector(BodyAt.X, BodyAt.Y, Floor + GuardRig.AnkleH) - H * (GuardRig.Leg1 + GuardRig.Leg2 - 6.f)
			+ Rb * (Side * 20.f);
		P.Knee[S] = P.Ankle[S] + S6Z * 100.f + H * 30.f;
		P.Foot[S] = FS6Frame((S6Z + Rb * (Side * 0.5f)).GetSafeNormal(), H);
	}
	return P;
}

// ---- the takedown --------------------------------------------------------------------------------------------------

void AScene6Backstage::StartChoke()
{
	ATakeCharacter* Cole = GetPovCharacter();
	ACastMember* M = G.Body;
	if (UGuardBrainComponent* Brain = S6GuardBrain(M)) { Brain->SetBrainActive(false); }	// the rig owns him now
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
	ChokeAt = S6Flat(M->GetActorLocation()) + S6Z * ColeLocation().Z;
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
	const FVector R = S6RightOf(ChokeFwd);
	const FVector Look = ChokeAt + S6Z * 115.f - ChokeFwd * 20.f;
	FVector Best = Look + R * 270.f + ChokeFwd * 90.f + S6Z * 25.f;
	float BestRoom = -1.f;
	for (float Side : { 1.f, -1.f })
	{
		const FVector Want = Look + R * (Side * 270.f) + ChokeFwd * 90.f + S6Z * 25.f;
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
	StepChoke(0.f);	// the first frame posed now: no flash of the reference pose
}

void AScene6Backstage::StepChoke(float Dt)
{
	ATakeCharacter* Cole = GetPovCharacter();
	// The beats are written for 5.2 s; another ChokeSeconds stretches them. Key poses chain through overlapping
	// windows (S6Win), so each move starts before the last has settled, and the feet step between plants.
	const float Scale = 5.2f / FMath::Max(ChokeSeconds, 1.f);
	const float T = StepAge * Scale;
	const float Tp = (StepAge - Dt) * Scale;
	const FVector F = ChokeFwd;
	const FVector R = S6RightOf(F);
	const FVector G0 = ChokeAt;
	const FS6Rig& GR = GuardRig;
	const FS6Rig& CR = ColeRig;
	BodyHeadDir = -F;
	BodyAt = G0 - F * 40.f + S6Z * 13.f;

	// The fight in him: up as he's grabbed, in bursts (he fights, tires, fights again), gone by 3 s.
	const float A = S6Win(T, 0.1f, 0.45f) * (1.f - S6Win(T, 1.7f, 3.0f));
	const float Burst = FMath::Min(1.3f, 0.5f + 0.5f * (S6Bump(T, 0.7f, 0.22f) + S6Bump(T, 1.45f, 0.2f) + S6Bump(T, 2.15f, 0.25f)));
	const float AB = A * Burst;
	// Noise from unrelated frequencies: it never settles into a wobble.
	const float N1 = 0.6f * FMath::Sin(T * 9.1f) + 0.4f * FMath::Sin(T * 15.7f + 0.4f);
	const float N2 = 0.6f * FMath::Sin(T * 7.3f + 1.1f) + 0.4f * FMath::Sin(T * 12.9f + 2.0f);
	const float N3 = 0.6f * FMath::Sin(T * 5.9f + 2.3f) + 0.4f * FMath::Sin(T * 17.3f + 0.7f);
	const float Pull = S6Win(T, 0.5f, 0.9f);	// Cole's step back, taking him off his balance

	// ---- the guard ----
	const FS6Pose GStand = GR.Stand(G0, F);
	// Grabbed and fighting it: dragged back onto his heels, both hands clawing at the arm round his throat, twisting,
	// kicking.
	FS6Pose GFight = GStand;
	{
		const FVector Twist = S6Yawed(F, 10.f * AB * N3);
		GFight.Pelvis = G0 + S6Z * (GR.PelvisH - 4.f + 3.f * AB * N2) - F * (10.f + 12.f * Pull + 6.f * AB * N1) + R * (5.f * AB * N2);
		GFight.Hips = FS6Frame::Lean(Twist, -9.f + 4.f * AB * N1, 6.f * AB * N2);
		GFight.Chest = FS6Frame::Lean(S6Yawed(Twist, 8.f * AB * N1), -22.f - 4.f * Pull + 7.f * AB * N2, 10.f * AB * N3);
		GFight.Head = FS6Frame::Lean(F, -30.f + 8.f * AB * N1, 12.f * AB * N2);
		const FVector ColeWrist = CR.IsAttached() ? CR.Bone(TEXT("hand_r")) : G0 + S6Z * 140.f;
		const FVector ColeForearm = CR.IsAttached() ? CR.Bone(TEXT("lowerarm_r")) : G0 + S6Z * 140.f;
		for (int32 S = 0; S < 2; ++S)
		{
			const float Side = S == 0 ? -1.f : 1.f;
			const FVector Grip = FMath::Lerp(ColeWrist, ColeForearm, S == 0 ? 0.3f : 0.75f);
			GFight.Hand[S] = Grip + F * 4.f - S6Z * (2.f + 4.f * AB * FMath::Max(0.f, S == 0 ? N1 : N2));
			GFight.Elbow[S] = G0 + S6Z * 80.f + R * (Side * 60.f) + F * 30.f;
			const float Kick = AB * FMath::Square(FMath::Max(0.f, FMath::Sin(T * 7.7f + Side * 1.9f)));
			GFight.Ankle[S] = G0 + R * (Side * (GR.HipHalf + 4.f)) - F * (6.f * Pull) + F * (16.f * Kick) + S6Z * (GR.AnkleH + 14.f * Kick);
			GFight.Knee[S] = GFight.Ankle[S] + F * 100.f + S6Z * 40.f;
			GFight.Foot[S] = FS6Frame::Lean(F, -12.f * Pull - 25.f * Kick);	// up on his heels, toes kicking
		}
	}
	// Going under: the knees give, the head drops onto Cole's arm, the arms hang.
	FS6Pose GLimp = GFight;
	{
		GLimp.Pelvis = G0 - F * 26.f + S6Z * (GR.PelvisH - 30.f);
		GLimp.Hips = FS6Frame::Lean(F, -4.f);
		GLimp.Chest = FS6Frame::Lean(F, -24.f, 5.f);
		GLimp.Head = S6Roll(FS6Frame::Lean(F, 22.f), 20.f);
		const FVector ChestTop = GLimp.Pelvis + GLimp.Chest.Up.GetSafeNormal() * (GR.ShoulderH - GR.PelvisH);
		for (int32 S = 0; S < 2; ++S)
		{
			const float Side = S == 0 ? -1.f : 1.f;
			const FVector Shoulder = ChestTop + R * (Side * GR.ShoulderHalf);
			GLimp.Hand[S] = Shoulder - S6Z * (GR.Arm1 + GR.Arm2 - 8.f) + F * 8.f + R * (Side * 6.f);
			GLimp.Elbow[S] = Shoulder - F * 30.f + R * (Side * 15.f) - S6Z * 20.f;
			GLimp.Ankle[S] = G0 + R * (Side * (GR.HipHalf + 6.f)) - F * 4.f + S6Z * GR.AnkleH;
			GLimp.Knee[S] = GLimp.Ankle[S] + F * 100.f + S6Z * 20.f + R * (Side * 10.f);
			GLimp.Foot[S] = FS6Frame::Lean(F, 0.f);
		}
	}
	// Down on the floor, slumped back against Cole, legs out.
	FS6Pose GSit = GLimp;
	GSit.Pelvis = G0 - F * 38.f + S6Z * 16.f;
	GSit.Hips = FS6Frame::Lean(F, -30.f);
	GSit.Chest = FS6Frame::Lean(F, -40.f, 6.f);
	GSit.Head = S6Roll(FS6Frame::Lean(F, 5.f), 28.f);
	for (int32 S = 0; S < 2; ++S)
	{
		const float Side = S == 0 ? -1.f : 1.f;
		GSit.Hand[S] = S6Flat(GSit.Pelvis) + S6Z * (G0.Z + 6.f) + R * (Side * 30.f) + F * 10.f;
		GSit.Elbow[S] = GSit.Hand[S] + R * (Side * 30.f) - F * 20.f + S6Z * 20.f;
		GSit.Ankle[S] = G0 + F * 42.f + R * (Side * 18.f) + S6Z * GR.AnkleH;
		GSit.Knee[S] = GSit.Ankle[S] + S6Z * 100.f - F * 10.f + R * (Side * 20.f);
		GSit.Foot[S] = FS6Frame::Lean(F, -60.f);
	}
	const FS6Pose GLie = GuardLying(0.8f);

	FS6Pose GP = FS6Pose::Lerp(GStand, GFight, S6Win(T, 0.12f, 0.45f));
	{
		// The grip slips first, then the knees go.
		const FS6Pose Before = GP;
		GP = FS6Pose::Lerp(GP, GLimp, S6Win(T, 2.6f, 3.4f));
		const float WHands = S6Win(T, 2.3f, 3.0f);
		for (int32 S = 0; S < 2; ++S)
		{
			GP.Hand[S] = FMath::Lerp(Before.Hand[S], GLimp.Hand[S], WHands);
			GP.Elbow[S] = FMath::Lerp(Before.Elbow[S], GLimp.Elbow[S], WHands);
		}
	}
	GP = FS6Pose::Lerp(GP, GSit, S6Win(T, 3.2f, 4.1f));
	GP = FS6Pose::Lerp(GP, GLie, S6Win(T, 3.95f, 4.85f));
	GuardRig.Apply(GP);

	// ---- Cole ----
	const FVector Neck = GR.Bone(TEXT("neck_02"));
	const FVector Head = GR.Bone(TEXT("head"));
	const FVector Chest = GR.Bone(TEXT("spine_04"));
	// His places: behind him for the hold, a step back as he pulls him over, down behind him, round to his right
	// side to lay him back (his head on Cole's left), up beside him.
	const FVector C0 = G0 - F * 30.f, C1 = G0 - F * 44.f, C2 = G0 - F * 74.f;
	const FVector Fl = (-R + F * 0.3f).GetSafeNormal();
	const FVector Rl = S6RightOf(Fl);
	const FVector C3 = G0 - F * 76.f + R * 52.f;
	const FVector CEndFeet = G0 - F * 92.f + R * 82.f;
	const FVector CEndFwd = (S6Flat(BodyAt - F * 30.f) - S6Flat(CEndFeet)).GetSafeNormal();
	auto Holding = [&](FS6Pose& P)
	{
		P.Hand[1] = Neck - R * 12.f + F * 3.f - S6Z * 5.f;		// his right arm round the throat, hand at the far side
		P.Elbow[1] = Neck + F * 60.f - S6Z * 10.f;				// the elbow under the chin
		P.Hand[0] = Head - F * 13.f - R * 2.f + S6Z * 2.f;		// the left hand behind the head
		P.Elbow[0] = Head - R * 70.f - F * 10.f - S6Z * 10.f;
	};
	const FVector CMid = FMath::Lerp(C0, C1, Pull);
	FS6Pose CHold = CR.Stand(CMid, F);
	CHold.Pelvis = CMid + S6Z * (CR.PelvisH - 10.f + 2.f * AB * N3) - F * (6.f + 3.f * AB * N1) + R * (3.f * AB * N2);
	CHold.Hips = FS6Frame::Lean(F, -6.f - 6.f * Pull + 2.f * AB * N1);
	CHold.Chest = FS6Frame::Lean(S6Yawed(F, -5.f * AB * N3), -4.f - 6.f * Pull + 3.f * AB * N2, -4.f - 4.f * AB * N1);
	CHold.Head = S6Roll(FS6Frame::Lean(F, 18.f), -14.f);
	Holding(CHold);
	// Going down with him, still holding.
	FS6Pose CSquat = CR.Stand(C2, F);
	CSquat.Pelvis = C2 + S6Z * (CR.PelvisH * 0.52f);
	CSquat.Hips = FS6Frame::Lean(F, 28.f);
	CSquat.Chest = FS6Frame::Lean(F, 34.f);
	CSquat.Head = FS6Frame::Lean(F, 30.f);
	Holding(CSquat);
	// Low at his side, laying him back: the left hand cradles the head down, the right on his chest.
	FS6Pose CLay = CR.Stand(C3, Fl);
	CLay.Pelvis = C3 + S6Z * (CR.PelvisH * 0.42f) - Fl * 6.f;
	CLay.Hips = FS6Frame::Lean(Fl, 32.f);
	CLay.Chest = FS6Frame::Lean(Fl, 58.f);
	CLay.Head = FS6Frame::Lean(Fl, 40.f);
	CLay.Hand[0] = Head - S6Z * 4.f - F * 4.f;
	CLay.Elbow[0] = C3 - F * 60.f + S6Z * 60.f;
	CLay.Hand[1] = Chest + S6Z * 12.f;
	CLay.Elbow[1] = C3 + F * 60.f + S6Z * 60.f;
	// Up beside him, looking down at the body.
	FS6Pose CEnd = CR.Stand(CEndFeet, CEndFwd);
	CEnd.Chest = FS6Frame::Lean(CEndFwd, 6.f);
	CEnd.Head = FS6Frame::Lean(CEndFwd, 25.f);

	FS6Pose CP = FS6Pose::Lerp(ColeStartPose, CHold, S6Win(T, 0.f, 0.42f));
	CP = FS6Pose::Lerp(CP, CSquat, S6Win(T, 3.0f, 4.0f));
	CP = FS6Pose::Lerp(CP, CLay, S6Win(T, 3.85f, 4.6f));
	CP = FS6Pose::Lerp(CP, CEnd, S6Win(T, 4.65f, 5.2f));
	// The feet step from place to place (right foot back first on the pull).
	{
		const float Sp = CR.HipHalf;
		const FVector Ah = S6Z * CR.AnkleH;
		const FVector Re = S6RightOf(CEndFwd);
		const FS6Plant PL[] = {
			{ 0.00f, 0.30f, C0 - R * (Sp + 12.f) + F * 4.f + Ah },
			{ 0.62f, 0.86f, C1 - R * (Sp + 14.f) + F * 2.f + Ah },
			{ 3.05f, 3.45f, C2 - R * (Sp + 20.f) + Ah },
			{ 3.95f, 4.30f, C3 - Rl * (Sp + 16.f) + Ah },
			{ 4.85f, 5.15f, CEndFeet - Re * (Sp + 2.f) + Ah } };
		const FS6Plant PR[] = {
			{ 0.10f, 0.42f, C0 + R * (Sp + 12.f) - F * 22.f + Ah },
			{ 0.50f, 0.72f, C1 + R * (Sp + 12.f) - F * 26.f + Ah },
			{ 3.15f, 3.55f, C2 + R * (Sp + 20.f) + Ah },
			{ 4.10f, 4.45f, C3 + Rl * (Sp + 16.f) + Ah },
			{ 4.75f, 5.05f, CEndFeet + Re * (Sp + 2.f) + Ah } };
		CP.Ankle[0] = S6Foot(ColeStartPose.Ankle[0], PL, UE_ARRAY_COUNT(PL), T, 12.f);
		CP.Ankle[1] = S6Foot(ColeStartPose.Ankle[1], PR, UE_ARRAY_COUNT(PR), T, 12.f);
	}
	FVector Face = CP.Hips.Fwd.GetSafeNormal2D();
	if (Face.IsNearlyZero()) { Face = F; }
	const FVector RN = S6RightOf(Face);
	for (int32 S = 0; S < 2; ++S)
	{
		const float Side = S == 0 ? -1.f : 1.f;
		CP.Knee[S] = CP.Ankle[S] + Face * 100.f + S6Z * 30.f + RN * (Side * 25.f);
	}
	// Cole's capsule follows his feet (the camera and the next beat start where he ends).
	if (Cole)
	{
		const FVector Feet = (CP.Ankle[0] + CP.Ankle[1]) * 0.5f;
		const float Half = Cole->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Cole->SetActorLocationAndRotation(FVector(Feet.X, Feet.Y, G0.Z + Half + 1.f), FRotator(0.f, S6Yaw2D(Face), 0.f),
			false, nullptr, ETeleportType::TeleportPhysics);
	}
	ColeRig.Apply(CP);

	if (T >= 3.95f && Tp < 3.95f)
	{
		const FVector At = G0;
		Play(TArray<FName>{ TEXT("sfx_thud") }, &At, 0.5f);
	}
	if (StepAge >= ChokeSeconds)
	{
		Step = EStep::Down;
		StepAge = 0.f;
		ColeRig.Detach();
		SetCrouch(false);
		FreezeCole(false);
		ViewStunt(false);
		if (Cole) { Cole->SetLook(S6Yaw2D(CEndFwd) + 25.f, -25.f); }
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
		// Out cold, still breathing.
		FS6Pose P = GuardLying(0.8f);
		const float B = FMath::Sin(StepAge * 1.7f);
		P.Chest = S6Tilt(P.Chest, 2.f * B);
		P.Head = S6Tilt(P.Head, 1.f * B);
		GuardRig.Apply(P);
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
	// Where he starts from: he steps over to the head end and bends to take him under the arms.
	ColeFrom = Cole->GetMesh()->GetComponentLocation();
	ColeFromFwd = Cole->GetActorForwardVector().GetSafeNormal2D();
	ColeStartPose = ColeRig.Stand(ColeFrom, ColeFromFwd);
	// The path his shoulders take (Cole walks backwards with them, the body trails like a sled): out along the body,
	// round, and in alongside the trunk on the room side, so the heave goes over the trunk's long rim.
	FVector Out, Along;
	S6TrunkAxes(TrunkPoint, BodyAt, Out, Along);
	const FVector H0 = BodyHeadDir.GetSafeNormal2D();
	const FVector L0 = S6Flat(BodyAt) + H0 * (GuardRig.ShoulderH - GuardRig.PelvisH);
	const FVector LEnd = S6Flat(TrunkPoint) + Out * 72.f - Along * 18.f;
	DragPath.Reset();
	DragPath.Add(L0);
	DragPath.Add(L0 + H0 * 70.f);
	DragPath.Add(LEnd + Along * 100.f);
	DragPath.Add(LEnd);
	S6BezAlong(DragPath, 0.f, &DragLen);
	GaitPhase = 0.f;
	LastColeFeet = ColeFrom;
	bLidOpen = true;
	LidAge = 0.f;
	Play(DragCues, &BodyAt, 0.8f);
	// A wide shot from the room's side, framing the body where it lies and the trunk.
	const FVector Focus = (S6Flat(BodyAt) + LEnd) * 0.5f + S6Z * (BodyAt.Z + 30.f);
	FVector Perp = S6RightOf((LEnd - S6Flat(BodyAt)).GetSafeNormal2D());
	if (Perp.IsNearlyZero()) { Perp = Out; }
	if (FVector::DotProduct(Perp, FVector(4860.f, 0.f, 0.f) - Focus) < 0.f) { Perp = -Perp; }
	const float Span = static_cast<float>(FVector::Dist2D(BodyAt, LEnd));
	FVector CamAt = Focus + Perp * FMath::Clamp(Span * 0.9f + 150.f, 380.f, 520.f) + S6Z * 240.f;
	{
		FHitResult Hit;
		FCollisionQueryParams Q(SCENE_QUERY_STAT(S6DragCam), false);
		Q.AddIgnoredActor(this); Q.AddIgnoredActor(Cole);
		if (G.Body) { Q.AddIgnoredActor(G.Body); }
		const FVector Eye = Focus + S6Z * 120.f;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Eye, CamAt, ECC_Camera, Q))
		{
			CamAt = Eye + (CamAt - Eye).GetSafeNormal() * FMath::Max(150.f, Hit.Distance - 30.f);
		}
	}
	if (StuntCam)
	{
		StuntCam->SetActorLocationAndRotation(CamAt, ((Focus - S6Z * 10.f) - CamAt).Rotation());
	}
	ViewStunt(true);
	StepDrag(0.f);
}

void AScene6Backstage::StepDrag(float Dt)
{
	ATakeCharacter* Cole = GetPovCharacter();
	if (DragPath.Num() < 4)
	{
		Step = EStep::ToPanel;
		return;
	}
	const float T = StepAge;
	const FS6Rig& GR = GuardRig;
	const float Floor = static_cast<float>(BodyAt.Z) - 13.f;
	const FVector Tr = S6Flat(TrunkPoint) + S6Z * Floor;
	FVector Out, Along;
	S6TrunkAxes(TrunkPoint, DragPath[0], Out, Along);
	const float Sp = ColeRig.HipHalf;
	const FVector Ah = S6Z * ColeRig.AnkleH;

	// The beats: the reach (he steps to the head end and takes him under the arms), the lift, the drag (the
	// shoulders along the path, the body trailing), the heave (the torso over the rim, the hips, the legs last).
	const float Lsp = GR.ShoulderH - GR.PelvisH;
	const FVector GrabSpot = DragPath[0] + BodyHeadDir.GetSafeNormal2D() * 52.f;
	const float Reach = FMath::Clamp(static_cast<float>(FVector::Dist2D(ColeFrom, GrabSpot)) / 170.f, 0.6f, 1.2f);
	const float Move0 = Reach + 0.25f;
	const float MoveT = FMath::Max(1.25f * DragLen / FMath::Max(DragSpeed, 10.f), 0.4f);
	const float Heave0 = Move0 + MoveT;
	const float HeaveT = 1.6f;
	const float Lift = S6Win(T, Reach - 0.05f, Reach + 0.45f);
	const float Gait = S6Win(T, Move0, Move0 + 0.35f) * (1.f - S6Win(T, Heave0 - 0.35f, Heave0));

	// The sled: the shoulders go where Cole pulls them, the pelvis trails at its length behind.
	if (T > Move0)
	{
		const FVector L = S6BezAlong(DragPath, DragLen * S6Ease((T - Move0) / MoveT));
		const FVector D = (L - S6Flat(BodyAt)).GetSafeNormal2D();
		if (!D.IsNearlyZero())
		{
			BodyHeadDir = D;
			const FVector P = L - D * Lsp;
			BodyAt = FVector(P.X, P.Y, BodyAt.Z);
		}
	}
	const FVector H = BodyHeadDir.GetSafeNormal2D();

	// ---- the body: shoulders up in Cole's hands, head lolling back, arms and heels trailing ----
	const float Bob = Gait * FMath::Sin(2.f * GaitPhase);	// twice a stride
	const FVector Pelvis = FVector(BodyAt.X, BodyAt.Y, Floor + 13.f);
	const FS6Frame Flat0(S6Z, H);
	FS6Pose GDrag;
	GDrag.Pelvis = Pelvis + S6Z * (2.f + 1.5f * Bob);
	GDrag.Hips = S6Tilt(Flat0, 17.f);
	GDrag.Chest = S6Tilt(Flat0, 35.f + 3.f * Bob);
	GDrag.Head = S6Roll(S6Tilt(GDrag.Chest, -22.f + 6.f * Bob), 18.f + 7.f * Gait * FMath::Sin(GaitPhase));
	const FVector Rb = FVector::CrossProduct(GDrag.Hips.Up, GDrag.Hips.Fwd).GetSafeNormal();
	for (int32 S = 0; S < 2; ++S)
	{
		const float Side = S == 0 ? -1.f : 1.f;
		GDrag.Hand[S] = S6Flat(Pelvis) + S6Z * (Floor + 6.f) + Rb * (Side * 32.f) + H * (18.f - 5.f * Gait * FMath::Sin(GaitPhase + Side));
		GDrag.Elbow[S] = GDrag.Hand[S] + Rb * (Side * 40.f) + S6Z * 10.f;
		GDrag.Ankle[S] = S6Flat(Pelvis) + S6Z * (Floor + GR.AnkleH) - H * (GR.Leg1 + GR.Leg2 - 8.f)
			+ Rb * (Side * 16.f + 3.f * Gait * FMath::Sin(GaitPhase));
		GDrag.Knee[S] = GDrag.Ankle[S] + S6Z * 100.f + H * 40.f;
		GDrag.Foot[S] = FS6Frame((S6Z + Rb * (Side * 0.5f)).GetSafeNormal(), H);
	}
	FS6Pose GP = FS6Pose::Lerp(GuardLying(0.8f), GDrag, Lift);
	if (T > Heave0)
	{
		// In the trunk: on his back along it, head at the far end, knees up under the lid.
		FS6Pose GIn;
		const FS6Frame InFlat(S6Z, -Along);
		GIn.Pelvis = Tr + Along * 22.f + S6Z * 20.f;
		GIn.Hips = S6Tilt(InFlat, 10.f);
		GIn.Chest = S6Tilt(InFlat, 22.f);
		GIn.Head = S6Roll(S6Tilt(GIn.Chest, 20.f), 30.f);
		const FVector Ri = FVector::CrossProduct(GIn.Hips.Up, GIn.Hips.Fwd).GetSafeNormal();
		for (int32 S = 0; S < 2; ++S)
		{
			const float Side = S == 0 ? -1.f : 1.f;
			GIn.Hand[S] = GIn.Pelvis + Ri * (Side * 22.f) - Along * 10.f + S6Z * 5.f;
			GIn.Elbow[S] = GIn.Hand[S] + Ri * (Side * 30.f) + S6Z * 10.f;
			GIn.Ankle[S] = Tr + Along * 58.f + Ri * (Side * 12.f) + S6Z * (10.f + GR.AnkleH);
			GIn.Knee[S] = GIn.Ankle[S] + S6Z * 100.f - Along * 20.f + Ri * (Side * 10.f);
			GIn.Foot[S] = FS6Frame::Lean(Along, 0.f);
		}
		const float U = T - Heave0;
		const float WT = S6Win(U, 0.1f, 0.85f);		// the torso over the rim first (he sits up in Cole's arms)
		const float WP = S6Win(U, 0.25f, 1.1f);		// then the hips
		const float WL = S6Win(U, 0.6f, 1.45f);		// the legs last, swung in
		const FS6Pose From = GP;
		GP.Pelvis = FMath::Lerp(From.Pelvis, GIn.Pelvis, WP) + S6Z * (60.f * FMath::Sin(UE_PI * WP));
		GP.Hips = FS6Frame::Lerp(From.Hips, GIn.Hips, WP);
		GP.Chest = S6Tilt(FS6Frame::Lerp(From.Chest, GIn.Chest, WT), 40.f * FMath::Sin(UE_PI * WT));
		GP.Head = S6Tilt(FS6Frame::Lerp(From.Head, GIn.Head, WT), 30.f * FMath::Sin(UE_PI * WT));
		for (int32 S = 0; S < 2; ++S)
		{
			GP.Hand[S] = FMath::Lerp(From.Hand[S], GIn.Hand[S], WT) + S6Z * (40.f * FMath::Sin(UE_PI * WT));
			GP.Elbow[S] = FMath::Lerp(From.Elbow[S], GIn.Elbow[S], WT);
			GP.Ankle[S] = FMath::Lerp(From.Ankle[S], GIn.Ankle[S], WL) + S6Z * (62.f * FMath::Sin(UE_PI * WL));
			GP.Knee[S] = FMath::Lerp(From.Knee[S], GIn.Knee[S], WL);
			GP.Foot[S] = FS6Frame::Lerp(From.Foot[S], GIn.Foot[S], WL);
		}
	}
	GuardRig.Apply(GP);
	if (G.Body)
	{
		G.Body->SetActorLocation(FVector(GP.Pelvis.X, GP.Pelvis.Y, Floor), false, nullptr, ETeleportType::TeleportPhysics);
	}

	// ---- Cole: bent over him, hands under his arms, walking backwards ----
	const FVector ShL = GR.Bone(TEXT("upperarm_l")), ShR = GR.Bone(TEXT("upperarm_r"));
	const FVector ShMid = (ShL + ShR) * 0.5f;
	const FVector Face = -H;
	const FVector Cr = S6RightOf(Face);
	const FVector CFeet = S6Flat(ShMid) + H * 52.f + S6Z * Floor;
	FS6Pose CDrag = ColeRig.Stand(CFeet, Face, FMath::Lerp(40.f, 28.f, Lift) + 2.f * FMath::Abs(Bob));
	CDrag.Hips = FS6Frame::Lean(Face, FMath::Lerp(30.f, 22.f, Lift));
	CDrag.Chest = FS6Frame::Lean(Face, FMath::Lerp(62.f, 50.f, Lift) + 4.f * Bob);
	CDrag.Head = FS6Frame::Lean(Face, 28.f);
	CDrag.Pelvis += H * 10.f;
	for (int32 S = 0; S < 2; ++S)
	{
		const float Side = S == 0 ? -1.f : 1.f;
		// They face each other but his right is the guard's right (the guard lies face up): left hand, left armpit.
		CDrag.Hand[S] = (S == 0 ? ShL : ShR) - S6Z * 6.f - H * 6.f;
		CDrag.Elbow[S] = CFeet + S6Z * 110.f + Cr * (Side * 70.f);
	}
	FS6Pose CP = FS6Pose::Lerp(ColeStartPose, CDrag, S6Win(T, 0.1f * Reach, Reach));
	if (T <= Reach)
	{
		// Two steps over to the head end.
		const FS6Plant PL[] = { { 0.f, 0.55f * Reach, GrabSpot + S6Z * Floor - Cr * (Sp + 10.f) + Ah } };
		const FS6Plant PR[] = { { 0.35f * Reach, Reach, GrabSpot + S6Z * Floor + Cr * (Sp + 10.f) + Ah } };
		CP.Ankle[0] = S6Foot(ColeStartPose.Ankle[0], PL, 1, T, 12.f);
		CP.Ankle[1] = S6Foot(ColeStartPose.Ankle[1], PR, 1, T, 12.f);
		LastColeFeet = CFeet;
	}
	else if (T <= Heave0)
	{
		// Backwards, the stride measured by the ground he covers, so the feet don't skate.
		const float Moved = static_cast<float>(FVector::Dist2D(CFeet, LastColeFeet));
		LastColeFeet = CFeet;
		GaitPhase -= FMath::Min(Moved, 40.f) / 40.f * UE_PI;
		LegsWalk(CP, ColeRig, CFeet, Face, GaitPhase, 20.f * Gait, 10.f);
	}
	else
	{
		// The heave: round to the trunk's side, the torso up and over in both arms, then his hands to the knees to tip
		// the legs in, and up.
		const float U = T - Heave0;
		const FVector FaceIn = -Out;
		const FVector Ci = S6RightOf(FaceIn);
		const FVector Stance1 = Tr + Out * 112.f - Along * 20.f;
		const FVector Stance2 = Tr + Out * 76.f + Along * 20.f;
		const FVector Stance = FMath::Lerp(Stance1, Stance2, S6Win(U, 0.5f, 0.95f));
		const float Rise = S6Win(U, 0.95f, 1.6f);
		FS6Pose CH = ColeRig.Stand(Stance, FaceIn, FMath::Lerp(36.f, 6.f, Rise));
		CH.Hips = FS6Frame::Lean(FaceIn, FMath::Lerp(28.f, 6.f, Rise));
		CH.Chest = FS6Frame::Lean(FaceIn, FMath::Lerp(48.f, 12.f, Rise) - 12.f * S6Bump(U, 0.5f, 0.2f));	// the heave
		CH.Head = FS6Frame::Lean(FaceIn, FMath::Lerp(25.f, 30.f, Rise));
		const bool bSwap = FVector::DotProduct(ShL - ShR, Ci) > 0.f;	// his left hand takes the pit on his left
		const FVector KnL = GR.Bone(TEXT("calf_l")), KnR = GR.Bone(TEXT("calf_r"));
		const float ToKnees = S6Win(U, 0.65f, 0.95f);
		const float Off = S6Win(U, 1.25f, 1.6f);
		for (int32 S = 0; S < 2; ++S)
		{
			const float Side = S == 0 ? -1.f : 1.f;
			const bool bL = (S == 0) != bSwap;
			const FVector Pit = (bL ? ShL : ShR) - S6Z * 6.f;
			const FVector Knee = (bL ? KnL : KnR) + S6Z * 6.f;
			const FVector Hd = FMath::Lerp(Pit, Knee, ToKnees) + S6Z * (25.f * FMath::Sin(UE_PI * ToKnees));
			CH.Hand[S] = FMath::Lerp(Hd, CH.Hand[S], Off);
			CH.Elbow[S] = Stance + S6Z * 110.f + Ci * (Side * 70.f);
		}
		CP = FS6Pose::Lerp(CP, CH, S6Win(U, 0.f, 0.35f));
		const FVector Ce = S6RightOf(-H);
		const FS6Plant PL[] = { { 0.0f, 0.32f, Stance1 - Ci * (Sp + 12.f) + Ah }, { 0.55f, 0.85f, Stance2 - Ci * (Sp + 12.f) + Ah } };
		const FS6Plant PR[] = { { 0.12f, 0.45f, Stance1 + Ci * (Sp + 12.f) + Ah }, { 0.68f, 0.98f, Stance2 + Ci * (Sp + 12.f) + Ah } };
		CP.Ankle[0] = S6Foot(LastColeFeet - Ce * (Sp + 10.f) + Ah, PL, 2, U, 12.f);
		CP.Ankle[1] = S6Foot(LastColeFeet + Ce * (Sp + 10.f) + Ah, PR, 2, U, 12.f);
	}
	FVector CFace = CP.Hips.Fwd.GetSafeNormal2D();
	if (CFace.IsNearlyZero()) { CFace = Face; }
	if (T <= Reach || T > Heave0)
	{
		const FVector RN = S6RightOf(CFace);
		for (int32 S = 0; S < 2; ++S)
		{
			const float Side = S == 0 ? -1.f : 1.f;
			CP.Knee[S] = CP.Ankle[S] + CFace * 100.f + S6Z * 30.f + RN * (Side * 25.f);
		}
	}
	if (Cole)
	{
		const FVector Feet = (CP.Ankle[0] + CP.Ankle[1]) * 0.5f;
		const float Half = Cole->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Cole->SetActorLocationAndRotation(FVector(Feet.X, Feet.Y, Floor + Half + 1.f), FRotator(0.f, S6Yaw2D(CFace), 0.f),
			false, nullptr, ETeleportType::TeleportPhysics);
	}
	ColeRig.Apply(CP);

	if (T >= Heave0 + HeaveT)
	{
		GuardRig.Detach();
		if (G.Body) { G.Body->SetActorHiddenInGame(true); }
		bLidOpen = false;
		LidAge = 0.f;
		Play(TArray<FName>{ TEXT("sfx_s6_trunk"), TEXT("sfx_thud") }, &TrunkPoint, 0.8f);
		ColeRig.Detach();
		if (Cole)
		{
			// He was placed without sweeping: nudge him out of the trunk or a wall before handing him back.
			Cole->TeleportTo(Cole->GetActorLocation(), Cole->GetActorRotation());
		}
		FreezeCole(false);
		ViewStunt(false);
		Step = EStep::ToPanel;
		StepAge = 0.f;
		RouteIndex = 0;
		if (Cole) { Cole->SetLook(S6Yaw2D(ObjectivePoint() - Cole->GetActorLocation()), -12.f); }
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
		if (Cole) { Cole->SetLook(S6Yaw2D(ExitStand - Cole->GetActorLocation()), -10.f); }
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
		const float K = S6Ease((T - 2.f) / 0.25f);
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
		if (ATakeCharacter* C = GetPovCharacter()) { C->SetActorRotation(FRotator(0.f, S6Yaw2D(PanelPoint - C->GetActorLocation()), 0.f)); }
		Objective = TEXT("");
		return true;
	}
	if (Step == EStep::ToExit && Near(ExitStand, UseReach))
	{
		Step = EStep::Wedging;
		StepAge = 0.f;
		SetCrouch(false);
		FreezeCole(true);
		if (ATakeCharacter* C = GetPovCharacter()) { C->SetActorRotation(FRotator(0.f, S6Yaw2D(ExitAlarmPoint - C->GetActorLocation()), 0.f)); }
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
		const float Open = bLidOpen ? S6Ease(LidAge / 0.4f) : 1.f - S6Ease(LidAge / 0.3f);
		TurnAboutEdge(TrunkLid, LidHome, FVector(0.f, 50.f, -50.f), FVector(1.f, 0.f, 0.f), LidOpenDegrees * Open);
	}
	// The lever: thrown, then the sandbag drops 0.45 s later onto the sheet.
	if (LeverAge >= 0.f)
	{
		LeverAge += Dt;
		TurnAboutEdge(LeverHandle, LeverHome, FVector(0.f, 0.f, -50.f), FVector(0.f, 1.f, 0.f), LeverPullDegrees * S6Ease(LeverAge / 0.25f));
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
		TurnAboutEdge(PanelCover, PanelHome, FVector(-50.f, 0.f, 0.f), FVector(0.f, 0.f, 1.f), PanelOpenDegrees * S6Ease(PanelAge / 0.35f));
	}
	// The prop store door swings wide open.
	if (ExitAge >= 0.f)
	{
		ExitAge += Dt;
		TurnAboutEdge(ExitLeaf, ExitHome, FVector(-50.f, 0.f, 0.f), FVector(0.f, 0.f, 1.f), ExitOpenDegrees * S6Ease(ExitAge / 0.8f));
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
	UGuardBrainComponent* Brain = CVarS6GuardBrain.GetValueOnGameThread() != 0 ? S6GuardBrain(G.Body) : nullptr;
	if (UGuardBrainComponent* Any = S6GuardBrain(G.Body); Any && (Step != EStep::Sneak || G.bDown || !Brain))
	{
		Any->SetBrainActive(false);
	}
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
		if (Brain && !G.bDown && GuardState == EGuard::Post)
		{
			Brain->SetBrainActive(true);
			G.Alert = Brain->GetSuspicion();
			return;
		}
		if (Brain && !G.bDown)
		{
			// Lured: the scene's walk, look and hearing; back at his post the brain takes him again, calm.
			const EGuard Before = GuardState;
			StepGuard(Dt);
			StepSight(Dt);
			if (Before != EGuard::Post && GuardState == EGuard::Post && Step == EStep::Sneak)
			{
				Brain->ResetBrain();
				Brain->SetBrainActive(true);
			}
			return;
		}
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
	if (!Canvas || !GetWorld())
	{
		return;
	}
	// QA Nov 16 (Marwan): the HUD's own widgets (ScriptHudWidgets.h), not plain text: the objective plate, the stance
	// icon (a small yellow figure), the guard's alert diamond, the waypoint, the bottom-centre action prompt.
	using ScriptHudWidgets::ETone;
	FTakeHudKit Kit;
	Kit.Begin(Canvas, GetWorld()->GetRealTimeSeconds(), 138.f);
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const bool bPlayable = Step == EStep::Sneak || Step == EStep::Down || Step == EStep::ToPanel || Step == EStep::ToExit;

	if (!Objective.IsEmpty())
	{
		ScriptHudWidgets::Objective(Kit, Objective);
	}
	if (bPlayable)
	{
		// The stance: crouched is silent.
		ScriptHudWidgets::StanceIcon(Kit, bColeCrouched);
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	// Who's onto him: ? then ! over the head.
	if (PC && G.Body && !G.bDown && G.Alert >= 0.04f)
	{
		FVector2D Scr;
		if (PC->ProjectWorldLocationToScreen(G.Body->GetActorLocation() + FVector(0.f, 0.f, 215.f), Scr, true))
		{
			ScriptHudWidgets::Alert(Kit, Scr, G.Alert);
		}
	}
	// Where to go: a waypoint on the next thing to use (clamped to the screen's edge when it's off screen).
	if (PC && bPlayable)
	{
		const FVector P = ObjectivePoint();
		FVector2D Scr;
		if (!P.IsZero() && PC->ProjectWorldLocationToScreen(P, Scr, true))
		{
			const float M = Kit.Px(48.f);
			const bool bOff = Scr.X < M || Scr.X > W - M || Scr.Y < M || Scr.Y > H - M;
			Scr.X = FMath::Clamp(Scr.X, M, W - M);
			Scr.Y = FMath::Clamp(Scr.Y, M, H - M);
			ScriptHudWidgets::Waypoint(Kit, Scr, bOff, static_cast<float>(FVector::Dist2D(P, ColeLocation())) / 100.f);
		}
	}

	// The prompt (bottom centre).
	FString Prompt;
	ETone PromptTone = ETone::Action;
	if (bPlayable)
	{
		const FVector Cole = ColeLocation();
		auto Near = [&](const FVector& At, float Reach) { return FVector::Dist2D(Cole, At) <= Reach && FMath::Abs(Cole.Z - At.Z) < 150.f; };
		if (Step == EStep::Sneak && !G.bDown && G.Body && Near(G.Body->GetActorLocation(), TakedownReach + 60.f))
		{
			if (!IsBehind(Cole)) { Prompt = TEXT("He's facing you: get behind him"); PromptTone = ETone::Danger; }
			else { Prompt = TEXT("[E] Choke-hold from behind"); }
		}
		else if (Step == EStep::Sneak && Near(LeverPoint, UseReach))
		{
			const bool bReady = bLeverArmed && GuardState != EGuard::ToLook && GuardState != EGuard::Looking;
			Prompt = bReady ? TEXT("[E] Throw the counterweight lever") : TEXT("Counterweight lever: he's still looking");
			PromptTone = bReady ? ETone::Action : ETone::Muted;
		}
		else if (Step == EStep::Down && Near(BodyAt, 200.f))
		{
			Prompt = TEXT("[E] Drag him into the costume trunk");
		}
		else if (Step == EStep::ToPanel && Near(PanelStand, UseReach))
		{
			Prompt = TEXT("[E] Rig the door scanner");
		}
		else if (Step == EStep::ToExit && Near(ExitStand, UseReach))
		{
			Prompt = TEXT("[E] Kill the alarm, wedge the door open");
		}
	}
	if (!Prompt.IsEmpty())
	{
		ScriptHudWidgets::Prompt(Kit, Prompt, PromptTone);
	}
	if (ToastLeft > 0.f && !Toast.IsEmpty())
	{
		ScriptHudWidgets::Toast(Kit, Toast, ETone::Action, FMath::Clamp(ToastLeft / 0.4f, 0.f, 1.f));
	}
}
