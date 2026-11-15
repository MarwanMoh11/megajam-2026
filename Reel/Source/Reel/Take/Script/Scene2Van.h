// Scene 2. INT. FLORIST VAN - CONTINUOUS (Max, raw feed). Owner: the s2-van session (docs/SCRIPT_ADAPT.md).
// The opening move flies the scene's own camera from the theater steps across the wet street, through the van's
// fogged windshield, and settles on Max. Then the signal-intercept minigame: two dials (A/D frequency, Left/Right
// gain) match the palm scanner's drifting wave on the scope until the SYNC meter fills, then SPACE patches the bypass
// cable, before Vance's hesitation at the podium looks suspicious (full = FailScene). On the lock: BZZT, Max slams the
// toggle and talks, the scene cuts to Vance, calls Scene 1's PalmAccepted() by name, and finishes when Scene 1 sets
// the flag S1PalmThrough (or after a safety timeout).
// Placed by tools/script/scene2.py (tag script_s2), which sets the world points below.

#pragma once

#include "CoreMinimal.h"
#include "Script/ScriptScene.h"
#include "TakeHudKit.h"
#include "Scene2Van.generated.h"

class UCameraComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class USoundBase;

UENUM()
enum class EScene2Phase : uint8
{
	Opening,	// the camera move from the steps into the van
	Tune,		// the minigame
	Locked,		// BZZT-LOCK, the toggle, Max's line
	Vance,		// cut to Vance at the scanner; waiting for Scene 1 to walk him in
	Done
};

UCLASS()
class REEL_API AScene2Van : public AScriptScene
{
	GENERATED_BODY()

public:
	AScene2Van();

	// ---- the camera (this actor is the scene's camera: SceneCamera = this) ----
	UPROPERTY(VisibleAnywhere, Category = "Scene2")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Scene2")
	TObjectPtr<UCameraComponent> Cam;

	/** The CRT's live feed of Vance's hand: captures into a render target the HUD draws. */
	UPROPERTY(VisibleAnywhere, Category = "Scene2")
	TObjectPtr<USceneCaptureComponent2D> FeedCapture;

	// The opening move, world points (scene2.py sets them from the van's placement).
	UPROPERTY(EditAnywhere, Category = "Scene2|Opening")
	FVector StepsPos = FVector(-3400.f, 250.f, 220.f);
	/** The pan starts looking here (the queue at the scanner) and swings to WindshieldLook. */
	UPROPERTY(EditAnywhere, Category = "Scene2|Opening")
	FVector StepsLook = FVector(-3520.f, 320.f, 150.f);
	UPROPERTY(EditAnywhere, Category = "Scene2|Opening")
	FVector WindshieldPos = FVector(-5380.f, 1080.f, 170.f);
	UPROPERTY(EditAnywhere, Category = "Scene2|Opening")
	FVector WindshieldLook = FVector(-5380.f, 1330.f, 160.f);
	UPROPERTY(EditAnywhere, Category = "Scene2|Opening")
	FVector InsidePos = FVector(-5400.f, 1400.f, 160.f);
	/** The settled shot on Max. */
	UPROPERTY(EditAnywhere, Category = "Scene2|Opening")
	FVector MaxCamPos = FVector(-5450.f, 1450.f, 150.f);
	UPROPERTY(EditAnywhere, Category = "Scene2|Opening")
	FVector MaxLook = FVector(-5340.f, 1690.f, 120.f);
	/** Seconds: pan on the steps, cross the street, push through the glass, settle. */
	UPROPERTY(EditAnywhere, Category = "Scene2|Opening")
	float BeatPan = 1.4f;
	UPROPERTY(EditAnywhere, Category = "Scene2|Opening")
	float BeatCross = 1.9f;
	UPROPERTY(EditAnywhere, Category = "Scene2|Opening")
	float BeatPush = 0.7f;
	UPROPERTY(EditAnywhere, Category = "Scene2|Opening")
	float BeatSettle = 1.0f;
	float OpeningLength() const { return BeatPan + BeatCross + BeatPush + BeatSettle; }
	UPROPERTY(EditAnywhere, Category = "Scene2|Opening")
	float StreetFov = 62.f;
	UPROPERTY(EditAnywhere, Category = "Scene2|Opening")
	float VanFov = 78.f;

	// The CRT feed: a security camera on the facade looking down at the scanner.
	UPROPERTY(EditAnywhere, Category = "Scene2|Feed")
	FVector FeedFrom = FVector(-3330.f, 120.f, 420.f);
	UPROPERTY(EditAnywhere, Category = "Scene2|Feed")
	FVector FeedAt = FVector(-3520.f, 320.f, 110.f);
	UPROPERTY(EditAnywhere, Category = "Scene2|Feed")
	float FeedFov = 38.f;

	/** Optional camera for the cut back to Vance at the scanner (null: Vance's follow camera). */
	UPROPERTY(EditAnywhere, Category = "Scene2")
	TObjectPtr<AActor> VanceCamera;

	// ---- the minigame (the sequencer's Tunes "S2.<name>" override these when present) ----
	/** Seconds of Vance's hesitation before the bouncer gets suspicious (fail). Each retake adds RetakeBonus. */
	UPROPERTY(EditAnywhere, Category = "Scene2|Game")
	float HesitationSeconds = 24.f;
	UPROPERTY(EditAnywhere, Category = "Scene2|Game")
	float RetakeBonus = 5.f;
	/** Frequency match window (cycles across the scope) and gain window (0..1). */
	UPROPERTY(EditAnywhere, Category = "Scene2|Game")
	float FreqTolerance = 0.28f;
	UPROPERTY(EditAnywhere, Category = "Scene2|Game")
	float GainTolerance = 0.09f;
	/** Seconds in the window to fill SYNC. */
	UPROPERTY(EditAnywhere, Category = "Scene2|Game")
	float SyncSeconds = 0.7f;
	/** Dial speeds while held (per second). */
	UPROPERTY(EditAnywhere, Category = "Scene2|Game")
	float FreqRate = 2.0f;
	UPROPERTY(EditAnywhere, Category = "Scene2|Game")
	float GainRate = 0.5f;
	/** Hesitation added by patching before SYNC is full (0..1 of the bar). */
	UPROPERTY(EditAnywhere, Category = "Scene2|Game")
	float BadPatchCost = 0.12f;
	/** Seconds after the cut to Vance before the scene finishes on its own if Scene 1 never sets S1PalmThrough. */
	UPROPERTY(EditAnywhere, Category = "Scene2|Game")
	float VanceTimeout = 9.f;

	// ---- sound (optional; null = silent, the subtitles still show) ----
	UPROPERTY(EditAnywhere, Category = "Scene2|Sound")
	TObjectPtr<USoundBase> LineOpen;		// "Palm reader? Seriously?!..."
	UPROPERTY(EditAnywhere, Category = "Scene2|Sound")
	TObjectPtr<USoundBase> LineRetake;		// "Again. Three seconds, Vance..."
	UPROPERTY(EditAnywhere, Category = "Scene2|Sound")
	TObjectPtr<USoundBase> LineLock;		// "You're a French duke now..."
	UPROPERTY(EditAnywhere, Category = "Scene2|Sound")
	TObjectPtr<USoundBase> LineNotYet;		// "Not yet, not yet!"
	UPROPERTY(EditAnywhere, Category = "Scene2|Sound")
	TObjectPtr<USoundBase> LineClose;		// "Come on, come on..."
	UPROPERTY(EditAnywhere, Category = "Scene2|Sound")
	TObjectPtr<USoundBase> LineBouncerA;	// "Sir? The glass."
	UPROPERTY(EditAnywhere, Category = "Scene2|Sound")
	TObjectPtr<USoundBase> LineBouncerB;	// "Is there a problem, monsieur?"
	UPROPERTY(EditAnywhere, Category = "Scene2|Sound")
	TObjectPtr<USoundBase> SfxDial;
	UPROPERTY(EditAnywhere, Category = "Scene2|Sound")
	TObjectPtr<USoundBase> SfxSpark;
	UPROPERTY(EditAnywhere, Category = "Scene2|Sound")
	TObjectPtr<USoundBase> SfxLock;			// BZZT-LOCK
	UPROPERTY(EditAnywhere, Category = "Scene2|Sound")
	TObjectPtr<USoundBase> SfxToggle;
	/** The van's idle and the rack's hum, looped while we are in the van. */
	UPROPERTY(EditAnywhere, Category = "Scene2|Sound")
	TObjectPtr<USoundBase> AmbHum;

	// ---- AScriptScene ----
	virtual void PostInitializeComponents() override;
	virtual void BeginScene() override;
	virtual void TickScene(float DeltaSeconds) override;
	virtual void EndScene() override;
	virtual void ResetScene() override;
	virtual bool HandleSceneKey(const FKey& Key, bool bPressed) override;
	virtual void DrawSceneHud(UCanvas* Canvas) override;

private:
	EScene2Phase Phase = EScene2Phase::Opening;
	float PhaseAge = 0.f;

	// Dials and the target wave.
	float Freq = 1.5f;	// cycles across the scope, 1..6
	float Gain = 0.22f;	// 0.1..1
	float TargetFreq = 3.6f;
	float TargetGain = 0.6f;
	float Seed = 0.f;
	float Sync = 0.f;	// 0..1
	float Hesitation = 0.f;	// 0..1
	int8 FreqHeld = 0;	// -1, 0, +1
	int8 GainHeld = 0;
	bool bKeyA = false, bKeyD = false, bKeyL = false, bKeyR = false;

	// Feedback ages (game seconds since; large = none).
	float SinceDial = 99.f;
	float SinceSpark = 99.f;
	float SinceLock = 99.f;
	float LastDialSound = 0.f;
	bool bSaidClose = false, bSaidBouncerA = false, bSaidBouncerB = false;
	bool bPalmCalled = false;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> FeedTarget;
	UPROPERTY(Transient)
	TObjectPtr<class UAudioComponent> HumComp;
	void StopHum();

	/** The HUD kit (type, keycaps, stamps): a member so its runtime fonts stay referenced. */
	UPROPERTY(Transient)
	FTakeHudKit Kit;

	float Tune(const TCHAR* Name, float Default) const;
	void SetPhase(EScene2Phase P) { Phase = P; PhaseAge = 0.f; }
	void PoseOpening(float T);
	void UpdateTarget();
	bool InWindow() const;
	void TryPatch();
	void Play(USoundBase* S) const;
	/** Call a no-parameter UFUNCTION on the Scene 1 actor by name (StageForVan, PalmAccepted); false if missing. */
	bool CallScene1(FName Function) const;
	float HesitationLimit() const;

	void DrawFeed(UCanvas* C, float X, float Y, float W, float H, float S);
	void DrawScope(UCanvas* C, float X, float Y, float W, float H, float S);
	void DrawDial(UCanvas* C, float CX, float CY, float R, float Value01, const FString& Label, const FString& KeyL,
		const FString& KeyR, bool bHeld, float S);
};
