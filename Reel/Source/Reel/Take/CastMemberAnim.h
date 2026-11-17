// QA pass (Nov 14), package cast: the cast's own anim instance (no Blueprint: everything here is C++).
//
// A cast member's body plays one base clip (an idle, a seated idle, an instrument, or the walk blend space) through
// the single-node player, and on top of it, in this order:
//   1. an act: a mocap clip (tools/mocap_clips.json, /Game/Take/Anim/Mocap) blended in, played once (or looped for a
//      while) and blended back out, on the whole body or the upper body only; when the clip is missing, the same act
//      is built from bones (two-bone IK on the arms, the spine, the head);
//   2. postures held by IK when no clip covers them: hands on the lap (the seated fallback), a glass, hands clasped,
//      a hand on the podium; the band's playing when no instrument clip exists;
//   3. the seated fallback: the standing clip bent at the hips and knees (step 5's pose), used only without SitIdle;
//   4. a look: the head and the top of the spine turn toward a point, eased, on whatever plays.
// Base changes and act switches cross-fade from the last pose (0.25 s), so nothing pops.
// Mesh space here: +Y forward, -X right, +Z up (the mannequin faces +Y; the body is turned -90 in the actor).
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSingleNodeInstanceProxy.h"
#include "CastMemberAnim.generated.h"

class USkeletalMeshComponent;
class UAnimSequenceBase;

/** What a cast member does with its body for a moment (a clip when one exists, else built from bones). */
UENUM(BlueprintType)
enum class ECastAct : uint8
{
	None,
	Refuse,		// the stop hand
	Wave,		// "go on through": a sweep toward the door
	Hello,		// a raised hand
	Applaud,
	Talk,
	Listen,
	Laugh,
	Sip,
	Nod,
	Shrug,
	Point,
	CheckWatch,
	LookAround,
	Headset,	// a hand to the headset (spoken to)
	Use,		// a reach to a device
	Lift,		// a reach down and up
	Bow,
	Speech,		// Vance at the podium (loops while he speaks)
	Aim			// chars-locomotion: the pistol raised (the template's pistol aim, upper body, held)
};

/** The band's instrument: its playing is built from bones when no instrument clip exists. */
enum class ECastInstrument : uint8 { None, Piano, Drums, Bass, Trumpet, Singer };

/** A posture the arms hold by IK under the idle. */
enum class ECastHold : uint8 { None, Lap, Glass, ClaspFront, ClaspBack, Podium };

/** The bones the layers use (UE5 mannequin names, CastMemberAnim.cpp). */
namespace CastBone
{
	enum Type : uint8
	{
		Root, Pelvis, Spine1, Spine2, Spine3, Spine4, Spine5, Neck1, Neck2, Head,
		ClavL, ClavR, UpperL, UpperR, LowerL, LowerR, HandL, HandR, MiddleL, MiddleR, ThumbL, ThumbR,
		ThighL, ThighR, CalfL, CalfR, Num
	};
}

/** How long an act built from bones lasts by default, seconds (below 0: until the next act). */
REEL_API float CastActDefaultSeconds(ECastAct Act);

/** Everything the actor tells its body, written on the game thread (ACastMember), copied by the proxy each update. */
USTRUCT()
struct FCastActing
{
	GENERATED_BODY()

	/** Motion layers on. Off: postures only (step 5's still cast). */
	bool bActing = true;
	uint32 Seed = 0;
	/** Size of the gestures built from bones (1 = normal; Vance 1.3). */
	float Energy = 1.f;

	/** The seated fallback: the base clip is a standing one, bent at the hips and knees. */
	bool bBendSeat = false;
	float SeatHipDegrees = 90.f;
	float SeatKneeDegrees = 90.f;

	ECastHold Hold = ECastHold::None;
	/** Podium: where the left hand rests, in the actor's frame (x forward, y right, z up from the feet, cm). */
	FVector HoldPointActor = FVector::ZeroVector;

	/** Look at LookWorld (eased; turned off when the point is behind). */
	bool bLook = false;
	FVector LookWorld = FVector::ZeroVector;
	float LookWeight = 0.85f;
	float LookSmoothSeconds = 0.35f;
	float LookMaxYaw = 70.f;
	float LookMaxPitch = 30.f;

	/** A weight shift, degrees (eased over about a second): forward, to the right, shoulders turned right. */
	float LeanForward = 0.f;
	float LeanSide = 0.f;
	float Twist = 0.f;

	/** The act: a new ActSerial starts Act (a clip when ActClip is set). */
	ECastAct Act = ECastAct::None;
	int32 ActSerial = 0;
	/** Seconds: 0 = the clip's length (or the act's own); below 0 = until the next act. */
	float ActSeconds = 0.f;
	bool bActPoint = false;
	FVector ActWorld = FVector::ZeroVector;
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> ActClip;
	float ActRate = 1.f;
	/** Blend the act's clip into the spine and above only (a standing clip on a seated or walking body). */
	bool bActUpperBody = false;
	float BlendIn = 0.3f;
	float BlendOut = 0.4f;

	/** The band, built from bones (no instrument clip): Beat is beats since the clock started. */
	ECastInstrument Instrument = ECastInstrument::None;
	bool bPlayInstrument = false;
	float Beat = 0.f;
	/** --- W3 cast2 --- the built playing's hand targets moved by this (the actor's frame: forward, right, up; cm), so
	 *  the hands meet the real prop (ACastMember::BandReach). */
	FVector BandShift = FVector::ZeroVector;
	/** --- W3 cast2 --- a hero lead's arms brought to rest (ACastMember::HeroArmIn...), degrees; 0 = off. */
	float HeroArmIn = 0.f;
	float HeroElbowBend = 0.f;
	float HeroPalmTurn = 0.f;

	/** A new BaseSerial cross-fades from the last pose (the base clip or the copied pose changed). */
	int32 BaseSerial = 0;

	/** The crew: the base pose is copied from the character's own mesh (same skeletal mesh). */
	TWeakObjectPtr<USkeletalMeshComponent> CopyFrom;

	/** The actor's frame to this component's space (the inverse of the body's relative transform). */
	FTransform ActorToComponent = FTransform::Identity;
};

/** The proxy: the layers above, evaluated on the animation worker threads. */
USTRUCT()
struct FCastAnimProxy : public FAnimSingleNodeInstanceProxy
{
	GENERATED_BODY()

public:
	FCastAnimProxy() {}
	FCastAnimProxy(UAnimInstance* InAnimInstance) : FAnimSingleNodeInstanceProxy(InAnimInstance) {}

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual void UpdateAnimationNode(const FAnimationUpdateContext& InContext) override;
	virtual bool Evaluate(FPoseContext& Output) override;

private:
	void CacheBones(const FCompactPose& Pose);
	void ApplyLayers(FCompactPose& Pose);

	FCastActing A;
	FTransform ComponentToWorldInv = FTransform::Identity;
	FVector LookCS = FVector::ZeroVector;
	FVector ActCS = FVector::ZeroVector;
	TArray<FTransform> CopiedPose;
	bool bCopy = false;
	bool bReelArm = false;	// the carried reel hangs on this body: the left arm is the copied pose's, untouched

	float Time = 0.f;

	// The look, eased (degrees in the body's frame: yaw to the right, pitch up).
	float LookYaw = 0.f, LookYawVel = 0.f;
	float LookPitch = 0.f, LookPitchVel = 0.f;
	float LookW = 0.f, LookWVel = 0.f;
	FVector LastHeadCS = FVector(0.f, 0.f, 160.f);

	float Lean[3] = {0.f, 0.f, 0.f};
	float LeanVel[3] = {0.f, 0.f, 0.f};

	// The act.
	int32 SeenSerial = INDEX_NONE;
	ECastAct CurAct = ECastAct::None;
	UAnimSequenceBase* CurClip = nullptr;	// kept alive by the instance's Acting.ActClip
	float ActT = 0.f;
	float ActLen = 0.f;
	float ActStartW = 0.f;
	float ActW = 0.f;
	float ClipTime = 0.f;
	bool bClipLoop = false;
	bool bUpperOnly = false;

	// Cross-fade from the last pose.
	int32 SeenBaseSerial = INDEX_NONE;
	TArray<FTransform> LastOut;
	TArray<FTransform> Snapshot;
	float SnapLeft = 0.f;
	float SnapSeconds = 0.25f;

	// Bone cache, rebuilt when the required bones change (LOD).
	uint16 CachedSerial = 0xFFFF;
	int32 CachedNum = -1;
	int32 Bones[CastBone::Num];
	TArray<uint8> UpperMask;
	FQuat RefHeadCS = FQuat::Identity;
};

/** The cast's anim instance: the single-node player (one base clip) plus the layers in FCastAnimProxy. */
UCLASS(Transient, NotBlueprintable)
class REEL_API UCastAnimInstance : public UAnimSingleNodeInstance
{
	GENERATED_BODY()

public:
	UCastAnimInstance(const FObjectInitializer& ObjectInitializer);

	/** Written by ACastMember on the game thread. */
	UPROPERTY(Transient)
	FCastActing Acting;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
};
