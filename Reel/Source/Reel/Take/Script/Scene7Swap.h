// Scene 7's reel swap: Vance steals the master print of Le Soleil Noir from the projector. Owner: s7-reelswap.
//
// One placed actor carries the whole beat: the booth projector (built in code, Scene7SwapMesh) with its upper
// magazine, the master and dummy reels, the silver canister, the rewind bench, the beam, a cine camera and a double
// of Vance whose hands are put on the props by two-bone IK (Scene7SwapAnim). The actor's origin is the floor under
// the projector; +X is the lens (toward the port glass), the operator works on the -Y side, the bench is behind him.
//
// Four presses of [E], each a beat with its own coverage, cut on the frame of the press:
//   1 OPEN    wide, then an insert on his hand at the magazine's latch; the door swings, his hand stops the reel
//   2 LIFT    over his shoulder into the magazine: he pinches off the film and lifts the print off the spindle; a
//             medium as he turns to the bench and lays it in the open canister
//   3 DUMMY   high on the bench: the weighted dummy comes up off the lid; low on the spindle: it seats, the door shuts
//   4 SEAL    top-down on the can: lid on, latch snapped; a slow push-in on his face as the can goes under his arm
// Then the hand-back: the double goes, the real Vance stands where he stood with the can under his arm.
//
// The hook for the scene that hosts it (s7-route):
//   AScene7Swap* Swap = AScene7Swap::Find(this);
//   Swap->BeginSwap(Vance);                  // on the Use press at GetUseSpot(): beat 1 starts on this frame
//   Swap->HandleSwapKey(Key, bPressed);      // forward keys while IsRunning() (true = consumed)
//   Swap->DrawSwapHud(Canvas);               // the prompt
//   if (Swap->IsDone()) ...                  // the canister is under his arm and the camera is his again
//   Swap->ResetSwap();                       // retakes / scene reset
// The host keeps Vance from walking while IsRunning() (the double stands in for him; he is hidden).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Script/Scene7SwapAnim.h"
#include "Scene7Swap.generated.h"

class UProceduralMeshComponent;
class USkeletalMeshComponent;
class UCineCameraComponent;
class UPointLightComponent;
class USpotLightComponent;
class UAudioComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UAnimSequence;
class USoundBase;
class ATakeCharacter;

/** One camera set-up. Positions are in the actor's space, or Vance's body space (X forward, Y right, Z up from his
 *  feet) with bBody, or the bench's space with bBench. */
USTRUCT(BlueprintType)
struct FScene7SwapShot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Shot") FVector From = FVector::ZeroVector;
	/** Where the camera ends up after Move seconds (the push-in / dolly); same as From for a locked-off shot. */
	UPROPERTY(EditAnywhere, Category = "Shot") FVector To = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, Category = "Shot") FVector Aim = FVector::ZeroVector;
	/** Aim at this bone of the double (plus Aim, in world axes) instead. */
	UPROPERTY(EditAnywhere, Category = "Shot") FName AimBone;
	UPROPERTY(EditAnywhere, Category = "Shot") float Focal = 35.f;
	/** Focal length at the end of the move (0: Focal throughout). */
	UPROPERTY(EditAnywhere, Category = "Shot") float FocalTo = 0.f;
	UPROPERTY(EditAnywhere, Category = "Shot") float Aperture = 2.8f;
	UPROPERTY(EditAnywhere, Category = "Shot") float Move = 2.f;
	/** Hand-held drift (cm). */
	UPROPERTY(EditAnywhere, Category = "Shot") float Drift = 0.4f;
	UPROPERTY(EditAnywhere, Category = "Shot") bool bBody = false;
	UPROPERTY(EditAnywhere, Category = "Shot") bool bBench = false;
};

UCLASS()
class REEL_API AScene7Swap : public AActor
{
	GENERATED_BODY()

public:
	AScene7Swap();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** The swap in the level (the first one), or null. */
	static AScene7Swap* Find(const UObject* WorldContextObject);

	// ---- the hook ----
	/** Vance is at the projector: the double takes his place and, with bStartNow, beat 1 starts this frame. */
	void BeginSwap(ATakeCharacter* InVance, bool bStartNow = true);
	/** The same, for the hosting scene: its POV character is Vance. */
	void StartSwap(class AScriptScene* Scene, bool bStartNow = true);
	/** A press: the next beat on this frame (or the current one hurries if it is still playing). */
	bool HandleSwapKey(const FKey& Key, bool bPressed);
	void DrawSwapHud(class UCanvas* Canvas);
	bool IsRunning() const { return Beat > 0 && !bDone; }
	bool IsDone() const { return bDone; }
	/** Everything back where it was (master on the spindle and turning, the can open on the bench). */
	void ResetSwap();
	/** Where Vance stands to begin (world) and his yaw there. */
	FVector GetUseSpot() const;
	float GetUseYaw() const;
	/** The canister (its root: body, lid, latch and the master print inside), e.g. to hand it to Scene 8. */
	USceneComponent* GetCanister() const { return CanRoot; }
	/** The canister goes under Character's left arm (attached to CanCarryBone). */
	void AttachCanisterTo(ATakeCharacter* Character);
	/** The projector's motor: the reels turn and the beam shines while it runs. */
	void SetProjectorRunning(bool bRun);

	/** Debug / screenshots: hold the swap at Beat (1..4) and T seconds into it (the double and the camera too). */
	void Scrub(int32 InBeat, float T);

	// ---- the set ----
	UPROPERTY(VisibleAnywhere, Category = "Swap") TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category = "Swap") TObjectPtr<UProceduralMeshComponent> Projector;
	UPROPERTY(VisibleAnywhere, Category = "Swap") TObjectPtr<USceneComponent> DoorHinge;
	UPROPERTY(VisibleAnywhere, Category = "Swap") TObjectPtr<UProceduralMeshComponent> Door;
	UPROPERTY(VisibleAnywhere, Category = "Swap") TObjectPtr<UProceduralMeshComponent> DoorLatch;
	UPROPERTY(VisibleAnywhere, Category = "Swap") TObjectPtr<UProceduralMeshComponent> MasterReel;
	UPROPERTY(VisibleAnywhere, Category = "Swap") TObjectPtr<UProceduralMeshComponent> DummyReel;
	UPROPERTY(VisibleAnywhere, Category = "Swap") TObjectPtr<UProceduralMeshComponent> FilmStrand;
	/** The bench: move it on the placed actor (the choreography follows it). */
	UPROPERTY(VisibleAnywhere, Category = "Swap") TObjectPtr<UProceduralMeshComponent> Bench;
	UPROPERTY(VisibleAnywhere, Category = "Swap") TObjectPtr<USceneComponent> CanRoot;
	UPROPERTY(VisibleAnywhere, Category = "Swap") TObjectPtr<UProceduralMeshComponent> Can;
	UPROPERTY(VisibleAnywhere, Category = "Swap") TObjectPtr<UProceduralMeshComponent> CanLid;
	UPROPERTY(VisibleAnywhere, Category = "Swap") TObjectPtr<UProceduralMeshComponent> CanLatch;
	UPROPERTY(VisibleAnywhere, Category = "Swap") TObjectPtr<UProceduralMeshComponent> Beam;
	UPROPERTY(VisibleAnywhere, Category = "Swap") TObjectPtr<USpotLightComponent> LensLight;
	UPROPERTY(VisibleAnywhere, Category = "Swap") TObjectPtr<UPointLightComponent> MagLight;
	UPROPERTY(VisibleAnywhere, Category = "Swap") TObjectPtr<UPointLightComponent> BenchLamp;
	UPROPERTY(VisibleAnywhere, Category = "Swap") TObjectPtr<USkeletalMeshComponent> Double;
	UPROPERTY(VisibleAnywhere, Category = "Swap") TObjectPtr<UCineCameraComponent> Cam;
	UPROPERTY(VisibleAnywhere, Category = "Swap") TObjectPtr<UAudioComponent> Hum;

	// ---- marks (actor space; the bench marks in the bench's space) ----
	UPROPERTY(EditAnywhere, Category = "Swap|Marks") FVector OpenSpot = FVector(-40.f, -64.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Swap|Marks") float OpenYaw = 52.f;
	UPROPERTY(EditAnywhere, Category = "Swap|Marks") FVector ReelSpot = FVector(-20.f, -50.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Swap|Marks") float ReelYaw = 68.f;
	/** Bench space: where he stands at the bench (he faces the bench's -Y). */
	UPROPERTY(EditAnywhere, Category = "Swap|Marks") FVector BenchSpot = FVector(-2.f, 42.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Swap|Marks") FVector CanOnBench = FVector(-24.f, 2.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Swap|Marks") FVector LidOnBench = FVector(26.f, 0.f, 0.f);
	/** Where he glances at the end (actor space): the booth's door. */
	UPROPERTY(EditAnywhere, Category = "Swap|Marks") FVector GlanceAt = FVector(-250.f, -150.f, 160.f);

	// ---- the carry (body space: X forward, Y right, Z up) ----
	UPROPERTY(EditAnywhere, Category = "Swap|Carry") FVector ReelCarry = FVector(30.f, 0.f, 118.f);
	UPROPERTY(EditAnywhere, Category = "Swap|Carry") FVector CanUnderArm = FVector(2.f, -27.f, 108.f);
	/** The real Vance carries the can on this bone (keeps it at his side whatever his arms do). */
	UPROPERTY(EditAnywhere, Category = "Swap|Carry") FName CanCarryBone = TEXT("spine_03");

	// ---- the double's acting ----
	UPROPERTY(EditAnywhere, Category = "Swap|Acting") TObjectPtr<UAnimSequence> IdleClip;
	UPROPERTY(EditAnywhere, Category = "Swap|Acting") float PalmOffset = 8.f;
	UPROPERTY(EditAnywhere, Category = "Swap|Acting") float GripDegrees = 65.f;
	/** Shoulder height and half-width (cm), for the elbows' poles. */
	UPROPERTY(EditAnywhere, Category = "Swap|Acting") float ShoulderZ = 145.f;
	UPROPERTY(EditAnywhere, Category = "Swap|Acting") float ShoulderHalf = 18.f;
	/** Lean into the bench (degrees). */
	UPROPERTY(EditAnywhere, Category = "Swap|Acting") float BenchLean = 24.f;

	// ---- timing ----
	/** Each beat's length (seconds); a press during a beat plays the rest at HurryRate. */
	UPROPERTY(EditAnywhere, Category = "Swap|Timing") TArray<float> BeatSeconds;
	UPROPERTY(EditAnywhere, Category = "Swap|Timing") float HurryRate = 2.5f;
	/** The feed reel's turn while the projector runs (degrees a second). */
	UPROPERTY(EditAnywhere, Category = "Swap|Timing") float ReelDegPerSec = 150.f;
	UPROPERTY(EditAnywhere, Category = "Swap|Timing") float DoorOpenDegrees = 100.f;

	// ---- coverage ----
	UPROPERTY(EditAnywhere, Category = "Swap|Shots") TMap<FName, FScene7SwapShot> Shots;

	// ---- words ----
	UPROPERTY(EditAnywhere, Category = "Swap|Words") TArray<FString> PromptWords;
	UPROPERTY(EditAnywhere, Category = "Swap|Words") FString Goal = TEXT("The master print is in the projector's upper magazine.");

	// ---- look ----
	UPROPERTY(EditAnywhere, Category = "Swap|Look") TObjectPtr<UMaterialInterface> SurfaceMaterial;
	UPROPERTY(EditAnywhere, Category = "Swap|Look") TObjectPtr<UMaterialInterface> BeamMaterial;
	UPROPERTY(EditAnywhere, Category = "Swap|Look") TArray<FLinearColor> SlotColors;
	/** Per slot: X metallic, Y roughness. */
	UPROPERTY(EditAnywhere, Category = "Swap|Look") TArray<FVector2D> SlotFinish;
	UPROPERTY(EditAnywhere, Category = "Swap|Look") float BeamLength = 900.f;
	UPROPERTY(EditAnywhere, Category = "Swap|Look") float BeamGlow = 0.05f;

	// ---- sound (cooked with the actor) ----
	UPROPERTY(EditAnywhere, Category = "Swap|Sound") TMap<FName, TObjectPtr<USoundBase>> Sounds;

protected:
	int32 Beat = 0;			// 0 not begun; 1..4 the beats
	float BeatT = 0.f;
	float Rate = 1.f;
	bool bQueued = false;	// a press came during the beat: the next beat starts when it ends
	bool bDone = false;
	bool bScrub = false;	// held by Scrub()
	bool bRunning = true;	// the projector
	float SpinAngle = 0.f;
	float SpinRate = 1.f;	// 0..1 of ReelDegPerSec
	float PrevT = 0.f;
	FName CurrentShot;
	float ShotT = 0.f;
	float PressFlash = 0.f;	// real seconds since the last press (the prompt's pop)

	UPROPERTY(Transient) TObjectPtr<ATakeCharacter> Vance;
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> Mids;

	TMap<USceneComponent*, TPair<USceneComponent*, FTransform>> Rest;

	void BuildMeshes();
	void ApplyLook();
	void StartBeat(int32 NewBeat);
	void EndSwap();
	/** The whole frame at (Beat, BeatT): the double, its hands, the props, the camera. */
	void Pose(float Dt);
	void SetShot(FName Name);
	void UpdateCamera(float Dt);
	void Cue(FName Name, float Volume = 1.f);
	bool Crossed(float T) const { return PrevT < T && BeatT >= T; }

	// Spaces.
	FTransform BenchT() const;
	FTransform BodyT(const FVector& Spot, float Yaw) const;
	FVector BenchSpotActor() const;
	float BenchYawActor() const;
	FVector ReelOnSpindle() const;
	FQuat FlatOnBench() const;
};
