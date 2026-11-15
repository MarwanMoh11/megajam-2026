#include "Script/Scene7SwapAnim.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/AnimNodeBase.h"
#include "BonePose.h"
#include "TwoBoneIK.h"

void UScene7SwapAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	BaseTime += DeltaSeconds;
}

void FScene7SwapAnimProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
	if (const UScene7SwapAnimInstance* I = Cast<UScene7SwapAnimInstance>(InAnimInstance))
	{
		Pose = I->Pose;
		Base = I->Base;
		Time = I->BaseTime;
		PalmOffset = I->PalmOffset;
		GripDegrees = I->GripDegrees;
	}
}

namespace
{
	FCompactPoseBoneIndex Bone(const FBoneContainer& BC, const FString& Name)
	{
		const int32 MeshIndex = BC.GetPoseBoneIndexForBoneName(FName(*Name));
		if (MeshIndex == INDEX_NONE) { return FCompactPoseBoneIndex(INDEX_NONE); }
		return BC.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIndex));
	}

	void SetCS(FCSPose<FCompactPose>& CS, FCompactPoseBoneIndex I, const FTransform& T)
	{
		TArray<FBoneTransform> One;
		One.Add(FBoneTransform(I, T));
		CS.SafeSetCSBoneTransforms(One);
	}

	/** Rotate one bone in component space about its own pivot (its children follow). */
	void Turn(FCSPose<FCompactPose>& CS, FCompactPoseBoneIndex I, const FQuat& Q)
	{
		if (!I.IsValid()) { return; }
		FTransform T = CS.GetComponentSpaceTransform(I);
		T.SetRotation((Q * T.GetRotation()).GetNormalized());
		SetCS(CS, I, T);
	}

	/** The hand's fingers direction, knuckle line (index minus pinky) and palm facing, in component space. */
	bool HandFrame(FCSPose<FCompactPose>& CS, const FBoneContainer& BC, const TCHAR* S, bool bRight, FVector& F, FVector& K, FVector& P)
	{
		const FCompactPoseBoneIndex H = Bone(BC, FString::Printf(TEXT("hand_%s"), S));
		const FCompactPoseBoneIndex M = Bone(BC, FString::Printf(TEXT("middle_01_%s"), S));
		const FCompactPoseBoneIndex Ix = Bone(BC, FString::Printf(TEXT("index_01_%s"), S));
		const FCompactPoseBoneIndex Pk = Bone(BC, FString::Printf(TEXT("pinky_01_%s"), S));
		if (!H.IsValid() || !M.IsValid() || !Ix.IsValid() || !Pk.IsValid()) { return false; }
		const FVector HP = CS.GetComponentSpaceTransform(H).GetLocation();
		F = (CS.GetComponentSpaceTransform(M).GetLocation() - HP).GetSafeNormal();
		K = (CS.GetComponentSpaceTransform(Ix).GetLocation() - CS.GetComponentSpaceTransform(Pk).GetLocation()).GetSafeNormal();
		P = bRight ? FVector::CrossProduct(F, K) : FVector::CrossProduct(K, F);
		P = P.GetSafeNormal();
		return !F.IsNearlyZero() && !P.IsNearlyZero();
	}

	void SolveArm(FCSPose<FCompactPose>& CS, const FBoneContainer& BC, const TCHAR* S, bool bRight, const FScene7SwapHand& H, float PalmOffset, float GripDegrees)
	{
		if (H.Alpha <= KINDA_SMALL_NUMBER) { return; }
		const FCompactPoseBoneIndex UI = Bone(BC, FString::Printf(TEXT("upperarm_%s"), S));
		const FCompactPoseBoneIndex LI = Bone(BC, FString::Printf(TEXT("lowerarm_%s"), S));
		const FCompactPoseBoneIndex HI = Bone(BC, FString::Printf(TEXT("hand_%s"), S));
		if (!UI.IsValid() || !LI.IsValid() || !HI.IsValid()) { return; }

		FVector F, K, P;
		FQuat Delta = FQuat::Identity;
		if (HandFrame(CS, BC, S, bRight, F, K, P))
		{
			const FQuat Cur = FRotationMatrix::MakeFromXZ(F, P).ToQuat();
			const FQuat Want = FRotationMatrix::MakeFromXZ(H.Fingers.GetSafeNormal(), H.PalmFacing.GetSafeNormal()).ToQuat();
			Delta = Want * Cur.Inverse();
		}

		FTransform U = CS.GetComponentSpaceTransform(UI);
		FTransform L = CS.GetComponentSpaceTransform(LI);
		FTransform Hd = CS.GetComponentSpaceTransform(HI);
		const FQuat HandRot = Hd.GetRotation();
		const FVector Wrist = H.Palm - H.Fingers.GetSafeNormal() * PalmOffset;
		AnimationCore::SolveTwoBoneIK(U, L, Hd, H.Pole, Wrist, false, 1.0, 1.0);
		Hd.SetRotation((Delta * HandRot).GetNormalized());

		TArray<FBoneTransform> Arm;
		Arm.Add(FBoneTransform(UI, U));
		Arm.Add(FBoneTransform(LI, L));
		Arm.Add(FBoneTransform(HI, Hd));
		Arm.Sort(FCompareBoneTransformIndex());
		CS.LocalBlendCSBoneTransforms(Arm, FMath::Clamp(H.Alpha, 0.f, 1.f));

		// The grip: each finger joint bends about the knuckle line, toward the palm.
		const float G = FMath::Clamp(H.Grip, 0.f, 1.f) * FMath::Clamp(H.Alpha, 0.f, 1.f);
		if (G <= KINDA_SMALL_NUMBER || !HandFrame(CS, BC, S, bRight, F, K, P)) { return; }
		static const TCHAR* Fingers[] = { TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky") };
		const float Sign = FVector::DotProduct(FVector::CrossProduct(K, F), P) >= 0.f ? 1.f : -1.f;
		for (const TCHAR* Finger : Fingers)
		{
			for (int32 J = 1; J <= 3; ++J)
			{
				const FCompactPoseBoneIndex I = Bone(BC, FString::Printf(TEXT("%s_0%d_%s"), Finger, J, S));
				const float Deg = GripDegrees * G * (J == 1 ? 0.75f : 1.f);
				Turn(CS, I, FQuat(K, Sign * FMath::DegreesToRadians(Deg)));
			}
		}
		// The thumb closes a little across the palm.
		for (int32 J = 2; J <= 3; ++J)
		{
			const FCompactPoseBoneIndex I = Bone(BC, FString::Printf(TEXT("thumb_0%d_%s"), J, S));
			Turn(CS, I, FQuat(F, (bRight ? -1.f : 1.f) * FMath::DegreesToRadians(GripDegrees * 0.4f * G)));
		}
	}
}

bool FScene7SwapAnimProxy::Evaluate(FPoseContext& Output)
{
	if (Base)
	{
		FAnimationPoseData Data(Output);
		const float Len = Base->GetPlayLength();
		const double T = Len > 0.f ? FMath::Fmod(Time, Len) : 0.0;
		Base->GetAnimationPose(Data, FAnimExtractContext(T, false, {}, true));
	}
	else
	{
		Output.ResetToRefPose();
	}

	const FBoneContainer& BC = Output.Pose.GetBoneContainer();
	FCSPose<FCompactPose> CS;
	CS.InitPose(Output.Pose);

	// Lean into the work.
	if (!FMath::IsNearlyZero(Pose.Lean))
	{
		const FQuat Q(Pose.LeanAxis.GetSafeNormal(), FMath::DegreesToRadians(Pose.Lean / 3.f));
		for (const TCHAR* Name : { TEXT("spine_01"), TEXT("spine_03"), TEXT("spine_05") })
		{
			FCompactPoseBoneIndex I = Bone(BC, Name);
			if (!I.IsValid() && FCString::Strcmp(Name, TEXT("spine_05")) == 0) { I = Bone(BC, TEXT("spine_02")); }
			Turn(CS, I, Q);
		}
	}

	// The head looks (neck takes 40%, the head the rest), at most 70 degrees off the chest's facing.
	if (Pose.LookAlpha > KINDA_SMALL_NUMBER)
	{
		const FCompactPoseBoneIndex Neck = Bone(BC, TEXT("neck_01"));
		const FCompactPoseBoneIndex Head = Bone(BC, TEXT("head"));
		const FCompactPoseBoneIndex Chest = Bone(BC, TEXT("spine_05")).IsValid() ? Bone(BC, TEXT("spine_05")) : Bone(BC, TEXT("spine_03"));
		if (Head.IsValid() && Chest.IsValid())
		{
			// The chest's facing: the component's forward (+Y for the mannequin) carried by the lean.
			const FQuat LeanQ(Pose.LeanAxis.GetSafeNormal(), FMath::DegreesToRadians(Pose.Lean));
			const FVector Facing = LeanQ.RotateVector(FVector(0.f, 1.f, 0.f));
			const FVector HeadPos = CS.GetComponentSpaceTransform(Head).GetLocation();
			FVector D = (Pose.LookAt - HeadPos).GetSafeNormal();
			if (!D.IsNearlyZero())
			{
				FQuat Q = FQuat::FindBetweenNormals(Facing, D);
				FVector Axis; float Angle;
				Q.ToAxisAndAngle(Axis, Angle);
				Angle = FMath::Min(Angle, FMath::DegreesToRadians(70.f)) * FMath::Clamp(Pose.LookAlpha, 0.f, 1.f);
				Turn(CS, Neck, FQuat(Axis, Angle * 0.4f));
				Turn(CS, Head, FQuat(Axis, Angle * 0.6f));
			}
		}
	}

	SolveArm(CS, BC, TEXT("l"), false, Pose.Left, PalmOffset, GripDegrees);
	SolveArm(CS, BC, TEXT("r"), true, Pose.Right, PalmOffset, GripDegrees);

	FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(CS), Output.Pose);
	return true;
}
