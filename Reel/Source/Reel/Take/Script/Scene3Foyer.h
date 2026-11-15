// Scene 3. INT. GRAND FOYER & STAIRCASE + the MAIN HALL (Vance, 35mm). Owner: the s3-foyer session.
// docs/SCRIPT.md Scene 3, docs/script/s3.md. Placed by tools/script/scene3.py (tag script_s3).
// Beats: Vance comes in from the scanner; a tipsy FILM CRITIC stumbles down the grand stair and sloshes champagne
// on his lapel; a dialogue choice (1/2 or arrows + E): Play Along sets the VipAlibi flag, Brush Off sends him off.
// Then the main hall (the great hall west of the velvet rope): walk toward the rope; Max's V.O. at RopeWarnX, the
// cut to Nora (FinishScene) FinishAfterVo later. Past RopeStopX a rope guard stops him: VipAlibi spends once (he is
// sent back to the bar), otherwise he is tackled (FailScene).

#pragma once

#include "CoreMinimal.h"
#include "Script/ScriptScene.h"
#include "Scene3Foyer.generated.h"

class ACastMember;
class APatrolGuard;
class USoundBase;
class UStaticMeshComponent;

UCLASS()
class REEL_API AScene3Foyer : public AScriptScene
{
	GENERATED_BODY()

public:
	AScene3Foyer();

	// ---- The cast (set by tools/script/scene3.py) ----

	/** The film critic: a free cast member tagged NoSetLife, standing on the grand stair with a coupe. */
	UPROPERTY(EditAnywhere, Category = "Scene3|Cast")
	TObjectPtr<ACastMember> Critic;

	/** The four rope guards (APatrolGuard, each with its own patrol points and a Guard-kit cast member). */
	UPROPERTY(EditAnywhere, Category = "Scene3|Cast")
	TArray<TObjectPtr<APatrolGuard>> RopeGuards;

	/** Hidden while this scene runs, shown again after (the old take's chief at the stair's foot). */
	UPROPERTY(EditAnywhere, Category = "Scene3|Cast")
	TArray<TObjectPtr<AActor>> HideDuringScene;

	// ---- The stair (tools/sets/front.py: x -2600 .. -1500 rising to z 450, |y| < 200) ----

	UPROPERTY(EditAnywhere, Category = "Scene3|Stair")
	float StairX0 = -2600.f;
	UPROPERTY(EditAnywhere, Category = "Scene3|Stair")
	float StairX1 = -1500.f;
	UPROPERTY(EditAnywhere, Category = "Scene3|Stair")
	float StairTopZ = 450.f;
	UPROPERTY(EditAnywhere, Category = "Scene3|Stair")
	float StairHalfY = 200.f;

	// ---- The critic's beat ----

	/** Seconds after BeginScene before he starts down (sooner if Vance comes within CriticTriggerRadius). */
	UPROPERTY(EditAnywhere, Category = "Scene3|Critic")
	float CriticDelay = 1.2f;
	UPROPERTY(EditAnywhere, Category = "Scene3|Critic")
	float CriticTriggerRadius = 450.f;
	/** His stumble down the stair, cm/s, and the side-to-side sway, cm. */
	UPROPERTY(EditAnywhere, Category = "Scene3|Critic")
	float CriticSpeed = 150.f;
	UPROPERTY(EditAnywhere, Category = "Scene3|Critic")
	float CriticSway = 22.f;
	/** He reaches Vance at this distance, or after CriticMaxSeconds of stumbling, whichever first. */
	UPROPERTY(EditAnywhere, Category = "Scene3|Critic")
	float MeetDistance = 120.f;
	UPROPERTY(EditAnywhere, Category = "Scene3|Critic")
	float CriticMaxSeconds = 3.5f;
	/** The choice opens this long after the splash. */
	UPROPERTY(EditAnywhere, Category = "Scene3|Critic")
	float ChoiceDelay = 0.6f;
	/** Where he wanders after Brush Off / after Play Along (world, floor). */
	UPROPERTY(EditAnywhere, Category = "Scene3|Critic")
	FVector CriticWanderTo = FVector(-2350.f, -900.f, 0.f);

	// ---- The hall and the rope ----

	/** Max's V.O. when Vance's feet pass this x (the main hall, short of the guards). */
	UPROPERTY(EditAnywhere, Category = "Scene3|Hall")
	float RopeWarnX = 1300.f;
	/** A guard stops him past this x (the guards walk at x ~2260, the rope is at 2400). */
	UPROPERTY(EditAnywhere, Category = "Scene3|Hall")
	float RopeStopX = 2130.f;
	/** The cut to Nora this long after the V.O. starts. */
	UPROPERTY(EditAnywhere, Category = "Scene3|Hall")
	float FinishAfterVo = 7.5f;
	/** Tackled: the retake this long after the guard's line. */
	UPROPERTY(EditAnywhere, Category = "Scene3|Hall")
	float TackleSeconds = 1.1f;
	/** The alibi: the guard sends him back this far west of RopeStopX, over AlibiPushSeconds. */
	UPROPERTY(EditAnywhere, Category = "Scene3|Hall")
	float AlibiPushBack = 320.f;
	UPROPERTY(EditAnywhere, Category = "Scene3|Hall")
	float AlibiPushSeconds = 0.7f;

	// ---- Lines (subtitles) and their voices (null: subtitle only) ----

	UPROPERTY(EditAnywhere, Category = "Scene3|Lines")
	FString CriticLine = TEXT("Mon Dieu! Watch the... wait. I know that jawline. You're the financier from Zurich! Brilliant piece in Cahiers last month!");
	UPROPERTY(EditAnywhere, Category = "Scene3|Lines")
	FString PlayAlongLine = TEXT("Keep your voice down, darling. I'm incognito.");
	UPROPERTY(EditAnywhere, Category = "Scene3|Lines")
	FString PlayAlongReply = TEXT("Ooh! My lips are sealed. I'll tell the door you're with me, monsieur!");
	UPROPERTY(EditAnywhere, Category = "Scene3|Lines")
	FString BrushOffLine = TEXT("You have the wrong man.");
	UPROPERTY(EditAnywhere, Category = "Scene3|Lines")
	FString BrushOffReply = TEXT("Hmph! Philistine. Zurich was much better dressed.");
	UPROPERTY(EditAnywhere, Category = "Scene3|Lines")
	FString MaxVoLine = TEXT("Too many eyes on the velvet rope, Vance. You walk up there without a seat assignment, they'll tackle you into the shrimp cocktail. Nora, you in position?");
	UPROPERTY(EditAnywhere, Category = "Scene3|Lines")
	FString GuardTackleLine = TEXT("You're not on the list. Down you go!");
	UPROPERTY(EditAnywhere, Category = "Scene3|Lines")
	FString GuardAlibiLine = TEXT("Name? ...Ah, the gentleman from Zurich. Your seat isn't ready, sir. Wait by the bar.");

	UPROPERTY(EditAnywhere, Category = "Scene3|Voice")
	TObjectPtr<USoundBase> CriticSound;
	UPROPERTY(EditAnywhere, Category = "Scene3|Voice")
	TObjectPtr<USoundBase> PlayAlongSound;
	UPROPERTY(EditAnywhere, Category = "Scene3|Voice")
	TObjectPtr<USoundBase> PlayAlongReplySound;
	UPROPERTY(EditAnywhere, Category = "Scene3|Voice")
	TObjectPtr<USoundBase> BrushOffSound;
	UPROPERTY(EditAnywhere, Category = "Scene3|Voice")
	TObjectPtr<USoundBase> BrushOffReplySound;
	UPROPERTY(EditAnywhere, Category = "Scene3|Voice")
	TObjectPtr<USoundBase> MaxVoSound;
	UPROPERTY(EditAnywhere, Category = "Scene3|Voice")
	TObjectPtr<USoundBase> GuardTackleSound;
	UPROPERTY(EditAnywhere, Category = "Scene3|Voice")
	TObjectPtr<USoundBase> GuardAlibiSound;
	/** The champagne's splash (null: none). */
	UPROPERTY(EditAnywhere, Category = "Scene3|Voice")
	TObjectPtr<USoundBase> SplashSound;

	// ---- AScriptScene ----
	virtual void BeginPlay() override;
	virtual void BeginScene() override;
	virtual void TickScene(float DeltaSeconds) override;
	virtual void EndScene() override;
	virtual void ResetScene() override;
	virtual bool HandleSceneKey(const FKey& Key, bool bPressed) override;
	virtual void DrawSceneHud(UCanvas* Canvas) override;

private:
	enum class EBeat : uint8 { Arrive, Stumble, Spill, Choice, Reply, Hall, Vo, Stopped, Done };
	enum class EChoice : uint8 { None, PlayAlong, BrushOff };

	void SetBeat(EBeat NewBeat);
	void StartStumble();
	void DoSpill();
	void Commit(EChoice Pick);
	void StopAtRope();
	void FreezeVance(bool bFreeze);
	void ShowHidden(bool bShow);
	void ClearFx();
	float StairZ(const FVector& P) const;
	ACastMember* GuardBody(APatrolGuard* Guard) const;
	APatrolGuard* NearestGuard(const FVector& P) const;
	FVector VanceFeet() const;

	EBeat Beat = EBeat::Arrive;
	float BeatT = 0.f;
	EChoice Picked = EChoice::None;
	int32 Hover = 0;			// 0 Play Along, 1 Brush Off
	bool bSaidReply = false;
	bool bAlibiSpent = false;
	bool bFrozen = false;
	float RetargetT = 0.f;
	FVector PushFrom = FVector::ZeroVector;
	FVector PushTo = FVector::ZeroVector;
	bool bPushing = false;
	double ChoiceOpenedAt = -1.0;	// real time
	double PickedAt = -1.0;		// real time

	// The critic's start pose (taken at BeginPlay).
	FVector CriticHome = FVector::ZeroVector;
	float CriticHomeYaw = 0.f;

	// The splash: droplets flying from his glass, and the wet mark on Vance's lapel.
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Drops;
	TArray<FVector> DropVel;
	float DropT = -1.f;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Stain;
};
