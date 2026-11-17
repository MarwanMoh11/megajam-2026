// Scene 6's stunt rig (owner: s6-backstage). A full-body pose built from world-space targets, written bone by bone
// into a hidden UPoseableMeshComponent that a visible mesh follows (SetLeaderPoseComponent), so Cole's own mesh and
// the guard's costumed body play the choke-hold, the sag, the drag and Cole's crouch-walk without any clip.
//
// A pose names where the pelvis goes, how the hips, chest and head are turned (a forward and an up each), where the
// hands and ankles go and which way the elbows and knees point. The rig orients the trunk bones as if the whole body
// were turned to each frame (each keeps its own reference offset), spreads the bend over the spine, and solves each
// arm and leg as a two-bone chain aimed at its target. Mannequin bone names (UE5 Manny); mesh space +Y forward.
#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"

class UPoseableMeshComponent;
class USkeletalMeshComponent;

/** A body's turn: where its front faces and where the top of its head points (world, need not be unit). */
struct FS6Frame
{
	FVector Fwd = FVector::ForwardVector;
	FVector Up = FVector::UpVector;
	FS6Frame() {}
	FS6Frame(const FVector& InFwd, const FVector& InUp) : Fwd(InFwd), Up(InUp) {}
	FQuat Quat() const;
	static FS6Frame Lerp(const FS6Frame& A, const FS6Frame& B, float T);
	/** Standing turned to Fwd (flattened), leaning forward by Lean degrees (negative leans back), rolled by Roll. */
	static FS6Frame Lean(const FVector& Fwd, float LeanDeg, float RollDeg = 0.f);
};

/** World-space targets for one frame. Index 0 is left, 1 is right. */
struct FS6Pose
{
	FVector Pelvis = FVector::ZeroVector;
	FS6Frame Hips, Chest, Head;
	FVector Hand[2] = { FVector::ZeroVector, FVector::ZeroVector };
	FVector Elbow[2] = { FVector::ZeroVector, FVector::ZeroVector };	// a point the elbow bends toward
	FVector Ankle[2] = { FVector::ZeroVector, FVector::ZeroVector };
	FVector Knee[2] = { FVector::ZeroVector, FVector::ZeroVector };	// a point the knee bends toward
	FS6Frame Foot[2];	// how each foot is turned (as if the whole body were turned so)
	// The hands (Marwan's QA, Nov 17: wrists left straight on the forearm, palms wherever the arm's twist put them,
	// fingers frozen open in the holds). HandAim 0: as before; 1: the hand turned so its fingers point along Finger and
	// its palm faces Palm (world; the wrist bends at most FS6Rig::WristMaxBend, part of the turn goes to the forearm).
	// Grip curls the fingers from the body's own relaxed hand: 1 closed round an arm or a collar, below 0 flatter.
	FVector Finger[2] = { FVector::ZeroVector, FVector::ZeroVector };
	FVector Palm[2] = { FVector::ZeroVector, FVector::ZeroVector };
	float HandAim[2] = { 0.f, 0.f };
	float Grip[2] = { 0.f, 0.f };
	void SetHand(int32 S, const FVector& InFinger, const FVector& InPalm, float InGrip, float InAim = 1.f)
	{
		Finger[S] = InFinger.GetSafeNormal();
		Palm[S] = InPalm.GetSafeNormal();
		Grip[S] = InGrip;
		HandAim[S] = InAim;
	}

	static FS6Pose Lerp(const FS6Pose& A, const FS6Pose& B, float T);
};

class FS6Rig
{
public:
	/** Pose: the hidden poseable this rig writes; Follower: the visible mesh that copies it while attached. */
	void Init(UPoseableMeshComponent* InPose, USkeletalMeshComponent* InFollower);
	bool IsReady() const;
	bool IsAttached() const { return bAttached; }

	/** The follower takes the rig's pose (from the reference pose until Apply). */
	void Attach();
	/** The follower goes back to its own animation. */
	void Detach();

	/** Writes the pose (the poseable is first put where the follower is). */
	void Apply(const FS6Pose& P);

	/** A bone's world location as last posed. */
	FVector Bone(FName Name) const;

	/** The follower's feet (its component origin) and facing (mesh +Y), right now. */
	FVector FeetNow() const;
	FVector FwdNow() const;

	// Measured from the reference pose at Attach (cm, world scale): heights over the feet and limb lengths.
	float PelvisH = 95.f;
	float NeckH = 150.f;
	float HeadH = 165.f;
	float ShoulderH = 140.f;
	float ShoulderHalf = 18.f;	// half the shoulder width
	float HipHalf = 10.f;
	float AnkleH = 9.f;
	float Arm1 = 28.f, Arm2 = 26.f, Leg1 = 44.f, Leg2 = 42.f;

	/** A standing (or crouched, by Drop cm) pose on Feet facing Fwd, arms hanging: the base the scene bends. */
	FS6Pose Stand(const FVector& Feet, const FVector& Fwd, float Drop = 0.f) const;

	/** How far an aimed hand may bend from the forearm, degrees; the share of its turn about the forearm that the
	 *  forearm takes (the rest twists at the wrist); the curl a full Grip adds to the finger joints (base, middle, tip),
	 *  degrees, and to the thumb's two outer joints. */
	float WristMaxBend = 62.f;
	float ForearmTwistShare = 0.55f;
	FVector GripCurl = FVector(42.f, 52.f, 30.f);
	float ThumbCurl = 28.f;

private:
	TWeakObjectPtr<UPoseableMeshComponent> Pose;
	TWeakObjectPtr<USkeletalMeshComponent> Follower;
	TMap<FName, FQuat> RefCS;	// component-space rotations in the reference pose
	bool bAttached = false;

	// The hands' own axes in the hand bone's space (from the reference pose): the way the fingers point, the way the
	// palm faces.
	FVector HandFingerLocal[2] = { FVector::ForwardVector, FVector::ForwardVector };
	FVector HandPalmLocal[2] = { FVector::UpVector, FVector::UpVector };
	bool bHandAxes = false;
	/** One finger joint: the body's relaxed hand (bone space, taken at Attach), the axis it curls about (bone space),
	 *  its side and how much of the grip it takes (degrees at Grip 1). */
	struct FFingerJoint { int32 Index = INDEX_NONE; int32 Side = 0; FTransform Relaxed; FVector Axis = FVector::UpVector; float CurlDeg = 0.f; };
	TArray<FFingerJoint> Fingers;

	void SetupHands();
	void PoseHand(int32 S, const FS6Pose& P);
	void PoseFingers(const FS6Pose& P);

	void SyncTransform();
	void Measure();
	void ResetAll();
	FTransform Get(FName B) const;
	void Orient(FName B, const FS6Frame& F);
	void Aim(FName B, FName Child, const FVector& Target);
	void Chain(FName A, FName B, FName C, const FVector& Target, const FVector& Pole, float L1, float L2);
};
