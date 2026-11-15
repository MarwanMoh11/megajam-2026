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

	/** Off: the sequencer does nothing and the old take plays (the fallback switch). */
	UPROPERTY(EditAnywhere, Category = "Script")
	bool bScriptOn = true;

	/** The splice's sound (a clack or a projector whir). Null: the take's clapper. */
	UPROPERTY(EditAnywhere, Category = "Script")
	TObjectPtr<class USoundBase> SpliceSound;

	/** How long the slug line holds on a scene's first cut, s. */
	UPROPERTY(EditAnywhere, Category = "Script")
	float SlugSeconds = 3.5f;

	/** How long the frame burns and the blooper card holds before a retake, s. */
	UPROPERTY(EditAnywhere, Category = "Script")
	float RetakeSeconds = 1.6f;

	/** The end: "Cut. Print it." holds this long, then the credits roll over sprockets. */
	UPROPERTY(EditAnywhere, Category = "Script")
	float PrintItSeconds = 3.f;

	UPROPERTY(EditAnywhere, Category = "Script")
	float CreditsSeconds = 22.f;

	/** The end credits, one line each ("" = a gap; a line in CAPS = a heading). */
	UPROPERTY(EditAnywhere, Category = "Script")
	TArray<FString> Credits;

	/** Free 1/2/3 switching while the script runs (off: the scenes cut for the player). */
	UPROPERTY(EditAnywhere, Category = "Script")
	bool bFreeSwitching = false;

	/** Tunables for the scenes and the HUD that need no restart: read with GetTune(Name, Default). */
	UPROPERTY(EditAnywhere, Category = "Script")
	TMap<FName, float> Tunes;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Start scene N (1-8): ResetScene, place the POV, splice cut, BeginScene. */
	void JumpTo(int32 SceneNumber);

	AScriptScene* GetActiveScene() const { return Active; }
	EScriptPov GetPov() const { return Pov; }

	/** The sequencer is running the take (on, with scenes): the director and the PC defer to it. */
	bool IsRunning() const { return bScriptOn && Scenes.Num() > 0; }

	/** The character for a POV (Vance = A, Nora = B, Cole = C); null for Max. */
	class ATakeCharacter* GetPovCharacter(EScriptPov ForPov) const;

	/** A key from the player controller: the active scene first, then the sequencer's own (R retake, Enter on the
	 *  end). True: consumed. */
	bool HandleKey(const FKey& Key, bool bPressed);

	/** The HUD's script layer (ATakeHUD::DrawScript): 0..1 through the splice (1 = its first frame), 0 none. */
	float GetSpliceAlpha() const;
	/** The slug line now showing and its age, s (empty when none). */
	FText GetSlug(float& OutAge) const;
	/** A retake is burning: its reason and age, s. */
	bool IsRetaking(FName& OutReason, float& OutAge) const;
	/** After the last scene: seconds since the end began; < 0 while the film runs. */
	float GetEndSeconds() const { return EndSeconds; }
	float GetTune(FName Name, float Default) const { const float* V = Tunes.Find(Name); return V ? *V : Default; }

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

	UPROPERTY(Transient)
	TObjectPtr<AActor> ViewCamera;

	/** The line Say is playing: stopped when the scene changes, burns or the film ends (not on a cut inside a scene). */
	UPROPERTY(Transient)
	TObjectPtr<class UAudioComponent> Voice;
	void StopVoice();

	double SpliceAt = -1.0;		// real time of the last splice
	double SlugAt = -1.0;		// real time the slug line went up
	FText SlugText;
	double RetakeAt = -1.0;		// real time the frame started burning; < 0 none
	FName RetakeReason;
	int32 RetakeScene = 0;
	float EndSeconds = -1.f;
	bool bStarted = false;		// the first scene has begun (after the title card)
	int32 PendingStart = 0;		// a jump asked for before play could start

	/** Place the POV character at the scene's StartSpot, put the player on it (or on the scene camera for Max). */
	void PlacePov(AScriptScene* Scene, EScriptPov ForPov, AActor* Camera);
	/** The director, PC and HUD switched to script mode (once). */
	void EnterScriptMode();

	/** SPINE: the cut itself: sound, the frame, the look for NewPov, the camera. */
	void Splice(EScriptPov NewPov, AActor* Camera);
	/** SPINE: after the last scene: "Cut. Print it.", credits over sprockets. */
	void EndOfFilm();
};
