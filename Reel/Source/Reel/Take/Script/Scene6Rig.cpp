// See Scene6Rig.h. Owner: the s6-backstage session.
#include "Script/Scene6Rig.h"

#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

namespace
{
	const FName NPelvis(TEXT("pelvis"));
	const FName NSpine[5] = { TEXT("spine_01"), TEXT("spine_02"), TEXT("spine_03"), TEXT("spine_04"), TEXT("spine_05") };
	const FName NNeck1(TEXT("neck_01")), NNeck2(TEXT("neck_02")), NHead(TEXT("head"));
	const FName NClav[2] = { TEXT("clavicle_l"), TEXT("clavicle_r") };
	const FName NUpper[2] = { TEXT("upperarm_l"), TEXT("upperarm_r") };
	const FName NLower[2] = { TEXT("lowerarm_l"), TEXT("lowerarm_r") };
	const FName NHand[2] = { TEXT("hand_l"), TEXT("hand_r") };
	const FName NThigh[2] = { TEXT("thigh_l"), TEXT("thigh_r") };
	const FName NCalf[2] = { TEXT("calf_l"), TEXT("calf_r") };
	const FName NFoot[2] = { TEXT("foot_l"), TEXT("foot_r") };

	// The mannequin's body frame in its component space: front +Y, up +Z.
	const FQuat& RefBody()
	{
		static const FQuat Q = FRotationMatrix::MakeFromXZ(FVector(0.f, 1.f, 0.f), FVector(0.f, 0.f, 1.f)).ToQuat();
		return Q;
	}

	void AllBones(TArray<FName>& Out)
	{
		Out.Add(NPelvis);
		for (const FName& N : NSpine) { Out.Add(N); }
		Out.Add(NNeck1); Out.Add(NNeck2); Out.Add(NHead);
		for (int32 S = 0; S < 2; ++S)
		{
			Out.Add(NClav[S]); Out.Add(NUpper[S]); Out.Add(NLower[S]); Out.Add(NHand[S]);
			Out.Add(NThigh[S]); Out.Add(NCalf[S]); Out.Add(NFoot[S]);
		}
	}
}

// ---- frames and poses ---------------------------------------------------------------------------------------------

FQuat FS6Frame::Quat() const
{
	const FVector F = Fwd.IsNearlyZero() ? FVector::ForwardVector : Fwd;
	const FVector U = Up.IsNearlyZero() ? FVector::UpVector : Up;
	return FRotationMatrix::MakeFromXZ(F, U).ToQuat();
}

FS6Frame FS6Frame::Lerp(const FS6Frame& A, const FS6Frame& B, float T)
{
	const FQuat Q = FQuat::Slerp(A.Quat(), B.Quat(), FMath::Clamp(T, 0.f, 1.f));
	return FS6Frame(Q.GetAxisX(), Q.GetAxisZ());
}

FS6Frame FS6Frame::Lean(const FVector& InFwd, float LeanDeg, float RollDeg)
{
	FVector F = InFwd.GetSafeNormal2D();
	if (F.IsNearlyZero()) { F = FVector::ForwardVector; }
	const float R = FMath::DegreesToRadians(LeanDeg);
	FVector Up = FVector::UpVector * FMath::Cos(R) + F * FMath::Sin(R);
	FVector Fw = F * FMath::Cos(R) - FVector::UpVector * FMath::Sin(R);
	if (!FMath::IsNearlyZero(RollDeg))
	{
		Up = FQuat(Fw, FMath::DegreesToRadians(RollDeg)).RotateVector(Up);
	}
	return FS6Frame(Fw, Up);
}

FS6Pose FS6Pose::Lerp(const FS6Pose& A, const FS6Pose& B, float T)
{
	T = FMath::Clamp(T, 0.f, 1.f);
	FS6Pose P;
	P.Pelvis = FMath::Lerp(A.Pelvis, B.Pelvis, T);
	P.Hips = FS6Frame::Lerp(A.Hips, B.Hips, T);
	P.Chest = FS6Frame::Lerp(A.Chest, B.Chest, T);
	P.Head = FS6Frame::Lerp(A.Head, B.Head, T);
	for (int32 S = 0; S < 2; ++S)
	{
		P.Hand[S] = FMath::Lerp(A.Hand[S], B.Hand[S], T);
		P.Elbow[S] = FMath::Lerp(A.Elbow[S], B.Elbow[S], T);
		P.Ankle[S] = FMath::Lerp(A.Ankle[S], B.Ankle[S], T);
		P.Knee[S] = FMath::Lerp(A.Knee[S], B.Knee[S], T);
		P.Foot[S] = FS6Frame::Lerp(A.Foot[S], B.Foot[S], T);
	}
	return P;
}

// ---- the rig ------------------------------------------------------------------------------------------------------

void FS6Rig::Init(UPoseableMeshComponent* InPose, USkeletalMeshComponent* InFollower)
{
	if (bAttached) { Detach(); }
	Pose = InPose;
	Follower = InFollower;
	RefCS.Reset();
	if (!InPose || !InFollower || !InFollower->GetSkeletalMeshAsset())
	{
		return;
	}
	if (InPose->GetSkinnedAsset() != InFollower->GetSkeletalMeshAsset())
	{
		InPose->SetSkinnedAssetAndUpdate(InFollower->GetSkeletalMeshAsset());
	}
	TArray<FName> Bones;
	AllBones(Bones);
	for (const FName& B : Bones)
	{
		InPose->ResetBoneTransformByName(B);
	}
	for (const FName& B : Bones)
	{
		RefCS.Add(B, InPose->GetBoneTransformByName(B, EBoneSpaces::ComponentSpace).GetRotation());
	}
}

bool FS6Rig::IsReady() const
{
	return Pose.IsValid() && Follower.IsValid() && RefCS.Num() > 0;
}

void FS6Rig::SyncTransform()
{
	if (Pose.IsValid() && Follower.IsValid())
	{
		Pose->SetWorldTransform(Follower->GetComponentTransform());
	}
}

void FS6Rig::ResetAll()
{
	TArray<FName> Bones;
	AllBones(Bones);
	for (const FName& B : Bones)
	{
		Pose->ResetBoneTransformByName(B);
	}
}

void FS6Rig::Measure()
{
	const FVector O = Follower->GetComponentLocation();
	auto H = [&](FName B) { return static_cast<float>(Bone(B).Z - O.Z); };
	PelvisH = H(NPelvis);
	NeckH = H(NNeck2);
	HeadH = H(NHead);
	ShoulderH = 0.5f * (H(NUpper[0]) + H(NUpper[1]));
	ShoulderHalf = 0.5f * static_cast<float>(FVector::Dist(Bone(NUpper[0]), Bone(NUpper[1])));
	HipHalf = 0.5f * static_cast<float>(FVector::Dist(Bone(NThigh[0]), Bone(NThigh[1])));
	AnkleH = 0.5f * (H(NFoot[0]) + H(NFoot[1]));
	Arm1 = static_cast<float>(FVector::Dist(Bone(NUpper[1]), Bone(NLower[1])));
	Arm2 = static_cast<float>(FVector::Dist(Bone(NLower[1]), Bone(NHand[1])));
	Leg1 = static_cast<float>(FVector::Dist(Bone(NThigh[1]), Bone(NCalf[1])));
	Leg2 = static_cast<float>(FVector::Dist(Bone(NCalf[1]), Bone(NFoot[1])));
}

void FS6Rig::Attach()
{
	if (!IsReady() || bAttached)
	{
		return;
	}
	SyncTransform();
	ResetAll();
	Pose->RefreshBoneTransforms();
	Measure();
	Follower->SetLeaderPoseComponent(Pose.Get());
	bAttached = true;
}

void FS6Rig::Detach()
{
	if (Follower.IsValid() && bAttached)
	{
		Follower->SetLeaderPoseComponent(nullptr);
	}
	bAttached = false;
}

FTransform FS6Rig::Get(FName B) const
{
	return Pose.IsValid() ? Pose->GetBoneTransformByName(B, EBoneSpaces::WorldSpace) : FTransform::Identity;
}

FVector FS6Rig::Bone(FName Name) const
{
	return Get(Name).GetLocation();
}

FVector FS6Rig::FeetNow() const
{
	return Follower.IsValid() ? Follower->GetComponentLocation() : FVector::ZeroVector;
}

FVector FS6Rig::FwdNow() const
{
	return Follower.IsValid() ? Follower->GetComponentTransform().TransformVectorNoScale(FVector(0.f, 1.f, 0.f)).GetSafeNormal2D()
		: FVector::ForwardVector;
}

void FS6Rig::Orient(FName B, const FS6Frame& F)
{
	const FQuat* Ref = RefCS.Find(B);
	if (!Ref)
	{
		return;
	}
	FTransform T = Get(B);
	T.SetRotation((F.Quat() * RefBody().Inverse() * (*Ref)).GetNormalized());
	Pose->SetBoneTransformByName(B, T, EBoneSpaces::WorldSpace);
}

void FS6Rig::Aim(FName B, FName Child, const FVector& Target)
{
	FTransform T = Get(B);
	const FVector From = (Get(Child).GetLocation() - T.GetLocation()).GetSafeNormal();
	const FVector To = (Target - T.GetLocation()).GetSafeNormal();
	if (From.IsNearlyZero() || To.IsNearlyZero())
	{
		return;
	}
	T.SetRotation((FQuat::FindBetweenNormals(From, To) * T.GetRotation()).GetNormalized());
	Pose->SetBoneTransformByName(B, T, EBoneSpaces::WorldSpace);
}

void FS6Rig::Chain(FName A, FName B, FName C, const FVector& Target, const FVector& PolePoint, float L1, float L2)
{
	const FVector S = Get(A).GetLocation();
	FVector ToT = Target - S;
	const float Reach = FMath::Max(L1 + L2 - 0.5f, 1.f);
	float D = static_cast<float>(ToT.Size());
	const FVector Dir = D > KINDA_SMALL_NUMBER ? ToT / D : FVector::DownVector;
	D = FMath::Clamp(D, FMath::Abs(L1 - L2) + 1.f, Reach);
	const FVector End = S + Dir * D;
	FVector Pole = PolePoint - S;
	Pole -= Dir * FVector::DotProduct(Pole, Dir);
	if (!Pole.Normalize())
	{
		Pole = FVector::CrossProduct(Dir, FVector::RightVector).GetSafeNormal();
	}
	const float Along = (L1 * L1 - L2 * L2 + D * D) / (2.f * D);
	const float Out = FMath::Sqrt(FMath::Max(0.f, L1 * L1 - Along * Along));
	const FVector Mid = S + Dir * Along + Pole * Out;
	Aim(A, B, Mid);
	Aim(B, C, End);
}

void FS6Rig::Apply(const FS6Pose& P)
{
	if (!IsReady())
	{
		return;
	}
	SyncTransform();
	{
		const FQuat* Ref = RefCS.Find(NPelvis);
		FTransform T = Get(NPelvis);
		T.SetLocation(P.Pelvis);
		if (Ref) { T.SetRotation((P.Hips.Quat() * RefBody().Inverse() * (*Ref)).GetNormalized()); }
		Pose->SetBoneTransformByName(NPelvis, T, EBoneSpaces::WorldSpace);
	}
	for (int32 I = 0; I < 5; ++I)
	{
		Orient(NSpine[I], FS6Frame::Lerp(P.Hips, P.Chest, (I + 1) / 5.f));
	}
	Orient(NNeck1, FS6Frame::Lerp(P.Chest, P.Head, 0.35f));
	Orient(NNeck2, FS6Frame::Lerp(P.Chest, P.Head, 0.7f));
	Orient(NHead, P.Head);
	for (int32 S = 0; S < 2; ++S)
	{
		Orient(NClav[S], P.Chest);
		Chain(NUpper[S], NLower[S], NHand[S], P.Hand[S], P.Elbow[S], Arm1, Arm2);
		Chain(NThigh[S], NCalf[S], NFoot[S], P.Ankle[S], P.Knee[S], Leg1, Leg2);
		Orient(NFoot[S], P.Foot[S]);
	}
	Pose->RefreshBoneTransforms();
	if (Follower.IsValid())
	{
		Follower->SetComponentTickEnabled(true);
	}
}

FS6Pose FS6Rig::Stand(const FVector& Feet, const FVector& InFwd, float Drop) const
{
	FVector F = InFwd.GetSafeNormal2D();
	if (F.IsNearlyZero()) { F = FVector::ForwardVector; }
	const FVector Z = FVector::UpVector;
	const FVector R = FVector::CrossProduct(Z, F);
	FS6Pose P;
	P.Pelvis = Feet + Z * (PelvisH - Drop);
	P.Hips = P.Chest = P.Head = FS6Frame::Lean(F, 0.f);
	for (int32 S = 0; S < 2; ++S)
	{
		const float Side = S == 0 ? -1.f : 1.f;
		const FVector Shoulder = Feet + Z * (ShoulderH - Drop) + R * (Side * ShoulderHalf);
		P.Hand[S] = Shoulder + R * (Side * 6.f) + F * 4.f - Z * (Arm1 + Arm2 - 4.f);
		P.Elbow[S] = Shoulder - F * 40.f + R * (Side * 10.f) - Z * 20.f;
		P.Ankle[S] = Feet + R * (Side * (HipHalf + 2.f)) + Z * AnkleH;
		P.Knee[S] = P.Ankle[S] + F * 100.f + Z * 40.f;
		P.Foot[S] = FS6Frame::Lean(F, 0.f);
	}
	return P;
}
