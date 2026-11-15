// See ScriptSequencer.h. Owner: the Spine session. Stubs marked SPINE are filled in there.

#include "Script/ScriptSequencer.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogScript, Log, All);

// ---- AScriptScene ----

AScriptScene::AScriptScene()
{
	PrimaryActorTick.bCanEverTick = false;	// the sequencer calls TickScene
}

void AScriptScene::BeginScene()
{
	bActive = true;
	UE_LOG(LogScript, Log, TEXT("SCRIPT begin scene %d %s"), SceneNumber, *GetName());
}

void AScriptScene::EndScene()
{
	bActive = false;
	UE_LOG(LogScript, Log, TEXT("SCRIPT end scene %d %s"), SceneNumber, *GetName());
}

AScriptSequencer* AScriptScene::GetSequencer() const { return AScriptSequencer::Get(this); }

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

AScriptSequencer::AScriptSequencer()
{
	PrimaryActorTick.bCanEverTick = true;
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
	UE_LOG(LogScript, Log, TEXT("SCRIPT sequencer found %d scenes"), Scenes.Num());
	const int32 First = GScriptJump > 0 ? GScriptJump : StartScene;
	if (First > 0) { JumpTo(First); }
	else if (Scenes.Num() > 0) { JumpTo(Scenes[0]->SceneNumber); }
}

void AScriptSequencer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Active) { Active->TickScene(DeltaSeconds); }
}

void AScriptSequencer::JumpTo(int32 SceneNumber)
{
	AScriptScene* Next = nullptr;
	for (AScriptScene* S : Scenes) { if (S && S->SceneNumber == SceneNumber) { Next = S; break; } }
	if (!Next) { UE_LOG(LogScript, Warning, TEXT("SCRIPT no scene %d"), SceneNumber); return; }
	if (Active) { Active->EndScene(); }
	Active = Next;
	Active->ResetScene();
	Pov = Active->Pov;
	Splice(Pov, Active->SceneCamera);	// SPINE: also place the POV character at StartSpot and possess it
	Active->BeginScene();
}

void AScriptSequencer::OnSceneFinished(AScriptScene* Scene)
{
	if (Scene != Active) { return; }
	const int32 Idx = Scenes.IndexOfByKey(Scene);
	if (Scenes.IsValidIndex(Idx + 1)) { JumpTo(Scenes[Idx + 1]->SceneNumber); }
	else { Active->EndScene(); Active = nullptr; EndOfFilm(); }
}

void AScriptSequencer::OnSceneFailed(AScriptScene* Scene, FName Reason)
{
	if (Scene != Active) { return; }
	UE_LOG(LogScript, Log, TEXT("SCRIPT blooper in scene %d: %s"), Scene->SceneNumber, *Reason.ToString());
	JumpTo(Scene->SceneNumber);	// SPINE: the frame burn and the blooper card first
}

void AScriptSequencer::OnCutTo(AScriptScene* Scene, EScriptPov NewPov, AActor* Camera)
{
	Pov = NewPov;
	Splice(NewPov, Camera);
}

void AScriptSequencer::OnSay(const FString& Speaker, const FString& Line, USoundBase* Sound)
{
	UE_LOG(LogScript, Log, TEXT("SCRIPT say %s: %s"), *Speaker, *Line);	// SPINE: HUD subtitle lane and the sound
}

void AScriptSequencer::Splice(EScriptPov NewPov, AActor* Camera)
{
	UE_LOG(LogScript, Log, TEXT("SCRIPT splice to pov %d"), (int32)NewPov);	// SPINE
}

void AScriptSequencer::EndOfFilm()
{
	UE_LOG(LogScript, Log, TEXT("SCRIPT end of film: PRINT IT"));	// SPINE
}
