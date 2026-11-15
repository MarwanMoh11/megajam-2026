// Scene 8, the rear alleyway (see Scene8Alley.h). Owner: s8-alley.

#include "Script/Scene8Alley.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Script/Scene8Car.h"
#include "Script/ScriptSequencer.h"
#include "Sound/SoundBase.h"
#include "TakeCharacter.h"
#include "TakeHudKit.h"
#include "TakeShapes.h"
#include "TakeStyle.h"

DEFINE_LOG_CATEGORY_STATIC(LogScene8, Log, All);

namespace
{
	const TCHAR* const AudioDir = TEXT("/Game/Take/Audio/Script/S8/");

	TSoftObjectPtr<USoundBase> S8Sound(const TCHAR* Name)
	{
		return TSoftObjectPtr<USoundBase>(FSoftObjectPath(FString::Printf(TEXT("%s%s.%s"), AudioDir, Name, Name)));
	}

	float Smooth(float A) { A = FMath::Clamp(A, 0.f, 1.f); return A * A * (3.f - 2.f * A); }

	const FLinearColor Steel(0.16f, 0.19f, 0.18f);
	const FLinearColor Navy(0.05f, 0.06f, 0.10f);
	const FLinearColor GuardSkin(0.55f, 0.42f, 0.34f);
	const FLinearColor Gun(0.02f, 0.02f, 0.02f);

	USceneComponent* MakeGuard(AActor* Owner, FName Name, USceneComponent* Parent)
	{
		USceneComponent* G = Owner->CreateDefaultSubobject<USceneComponent>(Name);
		G->SetupAttachment(Parent);
		const FString N = Name.ToString();
		FTakeShapes::CreateShape(Owner, *(N + TEXT("_Body")), ETakeShape::Cylinder, G, FVector(0.f, 0.f, 80.f),
			FVector(0.5f, 0.5f, 1.6f));
		FTakeShapes::CreateShape(Owner, *(N + TEXT("_Head")), ETakeShape::Sphere, G, FVector(0.f, 0.f, 172.f),
			FVector(0.26f));
		FTakeShapes::CreateShape(Owner, *(N + TEXT("_Arm")), ETakeShape::Cube, G, FVector(28.f, 16.f, 128.f),
			FVector(0.56f, 0.1f, 0.1f));
		FTakeShapes::CreateShape(Owner, *(N + TEXT("_Gun")), ETakeShape::Cube, G, FVector(60.f, 16.f, 134.f),
			FVector(0.2f, 0.05f, 0.1f));
		return G;
	}

	void PaintGuard(USceneComponent* G)
	{
		TArray<USceneComponent*> Kids;
		G->GetChildrenComponents(false, Kids);
		for (USceneComponent* K : Kids)
		{
			UStaticMeshComponent* M = Cast<UStaticMeshComponent>(K);
			if (!M) { continue; }
			const FString N = M->GetName();
			FTakeShapes::SetColour(M, N.EndsWith(TEXT("_Head")) ? GuardSkin : N.EndsWith(TEXT("_Gun")) ? Gun : Navy);
		}
	}

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
}

AScene8Alley::AScene8Alley()
{
	SceneNumber = 8;
	Pov = EScriptPov::Vance;
	Slug = NSLOCTEXT("Scene8", "Slug", "EXT. REAR ALLEYWAY - NIGHT");
	bPlayerWalks = true;
	CarClass = AScene8Car::StaticClass();
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

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// The fire exit: a 120 x 230 steel door hinged at -X, the black doorway behind it, the green EXIT sign above.
	Doorway = FTakeShapes::CreateShape(this, TEXT("Doorway"), ETakeShape::Cube, Root, FVector(0.f, -1.f, 116.f),
		FVector(1.26f, 0.02f, 2.34f), FRotator::ZeroRotator, false);
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

	Guard0 = MakeGuard(this, TEXT("Guard0"), Root);
	Guard1 = MakeGuard(this, TEXT("Guard1"), Root);

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
	MuzzleLight = MakeLight(TEXT("MuzzleLight"), 900.f, FLinearColor(1.f, 0.75f, 0.4f));
	SparkLight = MakeLight(TEXT("SparkLight"), 350.f, FLinearColor(1.f, 0.8f, 0.5f));
	PassingLight = MakeLight(TEXT("PassingLight"), 900.f, FLinearColor(1.f, 0.62f, 0.25f));
	MuzzleFlash = FTakeShapes::CreateShape(this, TEXT("MuzzleFlash"), ETakeShape::Sphere, Root, FVector::ZeroVector,
		FVector(0.22f), FRotator::ZeroRotator, false);
	MuzzleFlash->SetVisibility(false);
}

void AScene8Alley::BeginPlay()
{
	Super::BeginPlay();
	FTakeShapes::SetColour(Door, Steel);
	FTakeShapes::SetColour(Doorway, FLinearColor(0.004f, 0.004f, 0.004f));
	FTakeShapes::SetColour(ExitSign, FLinearColor(0.1f, 0.9f, 0.3f));
	FTakeShapes::SetColour(MuzzleFlash, FLinearColor(1.f, 0.85f, 0.5f));
	PaintGuard(Guard0);
	PaintGuard(Guard1);

	UWorld* World = GetWorld();
	if (!World) { return; }
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	P.Owner = this;
	UClass* CC = CarClass ? CarClass.Get() : AScene8Car::StaticClass();
	Car = World->SpawnActor<AScene8Car>(CC, ArrivePath->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World),
		FRotator::ZeroRotator, P);
	WideCam = SpawnCam(World, W(WideCamAt), W(WideCamLook), 55.f);
	DriveCam = SpawnCam(World, W(DriveCamAt), W(FVector(1200.f, -500.f, 80.f)), 50.f);
	MirrorCam = SpawnCam(World, GetActorLocation(), GetActorLocation() + FVector(1.f, 0.f, 0.f), MirrorFov);
	SceneCamera = WideCam;
	if (!StartSpot)
	{
		StartSpot = World->SpawnActor<ATargetPoint>(W(VanceStart), FRotator(0.f, GetActorRotation().Yaw - 90.f, 0.f), P);
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
		C->SetActorHiddenInGame(false);
		C->SetActorEnableCollision(true);
		if (UCharacterMovementComponent* M = C->GetCharacterMovement())
		{
			if (M->MovementMode == MOVE_None) { M->SetMovementMode(MOVE_Walking); }
		}
	}
}

void AScene8Alley::ResetScene()
{
	Beat = EScene8Beat::Done;
	BeatT = 0.f;
	NextShot = 0.f;
	ShotCount = 0;
	MuzzleAge = SparkAge = 1.f;
	bMaxSaid = bColeIn = bTie = bPrint = bArrived = false;
	DoorHinge->SetRelativeRotation(FRotator::ZeroRotator);
	Guard0->SetRelativeLocation(FVector(0.f, 30.f, 0.f));
	Guard1->SetRelativeLocation(FVector(0.f, 30.f, 0.f));
	Guard0->SetVisibility(false, true);
	Guard1->SetVisibility(false, true);
	MuzzleLight->SetIntensity(0.f);
	SparkLight->SetIntensity(0.f);
	PassingLight->SetIntensity(0.f);
	MuzzleFlash->SetVisibility(false);
	if (Interior) { Interior->Stop(); Interior = nullptr; }
	if (Car)
	{
		Car->SetActorHiddenInGame(false);
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
	if (ATakeCharacter* Cole = GetCole())
	{
		Cole->TeleportTo(W(ColeStart), FRotator(0.f, GetActorRotation().Yaw - 90.f, 0.f), false, true);
	}
	Enter(EScene8Beat::Burst);
}

void AScene8Alley::EndScene()
{
	if (Interior) { Interior->Stop(); Interior = nullptr; }
	PassingLight->SetIntensity(0.f);
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
	case EScene8Beat::Burst:
		PlayAt(DoorKickSound, GetActorLocation() + FVector(0.f, 0.f, 120.f));
		break;
	case EScene8Beat::Run:
		CutTo(EScriptPov::Vance, nullptr);
		if (Vance && Car)
		{
			const FVector To = Car->GetPassengerDoorWorld() - Vance->GetActorLocation();
			Vance->SetLook(To.Rotation().Yaw, -8.f);
			Vance->SnapFollowCamera();
		}
		NextShot = GuardsAt + 0.25f;
		break;
	case EScene8Beat::Dive:
		if (Vance)
		{
			Vance->SetActorHiddenInGame(true);
			Vance->SetActorEnableCollision(false);
			if (UCharacterMovementComponent* M = Vance->GetCharacterMovement()) { M->DisableMovement(); }
		}
		if (Car) { Car->SetRiders(true, true); }
		if (ATakeCharacter* Cole = GetCole())
		{
			Cole->SetActorHiddenInGame(true);
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
		SeatVance();
		if (Car) { Car->SetRiders(false, true); }
		if (USoundBase* L = Load(InteriorLoop)) { Interior = UGameplayStatics::SpawnSound2D(this, L, 0.8f); }
		CutTo(EScriptPov::Vance, MirrorCam);
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
	Vance->SetActorHiddenInGame(false);
	const float Half = Vance->GetCapsuleComponent() ? Vance->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	const FVector Seat = Car->GetPassengerSeatWorld();
	Vance->SetActorLocationAndRotation(Seat + FVector(0.f, 0.f, Half - SeatDrop),
		FRotator(0.f, Car->GetActorRotation().Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	// The rear-view mirror looks back at his face (about 70 cm over the hips).
	const FVector Face = Seat + FVector(0.f, 0.f, 72.f);
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
	const bool bFirst = (ShotCount % 2) == 0;
	USceneComponent* G = bFirst ? Guard0 : Guard1;
	++ShotCount;
	const FVector Muzzle = G->GetComponentTransform().TransformPosition(FVector(72.f, 16.f, 134.f));
	MuzzleFlash->SetWorldLocation(Muzzle);
	MuzzleFlash->SetVisibility(true);
	MuzzleLight->SetWorldLocation(Muzzle);
	MuzzleLight->SetIntensity(4000.f);
	MuzzleAge = 0.f;
	if (GunshotSounds.Num() > 0) { PlayAt(GunshotSounds[FMath::RandRange(0, GunshotSounds.Num() - 1)], Muzzle); }

	// The spark: on the brick beside the target, Miss cm along the wall, never on him.
	const FTransform T = GetActorTransform();
	const FVector Local = T.InverseTransformPosition(Target);
	const bool bNear = FMath::FRand() < 0.6f;
	const float Side = FMath::RandBool() ? 1.f : -1.f;
	const FVector Hit(Local.X + Side * Miss * FMath::FRandRange(0.7f, 1.2f), bNear ? NearWallY - 2.f : FarWallY + 2.f,
		FMath::FRandRange(50.f, 210.f));
	const FVector HitW = T.TransformPosition(Hit);
	const FRotator Out = T.TransformRotation(FRotator(0.f, bNear ? -90.f : 90.f, 0.f).Quaternion()).Rotator();
	if (UNiagaraSystem* Sys = SparkSystem.IsNull() ? nullptr : SparkSystem.LoadSynchronous())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Sys, HitW, Out, FVector(0.6f));
	}
	SparkLight->SetWorldLocation(HitW + Out.Vector() * 15.f);
	SparkLight->SetIntensity(1500.f);
	SparkAge = 0.f;
	if (RicochetSounds.Num() > 0) { PlayAt(RicochetSounds[FMath::RandRange(0, RicochetSounds.Num() - 1)], HitW, 0.8f); }
}

void AScene8Alley::TickScene(float DeltaSeconds)
{
	BeatT += DeltaSeconds;
	ATakeCharacter* Vance = GetPovCharacter();
	ATakeCharacter* Cole = GetCole();

	// Flashes die in a few frames.
	MuzzleAge += DeltaSeconds;
	SparkAge += DeltaSeconds;
	MuzzleLight->SetIntensity(MuzzleAge < 0.06f ? 4000.f : 0.f);
	MuzzleFlash->SetVisibility(MuzzleAge < 0.05f);
	SparkLight->SetIntensity(FMath::Max(0.f, 1500.f * (1.f - SparkAge / 0.12f)));

	// The car's run in: from the burst until it stops by the door.
	if (Beat == EScene8Beat::Burst || Beat == EScene8Beat::Run)
	{
		const float T = GetSceneSeconds() - ArriveDelay;
		if (T >= 0.f && !bArrived && Car)
		{
			const float A = FMath::Clamp(T / ArriveSeconds, 0.f, 1.f);
			const float Ease = 1.f - FMath::Pow(1.f - A, 2.2f);
			const float Drift = DriftDeg * FMath::Sin(PI * FMath::Clamp((A - 0.15f) / 0.85f, 0.f, 1.f));
			PoseCar(ArrivePath, Ease, Drift);
			Car->SetFlare(FMath::Max(0.f, 1.f - FMath::Abs(A - 0.3f) / 0.2f));
			Car->SetDoorOpen(Smooth((A - 0.85f) / 0.15f));
			if (T < DeltaSeconds + KINDA_SMALL_NUMBER) { PlayAt(CarArriveSound, W(FVector(-1500.f, -430.f, 80.f)), 1.2f); }
			if (!bMaxSaid && A > 0.78f)
			{
				bMaxSaid = true;
				Say(TEXT("Max"), TEXT("Get in! Get in!"), Load(MaxLine));
			}
			bArrived = A >= 1.f;
		}
	}

	switch (Beat)
	{
	case EScene8Beat::Burst:
	{
		// The door bangs open in a tenth of a second and bounces; they stumble out.
		const float Swing = BeatT < 0.12f ? BeatT / 0.12f : 1.f - 0.06f * FMath::Sin(FMath::Min(1.f, (BeatT - 0.12f) / 0.4f) * PI);
		DoorHinge->SetRelativeRotation(FRotator(0.f, -100.f * Swing, 0.f));
		if (BeatT < 0.9f)
		{
			if (Vance) { Vance->MoveToward(W(VanceStumble)); }
			if (Cole) { Cole->MoveToward(W(ColeStumble)); }
		}
		if (BeatT >= BurstSeconds) { Enter(EScene8Beat::Run); }
		break;
	}
	case EScene8Beat::Run:
	{
		// Cole makes for the back seat on his own.
		if (Cole && Car && !bColeIn)
		{
			const FVector Back = Car->GetActorTransform().TransformPosition(FVector(-120.f, 110.f, 0.f));
			if (FVector::Dist2D(Cole->GetFeetLocation(), Back) < 120.f)
			{
				bColeIn = true;
				Cole->SetActorHiddenInGame(true);
				Cole->SetActorEnableCollision(false);
				Car->SetRiders(false, true);
			}
			else { Cole->MoveToward(Back); }
		}
		// The guards burst out of the exit and take their marks.
		if (BeatT >= GuardsAt)
		{
			const float A = Smooth((BeatT - GuardsAt) / 0.4f);
			if (!Guard0->IsVisible())
			{
				Guard0->SetVisibility(true, true);
				Guard1->SetVisibility(true, true);
				Say(TEXT("Guard"), TEXT("Stop them!"), Load(GuardLine));
			}
			Guard0->SetRelativeLocation(FMath::Lerp(FVector(-20.f, 20.f, 0.f), GuardMark0, A));
			Guard1->SetRelativeLocation(FMath::Lerp(FVector(20.f, 20.f, 0.f), GuardMark1, A));
			if (Vance)
			{
				for (USceneComponent* G : { Guard0.Get(), Guard1.Get() })
				{
					const FVector To = Vance->GetActorLocation() - G->GetComponentLocation();
					G->SetWorldRotation(FRotator(0.f, To.Rotation().Yaw, 0.f));
				}
			}
		}
		if (Vance && BeatT >= NextShot)
		{
			const float Closing = FMath::Clamp(BeatT / RunSeconds, 0.f, 1.f);
			Shoot(Vance->GetActorLocation(), FMath::Lerp(MissStart, MissEnd, Closing));
			NextShot = BeatT + FMath::FRandRange(ShotGapMin, ShotGapMax) * FMath::Lerp(1.f, 0.6f, Closing);
		}
		if (Vance && Car && bArrived && FVector::Dist2D(Vance->GetFeetLocation(), Car->GetPassengerDoorWorld()) < ReachRadius)
		{
			Enter(EScene8Beat::Dive);
		}
		else if (BeatT >= RunSeconds)
		{
			FailScene(TEXT("PinnedDown"));
		}
		break;
	}
	case EScene8Beat::Dive:
	{
		if (Car) { Car->SetDoorOpen(1.f - Smooth(BeatT / 0.25f)); }
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
		// The car is parked: the street lamps sweep past it and the mirror trembles with the road.
		if (Car && PassingLight)
		{
			const float Cycle = FMath::Fmod(BeatT, 0.9f) / 0.9f;
			PassingLight->SetWorldLocation(Car->GetActorTransform().TransformPosition(
				FVector(FMath::Lerp(500.f, -500.f, Cycle), 220.f, 320.f)));
			PassingLight->SetIntensity(180.f * FMath::Sin(PI * Cycle));
		}
		if (MirrorCam && Car && Vance)
		{
			const FVector Face = Car->GetPassengerSeatWorld() + FVector(0.f, 0.f, 72.f);
			const FVector Shake(0.f, FMath::Sin(BeatT * 31.f) * 0.35f, FMath::Sin(BeatT * 23.f) * 0.5f);
			const FVector At = Car->GetMirrorWorld() + Shake;
			MirrorCam->SetActorLocationAndRotation(At, (Face - At).Rotation());
		}
		if (!bTie && BeatT >= TieAt)
		{
			bTie = true;
			if (Vance) { Vance->PlayGesture(TEXT("Headset")); }	// hand to the collar: the tie
		}
		if (!bPrint && BeatT >= PrintAt)
		{
			bPrint = true;
			Say(TEXT("Vance"), TEXT("Cut. Print it."), Load(VanceLine));
		}
		if (BeatT >= MirrorSeconds) { Enter(EScene8Beat::Done); }
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
	if (Beat == EScene8Beat::Run)
	{
		// The goal and the fire closing in: a bar that drains over RunSeconds.
		const float Left = FMath::Clamp(1.f - BeatT / RunSeconds, 0.f, 1.f);
		const float BW = Kit.Px(420.f), BH = Kit.Px(8.f);
		const float Y = Kit.PicBottom() - Kit.Px(70.f);
		Kit.Text(TakeStyle::EType::Goal, TEXT("Get in the car"), Kit.CX(), Y - Kit.Px(30.f), TakeStyle::Paper,
			ETakeAlign::Centre);
		Kit.Rect(Kit.CX() - BW * 0.5f, Y, BW, BH, FTakeHudKit::A(TakeStyle::Ink, 0.7f));
		Kit.Rect(Kit.CX() - BW * 0.5f, Y, BW * Left, BH, Left < 0.3f ? TakeStyle::Danger : TakeStyle::Julian);
	}
	else if (Beat == EScene8Beat::Mirror)
	{
		// The rear-view mirror: everything but its oblong goes dark, a chrome rim round it.
		const float MW = Kit.W * 0.6f, MH = Kit.H * 0.34f;
		const float X0 = Kit.CX() - MW * 0.5f, Y0 = Kit.H * 0.5f - MH * 0.5f;
		const FLinearColor Dark(0.f, 0.f, 0.f, 0.92f);
		Kit.Rect(0.f, 0.f, Kit.W, Y0, Dark);
		Kit.Rect(0.f, Y0 + MH, Kit.W, Kit.H - Y0 - MH, Dark);
		Kit.Rect(0.f, Y0, X0, MH, Dark);
		Kit.Rect(X0 + MW, Y0, Kit.W - X0 - MW, MH, Dark);
		Kit.Frame(X0, Y0, MW, MH, Kit.Px(10.f), TakeStyle::Ash);
		Kit.Frame(X0 + Kit.Px(10.f), Y0 + Kit.Px(10.f), MW - Kit.Px(20.f), MH - Kit.Px(20.f), Kit.Px(2.f), TakeStyle::Ink);
	}
}
