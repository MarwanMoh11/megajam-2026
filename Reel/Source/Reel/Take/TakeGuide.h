// Where to go next (QA Nov 15 evening, hud-guidance: "There should be guidance as to where to go next, visually natural
// ones as well"). The set says it first, the HUD last:
//   1. AStageGuide posts, placed per scene in the level: a warm pool of light on the goal (it sits on an interactable
//      when Highlight is set), dimmer pools along the path (bTrail), a glow in the doorway (bDoorGlow).
//   2. When a goal lights, the cast nearest the player glance toward it.
//   3. When the player wanders without getting closer (Guide.LostSeconds), a crew line names the goal, the cast glance
//      again and the goal's pool swells.
//   4. Longer still (Guide.MarkerSeconds), a soft marker and the objective fade in at the goal, or at the picture's edge
//      pointing to it (ATakeHUD::DrawGuide). They fade once he is on his way.
// Scene code can also name a goal directly (UTakeGuideSubsystem::SetGoal), for goals that move or have no post.
// Tunables without a restart: the sequencer's Tunes map (Guide.LostSeconds, Guide.MarkerSeconds, Guide.IdleRate).
// Console: Reel.Guide 0|1 (off/on), Reel.Guide.Lost (jump straight to the marker, for shots). Owner: hud-guidance.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "TakeGuide.generated.h"

class USpotLightComponent;
class URectLightComponent;
class USoundBase;
class ATakeCharacter;

/** A guide post: one goal (or a trail pool on the way to one) in one scene. */
UCLASS()
class REEL_API AStageGuide : public AActor
{
	GENERATED_BODY()

public:
	AStageGuide();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** The scene this post guides in (1-8). 0: parked, never lit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide")
	int32 SceneNumber = 0;

	/** Order inside the scene: the lowest step not yet done is lit. Posts sharing a step light together. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide")
	int32 Step = 0;

	/** A trail post: a dimmer pool on the way to its step's goal; never the goal, never "reached". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide")
	bool bTrail = false;

	/** Lit only once this sequencer flag is set. None: from the scene's start (+ StartDelay). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide")
	FName RequireFlag;

	/** Done when this sequencer flag is set. None: done when the player comes within ReachRadius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide")
	FName DoneFlag;

	/** Seconds into the scene before it lights. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide")
	float StartDelay = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide")
	float ReachRadius = 220.f;

	/** The fallback words under the soft marker ("The oak door"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide")
	FString Objective;

	/** Said once when the player wanders (subtitle; voiced when CrewSound is set). Empty: no line. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide")
	FString CrewLine;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide")
	FString CrewSpeaker = TEXT("MAX (V.O.)");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide")
	TObjectPtr<USoundBase> CrewSound;

	/** An interactable to light (a person to talk to, a panel): the pool and the marker follow it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide")
	TObjectPtr<AActor> Highlight;

	/** Cast within this distance of the player glance at the goal when it lights and when he wanders. 0: none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide")
	float GlanceRadius = 1400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide")
	int32 GlanceCount = 3;

	// ---- the light ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide|Light")
	bool bPool = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide|Light")
	float PoolHeight = 380.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide|Light")
	float PoolRadius = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide|Light")
	float PoolCandelas = 380.f;

	/** A trail post's pool, as a share of PoolCandelas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide|Light")
	float TrailShare = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide|Light")
	FLinearColor PoolColour = FLinearColor(1.f, 0.78f, 0.5f);

	/** A warm glow standing in a doorway, shining along the post's +X (point the arrow into the room he is in). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide|Light")
	bool bDoorGlow = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide|Light")
	FVector2D DoorSize = FVector2D(140.f, 230.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide|Light")
	float DoorCandelas = 40.f;

	/** While the player is lost the goal's light swells to this many times itself, breathing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide|Light")
	float CallBoost = 1.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide|Light")
	float EaseRate = 2.5f;

	/** Lit in the editor so it can be placed and judged; off in game until its step comes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide|Light")
	bool bPreviewInEditor = true;

	// ---- driven by UTakeGuideSubsystem ----
	/** Lit: 0..1 of its look. Call: 0..1 of the swell while the player is lost. */
	void SetTarget(float Lit, float Call) { LitTarget = Lit; CallTarget = Call; }
	FVector GetGoalPoint() const;
	bool bDone = false;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Guide")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Guide")
	TObjectPtr<USpotLightComponent> Pool;

	UPROPERTY(VisibleAnywhere, Category = "Guide")
	TObjectPtr<URectLightComponent> Door;

	void Apply(float Lit, float Call, double Time);

	float LitTarget = 0.f;
	float CallTarget = 0.f;
	float LitNow = 0.f;
	float CallNow = 0.f;
};

/** What the HUD draws: the soft marker and the objective. */
struct FTakeGuideView
{
	bool bShow = false;
	FVector Goal = FVector::ZeroVector;
	FString Objective;
	float Alpha = 0.f;
};

UCLASS()
class REEL_API UTakeGuideSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTakeGuideSubsystem* Get(const UObject* WorldContext);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override
	{
		return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
	}
	virtual void Tick(float DeltaSeconds) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTakeGuideSubsystem, STATGROUP_Tickables); }

	/** Scene code: the goal is here now (over any post's, for the marker and the wander check). Cleared on a new scene. */
	void SetGoal(const FVector& At, const FString& Objective, const FString& CrewLine = FString(),
		const FString& CrewSpeaker = TEXT("MAX (V.O.)"));
	void ClearGoal();

	// ---- the HUD ----
	FTakeGuideView GetView() const;
	void SetSubtitleUp(bool bUp) { bSubtitleUp = bUp; }
	/** A crew line is due (once per goal); true hands it over and marks it said. */
	bool TakeCrewLine(FString& OutSpeaker, FString& OutLine, USoundBase*& OutSound);

	/** Reel.Guide.Lost: straight to the marker (shots and tests). */
	void ForceLost();

private:
	void Reset(bool bNewScene);
	void Glance(const FVector& Goal, const FVector& From, float Radius, int32 Count) const;

	TArray<TWeakObjectPtr<AStageGuide>> Posts;
	TWeakObjectPtr<UObject> LastScene;
	int32 LastRetakes = -1;

	bool bCodeGoal = false;
	FVector CodeGoal = FVector::ZeroVector;
	FString CodeObjective;
	FString CodeCrewLine;
	FString CodeCrewSpeaker;
	bool bCodeCrewSaid = false;

	TWeakObjectPtr<AStageGuide> Goal;	// the active step's goal post
	FVector GoalAt = FVector::ZeroVector;
	FString GoalObjective;
	bool bHaveGoal = false;
	FVector LastGoalAt = FVector(1e9);

	float Best = 0.f;	// the closest he has come to this goal, cm (2D)
	float Lost = 0.f;	// seconds of wandering without getting closer
	float MarkerAlpha = 0.f;
	bool bSubtitleUp = false;
	bool bCrewDue = false;
	bool bCrewSaidForGoal = false;
	bool bGlancedLost = false;
};
