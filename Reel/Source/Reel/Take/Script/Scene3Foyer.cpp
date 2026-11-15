// Scene 3: the foyer, the critic, the choice, the main hall and the rope. See Scene3Foyer.h.
#include "Script/Scene3Foyer.h"

#include "Script/ScriptSequencer.h"
#include "CastMember.h"
#include "PatrolGuard.h"
#include "TakeCharacter.h"
#include "TakePlayerController.h"
#include "TakeHudKit.h"
#include "TakeShapes.h"
#include "TakeStyle.h"
#include "TakeTypes.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"

namespace Scene3Local
{
	const FName VipAlibi(TEXT("VipAlibi"));
	const FLinearColor Champagne(0.95f, 0.80f, 0.38f);
	const FLinearColor Wet(0.42f, 0.36f, 0.20f);
	constexpr int32 NumDrops = 14;

	const FString OptionHead[2] = { TEXT("PLAY ALONG"), TEXT("BRUSH OFF") };

	double RealNow(const UWorld* World) { return World ? World->GetRealTimeSeconds() : 0.0; }
}

AScene3Foyer::AScene3Foyer()
{
	SceneNumber = 3;
	Pov = EScriptPov::Vance;
	Slug = FText::FromString(TEXT("INT. GRAND FOYER & STAIRCASE - NIGHT"));
	bPlayerWalks = true;
	Tags.Add(TEXT("script_s3"));
}

void AScene3Foyer::BeginPlay()
{
	Super::BeginPlay();
	if (Critic)
	{
		CriticHome = Critic->GetActorLocation();
		CriticHomeYaw = static_cast<float>(Critic->GetActorRotation().Yaw);
	}
}

// --- helpers ----------------------------------------------------------------------------------------------------------

FVector AScene3Foyer::VanceFeet() const
{
	const ATakeCharacter* Vance = GetPovCharacter();
	return Vance ? Vance->GetFeetLocation() : FVector::ZeroVector;
}

float AScene3Foyer::StairZ(const FVector& P) const
{
	if (FMath::Abs(P.Y) > StairHalfY + 40.f || P.X <= StairX0)
	{
		return 0.f;
	}
	const float A = FMath::Clamp((static_cast<float>(P.X) - StairX0) / FMath::Max(StairX1 - StairX0, 1.f), 0.f, 1.f);
	return A * StairTopZ;
}

ACastMember* AScene3Foyer::GuardBody(APatrolGuard* Guard) const
{
	if (!Guard)
	{
		return nullptr;
	}
	TArray<AActor*> Kids;
	Guard->GetAttachedActors(Kids);
	for (AActor* Kid : Kids)
	{
		if (ACastMember* M = Cast<ACastMember>(Kid))
		{
			return M;
		}
	}
	return nullptr;
}

APatrolGuard* AScene3Foyer::NearestGuard(const FVector& P) const
{
	APatrolGuard* Best = nullptr;
	double BestD = TNumericLimits<double>::Max();
	for (APatrolGuard* G : RopeGuards)
	{
		if (G)
		{
			const double D = FVector::Dist2D(G->GetActorLocation(), P);
			if (D < BestD)
			{
				BestD = D;
				Best = G;
			}
		}
	}
	return Best;
}

void AScene3Foyer::FreezeVance(bool bFreeze)
{
	bFrozen = bFreeze;
	ATakeCharacter* Vance = GetPovCharacter();
	UCharacterMovementComponent* Move = Vance ? Vance->GetCharacterMovement() : nullptr;
	if (!Move)
	{
		return;
	}
	if (bFreeze)
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}
	else if (Move->MovementMode == MOVE_None)
	{
		Move->SetMovementMode(MOVE_Walking);
	}
}

void AScene3Foyer::ShowHidden(bool bShow)
{
	for (AActor* A : HideDuringScene)
	{
		if (A)
		{
			A->SetActorHiddenInGame(!bShow);
			TArray<AActor*> Kids;
			A->GetAttachedActors(Kids);
			for (AActor* Kid : Kids)
			{
				Kid->SetActorHiddenInGame(!bShow);
			}
		}
	}
}

void AScene3Foyer::ClearFx()
{
	for (UStaticMeshComponent* D : Drops)
	{
		if (D)
		{
			D->SetVisibility(false);
		}
	}
	DropT = -1.f;
	if (Stain)
	{
		Stain->DestroyComponent();
		Stain = nullptr;
	}
}

void AScene3Foyer::SetBeat(EBeat NewBeat)
{
	Beat = NewBeat;
	BeatT = 0.f;
	TAKE_LOG(TEXT("SCENE3 beat %d t=%.1f"), static_cast<int32>(NewBeat), GetSceneSeconds());
}

// --- hooks ------------------------------------------------------------------------------------------------------------

void AScene3Foyer::ResetScene()
{
	ClearFx();
	FreezeVance(false);
	bPushing = false;
	Picked = EChoice::None;
	Hover = 0;
	bSaidReply = false;
	bAlibiSpent = false;
	ChoiceOpenedAt = -1.0;
	PickedAt = -1.0;
	ConsumeFlag(Scene3Local::VipAlibi);	// a retake replays the choice
	if (Critic)
	{
		Critic->StopWalking();
		Critic->SetActorLocation(CriticHome);
		Critic->SetActorRotation(FRotator(0.f, CriticHomeYaw, 0.f));
		Critic->FaceYaw(CriticHomeYaw);
		Critic->SetActorHiddenInGame(false);
	}
	Beat = EBeat::Arrive;
	BeatT = 0.f;
}

void AScene3Foyer::BeginScene()
{
	Super::BeginScene();
	ShowHidden(false);
	SetBeat(EBeat::Arrive);
	TAKE_LOG(TEXT("SCENE3 begin critic=%s guards=%d hidden=%d"), Critic ? TEXT("yes") : TEXT("NO"), RopeGuards.Num(),
		HideDuringScene.Num());
}

void AScene3Foyer::EndScene()
{
	FreezeVance(false);
	ShowHidden(true);
	ClearFx();
	Super::EndScene();
}

void AScene3Foyer::StartStumble()
{
	SetBeat(EBeat::Stumble);
	RetargetT = 0.f;
	if (Critic)
	{
		Critic->PlayGesture(TEXT("Laugh"));
	}
	if (ATakePlayerController* PC = Cast<ATakePlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		if (Critic)
		{
			PC->StartLookPan(Critic->GetActorLocation() + FVector(0.f, 0.f, 150.f), 0.8f, true);
		}
	}
}

void AScene3Foyer::DoSpill()
{
	SetBeat(EBeat::Spill);
	FreezeVance(true);
	ATakeCharacter* Vance = GetPovCharacter();
	const FVector Feet = VanceFeet();
	FVector GlassAt = Feet + FVector(80.f, 0.f, 130.f);
	if (Critic)
	{
		Critic->StopWalking();
		const FVector C = Critic->GetActorLocation();
		Critic->FaceYaw(static_cast<float>((Feet - C).Rotation().Yaw));
		Critic->PlayGestureAt(TEXT("Talk"), Feet + FVector(0.f, 0.f, 150.f));
		GlassAt = C + (Feet - C).GetSafeNormal2D() * 40.f + FVector(0.f, 0.f, 125.f);
	}
	if (Vance && Critic)
	{
		const FVector ToCritic = (Critic->GetActorLocation() - Feet).GetSafeNormal2D();
		Vance->SetActorRotation(FRotator(0.f, ToCritic.Rotation().Yaw, 0.f));
		if (AController* C = Vance->GetController())
		{
			C->SetControlRotation(FRotator(-8.f, ToCritic.Rotation().Yaw, 0.f));
		}
	}

	// The splash: a fan of droplets from his coupe across Vance's lapel.
	const FVector Lapel = Feet + FVector(0.f, 0.f, 135.f);
	const FVector Dir = (Lapel - GlassAt).GetSafeNormal();
	if (Drops.Num() == 0)
	{
		for (int32 i = 0; i < Scene3Local::NumDrops; ++i)
		{
			UStaticMeshComponent* D = FTakeShapes::AddShapeAtRuntime(this, ETakeShape::Sphere, GetRootComponent(),
				FVector::ZeroVector, FVector(0.04f), Scene3Local::Champagne, false);
			if (D)
			{
				D->SetUsingAbsoluteLocation(true);
				D->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Drops.Add(D);
			}
		}
	}
	DropVel.SetNum(Drops.Num());
	for (int32 i = 0; i < Drops.Num(); ++i)
	{
		const float Spread = FMath::FRandRange(-0.45f, 0.45f);
		const FVector Side = FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal();
		DropVel[i] = (Dir + Side * Spread + FVector(0.f, 0.f, FMath::FRandRange(0.15f, 0.6f))).GetSafeNormal()
			* FMath::FRandRange(260.f, 420.f);
		Drops[i]->SetWorldLocation(GlassAt + FMath::VRand() * 4.f);
		const float S = FMath::FRandRange(0.025f, 0.055f);
		Drops[i]->SetWorldScale3D(FVector(S));
		Drops[i]->SetVisibility(true);
	}
	DropT = 0.f;

	// The wet mark on his white jacket (left lapel), carried by him until the scene ends.
	if (Vance && !Stain)
	{
		Stain = FTakeShapes::AddShapeAtRuntime(Vance, ETakeShape::Sphere, Vance->GetRootComponent(),
			FVector(24.f, -12.f, 42.f), FVector(0.03f, 0.16f, 0.22f), Scene3Local::Wet, false);
		if (Stain)
		{
			Stain->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	if (SplashSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SplashSound, Lapel);
	}
	Say(TEXT("CRITIC"), CriticLine, CriticSound);
}

void AScene3Foyer::Commit(EChoice Pick)
{
	if (Beat != EBeat::Choice || Picked != EChoice::None)
	{
		return;
	}
	Picked = Pick;
	Hover = Pick == EChoice::PlayAlong ? 0 : 1;
	PickedAt = Scene3Local::RealNow(GetWorld());
	bSaidReply = false;
	SetBeat(EBeat::Reply);
	FreezeVance(false);
	if (Pick == EChoice::PlayAlong)
	{
		SetFlag(Scene3Local::VipAlibi);
		Say(TEXT("VANCE"), PlayAlongLine, PlayAlongSound);
		TAKE_LOG(TEXT("SCENE3 choice PlayAlong: VipAlibi set"));
	}
	else
	{
		Say(TEXT("VANCE"), BrushOffLine, BrushOffSound);
		TAKE_LOG(TEXT("SCENE3 choice BrushOff"));
	}
	if (Critic)
	{
		Critic->PlayGesture(Pick == EChoice::PlayAlong ? TEXT("Nod") : TEXT("Shrug"));
	}
}

void AScene3Foyer::StopAtRope()
{
	SetBeat(EBeat::Stopped);
	const FVector Feet = VanceFeet();
	APatrolGuard* Guard = NearestGuard(Feet);
	if (ACastMember* Body = GuardBody(Guard))
	{
		Body->LookAtPoint(Feet + FVector(0.f, 0.f, 150.f), 3.f);
		Body->PlayGestureAt(TEXT("Stop"), Feet + FVector(0.f, 0.f, 120.f));
	}
	FreezeVance(true);
	if (!bAlibiSpent && ConsumeFlag(Scene3Local::VipAlibi))
	{
		bAlibiSpent = true;
		Say(TEXT("GUARD"), GuardAlibiLine, GuardAlibiSound);
		ATakeCharacter* Vance = GetPovCharacter();
		PushFrom = Vance ? Vance->GetActorLocation() : FVector::ZeroVector;
		PushTo = FVector(RopeStopX - AlibiPushBack, PushFrom.Y, PushFrom.Z);
		bPushing = Vance != nullptr;
		TAKE_LOG(TEXT("SCENE3 rope: VipAlibi spent, sent back"));
	}
	else
	{
		Say(TEXT("GUARD"), GuardTackleLine, GuardTackleSound);
		bPushing = false;
		TAKE_LOG(TEXT("SCENE3 rope: tackled"));
	}
}

void AScene3Foyer::TickScene(float DeltaSeconds)
{
	BeatT += DeltaSeconds;
	const FVector Feet = VanceFeet();

	// The splash's droplets: a short ballistic arc, then gone.
	if (DropT >= 0.f)
	{
		DropT += DeltaSeconds;
		for (int32 i = 0; i < Drops.Num() && i < DropVel.Num(); ++i)
		{
			DropVel[i].Z -= 980.f * DeltaSeconds;
			Drops[i]->SetWorldLocation(Drops[i]->GetComponentLocation() + DropVel[i] * DeltaSeconds);
		}
		if (DropT > 0.65f)
		{
			for (UStaticMeshComponent* D : Drops)
			{
				D->SetVisibility(false);
			}
			DropT = -1.f;
		}
	}

	// The critic's feet follow the stair's slope (WalkTo keeps his height).
	if (Critic && Critic->IsWalking())
	{
		FVector C = Critic->GetActorLocation();
		const float Z = StairZ(C);
		if (!FMath::IsNearlyEqual(static_cast<float>(C.Z), Z, 0.5f))
		{
			C.Z = Z;
			Critic->SetActorLocation(C);
		}
	}

	switch (Beat)
	{
	case EBeat::Arrive:
		if (BeatT >= CriticDelay || (Critic && FVector::Dist2D(Feet, Critic->GetActorLocation()) < CriticTriggerRadius))
		{
			StartStumble();
		}
		break;

	case EBeat::Stumble:
	{
		if (!Critic)
		{
			DoSpill();
			break;
		}
		const FVector C = Critic->GetActorLocation();
		if (FVector::Dist2D(C, Feet) <= MeetDistance + 10.f || BeatT >= CriticMaxSeconds)
		{
			DoSpill();
			break;
		}
		// A tipsy line: re-aim at Vance a few times a second, swaying side to side.
		RetargetT -= DeltaSeconds;
		if (RetargetT <= 0.f)
		{
			RetargetT = 0.3f;
			const FVector To = (Feet - C).GetSafeNormal2D();
			const FVector Side(-To.Y, To.X, 0.f);
			const float Sway = CriticSway * FMath::Sin(BeatT * 7.f);
			const FVector Goal = Feet - To * MeetDistance + Side * Sway;
			Critic->WalkTo(Goal, CriticSpeed);
		}
		break;
	}

	case EBeat::Spill:
		// The choice opens as his line lands (the voice's length less 2 s), never sooner than ChoiceDelay.
		if (BeatT >= FMath::Max(ChoiceDelay, CriticSound ? CriticSound->GetDuration() - 2.f : 0.f))
		{
			SetBeat(EBeat::Choice);
			ChoiceOpenedAt = Scene3Local::RealNow(GetWorld());
		}
		break;

	case EBeat::Choice:
		break;

	case EBeat::Reply:
	{
		USoundBase* VanceSound = Picked == EChoice::PlayAlong ? PlayAlongSound.Get() : BrushOffSound.Get();
		USoundBase* ReplySound = Picked == EChoice::PlayAlong ? PlayAlongReplySound.Get() : BrushOffReplySound.Get();
		const float ReplyAt = VanceSound ? VanceSound->GetDuration() + 0.25f : (Picked == EChoice::PlayAlong ? 2.6f : 1.6f);
		const float WanderAt = ReplyAt + (ReplySound ? FMath::Min(ReplySound->GetDuration(), 2.5f) : 1.6f);
		if (!bSaidReply && BeatT >= ReplyAt)
		{
			bSaidReply = true;
			if (Picked == EChoice::PlayAlong)
			{
				Say(TEXT("CRITIC"), PlayAlongReply, PlayAlongReplySound);
				if (Critic)
				{
					Critic->PlayGesture(TEXT("Laugh"));
				}
			}
			else
			{
				Say(TEXT("CRITIC"), BrushOffReply, BrushOffReplySound);
				if (Critic)
				{
					Critic->PlayGesture(TEXT("Refuse"));
				}
			}
		}
		if (bSaidReply && BeatT >= WanderAt)
		{
			if (Critic)
			{
				Critic->WalkTo(CriticWanderTo, 95.f);
			}
			SetBeat(EBeat::Hall);
		}
		// He can walk on while they talk; the hall's checks apply at once.
		if (Feet.X > RopeWarnX)
		{
			Say(TEXT("MAX (V.O.)"), MaxVoLine, MaxVoSound);
			SetBeat(EBeat::Vo);
		}
		break;
	}

	case EBeat::Hall:
		if (Feet.X > RopeWarnX)
		{
			Say(TEXT("MAX (V.O.)"), MaxVoLine, MaxVoSound);
			SetBeat(EBeat::Vo);
		}
		break;

	case EBeat::Vo:
		if (Feet.X > RopeStopX && FMath::Abs(Feet.Y) < 2000.f)
		{
			StopAtRope();
			break;
		}
		if (BeatT >= FinishAfterVo)
		{
			SetBeat(EBeat::Done);
			FreezeVance(false);
			FinishScene();
		}
		break;

	case EBeat::Stopped:
		if (bPushing)
		{
			const float A = FMath::Clamp(BeatT / FMath::Max(AlibiPushSeconds, 0.05f), 0.f, 1.f);
			if (ATakeCharacter* Vance = GetPovCharacter())
			{
				Vance->SetActorLocation(FMath::Lerp(PushFrom, PushTo, FMath::InterpEaseOut(0.f, 1.f, A, 2.f)));
			}
			if (A >= 1.f && BeatT >= 1.6f)
			{
				bPushing = false;
				FreezeVance(false);
				// Back to the V.O. beat, its clock running on (the cut still comes).
				Beat = EBeat::Vo;
				BeatT = FMath::Max(BeatT, FinishAfterVo - 3.f);
			}
		}
		else
		{
			if (BeatT >= TackleSeconds)
			{
				SetBeat(EBeat::Done);
				FailScene(TEXT("Tackled"));
			}
		}
		break;

	case EBeat::Done:
		break;
	}
}

bool AScene3Foyer::HandleSceneKey(const FKey& Key, bool bPressed)
{
	// While he is held (the splash, the choice, the guard), the walk keys go nowhere.
	const bool bWalkKey = Key == EKeys::W || Key == EKeys::A || Key == EKeys::S || Key == EKeys::D
		|| Key == EKeys::Up || Key == EKeys::Down || Key == EKeys::Left || Key == EKeys::Right;
	if (Beat != EBeat::Choice)
	{
		return bPressed && bFrozen && bWalkKey;
	}
	if (!bPressed)
	{
		return bWalkKey;
	}
	if (Key == EKeys::One || Key == EKeys::NumPadOne)
	{
		Commit(EChoice::PlayAlong);
		return true;
	}
	if (Key == EKeys::Two || Key == EKeys::NumPadTwo)
	{
		Commit(EChoice::BrushOff);
		return true;
	}
	if (Key == EKeys::Up || Key == EKeys::W || Key == EKeys::Left || Key == EKeys::A)
	{
		Hover = 0;
		return true;
	}
	if (Key == EKeys::Down || Key == EKeys::S || Key == EKeys::Right || Key == EKeys::D)
	{
		Hover = 1;
		return true;
	}
	if (Key == EKeys::E || Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::LeftMouseButton)
	{
		Commit(Hover == 0 ? EChoice::PlayAlong : EChoice::BrushOff);
		return true;
	}
	return false;
}

void AScene3Foyer::DrawSceneHud(UCanvas* Canvas)
{
	if (!Canvas)
	{
		return;
	}
	using namespace TakeStyle;
	const double Now = Scene3Local::RealNow(GetWorld());
	FTakeHudKit Kit;
	Kit.Begin(Canvas, Now, 138.f);

	// The alibi, once won: a small tag in the picture's top-left corner until it is spent.
	if (HasFlag(Scene3Local::VipAlibi))
	{
		const float X = Kit.Px(40.f);
		const float Y = Kit.PicY + Kit.Px(34.f);
		const FString Tag = TEXT("VIP ALIBI  -  one guard check");
		const float TW = Kit.Measure(EType::Tag, Tag);
		Kit.Rect(X - Kit.Px(12.f), Y - Kit.Px(16.f), TW + Kit.Px(24.f), Kit.Px(32.f), FLinearColor(0.f, 0.f, 0.f, 0.55f));
		Kit.Frame(X - Kit.Px(12.f), Y - Kit.Px(16.f), TW + Kit.Px(24.f), Kit.Px(32.f), Kit.Ln(1.f), TakeStyle::Julian);
		Kit.Text(EType::Tag, Tag, X, Y, TakeStyle::Julian);
	}

	// The goal line, low in the picture.
	FString Goal;
	switch (Beat)
	{
	case EBeat::Arrive:
	case EBeat::Stumble:
		Goal = TEXT("Glide in. The FOYER is yours.");
		break;
	case EBeat::Reply:
	case EBeat::Hall:
		Goal = TEXT("Cross the MAIN HALL toward the velvet rope");
		break;
	case EBeat::Vo:
		Goal = TEXT("Hold back from the GUARDS at the rope");
		break;
	default:
		break;
	}
	if (!Goal.IsEmpty())
	{
		Kit.Text(EType::Goal, Goal, Kit.CX(), Kit.PicBottom() - Kit.Px(130.f), FLinearColor(Paper.R, Paper.G, Paper.B, 0.85f),
			ETakeAlign::Centre);
	}

	// The choice: two cards side by side over the lower third.
	const bool bShowChoice = Beat == EBeat::Choice || (Beat == EBeat::Reply && PickedAt > 0.0 && Now - PickedAt < 0.45);
	if (!bShowChoice)
	{
		return;
	}
	const float In = ChoiceOpenedAt > 0.0 ? FMath::Clamp(static_cast<float>(Now - ChoiceOpenedAt) / 0.18f, 0.f, 1.f) : 1.f;
	const float Out = (Beat == EBeat::Reply) ? 1.f - FMath::Clamp(static_cast<float>(Now - PickedAt) / 0.45f, 0.f, 1.f) : 1.f;
	Kit.Fade = In * Out;

	const FString Lines[2] = { PlayAlongLine, BrushOffLine };
	const FString Keys[2] = { TEXT("1"), TEXT("2") };
	const float CardW = Kit.Px(560.f);
	const float CardH = Kit.Px(118.f);
	const float Gap = Kit.Px(28.f);
	const float Y0 = Kit.PicBottom() - Kit.Px(236.f);
	Kit.Text(EType::Label, TEXT("THE CRITIC  -  your answer"), Kit.CX(), Y0 - Kit.Px(26.f), Bone, ETakeAlign::Centre);
	for (int32 i = 0; i < 2; ++i)
	{
		const bool bOn = (Picked == EChoice::None) ? (Hover == i) : ((Picked == EChoice::PlayAlong) == (i == 0));
		const float X = Kit.CX() + (i == 0 ? -(CardW + Gap * 0.5f) : Gap * 0.5f);
		Kit.Rect(X, Y0, CardW, CardH, FLinearColor(0.f, 0.f, 0.f, bOn ? 0.78f : 0.55f));
		Kit.Frame(X, Y0, CardW, CardH, Kit.Ln(bOn ? 2.f : 1.f), bOn ? TakeStyle::Julian : Silver);
		const FTakeInk Ink = FTakeInk::Of(bOn ? TakeStyle::Julian : Bone, bOn ? TakeStyle::Julian : Bone);
		const float KX = X + Kit.Px(18.f);
		const float KW = Kit.Key(Keys[i], KX, Y0 + Kit.Px(30.f), Ink);
		Kit.Text(EType::Label, Scene3Local::OptionHead[i], KX + KW + Kit.Px(12.f), Y0 + Kit.Px(30.f),
			bOn ? TakeStyle::Julian : Bone);
		Kit.Text(EType::Line, FString::Printf(TEXT("\"%s\""), *Lines[i]), X + Kit.Px(18.f), Y0 + Kit.Px(76.f),
			bOn ? Paper : Ash);
	}
	Kit.Text(EType::Tag, TEXT("1 / 2   or   arrows + E"), Kit.CX(), Y0 + CardH + Kit.Px(22.f), Silver, ETakeAlign::Centre);
	Kit.Fade = 1.f;
}
