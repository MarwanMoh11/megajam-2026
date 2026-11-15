// Scene 8, the escape and the getaway (Vance, 35mm). Owner: s8-escape (was s8-alley). docs/SCRIPT.md Scene 8,
// docs/script/s8.md. Placed by tools/script/scene8.py (tag script_s8) on the outside face of the steel fire exit in the
// prop store's south wall, yaw 0: actor-space values are +X east along the alley, -Y out of the door across it;
// world-space values (the route, the room) end in "World". The alley set is scene8.py's; this actor owns the door, the
// chasing guards (AScene8Chaser), the car's run in, the cameras and the beats:
//   Escape (Vance's follow camera, the player RUNS) from the fight room where Scene 7 left him, through the real
//          corridors and rooms to the fire exit Cole rigged in Scene 6. Cole leads (and waits when Vance lags): he is
//          the guidance. Armed guards burst out behind them and chase down Vance's own trail, firing: muzzle flashes,
//          tracers, sparks on the walls beside him, a stumble that lets the gap close. Never a hit. Stand still too
//          long and they close in: FailScene "Caught" (StallSeconds, with a warning first).
//   Alley  the fire exit bangs open as the first of them reaches it, out into the rain; the GT drifts in round the
//          corner, MAX "Get in! Get in!"; the guards spill out of the exit and fire. Reach the passenger door
//          (AlleySeconds, else FailScene "PinnedDown"). Cole makes for the back seat on his own.
//   Dive   Vance dives in, the door slams; COLE: "Go, go, go!" (the car-door hand-off: s8-car-mirror)
//   Drive  (a camera at the alley's mouth) Max dumps the clutch, the V8 roars, the car fishtails out of the alley.
//   Mirror the final shot inside the car, framed in the rear-view mirror: Vance straightens his tie and smiles;
//          VANCE: "Cut. Print it." -> FinishScene: the sequencer's EndOfFilm smashes to black and rolls the credits.

#pragma once

#include "CoreMinimal.h"
#include "Script/ScriptScene.h"
#include "Scene8Alley.generated.h"

class AScene8Car;
class AScene8Chaser;
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
class UMaterialInterface;

UENUM()
enum class EScene8Beat : uint8
{
	Escape,
	Alley,
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

	/** The car's run in along ArrivePath (ease out), cued when Vance is CarCueDistance from the fire exit. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Timing")
	float ArriveSeconds = 2.4f;

	/** Time the player has in the alley to reach the passenger door once he is out of the exit. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Timing")
	float AlleySeconds = 9.f;

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

	/** How far beside Vance the rounds pass at the start of the chase and at its tightest (cm). */
	UPROPERTY(EditAnywhere, Category = "Scene8|Run")
	float MissStart = 220.f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Run")
	float MissEnd = 70.f;

	/** The building's outer face (Y) and the alley's far brick wall face (Y), where the bullets spark. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Run")
	float NearWallY = -4.f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Run")
	float FarWallY = -768.f;

	/** Vance runs the whole escape (chars-locomotion's ATakeCharacter::SetRunning, his RunSpeed); Cole runs ahead at
	 *  this (cm/s) through ScriptWalkTo. Walking comes back when the scene ends. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	float ColeRunSpeed = 560.f;

	/** Cole's way out, world space: from the fight room's door, through the corridors and rooms, to the inside of the
	 *  fire exit. Empty: he runs straight for the exit. Vance follows him (the player steers). */
	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	TArray<FVector> RouteWorld;

	/** Where Cole is when the scene opens (world): outside the fight room's door, out of sight; zero: a step behind
	 *  Vance (and no entrance). */
	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	FVector ColeStartWorld = FVector::ZeroVector;

	/** Cole's entrance: he bursts in to ColeMarkWorld, lobs the smoke grenade to SmokeWorld (the door the guards
	 *  will come through), "Max is out back! Move!", and leads off along RouteWorld. Zero: no entrance. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	FVector ColeMarkWorld = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	FVector SmokeWorld = FVector::ZeroVector;

	/** The grenade's flight (s) and how far the smoke billows (cm radius) and for how long it grows (s). */
	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	float GrenadeSeconds = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	float SmokeRadius = 260.f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	float SmokeGrowSeconds = 2.5f;

	/** Cole never gets further ahead of Vance than this (cm, straight line): he stops and turns to wave him on. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	float ColeLeadMax = 650.f;

	/** The door the guards burst out of behind them (world); zero: Vance's start. They appear once Vance's trail
	 *  is GapStart past it. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	FVector GuardsEnterWorld = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	int32 ChaserCount = 3;

	/** Trail spacing between the chasers (cm). */
	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	float ChaserSpacing = 170.f;

	/** The gap the lead guard keeps behind Vance along his trail: GapStart when they appear, closing to GapEnd over
	 *  GapCloseSeconds. Stand still and it closes to MinGap; StallSeconds there and they have him. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	float GapStart = 1000.f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	float GapEnd = 600.f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	float GapCloseSeconds = 10.f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	float MinGap = 320.f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	float StallSeconds = 5.f;

	/** The guards' running pace (cm/s), a touch under Vance's run; they speed up to GuardRunMax to hold the gap. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	float GuardRunSpeed = 500.f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	float GuardRunMax = 900.f;

	/** The stumble: this far down his trail after the guards appear, a round sparks off the wall at his shoulder and
	 *  he staggers to StumbleSpeed of his run for StumbleSeconds (the gap closes, then he finds his feet). */
	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	float StumbleAfter = 900.f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	float StumbleSeconds = 0.7f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	float StumbleSpeed = 0.35f;

	/** Vance within this (straight line) of the fire exit: the car starts its run in round the corner. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	float CarCueDistance = 1100.f;

	/** Vance or Cole within this of the fire exit's inside face: the door bangs open. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Escape")
	float DoorReach = 170.f;

	/** Where the guards stop outside the exit to fire at the car (actor space). */
	UPROPERTY(EditAnywhere, Category = "Scene8|Run")
	FVector GuardMark0 = FVector(-130.f, -140.f, 0.f);

	UPROPERTY(EditAnywhere, Category = "Scene8|Run")
	FVector GuardMark1 = FVector(70.f, -180.f, 0.f);

	UPROPERTY(EditAnywhere, Category = "Scene8|Run")
	FVector GuardMark2 = FVector(-20.f, -90.f, 0.f);

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

	/** The chasing guards (model, run and aim animations, pistol: the class's defaults). */
	UPROPERTY(EditAnywhere, Category = "Scene8|Assets")
	TSubclassOf<AScene8Chaser> ChaserClass;

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

	/** Vance's grunt as the near miss makes him stumble. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Assets")
	TSoftObjectPtr<USoundBase> StumbleSound;

	/** Cole's entrance: the door kicked in, the grenade's hiss, his line. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Assets")
	TSoftObjectPtr<USoundBase> ColeDoorSound;

	UPROPERTY(EditAnywhere, Category = "Scene8|Assets")
	TSoftObjectPtr<USoundBase> SmokeSound;

	UPROPERTY(EditAnywhere, Category = "Scene8|Assets")
	TSoftObjectPtr<USoundBase> ColeMoveLine;

	UPROPERTY(EditAnywhere, Category = "Scene8|Assets")
	TSoftObjectPtr<UMaterialInterface> SmokeMaterial;

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

	UPROPERTY(Transient)
	TArray<TObjectPtr<AScene8Chaser>> Chasers;

	/** The smoke grenade and its puffs (made at BeginPlay, hidden until thrown). */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Grenade;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> SmokePuffs;

	float ThrowT = -1.f;		// seconds since Cole let the grenade go; < 0 not yet
	bool bColeIntro = false;	// Cole's entrance is playing (he leads once it is over)
	bool bColeSaid = false;
	EScene8Beat Beat = EScene8Beat::Done;
	float BeatT = 0.f;		// seconds in the current beat
	float NextShot = 0.f;	// BeatT of the next shot (Escape, Alley, Dive, Drive)
	int32 ShotCount = 0;
	float SparkAge = 1.f;
	bool bMaxSaid = false, bColeIn = false, bTie = false, bPrint = false, bArrived = false;

	// The escape: Vance's trail (feet points ~30 cm apart, and the distance along it); the guards' lead distance on it.
	TArray<FVector> Trail;
	TArray<float> TrailDist;
	int32 ColeIdx = 0;			// the route point Cole runs for next
	int32 ColeGoing = -1;		// the point his ScriptWalkTo is heading for; -1 standing
	float OutDist = -1.f;		// the trail distance where Vance came out of the exit; < 0 inside
	float CarT = -1.f;			// seconds since the car's run in was cued; < 0 not yet
	float DoorT = -1.f;			// seconds since the fire exit banged open; < 0 shut
	float GuardsT = -1.f;		// seconds since the guards appeared; < 0 not yet
	float GuardsFrom = 0.f;		// the trail distance they appeared at
	float LeadDist = 0.f;		// the lead guard's distance along Trail
	float StumbleT = -1.f;		// seconds into the stumble; < 0 none
	bool bStumbled = false;
	float StallT = 0.f;			// seconds the guards have been on top of him
	bool bOutside = false;		// Vance is out of the exit
	bool bColeOut = false;		// Cole is out of the exit
	bool bWarned = false;

	void Enter(EScene8Beat NewBeat);
	FVector W(const FVector& Local) const { return GetActorTransform().TransformPosition(Local); }
	class ATakeCharacter* GetCole() const;

	/** The car at Alpha (0..1) along a path, with a yaw offset (drift, fishtail) and a lean on the chassis. */
	void PoseCar(USplineComponent* Path, float Alpha, float YawOffset);
	/** A guard's shot past Target (Miss cm to its side): the flash and the bang at his muzzle, the round traced into the
	 *  set, a spark and a whine where it lands. It never stops on Vance, Cole or the car. */
	void Shoot(const FVector& Target, float Miss);
	/** The escape, every frame: the trail, Cole, the guards on the trail, the gap, the shots, the door, the car. */
	void TickEscape(float DeltaSeconds, class ATakeCharacter* Vance, class ATakeCharacter* Cole);
	void TickCarArrival(float DeltaSeconds);
	void TickDoor(float DeltaSeconds);
	/** Cole bursts in, lobs the smoke, says the line; true while it plays. */
	bool TickColeEntrance(float DeltaSeconds, class ATakeCharacter* Cole);
	FVector TrailAt(float Dist, FVector* OutDir = nullptr) const;
	FVector ExitInside() const { return W(FVector(0.f, 90.f, 0.f)); }
	FVector ExitOutside() const { return W(FVector(0.f, -140.f, 0.f)); }
	void SetSpeeds(bool bRun);
	void PlayAt(const TSoftObjectPtr<USoundBase>& Sound, const FVector& Where, float Volume = 1.f);
	USoundBase* Load(const TSoftObjectPtr<USoundBase>& Sound) const;
	/** Put Vance and Cole back on their feet, visible, walking, colliding. */
	void FreeCast();
	void SeatVance();
};
