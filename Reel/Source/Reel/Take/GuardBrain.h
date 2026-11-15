// The guards' brain (stealth-ai, Nov 15): what a stealth player expects from a guard, as one component any scene can
// put on its own guard actor (an ACastMember it spawned, or any actor).
//
//   PATROL ──sees a glimpse──> SUSPICIOUS (stops, turns his head and body, "Huh?", a "?" over his head that fills)
//      ^                          │ lost him early: back to patrol ("Must be the light.")
//      │                          │ lost him after a good look, or a NOISE: INVESTIGATE (walks to the point)
//      │                          v
//   RETURN <── SEARCH (looks around, walks a couple of nearby spots) <── INVESTIGATE
//      (walks back the way he came)                                        ^
//   the meter full: ALERT ("There!", a red "!") -> CHASE (follows the player's trail) -> lost him: INVESTIGATE
//   the last place he saw him, then SEARCH. Close enough while chasing: CAUGHT (the scene decides what that means).
//
// Vision: a cone (a wide inner cone at full rate, a wider peripheral band at a slower rate) out to SightRange,
// blocked by walls (traces to the player's head and chest), slower with distance, in the dark (the lights around the
// player are summed and traced), when he is crouched and when he stands still. Hearing: ReportNoise() from anything
// (a thrown object, a crash, a device), plus the player's own footsteps (running carries far, walking only up close,
// crouching is silent). A downed colleague in view (KnockDown, and not hidden) raises the alarm and he searches.
// The scene stays in charge: it reads the state and gets events (OnEvent) and lines to speak (OnBark), and it can
// lure him (Lure), blind him, knock him down, hold him, or reset him for a retake.
//
// Everything tunable is a UPROPERTY(EditAnywhere) on the component. Console: Reel.Guard.Debug 1 draws the cones and
// the meter, Reel.Guard.Test spawns a test guard near the player in play-in-editor, Reel.Guard.Noise makes a noise.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GuardBrain.generated.h"

class ACastMember;
class ULocalLightComponent;
class USpotLightComponent;
class UTextRenderComponent;
class UGuardBrainComponent;

UENUM(BlueprintType)
enum class EGuardState : uint8
{
	Patrol,			// walking the route (or holding a post), glancing around
	Suspicious,		// stopped, looking toward what he half-saw; the meter fills while he still sees it
	Investigate,	// walking to a noise or a last-seen point (or a lure)
	Search,			// at the point: looking around, a couple of nearby spots
	Return,			// walking back the way he came to his route
	Alert,			// he saw you: chasing (or standing and pointing, bChaseWhenAlerted off)
	Caught,			// he reached you while chasing; holds until Reset
	Distracted,		// a Lure: looking at a point for its seconds (he walks there first if asked)
	Downed			// knocked out: no senses, no moves
};

/** What OnEvent reports. */
UENUM(BlueprintType)
enum class EGuardEvent : uint8
{
	Noticed,		// a glimpse: he turned to look (PATROL -> SUSPICIOUS)
	HeardNoise,		// he heard something and is going to look
	Investigating,	// he set off toward a point
	Searching,		// he reached the point and looks around
	GaveUp,			// nothing found, he is going back
	BackOnPatrol,	// back on his route
	Spotted,		// the meter is full: he saw the player (once per alert)
	LostTarget,		// the chase lost the player
	BodyFound,		// he saw a downed colleague (Other = the colleague's owner)
	Caught,			// he reached the player while chasing
	KnockedDown		// KnockDown() ran
};

/** One stop of a patrol, in world space. */
USTRUCT(BlueprintType)
struct FGuardStop
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	FVector Location = FVector::ZeroVector;

	/** Seconds he stands here before the next stop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard", meta = (ClampMin = "0"))
	float WaitSeconds = 2.f;

	/** Face FaceYaw while waiting here (else he keeps the way he walked in). Two stops at one spot with different
	 *  yaws make a sentry who looks one way, then the other. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	bool bFaceYaw = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	float FaceYaw = 0.f;

	/** Glance left and right while waiting (head and torch). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	bool bGlance = true;
};

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnGuardEvent, UGuardBrainComponent* /*Guard*/, EGuardEvent /*Event*/, AActor* /*Other*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnGuardBark, UGuardBrainComponent* /*Guard*/, EGuardEvent /*Why*/, const FString& /*Line*/);

UCLASS(ClassGroup = (Take), meta = (BlueprintSpawnableComponent))
class REEL_API UGuardBrainComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGuardBrainComponent();

	/** Adds a brain to Owner (registered, ticking), or returns the one it has. Use from a scene's C++. */
	static UGuardBrainComponent* AddTo(AActor* Owner);

	/** A noise at Location: every brain within HearingRange * Loudness hears it (half that through a wall) and goes
	 *  to look, unless he is chasing. Kind is for the log and the bark ("Crash", "Throw", "Footsteps"...). */
	UFUNCTION(BlueprintCallable, Category = "Take|Guard", meta = (WorldContext = "WorldContext"))
	static void ReportNoise(const UObject* WorldContext, FVector Location, float Loudness = 1.f, FName Kind = NAME_None, AActor* Instigator = nullptr);

	/** Every live brain in World. */
	static void GetAllBrains(const UWorld* World, TArray<UGuardBrainComponent*>& Out);

	// --- the scene's controls ----------------------------------------------------------------------------------------

	/** Sets the route (world space) and starts walking it from the nearest stop. */
	void SetPatrol(const TArray<FGuardStop>& Stops);
	void SetPatrolPoints(const TArray<FVector>& Points, float WaitSeconds = 2.f);

	/** Who he watches for. Unset (and bAutoTargetPlayer): the player's pawn, whoever that is now. */
	UFUNCTION(BlueprintCallable, Category = "Take|Guard")
	void SetTarget(AActor* NewTarget) { Target = NewTarget; }

	/** On (default) he thinks and moves; off he holds still and blind (a cutscene, a held beat). */
	UFUNCTION(BlueprintCallable, Category = "Take|Guard")
	void SetBrainActive(bool bOn);

	/** A distraction: he turns to Point ("What the...?") and looks at it for Seconds, walking there first if bWalk,
	 *  then returns. Ignored while chasing or downed. */
	UFUNCTION(BlueprintCallable, Category = "Take|Guard")
	void Lure(FVector Point, float Seconds, bool bWalk = true);

	/** He sees nothing for Seconds (dazzled); his hearing still works. */
	UFUNCTION(BlueprintCallable, Category = "Take|Guard")
	void Blind(float Seconds);

	/** Knocked out: he stops dead, senses off; colleagues who see him (while his owner is not hidden) raise the alarm.
	 *  Hide the owner (SetActorHiddenInGame) once the body is stashed. */
	UFUNCTION(BlueprintCallable, Category = "Take|Guard")
	void KnockDown();

	/** Straight to ALERT on the player (a scripted alarm). */
	UFUNCTION(BlueprintCallable, Category = "Take|Guard")
	void ForceAlert();

	/** Back to the start of his patrol where he was placed (or his first stop), calm, meter empty: for a retake. */
	UFUNCTION(BlueprintCallable, Category = "Take|Guard")
	void ResetBrain();

	UFUNCTION(BlueprintCallable, Category = "Take|Guard")
	EGuardState GetState() const { return State; }

	/** 0..1; 1 = detected. */
	UFUNCTION(BlueprintCallable, Category = "Take|Guard")
	float GetSuspicion() const { return Suspicion; }

	/** He has the player in sight this frame (inside the cone, not blocked, not blind). */
	UFUNCTION(BlueprintCallable, Category = "Take|Guard")
	bool SeesTarget() const { return bSeesTarget; }

	UFUNCTION(BlueprintCallable, Category = "Take|Guard")
	bool IsDowned() const { return State == EGuardState::Downed; }

	/** The player's light level last sampled (0 dark .. 1 lit), for a HUD "you are in shadow" cue. */
	float GetTargetLight() const { return TargetLight01; }

	static const TCHAR* StateName(EGuardState S);

	/** Makes the icon, bark text and torch (BeginPlay does it; call again after turning bSpawnTorch on later). */
	void EnsureCues();

	FOnGuardEvent OnEvent;
	/** A line to speak (route it to the scene's Say for a subtitle and voice). The "?"/"!" over his head shows it too. */
	FOnGuardBark OnBark;

	// --- tunables: sight ---------------------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Sight")
	bool bAutoTargetPlayer = true;

	/** How far he can see at all, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Sight", meta = (ClampMin = "100"))
	float SightRange = 1500.f;

	/** The cone where he sees at full rate, degrees either side of his look. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Sight", meta = (ClampMin = "1", ClampMax = "89"))
	float InnerHalfAngle = 30.f;

	/** The wider band where he sees slowly ("out of the corner of his eye"), degrees either side. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Sight", meta = (ClampMin = "1", ClampMax = "120"))
	float PeripheralHalfAngle = 55.f;

	/** Peripheral band: share of the full rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Sight", meta = (ClampMin = "0", ClampMax = "1"))
	float PeripheralRate = 0.35f;

	/** Closer than this he notices almost at once (whatever the light), cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Sight", meta = (ClampMin = "0"))
	float CloseRange = 300.f;

	/** Seconds of a clear, lit, standing, moving view to fill the meter at CloseRange and at SightRange (it scales
	 *  between them). The forgiving defaults give a cold player time to see the "?" and duck back. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Sight", meta = (ClampMin = "0.1"))
	float NearDetectSeconds = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Sight", meta = (ClampMin = "0.1"))
	float FarDetectSeconds = 4.f;

	/** His eyes above his feet, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Sight")
	float EyeHeight = 165.f;

	/** Crouched: rate x this, and his sight reaches CrouchRangeScale as far. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Sight", meta = (ClampMin = "0", ClampMax = "1"))
	float CrouchRate = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Sight", meta = (ClampMin = "0.1", ClampMax = "1"))
	float CrouchRangeScale = 0.7f;

	/** The player standing still: rate x this. Running: x RunRate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Sight", meta = (ClampMin = "0", ClampMax = "1"))
	float StillRate = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Sight", meta = (ClampMin = "1"))
	float RunRate = 1.4f;

	/** Seconds after he loses sight before the meter starts to drain, and the drain per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Sight", meta = (ClampMin = "0"))
	float DrainDelay = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Sight", meta = (ClampMin = "0"))
	float DrainPerSecond = 0.3f;

	/** The meter at which he stops and looks (PATROL -> SUSPICIOUS), and at which losing sight sends him to look. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Sight", meta = (ClampMin = "0", ClampMax = "1"))
	float NoticeThreshold = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Sight", meta = (ClampMin = "0", ClampMax = "1"))
	float InvestigateThreshold = 0.4f;

	/** SUSPICIOUS with nothing more to see: he goes back after this long, s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Sight", meta = (ClampMin = "0"))
	float SuspiciousHoldSeconds = 2.5f;

	/** After finding a body he is on edge for this long: rate x EdgyRate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Sight", meta = (ClampMin = "0"))
	float EdgySeconds = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Sight", meta = (ClampMin = "1"))
	float EdgyRate = 1.5f;

	// --- tunables: light ---------------------------------------------------------------------------------------------

	/** The lights around the player slow him down in the dark: off = always lit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Light")
	bool bUseLight = true;

	/** >= 0: use this light level (0..1) instead of sampling the lights (a scene that knows its own lighting). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Light")
	float LightOverride = -1.f;

	/** Lux at the player that counts as fully lit; the floor every place has; the rate in full dark. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Light", meta = (ClampMin = "0.1"))
	float LitLux = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Light", meta = (ClampMin = "0", ClampMax = "1"))
	float AmbientLight = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Light", meta = (ClampMin = "0", ClampMax = "1"))
	float DarkRate = 0.35f;

	/** The strongest lights on the player are traced for shadow (a crate between him and the lamp); this many. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Light", meta = (ClampMin = "0", ClampMax = "8"))
	int32 ShadowTraces = 3;

	/** Seconds between light samples. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Light", meta = (ClampMin = "0.05"))
	float LightSampleSeconds = 0.25f;

	// --- tunables: hearing -------------------------------------------------------------------------------------------

	/** A Loudness-1 noise carries this far, cm (half through a wall). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Hearing", meta = (ClampMin = "0"))
	float HearingRange = 1400.f;

	/** The player's footsteps: running (faster than RunSpeed) is heard this far, walking this far, crouched not at all. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Hearing", meta = (ClampMin = "0"))
	float RunNoiseRange = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Hearing", meta = (ClampMin = "0"))
	float WalkNoiseRange = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Hearing", meta = (ClampMin = "0"))
	float RunSpeed = 320.f;

	/** He turns toward a noise for this long before he sets off (the telegraph), s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Hearing", meta = (ClampMin = "0"))
	float NoiseReactSeconds = 0.8f;

	// --- tunables: moving --------------------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Move", meta = (ClampMin = "10"))
	float PatrolSpeed = 140.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Move", meta = (ClampMin = "10"))
	float InvestigateSpeed = 190.f;

	/** Chasing: keep it a little under the player's speed (250 walking) so a judge can still break away. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Move", meta = (ClampMin = "10"))
	float ChaseSpeed = 235.f;

	/** Turning on the spot, degrees per second (an owner that is not an ACastMember; a cast member turns itself). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Move", meta = (ClampMin = "10"))
	float TurnRate = 240.f;

	/** At a point within this, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Move", meta = (ClampMin = "5"))
	float AcceptRadius = 45.f;

	/** He stops this short of a wall between him and where he wants to go, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Move", meta = (ClampMin = "0"))
	float WallStandOff = 70.f;

	/** The route, in world space. Empty: he holds where he was placed, facing the way he was placed, glancing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Move")
	TArray<FGuardStop> Patrol;

	/** Patrol back and forth (A B C B A) instead of looping (A B C A). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Move")
	bool bPingPong = false;

	/** A glance while waiting: how far the head turns, degrees, and how long each look lasts, s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Move", meta = (ClampMin = "0", ClampMax = "90"))
	float GlanceDegrees = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Move", meta = (ClampMin = "0.2"))
	float GlanceSeconds = 1.6f;

	// --- tunables: search and chase ---------------------------------------------------------------------------------

	/** How long he searches at the point, s; how many nearby spots he walks to; how far they are, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Search", meta = (ClampMin = "0"))
	float SearchSeconds = 7.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Search", meta = (ClampMin = "0", ClampMax = "6"))
	int32 SearchSpots = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Search", meta = (ClampMin = "50"))
	float SearchRadius = 400.f;

	/** Full detection: chase the player (off: he stands, points and calls it; the scene acts on OnEvent Spotted). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Search")
	bool bChaseWhenAlerted = true;

	/** Out of sight this long while chasing: he has lost you and goes to look where he saw you last, s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Search", meta = (ClampMin = "0"))
	float LoseTargetSeconds = 2.5f;

	/** Chasing and this close: CAUGHT, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Search", meta = (ClampMin = "0"))
	float CatchRadius = 110.f;

	/** A downed colleague: he sees him within this range, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Search", meta = (ClampMin = "0"))
	float BodySightRange = 1200.f;

	/** Finding a body sends him straight to ALERT (off: he goes to it and searches, on edge). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Search")
	bool bBodyRaisesAlarm = false;

	// --- tunables: telegraphs ----------------------------------------------------------------------------------------

	/** The "?" / "!" over his head: on, height above his feet, size. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Cues")
	bool bShowIcon = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Cues")
	float IconHeight = 215.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Cues", meta = (ClampMin = "1"))
	float IconSize = 60.f;

	/** A torch of his own that follows his look and goes amber (suspicious) and red (alert). Off when the scene lights
	 *  him itself; set TorchLight to have the brain aim and tint the scene's own spot light instead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Cues")
	bool bSpawnTorch = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Cues", meta = (ClampMin = "0"))
	float TorchCandelas = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Cues")
	FLinearColor TorchCalm = FLinearColor(1.f, 0.92f, 0.78f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Cues")
	FLinearColor TorchSuspicious = FLinearColor(1.f, 0.7f, 0.3f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Cues")
	FLinearColor TorchAlert = FLinearColor(1.f, 0.25f, 0.18f);

	/** How fast the torch swings to where he looks, per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Cues", meta = (ClampMin = "0.1"))
	float TorchSwingRate = 6.f;

	/** The scene's own spot light for him to aim and tint (optional). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Cues")
	TObjectPtr<USpotLightComponent> TorchLight;

	/** Cast-member gestures for the telegraphs (None = no gesture). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Cues")
	FName NoticeGesture = TEXT("LookAt");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Cues")
	FName SearchGesture = TEXT("LookAround");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Cues")
	FName AlertGesture = TEXT("Refuse");

	/** Barks: one is picked at random (never the same twice running). Empty = silent for that moment. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Barks")
	TArray<FString> NoticeBarks = { TEXT("Huh?"), TEXT("Hm? Who's there?"), TEXT("What was that...?") };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Barks")
	TArray<FString> NoiseBarks = { TEXT("What was that?"), TEXT("Did you hear that?"), TEXT("Something over there...") };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Barks")
	TArray<FString> SearchBarks = { TEXT("Come out, come out..."), TEXT("I know someone's here."), TEXT("Where are you...") };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Barks")
	TArray<FString> GiveUpBarks = { TEXT("Must've been nothing."), TEXT("Just the old building settling."), TEXT("Rats. Big rats.") };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Barks")
	TArray<FString> SpottedBarks = { TEXT("Hey! You there!"), TEXT("Stop right there!"), TEXT("Intruder!") };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Barks")
	TArray<FString> LostBarks = { TEXT("Where'd he go?"), TEXT("Lost him..."), TEXT("He can't have gone far.") };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Barks")
	TArray<FString> BodyBarks = { TEXT("Man down! Man down!"), TEXT("Eddie? Eddie, wake up!") };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Barks")
	TArray<FString> LureBarks = { TEXT("What the...?"), TEXT("Who's messing with that?") };

	/** Show the bark as world text under the icon too (on when nothing is bound to OnBark). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Barks")
	bool bWorldBarks = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard|Barks", meta = (ClampMin = "0.2"))
	float BarkSeconds = 2.2f;

	/** Log every state change (LogTake). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	bool bLog = true;

	/** Readable name in the log and the debug text. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guard")
	FString GuardName;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// senses
	void Sense(float Dt);
	bool LineClear(const FVector& From, const FVector& To, const AActor* IgnoreB = nullptr) const;
	float SampleLight(const FVector& At) const;
	void RefreshLights();
	void CheckBodies();
	void HearNoise(const FVector& Where, float Loudness, FName Kind, AActor* Instigator);

	// states
	void Enter(EGuardState S);
	void StepState(float Dt);
	void StepPatrol(float Dt);
	void StepSearch(float Dt);
	void GoInvestigate(const FVector& Point, bool bTurnFirst);
	void BeginSearch();
	void BeginReturn();
	FVector Reachable(const FVector& Want) const;

	// body
	bool MoveTo(const FVector& Dest, float Speed, float Dt);	// true on arrival
	void StopMoving();
	void FaceToward(const FVector& Point);
	void FaceYawTo(float Yaw);
	void HeadLook(const FVector& Point, float Seconds);
	void Gesture(FName G);
	FVector Feet() const;
	FVector Eye() const;
	float LookYaw() const;
	void StepTurn(float Dt);

	// cues
	void Bark(EGuardEvent Why, const TArray<FString>& Lines);
	void Emit(EGuardEvent E, AActor* Other = nullptr);
	void UpdateCues(float Dt);
	void DrawDebug() const;

	AActor* GetTarget() const;
	bool TargetCrouched(const AActor* T) const;
	void TargetPoints(const AActor* T, FVector& Head, FVector& Chest, FVector& TFeet) const;

	TWeakObjectPtr<AActor> Target;
	TWeakObjectPtr<ACastMember> CastBody;	// the owner, when it is a cast member (it walks and turns itself)

	EGuardState State = EGuardState::Patrol;
	float StateTime = 0.f;
	float Suspicion = 0.f;
	float SinceSeen = 100.f;
	bool bSeesTarget = false;
	bool bActive = true;
	float BlindLeft = 0.f;
	float EdgyLeft = 0.f;
	FVector LastSeen = FVector::ZeroVector;
	FVector Stimulus = FVector::ZeroVector;	// where he is looking / going (the glimpse, the noise, the lure)

	// patrol
	int32 StopIndex = 0;
	int32 StopDir = 1;
	float WaitLeft = 0.f;
	bool bAtStop = false;
	FVector HomeLocation = FVector::ZeroVector;
	float HomeYaw = 0.f;
	float GlanceClock = 0.f;
	int32 GlanceStep = 0;

	// investigate / search / return
	FVector Goal = FVector::ZeroVector;
	float ReactLeft = 0.f;
	float LureLeft = 0.f;
	bool bLureWalk = false;
	TArray<FVector> SearchPoints;
	int32 SearchIndex = 0;
	float SearchLeft = 0.f;
	float SearchLookClock = 0.f;
	TArray<FVector> Crumbs;		// where he walked since he left his route (the way back)
	TArray<FVector> Trail;		// where the player was while he was chasing (the way to follow)
	float TrailClock = 0.f;
	bool bCaughtSent = false;
	bool bSpottedSent = false;
	TSet<TWeakObjectPtr<UGuardBrainComponent>> FoundBodies;

	// moving (an owner that is not a cast member)
	FVector WalkGoal = FVector(FLT_MAX);
	float WalkResend = 0.f;
	float WantYaw = 0.f;
	bool bTurningSelf = false;
	float HeadYawOffset = 0.f;	// the glance, added to his facing for sight and the torch

	// light
	TArray<TWeakObjectPtr<ULocalLightComponent>> Lights;
	float LightClock = 0.f;
	float LightsRefreshClock = 0.f;
	float TargetLight01 = 1.f;
	float NoiseClock = 0.f;

	// cues
	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> Icon;

	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> BarkText;

	UPROPERTY(Transient)
	TObjectPtr<USpotLightComponent> OwnTorch;

	float BarkLeft = 0.f;
	FString LastBark;
	float IconPulse = 0.f;
	FRotator TorchRot = FRotator::ZeroRotator;
};
