// Step 5, characters and props: one member of the cast on camera A, costumed from basic shapes.
//
// Three ways to use it (tools/build_cast.py places them all):
// - A posed guest or a band player: its own mannequin (Body), frozen in one pose of a template animation, with
//   costume pieces and an instrument attached to its bones (or standing on the floor), moving its upper body to the
//   band (the groove: on the bar clock's beat, see "Groove" below).
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

	// The groove (the bob on the beat): only the upper body moves (spine, neck, head on a poseable copy of the body),
	// never the whole mesh, on smooth cosine curves locked to the bar clock. Each curve is at its deepest on the beat's
	// frame (offset by the member's phase) and back at rest half a period later. tools/build_cast.py sets these per
	// role: the band most, each by instrument; guests a small nod or a weight shift, and many not at all.

	/** The chance (0-1) that this member moves at all, drawn once from its name (stable across runs). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Groove", meta = (ClampMin = "0", ClampMax = "1"))
	float MoveChance = 1.f;

	/** The nod: neck and head forward, deepest on the beat; NodBeats is its period in beats (2 = half time). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Groove")
	float NodDegrees = 0.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cast|Groove")
	float NodBeats = 1.f;

	/** The rock: the spine leaning forward and back (a dip on the phrase with RockBeats 4: a bar). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Groove")
	float RockDegrees = 0.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cast|Groove")
	float RockBeats = 2.f;

	/** The sway: the spine leaning side to side over SwayBeats (one side on the beat), the head kept level. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Groove")
	float SwayDegrees = 0.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cast|Groove")
	float SwayBeats = 2.f;

	/** The member's own offset in beats (0.5 = on the off-beat). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Groove")
	float PhaseBeats = 0.f;

	/** Drawn from the name: the chance of moving on the off-beat instead, of nodding at half time, a timing spread
	 *  (beats, evenly around the phase) and how much smaller than the full amplitude it may be (0-1). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Groove", meta = (ClampMin = "0", ClampMax = "1"))
	float OffbeatChance = 0.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cast|Groove", meta = (ClampMin = "0", ClampMax = "1"))
	float HalfTimeChance = 0.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cast|Groove")
	float PhaseSpreadBeats = 0.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cast|Groove", meta = (ClampMin = "0", ClampMax = "1"))
	float AmpSpread = 0.f;

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

	/** The frame the frozen pose is on the bones: seated or moving members go onto the poseable copy. */
	void Freeze();

	/** Draws the groove from the name: moves or not, its phase, half time, its amplitude. */
	void SetupGroove();

	void TickGroove();
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

	/** The frozen pose copied to PoseMesh (seated: bent at the hips and knees); Body hidden. */
	void MakePosed();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Pieces;

	/** Each piece's bone on the wearer (NAME_None: the mesh itself). */
	TArray<FName> PieceBones;

	UPROPERTY(Transient)
	TObjectPtr<UPoseableMeshComponent> PoseMesh;

	bool bDressParent = false;
	bool bWalker = false;
	bool bPosed = false;		// shown on the poseable copy (seated, or moving to the band)
	int32 FramesAlive = 0;
	FVector LastParentLocation = FVector::ZeroVector;
	float WalkSpeedSmoothed = 0.f;

	/** One bone the groove turns: its rest rotation (parent space), the nod and roll axes in parent space, and its
	 *  share of each curve. */
	struct FGrooveBone
	{
		int32 Index = INDEX_NONE;
		FQuat Rest = FQuat::Identity;
		FVector NodAxis = FVector::ZeroVector;
		FVector RollAxis = FVector::ZeroVector;
		float Nod = 0.f;
		float Rock = 0.f;
		float Sway = 0.f;
	};
	TArray<FGrooveBone> GrooveBones;
	bool bGrooves = false;
	float GroovePhase = 0.f;
	float GrooveNodBeats = 1.f;
	float GrooveScale = 1.f;
	bool bBowFiredLast = false;
	float BowTime = -1.f;
	float BowCancelAlpha = 1.f;
};
