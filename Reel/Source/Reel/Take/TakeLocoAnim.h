// chars-locomotion (Nov 15 evening): the body of every ATakeCharacter (Vance, Nora, Cole), replacing ABP_Unarmed.
//
// One C++ anim instance, no Blueprint: the template's idle / walk / jog blend space (BS_Idle_Walk_Run), driven by how
// far the actor ACTUALLY moved this frame, not by the movement component's velocity. So a scene that slides him with
// SetActorLocation (a scripted walk, a push) still walks him, at the matching stride, instead of gliding in an idle.
// On top of the clip:
//   - the crouch (ATakeCharacter::IsCrouching): the pelvis lowered, the feet kept where the clip put them by two-bone IK
//     on the legs, the spine bent forward and the head kept level: a crouched idle and a crouched walk from one clip;
//   - a step when he turns on the spot (the yaw changes while he stands), so a turn is not a statue on a turntable.
// A scene may still play its own clip on the mesh (SingleNode mode, Scene 7): going back to AnimationBlueprint mode
// starts this instance again on the blend space.
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSingleNodeInstanceProxy.h"
#include "TakeLocoAnim.generated.h"

class UBlendSpace;

/** What the game thread hands the worker thread each update. */
struct FTakeLocoState
{
	float Crouch = 0.f;			// 0 standing .. 1 crouched (eased)
	float CrouchDrop = 42.f;	// cm the pelvis goes down at full crouch
	float CrouchLean = 22.f;	// degrees the spine bends forward at full crouch
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

	FTakeLocoState S;
	uint16 CachedSerial = 0xFFFF;
	int32 CachedNum = -1;
	// pelvis, spine_01, spine_03, neck_01, head, thigh_l, calf_l, foot_l, thigh_r, calf_r, foot_r
	int32 Bones[11];
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

	/** Turn rate (deg/s) above which a standing body steps round. */
	float StepTurnRate = 50.f;

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

private:
	FVector LastLoc = FVector::ZeroVector;
	float LastYaw = 0.f;
	bool bHasLast = false;
	float Measured = 0.f;
	float Shown = 0.f;
	float Step = 0.f;
};
