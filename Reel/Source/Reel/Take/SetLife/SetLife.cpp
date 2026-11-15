// Set life: the stage manager (see SetLife.h and docs/SET_LIFE.md).
#include "SetLife/SetLife.h"

#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"

#include "HouseDimmer.h"
#include "QueueLine.h"
#include "TakeAudio.h"
#include "TakeCharacter.h"
#include "TakeDirector.h"
#include "TakeStyle.h"
#include "TakeTypes.h"

static TAutoConsoleVariable<int32> CVarSetLife(
	TEXT("Reel.SetLife"), 1,
	TEXT("1: set life (small rehearsed scenes among the guests and staff, overheard barks). 0: off; running pieces end."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarSetLifeMax(
	TEXT("Reel.SetLife.Max"), 3,
	TEXT("Set life: at most this many pieces at once."),
	ECVF_Default);

// --- zones and the pool ----------------------------------------------------------------------------------------------------

namespace SetLife
{
	ESetLifeZone ZoneOf(const FVector& P)
	{
		if (P.Z >= 700.f)
		{
			return ESetLifeZone::Catwalk;
		}
		if (P.X < -3220.f)
		{
			return ESetLifeZone::Street;
		}
		if (P.X < -1000.f)
		{
			return ESetLifeZone::Lobby;
		}
		return ESetLifeZone::Hall;
	}

	const TCHAR* ZoneName(ESetLifeZone Zone)
	{
		switch (Zone)
		{
		case ESetLifeZone::Street: return TEXT("Street");
		case ESetLifeZone::Lobby: return TEXT("Lobby");
		case ESetLifeZone::Hall: return TEXT("Hall");
		case ESetLifeZone::Catwalk: return TEXT("Catwalk");
		default: return TEXT("None");
		}
	}

	bool IsPoolKit(ECastKit Kit)
	{
		return Kit == ECastKit::GuestTux || Kit == ECastKit::GuestTails || Kit == ECastKit::GuestGown
			|| Kit == ECastKit::GuestGownHat || Kit == ECastKit::Staff;
	}
}

namespace SetLifeLocal
{
	// The clerk (AQueueLine::ClerkFrom / build_cast.py STAFF_AT) and the doorman's post: never cast anyone there.
	const FVector ClerkSpot(-2050.f, 1100.f, 0.f);
	const FVector ClerkDesk(-2050.f, 630.f, 0.f);
	constexpr float QueuePointRadius = 170.f;
	constexpr float ReturnTimeout = 30.f;

	// AHUD::Canvas is protected; this reads it through a pointer to member (the HUD class is not touched).
	struct FHudCanvas : public AHUD
	{
		static UCanvas* Of(AHUD* Hud) { return Hud ? (Hud->*(&FHudCanvas::Canvas)).Get() : nullptr; }
	};

	float YawTo(const FVector& From, const FVector& To)
	{
		const FVector D = To - From;
		return static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X)));
	}
}

// --- the registry ----------------------------------------------------------------------------------------------------------

TArray<FSetLifePieceInfo>& FSetLifeRegistry::Pieces()
{
	static TArray<FSetLifePieceInfo> List;
	return List;
}

FSetLifeRegistry::FSetLifeRegistry(const TCHAR* Name, ESetLifeZone Zone, float Cooldown, float Weight,
	TFunction<TUniquePtr<FSetLifePiece>()> Make)
{
	FSetLifePieceInfo Info;
	Info.Name = FName(Name);
	Info.Zone = Zone;
	Info.Cooldown = FMath::Max(Cooldown, 1.f);
	Info.Weight = FMath::Max(Weight, 0.01f);
	Info.Make = MoveTemp(Make);
	Pieces().Add(MoveTemp(Info));
}

// =============================================================================================================================
// FSetLifeStage
// =============================================================================================================================

FSetLifeStage::FSetLifeStage(USetLifeSubsystem* InOwner, const FSetLifePieceInfo& InInfo, TUniquePtr<FSetLifePiece> InPiece)
	: Owner(InOwner), Info(InInfo), Piece(MoveTemp(InPiece))
{
}

FSetLifeStage::~FSetLifeStage()
{
}

UWorld* FSetLifeStage::GetWorld() const
{
	return Owner ? Owner->GetWorld() : nullptr;
}

FVector FSetLifeStage::GetJulian() const
{
	return Owner ? Owner->GetJulian() : FVector::ZeroVector;
}

FVector FSetLifeStage::GetCamera() const
{
	FVector L, F;
	return Owner && Owner->GetCameraView(L, F) ? L : FVector::ZeroVector;
}

FVector FSetLifeStage::GetCameraForward() const
{
	FVector L, F;
	return Owner && Owner->GetCameraView(L, F) ? F : FVector::ForwardVector;
}

bool FSetLifeStage::IsInView(const FVector& P) const
{
	return Owner && Owner->IsInView(P);
}

bool FSetLifeStage::IsNearFocus(const FVector& P) const
{
	return Owner && Owner->IsNearFocus(P);
}

FVector FSetLifeStage::FloorAt(const FVector& P) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return P;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SetLifeFloor), false);
	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, P + FVector(0.f, 0.f, 100.f), P - FVector(0.f, 0.f, 300.f), ECC_Visibility, Params))
	{
		return Hit.ImpactPoint;
	}
	return P;
}

const FString& FSetLifeStage::Pick(const TArray<FString>& Options) const
{
	static const FString Empty;
	return Options.Num() > 0 ? Options[FMath::RandRange(0, Options.Num() - 1)] : Empty;
}

// --- casting -------------------------------------------------------------------------------------------------------------------

TArray<ACastMember*> FSetLifeStage::FindFree(const FSetLifeRole& Want, int32 Max) const
{
	TArray<ACastMember*> Out;
	if (!Owner)
	{
		return Out;
	}
	const ESetLifeZone Zone = Want.Zone != ESetLifeZone::None ? Want.Zone : Info.Zone;
	const FVector Julian = GetJulian();
	const bool bJulian = !Julian.IsZero();
	TArray<TPair<float, ACastMember*>> Scored;
	for (const TWeakObjectPtr<ACastMember>& Weak : Owner->Pool)
	{
		ACastMember* M = Weak.Get();
		if (!M || !Owner->IsFree(M))
		{
			continue;
		}
		if (Want.Kits.Num() > 0 && !Want.Kits.Contains(M->Kit))
		{
			continue;
		}
		if ((Want.Seat == ESetLifeSeat::Standing && M->bSeated) || (Want.Seat == ESetLifeSeat::Seated && !M->bSeated))
		{
			continue;
		}
		if ((Want.Glass == 1 && !M->bHoldsGlass) || (Want.Glass == 0 && M->bHoldsGlass))
		{
			continue;
		}
		const FVector P = M->GetActorLocation();
		if (Zone != ESetLifeZone::None && SetLife::ZoneOf(P) != Zone)
		{
			continue;
		}
		if (Want.bNear && Want.Radius > 0.f && FVector::Dist2D(P, Want.Near) > Want.Radius)
		{
			continue;
		}
		if (bJulian && FVector::Dist2D(P, Julian) < Want.MinFromJulian)
		{
			continue;
		}
		const bool bView = Owner->IsInView(P);
		if (Want.bInView && !bView)
		{
			continue;
		}
		if (Want.bNearFocus && !Owner->IsNearFocus(P))
		{
			continue;
		}
		if (Want.Filter && !Want.Filter(*M))
		{
			continue;
		}
		const float Base = Want.bNear ? static_cast<float>(FVector::Dist2D(P, Want.Near))
			: bJulian ? static_cast<float>(FVector::Dist2D(P, Julian)) : 0.f;
		Scored.Add(TPair<float, ACastMember*>(Base - (bView ? 600.f : 0.f) + FMath::FRand() * 300.f, M));
	}
	Scored.Sort([](const TPair<float, ACastMember*>& A, const TPair<float, ACastMember*>& B) { return A.Key < B.Key; });
	for (int32 i = 0; i < Scored.Num() && Out.Num() < Max; ++i)
	{
		Out.Add(Scored[i].Value);
	}
	return Out;
}

int32 FSetLifeStage::Cast(FName Role, const FSetLifeRole& Want)
{
	const TArray<ACastMember*> Found = FindFree(Want, 1);
	return Found.Num() > 0 ? CastMember(Role, Found[0]) : INDEX_NONE;
}

int32 FSetLifeStage::CastMember(FName Role, ACastMember* Member)
{
	if (!Owner || !Member || !Owner->IsFree(Member))
	{
		return INDEX_NONE;
	}
	FCast& C = Roles.AddDefaulted_GetRef();
	C.Name = Role;
	C.Member = Member;
	C.Home = Member->GetActorLocation();
	C.HomeYaw = static_cast<float>(Member->GetActorRotation().Yaw);
	Owner->Claimed.Add(Member);
	return Roles.Num() - 1;
}

ACastMember* FSetLifeStage::Get(int32 Role) const
{
	return Roles.IsValidIndex(Role) ? Roles[Role].Member.Get() : nullptr;
}

FVector FSetLifeStage::Where(int32 Role) const
{
	const ACastMember* M = Get(Role);
	return M ? M->GetActorLocation() : FVector::ZeroVector;
}

FVector FSetLifeStage::Head(int32 Role) const
{
	const ACastMember* M = Get(Role);
	if (!M)
	{
		return FVector::ZeroVector;
	}
	if (M->Body && M->Body->GetSkeletalMeshAsset() && M->Body->DoesSocketExist(TEXT("head")))
	{
		return M->Body->GetSocketLocation(TEXT("head"));
	}
	return M->GetActorLocation() + FVector(0.f, 0.f, M->bSeated ? 120.f : 160.f);
}

FVector FSetLifeStage::Home(int32 Role) const
{
	return Roles.IsValidIndex(Role) ? Roles[Role].Home : FVector::ZeroVector;
}

void FSetLifeStage::SetReturnVia(int32 Role, const TArray<FVector>& Points)
{
	if (Roles.IsValidIndex(Role))
	{
		Roles[Role].Via = Points;
	}
}

bool FSetLifeStage::IsSeatedRole(int32 Role) const
{
	const ACastMember* M = Get(Role);
	return M && M->bSeated;
}

FString FSetLifeStage::CastList() const
{
	FString Out;
	for (const FCast& C : Roles)
	{
		const ACastMember* M = C.Member.Get();
		Out += FString::Printf(TEXT("%s%s:%s"), Out.IsEmpty() ? TEXT("") : TEXT(","), *C.Name.ToString(),
			M ? *M->GetName() : TEXT("-"));
	}
	return Out;
}

// --- the beat sheet ------------------------------------------------------------------------------------------------------------

FSetLifeStage::FStep& FSetLifeStage::Add(EStep Kind, int32 Role)
{
	FStep& S = Steps.AddDefaulted_GetRef();
	S.Kind = Kind;
	S.Role = Role;
	return S;
}

FSetLifeStage& FSetLifeStage::At(float T) { Add(EStep::At).A = T; return *this; }
FSetLifeStage& FSetLifeStage::Wait(float Seconds) { Add(EStep::Wait).A = Seconds; return *this; }
FSetLifeStage& FSetLifeStage::WaitArrived(int32 Role, float MaxSeconds) { Add(EStep::WaitArrived, Role).A = MaxSeconds; return *this; }
FSetLifeStage& FSetLifeStage::WaitGesture(int32 Role, float MaxSeconds) { Add(EStep::WaitGesture, Role).A = MaxSeconds; return *this; }

FSetLifeStage& FSetLifeStage::WaitUntil(TFunction<bool(FSetLifeStage&)> Cond, float MaxSeconds, bool bAbortOnTimeout)
{
	FStep& S = Add(EStep::WaitUntil);
	S.Cond = MoveTemp(Cond);
	S.A = MaxSeconds;
	S.bFlag = bAbortOnTimeout;
	return *this;
}

FSetLifeStage& FSetLifeStage::Walk(int32 Role, const FVector& To, float Speed)
{
	FStep& S = Add(EStep::Walk, Role);
	S.Point = To;
	S.A = Speed;
	return *this;
}

FSetLifeStage& FSetLifeStage::WalkUpTo(int32 Role, int32 Other, float StopShort, float Speed)
{
	FStep& S = Add(EStep::WalkUpTo, Role);
	S.Other = Other;
	S.A = Speed;
	S.B = StopShort;
	return *this;
}

FSetLifeStage& FSetLifeStage::Shift(int32 Role, const FVector& Offset, float Seconds)
{
	FStep& S = Add(EStep::Shift, Role);
	S.Point = Offset;
	S.A = Seconds;
	return *this;
}

FSetLifeStage& FSetLifeStage::Face(int32 Role, float Yaw) { Add(EStep::Face, Role).A = Yaw; return *this; }
FSetLifeStage& FSetLifeStage::FaceTo(int32 Role, int32 Other) { Add(EStep::FaceTo, Role).Other = Other; return *this; }
FSetLifeStage& FSetLifeStage::FacePoint(int32 Role, const FVector& P) { Add(EStep::FacePoint, Role).Point = P; return *this; }
FSetLifeStage& FSetLifeStage::Gesture(int32 Role, FName G) { Add(EStep::Gesture, Role).Name = G; return *this; }

FSetLifeStage& FSetLifeStage::GestureAt(int32 Role, FName G, int32 Other)
{
	FStep& S = Add(EStep::GestureAt, Role);
	S.Name = G;
	S.Other = Other;
	return *this;
}

FSetLifeStage& FSetLifeStage::GestureAtPoint(int32 Role, FName G, const FVector& P)
{
	FStep& S = Add(EStep::GestureAtPoint, Role);
	S.Name = G;
	S.Point = P;
	return *this;
}

FSetLifeStage& FSetLifeStage::Look(int32 Role, int32 Other, float Seconds)
{
	FStep& S = Add(EStep::Look, Role);
	S.Other = Other;
	S.A = Seconds;
	return *this;
}

FSetLifeStage& FSetLifeStage::LookAtPoint(int32 Role, const FVector& P, float Seconds)
{
	FStep& S = Add(EStep::LookAtPoint, Role);
	S.Point = P;
	S.A = Seconds;
	return *this;
}

FSetLifeStage& FSetLifeStage::Say(int32 Role, const FString& Line, FName Sound, float Seconds)
{
	FStep& S = Add(EStep::Say, Role);
	S.Text = Line;
	S.Name = Sound;
	S.A = Seconds;
	return *this;
}

FSetLifeStage& FSetLifeStage::Do(TFunction<void(FSetLifeStage&)> Fn)
{
	Add(EStep::Do).Fn = MoveTemp(Fn);
	return *this;
}

FSetLifeStage& FSetLifeStage::Finish()
{
	Add(EStep::Finish);
	return *this;
}

void FSetLifeStage::Abort(const FString& Why)
{
	if (!bDone)
	{
		bDone = true;
		bAborted = true;
		AbortWhy = Why;
	}
}

bool FSetLifeStage::RunStep(FStep& S)
{
	// Returns true when the step is complete (the sheet moves on this frame).
	if (StepStart < 0.f)
	{
		StepStart = Elapsed;
	}
	const float InStep = Elapsed - StepStart;
	ACastMember* M = Get(S.Role);
	switch (S.Kind)
	{
	case EStep::At:
		return Elapsed >= S.A;
	case EStep::Wait:
		return InStep >= S.A;
	case EStep::WaitArrived:
		return !M || !M->IsWalking() || InStep >= S.A;
	case EStep::WaitGesture:
		return !M || !M->IsGesturing() || InStep >= S.A;
	case EStep::WaitUntil:
		if (S.Cond && S.Cond(*this))
		{
			return true;
		}
		if (InStep >= S.A)
		{
			if (S.bFlag)
			{
				Abort(TEXT("timeout"));
			}
			return true;
		}
		return false;
	case EStep::Walk:
		if (M && !M->bSeated)
		{
			Roles[S.Role].bShifting = false;
			M->WalkTo(S.Point, S.A);
		}
		return true;
	case EStep::WalkUpTo:
		if (M && !M->bSeated && Get(S.Other))
		{
			const FVector To = Where(S.Other);
			const FVector From = M->GetActorLocation();
			Roles[S.Role].bShifting = false;
			M->WalkTo(To + (From - To).GetSafeNormal2D() * S.B, S.A);
		}
		return true;
	case EStep::Shift:
		if (M && !M->bSeated)
		{
			FCast& C = Roles[S.Role];
			C.bShifting = true;
			C.ShiftFrom = M->GetActorLocation();
			C.ShiftTo = C.ShiftFrom + FVector(S.Point.X, S.Point.Y, 0.f);
			C.ShiftStart = Elapsed;
			C.ShiftSeconds = FMath::Max(S.A, 0.05f);
		}
		return true;
	case EStep::Face:
		if (M && !M->bSeated)
		{
			M->FaceYaw(S.A);
		}
		return true;
	case EStep::FaceTo:
		if (M && !M->bSeated && Get(S.Other))
		{
			M->FaceYaw(SetLifeLocal::YawTo(M->GetActorLocation(), Where(S.Other)));
		}
		return true;
	case EStep::FacePoint:
		if (M && !M->bSeated)
		{
			M->FaceYaw(SetLifeLocal::YawTo(M->GetActorLocation(), S.Point));
		}
		return true;
	case EStep::Gesture:
		if (M)
		{
			M->PlayGesture(S.Name);
		}
		return true;
	case EStep::GestureAt:
		if (M && Get(S.Other))
		{
			M->PlayGestureAt(S.Name, Head(S.Other));
		}
		return true;
	case EStep::GestureAtPoint:
		if (M)
		{
			M->PlayGestureAt(S.Name, S.Point.IsZero() ? FVector(0.f, 0.f, 0.01f) : S.Point);
		}
		return true;
	case EStep::Look:
		if (M && Get(S.Other))
		{
			M->LookAtPoint(Head(S.Other), S.A);
		}
		return true;
	case EStep::LookAtPoint:
		if (M)
		{
			M->LookAtPoint(S.Point, S.A);
		}
		return true;
	case EStep::Say:
		if (M && Owner)
		{
			Owner->Say(M, S.Text, S.Name, S.A, Info.Name);
		}
		return true;
	case EStep::Do:
		if (S.Fn)
		{
			S.Fn(*this);
		}
		return true;
	case EStep::Finish:
		Cursor = Steps.Num();
		return true;
	}
	return true;
}

bool FSetLifeStage::Step(float DeltaSeconds, bool bHold)
{
	bHeld = bHold;
	if (bDone)
	{
		return false;
	}
	// Everyone cast must still exist (a member destroyed under us ends the piece; nothing is touched).
	for (const FCast& C : Roles)
	{
		if (!C.Member.IsValid())
		{
			Abort(TEXT("member gone"));
			return false;
		}
	}
	if (!bHold)
	{
		Elapsed += DeltaSeconds;
		int32 Guard = 0;
		while (!bDone && Cursor < Steps.Num() && Guard++ < 256)
		{
			if (!RunStep(Steps[Cursor]))
			{
				break;
			}
			if (Cursor < Steps.Num())
			{
				++Cursor;
			}
			StepStart = -1.f;
		}
	}
	TickShifts();
	if (Piece && !bDone)
	{
		Piece->Tick(*this, bHold ? 0.f : DeltaSeconds);
	}
	TickProps();
	if (!bDone && Elapsed > MaxRunSeconds)
	{
		Abort(TEXT("overran"));
	}
	if (!bDone && Cursor >= Steps.Num())
	{
		bDone = true;
	}
	return !bDone;
}

void FSetLifeStage::TickShifts()
{
	for (FCast& C : Roles)
	{
		ACastMember* M = C.Member.Get();
		if (!C.bShifting || !M)
		{
			continue;
		}
		if (M->IsWalking())
		{
			C.bShifting = false;
			continue;
		}
		const float T = FMath::Clamp((Elapsed - C.ShiftStart) / C.ShiftSeconds, 0.f, 1.f);
		const float E = FMath::InterpEaseInOut(0.f, 1.f, T, 2.f);
		FVector P = FMath::Lerp(C.ShiftFrom, C.ShiftTo, E);
		P.Z = M->GetActorLocation().Z;
		M->SetActorLocation(P);
		if (T >= 1.f)
		{
			C.bShifting = false;
		}
	}
}

void FSetLifeStage::TickProps()
{
	for (FProp& P : Props)
	{
		USceneComponent* Carrier = P.Carrier.Get();
		const ACastMember* Holder = P.Holder.Get();
		if (Carrier && Holder)
		{
			Carrier->SetWorldRotation(FRotator(0.f, Holder->GetActorRotation().Yaw, 0.f));
		}
	}
	for (int32 i = Lights.Num() - 1; i >= 0; --i)
	{
		FLight& L = Lights[i];
		UPointLightComponent* Light = L.Light.Get();
		const float T = (Elapsed - L.Start) / FMath::Max(L.Seconds, 0.05f);
		if (!Light || T >= 1.f)
		{
			if (Light)
			{
				Light->DestroyComponent();
			}
			Lights.RemoveAtSwap(i);
			continue;
		}
		// Up in the first tenth, then a fall like a bulb's.
		const float K = T < 0.1f ? T / 0.1f : FMath::Square(1.f - (T - 0.1f) / 0.9f);
		FTakeShapes::SetCueLight(Light, L.Candelas * K, L.Colour);
	}
}

// --- props ---------------------------------------------------------------------------------------------------------------------

AActor* FSetLifeStage::GetPropsActor()
{
	if (AActor* A = PropsActor.Get())
	{
		return A;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* A = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
	if (!A)
	{
		return nullptr;
	}
	USceneComponent* Root = NewObject<USceneComponent>(A, TEXT("SetLifeRoot"));
	Root->SetMobility(EComponentMobility::Movable);
	A->SetRootComponent(Root);
	Root->RegisterComponent();
	A->AddInstanceComponent(Root);
	PropsActor = A;
	return A;
}

int32 FSetLifeStage::HoldProp(int32 Role, ETakeShape Shape, const FVector& SizeCm, const FLinearColor& Colour,
	const FVector& Offset, FName Bone)
{
	ACastMember* M = Get(Role);
	if (!M || !M->Body)
	{
		return INDEX_NONE;
	}
	USceneComponent* Carrier = NewObject<USceneComponent>(M);
	Carrier->SetMobility(EComponentMobility::Movable);
	const bool bBone = !Bone.IsNone() && M->Body->DoesSocketExist(Bone);
	Carrier->SetupAttachment(M->Body, bBone ? Bone : NAME_None);
	Carrier->SetUsingAbsoluteRotation(true);
	Carrier->SetUsingAbsoluteScale(true);
	Carrier->RegisterComponent();
	M->AddInstanceComponent(Carrier);
	Carrier->SetWorldRotation(FRotator(0.f, M->GetActorRotation().Yaw, 0.f));
	UStaticMeshComponent* Mesh = FTakeShapes::AddShapeAtRuntime(M, Shape, Carrier, Offset, SizeCm / 100.f, Colour, false);
	FProp& P = Props.AddDefaulted_GetRef();
	P.Mesh = Mesh;
	P.Carrier = Carrier;
	P.Holder = M;
	return Props.Num() - 1;
}

int32 FSetLifeStage::AddProp(ETakeShape Shape, const FVector& WorldLocation, const FVector& SizeCm, const FLinearColor& Colour,
	const FRotator& Rotation)
{
	AActor* A = GetPropsActor();
	if (!A)
	{
		return INDEX_NONE;
	}
	UStaticMeshComponent* Mesh = FTakeShapes::AddShapeAtRuntime(A, Shape, A->GetRootComponent(), FVector::ZeroVector,
		SizeCm / 100.f, Colour, false);
	if (Mesh)
	{
		Mesh->SetWorldLocationAndRotation(WorldLocation, Rotation);
	}
	FProp& P = Props.AddDefaulted_GetRef();
	P.Mesh = Mesh;
	return Props.Num() - 1;
}

UStaticMeshComponent* FSetLifeStage::GetProp(int32 Prop) const
{
	return Props.IsValidIndex(Prop) ? Props[Prop].Mesh.Get() : nullptr;
}

void FSetLifeStage::DropProp(int32 Prop, const FVector& WorldLocation, const FRotator& Rotation)
{
	UStaticMeshComponent* Mesh = GetProp(Prop);
	if (!Mesh)
	{
		return;
	}
	Mesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	Mesh->SetWorldLocationAndRotation(WorldLocation, Rotation);
}

void FSetLifeStage::RemoveProp(int32 Prop)
{
	if (!Props.IsValidIndex(Prop))
	{
		return;
	}
	FProp& P = Props[Prop];
	if (UStaticMeshComponent* Mesh = P.Mesh.Get())
	{
		Mesh->DestroyComponent();
	}
	if (USceneComponent* Carrier = P.Carrier.Get())
	{
		Carrier->DestroyComponent();
	}
	P = FProp();	// ids stay stable
}

void FSetLifeStage::Flash(const FVector& WorldLocation, float Candelas, float Seconds, const FLinearColor& Colour)
{
	AActor* A = GetPropsActor();
	if (!A)
	{
		return;
	}
	UPointLightComponent* Light = NewObject<UPointLightComponent>(A);
	FTakeShapes::SetupCueLight(Light, 900.f);
	Light->SetupAttachment(A->GetRootComponent());
	Light->RegisterComponent();
	A->AddInstanceComponent(Light);
	Light->SetWorldLocation(WorldLocation);
	FTakeShapes::SetCueLight(Light, 0.f, Colour);
	FLight& L = Lights.AddDefaulted_GetRef();
	L.Light = Light;
	L.Start = Elapsed;
	L.Seconds = Seconds;
	L.Candelas = Candelas;
	L.Colour = Colour;
}

void FSetLifeStage::Cleanup()
{
	for (int32 i = 0; i < Props.Num(); ++i)
	{
		RemoveProp(i);
	}
	Props.Reset();
	for (FLight& L : Lights)
	{
		if (UPointLightComponent* Light = L.Light.Get())
		{
			Light->DestroyComponent();
		}
	}
	Lights.Reset();
	if (AActor* A = PropsActor.Get())
	{
		A->Destroy();
	}
	PropsActor.Reset();
}

// =============================================================================================================================
// USetLifeSubsystem
// =============================================================================================================================

USetLifeSubsystem* USetLifeSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<USetLifeSubsystem>() : nullptr;
}

bool USetLifeSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void USetLifeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	HudHandle = AHUD::OnHUDPostRender.AddUObject(this, &USetLifeSubsystem::DrawCaptions);
	VoiceAttenuation = NewObject<USoundAttenuation>(this);
	FSoundAttenuationSettings& A = VoiceAttenuation->Attenuation;
	A.bAttenuate = true;
	A.bSpatialize = true;
	A.AttenuationShape = EAttenuationShape::Sphere;
	A.AttenuationShapeExtents = FVector(300.f, 0.f, 0.f);
	A.FalloffDistance = 1200.f;
}

void USetLifeSubsystem::Deinitialize()
{
	AHUD::OnHUDPostRender.Remove(HudHandle);
	HudHandle.Reset();
	// The world is going (a take restart reloads the level): drop everything, touch no actor.
	Running.Reset();
	Returns.Reset();
	Barks.Reset();
	Claimed.Reset();
	Pool.Reset();
	Super::Deinitialize();
}

void USetLifeSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// The pool is gathered on the first tick, once every actor has begun play (the queue line registers itself then).
	bGathered = false;
	NextStartAt = InWorld.GetTimeSeconds() + FirstStartDelay;
}

TStatId USetLifeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USetLifeSubsystem, STATGROUP_Tickables);
}

void USetLifeSubsystem::GatherPool()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	bGathered = true;
	Pool.Reset();
	QueuePoints.Reset();
	if (const AQueueLine* Queue = AQueueLine::Get(this))
	{
		const FTransform& T = Queue->GetActorTransform();
		for (const FVector& P : Queue->Places) { QueuePoints.Add(T.TransformPosition(P)); }
		for (const FVector& P : Queue->WayIn) { QueuePoints.Add(T.TransformPosition(P)); }
		for (const FVector& P : Queue->InsideSpots) { QueuePoints.Add(T.TransformPosition(P)); }
		QueuePoints.Add(T.TransformPosition(Queue->ScanPoint));
		QueuePoints.Add(T.TransformPosition(Queue->ClerkFrom));
		if (!Queue->ClerkPoint.IsZero())
		{
			QueuePoints.Add(T.TransformPosition(Queue->ClerkPoint));
		}
		QueuePoints.Add(T.TransformPosition(Queue->DoormanPoint));
	}
	QueuePoints.Add(SetLifeLocal::ClerkSpot);
	QueuePoints.Add(SetLifeLocal::ClerkDesk);
	for (TActorIterator<ACastMember> It(World); It; ++It)
	{
		ACastMember* M = *It;
		if (!M || !SetLife::IsPoolKit(M->Kit) || M->GetAttachParentActor())
		{
			continue;
		}
		Pool.Add(M);
		// Whoever stands on the queue's places at the start belongs to the queue for the whole take.
		if (IsQueuePoint(M->GetActorLocation()))
		{
			Excluded.Add(M);
		}
	}
	TAKE_LOG(TEXT("SETLIFE ready pool=%d excluded=%d pieces=%d on=%d"), Pool.Num(), Excluded.Num(),
		FSetLifeRegistry::Pieces().Num(), CVarSetLife.GetValueOnGameThread());
}

bool USetLifeSubsystem::IsQueuePoint(const FVector& P) const
{
	for (const FVector& Q : QueuePoints)
	{
		if (FVector::Dist2D(P, Q) < SetLifeLocal::QueuePointRadius && FMath::Abs(P.Z - Q.Z) < 300.f)
		{
			return true;
		}
	}
	return false;
}

bool USetLifeSubsystem::IsClaimed(const ACastMember* Member) const
{
	return Member && Claimed.Contains(TWeakObjectPtr<ACastMember>(const_cast<ACastMember*>(Member)));
}

bool USetLifeSubsystem::IsFree(const ACastMember* Member) const
{
	if (!Member || !IsValid(Member) || Member->IsHidden() || !Member->bActing || !SetLife::IsPoolKit(Member->Kit)
		|| Member->GetAttachParentActor())
	{
		return false;
	}
	TWeakObjectPtr<ACastMember> Weak(const_cast<ACastMember*>(Member));
	if (Claimed.Contains(Weak) || Excluded.Contains(Weak))
	{
		return false;
	}
	// Someone else is driving it (the queue walks its guests), or it stands where the queue and the clerk work.
	return !Member->IsWalking() && !IsQueuePoint(Member->GetActorLocation());
}

FVector USetLifeSubsystem::GetJulian() const
{
	const ATakeDirector* Director = ATakeDirector::Get(this);
	const ATakeCharacter* Julian = Director ? Director->GetCharacter(ETakeCamera::A) : nullptr;
	return Julian ? Julian->GetActorLocation() - FVector(0.f, 0.f, Julian->GetSimpleCollisionHalfHeight()) : FVector::ZeroVector;
}

bool USetLifeSubsystem::GetCameraView(FVector& OutLocation, FVector& OutForward) const
{
	const APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!Cam)
	{
		return false;
	}
	OutLocation = Cam->GetCameraLocation();
	OutForward = Cam->GetCameraRotation().Vector();
	return true;
}

bool USetLifeSubsystem::IsInView(const FVector& P) const
{
	FVector L, F;
	if (!GetCameraView(L, F))
	{
		return false;
	}
	const FVector To = P - L;
	const double D = To.Size();
	return D > 1.0 && FVector::DotProduct(To / D, F) > 0.79;	// about 75 degrees across
}

bool USetLifeSubsystem::IsNearFocus(const FVector& P) const
{
	const FVector Julian = GetJulian();
	if (!Julian.IsZero() && FVector::Dist2D(P, Julian) < FocusRadius)
	{
		return true;
	}
	FVector L, F;
	return GetCameraView(L, F) && FVector::Dist(P, L) < ViewRadius && IsInView(P);
}

int32 USetLifeSubsystem::GetGate(ESetLifeZone Zone, FString& OutWhy) const
{
	if (CVarSetLife.GetValueOnGameThread() == 0)
	{
		OutWhy = TEXT("off");
		return 2;
	}
	const ATakeDirector* Director = ATakeDirector::Get(this);
	if (!Director)
	{
		return 0;
	}
	if (Director->bAlarm)
	{
		OutWhy = TEXT("alarm");
		return 2;
	}
	if (Zone == ESetLifeZone::Hall)
	{
		if (const AHouseDimmer* Dimmer = ::Cast<AHouseDimmer>(Director->HeistDimmer.Get()))
		{
			if (Dimmer->IsRoomBlind())
			{
				OutWhy = TEXT("dark");
				return 2;
			}
		}
	}
	if (Director->Phase != ETakePhase::Rolling && Director->Phase != ETakePhase::Cleared)
	{
		OutWhy = TEXT("phase");
		return 1;
	}
	if (Director->IsSlowed())
	{
		OutWhy = TEXT("slow");
		return 1;
	}
	if (const AQueueLine* Queue = AQueueLine::Get(this))
	{
		if (Queue->IsTeaching())
		{
			OutWhy = TEXT("teaching");
			return 1;
		}
	}
	return 0;
}

void USetLifeSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UWorld* World = GetWorld();
	if (!World || !World->HasBegunPlay())
	{
		return;
	}
	if (!bGathered)
	{
		GatherPool();
	}
	const float Now = World->GetTimeSeconds();
	const bool bOn = CVarSetLife.GetValueOnGameThread() != 0;
	if (bOn != bWasOn)
	{
		TAKE_LOG(TEXT("SETLIFE %s"), bOn ? TEXT("on") : TEXT("off"));
		bWasOn = bOn;
	}

	// The pieces: step them, end those the director's state or distance ends.
	for (int32 i = 0; i < Running.Num(); ++i)
	{
		FSetLifeStage& S = *Running[i];
		FString Why;
		const int32 Gate = GetGate(S.GetZone(), Why);
		if (Gate == 2)
		{
			S.Abort(Why);
		}
		else if (!S.bAnchorSet && S.Roles.Num() > 0)
		{
			S.Anchor = S.Where(0);
		}
		if (!S.bDone && S.Elapsed > 3.f)
		{
			const FVector Julian = GetJulian();
			if (!Julian.IsZero() && FVector::Dist2D(S.Anchor, Julian) > LeaveRadius && !IsInView(S.Anchor))
			{
				S.Abort(TEXT("left behind"));
			}
		}
		if (!S.bDone)
		{
			S.Step(DeltaTime, Gate == 1);
		}
		if (S.bDone)
		{
			FinishStage(S);
			Running.RemoveAt(i);
			--i;
		}
	}

	TickReturns(Now);
	TickBarks(Now);
	if (bOn)
	{
		Schedule(Now);
	}
}

void USetLifeSubsystem::Schedule(float Now)
{
	if (Now < NextStartAt || Running.Num() >= FMath::Clamp(CVarSetLifeMax.GetValueOnGameThread(), 0, 8))
	{
		return;
	}
	NextStartAt = Now + 0.5f;	// look again in half a second whatever happens
	// The zones on show: Julian's and the camera's (Maya's camera on the catwalk looks into the hall).
	TArray<ESetLifeZone, TInlineAllocator<3>> Zones;
	const FVector Julian = GetJulian();
	if (!Julian.IsZero())
	{
		Zones.AddUnique(SetLife::ZoneOf(Julian));
	}
	FVector CamL, CamF;
	if (GetCameraView(CamL, CamF))
	{
		Zones.AddUnique(SetLife::ZoneOf(CamL));
		Zones.AddUnique(SetLife::ZoneOf(CamL + CamF * 1500.f));
	}
	TArray<const FSetLifePieceInfo*> Candidates;
	float Total = 0.f;
	for (const FSetLifePieceInfo& Info : FSetLifeRegistry::Pieces())
	{
		if (!Zones.Contains(Info.Zone) || !Info.Make)
		{
			continue;
		}
		const float* Until = CooldownUntil.Find(Info.Name);
		if (Until && Now < *Until)
		{
			continue;
		}
		bool bRunning = false;
		for (const TUniquePtr<FSetLifeStage>& S : Running)
		{
			bRunning |= S->GetName() == Info.Name;
		}
		FString Why;
		if (bRunning || GetGate(Info.Zone, Why) != 0)
		{
			continue;
		}
		Candidates.Add(&Info);
		Total += Info.Weight;
	}
	// Julian's own zone first: while nothing runs where he stands, only its pieces are tried, so the camera's
	// look-ahead zone (the lobby, seen from the queue) cannot take the first slot from the street's piece.
	if (!Julian.IsZero())
	{
		const ESetLifeZone Home = SetLife::ZoneOf(Julian);
		bool bHomeBusy = false;
		for (const TUniquePtr<FSetLifeStage>& S : Running)
		{
			bHomeBusy |= S->GetZone() == Home;
		}
		if (!bHomeBusy && Candidates.ContainsByPredicate([Home](const FSetLifePieceInfo* I) { return I->Zone == Home; }))
		{
			Candidates.RemoveAll([Home](const FSetLifePieceInfo* I) { return I->Zone != Home; });
			Total = 0.f;
			for (const FSetLifePieceInfo* I : Candidates)
			{
				Total += I->Weight;
			}
		}
	}
	// Weighted order, at most three tries a look.
	for (int32 Try = 0; Try < 3 && Candidates.Num() > 0; ++Try)
	{
		float R = FMath::FRand() * Total;
		int32 Index = Candidates.Num() - 1;
		for (int32 i = 0; i < Candidates.Num(); ++i)
		{
			R -= Candidates[i]->Weight;
			if (R <= 0.f)
			{
				Index = i;
				break;
			}
		}
		const FSetLifePieceInfo* Info = Candidates[Index];
		if (TryStart(*Info, Now))
		{
			NextStartAt = Now + FMath::FRandRange(StartGapMin, StartGapMax);
			return;
		}
		CooldownUntil.Add(Info->Name, Now + RetryDelay);
		Total -= Info->Weight;
		Candidates.RemoveAt(Index);
	}
}

bool USetLifeSubsystem::TryStart(const FSetLifePieceInfo& Info, float Now)
{
	TUniquePtr<FSetLifeStage> Stage = MakeUnique<FSetLifeStage>(this, Info, Info.Make());
	if (!Stage->Piece)
	{
		return false;
	}
	const bool bOk = Stage->Piece->Setup(*Stage) && Stage->Roles.Num() > 0 && Stage->Steps.Num() > 0;
	if (!bOk)
	{
		TAKE_LOG(TEXT("SETLIFE skip %s (no cast) roles=%d"), *Info.Name.ToString(), Stage->Roles.Num());
		// Nothing has moved: release the claims and remove any props at once.
		for (const FSetLifeStage::FCast& C : Stage->Roles)
		{
			Claimed.Remove(C.Member);
		}
		Stage->Cleanup();
		return false;
	}
	if (!Stage->bAnchorSet)
	{
		Stage->Anchor = Stage->Where(0);
	}
	TAKE_LOG(TEXT("SETLIFE start %s cast=%s at=(%.0f,%.0f) zone=%s"), *Info.Name.ToString(), *Stage->CastList(),
		Stage->Anchor.X, Stage->Anchor.Y, SetLife::ZoneName(Info.Zone));
	Running.Add(MoveTemp(Stage));
	return true;
}

void USetLifeSubsystem::FinishStage(FSetLifeStage& S)
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (S.Piece)
	{
		S.Piece->End(S, S.bAborted);
	}
	S.Cleanup();
	for (FSetLifeStage::FCast& C : S.Roles)
	{
		if (ACastMember* M = C.Member.Get())
		{
			BeginReturn(M, C.Home, C.HomeYaw, C.Via, true);
		}
		else
		{
			Claimed.Remove(C.Member);
		}
	}
	// Barks of a piece that ended stop with it.
	for (int32 i = Barks.Num() - 1; i >= 0; --i)
	{
		for (const FSetLifeStage::FCast& C : S.Roles)
		{
			if (Barks.IsValidIndex(i) && Barks[i].Speaker == C.Member && S.bAborted)
			{
				Barks.RemoveAt(i);
				break;
			}
		}
	}
	const FSetLifePieceInfo* Info = FSetLifeRegistry::Pieces().FindByPredicate(
		[&S](const FSetLifePieceInfo& I) { return I.Name == S.GetName(); });
	CooldownUntil.Add(S.GetName(), Now + (Info ? Info->Cooldown : 45.f) * FMath::FRandRange(0.8f, 1.2f));
	TAKE_LOG(TEXT("SETLIFE end %s %s t=%.1f"), *S.GetName().ToString(),
		S.bAborted ? *FString::Printf(TEXT("abort(%s)"), *S.AbortWhy) : TEXT("ok"), S.Elapsed);
}

void USetLifeSubsystem::AbortAll(const FString& Why)
{
	for (TUniquePtr<FSetLifeStage>& S : Running)
	{
		S->Abort(Why);
	}
}

// --- giving the cast back --------------------------------------------------------------------------------------------------

void USetLifeSubsystem::BeginReturn(ACastMember* Member, const FVector& Home, float HomeYaw, const TArray<FVector>& Via, bool bTurn)
{
	FReturn& R = Returns.AddDefaulted_GetRef();
	R.Member = Member;
	R.Path = Via;
	R.Home = Home;
	R.HomeYaw = HomeYaw;
	R.bTurn = bTurn && !Member->bSeated;
	R.Started = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
}

void USetLifeSubsystem::TickReturns(float Now)
{
	FVector CamL, CamF;
	const bool bCam = GetCameraView(CamL, CamF);
	for (int32 i = Returns.Num() - 1; i >= 0; --i)
	{
		FReturn& R = Returns[i];
		ACastMember* M = R.Member.Get();
		bool bHome = false;
		if (!M)
		{
			Claimed.Remove(R.Member);
			Returns.RemoveAtSwap(i);
			continue;
		}
		if (M->bSeated)
		{
			bHome = true;
		}
		else if (R.bSliding)
		{
			// A short way (a step back to undo): slide, no turn.
			const float T = FMath::Clamp((Now - R.SlideStart) / 0.5f, 0.f, 1.f);
			FVector P = FMath::Lerp(R.SlideFrom, R.Home, FMath::InterpEaseInOut(0.f, 1.f, T, 2.f));
			P.Z = M->GetActorLocation().Z;
			M->SetActorLocation(P);
			bHome = T >= 1.f;
		}
		else if (!M->IsWalking() || !R.bKicked)
		{
			R.bKicked = true;
			const FVector Here = M->GetActorLocation();
			const bool bUnseen = bCam && FVector::Dist(Here, CamL) > 1200.f && M->Body && !M->Body->WasRecentlyRendered(0.5f);
			if ((bUnseen && FVector::Dist2D(Here, R.Home) > 60.f) || Now - R.Started > SetLifeLocal::ReturnTimeout)
			{
				// Nobody is looking (or it took too long): snap to just short of home and finish with a tiny walk,
				// so the member leaves the walk blend space properly (WalkTo's stop puts its idle back).
				const FVector Dir = (Here - R.Home).GetSafeNormal2D();
				M->SetActorLocation(FVector(R.Home.X, R.Home.Y, Here.Z) + Dir * 20.f);
				M->WalkTo(R.Home, 60.f);
				R.Path.Reset();
				R.Started = Now - SetLifeLocal::ReturnTimeout + 3.f;	// the tiny walk gets 3 s
			}
			else if (R.Path.Num() > 0)
			{
				M->WalkTo(R.Path[0], 110.f);
				R.Path.RemoveAt(0);
			}
			else if (FVector::Dist2D(Here, R.Home) > 50.f || M->IsWalking())
			{
				M->WalkTo(R.Home, 110.f);
			}
			else
			{
				R.bSliding = true;
				R.SlideFrom = Here;
				R.SlideStart = Now;
			}
		}
		if (bHome)
		{
			if (R.bTurn)
			{
				M->FaceYaw(R.HomeYaw);
			}
			Claimed.Remove(R.Member);
			Returns.RemoveAtSwap(i);
		}
	}
}

// --- barks -----------------------------------------------------------------------------------------------------------------

void USetLifeSubsystem::Say(ACastMember* Speaker, const FString& Line, FName Sound, float Seconds, FName Piece)
{
	UWorld* World = GetWorld();
	if (!Speaker || !World || Line.IsEmpty())
	{
		return;
	}
	const float Now = World->GetTimeSeconds();
	const float Dur = Seconds > 0.f ? Seconds : FMath::Clamp(1.3f + 0.055f * Line.Len(), 1.8f, 5.f);
	// One line per speaker at a time.
	Barks.RemoveAll([Speaker](const FBark& B) { return B.Speaker.Get() == Speaker; });
	FBark& B = Barks.AddDefaulted_GetRef();
	B.Speaker = Speaker;
	B.Line = Line;
	B.Start = Now;
	B.Until = Now + Dur;
	NextBarkCheck = 0.f;
	const FVector Julian = GetJulian();
	const float D = Julian.IsZero() ? 1e9f : static_cast<float>(FVector::Dist(Julian, Speaker->GetActorLocation()));
	bool bVoiced = false;
	if (!Sound.IsNone() && D < VoiceRadius)
	{
		if (UTakeAudioComponent* Audio = UTakeAudioComponent::Get(this))
		{
			if (USoundBase* S = Audio->FindSound(Sound))
			{
				UGameplayStatics::PlaySoundAtLocation(this, S, Speaker->GetActorLocation() + FVector(0.f, 0.f, 160.f),
					FRotator::ZeroRotator, 1.f, 1.f, 0.f, VoiceAttenuation);
				bVoiced = true;
			}
		}
	}
	TAKE_LOG(TEXT("SETLIFE say %s %s \"%s\" sound=%s%s d=%.0f"), *Piece.ToString(), *Speaker->GetName(), *Line,
		*Sound.ToString(), bVoiced ? TEXT("") : TEXT("(none)"), D);
}

void USetLifeSubsystem::TickBarks(float Now)
{
	Barks.RemoveAll([Now](const FBark& B) { return !B.Speaker.IsValid() || Now > B.Until; });
	if (Barks.Num() == 0 || Now < NextBarkCheck)
	{
		return;
	}
	NextBarkCheck = Now + 0.2f;
	UWorld* World = GetWorld();
	const FVector Julian = GetJulian();
	FVector CamL, CamF;
	const bool bCam = GetCameraView(CamL, CamF);
	const ATakeDirector* Director = ATakeDirector::Get(this);
	const AActor* JulianActor = Director ? Director->GetCharacter(ETakeCamera::A) : nullptr;
	for (FBark& B : Barks)
	{
		const ACastMember* M = B.Speaker.Get();
		B.bSeen = false;
		if (!M || Julian.IsZero() || !bCam || FVector::Dist(M->GetActorLocation(), Julian) > CaptionRadius)
		{
			continue;
		}
		const FVector Head = M->GetActorLocation() + FVector(0.f, 0.f, M->bSeated ? 120.f : 165.f);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(SetLifeBark), false);
		Params.AddIgnoredActor(M);
		if (JulianActor)
		{
			Params.AddIgnoredActor(JulianActor);
		}
		FHitResult Hit;
		B.bSeen = !World->LineTraceSingleByChannel(Hit, CamL, Head, ECC_Visibility, Params);
	}
}

void USetLifeSubsystem::DrawCaptions(AHUD* HUD, UCanvas* DebugCanvas)
{
	if (!HUD || HUD->GetWorld() != GetWorld() || Barks.Num() == 0 || CVarSetLife.GetValueOnGameThread() == 0)
	{
		return;
	}
	UCanvas* Canvas = SetLifeLocal::FHudCanvas::Of(HUD);
	Canvas = Canvas ? Canvas : DebugCanvas;
	UWorld* World = GetWorld();
	FVector CamL, CamF;
	if (!Canvas || !World || !GetCameraView(CamL, CamF))
	{
		return;
	}
	using namespace TakeStyle;
	const float Now = World->GetTimeSeconds();
	Kit.Begin(Canvas, FPlatformTime::Seconds(), LetterboxBar(Canvas->ClipX, Canvas->ClipY));
	const float Scale = 0.72f;
	const float EmH = Kit.Em(EType::Line) * Scale;
	const float Pad = Kit.Px(10.f);
	TArray<FBox2D, TInlineAllocator<4>> Drawn;
	for (const FBark& B : Barks)
	{
		const ACastMember* M = B.Speaker.Get();
		if (!M || !B.bSeen)
		{
			continue;
		}
		FVector Head = M->GetActorLocation() + FVector(0.f, 0.f, M->bSeated ? 120.f : 165.f);
		if (M->Body && M->Body->DoesSocketExist(TEXT("head")))
		{
			Head = M->Body->GetSocketLocation(TEXT("head"));
		}
		Head.Z += 38.f;
		if (FVector::DotProduct((Head - CamL).GetSafeNormal(), CamF) < 0.3)
		{
			continue;
		}
		const FVector S = Canvas->Project(Head);
		if (S.X < 0.f || S.X > Canvas->ClipX || S.Y < 0.f || S.Y > Canvas->ClipY)
		{
			continue;
		}
		const float Alpha = FMath::Min(FMath::Clamp((Now - B.Start) / 0.15f, 0.f, 1.f), FMath::Clamp((B.Until - Now) / 0.3f, 0.f, 1.f));
		if (Alpha <= 0.01f)
		{
			continue;
		}
		const float TextW = Kit.Measure(EType::Line, B.Line) * Scale;
		const float X = FMath::Clamp(static_cast<float>(S.X), TextW * 0.5f + Pad * 2.f, Canvas->ClipX - TextW * 0.5f - Pad * 2.f);
		float Y = FMath::Clamp(static_cast<float>(S.Y), Kit.PicY + EmH, Kit.PicBottom() - EmH);
		FBox2D Box(FVector2D(X - TextW * 0.5f - Pad, Y - EmH * 0.8f), FVector2D(X + TextW * 0.5f + Pad, Y + EmH * 0.8f));
		for (int32 Guard = 0; Guard < 4; ++Guard)
		{
			bool bHit = false;
			for (const FBox2D& D : Drawn)
			{
				bHit |= D.Intersect(Box);
			}
			if (!bHit)
			{
				break;
			}
			Y -= EmH * 1.7f;
			Box = Box.ShiftBy(FVector2D(0.f, -EmH * 1.7f));
		}
		Drawn.Add(Box);
		Kit.Rect(Box.Min.X, Box.Min.Y, Box.Max.X - Box.Min.X, Box.Max.Y - Box.Min.Y, FTakeHudKit::A(Ink, 0.5f * Alpha));
		Kit.Push(FVector2D(X, Y), 0.f, Scale);
		Kit.Text(EType::Line, B.Line, X, Y, FTakeHudKit::A(Bone, Alpha), ETakeAlign::Centre);
		Kit.Pop();
	}
}
