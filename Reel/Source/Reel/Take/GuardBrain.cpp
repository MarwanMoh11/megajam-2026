// The guards' brain (stealth-ai, Nov 15). See GuardBrain.h for the states and the scene's controls.

#include "GuardBrain.h"
#include "CastMember.h"
#include "TakeTypes.h"
#include "TakePlayerController.h"
#include "TakeCharacter.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/LocalLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectIterator.h"

static TAutoConsoleVariable<int32> CVarGuardDebug(TEXT("Reel.Guard.Debug"), 0,
	TEXT("1: draw each guard brain's sight cones, state, meter and the player's light level."));

namespace
{
	TArray<TWeakObjectPtr<UGuardBrainComponent>>& Registry()
	{
		static TArray<TWeakObjectPtr<UGuardBrainComponent>> Brains;
		return Brains;
	}

	float Yaw2D(const FVector& V)
	{
		return static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(V.Y, V.X)));
	}

	FVector YawDir(float Yaw)
	{
		const float R = FMath::DegreesToRadians(Yaw);
		return FVector(FMath::Cos(R), FMath::Sin(R), 0.f);
	}

	const float BarkGap = 1.2f;	// s between two barks (Spotted and Body cut in)
}

const TCHAR* UGuardBrainComponent::StateName(EGuardState S)
{
	switch (S)
	{
	case EGuardState::Patrol:		return TEXT("PATROL");
	case EGuardState::Suspicious:	return TEXT("SUSPICIOUS");
	case EGuardState::Investigate:	return TEXT("INVESTIGATE");
	case EGuardState::Search:		return TEXT("SEARCH");
	case EGuardState::Return:		return TEXT("RETURN");
	case EGuardState::Alert:		return TEXT("ALERT");
	case EGuardState::Caught:		return TEXT("CAUGHT");
	case EGuardState::Distracted:	return TEXT("DISTRACTED");
	case EGuardState::Downed:		return TEXT("DOWNED");
	}
	return TEXT("?");
}

static const TCHAR* EventName(EGuardEvent E)
{
	switch (E)
	{
	case EGuardEvent::Noticed:		return TEXT("Noticed");
	case EGuardEvent::HeardNoise:	return TEXT("HeardNoise");
	case EGuardEvent::Investigating:return TEXT("Investigating");
	case EGuardEvent::Searching:	return TEXT("Searching");
	case EGuardEvent::GaveUp:		return TEXT("GaveUp");
	case EGuardEvent::BackOnPatrol:	return TEXT("BackOnPatrol");
	case EGuardEvent::Spotted:		return TEXT("Spotted");
	case EGuardEvent::LostTarget:	return TEXT("LostTarget");
	case EGuardEvent::BodyFound:	return TEXT("BodyFound");
	case EGuardEvent::Caught:		return TEXT("Caught");
	case EGuardEvent::KnockedDown:	return TEXT("KnockedDown");
	}
	return TEXT("?");
}

UGuardBrainComponent::UGuardBrainComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

UGuardBrainComponent* UGuardBrainComponent::AddTo(AActor* Owner)
{
	if (!Owner)
	{
		return nullptr;
	}
	if (UGuardBrainComponent* Have = Owner->FindComponentByClass<UGuardBrainComponent>())
	{
		return Have;
	}
	UGuardBrainComponent* B = NewObject<UGuardBrainComponent>(Owner, TEXT("GuardBrain"));
	Owner->AddInstanceComponent(B);
	B->RegisterComponent();	// BeginPlay runs here when the owner has already begun play
	return B;
}

void UGuardBrainComponent::OnRegister()
{
	Super::OnRegister();
	Registry().AddUnique(this);
}

void UGuardBrainComponent::OnUnregister()
{
	Registry().Remove(this);
	Super::OnUnregister();
}

void UGuardBrainComponent::GetAllBrains(const UWorld* World, TArray<UGuardBrainComponent*>& Out)
{
	Out.Reset();
	for (const TWeakObjectPtr<UGuardBrainComponent>& W : Registry())
	{
		if (UGuardBrainComponent* B = W.Get())
		{
			if (B->GetWorld() == World && B->HasBegunPlay())
			{
				Out.Add(B);
			}
		}
	}
}

void UGuardBrainComponent::BeginPlay()
{
	Super::BeginPlay();
	AActor* Owner = GetOwner();
	CastBody = ::Cast<ACastMember>(Owner);
	HomeLocation = Feet();
	HomeYaw = Owner ? static_cast<float>(Owner->GetActorRotation().Yaw) : 0.f;
	WantYaw = HomeYaw;
	if (GuardName.IsEmpty() && Owner)
	{
#if WITH_EDITOR
		GuardName = Owner->GetActorLabel();
#else
		GuardName = Owner->GetName();
#endif
	}
	EnsureCues();
	RefreshLights();
	State = EGuardState::Patrol;
	StopIndex = 0;
	float Best = FLT_MAX;
	for (int32 i = 0; i < Patrol.Num(); ++i)
	{
		const float D = FVector::DistSquared2D(Patrol[i].Location, HomeLocation);
		if (D < Best)
		{
			Best = D;
			StopIndex = i;
		}
	}
	if (bLog)
	{
		TAKE_LOG(TEXT("GUARD %s brain on (%d stops)"), *GuardName, Patrol.Num());
	}
}

void UGuardBrainComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Registry().Remove(this);
	Super::EndPlay(EndPlayReason);
}

// ---- the scene's controls ------------------------------------------------------------------------------------------

void UGuardBrainComponent::SetPatrol(const TArray<FGuardStop>& Stops)
{
	Patrol = Stops;
	StopIndex = 0;
	StopDir = 1;
	bAtStop = false;
	float Best = FLT_MAX;
	const FVector F = Feet();
	for (int32 i = 0; i < Patrol.Num(); ++i)
	{
		const float D = FVector::DistSquared2D(Patrol[i].Location, F);
		if (D < Best)
		{
			Best = D;
			StopIndex = i;
		}
	}
}

void UGuardBrainComponent::SetPatrolPoints(const TArray<FVector>& Points, float WaitSeconds)
{
	TArray<FGuardStop> Stops;
	for (const FVector& P : Points)
	{
		FGuardStop S;
		S.Location = P;
		S.WaitSeconds = WaitSeconds;
		Stops.Add(S);
	}
	SetPatrol(Stops);
}

void UGuardBrainComponent::SetBrainActive(bool bOn)
{
	if (bActive == bOn)
	{
		return;
	}
	bActive = bOn;
	if (!bOn)
	{
		StopMoving();
		bSeesTarget = false;
	}
	UpdateCues(0.f);
}

void UGuardBrainComponent::Lure(FVector Point, float Seconds, bool bWalk)
{
	if (State == EGuardState::Alert || State == EGuardState::Caught || State == EGuardState::Downed)
	{
		return;
	}
	Stimulus = Point;
	LureLeft = FMath::Max(Seconds, 0.1f);
	bLureWalk = bWalk;
	if (bWalk)
	{
		const FVector F = Feet();
		FVector To = Point - F;
		To.Z = 0.f;
		const float D = static_cast<float>(To.Size());
		const FVector Short = D > 160.f ? Point - To / D * 150.f : F;
		Goal = Reachable(FVector(Short.X, Short.Y, F.Z));
	}
	else
	{
		Goal = Feet();
	}
	Bark(EGuardEvent::HeardNoise, LureBarks);
	Enter(EGuardState::Distracted);
	FaceToward(Point);
	HeadLook(Point, Seconds);
	Gesture(NoticeGesture);
}

void UGuardBrainComponent::Blind(float Seconds)
{
	BlindLeft = FMath::Max(BlindLeft, Seconds);
	bSeesTarget = false;
}

void UGuardBrainComponent::KnockDown()
{
	if (State != EGuardState::Downed)
	{
		Enter(EGuardState::Downed);
	}
}

void UGuardBrainComponent::ForceAlert()
{
	if (State == EGuardState::Downed)
	{
		return;
	}
	if (AActor* T = GetTarget())
	{
		FVector H, C, F;
		TargetPoints(T, H, C, F);
		LastSeen = F;
		Stimulus = F;
	}
	Suspicion = 1.f;
	SinceSeen = 0.f;
	Enter(EGuardState::Alert);
}

void UGuardBrainComponent::ResetBrain()
{
	AActor* Owner = GetOwner();
	StopMoving();
	if (Owner)
	{
		FVector At = HomeLocation;
		if (const ACharacter* Ch = ::Cast<ACharacter>(Owner))
		{
			At.Z += Ch->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		}
		Owner->SetActorLocationAndRotation(At, FRotator(0.f, HomeYaw, 0.f));
	}
	if (ACastMember* M = CastBody.Get())
	{
		M->FaceYaw(HomeYaw);
	}
	WantYaw = HomeYaw;
	State = EGuardState::Patrol;
	StateTime = 0.f;
	Suspicion = 0.f;
	SinceSeen = 100.f;
	bSeesTarget = false;
	BlindLeft = 0.f;
	EdgyLeft = 0.f;
	LureLeft = 0.f;
	ReactLeft = 0.f;
	HeadYawOffset = 0.f;
	Crumbs.Reset();
	Trail.Reset();
	SearchPoints.Reset();
	FoundBodies.Reset();
	bCaughtSent = false;
	bSpottedSent = false;
	bAtStop = false;
	BarkLeft = 0.f;
	SetPatrol(Patrol);
	if (bLog)
	{
		TAKE_LOG(TEXT("GUARD %s reset"), *GuardName);
	}
	UpdateCues(0.f);
}

// ---- tick ----------------------------------------------------------------------------------------------------------

void UGuardBrainComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const float Dt = FMath::Min(DeltaTime, 0.1f);
	if (bActive)
	{
		BlindLeft = FMath::Max(0.f, BlindLeft - Dt);
		EdgyLeft = FMath::Max(0.f, EdgyLeft - Dt);
		StateTime += Dt;
		if (State != EGuardState::Downed)
		{
			Sense(Dt);
			CheckBodies();
		}
		StepState(Dt);
		StepTurn(Dt);
	}
	UpdateCues(Dt);
	if (CVarGuardDebug.GetValueOnGameThread() != 0)
	{
		DrawDebug();
	}
}

// ---- senses --------------------------------------------------------------------------------------------------------

// The character the player is on: Take's controller does not possess it, so GetPlayerPawn is empty in the film.
static APawn* GuardBrainPlayerBody(const UObject* Ctx)
{
	if (const ATakePlayerController* PC = Cast<ATakePlayerController>(UGameplayStatics::GetPlayerController(Ctx, 0)))
	{
		if (ATakeCharacter* C = PC->GetCurrentCharacter())
		{
			return C;
		}
	}
	return UGameplayStatics::GetPlayerPawn(Ctx, 0);
}

AActor* UGuardBrainComponent::GetTarget() const
{
	if (AActor* T = Target.Get())
	{
		return T;
	}
	return bAutoTargetPlayer ? GuardBrainPlayerBody(this) : nullptr;
}

// Running: the game's own run (held or forced) while moving, else plain speed over RunSpeed.
static bool GuardBrainTargetRuns(const AActor* T, float Speed2D, float RunSpeedCm)
{
	if (const ATakeCharacter* TC = ::Cast<ATakeCharacter>(T))
	{
		if (TC->IsRunning() && Speed2D > 40.f)
		{
			return true;
		}
	}
	return Speed2D > RunSpeedCm;
}

bool UGuardBrainComponent::TargetCrouched(const AActor* T) const
{
	if (!T)
	{
		return false;
	}
	if (const ATakeCharacter* TC = ::Cast<ATakeCharacter>(T))
	{
		if (TC->IsCrouching())
		{
			return true;
		}
	}
	if (const ACharacter* C = ::Cast<ACharacter>(T))
	{
		if (C->bIsCrouched || (C->GetCharacterMovement() && C->GetCharacterMovement()->IsCrouching()))
		{
			return true;
		}
	}
	return T->ActorHasTag(TEXT("Crouched"));
}

void UGuardBrainComponent::TargetPoints(const AActor* T, FVector& Head, FVector& Chest, FVector& TFeet) const
{
	const FVector L = T->GetActorLocation();
	if (const ACharacter* C = ::Cast<ACharacter>(T))
	{
		const float H = C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		TFeet = L - FVector(0.f, 0.f, H);
		Head = TFeet + FVector(0.f, 0.f, 2.f * H * 0.9f);
		Chest = TFeet + FVector(0.f, 0.f, 2.f * H * 0.62f);
		return;
	}
	TFeet = L;
	Head = L + FVector(0.f, 0.f, 165.f);
	Chest = L + FVector(0.f, 0.f, 115.f);
}

FVector UGuardBrainComponent::Feet() const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return FVector::ZeroVector;
	}
	FVector L = Owner->GetActorLocation();
	if (const ACharacter* Ch = ::Cast<ACharacter>(Owner))
	{
		L.Z -= Ch->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}
	return L;
}

FVector UGuardBrainComponent::Eye() const
{
	return Feet() + FVector(0.f, 0.f, EyeHeight);
}

float UGuardBrainComponent::LookYaw() const
{
	const AActor* Owner = GetOwner();
	return (Owner ? static_cast<float>(Owner->GetActorRotation().Yaw) : 0.f) + HeadYawOffset;
}

bool UGuardBrainComponent::LineClear(const FVector& From, const FVector& To, const AActor* IgnoreB) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	FCollisionQueryParams Q(SCENE_QUERY_STAT(GuardSight), false, GetOwner());
	if (IgnoreB)
	{
		Q.AddIgnoredActor(IgnoreB);
	}
	return !World->LineTraceTestByChannel(From, To, ECC_Visibility, Q);
}

void UGuardBrainComponent::Sense(float Dt)
{
	AActor* T = GetTarget();
	bSeesTarget = false;
	if (!T)
	{
		return;
	}
	FVector Head, Chest, TF;
	TargetPoints(T, Head, Chest, TF);
	const bool bCrouch = TargetCrouched(T);
	const float Speed2D = static_cast<float>(T->GetVelocity().Size2D());
	const FVector E = Eye();
	const float Dist = static_cast<float>(FVector::Dist(E, Chest));

	// Footsteps: running carries, walking only up close, crouching is silent.
	NoiseClock -= Dt;
	if (!bCrouch && Speed2D > 40.f && NoiseClock <= 0.f && State != EGuardState::Alert && State != EGuardState::Caught)
	{
		const float R = GuardBrainTargetRuns(T, Speed2D, RunSpeed) ? RunNoiseRange : WalkNoiseRange;
		const float FD = static_cast<float>(FVector::Dist2D(TF, Feet()));
		if (R > 0.f && FD < R && (FD < R * 0.5f || LineClear(E, Chest, T)))
		{
			NoiseClock = 1.5f;
			HearNoise(TF, 1.f, TEXT("Footsteps"), T);
		}
	}

	// The light on him, a few times a second (only worth it when he could be seen).
	LightClock -= Dt;
	LightsRefreshClock -= Dt;
	if (LightClock <= 0.f)
	{
		LightClock = LightSampleSeconds;
		if (LightOverride >= 0.f)
		{
			TargetLight01 = FMath::Clamp(LightOverride, 0.f, 1.f);
		}
		else if (!bUseLight)
		{
			TargetLight01 = 1.f;
		}
		else if (Dist < SightRange + 200.f)
		{
			if (LightsRefreshClock <= 0.f)
			{
				RefreshLights();
			}
			TargetLight01 = SampleLight(Chest);
		}
	}

	float Rate = 0.f;
	const float Range = SightRange * (bCrouch ? CrouchRangeScale : 1.f);
	if (BlindLeft <= 0.f && Dist < Range)
	{
		const float Ang = FMath::Abs(FMath::FindDeltaAngleDegrees(LookYaw(), Yaw2D(Chest - E)));
		float AngF = Ang <= InnerHalfAngle ? 1.f : (Ang <= PeripheralHalfAngle ? PeripheralRate : 0.f);
		if (AngF > 0.f && (LineClear(E, Head, T) || LineClear(E, Chest, T)))
		{
			bSeesTarget = true;
			const float A = FMath::Clamp((Dist - CloseRange) / FMath::Max(Range - CloseRange, 1.f), 0.f, 1.f);
			const float Secs = FMath::Lerp(NearDetectSeconds, FarDetectSeconds, A);
			Rate = AngF / FMath::Max(Secs, 0.05f);
			if (Dist > CloseRange)
			{
				Rate *= FMath::Lerp(DarkRate, 1.f, TargetLight01);
			}
			else
			{
				Rate *= 2.f;	// right in front of him
			}
			if (bCrouch)
			{
				Rate *= CrouchRate;
			}
			Rate *= Speed2D < 20.f ? StillRate : (GuardBrainTargetRuns(T, Speed2D, RunSpeed) ? RunRate : 1.f);
			if (EdgyLeft > 0.f)
			{
				Rate *= EdgyRate;
			}
		}
	}

	if (bSeesTarget)
	{
		Suspicion = FMath::Min(1.f, Suspicion + Rate * Dt);
		SinceSeen = 0.f;
		LastSeen = TF;
		Stimulus = TF;
	}
	else
	{
		SinceSeen += Dt;
		if (SinceSeen > DrainDelay && State != EGuardState::Alert && State != EGuardState::Caught)
		{
			Suspicion = FMath::Max(0.f, Suspicion - DrainPerSecond * Dt);
		}
	}
}

void UGuardBrainComponent::RefreshLights()
{
	LightsRefreshClock = 5.f;
	Lights.Reset();
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (TObjectIterator<ULocalLightComponent> It; It; ++It)
	{
		ULocalLightComponent* L = *It;
		if (L && L->GetWorld() == World && L->IsRegistered() && L->bAffectsWorld && L->Intensity > 0.f)
		{
			Lights.Add(L);
		}
	}
}

float UGuardBrainComponent::SampleLight(const FVector& At) const
{
	struct FHit { float Lux; const ULocalLightComponent* L; };
	TArray<FHit, TInlineAllocator<16>> Hits;
	float Lux = 0.f;
	for (const TWeakObjectPtr<ULocalLightComponent>& W : Lights)
	{
		const ULocalLightComponent* L = W.Get();
		if (!L || !L->IsVisible() || L->Intensity <= 0.f)
		{
			continue;
		}
		const FVector P = L->GetComponentLocation();
		const float R = FMath::Max(L->AttenuationRadius, 1.f);
		const float D = static_cast<float>(FVector::Dist(P, At));
		if (D >= R)
		{
			continue;
		}
		float CosHalf = -1.f;
		float SpotF = 1.f;
		if (const USpotLightComponent* S = ::Cast<USpotLightComponent>(L))
		{
			const float Outer = FMath::DegreesToRadians(FMath::Clamp(S->OuterConeAngle, 1.f, 89.f));
			const float Inner = FMath::DegreesToRadians(FMath::Clamp(S->InnerConeAngle, 0.f, S->OuterConeAngle));
			CosHalf = FMath::Cos(Outer);
			const float C = static_cast<float>(FVector::DotProduct(S->GetForwardVector(), (At - P) / FMath::Max(D, 1.f)));
			SpotF = FMath::SmoothStep(CosHalf, FMath::Max(FMath::Cos(Inner), CosHalf + 0.001f), C);
			if (SpotF <= 0.f)
			{
				continue;
			}
		}
		const float Cd = L->Intensity * ULocalLightComponent::GetUnitsConversionFactor(L->IntensityUnits, ELightUnits::Candelas, CosHalf);
		const float Dm = FMath::Max(D / 100.f, 0.5f);
		const float Win = FMath::Square(FMath::Clamp(1.f - FMath::Pow(D / R, 4.f), 0.f, 1.f));
		const float Li = Cd / (Dm * Dm) * Win * SpotF;
		if (Li > 0.05f)
		{
			Hits.Add({ Li, L });
		}
	}
	Hits.Sort([](const FHit& A, const FHit& B) { return A.Lux > B.Lux; });
	const UWorld* World = GetWorld();
	const AActor* T = GetTarget();
	for (int32 i = 0; i < Hits.Num(); ++i)
	{
		float Li = Hits[i].Lux;
		if (i < ShadowTraces && World)
		{
			FCollisionQueryParams Q(SCENE_QUERY_STAT(GuardLight), false, Hits[i].L->GetOwner());
			if (T)
			{
				Q.AddIgnoredActor(T);
			}
			if (World->LineTraceTestByChannel(Hits[i].L->GetComponentLocation(), At, ECC_Visibility, Q))
			{
				Li *= 0.15f;	// in this lamp's shadow
			}
		}
		Lux += Li;
	}
	return FMath::Clamp(AmbientLight + Lux / LitLux, 0.f, 1.f);
}

void UGuardBrainComponent::CheckBodies()
{
	if (State == EGuardState::Alert || State == EGuardState::Caught || BlindLeft > 0.f)
	{
		return;
	}
	const UWorld* World = GetWorld();
	if (!World || (GFrameCounter % 8) != 0)
	{
		return;
	}
	TArray<UGuardBrainComponent*> All;
	GetAllBrains(World, All);
	const FVector E = Eye();
	for (UGuardBrainComponent* B : All)
	{
		if (B == this || B->State != EGuardState::Downed || FoundBodies.Contains(B))
		{
			continue;
		}
		AActor* BodyActor = B->GetOwner();
		if (!BodyActor || BodyActor->IsHidden())
		{
			continue;
		}
		const FVector P = B->Feet() + FVector(0.f, 0.f, 40.f);
		if (FVector::Dist(P, E) > BodySightRange)
		{
			continue;
		}
		const float Ang = FMath::Abs(FMath::FindDeltaAngleDegrees(LookYaw(), Yaw2D(P - E)));
		if (Ang > PeripheralHalfAngle || !LineClear(E, P, BodyActor))
		{
			continue;
		}
		FoundBodies.Add(B);
		EdgyLeft = EdgySeconds;
		BarkLeft = 0.f;
		Bark(EGuardEvent::BodyFound, BodyBarks);
		Emit(EGuardEvent::BodyFound, BodyActor);
		if (bBodyRaisesAlarm)
		{
			ForceAlert();
		}
		else
		{
			Suspicion = FMath::Max(Suspicion, InvestigateThreshold);
			GoInvestigate(B->Feet(), true);
		}
		return;
	}
}

void UGuardBrainComponent::ReportNoise(const UObject* WorldContext, FVector Location, float Loudness, FName Kind, AActor* Instigator)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return;
	}
	TArray<UGuardBrainComponent*> All;
	GetAllBrains(World, All);
	for (UGuardBrainComponent* B : All)
	{
		if (!B->bActive || B->State == EGuardState::Downed)
		{
			continue;
		}
		const float R = B->HearingRange * FMath::Max(Loudness, 0.f);
		const float D = static_cast<float>(FVector::Dist(B->Eye(), Location));
		if (D > R)
		{
			continue;
		}
		if (D > R * 0.5f && !B->LineClear(B->Eye(), Location + FVector(0.f, 0.f, 30.f), Instigator))
		{
			continue;	// through a wall it carries half as far
		}
		B->HearNoise(Location, Loudness, Kind, Instigator);
	}
}

void UGuardBrainComponent::HearNoise(const FVector& Where, float Loudness, FName Kind, AActor* Instigator)
{
	if (!bActive || State == EGuardState::Alert || State == EGuardState::Caught || State == EGuardState::Downed)
	{
		return;
	}
	if (bLog)
	{
		TAKE_LOG(TEXT("GUARD %s heard %s at (%.0f %.0f)"), *GuardName, Kind.IsNone() ? TEXT("a noise") : *Kind.ToString(), Where.X, Where.Y);
	}
	// Already on his way to (or searching) about the same spot: just update it.
	if ((State == EGuardState::Investigate || State == EGuardState::Search) && FVector::Dist2D(Where, Stimulus) < 250.f)
	{
		Stimulus = Where;
		return;
	}
	Stimulus = Where;
	Suspicion = FMath::Max(Suspicion, NoticeThreshold);
	Bark(EGuardEvent::HeardNoise, NoiseBarks);
	Emit(EGuardEvent::HeardNoise, Instigator);
	GoInvestigate(Where, true);
}

// ---- states --------------------------------------------------------------------------------------------------------

void UGuardBrainComponent::Enter(EGuardState S)
{
	const EGuardState Old = State;
	if (Old == EGuardState::Patrol && S != EGuardState::Patrol)
	{
		Crumbs.Reset();
		Crumbs.Add(Feet());	// where he left his route
	}
	State = S;
	StateTime = 0.f;
	if (S != EGuardState::Alert && S != EGuardState::Caught)
	{
		bSpottedSent = false;	// a new sighting after he lost you is a new Spotted
	}
	if (bLog && Old != S)
	{
		TAKE_LOG(TEXT("GUARD %s %s -> %s (meter %.2f, light %.2f)"), *GuardName, StateName(Old), StateName(S), Suspicion, TargetLight01);
	}
	switch (S)
	{
	case EGuardState::Patrol:
		bSpottedSent = false;
		bCaughtSent = false;
		bAtStop = false;
		HeadYawOffset = 0.f;
		if (Old == EGuardState::Return)
		{
			Emit(EGuardEvent::BackOnPatrol);
		}
		break;
	case EGuardState::Suspicious:
		StopMoving();
		FaceToward(Stimulus);
		HeadLook(Stimulus + FVector(0.f, 0.f, 120.f), SuspiciousHoldSeconds);
		Bark(EGuardEvent::Noticed, NoticeBarks);
		Gesture(NoticeGesture);
		Emit(EGuardEvent::Noticed);
		break;
	case EGuardState::Alert:
		Trail.Reset();
		TrailClock = 0.f;
		if (!bSpottedSent)
		{
			bSpottedSent = true;
			BarkLeft = 0.f;
			Bark(EGuardEvent::Spotted, SpottedBarks);
			Gesture(AlertGesture);
			Emit(EGuardEvent::Spotted, GetTarget());
		}
		if (!bChaseWhenAlerted)
		{
			StopMoving();
		}
		break;
	case EGuardState::Caught:
		StopMoving();
		if (!bCaughtSent)
		{
			bCaughtSent = true;
			Emit(EGuardEvent::Caught, GetTarget());
		}
		break;
	case EGuardState::Search:
		StopMoving();
		Bark(EGuardEvent::Searching, SearchBarks);
		Gesture(SearchGesture);
		Emit(EGuardEvent::Searching);
		break;
	case EGuardState::Return:
		Emit(EGuardEvent::GaveUp);
		break;
	case EGuardState::Downed:
		StopMoving();
		Suspicion = 0.f;
		bSeesTarget = false;
		Emit(EGuardEvent::KnockedDown);
		break;
	default:
		break;
	}
}

void UGuardBrainComponent::GoInvestigate(const FVector& Point, bool bTurnFirst)
{
	Stimulus = Point;
	const FVector F = Feet();
	Goal = Reachable(FVector(Point.X, Point.Y, F.Z));
	ReactLeft = bTurnFirst ? NoiseReactSeconds : 0.25f;
	Enter(EGuardState::Investigate);
	StopMoving();
	FaceToward(Point);
	HeadLook(Point + FVector(0.f, 0.f, 100.f), ReactLeft + 0.5f);
	Emit(EGuardEvent::Investigating);
}

FVector UGuardBrainComponent::Reachable(const FVector& Want) const
{
	const FVector F = Feet();
	const FVector Up(0.f, 0.f, 60.f);
	const UWorld* World = GetWorld();
	if (!World)
	{
		return Want;
	}
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(GuardPath), false, GetOwner());
	if (AActor* T = GetTarget())
	{
		Q.AddIgnoredActor(T);
	}
	const FVector To(Want.X, Want.Y, F.Z);
	if (!World->LineTraceSingleByChannel(Hit, F + Up, To + Up, ECC_Visibility, Q))
	{
		return To;
	}
	FVector Dir = To - F;
	Dir.Z = 0.f;
	const float Len = static_cast<float>(Dir.Size());
	if (Len < 1.f)
	{
		return F;
	}
	Dir /= Len;
	const float Reach = FMath::Max(0.f, static_cast<float>(FVector::Dist2D(F, Hit.Location)) - WallStandOff);
	return F + Dir * Reach;
}

void UGuardBrainComponent::BeginSearch()
{
	Enter(EGuardState::Search);
	SearchLeft = SearchSeconds;
	SearchLookClock = 0.f;
	SearchIndex = 0;
	SearchPoints.Reset();
	const FVector C = Feet();
	for (int32 i = 0, Tries = 0; i < SearchSpots && Tries < SearchSpots * 4; ++Tries)
	{
		const float Yaw = FMath::FRandRange(0.f, 360.f);
		const float R = SearchRadius * FMath::FRandRange(0.45f, 1.f);
		const FVector Want = C + YawDir(Yaw) * R;
		const FVector Got = Reachable(Want);
		if (FVector::Dist2D(Got, C) > SearchRadius * 0.35f)
		{
			SearchPoints.Add(Got);
			++i;
		}
	}
}

void UGuardBrainComponent::BeginReturn()
{
	Suspicion = FMath::Min(Suspicion, NoticeThreshold * 0.5f);
	HeadYawOffset = 0.f;
	Enter(EGuardState::Return);
}

void UGuardBrainComponent::StepState(float Dt)
{
	const bool bCalm = State == EGuardState::Patrol || State == EGuardState::Return || State == EGuardState::Search
		|| State == EGuardState::Investigate || State == EGuardState::Distracted;
	if (bCalm)
	{
		if (Suspicion >= 1.f)
		{
			Enter(EGuardState::Alert);
		}
		else if (bSeesTarget && Suspicion >= NoticeThreshold)
		{
			Enter(EGuardState::Suspicious);
		}
	}

	switch (State)
	{
	case EGuardState::Patrol:
		StepPatrol(Dt);
		break;

	case EGuardState::Suspicious:
		StopMoving();
		if (Suspicion >= 1.f)
		{
			Enter(EGuardState::Alert);
			break;
		}
		if (bSeesTarget)
		{
			FaceToward(Stimulus);
			if (FMath::Fmod(StateTime, 0.5f) < Dt)
			{
				HeadLook(Stimulus + FVector(0.f, 0.f, 120.f), 1.f);
			}
		}
		else if (SinceSeen > 0.6f && Suspicion >= InvestigateThreshold)
		{
			GoInvestigate(LastSeen, false);
		}
		else if (SinceSeen > SuspiciousHoldSeconds)
		{
			Bark(EGuardEvent::GaveUp, GiveUpBarks);
			BeginReturn();
		}
		break;

	case EGuardState::Investigate:
		if (ReactLeft > 0.f)
		{
			ReactLeft -= Dt;
			StopMoving();
			FaceToward(Goal);
		}
		else if (MoveTo(Goal, InvestigateSpeed, Dt))
		{
			FaceToward(Stimulus);
			BeginSearch();
		}
		break;

	case EGuardState::Distracted:
		if (bLureWalk && FVector::Dist2D(Feet(), Goal) > AcceptRadius)
		{
			MoveTo(Goal, InvestigateSpeed, Dt);
		}
		else
		{
			bLureWalk = false;
			StopMoving();
			FaceToward(Stimulus);
			LureLeft -= Dt;
			if (LureLeft <= 0.f)
			{
				BeginReturn();
			}
		}
		break;

	case EGuardState::Search:
		StepSearch(Dt);
		break;

	case EGuardState::Return:
	{
		const FVector F = Feet();
		if (Crumbs.Num() > 0)
		{
			// The earliest crumb he can see from here (shortcuts the way he came).
			int32 Pick = Crumbs.Num() - 1;
			for (int32 i = 0; i < Crumbs.Num(); ++i)
			{
				if (LineClear(F + FVector(0.f, 0.f, 60.f), Crumbs[i] + FVector(0.f, 0.f, 60.f), GetTarget()))
				{
					Pick = i;
					break;
				}
			}
			if (MoveTo(Crumbs[Pick], PatrolSpeed, Dt))
			{
				Crumbs.SetNum(Pick);
			}
		}
		else
		{
			FVector Back = HomeLocation;
			if (Patrol.Num() > 0)
			{
				float Best = FLT_MAX;
				for (int32 i = 0; i < Patrol.Num(); ++i)
				{
					const float D = FVector::DistSquared2D(Patrol[i].Location, F);
					if (D < Best)
					{
						Best = D;
						StopIndex = i;
					}
				}
				Back = Patrol[StopIndex].Location;
			}
			if (MoveTo(Back, PatrolSpeed, Dt))
			{
				Enter(EGuardState::Patrol);
			}
		}
		break;
	}

	case EGuardState::Alert:
	{
		AActor* T = GetTarget();
		if (!T)
		{
			BeginReturn();
			break;
		}
		FVector H, C, TF;
		TargetPoints(T, H, C, TF);
		if (bSeesTarget)
		{
			Suspicion = 1.f;
		}
		// Track him a little after losing sight (footsteps): the trail is the way to follow.
		TrailClock -= Dt;
		if (SinceSeen < LoseTargetSeconds * 0.6f && TrailClock <= 0.f)
		{
			TrailClock = 0.2f;
			if (Trail.Num() == 0 || FVector::Dist2D(Trail.Last(), TF) > 90.f)
			{
				Trail.Add(TF);
			}
		}
		if (!bChaseWhenAlerted)
		{
			StopMoving();
			FaceToward(bSeesTarget ? TF : LastSeen);
		}
		else
		{
			const FVector F = Feet();
			const FVector Up(0.f, 0.f, 60.f);
			if (bSeesTarget && LineClear(F + Up, TF + Up, T))
			{
				Trail.Reset();
				MoveTo(TF, ChaseSpeed, Dt);
			}
			else if (Trail.Num() > 0)
			{
				// The newest trail point he can see from here, else the oldest one.
				int32 Pick = 0;
				for (int32 i = Trail.Num() - 1; i >= 0; --i)
				{
					if (LineClear(F + Up, Trail[i] + Up, T))
					{
						Pick = i;
						break;
					}
				}
				if (MoveTo(Trail[Pick], ChaseSpeed, Dt))
				{
					Trail.RemoveAt(0, Pick + 1);
				}
			}
			else
			{
				MoveTo(LastSeen, ChaseSpeed, Dt);
			}
			if (SinceSeen < 0.5f && FVector::Dist2D(F, TF) < CatchRadius)
			{
				Enter(EGuardState::Caught);
				break;
			}
		}
		if (SinceSeen > LoseTargetSeconds)
		{
			Bark(EGuardEvent::LostTarget, LostBarks);
			Emit(EGuardEvent::LostTarget, T);
			Suspicion = 0.7f;
			GoInvestigate(Trail.Num() > 0 ? Trail.Last() : LastSeen, false);
		}
		break;
	}

	case EGuardState::Caught:
		StopMoving();
		if (AActor* T = GetTarget())
		{
			FaceToward(T->GetActorLocation());
		}
		break;

	case EGuardState::Downed:
		break;
	}

	// The way back: a crumb every 120 cm while he is off his route (not while he walks it back).
	if (State != EGuardState::Patrol && State != EGuardState::Return && State != EGuardState::Downed)
	{
		const FVector F = Feet();
		if (Crumbs.Num() == 0 || FVector::Dist2D(Crumbs.Last(), F) > 120.f)
		{
			if (Crumbs.Num() < 256)
			{
				Crumbs.Add(F);
			}
		}
	}

	// The head comes back to centre unless a glance or the search holds it.
	if (State != EGuardState::Patrol && State != EGuardState::Search)
	{
		HeadYawOffset = FMath::FInterpTo(HeadYawOffset, 0.f, Dt, 4.f);
	}
}

void UGuardBrainComponent::StepPatrol(float Dt)
{
	if (Patrol.Num() == 0)
	{
		if (FVector::Dist2D(Feet(), HomeLocation) > AcceptRadius * 2.f)
		{
			MoveTo(HomeLocation, PatrolSpeed, Dt);
			return;
		}
		StopMoving();
		FaceYawTo(HomeYaw);
	}
	else
	{
		StopIndex = FMath::Clamp(StopIndex, 0, Patrol.Num() - 1);
		const FGuardStop& Stop = Patrol[StopIndex];
		if (!bAtStop)
		{
			HeadYawOffset = FMath::FInterpTo(HeadYawOffset, 0.f, Dt, 4.f);
			if (MoveTo(Stop.Location, PatrolSpeed, Dt))
			{
				bAtStop = true;
				WaitLeft = Stop.WaitSeconds;
				GlanceClock = 0.f;
				GlanceStep = 0;
				if (Stop.bFaceYaw)
				{
					FaceYawTo(Stop.FaceYaw);
				}
			}
			return;
		}
		if (Stop.bFaceYaw)
		{
			FaceYawTo(Stop.FaceYaw);
		}
		WaitLeft -= Dt;
		if (WaitLeft <= 0.f)
		{
			bAtStop = false;
			if (Patrol.Num() > 1)
			{
				if (bPingPong)
				{
					if (StopIndex + StopDir < 0 || StopIndex + StopDir >= Patrol.Num())
					{
						StopDir = -StopDir;
					}
					StopIndex += StopDir;
				}
				else
				{
					StopIndex = (StopIndex + 1) % Patrol.Num();
				}
			}
			return;
		}
		if (!Stop.bGlance)
		{
			HeadYawOffset = FMath::FInterpTo(HeadYawOffset, 0.f, Dt, 4.f);
			return;
		}
	}

	// Waiting: glance centre, left, centre, right (the head and the torch both turn).
	GlanceClock += Dt;
	if (GlanceClock >= GlanceSeconds)
	{
		GlanceClock = 0.f;
		GlanceStep = (GlanceStep + 1) % 4;
		const float Off = GlanceStep == 1 ? -GlanceDegrees : (GlanceStep == 3 ? GlanceDegrees : 0.f);
		const AActor* Owner = GetOwner();
		const float Yaw = (Owner ? static_cast<float>(Owner->GetActorRotation().Yaw) : 0.f) + Off;
		HeadLook(Eye() + YawDir(Yaw) * 600.f, GlanceSeconds);
	}
	const float Want = GlanceStep == 1 ? -GlanceDegrees : (GlanceStep == 3 ? GlanceDegrees : 0.f);
	HeadYawOffset = FMath::FInterpTo(HeadYawOffset, Want, Dt, 3.f);
}

void UGuardBrainComponent::StepSearch(float Dt)
{
	SearchLeft -= Dt;
	SearchLookClock += Dt;
	const float LookFor = 2.2f;
	if (SearchLookClock < LookFor || SearchIndex >= SearchPoints.Num())
	{
		// Looking around on the spot: the head (and torch) sweep left and right.
		StopMoving();
		const float Sweep = FMath::Sin(SearchLookClock * 2.4f) * 65.f;
		HeadYawOffset = FMath::FInterpTo(HeadYawOffset, Sweep, Dt, 6.f);
		if (FMath::Fmod(SearchLookClock, 0.6f) < Dt)
		{
			HeadLook(Eye() + YawDir(LookYaw()) * 600.f, 0.8f);
		}
	}
	else
	{
		HeadYawOffset = FMath::FInterpTo(HeadYawOffset, 0.f, Dt, 4.f);
		if (MoveTo(SearchPoints[SearchIndex], PatrolSpeed * 1.15f, Dt))
		{
			++SearchIndex;
			SearchLookClock = 0.f;
		}
	}
	if (SearchLeft <= 0.f)
	{
		Bark(EGuardEvent::GaveUp, GiveUpBarks);
		BeginReturn();
	}
}

// ---- body ----------------------------------------------------------------------------------------------------------

bool UGuardBrainComponent::MoveTo(const FVector& Dest, float Speed, float Dt)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return true;
	}
	const FVector F = Feet();
	FVector To = Dest - F;
	To.Z = 0.f;
	const float D = static_cast<float>(To.Size());
	if (D <= AcceptRadius)
	{
		StopMoving();
		return true;
	}
	if (ACastMember* M = CastBody.Get())
	{
		WalkResend -= Dt;
		if (FVector::Dist2D(WalkGoal, Dest) > 25.f || WalkResend <= 0.f || !M->IsWalking())
		{
			M->WalkTo(Dest, Speed);
			WalkGoal = Dest;
			WalkResend = 0.5f;
		}
		if (M->IsWalking())
		{
			return false;
		}
		// A cast member that does not walk itself (dressing a parent): move the actor here.
	}
	const float Step = FMath::Min(Speed * Dt, D);
	Owner->SetActorLocation(Owner->GetActorLocation() + To / D * Step);
	WantYaw = Yaw2D(To);
	bTurningSelf = true;
	return false;
}

void UGuardBrainComponent::StopMoving()
{
	if (ACastMember* M = CastBody.Get())
	{
		if (M->IsWalking())
		{
			M->StopWalking();
		}
	}
	WalkGoal = FVector(FLT_MAX);
}

void UGuardBrainComponent::FaceYawTo(float Yaw)
{
	WantYaw = FRotator::NormalizeAxis(Yaw);
	if (ACastMember* M = CastBody.Get())
	{
		const AActor* Owner = GetOwner();
		if (Owner && FMath::Abs(FMath::FindDeltaAngleDegrees(static_cast<float>(Owner->GetActorRotation().Yaw), WantYaw)) > 1.f)
		{
			M->FaceYaw(WantYaw);
		}
		return;
	}
	bTurningSelf = true;
}

void UGuardBrainComponent::FaceToward(const FVector& Point)
{
	const FVector To = Point - Feet();
	if (To.SizeSquared2D() > 100.f)
	{
		FaceYawTo(Yaw2D(To));
	}
}

void UGuardBrainComponent::StepTurn(float Dt)
{
	AActor* Owner = GetOwner();
	if (!Owner || !bTurningSelf || (CastBody.IsValid() && CastBody->IsWalking()))
	{
		return;
	}
	if (CastBody.IsValid() && !bTurningSelf)
	{
		return;
	}
	const float Yaw = static_cast<float>(Owner->GetActorRotation().Yaw);
	const float Delta = FMath::FindDeltaAngleDegrees(Yaw, WantYaw);
	if (FMath::Abs(Delta) < 0.5f)
	{
		bTurningSelf = false;
		return;
	}
	const float Turn = FMath::Clamp(Delta, -TurnRate * Dt, TurnRate * Dt);
	Owner->SetActorRotation(FRotator(0.f, Yaw + Turn, 0.f));
}

void UGuardBrainComponent::HeadLook(const FVector& Point, float Seconds)
{
	if (ACastMember* M = CastBody.Get())
	{
		M->LookAtPoint(Point, Seconds);
	}
}

void UGuardBrainComponent::Gesture(FName G)
{
	if (G.IsNone())
	{
		return;
	}
	if (ACastMember* M = CastBody.Get())
	{
		M->PlayGesture(G);
	}
}

// ---- cues ----------------------------------------------------------------------------------------------------------

void UGuardBrainComponent::Bark(EGuardEvent Why, const TArray<FString>& Lines)
{
	if (Lines.Num() == 0 || (BarkLeft > BarkSeconds - BarkGap && Why != EGuardEvent::Spotted && Why != EGuardEvent::BodyFound))
	{
		return;
	}
	int32 Pick = FMath::RandRange(0, Lines.Num() - 1);
	if (Lines.Num() > 1 && Lines[Pick] == LastBark)
	{
		Pick = (Pick + 1) % Lines.Num();
	}
	LastBark = Lines[Pick];
	BarkLeft = BarkSeconds;
	OnBark.Broadcast(this, Why, LastBark);
	if (bLog)
	{
		TAKE_LOG(TEXT("GUARD %s says \"%s\""), *GuardName, *LastBark);
	}
	if (BarkText)
	{
		BarkText->SetText(FText::FromString(LastBark));
	}
}

void UGuardBrainComponent::Emit(EGuardEvent E, AActor* Other)
{
	if (bLog)
	{
		TAKE_LOG(TEXT("GUARD %s event %s"), *GuardName, EventName(E));
	}
	OnEvent.Broadcast(this, E, Other);
}

void UGuardBrainComponent::EnsureCues()
{
	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}
	USceneComponent* Root = Owner->GetRootComponent();
	if (bShowIcon && !Icon)
	{
		Icon = NewObject<UTextRenderComponent>(Owner, TEXT("GuardIcon"));
		Icon->SetUsingAbsoluteLocation(true);
		Icon->SetUsingAbsoluteRotation(true);
		Icon->SetUsingAbsoluteScale(true);
		if (Root)
		{
			Icon->SetupAttachment(Root);
		}
		Icon->SetHorizontalAlignment(EHTA_Center);
		Icon->SetVerticalAlignment(EVRTA_TextCenter);
		Icon->SetWorldSize(IconSize);
		Icon->SetText(FText::FromString(TEXT("?")));
		Icon->SetCastShadow(false);
		Icon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Icon->RegisterComponent();
		Icon->SetVisibility(false);

		BarkText = NewObject<UTextRenderComponent>(Owner, TEXT("GuardBark"));
		BarkText->SetUsingAbsoluteLocation(true);
		BarkText->SetUsingAbsoluteRotation(true);
		BarkText->SetUsingAbsoluteScale(true);
		if (Root)
		{
			BarkText->SetupAttachment(Root);
		}
		BarkText->SetHorizontalAlignment(EHTA_Center);
		BarkText->SetVerticalAlignment(EVRTA_TextCenter);
		BarkText->SetWorldSize(IconSize * 0.32f);
		BarkText->SetTextRenderColor(FColor(245, 240, 225));
		BarkText->SetCastShadow(false);
		BarkText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BarkText->RegisterComponent();
		BarkText->SetVisibility(false);
	}
	if (bSpawnTorch && !OwnTorch && !TorchLight)
	{
		OwnTorch = NewObject<USpotLightComponent>(Owner, TEXT("GuardTorch"));
		OwnTorch->SetUsingAbsoluteLocation(true);
		OwnTorch->SetUsingAbsoluteRotation(true);
		if (Root)
		{
			OwnTorch->SetupAttachment(Root);
		}
		OwnTorch->SetIntensityUnits(ELightUnits::Candelas);
		OwnTorch->SetIntensity(TorchCandelas);
		OwnTorch->SetLightColor(TorchCalm);
		OwnTorch->SetInnerConeAngle(14.f);
		OwnTorch->SetOuterConeAngle(24.f);
		OwnTorch->SetAttenuationRadius(SightRange * 1.1f);
		OwnTorch->SetCastShadows(false);
		OwnTorch->RegisterComponent();
	}
	TorchRot = FRotator(-12.f, LookYaw(), 0.f);
}

void UGuardBrainComponent::UpdateCues(float Dt)
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const FVector F = Feet();
	const bool bAlert = State == EGuardState::Alert || State == EGuardState::Caught;
	const bool bWary = State == EGuardState::Suspicious || State == EGuardState::Investigate
		|| State == EGuardState::Search || State == EGuardState::Distracted;
	IconPulse += Dt;
	BarkLeft = FMath::Max(0.f, BarkLeft - Dt);

	// The camera, for facing the text at it.
	FVector Cam = F + FVector(-500.f, 0.f, 200.f);
	if (APlayerCameraManager* PCM = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		Cam = PCM->GetCameraLocation();
	}

	if (Icon)
	{
		const bool bShow = bActive && bShowIcon && State != EGuardState::Downed && (bAlert || bWary || Suspicion > 0.03f);
		Icon->SetVisibility(bShow);
		if (bShow)
		{
			const FVector At = F + FVector(0.f, 0.f, IconHeight);
			Icon->SetWorldLocationAndRotation(At, (Cam - At).Rotation());
			float Size;
			FColor Col;
			if (bAlert)
			{
				Icon->SetText(FText::FromString(TEXT("!")));
				Size = IconSize * (1.15f + 0.12f * FMath::Sin(IconPulse * 14.f));
				Col = FColor(255, 40, 30);
			}
			else
			{
				Icon->SetText(FText::FromString(TEXT("?")));
				// The meter: grey-white at a glimpse, yellow, orange, then red-orange just before he calls it.
				const float S = FMath::Max(Suspicion, bWary ? 0.35f : 0.f);
				const FLinearColor C = S < 0.5f
					? FMath::Lerp(FLinearColor(0.85f, 0.85f, 0.8f), FLinearColor(1.f, 0.85f, 0.1f), S * 2.f)
					: FMath::Lerp(FLinearColor(1.f, 0.85f, 0.1f), FLinearColor(1.f, 0.3f, 0.05f), (S - 0.5f) * 2.f);
				Col = C.ToFColor(true);
				const float Beat = bSeesTarget ? 0.08f * FMath::Sin(IconPulse * (6.f + 10.f * Suspicion)) : 0.f;
				Size = IconSize * (0.55f + 0.55f * S + Beat);
			}
			Icon->SetWorldSize(Size);
			Icon->SetTextRenderColor(Col);
		}
	}
	if (BarkText)
	{
		const bool bShow = bWorldBarks && BarkLeft > 0.f && State != EGuardState::Downed;
		BarkText->SetVisibility(bShow);
		if (bShow)
		{
			const FVector At = F + FVector(0.f, 0.f, IconHeight - IconSize * 0.75f);
			BarkText->SetWorldLocationAndRotation(At, (Cam - At).Rotation());
		}
	}

	USpotLightComponent* Torch = TorchLight ? TorchLight.Get() : OwnTorch.Get();
	if (Torch)
	{
		FRotator Want(-FMath::RadiansToDegrees(FMath::Atan2(EyeHeight, 0.45f * SightRange)), LookYaw(), 0.f);
		if ((bAlert || State == EGuardState::Suspicious) && !Stimulus.IsZero())
		{
			const FVector Aim = (bSeesTarget ? Stimulus + FVector(0.f, 0.f, 90.f) : Stimulus) - Eye();
			Want = Aim.Rotation();
		}
		else if (State == EGuardState::Distracted)
		{
			Want = (Stimulus - Eye()).Rotation();
		}
		TorchRot = Dt > 0.f ? FMath::RInterpTo(TorchRot, Want, Dt, TorchSwingRate) : Want;
		Torch->SetWorldLocationAndRotation(Eye() + YawDir(LookYaw()) * 25.f - FVector(0.f, 0.f, 20.f), TorchRot);
		const FLinearColor C = bAlert ? TorchAlert : (bWary || Suspicion > NoticeThreshold ? TorchSuspicious : TorchCalm);
		Torch->SetLightColor(C);
		Torch->SetVisibility(bActive && State != EGuardState::Downed && BlindLeft <= 0.f);
	}
}

void UGuardBrainComponent::DrawDebug() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const FVector E = Eye();
	const float Y = LookYaw();
	const FColor C = State == EGuardState::Alert ? FColor::Red : (Suspicion > NoticeThreshold ? FColor::Orange : FColor::Green);
	for (const float A : { -PeripheralHalfAngle, -InnerHalfAngle, InnerHalfAngle, PeripheralHalfAngle })
	{
		const bool bInner = FMath::Abs(A) == InnerHalfAngle;
		DrawDebugLine(World, E, E + YawDir(Y + A) * SightRange, bInner ? C : FColor(90, 90, 90), false, -1.f, 0, bInner ? 2.f : 1.f);
	}
	DrawDebugString(World, E + FVector(0.f, 0.f, 80.f), FString::Printf(TEXT("%s %s  meter %.2f  light %.2f%s"),
		*GuardName, StateName(State), Suspicion, TargetLight01, bSeesTarget ? TEXT("  SEES") : TEXT("")), nullptr, C, 0.f, true);
	for (int32 i = 1; i < Crumbs.Num(); ++i)
	{
		DrawDebugLine(World, Crumbs[i - 1] + FVector(0, 0, 10), Crumbs[i] + FVector(0, 0, 10), FColor::Cyan, false, -1.f, 0, 1.f);
	}
	for (int32 i = 1; i < Trail.Num(); ++i)
	{
		DrawDebugLine(World, Trail[i - 1] + FVector(0, 0, 10), Trail[i] + FVector(0, 0, 10), FColor::Magenta, false, -1.f, 0, 2.f);
	}
	if (State == EGuardState::Investigate || State == EGuardState::Distracted)
	{
		DrawDebugSphere(World, Goal, 30.f, 8, FColor::Yellow);
	}
}

// ---- console: test hooks for play-in-editor ------------------------------------------------------------------------

static APawn* GuardTestPawn(UWorld* World, FVector& OutFeet, FVector& OutFwd)
{
	APawn* P = World ? GuardBrainPlayerBody(World) : nullptr;
	if (!P)
	{
		return nullptr;
	}
	OutFeet = P->GetActorLocation();
	if (const ACharacter* C = Cast<ACharacter>(P))
	{
		OutFeet.Z -= C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}
	OutFwd = YawDir(static_cast<float>(P->GetActorRotation().Yaw));
	return P;
}

// Reel.Guard.Test [Distance=900] [Half=350]: a guard (a cast member in the Guard kit, with his own torch) appears
// Distance in front of the player, side-on, patrolling Half either side across the player's view.
static FAutoConsoleCommandWithWorldAndArgs GGuardTestCmd(TEXT("Reel.Guard.Test"),
	TEXT("Reel.Guard.Test [Distance=900] [Half=350]: spawn a test guard with a brain in front of the player (play only)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		FVector At, Fwd;
		if (!World || !World->IsGameWorld() || !GuardTestPawn(World, At, Fwd))
		{
			return;
		}
		const float Dist = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 900.f;
		const float Half = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 350.f;
		const FVector Right(-Fwd.Y, Fwd.X, 0.f);
		const FVector C = At + Fwd * Dist;
		const FTransform Xf(FRotator(0.f, Yaw2D(Right), 0.f), C - Right * Half);
		ACastMember* M = World->SpawnActorDeferred<ACastMember>(ACastMember::StaticClass(), Xf, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!M)
		{
			return;
		}
		M->Kit = ECastKit::Guard;
		M->Tags.Add(TEXT("guard_test"));
		M->FinishSpawning(Xf);
		UGuardBrainComponent* B = UGuardBrainComponent::AddTo(M);
		B->GuardName = FString::Printf(TEXT("TEST%d"), FMath::RandRange(10, 99));
		B->bSpawnTorch = true;
		B->EnsureCues();
		TArray<FGuardStop> Stops;
		FGuardStop S;
		S.WaitSeconds = 3.f;
		S.Location = C - Right * Half;
		Stops.Add(S);
		S.Location = C + Right * Half;
		Stops.Add(S);
		B->SetPatrol(Stops);
		TAKE_LOG(TEXT("GUARD test guard %s at (%.0f %.0f %.0f)"), *B->GuardName, Xf.GetLocation().X, Xf.GetLocation().Y, Xf.GetLocation().Z);
	}));

// Reel.Guard.Noise [Right=0] [Forward=0] [Loudness=1]: a noise this far to the player's right and front.
static FAutoConsoleCommandWithWorldAndArgs GGuardNoiseCmd(TEXT("Reel.Guard.Noise"),
	TEXT("Reel.Guard.Noise [Right=0] [Forward=0] [Loudness=1]: a noise relative to the player; every brain in range goes to look."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		FVector At, Fwd;
		if (!GuardTestPawn(World, At, Fwd))
		{
			return;
		}
		const FVector Right(-Fwd.Y, Fwd.X, 0.f);
		const float R = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 0.f;
		const float F = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 0.f;
		const float L = Args.Num() > 2 ? FCString::Atof(*Args[2]) : 1.f;
		const FVector P = At + Right * R + Fwd * F;
		TAKE_LOG(TEXT("GUARD noise at (%.0f %.0f) loudness %.1f"), P.X, P.Y, L);
		UGuardBrainComponent::ReportNoise(World, P, L, TEXT("TestNoise"), nullptr);
	}));

// Reel.Guard.Log: every brain's state. Reel.Guard.Reset: every brain back home, calm. Reel.Guard.Down: the
// newest test guard is knocked down (to test a colleague finding him).
static FAutoConsoleCommandWithWorldAndArgs GGuardLogCmd(TEXT("Reel.Guard.Log"), TEXT("Log every guard brain's state."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		TArray<UGuardBrainComponent*> All;
		UGuardBrainComponent::GetAllBrains(World, All);
		for (UGuardBrainComponent* B : All)
		{
			const FVector L = B->GetOwner()->GetActorLocation();
			TAKE_LOG(TEXT("GUARD %s %s meter %.2f light %.2f sees %d at (%.0f %.0f)"), *B->GuardName,
				UGuardBrainComponent::StateName(B->GetState()), B->GetSuspicion(), B->GetTargetLight(), B->SeesTarget() ? 1 : 0, L.X, L.Y);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GGuardResetCmd(TEXT("Reel.Guard.Reset"), TEXT("Every guard brain back home, calm."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		TArray<UGuardBrainComponent*> All;
		UGuardBrainComponent::GetAllBrains(World, All);
		for (UGuardBrainComponent* B : All)
		{
			B->ResetBrain();
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GGuardDownCmd(TEXT("Reel.Guard.Down"), TEXT("Knock down the guard brain nearest the player."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		FVector At, Fwd;
		if (!GuardTestPawn(World, At, Fwd))
		{
			return;
		}
		TArray<UGuardBrainComponent*> All;
		UGuardBrainComponent::GetAllBrains(World, All);
		UGuardBrainComponent* Best = nullptr;
		double BestD = DBL_MAX;
		for (UGuardBrainComponent* B : All)
		{
			const double D = FVector::DistSquared(B->GetOwner()->GetActorLocation(), At);
			if (D < BestD && !B->IsDowned())
			{
				BestD = D;
				Best = B;
			}
		}
		if (Best)
		{
			Best->KnockDown();
		}
	}));
