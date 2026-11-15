// The script's scene contract (docs/SCRIPT_ADAPT.md). Owner of this file: the Spine session. Scene sessions subclass
// AScriptScene in their own files (Take/Script/SceneN*.h/.cpp) and never edit this file: ask Spine for a change.
// (Nov 15 evening: the movement flags, bAllowRun and bAllowCrouch, belong to the chars-locomotion session.)
// The sequencer (AScriptSequencer, Spine) plays the scenes in SceneNumber order, makes the splice cut into each,
// sets the POV's look (Vance = 35mm letterbox and grain, crew = raw feed), and calls the hooks below.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "ScriptScene.generated.h"

class UCanvas;
class AScriptSequencer;

/** Whose eyes a scene is shot through. Vance gets the Final Cut look; the crew get the Uncut Feed. */
UENUM(BlueprintType)
enum class EScriptPov : uint8
{
	Vance,	// was Julian (camera A)
	Max,	// new: the van
	Nora,	// was Maya (camera B)
	Cole	// was Vex (camera C)
};

UCLASS(Abstract)
class REEL_API AScriptScene : public AActor
{
	GENERATED_BODY()

public:
	AScriptScene();

	/** Order in the film, 1-8. The sequencer plays scenes in this order. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script")
	int32 SceneNumber = 0;

	/** Whose camera and look the scene opens on. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script")
	EScriptPov Pov = EScriptPov::Vance;

	/** Slug line shown on the cut, e.g. "EXT. THE GRAND LUMIERE THEATER - NIGHT". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script")
	FText Slug;

	/** Where the POV character is placed on BeginScene and on a retake. Null: leave the character where it is. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script")
	TObjectPtr<AActor> StartSpot;

	/** Optional camera actor the scene opens on. Null: the POV character's follow camera. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script")
	TObjectPtr<AActor> SceneCamera;

	/** True while the player can walk the POV character (false for minigames, fights, scripted shots). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script")
	bool bPlayerWalks = true;

	/** chars-locomotion (Nov 15): the player may run (hold Shift) in this scene: the escape and the finale only.
	 *  ATakeCharacter reads it from the active scene every frame (ATakeCharacter::CanPlayerRun). Default off. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script|Movement")
	bool bAllowRun = false;

	/** chars-locomotion (Nov 15): the player may crouch (C or Ctrl toggles) in this scene. Default off. A scene can also
	 *  crouch the POV itself (ATakeCharacter::SetCrouching) whatever this says. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script|Movement")
	bool bAllowCrouch = false;

	// ---- Hooks the sequencer calls. Override; call Super for Begin/End. ----

	/** The frame the splice cut lands on this scene (first time and every retake, after ResetScene). */
	virtual void BeginScene();

	/** Every frame while this scene is the active one (game time, so slow time applies). */
	virtual void TickScene(float DeltaSeconds) {}

	/** The sequencer leaves this scene (it finished, or a jump went elsewhere). */
	virtual void EndScene();

	/** Put everything this scene moved back to its start state, for a retake or a jump. */
	virtual void ResetScene() {}

	/** A key went down or up while this scene is active. Return true to consume it (walk and cut keys then skip it). */
	virtual bool HandleSceneKey(const FKey& Key, bool bPressed) { return false; }

	/** Draw the scene's own HUD (minigame, choices, prompts) on top of the film frame. Called by ATakeHUD. */
	virtual void DrawSceneHud(UCanvas* Canvas) {}

	// ---- Calls a scene makes. ----

	/** The objective is done: the sequencer cuts to the next scene. */
	UFUNCTION(BlueprintCallable, Category = "Script")
	void FinishScene();

	/** A blooper: the sequencer burns the frame and retakes this scene from its start. */
	UFUNCTION(BlueprintCallable, Category = "Script")
	void FailScene(FName Reason);

	/** A cut inside the scene (e.g. Scene 2 cuts back to Vance at the scanner): splice cut to that POV's look,
	 *  and to Camera if given, else that POV's follow camera. */
	UFUNCTION(BlueprintCallable, Category = "Script")
	void CutTo(EScriptPov NewPov, AActor* Camera = nullptr);

	/** A subtitle (and Sound if given), through the HUD's subtitle lane. */
	UFUNCTION(BlueprintCallable, Category = "Script")
	void Say(const FString& Speaker, const FString& Line, class USoundBase* Sound = nullptr);

	/** Story flags shared across scenes, e.g. "VipAlibi" set in Scene 3 and spent at a guard check later. */
	UFUNCTION(BlueprintCallable, Category = "Script")
	void SetFlag(FName Flag);
	UFUNCTION(BlueprintCallable, Category = "Script")
	bool HasFlag(FName Flag) const;
	/** True and clears it if set. */
	UFUNCTION(BlueprintCallable, Category = "Script")
	bool ConsumeFlag(FName Flag);

	bool IsActive() const { return bActive; }
	AScriptSequencer* GetSequencer() const;

	/** The character the player is on for this POV (Vance, Nora, Cole); null for Max (the van is a camera). */
	class ATakeCharacter* GetPovCharacter() const;

	/** Seconds since this scene's BeginScene (game time; restarts on a retake). */
	float GetSceneSeconds() const { return SceneSeconds; }

	/** Retakes of this scene so far this session (0 on the first go): for hints after a couple of bloopers. */
	int32 GetRetakes() const { return Retakes; }

protected:
	/** Set by the sequencer around BeginScene/EndScene. */
	bool bActive = false;
	float SceneSeconds = 0.f;
	int32 Retakes = 0;
	friend class AScriptSequencer;
};
