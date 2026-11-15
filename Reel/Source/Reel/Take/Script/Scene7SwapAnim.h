// The reel swap's Vance double: a native anim instance (no Anim Blueprint) that plays a breathing idle and lays
// procedural acting on top: a spine lean, a head look, two-bone IK on both arms (hands onto the reel, the magazine
// door and the canister) and a finger grip. Owner: s7-reelswap. AScene7Swap fills FScene7SwapPose every tick.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Scene7SwapAnim.generated.h"

class UAnimSequence;

/** One hand's goal, in the skeletal mesh's component space. */
USTRUCT()
struct FScene7SwapHand
{
	GENERATED_BODY()

	/** 0 = the clip's arm, 1 = fully on the target. */
	float Alpha = 0.f;
	/** Where the middle of the palm goes. */
	FVector Palm = FVector::ZeroVector;
	/** The way the fingers point, and the way the palm faces (unit vectors). */
	FVector Fingers = FVector::ForwardVector;
	FVector PalmFacing = -FVector::UpVector;
	/** Where the elbow should point toward (a point, not a direction). */
	FVector Pole = FVector::ZeroVector;
	/** 0 open, 1 a closed grip. */
	float Grip = 0.f;
};

/** Everything the double does this frame (component space). */
USTRUCT()
struct FScene7SwapPose
{
	GENERATED_BODY()

	FScene7SwapHand Left;
	FScene7SwapHand Right;
	/** Forward bend of the spine (degrees, split over the three spine bones), about LeanAxis. */
	float Lean = 0.f;
	FVector LeanAxis = FVector::RightVector;
	/** The head looks at this point (component space) by LookAlpha. */
	FVector LookAt = FVector::ZeroVector;
	float LookAlpha = 0.f;
};

class FScene7SwapAnimProxy : public FAnimInstanceProxy
{
public:
	FScene7SwapAnimProxy() = default;
	explicit FScene7SwapAnimProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;

	FScene7SwapPose Pose;
	UAnimSequence* Base = nullptr;
	float Time = 0.f;
	float PalmOffset = 8.f;
	float GripDegrees = 70.f;
};

UCLASS(Transient, NotBlueprintable)
class REEL_API UScene7SwapAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** Set by AScene7Swap each tick, read on the anim thread through the proxy. */
	FScene7SwapPose Pose;

	UPROPERTY(Transient) TObjectPtr<UAnimSequence> Base;
	/** Seconds into Base (loops). */
	float BaseTime = 0.f;
	/** Wrist to the middle of the palm (cm). */
	float PalmOffset = 8.f;
	/** A full grip bends each finger joint this far. */
	float GripDegrees = 70.f;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override { return new FScene7SwapAnimProxy(this); }
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override { delete InProxy; }
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
