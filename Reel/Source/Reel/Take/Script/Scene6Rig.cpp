// See Scene6Rig.h. Owner: the s6-backstage session.
#include "Script/Scene6Rig.h"

#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "AnimationRuntime.h"

namespace
{
	const FName S6NPelvis(TEXT("pelvis"));
	const FName S6NSpine[5] = { TEXT("spine_01"), TEXT("spine_02"), TEXT("spine_03"), TEXT("spine_04"), TEXT("spine_05") };
	const FName S6NNeck1(TEXT("neck_01")), S6NNeck2(TEXT("neck_02")), S6NHead(TEXT("head"));
	const FName S6NClav[2] = { TEXT("clavicle_l"), TEXT("clavicle_r") };
	const FName S6NUpper[2] = { TEXT("upperarm_l"), TEXT("upperarm_r") };
	const FName S6NLower[2] = { TEXT("lowerarm_l"), TEXT("lowerarm_r") };
	const FName S6NHand[2] = { TEXT("hand_l"), TEXT("hand_r") };
	const FName S6NThigh[2] = { TEXT("thigh_l"), TEXT("thigh_r") };
	const FName S6NCalf[2] = { TEXT("calf_l"), TEXT("calf_r") };
	const FName S6NFoot[2] = { TEXT("foot_l"), TEXT("foot_r") };

	// The mannequin's body frame in its component space: front +Y, up +Z.
	const FQuat& S6RefBody()
	{
		static const FQuat Q = FRotationMatrix::MakeFromXZ(FVector(0.f, 1.f, 0.f), FVector(0.f, 0.f, 1.f)).ToQuat();
		return Q;
	}

	// The visible layers riding on a rig's follower (costume shells following it, a cast member's copied hero body):
	// a follower of a follower does not get the pose (Unreal does not chain leader poses), so while the rig is on they
	// follow the poseable directly, and each goes back to the leader it had when the rig comes off. Kept here, per
	// rig, so FS6Rig's layout (a member of a UCLASS) stays as it was.
	TMap<const void*, TMap<TWeakObjectPtr<USkeletalMeshComponent>, TWeakObjectPtr<USkinnedMeshComponent>>>& S6Riders()
	{
		static TMap<const void*, TMap<TWeakObjectPtr<USkeletalMeshComponent>, TWeakObjectPtr<USkinnedMeshComponent>>> M;
		return M;
	}

	/** Puts every body layer on Follower's actor (and the actors riding on it) onto Pose, remembering its leader. */
	void S6TakeRiders(const void* Key, UPoseableMeshComponent* Pose, USkeletalMeshComponent* Follower)
	{
		AActor* Owner = Follower ? Follower->GetOwner() : nullptr;
		if (!Owner || !Pose)
		{
			return;
		}
		TArray<AActor*> Actors;
		Actors.Add(Owner);
		Owner->GetAttachedActors(Actors, false, true);
		auto& Mine = S6Riders().FindOrAdd(Key);
		for (AActor* A : Actors)
		{
			if (!A) { continue; }
			TArray<USkeletalMeshComponent*> Comps;
			A->GetComponents<USkeletalMeshComponent>(Comps);
			for (USkeletalMeshComponent* C : Comps)
			{
				if (!C || C == Follower || !C->GetSkinnedAsset()) { continue; }
				USkinnedMeshComponent* Leader = C->LeaderPoseComponent.Get();
				const bool bBody = C->GetBoneIndex(TEXT("pelvis")) != INDEX_NONE || C->GetBoneIndex(TEXT("head")) != INDEX_NONE;
				if (Leader == Pose || (!Leader && !bBody)) { continue; }
				Mine.Add(C, Leader);	// a leader changed mid-stunt (a crew act starting or ending) is the one to restore
				C->SetLeaderPoseComponent(Pose);
			}
		}
	}

	// The bodies that take their bounds from the poseable while the rig is on. A follower's own bounds stay where its
	// actor stands while the posed body is dragged metres away, so its shadow was culled and cached at the old spot
	// (boot shadows left behind on the drag to the trunk, Marwan's QA, Nov 16).
	TSet<TWeakObjectPtr<USkinnedMeshComponent>>& S6BoundsTaken()
	{
		static TSet<TWeakObjectPtr<USkinnedMeshComponent>> S;
		return S;
	}

	void S6TakeBounds(USkinnedMeshComponent* C)
	{
		if (C && !C->bUseBoundsFromLeaderPoseComponent)
		{
			C->bUseBoundsFromLeaderPoseComponent = true;
			S6BoundsTaken().Add(C);
		}
	}

	void S6GiveBounds(USkinnedMeshComponent* C)
	{
		if (C && S6BoundsTaken().Remove(C) > 0)
		{
			C->bUseBoundsFromLeaderPoseComponent = false;
			C->UpdateBounds();
			C->MarkRenderTransformDirty();
		}
	}

	void S6DropRiders(const void* Key)
	{
		if (auto* Mine = S6Riders().Find(Key))
		{
			for (auto& Pair : *Mine)
			{
				if (USkeletalMeshComponent* C = Pair.Key.Get())
				{
					C->SetLeaderPoseComponent(Pair.Value.Get());
					S6GiveBounds(C);
				}
			}
		}
		S6Riders().Remove(Key);
	}

	/** The finger bones (thumb, index, middle, ring, pinky and their metacarpals). */
	bool S6IsFinger(const FName& N)
	{
		const FString S = N.ToString();
		return S.StartsWith(TEXT("thumb_")) || S.StartsWith(TEXT("index_")) || S.StartsWith(TEXT("middle_"))
			|| S.StartsWith(TEXT("ring_")) || S.StartsWith(TEXT("pinky_"));
	}

	void S6AllBones(TArray<FName>& Out)
	{
		Out.Add(S6NPelvis);
		for (const FName& N : S6NSpine) { Out.Add(N); }
		Out.Add(S6NNeck1); Out.Add(S6NNeck2); Out.Add(S6NHead);
		for (int32 S = 0; S < 2; ++S)
		{
			Out.Add(S6NClav[S]); Out.Add(S6NUpper[S]); Out.Add(S6NLower[S]); Out.Add(S6NHand[S]);
			Out.Add(S6NThigh[S]); Out.Add(S6NCalf[S]); Out.Add(S6NFoot[S]);
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
		// An unaimed hand has no Finger/Palm: the lerp keeps the aimed one's way and HandAim fades it in or out.
		P.Finger[S] = FMath::Lerp(A.Finger[S], B.Finger[S], T);
		P.Palm[S] = FMath::Lerp(A.Palm[S], B.Palm[S], T);
		P.HandAim[S] = FMath::Lerp(A.HandAim[S], B.HandAim[S], T);
		P.Grip[S] = FMath::Lerp(A.Grip[S], B.Grip[S], T);
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
	S6AllBones(Bones);
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
	S6AllBones(Bones);
	for (const FName& B : Bones)
	{
		Pose->ResetBoneTransformByName(B);
	}
}

void FS6Rig::Measure()
{
	const FVector O = Follower->GetComponentLocation();
	auto H = [&](FName B) { return static_cast<float>(Bone(B).Z - O.Z); };
	PelvisH = H(S6NPelvis);
	NeckH = H(S6NNeck2);
	HeadH = H(S6NHead);
	ShoulderH = 0.5f * (H(S6NUpper[0]) + H(S6NUpper[1]));
	ShoulderHalf = 0.5f * static_cast<float>(FVector::Dist(Bone(S6NUpper[0]), Bone(S6NUpper[1])));
	HipHalf = 0.5f * static_cast<float>(FVector::Dist(Bone(S6NThigh[0]), Bone(S6NThigh[1])));
	AnkleH = 0.5f * (H(S6NFoot[0]) + H(S6NFoot[1]));
	Arm1 = static_cast<float>(FVector::Dist(Bone(S6NUpper[1]), Bone(S6NLower[1])));
	Arm2 = static_cast<float>(FVector::Dist(Bone(S6NLower[1]), Bone(S6NHand[1])));
	Leg1 = static_cast<float>(FVector::Dist(Bone(S6NThigh[1]), Bone(S6NCalf[1])));
	Leg2 = static_cast<float>(FVector::Dist(Bone(S6NCalf[1]), Bone(S6NFoot[1])));
}

void FS6Rig::Attach()
{
	if (!IsReady() || bAttached)
	{
		return;
	}
	SyncTransform();
	// The fingers: the poseable's are the reference pose (flat, splayed), which read as a mess of fingers in the
	// takedown and the drag (Marwan, Nov 16). Take the relaxed hands the body had from its animation as the rig
	// comes on; every frame they are laid back from that and curled by the pose's Grip (PoseFingers, Nov 17).
	{
		const TArray<FTransform>& Anim = Follower->GetBoneSpaceTransforms();
		const FReferenceSkeleton& RefSkel = Pose->GetSkinnedAsset()->GetRefSkeleton();
		if (Anim.Num() == Pose->BoneSpaceTransforms.Num() && Anim.Num() == RefSkel.GetNum())
		{
			for (int32 I = 0; I < Anim.Num(); ++I)
			{
				if (S6IsFinger(RefSkel.GetBoneName(I)))
				{
					Pose->BoneSpaceTransforms[I] = Anim[I];
				}
			}
		}
		SetupHands();
	}
	ResetAll();
	Pose->RefreshBoneTransforms();
	Measure();
	Follower->SetLeaderPoseComponent(Pose.Get());
	S6TakeBounds(Follower.Get());
	S6TakeRiders(this, Pose.Get(), Follower.Get());
	bAttached = true;
}

void FS6Rig::Detach()
{
	if (Follower.IsValid() && bAttached)
	{
		Follower->SetLeaderPoseComponent(nullptr);
		S6GiveBounds(Follower.Get());
	}
	if (bAttached)
	{
		S6DropRiders(this);
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
	T.SetRotation((F.Quat() * S6RefBody().Inverse() * (*Ref)).GetNormalized());
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
	// Twist the upper bone about its own axis so the joint's own bend (the reference pose's slight elbow or knee bend,
	// reset every frame) lies in the solved plane: the lower bone then swings as a hinge instead of bending sideways
	// through the mesh. A near-straight reference bend has no direction to trust: left as the plain aim.
	{
		const FTransform TA = Get(A);
		const FVector U = (Mid - TA.GetLocation()).GetSafeNormal();
		const FVector CurL = Get(C).GetLocation() - Get(B).GetLocation();
		FVector Have = CurL - U * FVector::DotProduct(CurL, U);
		FVector Want = -(Pole - U * FVector::DotProduct(Pole, U));
		if (!U.IsNearlyZero() && Have.Size() > 0.12f * CurL.Size() && Want.Normalize() && Have.Normalize())
		{
			FTransform T = TA;
			T.SetRotation((FQuat::FindBetweenNormals(Have, Want) * T.GetRotation()).GetNormalized());
			Pose->SetBoneTransformByName(A, T, EBoneSpaces::WorldSpace);
		}
	}
	Aim(B, C, End);
}

void FS6Rig::Apply(const FS6Pose& P)
{
	if (!IsReady())
	{
		return;
	}
	SyncTransform();
	// Every frame from the reference pose: the limbs' aims start from the same place each time, so their twist cannot
	// creep from frame to frame (the arms and legs used to wind up slowly over a long stunt).
	ResetAll();
	{
		const FQuat* Ref = RefCS.Find(S6NPelvis);
		FTransform T = Get(S6NPelvis);
		T.SetLocation(P.Pelvis);
		if (Ref) { T.SetRotation((P.Hips.Quat() * S6RefBody().Inverse() * (*Ref)).GetNormalized()); }
		Pose->SetBoneTransformByName(S6NPelvis, T, EBoneSpaces::WorldSpace);
	}
	for (int32 I = 0; I < 5; ++I)
	{
		Orient(S6NSpine[I], FS6Frame::Lerp(P.Hips, P.Chest, (I + 1) / 5.f));
	}
	Orient(S6NNeck1, FS6Frame::Lerp(P.Chest, P.Head, 0.35f));
	Orient(S6NNeck2, FS6Frame::Lerp(P.Chest, P.Head, 0.7f));
	Orient(S6NHead, P.Head);
	for (int32 S = 0; S < 2; ++S)
	{
		Orient(S6NClav[S], P.Chest);
		Chain(S6NUpper[S], S6NLower[S], S6NHand[S], P.Hand[S], P.Elbow[S], Arm1, Arm2);
		PoseHand(S, P);
		Chain(S6NThigh[S], S6NCalf[S], S6NFoot[S], P.Ankle[S], P.Knee[S], Leg1, Leg2);
		Orient(S6NFoot[S], P.Foot[S]);
	}
	PoseFingers(P);
	Pose->RefreshBoneTransforms();
	if (Follower.IsValid())
	{
		Follower->SetComponentTickEnabled(true);
		if (bAttached)
		{
			// Again every frame: a crew act starting or ending re-points the costume to another body.
			S6TakeRiders(this, Pose.Get(), Follower.Get());
			// The props on the bodies' sockets (his gun, glasses, mask, shoes): a follower does not move its attachments
			// when only its leader is posed, so they hung where they were (Marwan's QA, Nov 16). Moved here, from the
			// sockets as the leader now has them.
			Follower->UpdateChildTransforms(EUpdateTransformFlags::None, ETeleportType::TeleportPhysics);
			// Bounds from the posed body, sent to the renderer every frame: the shadow moves with him.
			Pose->UpdateBounds();
			Follower->UpdateBounds();
			Follower->MarkRenderTransformDirty();
			if (auto* Mine = S6Riders().Find(this))
			{
				for (auto& Pair : *Mine)
				{
					if (USkeletalMeshComponent* C = Pair.Key.Get())
					{
						S6TakeBounds(C);
						C->UpdateBounds();
						C->MarkRenderTransformDirty();
						C->UpdateChildTransforms(EUpdateTransformFlags::None, ETeleportType::TeleportPhysics);
					}
				}
			}
		}
	}
}

FS6Pose FS6Rig::Stand(const FVector& Feet, const FVector& InFwd, float Drop) const
{
	FVector F = InFwd.GetSafeNormal2D();
	if (F.IsNearlyZero()) { F = FVector::ForwardVector; }
	const FVector RigZ = FVector::UpVector;
	const FVector R = FVector::CrossProduct(RigZ, F);
	FS6Pose P;
	P.Pelvis = Feet + RigZ * (PelvisH - Drop);
	P.Hips = P.Chest = P.Head = FS6Frame::Lean(F, 0.f);
	for (int32 S = 0; S < 2; ++S)
	{
		const float Side = S == 0 ? -1.f : 1.f;
		const FVector Shoulder = Feet + RigZ * (ShoulderH - Drop) + R * (Side * ShoulderHalf);
		P.Hand[S] = Shoulder + R * (Side * 6.f) + F * 4.f - RigZ * (Arm1 + Arm2 - 4.f);
		P.Elbow[S] = Shoulder - F * 40.f + R * (Side * 10.f) - RigZ * 20.f;
		P.Ankle[S] = Feet + R * (Side * (HipHalf + 2.f)) + RigZ * AnkleH;
		P.Knee[S] = P.Ankle[S] + F * 100.f + RigZ * 40.f;
		P.Foot[S] = FS6Frame::Lean(F, 0.f);
	}
	return P;
}

// ---- the hands (Marwan's QA, Nov 17) --------------------------------------------------------------------------------

void FS6Rig::SetupHands()
{
	Fingers.Reset();
	bHandAxes = false;
	const FReferenceSkeleton& RefSkel = Pose->GetSkinnedAsset()->GetRefSkeleton();
	auto RefCSOf = [&](const TCHAR* Name, int32 S, FTransform& Out) -> int32
	{
		const int32 I = RefSkel.FindBoneIndex(FName(*FString::Printf(TEXT("%s_%s"), Name, S == 0 ? TEXT("l") : TEXT("r"))));
		if (I != INDEX_NONE) { Out = FAnimationRuntime::GetComponentSpaceTransformRefPose(RefSkel, I); }
		return I;
	};
	// The hand's axes, from the reference pose: the fingers toward the middle knuckle, the palm the side the thumb's
	// root sits on (palms down in the mannequin's reference pose: logged as a check).
	bool bAll = true;
	for (int32 S = 0; S < 2; ++S)
	{
		FTransform H, Mid, Ind, Pin, Thu;
		if (RefCSOf(TEXT("hand"), S, H) == INDEX_NONE || RefCSOf(TEXT("middle_01"), S, Mid) == INDEX_NONE
			|| RefCSOf(TEXT("index_01"), S, Ind) == INDEX_NONE || RefCSOf(TEXT("pinky_01"), S, Pin) == INDEX_NONE
			|| RefCSOf(TEXT("thumb_02"), S, Thu) == INDEX_NONE)
		{
			bAll = false;
			break;
		}
		const FVector Fing = (Mid.GetLocation() - H.GetLocation()).GetSafeNormal();
		FVector PalmN = FVector::CrossProduct(Fing, Ind.GetLocation() - Pin.GetLocation()).GetSafeNormal();
		if (FVector::DotProduct(PalmN, Thu.GetLocation() - H.GetLocation()) < 0.f) { PalmN = -PalmN; }
		UE_LOG(LogTemp, Log, TEXT("S6 rig hand %d: palm %s (down-ness %.2f)"), S, *PalmN.ToString(), -PalmN.Z);
		HandFingerLocal[S] = H.GetRotation().Inverse().RotateVector(Fing);
		HandPalmLocal[S] = H.GetRotation().Inverse().RotateVector(PalmN);
	}
	bHandAxes = bAll;
	if (!bAll)
	{
		return;
	}
	// The finger joints: each curls about the line across its finger and the palm (from the body's own relaxed hand
	// when the follower has one, else the reference pose).
	const TArray<FTransform>& AnimCS = Follower->GetComponentSpaceTransforms();
	const bool bAnim = AnimCS.Num() == RefSkel.GetNum();
	auto CSOf = [&](int32 I)
	{
		return bAnim ? AnimCS[I] : FAnimationRuntime::GetComponentSpaceTransformRefPose(RefSkel, I);
	};
	static const TCHAR* const S6Digits[5] = { TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky"), TEXT("thumb") };
	for (int32 S = 0; S < 2; ++S)
	{
		const TCHAR* Sx = S == 0 ? TEXT("l") : TEXT("r");
		const int32 HandI = RefSkel.FindBoneIndex(S6NHand[S]);
		if (HandI == INDEX_NONE)
		{
			continue;
		}
		const FVector PalmCS = CSOf(HandI).GetRotation().RotateVector(HandPalmLocal[S]);
		for (int32 D = 0; D < 5; ++D)
		{
			int32 J[3];
			for (int32 K = 0; K < 3; ++K)
			{
				J[K] = RefSkel.FindBoneIndex(FName(*FString::Printf(TEXT("%s_0%d_%s"), S6Digits[D], K + 1, Sx)));
			}
			if (J[0] == INDEX_NONE || J[2] == INDEX_NONE)
			{
				continue;
			}
			const FVector Along = (CSOf(J[2]).GetLocation() - CSOf(J[0]).GetLocation()).GetSafeNormal();
			// Turning Along about (Along x Palm) by a positive angle moves it toward the palm: the curl.
			const FVector AxisCS = FVector::CrossProduct(Along, PalmCS).GetSafeNormal();
			if (AxisCS.IsNearlyZero())
			{
				continue;
			}
			for (int32 K = 0; K < 3; ++K)
			{
				if (J[K] == INDEX_NONE || !Pose->BoneSpaceTransforms.IsValidIndex(J[K]))
				{
					continue;
				}
				FFingerJoint F;
				F.Index = J[K];
				F.Side = S;
				F.Relaxed = Pose->BoneSpaceTransforms[J[K]];
				F.Axis = CSOf(J[K]).GetRotation().Inverse().RotateVector(AxisCS).GetSafeNormal();
				F.CurlDeg = D == 4 ? ThumbCurl * (K == 0 ? 0.3f : 1.f) : static_cast<float>(GripCurl[K]);
				Fingers.Add(F);
			}
		}
	}
}

void FS6Rig::PoseHand(int32 S, const FS6Pose& P)
{
	const float Aim = FMath::Clamp(P.HandAim[S], 0.f, 1.f);
	FVector Fw = P.Finger[S], Pw = P.Palm[S];
	if (!bHandAxes || Aim < 0.01f || !Fw.Normalize() || !Pw.Normalize())
	{
		return;
	}
	const FTransform Lo = Get(S6NLower[S]);
	const FTransform H = Get(S6NHand[S]);
	const FVector Arm = (H.GetLocation() - Lo.GetLocation()).GetSafeNormal();
	if (Arm.IsNearlyZero())
	{
		return;
	}
	// The wrist bends at most WristMaxBend: past that the fingers' way is pulled back toward the forearm's.
	const float MaxRad = FMath::DegreesToRadians(WristMaxBend);
	if (FMath::Acos(FMath::Clamp(static_cast<float>(FVector::DotProduct(Arm, Fw)), -1.f, 1.f)) > MaxRad)
	{
		FVector Ax = FVector::CrossProduct(Arm, Fw);
		if (!Ax.Normalize())
		{
			Ax = FVector::CrossProduct(Arm, Pw);	// straight back along the arm: bend toward the palm
			if (!Ax.Normalize()) { return; }
		}
		Fw = FQuat(Ax, MaxRad).RotateVector(Arm);
	}
	Pw -= Fw * FVector::DotProduct(Pw, Fw);
	if (!Pw.Normalize())
	{
		return;
	}
	const FQuat Local = FRotationMatrix::MakeFromXY(HandFingerLocal[S], HandPalmLocal[S]).ToQuat();
	const FQuat Want = (FRotationMatrix::MakeFromXY(Fw, Pw).ToQuat() * Local.Inverse()).GetNormalized();
	const FQuat Have = H.GetRotation();
	const FQuat Final = FQuat::Slerp(Have, Want, Aim).GetNormalized();
	// Part of the turn about the forearm's own line goes to the forearm (its twist bones follow it), so the wrist does
	// not wring. A turn about that line leaves the hand where it is.
	if (ForearmTwistShare > 0.f)
	{
		FQuat Swing, Twist;
		(Final * Have.Inverse()).GetNormalized().ToSwingTwist(Arm, Swing, Twist);
		FTransform LoT = Lo;
		LoT.SetRotation((FQuat::Slerp(FQuat::Identity, Twist, ForearmTwistShare) * Lo.GetRotation()).GetNormalized());
		Pose->SetBoneTransformByName(S6NLower[S], LoT, EBoneSpaces::WorldSpace);
	}
	FTransform HT = Get(S6NHand[S]);
	HT.SetRotation(Final);
	Pose->SetBoneTransformByName(S6NHand[S], HT, EBoneSpaces::WorldSpace);
}

void FS6Rig::PoseFingers(const FS6Pose& P)
{
	for (const FFingerJoint& J : Fingers)
	{
		if (!Pose->BoneSpaceTransforms.IsValidIndex(J.Index))
		{
			continue;
		}
		FTransform T = J.Relaxed;
		const float G = FMath::Clamp(P.Grip[J.Side], -0.6f, 1.2f);
		if (!FMath::IsNearlyZero(G))
		{
			T.SetRotation((T.GetRotation() * FQuat(J.Axis, FMath::DegreesToRadians(J.CurlDeg * G))).GetNormalized());
		}
		Pose->BoneSpaceTransforms[J.Index] = T;
	}
}
