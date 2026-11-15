// Scene 4. INT. THE CATWALKS / LIGHTING RIG - CONTINUOUS (Nora, raw). Owner: s4-catwalk (docs/script/s4.md).
//
// Nora (Maya) on the catwalk over the hall: a bird's-eye camera looks down through the grating at the rope; each of
// the four rope guards draws his patrol cone (red while it covers Vance's lane). Two sabotages from above:
//   [1] the rigged spotlight: loosen a clamp, the follow spot (AFollowSpot) swings down and blinds the EAST DOOR
//       guard; GUARD 1 turns to yell up at it while it burns.
//   [2] the champagne line: a weighted line taps the latch of the service cart (ACartDevice) carrying a champagne
//       tower; it rolls and the glasses crash at the WEST PILLAR. The WEST guard goes to the mess; if GUARD 1 is
//       yelling at the spot, he sends GUARD 2 to help ("...go help the staff clean that mess").
//   [SPACE] cue Vance: every cone must be off the lane, or it is a retake. The cart first (no spot) = the guards close
//       ranks, a retake. The VipAlibi flag (Scene 3) excuses GUARD 2 (he has Vance on his clipboard).
// Lane clear and cued: CutTo(Vance, TrackCamera), one unbroken tracking shot alongside Vance as he strolls the lane
// past the distracted guards, through the crimson curtain at the rope gap, and the doors swing shut. FinishScene.
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

	/** How long the spot blinds the East Door guard (and Guard 1 yells), s. */
	UPROPERTY(EditAnywhere, Category = "Scene4|Tune")
	float SpotSeconds = 7.f;
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
