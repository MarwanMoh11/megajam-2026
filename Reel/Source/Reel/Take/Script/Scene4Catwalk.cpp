// Scene 4: the catwalks (Nora), then the tracking shot of Vance. Owner: s4-catwalk. See Scene4Catwalk.h.

#include "Script/Scene4Catwalk.h"
#include "Script/ScriptHudWidgets.h"
#include "Script/Scene5Auditorium.h"

#include "Script/ScriptSequencer.h"
#include "FollowSpot.h"
#include "CartDevice.h"
#include "TakeCharacter.h"
#include "Watcher.h"
#include "TakeAudio.h"
#include "TakeDirector.h"
#include "TakeShapes.h"
#include "TakeHudKit.h"
#include "TakeRigCard.h"
#include "TakeTypes.h"

#include "Components/CapsuleComponent.h"
#include "Components/SpotLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
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
	const FName FailWindow(TEXT("THE SPOT BLEW BEFORE VANCE MOVED"));

	const FLinearColor Iron(0.03f, 0.03f, 0.032f);
	const FLinearColor Steel(0.1f, 0.1f, 0.11f);
	const FLinearColor Brass(0.75f, 0.52f, 0.12f);
	const FLinearColor Sackcloth(0.45f, 0.36f, 0.22f);
	const FLinearColor Hemp(0.7f, 0.62f, 0.45f);
	const FLinearColor LensOff(0.12f, 0.12f, 0.1f);

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

	void HudText(UCanvas* C, const FString& S, float X, float Y, const FLinearColor& Col, bool bLarge, bool bCentre = false)
	{
		// QA Nov 16: the HUD's type on a small ink plate (TakeHudKit), not the engine font. Y is the label's top.
		if (!C || S.IsEmpty())
		{
			return;
		}
		FTakeHudKit K;
		K.Begin(C, 0.0, 138.f);
		const TakeStyle::EType Type = bLarge ? TakeStyle::EType::Goal : TakeStyle::EType::Tag;
		const float TW = K.Measure(Type, S);
		const float Em = K.Em(Type);
		const float CY = Y + Em * 0.7f;
		const float X0 = bCentre ? X - TW * 0.5f : X;
		K.Rect(X0 - K.Px(6.f), CY - Em * 0.75f, TW + K.Px(12.f), Em * 1.5f, FTakeHudKit::A(TakeStyle::Ink, 0.6f * Col.A));
		K.Text(Type, S, X0, CY, Col);
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
	BuildRigs();
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
	LampT = -1.f;
	BagT = -1.f;
	bLatchHit = false;
	bHinted = false;
	bWindowClosed = false;
	StuckFor = 0.f;

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
	StepRigs(0.f);

	GhostVance(false);
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
	if (Cart)
	{
		Cart->RollSeconds = CartRollSeconds;
	}
	HoldGuards(true);
	StepGuards(0.f);
	PlaceVanceAtBar();

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
	GhostVance(false);
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
	if (Phase == EPhase::Plan)
	{
		const bool bWasBurning = SpotLeft > 0.f;
		SpotLeft = FMath::Max(SpotLeft - Dt, 0.f);
		if (bWasBurning && SpotLeft <= 0.f && !bWindowClosed)
		{
			// The lamp blows: the window is gone.
			bWindowClosed = true;
			if (UTakeAudioComponent* A = UTakeAudioComponent::Get(this))
			{
				A->PlayCue(TEXT("sfx_glass_smash"), 0.5f);
			}
			Say(TEXT("NORA"), TEXT("The bulb's blown. They're turning back... we lost it."));
			ScheduleFail(FailWindow, WindowClosedSeconds);
		}
		// Nora points at the lamp once if nothing has been touched.
		if (!bHinted && !bSpotUsed && !bCartUsed && GetSceneSeconds() > HintSeconds)
		{
			bHinted = true;
			Say(TEXT("NORA"), TEXT("That lamp under the pipe, by the east door. Loosen its clamp and it swings right into his eyes."));
		}
	}

	// The sandbag falls; on the latch, the cart rolls (the device moves it), then the crash.
	StepRigs(Dt);
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
				if (SpotLeft > 0.f)
				{
					SendGuard2IfOrdered();
				}
				else if (!bAlibi)
				{
					Say(TEXT("GUARD 1"), TEXT("Hold your posts! Nobody leaves the rope."), Guard1HoldVoice);
				}
				TAKE_LOG(TEXT("S4 CRASH spot=%d"), SpotLeft > 0.f ? 1 : 0);
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
	else if (Phase == EPhase::Seat)
	{
		StepSeat(Dt);
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
	// This frame: the clamp's handle spins off, the lamp starts to tip, the East Door guard is in the glare.
	bSpotUsed = true;
	SpotLeft = SpotSeconds;
	LampT = 0.f;
	FlashLeft = 0.12f;
	if (UTakeAudioComponent* A = UTakeAudioComponent::Get(this))
	{
		A->PlayCue(TEXT("Hall/sfx_hall_clamp"));	// the clamp gives, the lamp creaks round
		A->PlayCue(TEXT("sfx_lamp_strike"));
	}
	StepRigs(0.f);
	if (bCrashed)
	{
		SendGuard2IfOrdered();	// the mess is already there: Guard 1 yells at both
	}
	else
	{
		Say(TEXT("GUARD 1"), TEXT("What the hell? Somebody fix that spot!"));
	}
	StepGuards(0.f);	// the cones change this frame
	TAKE_LOG(TEXT("S4 SPOT"));
}

void AScene4Catwalk::SendGuard2IfOrdered()
{
	if (bGuard2Sent || bAlibi)
	{
		return;
	}
	bGuard2Sent = true;
	Say(TEXT("GUARD 1"), TEXT("What the hell, fix that spot! And go help the staff clean that mess before Pendleton sees it!"), Guard1SpotVoice);
}

void AScene4Catwalk::PressCart()
{
	if (bCartUsed || BagT >= 0.f)
	{
		return;
	}
	// This frame: the line is cut and the sandbag drops; it hits the latch SandbagDropSeconds later and the cart rolls.
	BagT = 0.f;
	FlashLeft = 0.12f;
	if (UTakeAudioComponent* A = UTakeAudioComponent::Get(this))
	{
		A->PlayCue(TEXT("sfx_winch_release"));
	}
	StepRigs(0.f);
	TAKE_LOG(TEXT("S4 LINE"));
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
	LaneIndex = Lane.Num() > 1 ? 1 : 0;
	bCamPlaced = false;
	StuckFor = 0.f;
	Say(TEXT("NORA"), TEXT("Lane's clear. Go, Vance. Now."), NoraGoVoice);
	if (ATakeCharacter* V = GetVance())
	{
		PlaceVanceAtBar();
		UCharacterMovementComponent* const Move = V->GetCharacterMovement();
		Move->SetComponentTickEnabled(true);
		Move->MaxWalkSpeed = StrollSpeed;
		Move->bRunPhysicsWithNoController = true;
		VanceLast = V->GetActorLocation();
		GhostVance(true);	// guests and guards never stall the shot; walls and props still do (the lane clears them)
	}
	StepTrack(0.f);
	CutTo(EScriptPov::Vance, TrackCamera);
	TAKE_LOG(TEXT("S4 CUE clear: tracking shot"));
}

void AScene4Catwalk::PlaceVanceAtBar()
{
	ATakeCharacter* const V = GetVance();
	if (!V || Lane.Num() == 0)
	{
		return;
	}
	const FVector Start = Lane[0];
	V->SetActorLocation(FVector(Start.X, Start.Y, Start.Z + V->GetSimpleCollisionHalfHeight() + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
	if (Lane.Num() > 1)
	{
		V->SetActorRotation(FRotator(0.f, YawTo(Lane[0], Lane[1]), 0.f));
	}
	V->GetCharacterMovement()->StopMovementImmediately();
}

void AScene4Catwalk::GhostVance(bool bGhost)
{
	ATakeCharacter* const V = GetVance();
	UCapsuleComponent* const Cap = V ? V->GetCapsuleComponent() : nullptr;
	if (!Cap || bGhost == bVanceGhosted)
	{
		return;
	}
	if (bGhost)
	{
		VancePawnResponse = Cap->GetCollisionResponseToChannel(ECC_Pawn);
		Cap->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}
	else
	{
		Cap->SetCollisionResponseToChannel(ECC_Pawn, VancePawnResponse);
	}
	bVanceGhosted = bGhost;
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
	const FVector SpotAt = LampAt;
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
	UCharacterMovementComponent* const Move = V->GetCharacterMovement();

	// Vance walks the lane on his own feet: the movement component carries him (his walk cycle plays from it, and he
	// collides with every prop; the lane is laid to clear them). If something still holds him (a prop the lane grazes),
	// he is stepped on along the lane after StuckSeconds, so the shot never stalls.
	FVector Way = V->GetActorForwardVector();
	while (Lane.IsValidIndex(LaneIndex) && FVector::Dist2D(VLoc, Lane[LaneIndex]) < 45.f)
	{
		++LaneIndex;
	}
	if (Lane.IsValidIndex(LaneIndex))
	{
		const FVector Target(Lane[LaneIndex].X, Lane[LaneIndex].Y, VLoc.Z);
		Way = (Target - VLoc).GetSafeNormal2D();
		if (Move)
		{
			Move->SetComponentTickEnabled(true);
			Move->MaxWalkSpeed = StrollSpeed;
		}
		V->MoveToward(Target);
		if (Dt > 0.f)
		{
			const float Made = static_cast<float>(FVector::Dist2D(VLoc, VanceLast));
			StuckFor = (Made < StrollSpeed * Dt * 0.2f) ? StuckFor + Dt : 0.f;
			if (StuckFor > StuckSeconds)
			{
				const float Hop = FMath::Min(static_cast<float>(FVector::Dist2D(VLoc, Target)), 90.f);
				V->SetActorLocation(VLoc + Way * Hop, false, nullptr, ETeleportType::TeleportPhysics);
				StuckFor = 0.f;
				TAKE_LOG(TEXT("S4 TRACK stuck at (%.0f, %.0f): stepped %.0f cm on"), VLoc.X, VLoc.Y, Hop);
			}
		}
		VanceLast = VLoc;
	}
	else if (Phase == EPhase::Track)
	{
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

	// The camera: beside him, a pace ahead, on a smooth rail; it stops at the curtain (it never enters the dark), and
	// it never sits inside a prop: anything between him and it pulls it in.
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
	{
		const FVector Eye = VLoc + FVector(0.f, 0.f, 60.f);
		FCollisionQueryParams Q(SCENE_QUERY_STAT(S4TrackCam), false, V);
		Q.AddIgnoredActor(this);
		FHitResult Hit;
		if (GetWorld() && GetWorld()->SweepSingleByChannel(Hit, Eye, Want, FQuat::Identity, ECC_Camera, FCollisionShape::MakeSphere(12.f), Q))
		{
			const FVector Dir = (Want - Eye).GetSafeNormal();
			Want = Eye + Dir * FMath::Max(static_cast<float>(Hit.Distance) - TrackCamClearance, 60.f);
		}
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
		if (Phase == EPhase::Doors && BeginSeat())
		{
			return;
		}
		Phase = EPhase::Done;
		if (ATakeCharacter* V = GetVance())
		{
			V->GetCharacterMovement()->MaxWalkSpeed = V->WalkSpeed;
		}
		TAKE_LOG(TEXT("S4 FINISH"));
		FinishScene();
	}
}

// QA Nov 16 (Marwan): "when Vance crosses into the reserved seating we should see him go and sit in his seat, and
// stay seated for when the camera finds him". After the doors: a cut to Scene 5's seat camera, Vance walking the last
// of the row's leg room into his seat, sitting (Scene 5's own SitVance, so the two scenes agree on the seat), a held
// beat, then the scene ends. Scene 5 opens with him in that seat.
bool AScene4Catwalk::BeginSeat()
{
	ATakeCharacter* const V = GetVance();
	AScene5Auditorium* S5 = nullptr;
	for (TActorIterator<AScene5Auditorium> It(GetWorld()); It; ++It)
	{
		S5 = *It;
		break;
	}
	if (!V || !S5 || !S5->SeatCamera || SeatWalkIn <= 0.f)
	{
		return false;
	}
	SeatScene = S5;
	bSeatSat = false;
	SeatSatAt = 0.f;
	// Off camera, on the cut: a few steps along his row's leg room toward the centre aisle, facing his seat.
	const FVector Stand = S5->StandAt;
	const FVector Along(0.f, Stand.Y < 0.f ? 1.f : -1.f, 0.f);
	const FVector From = Stand + Along * SeatWalkIn;
	const float Half = V->GetCapsuleComponent() ? V->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	const FRotator Face(0.f, (-Along).Rotation().Yaw, 0.f);
	GhostVance(true);	// the row's seated knees don't hold him up
	V->TeleportTo(FVector(From.X, From.Y, Stand.Z + Half + 2.f), Face, false, true);
	V->SetActorRotation(Face);
	V->SetLook(Face.Yaw, -4.f);
	if (UCharacterMovementComponent* Move = V->GetCharacterMovement())
	{
		Move->SetComponentTickEnabled(true);
		Move->SetMovementMode(MOVE_Walking);
		Move->MaxWalkSpeed = StrollSpeed * 0.8f;
	}
	S5->FrameSeatCamera();
	CutTo(EScriptPov::Vance, S5->SeatCamera);
	Phase = EPhase::Seat;
	PhaseSeconds = 0.f;
	TAKE_LOG(TEXT("S4 SEAT: Vance walks into his seat (%.0f cm)"), SeatWalkIn);
	return true;
}

void AScene4Catwalk::StepSeat(float Dt)
{
	ATakeCharacter* const V = GetVance();
	AScene5Auditorium* const S5 = SeatScene.Get();
	if (!bSeatSat)
	{
		const FVector Stand = S5 ? S5->StandAt : FVector::ZeroVector;
		const bool bThere = !V || !S5 || FVector::Dist2D(V->GetActorLocation(), Stand) < 25.f;
		if (bThere || PhaseSeconds > SeatWalkMaxSeconds)
		{
			GhostVance(false);
			if (S5)
			{
				S5->SeatVanceNow();
			}
			bSeatSat = true;
			SeatSatAt = PhaseSeconds;
			TAKE_LOG(TEXT("S4 SEAT: seated after %.1f s"), PhaseSeconds);
		}
		else if (V)
		{
			V->MoveToward(FVector(Stand.X, Stand.Y, V->GetActorLocation().Z));
		}
		return;
	}
	if (S5)
	{
		S5->HoldVanceSeated();
	}
	if (PhaseSeconds - SeatSatAt >= SeatHoldSeconds)
	{
		Phase = EPhase::Done;
		TAKE_LOG(TEXT("S4 FINISH (seated)"));
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
// Nora's rigs: the lamp on its clamp, the sandbag on its line, her catwalk (shapes built at play; no collision)
// ---------------------------------------------------------------------------------------------------------------------

namespace
{
	UMaterialInstanceDynamic* SignalMid(UStaticMeshComponent* C, const FLinearColor& Col)
	{
		UMaterialInterface* const M = FTakeShapes::GetSignalMaterial();
		if (!C || !M)
		{
			return nullptr;
		}
		UMaterialInstanceDynamic* Mid = C->CreateDynamicMaterialInstance(0, M);
		if (Mid)
		{
			Mid->SetVectorParameterValue(FTakeShapes::ColorParam, Col);
		}
		return Mid;
	}
}

FVector AScene4Catwalk::EastDoorEyes() const
{
	const int32 E = FindGuard(EScene4Role::EastDoor);
	if (E == INDEX_NONE)
	{
		return LampParkedAim;
	}
	return FVector(Guards[E].Loc.X, Guards[E].Loc.Y, FloorZ + 165.f);
}

FVector AScene4Catwalk::CartLatch() const
{
	return FVector(BattenAt.X, BattenAt.Y, FloorZ + 95.f);
}

void AScene4Catwalk::BuildRigs()
{
	for (UActorComponent* C : RigComps)
	{
		if (C)
		{
			C->DestroyComponent();
		}
	}
	RigComps.Reset();
	LampTilt = nullptr;
	LampLens = LampShaft = LampPool = ClampHandle = ClampJaw = Sandbag = SandLine = nullptr;
	LampLight = nullptr;

	// A shape in world space (Parent null) or relative to Parent.
	auto Shape = [this](ETakeShape S, const FVector& At, const FVector& Scale, const FRotator& Rot, const FLinearColor& Col,
		USceneComponent* Parent = nullptr, bool bShadow = true) -> UStaticMeshComponent*
	{
		UStaticMeshComponent* C = FTakeShapes::AddShapeAtRuntime(this, S, Parent ? Parent : Root.Get(), FVector::ZeroVector, Scale, Col, bShadow);
		if (!C)
		{
			return nullptr;
		}
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (Parent)
		{
			C->SetRelativeLocationAndRotation(At, Rot);
		}
		else
		{
			C->SetWorldLocationAndRotation(At, Rot);
		}
		RigComps.Add(C);
		return C;
	};
	// A bar (cube or cylinder) from A to B, Thick cm across.
	auto Bar = [&Shape](const FVector& A, const FVector& B, float Thick, const FLinearColor& Col, ETakeShape S = ETakeShape::Cylinder) -> UStaticMeshComponent*
	{
		const FVector D = B - A;
		const float L = static_cast<float>(D.Size());
		if (L < 1.f)
		{
			return nullptr;
		}
		return Shape(S, (A + B) * 0.5f, FVector(Thick / 100.f, Thick / 100.f, L / 100.f), FRotationMatrix::MakeFromZ(D / L).Rotator(), Col);
	};
	// A pipe batten along X through At, hung on two cables into the dark.
	auto Batten = [&Bar, this](const FVector& At)
	{
		const FVector Half(BattenLength * 0.5f, 0.f, 0.f);
		Bar(At - Half, At + Half, 6.f, Steel);
		for (const float Sgn : { -1.f, 1.f })
		{
			const FVector End = At + Half * Sgn * 0.9f;
			Bar(End, End + FVector(0.f, 0.f, 1600.f), 1.5f, Iron);
		}
	};

	// ---- Nora's catwalk: stringers, a grating of slats, posts, two rails, hangers ----
	if (bBuildCatwalk && FVector::Dist(CatwalkFrom, CatwalkTo) > 50.f)
	{
		const FVector D = (CatwalkTo - CatwalkFrom).GetSafeNormal();
		const float L = static_cast<float>(FVector::Dist(CatwalkFrom, CatwalkTo));
		const FVector Side = FVector::CrossProduct(FVector::UpVector, D).GetSafeNormal() * (CatwalkWidth * 0.5f);
		for (const float Sgn : { -1.f, 1.f })
		{
			const FVector Off = Side * Sgn;
			Bar(CatwalkFrom + Off, CatwalkTo + Off, 9.f, Iron, ETakeShape::Cube);
			Bar(CatwalkFrom + Off + FVector(0.f, 0.f, 105.f), CatwalkTo + Off + FVector(0.f, 0.f, 105.f), 5.f, Steel);
			Bar(CatwalkFrom + Off + FVector(0.f, 0.f, 55.f), CatwalkTo + Off + FVector(0.f, 0.f, 55.f), 3.f, Iron);
			for (float s = 0.f; s <= L + 1.f; s += 250.f)
			{
				const FVector P = CatwalkFrom + D * s + Off;
				Bar(P, P + FVector(0.f, 0.f, 105.f), 4.f, Iron, ETakeShape::Cube);
				if (FMath::Fmod(s, 500.f) < 1.f)
				{
					Bar(P + FVector(0.f, 0.f, 105.f), P + FVector(0.f, 0.f, 1200.f), 1.5f, Iron);
				}
			}
		}
		for (float s = 15.f; s < L; s += 32.f)
		{
			const FVector P = CatwalkFrom + D * s;
			Bar(P - Side, P + Side, 3.f, Steel, ETakeShape::Cube);
		}
	}

	// ---- The lamp: a pipe batten, a C-clamp with a brass T-handle, a stem, and the lamp on its tilt ----
	Batten(LampAt + FVector(0.f, 0.f, 70.f));
	Shape(ETakeShape::Cube, LampAt + FVector(0.f, 0.f, 70.f), FVector(0.2f, 0.17f, 0.26f), FRotator::ZeroRotator, Iron);
	ClampJaw = Shape(ETakeShape::Cube, LampAt + FVector(0.f, 13.f, 70.f), FVector(0.16f, 0.05f, 0.22f), FRotator::ZeroRotator, Iron);
	ClampHandle = Shape(ETakeShape::Cylinder, LampAt + FVector(0.f, 30.f, 70.f), FVector(0.06f, 0.06f, 0.36f), FRotator(0.f, 0.f, 90.f), Brass);
	Bar(LampAt + FVector(0.f, 0.f, 58.f), LampAt, 7.f, Iron);
	Shape(ETakeShape::Sphere, LampAt, FVector(0.14f), FRotator::ZeroRotator, Iron);

	LampTilt = NewObject<USceneComponent>(this, TEXT("S4LampTilt"));
	LampTilt->SetupAttachment(Root);
	LampTilt->RegisterComponent();
	AddInstanceComponent(LampTilt);
	LampTilt->SetWorldLocation(LampAt);
	RigComps.Add(LampTilt);
	// Its +X is the beam: a black drum, a rear cap, the lens, four barn doors.
	Shape(ETakeShape::Cylinder, FVector(-8.f, 0.f, 0.f), FVector(0.56f, 0.56f, 0.9f), FRotator(90.f, 0.f, 0.f), Iron, LampTilt);
	Shape(ETakeShape::Cylinder, FVector(-55.f, 0.f, 0.f), FVector(0.42f, 0.42f, 0.08f), FRotator(90.f, 0.f, 0.f), Steel, LampTilt);
	Shape(ETakeShape::Cube, FVector(-5.f, 0.f, 32.f), FVector(0.5f, 0.08f, 0.06f), FRotator::ZeroRotator, Brass, LampTilt);	// the operator's handle
	LampLens = Shape(ETakeShape::Cylinder, FVector(38.f, 0.f, 0.f), FVector(0.46f, 0.46f, 0.03f), FRotator(90.f, 0.f, 0.f), LensOff, LampTilt, false);
	SignalMid(LampLens, LampColour * 4.f);
	Shape(ETakeShape::Cube, FVector(52.f, 0.f, 27.f), FVector(0.28f, 0.5f, 0.015f), FRotator(-18.f, 0.f, 0.f), Iron, LampTilt);
	Shape(ETakeShape::Cube, FVector(52.f, 0.f, -27.f), FVector(0.28f, 0.5f, 0.015f), FRotator(18.f, 0.f, 0.f), Iron, LampTilt);
	Shape(ETakeShape::Cube, FVector(52.f, 27.f, 0.f), FVector(0.28f, 0.015f, 0.5f), FRotator(0.f, 18.f, 0.f), Iron, LampTilt);
	Shape(ETakeShape::Cube, FVector(52.f, -27.f, 0.f), FVector(0.28f, 0.015f, 0.5f), FRotator(0.f, -18.f, 0.f), Iron, LampTilt);

	LampLight = NewObject<USpotLightComponent>(this, TEXT("S4LampLight"));
	LampLight->SetupAttachment(LampTilt);
	LampLight->SetMobility(EComponentMobility::Movable);
	LampLight->RegisterComponent();
	AddInstanceComponent(LampLight);
	LampLight->SetRelativeLocation(FVector(42.f, 0.f, 0.f));
	LampLight->SetIntensityUnits(ELightUnits::Candelas);
	LampLight->SetIntensity(LampCandelas);
	LampLight->SetOuterConeAngle(LampConeDegrees);
	LampLight->SetInnerConeAngle(LampConeDegrees * 0.6f);
	LampLight->SetAttenuationRadius(3000.f);
	LampLight->SetLightColor(LampColour);
	LampLight->SetVolumetricScatteringIntensity(3.f);
	LampLight->SetCastShadows(true);
	RigComps.Add(LampLight);

	// The shaft: a translucent cone from the lens to where it lands (stretched each frame by AimLamp).
	UMaterialInterface* BeamMat = BeamMaterial;
	if (!BeamMat)
	{
		BeamMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Take/Look/M_S4Beam.M_S4Beam"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	if (BeamMat)
	{
		LampShaft = Shape(ETakeShape::Cone, FVector(300.f, 0.f, 0.f), FVector(1.f), FRotator(90.f, 0.f, 0.f), LampColour, LampTilt, false);
		if (LampShaft)
		{
			UMaterialInstanceDynamic* Mid = LampShaft->CreateDynamicMaterialInstance(0, BeamMat);
			if (Mid)
			{
				Mid->SetVectorParameterValue(FTakeShapes::ColorParam, LampColour);
				Mid->SetScalarParameterValue(TEXT("Opacity"), BeamOpacity);
			}
		}
	}
	// The glare on the guard's eyes (shown while he is blinded).
	LampPool = Shape(ETakeShape::Sphere, EastDoorEyes(), FVector(0.3f), FRotator::ZeroRotator, LampColour, nullptr, false);
	SignalMid(LampPool, LampColour * 6.f);
	if (LampPool)
	{
		LampPool->SetVisibility(false);
	}

	// ---- The sandbag: its batten over the cart's latch, the hemp line, the bag, a frayed stub on the hook ----
	Batten(BattenAt);
	Shape(ETakeShape::Cube, BattenAt + FVector(0.f, 0.f, -6.f), FVector(0.12f, 0.12f, 0.12f), FRotator::ZeroRotator, Iron);	// the cleat
	Bar(BattenAt, BattenAt - FVector(0.f, 0.f, 18.f), 2.5f, Hemp);	// the stub that stays when the line is cut
	SandLine = Shape(ETakeShape::Cylinder, BattenAt, FVector(0.025f, 0.025f, 1.f), FRotator::ZeroRotator, Hemp);
	Sandbag = Shape(ETakeShape::Cylinder, BattenAt, FVector(0.34f, 0.34f, 0.5f), FRotator::ZeroRotator, Sackcloth);
	// The latch it will hit: a brass catch on the cart's top rail.
	Shape(ETakeShape::Cube, CartLatch() + FVector(0.f, 0.f, 4.f), FVector(0.14f, 0.1f, 0.08f), FRotator::ZeroRotator, Brass);

	StepRigs(0.f);
}

void AScene4Catwalk::AimLamp(const FRotator& Aim)
{
	LampAimNow = Aim;
	if (LampTilt)
	{
		LampTilt->SetWorldRotation(Aim);
	}
}

void AScene4Catwalk::StepRigs(float Dt)
{
	// ---- the lamp ----
	if (LampT >= 0.f)
	{
		LampT += Dt;
	}
	const FVector Eyes = EastDoorEyes();
	FVector Target = LampParkedAim;
	float Wobble = 0.f;
	if (LampT >= 0.f)
	{
		const float A = FMath::Clamp(LampT / FMath::Max(LampSwingSeconds, 0.05f), 0.f, 1.f);
		Target = FMath::Lerp(LampParkedAim, Eyes, A * A);	// it falls into the swing
		if (A >= 1.f)
		{
			const float T = LampT - LampSwingSeconds;
			Wobble = FMath::Sin(T * 15.f) * 6.f * FMath::Exp(-T * 3.f);
		}
	}
	FRotator Aim = (Target - LampAt).Rotation();
	Aim.Pitch += Wobble;
	AimLamp(Aim);
	const bool bLit = !bWindowClosed;
	if (LampLight)
	{
		LampLight->SetVisibility(bLit);
		LampLight->SetIntensity(LampT >= 0.f ? LampCandelas * 1.6f : LampCandelas);
	}
	if (LampLens)
	{
		if (UMaterialInstanceDynamic* Mid = Cast<UMaterialInstanceDynamic>(LampLens->GetMaterial(0)))
		{
			Mid->SetVectorParameterValue(FTakeShapes::ColorParam, bLit ? LampColour * 4.f : LensOff);
		}
	}
	if (LampShaft)
	{
		const float Len = FMath::Max(static_cast<float>(FVector::Dist(LampAt, Target)) - 45.f, 50.f);
		const float Rad = 25.f + Len * FMath::Tan(FMath::DegreesToRadians(LampConeDegrees));
		LampShaft->SetRelativeLocation(FVector(45.f + Len * 0.5f, 0.f, 0.f));
		LampShaft->SetRelativeScale3D(FVector(Rad / 50.f, Rad / 50.f, Len / 100.f));
		LampShaft->SetVisibility(bLit);
	}
	if (LampPool)
	{
		const bool bGlare = bLit && LampT >= LampSwingSeconds && (SpotLeft > 0.f || Phase == EPhase::Track || Phase == EPhase::Doors);
		LampPool->SetVisibility(bGlare);
		if (bGlare)
		{
			const float Pulse = 0.28f + 0.05f * FMath::Sin(GetSceneSeconds() * 9.f);
			LampPool->SetWorldLocationAndRotation(Eyes, FRotator::ZeroRotator);
			LampPool->SetWorldScale3D(FVector(Pulse));
		}
	}
	// The clamp: the jaw springs open and the T-handle spins off and drops.
	if (ClampJaw)
	{
		const float Open = LampT >= 0.f ? FMath::Clamp(LampT / 0.12f, 0.f, 1.f) : 0.f;
		ClampJaw->SetWorldLocationAndRotation(LampAt + FVector(0.f, 13.f + 10.f * Open, 70.f), FRotator(0.f, 0.f, -35.f * Open));
	}
	if (ClampHandle)
	{
		const FVector Home = LampAt + FVector(0.f, 30.f, 70.f);
		if (LampT < 0.f)
		{
			ClampHandle->SetWorldLocationAndRotation(Home, FRotator(0.f, 0.f, 90.f));
			ClampHandle->SetVisibility(true);
		}
		else
		{
			const float T = LampT;
			const FVector P = Home + FVector(0.f, 120.f * T, 160.f * T - 490.f * T * T);
			ClampHandle->SetWorldLocationAndRotation(P, FRotator(T * 900.f, 0.f, 90.f + T * 1400.f));
			ClampHandle->SetVisibility(P.Z > FloorZ + 5.f);
		}
	}

	// ---- the sandbag ----
	if (BagT >= 0.f)
	{
		BagT += Dt;
	}
	const float BagHalf = 25.f;
	const FVector Hang = BattenAt - FVector(0.f, 0.f, SandbagHang + BagHalf);
	const FVector Hit = CartLatch() + FVector(0.f, 0.f, BagHalf + 6.f);
	FVector Bag = Hang;
	float LineLen = SandbagHang;
	if (BagT >= 0.f)
	{
		const float A = FMath::Clamp(BagT / FMath::Max(SandbagDropSeconds, 0.05f), 0.f, 1.f);
		Bag = FMath::Lerp(Hang, Hit, A * A);
		if (A >= 1.f && !bLatchHit)
		{
			// The latch: the cart rolls, this frame.
			bLatchHit = true;
			bCartUsed = true;
			CartT = 0.f;
			FlashLeft = 0.12f;
			if (Cart)
			{
				Cart->RollSeconds = CartRollSeconds;
				Cart->Fire();
			}
			PlaceTowerOnCart();
			if (UTakeAudioComponent* Au = UTakeAudioComponent::Get(this))
			{
				Au->PlayCue(TEXT("sfx_thud"));
				Au->PlayCue(TEXT("Hall/sfx_hall_gasp"));	// the room gasps as the champagne goes
			}
			TAKE_LOG(TEXT("S4 LATCH: the cart rolls"));
		}
		if (bLatchHit)
		{
			// Off the latch onto the floor beside it.
			const float B = FMath::Clamp((BagT - SandbagDropSeconds) / 0.25f, 0.f, 1.f);
			Bag = FMath::Lerp(Hit, FVector(Hit.X - 45.f, Hit.Y - 30.f, FloorZ + BagHalf * 0.7f), B);
		}
	}
	if (Sandbag)
	{
		Sandbag->SetWorldLocationAndRotation(Bag, bLatchHit ? FRotator(0.f, 0.f, 70.f * FMath::Clamp((BagT - SandbagDropSeconds) / 0.25f, 0.f, 1.f)) : FRotator::ZeroRotator);
	}
	if (SandLine)
	{
		// Hanging: from the hook to the bag. Cut: it falls with the bag, and lies limp once the bag is down.
		const FVector Top = BagT < 0.f ? BattenAt - FVector(0.f, 0.f, 18.f) : Bag + FVector(0.f, 0.f, BagHalf + LineLen);
		const FVector Bot = Bag + FVector(0.f, 0.f, BagHalf);
		SandLine->SetVisibility(!bLatchHit);
		const float L = static_cast<float>(FVector::Dist(Top, Bot));
		SandLine->SetWorldLocationAndRotation((Top + Bot) * 0.5f, FRotator::ZeroRotator);
		SandLine->SetWorldScale3D(FVector(0.025f, 0.025f, FMath::Max(L, 1.f) / 100.f));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Nora's feed: the lane, the guards, the two rigs and what each will do
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
	const float Now = GetSceneSeconds();

	// Through the grating: a faint steel mesh over the feed.
	const float Step = 120.f * S;
	for (float X = FMath::Fmod(Now * 3.f * S, Step); X < W; X += Step)
	{
		HudRect(Canvas, X, 0.f, 2.f * S, H, FLinearColor(0.f, 0.f, 0.f, 0.14f));
	}
	HudRect(Canvas, 0.f, 0.f, W, 60.f * S, FLinearColor(0.f, 0.f, 0.f, 0.45f));
	HudRect(Canvas, 0.f, H - 70.f * S, W, 70.f * S, FLinearColor(0.f, 0.f, 0.f, 0.55f));

	auto Proj = [Canvas](const FVector& P, FVector2D& Out) -> bool
	{
		const FVector Sc = Canvas->Project(P, false);
		Out = FVector2D(Sc.X, Sc.Y);
		return Sc.Z > 0.0;
	};
	auto Dashed = [&](const FVector& A, const FVector& B, const FLinearColor& Col, float Thick, float DashCm)
	{
		const int32 N = FMath::Max(2, FMath::CeilToInt(FVector::Dist(A, B) / DashCm));
		for (int32 s = 0; s < N; s += 2)
		{
			FVector2D P0, P1;
			if (Proj(FMath::Lerp(A, B, float(s) / N), P0) && Proj(FMath::Lerp(A, B, float(FMath::Min(s + 1, N)) / N), P1))
			{
				HudLine(Canvas, P0, P1, Col, Thick);
			}
		}
	};
	const FVector Gap = GetActorLocation();
	const FVector GapFwd = GetActorForwardVector();
	auto WatchedBy = [&](const FVector& P) -> int32
	{
		if (FVector::DotProduct(P - Gap, GapFwd) > 0.0)
		{
			return INDEX_NONE;
		}
		for (int32 i = 0; i < Guards.Num(); ++i)
		{
			const FScene4Guard& G = Guards[i];
			if (G.bDistracted)
			{
				continue;
			}
			FVector D = P - G.Loc;
			D.Z = 0.0;
			const double Dist = D.Size();
			if (Dist <= G.ConeRange && Dist > 1.0
				&& FVector::DotProduct(D / Dist, GuardForward(G)) >= FMath::Cos(FMath::DegreesToRadians(G.ConeHalfAngle)))
			{
				return i;
			}
		}
		return INDEX_NONE;
	};

	// The guards' sight: faint wedges; only the ones that cross the lane stand out.
	TArray<FString> Watchers;
	for (int32 i = 0; i < Guards.Num(); ++i)
	{
		const FScene4Guard& G = Guards[i];
		FLinearColor Col = G.bSeesLane ? ConeRed : (G.bDistracted ? ConeOff : ConeIdle);
		if (G.Role == EScene4Role::Guard2 && bAlibi)
		{
			Col = ConeAlibi;
		}
		if (G.bSeesLane)
		{
			Watchers.Add(G.Label);
		}
		const FVector Apex(G.Loc.X, G.Loc.Y, FloorZ + 4.f);
		FVector2D A2;
		if (!Proj(Apex, A2))
		{
			continue;
		}
		if (!G.bDistracted)
		{
			const bool bFlash = (i == SpottedBy) && FMath::Fmod(Now * 6.f, 1.f) < 0.5f;
			const float Alpha = G.bSeesLane ? (bFlash ? 0.5f : 0.2f) : 0.07f;
			const int32 Seg = 14;
			FVector2D Prev;
			bool bPrev = false;
			for (int32 k = 0; k <= Seg; ++k)
			{
				const float Ang = G.FaceYaw - G.ConeHalfAngle + 2.f * G.ConeHalfAngle * k / Seg;
				FVector2D E2;
				const bool bOk = Proj(Apex + FRotator(0.f, Ang, 0.f).Vector() * G.ConeRange, E2);
				if (bOk && bPrev)
				{
					if (const FTexture* Tex = Canvas->DefaultTexture ? Canvas->DefaultTexture->GetResource() : nullptr)
					{
						FCanvasTriangleItem Tri(A2, Prev, E2, Tex);
						Tri.SetColor(FLinearColor(Col.R, Col.G, Col.B, Alpha));
						Tri.BlendMode = SE_BLEND_Translucent;
						Canvas->DrawItem(Tri);
					}
				}
				if (bOk && (k == 0 || k == Seg))
				{
					HudLine(Canvas, A2, E2, FLinearColor(Col.R, Col.G, Col.B, G.bSeesLane ? 0.7f : 0.3f), 1.5f * S);
				}
				Prev = E2;
				bPrev = bOk;
			}
		}
		HudRect(Canvas, A2.X - 6.f * S, A2.Y - 6.f * S, 12.f * S, 12.f * S, FLinearColor(Col.R, Col.G, Col.B, 0.95f));
		HudText(Canvas, G.Label, A2.X, A2.Y - 44.f * S, FLinearColor(Col.R, Col.G, Col.B, 1.f), false, true);
		HudText(Canvas, G.State, A2.X, A2.Y - 24.f * S, FLinearColor(0.88f, 0.88f, 0.85f, 0.95f), false, true);
	}

	// The lane: gold where nobody looks, red where a guard does.
	for (int32 i = 0; i + 1 < Lane.Num(); ++i)
	{
		const FVector A = Lane[i] + FVector(0.f, 0.f, 3.f);
		const FVector B = Lane[i + 1] + FVector(0.f, 0.f, 3.f);
		const int32 N = FMath::Max(1, FMath::CeilToInt(FVector::Dist2D(A, B) / 45.f));
		for (int32 s = 0; s < N; s += 2)
		{
			const FVector Q0 = FMath::Lerp(A, B, float(s) / N);
			const FVector Q1 = FMath::Lerp(A, B, float(FMath::Min(s + 1, N)) / N);
			FVector2D P0, P1;
			if (Proj(Q0, P0) && Proj(Q1, P1))
			{
				HudLine(Canvas, P0, P1, WatchedBy((Q0 + Q1) * 0.5f) != INDEX_NONE ? ConeRed : LaneGold, 5.f * S);
			}
		}
	}
	if (Lane.Num() > 0)
	{
		FVector2D P;
		if (Proj(Lane[0] + FVector(0.f, 0.f, 200.f), P))
		{
			HudText(Canvas, TEXT("VANCE"), P.X, P.Y - 20.f * S, LaneGold, false, true);
		}
		if (Proj(Lane.Last(), P))
		{
			HudText(Canvas, TEXT("THE CURTAIN"), P.X, P.Y + 8.f * S, LaneGold, false, true);
		}
	}

	// A device's card: a leader from the rig to a box with its key, its name and what it does (or did).
	FTakeHudKit Kit;
	Kit.Begin(Canvas, FPlatformTime::Seconds(), 0.f);
	auto Card = [&](const FVector& At, float DX, float DY, const FString& Key, const FString& Title, const FString& Line,
		const FLinearColor& Col, float Bar)
	{
		FVector2D A;
		if (!Proj(At, A))
		{
			return;
		}
		TakeRigCard::Draw(Kit, A, DX, DY, Key, Title, Line, Col, Bar);	// hud-guidance's kit-styled card (QA: "cards look ugly")
	};

	// [1] The spotlight.
	{
		const FVector Eyes = EastDoorEyes();
		if (!bSpotUsed)
		{
			Dashed(LampAt, Eyes, FLinearColor(1.f, 0.95f, 0.75f, 0.85f), 2.5f * S, 40.f);	// where it will swing
			FVector2D E;
			if (Proj(Eyes, E))
			{
				HudText(Canvas, TEXT("HIS EYES"), E.X, E.Y + 12.f * S, FLinearColor(1.f, 0.95f, 0.75f, 0.95f), false, true);
			}
			Card(LampAt, 60.f, -150.f, TEXT("1"), TEXT("THE SPOTLIGHT CLAMP"), TEXT("Loosen it: the lamp swings into the EAST DOOR guard's eyes."), LaneGold, -1.f);
		}
		else if (bWindowClosed)
		{
			Card(LampAt, 60.f, -150.f, FString(), TEXT("THE SPOTLIGHT"), TEXT("Bulb blown. The East Door guard can see again."), ConeRed, -1.f);
		}
		else
		{
			Card(LampAt, 60.f, -150.f, FString(), TEXT("THE SPOTLIGHT  -  EAST DOOR BLINDED"), TEXT("Guard 1 is yelling up at it. The bulb won't last."), ConeAlibi, SpotLeft / FMath::Max(SpotSeconds, 0.1f));
		}
	}
	// [2] The sandbag line.
	{
		const FVector Mess = Cart ? Cart->GetEffectPoint() : CartLatch();
		if (BagT < 0.f)
		{
			Dashed(CartLatch(), Mess + FVector(0.f, 0.f, 10.f), FLinearColor(1.f, 0.95f, 0.75f, 0.85f), 2.5f * S, 30.f);	// where the cart will roll
			Card(BattenAt - FVector(0.f, 0.f, SandbagHang * 0.5f), -530.f, -40.f, TEXT("2"), TEXT("THE SANDBAG LINE"), TEXT("Cut it: the bag hits the cart's latch, the champagne rolls."), LaneGold, -1.f);
		}
		else
		{
			const bool bSent = bGuard2Sent || bAlibi;
			Card(BattenAt - FVector(0.f, 0.f, SandbagHang * 0.5f), -530.f, -40.f, FString(), bCrashed ? TEXT("THE CHAMPAGNE  -  CRASHED") : TEXT("THE CHAMPAGNE  -  ROLLING"),
				bCrashed ? (bSent ? FString(TEXT("The West Pillar guard is at the mess, with help.")) : FString(TEXT("West Pillar's at the mess. Guard 2 holds: Guard 1 isn't rattled yet.")))
						 : FString(TEXT("...")), ConeAlibi, -1.f);
		}
		FVector2D M;
		if (Proj(Mess, M))
		{
			HudText(Canvas, bCrashed ? TEXT("THE MESS") : TEXT("WEST PILLAR"), M.X, M.Y + 8.f * S, FLinearColor(0.85f, 0.85f, 0.85f, 0.85f), false, true);
		}
	}

	// The feed's own labels and the job.
	HudText(Canvas, TEXT("NORA  -  RIGGING CAM  -  +60 FT"), 40.f * S, 16.f * S, FLinearColor(0.9f, 0.9f, 0.9f, 1.f), false);
	if (FMath::Fmod(Now, 1.f) < 0.6f)
	{
		HudText(Canvas, TEXT("REC"), W - 110.f * S, 16.f * S, ConeRed, false);
	}
	// QA Nov 16: the job and the keys on the HUD's plates with keycaps (ScriptHudWidgets.h), not plain text.
	{
		using ScriptHudWidgets::ETone;
		const bool bClear = Watchers.Num() == 0;
		if (bClear)
		{
			const float Pulse = 0.8f + 0.2f * FMath::Sin(Now * 7.f);
			ScriptHudWidgets::RichPlate(Kit, TakeStyle::EType::Goal, TEXT("Lane clear: [SPACE] Send Vance"), Kit.CX(),
				Kit.Px(96.f), ETone::Good, Pulse, Kit.W * 0.6f);
		}
		else
		{
			ScriptHudWidgets::RichPlate(Kit, TakeStyle::EType::Goal,
				FString::Printf(TEXT("Clear Vance's lane: watched by %s"), *FString::Join(Watchers, TEXT(", "))), Kit.CX(),
				Kit.Px(96.f), ETone::Danger, 1.f, Kit.W * 0.6f);
		}
		ScriptHudWidgets::RichPlate(Kit, TakeStyle::EType::Label,
			TEXT("[1] Spotlight clamp     [2] Sandbag line     [SPACE] Send Vance when no guard sees the gold lane"),
			Kit.CX(), Kit.H - Kit.Px(56.f), ETone::Action, 1.f, Kit.W * 0.8f);
	}

	if (FlashLeft > 0.f)
	{
		HudRect(Canvas, 0.f, 0.f, W, H, FLinearColor(1.f, 1.f, 1.f, 0.2f * FlashLeft / 0.12f));
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
