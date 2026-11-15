// Scene 5. INT. PRIVATE AUDITORIUM. Owner: s5-auditorium. See Scene5Auditorium.h for the beats.
#include "Script/Scene5Auditorium.h"

#include "CastMember.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HouseDimmer.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "TakeCharacter.h"
#include "TakeHudKit.h"
#include "TakeShapes.h"
#include "TakeStyle.h"

DEFINE_LOG_CATEGORY_STATIC(LogScene5, Log, All);

namespace Scene5Local
{
	const FName KeepTag(TEXT("HouseKeep"));
	const FLinearColor SpotWarm(1.f, 0.86f, 0.66f);
	const FLinearColor TorchWhite(1.f, 0.93f, 0.78f);
	const FLinearColor LampRed(1.f, 0.04f, 0.02f);
	const FLinearColor LampGreen(0.1f, 1.f, 0.2f);
	const FVector HouseMiddle(2700.f, 0.f, 170.f);
}

AScene5Auditorium::AScene5Auditorium()
{
	SceneNumber = 5;
	Pov = EScriptPov::Vance;
	Slug = FText::FromString(TEXT("INT. PRIVATE AUDITORIUM - NIGHT"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	PendletonSpot = FTakeShapes::CreateCueSpot(this, TEXT("PendletonSpot"), Root, FVector::ZeroVector, FRotator::ZeroRotator, 3000.f);
	PendletonSpot->ComponentTags.Add(Scene5Local::KeepTag);

	DoorLamp = FTakeShapes::CreateCuePoint(this, TEXT("DoorLamp"), Root, FVector::ZeroVector, 350.f);
	DoorLamp->ComponentTags.Add(Scene5Local::KeepTag);

	DoorBulb = FTakeShapes::CreateShape(this, TEXT("DoorBulb"), ETakeShape::Sphere, Root, FVector::ZeroVector,
		FVector(0.05f), FRotator::ZeroRotator, false);

	TitleText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TitleText"));
	TitleText->SetupAttachment(Root);
	TitleText->SetHorizontalAlignment(EHTA_Center);
	TitleText->SetVerticalAlignment(EVRTA_TextCenter);
	TitleText->SetWorldSize(150.f);
	TitleText->SetTextRenderColor(FColor(255, 236, 200));
	TitleText->SetCastShadow(false);
	TitleText->SetHiddenInGame(true);

	SubTitleText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("SubTitleText"));
	SubTitleText->SetupAttachment(Root);
	SubTitleText->SetHorizontalAlignment(EHTA_Center);
	SubTitleText->SetVerticalAlignment(EVRTA_TextCenter);
	SubTitleText->SetWorldSize(60.f);
	SubTitleText->SetTextRenderColor(FColor(230, 210, 170));
	SubTitleText->SetCastShadow(false);
	SubTitleText->SetHiddenInGame(true);
}

void AScene5Auditorium::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	PlaceParts();
}

void AScene5Auditorium::BeginPlay()
{
	Super::BeginPlay();
	PlaceParts();
	if (!Dimmer)
	{
		for (TActorIterator<AHouseDimmer> It(GetWorld()); It; ++It) { Dimmer = *It; break; }
	}
	WatchStarts.Reset();
	for (const FScene5Watch& W : Watches)
	{
		WatchStarts.Add(W.Who ? W.Who->GetActorTransform() : FTransform::Identity);
		if (W.Who) { W.Who->Tags.AddUnique(Scene5Local::KeepTag); }	// the dimmer never turns their heads
	}
	if (UMaterialInterface* M = TitleMaterial.LoadSynchronous())
	{
		TitleText->SetTextMaterial(M);
		SubTitleText->SetTextMaterial(M);
	}
	FTakeShapes::SetCueLight(PendletonSpot, 0.f, Scene5Local::SpotWarm);
	UpdateLamp();
}

void AScene5Auditorium::PlaceParts()
{
	if (PendletonSpot)
	{
		const FVector Dir = (PendletonSpotTo - PendletonSpotFrom).GetSafeNormal();
		PendletonSpot->SetWorldLocationAndRotation(PendletonSpotFrom, Dir.Rotation());
	}
	if (DoorLamp)
	{
		DoorLamp->SetWorldLocation(DoorLampAt + FVector(0.f, 10.f, 0.f));	// just into the hall off the wall
	}
	if (DoorBulb)
	{
		DoorBulb->SetWorldLocation(DoorLampAt);
	}
	const FRotator Face(0.f, TitleYaw, 0.f);
	if (TitleText)
	{
		TitleText->SetWorldLocationAndRotation(TitleAt, Face);
		TitleText->SetText(FText::FromString(Title));
	}
	if (SubTitleText)
	{
		SubTitleText->SetWorldLocationAndRotation(TitleAt - FVector(0.f, 0.f, 150.f), Face);
		SubTitleText->SetText(FText::FromString(SubTitle));
	}
}

// ---- the hooks ----------------------------------------------------------------------------------------------------

void AScene5Auditorium::ResetScene()
{
	StopScore(0.1f);
	if (SpeechComp) { SpeechComp->Stop(); }
	Say(FString(), FString(), nullptr);  // clear the subtitle lane (the speech's line holds ~12 s)
	if (Dimmer) { Dimmer->EndScreening(); }
	ShowTitle(false);
	for (USpotLightComponent* T : Torches) { FTakeShapes::SetCueLight(T, 0.f, Scene5Local::TorchWhite); }
	for (int32 i = 0; i < Watches.Num() && i < WatchStarts.Num(); ++i)
	{
		if (Watches[i].Who) { Watches[i].Who->SetActorTransform(WatchStarts[i]); }
	}
	Seen = 0.f;
	SeenBy = -1;
	bWhoSaid = false;
	bVanceSaid = false;
	GraceLeft = 0.f;
	bDoorUnlocked = false;
}

void AScene5Auditorium::BeginScene()
{
	Super::BeginScene();
	PlaceParts();
	EnsureTorches();
	Phase = EScene5Beat::Speech;
	PhaseTime = 0.f;
	bPointed = false;
	BeatT = 0.f;
	SneakTime = 0.f;
	Seen = 0.f;
	bLitNow = bMovingNow = false;

	// The house low, Pendleton alone in his spot; Vance stands at his aisle seat and waits.
	if (Dimmer)
	{
		Dimmer->EndScreening();
		Dimmer->SetHouseNow(SpeechHouseLevel);
	}
	FTakeShapes::SetCueLight(PendletonSpot, PendletonSpotCandelas, Scene5Local::SpotWarm, 1.2f);
	FTakeShapes::SetCueCone(PendletonSpot, PendletonSpotDegrees, 0.75f);
	UpdateLamp();
	SetWalkSpeed(0.f);

	const bool bShort = GetRetakes() > 0;
	SpeechEnd = bShort ? 1.2f : SoundSeconds(PendletonSound, SpeechSeconds) + 0.4f;
	if (!bShort)
	{
		// Played here (not through Say's sound) so [Space] can cut it off.
		Say(TEXT("PENDLETON"), PendletonLine, nullptr);
		if (USoundBase* S = PendletonSound.LoadSynchronous())
		{
			SpeechComp = UGameplayStatics::SpawnSound2D(this, S, 1.f, 1.f, 0.f, nullptr, false, false);
		}
	}
	if (ACastMember* P = GetPendletonCast())
	{
		P->LookAtPoint(Scene5Local::HouseMiddle, SpeechEnd);
	}
	UE_LOG(LogScene5, Log, TEXT("S5 begin: speech %.1f s (retake %d)"), SpeechEnd, GetRetakes());
}

void AScene5Auditorium::EndScene()
{
	Super::EndScene();
	RestoreWalkSpeed();
	StopScore(1.5f);
	if (SpeechComp) { SpeechComp->Stop(); }
	// Left mid-scene (a jump, a cut): Pendleton's line must not run on over the next scene. Done keeps Vance's line.
	if (Phase != EScene5Beat::Done) { Say(FString(), FString(), nullptr); }
	ShowTitle(false);
	for (USpotLightComponent* T : Torches) { FTakeShapes::SetCueLight(T, 0.f, Scene5Local::TorchWhite); }
	FTakeShapes::SetCueLight(PendletonSpot, 0.f, Scene5Local::SpotWarm);
	// Finished: the film runs on into Scenes 6 and 7. Left any other way (a jump): the house comes back up.
	if (Phase != EScene5Beat::Done && Dimmer)
	{
		Dimmer->EndScreening();
	}
}

bool AScene5Auditorium::HandleSceneKey(const FKey& Key, bool bPressed)
{
	if (Phase == EScene5Beat::Speech && (Key == EKeys::SpaceBar || Key == EKeys::Enter))
	{
		if (bPressed && PhaseTime < SpeechEnd - PointLead)
		{
			if (SpeechComp) { SpeechComp->FadeOut(0.3f, 0.f); }
			PhaseTime = FMath::Max(0.f, SpeechEnd - PointLead);
		}
		return true;
	}
	return false;
}

void AScene5Auditorium::TickScene(float DeltaSeconds)
{
	PhaseTime += DeltaSeconds;
	GraceLeft = FMath::Max(0.f, GraceLeft - DeltaSeconds);

	if (Phase == EScene5Beat::Speech)
	{
		if (!bPointed && PhaseTime >= SpeechEnd - PointLead)
		{
			bPointed = true;
			if (ACastMember* P = GetPendletonCast()) { P->PlayGestureAt(TEXT("Point"), BoothPoint); }
		}
		if (PhaseTime >= SpeechEnd)
		{
			Blackout();
		}
		UpdateLamp();
		return;
	}
	if (Phase == EScene5Beat::Done)
	{
		return;
	}

	// The score's clock: a timpani hit every BeatSeconds.
	SneakTime += DeltaSeconds;
	BeatT += DeltaSeconds;
	if (BeatT >= BeatSeconds)
	{
		BeatT = FMath::Fmod(BeatT, BeatSeconds);
		OnBeat();
	}
	if (SneakTime >= TitleSeconds) { ShowTitle(false); }
	StepTorches(DeltaSeconds);
	UpdateLamp();

	ATakeCharacter* const Vance = GetPovCharacter();
	if (Phase == EScene5Beat::Sneak && Vance)
	{
		const FVector At = Vance->GetActorLocation();
		bMovingNow = Vance->GetVelocity().Size2D() > MovingSpeed;
		int32 Who = -1;
		bLitNow = IsLit(At, Who);
		if (bLitNow && bMovingNow && !InBoomWindow())
		{
			Seen += DeltaSeconds / FMath::Max(SeenSeconds, 0.05f);
			SeenBy = Who;
			if (!bWhoSaid && Seen > 0.4f && Watches.IsValidIndex(Who))
			{
				bWhoSaid = true;
				SayLine(Watches[Who].Speaker, WhoLine, WhoSound);
			}
		}
		else
		{
			Seen = FMath::Max(0.f, Seen - DeltaSeconds / FMath::Max(SeenSeconds * 2.f, 0.1f));
		}
		if (Seen >= 1.f)
		{
			if (ConsumeFlag(TEXT("VipAlibi")))
			{
				// Scene 3's alibi: the usher takes him for a guest looking for the powder room.
				SayLine(Watches.IsValidIndex(SeenBy) ? Watches[SeenBy].Speaker : TEXT("USHER"), AlibiLine, AlibiSound);
				Seen = 0.f;
				GraceLeft = AlibiGraceSeconds;
				UE_LOG(LogScene5, Log, TEXT("S5 seen: VipAlibi spent"));
			}
			else
			{
				SayLine(TEXT("GUARD"), SeenLine, SeenSound);
				SetWalkSpeed(0.f);
				SetPhase(EScene5Beat::Caught);
				UE_LOG(LogScene5, Log, TEXT("S5 seen by watch %d"), SeenBy);
			}
		}
		else if (FVector::Dist2D(At, DoorMark) <= DoorRadius)
		{
			ArriveAtDoor();
		}
	}
	else if (Phase == EScene5Beat::Caught)
	{
		if (PhaseTime >= 1.0f)
		{
			SetPhase(EScene5Beat::Done);
			FailScene(TEXT("SEEN IN THE DARK"));
		}
	}
	else if (Phase == EScene5Beat::Door)
	{
		if (!bVanceSaid && PhaseTime >= 0.7f)
		{
			bVanceSaid = true;
			SayLine(TEXT("VANCE"), VanceLine, VanceSound);
			DoorEnd = PhaseTime + SoundSeconds(VanceSound, 3.6f) + 0.6f;
		}
		if (bVanceSaid && PhaseTime >= DoorEnd)
		{
			SetPhase(EScene5Beat::Done);
			UE_LOG(LogScene5, Log, TEXT("S5 finished at the door"));
			FinishScene();
		}
	}
}

// ---- the beats ---------------------------------------------------------------------------------------------------

void AScene5Auditorium::Blackout()
{
	SetPhase(EScene5Beat::Sneak);
	if (SpeechComp) { SpeechComp->FadeOut(0.2f, 0.f); }
	Say(FString(), FString(), nullptr);  // the speech is over: its line goes with the lights
	PlayOneShot(LightsCutSound);
	PlayOneShot(ProjectorSound, 0.7f);
	if (Dimmer) { Dimmer->BeginScreening(DarkHouseLevel); }
	FTakeShapes::SetCueLight(PendletonSpot, 0.f, Scene5Local::SpotWarm);
	ShowTitle(true);
	if (USoundBase* S = ScoreSound.LoadSynchronous())
	{
		StopScore(0.f);
		ScoreComp = UGameplayStatics::SpawnSound2D(this, S, ScoreVolume, 1.f, 0.f, nullptr, false, false);
	}
	EnsureTorches();
	SneakTime = 0.f;
	BeatT = 0.f;
	OnBeat();	// the title lands on the first hit
	SetWalkSpeed(SneakSpeed);
	if (SceneCamera)
	{
		CutTo(EScriptPov::Vance, nullptr);	// off Pendleton's close shot, back to Vance in the dark
	}
	UE_LOG(LogScene5, Log, TEXT("S5 blackout: house cut, film rolling"));
}

void AScene5Auditorium::OnBeat()
{
	PlayOneShot(TimpaniSound, TimpaniVolume);
	if (Dimmer) { Dimmer->FlashScreen(1.f); }
	LastBeatReal = FPlatformTime::Seconds();
}

void AScene5Auditorium::ArriveAtDoor()
{
	SetPhase(EScene5Beat::Door);
	SetWalkSpeed(0.f);
	PlayOneShot(MagLockSound);
	UE_LOG(LogScene5, Log, TEXT("S5 at the door"));
}

bool AScene5Auditorium::InBoomWindow() const
{
	return BeatT <= MoveAfter || BeatT >= BeatSeconds - MoveBefore;
}

void AScene5Auditorium::SetPhase(EScene5Beat NewPhase)
{
	Phase = NewPhase;
	PhaseTime = 0.f;
}

// ---- the torches -----------------------------------------------------------------------------------------------------

void AScene5Auditorium::EnsureTorches()
{
	while (Torches.Num() < Watches.Num())
	{
		USpotLightComponent* T = NewObject<USpotLightComponent>(this, NAME_None, RF_Transient);
		T->SetupAttachment(Root);
		T->ComponentTags.Add(Scene5Local::KeepTag);
		FTakeShapes::SetupCueLight(T, TorchRange * 1.15f);
		T->RegisterComponent();
		FTakeShapes::SetCueLight(T, 0.f, Scene5Local::TorchWhite);
		Torches.Add(T);
	}
}

void AScene5Auditorium::StepTorches(float DeltaSeconds)
{
	for (int32 i = 0; i < Watches.Num() && i < Torches.Num(); ++i)
	{
		const FScene5Watch& W = Watches[i];
		USpotLightComponent* const T = Torches[i];
		if (!W.Who || !T)
		{
			continue;
		}
		const float U = 0.5f - 0.5f * FMath::Cos(2.f * PI * (SneakTime / FMath::Max(W.SweepSeconds, 0.5f) + W.Phase));
		const float Yaw = FMath::Lerp(W.YawA, W.YawB, U);
		const FVector Fwd = FRotator(0.f, Yaw, 0.f).Vector();
		const FVector Feet = W.Who->GetActorLocation();
		T->SetWorldLocationAndRotation(FVector(Feet.X, Feet.Y, TorchHeight) + Fwd * 30.f, FRotator(-9.f, Yaw, 0.f));
		FTakeShapes::SetCueCone(T, TorchHalfDegrees, 0.55f);
		FTakeShapes::SetCueLight(T, TorchCandelas, Scene5Local::TorchWhite, 1.5f);
		if (ACastMember* C = Cast<ACastMember>(W.Who))
		{
			if (FMath::Abs(FMath::FindDeltaAngleDegrees(static_cast<float>(C->GetActorRotation().Yaw), Yaw)) > 4.f)
			{
				C->FaceYaw(Yaw);
			}
		}
		else
		{
			W.Who->SetActorRotation(FRotator(0.f, Yaw, 0.f));
		}
	}
}

bool AScene5Auditorium::IsLit(const FVector& At, int32& OutWho) const
{
	OutWho = -1;
	if (GraceLeft > 0.f)
	{
		return false;
	}
	for (int32 i = 0; i < Watches.Num() && i < Torches.Num(); ++i)
	{
		const USpotLightComponent* const T = Torches[i];
		if (!Watches[i].Who || !T)
		{
			continue;
		}
		FVector D = At - T->GetComponentLocation();
		D.Z = 0.f;
		const float Dist = D.Size();
		if (Dist > TorchRange || Dist < 1.f)
		{
			continue;
		}
		FVector Fwd = T->GetForwardVector();
		Fwd.Z = 0.f;
		const float Cos = FVector::DotProduct(D / Dist, Fwd.GetSafeNormal());
		const float Half = TorchHalfDegrees + FMath::RadiansToDegrees(FMath::Atan2(35.f, Dist));	// his shoulders
		if (Cos >= FMath::Cos(FMath::DegreesToRadians(Half)))
		{
			OutWho = i;
			return true;
		}
	}
	return false;
}

// ---- the door, the title, sound, walking -----------------------------------------------------------------------------

void AScene5Auditorium::SetDoorUnlocked(bool bUnlocked)
{
	bDoorUnlocked = bUnlocked;
	UpdateLamp();
}

void AScene5Auditorium::UpdateLamp()
{
	if (!DoorBulb || !DoorLamp)
	{
		return;
	}
	bool bOn = true;
	if (Phase == EScene5Beat::Door && !bDoorUnlocked)
	{
		bOn = FMath::Fmod(PhaseTime, 0.25f) < 0.125f;	// the refusal: a fast red blink
	}
	const FLinearColor C = bDoorUnlocked ? Scene5Local::LampGreen : Scene5Local::LampRed;
	FTakeShapes::SetCueLight(DoorLamp, bOn ? 6.f : 0.f, C);
	BulbMID = FTakeShapes::SetColour(DoorBulb, bOn ? C * 4.f : C * 0.05f);
	FTakeShapes::SetSignal(DoorBulb, bOn ? TakeStyle::StencilRed : static_cast<uint8>(0));
}

void AScene5Auditorium::ShowTitle(bool bShow)
{
	if (TitleText) { TitleText->SetHiddenInGame(!bShow); }
	if (SubTitleText) { SubTitleText->SetHiddenInGame(!bShow); }
}

void AScene5Auditorium::StopScore(float FadeSeconds)
{
	if (ScoreComp)
	{
		if (FadeSeconds > 0.f) { ScoreComp->FadeOut(FadeSeconds, 0.f); }
		else { ScoreComp->Stop(); }
		ScoreComp = nullptr;
	}
}

void AScene5Auditorium::SayLine(const FString& Speaker, const FString& Line, const TSoftObjectPtr<USoundBase>& Sound)
{
	Say(Speaker, Line, Sound.LoadSynchronous());
}

float AScene5Auditorium::SoundSeconds(const TSoftObjectPtr<USoundBase>& Sound, float Fallback) const
{
	const USoundBase* S = Sound.LoadSynchronous();
	const float D = S ? S->GetDuration() : 0.f;
	return (D > 0.1f && D < 120.f) ? D : Fallback;
}

void AScene5Auditorium::PlayOneShot(const TSoftObjectPtr<USoundBase>& Sound, float Volume)
{
	if (USoundBase* S = Sound.LoadSynchronous())
	{
		UGameplayStatics::PlaySound2D(this, S, Volume);
	}
}

ACastMember* AScene5Auditorium::GetPendletonCast() const
{
	if (!Pendleton)
	{
		return nullptr;
	}
	if (ACastMember* C = Cast<ACastMember>(Pendleton))
	{
		return C;
	}
	TArray<AActor*> Kids;
	Pendleton->GetAttachedActors(Kids);
	for (AActor* K : Kids)
	{
		if (ACastMember* C = Cast<ACastMember>(K)) { return C; }
	}
	return nullptr;
}

void AScene5Auditorium::SetWalkSpeed(float Speed)
{
	ATakeCharacter* const C = GetPovCharacter();
	UCharacterMovementComponent* const Move = C ? C->GetCharacterMovement() : nullptr;
	if (!Move)
	{
		return;
	}
	if (SavedWalkSpeed < 0.f)
	{
		SavedWalkSpeed = Move->MaxWalkSpeed;
	}
	Move->MaxWalkSpeed = Speed;
	if (Speed <= 0.f)
	{
		Move->StopMovementImmediately();
	}
}

void AScene5Auditorium::RestoreWalkSpeed()
{
	ATakeCharacter* const C = GetPovCharacter();
	if (UCharacterMovementComponent* const Move = C ? C->GetCharacterMovement() : nullptr)
	{
		if (SavedWalkSpeed >= 0.f) { Move->MaxWalkSpeed = SavedWalkSpeed; }
	}
	SavedWalkSpeed = -1.f;
}

// ---- the HUD -------------------------------------------------------------------------------------------------------

void AScene5Auditorium::DrawSceneHud(UCanvas* Canvas)
{
	using namespace TakeStyle;
	if (!Canvas)
	{
		return;
	}
	FTakeHudKit Kit;
	Kit.Begin(Canvas, FPlatformTime::Seconds(), 0.f);

	if (Phase == EScene5Beat::Speech)
	{
		Kit.Text(EType::Label, TEXT("SPACE  skip"), Kit.W - Kit.Px(48.f), Kit.RowB2(), FTakeHudKit::A(Ash, 0.8f), ETakeAlign::Right);
		return;
	}
	if (Phase == EScene5Beat::Done)
	{
		return;
	}

	// The goal, in the top bar.
	Kit.Text(EType::Goal, TEXT("Slip out to the oak door on the south wall"), Kit.CX(), Kit.RowT1(), Paper, ETakeAlign::Centre);
	Kit.Text(EType::Label, TEXT("In torchlight, move only on the BOOM of the timpani"), Kit.CX(), Kit.RowT2(), Julian, ETakeAlign::Centre);

	// The drum mark, in the bottom bar: a frame closes onto the diamond and the hit lands on it.
	const float Cx = Kit.CX();
	const float Cy = Kit.PicBottom() + Kit.Bar * 0.45f;
	const float U = FMath::Clamp(BeatT / FMath::Max(BeatSeconds, 0.1f), 0.f, 1.f);
	const bool bWindow = InBoomWindow();
	const float Big = Kit.Px(96.f), Small = Kit.Px(26.f);
	const float Side = FMath::Lerp(Big, Small, U);
	const FLinearColor Mark = bWindow ? Julian : FTakeHudKit::A(Bone, 0.55f);
	Kit.Frame(Cx - Side * 0.5f, Cy - Side * 0.5f, Side, Side, Kit.Ln(2.f), Mark);
	const float Since = static_cast<float>(FPlatformTime::Seconds() - LastBeatReal);
	const float Pop = Since < 0.25f ? (1.f - Since / 0.25f) : 0.f;
	Kit.Diamond(Cx, Cy, Small * (1.f + 1.6f * Pop), Pop > 0.f ? Paper : Mark);

	const float Gap = Kit.Px(72.f);
	if (bWindow)
	{
		Kit.Text(EType::Goal, TEXT("BOOM - MOVE"), Cx - Gap, Cy, Julian, ETakeAlign::Right);
	}
	else if (bLitNow)
	{
		Kit.Text(EType::Goal, TEXT("HOLD STILL"), Cx - Gap, Cy, Danger, ETakeAlign::Right);
	}
	Kit.Text(EType::Label, bLitNow ? TEXT("IN THE TORCHLIGHT") : TEXT("in the dark"), Cx + Gap, Cy,
		bLitNow ? Danger : FTakeHudKit::A(Ash, 0.7f), ETakeAlign::Left);

	// SEEN fills while he moves off the boom in a torch.
	if (Seen > 0.01f)
	{
		const float Bw = Kit.Px(220.f), Bh = Kit.Px(8.f);
		const float Bx = Cx - Bw * 0.5f, By = Cy + Kit.Px(44.f);
		Kit.Rect(Bx, By, Bw, Bh, FTakeHudKit::A(Ink, 0.8f));
		Kit.Rect(Bx, By, Bw * FMath::Clamp(Seen, 0.f, 1.f), Bh, Danger);
	}

	// The door's marker, when it is in front of the camera.
	if (Phase == EScene5Beat::Sneak)
	{
		APlayerController* const PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		if (PC && PC->PlayerCameraManager)
		{
			const FVector Cam = PC->PlayerCameraManager->GetCameraLocation();
			const FVector Dir = PC->PlayerCameraManager->GetCameraRotation().Vector();
			if (FVector::DotProduct(DoorLampAt - Cam, Dir) > 50.f)
			{
				const FVector P = Canvas->Project(DoorLampAt);
				const float Y = P.Y - Kit.Px(34.f);
				Kit.Diamond(P.X, Y, Kit.Px(12.f), Danger);
				Kit.Text(EType::Tag, TEXT("DOOR"), P.X, Y - Kit.Px(20.f), Danger, ETakeAlign::Centre);
			}
		}
	}
}
