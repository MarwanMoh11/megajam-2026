// Step 5, characters and props: one member of the cast on camera A, costumed from basic shapes.
//
// Three ways to use it (tools/build_cast.py places them all):
// - A posed guest or a band player: its own mannequin (Body), frozen in one pose of a template animation, with
//   costume pieces and an instrument attached to its bones (or standing on the floor). Standing, it plays the
//   template idle like an NPC in a stealth game (its own start and rate); seated, or at an instrument, it is still.
// - Attached to a watcher (a PatrolGuard: the guards, the doorman, Vance, the bodyguard): its own mannequin replaces
//   the grey cylinder (hidden at BeginPlay, its collision kept), and walks on the template's idle/walk blend space
//   at the speed the watcher moves.
// - Attached to a character (Julian, Maya, Vex): no mannequin of its own; the pieces go on the character's mesh.
//   On Julian it also plays The Bow: the frame the director fires the "Bow" trigger, a bow from the waist (a
//   poseable copy of his mesh, bent at the spine for BowSeconds; input is never taken).
// A seated guest or player is the standing pose on a poseable copy of the body, bent at the hips and knees.
// Nothing here is a rule: no collision, no stencil, no red or gold (docs/ART_STYLE.md §15.10).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CastMember.generated.h"

class USkeletalMeshComponent;
class USkinnedMeshComponent;
class UPoseableMeshComponent;
class UStaticMeshComponent;
class UAnimationAsset;
class UMaterialInterface;

/** The costume (and instrument) a cast member wears. In Python: unreal.CastKit. */
UENUM(BlueprintType)
enum class ECastKit : uint8
{
	None,
	GuestTux,		// dinner jacket: shirt front, bow tie, hair
	GuestTails,		// tailcoat and top hat
	GuestGown,		// a long gown and a hair bun (Quinn)
	GuestGownHat,	// a gown, a stole and a small hat (Quinn)
	Staff,			// bartender, attendant: shirt front, bow tie, waistcoat
	Guard,			// peaked cap, epaulettes, belt
	Doorman,		// tall peaked cap, epaulettes, a long coat's skirts
	Vance,			// tall top hat, tails, a cane
	Bodyguard,		// broad shoulders, dark glasses, bald
	BandBass,		// fedora; an upright bass
	BandTrumpet,	// fedora; a muted trumpet at the lips
	BandDrums,		// fedora; a drum kit (seated)
	BandPiano,		// fedora; an upright piano (seated, in profile)
	BandSinger,		// a gown; a microphone on a stand (Quinn)
	Julian,			// bow tie and hair on his white jacket
	Maya,			// headset and tool belt
	Vex				// headset and beanie
};

UCLASS()
class REEL_API ACastMember : public AActor
{
	GENERATED_BODY()

public:
	ACastMember();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** The costume. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Cast")
	ECastKit Kit = ECastKit::GuestTux;

	/** The body's tint (§15.10: guests 0.20-0.35, the band 0.04, heavies 0.01-0.03). Pieces take their tones from it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Cast")
	FLinearColor Tint = FLinearColor(0.28f, 0.28f, 0.28f);

	/** Quinn's mesh instead of Manny's (the gowns force it). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast")
	bool bQuinn = false;

	/** Seated: the seated pose (SeatedAnim at SeatedTime) and SeatedDrop instead of the standing one. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast")
	bool bSeated = false;

	/** Uniform scale of the body and its pieces (the bodyguard is bigger). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast")
	float BodyScale = 1.f;

	/** The standing pose: this animation, frozen at PoseTime (s). Empty: the template idle. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Pose")
	TObjectPtr<UAnimationAsset> PoseAnim;

	UPROPERTY(EditAnywhere, Category = "Take|Cast|Pose")
	float PoseTime = 0.f;

	/** The seated pose before the bend at the hips and knees. Empty: the template idle. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Pose")
	TObjectPtr<UAnimationAsset> SeatedAnim;

	UPROPERTY(EditAnywhere, Category = "Take|Cast|Pose")
	float SeatedTime = 0.12f;

	/** How far a seated body sits below the actor (cm): the actor stands on the seat's top. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Pose")
	float SeatedDrop = 86.f;

	/** Seated: the standing pose is bent at the hips and knees by these (degrees) on a poseable copy of the body. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Pose")
	float SeatedHipDegrees = 90.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cast|Pose")
	float SeatedKneeDegrees = 90.f;

	/** Attached to a watcher: walk on WalkBlendSpace at the parent's speed (else frozen in the pose). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Pose")
	TObjectPtr<UAnimationAsset> WalkBlendSpace;

	/** Standing (not seated, not walking with a watcher): play PoseAnim looped, as an NPC idles in a stealth game, from a
	 *  time and at a rate (1 +/- IdleRateSpread) drawn once from the name, so no two breathe together. Off: frozen at
	 *  PoseTime. Nothing moves with the band's beat. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Pose")
	bool bIdle = true;

	UPROPERTY(EditAnywhere, Category = "Take|Cast|Pose", meta = (ClampMin = "0", ClampMax = "0.5"))
	float IdleRateSpread = 0.1f;

	/** The body casts a shadow (the pieces never do). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast")
	bool bBodyShadow = true;

	/** Julian: the trigger that makes him bow, and the bow itself. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Bow")
	FName BowTrigger = TEXT("Bow");

	UPROPERTY(EditAnywhere, Category = "Take|Cast|Bow")
	float BowSeconds = 0.9f;

	/** The bend at the bottom of the bow (degrees), spread over the spine; the head adds BowHeadDegrees. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Bow")
	float BowDegrees = 42.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cast|Bow")
	float BowHeadDegrees = 12.f;

	/** The bow stops (blends out over 0.15 s) once Julian moves faster than this, cm/s. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Bow")
	float BowCancelSpeed = 60.f;

	/** Test hook (Python in play): plays the bow without the trigger. */
	UFUNCTION(BlueprintCallable, Category = "Take|Cast")
	void BowNow();

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<USceneComponent> Root;

	/** The mannequin (unused when dressing a character's own mesh). */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<USkeletalMeshComponent> Body;

private:
	/** The mesh the pieces go on: Body, or the attach parent character's mesh. */
	USkeletalMeshComponent* GetWearer() const;

	/** Builds the costume pieces for Kit on Wearer (removing old ones). */
	void BuildPieces(bool bTransient);

	/** Sets mesh, tint, pose (or walk) on Body. */
	void SetupBody();

	/** Hides a watcher parent's grey stand-in (Body, Cap) without touching its collision. */
	void HideParentStandIn();

	/** The frame the frozen pose is on the bones: a seated member goes onto the poseable copy, and the body stops ticking. */
	void Freeze();

	void TickWalk(float DeltaSeconds);
	void TickBow(float DeltaSeconds);
	void ApplyBowPose(float Alpha);
	void EndBow();

	/** Creates PoseMesh (once) as a copy of Source: mesh, place, materials, lighting channels, shadow. */
	void EnsurePoseMesh(USkeletalMeshComponent* Source);

	/** Turns Bone about Axis (component space, degrees) on PoseMesh. */
	void BendBone(FName Bone, const FVector& Axis, float Degrees);

	/** Moves the pieces onto Target's bones (the poseable copy or back to the wearer). */
	void ReattachPieces(USceneComponent* Target);

	/** Seated: the frozen pose copied to PoseMesh and bent at the hips and knees; Body hidden. */
	void MakePosed();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Pieces;

	/** Each piece's bone on the wearer (NAME_None: the mesh itself). */
	TArray<FName> PieceBones;

	UPROPERTY(Transient)
	TObjectPtr<UPoseableMeshComponent> PoseMesh;

	bool bDressParent = false;
	bool bWalker = false;
	bool bPosed = false;		// shown on the poseable copy (seated)
	bool bIdling = false;		// the body plays its idle looped
	int32 FramesAlive = 0;
	FVector LastParentLocation = FVector::ZeroVector;
	float WalkSpeedSmoothed = 0.f;

	bool bBowFiredLast = false;
	float BowTime = -1.f;
	float BowCancelAlpha = 1.f;
};
