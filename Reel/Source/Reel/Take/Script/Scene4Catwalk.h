// Scene 4. INT. THE CATWALKS / LIGHTING RIG - CONTINUOUS (Nora, raw). Owner: s4-catwalk (docs/script/s4.md).
//
// Nora (Maya) on the catwalk over the hall: a bird's-eye camera looks down through the grating at the rope; each of
// the four rope guards has a faint sight wedge, and Vance's gold lane turns red wherever a guard sees it. Two rigs
// hang in her view (built here as shapes at play, each with a card saying what it is and what it will do):
//   [1] the spotlight clamp: a lit lamp under a pipe batten, clamped on a harmless patch of floor. Loosened, its
//       brass T-handle spins off, the lamp swings into the EAST DOOR guard's eyes (shaft, glare); GUARD 1 yells up at
//       it. The bulb lasts SpotSeconds; if it blows before Vance is cued, the window is gone (a retake).
//   [2] the sandbag line: a sandbag hangs on a hemp line over the champagne cart (ACartDevice). Cut, it drops onto
//       the cart's latch, the cart rolls and the tower crashes at the WEST PILLAR: that guard goes to the mess. If
//       GUARD 1 is yelling at the spot (either order), he sends GUARD 2 to help ("...go help the staff clean that
//       mess"). The VipAlibi flag (Scene 3) excuses GUARD 2 (he has Vance on his clipboard).
//   [SPACE] send Vance: no guard may see the lane, or it is a retake.
// Lane clear and cued: CutTo(Vance, TrackCamera), one unbroken tracking shot alongside Vance as he walks the lane on
// his own feet (movement component: his walk cycle plays, props block him; the lane is laid to clear them) past the
// distracted guards, through the crimson curtain at the rope gap, and the doors swing shut. FinishScene.
//
// The scene drives its guards (moves them itself) and its two devices (FireAt/Fire + StepDevice) while active.
// tools/script/scene4.py places this actor (tag script_s4) at the rope gap, facing into the auditorium (+X), and
// sets the guards, devices, cameras and the lane.

#pragma once

#include "CoreMinimal.h"
#include "Script/ScriptScene.h"
#include "Scene4Catwalk.generated.h"

class AFollowSpot;
class ACartDevice;
class UStaticMeshComponent;
class USoundBase;
class UCanvas;

UENUM(BlueprintType)
enum class EScene4Role : uint8
{
	EastDoor,	// blinded by the spot
	WestPillar,	// lured by the champagne crash
	Guard1,		// the sergeant: yells at the spot while it burns
	Guard2		// sent to the mess by Guard 1 (or excused by the VIP Alibi)
};

/** One rope guard as Scene 4 sees him: who he is, where he stands, how his loop sweeps. */
USTRUCT(BlueprintType)
struct FScene4Guard
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Scene4")
	TObjectPtr<AActor> Actor;

	UPROPERTY(EditAnywhere, Category = "Scene4")
	EScene4Role Role = EScene4Role::EastDoor;

	/** The name on his cone, e.g. "EAST DOOR". */
	UPROPERTY(EditAnywhere, Category = "Scene4")
	FString Label;

	/** His post (world). Zero: where the actor stands when the scene first begins. */
	UPROPERTY(EditAnywhere, Category = "Scene4")
	FVector Post = FVector::ZeroVector;

	/** The yaw he faces at his post (world, degrees). */
	UPROPERTY(EditAnywhere, Category = "Scene4")
	float Yaw = 180.f;

	/** His tight loop: he paces Post -> Post + Pace -> Post, sweeping his look +-Sweep degrees, every Period s. */
	UPROPERTY(EditAnywhere, Category = "Scene4")
	FVector Pace = FVector(0.f, 150.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Scene4")
	float Sweep = 25.f;
	UPROPERTY(EditAnywhere, Category = "Scene4")
	float Period = 6.f;
	/** Offset into the loop, s (so the four do not pace in step). */
	UPROPERTY(EditAnywhere, Category = "Scene4")
	float Phase = 0.f;

	/** His cone: how far and how wide he sees (cm, half-angle degrees). */
	UPROPERTY(EditAnywhere, Category = "Scene4")
	float ConeRange = 1100.f;
	UPROPERTY(EditAnywhere, Category = "Scene4")
	float ConeHalfAngle = 35.f;

	// ---- runtime ----
	FTransform Home;			// the actor's transform before the scene touched it (restored by ResetScene)
	FVector Loc = FVector::ZeroVector;
	float FaceYaw = 0.f;
	bool bDistracted = false;	// off the lane (blinded, lured, yelling, excused)
	bool bSeesLane = false;
	FString State;				// "PATROL", "BLINDED", ...
	FVector GoTo = FVector::ZeroVector;	// lured: walking here
	bool bGoing = false;
};

UCLASS()
class REEL_API AScene4Catwalk : public AScriptScene
{
	GENERATED_BODY()

public:
	AScene4Catwalk();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginScene() override;
	virtual void TickScene(float DeltaSeconds) override;
	virtual void EndScene() override;
	virtual void ResetScene() override;
	virtual bool HandleSceneKey(const FKey& Key, bool bPressed) override;
	virtual void DrawSceneHud(UCanvas* Canvas) override;

	// ---- the set (tools/script/scene4.py fills these) ----

	UPROPERTY(EditAnywhere, Category = "Scene4")
	TArray<FScene4Guard> Guards;

	/** The rigged spotlight (Maya's follow spot on the lighting platform). */
	UPROPERTY(EditAnywhere, Category = "Scene4")
	TObjectPtr<AFollowSpot> Spot;

	/** The champagne cart; its crash point is the West Pillar's mess. */
	UPROPERTY(EditAnywhere, Category = "Scene4")
	TObjectPtr<ACartDevice> Cart;

	/** The bird's-eye camera (SceneCamera is set to it). */
	UPROPERTY(EditAnywhere, Category = "Scene4")
	TObjectPtr<AActor> BirdsEyeCamera;

	/** The tracking shot's camera: moved by the scene every frame of the shot. */
	UPROPERTY(EditAnywhere, Category = "Scene4")
	TObjectPtr<AActor> TrackCamera;

	/** Vance's lane, world points: from the bar to the rope gap and through the curtain (the last point is inside
	 *  the auditorium). The cones are tested against it; Vance walks it in the tracking shot. */
	UPROPERTY(EditAnywhere, Category = "Scene4")
	TArray<FVector> Lane;

	// ---- tuning ----

	// ---- Nora's rigs, built here as shapes at play (tools/script/scene4.py sets the points; world cm) ----

	/** The rigged lamp: the pivot of its yoke, hung under a pipe batten over the hall, in Nora's view. */
	UPROPERTY(EditAnywhere, Category = "Scene4|Rig")
	FVector LampAt = FVector(1950.f, 120.f, 520.f);
	/** Where the lit lamp points while it is clamped (a harmless pool on the floor). */
	UPROPERTY(EditAnywhere, Category = "Scene4|Rig")
	FVector LampParkedAim = FVector(1700.f, 450.f, 0.f);
	/** Loosened, it swings onto the East Door guard's eyes over this long (with a damped wobble after), s. */
	UPROPERTY(EditAnywhere, Category = "Scene4|Rig")
	float LampSwingSeconds = 0.45f;
	UPROPERTY(EditAnywhere, Category = "Scene4|Rig")
	float LampCandelas = 9000.f;
	UPROPERTY(EditAnywhere, Category = "Scene4|Rig")
	float LampConeDegrees = 11.f;
	UPROPERTY(EditAnywhere, Category = "Scene4|Rig")
	FLinearColor LampColour = FLinearColor(1.f, 0.9f, 0.72f);
	/** The visible shaft of light (a translucent cone). Null: /Game/Take/Look/M_S4Beam, else no shaft mesh. */
	UPROPERTY(EditAnywhere, Category = "Scene4|Rig")
	TObjectPtr<class UMaterialInterface> BeamMaterial;
	UPROPERTY(EditAnywhere, Category = "Scene4|Rig")
	float BeamOpacity = 0.22f;

	/** The sandbag's hook: on a pipe batten straight over the cart's latch. The line hangs SandbagHang below it. */
	UPROPERTY(EditAnywhere, Category = "Scene4|Rig")
	FVector BattenAt = FVector(1700.f, -1880.f, 560.f);
	UPROPERTY(EditAnywhere, Category = "Scene4|Rig")
	float SandbagHang = 260.f;
	/** The sandbag's fall to the latch, s (it starts falling on the press). */
	UPROPERTY(EditAnywhere, Category = "Scene4|Rig")
	float SandbagDropSeconds = 0.28f;
	/** The pipe battens' length, cm (both rigs hang under one). */
	UPROPERTY(EditAnywhere, Category = "Scene4|Rig")
	float BattenLength = 500.f;

	/** Nora's own catwalk under her camera: deck from CatwalkFrom to CatwalkTo (deck height), width, rails. */
	UPROPERTY(EditAnywhere, Category = "Scene4|Rig")
	bool bBuildCatwalk = true;
	UPROPERTY(EditAnywhere, Category = "Scene4|Rig")
	FVector CatwalkFrom = FVector(960.f, -1600.f, 900.f);
	UPROPERTY(EditAnywhere, Category = "Scene4|Rig")
	FVector CatwalkTo = FVector(960.f, 900.f, 900.f);
	UPROPERTY(EditAnywhere, Category = "Scene4|Rig")
	float CatwalkWidth = 90.f;

	/** Nobody pressed anything for this long: Nora points at the lamp (once), s. */
	UPROPERTY(EditAnywhere, Category = "Scene4|Tune")
	float HintSeconds = 9.f;
	/** The spot went cold before Vance was cued: the retake comes this long after, s. */
	UPROPERTY(EditAnywhere, Category = "Scene4|Tune")
	float WindowClosedSeconds = 2.5f;

	/** How long the spot blinds the East Door guard (and Guard 1 yells), s. */
	UPROPERTY(EditAnywhere, Category = "Scene4|Tune")
	float SpotSeconds = 10.f;
	/** The cart's roll to the crash, s. */
	UPROPERTY(EditAnywhere, Category = "Scene4|Tune")
	float CartRollSeconds = 1.6f;
	/** Lured guards walk to the mess at this speed, cm/s. */
	UPROPERTY(EditAnywhere, Category = "Scene4|Tune")
	float LuredWalkSpeed = 260.f;
	/** Crash with no spot burning: the guards close ranks; the retake comes this long after, s. */
	UPROPERTY(EditAnywhere, Category = "Scene4|Tune")
	float CloseRanksSeconds = 2.2f;
	/** Cued while a cone covers the lane: the retake comes this long after (the guard calls out), s. */
	UPROPERTY(EditAnywhere, Category = "Scene4|Tune")
	float SpottedSeconds = 1.6f;

	/** Nora's feed sways a little (vertigo, sixty feet up on a narrow catwalk): cm and degrees, and its speed. */
	UPROPERTY(EditAnywhere, Category = "Scene4|Tune")
	float SwayCm = 4.f;
	UPROPERTY(EditAnywhere, Category = "Scene4|Tune")
	float SwayRollDegrees = 0.6f;
	UPROPERTY(EditAnywhere, Category = "Scene4|Tune")
	float SwayHz = 0.23f;

	/** The tracking shot: Vance's stroll speed, cm/s; the camera's place beside him (cm: along his way, to his
	 *  side, up) and its smoothing (1/s). */
	UPROPERTY(EditAnywhere, Category = "Scene4|Track")
	float StrollSpeed = 170.f;
	UPROPERTY(EditAnywhere, Category = "Scene4|Track")
	FVector TrackOffset = FVector(60.f, -260.f, 150.f);
	UPROPERTY(EditAnywhere, Category = "Scene4|Track")
	float TrackLag = 3.f;
	UPROPERTY(EditAnywhere, Category = "Scene4|Track")
	float TrackFOV = 55.f;
	/** The doors swing shut over this long once Vance is through, then the scene holds HoldAfterDoors and finishes. */
	UPROPERTY(EditAnywhere, Category = "Scene4|Track")
	float DoorCloseSeconds = 1.4f;
	UPROPERTY(EditAnywhere, Category = "Scene4|Track")
	float HoldAfterDoors = 1.0f;
	/** The door leaves' open angle, degrees (they close to 0). */
	UPROPERTY(EditAnywhere, Category = "Scene4|Track")
	float DoorOpenDegrees = 100.f;
	/** Longest the tracking shot may run before it finishes anyway, s (Vance stuck on something). */
	UPROPERTY(EditAnywhere, Category = "Scene4|Track")
	float TrackMaxSeconds = 25.f;
	/** The camera keeps this far off anything it would hit between it and Vance, cm. */
	UPROPERTY(EditAnywhere, Category = "Scene4|Track")
	float TrackCamClearance = 35.f;
	/** Vance made no headway for this long (something in his way): he is stepped on along the lane, s. */
	UPROPERTY(EditAnywhere, Category = "Scene4|Track")
	float StuckSeconds = 0.5f;

	/** The champagne tower on the cart: tiers (bottom has Tiers glasses a side... a pyramid), glass size, cm. */
	UPROPERTY(EditAnywhere, Category = "Scene4|Champagne")
	int32 TowerTiers = 4;
	UPROPERTY(EditAnywhere, Category = "Scene4|Champagne")
	float GlassSpacing = 14.f;
	UPROPERTY(EditAnywhere, Category = "Scene4|Champagne")
	FVector GlassScale = FVector(0.08f, 0.08f, 0.13f);
	/** The tower's base above the cart's actor origin, cm (the cart's top). */
	UPROPERTY(EditAnywhere, Category = "Scene4|Champagne")
	float TowerBaseZ = 95.f;
	UPROPERTY(EditAnywhere, Category = "Scene4|Champagne")
	float ShardSpeed = 380.f;

	// ---- voices (null: the subtitle shows alone) ----
	UPROPERTY(EditAnywhere, Category = "Scene4|Voice")
	TObjectPtr<USoundBase> NoraOpenVoice;
	UPROPERTY(EditAnywhere, Category = "Scene4|Voice")
	TObjectPtr<USoundBase> Guard1SpotVoice;
	UPROPERTY(EditAnywhere, Category = "Scene4|Voice")
	TObjectPtr<USoundBase> NoraGoVoice;
	UPROPERTY(EditAnywhere, Category = "Scene4|Voice")
	TObjectPtr<USoundBase> Guard1HoldVoice;
	UPROPERTY(EditAnywhere, Category = "Scene4|Voice")
	TObjectPtr<USoundBase> GuardSpottedVoice;
	UPROPERTY(EditAnywhere, Category = "Scene4|Voice")
	TObjectPtr<USoundBase> NoraRetakeVoice;

	// ---- the threshold at the rope gap (components of this actor, which stands in the gap facing +X) ----
	UPROPERTY(VisibleAnywhere, Category = "Scene4")
	TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category = "Scene4")
	TObjectPtr<USceneComponent> HingeL;
	UPROPERTY(VisibleAnywhere, Category = "Scene4")
	TObjectPtr<USceneComponent> HingeR;
	UPROPERTY(VisibleAnywhere, Category = "Scene4")
	TObjectPtr<UStaticMeshComponent> LeafL;
	UPROPERTY(VisibleAnywhere, Category = "Scene4")
	TObjectPtr<UStaticMeshComponent> LeafR;
	UPROPERTY(VisibleAnywhere, Category = "Scene4")
	TObjectPtr<UStaticMeshComponent> CurtainL;
	UPROPERTY(VisibleAnywhere, Category = "Scene4")
	TObjectPtr<UStaticMeshComponent> CurtainR;
	UPROPERTY(VisibleAnywhere, Category = "Scene4")
	TObjectPtr<UStaticMeshComponent> ArchTop;

	/** The threshold's half-width (the rope gap), height, cm. */
	UPROPERTY(EditAnywhere, Category = "Scene4|Threshold")
	float GapHalf = 150.f;
	UPROPERTY(EditAnywhere, Category = "Scene4|Threshold")
	float DoorHeight = 330.f;
	/** The doors stand this far behind the curtain (+X, into the auditorium), cm. */
	UPROPERTY(EditAnywhere, Category = "Scene4|Threshold")
	float DoorsBehind = 60.f;
	UPROPERTY(EditAnywhere, Category = "Scene4|Threshold")
	bool bShowThreshold = true;
	UPROPERTY(EditAnywhere, Category = "Scene4|Threshold")
	FLinearColor CurtainColour = FLinearColor(0.32f, 0.01f, 0.02f);
	UPROPERTY(EditAnywhere, Category = "Scene4|Threshold")
	FLinearColor DoorColour = FLinearColor(0.09f, 0.04f, 0.02f);

	virtual void OnConstruction(const FTransform& Transform) override;

private:
	enum class EPhase : uint8 { Plan, Fail, Track, Doors, Done };
	EPhase Phase = EPhase::Plan;
	float PhaseSeconds = 0.f;

	bool bSpotUsed = false;
	float SpotLeft = 0.f;		// > 0 while the spot burns on the East Door guard
	bool bCartUsed = false;
	bool bCrashed = false;
	float CartT = 0.f;
	bool bClosedRanks = false;
	bool bGuard2Sent = false;
	bool bAlibi = false;
	FName FailReason;
	float FailIn = -1.f;
	int32 SpottedBy = -1;

	// The tracking shot.
	int32 LaneIndex = 0;
	FVector CamLoc = FVector::ZeroVector;
	bool bCamPlaced = false;
	float DoorAlpha = 1.f;		// 1 open .. 0 shut

	// The champagne tower.
	struct FGlass
	{
		TObjectPtr<UStaticMeshComponent> Comp;
		FVector Rel = FVector::ZeroVector;	// on the cart, relative to the cart body
		FVector Vel = FVector::ZeroVector;	// flying
		FRotator Spin = FRotator::ZeroRotator;
		bool bFlying = false;
		bool bDown = false;
	};
	TArray<FGlass> Glasses;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> GlassComps;
	float FloorZ = 0.f;
	float FlashLeft = 0.f;		// the HUD's white flick on a press
	FTransform BirdHome;
	bool bBirdHome = false;

	// The rigs (runtime shapes; Transient so they are never saved).
	UPROPERTY(Transient)
	TArray<TObjectPtr<UActorComponent>> RigComps;
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> LampTilt;		// at LampAt: aims the lamp (its +X is the beam)
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> LampLens;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> LampShaft;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> LampPool;
	UPROPERTY(Transient)
	TObjectPtr<class USpotLightComponent> LampLight;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> ClampHandle;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> ClampJaw;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Sandbag;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> SandLine;
	FRotator LampAimNow = FRotator::ZeroRotator;
	float LampT = -1.f;			// seconds since the clamp was loosened (< 0: clamped)
	float BagT = -1.f;			// seconds since the line was cut (< 0: hanging)
	bool bLatchHit = false;
	bool bHinted = false;
	bool bWindowClosed = false;
	FVector VanceLast = FVector::ZeroVector;
	float StuckFor = 0.f;
	ECollisionResponse VancePawnResponse = ECR_Block;
	bool bVanceGhosted = false;

	void BuildRigs();
	void StepRigs(float Dt);
	void AimLamp(const FRotator& Aim);
	FVector EastDoorEyes() const;
	FVector CartLatch() const;
	void PlaceVanceAtBar();
	void GhostVance(bool bGhost);
	void SendGuard2IfOrdered();

	void BuildTower();
	void PlaceTowerOnCart();
	void ShatterTower();
	void StepGlasses(float Dt);

	void PressSpot();
	void PressCart();
	void PressCue();

	void StepGuards(float Dt);
	void StepTrack(float Dt);
	void StepDoors(float Dt);
	void SetDoors(float OpenAlpha);
	int32 FindGuard(EScene4Role Role) const;
	FVector GuardForward(const FScene4Guard& G) const;
	bool ConeCoversLane(const FScene4Guard& G) const;
	void MoveGuardActor(FScene4Guard& G);
	class ATakeCharacter* GetVance() const;
	/** The director skips held watchers (their patrol step and sight test): this scene moves its guards itself. */
	void HoldGuards(bool bHold);
	void ScheduleFail(FName Reason, float After);
};
