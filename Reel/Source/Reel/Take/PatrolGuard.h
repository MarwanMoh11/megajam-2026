// The patrol guard: waypoints, turning on bar lines (rule 6). Owner: (d).

#pragma once

#include "CoreMinimal.h"
#include "Watcher.h"
#include "PatrolGuard.generated.h"

class APatrolPoint;
class USpotLightComponent;

/** Where the guard's lure is. */
enum class EGuardLurePhase : uint8
{
	None,
	Going,		// walking to the lure point
	Looking,	// standing, facing the point, until the lure's duration ends
	Returning	// walking back to where the patrol says he should be
};

/**
 * His patrol is a pure function of the bar clock: PatrolPoints cycle, each held for its Bars; at the bar line
 * where a point's hold ends he turns (or walks at WalkSpeed, then turns) to the next. A reload therefore restores
 * him exactly. The frame is a flat red wedge on the floor (a cone pitched to lie along +X and squashed flat),
 * FrameRange long with FrameHalfAngle either side; frames ignore walls in step 1.
 */
UCLASS()
class REEL_API APatrolGuard : public AWatcher
{
	GENERATED_BODY()

public:
	APatrolGuard();

	/** Sizes the wedge from FrameRange / FrameHalfAngle (editor and runtime). */
	virtual void OnConstruction(const FTransform& Transform) override;	// BODY: (d)

	virtual void BeginPlay() override;	// BODY: (d)

	/** Patrol from the clock (EvaluatePatrol), or the lure (go, look, return), then the beat pulse. */
	virtual void StepWatcher(float DeltaSeconds, UBarClockComponent* Clock) override;	// BODY: (d)

	/** Julian's feet within FrameRange (2D) and within FrameHalfAngle of his facing, while live. */
	virtual bool HoldsJulian(const AJulian* Julian) const override;	// BODY: (d)

	/** Blind as the base. Lure: State Lured for Duration, LurePoint = Point, he turns to it and starts walking this
	 *  frame. Mask: nothing in step 1 (returns false). */
	virtual bool ApplyEffect(EDeviceEffect Effect, float Duration, const FVector& Point) override;	// BODY: (d)

	virtual FString GetStateText() const override;	// BODY: (d)

	/** Pure function of the clock: where the patrol puts him and which way he faces at Clock's current time.
	 *  Step 2: the patrol's time is the bars since bar 1 PLUS PatrolOffsetBars. */
	void EvaluatePatrol(const UBarClockComponent* Clock, FVector& OutLocation, float& OutYaw) const;	// BODY: (d)

	// --- step 2 (BODY: W) --------------------------------------------------------------------------------------------

	/** The index in PatrolPoints of the point the patrol holds at the START of Bar (1-based bar, PatrolOffsetBars
	 *  applied; -1 with no points). Pure. */
	int32 GetPatrolIndexAtBar(int32 Bar) const;	// BODY: W

	/** The patrol moves on to another point on the bar line that starts Bar (GetPatrolIndexAtBar(Bar) differs from
	 *  GetPatrolIndexAtBar(Bar - 1)). False with fewer than two points, before he has arrived, or while lured. */
	virtual bool ChangesAtBar(int32 Bar, const UBarClockComponent* Clock) const override;	// BODY: W

	/** The four far corners of the wedge in FramePivot's space: at FrameRange along +X, at +-FrameHalfAngle, on the
	 *  floor and at FrameLineHeight. */
	virtual void GetFrameFarCorners(TArray<FVector>& OutLocal) const override;	// BODY: W

	/** Bars added to the patrol's clock: Vance's cycle (crowd 4 bars, lift 4 bars) with offset 1 puts the start of
	 *  each speech on a band break (bars 8, 16, 24...). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Guard")
	int32 PatrolOffsetBars = 0;

	// --- step 3 (BODY: W): the guards who walk in at the alarm -------------------------------------------------------

	/** The patrol's clock starts at his arrival bar (GetArriveBar) and not at bar 1: he is at PatrolPoints[0] on the
	 *  bar he arrives, whatever bar that is. PatrolOffsetBars still adds. Only meaningful with ArriveOn set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Guard")
	bool bPatrolFromArrival = false;

	/** The patrol does not loop: he goes through PatrolPoints once (holding each for its Bars, walking between them)
	 *  and then holds the last point for good. ChangesAtBar is false once he is there. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Guard")
	bool bPatrolOnce = false;

	/** Vance: a Lure only turns him to the point for its duration, he does not walk (rule 6). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Guard")
	bool bTurnsOnly = false;

	/** Height of the frame lines' upper far corners, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Guard")
	float FrameLineHeight = 200.f;

	/** The patrol, in order; it loops. Empty: he holds his placed pose. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Guard")
	TArray<TObjectPtr<APatrolPoint>> PatrolPoints;

	/** Frame length, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Guard")
	float FrameRange = 900.f;

	/** Frame half-angle, degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Guard")
	float FrameHalfAngle = 30.f;

	/** Walking speed between patrol points and on a lure, cm/s. */
	UPROPERTY(EditAnywhere, Category = "Take|Guard")
	float WalkSpeed = 200.f;

	/** How long a turn on a bar line takes, s (0 = snap). */
	UPROPERTY(EditAnywhere, Category = "Take|Guard")
	float TurnSeconds = 0.2f;

	/** On a lure he stops this far short of the point, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Guard")
	float LureStandOff = 150.f;

	/** Grey stand-in body (cylinder). */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> Body;

	/** A cap block on the front of the head so his facing reads from above. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> Cap;

	// --- QA sight: his torch (Marwan's note 5: no vision cone) -------------------------------------------------------
	// The soft pool his torch throws ahead of him is what he sees; when he holds Julian it tightens and warms onto
	// him. Look only: HoldsJulian is the same wedge as before (FrameRange, FrameHalfAngle).

	/** Off: no torch (a watcher whose sight is told some other way). */
	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	bool bSightLight = true;

	// --- W3 lights --- (900 cd, warm, 0.35 haze could not be made out in play: the hall's keys give the floor
	// 10-15 lux and the lobby's red carpet swallows a warm pool. Now a pale hard-edged pool 3-4x the set light and a
	// visible shaft in the haze, so "he is looking there" reads from anywhere in the room.)
	/** The torch at rest, candelas (a lit floor reads 95-130 at about 30-40 lux). W3 lights: was 900. */
	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0"))
	float SightLightCandelas = 3000.f;

	/** The torch's colour at rest: a pale hot tungsten, whiter than the set's lamps and the marks. W3: was (1, .84, .62). */
	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	FLinearColor SightLightColour = FLinearColor(1.f, 0.92f, 0.78f);

	/** It aims at the floor this far along FrameRange (0..1). W3 lights: was 0.5. */
	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0.1", ClampMax = "1"))
	float SightLightAimFraction = 0.45f;

	/** W3 lights: the pool's inner cone as a share of the outer at rest (1 = a hard edge; was 0.55). */
	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0", ClampMax = "0.95"))
	float SightLightEdge = 0.75f;

	/** W3 lights: the widest the rest pool gets, degrees (a 45-degree frame made a faint wash; 70 = old behaviour). */
	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "4", ClampMax = "70"))
	float SightLightMaxConeDegrees = 28.f;

	/** The pool's half-angle as a share of FrameHalfAngle. */
	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0.2", ClampMax = "2"))
	float SightLightConeScale = 1.f;

	/** The beam's glow in the haze (0 = none). W3 lights: was 0.35; 4 shows a shaft even over the red carpet (a signal material, a pool does not show on it). */
	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0"))
	float SightLightHaze = 4.f;

	/** The torch's height under his eye, cm, and how far in front of him it is held. */
	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	FVector SightLightOffset = FVector(25.f, 0.f, -15.f);

	/** While he holds Julian: candelas, colour (warmer) and half-angle of the tightened pool on him. */
	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0"))
	float HoldLightCandelas = 4500.f;	// W3 lights: was 2400 (under the new rest torch)

	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	FLinearColor HoldLightColour = FLinearColor(1.f, 0.55f, 0.32f);

	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "2", ClampMax = "45"))
	float HoldConeDegrees = 9.f;

	/** How fast the pool swings onto him and back, per second (0 = snaps). */
	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0"))
	float HoldEaseRate = 12.f;

	/** How fast the torch comes up on his arrival and dims when he is blinded, per second (0 = snaps). */
	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0"))
	float LightEaseRate = 8.f;

	/** Blinded: the torch drops to this share (he is dazzled; it is not watching). */
	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0", ClampMax = "1"))
	float BlindedLightScale = 0.1f;

	/** The alarm: the torch goes this colour and this much brighter. */
	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	FLinearColor AlarmLightColour = FLinearColor(0.86f, 0.93f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Take|Cues", meta = (ClampMin = "0"))
	float AlarmLightScale = 1.4f;

	/** On the frame he starts holding Julian, his cast body plays the "LookAt" gesture (ACastMember::PlayGesture). */
	UPROPERTY(EditAnywhere, Category = "Take|Cues")
	bool bLookAtGesture = true;

	/** The torch (a spot light, no shadows). */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<USpotLightComponent> SightLight;

protected:
	/** QA sight: the base (planning lift), then the torch: rest pool, hold, blinded, alarm. */
	virtual void UpdateWorldCues(float DeltaSeconds) override;

	/** The torch's hold (0 at rest, 1 on Julian) and level (0 off, 1 on), now. */
	float HoldAlpha = 0.f;
	float LightAlpha = 0.f;

	/** He held Julian at the last UpdateWorldCues (the LookAt gesture plays on the change). */
	bool bCueWasHolding = false;

protected:
	EGuardLurePhase LurePhase = EGuardLurePhase::None;
	FVector LureStart = FVector::ZeroVector;
	float LureTime = 0.f;

	/** The lure's own timer (stacks with a Blind's dark timer): Going/Looking until it ends, then Returning. */
	float LureSecondsLeft = 0.f;

	/** Dark first (base), else LURED while the lure's timer runs. */
	virtual void SyncState() override;

	/** The patrol point held at the last StepWatcher (-1 = none yet): a change flashes the new point's ArriveLine. */
	int32 LastPatrolIndex = -1;

private:
	/** Sizes the wedge from FrameRange / FrameHalfAngle and colours the wedge, body and cap (OnConstruction and BeginPlay). */
	void LayoutGuard();
};
