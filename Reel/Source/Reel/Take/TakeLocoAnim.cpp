// chars-locomotion (Nov 15 evening). See TakeLocoAnim.h.
// Mesh space here: +Y forward, -X right (+X left), +Z up (the mannequin faces +Y; the body is turned -90 in the actor).

#include "TakeLocoAnim.h"

#include "Animation/BlendSpace.h"
#include "Animation/AnimNodeBase.h"
#include "GameFramework/Actor.h"
#include "TakeCharacter.h"

namespace TakeLoco
{
	enum EBone { Pelvis, Spine1, Spine3, Neck1, Head, ThighL, CalfL, FootL, ThighR, CalfR, FootR, Num };
	const FName Names[Num] = {TEXT("pelvis"), TEXT("spine_01"), TEXT("spine_03"), TEXT("neck_01"), TEXT("head"),
		TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l"), TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r")};

	const TCHAR* SpacePath = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/BS_Idle_Walk_Run.BS_Idle_Walk_Run");

	float Ease(float X) { return FMath::SmoothStep(0.f, 1.f, FMath::Clamp(X, 0.f, 1.f)); }

	FTransform CSAt(const FCompactPose& P, int32 Index)
	{
		FCompactPoseBoneIndex I(Index);
		FTransform T = P[I];
		for (FCompactPoseBoneIndex Parent = P.GetParentBoneIndex(I); Parent.IsValid(); Parent = P.GetParentBoneIndex(Parent))
		{
			T = T * P[Parent];
		}
		return T;
	}

	FQuat ParentRot(const FCompactPose& P, int32 Index)
	{
		const FCompactPoseBoneIndex Parent = P.GetParentBoneIndex(FCompactPoseBoneIndex(Index));
		return Parent.IsValid() ? CSAt(P, Parent.GetInt()).GetRotation() : FQuat::Identity;
	}

	/** Turns a bone by Q (component space) about its own joint; its children follow. */
	void Rotate(FCompactPose& P, int32 Index, const FQuat& Q)
	{
		const FQuat PR = ParentRot(P, Index);
		FTransform& L = P[FCompactPoseBoneIndex(Index)];
		L.SetRotation((PR.Inverse() * Q * PR * L.GetRotation()).GetNormalized());
	}

	/** Two-bone IK: the foot to Target, the knee toward Pole; the foot keeps the component-space rotation FootRot. */
	void Leg(FCompactPose& P, int32 T, int32 C, int32 F, const FVector& Target, const FVector& Pole, const FQuat& FootRot)
	{
		const FVector A = CSAt(P, T).GetLocation();
		const FVector B = CSAt(P, C).GetLocation();
		const FVector Cp = CSAt(P, F).GetLocation();
		const float L1 = static_cast<float>((B - A).Size());
		const float L2 = static_cast<float>((Cp - B).Size());
		const FVector AT = Target - A;
		float D = static_cast<float>(AT.Size());
		if (L1 < 1.f || L2 < 1.f || D < 1.f)
		{
			return;
		}
		const FVector Dir = AT / D;
		D = FMath::Clamp(D, FMath::Abs(L1 - L2) + 0.5f, (L1 + L2) * 0.999f);
		FVector Side = (Pole - Dir * FVector::DotProduct(Pole, Dir)).GetSafeNormal();
		if (Side.IsNearlyZero())
		{
			Side = FVector(0.f, 1.f, 0.f);
		}
		const float Along = (L1 * L1 - L2 * L2 + D * D) / (2.f * D);
		const float Out = FMath::Sqrt(FMath::Max(L1 * L1 - Along * Along, 0.f));
		const FVector NewB = A + Dir * Along + Side * Out;
		const FVector NewC = A + Dir * D;

		Rotate(P, T, FQuat::FindBetweenNormals((B - A) / L1, (NewB - A).GetSafeNormal()));
		const FVector B2 = CSAt(P, C).GetLocation();
		const FVector C2 = CSAt(P, F).GetLocation();
		Rotate(P, C, FQuat::FindBetweenNormals((C2 - B2).GetSafeNormal(), (NewC - B2).GetSafeNormal()));
		FTransform& FootLocal = P[FCompactPoseBoneIndex(F)];
		FootLocal.SetRotation((ParentRot(P, F).Inverse() * FootRot).GetNormalized());
	}
}

// --- the instance -------------------------------------------------------------------------------------------------------

UTakeLocoAnim::UTakeLocoAnim(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UBlendSpace* UTakeLocoAnim::LocoSpace()
{
	static TWeakObjectPtr<UBlendSpace> Cached;
	if (!Cached.IsValid())
	{
		Cached = LoadObject<UBlendSpace>(nullptr, TakeLoco::SpacePath);
	}
	return Cached.Get();
}

FAnimInstanceProxy* UTakeLocoAnim::CreateAnimInstanceProxy()
{
	return new FTakeLocoProxy(this);
}

void UTakeLocoAnim::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	// Always the blend space, whatever the component's AnimationData last held (a scene's own clip, Scene 7).
	if (UBlendSpace* Space = LocoSpace())
	{
		if (GetAnimationAsset() != Space)
		{
			SetAnimationAsset(Space, true, 1.f);
		}
		SetPlaying(true);
		SetLooping(true);
		SetBlendSpacePosition(FVector::ZeroVector);
	}
	bHasLast = false;
	Measured = Shown = Step = 0.f;
}

void UTakeLocoAnim::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	const AActor* Owner = GetOwningActor();
	if (!Owner || DeltaSeconds <= 0.f)
	{
		return;
	}
	const float Dt = FMath::Min(DeltaSeconds, 0.1f);

	// How far the body really went this frame: the movement component's walk and a scene's SetActorLocation alike.
	// A jump of more than 2.5 m in one frame is a placement (a retake, a checkpoint), not a walk.
	const FVector L = Owner->GetActorLocation();
	const float Yaw = static_cast<float>(Owner->GetActorRotation().Yaw);
	float Raw = 0.f;
	float YawRate = 0.f;
	if (bHasLast)
	{
		const float D = static_cast<float>(FVector::Dist2D(L, LastLoc));
		Raw = D > 250.f ? 0.f : FMath::Min(D / DeltaSeconds, 900.f);
		const float DY = FMath::Abs(FMath::FindDeltaAngleDegrees(LastYaw, Yaw));
		YawRate = DY > 90.f ? 0.f : DY / DeltaSeconds;
	}
	LastLoc = L;
	LastYaw = Yaw;
	bHasLast = true;
	Measured = FMath::FInterpTo(Measured, Raw, Dt, 14.f);

	// Turning on the spot: a slow step, so the feet move with the turn.
	const float WantStep = (Measured < 40.f && YawRate > StepTurnRate) ? FMath::Clamp(YawRate * 0.6f, 60.f, 130.f) : 0.f;
	Step = FMath::FInterpTo(Step, WantStep, Dt, 8.f);
	Shown = FMath::Max(Measured, Step);
	SetBlendSpacePosition(FVector(Shown < 6.f ? 0.f : Shown, 0.f, 0.f));

	// The crouch eases over about a fifth of a second, starting the frame it is asked for.
	const ATakeCharacter* Character = Cast<ATakeCharacter>(Owner);
	const float Want = Character && Character->IsCrouching() ? 1.f : 0.f;
	State.Crouch = FMath::FInterpConstantTo(State.Crouch, Want, Dt, 5.f);
	GetProxyOnGameThread<FTakeLocoProxy>().SetState(State);
}

// --- the proxy ----------------------------------------------------------------------------------------------------------

void FTakeLocoProxy::CacheBones(const FCompactPose& Pose)
{
	const FBoneContainer& BC = Pose.GetBoneContainer();
	if (BC.GetSerialNumber() == CachedSerial && Pose.GetNumBones() == CachedNum)
	{
		return;
	}
	CachedSerial = BC.GetSerialNumber();
	CachedNum = Pose.GetNumBones();
	for (int32 b = 0; b < TakeLoco::Num; ++b)
	{
		const int32 MeshIndex = BC.GetPoseBoneIndexForBoneName(TakeLoco::Names[b]);
		Bones[b] = MeshIndex != INDEX_NONE ? BC.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIndex)).GetInt() : INDEX_NONE;
	}
}

bool FTakeLocoProxy::Evaluate(FPoseContext& Output)
{
	FAnimSingleNodeInstanceProxy::Evaluate(Output);
	if (S.Crouch < 0.001f)
	{
		return true;
	}
	using namespace TakeLoco;
	FCompactPose& Pose = Output.Pose;
	CacheBones(Pose);
	for (int32 b = 0; b < Num; ++b)
	{
		if (Bones[b] == INDEX_NONE)
		{
			return true;	// a low LOD without the legs: no crouch
		}
	}
	const float W = Ease(S.Crouch);

	// Where the clip put the feet (they stay there).
	const FTransform FootL0 = CSAt(Pose, Bones[FootL]);
	const FTransform FootR0 = CSAt(Pose, Bones[FootR]);

	// The hips down and a little back.
	{
		const FQuat PR = ParentRot(Pose, Bones[Pelvis]);
		Pose[FCompactPoseBoneIndex(Bones[Pelvis])].AddToTranslation(PR.Inverse().RotateVector(FVector(0.f, -6.f * W, -S.CrouchDrop * W)));
	}
	// The back bent forward (a turn about +X takes up toward the back, so forward is negative), the head kept level.
	const FVector Swing(1.f, 0.f, 0.f);
	Rotate(Pose, Bones[Spine1], FQuat(Swing, FMath::DegreesToRadians(-0.6f * S.CrouchLean * W)));
	Rotate(Pose, Bones[Spine3], FQuat(Swing, FMath::DegreesToRadians(-0.4f * S.CrouchLean * W)));
	Rotate(Pose, Bones[Neck1], FQuat(Swing, FMath::DegreesToRadians(0.8f * S.CrouchLean * W)));

	// The legs fold to keep the feet down: knees forward and a little out.
	Leg(Pose, Bones[ThighL], Bones[CalfL], Bones[FootL], FootL0.GetLocation(), FVector(0.3f, 1.f, 0.f), FootL0.GetRotation());
	Leg(Pose, Bones[ThighR], Bones[CalfR], Bones[FootR], FootR0.GetLocation(), FVector(-0.3f, 1.f, 0.f), FootR0.GetRotation());
	return true;
}
