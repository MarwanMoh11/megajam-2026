// Scene 8, the escape and the getaway (see Scene8Alley.h). Owner: s8-escape.

#include "Script/Scene8Alley.h"

#include "Algo/BinarySearch.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "CollisionQueryParams.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/HitResult.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#if __has_include("Script/Scene7Booth.h")
#include "Script/Scene7Booth.h"
#define S8_HAS_BOOTH 1
#endif
#include "Script/Scene8Car.h"
#include "Script/Scene8Chaser.h"
#include "Script/ScriptSequencer.h"
#include "Sound/SoundBase.h"
#include "TakeCharacter.h"
#include "TakeGuide.h"
#include "TakeHudKit.h"
#include "TakeShapes.h"
#include "TakeStyle.h"

DEFINE_LOG_CATEGORY_STATIC(LogScene8, Log, All);

// The test autopilot (scene8_pie.py): Vance runs Cole's route to the car by himself, so a PIE run can be shot.
static TAutoConsoleVariable<int32> CVarS8Autopilot(TEXT("Reel.S8.Autopilot"), 0,
	TEXT("1: in Scene 8 Vance runs the escape route to the car on his own (testing); WASD takes over."));

namespace
{
	const TCHAR* const AudioDir = TEXT("/Game/Take/Audio/Script/S8/");

	TSoftObjectPtr<USoundBase> S8Sound(const TCHAR* Name)
	{
		return TSoftObjectPtr<USoundBase>(FSoftObjectPath(FString::Printf(TEXT("%s%s.%s"), AudioDir, Name, Name)));
	}

	float S8Smooth(float A) { A = FMath::Clamp(A, 0.f, 1.f); return A * A * (3.f - 2.f * A); }

	const FLinearColor S8Steel(0.16f, 0.19f, 0.18f);

	ACameraActor* SpawnCam(UWorld* World, const FVector& At, const FVector& LookAt, float Fov)
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ACameraActor* C = World->SpawnActor<ACameraActor>(At, (LookAt - At).Rotation(), P);
		if (C)
		{
			C->GetCameraComponent()->SetConstraintAspectRatio(false);
			C->GetCameraComponent()->SetFieldOfView(Fov);
		}
		return C;
	}

	// Seated, Vance faces the way the car does. The character turns with its controller's rotation, so that turns too.
	void FaceForward(APawn* Vance, AActor* Car)
	{
		if (!Vance || !Car) { return; }
		const FRotator R(0.f, Car->GetActorRotation().Yaw, 0.f);
		if (AController* C = Vance->GetController()) { C->SetControlRotation(R); }
		Vance->SetActorRotation(R);
	}

	// A cast character's dressed body is a separate actor attached to it (ACastMember), which
	// SetActorHiddenInGame does not reach: hide or show the attached actors with it.
	void SetHiddenWithKids(AActor* A, bool bHide)
	{
		if (!A) { return; }
		A->SetActorHiddenInGame(bHide);
		TArray<AActor*> Kids;
		A->GetAttachedActors(Kids, true, true);
		for (AActor* K : Kids) { if (K) { K->SetActorHiddenInGame(bHide); } }
	}

	void IgnoreWithKids(FCollisionQueryParams& Q, AActor* A)
	{
		if (!A) { return; }
		Q.AddIgnoredActor(A);
		TArray<AActor*> Kids;
		A->GetAttachedActors(Kids, true, true);
		for (AActor* K : Kids) { Q.AddIgnoredActor(K); }
	}

	// The generated GT is a closed shell (its windows are only texture): the mirror camera sits inside it, so the body
	// is hidden for the mirror shot and shown again on a reset. Only a body whose mesh loaded is shown.
	void ShowCarBody(AActor* Car, bool bShow)
	{
		if (!Car) { return; }
		TArray<UStaticMeshComponent*> Parts;
		Car->GetComponents(Parts);
		for (UStaticMeshComponent* M : Parts)
		{
			if (M && M->GetFName() == TEXT("Body") && M->GetStaticMesh()) { M->SetVisibility(bShow); }
		}
	}
}

AScene8Alley::AScene8Alley()
{
	SceneNumber = 8;
	Pov = EScriptPov::Vance;
	Slug = NSLOCTEXT("Scene8", "Slug", "INT. SERVICE CORRIDOR / EXT. REAR ALLEYWAY - NIGHT");
	bPlayerWalks = true;
	bAllowRun = true;
	CarClass = AScene8Car::StaticClass();
	ChaserClass = AScene8Chaser::StaticClass();
	SparkSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Take/FX/NS_Take_Sparks.NS_Take_Sparks")));
	DoorKickSound = S8Sound(TEXT("s8_door_kick"));
	GunshotSounds = { S8Sound(TEXT("s8_gunshot_1")), S8Sound(TEXT("s8_gunshot_2")), S8Sound(TEXT("s8_gunshot_3")) };
	RicochetSounds = { S8Sound(TEXT("s8_ricochet_1")), S8Sound(TEXT("s8_ricochet_2")), S8Sound(TEXT("s8_ricochet_3")) };
	CarArriveSound = S8Sound(TEXT("s8_car_arrive"));
	CarAwaySound = S8Sound(TEXT("s8_car_away"));
	CarDoorSound = S8Sound(TEXT("s8_car_door_slam"));
	InteriorLoop = S8Sound(TEXT("s8_car_interior"));
	MaxLine = S8Sound(TEXT("vo_s8_max_getin"));
	ColeLine = S8Sound(TEXT("vo_s8_cole_go"));
	GuardLine = S8Sound(TEXT("vo_s8_guard_stop"));
	VanceLine = S8Sound(TEXT("vo_s8_vance_print"));
	StumbleSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(
		TEXT("/Game/Take/Audio/Script/S7/vo_s7_vance_grunt_1.vo_s7_vance_grunt_1")));
	ColeDoorSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Take/Audio/Script/S7/sfx_s7_door_kick.sfx_s7_door_kick")));
	SmokeSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Take/Audio/Script/S7/sfx_s7_smoke.sfx_s7_smoke")));
	ColeMoveLine = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Take/Audio/Script/S7/vo_s7_cole_move.vo_s7_cole_move")));
	SmokeMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Take/Script/S7/M_S7_Smoke.M_S7_Smoke")));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// The fire exit: a 120 x 230 steel door hinged at -X (swings out to -Y), the EXIT sign above. The opening behind it
	// is real (scene8.py cuts it in the prop store's wall); the old black doorway slab stays hidden.
	Doorway = FTakeShapes::CreateShape(this, TEXT("Doorway"), ETakeShape::Cube, Root, FVector(0.f, -1.f, 116.f),
		FVector(1.26f, 0.02f, 2.34f), FRotator::ZeroRotator, false);
	Doorway->SetHiddenInGame(true);
	Doorway->SetVisibility(false);
	DoorHinge = CreateDefaultSubobject<USceneComponent>(TEXT("DoorHinge"));
	DoorHinge->SetupAttachment(Root);
	DoorHinge->SetRelativeLocation(FVector(-62.f, -6.f, 0.f));
	Door = FTakeShapes::CreateShape(this, TEXT("Door"), ETakeShape::Cube, DoorHinge, FVector(62.f, 0.f, 116.f),
		FVector(1.22f, 0.06f, 2.3f));
	ExitSign = FTakeShapes::CreateShape(this, TEXT("ExitSign"), ETakeShape::Cube, Root, FVector(0.f, -6.f, 262.f),
		FVector(0.5f, 0.06f, 0.18f), FRotator::ZeroRotator, false);

	ArrivePath = CreateDefaultSubobject<USplineComponent>(TEXT("ArrivePath"));
	ArrivePath->SetupAttachment(Root);
	ArrivePath->ClearSplinePoints(false);
	for (const FVector& P : { FVector(-3300.f, 1100.f, 0.f), FVector(-3250.f, -150.f, 0.f), FVector(-2700.f, -430.f, 0.f),
		FVector(-600.f, -440.f, 0.f), FVector(900.f, -430.f, 0.f) })
	{
		ArrivePath->AddSplinePoint(P, ESplineCoordinateSpace::Local, false);
	}
	ArrivePath->UpdateSpline();

	DepartPath = CreateDefaultSubobject<USplineComponent>(TEXT("DepartPath"));
	DepartPath->SetupAttachment(Root);
	DepartPath->ClearSplinePoints(false);
	for (const FVector& P : { FVector(900.f, -430.f, 0.f), FVector(1220.f, -470.f, 0.f), FVector(1310.f, -820.f, 0.f),
		FVector(1310.f, -1300.f, 0.f), FVector(1310.f, -2300.f, 0.f) })
	{
		DepartPath->AddSplinePoint(P, ESplineCoordinateSpace::Local, false);
	}
	DepartPath->UpdateSpline();

	auto MakeLight = [this](FName Name, float Radius, const FLinearColor& C) {
		UPointLightComponent* L = CreateDefaultSubobject<UPointLightComponent>(Name);
		L->SetupAttachment(Root);
		L->SetMobility(EComponentMobility::Movable);
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(0.f);
		L->SetAttenuationRadius(Radius);
		L->SetLightColor(C);
		L->SetCastShadows(false);
		return L;
	};
	SparkLight = MakeLight(TEXT("SparkLight"), 350.f, FLinearColor(1.f, 0.8f, 0.5f));
	PassingLight = MakeLight(TEXT("PassingLight"), 900.f, FLinearColor(1.f, 0.62f, 0.25f));
}

void AScene8Alley::BeginPlay()
{
	Super::BeginPlay();
	FTakeShapes::SetColour(Door, S8Steel);
	FTakeShapes::SetColour(ExitSign, FLinearColor(0.1f, 0.9f, 0.3f));
	Doorway->SetHiddenInGame(true);
	// The smoke grenade and the puffs it billows into.
	Grenade = FTakeShapes::AddShapeAtRuntime(this, ETakeShape::Cylinder, Root, FVector::ZeroVector, FVector(0.07f, 0.07f, 0.12f),
		FLinearColor(0.12f, 0.14f, 0.1f), false);
	UMaterialInterface* SmokeM = SmokeMaterial.IsNull() ? nullptr : SmokeMaterial.LoadSynchronous();
	for (int32 i = 0; i < 9; ++i)
	{
		UStaticMeshComponent* S = FTakeShapes::AddShapeAtRuntime(this, ETakeShape::Sphere, Root, FVector::ZeroVector,
			FVector(1.f), FLinearColor(0.55f, 0.56f, 0.58f), false);
		if (SmokeM) { S->SetMaterial(0, SmokeM); }
		SmokePuffs.Add(S);
	}

	UWorld* World = GetWorld();
	if (!World) { return; }
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	P.Owner = this;
	UClass* CC = CarClass ? CarClass.Get() : AScene8Car::StaticClass();
	Car = World->SpawnActor<AScene8Car>(CC, ArrivePath->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World),
		FRotator::ZeroRotator, P);
	UClass* GC = ChaserClass ? ChaserClass.Get() : AScene8Chaser::StaticClass();
	for (int32 i = 0; i < FMath::Clamp(ChaserCount, 0, 6); ++i)
	{
		if (AScene8Chaser* C = World->SpawnActor<AScene8Chaser>(GC, GetActorLocation(), FRotator::ZeroRotator, P))
		{
			C->Show(false);
			Chasers.Add(C);
		}
	}
	DriveCam = SpawnCam(World, W(DriveCamAt), W(FVector(1200.f, -500.f, 80.f)), 50.f);
	MirrorCam = SpawnCam(World, GetActorLocation(), GetActorLocation() + FVector(1.f, 0.f, 0.f), MirrorFov);
	SceneCamera = nullptr;		// the splice cut lands on Vance's own follow camera, where Scene 7 left him
	if (!StartSpot)
	{
		// No placed start (scene8.py sets the fight room's): inside the prop store, facing the exit.
		StartSpot = World->SpawnActor<ATargetPoint>(W(FVector(0.f, 600.f, 100.f)), FRotator(0.f, GetActorRotation().Yaw - 90.f, 0.f), P);
	}
	ResetScene();
}

void AScene8Alley::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Interior) { Interior->Stop(); }
	Super::EndPlay(Reason);
}

ATakeCharacter* AScene8Alley::GetCole() const
{
	AScriptSequencer* S = GetSequencer();
	return S ? S->GetPovCharacter(EScriptPov::Cole) : nullptr;
}

USoundBase* AScene8Alley::Load(const TSoftObjectPtr<USoundBase>& Sound) const
{
	return Sound.IsNull() ? nullptr : Sound.LoadSynchronous();
}

void AScene8Alley::PlayAt(const TSoftObjectPtr<USoundBase>& Sound, const FVector& Where, float Volume)
{
	if (USoundBase* S = Load(Sound)) { UGameplayStatics::PlaySoundAtLocation(this, S, Where, Volume); }
}

void AScene8Alley::FreeCast()
{
	for (ATakeCharacter* C : { GetPovCharacter(), GetCole() })
	{
		if (!C) { continue; }
		SetHiddenWithKids(C, false);
		C->SetActorEnableCollision(true);
		if (UCharacterMovementComponent* M = C->GetCharacterMovement())
		{
			if (M->MovementMode == MOVE_None) { M->SetMovementMode(MOVE_Walking); }
		}
	}
}

void AScene8Alley::SetSpeeds(bool bRun)
{
	if (ATakeCharacter* Vance = GetPovCharacter()) { Vance->SetRunning(bRun); }
	if (ATakeCharacter* Cole = GetCole())
	{
		Cole->StopScriptWalk();
		Cole->SetRunning(false);
	}
	ColeGoing = -1;
}

void AScene8Alley::ResetScene()
{
	Beat = EScene8Beat::Done;
	BeatT = 0.f;
	NextShot = 0.f;
	ShotCount = 0;
	SparkAge = 1.f;
	bMaxSaid = bColeIn = bTie = bPrint = bArrived = false;
	Trail.Reset();
	TrailDist.Reset();
	ColeIdx = 0;
	ColeGoing = -1;
	OutDist = -1.f;
	CarT = DoorT = GuardsT = StumbleT = -1.f;
	GuardsFrom = LeadDist = 0.f;
	bStumbled = bOutside = bColeOut = bWarned = false;
	StallT = 0.f;
	ThrowT = -1.f;
	bColeIntro = bColeSaid = false;
	if (Grenade) { Grenade->SetHiddenInGame(true); }
	for (UStaticMeshComponent* S : SmokePuffs) { if (S) { S->SetHiddenInGame(true); } }
	DoorHinge->SetRelativeRotation(FRotator::ZeroRotator);
	SparkLight->SetIntensity(0.f);
	PassingLight->SetIntensity(0.f);
	for (AScene8Chaser* C : Chasers) { if (C) { C->Show(false); C->SetActorLocation(ExitInside()); } }
	if (Interior) { Interior->Stop(); Interior = nullptr; }
	if (Car)
	{
		Car->EndFinale();
		Car->SetActorHiddenInGame(false);
		ShowCarBody(Car, true);
		PoseCar(ArrivePath, 0.f, 0.f);
		Car->SetFlare(0.f);
		Car->SetDoorOpen(0.f);
		Car->SetRiders(false, false);
	}
	FreeCast();
}

void AScene8Alley::BeginScene()
{
	Super::BeginScene();
	ResetScene();
	ATakeCharacter* Vance = GetPovCharacter();
	if (ATakeCharacter* Cole = GetCole())
	{
		// Where Scene 7 left him (by the fight room's door), else a step behind Vance.
		FVector At = ColeStartWorld;
		if (At.IsNearlyZero() && Vance) { At = Vance->GetActorLocation() - Vance->GetActorForwardVector() * 140.f + Vance->GetActorRightVector() * 60.f; }
		if (!At.IsNearlyZero())
		{
			const FVector First = RouteWorld.Num() > 0 ? RouteWorld[0] : ExitInside();
			Cole->TeleportTo(At + FVector(0.f, 0.f, 100.f), FRotator(0.f, (First - At).Rotation().Yaw, 0.f), false, true);
		}
	}
	// The silver canister from Scene 7 under his arm (s7's component; false when no booth is in the level).
#ifdef S8_HAS_BOOTH
	AScene7Booth::AttachCanisterTo(this, Vance);
#endif
	SetSpeeds(true);
	// Cole's entrance: he kicks the door in and runs to his mark (the grenade and the line follow in the tick).
	ATakeCharacter* Cole = GetCole();
	if (Cole && !ColeMarkWorld.IsNearlyZero() && !ColeStartWorld.IsNearlyZero())
	{
		bColeIntro = true;
		Cole->ScriptWalkTo(ColeMarkWorld, ColeRunSpeed, 50.f);
		PlayAt(ColeDoorSound, ColeStartWorld + FVector(0.f, 0.f, 120.f));
	}
	Enter(EScene8Beat::Escape);
}

void AScene8Alley::EndScene()
{
	if (Interior) { Interior->Stop(); Interior = nullptr; }
	PassingLight->SetIntensity(0.f);
	SetSpeeds(false);
	for (AScene8Chaser* C : Chasers) { if (C) { C->Show(false); } }
	// The film is over or a jump went elsewhere: nobody stays sunk in a car seat.
	if (Beat != EScene8Beat::Mirror && Beat != EScene8Beat::Done) { FreeCast(); }
	Super::EndScene();
}

void AScene8Alley::Enter(EScene8Beat NewBeat)
{
	Beat = NewBeat;
	BeatT = 0.f;
	ATakeCharacter* Vance = GetPovCharacter();
	UE_LOG(LogScene8, Log, TEXT("S8 beat %d"), (int32)NewBeat);
	switch (NewBeat)
	{
	case EScene8Beat::Escape:
		// No cut of our own: the sequencer's splice lands on his follow camera, in the room where the fight ended.
		if (Vance)
		{
			const FVector First = RouteWorld.Num() > 0 ? RouteWorld[0] : ExitInside();
			Vance->SetLook((First - Vance->GetActorLocation()).Rotation().Yaw, -10.f);
			Vance->SnapFollowCamera();
			if (CVarS8Autopilot.GetValueOnGameThread() > 0)
			{
				TArray<FVector> Path = RouteWorld;
				Path.Add(ExitInside());
				Path.Add(ExitOutside());
				Vance->ScriptWalkPath(Path, Vance->RunSpeed, 50.f);
			}
		}
		NextShot = 1e9f;
		// hud-guidance: the wander clock, Cole's nudge and the late marker point at the fire exit.
		if (UTakeGuideSubsystem* G = UTakeGuideSubsystem::Get(this))
		{
			G->SetGoal(ExitInside(), TEXT("The fire exit"), TEXT("Stay on me! Down the back corridor, the car's out back!"));
		}
		break;
	case EScene8Beat::Alley:
		NextShot = 0.25f;
		break;
	case EScene8Beat::Dive:
		if (Vance)
		{
			Vance->SetRunning(false);
			SetHiddenWithKids(Vance, true);
			Vance->SetActorEnableCollision(false);
			if (UCharacterMovementComponent* M = Vance->GetCharacterMovement()) { M->DisableMovement(); }
		}
		if (Car) { Car->SetRiders(true, true); }
		if (ATakeCharacter* Cole = GetCole())
		{
			Cole->StopScriptWalk();
			SetHiddenWithKids(Cole, true);
			Cole->SetActorEnableCollision(false);
			bColeIn = true;
		}
		if (DriveCam && Car) { DriveCam->SetActorRotation((Car->GetActorLocation() - DriveCam->GetActorLocation()).Rotation()); }
		CutTo(EScriptPov::Vance, DriveCam);
		if (Car) { PlayAt(CarDoorSound, Car->GetActorLocation()); }
		Say(TEXT("Cole"), TEXT("Go, go, go!"), Load(ColeLine));
		NextShot = 0.2f;
		break;
	case EScene8Beat::Drive:
		if (Car) { PlayAt(CarAwaySound, Car->GetActorLocation(), 1.2f); }
		break;
	case EScene8Beat::Mirror:
		// s8-car-mirror's final shot (AScene8Car::BeginFinale): his seated body in the car stands in for him.
		if (Car) { Car->SetActorHiddenInGame(false); Car->BeginFinale(); }
		SetHiddenWithKids(Vance, true);
		SetHiddenWithKids(GetCole(), true);
		for (AScene8Chaser* C : Chasers) { if (C) { C->Show(false); } }
		if (USoundBase* L = Load(InteriorLoop)) { Interior = UGameplayStatics::SpawnSound2D(this, L, 0.8f); }
		CutTo(EScriptPov::Vance, Car ? static_cast<AActor*>(Car) : MirrorCam.Get());
		break;
	case EScene8Beat::Done:
		FinishScene();
		break;
	}
}

void AScene8Alley::SeatVance()
{
	ATakeCharacter* Vance = GetPovCharacter();
	if (!Vance || !Car) { return; }
	if (UCharacterMovementComponent* M = Vance->GetCharacterMovement()) { M->DisableMovement(); }
	Vance->SetActorEnableCollision(false);
	SetHiddenWithKids(Vance, false);
	const float Half = Vance->GetCapsuleComponent() ? Vance->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	const FVector Seat = Car->GetPassengerSeatWorld();
	Vance->SetActorLocationAndRotation(Seat + FVector(0.f, 0.f, Half - SeatDrop),
		FRotator(0.f, Car->GetActorRotation().Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	FaceForward(Vance, Car);
	// The rear-view mirror looks back at his eyes (about 60 cm over the seat).
	const FVector Face = Seat + FVector(0.f, 0.f, 60.f);
	if (MirrorCam)
	{
		MirrorCam->GetCameraComponent()->SetFieldOfView(MirrorFov);
		MirrorCam->SetActorLocationAndRotation(Car->GetMirrorWorld(), (Face - Car->GetMirrorWorld()).Rotation());
	}
}

void AScene8Alley::PoseCar(USplineComponent* Path, float Alpha, float YawOffset)
{
	if (!Car || !Path) { return; }
	const float D = Path->GetSplineLength() * FMath::Clamp(Alpha, 0.f, 1.f);
	const FVector L = Path->GetLocationAtDistanceAlongSpline(D, ESplineCoordinateSpace::World);
	const float Yaw = Path->GetRotationAtDistanceAlongSpline(D, ESplineCoordinateSpace::World).Yaw + YawOffset;
	Car->SetActorLocationAndRotation(L, FRotator(0.f, Yaw, 0.f));
	// Body roll away from the slide.
	Car->Chassis->SetRelativeRotation(FRotator(0.f, 0.f, -YawOffset * 0.12f));
}

void AScene8Alley::Shoot(const FVector& Target, float Miss)
{
	// The shooter: the next guard in turn who is in the shot.
	AScene8Chaser* G = nullptr;
	for (int32 k = 0; k < Chasers.Num() && !G; ++k)
	{
		AScene8Chaser* C = Chasers[(ShotCount + k) % Chasers.Num()];
		if (C && !C->IsHidden()) { G = C; }
	}
	++ShotCount;
	UWorld* World = GetWorld();
	if (!G || !World) { return; }

	const FVector M = G->GetMuzzle();
	const FVector Flat = (Target - M).GetSafeNormal2D();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Flat);
	const FVector Aim = Target + Right * (FMath::RandBool() ? Miss : -Miss) + FVector(0.f, 0.f, FMath::FRandRange(-60.f, 50.f));
	const FVector End = M + (Aim - M).GetSafeNormal() * 4000.f;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(Scene8Shot), false);
	Q.AddIgnoredActor(this);
	for (AScene8Chaser* C : Chasers) { Q.AddIgnoredActor(C); }
	IgnoreWithKids(Q, GetPovCharacter());
	IgnoreWithKids(Q, GetCole());
	IgnoreWithKids(Q, Car);
	// A wall between him and his man (round a corner, behind a door frame): he holds his fire.
	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, M, Target, ECC_Visibility, Q)) { return; }
	// The round goes wide, into whatever is beside or behind his man: a wall at his shoulder, the floor, the far wall.
	const bool bHit = World->LineTraceSingleByChannel(Hit, M, End, ECC_Visibility, Q);

	const FVector HitW = bHit ? Hit.ImpactPoint : End;
	G->Fire(HitW);
	if (GunshotSounds.Num() > 0) { PlayAt(GunshotSounds[FMath::RandRange(0, GunshotSounds.Num() - 1)], M); }
	if (!bHit) { return; }
	const FRotator Out = Hit.ImpactNormal.Rotation();
	if (UNiagaraSystem* Sys = SparkSystem.IsNull() ? nullptr : SparkSystem.LoadSynchronous())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Sys, HitW, Out, FVector(0.6f));
	}
	SparkLight->SetWorldLocation(HitW + Hit.ImpactNormal * 15.f);
	SparkLight->SetIntensity(600.f);
	SparkAge = 0.f;
	if (RicochetSounds.Num() > 0) { PlayAt(RicochetSounds[FMath::RandRange(0, RicochetSounds.Num() - 1)], HitW, 0.8f); }
}

FVector AScene8Alley::TrailAt(float Dist, FVector* OutDir) const
{
	const int32 N = Trail.Num();
	if (N == 0) { if (OutDir) { *OutDir = GetActorForwardVector(); } return GetActorLocation(); }
	if (N == 1 || Dist <= 0.f)
	{
		if (OutDir) { *OutDir = N > 1 ? (Trail[1] - Trail[0]).GetSafeNormal2D() : GetActorForwardVector(); }
		return Trail[0];
	}
	const int32 Hi = Algo::LowerBound(TrailDist, Dist);
	if (Hi >= N)
	{
		if (OutDir) { *OutDir = (Trail[N - 1] - Trail[N - 2]).GetSafeNormal2D(); }
		return Trail[N - 1];
	}
	if (Hi == 0) { if (OutDir) { *OutDir = (Trail[1] - Trail[0]).GetSafeNormal2D(); } return Trail[0]; }
	const float Span = FMath::Max(1.f, TrailDist[Hi] - TrailDist[Hi - 1]);
	if (OutDir) { *OutDir = (Trail[Hi] - Trail[Hi - 1]).GetSafeNormal2D(); }
	return FMath::Lerp(Trail[Hi - 1], Trail[Hi], (Dist - TrailDist[Hi - 1]) / Span);
}

void AScene8Alley::TickCarArrival(float DeltaSeconds)
{
	ATakeCharacter* Vance = GetPovCharacter();
	if (CarT < 0.f)
	{
		const bool bNear = Vance && FVector::Dist(Vance->GetFeetLocation(), ExitInside()) < CarCueDistance;
		if (!bNear && DoorT < 0.f) { return; }
		CarT = 0.f;
		PlayAt(CarArriveSound, W(FVector(-1500.f, -430.f, 80.f)), 1.2f);
	}
	else { CarT += DeltaSeconds; }
	if (bArrived || !Car) { return; }
	const float A = FMath::Clamp(CarT / ArriveSeconds, 0.f, 1.f);
	const float Ease = 1.f - FMath::Pow(1.f - A, 2.2f);
	const float Drift = DriftDeg * FMath::Sin(PI * FMath::Clamp((A - 0.15f) / 0.85f, 0.f, 1.f));
	PoseCar(ArrivePath, Ease, Drift);
	Car->SetFlare(FMath::Max(0.f, 1.f - FMath::Abs(A - 0.3f) / 0.2f));
	Car->SetDoorOpen(S8Smooth((A - 0.85f) / 0.15f));
	if (!bMaxSaid && A > 0.78f)
	{
		bMaxSaid = true;
		Say(TEXT("Max"), TEXT("Get in! Get in!"), Load(MaxLine));
	}
	bArrived = A >= 1.f;
}

void AScene8Alley::TickDoor(float DeltaSeconds)
{
	if (DoorT < 0.f)
	{
		// The bar Cole rigged in Scene 6: whoever reaches it first slams it and the door bangs out into the rain.
		bool bHit = false;
		for (ATakeCharacter* C : { GetPovCharacter(), GetCole() })
		{
			if (!C) { continue; }
			const FVector F = C->GetFeetLocation();
			if (FVector::Dist2D(F, ExitInside()) < DoorReach && FMath::Abs(F.Z - ExitInside().Z) < 150.f) { bHit = true; }
		}
		if (!bHit) { return; }
		DoorT = 0.f;
		PlayAt(DoorKickSound, GetActorLocation() + FVector(0.f, 0.f, 120.f));
	}
	DoorT += DeltaSeconds;
	// Open in a tenth of a second, a bounce off the wall, then it hangs open.
	const float Swing = DoorT < 0.12f ? DoorT / 0.12f : 1.f - 0.06f * FMath::Sin(FMath::Min(1.f, (DoorT - 0.12f) / 0.4f) * PI);
	DoorHinge->SetRelativeRotation(FRotator(0.f, -100.f * Swing, 0.f));
}

bool AScene8Alley::TickColeEntrance(float DeltaSeconds, ATakeCharacter* Cole)
{
	if (!bColeIntro && ThrowT < 0.f) { return false; }
	// At his mark (or late): he turns to the door the guards will come through and lobs the grenade.
	if (ThrowT < 0.f)
	{
		const bool bAtMark = !Cole || FVector::Dist2D(Cole->GetFeetLocation(), ColeMarkWorld) < 80.f || BeatT > 1.4f;
		if (!bAtMark) { return true; }
		ThrowT = 0.f;
		if (Cole)
		{
			Cole->StopScriptWalk();
			Cole->SetActorRotation(FRotator(0.f, (SmokeWorld - Cole->GetActorLocation()).Rotation().Yaw, 0.f));
			Cole->PlayGestureAt(TEXT("Use"), SmokeWorld);
		}
		if (Grenade) { Grenade->SetHiddenInGame(false); }
	}
	else { ThrowT += DeltaSeconds; }

	const FVector From = (Cole ? Cole->GetActorLocation() : ColeMarkWorld) + FVector(0.f, 0.f, 50.f);
	const float A = ThrowT / FMath::Max(0.05f, GrenadeSeconds);
	if (A < 1.f && Grenade)
	{
		FVector P = FMath::Lerp(From, SmokeWorld + FVector(0.f, 0.f, 8.f), A);
		P.Z += 90.f * FMath::Sin(A * PI);
		Grenade->SetWorldLocationAndRotation(P, FRotator(A * 720.f, 0.f, A * 300.f));
	}
	else if (Grenade && ThrowT - DeltaSeconds < GrenadeSeconds)	// the frame it lands
	{
		Grenade->SetHiddenInGame(true);
		PlayAt(SmokeSound, SmokeWorld);
	}
	// The smoke billows out over SmokeGrowSeconds and hangs there (it hides the doorway the guards will come through).
	const float S = ThrowT - GrenadeSeconds;
	if (S >= 0.f)
	{
		const float Grow = S8Smooth(S / FMath::Max(0.1f, SmokeGrowSeconds));
		for (int32 i = 0; i < SmokePuffs.Num(); ++i)
		{
			UStaticMeshComponent* P = SmokePuffs[i];
			if (!P) { continue; }
			const float Ang = i * 2.f * PI / SmokePuffs.Num();
			const float R = SmokeRadius * (i % 2 ? 0.55f : 0.3f) * Grow;
			const FVector Off(FMath::Cos(Ang) * R, FMath::Sin(Ang) * R, 30.f + 35.f * (i % 3) + 40.f * Grow
				+ 6.f * FMath::Sin(S * 1.3f + i));
			P->SetWorldLocation(SmokeWorld + Off);
			P->SetWorldScale3D(FVector(SmokeRadius / 100.f * (0.25f + 0.75f * Grow) * (0.75f + 0.12f * (i % 3))));
			P->SetHiddenInGame(false);
		}
	}
	if (!bColeSaid && ThrowT >= GrenadeSeconds + 0.1f)
	{
		bColeSaid = true;
		if (Cole) { Cole->SetActorRotation(FRotator(0.f, (From - FVector(0.f, 0.f, 50.f) - SmokeWorld).Rotation().Yaw, 0.f)); }
		Say(TEXT("Cole"), TEXT("Max is out back! Move!"), Load(ColeMoveLine));
	}
	if (bColeIntro && ThrowT >= GrenadeSeconds + 0.45f)
	{
		bColeIntro = false;		// he leads from here
		ColeGoing = -1;
	}
	return bColeIntro;
}

void AScene8Alley::TickEscape(float DeltaSeconds, ATakeCharacter* Vance, ATakeCharacter* Cole)
{
	if (!Vance) { return; }
	const FVector Feet = Vance->GetFeetLocation();
	const FTransform T = GetActorTransform();
	auto IsOutside = [&T](const FVector& P) { return T.InverseTransformPosition(P).Y < -30.f; };

	// His trail: the guards run exactly where he ran (stairs, doors and all).
	if (Trail.Num() == 0 || FVector::Dist(Trail.Last(), Feet) >= 30.f)
	{
		TrailDist.Add(Trail.Num() ? TrailDist.Last() + FVector::Dist(Trail.Last(), Feet) : 0.f);
		Trail.Add(Feet);
	}
	const float Head = TrailDist.Last();
	if (!bOutside && IsOutside(Feet))
	{
		bOutside = true;
		OutDist = Head;
	}

	// Cole leads: through the route, out of the exit, then into the back seat once the car is there.
	if (Cole && !bColeIn && !bColeIntro)
	{
		const int32 NR = RouteWorld.Num();
		auto ColePoint = [&](int32 i) { return i < NR ? RouteWorld[i] : i == NR ? ExitInside() : ExitOutside() + T.TransformVector(FVector(200.f, -150.f, 0.f)); };
		const FVector CF = Cole->GetFeetLocation();
		if (ColeIdx <= NR + 1 && FVector::Dist2D(CF, ColePoint(ColeIdx)) < 90.f) { ++ColeIdx; }
		if (!bColeOut && IsOutside(CF)) { bColeOut = true; }
		const bool bTooFar = !bColeOut && FVector::Dist(CF, Feet) > ColeLeadMax;
		if (ColeIdx > NR + 1 && bArrived && Car)
		{
			const FVector Back = Car->GetActorTransform().TransformPosition(FVector(-120.f, 110.f, 0.f));
			if (FVector::Dist2D(CF, Back) < 130.f)
			{
				bColeIn = true;
				Cole->StopScriptWalk();
				SetHiddenWithKids(Cole, true);
				Cole->SetActorEnableCollision(false);
				Car->SetRiders(false, true);
			}
			else if (ColeGoing != 100) { ColeGoing = 100; Cole->ScriptWalkTo(Back, ColeRunSpeed, 60.f); }
		}
		else if (bTooFar || ColeIdx > NR + 1)
		{
			// He waits for Vance (or for the car), turned to him: "this way".
			if (ColeGoing != -1) { ColeGoing = -1; Cole->StopScriptWalk(); }
			const FVector To = (bTooFar ? Feet : (Car ? Car->GetActorLocation() : Feet)) - CF;
			Cole->SetActorRotation(FMath::RInterpTo(Cole->GetActorRotation(), FRotator(0.f, To.Rotation().Yaw, 0.f), DeltaSeconds, 8.f));
		}
		else if (ColeGoing != ColeIdx)
		{
			ColeGoing = ColeIdx;
			Cole->ScriptWalkTo(ColePoint(ColeIdx), ColeRunSpeed, 60.f);
		}
	}

	// The guards burst out of the door behind them once Vance is GapStart past it.
	if (GuardsT < 0.f)
	{
		const FVector From = GuardsEnterWorld.IsNearlyZero() ? Trail[0] : GuardsEnterWorld;
		float EnterDist = -1.f, Best = 250.f;
		for (int32 i = 0; i < Trail.Num(); ++i)
		{
			const float D = FVector::Dist(Trail[i], From);
			if (D < Best) { Best = D; EnterDist = TrailDist[i]; }
		}
		if (EnterDist < 0.f && Head > 2500.f) { EnterDist = Head - GapStart; }		// he went another way: they find him
		if (EnterDist >= 0.f && Head - EnterDist >= GapStart)
		{
			GuardsT = 0.f;
			GuardsFrom = LeadDist = EnterDist;
			Say(TEXT("Guard"), TEXT("Stop them!"), Load(GuardLine));
			NextShot = BeatT + 0.45f;
		}
	}
	if (GuardsT < 0.f) { return; }
	GuardsT += DeltaSeconds;

	// The gap: they hold Target behind him, closing as the chase goes on; standing still lets them close to MinGap.
	// They keep his pace (slow corners cost nothing); only standing still lets them run him down.
	const float VSpeed = Vance->GetVelocity().Size2D();
	const bool bStill = VSpeed < 60.f;
	const float Gap = Head - LeadDist;
	const float Target = bStill ? MinGap : FMath::Lerp(GapStart, GapEnd, FMath::Clamp(GuardsT / FMath::Max(0.1f, GapCloseSeconds), 0.f, 1.f));
	const float Base = bStill ? GuardRunSpeed : FMath::Min(VSpeed, GuardRunSpeed + 40.f);
	const float Speed = FMath::Clamp(Base + 1.2f * (Gap - Target), 0.f, GuardRunMax);
	LeadDist = FMath::Min(LeadDist + Speed * DeltaSeconds, FMath::Max(LeadDist, Head - MinGap));
	if (!bOutside && bStill && Head - LeadDist < MinGap + 80.f) { StallT += DeltaSeconds; }
	else { StallT = FMath::Max(0.f, StallT - 2.f * DeltaSeconds); }
	if (!bWarned && StallT > 1.2f)
	{
		bWarned = true;
		Say(TEXT("Cole"), TEXT("Don't stop! Move!"));
	}
	if (StallT >= StallSeconds) { FailScene(TEXT("Caught")); return; }

	// Each guard on the trail, one spacing behind the next; out of the exit they fan to their marks and aim.
	const FVector Marks[3] = { W(GuardMark0), W(GuardMark1), W(GuardMark2) };
	for (int32 i = 0; i < Chasers.Num(); ++i)
	{
		AScene8Chaser* C = Chasers[i];
		if (!C) { continue; }
		const float Di = LeadDist - i * ChaserSpacing;
		if (Di < GuardsFrom) { if (!C->IsHidden()) { C->Show(false); } continue; }
		if (C->IsHidden()) { C->Show(true); }
		FVector Dir;
		FVector P = TrailAt(Di, &Dir);
		float Yaw = Dir.Rotation().Yaw;
		bool bRun = Speed > 60.f;
		if (OutDist >= 0.f && Di > OutDist + 120.f)
		{
			const FVector Doorstep = TrailAt(OutDist + 120.f);
			const float A = FMath::Clamp((Di - OutDist - 120.f) / 250.f, 0.f, 1.f);
			P = FMath::Lerp(Doorstep, Marks[i % 3], A);
			Yaw = (Feet - P).Rotation().Yaw;
			bRun = A < 1.f;
		}
		const FRotator Was = C->GetActorRotation();
		C->SetActorLocationAndRotation(P, FMath::RInterpTo(Was, FRotator(0.f, Yaw, 0.f), DeltaSeconds, 10.f));
		C->SetRunning(bRun);
	}

	// The stumble: a round sparks off the wall at his shoulder, he staggers, the gap closes, he finds his feet.
	if (!bStumbled && !bOutside && Head - (GuardsFrom + GapStart) >= StumbleAfter)
	{
		bStumbled = true;
		StumbleT = 0.f;
		Vance->SetRunning(false);
		Vance->AddLook(FMath::FRandRange(-5.f, 5.f), -4.f);
		Shoot(Vance->GetActorLocation() + FVector(0.f, 0.f, 40.f), 45.f);
		if (USoundBase* S = Load(StumbleSound)) { UGameplayStatics::PlaySoundAtLocation(this, S, Feet, 0.9f); }
		NextShot = BeatT + 0.25f;
	}
	if (StumbleT >= 0.f)
	{
		StumbleT += DeltaSeconds;
		if (StumbleT >= StumbleSeconds) { StumbleT = -1.f; Vance->SetRunning(true); }
	}

	// The shots: closer as the chase tightens, never on him.
	if (BeatT >= NextShot)
	{
		const float Tight = FMath::Clamp(GuardsT / FMath::Max(0.1f, GapCloseSeconds), 0.f, 1.f);
		const float Miss = FMath::Lerp(MissStart, MissEnd, Tight) * FMath::FRandRange(0.85f, 1.3f);
		Shoot(Vance->GetActorLocation() + FVector(0.f, 0.f, 30.f), Miss);
		NextShot = BeatT + FMath::FRandRange(ShotGapMin, ShotGapMax);
	}
}

void AScene8Alley::TickScene(float DeltaSeconds)
{
	BeatT += DeltaSeconds;
	ATakeCharacter* Vance = GetPovCharacter();
	ATakeCharacter* Cole = GetCole();

	SparkAge += DeltaSeconds;
	SparkLight->SetIntensity(FMath::Max(0.f, 600.f * (1.f - SparkAge / 0.12f)));

	if (Beat == EScene8Beat::Escape || Beat == EScene8Beat::Alley)
	{
		TickColeEntrance(DeltaSeconds, Cole);
		TickDoor(DeltaSeconds);
		TickCarArrival(DeltaSeconds);
		TickEscape(DeltaSeconds, Vance, Cole);
		if (Beat != EScene8Beat::Escape && Beat != EScene8Beat::Alley) { return; }	// a retake started
	}

	switch (Beat)
	{
	case EScene8Beat::Escape:
		if (bOutside) { Enter(EScene8Beat::Alley); }
		break;
	case EScene8Beat::Alley:
	{
		if (Vance && Car && bArrived && CVarS8Autopilot.GetValueOnGameThread() > 0 && !Vance->IsScriptWalking())
		{
			Vance->ScriptWalkTo(Car->GetPassengerDoorWorld(), Vance->RunSpeed, ReachRadius * 0.5f);
		}
		if (Vance && Car && bArrived && FVector::Dist2D(Vance->GetFeetLocation(), Car->GetPassengerDoorWorld()) < ReachRadius)
		{
			Enter(EScene8Beat::Dive);
		}
		else if (BeatT >= AlleySeconds)
		{
			FailScene(TEXT("Pinned_Down"));
		}
		break;
	}
	case EScene8Beat::Dive:
	{
		if (Car) { Car->SetDoorOpen(1.f - S8Smooth(BeatT / 0.25f)); }
		if (Car && BeatT >= NextShot) { Shoot(Car->GetActorLocation(), 260.f); NextShot = BeatT + 0.3f; }
		if (BeatT >= DiveSeconds) { Enter(EScene8Beat::Drive); }
		break;
	}
	case EScene8Beat::Drive:
	{
		const float A = FMath::Clamp(BeatT / DriveSeconds, 0.f, 1.f);
		const float Ease = A * A;
		const float Tail = -FishtailDeg * FMath::Sin(PI * FMath::Clamp(A / 0.6f, 0.f, 1.f))
			+ 0.5f * FishtailDeg * FMath::Sin(2.f * PI * 2.2f * A) * (1.f - A);
		PoseCar(DepartPath, Ease, Tail);
		if (Car && DriveCam)
		{
			const FRotator Want = (Car->GetActorLocation() + FVector(0.f, 0.f, 60.f) - DriveCam->GetActorLocation()).Rotation();
			DriveCam->SetActorRotation(FMath::RInterpTo(DriveCam->GetActorRotation(), Want, DeltaSeconds, 6.f));
		}
		if (Car && A < 0.5f && BeatT >= NextShot) { Shoot(Car->GetActorLocation(), 300.f); NextShot = BeatT + 0.35f; }
		if (A >= 1.f) { Enter(EScene8Beat::Mirror); }
		break;
	}
	case EScene8Beat::Mirror:
	{
		if (Car) { Car->TickFinale(BeatT, DeltaSeconds); }
		if (!bPrint && BeatT >= (Car ? Car->PrintAt : PrintAt))
		{
			bPrint = true;
			Say(TEXT("Vance"), TEXT("Cut. Print it."), Load(VanceLine));
			SetFlag(TEXT("PrintItSaid"));	// the sequencer's end then skips its own "Cut. Print it."
		}
		if (BeatT >= (Car ? Car->FinaleSeconds : MirrorSeconds)) { Enter(EScene8Beat::Done); }
		break;
	}
	default:
		break;
	}
}

void AScene8Alley::DrawSceneHud(UCanvas* Canvas)
{
	if (!Canvas || !GetWorld()) { return; }
	FTakeHudKit Kit;
	Kit.Begin(Canvas, GetWorld()->GetRealTimeSeconds(), 0.f);
	// At the top of the picture, as in Scene 3, off the subtitle lane at the bottom.
	const float GoalY = Kit.PicY + Kit.Px(34.f);
	if (Beat == EScene8Beat::Escape)
	{
		Kit.Text(TakeStyle::EType::Goal, TEXT("Run! Follow Cole to the exit"), Kit.CX(), GoalY, TakeStyle::Paper, ETakeAlign::Centre);
		if (StallT > 0.4f)
		{
			Kit.Text(TakeStyle::EType::Goal, TEXT("KEEP MOVING"), Kit.CX(), GoalY + Kit.Px(34.f), TakeStyle::Danger, ETakeAlign::Centre);
		}
	}
	else if (Beat == EScene8Beat::Alley)
	{
		// The goal and the fire closing in: a bar that drains over AlleySeconds.
		const float Left = FMath::Clamp(1.f - BeatT / AlleySeconds, 0.f, 1.f);
		const float BW = Kit.Px(420.f), BH = Kit.Px(8.f);
		const float Y = Kit.PicY + Kit.Px(62.f);
		Kit.Text(TakeStyle::EType::Goal, TEXT("Get in the car"), Kit.CX(), GoalY, TakeStyle::Paper, ETakeAlign::Centre);
		Kit.Rect(Kit.CX() - BW * 0.5f, Y, BW, BH, FTakeHudKit::A(TakeStyle::Ink, 0.7f));
		Kit.Rect(Kit.CX() - BW * 0.5f, Y, BW * Left, BH, Left < 0.3f ? TakeStyle::Danger : TakeStyle::Julian);
	}
}
