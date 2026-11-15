// See ScriptSequencer.h. Owner: the Spine session.

#include "Script/ScriptSequencer.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TakeDirector.h"
#include "TakePlayerController.h"
#include "TakeCharacter.h"
#include "TakeAudio.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogReelScript, Log, All);

static double ScriptNow() { return FPlatformTime::Seconds(); }

// ---- AScriptScene ----

AScriptScene::AScriptScene()
{
	PrimaryActorTick.bCanEverTick = false;	// the sequencer calls TickScene
}

void AScriptScene::BeginScene()
{
	bActive = true;
	UE_LOG(LogReelScript, Log, TEXT("SCRIPT begin scene %d %s"), SceneNumber, *GetName());
}

void AScriptScene::EndScene()
{
	bActive = false;
	UE_LOG(LogReelScript, Log, TEXT("SCRIPT end scene %d %s"), SceneNumber, *GetName());
}

AScriptSequencer* AScriptScene::GetSequencer() const { return AScriptSequencer::Get(this); }

ATakeCharacter* AScriptScene::GetPovCharacter() const
{
	const AScriptSequencer* S = GetSequencer();
	return S ? S->GetPovCharacter(S->GetPov()) : nullptr;
}

void AScriptScene::FinishScene()
{
	if (AScriptSequencer* S = GetSequencer()) { S->OnSceneFinished(this); }
}

void AScriptScene::FailScene(FName Reason)
{
	if (AScriptSequencer* S = GetSequencer()) { S->OnSceneFailed(this, Reason); }
}

void AScriptScene::CutTo(EScriptPov NewPov, AActor* Camera)
{
	if (AScriptSequencer* S = GetSequencer()) { S->OnCutTo(this, NewPov, Camera); }
}

void AScriptScene::Say(const FString& Speaker, const FString& Line, USoundBase* Sound)
{
	if (AScriptSequencer* S = GetSequencer()) { S->OnSay(Speaker, Line, Sound); }
}

void AScriptScene::SetFlag(FName Flag)
{
	if (AScriptSequencer* S = GetSequencer()) { S->Flags.Add(Flag); }
}

bool AScriptScene::HasFlag(FName Flag) const
{
	const AScriptSequencer* S = GetSequencer();
	return S && S->Flags.Contains(Flag);
}

bool AScriptScene::ConsumeFlag(FName Flag)
{
	AScriptSequencer* S = GetSequencer();
	return S && S->Flags.Remove(Flag) > 0;
}

// ---- AScriptSequencer ----

static int32 GScriptJump = 0;
static FAutoConsoleCommandWithWorldAndArgs GScriptJumpCmd(
	TEXT("Reel.Script.Jump"), TEXT("Reel.Script.Jump N: start scene N (1-8) now."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		const int32 N = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 1;
		GScriptJump = N;
		if (AScriptSequencer* S = AScriptSequencer::Get(World)) { S->JumpTo(N); }
	}));

static FAutoConsoleCommandWithWorldAndArgs GScriptFinishCmd(
	TEXT("Reel.Script.Finish"), TEXT("Reel.Script.Finish: the active scene finishes now (test the cut to the next)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		AScriptSequencer* S = AScriptSequencer::Get(World);
		if (S && S->GetActiveScene()) { S->GetActiveScene()->FinishScene(); }
	}));

static FAutoConsoleCommandWithWorldAndArgs GScriptFailCmd(
	TEXT("Reel.Script.Fail"), TEXT("Reel.Script.Fail: the active scene fails now (test the retake)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		AScriptSequencer* S = AScriptSequencer::Get(World);
		if (S && S->GetActiveScene()) { S->GetActiveScene()->FailScene(TEXT("Test")); }
	}));

AScriptSequencer::AScriptSequencer()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = false;
	Credits = {
		TEXT("THE DIRECTOR'S CUT"), TEXT(""),
		TEXT("VANCE"), TEXT("the con man in the white jacket"), TEXT(""),
		TEXT("NORA"), TEXT("the catwalk"), TEXT(""),
		TEXT("COLE"), TEXT("the muscle backstage"), TEXT(""),
		TEXT("MAX"), TEXT("the van"), TEXT(""),
		TEXT("ARTHUR PENDLETON"), TEXT("the man with the master print"), TEXT(""),
		TEXT("AND YOU"), TEXT("as the director"), TEXT(""),
		TEXT("MADE IN UNREAL ENGINE 5.9"), TEXT("for the Epic MegaJam 2026"), TEXT(""),
		TEXT("DIRECTED BY"), TEXT("Marwan Abudaif"), TEXT(""),
		TEXT("No film was harmed."), TEXT("Several takes were."),
	};
}

AScriptSequencer* AScriptSequencer::Get(const UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World) { return nullptr; }
	for (TActorIterator<AScriptSequencer> It(World); It; ++It) { return *It; }
	return nullptr;
}

void AScriptSequencer::BeginPlay()
{
	Super::BeginPlay();
	Scenes.Reset();
	for (TActorIterator<AScriptScene> It(GetWorld()); It; ++It) { Scenes.Add(*It); }
	Scenes.Sort([](const AScriptScene& A, const AScriptScene& B) { return A.SceneNumber < B.SceneNumber; });
	UE_LOG(LogReelScript, Log, TEXT("SCRIPT sequencer found %d scenes (on=%d)"), Scenes.Num(), bScriptOn ? 1 : 0);
	if (!SpliceSound)
	{
		SpliceSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Take/Audio/sfx_clapper.sfx_clapper"),
			nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	if (!IsRunning()) { return; }
	EnterScriptMode();
	// A jump asked for (console before play, or StartScene) skips the title card; else the title card plays first
	// and Tick starts the first scene when it goes.
	PendingStart = GScriptJump > 0 ? GScriptJump : StartScene;
}

void AScriptSequencer::EnterScriptMode()
{
	if (ATakeDirector* Dir = ATakeDirector::Get(this)) { Dir->bScriptDriven = true; }
}

void AScriptSequencer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsRunning()) { return; }
	ATakeDirector* Dir = ATakeDirector::Get(this);
	if (!bStarted)
	{
		// Wait for the director's first tick (it decides the title card), then for the title card to go.
		if (!Dir || !Dir->bCheckpointRestored) { return; }
		if (PendingStart > 0)
		{
			const int32 N = PendingStart;
			PendingStart = 0;
			JumpTo(N);
		}
		else if (Dir->Phase != ETakePhase::Title && Scenes.Num() > 0)
		{
			JumpTo(Scenes[0]->SceneNumber);
		}
		return;
	}
	if (EndSeconds >= 0.f)
	{
		EndSeconds += DeltaSeconds;
		return;
	}
	if (RetakeAt >= 0.0)
	{
		if (ScriptNow() - RetakeAt >= RetakeSeconds)
		{
			RetakeAt = -1.0;
			JumpTo(RetakeScene);
		}
		return;
	}
	if (Active)
	{
		Active->SceneSeconds += DeltaSeconds;
		Active->TickScene(DeltaSeconds);
	}
}

void AScriptSequencer::JumpTo(int32 SceneNumber)
{
	AScriptScene* Next = nullptr;
	for (AScriptScene* S : Scenes) { if (S && S->SceneNumber == SceneNumber) { Next = S; break; } }
	if (!Next) { UE_LOG(LogReelScript, Warning, TEXT("SCRIPT no scene %d"), SceneNumber); return; }
	ATakeDirector* Dir = ATakeDirector::Get(this);
	if (Dir)
	{
		Dir->bScriptDriven = true;
		if (Dir->Phase != ETakePhase::Rolling) { Dir->SetPhase(ETakePhase::Rolling); }
		// A jump past the title card or the old end: everyone free to move again (the title card holds them).
		for (ATakeCharacter* C : Dir->Characters)
		{
			UCharacterMovementComponent* Move = C ? C->GetCharacterMovement() : nullptr;
			if (Move && Move->MovementMode.GetValue() == MOVE_None) { Move->SetMovementMode(MOVE_Walking); }
		}
	}
	bStarted = true;
	PendingStart = 0;
	EndSeconds = -1.f;
	RetakeAt = -1.0;
	if (Active && Active->IsActive()) { Active->EndScene(); }
	Active = Next;
	Active->ResetScene();
	Active->SceneSeconds = 0.f;
	Pov = Active->Pov;
	SlugText = Active->Slug;
	SlugAt = SlugText.IsEmpty() ? -1.0 : ScriptNow();
	Splice(Pov, Active->SceneCamera);
	Active->BeginScene();
}

void AScriptSequencer::OnSceneFinished(AScriptScene* Scene)
{
	if (Scene != Active || RetakeAt >= 0.0) { return; }
	const int32 Idx = Scenes.IndexOfByKey(Scene);
	if (Scenes.IsValidIndex(Idx + 1)) { JumpTo(Scenes[Idx + 1]->SceneNumber); }
	else { Active->EndScene(); Active = nullptr; EndOfFilm(); }
}

void AScriptSequencer::OnSceneFailed(AScriptScene* Scene, FName Reason)
{
	if (Scene != Active || RetakeAt >= 0.0) { return; }
	UE_LOG(LogReelScript, Log, TEXT("SCRIPT blooper in scene %d: %s"), Scene->SceneNumber, *Reason.ToString());
	RetakeAt = ScriptNow();
	RetakeReason = Reason;
	RetakeScene = Scene->SceneNumber;
	++Scene->Retakes;
	if (ATakeDirector* Dir = ATakeDirector::Get(this))
	{
		if (Dir->Audio) { Dir->Audio->OnBlooper(); }
	}
	// Everyone stops where they are while the frame burns: the PC is off its character.
	if (ATakeDirector* Dir = ATakeDirector::Get(this))
	{
		if (Dir->PC) { Dir->PC->Current = nullptr; }
	}
}

void AScriptSequencer::OnCutTo(AScriptScene* Scene, EScriptPov NewPov, AActor* Camera)
{
	if (Scene != Active) { return; }
	Pov = NewPov;
	Splice(NewPov, Camera);
}

void AScriptSequencer::OnSay(const FString& Speaker, const FString& Line, USoundBase* Sound)
{
	UE_LOG(LogReelScript, Log, TEXT("SCRIPT say %s: %s"), *Speaker, *Line);
	if (ATakeDirector* Dir = ATakeDirector::Get(this))
	{
		const FString Text = Speaker.IsEmpty() ? Line : FString::Printf(TEXT("%s: %s"), *Speaker.ToUpper(), *Line);
		Dir->Flash(Text, ETakeFlashKind::Line, 2.f + 0.06f * Line.Len());
	}
	if (Sound) { UGameplayStatics::PlaySound2D(this, Sound); }
}

ATakeCharacter* AScriptSequencer::GetPovCharacter(EScriptPov ForPov) const
{
	const ATakeDirector* Dir = ATakeDirector::Get(this);
	if (!Dir) { return nullptr; }
	switch (ForPov)
	{
	case EScriptPov::Vance: return Dir->GetCharacter(ETakeCamera::A);
	case EScriptPov::Nora: return Dir->GetCharacter(ETakeCamera::B);
	case EScriptPov::Cole: return Dir->GetCharacter(ETakeCamera::C);
	default: return nullptr;
	}
}

void AScriptSequencer::PlacePov(AScriptScene* Scene, EScriptPov ForPov, AActor* Camera)
{
	ATakeDirector* Dir = ATakeDirector::Get(this);
	ATakePlayerController* PC = Dir ? Dir->PC.Get() : nullptr;
	if (!PC) { PC = Cast<ATakePlayerController>(UGameplayStatics::GetPlayerController(this, 0)); }
	ATakeCharacter* Char = GetPovCharacter(ForPov);
	// The scene's own start: the POV it opens on goes to its StartSpot (a cut inside the scene leaves people be).
	if (Char && Scene && Scene->StartSpot && ForPov == Scene->Pov && Camera == Scene->SceneCamera)
	{
		const FRotator R = Scene->StartSpot->GetActorRotation();
		Char->TeleportTo(Scene->StartSpot->GetActorLocation(), FRotator(0.f, R.Yaw, 0.f), false, true);
		Char->SetLook(R.Yaw, 0.f);
	}
	if (!PC) { return; }
	if (Char)
	{
		PC->SnapTo(Char);	// cancels any flight, the view on the character this frame
		if (Dir) { Dir->CurrentCamera = Char->Id; }
	}
	if (Camera) { PC->SetViewTarget(Camera); }
	// Max has no body, and a scene that does not walk keeps WASD for itself.
	if (!Char || (Scene && !Scene->bPlayerWalks)) { PC->Current = nullptr; }
	ViewCamera = Camera;
}

void AScriptSequencer::Splice(EScriptPov NewPov, AActor* Camera)
{
	UE_LOG(LogReelScript, Log, TEXT("SCRIPT splice to pov %d"), (int32)NewPov);
	SpliceAt = ScriptNow();
	PlacePov(Active, NewPov, Camera);
	if (SpliceSound) { UGameplayStatics::PlaySound2D(this, SpliceSound); }
	else if (ATakeDirector* Dir = ATakeDirector::Get(this))
	{
		if (Dir->Audio) { Dir->Audio->OnCut(); }
	}
}

void AScriptSequencer::EndOfFilm()
{
	UE_LOG(LogReelScript, Log, TEXT("SCRIPT end of film: PRINT IT"));
	EndSeconds = 0.f;
	if (ATakeDirector* Dir = ATakeDirector::Get(this))
	{
		if (Dir->Audio) { Dir->Audio->OnPrintIt(); }
		if (Dir->PC) { Dir->PC->Current = nullptr; }
	}
}

bool AScriptSequencer::HandleKey(const FKey& Key, bool bPressed)
{
	if (!IsRunning() || !bStarted) { return false; }
	if (EndSeconds >= 0.f)
	{
		// The credits: Enter, Space or R after "Print it." runs the film again from the top.
		if (bPressed && EndSeconds > PrintItSeconds
			&& (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::R) && Scenes.Num() > 0)
		{
			Flags.Reset();
			JumpTo(Scenes[0]->SceneNumber);
		}
		return Key != EKeys::Escape;
	}
	if (RetakeAt >= 0.0) { return Key != EKeys::Escape; }
	if (Active && Active->HandleSceneKey(Key, bPressed)) { return true; }
	if (bPressed && Key == EKeys::R && Active)
	{
		Active->FailScene(TEXT("Retake"));
		return true;
	}
	if (!bFreeSwitching && (Key == EKeys::One || Key == EKeys::Two || Key == EKeys::Three)) { return true; }
	return false;
}

float AScriptSequencer::GetSpliceAlpha() const
{
	if (SpliceAt < 0.0 || SpliceSeconds <= 0.f) { return 0.f; }
	const float T = static_cast<float>(ScriptNow() - SpliceAt) / SpliceSeconds;
	return T >= 1.f ? 0.f : 1.f - T;
}

FText AScriptSequencer::GetSlug(float& OutAge) const
{
	OutAge = SlugAt < 0.0 ? 0.f : static_cast<float>(ScriptNow() - SlugAt);
	if (SlugAt < 0.0 || OutAge > SlugSeconds || RetakeAt >= 0.0 || EndSeconds >= 0.f) { return FText::GetEmpty(); }
	return SlugText;
}

bool AScriptSequencer::IsRetaking(FName& OutReason, float& OutAge) const
{
	if (RetakeAt < 0.0) { return false; }
	OutReason = RetakeReason;
	OutAge = static_cast<float>(ScriptNow() - RetakeAt);
	return true;
}
