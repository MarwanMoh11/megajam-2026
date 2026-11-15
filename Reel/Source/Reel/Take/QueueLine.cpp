// Scene 1's queue and its taught first switch (QA notes 1, 2, 10). Owner: scene1. The beat sheet is in QueueLine.h.

#include "QueueLine.h"

#include "CastMember.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Device.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FollowSpot.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Julian.h"
#include "Mark.h"
#include "ScannerWatcher.h"
#include "TakeCharacter.h"
#include "TakeDirector.h"
#include "TakeGameInstance.h"
#include "TakePlayerController.h"

// Unity builds merge .cpp files: file-local helpers live in a uniquely named namespace.
namespace QueueLineLocal
{
	/** The line that began play last (one per level). */
	TWeakObjectPtr<AQueueLine> GCurrent;

	/** ATakeDirector::BeginSlow / EndSlow's reason for the taught beats. */
	FName SlowReason() { return FName(TEXT("QueueTutor")); }

	const TCHAR* BeatName(EQueueBeat Beat)
	{
		switch (Beat)
		{
		case EQueueBeat::Queue:		return TEXT("Queue");
		case EQueueBeat::Refuse:	return TEXT("Refuse");
		case EQueueBeat::Aside:		return TEXT("Aside");
		case EQueueBeat::AskCut:	return TEXT("AskCut");
		case EQueueBeat::AskUse:	return TEXT("AskUse");
		case EQueueBeat::AskBack:	return TEXT("AskBack");
		case EQueueBeat::Through:	return TEXT("Through");
		case EQueueBeat::Lobby:		return TEXT("Lobby");
		case EQueueBeat::Done:		return TEXT("Done");
		default:					return TEXT("?");
		}
	}

	/** The yaw from A to B (2D), degrees; Fallback when they meet. */
	float YawTo(const FVector& A, const FVector& B, float Fallback)
	{
		const FVector D(B.X - A.X, B.Y - A.Y, 0.0);
		return D.IsNearlyZero() ? Fallback : static_cast<float>(D.Rotation().Yaw);
	}

	/** How long the HUD keeps a key prompt flashed for a taught beat, s (real time: the HUD's clocks are). */
	constexpr float PromptSeconds = 8.f;
}

AQueueLine::AQueueLine()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AQueueLine::BeginPlay()
{
	Super::BeginPlay();
	QueueLineLocal::GCurrent = this;
}

void AQueueLine::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (QueueLineLocal::GCurrent.Get() == this)
	{
		QueueLineLocal::GCurrent.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

AQueueLine* AQueueLine::Get(const UObject* WorldContextObject)
{
	AQueueLine* const Line = QueueLineLocal::GCurrent.Get();
	if (!Line || !WorldContextObject || Line->GetWorld() != WorldContextObject->GetWorld())
	{
		return nullptr;
	}
	return Line;
}

void AQueueLine::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// The first frame: every actor has begun play (the cast stands where the map put it), so Julian goes onto his
	// place and the line is built before the opening slate is over. Then the director steps the line (StepLine).
	if (ATakeDirector* const Director = ATakeDirector::Get(this))
	{
		if (!bJulianPlaced)
		{
			PlaceJulian(Director);
		}
		if (!bSetUp)
		{
			SetupLine(*Director);
		}
	}
	SetActorTickEnabled(false);
}

double AQueueLine::RealNow() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetRealTimeSeconds() : 0.0;
}

FVector AQueueLine::ToWorld(const FVector& Local) const
{
	return GetActorTransform().TransformPosition(Local);
}

AScannerWatcher* AQueueLine::FindScanner(const ATakeDirector& Director) const
{
	for (AWatcher* EachWatcher : Director.Watchers)
	{
		if (AScannerWatcher* const Scanner = Cast<AScannerWatcher>(EachWatcher))
		{
			return Scanner;
		}
	}
	return nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
// Setting up: Julian on his place, the guests found, the line built
// ---------------------------------------------------------------------------------------------------------------------

void AQueueLine::PlaceJulian(ATakeDirector* Director)
{
	bJulianPlaced = true;
	if (!Director || !Director->bQueueScene1 || Places.Num() == 0 || !Director->Julian)
	{
		return;
	}
	const UTakeGameInstance* const GI = Director->GetTakeGameInstance();
	if (!GI || !GI->CheckpointMark.IsEmpty())
	{
		// Restored past the street: the checkpoint places him (ATakeDirector::RestoreCheckpoint).
		return;
	}

	const bool bRetake = (GI->IntroShownMask & BitQueueWalked) != 0;
	const int32 Place = FMath::Clamp(bRetake ? RetakeJulianPlace : JulianPlace, 0, Places.Num() - 1);
	AJulian* const Julian = Director->Julian.Get();
	const FVector Feet = ToWorld(Places[Place]);
	const FVector FeetToCentre = Julian->GetActorLocation() - Julian->GetFeetLocation();
	Julian->SetActorLocationAndRotation(Feet + FeetToCentre, FRotator(0.f, JulianStartYaw, 0.f), false, nullptr,
		ETeleportType::TeleportPhysics);
	if (UCharacterMovementComponent* const Move = Julian->GetCharacterMovement())
	{
		Move->StopMovementImmediately();
	}
	Julian->SetLook(JulianStartYaw, Julian->StartLookPitch);
	Julian->SnapFollowCamera();
	TAKE_LOG(TEXT("QUEUE Julian on place %d of %d%s (%.0f, %.0f)"), Place, Places.Num(), bRetake ? TEXT(" (retake)") : TEXT(""),
		Feet.X, Feet.Y);
}

void AQueueLine::SetupLine(ATakeDirector& Director)
{
	bSetUp = true;
	bOldFlow = !Director.bQueueScene1;
	StepTimer = FirstStepSeconds;

	UTakeGameInstance* const GI = Director.GetTakeGameInstance();
	const bool bRetake = GI && (GI->IntroShownMask & BitQueueWalked) != 0;
	const bool bPastStreet = GI && !GI->CheckpointMark.IsEmpty();

	// The guests: the nearest free standing cast member to each place but Julian's (on the map's own layout).
	TArray<ACastMember*> Candidates;
	for (TActorIterator<ACastMember> It(GetWorld()); It; ++It)
	{
		ACastMember* const Member = *It;
		if (Member && !Member->GetAttachParentActor() && !Member->bSeated)
		{
			Candidates.Add(Member);
		}
	}
	const int32 JulianAt = FMath::Clamp(JulianPlace, 0, FMath::Max(0, Places.Num() - 1));
	for (int32 PlaceIndex = 0; PlaceIndex < Places.Num(); ++PlaceIndex)
	{
		if (PlaceIndex == JulianAt)
		{
			Line.Add(INDEX_NONE);
			continue;
		}
		const FVector Point = ToWorld(Places[PlaceIndex]);
		ACastMember* Best = nullptr;
		double BestDistance = GuestFindRadius;
		for (ACastMember* const Candidate : Candidates)
		{
			const double Distance = FVector::Dist2D(Candidate->GetActorLocation(), Point);
			if (Distance <= BestDistance)
			{
				Best = Candidate;
				BestDistance = Distance;
			}
		}
		if (!Best)
		{
			TAKE_LOG(TEXT("QUEUE place %d: no guest within %.0f cm (the line closes up)"), PlaceIndex, GuestFindRadius);
			continue;
		}
		Candidates.Remove(Best);
		const int32 GuestIndex = Guests.AddDefaulted();
		FQueueGuest& Guest = Guests[GuestIndex];
		Guest.Member = Best;
		Guest.TargetPlace = PlaceIndex;
		Guest.LegTarget = Best->GetActorLocation();
		if (bGuestsBlock && !bOldFlow)
		{
			UCapsuleComponent* const Capsule = NewObject<UCapsuleComponent>(this, NAME_None, RF_Transient);
			Capsule->InitCapsuleSize(BlockRadius, BlockHalfHeight);
			Capsule->SetCanEverAffectNavigation(false);
			Capsule->SetCollisionObjectType(ECC_WorldDynamic);
			Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
			Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);	// Julian only: cameras, sight and aim pass
			Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Capsule->SetHiddenInGame(true);
			Capsule->RegisterComponent();
			AddInstanceComponent(Capsule);
			Guest.Blocker = Capsule;
			SetBlocker(Guest, true);
		}
		Line.Add(GuestIndex);
	}
	if (!Line.Contains(INDEX_NONE))
	{
		Line.Add(INDEX_NONE);
	}

	// The old flow, a later take or a checkpoint past the street: the guests before Julian are already inside.
	int32 SendIn = 0;
	if (bOldFlow || bPastStreet)
	{
		SendIn = JulianIndex();
	}
	else if (bRetake)
	{
		SendIn = FMath::Max(0, JulianIndex() - FMath::Max(0, RetakeJulianPlace));
	}
	for (int32 Each = 0; Each < SendIn && Line.Num() > 0 && Line[0] != INDEX_NONE; ++Each)
	{
		FQueueGuest& Guest = Guests[Line[0]];
		Line.RemoveAt(0);
		const FVector Spot = NextInsideSpot(Guest);
		PutGuest(Guest, Spot, QueueLineLocal::YawTo(ToWorld(ScanPoint), Spot, 0.f));
		Guest.TargetPlace = INDEX_NONE;
		Guest.Leg = WayIn.Num() + 1;
		Guest.bInside = true;
		SetBlocker(Guest, false);
	}
	if (SendIn > 0 && !bOldFlow && !bPastStreet)
	{
		// The rest stand on their new places at once (a retake starts with the line already moved up).
		for (int32 Index = 0; Index < Line.Num(); ++Index)
		{
			if (Line[Index] != INDEX_NONE && Places.IsValidIndex(Index))
			{
				FQueueGuest& Guest = Guests[Line[Index]];
				const float Yaw = Index > 0 ? QueueLineLocal::YawTo(Places[Index], Places[Index - 1], 0.f) : 0.f;
				PutGuest(Guest, ToWorld(Places[Index]), Yaw);
				Guest.TargetPlace = Index;
			}
		}
	}
	if (bPastStreet)
	{
		Beat = EQueueBeat::Through;
	}
	TAKE_LOG(TEXT("QUEUE set up: %d guests, Julian %d in line, %d sent in%s%s"), Guests.Num(), JulianIndex(), SendIn,
		bOldFlow ? TEXT(", the old flow") : TEXT(""), bRetake ? TEXT(", a retake") : TEXT(""));

	// --- W3 opening: the searchlight (the B device nearest the scanner), the doorman, the hold wall, the clerk.
	AScannerWatcher* const Scanner = FindScanner(Director);
	if (Scanner)
	{
		double BestDistance = TNumericLimits<double>::Max();
		for (ADevice* const EachDevice : Director.Devices)
		{
			AFollowSpot* const Spot = Cast<AFollowSpot>(EachDevice);
			if (Spot && Spot->FiredFrom == ETakeCamera::B)
			{
				const double Distance = FVector::Dist2D(Spot->GetActorLocation(), Scanner->GetActorLocation());
				if (Distance < BestDistance)
				{
					BestDistance = Distance;
					Searchlight = Spot;
				}
			}
		}
	}
	if (!bOldFlow && !bPastStreet)
	{
		SpawnDoorman(Director);
		if (Doorman && Scanner)
		{
			Scanner->BlindedLine.Reset();	// he says it, on the landing frame, in shot
		}
		if (bHoldLine)
		{
			HoldBox = NewObject<UBoxComponent>(this, NAME_None, RF_Transient);
			HoldBox->SetCanEverAffectNavigation(false);
			HoldBox->SetCollisionObjectType(ECC_WorldDynamic);
			HoldBox->SetCollisionResponseToAllChannels(ECR_Ignore);
			HoldBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);	// people only: cameras, sight and aim pass
			HoldBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			HoldBox->SetHiddenInGame(true);
			HoldBox->SetBoxExtent(FVector(10.f, 10.f, 10.f), false);
			HoldBox->RegisterComponent();
			AddInstanceComponent(HoldBox);
		}
	}
	if (bLobbyGreet)
	{
		FindClerk();
	}
	TAKE_LOG(TEXT("QUEUE opening: searchlight %s, doorman %s, hold %s, clerk %s"), Searchlight.IsValid() ? TEXT("yes") : TEXT("no"),
		Doorman ? TEXT("yes") : TEXT("no"), HoldBox ? TEXT("yes") : TEXT("no"), Clerk.IsValid() ? TEXT("yes") : TEXT("no"));
}

// ---------------------------------------------------------------------------------------------------------------------
// W3 opening: the hold wall, the doorman at the scanner, the clerk in the lobby
// ---------------------------------------------------------------------------------------------------------------------

void AQueueLine::StepHold(ATakeDirector& Director)
{
	if (!HoldBox || bHoldDone)
	{
		return;
	}
	AJulian* const Julian = Director.Julian.Get();
	AScannerWatcher* const Scanner = FindScanner(Director);
	const int32 JulianAt = JulianIndex();
	const bool bPast = Beat == EQueueBeat::Through || Beat == EQueueBeat::Lobby || Beat == EQueueBeat::Done
		|| (Scanner && Julian && Scanner->HasPassed(Julian));
	if (bPast || !Julian || JulianAt < 0 || Places.Num() == 0)
	{
		HoldBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		bHoldOn = false;
		bHoldDone = bPast;
		return;
	}

	// The near face: just behind the one ahead of him, never on his own place; at the front, past the gold mark.
	const FVector Mine = ToWorld(Places[FMath::Min(JulianAt, Places.Num() - 1)]);
	float Edge = static_cast<float>(Mine.X) + FrontHoldLead;
	if (JulianAt > 0)
	{
		const FVector Ahead = ToWorld(Places[FMath::Min(JulianAt - 1, Places.Num() - 1)]);
		Edge = FMath::Max(static_cast<float>(Ahead.X) - HoldGap, static_cast<float>(Mine.X) + HoldMinLead);
	}
	const bool bOpen = JulianAt == 0 && Scanner && !Scanner->IsLive();	// his turn and the scanner blind: go
	const float Radius = Julian->GetCapsuleComponent() ? Julian->GetCapsuleComponent()->GetScaledCapsuleRadius() : 40.f;
	const FVector Feet = Julian->GetFeetLocation();
	bool bWant = !bOpen && Edge < HoldFarX - 10.f;
	if (bWant && !bHoldOn && Feet.X + Radius > Edge - 2.f)
	{
		bWant = false;	// already past where it would stand (the blind ran out on him): never box him in
	}
	if (bWant)
	{
		const float HalfX = 0.5f * (HoldFarX - Edge);
		HoldBox->SetBoxExtent(FVector(HalfX, 0.5f * (HoldMaxY - HoldMinY), 0.5f * HoldHeight), false);
		HoldBox->SetWorldLocationAndRotation(FVector(Edge + HalfX, 0.5f * (HoldMinY + HoldMaxY), 0.5f * HoldHeight),
			FRotator::ZeroRotator);
	}
	if (bWant != bHoldOn)
	{
		bHoldOn = bWant;
		HoldBox->SetCollisionEnabled(bWant ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		TAKE_LOG(TEXT("QUEUE hold %s (edge x %.0f, Julian %d in line)"), bWant ? TEXT("on") : TEXT("off"), Edge, JulianAt);
	}

	// Walking into it: the line answers (the guest ahead turns to him, or the doorman's stop hand), every so often.
	const UCharacterMovementComponent* const Move = Julian->GetCharacterMovement();
	const double Now = RealNow();
	if (bHoldOn && Move && Move->GetCurrentAcceleration().X > 1.f && Feet.X + Radius >= Edge - NudgeDistance
		&& Feet.Y > HoldMinY - Radius && Feet.Y < HoldMaxY + Radius && Now - LastNudgeAt > NudgeCooldown)
	{
		LastNudgeAt = Now;
		const FVector Head = Julian->GetActorLocation() + FVector(0.f, 0.f, 60.f);
		ACastMember* const Ahead = (JulianAt > 0 && Line.IsValidIndex(JulianAt - 1) && Line[JulianAt - 1] != INDEX_NONE)
			? Guests[Line[JulianAt - 1]].Member.Get() : nullptr;
		if (Ahead)
		{
			Ahead->LookAtPoint(Head, 2.5f);
			Ahead->PlayGestureAt(TEXT("Shrug"), Head);
			Director.Flash(NudgeGuestLine, ETakeFlashKind::Line, 2.f);
		}
		else if (Doorman)
		{
			Doorman->FaceYaw(QueueLineLocal::YawTo(Doorman->GetActorLocation(), Head, DoormanYaw));
			Doorman->PlayGestureAt(TEXT("Refuse"), Head);
			Doorman->LookAtPoint(Head, 2.5f);
			if (Beat == EQueueBeat::Queue)
			{
				Director.Flash(NudgeDoormanLine, ETakeFlashKind::Line, 2.f);
			}
		}
		TAKE_LOG(TEXT("QUEUE nudge: Julian at the hold (%s)"), Ahead ? TEXT("the guest ahead") : TEXT("the doorman"));
	}
}

void AQueueLine::SpawnDoorman(ATakeDirector& Director)
{
	if (!bDoorman || Doorman || !GetWorld())
	{
		return;
	}
	const FTransform At(FRotator(0.f, DoormanYaw, 0.f), DoormanPoint);
	ACastMember* const Member = GetWorld()->SpawnActorDeferred<ACastMember>(ACastMember::StaticClass(), At, this, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Member)
	{
		return;
	}
	Member->Kit = ECastKit::Doorman;
	Member->Tint = DoormanTint;
	Member->FinishSpawning(At);
	Member->FaceYaw(DoormanYaw);
	Doorman = Member;
}

void AQueueLine::StepDoorman(ATakeDirector& Director)
{
	if (!Doorman)
	{
		return;
	}
	const double Now = RealNow();
	const FVector Here = Doorman->GetActorLocation();

	// His reaction plays at real speed under slow time, like the voices (then the world's speed again).
	if (ReactUntil > 0.0)
	{
		const AWorldSettings* const Settings = GetWorld() ? GetWorld()->GetWorldSettings() : nullptr;
		if (Now < ReactUntil && bReactAtRealSpeed && Settings)
		{
			Doorman->CustomTimeDilation = 1.f / FMath::Max(0.05f, Settings->GetEffectiveTimeDilation());
		}
		else
		{
			Doorman->CustomTimeDilation = 1.f;
			ReactUntil = -1.0;
		}
	}

	// Each guest at the scanner: he waves it on through the gate.
	for (int32 Index = 0; Index < Guests.Num(); ++Index)
	{
		const FQueueGuest& Guest = Guests[Index];
		if (Guest.Leg == 0 && Guest.ScanLeft > 0.f && WavedGuest != Index && Guest.Member.IsValid())
		{
			WavedGuest = Index;
			const FVector Gate = WayIn.Num() > 0 ? ToWorld(WayIn[0]) : ToWorld(ScanPoint) + FVector(200.f, 0.f, 0.f);
			Doorman->LookAtPoint(Guest.Member->GetActorLocation() + FVector(0.f, 0.f, 150.f), 1.5f);
			Doorman->PlayGestureAt(TEXT("Wave"), Gate);
		}
	}

	// The searchlight lands (the same frame as Maya's press): he turns to it, shields his eyes and says so, in shot.
	AFollowSpot* const Spot = Searchlight.Get();
	if (!Spot || !Spot->LandedThisFrame() || Beat == EQueueBeat::Lobby || Beat == EQueueBeat::Done)
	{
		return;
	}
	const FVector Lamp = Spot->GetActorLocation();
	const FVector End = Spot->GetBeamEnd();
	const AActor* const Hit = Spot->GetBeamHitActor();
	const AScannerWatcher* const Scanner = FindScanner(Director);
	const bool bDazzled = Hit == Doorman.Get() || (Scanner && Hit == Scanner) || FVector::Dist(End, Here) <= DazzleRadius;
	const FVector LookPoint = bDazzled ? Lamp : End;
	Doorman->FaceYaw(QueueLineLocal::YawTo(Here, LookPoint, DoormanYaw));
	Doorman->LookAtPoint(LookPoint, ReactSeconds + 1.f);
	Doorman->PlayGestureAt(bDazzled ? ShieldGesture : FName(TEXT("Point")), LookPoint);
	ReactUntil = Now + ReactSeconds;
	Director.Flash(LightLine, ETakeFlashKind::Line, 2.5f);
	// Maya's own line after his: on the first take her lesson line ("cut back with 1") follows instead.
	const UTakeGameInstance* const GI = Director.GetTakeGameInstance();
	const bool bBackToTeach = bTeach && GI && (GI->IntroShownMask & BitTaughtBack) == 0;
	if (!bBackToTeach && !MayaFireLine.IsEmpty())
	{
		Director.Flash(MayaFireLine, ETakeFlashKind::Line, 3.f);
	}
	TAKE_LOG(TEXT("QUEUE doorman: the light (%s, beam ends %.0f cm from him)"), bDazzled ? TEXT("dazzled") : TEXT("looks where it lands"),
		FVector::Dist(End, Here));
}

bool AQueueLine::ClaimsSpotLine(const AFollowSpot* Spot, const AActor* Target) const
{
	return !bOldFlow && Doorman && Spot && Spot == Searchlight.Get() && Beat != EQueueBeat::Lobby && Beat != EQueueBeat::Done;
}

void AQueueLine::FindClerk()
{
	ACastMember* Best = nullptr;
	double BestDistance = ClerkFindRadius;
	for (TActorIterator<ACastMember> It(GetWorld()); It; ++It)
	{
		ACastMember* const Member = *It;
		if (Member && Member != Doorman && !Member->GetAttachParentActor() && !Member->bSeated)
		{
			const double Distance = FVector::Dist2D(Member->GetActorLocation(), ClerkFrom);
			if (Distance <= BestDistance)
			{
				Best = Member;
				BestDistance = Distance;
			}
		}
	}
	Clerk = Best;
	if (Best && !ClerkPoint.IsNearlyZero())
	{
		const FVector Point(ClerkPoint.X, ClerkPoint.Y, Best->GetActorLocation().Z);
		Best->SetActorLocationAndRotation(Point, FRotator(0.f, ClerkYaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		Best->FaceYaw(ClerkYaw);
	}
}

AMark* AQueueLine::FindDeskMark(const ATakeDirector& Director) const
{
	for (const TObjectPtr<AMark>& EachMark : Director.Marks)
	{
		if (EachMark && EachMark->MarkName == DeskMarkName)
		{
			return EachMark.Get();
		}
	}
	return nullptr;
}

void AQueueLine::StepLobby(ATakeDirector& Director)
{
	if (Beat == EQueueBeat::Done)
	{
		return;
	}
	if (Director.Scene > 1)
	{
		// On into the hall without signing in: the opening is over, quietly.
		Beat = EQueueBeat::Done;
		TAKE_LOG(TEXT("QUEUE beat -> Done (the hall)"));
		return;
	}
	if (Beat == EQueueBeat::Through)
	{
		if (bLobbyGreet && Director.HasFired(FName(TakeTriggers::Door)))
		{
			EnterBeat(Director, EQueueBeat::Lobby);
		}
		return;
	}

	// The clerk keeps his eyes on Julian while he crosses the lobby.
	AJulian* const Julian = Director.Julian.Get();
	ACastMember* const Desk = Clerk.Get();
	const double Now = RealNow();
	if (Desk && Julian && Now >= ClerkFaceAt)
	{
		ClerkFaceAt = Now + 0.4;
		Desk->FaceYaw(QueueLineLocal::YawTo(Desk->GetActorLocation(), Julian->GetActorLocation(), ClerkYaw));
	}
	const AMark* const DeskMark = FindDeskMark(Director);
	if (Julian && DeskMark && FVector::Dist2D(Julian->GetFeetLocation(), DeskMark->GetActorLocation()) <= DeskRadius)
	{
		// Signed in: he takes the paddle, the clerk answers, the director's lines take it from here.
		Julian->PlayGesture(TEXT("Lift"));
		if (Desk)
		{
			Desk->PlayGestureAt(TEXT("Talk"), Julian->GetActorLocation() + FVector(0.f, 0.f, 60.f));
		}
		SayLine(Director, DeskLine);
		if (!PaddleNote.IsEmpty())
		{
			Director.Flash(PaddleNote, ETakeFlashKind::Julian, 3.f);
		}
		EnterBeat(Director, EQueueBeat::Done);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// The guests
// ---------------------------------------------------------------------------------------------------------------------

void AQueueLine::WalkGuest(FQueueGuest& Guest, const FVector& Point)
{
	ACastMember* const Member = Guest.Member.Get();
	if (!Member)
	{
		return;
	}
	Guest.LegTarget = Point;
	Guest.OrderAge = 0.f;
	Guest.OrderExpect = static_cast<float>(FVector::Dist2D(Member->GetActorLocation(), Point)) / FMath::Max(10.f, GuestWalkSpeed);
	Member->WalkTo(Point, GuestWalkSpeed);
}

void AQueueLine::PutGuest(FQueueGuest& Guest, const FVector& Point, float Yaw)
{
	ACastMember* const Member = Guest.Member.Get();
	if (!Member)
	{
		return;
	}
	Member->SetActorLocationAndRotation(Point, FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	Member->WalkTo(Point, GuestWalkSpeed);	// a walk it was on ends where it now stands
	Member->FaceYaw(Yaw);
	Guest.LegTarget = Point;
	Guest.OrderAge = 0.f;
	Guest.OrderExpect = 0.f;
	if (Guest.Blocker)
	{
		Guest.Blocker->SetWorldLocation(Point + FVector(0.f, 0.f, BlockHalfHeight));
	}
}

bool AQueueLine::GuestArrived(const FQueueGuest& Guest) const
{
	const ACastMember* const Member = Guest.Member.Get();
	if (!Member)
	{
		return true;
	}
	return FVector::Dist2D(Member->GetActorLocation(), Guest.LegTarget) <= GuestArriveRadius
		|| (!Member->IsWalking() && Guest.OrderAge > 0.25f);
}

bool AQueueLine::ScanPointClear() const
{
	for (const FQueueGuest& Guest : Guests)
	{
		if (Guest.Leg == 0 && Guest.Member.IsValid())
		{
			return false;
		}
	}
	return true;
}

void AQueueLine::SetBlocker(FQueueGuest& Guest, bool bOn)
{
	if (!Guest.Blocker)
	{
		return;
	}
	Guest.Blocker->SetCollisionEnabled(bOn ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	if (bOn)
	{
		if (const ACastMember* const Member = Guest.Member.Get())
		{
			Guest.Blocker->SetWorldLocation(Member->GetActorLocation() + FVector(0.f, 0.f, BlockHalfHeight));
		}
	}
}

FVector AQueueLine::NextInsideSpot(FQueueGuest& Guest)
{
	if (InsideSpots.Num() == 0)
	{
		return ToWorld(WayIn.Num() > 0 ? WayIn.Last() : ScanPoint);
	}
	const int32 Count = InsideSpots.Num();
	const int32 Index = InsideCount % Count;
	const int32 Round = InsideCount / Count;
	++InsideCount;
	Guest.InsideSpot = Index;
	return ToWorld(InsideSpots[Index]) + FVector(0.f, static_cast<float>(Round) * InsideSpread, 0.f);
}

void AQueueLine::StepGuests(ATakeDirector& Director, float DeltaSeconds)
{
	const float Dt = FMath::Max(0.f, DeltaSeconds);
	const FVector ScanWorld = ToWorld(ScanPoint);

	// Every guest: the order's clock, its blocker on it, and a walk that never arrives finished by hand (a cast
	// member that cannot walk still clears the line).
	for (FQueueGuest& Guest : Guests)
	{
		ACastMember* const Member = Guest.Member.Get();
		if (!Member)
		{
			continue;
		}
		Guest.OrderAge += Dt;
		if (Guest.Blocker && Guest.Blocker->IsCollisionEnabled())
		{
			Guest.Blocker->SetWorldLocation(Member->GetActorLocation() + FVector(0.f, 0.f, BlockHalfHeight));
		}
		if (!GuestArrived(Guest) && Guest.OrderAge > Guest.OrderExpect + 2.f)
		{
			PutGuest(Guest, Guest.LegTarget, static_cast<float>(Member->GetActorRotation().Yaw));
		}
	}

	// The guests going in: the scan point (a beat, a gesture), then the way in, then their spot inside.
	for (FQueueGuest& Guest : Guests)
	{
		ACastMember* const Member = Guest.Member.Get();
		if (!Member || Guest.Leg < 0 || Guest.bInside || !GuestArrived(Guest))
		{
			continue;
		}
		if (Guest.Leg == 0)
		{
			if (Guest.ScanLeft < 0.f)
			{
				// At the scanner: face the gate and show the invitation.
				const FVector Next = WayIn.Num() > 0 ? ToWorld(WayIn[0]) : ScanWorld + FVector(200.f, 0.f, 0.f);
				Member->FaceYaw(QueueLineLocal::YawTo(ScanWorld, Next, 0.f));
				Member->PlayGesture(ScanGesture);
				Guest.ScanLeft = ScanSeconds;
				continue;
			}
			Guest.ScanLeft -= Dt;
			if (Guest.ScanLeft > 0.f)
			{
				continue;
			}
			// Passed: through the gate. Julian may follow up to the scanner now.
			SetBlocker(Guest, false);
		}
		++Guest.Leg;
		if (Guest.Leg <= WayIn.Num())
		{
			WalkGuest(Guest, ToWorld(WayIn[Guest.Leg - 1]));
		}
		else if (Guest.Leg == WayIn.Num() + 1)
		{
			WalkGuest(Guest, NextInsideSpot(Guest));
		}
		else
		{
			Guest.bInside = true;
		}
	}

	// The front: the guest on place 0 steps into the scanner when the timer is up and the scanner is free.
	StepTimer -= Dt;
	if (StepTimer <= 0.f && Line.Num() > 0 && Line[0] != INDEX_NONE && ScanPointClear())
	{
		FQueueGuest& Front = Guests[Line[0]];
		if (Front.TargetPlace == 0 && GuestArrived(Front))
		{
			Line.RemoveAt(0);
			Front.TargetPlace = INDEX_NONE;
			Front.Leg = 0;
			Front.ScanLeft = -1.f;
			WalkGuest(Front, ScanWorld);
			StepTimer = StepSeconds;
			TAKE_LOG(TEXT("QUEUE steps up: Julian %d in line"), JulianIndex());
		}
	}

	// Everyone in the line walks up to their place once the one ahead has started to (Julian: once his feet are on
	// his place). Nobody passes Julian: the guests behind him wait for him.
	const FVector JulianFeet = Director.Julian ? Director.Julian->GetFeetLocation() : FVector::ZeroVector;
	for (int32 Index = 0; Index < Line.Num() && Index < Places.Num(); ++Index)
	{
		if (Line[Index] == INDEX_NONE)
		{
			continue;
		}
		FQueueGuest& Guest = Guests[Line[Index]];
		if (Guest.TargetPlace == Index)
		{
			continue;
		}
		bool bAheadMoved = true;
		if (Index > 0)
		{
			const int32 Ahead = Line[Index - 1];
			bAheadMoved = (Ahead == INDEX_NONE)
				? (Director.Julian && FVector::Dist2D(JulianFeet, ToWorld(Places[Index - 1])) <= PlaceRadius)
				: (Guests[Ahead].TargetPlace == Index - 1);
		}
		if (bAheadMoved)
		{
			Guest.TargetPlace = Index;
			WalkGuest(Guest, ToWorld(Places[Index]));
		}
	}

	// The guest behind Julian stops short of him (he may stand a little shy of his place): it never shoves him.
	const int32 JulianAt = JulianIndex();
	if (Director.Julian && JulianAt >= 0 && Line.IsValidIndex(JulianAt + 1) && Line[JulianAt + 1] != INDEX_NONE)
	{
		FQueueGuest& Behind = Guests[Line[JulianAt + 1]];
		ACastMember* const Member = Behind.Member.Get();
		if (Member && Member->IsWalking() && FVector::Dist2D(Member->GetActorLocation(), JulianFeet) < KeepClearOfJulian)
		{
			WalkGuest(Behind, Member->GetActorLocation());	// stops where it is
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// The beats
// ---------------------------------------------------------------------------------------------------------------------

void AQueueLine::StepLine(ATakeDirector& Director, float DeltaSeconds)
{
	if (!bJulianPlaced)
	{
		PlaceJulian(&Director);
	}
	if (!bSetUp)
	{
		SetupLine(Director);
	}
	if (bOldFlow)
	{
		return;
	}
	StepGuests(Director, DeltaSeconds);
	StepHold(Director);		// W3 opening
	StepDoorman(Director);	// W3 opening
	StepBeats(Director);
}

void AQueueLine::SayLine(ATakeDirector& Director, const FString& Text)
{
	if (Text.IsEmpty())
	{
		return;
	}
	LineSaidAt = RealNow();
	LineDoneSince = -1.0;
	bLineVoiceSeen = false;
	LineSeconds = LineBaseSeconds + LinePerCharSeconds * static_cast<float>(Text.Len());
	Director.Flash(Text, ETakeFlashKind::Line, FMath::Max(2.f, LineSeconds + 1.f));
	TAKE_LOG(TEXT("QUEUE says %s"), *Text);
}

bool AQueueLine::LineDone(const ATakeDirector& Director) const
{
	// The voice starts when the HUD shows the subtitle (VoiceSecondsLeft is 0 on the Flash frame): a voiced line is
	// done once its voice has risen and fallen back to 0; an unvoiced one after its reading time. Never hangs.
	const double Since = RealNow() - LineSaidAt;
	if (Since >= static_cast<double>(LineSeconds + LineMaxExtraSeconds))
	{
		return true;
	}
	if (Since < FMath::Max(LineSeconds, LineVoiceStartSeconds) || Director.VoiceSecondsLeft() > 0.f)
	{
		return false;
	}
	return bLineVoiceSeen || Since >= static_cast<double>(LineSeconds + LineUnvoicedPad);
}

void AQueueLine::SetSlow(ATakeDirector& Director, bool bSlow)
{
	if (bSlow == bSlowing)
	{
		return;
	}
	bSlowing = bSlow;
	if (bSlow)
	{
		SlowStartedAt = RealNow();
		Director.BeginSlow(QueueLineLocal::SlowReason(), TutorSlowScale);
	}
	else
	{
		Director.EndSlow(QueueLineLocal::SlowReason());
	}
	TAKE_LOG(TEXT("QUEUE slow %s (%s)"), bSlow ? TEXT("on") : TEXT("off"), QueueLineLocal::BeatName(Beat));
}

bool AQueueLine::TakeTeachBit(ATakeDirector& Director, uint8 Bit)
{
	if (!bTeach)
	{
		return false;
	}
	UTakeGameInstance* const GI = Director.GetTakeGameInstance();
	if (!GI || (GI->IntroShownMask & Bit) != 0)
	{
		return false;
	}
	GI->IntroShownMask |= Bit;
	return true;
}

void AQueueLine::EnterBeat(ATakeDirector& Director, EQueueBeat NewBeat)
{
	TAKE_LOG(TEXT("QUEUE beat %s -> %s"), QueueLineLocal::BeatName(Beat), QueueLineLocal::BeatName(NewBeat));
	Beat = NewBeat;
	BeatStartedAt = RealNow();
	LineDoneSince = -1.0;	// the slow's wait for a press counts from this beat's line

	ATakePlayerController* const PC = Director.PC;
	ATakeCharacter* const Maya = Director.GetCharacter(ETakeCamera::B);

	switch (NewBeat)
	{
	case EQueueBeat::Refuse:
		SayLine(Director, RefuseLine);
		if (Director.Julian)
		{
			Director.Julian->PlayGesture(RefuseGesture);
			if (Doorman)
			{
				// W3 opening: the man at the scanner turns to him and holds up his hand as he says it.
				const FVector Head = Director.Julian->GetActorLocation() + FVector(0.f, 0.f, 60.f);
				Doorman->FaceYaw(QueueLineLocal::YawTo(Doorman->GetActorLocation(), Head, DoormanYaw));
				Doorman->LookAtPoint(Head, 3.f);
				Doorman->PlayGestureAt(TEXT("Refuse"), Head);
			}
		}
		break;

	case EQueueBeat::Aside:
	{
		// The switch is taught only from Julian's own view (the glance starts from it).
		const bool bOnJulian = PC && !PC->IsInFlight() && PC->GetCurrentCharacter() == Director.Julian.Get();
		const bool bTeachCut = bOnJulian && Maya && TakeTeachBit(Director, BitTaughtCut);
		bTeachingNow = bTeachCut;
		SayLine(Director, AsideLine);
		if (bTeachCut)
		{
			// As he says her name: time slows and the camera turns up to her on the terrace.
			SetSlow(Director, true);
			PC->StartGlance(Maya->GetActorLocation() + FVector(0.f, 0.f, GlanceTargetHeight));
		}
		break;
	}

	case EQueueBeat::AskCut:
		SayLine(Director, CutLine);
		Director.Flash(TEXT("[2] Cut to MAYA"), ETakeFlashKind::Note, QueueLineLocal::PromptSeconds);
		break;

	case EQueueBeat::AskUse:
		bUseStarted = false;
		break;

	case EQueueBeat::AskBack:
	{
		bBackSaid = false;
		const bool bTeachBack = TakeTeachBit(Director, BitTaughtBack);
		bTeachingNow = bTeachBack;
		bBackTaught = bTeachBack;
		SetSlow(Director, bTeachBack);	// not taught: the world runs again from the fire
		break;
	}

	case EQueueBeat::Through:
		SetSlow(Director, false);
		bTeachingNow = false;
		if (PC)
		{
			PC->EndGlance();
		}
		break;

	case EQueueBeat::Lobby:
	{
		// W3 opening: the clerk greets him: turns to him, raises a hand, speaks (named in the subtitle); the camera
		// eases toward him once a session (it steers the walk to the desk; the mouse takes it back).
		ACastMember* const Desk = Clerk.Get();
		AJulian* const Julian = Director.Julian.Get();
		if (Desk && Julian)
		{
			const FVector Head = Julian->GetActorLocation() + FVector(0.f, 0.f, 60.f);
			Desk->FaceYaw(QueueLineLocal::YawTo(Desk->GetActorLocation(), Head, ClerkYaw));
			Desk->LookAtPoint(Head, 4.f);
			Desk->PlayGestureAt(TEXT("Hello"), Head);
			UTakeGameInstance* const GI = Director.GetTakeGameInstance();
			const bool bOnJulian = PC && !PC->IsInFlight() && PC->GetCurrentCharacter() == Julian;
			if (bOnJulian && GI && (GI->IntroShownMask & BitLobbyPan) == 0)
			{
				GI->IntroShownMask |= BitLobbyPan;
				PC->StartLookPan(Desk->GetActorLocation() + FVector(0.f, 0.f, ClerkHeadHeight), LobbyPanSeconds, true);
			}
		}
		SayLine(Director, GreetLine);
		break;
	}

	default:
		break;
	}
}

void AQueueLine::StepBeats(ATakeDirector& Director)
{
	if (Beat == EQueueBeat::Through || Beat == EQueueBeat::Lobby || Beat == EQueueBeat::Done)
	{
		StepLobby(Director);	// W3 opening
		return;
	}

	ATakePlayerController* const PC = Director.PC;
	AJulian* const Julian = Director.Julian.Get();
	ATakeCharacter* const Maya = Director.GetCharacter(ETakeCamera::B);
	AScannerWatcher* const Scanner = FindScanner(Director);
	const double Now = RealNow();

	// Through the gate, or out of Scene 1 by any way: the director's own lines from here.
	if (Director.HasFired(FName(TakeTriggers::Door)) || Director.Scene > 1 || (Scanner && Julian && Scanner->HasPassed(Julian)))
	{
		EnterBeat(Director, EQueueBeat::Through);
		return;
	}

	// The line's voice has started (it starts when the HUD shows the subtitle, a frame or more after the Flash).
	if (Director.VoiceSecondsLeft() > 0.f && Now - LineSaidAt >= 0.0)
	{
		bLineVoiceSeen = true;
	}
	// The line said: when it was first done (the slow's wait for the press counts from there).
	if (LineDoneSince < 0.0 && LineDone(Director))
	{
		LineDoneSince = Now;
	}

	// The slow never outstays: TutorHoldAfterLineSeconds of waiting for the press, TutorMaxSeconds in all. The beats
	// go on unslowed.
	if (bSlowing)
	{
		const bool bTooLong = TutorMaxSeconds > 0.f && Now - SlowStartedAt > TutorMaxSeconds;
		const bool bWaitedTooLong = LineDoneSince >= 0.0 && Now - LineDoneSince > TutorHoldAfterLineSeconds;
		if (bTooLong || bWaitedTooLong)
		{
			TAKE_LOG(TEXT("QUEUE slow gives up (%s) after %.1f real s, %.1f real s after the line"), bTooLong ? TEXT("too long in all") : TEXT("no press"),
				Now - SlowStartedAt, LineDoneSince >= 0.0 ? Now - LineDoneSince : 0.0);
			SetSlow(Director, false);
		}
	}

	const ATakeCharacter* const On = PC ? PC->GetCurrentCharacter() : nullptr;
	const ATakeCharacter* const Heading = PC ? PC->GetFlightTarget() : nullptr;
	const bool bInFlight = PC && PC->IsInFlight();

	switch (Beat)
	{
	case EQueueBeat::Queue:
	{
		// His turn: nobody ahead of him, the scanner free, his feet on the front place.
		if (JulianIndex() == 0 && ScanPointClear() && Julian && Places.Num() > 0
			&& FVector::Dist2D(Julian->GetFeetLocation(), ToWorld(Places[0])) <= PlaceRadius)
		{
			if (UTakeGameInstance* const GI = Director.GetTakeGameInstance())
			{
				GI->IntroShownMask |= BitQueueWalked;
			}
			// Blinded already (Maya went early): nothing to refuse him.
			EnterBeat(Director, (Scanner && !Scanner->IsLive()) ? EQueueBeat::Through : EQueueBeat::Refuse);
		}
		break;
	}

	case EQueueBeat::Refuse:
		if (LineDone(Director))
		{
			EnterBeat(Director, EQueueBeat::Aside);
		}
		break;

	case EQueueBeat::Aside:
		if (LineDone(Director))
		{
			// The cut is taught unless the player has already made it (2 during the aside): then straight to Maya's beat.
			const UTakeGameInstance* const GI = Director.GetTakeGameInstance();
			const bool bUseToTeach = bTeach && GI && (GI->IntroShownMask & BitTaughtUse) == 0;
			const bool bToMaya = Maya && (Heading == Maya || (!bInFlight && On == Maya));
			EnterBeat(Director, (bTeachingNow && !bToMaya) ? EQueueBeat::AskCut
				: ((bTeachingNow || bUseToTeach) ? EQueueBeat::AskUse : EQueueBeat::Through));
		}
		break;

	case EQueueBeat::AskCut:
		// Her line said, and the view on its way to her (or on her): the next beat waits for the landing.
		if (LineDone(Director) && Maya && (Heading == Maya || (!bInFlight && On == Maya)))
		{
			EnterBeat(Director, EQueueBeat::AskUse);
		}
		break;

	case EQueueBeat::AskUse:
	{
		if (!bUseStarted && PC && !bInFlight && On == Maya && Maya)
		{
			// Landed on Maya: her look eases onto the scanner and she says what to do.
			bUseStarted = true;
			const bool bTeachUse = TakeTeachBit(Director, BitTaughtUse);
			bTeachingNow = bTeachUse;
			SetSlow(Director, bTeachUse);
			if (bTeachUse)
			{
				if (Scanner)
				{
					PC->StartLookPan(Scanner->GetAimPoint(), LookPanSeconds);
				}
				SayLine(Director, UseLine);
			}
		}
		const bool bBlind = Scanner && !Scanner->IsLive();
		if (bBlind && (!bUseStarted || LineDone(Director)))
		{
			EnterBeat(Director, EQueueBeat::AskBack);
		}
		else if (bUseStarted && !bBlind && !bInFlight && On && On != Maya)
		{
			// Left Maya without the light: the director's own lines take it from here.
			EnterBeat(Director, EQueueBeat::Through);
		}
		break;
	}

	case EQueueBeat::AskBack:
	{
		if (!bBackTaught)
		{
			EnterBeat(Director, EQueueBeat::Through);
			break;
		}
		const bool bBackOnJulian = Julian && (Heading == Julian || (!bInFlight && On == Julian));
		if (!bBackSaid && Now - BeatStartedAt >= FireLineSeconds && Director.VoiceSecondsLeft() <= 0.f)
		{
			// After Maya's own searchlight line: the cut back, unless the player has already made it.
			bBackSaid = true;
			if (!bBackOnJulian)
			{
				SayLine(Director, BackLine);
				Director.Flash(TEXT("[1] Cut back to JULIAN"), ETakeFlashKind::Note, QueueLineLocal::PromptSeconds);
			}
		}
		// Landed back on Julian with the line said: the slow eases out and he walks through. The blind running out
		// (the scanner live again) ends it too.
		if ((bBackSaid && LineDone(Director) && !bInFlight && On == Julian) || (Scanner && Scanner->IsLive()))
		{
			EnterBeat(Director, EQueueBeat::Through);
		}
		break;
	}

	default:
		break;
	}
}

FString AQueueLine::GetObjective(const ATakeDirector& Director) const
{
	if (bOldFlow || !bSetUp || !Director.PC)
	{
		return FString();
	}
	const ATakePlayerController* const PC = Director.PC.Get();
	const ATakeCharacter* const On = PC->GetCurrentCharacter();
	const bool bOnJulian = On && On == Director.Julian.Get();
	const bool bOnMaya = On && On->Id == ETakeCamera::B;

	switch (Beat)
	{
	case EQueueBeat::Queue:
	{
		const int32 JulianAt = JulianIndex();
		if (!bOnJulian || JulianAt < 0 || !Director.Julian || Places.Num() == 0)
		{
			return FString();
		}
		const FVector Place = ToWorld(Places[FMath::Min(JulianAt, Places.Num() - 1)]);
		const bool bOnPlace = FVector::Dist2D(Director.Julian->GetFeetLocation(), Place) <= PlaceRadius;
		if (JulianAt == 0)
		{
			return bOnPlace ? FString(TEXT("JULIAN's turn. Wait for the scanner"))
				: FString(TEXT("JULIAN's turn: step up to the SCANNER"));
		}
		return bOnPlace
			? FString::Printf(TEXT("No invitation, so JULIAN queues. %d ahead of him: wait your turn"), JulianAt)
			: FString(TEXT("The line moved: walk JULIAN up to keep his place"));
	}
	case EQueueBeat::Refuse:
	case EQueueBeat::Aside:
		return bOnJulian ? FString(TEXT("No match: JULIAN is not on the list. He needs a distraction")) : FString();
	case EQueueBeat::AskCut:
		return bOnJulian ? FString(TEXT("[2] Cut to MAYA on the terrace: her SEARCHLIGHT can blind the scanner")) : FString();
	case EQueueBeat::AskUse:
		if (bOnMaya && !PC->IsInFlight() && Director.CurrentUse.Device)
		{
			return FString(TEXT("Look at the SCANNER and press [E]: the SEARCHLIGHT blinds it"));
		}
		return FString();
	case EQueueBeat::AskBack:
		return bOnMaya ? FString(TEXT("[1] Cut back to JULIAN and walk him through the open gate")) : FString();
	case EQueueBeat::Lobby:
		return (bOnJulian && !PC->IsInFlight()) ? DeskObjective : FString();	// W3 opening
	default:
		return FString();
	}
}
