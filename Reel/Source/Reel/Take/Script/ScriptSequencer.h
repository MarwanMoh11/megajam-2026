// Plays the script's scenes in order (docs/SCRIPT_ADAPT.md). Owner: the Spine session.
// One per level. Finds every AScriptScene on BeginPlay, sorts by SceneNumber, starts the first (or the one given by
// the console command Reel.Script.Jump N). The bodies marked SPINE are stubs the Spine session fills in: the splice
// cut, the POV look (letterbox and grain for Vance only), possession, retakes, subtitles and the ending.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScriptScene.h"
#include "ScriptSequencer.generated.h"

UCLASS()
class REEL_API AScriptSequencer : public AActor
{
	GENERATED_BODY()

public:
	AScriptSequencer();

	static AScriptSequencer* Get(const UObject* WorldContextObject);

	/** Scene to start on in play; 0 = the first. Reel.Script.Jump N overrides it at run time. */
	UPROPERTY(EditAnywhere, Category = "Script")
	int32 StartScene = 0;

	/** Seconds of the splice cut (the clapper or reel whir plays on its first frame). */
	UPROPERTY(EditAnywhere, Category = "Script")
	float SpliceSeconds = 0.35f;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Start scene N (1-8): ResetScene, place the POV, splice cut, BeginScene. */
	void JumpTo(int32 SceneNumber);

	AScriptScene* GetActiveScene() const { return Active; }
	EScriptPov GetPov() const { return Pov; }

	// Called by AScriptScene.
	void OnSceneFinished(AScriptScene* Scene);
	void OnSceneFailed(AScriptScene* Scene, FName Reason);
	void OnCutTo(AScriptScene* Scene, EScriptPov NewPov, AActor* Camera);
	void OnSay(const FString& Speaker, const FString& Line, class USoundBase* Sound);

	TSet<FName> Flags;

protected:
	UPROPERTY(Transient)
	TArray<TObjectPtr<AScriptScene>> Scenes;

	UPROPERTY(Transient)
	TObjectPtr<AScriptScene> Active;

	EScriptPov Pov = EScriptPov::Vance;

	/** SPINE: the cut itself: sound, the frame, the look for NewPov, the camera. */
	void Splice(EScriptPov NewPov, AActor* Camera);
	/** SPINE: after the last scene: "Cut. Print it.", credits over sprockets. */
	void EndOfFilm();
};
