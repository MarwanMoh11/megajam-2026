// Scene 7, part 2 (tag Part=2). INT. PROJECTION BOOTH - CONTINUOUS (Vance, 35mm). Owner: s7-route (docs/script/s7.md).
// Part 0 (AScene7Route) walks Vance from the rigged door to the projector mark in the booth over the lobby doors. This
// actor sits on the booth's floor, +X toward the viewing glass, and carries the booth's set (projector, magazine,
// reels, canister, the doors, the alarm) and the rest of Scene 7:
//   SWAP     s7-reelswap's part 1 plays the swap (flag Canister); without it, the old hold-[E] swap (1.2 s).
//   VENUE    cut to the auditorium: the film dies on the screen, Pendleton at the podium looks up at the booth:
//            "Lock the building. Nobody leaves." The alarm, red strobes, the house screams.
//   ESCAPE   back on Vance: the booth's stair door has mag-locked (red), so out by the gallery door onto the lobby's
//            balcony (the player runs). A cut: Pendleton and his chief bodyguard coming up the grand stair. Vance
//            slips west along the balcony into the security room.
//   CAUGHT   they come in behind him; Pendleton's line.
//   then FinishScene: s7-fight's part 3 (AScene7Fight) fights on these marks, then Scene 8 (s8-escape).
// Nothing teleports Vance: he starts on the projector mark and walks every step to the fight's mark.

#pragma once

#include "CoreMinimal.h"
#include "Script/ScriptScene.h"
#include "Script/Scene7Route.h"
#include "Scene7Booth.generated.h"

class UStaticMeshComponent;
class USkeletalMeshComponent;
class UPointLightComponent;
class UAudioComponent;
class UAnimationAsset;
class USkeletalMesh;
class UStaticMesh;
class UMaterialInterface;
class UNiagaraSystem;
class USoundBase;
class ACameraActor;
class ATakeCharacter;

/** One fixed camera of the scene, in the booth actor's space. */
USTRUCT(BlueprintType)
struct FScene7Shot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene7")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene7")
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene7")
	float Fov = 40.f;
};

UENUM()
enum class EScene7Stage : uint8
{
	Approach,		// a cold jump: onto the projector mark
	Swap,			// the swap (s7-reelswap's actor, or the hold-[E] fallback)
	Swapped,		// the canister under his arm, a breath
	Venue,			// the cut to the auditorium: the film dies, Pendleton, the alarm
	Escape,			// the player runs Vance out across the balcony to the security room
	Caught,			// Pendleton and the bodyguard come in behind him
	Done
};

UCLASS()
class REEL_API AScene7Booth : public AScriptScene
{
	GENERATED_BODY()

public:
	AScene7Booth();

	virtual void BeginPlay() override;
	virtual void BeginScene() override;
	virtual void TickScene(float DeltaSeconds) override;
	virtual void EndScene() override;
	virtual void ResetScene() override;
	virtual bool HandleSceneKey(const FKey& Key, bool bPressed) override;
	virtual void DrawSceneHud(UCanvas* Canvas) override;

	/** For Scene 8: the silver canister goes under Character's arm (it stays there until ResetScene here). Finds the
	 *  booth in the world; false if there is none. Call it from Scene 8's BeginScene. */
	static bool AttachCanisterTo(const UObject* WorldContextObject, ATakeCharacter* Character);

	UStaticMeshComponent* GetCanister() const { return Canister; }

	// ---- the set (components; their transforms are tuned on the placed instance by tools/script/scene7.py) ----
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UStaticMeshComponent> Projector;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UStaticMeshComponent> Magazine;		// the upper magazine's drum
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<USceneComponent> MagazineHinge;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UStaticMeshComponent> MagazineLid;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UStaticMeshComponent> MasterReel;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UStaticMeshComponent> DummyReel;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UStaticMeshComponent> LowerReel;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UStaticMeshComponent> Canister;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UStaticMeshComponent> CanisterLid;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UStaticMeshComponent> AlarmBox;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UStaticMeshComponent> AlarmLever;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UStaticMeshComponent> Glass;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<USceneComponent> DoorNorthHinge;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UStaticMeshComponent> DoorNorth;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<USceneComponent> DoorSouthHinge;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UStaticMeshComponent> DoorSouth;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UStaticMeshComponent> Stiletto;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UStaticMeshComponent> Grenade;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UStaticMeshComponent> CurtainHeap;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TArray<TObjectPtr<UStaticMeshComponent>> Shards;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TArray<TObjectPtr<UStaticMeshComponent>> SmokePuffs;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UPointLightComponent> LampSpill;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UPointLightComponent> StrobeA;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UPointLightComponent> StrobeB;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UPointLightComponent> FireLight;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UPointLightComponent> KeyLight;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<USkeletalMeshComponent> Pendleton;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<USkeletalMeshComponent> Bodyguard;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<USkeletalMeshComponent> Cole;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UAudioComponent> HumAudio;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UAudioComponent> AlarmAudio;

	// ---- marks (booth space) ----
	/** Where Vance stands to work the magazine, and his yaw there (booth space). */
	UPROPERTY(EditAnywhere, Category = "Scene7|Marks") FVector UseSpot = FVector(-60.f, -150.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Scene7|Marks") float UseYaw = 0.f;
	/** How near UseSpot (cm, flat) the Use prompt shows and the hold works. */
	UPROPERTY(EditAnywhere, Category = "Scene7|Marks") float UseRadius = 140.f;
	/** Vance's place and yaw for the fight. */
	UPROPERTY(EditAnywhere, Category = "Scene7|Marks") FVector FightSpot = FVector(-30.f, 40.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Scene7|Marks") float FightYaw = 90.f;
	UPROPERTY(EditAnywhere, Category = "Scene7|Marks") FVector PendletonDoor = FVector(110.f, 560.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Scene7|Marks") FVector PendletonSpot = FVector(60.f, 300.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Scene7|Marks") FVector BodyguardDoor = FVector(110.f, 620.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Scene7|Marks") FVector BodyguardSpot = FVector(-40.f, 220.f, 0.f);
	/** Where Pendleton stands to slam the alarm (by AlarmBox). */
	UPROPERTY(EditAnywhere, Category = "Scene7|Marks") FVector AlarmSpot = FVector(-110.f, 420.f, 0.f);
	/** The bodyguard's head meets the projector housing here. */
	UPROPERTY(EditAnywhere, Category = "Scene7|Marks") FVector SlamSpot = FVector(10.f, -60.f, 0.f);
	/** Pendleton's throw: through the glass at GlassPoint, down to the curtain heap at LandSpot (booth space). */
	UPROPERTY(EditAnywhere, Category = "Scene7|Marks") FVector GlassPoint = FVector(190.f, 150.f, 140.f);
	UPROPERTY(EditAnywhere, Category = "Scene7|Marks") FVector LandSpot = FVector(330.f, 150.f, -440.f);
	UPROPERTY(EditAnywhere, Category = "Scene7|Marks") FVector ColeDoor = FVector(110.f, -580.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Scene7|Marks") FVector ColeSpot = FVector(110.f, -420.f, 0.f);
	/** The smoke grenade lands here (down the stairwell, outside the south door). */
	UPROPERTY(EditAnywhere, Category = "Scene7|Marks") FVector SmokeSpot = FVector(110.f, -800.f, -150.f);
	/** The world point the "film burns out" shot looks at (the screen); used only if the shot is missing. */
	UPROPERTY(EditAnywhere, Category = "Scene7|Marks") FVector ScreenWorld = FVector(4388.f, 0.f, 600.f);

	// ---- the canister under his arm ----
	UPROPERTY(EditAnywhere, Category = "Scene7|Canister") FName CanisterBone = TEXT("lowerarm_l");
	UPROPERTY(EditAnywhere, Category = "Scene7|Canister") FTransform CanisterInArm;
	/** For the block: the canister comes up in the right hand. */
	UPROPERTY(EditAnywhere, Category = "Scene7|Canister") FName CanisterBlockBone = TEXT("hand_r");
	UPROPERTY(EditAnywhere, Category = "Scene7|Canister") FTransform CanisterInHand;
	UPROPERTY(EditAnywhere, Category = "Scene7|Canister") FName StilettoBone = TEXT("hand_r");
	UPROPERTY(EditAnywhere, Category = "Scene7|Canister") FTransform StilettoInHand;

	// ---- cameras (booth space): Approach, Swap, Door, Insert, Line, Alarm, Screen, Parry, ParryHit, Block, BlockHit,
	//      Throw, Fall, Cole, Miss ----
	UPROPERTY(EditAnywhere, Category = "Scene7|Shots") TMap<FName, FScene7Shot> Shots;

	// ---- timing (game seconds unless said) ----
	/** The heist's swap: hold Use this long at the magazine. */
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float SwapHoldSeconds = 1.2f;
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float SwappedSeconds = 0.9f;
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float DoorSeconds = 2.2f;
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float LineSeconds = 3.0f;
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float AlarmSeconds = 4.0f;
	/** The attacker's wind-up before slow time and the prompt. */
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float WindupSeconds = 0.45f;
	/** Real seconds the prompt stays up (slow time). */
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float PromptRealSeconds = 1.8f;
	/** First go of each prompt: a little more time. Retakes: PromptRealSeconds. */
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float FirstPromptBonus = 0.6f;
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float PromptSlowScale = 0.12f;
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float StrikeSeconds = 1.6f;
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float MissSeconds = 1.1f;
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float FallSeconds = 2.6f;
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float FallSlowScale = 0.35f;
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float ColeSeconds = 3.6f;
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float StrobeHz = 2.5f;
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float StrobeIntensity = 120.f;	// candela
	/** The reels turn this fast while the projector runs (local rotation per second). */
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") FRotator ReelSpin = FRotator(0.f, 0.f, 240.f);
	/** The magazine lid opens this far (yaw about its hinge, degrees). */
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float LidOpenDegrees = -110.f;
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float DoorOpenDegrees = -100.f;
	/** Projector housing glow while the film runs (the lamp spill, candela). */
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float LampCandela = 60.f;

	// ---- the prompts (keys and words) ----
	UPROPERTY(EditAnywhere, Category = "Scene7|Prompts") TArray<FKey> PromptKeys;
	UPROPERTY(EditAnywhere, Category = "Scene7|Prompts") TArray<FString> PromptWords;

	// ---- lines ----
	UPROPERTY(EditAnywhere, Category = "Scene7|Lines") FString PendletonLine = TEXT("Lock the building. Nobody leaves.");
	UPROPERTY(EditAnywhere, Category = "Scene7|Lines") FString ColeLine = TEXT("Max is out back! Move!");
	UPROPERTY(EditAnywhere, Category = "Scene7|Lines") FString SwapHint = TEXT("Hold [E] at the projector: swap the reel");

	// ---- assets (defaults set in the constructor; the overlay may change them) ----
	UPROPERTY(EditAnywhere, Category = "Scene7|Assets") TObjectPtr<USkeletalMesh> PendletonMesh;
	UPROPERTY(EditAnywhere, Category = "Scene7|Assets") TObjectPtr<USkeletalMesh> BodyguardMesh;
	UPROPERTY(EditAnywhere, Category = "Scene7|Assets") TObjectPtr<USkeletalMesh> ColeMesh;
	UPROPERTY(EditAnywhere, Category = "Scene7|Assets") TMap<FName, TObjectPtr<UAnimationAsset>> Anims;
	UPROPERTY(EditAnywhere, Category = "Scene7|Assets") TObjectPtr<UNiagaraSystem> SparksFX;
	UPROPERTY(EditAnywhere, Category = "Scene7|Assets") TObjectPtr<UNiagaraSystem> FireFX;
	UPROPERTY(EditAnywhere, Category = "Scene7|Assets") TObjectPtr<UNiagaraSystem> GlassFX;
	UPROPERTY(EditAnywhere, Category = "Scene7|Assets") TObjectPtr<UMaterialInterface> SmokeMaterial;
	/** Scene 7's sounds live in /Game/Take/Audio/Script/S7/<name>; missing ones fall back to these take cues. */
	UPROPERTY(EditAnywhere, Category = "Scene7|Assets") TMap<FName, FName> CueFallbacks;
	// ---- the venue, the escape, the catch (world space) ----
	/** Cameras in world space: Venue (the screen and the podium), Pendleton (close on him), Stair (the grand stair from
	 *  the balcony), Caught (inside the security room, on its door). */
	UPROPERTY(EditAnywhere, Category = "Scene7|World") TMap<FName, FScene7Shot> WorldShots;
	/** Pendleton at the podium for the venue cut (feet), and his yaw (facing the house and the booth). */
	UPROPERTY(EditAnywhere, Category = "Scene7|World") FVector PodiumSpot = FVector(3890.f, -480.f, 20.f);
	UPROPERTY(EditAnywhere, Category = "Scene7|World") float PodiumYaw = 180.f;
	/** Scene 5's own Pendleton (and anything else with this tag) hides while Scene 7 shows ours. */
	UPROPERTY(EditAnywhere, Category = "Scene7|World") FName VenueHideTag = TEXT("s7_venue_hide");
	/** The two strobes' places in the venue cut, then on the escape (world). */
	UPROPERTY(EditAnywhere, Category = "Scene7|World") TArray<FVector> StrobesVenue;
	UPROPERTY(EditAnywhere, Category = "Scene7|World") TArray<FVector> StrobesEscape;
	/** The escape's waypoints (the guide points at the next; reaching the first cuts to the stair). */
	UPROPERTY(EditAnywhere, Category = "Scene7|World") TArray<FScene7RouteStep> EscapeSteps;
	/** Vance runs (cm/s) on the escape: the alarm is going. */
	UPROPERTY(EditAnywhere, Category = "Scene7|World") float EscapeSpeed = 470.f;
	/** Pendleton and the bodyguard walk this path (feet, world): the grand stair's foot, up, onto the balcony. */
	UPROPERTY(EditAnywhere, Category = "Scene7|World") TArray<FVector> GuardPath;
	UPROPERTY(EditAnywhere, Category = "Scene7|World") float GuardSpeed = 230.f;
	/** The bodyguard walks this far behind Pendleton on the path (cm). */
	UPROPERTY(EditAnywhere, Category = "Scene7|World") float GuardGap = 140.f;
	/** The security room (the fight room): feet inside this box (world x/y min..max, floor z) = caught. */
	UPROPERTY(EditAnywhere, Category = "Scene7|World") FVector RoomMin = FVector(-3200.f, -2000.f, 450.f);
	UPROPERTY(EditAnywhere, Category = "Scene7|World") FVector RoomMax = FVector(-2100.f, -1460.f, 450.f);
	/** They come in by this door (outside it, then their marks). */
	UPROPERTY(EditAnywhere, Category = "Scene7|World") FVector RoomDoorOutside = FVector(-2650.f, -1250.f, 450.f);
	/** The fight's marks (feet, world) and yaws: s7-fight's AScene7Fight starts from here. */
	UPROPERTY(EditAnywhere, Category = "Scene7|World") FVector VanceFightMark = FVector(-2650.f, -1680.f, 450.f);
	UPROPERTY(EditAnywhere, Category = "Scene7|World") float VanceFightYaw = 90.f;
	UPROPERTY(EditAnywhere, Category = "Scene7|World") FVector PendletonFightMark = FVector(-2600.f, -1470.f, 450.f);
	UPROPERTY(EditAnywhere, Category = "Scene7|World") FVector BodyguardFightMark = FVector(-2730.f, -1500.f, 450.f);
	UPROPERTY(EditAnywhere, Category = "Scene7|World") float CaughtSeconds = 3.4f;
	UPROPERTY(EditAnywhere, Category = "Scene7|World") FString CaughtLine = TEXT("Going somewhere, Mr. Vance? That reel belongs to me.");
	UPROPERTY(EditAnywhere, Category = "Scene7|World") FString VenueLine = TEXT("The booth! Lock the building. Nobody leaves.");
	UPROPERTY(EditAnywhere, Category = "Scene7|World") float VenueSeconds = 6.0f;
	/** Shown when the swap is done and before the stair cut (Vance into his cuff). */
	UPROPERTY(EditAnywhere, Category = "Scene7|World") FString SwappedLine = TEXT("Le Soleil Noir. Exit, stage left.");

	/** Hide these tagged actors while the scene runs (the old box's stairs and front wall). */
	UPROPERTY(EditAnywhere, Category = "Scene7|Assets") FName HideTag = TEXT("s7_hide");

protected:
	EScene7Stage Stage = EScene7Stage::Approach;
	int32 EscapeStep = 0;
	float GuardDist = -1.f;		// how far Pendleton is along GuardPath; < 0 they wait at its start
	float SavedWalkSpeed = 0.f;
	bool bStairShot = false;	// the stair cut is up
	float StairShotT = 0.f;
	bool bSwapActor = false;	// s7-reelswap's actor plays the swap
	bool bFightActor = false;
	bool bLeftMark = false;
	float StageT = 0.f;			// game seconds in this stage
	float StageReal = 0.f;		// real seconds in this stage
	int32 PromptIndex = 0;		// 0 parry, 1 block, 2 throw
	float SwapHeld = 0.f;		// seconds Use has been held at the magazine
	bool bUseDown = false;
	bool bSwapDone = false;
	bool bCheckpointAlarm = false;	// a miss retakes from the alarm
	bool bFailing = false;			// FailScene was called by us: the next BeginScene starts at the checkpoint
	bool bAlarmOn = false;
	bool bGlassBroken = false;
	bool bSmoke = false;
	TArray<bool> FirstTry;
	TArray<FVector> ShardVel;
	FVector PendletonFallFrom = FVector::ZeroVector;
	FName CurrentShot;
	float PrevT = 0.f;			// StageT on the previous tick (At() fires once as StageT crosses a time)
	float LastSwapA = 0.f;		// swap progress last tick (its sounds fire as it crosses)
	bool bFire = false;			// the dummy film burns on the magazine
	float FireSpawnIn = 0.f;
	float SmokeT = -1.f;		// seconds since the smoke started; < 0 none
	float ShardT = -1.f;
	FVector VanceMeshRel = FVector::ZeroVector;
	bool bHaveVanceMesh = false;
	FVector BodyguardFrom = FVector::ZeroVector;
	FVector PendletonFrom = FVector::ZeroVector;

	bool At(float Time) const { return PrevT < Time && StageT >= Time; }
	void PutCanisterOn(ATakeCharacter* Char, bool bInHand);
	void ApplySwap(float A);
	void Puppet(USkeletalMeshComponent* Mesh, const FVector& Local, float FaceYaw);
	void FaceVance(float Yaw);
	class APlayerController* GetPC() const;

	UPROPERTY(Transient) TMap<FName, TObjectPtr<ACameraActor>> Cams;
	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> Hidden;
	UPROPERTY(Transient) TMap<FName, TObjectPtr<USoundBase>> SoundCache;

	// The rest pose of every moving component, captured once (ResetScene restores it).
	TMap<USceneComponent*, FTransform> Rest;

	void SetStage(EScene7Stage NewStage);
	void PuppetWorld(USkeletalMeshComponent* Mesh, const FVector& World, float FaceYaw);
	FVector PathPoint(float Dist, FVector* OutDir = nullptr) const;
	void PlaceStrobes(const TArray<FVector>& At);
	void SetVenueHidden(bool bHide);
	void PointGuide();
	FVector VanceFeet() const;
	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> VenueHidden;
	void StartAtCheckpoint();
	void Shot(FName Name);
	void Cue(FName Name, float Volume = 1.f);
	USoundBase* FindS7Sound(FName Name);
	void Act(USkeletalMeshComponent* Mesh, FName Anim, bool bLoop, float Rate = 1.f);
	void ActVance(FName Anim, bool bLoop, float Rate = 1.f);
	void VanceBackToBlueprint();
	void PlaceVance(const FVector& Local, float Yaw);
	void PutCanisterOnVance(bool bInHand);
	void BurstAt(UNiagaraSystem* System, const FVector& World, float Scale = 1.f);
	void SetAlarm(bool bOn);
	void BreakGlass();
	void StepPuppets(float Dt);
	void StepFX(float Dt, float RealDt);
	void BeginPrompt(int32 Index);
	void PromptHit();
	void PromptMissed();
	FVector W(const FVector& Local) const { return GetActorTransform().TransformPosition(Local); }
	float Ease(float A) const { return FMath::InterpEaseInOut(0.f, 1.f, FMath::Clamp(A, 0.f, 1.f), 2.f); }
};
