// Scene 4: the catwalks (Nora), then the tracking shot of Vance. Owner: s4-catwalk. See Scene4Catwalk.h.

#include "Script/Scene4Catwalk.h"

#include "Script/ScriptSequencer.h"
#include "FollowSpot.h"
#include "CartDevice.h"
#include "TakeCharacter.h"
#include "Watcher.h"
#include "TakeAudio.h"
#include "TakeDirector.h"
#include "TakeShapes.h"
#include "TakeTypes.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"

namespace
{
	const FName FailSpotted(TEXT("SPOTTED AT THE ROPE"));
	const FName FailRanks(TEXT("THE GUARDS CLOSED RANKS"));

	const FLinearColor ConeRed(0.95f, 0.08f, 0.05f);
	const FLinearColor ConeIdle(0.85f, 0.85f, 0.8f);
	const FLinearColor ConeOff(0.35f, 0.35f, 0.38f);
	const FLinearColor ConeAlibi(0.2f, 0.85f, 0.35f);
	const FLinearColor LaneGold(1.0f, 0.72f, 0.15f);
	const FLinearColor Champagne(0.92f, 0.82f, 0.5f);

	float YawTo(const FVector& From, const FVector& To)
	{
		const FVector D = (To - From).GetSafeNormal2D();
		return D.IsNearlyZero() ? 0.f : static_cast<float>(D.Rotation().Yaw);
	}

	UFont* HudFont(bool bLarge)
	{
		if (!GEngine)
		{
			return nullptr;
		}
		return bLarge ? GEngine->GetLargeFont() : GEngine->GetMediumFont();
	}

	void HudText(UCanvas* C, const FString& S, float X, float Y, const FLinearColor& Col, bool bLarge, bool bCentre = false)
	{
		UFont* F = HudFont(bLarge);
		if (!F)
		{
			return;
		}
		FCanvasTextItem T(FVector2D(X, Y), FText::FromString(S), F, Col);
		T.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f));
		T.bCentreX = bCentre;
		const float Scale = FMath::Max(C->ClipY / 1080.f, 0.5f) * (bLarge ? 1.0f : 1.15f);
		T.Scale = FVector2D(Scale, Scale);
		C->DrawItem(T);
	}

	void HudRect(UCanvas* C, float X, float Y, float W, float H, const FLinearColor& Col)
	{
		FCanvasTileItem T(FVector2D(X, Y), FVector2D(W, H), Col);
		T.BlendMode = SE_BLEND_Translucent;
		C->DrawItem(T);
	}

	void HudLine(UCanvas* C, const FVector2D& A, const FVector2D& B, const FLinearColor& Col, float Thick)
	{
		FCanvasLineItem L(A, B);
		L.SetColor(Col);
		L.LineThickness = Thick;
		C->DrawItem(L);
	}
}

AScene4Catwalk::AScene4Catwalk()
{
	SceneNumber = 4;
	Pov = EScriptPov::Nora;
	Slug = FText::FromString(TEXT("INT. THE CATWALKS / LIGHTING RIG - CONTINUOUS"));
	bPlayerWalks = false;

	// A late tick: the director steps every watcher on the bar clock; while Scene 4 runs, its guards stand where the
	// scene says (after the director has moved them this frame).
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// The threshold at the rope gap: a crimson valance and two curtain panels drawn to the sides, and behind them two
	// walnut door leaves on hinges (open into the auditorium; they swing shut behind Vance).
	CurtainL = FTakeShapes::CreateShape(this, TEXT("CurtainL"), ETakeShape::Cube, Root);
	CurtainR = FTakeShapes::CreateShape(this, TEXT("CurtainR"), ETakeShape::Cube, Root);
	ArchTop = FTakeShapes::CreateShape(this, TEXT("ArchTop"), ETakeShape::Cube, Root);
	HingeL = CreateDefaultSubobject<USceneComponent>(TEXT("HingeL"));
	HingeL->SetupAttachment(Root);
	HingeR = CreateDefaultSubobject<USceneComponent>(TEXT("HingeR"));
	HingeR->SetupAttachment(Root);
	LeafL = FTakeShapes::CreateShape(this, TEXT("LeafL"), ETakeShape::Cube, HingeL);
	LeafR = FTakeShapes::CreateShape(this, TEXT("LeafR"), ETakeShape::Cube, HingeR);

	// Four guards by default (scene4.py sets the actors and posts).
	Guards.SetNum(4);
	Guards[0].Role = EScene4Role::EastDoor;	Guards[0].Label = TEXT("EAST DOOR");
	Guards[1].Role = EScene4Role::WestPillar;	Guards[1].Label = TEXT("WEST PILLAR");
	Guards[2].Role = EScene4Role::Guard1;		Guards[2].Label = TEXT("GUARD 1");
	Guards[3].Role = EScene4Role::Guard2;		Guards[3].Label = TEXT("GUARD 2");
}

void AScene4Catwalk::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const float H = DoorHeight;
	const float PanelW = 90.f;
	// Curtain panels: drawn to the gap's sides, overlapping its edges a little.
	CurtainL->SetRelativeLocation(FVector(0.f, -(GapHalf + PanelW * 0.5f - 35.f), (H + 60.f) * 0.5f));
	CurtainL->SetRelativeScale3D(FVector(0.14f, PanelW / 100.f, (H + 60.f) / 100.f));
	CurtainR->SetRelativeLocation(FVector(0.f, (GapHalf + PanelW * 0.5f - 35.f), (H + 60.f) * 0.5f));
	CurtainR->SetRelativeScale3D(FVector(0.14f, PanelW / 100.f, (H + 60.f) / 100.f));
	ArchTop->SetRelativeLocation(FVector(0.f, 0.f, H + 60.f + 35.f));
	ArchTop->SetRelativeScale3D(FVector(0.2f, (GapHalf * 2.f + PanelW * 2.f) / 100.f, 0.7f));

	HingeL->SetRelativeLocation(FVector(DoorsBehind, -GapHalf, 0.f));
	HingeR->SetRelativeLocation(FVector(DoorsBehind, GapHalf, 0.f));
	LeafL->SetRelativeLocation(FVector(0.f, GapHalf * 0.5f, H * 0.5f));
	LeafL->SetRelativeScale3D(FVector(0.08f, GapHalf / 100.f, H / 100.f));
	LeafR->SetRelativeLocation(FVector(0.f, -GapHalf * 0.5f, H * 0.5f));
	LeafR->SetRelativeScale3D(FVector(0.08f, GapHalf / 100.f, H / 100.f));

	for (UStaticMeshComponent* C : { CurtainL.Get(), CurtainR.Get(), ArchTop.Get() })
	{
		FTakeShapes::SetColour(C, CurtainColour);
		C->SetVisibility(bShowThreshold);
	}
	for (UStaticMeshComponent* C : { LeafL.Get(), LeafR.Get() })
	{
		FTakeShapes::SetColour(C, DoorColour);
		C->SetVisibility(bShowThreshold);
	}
	SetDoors(1.f);
}

void AScene4Catwalk::BeginPlay()
{
	Super::BeginPlay();
	for (FScene4Guard& G : Guards)
	{
		if (G.Actor)
		{
			G.Home = G.Actor->GetActorTransform();
			if (G.Post.IsNearlyZero())
			{
				G.Post = G.Home.GetLocation();
			}
		}
	}
	FloorZ = Cart ? static_cast<float>(Cart->GetActorLocation().Z) : static_cast<float>(GetActorLocation().Z);
	BuildTower();
	if (BirdsEyeCamera)
	{
		SceneCamera = BirdsEyeCamera;
		BirdHome = BirdsEyeCamera->GetActorTransform();
		bBirdHome = true;
	}
	SetDoors(1.f);
}

void AScene4Catwalk::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsActive())
	{
		return;
	}
	// After the director's watcher step this frame: the guards stand where the scene put them.
	for (FScene4Guard& G : Guards)
	{
		MoveGuardActor(G);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// The hooks
// ---------------------------------------------------------------------------------------------------------------------

void AScene4Catwalk::ResetScene()
{
	Phase = EPhase::Plan;
	PhaseSeconds = 0.f;
	bSpotUsed = false;
	SpotLeft = 0.f;
	bCartUsed = false;
	bCrashed = false;
	CartT = 0.f;
	bClosedRanks = false;
	bGuard2Sent = false;
	FailReason = NAME_None;
	FailIn = -1.f;
	SpottedBy = -1;
	LaneIndex = 0;
	bCamPlaced = false;
	FlashLeft = 0.f;

	for (FScene4Guard& G : Guards)
	{
		G.bDistracted = false;
		G.bSeesLane = false;
		G.bGoing = false;
		G.State = TEXT("PATROL");
		G.Loc = G.Post;
		G.FaceYaw = G.Yaw;
		if (G.Actor)
		{
			G.Actor->SetActorTransform(G.Home);
		}
	}
	if (Spot)
	{
		Spot->ResetSpot();
	}
	if (Cart)
	{
		Cart->ResetCart();
	}
	for (FGlass& Gl : Glasses)
	{
		Gl.bFlying = false;
		Gl.bDown = false;
		Gl.Vel = FVector::ZeroVector;
		if (Gl.Comp)
		{
			Gl.Comp->SetRelativeScale3D(GlassScale);
			Gl.Comp->SetWorldRotation(FRotator::ZeroRotator);
		}
	}
	PlaceTowerOnCart();
	SetDoors(1.f);

	if (ATakeCharacter* V = GetVance())
	{
		V->GetCharacterMovement()->MaxWalkSpeed = V->WalkSpeed;
		V->GetCharacterMovement()->SetComponentTickEnabled(true);
	}
}

void AScene4Catwalk::BeginScene()
{
	Super::BeginScene();
	ResetScene();
	bAlibi = HasFlag(FName(TEXT("VipAlibi")));
	if (Spot)
	{
		Spot->Duration = SpotSeconds;
	}
	if (Cart)
	{
		Cart->RollSeconds = CartRollSeconds;
	}
	HoldGuards(true);
	StepGuards(0.f);

	if (GetRetakes() > 0)
	{
		Say(TEXT("NORA"), TEXT("Hold, hold... they saw you. Again."), NoraRetakeVoice);
	}
	else
	{
		Say(TEXT("NORA"), TEXT("I'm in the rafters. Looking down at the whole chessboard. Vance, hold by the bar. Let me clear your lane."), NoraOpenVoice);
	}
	TAKE_LOG(TEXT("S4 BEGIN alibi=%d guards=%d"), bAlibi ? 1 : 0, Guards.Num());
}

void AScene4Catwalk::EndScene()
{
	if (ATakeCharacter* V = GetVance())
	{
		V->GetCharacterMovement()->MaxWalkSpeed = V->WalkSpeed;
		V->GetCharacterMovement()->SetComponentTickEnabled(true);
	}
	// The guards go back to their own posts (the other scenes own them), and to the director.
	HoldGuards(false);
	for (FScene4Guard& G : Guards)
	{
		if (G.Actor)
		{
			G.Actor->SetActorTransform(G.Home);
		}
	}
	if (Spot)
	{
		Spot->ResetSpot();
	}
	Super::EndScene();
}

void AScene4Catwalk::TickScene(float DeltaSeconds)
{
	const float Dt = DeltaSeconds;
	PhaseSeconds += Dt;
	FlashLeft = FMath::Max(FlashLeft - Dt, 0.f);

	// The spot burns down only while Nora plans; once Vance is cued the guards stay distracted for the shot.
	if (Phase == EPhase::Plan || Phase == EPhase::Fail)
	{
		SpotLeft = FMath::Max(SpotLeft - Dt, 0.f);
	}
	if (Spot)
	{
		if (!ATakeDirector::Get(this))
		{
			Spot->StepDevice(Dt);	// the director steps every device when it runs
		}
	}

	// The cart: roll (the device moves it), then the crash.
	if (Cart)
	{
		if (!ATakeDirector::Get(this))
		{
			Cart->StepDevice(Dt);
		}
		if (bCartUsed && !bCrashed)
		{
			CartT += Dt;
			PlaceTowerOnCart();
			if (CartT >= CartRollSeconds)
			{
				bCrashed = true;
				ShatterTower();
				const bool bSpotBurning = SpotLeft > 0.f;
				if (!bSpotBurning && !bAlibi)
				{
					bClosedRanks = true;
					Say(TEXT("GUARD 1"), TEXT("Hold your posts! Nobody leaves the rope."), Guard1HoldVoice);
					ScheduleFail(FailRanks, CloseRanksSeconds);
				}
				else if (bSpotBurning)
				{
					bGuard2Sent = !bAlibi;
					Say(TEXT("GUARD 1"), TEXT("What the hell, fix that spot! And go help the staff clean that mess before Pendleton sees it!"), Guard1SpotVoice);
				}
				TAKE_LOG(TEXT("S4 CRASH spot=%d ranks=%d"), bSpotBurning ? 1 : 0, bClosedRanks ? 1 : 0);
			}
		}
	}
	StepGlasses(Dt);
	StepGuards(Dt);

	// Nora's feed: a slow sway, never still (the vertigo); a jolt on each press.
	if (BirdsEyeCamera && bBirdHome && (Phase == EPhase::Plan || Phase == EPhase::Fail))
	{
		const float T = GetSceneSeconds() * 2.f * PI * SwayHz;
		const float Jolt = FlashLeft / 0.12f;
		const FVector Off(FMath::Sin(T * 1.3f) * SwayCm, FMath::Sin(T) * SwayCm, FMath::Sin(T * 0.7f) * SwayCm * 0.5f - Jolt * 6.f);
		const FRotator Rock(FMath::Sin(T * 0.9f) * SwayRollDegrees * 0.5f - Jolt * 0.8f, 0.f, FMath::Sin(T) * SwayRollDegrees);
		BirdsEyeCamera->SetActorLocationAndRotation(BirdHome.GetLocation() + Off, (BirdHome.GetRotation() * FQuat(Rock)).Rotator());
	}

	if (FailIn >= 0.f)
	{
		FailIn -= Dt;
		if (FailIn < 0.f)
		{
			Phase = EPhase::Done;
			FailScene(FailReason);
			return;
		}
	}

	if (Phase == EPhase::Track)
	{
		StepTrack(Dt);
	}
	else if (Phase == EPhase::Doors)
	{
		StepTrack(Dt);	// the camera settles; Vance walks on into the dark
		StepDoors(Dt);
	}
}

bool AScene4Catwalk::HandleSceneKey(const FKey& Key, bool bPressed)
{
	const bool bOne = Key == EKeys::One || Key == EKeys::NumPadOne;
	const bool bTwo = Key == EKeys::Two || Key == EKeys::NumPadTwo;
	const bool bCue = Key == EKeys::SpaceBar || Key == EKeys::Enter;
	if (!bOne && !bTwo && !bCue)
	{
		return false;
	}
	if (!bPressed)
	{
		return true;
	}
	if (Phase != EPhase::Plan)
	{
		return true;
	}
	if (bOne)
	{
		PressSpot();
	}
	else if (bTwo)
	{
		PressCart();
	}
	else
	{
		PressCue();
	}
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
// The presses
// ---------------------------------------------------------------------------------------------------------------------

void AScene4Catwalk::PressSpot()
{
	if (bSpotUsed)
	{
		return;
	}
	bSpotUsed = true;
	SpotLeft = SpotSeconds;
	FlashLeft = 0.12f;
	const int32 E = FindGuard(EScene4Role::EastDoor);
	if (Spot && E != INDEX_NONE)
	{
		if (AWatcher* W = Cast<AWatcher>(Guards[E].Actor))
		{
			FString Why;
			Spot->Duration = SpotSeconds;
			if (!Spot->FireAt(W, Why))
			{
				TAKE_LOG(TEXT("S4 SPOT did not fire: %s"), *Why);
			}
		}
	}
	if (UTakeAudioComponent* A = UTakeAudioComponent::Get(this))
	{
		A->PlayCue(TEXT("sfx_spot_on"));
	}
	// The crash already happened (only with the alibi: the guards held): Guard 1 now yells at both.
	if (bCrashed && !bClosedRanks)
	{
		Say(TEXT("GUARD 1"), TEXT("What the hell, fix that spot! And go help the staff clean that mess before Pendleton sees it!"), Guard1SpotVoice);
	}
	StepGuards(0.f);	// the cones change this frame
	TAKE_LOG(TEXT("S4 SPOT"));
}

void AScene4Catwalk::PressCart()
{
	if (bCartUsed)
	{
		return;
	}
	bCartUsed = true;
	CartT = 0.f;
	FlashLeft = 0.12f;
	if (Cart)
	{
		Cart->RollSeconds = CartRollSeconds;
		Cart->Fire();	// moves this frame
	}
	PlaceTowerOnCart();
	TAKE_LOG(TEXT("S4 CART"));
}

void AScene4Catwalk::PressCue()
{
	StepGuards(0.f);
	SpottedBy = -1;
	for (int32 i = 0; i < Guards.Num(); ++i)
	{
		if (Guards[i].bSeesLane)
		{
			SpottedBy = i;
			break;
		}
	}
	FlashLeft = 0.12f;
	if (SpottedBy != INDEX_NONE)
	{
		Phase = EPhase::Fail;
		Say(TEXT("GUARD"), TEXT("Hey! You, in the jacket. Stop right there."), GuardSpottedVoice);
		ScheduleFail(FailSpotted, SpottedSeconds);
		TAKE_LOG(TEXT("S4 CUE spotted by %s"), *Guards[SpottedBy].Label);
		return;
	}

	// The lane is clear: cut to Vance, one unbroken tracking shot.
	if (bAlibi)
	{
		ConsumeFlag(FName(TEXT("VipAlibi")));
	}
	Phase = EPhase::Track;
	PhaseSeconds = 0.f;
	LaneIndex = 0;
	if (Spot)
	{
		Spot->HoldBeam(TrackMaxSeconds + 10.f);
	}
	bCamPlaced = false;
	Say(TEXT("NORA"), TEXT("Lane's clear. Go, Vance. Now."), NoraGoVoice);
	if (ATakeCharacter* V = GetVance())
	{
		if (Lane.Num() > 0)
		{
			const FVector Start = Lane[0];
			V->SetActorLocation(FVector(Start.X, Start.Y, Start.Z + V->GetSimpleCollisionHalfHeight() + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
			if (Lane.Num() > 1)
			{
				V->SetActorRotation(FRotator(0.f, YawTo(Lane[0], Lane[1]), 0.f));
			}
			LaneIndex = Lane.Num() > 1 ? 1 : 0;
		}
		V->GetCharacterMovement()->MaxWalkSpeed = StrollSpeed;
		V->GetCharacterMovement()->bRunPhysicsWithNoController = true;
	}
	StepTrack(0.f);
	CutTo(EScriptPov::Vance, TrackCamera);
	TAKE_LOG(TEXT("S4 CUE clear: tracking shot"));
}

void AScene4Catwalk::HoldGuards(bool bHold)
{
	for (FScene4Guard& G : Guards)
	{
		if (AWatcher* W = Cast<AWatcher>(G.Actor))
		{
			W->bScriptHeld = bHold;
		}
	}
}

void AScene4Catwalk::ScheduleFail(FName Reason, float After)
{
	if (FailIn >= 0.f)
	{
		return;
	}
	Phase = EPhase::Fail;
	FailReason = Reason;
	FailIn = FMath::Max(After, 0.f);
}

// ---------------------------------------------------------------------------------------------------------------------
// The guards
// ---------------------------------------------------------------------------------------------------------------------

int32 AScene4Catwalk::FindGuard(EScene4Role InRole) const
{
	for (int32 i = 0; i < Guards.Num(); ++i)
	{
		if (Guards[i].Role == InRole)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

FVector AScene4Catwalk::GuardForward(const FScene4Guard& G) const
{
	return FRotator(0.f, G.FaceYaw, 0.f).Vector();
}

bool AScene4Catwalk::ConeCoversLane(const FScene4Guard& G) const
{
	if (Lane.Num() < 2)
	{
		return false;
	}
	const FVector Fwd = GuardForward(G);
	const float CosHalf = FMath::Cos(FMath::DegreesToRadians(G.ConeHalfAngle));
	const FVector Gap = GetActorLocation();
	const FVector GapFwd = GetActorForwardVector();
	for (int32 i = 0; i + 1 < Lane.Num(); ++i)
	{
		const FVector A = Lane[i];
		const FVector B = Lane[i + 1];
		const int32 Steps = FMath::Max(1, FMath::CeilToInt(FVector::Dist2D(A, B) / 50.f));
		for (int32 s = 0; s <= Steps; ++s)
		{
			const FVector P = FMath::Lerp(A, B, static_cast<float>(s) / Steps);
			// Only the hall side of the threshold counts (inside the auditorium nobody at the rope sees him).
			if (FVector::DotProduct(P - Gap, GapFwd) > 0.0)
			{
				continue;
			}
			FVector D = P - G.Loc;
			D.Z = 0.0;
			const double Dist = D.Size();
			if (Dist > G.ConeRange || Dist < 1.0)
			{
				continue;
			}
			if (FVector::DotProduct(D / Dist, Fwd) >= CosHalf)
			{
				return true;
			}
		}
	}
	return false;
}

void AScene4Catwalk::StepGuards(float Dt)
{
	const float T = GetSceneSeconds();
	const bool bSpot = SpotLeft > 0.f || ((Phase == EPhase::Track || Phase == EPhase::Doors) && bSpotUsed);
	const FVector SpotAt = Spot ? Spot->GetActorLocation() : GetActorLocation() + FVector(-1500.f, 0.f, 900.f);
	const FVector Mess = Cart ? Cart->GetEffectPoint() : GetActorLocation();

	for (int32 i = 0; i < Guards.Num(); ++i)
	{
		FScene4Guard& G = Guards[i];
		const float W = 2.f * PI * (T + G.Phase) / FMath::Max(G.Period, 0.5f);
		const FVector PatrolLoc = G.Post + G.Pace * (0.5f - 0.5f * FMath::Cos(W));
		const float PatrolYaw = G.Yaw + G.Sweep * FMath::Sin(W);

		bool bLured = false;
		G.bDistracted = false;
		switch (G.Role)
		{
		case EScene4Role::EastDoor:
			if (bSpot)
			{
				G.bDistracted = true;
				G.State = TEXT("BLINDED");
				G.Loc = FMath::VInterpTo(G.Loc, G.Post, Dt, 4.f);
				G.FaceYaw = YawTo(SpotAt, G.Loc);	// turned away from the glare
			}
			break;
		case EScene4Role::Guard1:
			if (bSpot)
			{
				G.bDistracted = true;
				G.State = TEXT("YELLING AT THE SPOT");
				G.Loc = FMath::VInterpTo(G.Loc, G.Post, Dt, 4.f);
				G.FaceYaw = YawTo(G.Loc, SpotAt);
			}
			break;
		case EScene4Role::WestPillar:
			if (bCrashed)
			{
				bLured = true;
				G.State = TEXT("AT THE MESS");
			}
			break;
		case EScene4Role::Guard2:
			if (bAlibi)
			{
				G.bDistracted = true;
				G.State = TEXT("ALIBI - YOU'RE ON HIS LIST");
			}
			else if (bGuard2Sent)
			{
				bLured = true;
				G.State = TEXT("SENT TO CLEAN UP");
			}
			break;
		}

		if (bLured)
		{
			G.bDistracted = true;
			// Stand round the mess, not on it: each guard on his own side.
			const FVector Side = (G.Post - Mess).GetSafeNormal2D();
			const FVector Goal = Mess + Side * 160.f;
			const FVector To = Goal - G.Loc;
			const float Step = LuredWalkSpeed * Dt;
			if (To.Size2D() > Step && Step > 0.f)
			{
				G.Loc += To.GetSafeNormal2D() * Step;
				G.FaceYaw = YawTo(G.Loc, Goal);
			}
			else
			{
				if (Step > 0.f)
				{
					G.Loc = FVector(Goal.X, Goal.Y, G.Loc.Z);
				}
				G.FaceYaw = YawTo(G.Loc, Mess);
			}
		}
		else if (!G.bDistracted)
		{
			if (bClosedRanks)
			{
				G.State = TEXT("CLOSED RANKS");
				G.Loc = FMath::VInterpTo(G.Loc, G.Post, Dt, 4.f);
				G.FaceYaw = G.Yaw;
			}
			else
			{
				G.State = TEXT("PATROL");
				G.Loc = Dt > 0.f ? FMath::VInterpTo(G.Loc, PatrolLoc, Dt, 6.f) : PatrolLoc;
				G.FaceYaw = PatrolYaw;
			}
		}

		G.bSeesLane = !G.bDistracted && ConeCoversLane(G);
		MoveGuardActor(G);
	}
}

void AScene4Catwalk::MoveGuardActor(FScene4Guard& G)
{
	if (!G.Actor)
	{
		return;
	}
	const FVector Z = G.Home.GetLocation();
	G.Actor->SetActorLocationAndRotation(FVector(G.Loc.X, G.Loc.Y, Z.Z), FRotator(0.f, G.FaceYaw, 0.f));
}

// ---------------------------------------------------------------------------------------------------------------------
// The champagne tower
// ---------------------------------------------------------------------------------------------------------------------

void AScene4Catwalk::BuildTower()
{
	for (UStaticMeshComponent* C : GlassComps)
	{
		if (C)
		{
			C->DestroyComponent();
		}
	}
	GlassComps.Reset();
	Glasses.Reset();
	if (!Cart || TowerTiers <= 0)
	{
		return;
	}
	const float GlassH = GlassScale.Z * 100.f;
	for (int32 Tier = 0; Tier < TowerTiers; ++Tier)
	{
		const int32 N = TowerTiers - Tier;
		const float Half = (N - 1) * GlassSpacing * 0.5f;
		for (int32 ix = 0; ix < N; ++ix)
		{
			for (int32 iy = 0; iy < N; ++iy)
			{
				FGlass Gl;
				Gl.Rel = FVector(ix * GlassSpacing - Half, iy * GlassSpacing - Half, TowerBaseZ + GlassH * 0.5f + Tier * GlassH * 0.92f);
				Gl.Comp = FTakeShapes::AddShapeAtRuntime(this, Tier == TowerTiers - 1 ? ETakeShape::Cone : ETakeShape::Cylinder, Root, FVector::ZeroVector, GlassScale, Champagne, false);
				if (Gl.Comp)
				{
					Gl.Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					Gl.Comp->SetUsingAbsoluteLocation(true);
					Gl.Comp->SetUsingAbsoluteRotation(true);
					Gl.Comp->SetUsingAbsoluteScale(true);
					GlassComps.Add(Gl.Comp);
				}
				Glasses.Add(Gl);
			}
		}
	}
	PlaceTowerOnCart();
}

void AScene4Catwalk::PlaceTowerOnCart()
{
	if (!Cart)
	{
		return;
	}
	const float Alpha = bCartUsed ? FMath::Clamp(CartT / FMath::Max(CartRollSeconds, 0.01f), 0.f, 1.f) : 0.f;
	const FVector Base = Cart->GetActorLocation() + Cart->GetActorTransform().TransformVector(Cart->CrashOffset) * Alpha;
	const FRotator Yaw(0.f, Cart->GetActorRotation().Yaw, 0.f);
	for (FGlass& Gl : Glasses)
	{
		if (Gl.Comp && !Gl.bFlying && !Gl.bDown)
		{
			// A little wobble as it rolls.
			const float Wob = bCartUsed ? FMath::Sin(CartT * 30.f + Gl.Rel.Z) * 1.5f : 0.f;
			Gl.Comp->SetWorldLocation(Base + Yaw.RotateVector(Gl.Rel) + FVector(Wob, 0.f, 0.f));
		}
	}
}

void AScene4Catwalk::ShatterTower()
{
	if (UTakeAudioComponent* A = UTakeAudioComponent::Get(this))
	{
		A->PlayCue(TEXT("sfx_glass_smash"));
	}
	const FVector Dir = Cart ? Cart->GetActorTransform().TransformVector(Cart->CrashOffset).GetSafeNormal2D() : FVector::XAxisVector;
	FRandomStream R(4242);
	for (FGlass& Gl : Glasses)
	{
		Gl.bFlying = true;
		const FVector Out = (Dir * 1.2f + FVector(R.FRandRange(-1.f, 1.f), R.FRandRange(-1.f, 1.f), 0.f)).GetSafeNormal2D();
		Gl.Vel = Out * ShardSpeed * R.FRandRange(0.4f, 1.1f) + FVector(0.f, 0.f, R.FRandRange(120.f, 380.f));
		Gl.Spin = FRotator(R.FRandRange(-720.f, 720.f), R.FRandRange(-720.f, 720.f), R.FRandRange(-720.f, 720.f));
	}
}

void AScene4Catwalk::StepGlasses(float Dt)
{
	if (Dt <= 0.f)
	{
		return;
	}
	for (FGlass& Gl : Glasses)
	{
		if (!Gl.bFlying || !Gl.Comp)
		{
			continue;
		}
		Gl.Vel.Z -= 980.f * Dt;
		FVector P = Gl.Comp->GetComponentLocation() + Gl.Vel * Dt;
		Gl.Comp->AddWorldRotation(Gl.Spin * Dt);
		if (P.Z <= FloorZ + 2.f)
		{
			// Broken on the marble: a flat shard where it landed.
			P.Z = FloorZ + 1.f;
			Gl.bFlying = false;
			Gl.bDown = true;
			Gl.Comp->SetWorldRotation(FRotator(0.f, Gl.Spin.Yaw, 0.f));
			Gl.Comp->SetWorldScale3D(FVector(GlassScale.X * 1.3f, GlassScale.Y * 0.7f, 0.015f));
		}
		Gl.Comp->SetWorldLocation(P);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// The tracking shot and the doors
// ---------------------------------------------------------------------------------------------------------------------

ATakeCharacter* AScene4Catwalk::GetVance() const
{
	const AScriptSequencer* const Seq = GetSequencer();
	return Seq ? Seq->GetPovCharacter(EScriptPov::Vance) : nullptr;
}

void AScene4Catwalk::StepTrack(float Dt)
{
	ATakeCharacter* const V = GetVance();
	if (!V)
	{
		if (Phase == EPhase::Track)
		{
			Phase = EPhase::Doors;
			PhaseSeconds = 0.f;
		}
		return;
	}
	const FVector VLoc = V->GetActorLocation();

	// Vance strolls the lane. The scene carries him along it (the hall's guests and stools can't stall the shot):
	// his movement component rests while the velocity it reports is set, so his walk plays.
	FVector Way = V->GetActorForwardVector();
	UCharacterMovementComponent* const Move = V->GetCharacterMovement();
	if (Lane.IsValidIndex(LaneIndex))
	{
		FVector At = VLoc;
		float Step = StrollSpeed * Dt;
		while (Step > 0.f && Lane.IsValidIndex(LaneIndex))
		{
			const FVector Target(Lane[LaneIndex].X, Lane[LaneIndex].Y, At.Z);
			const float Dist = static_cast<float>(FVector::Dist2D(At, Target));
			if (Dist <= Step)
			{
				At = Target;
				Step -= Dist;
				++LaneIndex;
			}
			else
			{
				At += (Target - At).GetSafeNormal2D() * Step;
				Step = 0.f;
			}
		}
		if (Lane.IsValidIndex(LaneIndex))
		{
			Way = (FVector(Lane[LaneIndex].X, Lane[LaneIndex].Y, At.Z) - At).GetSafeNormal2D();
		}
		if (Dt > 0.f)
		{
			const FRotator Face = FMath::RInterpTo(V->GetActorRotation(), FRotator(0.f, Way.Rotation().Yaw, 0.f), Dt, 6.f);
			V->SetActorLocationAndRotation(At, Face, false, nullptr, ETeleportType::None);
		}
		if (Move)
		{
			Move->SetComponentTickEnabled(false);
			Move->Velocity = Way * StrollSpeed;
		}
	}
	else if (Phase == EPhase::Track)
	{
		if (Move)
		{
			Move->Velocity = FVector::ZeroVector;
		}
		Phase = EPhase::Doors;
		PhaseSeconds = 0.f;
		if (UTakeAudioComponent* A = UTakeAudioComponent::Get(this))
		{
			A->PlayCue(TEXT("sfx_door_close"));
		}
	}
	if (Phase == EPhase::Track && PhaseSeconds > TrackMaxSeconds)
	{
		Phase = EPhase::Doors;
		PhaseSeconds = 0.f;
	}

	// The camera: beside him, a pace ahead, on a smooth rail; it stops at the curtain (it never enters the dark).
	ACameraActor* const Cam = Cast<ACameraActor>(TrackCamera);
	if (!TrackCamera)
	{
		return;
	}
	const FVector LaneFwd = Lane.Num() >= 2 ? (Lane.Last() - Lane[0]).GetSafeNormal2D() : GetActorForwardVector();
	const FVector Fwd = Way.IsNearlyZero() ? LaneFwd : FMath::Lerp(LaneFwd, Way, 0.5f).GetSafeNormal2D();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Fwd);
	FVector Want = VLoc + Fwd * TrackOffset.X + Right * TrackOffset.Y + FVector(0.f, 0.f, TrackOffset.Z - 90.f);
	// Never past the threshold: hold on the hall side of the curtain.
	const FVector Gap = GetActorLocation();
	const FVector GapFwd = GetActorForwardVector();
	const double Past = FVector::DotProduct(Want - Gap, GapFwd);
	if (Past > -120.0)
	{
		Want -= GapFwd * (Past + 120.0);
	}
	if (!bCamPlaced || Dt <= 0.f)
	{
		CamLoc = Want;
		bCamPlaced = true;
	}
	else
	{
		CamLoc = FMath::VInterpTo(CamLoc, Want, Dt, TrackLag);
	}
	const FVector LookAt = (Phase == EPhase::Doors) ? Gap + FVector(0.f, 0.f, 170.f) : VLoc + FVector(0.f, 0.f, 60.f);
	FRotator Rot = (LookAt - CamLoc).Rotation();
	if (bCamPlaced && Dt > 0.f)
	{
		Rot = FMath::RInterpTo(TrackCamera->GetActorRotation(), Rot, Dt, 5.f);
	}
	TrackCamera->SetActorLocationAndRotation(CamLoc, Rot);
	if (Cam && Cam->GetCameraComponent())
	{
		Cam->GetCameraComponent()->SetFieldOfView(TrackFOV);
	}
}

void AScene4Catwalk::StepDoors(float Dt)
{
	const float A = 1.f - FMath::Clamp(PhaseSeconds / FMath::Max(DoorCloseSeconds, 0.05f), 0.f, 1.f);
	SetDoors(A * A);	// they gather speed as they swing shut
	if (PhaseSeconds >= DoorCloseSeconds + HoldAfterDoors && Phase != EPhase::Done)
	{
		Phase = EPhase::Done;
		if (ATakeCharacter* V = GetVance())
		{
			V->GetCharacterMovement()->MaxWalkSpeed = V->WalkSpeed;
		}
		TAKE_LOG(TEXT("S4 FINISH"));
		FinishScene();
	}
}

void AScene4Catwalk::SetDoors(float OpenAlpha)
{
	DoorAlpha = OpenAlpha;
	const float Ang = DoorOpenDegrees * OpenAlpha;
	if (HingeL)
	{
		HingeL->SetRelativeRotation(FRotator(0.f, -Ang, 0.f));
	}
	if (HingeR)
	{
		HingeR->SetRelativeRotation(FRotator(0.f, Ang, 0.f));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Nora's feed: the cones, the lane, the presses
// ---------------------------------------------------------------------------------------------------------------------

void AScene4Catwalk::DrawSceneHud(UCanvas* Canvas)
{
	if (!Canvas || !(Phase == EPhase::Plan || Phase == EPhase::Fail))
	{
		return;
	}
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const float S = H / 1080.f;

	// Through the grating: a faint steel mesh over the feed, heavier at the edges (vertigo).
	const float Step = 120.f * S;
	for (float X = FMath::Fmod(GetSceneSeconds() * 3.f * S, Step); X < W; X += Step)
	{
		HudRect(Canvas, X, 0.f, 2.f * S, H, FLinearColor(0.f, 0.f, 0.f, 0.22f));	// a tile: line items ignore alpha
	}
	HudRect(Canvas, 0.f, 0.f, W, 60.f * S, FLinearColor(0.f, 0.f, 0.f, 0.35f));
	HudRect(Canvas, 0.f, H - 60.f * S, W, 60.f * S, FLinearColor(0.f, 0.f, 0.f, 0.35f));

	auto Proj = [Canvas](const FVector& P, FVector2D& Out) -> bool
	{
		const FVector Sc = Canvas->Project(P, false);
		Out = FVector2D(Sc.X, Sc.Y);
		return Sc.Z > 0.0;
	};

	// The lane: Vance's walk from the bar to the curtain, gold dashes on the floor.
	for (int32 i = 0; i + 1 < Lane.Num(); ++i)
	{
		const FVector A = Lane[i] + FVector(0.f, 0.f, 3.f);
		const FVector B = Lane[i + 1] + FVector(0.f, 0.f, 3.f);
		const int32 N = FMath::Max(1, FMath::CeilToInt(FVector::Dist2D(A, B) / 60.f));
		for (int32 s = 0; s < N; s += 2)
		{
			FVector2D P0, P1;
			if (Proj(FMath::Lerp(A, B, float(s) / N), P0) && Proj(FMath::Lerp(A, B, float(FMath::Min(s + 1, N)) / N), P1))
			{
				HudLine(Canvas, P0, P1, LaneGold, 4.f * S);
			}
		}
	}
	if (Lane.Num() > 0)
	{
		FVector2D P;
		if (Proj(Lane[0], P))
		{
			HudText(Canvas, TEXT("VANCE (AT THE BAR)"), P.X, P.Y + 10.f * S, LaneGold, false, true);
		}
	}

	// The cones.
	int32 Eyes = 0;
	for (int32 i = 0; i < Guards.Num(); ++i)
	{
		const FScene4Guard& G = Guards[i];
		FLinearColor Col = G.bSeesLane ? ConeRed : (G.bDistracted ? ConeOff : ConeIdle);
		if (G.Role == EScene4Role::Guard2 && bAlibi)
		{
			Col = ConeAlibi;
		}
		const bool bFlashSpotted = (i == SpottedBy) && FMath::Fmod(GetSceneSeconds() * 6.f, 1.f) < 0.5f;
		const float Alpha = G.bSeesLane ? (bFlashSpotted ? 0.65f : 0.32f) : 0.14f;
		Eyes += G.bSeesLane ? 1 : 0;

		const float Z = (Cart ? FloorZ : static_cast<float>(G.Loc.Z)) + 4.f;
		const FVector Apex(G.Loc.X, G.Loc.Y, Z);
		FVector2D A2;
		if (!Proj(Apex, A2))
		{
			continue;
		}
		const int32 Seg = 14;
		FVector2D Prev;
		bool bPrev = false;
		for (int32 k = 0; k <= Seg; ++k)
		{
			const float Ang = G.FaceYaw - G.ConeHalfAngle + 2.f * G.ConeHalfAngle * k / Seg;
			const FVector Edge = Apex + FRotator(0.f, Ang, 0.f).Vector() * G.ConeRange;
			FVector2D E2;
			const bool bOk = Proj(Edge, E2);
			if (bOk && bPrev)
			{
				if (const FTexture* Tex = Canvas->DefaultTexture ? Canvas->DefaultTexture->GetResource() : nullptr)
				{
					FCanvasTriangleItem Tri(A2, Prev, E2, Tex);
					Tri.SetColor(FLinearColor(Col.R, Col.G, Col.B, Alpha));
					Tri.BlendMode = SE_BLEND_Translucent;
					Canvas->DrawItem(Tri);
				}
				HudLine(Canvas, Prev, E2, FLinearColor(Col.R, Col.G, Col.B, 0.8f), 2.f * S);
			}
			if (bOk && (k == 0 || k == Seg))
			{
				HudLine(Canvas, A2, E2, FLinearColor(Col.R, Col.G, Col.B, 0.6f), 2.f * S);
			}
			Prev = E2;
			bPrev = bOk;
		}
		HudRect(Canvas, A2.X - 7.f * S, A2.Y - 7.f * S, 14.f * S, 14.f * S, FLinearColor(Col.R, Col.G, Col.B, 0.95f));
		HudText(Canvas, G.Label, A2.X, A2.Y - 46.f * S, FLinearColor(Col.R, Col.G, Col.B, 1.f), false, true);
		HudText(Canvas, G.State, A2.X, A2.Y - 26.f * S, FLinearColor(0.85f, 0.85f, 0.85f, 0.9f), false, true);
	}

	// The two rigs.
	if (Spot)
	{
		FVector2D P;
		if (Proj(Spot->GetActorLocation() + FVector(0.f, 0.f, 150.f), P))
		{
			HudText(Canvas, bSpotUsed ? (SpotLeft > 0.f ? FString::Printf(TEXT("SPOT  %.1f s"), SpotLeft) : TEXT("SPOT  COOLED")) : TEXT("[1] SPOT CLAMP"), P.X, P.Y, bSpotUsed ? ConeOff : LaneGold, false, true);
		}
	}
	if (Cart)
	{
		FVector2D P;
		if (Proj(Cart->GetActorLocation() + FVector(0.f, 0.f, 160.f), P))
		{
			HudText(Canvas, bCartUsed ? (bCrashed ? TEXT("") : TEXT("ROLLING...")) : TEXT("[2] CART LATCH"), P.X, P.Y, bCartUsed ? ConeOff : LaneGold, false, true);
		}
		FVector2D M;
		if (Proj(Cart->GetEffectPoint(), M))
		{
			HudText(Canvas, bCrashed ? TEXT("CRASH") : TEXT("WEST PILLAR"), M.X, M.Y + 8.f * S, FLinearColor(0.8f, 0.8f, 0.8f, 0.8f), false, true);
		}
	}

	// The feed's own labels and the job.
	HudText(Canvas, TEXT("NORA  -  RIGGING CAM  -  +60 FT"), 40.f * S, 16.f * S, FLinearColor(0.9f, 0.9f, 0.9f, 1.f), false);
	if (FMath::Fmod(GetSceneSeconds(), 1.f) < 0.6f)
	{
		HudText(Canvas, TEXT("REC"), W - 110.f * S, 16.f * S, ConeRed, false);
	}
	const bool bClear = Eyes == 0;
	HudText(Canvas, bClear ? TEXT("LANE CLEAR  -  [SPACE] CUE VANCE") : FString::Printf(TEXT("LANE WATCHED BY %d"), Eyes), W * 0.5f, 70.f * S,
		bClear ? ConeAlibi : ConeRed, true, true);
	HudText(Canvas, TEXT("[1] Loosen the spotlight clamp      [2] Drop the line on the cart latch      [SPACE] Cue Vance when no cone is red on the gold lane"),
		W * 0.5f, H - 46.f * S, FLinearColor(0.95f, 0.95f, 0.9f, 1.f), false, true);

	if (FlashLeft > 0.f)
	{
		HudRect(Canvas, 0.f, 0.f, W, H, FLinearColor(1.f, 1.f, 1.f, 0.25f * FlashLeft / 0.12f));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Test hook: Reel.S4.Press 1 | 2 | cue  (the same presses as the keys; tools/script/scene4_pie.py drives it)
// ---------------------------------------------------------------------------------------------------------------------

static void S4Press(const TArray<FString>& Args, UWorld* World)
{
	if (!World || Args.Num() == 0)
	{
		return;
	}
	const FString A = Args[0].ToLower();
	const FKey Key = A == TEXT("1") ? EKeys::One : (A == TEXT("2") ? EKeys::Two : EKeys::SpaceBar);
	for (TActorIterator<AScene4Catwalk> It(World); It; ++It)
	{
		if (It->IsActive())
		{
			It->HandleSceneKey(Key, true);
			It->HandleSceneKey(Key, false);
		}
	}
}

static FAutoConsoleCommandWithWorldAndArgs GS4PressCommand(
	TEXT("Reel.S4.Press"), TEXT("Scene 4: press 1 (the spot clamp), 2 (the cart latch) or cue (Space)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&S4Press));
