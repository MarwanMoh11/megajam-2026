// A gold floor mark: a place to stand. Standing still on one is MARK; the goal mark clears the test. Owner: (m).
// Since the Nov 13 rework marks are no longer clicked (no cue ring, no click volume): AJulian::StepMarks finds them.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Mark.generated.h"

class UStaticMeshComponent;
class USpotLightComponent;

/** Place it on the floor (actor location = the floor point Julian stands on). Its shapes are NoCollision. */
UCLASS()
class REEL_API AMark : public AActor
{
	GENERATED_BODY()

public:
	AMark();

	/** Colours the disc gold and hides the X. */
	virtual void BeginPlay() override;	// BODY: (m)

	/** Same look as BeginPlay, in the editor. */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Called by AJulian::StepMarks: the gold X under him shows while he stands on this mark (MARK). This frame. */
	void SetOccupied(bool bOccupied);	// BODY: (m)

	/** The floor point Julian stands on (the actor location). */
	FVector GetStandPoint() const;	// BODY: (m)

	bool IsOccupied() const { return bIsOccupied; }

	/** The name the HUD and the log use ("ROPE", "PAST THE ROPE"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Mark")
	FString MarkName = TEXT("MARK");

	/** The mark past the rope: Julian standing on it clears the test (StepScene). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Mark")
	bool bGoal = false;

	/** Step 4, pass 3 (critique): the T plate's and the disc's size, x the pass-3 plate (80 cm bar). Visual only:
	 *  standing on the mark is the director's test, not these meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Look", meta = (ClampMin = "0.5", ClampMax = "2.5"))
	float PlateScale = 1.4f;

	// Step 2: checkpoints are no longer a mark's own business. An ASceneTrigger box saves one and names the mark
	// Julian restarts on (ASceneTrigger::CheckpointMark).

	// --- step 2: an action mark (rule 3, "the mark behind Vance lifts the keycard"). BODY: S ------------------------

	/** > 0: this is an action mark. Julian standing still on it, not SEEN, for this long does the action. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Mark")
	float ActionSeconds = 0.f;

	/** The trigger the finished action fires (ATakeDirector::FireTrigger), e.g. "Keycard". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Mark")
	FName ActionTrigger;

	/** The HUD's word over the progress bar while it runs ("LIFTING THE KEYCARD"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Mark")
	FString ActionVerb = TEXT("Lifting");

	/** ATakeDirector::StepMarkActions calls this every frame for every action mark: bRunning is "Julian stands on
	 *  this mark and is not SEEN this frame". Running: ActionProgress += Dt; not running: ActionProgress = 0 (it
	 *  starts over). Returns true on the one frame ActionProgress reaches ActionSeconds (bActionDone becomes true and
	 *  stays). Once done it does nothing more. The disc pulses brighter while it runs (ApplyLook). */
	bool StepAction(float DeltaSeconds, bool bRunning);	// BODY: S

	/** 0..1 through the action (0 when not running or not an action mark, 1 when done). The HUD's bar. */
	float GetActionAlpha() const;	// BODY: S

	bool IsActionMark() const { return ActionSeconds > 0.f; }
	bool IsActionDone() const { return bActionDone; }

	/** The checkpoint restore: an action whose trigger had already fired is done (no second lift). */
	void SetActionDone() { bActionDone = true; ActionProgress = ActionSeconds; }

	// --- step 3: marks that appear, and what an action can ask for and do (BODY: S) ---------------------------------

	/** Set: the mark is hidden and cannot be stood on until this trigger has fired (the case mark appears with the
	 *  keycard, the trap's mark when the winch lands). ATakeDirector calls SetAppeared from FireTrigger and from the
	 *  first tick (after the checkpoint restore). None: it is there from the start. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Mark")
	FName AppearOn;

	/** Shows or hides the whole mark (SetActorHiddenInGame) and sets bAppeared, this frame. */
	void SetAppeared(bool bInAppeared);	// BODY: S

	/** False only for an AppearOn mark whose trigger has not fired: AJulian::StepMarks skips it. */
	bool HasAppeared() const { return bAppeared; }

	/** Set: the action only runs once this trigger has fired (the exits and the bow need the Case trigger). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Mark")
	FName ActionNeeds;

	/** The action runs even while Julian is SEEN (the exits: he is leaving anyway). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Mark")
	bool bActionWhileSeen = false;

	/** The action only runs while the broadcast drone (a watcher with ExposesOffMark) is blinded (The Bow). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Mark")
	bool bNeedsBlindDrone = false;

	/** > 0: the finished action starts this scene (the keycard starts Scene 3), with ActionTitle as the big title. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Mark")
	int32 ActionScene = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Mark")
	FString ActionTitle;

	/** Not empty: the finished action saves a checkpoint with this name ("THE CASE"). Julian restarts on
	 *  ActionCheckpointMark, or on this mark when that is not set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Mark")
	FString ActionCheckpointName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Mark")
	TObjectPtr<AMark> ActionCheckpointMark;

	/** Julian's feet within this 2D distance of the stand point, standing still, counts as on the mark, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Mark")
	float StandRadius = 60.f;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<USceneComponent> Root;

	/** The gold disc (a flat cylinder). */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> Disc;

	/** The gold X under Julian: two flat bars at +-45 degrees, hidden until occupied. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> CrossA;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> CrossB;

	// --- QA sight: a gently lit place to stand (Marwan's note 18: no T marks) ----------------------------------------
	// With the drawn overlays off (ATakeDirector::bDrawnSightOverlays) the plate and disc are hidden in game (still
	// shown in the editor) and the mark is a soft warm pool of light from above. Julian on it: the pool settles on him
	// (tighter, a little brighter). An action running: it warms and brightens with the progress. Done: it dims to an
	// ember. Look only: standing on the mark is still AJulian::StepMarks's test (StandRadius).

	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	bool bMarkLight = true;

	/** The light's height over the stand point, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "50"))
	float PoolHeight = 320.f;

	/** At rest: candelas, the pool's radius on the floor (cm) and colour. */
	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0"))
	float PoolCandelas = 260.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "10"))
	float PoolRadius = 80.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	FLinearColor PoolColour = FLinearColor(1.f, 0.8f, 0.55f);

	/** Julian on it: candelas, radius and colour (it settles on him). */
	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0"))
	float OccupiedCandelas = 380.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "10"))
	float OccupiedRadius = 58.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	FLinearColor OccupiedColour = FLinearColor(1.f, 0.76f, 0.48f);

	/** An action at its end (it runs from the occupied look to this): candelas and colour. */
	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0"))
	float ActionCandelas = 650.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	FLinearColor ActionColour = FLinearColor(1.f, 0.66f, 0.36f);

	/** A done action mark keeps this share of the rest pool (an ember: its work is done). */
	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0", ClampMax = "1"))
	float DoneScale = 0.35f;

	/** How fast the pool follows, per second (0 = snaps). */
	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0"))
	float PoolEaseRate = 7.f;

	/** The pool's glow in the haze (0 = none, the cheap default). */
	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0"))
	float PoolHaze = 0.f;

	/** The pool (a spot light from above, no shadows). */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<USpotLightComponent> PoolLight;

	/** Eases the pool toward its look (the only thing a mark ticks for). */
	virtual void Tick(float DeltaSeconds) override;

protected:
	/** The pool now (eased toward PoolTarget's values). */
	float PoolCandelasNow = 0.f;
	float PoolRadiusNow = 80.f;
	FLinearColor PoolColourNow = FLinearColor::White;

	/** The pool's look for the state now. */
	void GetPoolTarget(float& OutCandelas, float& OutRadius, FLinearColor& OutColour) const;

	/** Moves the pool toward its look by DeltaSeconds (< 0: snaps) and applies it to PoolLight. */
	void ApplyPool(float DeltaSeconds);

protected:
	bool bIsOccupied = false;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	float ActionProgress = 0.f;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	bool bActionDone = false;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	bool bAppeared = true;

private:
	/** Disc and X colours, X visibility from the occupied state. This frame. */
	void ApplyLook();
};
