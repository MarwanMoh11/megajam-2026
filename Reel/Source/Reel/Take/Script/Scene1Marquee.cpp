// See Scene1Marquee.h (the beats). Owner: s1-marquee.

#include "Script/Scene1Marquee.h"

#include "CanvasItem.h"
#include "CastMember.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Julian.h"
#include "Kismet/GameplayStatics.h"
#include "QueueLine.h"
#include "ScannerWatcher.h"
#include "Sound/SoundBase.h"
#include "TakeDirector.h"
#include "TakePlayerController.h"
#include "TakeShapes.h"

DEFINE_LOG_CATEGORY_STATIC(LogScene1, Log, All);

// Unity builds merge .cpp files: file-local helpers live in a uniquely named namespace.
namespace Scene1MarqueeLocal
{
	/** The live view eases onto Point (the player's mouse takes it back at once), on Vance only. */
	void PanTo(const AActor* Context, const FVector& Point, float Seconds)
	{
		const ATakeDirector* const Director = ATakeDirector::Get(Context);
		ATakePlayerController* const PC = Director ? Director->PC.Get() : nullptr;
		if (PC && !PC->IsInFlight() && PC->GetCurrentCharacter() && PC->GetCurrentCharacter() == Director->Julian.Get())
		{
			PC->StartLookPan(Point, Seconds);
		}
	}

	float YawTo(const FVector& From, const FVector& To, float Fallback)
	{
		const FVector D = To - From;
		return D.SizeSquared2D() < 1.f ? Fallback : static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X)));
	}

	TSoftObjectPtr<USoundBase> Snd(const TCHAR* Path)
	{
		return TSoftObjectPtr<USoundBase>(FSoftObjectPath(Path));
	}

	constexpr int32 CableBeads = 18;
	const FVector BoltedAt(4.f, 0.f, 105.f);	// the scanner on the podium's top (PodiumRoot space)
	const FRotator BoltedTilt(-18.f, 0.f, 0.f);	// its face tilted toward the guest (+X)
}

AScene1Marquee::AScene1Marquee()
{
	using namespace Scene1MarqueeLocal;
	PrimaryActorTick.bCanEverTick = true;	// PalmAccepted and StageAtPlate run while Scene 2 is the active scene
	PrimaryActorTick.bStartWithTickEnabled = true;

	SceneNumber = 1;
	Pov = EScriptPov::Vance;
	Slug = FText::FromString(TEXT("EXT. THE GRAND LUMIERE THEATER - NIGHT"));
	bPlayerWalks = true;

	PodiumRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PodiumRoot"));
	RootComponent = PodiumRoot;
	PodiumRoot->SetMobility(EComponentMobility::Movable);

	Podium = FTakeShapes::CreateShape(this, TEXT("Podium"), ETakeShape::Cube, PodiumRoot);
	Podium->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	Podium->SetRelativeScale3D(FVector(0.42f, 0.5f, 1.f));
	PodiumCap = FTakeShapes::CreateShape(this, TEXT("PodiumCap"), ETakeShape::Cube, PodiumRoot);
	PodiumCap->SetRelativeLocation(FVector(0.f, 0.f, 102.5f));
	PodiumCap->SetRelativeScale3D(FVector(0.5f, 0.58f, 0.05f));

	ScannerRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ScannerRoot"));
	ScannerRoot->SetupAttachment(PodiumRoot);
	ScannerRoot->SetMobility(EComponentMobility::Movable);
	ScannerRoot->SetRelativeLocationAndRotation(BoltedAt, BoltedTilt);

	ScannerBody = FTakeShapes::CreateShape(this, TEXT("ScannerBody"), ETakeShape::Cube, ScannerRoot);
	ScannerBody->SetRelativeLocation(FVector(0.f, 0.f, 3.f));
	ScannerBody->SetRelativeScale3D(FVector(0.34f, 0.26f, 0.06f));
	Glass = FTakeShapes::CreateShape(this, TEXT("Glass"), ETakeShape::Cube, ScannerRoot, FVector::ZeroVector, FVector::OneVector, FRotator::ZeroRotator, false);
	Glass->SetRelativeLocation(FVector(0.f, 0.f, 6.3f));
	Glass->SetRelativeScale3D(FVector(0.26f, 0.18f, 0.012f));
	Laser = FTakeShapes::CreateShape(this, TEXT("Laser"), ETakeShape::Cube, ScannerRoot, FVector::ZeroVector, FVector::OneVector, FRotator::ZeroRotator, false);
	Laser->SetRelativeLocation(FVector(0.f, 0.f, 7.2f));
	Laser->SetRelativeScale3D(FVector(0.012f, 0.19f, 0.01f));
	PlateLight = FTakeShapes::CreateCuePoint(this, TEXT("PlateLight"), ScannerRoot, FVector(0.f, 0.f, 22.f), 260.f);

	// The coiled cable: beads on a helix from the scanner's back, down the podium's -X side, to the pavement.
	for (int32 Index = 0; Index < CableBeads; ++Index)
	{
		const FName Name(*FString::Printf(TEXT("Cable%02d"), Index));
		UStaticMeshComponent* const Bead = FTakeShapes::CreateShape(this, Name, ETakeShape::Sphere, PodiumRoot, FVector::ZeroVector, FVector::OneVector, FRotator::ZeroRotator, false);
		const float T = static_cast<float>(Index) / static_cast<float>(CableBeads - 1);
		const float Angle = T * 7.f * UE_TWO_PI;
		Bead->SetRelativeLocation(FVector(-25.f + 4.f * FMath::Cos(Angle), 12.f + 4.f * FMath::Sin(Angle), FMath::Lerp(100.f, 6.f, T)));
		Bead->SetRelativeScale3D(FVector(0.04f));
		Cable.Add(Bead);
	}

	// Everything moves: the scanner rides on hands, the lights and lamps change in play.
	for (UStaticMeshComponent* const Comp : { Podium.Get(), PodiumCap.Get(), ScannerBody.Get(), Glass.Get(), Laser.Get() })
	{
		Comp->SetMobility(EComponentMobility::Movable);
	}
	for (UStaticMeshComponent* const Bead : Cable)
	{
		Bead->SetMobility(EComponentMobility::Movable);
	}

	JazzAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("JazzAudio"));
	JazzAudio->SetupAttachment(PodiumRoot);
	JazzAudio->bAutoActivate = false;
	RainAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("RainAudio"));
	RainAudio->SetupAttachment(PodiumRoot);
	RainAudio->bAutoActivate = false;

	ManagerVoice = Snd(TEXT("/Game/Take/Audio/Script/S1/vo_s1_manager_whisper.vo_s1_manager_whisper"));
	DirectiveVoice = Snd(TEXT("/Game/Take/Audio/Script/S1/vo_s1_bouncer_directives.vo_s1_bouncer_directives"));
	HandVoice = Snd(TEXT("/Game/Take/Audio/Script/S1/vo_s1_bouncer_hand.vo_s1_bouncer_hand"));
	EnjoyVoice = Snd(TEXT("/Game/Take/Audio/Script/S1/vo_s1_bouncer_enjoy.vo_s1_bouncer_enjoy"));
	BeepSound = Snd(TEXT("/Game/Take/Audio/Script/S1/sfx_s1_beep.sfx_s1_beep"));
	LaserSound = Snd(TEXT("/Game/Take/Audio/Script/S1/sfx_s1_laser.sfx_s1_laser"));
	RainSound = Snd(TEXT("/Game/Take/Audio/Script/S1/amb_s1_rain.amb_s1_rain"));
	ChimeSound = Snd(TEXT("/Game/Take/Audio/sfx_chime.sfx_chime"));
	BoltSound = Snd(TEXT("/Game/Take/Audio/sfx_thud.sfx_thud"));
	JazzSound = Snd(TEXT("/Game/Take/Audio/mus_bed.mus_bed"));
}

void AScene1Marquee::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (PodiumMaterial) { Podium->SetMaterial(0, PodiumMaterial); }
	else { FTakeShapes::SetColour(Podium, FLinearColor(0.07f, 0.035f, 0.02f)); }
	FTakeShapes::SetColour(PodiumCap, FLinearColor(0.55f, 0.4f, 0.15f));
	if (SteelMaterial) { ScannerBody->SetMaterial(0, SteelMaterial); }
	else { FTakeShapes::SetColour(ScannerBody, FLinearColor(0.5f, 0.52f, 0.55f)); }
	for (UStaticMeshComponent* const Bead : Cable)
	{
		FTakeShapes::SetColour(Bead, FLinearColor(0.02f, 0.02f, 0.022f));
	}
	FTakeShapes::SetLamp(Glass, IdleGlow, 0);
	FTakeShapes::SetLamp(Laser, LaserGlow, 0);
	Laser->SetVisibility(false);
}

void AScene1Marquee::BeginPlay()
{
	Super::BeginPlay();
	// Off the set until a scene asks for it (Scene 1's bolt, or Scene 2's StageAtPlate).
	PutScanner(EScannerAt::Hidden);
	SetPlateGlow(IdleGlow, 0.f);
}

// ---------------------------------------------------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------------------------------------------------

double AScene1Marquee::Now() const
{
	const UWorld* const World = GetWorld();
	return World ? World->GetRealTimeSeconds() : 0.0;
}

AQueueLine* AScene1Marquee::GetQueue() const { return AQueueLine::Get(this); }

AScannerWatcher* AScene1Marquee::GetScanner() const
{
	for (TActorIterator<AScannerWatcher> It(GetWorld()); It; ++It) { return *It; }
	return nullptr;
}

AJulian* AScene1Marquee::GetJulian() const
{
	const ATakeDirector* const Director = ATakeDirector::Get(this);
	return Director ? Director->Julian.Get() : nullptr;
}

FVector AScene1Marquee::GetPlatePoint() const
{
	return GetActorTransform().TransformPosition(Scene1MarqueeLocal::BoltedAt + FVector(0.f, 0.f, 7.f));
}

void AScene1Marquee::EnterBeat(EScene1Beat NewBeat)
{
	Beat = NewBeat;
	BeatAt = Now();
	UE_LOG(LogScene1, Log, TEXT("S1 beat %d"), static_cast<int32>(NewBeat));
}

float AScene1Marquee::SpeakLine(const FString& Speaker, const FString& Line, const TSoftObjectPtr<USoundBase>& Voice)
{
	USoundBase* const Sound = Voice.LoadSynchronous();
	const float Seconds = Sound ? FMath::Max(0.8f, Sound->GetDuration() + 0.2f) : 0.9f + 0.055f * static_cast<float>(Line.Len());
	Say(Speaker, Line, Sound);
	LineEndsAt = Now() + Seconds;
	if (AQueueLine* const Queue = GetQueue())
	{
		if (Speaker == TEXT("BOUNCER")) { Queue->DoormanSpeaks(Seconds); }
	}
	return Seconds;
}

void AScene1Marquee::PlayAt(const TSoftObjectPtr<USoundBase>& Sound, const FVector& Where, float Volume)
{
	if (USoundBase* const Loaded = Sound.LoadSynchronous())
	{
		UGameplayStatics::PlaySoundAtLocation(this, Loaded, Where, Volume);
	}
}

void AScene1Marquee::SetPlateGlow(const FLinearColor& Glow, float Candelas)
{
	FTakeShapes::SetLamp(Glass, Glow, 0);
	FLinearColor LightColour = Glow;
	LightColour.A = 1.f;
	const float Peak = FMath::Max3(LightColour.R, LightColour.G, LightColour.B);
	if (Peak > 0.f) { LightColour = LightColour * (1.f / Peak); }
	FTakeShapes::SetCueLight(PlateLight, ScannerAt == EScannerAt::Hidden ? 0.f : Candelas, LightColour);
}

void AScene1Marquee::PutScanner(EScannerAt Where)
{
	using namespace Scene1MarqueeLocal;
	ScannerAt = Where;
	const bool bShow = Where != EScannerAt::Hidden;
	ScannerRoot->SetVisibility(bShow, true);
	Laser->SetVisibility(false);
	for (UStaticMeshComponent* const Bead : Cable) { Bead->SetVisibility(Where == EScannerAt::Bolted); }

	ACastMember* Holder = nullptr;
	if (Where == EScannerAt::Manager) { Holder = Manager; }
	else if (Where == EScannerAt::Bouncer) { if (AQueueLine* const Queue = GetQueue()) { Holder = Queue->GetDoorman(); } }

	if (Holder && Holder->Body)
	{
		ScannerRoot->AttachToComponent(Holder->Body, FAttachmentTransformRules(EAttachmentRule::SnapToTarget,
			EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false), HandSocket);
		ScannerRoot->SetRelativeLocationAndRotation(FVector(0.f, 6.f, 0.f), FRotator(0.f, 0.f, 90.f));
	}
	else
	{
		ScannerRoot->AttachToComponent(PodiumRoot, FAttachmentTransformRules::KeepRelativeTransform);
		ScannerRoot->SetRelativeLocationAndRotation(BoltedAt, BoltedTilt);
		ScannerRoot->SetRelativeScale3D(FVector::OneVector);
	}
	FTakeShapes::SetCueLight(PlateLight, 0.f, FLinearColor::White);
}

void AScene1Marquee::HideManager()
{
	if (Manager)
	{
		if (ScannerAt == EScannerAt::Manager) { PutScanner(EScannerAt::Hidden); }
		Manager->Destroy();
		Manager = nullptr;
	}
}

void AScene1Marquee::StartAmbience(bool bOn)
{
	struct FBed { UAudioComponent* Comp; const TSoftObjectPtr<USoundBase>* Sound; float Volume; bool bMuffle; };
	const FBed Beds[] = { { JazzAudio, &JazzSound, JazzVolume, true }, { RainAudio, &RainSound, RainVolume, false } };
	for (const FBed& Bed : Beds)
	{
		if (!Bed.Comp) { continue; }
		if (!bOn)
		{
			if (Bed.Comp->IsPlaying()) { Bed.Comp->FadeOut(1.f, 0.f); }
			continue;
		}
		USoundBase* const Sound = Bed.Sound->LoadSynchronous();
		if (!Sound || Bed.Volume <= 0.f) { continue; }
		Bed.Comp->SetSound(Sound);
		Bed.Comp->bAllowSpatialization = false;
		Bed.Comp->SetLowPassFilterEnabled(Bed.bMuffle);
		if (Bed.bMuffle) { Bed.Comp->SetLowPassFilterFrequency(JazzLowPassHz); }
		Bed.Comp->FadeIn(0.8f, Bed.Volume);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Reset and staging
// ---------------------------------------------------------------------------------------------------------------------

bool AScene1Marquee::DoReset(int32 Ahead)
{
	AQueueLine* const Queue = GetQueue();
	if (!Queue || !Queue->ScriptReset(Ahead))
	{
		return false;
	}
	HideManager();
	PutScanner(EScannerAt::Hidden);
	SetPlateGlow(IdleGlow, 0.f);
	if (AScannerWatcher* const Scanner = GetScanner()) { Scanner->ScriptOpen(false); }
	ScanGuest = INDEX_NONE;
	bScanBeeped = false;
	GreenUntil = 0.0;
	CardAt = -1.0;
	VoidAt = -1.0;
	PalmAt = -1.0;
	PalmStep = 0;
	bPalmDone = false;
	ConsumeFlag(PalmThroughFlag());
	return true;
}

void AScene1Marquee::ResetScene()
{
	bNeedReset = true;
	bNeedStage = false;
	Beat = EScene1Beat::Setup;
	if (DoReset(GuestsAhead))
	{
		bNeedReset = false;	// else on the first TickScene (the director's Julian not ready yet)
	}
}

void AScene1Marquee::StageAtPlate()
{
	bNeedStage = true;
	Beat = EScene1Beat::Done;
}

void AScene1Marquee::BeginScene()
{
	Super::BeginScene();
	if (!bNeedReset) { EnterBeat(EScene1Beat::Queue); CardAt = Now() + 0.6; }
	StartAmbience(true);
}

void AScene1Marquee::EndScene()
{
	Super::EndScene();
	StartAmbience(false);
	if (AQueueLine* const Queue = GetQueue()) { Queue->bScriptHoldFront = false; }
}

// ---------------------------------------------------------------------------------------------------------------------
// The beats
// ---------------------------------------------------------------------------------------------------------------------

void AScene1Marquee::TickScene(float DeltaSeconds)
{
	using namespace Scene1MarqueeLocal;
	if (bNeedReset)
	{
		if (!DoReset(GuestsAhead)) { return; }
		bNeedReset = false;
		EnterBeat(EScene1Beat::Queue);
		CardAt = Now() + 0.6;
	}
	AQueueLine* const Queue = GetQueue();
	if (!Queue) { return; }
	Queue->ScriptStep(DeltaSeconds);
	ACastMember* const Bouncer = Queue->GetDoorman();
	const double T = Now();
	const double Age = T - BeatAt;

	// The floor manager, back inside: gone.
	if (Manager && Beat > EScene1Beat::Bolt && (!Manager->IsWalking() || Beat >= EScene1Beat::Line))
	{
		HideManager();
	}

	switch (Beat)
	{
	case EScene1Beat::Queue:
		Queue->bScriptHoldFront = true;
		if (Age >= ManagerDelay && GetWorld())
		{
			const FTransform At(FRotator(0.f, YawTo(ManagerFrom, ManagerTo, 180.f), 0.f), ManagerFrom);
			Manager = GetWorld()->SpawnActorDeferred<ACastMember>(ACastMember::StaticClass(), At, this, nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (Manager)
			{
				Manager->Kit = ECastKit::GuestTux;
				Manager->Tint = ManagerTint;
				Manager->FinishSpawning(At);
				Manager->WalkTo(ManagerTo, ManagerSpeed);
				PutScanner(EScannerAt::Manager);
			}
			EnterBeat(EScene1Beat::Manager);
		}
		break;

	case EScene1Beat::Manager:
	{
		// Blocked on the way out (the doors, a rope): glide him along the line by hand, so the scene never waits on a path.
		const float WalkLen = FVector::Dist2D(ManagerFrom, ManagerTo);
		if (Manager && Manager->IsWalking() && WalkLen > 1.f)
		{
			const float Along = FMath::Min(WalkLen, ManagerSpeed * float(Age));
			const FVector Want = ManagerFrom + (ManagerTo - ManagerFrom) * (Along / WalkLen);
			const FVector M = Manager->GetActorLocation();
			if (FVector::Dist2D(M, Want) > 60.f) { Manager->SetActorLocation(FVector(Want.X, Want.Y, M.Z)); }
		}
		if (!Manager || (!Manager->IsWalking() && Age > 0.3) || Age > WalkLen / FMath::Max(ManagerSpeed, 1.f) + 1.0)
		{
			if (Manager && Bouncer)
			{
				const FVector M = Manager->GetActorLocation();
				const FVector B = Bouncer->GetActorLocation();
				Manager->FaceYaw(YawTo(M, B, 0.f));
				Manager->PlayGestureAt(TEXT("Talk"), B + FVector(0.f, 0.f, 160.f));
				Bouncer->FaceYaw(YawTo(B, M, 0.f));
				Bouncer->LookAtPoint(M + FVector(0.f, 0.f, 160.f), 3.f);
				Bouncer->PlayGesture(TEXT("Listen"));
			}
			if (Bouncer) { PanTo(this, Bouncer->GetActorLocation() + FVector(0.f, 0.f, 150.f), 0.8f); }
			SpeakLine(TEXT("FLOOR MANAGER"), ManagerLine, ManagerVoice);
			EnterBeat(EScene1Beat::Whisper);
		}
		break;
	}

	case EScene1Beat::Whisper:
		if (T >= LineEndsAt)
		{
			if (Manager && Bouncer) { Manager->PlayGestureAt(TEXT("Use"), Bouncer->GetActorLocation() + FVector(0.f, 0.f, 100.f)); }
			if (Bouncer) { Bouncer->PlayGesture(TEXT("Nod")); }
			EnterBeat(EScene1Beat::Bolt);
		}
		break;

	case EScene1Beat::Bolt:
		if (Age >= HandoverSeconds && ScannerAt != EScannerAt::Bouncer && ScannerAt != EScannerAt::Bolted)
		{
			PutScanner(EScannerAt::Bouncer);
			if (Manager) { Manager->WalkTo(ManagerFrom, ManagerSpeed); }
			if (Bouncer)
			{
				Bouncer->FaceYaw(YawTo(Bouncer->GetActorLocation(), GetActorLocation(), Queue->DoormanYaw));
				Bouncer->PlayGestureAt(TEXT("Use"), GetPlatePoint());
			}
		}
		if (Age >= HandoverSeconds + BoltSeconds && ScannerAt == EScannerAt::Bouncer)
		{
			PutScanner(EScannerAt::Bolted);
			PlayAt(BoltSound, GetPlatePoint());
			PanTo(this, GetPlatePoint(), 0.5f);
			SetPlateGlow(IdleGlow, 60.f);
			if (Bouncer)
			{
				Bouncer->FaceYaw(Queue->DoormanYaw);
				Bouncer->PlayGesture(TEXT("Talk"));
			}
			SpeakLine(TEXT("BOUNCER"), DirectiveLine, DirectiveVoice);
			VoidAt = Now() + 0.45 * (LineEndsAt - Now());	// about "Magnetic cards are void"
			EnterBeat(EScene1Beat::Directive);
		}
		break;

	case EScene1Beat::Directive:
		if (T >= LineEndsAt)
		{
			Queue->bScriptHoldFront = false;
			EnterBeat(EScene1Beat::Line);
		}
		break;

	case EScene1Beat::Line:
		StepLineBeats();
		if (Queue->IsJuliansTurn())
		{
			Queue->bScriptOpenFront = true;
			PanTo(this, GetPlatePoint(), 0.6f);
			EnterBeat(EScene1Beat::Turn);
		}
		break;

	case EScene1Beat::Turn:
		StepLineBeats();
		if (const AJulian* const Julian = GetJulian())
		{
			if (FVector::Dist2D(Julian->GetFeetLocation(), Queue->GetScanPointWorld()) <= StepUpRadius)
			{
				if (Bouncer)
				{
					Bouncer->FaceYaw(YawTo(Bouncer->GetActorLocation(), Julian->GetActorLocation(), Queue->DoormanYaw));
					Bouncer->PlayGestureAt(TEXT("Point"), GetPlatePoint());
				}
				SpeakLine(TEXT("BOUNCER"), HandLine, HandVoice);
				EnterBeat(EScene1Beat::Hand);
			}
		}
		break;

	case EScene1Beat::Hand:
		if (T >= LineEndsAt + CutAfterLine)
		{
			EnterBeat(EScene1Beat::Done);
			FinishScene();
		}
		break;

	default:
		break;
	}
}

void AScene1Marquee::StepLineBeats()
{
	AQueueLine* const Queue = GetQueue();
	if (!Queue) { return; }
	int32 Index = INDEX_NONE;
	float Left = 0.f;
	ACastMember* const Guest = Queue->GetScanningGuest(Index, Left);
	if (!Guest)
	{
		ScanGuest = INDEX_NONE;
		return;
	}
	if (Index != ScanGuest)
	{
		ScanGuest = Index;
		bScanBeeped = false;
		Guest->FaceYaw(Scene1MarqueeLocal::YawTo(Guest->GetActorLocation(), GetPlatePoint(), 0.f));
		Guest->PlayGestureAt(TEXT("Use"), GetPlatePoint());
	}
	if (!bScanBeeped && Left <= Queue->ScanSeconds * BeepAtShare)
	{
		bScanBeeped = true;
		PlayAt(BeepSound, GetPlatePoint());
		SetPlateGlow(GreenGlow, 160.f);
		GreenUntil = Now() + GreenSeconds;
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Own tick: the plate's green fading back, Scene 2's staging and the palm accepted
// ---------------------------------------------------------------------------------------------------------------------

void AScene1Marquee::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GreenUntil > 0.0 && Now() >= GreenUntil && PalmStep == 0)
	{
		GreenUntil = 0.0;
		SetPlateGlow(IdleGlow, 60.f);
	}
	if (bNeedStage && DoReset(0))
	{
		bNeedStage = false;
		Beat = EScene1Beat::Done;
		AQueueLine* const Queue = GetQueue();
		AJulian* const Julian = GetJulian();
		if (Queue) { Queue->bScriptOpenFront = true; }
		if (Queue && Julian)
		{
			// Vance at the plate, facing it.
			const FVector Scan = Queue->GetScanPointWorld();
			const FVector FeetToCentre = Julian->GetActorLocation() - Julian->GetFeetLocation();
			const float Yaw = Scene1MarqueeLocal::YawTo(Scan, GetPlatePoint(), 0.f);
			Julian->SetActorLocationAndRotation(Scan + FeetToCentre, FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			if (UCharacterMovementComponent* const Move = Julian->GetCharacterMovement()) { Move->StopMovementImmediately(); }
			Julian->SetLook(Yaw, Julian->StartLookPitch);
			Julian->SnapFollowCamera();
		}
		PutScanner(EScannerAt::Bolted);
		SetPlateGlow(IdleGlow, 60.f);
		UE_LOG(LogScene1, Log, TEXT("S1 staged at the plate (Scene 2)"));
	}
	StepPalm();
}

float AScene1Marquee::PalmSeconds() const
{
	const USoundBase* const Sound = EnjoyVoice.Get();
	const float Line = Sound ? Sound->GetDuration() + 0.2f : 0.9f + 0.055f * static_cast<float>(EnjoyLine.Len());
	return LaserSeconds + 0.35f + Line;
}

void AScene1Marquee::PalmAccepted()
{
	if (PalmStep > 0 && PalmStep < 6)
	{
		return;
	}
	if (ScannerAt != EScannerAt::Bolted) { PutScanner(EScannerAt::Bolted); }
	PalmAt = Now();
	PalmStep = 1;
	bPalmDone = false;
	GreenUntil = 0.0;
	Laser->SetVisibility(true);
	SetPlateGlow(LaserGlow * 0.25f, 120.f);
	PlayAt(LaserSound, GetPlatePoint());
	UE_LOG(LogScene1, Log, TEXT("S1 palm accepted: the sweep"));
}

void AScene1Marquee::StepPalm()
{
	if (PalmStep <= 0 || PalmStep >= 6)
	{
		return;
	}
	AQueueLine* const Queue = GetQueue();
	ACastMember* const Bouncer = Queue ? Queue->GetDoorman() : nullptr;
	const double Age = Now() - PalmAt;
	if (PalmStep == 1)
	{
		// The red line sweeps across the glass and back.
		const float Phase = FMath::Clamp(static_cast<float>(Age) / LaserSeconds, 0.f, 1.f);
		const float X = -11.f * FMath::Cos(Phase * UE_TWO_PI);
		Laser->SetRelativeLocation(FVector(X, 0.f, 7.2f));
		if (Age >= LaserSeconds)
		{
			Laser->SetVisibility(false);
			PlayAt(ChimeSound, GetPlatePoint());
			SetPlateGlow(GreenGlow, 200.f);
			if (AScannerWatcher* const Scanner = GetScanner()) { Scanner->ScriptOpen(true); }
			if (Queue)
			{
				Queue->bScriptOpenFront = true;
				if (Bouncer) { Bouncer->WalkTo(Queue->DoormanPoint + AsideOffset, 120.f); }
			}
			PalmStep = 2;
		}
	}
	else if (PalmStep == 2 && Age >= LaserSeconds + 0.35f)
	{
		if (Bouncer)
		{
			const AScannerWatcher* const Scanner = GetScanner();
			const FVector Gate = Scanner ? Scanner->GetActorLocation() + Scanner->GetActorForwardVector() * 250.f : GetActorLocation();
			Bouncer->PlayGestureAt(TEXT("Wave"), Gate);
		}
		SpeakLine(TEXT("BOUNCER"), EnjoyLine, EnjoyVoice);
		PalmStep = 3;
	}
	else if (PalmStep == 3 && Now() >= LineEndsAt - 0.6)
	{
		// He nods and glides inside: the walk-in through the gate (the queue's WayIn) and the doors.
		PalmStep = 4;
		WalkInAt = Now();
		WalkLeg = 0;
		if (const AJulian* const Julian = GetJulian()) { LastWalkFeet = Julian->GetFeetLocation(); }
	}
	else if (PalmStep == 4)
	{
		AJulian* const Julian = GetJulian();
		const TArray<FVector> Empty;
		const TArray<FVector>& Way = Queue ? Queue->WayIn : Empty;
		if (!Julian || Way.Num() == 0)
		{
			PalmStep = 5;
		}
		else
		{
			const FVector Target = Queue->GetActorTransform().TransformPosition(Way[FMath::Min(WalkLeg, Way.Num() - 1)]);
			const FVector Feet = Julian->GetFeetLocation();
			if (FVector::Dist2D(Feet, Target) < 60.f)
			{
				++WalkLeg;
			}
			const bool bTimeUp = Now() - WalkInAt > WalkInMaxSeconds;
			if (WalkLeg >= Way.Num() || bTimeUp)
			{
				if (bTimeUp)
				{
					// Never stuck: put him inside the doors.
					const FVector Last = Queue->GetActorTransform().TransformPosition(Way.Last());
					Julian->SetActorLocation(Last + (Julian->GetActorLocation() - Feet), false, nullptr, ETeleportType::TeleportPhysics);
				}
				PalmStep = 5;
			}
			else
			{
				Julian->MoveToward(Target);
				// No controller moving him (another POV possessed): glide him there by hand.
				const float Dt = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;
				if (FVector::Dist2D(Feet, LastWalkFeet) < 0.2f && Now() - WalkInAt > 0.3)
				{
					const FVector Step = (Target - Feet).GetSafeNormal2D() * 160.f * Dt;
					Julian->SetActorLocationAndRotation(Julian->GetActorLocation() + Step, FRotator(0.f, Step.Rotation().Yaw, 0.f),
						false, nullptr, ETeleportType::TeleportPhysics);
				}
				LastWalkFeet = Julian->GetFeetLocation();
			}
		}
	}
	if (PalmStep == 5 && Now() >= LineEndsAt)
	{
		PalmStep = 6;
		bPalmDone = true;
		SetFlag(PalmThroughFlag());
		UE_LOG(LogScene1, Log, TEXT("S1 palm accepted: done, Vance is through (flag S1PalmThrough)"));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// HUD: the goal in the letterbox bar, the VIP card insert, the composure flicker
// ---------------------------------------------------------------------------------------------------------------------

void AScene1Marquee::DrawSceneHud(UCanvas* Canvas)
{
	if (!Canvas || !GEngine)
	{
		return;
	}
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const float S = H / 1080.f;
	const double T = Now();
	UFont* const Font = GEngine->GetMediumFont();
	const FLinearColor Gold(0.95f, 0.78f, 0.42f, 1.f);

	auto Box = [Canvas](float X, float Y, float BW, float BH, const FLinearColor& C)
	{
		FCanvasTileItem Tile(FVector2D(X, Y), FVector2D(BW, BH), C);
		Tile.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Tile);
	};
	auto Text = [Canvas, Font](const FString& Str, float X, float Y, float Scale, const FLinearColor& C, bool bCentre)
	{
		FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Str), Font, C);
		Item.Scale = FVector2D(Scale, Scale);
		Item.bCentreX = bCentre;
		Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.8f));
		Canvas->DrawItem(Item);
	};

	// The composure flicker: two dark frames as "void" lands.
	const double VoidAge = VoidAt > 0.0 ? T - VoidAt : -1.0;
	const bool bFlicker = (VoidAge >= 0.0 && VoidAge < 0.07) || (VoidAge >= 0.17 && VoidAge < 0.23);
	if (bFlicker)
	{
		Box(0.f, 0.f, W, H, FLinearColor(0.f, 0.f, 0.f, 0.5f));
	}

	// The goal, small, in the top letterbox bar.
	FString Goal;
	if (Beat >= EScene1Beat::Queue && Beat <= EScene1Beat::Line) { Goal = WaitGoal; }
	else if (Beat == EScene1Beat::Turn) { Goal = StepGoal; }
	if (!Goal.IsEmpty())
	{
		const float Bar = FMath::Max(0.f, 0.5f * (H - W / 2.39f));
		const float Y = Bar > 30.f * S ? Bar * 0.5f - 12.f * S : 24.f * S;
		Text(Goal.ToUpper(), W * 0.04f, Y, 1.1f * S, Gold, false);
	}

	// The forged card: up from the bottom right at the start, back with VOID on the directive, then it drops away.
	float CardAlpha = 0.f;
	float CardDrop = 0.f;
	bool bVoid = false;
	const double CardAge = CardAt > 0.0 ? T - CardAt : -1.0;
	if (CardAge >= 0.0 && CardAge < 4.0)
	{
		const float In = FMath::Clamp(static_cast<float>(CardAge) / 0.35f, 0.f, 1.f);
		const float Out = FMath::Clamp((4.f - static_cast<float>(CardAge)) / 0.4f, 0.f, 1.f);
		CardAlpha = FMath::Min(In, Out);
		CardDrop = (1.f - In) * 160.f * S + (1.f - Out) * 160.f * S;
	}
	if (VoidAge >= 0.0 && VoidAge < 2.2)
	{
		bVoid = true;
		CardAlpha = 1.f;
		CardDrop = VoidAge > 1.2 ? static_cast<float>(FMath::Square(VoidAge - 1.2) * 900.0) * S : 0.f;
	}
	if (CardAlpha > 0.f)
	{
		const float CW = 300.f * S, CH = 180.f * S;
		const float Jitter = bFlicker ? 9.f * S : 0.f;
		const float X = W - CW - 60.f * S + Jitter;
		const float Y = H - CH - 150.f * S + CardDrop;
		Box(X - 3.f * S, Y - 3.f * S, CW + 6.f * S, CH + 6.f * S, FLinearColor(0.8f, 0.62f, 0.3f, CardAlpha));
		Box(X, Y, CW, CH, FLinearColor(0.06f, 0.05f, 0.05f, CardAlpha));
		Box(X, Y + CH * 0.62f, CW, CH * 0.16f, FLinearColor(0.f, 0.f, 0.f, CardAlpha));	// the mag stripe
		Text(TEXT("V . I . P"), X + CW * 0.5f, Y + 18.f * S, 1.6f * S, FLinearColor(Gold.R, Gold.G, Gold.B, CardAlpha), true);
		Text(TEXT("THE GRAND LUMIERE  -  ADMIT ONE"), X + CW * 0.5f, Y + 70.f * S, 0.8f * S, FLinearColor(0.85f, 0.8f, 0.7f, CardAlpha), true);
		if (bVoid)
		{
			Text(TEXT("VOID"), X + CW * 0.5f, Y + CH * 0.3f, 2.6f * S, FLinearColor(0.9f, 0.08f, 0.06f, 0.9f), true);
		}
	}
}
