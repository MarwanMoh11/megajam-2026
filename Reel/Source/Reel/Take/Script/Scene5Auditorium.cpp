// Scene 5. INT. PRIVATE AUDITORIUM. Owner: s5-auditorium. See Scene5Auditorium.h for the beats.
#include "Script/Scene5Auditorium.h"
#include "Script/ScriptHudWidgets.h"

#include "AnimationRuntime.h"
#include "Animation/AnimSequenceBase.h"
#include "CastMember.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Canvas.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HouseDimmer.h"
#include "TakeGuide.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "TakeCharacter.h"
#include "TakeHudKit.h"
#include "TakeShapes.h"
#include "TakeStyle.h"
#include "Script/Scene5Lipsync.inl"

DEFINE_LOG_CATEGORY_STATIC(LogScene5, Log, All);

namespace Scene5Local
{
	const FName KeepTag(TEXT("HouseKeep"));
	const FLinearColor SpotWarm(1.f, 0.86f, 0.66f);
	const FLinearColor TorchWhite(1.f, 0.93f, 0.78f);
	const FLinearColor LampRed(1.f, 0.04f, 0.02f);
	const FLinearColor LampGreen(0.1f, 1.f, 0.2f);
	const FLinearColor ScreenCool(0.62f, 0.70f, 0.95f);
	const FVector HouseMiddle(2700.f, 0.f, 170.f);

	float Smooth(float U) { U = FMath::Clamp(U, 0.f, 1.f); return U * U * (3.f - 2.f * U); }
	float LerpYaw(float A, float B, float U) { return A + FMath::FindDeltaAngleDegrees(A, B) * U; }

	/** The baked mouth at T seconds into the voice: Open and Wide, 0..1 (0 outside the bake). */
	void Lips(float T, float& OutOpen, float& OutWide)
	{
		OutOpen = OutWide = 0.f;
		const float F = T * Scene5Lipsync::Fps;
		if (F < 0.f || F >= static_cast<float>(Scene5Lipsync::Frames - 1))
		{
			return;
		}
		const int32 I = FMath::FloorToInt(F);
		const float U = F - static_cast<float>(I);
		OutOpen = FMath::Lerp(static_cast<float>(Scene5Lipsync::Open[I]), static_cast<float>(Scene5Lipsync::Open[I + 1]), U) / 255.f;
		OutWide = FMath::Lerp(static_cast<float>(Scene5Lipsync::Wide[I]), static_cast<float>(Scene5Lipsync::Wide[I + 1]), U) / 255.f;
	}
}

AScene5Auditorium::AScene5Auditorium()
{
	SceneNumber = 5;
	Pov = EScriptPov::Vance;
	Slug = FText::FromString(TEXT("INT. PRIVATE AUDITORIUM - NIGHT"));
	bAllowCrouch = true;	// the sneak is crouched (BeginSneak); C still toggles it

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
	SitVance(false);
	if (PendletonMouth) { PendletonMouth->SetVisibility(false); }
	if (ACastMember* const P = GetPendletonCast()) { P->SetTalkHead(0.f); }
	if (VanceFill) { FTakeShapes::SetCueLight(VanceFill, 0.f, Scene5Local::ScreenCool); }
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
	bPointed = bCutaway = bCutBack = false;
	bStood = bGoSaid = bReplySaid = false;
	RiseLeg = 0;
	RiseStuck = 0.f;
	SneakTime = 0.f;
	Seen = 0.f;
	bLitNow = bMovingNow = false;
	SpeechPlayStart = -1.f;
	bPlayerWalks = false;	// the scene walks him until the sneak

	// The house low, Pendleton alone in his spot; Vance in his seat in the row.
	if (Dimmer)
	{
		Dimmer->EndScreening();
		Dimmer->SetHouseNow(SpeechHouseLevel);
	}
	FTakeShapes::SetCueLight(PendletonSpot, PendletonSpotCandelas, Scene5Local::SpotWarm, 1.2f);
	FTakeShapes::SetCueCone(PendletonSpot, PendletonSpotDegrees, 0.75f);
	UpdateLamp();
	SetWalkSpeed(0.f);

	if (GetRetakes() > 0)
	{
		// A retake: straight back into the dark with the film running, Vance in the aisle (the StartSpot).
		Blackout();
		BeginSneak();
		UE_LOG(LogScene5, Log, TEXT("S5 begin: retake %d, straight to the sneak"), GetRetakes());
		return;
	}

	FrameSeatCamera();
	SitVance(true);
	SpeechEnd = SoundSeconds(PendletonSound, SpeechSeconds) + 0.4f;
	// Played here (not through Say's sound) so [Space] can cut it off.
	Say(TEXT("PENDLETON"), PendletonLine, nullptr);
	if (USoundBase* S = PendletonSound.LoadSynchronous())
	{
		SpeechComp = UGameplayStatics::SpawnSound2D(this, S, 1.f, 1.f, 0.f, nullptr, false, false);
		SpeechPlayStart = 0.f;
	}
	if (ACastMember* P = GetPendletonCast())
	{
		P->LookAtPoint(Scene5Local::HouseMiddle, SpeechEnd);
		if (!SpeechGesture.IsNone()) { P->PlayGesture(SpeechGesture); }
	}
	EnsureMouth();
	UE_LOG(LogScene5, Log, TEXT("S5 begin: speech %.1f s, seated %d, lipsync %.2f s"), SpeechEnd, bSeated ? 1 : 0, Scene5Lipsync::Seconds);
}

void AScene5Auditorium::EndScene()
{
	Super::EndScene();
	if (ATakeCharacter* const V = GetPovCharacter()) { V->SetCrouching(false); }
	RestoreWalkSpeed();
	SitVance(false);
	StopScore(1.5f);
	if (SpeechComp) { SpeechComp->Stop(); }
	if (PendletonMouth) { PendletonMouth->SetVisibility(false); }
	if (ACastMember* const P = GetPendletonCast()) { P->SetTalkHead(0.f); }
	if (VanceFill) { FTakeShapes::SetCueLight(VanceFill, 0.f, Scene5Local::ScreenCool); }
	// Left mid-scene (a jump, a cut): Pendleton's line must not run on over the next scene. Done keeps Vance's line.
	if (Phase != EScene5Beat::Done) { Say(FString(), FString(), nullptr); }
	ShowTitle(false);
	for (USpotLightComponent* T : Torches) { FTakeShapes::SetCueLight(T, 0.f, Scene5Local::TorchWhite); }
	FTakeShapes::SetCueLight(PendletonSpot, 0.f, Scene5Local::SpotWarm);
	bPlayerWalks = true;
	// Finished: the film runs on into Scenes 6 and 7. Left any other way (a jump): the house comes back up.
	if (Phase != EScene5Beat::Done && Dimmer)
	{
		Dimmer->EndScreening();
	}
}

void AScene5Auditorium::StopFilm(bool bHouseUp)
{
	StopScore(0.f);
	if (!Dimmer)
	{
		for (TActorIterator<AHouseDimmer> It(GetWorld()); It; ++It) { Dimmer = *It; break; }
	}
	if (!Dimmer)
	{
		return;
	}
	const float Level = Dimmer->GetHouseLevel();
	Dimmer->EndScreening();
	if (!bHouseUp) { Dimmer->SetHouseNow(Level); }
	UE_LOG(LogScene5, Log, TEXT("S5 StopFilm (house up %d)"), bHouseUp ? 1 : 0);
}

bool AScene5Auditorium::HandleSceneKey(const FKey& Key, bool bPressed)
{
	const bool bSkip = Key == EKeys::SpaceBar || Key == EKeys::Enter;
	if (Phase == EScene5Beat::Speech && bSkip)
	{
		if (bPressed && PhaseTime < SpeechEnd - PointLead)
		{
			if (SpeechComp) { SpeechComp->FadeOut(0.3f, 0.f); }
			SpeechPlayStart = -1.f;	// the mouth shuts with the voice
			PhaseTime = FMath::Max(0.f, SpeechEnd - PointLead);
			// The point must be seen: off the cutaway onto Pendleton now, and no cutaway later.
			if (bCutaway && !bCutBack && PendletonCamera2) { CutTo(EScriptPov::Vance, PendletonCamera2); }
			bCutaway = bCutBack = true;
		}
		return true;
	}
	if (Phase == EScene5Beat::Rise && bSkip)
	{
		if (bPressed)
		{
			// To the aisle at once: the reply still plays over the walk.
			if (!bGoSaid) { bGoSaid = true; SayLine(TEXT("VANCE"), GoLine, GoSound); ReplyAt = PhaseTime + 0.6f; }
			SitVance(false);
			bStood = true;
			RiseLeg = 2;
			if (ATakeCharacter* const Vance = GetPovCharacter())
			{
				const float Half = Vance->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
				Vance->TeleportTo(FVector(AisleAt.X, AisleAt.Y, AisleAt.Z + Half + 2.f), Vance->GetActorRotation(), false, true);
			}
		}
		return true;
	}
	return false;
}

void AScene5Auditorium::TickScene(float DeltaSeconds)
{
	PhaseTime += DeltaSeconds;
	HoldVanceSeated();	// seated until the rise stands him up
	GraceLeft = FMath::Max(0.f, GraceLeft - DeltaSeconds);

	if (Phase == EScene5Beat::Speech)
	{
		StepMouth();
		const float CutAt = SpeechEnd * CutawayAt;
		if (!bCutaway && SeatCamera && PendletonCamera2 && bSeated && PhaseTime >= CutAt && CutAt + CutawaySeconds < SpeechEnd - PointLead)
		{
			bCutaway = true;
			CutTo(EScriptPov::Vance, SeatCamera);	// Vance in his seat, listening
		}
		if (bCutaway && !bCutBack && PhaseTime >= CutAt + CutawaySeconds)
		{
			bCutBack = true;
			if (PendletonCamera2) { CutTo(EScriptPov::Vance, PendletonCamera2); }
		}
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

	SneakTime += DeltaSeconds;
	if (TitleSeconds > 0.f && SneakTime >= TitleSeconds) { ShowTitle(false); }
	StepTorches(DeltaSeconds);
	UpdateLamp();

	if (Phase == EScene5Beat::Rise)
	{
		StepRise();
		return;
	}

	ATakeCharacter* const Vance = GetPovCharacter();
	if (Phase == EScene5Beat::Sneak && Vance)
	{
		// Cole's answer, if the walk out of the row was cut short.
		if (bGoSaid && !bReplySaid && SneakTime >= ReplyAt)	// ReplyAt: seconds after the blackout
		{
			bReplySaid = true;
			SayLine(ReplySpeaker, ReplyLine, ReplySound);
		}
		const FVector At = Vance->GetActorLocation();
		bMovingNow = Vance->GetVelocity().Size2D() > MovingSpeed;
		int32 Who = -1;
		bLitNow = IsLit(At, Who);
		if (bLitNow && bMovingNow)
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
	SetPhase(EScene5Beat::Rise);
	if (SpeechComp) { SpeechComp->FadeOut(0.2f, 0.f); }
	SpeechPlayStart = -1.f;
	if (PendletonMouth) { PendletonMouth->SetVisibility(false); }
	if (ACastMember* const P = GetPendletonCast()) { P->SetTalkHead(0.f); }
	Say(FString(), FString(), nullptr);  // the speech is over: its line goes with the lights
	PlayOneShot(LightsCutSound);
	PlayOneShot(TimpaniSound, TimpaniVolume);	// one boom with the dark: the film's first frame
	PlayOneShot(ProjectorSound, 0.7f);
	if (Dimmer)
	{
		Dimmer->BeginScreening(DarkHouseLevel);
		Dimmer->FlashScreen(1.f);
	}
	FTakeShapes::SetCueLight(PendletonSpot, 0.f, Scene5Local::SpotWarm);
	ShowTitle(TitleSeconds > 0.f);
	if (USoundBase* S = ScoreSound.LoadSynchronous())
	{
		StopScore(0.f);
		// The sneak's cue (ElevenLabs, Nov 16), louder than the level's tuned ScoreVolume (Marwan: "make the music louder").
		ScoreComp = UGameplayStatics::SpawnSound2D(this, S, FMath::Min(ScoreVolume * 1.3f, 1.3f), 1.f, 0.f, nullptr, false, false);
	}
	EnsureTorches();
	SneakTime = 0.f;

	// The screen's cool bounce on Vance's face, so the seat and exit shots read in the black hall.
	ATakeCharacter* const Vance = GetPovCharacter();
	if (Vance && VanceFillCandelas > 0.f)
	{
		if (!VanceFill)
		{
			VanceFill = NewObject<UPointLightComponent>(this, NAME_None, RF_Transient);
			VanceFill->SetupAttachment(Root);
			VanceFill->ComponentTags.Add(Scene5Local::KeepTag);
			FTakeShapes::SetupCueLight(VanceFill, 450.f);
			VanceFill->RegisterComponent();
		}
		VanceFill->AttachToComponent(Vance->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		VanceFill->SetRelativeLocation(FVector(140.f, 30.f, 60.f));	// ahead of him, toward the screen, at the face
		FTakeShapes::SetCueLight(VanceFill, VanceFillCandelas, Scene5Local::ScreenCool);
	}

	if (bSeated && SeatCamera)
	{
		CutTo(EScriptPov::Vance, SeatCamera);	// off Pendleton, onto Vance in the dark row
	}
	else if (GetRetakes() == 0)
	{
		// No seat (no clip, or no camera): the rise is just the walk out from where he stands.
		bStood = true;
		SetWalkSpeed(RiseSpeed);
		if (ExitCamera) { CutTo(EScriptPov::Vance, ExitCamera); }
	}
	UE_LOG(LogScene5, Log, TEXT("S5 blackout: house cut, film rolling"));
}

void AScene5Auditorium::StepRise()
{
	if (!bGoSaid && PhaseTime >= GoLineAt)
	{
		bGoSaid = true;
		SayLine(TEXT("VANCE"), GoLine, GoSound);
		ReplyAt = PhaseTime + SoundSeconds(GoSound, 2.6f) + 0.2f;
	}
	if (bGoSaid && !bReplySaid && PhaseTime >= ReplyAt)
	{
		bReplySaid = true;
		SayLine(ReplySpeaker, ReplyLine, ReplySound);
	}
	if (!bStood && PhaseTime >= StandUpAt)
	{
		bStood = true;
		SitVance(false);
		SetWalkSpeed(RiseSpeed);
		if (ExitCamera) { CutTo(EScriptPov::Vance, ExitCamera); }	// the cut on the action: he is up
		RiseLastAt = FVector::ZeroVector;
		RiseStuck = 0.f;
	}
	if (!bStood)
	{
		return;
	}
	ATakeCharacter* const Vance = GetPovCharacter();
	if (!Vance)
	{
		BeginSneak();
		return;
	}
	const bool bTimeUp = PhaseTime >= StandUpAt + RiseMaxSeconds;
	if (RiseLeg < 2 && !bTimeUp)
	{
		const FVector Target = RiseLeg == 0 ? RowExit : AisleAt;
		const FVector At = Vance->GetActorLocation();
		if (FVector::Dist2D(At, Target) < 45.f)
		{
			++RiseLeg;
		}
		else
		{
			Vance->MoveToward(Target);
			// Stuck on a chair or a knee: hop to the leg's end (off camera it reads as a step).
			if (FVector::Dist2D(At, RiseLastAt) < 1.f) { RiseStuck += GetWorld()->GetDeltaSeconds(); }
			else { RiseStuck = 0.f; }
			RiseLastAt = At;
			if (RiseStuck > 0.6f)
			{
				const float Half = Vance->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
				Vance->TeleportTo(FVector(Target.X, Target.Y, Target.Z + Half + 2.f), Vance->GetActorRotation(), false, true);
				RiseStuck = 0.f;
				++RiseLeg;
				UE_LOG(LogScene5, Log, TEXT("S5 rise: stuck, hopped to leg %d"), RiseLeg);
			}
		}
		return;
	}
	if (bTimeUp && RiseLeg < 2)
	{
		const float Half = Vance->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Vance->TeleportTo(FVector(AisleAt.X, AisleAt.Y, AisleAt.Z + Half + 2.f), Vance->GetActorRotation(), false, true);
	}
	BeginSneak();
}

void AScene5Auditorium::BeginSneak()
{
	SetPhase(EScene5Beat::Sneak);	// the rise's clock was SneakTime's: Cole's ReplyAt carries over
	SitVance(false);
	SetWalkSpeed(SneakSpeed);
	bPlayerWalks = true;
	if (ATakeCharacter* const V = GetPovCharacter()) { V->SetCrouching(bCrouchSneak); }	// low along the wall
	if (ATakeCharacter* const Vance = GetPovCharacter())
	{
		const float Yaw = (DoorMark - Vance->GetActorLocation()).GetSafeNormal2D().Rotation().Yaw;
		Vance->SetLook(Yaw, -12.f);
	}
	if (VanceFill) { FTakeShapes::SetCueLight(VanceFill, 0.f, Scene5Local::ScreenCool); }
	CutTo(EScriptPov::Vance, nullptr);	// the follow camera: the player has him
	if (UTakeGuideSubsystem* const G = UTakeGuideSubsystem::Get(this)) { G->SetGoal(DoorMark, TEXT("The oak door"), TEXT("South wall, Vance. The oak door.")); }
	UE_LOG(LogScene5, Log, TEXT("S5 sneak: the player walks (%.1f s after the blackout)"), SneakTime);
}

void AScene5Auditorium::ArriveAtDoor()
{
	if (ATakeCharacter* const V = GetPovCharacter()) { V->SetCrouching(false); }	// up at the keypad
	SetPhase(EScene5Beat::Door);
	SetWalkSpeed(0.f);
	PlayOneShot(MagLockSound);
	if (UTakeGuideSubsystem* const G = UTakeGuideSubsystem::Get(this)) { G->ClearGoal(); }
	UE_LOG(LogScene5, Log, TEXT("S5 at the door"));
}

void AScene5Auditorium::SetPhase(EScene5Beat NewPhase)
{
	Phase = NewPhase;
	PhaseTime = 0.f;
}

// ---- Vance in his seat -------------------------------------------------------------------------------------------

void AScene5Auditorium::HoldVanceSeated()
{
	ATakeCharacter* const Vance = GetPovCharacter();
	if (!Vance || !bSeated)
	{
		return;
	}
	if (UCharacterMovementComponent* const Move = Vance->GetCharacterMovement())
	{
		if (Move->MovementMode != MOVE_None)
		{
			Move->StopMovementImmediately();
			Move->DisableMovement();
			UE_LOG(LogScene5, Log, TEXT("S5: Vance's movement came back on in his seat: held"));
		}
	}
	const float Half = Vance->GetCapsuleComponent() ? Vance->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	const FVector At = SeatAt + SitOffset + FVector(0.f, 0.f, Half);
	if (FVector::DistSquared(Vance->GetActorLocation(), At) > 4.0)
	{
		Vance->SetActorLocationAndRotation(At, FRotator(0.f, SeatYaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void AScene5Auditorium::FrameSeatCamera()
{
	ACameraActor* const Cam = Cast<ACameraActor>(SeatCamera);
	if (!Cam)
	{
		return;
	}
	const FRotator Seat(0.f, SeatYaw, 0.f);
	const FVector Target = SeatAt + FVector(0.f, 0.f, 95.f);
	const FVector At = SeatAt + Seat.RotateVector(SeatCamOffset);
	Cam->SetActorLocationAndRotation(At, (Target - At).Rotation());
	if (UCameraComponent* const C = Cam->GetCameraComponent())
	{
		C->SetFieldOfView(SeatCamFov);
	}
}

void AScene5Auditorium::SitVance(bool bSit)
{
	ATakeCharacter* const Vance = GetPovCharacter();
	if (!Vance || bSit == bSeated)
	{
		return;
	}
	USkeletalMeshComponent* const Mesh = Vance->GetMesh();
	UCharacterMovementComponent* const Move = Vance->GetCharacterMovement();
	const float Half = Vance->GetCapsuleComponent() ? Vance->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	if (bSit)
	{
		UAnimSequenceBase* const Clip = SitClip.LoadSynchronous();
		if (!Mesh || !Clip)
		{
			UE_LOG(LogScene5, Warning, TEXT("S5: no sit clip, Vance stands through the speech"));
			return;
		}
		Vance->SetActorEnableCollision(false);
		// The hall's crowd fills every aisle seat: the guest on Vance's seat leaves for the scene (hidden, no collision).
		for (TActorIterator<ACastMember> It(GetWorld()); It; ++It)
		{
			ACastMember* const Guest = *It;
			if (Guest && Guest->GetAttachParentActor() != Vance && !Guest->IsHidden()
				&& FVector::Dist2D(Guest->GetActorLocation(), SeatAt) < 60.f)
			{
				Guest->SetActorHiddenInGame(true);
				Guest->SetActorEnableCollision(false);
				UE_LOG(LogScene5, Log, TEXT("S5: %s gave up Vance's seat"), *Guest->GetName());
			}
		}
		if (Move) { Move->StopMovementImmediately(); Move->DisableMovement(); }
		const FVector At = SeatAt + SitOffset + FVector(0.f, 0.f, Half);
		Vance->TeleportTo(At, FRotator(0.f, SeatYaw, 0.f), false, true);
		Vance->SetActorRotation(FRotator(0.f, SeatYaw, 0.f));
		Vance->SetLook(SeatYaw, -6.f);
		Mesh->PlayAnimation(Clip, true);
		bSeated = true;
		return;
	}
	// Up: the walk's anim blueprint back, the capsule on, on his feet in the leg room.
	if (Mesh) { Mesh->SetAnimationMode(EAnimationMode::AnimationBlueprint); }
	Vance->SetActorEnableCollision(true);
	const FRotator Face(0.f, (RowExit - StandAt).GetSafeNormal2D().Rotation().Yaw, 0.f);
	const FVector At = StandAt + FVector(0.f, 0.f, Half + 2.f);
	if (!Vance->TeleportTo(At, Face, false, false))
	{
		Vance->TeleportTo(At, Face, false, true);
	}
	Vance->SetActorRotation(Face);
	if (Move) { Move->SetMovementMode(MOVE_Walking); }
	bSeated = false;
}

// ---- Pendleton's mouth ---------------------------------------------------------------------------------------------

void AScene5Auditorium::EnsureMouth()
{
	if (!bLipSync)
	{
		return;
	}
	ACastMember* const P = GetPendletonCast();
	USkeletalMeshComponent* const Body = P ? P->Body.Get() : nullptr;
	const USkeletalMesh* const Mesh = Body ? Body->GetSkeletalMeshAsset() : nullptr;
	if (!Mesh)
	{
		UE_LOG(LogScene5, Warning, TEXT("S5: no Pendleton body for the mouth"));
		return;
	}
	const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
	const int32 Head = Ref.FindBoneIndex(TEXT("head"));
	if (Head == INDEX_NONE)
	{
		return;
	}
	MouthHeadRef = FAnimationRuntime::GetComponentSpaceTransformRefPose(Ref, Head);
	if (!PendletonMouth)
	{
		PendletonMouth = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
		FTakeShapes::SetupShape(PendletonMouth, ETakeShape::Sphere, false);
		PendletonMouth->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PendletonMouth->SetGenerateOverlapEvents(false);
		PendletonMouth->SetCastShadow(false);
		PendletonMouth->SetupAttachment(Body, TEXT("head"));
		PendletonMouth->RegisterComponent();
		FTakeShapes::SetColour(PendletonMouth, MouthColour);
	}
	else if (PendletonMouth->GetAttachParent() != Body)
	{
		PendletonMouth->AttachToComponent(Body, FAttachmentTransformRules::KeepRelativeTransform, TEXT("head"));
	}
	bMouthPlaced = true;
	PendletonMouth->SetVisibility(true);
	StepMouth();
}

void AScene5Auditorium::StepMouth()
{
	float Open = 0.f, Wide = 0.5f;
	bool bTalking = false, bBaked = false;
	if (SpeechPlayStart >= 0.f && SpeechComp && SpeechComp->IsPlaying())
	{
		bTalking = true;
		const float T = PhaseTime - SpeechPlayStart + MouthLead;
		const float Voice = SoundSeconds(PendletonSound, 0.f);
		if (FMath::Abs(Voice - Scene5Lipsync::Seconds) < 0.5f)
		{
			Scene5Local::Lips(T, Open, Wide);
			bBaked = true;
		}
		else
		{
			// Re-voiced without a new bake (tools/script/scene5_lipsync.py): a plain syllable flap while he talks.
			const float S = 0.5f + 0.5f * FMath::Sin(T * 2.f * PI * 3.7f) * FMath::Sin(T * 2.f * PI * 0.9f + 1.f);
			Open = T < Voice - 0.3f ? FMath::Clamp(S * 1.2f, 0.f, 1.f) : 0.f;
		}
	}
	// His head carries the line (the baked mouth curve is the voice's loudness): no mouth since Marwan's QA, Nov 17.
	if (ACastMember* const P = GetPendletonCast())
	{
		P->SetTalkHead(bTalking ? TalkHead : 0.f, bTalking && bBaked ? Open : -1.f);
	}
	if (!PendletonMouth || !bMouthPlaced)
	{
		return;
	}
	// The mesh faces +Y: the body's forward X is the mesh's +Y, its left Y the mesh's -X.
	const FVector Off(-MouthOffset.Y, MouthOffset.X, MouthOffset.Z - Open * MouthOpenCm * 0.5f);
	const float W = FMath::Max(MouthSize.X + (Wide - 0.5f) * 2.f * MouthWideCm * (0.4f + 0.6f * Open), 1.f);
	const float H = MouthSize.Z + Open * MouthOpenCm;
	const FTransform PieceCS(FQuat::Identity, MouthHeadRef.GetLocation() + Off, FVector(W, MouthSize.Y, H) / 100.f);
	PendletonMouth->SetRelativeTransform(PieceCS.GetRelativeTransform(MouthHeadRef));
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

EScene5Look AScene5Auditorium::WatchLook(int32 I, float& OutYaw, float& OutLeft) const
{
	using namespace Scene5Local;
	const FScene5Watch& W = Watches[I];
	const float Away = FMath::Max(W.AwaySeconds, 1.f), Turn = FMath::Max(W.TurnSeconds, 0.2f), Check = FMath::Max(W.CheckSeconds, 0.5f);
	const float Cycle = Away + Turn + Check + Turn;
	float T = FMath::Fmod(SneakTime + FMath::Fmod(W.Phase, 1.f) * Cycle, Cycle);
	if (T < 0.f) { T += Cycle; }
	if (T < Away)
	{
		OutYaw = W.AwayYaw;
		OutLeft = Away - T;
		return EScene5Look::Film;
	}
	T -= Away;
	if (T < Turn)
	{
		OutYaw = LerpYaw(W.AwayYaw, W.YawA, Smooth(T / Turn));
		OutLeft = Turn - T;
		return EScene5Look::TurnIn;
	}
	T -= Turn;
	if (T < Check)
	{
		const float S = 0.5f - 0.5f * FMath::Cos(2.f * PI * T / Check);	// A to B and back to A
		OutYaw = LerpYaw(W.YawA, W.YawB, S);
		OutLeft = Check - T;
		return EScene5Look::Check;
	}
	T -= Check;
	OutYaw = LerpYaw(W.YawA, W.AwayYaw, Smooth(T / Turn));
	OutLeft = Turn - T;
	return EScene5Look::TurnOut;
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
		float Yaw = 0.f, Left = 0.f;
		const EScene5Look Look = WatchLook(i, Yaw, Left);
		const float Turn = FMath::Max(W.TurnSeconds, 0.2f);
		// How far the torch is raised: down on the film, up through the turn in, held, down through the turn out.
		float Up = 0.f;
		switch (Look)
		{
		case EScene5Look::Film: Up = 0.f; break;
		case EScene5Look::TurnIn: Up = 1.f - Left / Turn; break;
		case EScene5Look::Check: Up = 1.f; break;
		case EScene5Look::TurnOut: Up = Left / Turn; break;
		}
		Up = Scene5Local::Smooth(Up);
		const FVector Fwd = FRotator(0.f, Yaw, 0.f).Vector();
		const FVector Feet = W.Who->GetActorLocation();
		T->SetWorldLocationAndRotation(FVector(Feet.X, Feet.Y, TorchHeight) + Fwd * 30.f,
			FRotator(FMath::Lerp(-62.f, -9.f, Up), Yaw, 0.f));
		FTakeShapes::SetCueCone(T, FMath::Lerp(TorchHalfDegrees * 0.7f, TorchHalfDegrees, Up), 0.55f);
		FTakeShapes::SetCueLight(T, TorchCandelas * FMath::Lerp(LoweredTorchShare, 1.f, Up), Scene5Local::TorchWhite, 1.5f * Up);
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
		float Yaw = 0.f, Left = 0.f;
		const EScene5Look Look = WatchLook(i, Yaw, Left);
		const bool bRaised = Look == EScene5Look::Check
			|| (Look == EScene5Look::TurnIn && Left < 0.5f * FMath::Max(Watches[i].TurnSeconds, 0.2f));
		if (!bRaised)
		{
			continue;	// on the film: blind
		}
		FVector D = At - T->GetComponentLocation();
		D.Z = 0.f;
		const float Dist = D.Size();
		if (Dist > TorchRange || Dist < 1.f)
		{
			continue;
		}
		const FVector Fwd = FRotator(0.f, Yaw, 0.f).Vector();
		const float Cos = FVector::DotProduct(D / Dist, Fwd);
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

	if (Phase == EScene5Beat::Speech || Phase == EScene5Beat::Rise)
	{
		Kit.Text(EType::Label, TEXT("SPACE  skip"), Kit.W - Kit.Px(48.f), Kit.RowB2(), FTakeHudKit::A(Ash, 0.8f), ETakeAlign::Right);
		return;
	}
	if (Phase == EScene5Beat::Done)
	{
		return;
	}

	// The goal, top centre on the HUD's plates (QA Nov 16: no bars in play, nothing as plain text).
	ScriptHudWidgets::Objective(Kit, TEXT("Slip along the south wall to the oak door by the stage"));
	ScriptHudWidgets::SubObjective(Kit, TEXT("Move while the torches face the film. Freeze in a raised beam."),
		ScriptHudWidgets::ETone::Action);

	// The watchers, in the bottom bar: each one's look and how long it lasts.
	const float Cy = Kit.PicBottom() + Kit.Bar * 0.38f;
	const int32 N = FMath::Min(Watches.Num(), Torches.Num());
	const float Col = Kit.Px(420.f);
	for (int32 i = 0; i < N; ++i)
	{
		float Yaw = 0.f, Left = 0.f;
		const EScene5Look Look = WatchLook(i, Yaw, Left);
		const float X = Kit.CX() + (static_cast<float>(i) - 0.5f * static_cast<float>(N - 1)) * Col;
		FString Word;
		FLinearColor C = FTakeHudKit::A(Ash, 0.85f);
		if (Look == EScene5Look::Film && Left > TurnWarnSeconds)
		{
			Word = FString::Printf(TEXT("%s  on the film  %d"), *Watches[i].Speaker, FMath::CeilToInt(Left));
		}
		else if (Look == EScene5Look::Film || Look == EScene5Look::TurnIn)
		{
			Word = FString::Printf(TEXT("%s  TURNING"), *Watches[i].Speaker);
			C = Julian;
		}
		else if (Look == EScene5Look::Check)
		{
			Word = FString::Printf(TEXT("%s  CHECKING THE AISLE"), *Watches[i].Speaker);
			C = Danger;
		}
		else
		{
			Word = FString::Printf(TEXT("%s  back to the film"), *Watches[i].Speaker);
		}
		Kit.Diamond(X - Kit.Measure(EType::Label, Word) * 0.5f - Kit.Px(16.f), Cy, Kit.Px(9.f), C);
		Kit.Text(EType::Label, Word, X, Cy, C, ETakeAlign::Centre);
	}
	const float Cy2 = Cy + Kit.Px(34.f);
	Kit.Text(EType::Label, bLitNow ? TEXT("IN THE TORCHLIGHT: HOLD STILL") : TEXT("in the dark"), Kit.CX(), Cy2,
		bLitNow ? Danger : FTakeHudKit::A(Ash, 0.7f), ETakeAlign::Centre);

	// SEEN fills while he moves in a raised torch.
	if (Seen > 0.01f)
	{
		const float Bw = Kit.Px(220.f), Bh = Kit.Px(8.f);
		const float Bx = Kit.CX() - Bw * 0.5f, By = Cy2 + Kit.Px(22.f);
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
