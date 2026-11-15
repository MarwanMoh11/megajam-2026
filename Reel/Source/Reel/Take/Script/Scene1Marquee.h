// Scene 1. EXT. THE GRAND LUMIERE THEATER - NIGHT (Vance, 35mm). docs/SCRIPT.md. Owner: s1-marquee.
//
// THE BEATS (about 30 s)
//  Queue      Vance (Julian) in the line, GuestsAhead guests ahead; the front waits (AQueueLine::bScriptHoldFront).
//             A VIP card insert slides up: the forged mag-stripe card in his sleeve.
//  Manager    A suited FLOOR MANAGER steps out of the doors and walks to the BOUNCER (the queue's doorman).
//  Whisper    He whispers (his line), hands over a brushed-steel palm scanner on a coiled cable, goes back in.
//  Bolt       The Bouncer bolts it onto the podium at the door (a thud, the plate lights).
//  Directive  BOUNCER: "Directives from Mr. Pendleton. Magnetic cards are void. Palm verification only."
//             Vance's composure flickers (two dark frames; the card insert gets VOID and drops away).
//  Line       The line moves. Each guest at the plate presses a hand: BEEP, green, in.
//  Turn       Vance first in line, the plate free: the hold stands down; "Step up to the scanner".
//  Hand       Vance at the plate: BOUNCER "Hand on the glass." When it is said: FinishScene (Spine's splice cut into
//             Scene 2, the van).
// Scene 2 ends here too: s2-van calls PalmAccepted() (the laser, the chime, green, "Enjoy the screening, Your Grace",
// the gate opens) and StageAtPlate() to start from this state on a jump or retake.
// Placed by tools/script/scene1.py (tag script_s1) at the podium, +X toward the guest at the plate.

#pragma once

#include "CoreMinimal.h"
#include "Script/ScriptScene.h"
#include "Scene1Marquee.generated.h"

class ACastMember;
class AQueueLine;
class AScannerWatcher;
class UAudioComponent;
class UPointLightComponent;
class USoundBase;
class UStaticMeshComponent;
class UMaterialInterface;

UENUM()
enum class EScene1Beat : uint8
{
	Setup,		// waiting for the line to be put back (the director's Julian)
	Queue,
	Manager,
	Whisper,
	Bolt,
	Directive,
	Line,
	Turn,
	Hand,
	Done
};

UCLASS()
class REEL_API AScene1Marquee : public AScriptScene
{
	GENERATED_BODY()

public:
	AScene1Marquee();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void BeginScene() override;
	virtual void TickScene(float DeltaSeconds) override;
	virtual void EndScene() override;
	virtual void ResetScene() override;
	virtual void DrawSceneHud(UCanvas* Canvas) override;

	// ---- For Scene 2 (s2-van) ----

	/** Scene 2's end, at the plate: the red laser sweeps Vance's hand, CHIME, the plate and the arch go green, the
	 *  Bouncer steps aside ("Enjoy the screening, Your Grace") and the gate opens, so Vance can walk in. Runs on this
	 *  actor's own tick (Scene 1 need not be the active scene). Call once; a second call while it plays is ignored. */
	UFUNCTION(BlueprintCallable, Category = "Script|Scene1")
	void PalmAccepted();

	/** PalmAccepted has played out (the line said, the gate open): about PalmSeconds() after the call. */
	UFUNCTION(BlueprintCallable, Category = "Script|Scene1")
	bool IsPalmDone() const { return bPalmDone; }

	/** How long PalmAccepted takes, s (the sweep, the chime, the Bouncer's line). */
	UFUNCTION(BlueprintCallable, Category = "Script|Scene1")
	float PalmSeconds() const;

	/** Scene 2's start state: nobody ahead, Vance at the plate facing it, the scanner bolted and lit, the Bouncer at
	 *  his post, the gate shut. Call from Scene 2's ResetScene or BeginScene: if the director is not ready yet it
	 *  happens on the first frame it is. */
	UFUNCTION(BlueprintCallable, Category = "Script|Scene1")
	void StageAtPlate();

	/** s2-van's name for StageAtPlate (it calls by name through FindFunction). */
	UFUNCTION(BlueprintCallable, Category = "Script|Scene1")
	void StageForVan() { StageAtPlate(); }

	/** Set (SetFlag) when Vance is through the doors after PalmAccepted: Scene 2 then finishes. */
	static FName PalmThroughFlag() { return TEXT("S1PalmThrough"); }

	/** After the Bouncer's line, Vance walks in through the gate (the queue's WayIn) at this pace; past this many
	 *  seconds he is put there. */
	UPROPERTY(EditAnywhere, Category = "Script|Scene1", meta = (ClampMin = "1"))
	float WalkInMaxSeconds = 5.f;

	/** The plate's centre, world (Scene 2's CRT feed can aim a capture here). */
	UFUNCTION(BlueprintCallable, Category = "Script|Scene1")
	FVector GetPlatePoint() const;

	// ---- Tunables ----

	/** Guests left ahead of Vance at the start (the rest are already inside). */
	UPROPERTY(EditAnywhere, Category = "Script|Scene1", meta = (ClampMin = "0"))
	int32 GuestsAhead = 2;

	/** The floor manager comes out this long after the cut, s. */
	UPROPERTY(EditAnywhere, Category = "Script|Scene1", meta = (ClampMin = "0"))
	float ManagerDelay = 2.f;

	/** Where he comes out (inside the doors) and where he stops by the Bouncer, world cm. */
	UPROPERTY(EditAnywhere, Category = "Script|Scene1")
	FVector ManagerFrom = FVector(-3120.f, 150.f, 0.f);

	UPROPERTY(EditAnywhere, Category = "Script|Scene1")
	FVector ManagerTo = FVector(-3560.f, 150.f, 0.f);

	UPROPERTY(EditAnywhere, Category = "Script|Scene1", meta = (ClampMin = "10"))
	float ManagerSpeed = 170.f;

	UPROPERTY(EditAnywhere, Category = "Script|Scene1")
	FLinearColor ManagerTint = FLinearColor(0.045f, 0.045f, 0.055f);

	/** The handover: the scanner in his hand, then the Bouncer's, then bolted, s. */
	UPROPERTY(EditAnywhere, Category = "Script|Scene1", meta = (ClampMin = "0"))
	float HandoverSeconds = 0.8f;

	UPROPERTY(EditAnywhere, Category = "Script|Scene1", meta = (ClampMin = "0"))
	float BoltSeconds = 1.f;

	/** The hand socket the carried scanner rides on. */
	UPROPERTY(EditAnywhere, Category = "Script|Scene1")
	FName HandSocket = TEXT("hand_r");

	/** A guest's BEEP comes when its scan has this share left (AQueueLine::ScanSeconds). */
	UPROPERTY(EditAnywhere, Category = "Script|Scene1", meta = (ClampMin = "0", ClampMax = "1"))
	float BeepAtShare = 0.5f;

	/** The plate stays green this long after a guest's beep, s. */
	UPROPERTY(EditAnywhere, Category = "Script|Scene1", meta = (ClampMin = "0"))
	float GreenSeconds = 0.9f;

	/** Vance's feet this close to the scan point: he is at the plate, cm. */
	UPROPERTY(EditAnywhere, Category = "Script|Scene1", meta = (ClampMin = "10"))
	float StepUpRadius = 115.f;

	/** After "Hand on the glass." is said, the cut comes this much later, s. */
	UPROPERTY(EditAnywhere, Category = "Script|Scene1", meta = (ClampMin = "0"))
	float CutAfterLine = 0.25f;

	/** PalmAccepted: the laser's sweep, then the chime and the Bouncer's line. */
	UPROPERTY(EditAnywhere, Category = "Script|Scene1", meta = (ClampMin = "0.1"))
	float LaserSeconds = 1.1f;

	/** The Bouncer steps aside by this much (world), letting Vance by. */
	UPROPERTY(EditAnywhere, Category = "Script|Scene1")
	FVector AsideOffset = FVector(40.f, -90.f, 0.f);

	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Lines")
	FString ManagerLine = TEXT("Word from Mr. Pendleton. The cards are out. Palms only, tonight.");

	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Lines")
	FString DirectiveLine = TEXT("Directives from Mr. Pendleton. Magnetic cards are void. Palm verification only.");

	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Lines")
	FString HandLine = TEXT("Hand on the glass.");

	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Lines")
	FString EnjoyLine = TEXT("Enjoy the screening, Your Grace.");

	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Lines")
	FString WaitGoal = TEXT("Wait your turn in the queue");

	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Lines")
	FString StepGoal = TEXT("Step up to the palm scanner");

	// ---- Sounds (a missing one is skipped; the subtitle still shows) ----

	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Sound")
	TSoftObjectPtr<USoundBase> ManagerVoice;
	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Sound")
	TSoftObjectPtr<USoundBase> DirectiveVoice;
	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Sound")
	TSoftObjectPtr<USoundBase> HandVoice;
	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Sound")
	TSoftObjectPtr<USoundBase> EnjoyVoice;
	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Sound")
	TSoftObjectPtr<USoundBase> BeepSound;
	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Sound")
	TSoftObjectPtr<USoundBase> ChimeSound;
	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Sound")
	TSoftObjectPtr<USoundBase> BoltSound;
	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Sound")
	TSoftObjectPtr<USoundBase> LaserSound;
	/** The band inside, muffled through the doors (low-passed). */
	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Sound")
	TSoftObjectPtr<USoundBase> JazzSound;
	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Sound")
	TSoftObjectPtr<USoundBase> RainSound;

	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Sound", meta = (ClampMin = "0", ClampMax = "2"))
	float JazzVolume = 0.45f;
	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Sound", meta = (ClampMin = "100"))
	float JazzLowPassHz = 700.f;
	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Sound", meta = (ClampMin = "0", ClampMax = "2"))
	float RainVolume = 0.35f;

	// ---- The look of the scanner and the podium (null: plain coloured shapes) ----

	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Look")
	TObjectPtr<UMaterialInterface> PodiumMaterial;
	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Look")
	TObjectPtr<UMaterialInterface> SteelMaterial;

	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Look")
	FLinearColor IdleGlow = FLinearColor(0.15f, 0.45f, 1.6f);
	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Look")
	FLinearColor GreenGlow = FLinearColor(0.2f, 3.2f, 0.6f);
	UPROPERTY(EditAnywhere, Category = "Script|Scene1|Look")
	FLinearColor LaserGlow = FLinearColor(6.f, 0.15f, 0.1f);

	// ---- Components ----

	UPROPERTY(VisibleAnywhere, Category = "Script|Scene1")
	TObjectPtr<USceneComponent> PodiumRoot;
	UPROPERTY(VisibleAnywhere, Category = "Script|Scene1")
	TObjectPtr<UStaticMeshComponent> Podium;
	UPROPERTY(VisibleAnywhere, Category = "Script|Scene1")
	TObjectPtr<UStaticMeshComponent> PodiumCap;
	/** The palm scanner: carried, then bolted on the podium's top. */
	UPROPERTY(VisibleAnywhere, Category = "Script|Scene1")
	TObjectPtr<USceneComponent> ScannerRoot;
	UPROPERTY(VisibleAnywhere, Category = "Script|Scene1")
	TObjectPtr<UStaticMeshComponent> ScannerBody;
	UPROPERTY(VisibleAnywhere, Category = "Script|Scene1")
	TObjectPtr<UStaticMeshComponent> Glass;
	UPROPERTY(VisibleAnywhere, Category = "Script|Scene1")
	TObjectPtr<UStaticMeshComponent> Laser;
	UPROPERTY(VisibleAnywhere, Category = "Script|Scene1")
	TObjectPtr<UPointLightComponent> PlateLight;
	/** The coiled cable down the podium's side (shown once bolted). */
	UPROPERTY(VisibleAnywhere, Category = "Script|Scene1")
	TArray<TObjectPtr<UStaticMeshComponent>> Cable;
	UPROPERTY(VisibleAnywhere, Category = "Script|Scene1")
	TObjectPtr<UAudioComponent> JazzAudio;
	UPROPERTY(VisibleAnywhere, Category = "Script|Scene1")
	TObjectPtr<UAudioComponent> RainAudio;

protected:
	EScene1Beat Beat = EScene1Beat::Setup;
	double BeatAt = 0.0;		// real time the beat began
	double LineEndsAt = 0.0;	// real time the last line is done
	bool bNeedReset = false;
	bool bNeedStage = false;

	UPROPERTY(Transient)
	TObjectPtr<ACastMember> Manager;

	int32 ScanGuest = INDEX_NONE;	// the guest at the plate now
	bool bScanBeeped = false;
	double GreenUntil = 0.0;

	// Composure flicker and the card insert (HUD), real times.
	double CardAt = -1.0;
	double VoidAt = -1.0;

	// PalmAccepted
	double PalmAt = -1.0;
	int32 PalmStep = 0;
	double WalkInAt = 0.0;
	int32 WalkLeg = 0;
	FVector LastWalkFeet = FVector::ZeroVector;
	bool bPalmDone = false;

	enum class EScannerAt : uint8 { Hidden, Manager, Bouncer, Bolted };
	EScannerAt ScannerAt = EScannerAt::Hidden;

	double Now() const;
	void EnterBeat(EScene1Beat NewBeat);
	/** Say a line (subtitle and voice); returns its length, s, and sets LineEndsAt. */
	float SpeakLine(const FString& Speaker, const FString& Line, const TSoftObjectPtr<USoundBase>& Voice);
	void PlayAt(const TSoftObjectPtr<USoundBase>& Sound, const FVector& Where, float Volume = 1.f);
	void PutScanner(EScannerAt Where);
	void SetPlateGlow(const FLinearColor& Glow, float Candelas);
	void StartAmbience(bool bOn);
	void StepLineBeats();
	void StepPalm();
	/** Back to the start: the line, the Bouncer, the scanner hidden, the manager gone. True when done. */
	bool DoReset(int32 Ahead);
	void HideManager();
	AQueueLine* GetQueue() const;
	AScannerWatcher* GetScanner() const;
	class AJulian* GetJulian() const;
};
