// Scene 7's fist fight in the security room (docs/SCRIPT.md, Scene 7, "scripted cinematic fist fight"). Owner: the
// s7-fight session. A placed actor that s7-route's Scene 7 part 1 (AScene7Booth) drives once Vance is caught there by
// Pendleton and his chief bodyguard: Find(this)->StartFight(this), HandleFightKey from its HandleSceneKey while
// IsRunning, FinishScene when IsDone (Scene 8, s8-escape, opens with both men down), ResetFight from ResetScene. The
// fight ticks itself and draws its own HUD (AHUD::OnHUDPostRender). Placed by tools/script/scene7_fight.py on Vance's fight mark, +X the way he faces (north, the
// balcony door). It carries the fight's own cast (the bodyguard and Pendleton as puppets on part 2's marks), the
// stiletto and two camera actors it cuts between; Vance is the real POV character, animated in place.
//
// Beats: an establishing wide and a crash zoom; (1) the bodyguard's hook: [Q] PARRY, Vance slips it, the guard
// stumbles past and Vance drives his head into the monitor bank (sparks), he goes down; (2) Pendleton's switchblade:
// [E] BLOCK WITH THE CANISTER, CLANG, sparks, the blade spins away; (3) Pendleton lunges: [F] SWEEP, in slow motion
// his legs go and he crashes down. A miss lets the blow land on Vance, freezes on a "CUT!" card and retakes that beat
// (Take 2, 3...). Every press is answered on its frame (HandleFightKey).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "Scene7Fight.generated.h"

class UStaticMeshComponent;
class USkeletalMeshComponent;
class UAnimationAsset;
class USkeletalMesh;
class UNiagaraSystem;
class USoundBase;
class ACameraActor;
class ATakeCharacter;
class AScriptScene;
class UCanvas;
class AHUD;

/** What a fight camera keeps in frame. */
UENUM(BlueprintType)
enum class EFightSubject : uint8
{
	Vance,
	Guard,
	Pendleton,
	VanceGuard,			// the midpoint of the two
	VancePendleton,
	Canister,
	Stiletto
};

/** One camera set-up, in the fight actor's space. The camera aims at Subject's Bone every frame (Track = 1). */
USTRUCT(BlueprintType)
struct FFightShot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fight") FVector From = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fight") EFightSubject Subject = EFightSubject::Vance;
	/** Bone on the subject (puppets and Vance); None: the actor's middle. Ignored for Canister and Stiletto. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fight") FName Bone = TEXT("head");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fight") FVector LookOffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fight") float Fov = 40.f;
	/** The 70s zoom: Fov eases to EndFov over ZoomSeconds (game time). EndFov <= 0: no zoom. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fight") float EndFov = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fight") float ZoomSeconds = 0.5f;
	/** The camera itself drifts this far (fight space, cm) over the shot's first two seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fight") FVector Drift = FVector::ZeroVector;
	/** Handheld wobble, degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fight") float Handheld = 0.4f;
	/** 1: the aim follows the subject every frame; 0: aimed once on the cut. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fight") float Track = 1.f;
};

UENUM()
enum class EFightStage : uint8
{
	Idle,		// not started (or reset)
	Intro,		// the establishing wide and the push in on Vance
	Windup,		// the attacker winds up (Beat = which)
	Prompt,		// slow time, the key is up
	Strike,		// the press landed: Vance's move plays out
	Miss,		// the blow lands on Vance
	Cut,		// frozen on the CUT card, then the retake of this beat
	Outro,		// both down, Vance stands, the canister under his arm
	Done
};

UCLASS()
class REEL_API AScene7Fight : public AActor
{
	GENERATED_BODY()

public:
	AScene7Fight();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

	/** The placed fight in Ctx's world, or null. */
	static AScene7Fight* Find(const UObject* Ctx);
	/** Start (or restart) the fight from its establishing wide. Scene: the driving scene (Vance is its POV character,
	 *  its Say carries the lines); null: player 0's pawn. */
	void StartFight(AScriptScene* Scene);
	/** Back to before the fight: the puppets hidden, Vance back to his walk, slow time off. */
	void ResetFight();
	bool IsRunning() const { return Stage != EFightStage::Idle && Stage != EFightStage::Done; }
	/** The last blow has landed and the coverage has played out: both men down, Vance standing on his walk again. */
	bool IsDone() const { return Stage == EFightStage::Done; }
	/** From the driving scene's HandleSceneKey while IsRunning(): answers a prompt on this frame. True: consumed. */
	bool HandleFightKey(const FKey& Key, bool bPressed);

	// ---- the cast and props ----
	UPROPERTY(VisibleAnywhere, Category = "Fight") TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category = "Fight") TObjectPtr<USkeletalMeshComponent> Guard;
	UPROPERTY(VisibleAnywhere, Category = "Fight") TObjectPtr<USkeletalMeshComponent> Pendleton;
	UPROPERTY(VisibleAnywhere, Category = "Fight") TObjectPtr<UStaticMeshComponent> Stiletto;

	UPROPERTY(EditAnywhere, Category = "Fight|Assets") TObjectPtr<USkeletalMesh> GuardMesh;
	UPROPERTY(EditAnywhere, Category = "Fight|Assets") TObjectPtr<USkeletalMesh> PendletonMesh;
	/** Clips by role: Idle Stance Walk Hook Jab Slash Lunge Sweep Duck Block HitHeavy HitMed HitLight FallBack
	 *  FallFront Stand. tools/script/scene7_fight.py swaps in the Mixamo fight clips when they exist. */
	UPROPERTY(EditAnywhere, Category = "Fight|Assets") TMap<FName, TObjectPtr<UAnimationAsset>> Anims;
	UPROPERTY(EditAnywhere, Category = "Fight|Assets") TObjectPtr<UNiagaraSystem> SparksFX;
	/** Sounds by role (punch, clang, slam, fall, grunt...): /Game/Take/Audio/Script/S7 assets. */
	UPROPERTY(EditAnywhere, Category = "Fight|Assets") TMap<FName, TObjectPtr<USoundBase>> Sounds;
	/** The canister raised as a shield, on the booth's CanisterBlockBone (hand_r). Identity: the booth's CanisterInHand. */
	UPROPERTY(EditAnywhere, Category = "Fight|Assets") FTransform CanisterBlock;
	/** The switchblade in Pendleton's right hand. */
	UPROPERTY(EditAnywhere, Category = "Fight|Assets") FName StilettoBone = TEXT("hand_r");
	UPROPERTY(EditAnywhere, Category = "Fight|Assets") FTransform StilettoInHand;
	/** Actors with this tag are hidden while the fight runs (anything of part 2's left standing on the marks). */
	UPROPERTY(EditAnywhere, Category = "Fight|Assets") FName HideTag = TEXT("s7_fight_hide");

	// ---- marks (fight space, floor level; Vance stands at the origin facing +X = north, the balcony door; +Y = west;
	//      the monitor bank and desk are behind him at X -280..-295) ----
	/** Where part 2 leaves the bodyguard (he steps in to GuardSpot in the intro). */
	UPROPERTY(EditAnywhere, Category = "Fight|Marks") FVector GuardStart = FVector(180.f, 80.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Fight|Marks") FVector GuardSpot = FVector(112.f, 12.f, 0.f);
	/** The hook misses: his weight carries him past Vance to here. */
	UPROPERTY(EditAnywhere, Category = "Fight|Marks") FVector GuardStumble = FVector(-55.f, 22.f, 0.f);
	/** Face-first into the monitors here, then he drops on his back. */
	UPROPERTY(EditAnywhere, Category = "Fight|Marks") FVector GuardSlamSpot = FVector(-178.f, 12.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Fight|Marks") float GuardSlamYaw = 180.f;
	UPROPERTY(EditAnywhere, Category = "Fight|Marks") FVector PendletonStart = FVector(210.f, -50.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Fight|Marks") FVector PendletonAttack = FVector(100.f, -20.f, 0.f);
	/** Where Pendleton lands after the sweep. */
	UPROPERTY(EditAnywhere, Category = "Fight|Marks") FVector PendletonLand = FVector(165.f, -25.f, 0.f);
	/** How far Vance is driven back by a blow that lands (cm). */
	UPROPERTY(EditAnywhere, Category = "Fight|Marks") float MissKnockBack = 55.f;
	UPROPERTY(EditAnywhere, Category = "Fight|Marks") float GuardScale = 1.12f;

	// ---- coverage: Wide VanceCU OTSGuard DuckLow Reverse SlamCU GuardDown BladeInsert Profile BlockCU LungeWide
	//      SweepLow Overhead Final MissGuard MissBlade MissLunge ----
	UPROPERTY(EditAnywhere, Category = "Fight|Shots") TMap<FName, FFightShot> Shots;

	// ---- the prompts ----
	UPROPERTY(EditAnywhere, Category = "Fight|Prompts") TArray<FKey> PromptKeys;
	UPROPERTY(EditAnywhere, Category = "Fight|Prompts") TArray<FString> PromptWords;
	/** On the CUT card: what went wrong in each beat. */
	UPROPERTY(EditAnywhere, Category = "Fight|Prompts") TArray<FString> MissWords;

	// ---- timing (game seconds unless said) ----
	UPROPERTY(EditAnywhere, Category = "Fight|Timing") float IntroSeconds = 2.4f;
	/** Each beat's wind-up before the prompt (hook, blade, lunge). */
	UPROPERTY(EditAnywhere, Category = "Fight|Timing") TArray<float> WindupSeconds;
	/** Real seconds the key stays up. The first go of each beat (on Take 1) gets FirstPromptBonus more. */
	UPROPERTY(EditAnywhere, Category = "Fight|Timing") float PromptRealSeconds = 1.6f;
	UPROPERTY(EditAnywhere, Category = "Fight|Timing") float FirstPromptBonus = 0.9f;
	UPROPERTY(EditAnywhere, Category = "Fight|Timing") float PromptSlowScale = 0.08f;
	/** Each beat's strike, start to the next beat. */
	UPROPERTY(EditAnywhere, Category = "Fight|Timing") TArray<float> StrikeSeconds;
	/** The sweep plays in slow motion at this scale. */
	UPROPERTY(EditAnywhere, Category = "Fight|Timing") float SweepSlowScale = 0.3f;
	/** Hit-stop on every contact: time nearly stops for this many real seconds. */
	UPROPERTY(EditAnywhere, Category = "Fight|Timing") float HitStopReal = 0.08f;
	UPROPERTY(EditAnywhere, Category = "Fight|Timing") float MissSeconds = 0.9f;
	/** Real seconds frozen on the CUT card before the retake. */
	UPROPERTY(EditAnywhere, Category = "Fight|Timing") float CutRealSeconds = 1.7f;
	UPROPERTY(EditAnywhere, Category = "Fight|Timing") float OutroSeconds = 2.2f;
	/** Camera kick on contact (degrees) and its fall-off (per real second). */
	UPROPERTY(EditAnywhere, Category = "Fight|Timing") float ImpactKick = 2.6f;
	UPROPERTY(EditAnywhere, Category = "Fight|Timing") float KickDecay = 7.f;

	// ---- lines ----
	UPROPERTY(EditAnywhere, Category = "Fight|Lines") FString PendletonLine = TEXT("Take it off him.");

protected:
	EFightStage Stage = EFightStage::Idle;
	int32 Beat = 0;				// 0 hook, 1 blade, 2 lunge
	int32 Take = 1;				// the slate: 1 + the misses so far
	float StageT = 0.f;			// game seconds in this stage
	float PrevT = 0.f;
	float StageReal = 0.f;		// real seconds in this stage
	float FightT = 0.f;			// game seconds since StartFight (the slides run on it, across stages)
	float BladeT = -1.f;		// game seconds since the switchblade flicked open; < 0 closed
	float HitStopLeft = 0.f;	// real seconds of hit-stop left
	float Kick = 0.f;			// camera kick, degrees, decaying
	float FlashRed = 0.f;		// the miss flash, 0-1, decaying in real time
	float Stamp = 0.f;			// the "hit" stamp under the prompt, 0-1
	FString StampWords;
	bool bFirstGo[3] = { true, true, true };

	// Puppet motion: each slides from A to B over a span of the stage (game seconds), easing.
	struct FSlide { FVector From = FVector::ZeroVector; FVector To = FVector::ZeroVector; float FromYaw = 0.f; float Yaw = 0.f; float T0 = 0.f; float T1 = 0.f; bool bOn = false; };
	FSlide GuardSlide, PendSlide, VanceSlide;
	FVector VancePos = FVector::ZeroVector;	// fight space
	float VanceYaw = 0.f;
	FVector GuardPos, PendPos;
	float GuardYaw = 180.f, PendYaw = 180.f;

	// The thrown stiletto.
	FVector BladeVel = FVector::ZeroVector;
	bool bBladeFlying = false;

	// Cameras: two actors so every change is a hard cut.
	UPROPERTY(Transient) TObjectPtr<ACameraActor> CamA;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> CamB;
	bool bOnA = false;
	FName CurShot;
	float ShotT = 0.f;			// game seconds on this shot
	FVector ShotAim = FVector::ZeroVector;

	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> Hidden;
	FVector VanceMeshRel = FVector::ZeroVector;
	bool bHaveVanceMesh = false;

	ATakeCharacter* GetVance() const;
	class APlayerController* GetPC() const;
	FVector W(const FVector& Local) const { return GetActorTransform().TransformPosition(Local); }
	float WYaw(float Yaw) const { return GetActorRotation().Yaw + Yaw; }
	bool At(float Time) const { return PrevT < Time && StageT >= Time; }
	static float Ease(float A) { return FMath::InterpEaseInOut(0.f, 1.f, FMath::Clamp(A, 0.f, 1.f), 2.f); }

	void SetStage(EFightStage NewStage);
	void BeginBeat(int32 Index);
	void BeginPrompt();
	void Answer(bool bRight);
	void StartStrike();
	void StartMiss();
	void ActEnd(USkeletalMeshComponent* Mesh, FName Anim);
	UAnimationAsset* FindAnim(FName Anim) const;
	float YawTo(const FVector& From, const FVector& To) const { return (To - From).Rotation().Yaw; }
	void Shot(FName Name);
	void StepCamera(float Dt, float RealDt);
	FVector SubjectPoint(EFightSubject Subject, FName Bone) const;
	void Act(USkeletalMeshComponent* Mesh, FName Anim, bool bLoop, float Rate = 1.f);
	void ActVance(FName Anim, bool bLoop, float Rate = 1.f);
	void Slide(FSlide& S, const FVector& From, const FVector& To, float FromYaw, float Yaw, float T0, float T1);
	void StepSlides();
	void PlaceAll();
	void Impact(const FVector& World, FName Sound, float KickAmount, bool bSparks);
	void Sfx(FName Name, float Volume = 1.f);
	void CanisterToHand(bool bInHand);
	void SetSlow(FName Reason, bool bOn, float Scale, float EaseSeconds);
	void ReleaseVance(bool bToWalk);
	void TickStage(float Dt);
	void BeginFight();
	void ClearFight(bool bHideCast);
	void DrawHud(AHUD* Hud, UCanvas* Canvas);
	UPROPERTY(Transient) TObjectPtr<AScriptScene> OwnerScene;
	FDelegateHandle HudHandle;
	FVector BoneOr(USkeletalMeshComponent* Mesh, FName Bone, const FVector& Fallback) const;
};
