// Scene 8, EXT. REAR ALLEYWAY - NIGHT (Vance, 35mm): the getaway and the last shot. Owner: s8-alley.
// docs/SCRIPT.md Scene 8, docs/script/s8.md. Placed by tools/script/scene8.py (tag script_s8) on the outside face of
// the steel fire exit in the service wing's south wall, yaw 0: everything below is in the actor's space (+X east
// along the alley, -Y out of the door across it). The alley set itself (brick wall, lamp, bins, puddles, rain) is the
// overlay's; this actor owns the door, the guards, the car, the cameras and the beats:
//   Burst  (wide shot) the door bangs open, Vance and Cole stumble out, headlights flare round the corner, the GT drifts
//          in and stops, passenger door open; MAX: "Get in! Get in!"
//   Run    (Vance's follow camera, the player walks) reach the passenger door before the guards' fire closes in
//          (RunSeconds, else FailScene "PinnedDown"); guards burst from the exit, muzzle flashes, sparks off the brick,
//          nobody is hit. Cole runs for the back seat on his own.
//   Dive   Vance dives in, the door slams; COLE: "Go, go, go!"
//   Drive  (a camera at the alley's mouth) Max dumps the clutch, the V8 roars, the car fishtails out of the alley.
//   Mirror the final shot inside the car, framed in the rear-view mirror: Vance straightens his tie and smiles;
//          VANCE: "Cut. Print it." -> FinishScene: the sequencer's EndOfFilm smashes to black and rolls the credits.

#pragma once

#include "CoreMinimal.h"
#include "Script/ScriptScene.h"
#include "Scene8Alley.generated.h"

class AScene8Car;
class ACameraActor;
class UAudioComponent;
class UNiagaraSystem;
class UPointLightComponent;
class USoundBase;
class USplineComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UAnimationAsset;
class UStaticMeshComponent;

UENUM()
enum class EScene8Beat : uint8
{
	Burst,
	Run,
	Dive,
	Drive,
	Mirror,
	Done
};

UCLASS()
class REEL_API AScene8Alley : public AScriptScene
{
	GENERATED_BODY()

public:
	AScene8Alley();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	virtual void BeginScene() override;
	virtual void TickScene(float DeltaSeconds) override;
	virtual void EndScene() override;
	virtual void ResetScene() override;
	virtual void DrawSceneHud(UCanvas* Canvas) override;

	// ---- Tuning (seconds and cm; actor space) ----

	/** The opening wide shot, from the door bang to the cut to Vance's camera. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Timing")
	float BurstSeconds = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Timing")
	float ArriveDelay = 0.25f;

	/** The car's run in along ArrivePath (ease out). */
	UPROPERTY(EditAnywhere, Category = "Scene8|Timing")
	float ArriveSeconds = 2.4f;

	/** Time the player has to reach the passenger door. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Timing")
	float RunSeconds = 7.f;

	/** Seconds into the run the guards burst out of the exit. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Timing")
	float GuardsAt = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Timing")
	float ShotGapMin = 0.28f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Timing")
	float ShotGapMax = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Timing")
	float DiveSeconds = 0.6f;

	/** The car's run out along DepartPath (ease in). */
	UPROPERTY(EditAnywhere, Category = "Scene8|Timing")
	float DriveSeconds = 2.8f;

	/** The final shot's length; the line lands at PrintAt. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Timing")
	float MirrorSeconds = 3.6f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Timing")
	float TieAt = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Timing")
	float PrintAt = 1.3f;

	/** Vance's feet within this of the passenger door: he dives in. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Run")
	float ReachRadius = 200.f;

	/** How far from Vance the bullets spark at the start of the run and at its end (the fire closes in). */
	UPROPERTY(EditAnywhere, Category = "Scene8|Run")
	float MissStart = 320.f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Run")
	float MissEnd = 70.f;

	/** The building's outer face (Y) and the alley's far brick wall face (Y), where the bullets spark. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Run")
	float NearWallY = -4.f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Run")
	float FarWallY = -768.f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Run")
	FVector VanceStart = FVector(40.f, -110.f, 100.f);

	UPROPERTY(EditAnywhere, Category = "Scene8|Run")
	FVector ColeStart = FVector(-30.f, -45.f, 100.f);

	/** Where the two burst out to in the wide shot (they stumble forward this far on their own). */
	UPROPERTY(EditAnywhere, Category = "Scene8|Run")
	FVector VanceStumble = FVector(80.f, -260.f, 0.f);

	UPROPERTY(EditAnywhere, Category = "Scene8|Run")
	FVector ColeStumble = FVector(-20.f, -230.f, 0.f);

	/** The guards' marks out of the door. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Run")
	FVector GuardMark0 = FVector(-130.f, -140.f, 0.f);

	UPROPERTY(EditAnywhere, Category = "Scene8|Run")
	FVector GuardMark1 = FVector(70.f, -180.f, 0.f);

	/** The car's drift angle while it settles (deg), and its fishtail on the way out. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Car")
	float DriftDeg = 30.f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Car")
	float FishtailDeg = 22.f;

	/** Hips drop from standing (cm) when Vance sits in the car for the last shot. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Car")
	float SeatDrop = 95.f;

	/** The opening wide shot and the drive-off shot (actor space), each with what it looks at. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Cameras")
	FVector WideCamAt = FVector(1750.f, -620.f, 175.f);

	UPROPERTY(EditAnywhere, Category = "Scene8|Cameras")
	FVector WideCamLook = FVector(-300.f, -380.f, 110.f);

	UPROPERTY(EditAnywhere, Category = "Scene8|Cameras")
	FVector DriveCamAt = FVector(250.f, -620.f, 120.f);

	UPROPERTY(EditAnywhere, Category = "Scene8|Cameras")
	float MirrorFov = 38.f;

	// ---- Assets (soft: a missing one is silent, never a crash) ----

	UPROPERTY(EditAnywhere, Category = "Scene8|Assets")
	TSubclassOf<AScene8Car> CarClass;

	UPROPERTY(EditAnywhere, Category = "Scene8|Assets")
	TSoftObjectPtr<UNiagaraSystem> SparkSystem;

	/** The guards' rigged model and their aiming pose; unset or missing: grey-box guards. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Assets")
	TSoftObjectPtr<USkeletalMesh> GuardMesh;

	UPROPERTY(EditAnywhere, Category = "Scene8|Assets")
	TSoftObjectPtr<UAnimationAsset> GuardPose;

	UPROPERTY(EditAnywhere, Category = "Scene8|Assets")
	TSoftObjectPtr<USoundBase> DoorKickSound;

	UPROPERTY(EditAnywhere, Category = "Scene8|Assets")
	TArray<TSoftObjectPtr<USoundBase>> GunshotSounds;

	UPROPERTY(EditAnywhere, Category = "Scene8|Assets")
	TArray<TSoftObjectPtr<USoundBase>> RicochetSounds;

	UPROPERTY(EditAnywhere, Category = "Scene8|Assets")
	TSoftObjectPtr<USoundBase> CarArriveSound;

	UPROPERTY(EditAnywhere, Category = "Scene8|Assets")
	TSoftObjectPtr<USoundBase> CarAwaySound;

	UPROPERTY(EditAnywhere, Category = "Scene8|Assets")
	TSoftObjectPtr<USoundBase> CarDoorSound;

	UPROPERTY(EditAnywhere, Category = "Scene8|Assets")
	TSoftObjectPtr<USoundBase> InteriorLoop;

	UPROPERTY(EditAnywhere, Category = "Scene8|Assets")
	TSoftObjectPtr<USoundBase> MaxLine;

	UPROPERTY(EditAnywhere, Category = "Scene8|Assets")
	TSoftObjectPtr<USoundBase> ColeLine;

	UPROPERTY(EditAnywhere, Category = "Scene8|Assets")
	TSoftObjectPtr<USoundBase> GuardLine;

	UPROPERTY(EditAnywhere, Category = "Scene8|Assets")
	TSoftObjectPtr<USoundBase> VanceLine;

	// ---- Components ----

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<USceneComponent> Root;

	/** The steel fire exit on its hinge (swings out to -Y), the dark doorway behind it, the EXIT sign. */
	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<USceneComponent> DoorHinge;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<UStaticMeshComponent> Door;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<UStaticMeshComponent> Doorway;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<UStaticMeshComponent> ExitSign;

	/** The car's way in (round the corner at the alley's west end) and out (through the gap in the far wall). */
	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<USplineComponent> ArrivePath;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<USplineComponent> DepartPath;

	/** Two grey-box guards (body, head, pistol), hidden in the doorway until they burst out. */
	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<USceneComponent> Guard0;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<USceneComponent> Guard1;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<USkeletalMeshComponent> Guard0Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<USkeletalMeshComponent> Guard1Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<UPointLightComponent> MuzzleLight;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<UStaticMeshComponent> MuzzleFlash;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<UPointLightComponent> SparkLight;

	/** A sodium street lamp that sweeps past the car in the final shot (the car is still: the light moves). */
	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<UPointLightComponent> PassingLight;

protected:
	UPROPERTY(Transient)
	TObjectPtr<AScene8Car> Car;

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> WideCam;

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> DriveCam;

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> MirrorCam;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Interior;

	EScene8Beat Beat = EScene8Beat::Done;
	float BeatT = 0.f;		// seconds in the current beat
	float NextShot = 0.f;	// BeatT of the next shot (Run, Dive, Drive)
	int32 ShotCount = 0;
	float MuzzleAge = 1.f;
	float SparkAge = 1.f;
	bool bMaxSaid = false, bColeIn = false, bTie = false, bPrint = false, bArrived = false;

	void Enter(EScene8Beat NewBeat);
	FVector W(const FVector& Local) const { return GetActorTransform().TransformPosition(Local); }
	class ATakeCharacter* GetCole() const;

	/** The car at Alpha (0..1) along a path, with a yaw offset (drift, fishtail) and a lean on the chassis. */
	void PoseCar(USplineComponent* Path, float Alpha, float YawOffset);
	/** One guard's shot: muzzle flash, the bang, a spark and a whine on the brick Miss cm from Target. */
	void Shoot(const FVector& Target, float Miss);
	void PlayAt(const TSoftObjectPtr<USoundBase>& Sound, const FVector& Where, float Volume = 1.f);
	USoundBase* Load(const TSoftObjectPtr<USoundBase>& Sound) const;
	/** Put Vance and Cole back on their feet, visible, walking, colliding. */
	void FreeCast();
	void SeatVance();
};
