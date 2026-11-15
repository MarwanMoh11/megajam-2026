// See Scene6Backstage.h. Owner: the s6-backstage session.
#include "Script/Scene6Backstage.h"

#include "CastMember.h"
#include "TakeCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "CanvasItem.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogScene6, Log, All);

namespace
{
	const FName TagLid(TEXT("s6_trunk_lid"));
	const FName TagLever(TEXT("s6_lever_handle"));
	const FName TagSandbag(TEXT("s6_sandbag"));
	const FName TagSheet(TEXT("s6_sheet"));
	const FName TagDoor(TEXT("s6_door_leaf"));

	float Ease(float T) { T = FMath::Clamp(T, 0.f, 1.f); return T * T * (3.f - 2.f * T); }
	float Yaw2D(const FVector& V) { return static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(V.Y, V.X))); }
}

AScene6Backstage::AScene6Backstage()
{
	SceneNumber = 6;
	Pov = EScriptPov::Cole;
	Slug = FText::FromString(TEXT("INT. BACKSTAGE CORRIDOR & RIGGING - CONTINUOUS"));
	bPlayerWalks = true;

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

	LockLamp = CreateDefaultSubobject<UPointLightComponent>(TEXT("LockLamp"));
	LockLamp->SetupAttachment(Root);
	LockLamp->SetIntensityUnits(ELightUnits::Candelas);
	LockLamp->SetIntensity(40.f);
	LockLamp->SetAttenuationRadius(220.f);
	LockLamp->SetLightColor(FLinearColor(1.f, 0.05f, 0.03f));
	LockLamp->SetCastShadows(false);
}

void AScene6Backstage::BeginPlay()
{
	Super::BeginPlay();
	LockLamp->SetWorldLocation(PanelPoint + FVector(0.f, -25.f, 10.f));
	if (UMaterialInterface* Film = FilmMaterial.LoadSynchronous())
	{
		FilmMID = UMaterialInstanceDynamic::Create(Film, this);
		ScreenBack->SetMaterial(0, FilmMID);
		FilmMID->SetScalarParameterValue(TEXT("Play"), 1.f);
		FilmMID->SetScalarParameterValue(TEXT("Brightness"), FilmBrightness);
		FilmMID->SetScalarParameterValue(TEXT("T0"), GetWorld()->GetTimeSeconds());
		FilmMID->SetScalarParameterValue(TEXT("Aspect"), 1000.f / 480.f);
	}
	FindProps();
	SpawnCast();
	ResetScene();
}

// ---- the set -------------------------------------------------------------------------------------------------------

ACastMember* AScene6Backstage::SpawnOne(const FVector& At, float Yaw, uint8 Kit, const FString& Label, const FLinearColor& Tint)
{
	const FTransform Xf(FRotator(0.f, Yaw, 0.f), At);
	ACastMember* M = GetWorld()->SpawnActorDeferred<ACastMember>(ACastMember::StaticClass(), Xf, this, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!M)
	{
		return nullptr;
	}
	M->Kit = static_cast<ECastKit>(Kit);
	M->Tint = Tint;
	M->Tags.Add(TEXT("script_s6"));
#if WITH_EDITOR
	M->SetActorLabel(Label);
#endif
	M->FinishSpawning(Xf);
	return M;
}

void AScene6Backstage::SpawnCast()
{
	if (bSpawned)
	{
		return;
	}
	bSpawned = true;
	A.Name = TEXT("ENFORCER");
	A.Home = PatrolA0;
	A.HomeYaw = Yaw2D(PatrolA1 - PatrolA0);
	A.Range = 800.f;
	A.HalfAngle = 40.f;
	A.Body = SpawnOne(A.Home, A.HomeYaw, (uint8)ECastKit::Bodyguard, TEXT("S6_Enforcer_A"), FLinearColor(0.035f, 0.035f, 0.04f));

	B.Name = TEXT("ENFORCER");
	B.Home = PostB;
	B.HomeYaw = -90.f;	// down the stairs
	B.Range = 1000.f;
	B.HalfAngle = 25.f;
	B.Body = SpawnOne(B.Home, B.HomeYaw, (uint8)ECastKit::Bodyguard, TEXT("S6_Enforcer_B"), FLinearColor(0.05f, 0.04f, 0.04f));

	P.Name = TEXT("PROJECTIONIST");
	P.Home = PostProjectionist;
	P.HomeYaw = 90.f;	// facing the booth door
	P.Range = 750.f;
	P.HalfAngle = 38.f;
	P.Body = SpawnOne(P.Home, P.HomeYaw, (uint8)ECastKit::Staff, TEXT("S6_Projectionist"), FLinearColor(0.24f, 0.2f, 0.15f));
	UE_LOG(LogScene6, Log, TEXT("S6 cast: A %s B %s P %s"), A.Body ? TEXT("ok") : TEXT("none"),
		B.Body ? TEXT("ok") : TEXT("none"), P.Body ? TEXT("ok") : TEXT("none"));
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
		else if (Act->ActorHasTag(TagDoor)) { DoorLeaf = Act; DoorHome = Act->GetActorTransform(); }
	}
	for (AActor* Act : { TrunkLid.Get(), LeverHandle.Get(), Sandbag.Get(), Sheet.Get(), DoorLeaf.Get() })
	{
		if (Act)
		{
			if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Act->GetRootComponent()))
			{
				Prim->SetMobility(EComponentMobility::Movable);
			}
		}
	}
	UE_LOG(LogScene6, Log, TEXT("S6 props: lid %d lever %d sandbag %d sheet %d door %d"), TrunkLid != nullptr,
		LeverHandle != nullptr, Sandbag != nullptr, Sheet != nullptr, DoorLeaf != nullptr);
}

void AScene6Backstage::PlaceGuardsHome()
{
	for (FS6Guard* G : { &A, &B, &P })
	{
		G->Alert = 0.f;
		G->bDown = false;
		if (ACastMember* M = G->Body)
		{
			M->StopWalking();
			M->SetActorHiddenInGame(false);
			M->SetActorLocationAndRotation(G->Home, FRotator(0.f, G->HomeYaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			M->FaceYaw(G->HomeYaw);
			M->PlayGesture(NAME_None);
		}
	}
	B.Range = 1000.f;
	B.HalfAngle = 25.f;
}

void AScene6Backstage::ResetScene()
{
	Step = EStep::Patrol;
	StepAge = 0.f;
	PlaceGuardsHome();
	PatrolLeg = 1;
	PauseLeft = 0.8f;
	DownAge = PDownAge = -1.f;
	BRoute.Reset();
	BRouteIndex = 0;
	BLookLeft = 0.f;
	bBReturning = false;
	GlanceClock = 0.f;
	bGlancing = false;
	CaughtReason = NAME_None;
	CaughtAge = 0.f;
	LeverAge = CrashAge = DoorAge = LidAge = -1.f;
	bLeverPulled = bUnlocked = false;
	FlashLeft = 0.f;
	LastLoudCycle = -1;
	ToastLeft = 0.f;
	if (TrunkLid) TrunkLid->SetActorTransform(LidHome);
	if (LeverHandle) LeverHandle->SetActorTransform(LeverHome);
	if (Sandbag) Sandbag->SetActorTransform(SandbagHome);
	if (Sheet) Sheet->SetActorTransform(SheetHome);
	if (DoorLeaf) DoorLeaf->SetActorTransform(DoorHome);
	LockLamp->SetLightColor(FLinearColor(1.f, 0.05f, 0.03f));
	Objective = TEXT("Take out the patrolling enforcer from behind - only while the film is LOUD");
}

void AScene6Backstage::BeginScene()
{
	Super::BeginScene();
	ResetScene();
	SayCue(TEXT("COLE"), TEXT("Two heavies blocking the stairs. Going dark."), ColeOpenLine);
	if (GetRetakes() >= 2)
	{
		Flash(TEXT("Follow him while his back is turned. Press E when the screen goes LOUD."), 5.f);
	}
	UE_LOG(LogScene6, Log, TEXT("S6 begin (retake %d)"), GetRetakes());
}

void AScene6Backstage::EndScene()
{
	Super::EndScene();
	for (FS6Guard* G : { &A, &B, &P })
	{
		if (G->Body)
		{
			G->Body->StopWalking();
		}
	}
}

// ---- the film's clock ---------------------------------------------------------------------------------------------

float AScene6Backstage::FilmT() const
{
	return FMath::Fmod(GetSceneSeconds() + FilmOffset, FMath::Max(FilmPeriod, 1.f));
}

bool AScene6Backstage::IsLoud() const { return FilmT() >= LoudStart; }

bool AScene6Backstage::IsBuilding() const
{
	const float T = FilmT();
	return T >= LoudStart - BuildSeconds && T < LoudStart;
}

bool AScene6Backstage::CycleIsExplosion() const
{
	const int32 Cycle = FMath::FloorToInt((GetSceneSeconds() + FilmOffset) / FMath::Max(FilmPeriod, 1.f));
	return (Cycle % 2) == 1;
}

void AScene6Backstage::StepFilm(float Dt)
{
	const int32 Cycle = FMath::FloorToInt((GetSceneSeconds() + FilmOffset) / FMath::Max(FilmPeriod, 1.f));
	if (IsLoud() && Cycle != LastLoudCycle)
	{
		LastLoudCycle = Cycle;
		if (CycleIsExplosion())
		{
			Play(ExplosionCues, nullptr, 1.f);
			FlashColor = FLinearColor(1.f, 0.55f, 0.2f);
		}
		else
		{
			Play(GunfightCues, nullptr, 1.f);
			FlashColor = FLinearColor(1.f, 0.95f, 0.85f);
		}
		FlashLeft = FilmPeriod - LoudStart;
	}
	const float Window = FilmPeriod - LoudStart;
	if (!CycleIsExplosion() && FlashLeft > Window - 1.4f && FlashLeft - Dt <= Window - 1.4f)
	{
		Play(GunfightCues, nullptr, 0.9f);	// the second burst: the gunfight covers the whole window
	}
	FlashLeft = FMath::Max(0.f, FlashLeft - Dt);
	const float Now = GetSceneSeconds();
	float Flash = 0.f;
	if (FlashLeft > 0.f)
	{
		if (CycleIsExplosion())
		{
			const float Age = (FilmPeriod - LoudStart) - FlashLeft;
			Flash = 4.f * FMath::Exp(-2.2f * Age) + 0.6f;	// one big bloom, then the fire
		}
		else
		{
			Flash = (FMath::Fmod(Now * 11.f, 1.f) < 0.35f) ? 2.6f : 0.25f;	// muzzle flashes
		}
	}
	const float Noise = 0.75f + 0.15f * FMath::Sin(Now * 23.f) + 0.1f * FMath::Sin(Now * 61.f + 1.3f);
	FlickerLight->SetIntensity(FlickerCandelas * (Noise + Flash));
	FlickerLight->SetLightColor(Flash > 0.5f ? FlashColor : FLinearColor(0.72f, 0.8f, 1.f));
	if (FilmMID)
	{
		FilmMID->SetScalarParameterValue(TEXT("Brightness"), FilmBrightness * (Noise + 0.6f * Flash));
	}
}

// ---- the people ----------------------------------------------------------------------------------------------------

float AScene6Backstage::FloorZ(const FVector& At) const
{
	const bool bInStairX = At.X >= StairX.X - 40.f && At.X <= StairX.Y + 40.f;
	if (bInStairX && At.Y >= StairY.X && At.Y <= StairY.Y)
	{
		return StairTopZ * (At.Y - StairY.X) / FMath::Max(StairY.Y - StairY.X, 1.f);
	}
	if (At.Z > StairTopZ * 0.5f && At.Y > StairY.Y && At.X >= LandingX.X && At.X <= LandingX.Y + 40.f)
	{
		return StairTopZ;
	}
	return 0.f;
}

void AScene6Backstage::StepPatrolA(float Dt)
{
	ACastMember* M = A.Body;
	if (!M || A.bDown)
	{
		return;
	}
	if (PauseLeft > 0.f)
	{
		PauseLeft -= Dt;
		if (PauseLeft <= 0.f)
		{
			M->WalkTo(PatrolLeg == 1 ? PatrolA1 : PatrolA0, PatrolSpeed);
		}
		return;
	}
	if (!M->IsWalking())
	{
		PauseLeft = PatrolPause;
		PatrolLeg = 1 - PatrolLeg;
		if (FMath::RandBool())
		{
			M->PlayGesture(TEXT("LookAround"));
		}
	}
}

void AScene6Backstage::StepLure(float Dt)
{
	ACastMember* M = B.Body;
	if (!M || B.bDown || BRoute.Num() == 0)
	{
		return;
	}
	FVector L = M->GetActorLocation();
	L.Z = FloorZ(L);
	M->SetActorLocation(L);
	if (BLookLeft > 0.f)
	{
		BLookLeft -= Dt;
		if (BLookLeft <= 0.f)
		{
			// Back to his post, the way he came.
			bBReturning = true;
			BRoute = { FVector(StairX.X - 140.f, StairY.X - 80.f, 0.f), FVector(0.5f * (StairX.X + StairX.Y), StairY.X - 80.f, 0.f), FVector(PostB.X, PostB.Y, 0.f) };
			BRouteIndex = 0;
			M->WalkTo(BRoute[0], LureSpeed * 0.7f);
		}
		return;
	}
	if (M->IsWalking())
	{
		return;
	}
	++BRouteIndex;
	if (BRouteIndex < BRoute.Num())
	{
		M->WalkTo(BRoute[BRouteIndex], bBReturning ? LureSpeed * 0.7f : LureSpeed);
		return;
	}
	if (!bBReturning)
	{
		BLookLeft = LureLookSeconds;
		M->FaceYaw(Yaw2D(SheetPoint - L));
		M->LookAtPoint(SheetPoint + FVector(0.f, 0.f, 120.f), LureLookSeconds);
		M->PlayGesture(TEXT("LookAround"));
	}
	else
	{
		BRoute.Reset();
		M->FaceYaw(B.HomeYaw);
		B.Range = 1000.f;
		B.HalfAngle = 25.f;
	}
}

void AScene6Backstage::StepProjectionist(float Dt)
{
	ACastMember* M = P.Body;
	if (!M || P.bDown)
	{
		return;
	}
	GlanceClock += Dt;
	if (!bGlancing && GlanceClock >= GlanceEvery)
	{
		bGlancing = true;
		GlanceClock = 0.f;
		M->FaceYaw(P.HomeYaw + 180.f);
	}
	else if (bGlancing && GlanceClock >= GlanceHold + 1.1f)	// + the turn
	{
		bGlancing = false;
		GlanceClock = 0.f;
		M->FaceYaw(P.HomeYaw);
	}
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

bool AScene6Backstage::IsBehind(const FS6Guard& G, const FVector& ColeAt) const
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

bool AScene6Backstage::CanSee(const FS6Guard& G, const FVector& ColeAt) const
{
	const ACastMember* M = G.Body;
	if (!M || G.bDown)
	{
		return false;
	}
	const FVector At = M->GetActorLocation();
	FVector To = ColeAt - At;
	// A floor apart, A and the projectionist don't look up or down; B at the top of the stair looks down it.
	const float MaxDz = (&G == &B) ? 420.f : 170.f;
	if (FMath::Abs(To.Z) > MaxDz)
	{
		return false;
	}
	To.Z = 0.f;
	const float Dist = static_cast<float>(To.Size());
	if (Dist > G.Range)
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
	for (const FS6Guard* O : { &A, &B, &P })
	{
		if (O->Body) Q.AddIgnoredActor(O->Body);
	}
	if (const AActor* Cole = GetPovCharacter()) Q.AddIgnoredActor(Cole);
	FHitResult Hit;
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, At + FVector(0.f, 0.f, 165.f),
		ColeAt + FVector(0.f, 0.f, 150.f), ECC_Visibility, Q);
	return !bBlocked;
}

void AScene6Backstage::StepSight(FS6Guard& G, float Dt, bool bLooking)
{
	if (G.bDown || !G.Body)
	{
		return;
	}
	const FVector Cole = ColeLocation();
	if (bLooking && CanSee(G, Cole))
	{
		const float Dist = static_cast<float>(FVector::Dist2D(Cole, G.Body->GetActorLocation()));
		const float Speed = FMath::Lerp(1.8f, 0.7f, FMath::Clamp(Dist / G.Range, 0.f, 1.f));
		G.Alert += Dt * Speed / FMath::Max(SpotSeconds, 0.1f);
		if (G.Alert >= 1.f)
		{
			Catch(G, TEXT("SPOTTED BACKSTAGE"));
		}
	}
	else
	{
		G.Alert = FMath::Max(0.f, G.Alert - Dt * 0.8f);
	}
}

void AScene6Backstage::Catch(FS6Guard& G, FName Reason)
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
	if (&G == &P) { Say(G.Name, TEXT("Hey! Who are you?")); }
	else { SayCue(G.Name, TEXT("Hey! You!"), TArray<FName>{ TEXT("vo_s6_enforcer_hey") }); }
	UE_LOG(LogScene6, Log, TEXT("S6 caught by %s: %s"), *G.Name, *Reason.ToString());
}

void AScene6Backstage::KnockDown(FS6Guard& G)
{
	G.bDown = true;
	G.Alert = 0.f;
	if (ACastMember* M = G.Body)
	{
		M->StopWalking();
		M->PlayGesture(NAME_None);
		M->FaceYaw(static_cast<float>(M->GetActorRotation().Yaw));
	}
	if (ATakeCharacter* Cole = Cast<ATakeCharacter>(GetPovCharacter()))
	{
		if (G.Body)
		{
			Cole->PlayGestureAt(TEXT("Use"), G.Body->GetActorLocation() + FVector(0.f, 0.f, 140.f));
		}
	}
	FVector At = G.Body ? G.Body->GetActorLocation() : GetActorLocation();
	Play(TakedownCues, &At, 0.8f);
}

// ---- use -----------------------------------------------------------------------------------------------------------

bool AScene6Backstage::TryUse()
{
	const FVector Cole = ColeLocation();
	auto Near = [&](const FVector& At, float Reach)
	{
		return FVector::Dist2D(Cole, At) <= Reach && FMath::Abs(Cole.Z - At.Z) < 120.f;
	};
	// Enforcer A: the sound-masked takedown.
	if (!A.bDown && A.Body && Near(A.Body->GetActorLocation(), TakedownReach))
	{
		if (!IsBehind(A, Cole))
		{
			Catch(A, TEXT("FACE TO FACE"));
		}
		else if (!IsLoud())
		{
			Flash(TEXT("Too quiet - he heard you. Wait for the film's noise."), 3.f);
			Catch(A, TEXT("HE HEARD YOU"));
		}
		else
		{
			KnockDown(A);
			Step = EStep::Dragging;
			StepAge = 0.f;
			DragFrom = A.Body->GetActorLocation();
			if (const AActor* C = GetPovCharacter()) ColeDragFrom = C->GetActorLocation();
			DownAge = 0.f;
			Objective = TEXT("");
		}
		return true;
	}
	// The projectionist: from behind, while he faces the door.
	if (!P.bDown && P.Body && Near(P.Body->GetActorLocation(), TakedownReach))
	{
		if (bGlancing || !IsBehind(P, Cole))
		{
			Catch(P, TEXT("THE PROJECTIONIST SAW YOU"));
		}
		else
		{
			KnockDown(P);
			PDownAge = 0.f;
			Objective = TEXT("Hit the mag-lock override by the booth door");
		}
		return true;
	}
	// The counterweight lever.
	if (!bLeverPulled && Near(LeverPoint, UseReach))
	{
		if (!A.bDown || Step == EStep::Dragging)
		{
			Flash(TEXT("Not with the patrol still walking."), 2.f);
			return true;
		}
		bLeverPulled = true;
		LeverAge = 0.f;
		Play(LeverCues, &LeverPoint, 0.9f);
		return true;
	}
	// The mag-lock override.
	if (!bUnlocked && Near(PanelPoint - FVector(0.f, 0.f, PanelPoint.Z - StairTopZ), UseReach))
	{
		if (!P.bDown)
		{
			Flash(TEXT("The projectionist would hear it. Him first."), 2.f);
			return true;
		}
		bUnlocked = true;
		DoorAge = 0.f;
		LockLamp->SetLightColor(FLinearColor(0.1f, 1.f, 0.2f));
		Play(UnlockCues, &PanelPoint, 1.f);
		SayCue(TEXT("COLE"), TEXT("Door's open. Make it quick."), ColeDoorLine);
		Step = EStep::Done;
		StepAge = 0.f;
		Objective = TEXT("");
		return true;
	}
	return false;
}

bool AScene6Backstage::HandleSceneKey(const FKey& Key, bool bPressed)
{
	if (Step == EStep::Caught || Step == EStep::Done || Step == EStep::Dragging)
	{
		// A scripted moment: Use does nothing, movement keys still pass (key-ups too, so nothing sticks).
		return bPressed && (Key == EKeys::E || Key == EKeys::LeftMouseButton);
	}
	if (bPressed && (Key == EKeys::E || Key == EKeys::LeftMouseButton))
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
		const float Open = Step == EStep::Dragging ? Ease(LidAge / 0.35f) : 1.f - Ease(LidAge / 0.3f);
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
			if (Fall >= 1.f)
			{
				CrashAge = 0.f;
			}
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
			SayCue(TEXT("ENFORCER"), TEXT("What the hell was that?"), EnforcerHuhLine);
			if (!B.bDown && B.Body && Step != EStep::Caught)
			{
				B.Range = 800.f;
				B.HalfAngle = 40.f;
				bBReturning = false;
				BLookLeft = 0.f;
				BRoute = { FVector(0.5f * (StairX.X + StairX.Y), StairY.X - 80.f, 0.f), FVector(StairX.X - 140.f, StairY.X - 80.f, 0.f), FVector(LookoutB.X, LookoutB.Y, 0.f) };
				BRouteIndex = 0;
				// Down the stairs first: his feet keep the stair's height (FloorZ) every frame.
				B.Body->WalkTo(FVector(0.5f * (StairX.X + StairX.Y), StairY.X - 80.f, B.Body->GetActorLocation().Z), LureSpeed);
				Step = EStep::Lured;
				Objective = P.bDown ? TEXT("Hit the mag-lock override by the booth door")
					: TEXT("Up the stairs: knock out the projectionist from behind");
			}
		}
		if (Sheet)
		{
			const float Shake = FMath::Exp(-2.5f * CrashAge) * FMath::Sin(CrashAge * 48.f);
			const FQuat Q = SheetHome.GetRotation() * FQuat(FVector(1.f, 0.f, 0.f), FMath::DegreesToRadians(6.f * Shake));
			Sheet->SetActorLocationAndRotation(SheetHome.GetLocation() + SheetHome.GetRotation().RotateVector(FVector(0.f, 8.f * Shake, 0.f)), Q);
		}
	}
	// The booth door swings open.
	if (DoorAge >= 0.f)
	{
		DoorAge += Dt;
		TurnAboutEdge(DoorLeaf, DoorHome, FVector(-50.f, 0.f, 0.f), FVector(0.f, 0.f, 1.f), DoorOpenDegrees * Ease((DoorAge - 0.3f) / 0.9f));
	}
	// The projectionist crumples.
	if (PDownAge >= 0.f && P.Body)
	{
		PDownAge += Dt;
		const float T = Ease(PDownAge / 0.45f);
		const FRotator R(80.f * T, P.Body->GetActorRotation().Yaw, 0.f);
		FVector L = P.Home;
		L.X = P.Body->GetActorLocation().X;
		L.Y = P.Body->GetActorLocation().Y;
		L.Z = P.Home.Z + 12.f * T;
		P.Body->SetActorLocationAndRotation(L, R);
		if (PDownAge > 0.6f) PDownAge = -1.f;
	}
}

void AScene6Backstage::StepDrag(float Dt)
{
	ACastMember* M = A.Body;
	if (!M)
	{
		Step = EStep::Lever;
		return;
	}
	const float Tip = 0.45f;
	const float Hold = 0.25f;
	const float DragDist = static_cast<float>(FVector::Dist2D(DragFrom, TrunkPoint));
	const float DragTime = FMath::Clamp(DragDist / 300.f, 0.6f, 2.2f);
	const float T = StepAge;
	const float Yaw = static_cast<float>(M->GetActorRotation().Yaw);
	if (T <= Tip + Hold)
	{
		const float K = Ease(T / Tip);
		M->SetActorLocationAndRotation(DragFrom + FVector(0.f, 0.f, 12.f * K), FRotator(80.f * K, Yaw, 0.f));
		return;
	}
	if (LidAge < 0.f)
	{
		LidAge = 0.f;
	}
	const float K = Ease((T - Tip - Hold) / DragTime);
	FVector L = FMath::Lerp(DragFrom, FVector(TrunkPoint.X, TrunkPoint.Y, DragFrom.Z), K);
	L.Z = DragFrom.Z + 12.f + 30.f * FMath::Sin(K * UE_PI) * (K > 0.85f ? 1.f : 0.3f);
	M->SetActorLocationAndRotation(L, FRotator(80.f, Yaw, 0.f));
	if (ACharacter* Cole = GetPovCharacter())
	{
		// Cole drags him: walks backward to the trunk, a step ahead of the body.
		FVector Dir = (FVector(TrunkPoint.X, TrunkPoint.Y, 0.f) - FVector(DragFrom.X, DragFrom.Y, 0.f)).GetSafeNormal();
		const FVector ColeTo = FVector(TrunkPoint.X, TrunkPoint.Y, ColeDragFrom.Z) + FVector(110.f, 0.f, 0.f);
		FVector C = FMath::Lerp(ColeDragFrom, ColeTo, K);
		C.Z = ColeDragFrom.Z;
		Cole->SetActorLocation(C, false, nullptr, ETeleportType::TeleportPhysics);
		(void)Dir;
	}
	if (K >= 1.f)
	{
		M->SetActorHiddenInGame(true);
		LidAge = 0.f;
		Step = EStep::Lever;
		StepAge = 0.f;
		Play(TArray<FName>{ TEXT("sfx_s6_trunk"), TEXT("sfx_thud") }, &TrunkPoint, 0.7f);
		SayCue(TEXT("COLE"), TEXT("One down."), TArray<FName>{ TEXT("vo_s6_cole_onedown") });
		Objective = TEXT("Pull the counterweight lever on the west wall: draw the stair guard off");
	}
}

// ---- tick ----------------------------------------------------------------------------------------------------------

void AScene6Backstage::TickScene(float DeltaSeconds)
{
	const float Dt = FMath::Min(DeltaSeconds, 0.1f);
	StepAge += Dt;
	ToastLeft = FMath::Max(0.f, ToastLeft - Dt);
	StepFilm(Dt);
	StepProps(Dt);
	if (Step == EStep::Caught)
	{
		CaughtAge += Dt;
		if (CaughtAge >= 1.0f)
		{
			const FName Reason = CaughtReason;
			CaughtAge = -100.f;
			FailScene(Reason);
		}
		return;
	}
	if (Step == EStep::Done)
	{
		if (StepAge >= 2.8f)
		{
			StepAge = -1000.f;
			FinishScene();
		}
		return;
	}
	if (Step == EStep::Dragging)
	{
		StepDrag(Dt);
		StepProjectionist(Dt);
		return;
	}
	if (Step == EStep::Patrol)
	{
		StepPatrolA(Dt);
	}
	StepLure(Dt);
	StepProjectionist(Dt);
	StepSight(A, Dt, true);
	StepSight(B, Dt, true);
	if (Step != EStep::Caught)
	{
		const float PYaw = P.Body ? static_cast<float>(P.Body->GetActorRotation().Yaw) : 0.f;
		const bool bFacingRoom = FMath::Abs(FMath::FindDeltaAngleDegrees(PYaw, P.HomeYaw + 180.f)) < 60.f;
		StepSight(P, Dt, bFacingRoom);
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
			if (USoundBase* Snd = LoadObject<USoundBase>(nullptr, Path, {}, LOAD_NoWarn | LOAD_Quiet))
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

void AScene6Backstage::DrawSceneHud(UCanvas* Canvas)
{
	if (!Canvas || !GEngine)
	{
		return;
	}
	UFont* Big = GEngine->GetMediumFont();	// the large engine font draws boxes here
	UFont* Small = GEngine->GetMediumFont();
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const float S = FMath::Max(0.6f, H / 1080.f);
	auto Text = [&](UFont* F, const FString& Str, float X, float Y, const FLinearColor& C, float Scale, bool bCentre)
	{
		FCanvasTextItem It(FVector2D(X, Y), FText::FromString(Str), F, C);
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

	// The film's sound: the cover meter (top left).
	const float X0 = 48.f * S, Y0 = 130.f * S, BarW = 420.f * S, BarH = 16.f * S;
	const float T = FilmT();
	const bool bLoud = IsLoud(), bBuild = IsBuilding();
	float Level = 0.18f + 0.06f * FMath::Sin(GetSceneSeconds() * 9.f);
	if (bBuild) Level = FMath::Lerp(0.25f, 0.7f, (T - (LoudStart - BuildSeconds)) / FMath::Max(BuildSeconds, 0.1f));
	if (bLoud) Level = 0.85f + 0.15f * FMath::Sin(GetSceneSeconds() * 31.f);
	const FLinearColor Amber(1.f, 0.62f, 0.12f), Grey(0.75f, 0.75f, 0.75f), Red(1.f, 0.18f, 0.12f);
	FString Label = TEXT("FILM AUDIO  -  dialogue (quiet)");
	if (bBuild) Label = FString::Printf(TEXT("FILM AUDIO  -  %s in %.1f"), CycleIsExplosion() ? TEXT("EXPLOSION") : TEXT("GUNFIGHT"), LoudStart - T);
	if (bLoud) Label = FString::Printf(TEXT("FILM AUDIO  -  %s: LOUD, MOVE"), CycleIsExplosion() ? TEXT("EXPLOSION") : TEXT("GUNFIGHT"));
	Text(Small, Label, X0, Y0 - 34.f * S, bLoud ? Amber : Grey, 1.5f * S, false);
	Box(X0, Y0, BarW, BarH, FLinearColor(0.f, 0.f, 0.f, 0.55f));
	Box(X0, Y0, BarW * FMath::Clamp(Level, 0.f, 1.f), BarH, bLoud ? Amber : (bBuild ? FLinearColor(0.9f, 0.8f, 0.4f, 0.9f) : FLinearColor(0.6f, 0.6f, 0.6f, 0.8f)));
	// The threshold: above it, a takedown is covered.
	Box(X0 + BarW * 0.8f, Y0 - 4.f * S, 2.f * S, BarH + 8.f * S, FLinearColor(1.f, 1.f, 1.f, 0.8f));

	if (!Objective.IsEmpty())
	{
		Text(Small, Objective, X0, Y0 + 26.f * S, FLinearColor(0.9f, 0.9f, 0.9f), 1.35f * S, false);
	}

	// Who's onto him: ? then ! over the head.
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		for (const FS6Guard* G : { &A, &B, &P })
		{
			if (!G->Body || G->bDown || G->Alert < 0.04f || G->Body->IsHidden())
			{
				continue;
			}
			FVector2D Scr;
			if (PC->ProjectWorldLocationToScreen(G->Body->GetActorLocation() + FVector(0.f, 0.f, 215.f), Scr, true))
			{
				const bool bBang = G->Alert >= 0.6f;
				Text(Big, bBang ? TEXT("!") : TEXT("?"), Scr.X, Scr.Y - 20.f * S, bBang ? Red : FLinearColor(1.f, 0.85f, 0.2f),
					(2.2f + 2.4f * G->Alert) * S, true);
			}
		}
	}

	// The prompt (bottom centre).
	FString Prompt;
	FLinearColor PromptC = FLinearColor::White;
	if (Step != EStep::Caught && Step != EStep::Done && Step != EStep::Dragging)
	{
		const FVector Cole = ColeLocation();
		auto Near = [&](const FVector& At, float Reach) { return FVector::Dist2D(Cole, At) <= Reach && FMath::Abs(Cole.Z - At.Z) < 120.f; };
		if (!A.bDown && A.Body && Near(A.Body->GetActorLocation(), TakedownReach + 60.f))
		{
			if (!IsBehind(A, Cole)) { Prompt = TEXT("He's facing you"); PromptC = Red; }
			else if (bLoud) { Prompt = TEXT("[E]  Choke-hold - the gunfire covers it"); PromptC = Amber; }
			else { Prompt = TEXT("[E]  Choke-hold   (too quiet - wait for the noise)"); PromptC = Red; }
		}
		else if (!P.bDown && P.Body && Near(P.Body->GetActorLocation(), TakedownReach + 60.f))
		{
			if (bGlancing || !IsBehind(P, Cole)) { Prompt = TEXT("He's looking this way"); PromptC = Red; }
			else { Prompt = TEXT("[E]  Knock out the projectionist"); }
		}
		else if (!bLeverPulled && Near(LeverPoint, UseReach))
		{
			Prompt = A.bDown ? TEXT("[E]  Pull the counterweight lever") : TEXT("Counterweight lever - not with the patrol up");
			PromptC = A.bDown ? FLinearColor::White : Grey;
		}
		else if (!bUnlocked && Near(PanelPoint - FVector(0.f, 0.f, PanelPoint.Z - StairTopZ), UseReach))
		{
			Prompt = P.bDown ? TEXT("[E]  Mag-Lock Override") : TEXT("Mag-lock - the projectionist would hear it");
			PromptC = P.bDown ? FLinearColor::White : Grey;
		}
	}
	if (!Prompt.IsEmpty())
	{
		Text(Big, Prompt, W * 0.5f, H * 0.74f, PromptC, 2.0f * S, true);
	}
	if (ToastLeft > 0.f && !Toast.IsEmpty())
	{
		const float Alpha = FMath::Clamp(ToastLeft / 0.4f, 0.f, 1.f);
		Text(Big, Toast, W * 0.5f, H * 0.36f, FLinearColor(1.f, 0.9f, 0.7f, Alpha), 2.4f * S, true);
	}
}
