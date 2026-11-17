// chars-locomotion (Nov 15 evening): the body of every ATakeCharacter (Vance, Nora, Cole), replacing ABP_Unarmed.
//
// One C++ anim instance, no Blueprint: the template's idle / walk / jog blend space (BS_Idle_Walk_Run), driven by how
// far the actor ACTUALLY moved this frame, not by the movement component's velocity. So a scene that slides him with
// SetActorLocation (a scripted walk, a push) still walks him, at the matching stride, instead of gliding in an idle.
// On top of the clip:
//   - the crouch (ATakeCharacter::IsCrouching): the pelvis lowered, the feet kept where the clip put them by two-bone IK
//     on the legs, the spine bent forward and the head kept level: a crouched idle and a crouched walk from one clip;
//   - a step when he turns on the spot (the yaw changes while he stands), so a turn is not a statue on a turntable;
//   - calm arms (Marwan's QA, Nov 17: the run pumped the hands up to the face): the arms, hands and fingers held near
//     the idle's, keeping only a little of the walk's and run's own swing (Reel.Hero.ArmCalm).
// A scene may still play its own clip on the mesh (SingleNode mode, Scene 7): going back to AnimationBlueprint mode
// starts this instance again on the blend space.
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSingleNodeInstanceProxy.h"
#include "TakeLocoAnim.generated.h"

class UBlendSpace;
class UAnimSequenceBase;

/** What the game thread hands the worker thread each update. */
struct FTakeLocoState
{
	float Crouch = 0.f;			// 0 standing .. 1 crouched (eased)
	float CrouchDrop = 42.f;	// cm the pelvis goes down at full crouch
	float CrouchLean = 22.f;	// degrees the spine bends forward at full crouch

	// The reel under the left arm (all relative to the hold bone, so the hand stays on the can as the chest moves).
	float Carry = 0.f;							// 0 arms free .. 1 the left arm round the reel (eased)
	FVector CarryWrist = FVector::ZeroVector;	// where the wrist goes
	FVector CarryFinger = FVector::ForwardVector;	// the way the fingers point (round the rim)
	FVector CarryBack = FVector::UpVector;		// the way the back of the hand faces (away from the body)
	FVector CarryPole = FVector::ForwardVector;	// the way the elbow points
	FVector CarryCurl = FVector::ZeroVector;	// degrees the three finger joints curl over the rim

	// The arms held near the idle's: 0 the clip's own arms .. 1 the idle's, still. IdleClip is kept alive by the
	// instance (UTakeLocoAnim::CalmIdle).
	float ArmCalm = 0.f;
	UAnimSequenceBase* IdleClip = nullptr;
};

USTRUCT()
struct FTakeLocoProxy : public FAnimSingleNodeInstanceProxy
{
	GENERATED_BODY()

public:
	FTakeLocoProxy() {}
	FTakeLocoProxy(UAnimInstance* InAnimInstance) : FAnimSingleNodeInstanceProxy(InAnimInstance) {}

	/** The game thread's state for this update (written in UTakeLocoAnim::NativeUpdateAnimation, same frame). */
	void SetState(const FTakeLocoState& InState) { S = InState; }

	virtual bool Evaluate(FPoseContext& Output) override;

private:
	void CacheBones(const FCompactPose& Pose);
	void Crouch(FCompactPose& Pose);
	void Carry(FCompactPose& Pose);
	void CalmArms(FPoseContext& Output);

	FTakeLocoState S;
	uint16 CachedSerial = 0xFFFF;
	int32 CachedNum = -1;
	// pelvis, spine_01, spine_03, neck_01, head, thigh_l, calf_l, foot_l, thigh_r, calf_r, foot_r
	int32 Bones[11];
	// spine_03 (the hold bone), upperarm_l, lowerarm_l, hand_l, then index/middle/ring/pinky _01 _02 _03 (left)
	int32 Arm[16];
	// The compact bones from each clavicle down (the arms, hands and fingers).
	TArray<bool> ArmMask;
};

UCLASS(Transient, NotBlueprintable)
class REEL_API UTakeLocoAnim : public UAnimSingleNodeInstance
{
	GENERATED_BODY()

public:
	UTakeLocoAnim(const FObjectInitializer& ObjectInitializer);

	/** The speed the legs show this frame, cm/s (measured, smoothed). */
	float GetShownSpeed() const { return Shown; }

	/** Copied by the proxy every update. */
	FTakeLocoState State;

	/** The blend space every character walks on (its axis is cm/s). */
	static UBlendSpace* LocoSpace();

	/** Puts Reel under Body's left arm: on the chest bone, upright along his side, tagged so the body's UTakeLocoAnim
	 *  wraps the left arm round it while it is there. False (nothing done) if Body has no such bone. */
	static bool CarryReel(USkeletalMeshComponent* Body, USceneComponent* Reel, bool bKeepScale = false);
	/** The visible reel CarryReel hung on Body, or null. */
	static USceneComponent* FindCarriedReel(const USceneComponent* Body);

	/** A scene's own clip on a body that walks on this instance (the Scene 7 fight), keeping the reel arm: the mesh
	 *  stays on this instance, which stops driving the blend space, so Mesh->SetAnimation / Play / SetPosition play
	 *  the clip and the left arm still holds the can. False (nothing done) if Mesh does not use this class. */
	static bool BeginClip(USkeletalMeshComponent* Mesh);
	/** Plays Anim on Mesh: through BeginClip when Mesh walks on this class (the reel arm stays), else PlayAnimation. */
	static void PlayClip(USkeletalMeshComponent* Mesh, UAnimationAsset* Anim, bool bLoop);
	/** Back to the walk after BeginClip. */
	static void EndClip(USkeletalMeshComponent* Mesh);

	/** Turn rate (deg/s) above which a standing body steps round. */
	float StepTurnRate = 50.f;

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

private:
	void StartWalk();
	void UpdateCarry(float Dt);

	bool bClip = false;		// a scene's clip is playing on this instance (BeginClip): no blend space, no crouch
	/** The idle the calm arms rest from (the blend space's own idle). */
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> CalmIdle;
	FVector LastLoc = FVector::ZeroVector;
	float LastYaw = 0.f;
	bool bHasLast = false;
	float Measured = 0.f;
	float Shown = 0.f;
	float Step = 0.f;
};
