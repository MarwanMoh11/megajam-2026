// chars-locomotion (Nov 15 evening). See TakeLocoAnim.h.
// Mesh space here: +Y forward, -X right (+X left), +Z up (the mannequin faces +Y; the body is turned -90 in the actor).

#include "TakeLocoAnim.h"

#include "Animation/BlendSpace.h"
#include "Animation/AnimNodeBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "HAL/IConsoleManager.h"
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

	// --- the reel under the left arm (Nov 16) ---------------------------------------------------------------------------
	// The can rides on the chest bone, upright along his left side between hip and armpit; the left arm comes down
	// outside it and the fingers hook round the lower front of the rim, the way a film can or a big portfolio is carried.
	// Hand and can are both placed from the same bone, so the hand cannot slide off it while he runs.
	enum ECarry { CHold, CUpper, CLower, CHand, CFinger0, CarryNum = CFinger0 + 12 };
	const FName CarryNames[CarryNum] = {TEXT("spine_03"), TEXT("upperarm_l"), TEXT("lowerarm_l"), TEXT("hand_l"),
		TEXT("index_01_l"), TEXT("index_02_l"), TEXT("index_03_l"), TEXT("middle_01_l"), TEXT("middle_02_l"), TEXT("middle_03_l"),
		TEXT("ring_01_l"), TEXT("ring_02_l"), TEXT("ring_03_l"), TEXT("pinky_01_l"), TEXT("pinky_02_l"), TEXT("pinky_03_l")};
	const FName CarryTag(TEXT("TakeCarriedReel"));
	// A seated carrier (the car): the can sits further forward and a little higher under the arm, clear of the seat
	// back behind him and the cushion under him. The reel's owner tags it before CarryReel.
	const FName CarrySeatedTag(TEXT("TakeCarriedReelSeated"));
	const FVector CarrySeatedOffset(0.f, 13.f, 6.f);
	const float ReelHalfThick = 3.5f;	// the canister is the engine cylinder (1 m across, 1 m tall) at Size x Size x 0.07

	// The arm makes room for the can: the can lies against his ribs, its top tucked toward the armpit and its lower edge
	// leaning out past the hip (Tilt), and the elbow points out to the side (Elbow), so the upper arm comes down OUTSIDE
	// the can's outer face and the forearm comes back in to the hand on the lower rim. The first version pointed the elbow
	// straight back, which ran the upper arm through the disc.
	TAutoConsoleVariable<int32> CVarCarryArm(TEXT("Reel.Carry.Arm"), 1, TEXT("1: the left arm wraps the carried reel. 0: the arm is left to the clip."));
	TAutoConsoleVariable<float> CVarCarrySize(TEXT("Reel.Carry.Size"), 0.38f, TEXT("Carried reel: its width in metres (0.42 on the bench)."));
	TAutoConsoleVariable<float> CVarCarryX(TEXT("Reel.Carry.X"), 2.f, TEXT("Carried reel: its centre, cm to the left of the left shoulder joint."));
	TAutoConsoleVariable<float> CVarCarryY(TEXT("Reel.Carry.Y"), 5.f, TEXT("Carried reel: its centre, cm forward of the left shoulder joint."));
	TAutoConsoleVariable<float> CVarCarryZ(TEXT("Reel.Carry.Z"), -33.f, TEXT("Carried reel: its centre, cm above the left shoulder joint (negative: below)."));
	TAutoConsoleVariable<float> CVarCarryTilt(TEXT("Reel.Carry.Tilt"), 8.f, TEXT("Carried reel: degrees its lower edge leans out from the hip."));
	TAutoConsoleVariable<float> CVarCarryToe(TEXT("Reel.Carry.Toe"), 0.f, TEXT("Carried reel: degrees its front edge turns out from the body."));
	TAutoConsoleVariable<float> CVarCarryGrip(TEXT("Reel.Carry.Grip"), 30.f, TEXT("Carried reel: where on the rim the hand grips, degrees forward of the lowest point."));
	TAutoConsoleVariable<float> CVarCarryInset(TEXT("Reel.Carry.Inset"), 8.5f, TEXT("Carried reel: cm from the rim in to the wrist."));
	TAutoConsoleVariable<float> CVarCarryPalm(TEXT("Reel.Carry.Palm"), 3.f, TEXT("Carried reel: cm from the can's outer face to the wrist."));
	TAutoConsoleVariable<float> CVarCarryElbow(TEXT("Reel.Carry.Elbow"), 1.3f, TEXT("Carried reel: how far the elbow points out to the side (0 straight back)."));
	TAutoConsoleVariable<float> CVarCarryCurl1(TEXT("Reel.Carry.Curl1"), 75.f, TEXT("Carried reel: degrees the knuckles curl over the rim."));
	TAutoConsoleVariable<float> CVarCarryCurl2(TEXT("Reel.Carry.Curl2"), 20.f, TEXT("Carried reel: degrees the middle finger joints curl."));
	TAutoConsoleVariable<float> CVarCarryCurl3(TEXT("Reel.Carry.Curl3"), 10.f, TEXT("Carried reel: degrees the fingertips curl."));

	FTransform RefCS(const FReferenceSkeleton& Ref, int32 Bone)
	{
		FTransform T = Ref.GetRefBonePose()[Bone];
		for (int32 Parent = Ref.GetParentIndex(Bone); Parent != INDEX_NONE; Parent = Ref.GetParentIndex(Parent))
		{
			T = T * Ref.GetRefBonePose()[Parent];
		}
		return T;
	}

	struct FCarrySpot
	{
		FTransform ReelOnBone;				// the can, relative to the hold bone
		FVector Wrist, Finger, Back, Pole;	// the hand, relative to the hold bone
	};

	/** Where the can and the hand go on this body, worked out on its reference pose (mesh space: +X left, +Y forward). */
	bool CarrySpot(const USkeletalMeshComponent* Body, FCarrySpot& Out, bool bSeated = false)
	{
		const USkeletalMesh* Mesh = Body ? Body->GetSkeletalMeshAsset() : nullptr;
		if (!Mesh)
		{
			return false;
		}
		const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
		const int32 Hold = Ref.FindBoneIndex(CarryNames[CHold]);
		const int32 Shoulder = Ref.FindBoneIndex(CarryNames[CUpper]);
		if (Hold == INDEX_NONE || Shoulder == INDEX_NONE)
		{
			return false;
		}
		const FTransform HoldRef = RefCS(Ref, Hold);
		const float Size = FMath::Clamp(CVarCarrySize.GetValueOnGameThread(), 0.1f, 0.6f);
		const float Radius = 50.f * Size;
		const FVector Centre = RefCS(Ref, Shoulder).GetLocation()
			+ FVector(CVarCarryX.GetValueOnGameThread(), CVarCarryY.GetValueOnGameThread(), CVarCarryZ.GetValueOnGameThread())
			+ (bSeated ? CarrySeatedOffset : FVector::ZeroVector);
		// The disc's own frame: Face out of its outer side (away from the body), Fore and Up in its plane.
		const float Tilt = FMath::DegreesToRadians(CVarCarryTilt.GetValueOnGameThread());
		const float Toe = FMath::DegreesToRadians(CVarCarryToe.GetValueOnGameThread());
		const float ST = FMath::Sin(Tilt), CT = FMath::Cos(Tilt), SP = FMath::Sin(Toe), CP = FMath::Cos(Toe);
		const FVector Face(CT * CP, -CT * SP, ST);
		const FVector Fore(SP, CP, 0.f);
		const FVector Up(-ST * CP, ST * SP, CT);
		const float A = FMath::DegreesToRadians(CVarCarryGrip.GetValueOnGameThread());
		const FVector Radial = Fore * FMath::Sin(A) - Up * FMath::Cos(A);	// from the can's centre out to the grip on the rim
		const float BodyScale = FMath::Max(static_cast<float>(Body->GetComponentScale().GetAbsMax()), 0.01f);
		const FTransform ReelCS(FRotationMatrix::MakeFromZX(Face, Fore).ToQuat(), Centre, FVector(Size, Size, 0.07f) / BodyScale);
		Out.ReelOnBone = ReelCS.GetRelativeTransform(HoldRef);
		const FVector Wrist = Centre + Radial * (Radius - CVarCarryInset.GetValueOnGameThread())
			+ Face * (ReelHalfThick + CVarCarryPalm.GetValueOnGameThread());
		Out.Wrist = HoldRef.InverseTransformPosition(Wrist);
		Out.Finger = HoldRef.InverseTransformVectorNoScale(Radial);
		Out.Back = HoldRef.InverseTransformVectorNoScale(Face);
		Out.Pole = HoldRef.InverseTransformVectorNoScale(FVector(CVarCarryElbow.GetValueOnGameThread(), -1.f, 0.15f));
		return true;
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

bool UTakeLocoAnim::CarryReel(USkeletalMeshComponent* Body, USceneComponent* Reel, bool bKeepScale)
{
	TakeLoco::FCarrySpot Spot;
	if (!Body || !Reel || !TakeLoco::CarrySpot(Body, Spot, Reel->ComponentHasTag(TakeLoco::CarrySeatedTag)))
	{
		return false;
	}
	Reel->ComponentTags.AddUnique(TakeLoco::CarryTag);
	// bKeepScale: a reel built at its real size from parts (the car's), not the unit cylinder scaled down.
	const FVector Scale = bKeepScale ? Reel->GetRelativeScale3D() : Spot.ReelOnBone.GetScale3D();
	Reel->AttachToComponent(Body, FAttachmentTransformRules::KeepRelativeTransform, TakeLoco::CarryNames[TakeLoco::CHold]);
	Reel->SetRelativeTransform(FTransform(Spot.ReelOnBone.GetRotation(), Spot.ReelOnBone.GetLocation(), Scale));
	return true;
}

USceneComponent* UTakeLocoAnim::FindCarriedReel(const USceneComponent* Body)
{
	if (Body)
	{
		for (USceneComponent* Child : Body->GetAttachChildren())
		{
			if (Child && Child->GetAttachSocketName() == TakeLoco::CarryNames[TakeLoco::CHold] && Child->ComponentHasTag(TakeLoco::CarryTag)
				&& Child->IsVisible())
			{
				return Child;
			}
		}
	}
	return nullptr;
}

bool UTakeLocoAnim::BeginClip(USkeletalMeshComponent* Mesh)
{
	if (!Mesh || Mesh->GetAnimClass() != UTakeLocoAnim::StaticClass())
	{
		return false;
	}
	if (Mesh->GetAnimationMode() != EAnimationMode::AnimationBlueprint)
	{
		Mesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	}
	UTakeLocoAnim* Anim = Cast<UTakeLocoAnim>(Mesh->GetAnimInstance());
	if (!Anim)
	{
		return false;
	}
	Anim->bClip = true;
	Anim->State.Crouch = 0.f;
	return true;
}

void UTakeLocoAnim::PlayClip(USkeletalMeshComponent* Mesh, UAnimationAsset* Anim, bool bLoop)
{
	if (!Mesh || !Anim)
	{
		return;
	}
	if (BeginClip(Mesh))
	{
		Mesh->SetAnimation(Anim);
		Mesh->Play(bLoop);
	}
	else
	{
		Mesh->PlayAnimation(Anim, bLoop);
	}
}

void UTakeLocoAnim::EndClip(USkeletalMeshComponent* Mesh)
{
	UTakeLocoAnim* Anim = Mesh ? Cast<UTakeLocoAnim>(Mesh->GetAnimInstance()) : nullptr;
	if (Anim && Anim->bClip)
	{
		Anim->bClip = false;
		Anim->StartWalk();
	}
}

void UTakeLocoAnim::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	bClip = false;
	StartWalk();
}

void UTakeLocoAnim::StartWalk()
{
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
	if (bClip)
	{
		// A scene's clip (the fight): the clip is the body, and only the reel arm is laid over it.
		bHasLast = false;
		State.Crouch = 0.f;
		UpdateCarry(Dt);
		return;
	}

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
	// BS_Idle_Walk_Run is Direction (X) by Speed (Y): the speed goes in Y, or the body idles at every speed.
	SetBlendSpacePosition(FVector(0.f, Shown < 6.f ? 0.f : Shown, 0.f));

	// The crouch eases over about a fifth of a second, starting the frame it is asked for.
	const ATakeCharacter* Character = Cast<ATakeCharacter>(Owner);
	const float Want = Character && Character->IsCrouching() ? 1.f : 0.f;
	State.Crouch = FMath::FInterpConstantTo(State.Crouch, Want, Dt, 5.f);
	UpdateCarry(Dt);
}

void UTakeLocoAnim::UpdateCarry(float Dt)
{
	// The reel: while it hangs on this body's chest bone (CarryReel put it there), the left arm goes round it.
	// The can hangs on the body you SEE: a lead's hero body (ACastMember::Body) copies this mesh's pose bone for bone,
	// so the arm is solved here, on the skeleton that is animated, and the hero body shows it.
	USkeletalMeshComponent* Body = GetSkelMeshComponent();
	USceneComponent* Reel = nullptr;
	if (Body && TakeLoco::CVarCarryArm.GetValueOnGameThread() > 0)
	{
		Reel = FindCarriedReel(Body);
		const ATakeCharacter* Character = Cast<ATakeCharacter>(GetOwningActor());
		if (!Reel && Character)
		{
			Reel = FindCarriedReel(Character->GetVisibleBody());
		}
	}
	TakeLoco::FCarrySpot Spot;
	const bool bCarry = Reel && TakeLoco::CarrySpot(Body, Spot, Reel->ComponentHasTag(TakeLoco::CarrySeatedTag));
	if (bCarry)
	{
		State.CarryWrist = Spot.Wrist;
		State.CarryFinger = Spot.Finger;
		State.CarryBack = Spot.Back;
		State.CarryPole = Spot.Pole;
		State.CarryCurl = FVector(TakeLoco::CVarCarryCurl1.GetValueOnGameThread(), TakeLoco::CVarCarryCurl2.GetValueOnGameThread(),
			TakeLoco::CVarCarryCurl3.GetValueOnGameThread());
		if (!Reel->GetRelativeLocation().Equals(Spot.ReelOnBone.GetLocation(), 0.01f)
			|| !Reel->GetRelativeRotation().Quaternion().Equals(Spot.ReelOnBone.GetRotation(), 0.001f))
		{
			Reel->SetRelativeLocationAndRotation(Spot.ReelOnBone.GetLocation(), Spot.ReelOnBone.GetRotation());	// the console values moved it
		}
	}
	State.Carry = FMath::FInterpConstantTo(State.Carry, bCarry ? 1.f : 0.f, Dt, 4.f);
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
	for (int32 b = 0; b < TakeLoco::CarryNum; ++b)
	{
		const int32 MeshIndex = BC.GetPoseBoneIndexForBoneName(TakeLoco::CarryNames[b]);
		Arm[b] = MeshIndex != INDEX_NONE ? BC.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIndex)).GetInt() : INDEX_NONE;
	}
}

bool FTakeLocoProxy::Evaluate(FPoseContext& Output)
{
	FAnimSingleNodeInstanceProxy::Evaluate(Output);
	if (S.Crouch < 0.001f && S.Carry < 0.001f)
	{
		return true;
	}
	FCompactPose& Pose = Output.Pose;
	CacheBones(Pose);
	if (S.Crouch >= 0.001f)
	{
		Crouch(Pose);
	}
	if (S.Carry >= 0.001f)
	{
		Carry(Pose);	// after the crouch: the arm is fitted to where the chest ends up
	}
	return true;
}

void FTakeLocoProxy::Carry(FCompactPose& Pose)
{
	using namespace TakeLoco;
	for (int32 b = 0; b <= CHand; ++b)
	{
		if (Arm[b] == INDEX_NONE)
		{
			return;		// a body without these bones: the arm stays with the clip
		}
	}
	const float W = Ease(S.Carry);
	FQuat Was[CarryNum];
	for (int32 b = CUpper; b < CarryNum; ++b)
	{
		Was[b] = Arm[b] != INDEX_NONE ? Pose[FCompactPoseBoneIndex(Arm[b])].GetRotation() : FQuat::Identity;
	}
	const FTransform Hold = CSAt(Pose, Arm[CHold]);
	const FVector Wrist = Hold.TransformPosition(S.CarryWrist);
	const FVector Finger = Hold.TransformVectorNoScale(S.CarryFinger).GetSafeNormal();
	const FVector Back = Hold.TransformVectorNoScale(S.CarryBack).GetSafeNormal();
	const FVector Pole = Hold.TransformVectorNoScale(S.CarryPole);

	// Shoulder, elbow, wrist: the same two-bone solve as the legs.
	Leg(Pose, Arm[CUpper], Arm[CLower], Arm[CHand], Wrist, Pole, CSAt(Pose, Arm[CHand]).GetRotation());

	// The hand: read which way it lies from its own knuckles (so no bone-axis convention is assumed), then turn it so
	// the fingers run round the rim and the back of the hand faces away from the body.
	const int32 Index1 = Arm[CFinger0], Middle1 = Arm[CFinger0 + 3], Pinky1 = Arm[CFinger0 + 9];
	if (Index1 != INDEX_NONE && Middle1 != INDEX_NONE && Pinky1 != INDEX_NONE)
	{
		const FVector F0 = (CSAt(Pose, Middle1).GetLocation() - CSAt(Pose, Arm[CHand]).GetLocation()).GetSafeNormal();
		const FVector Across0 = CSAt(Pose, Index1).GetLocation() - CSAt(Pose, Pinky1).GetLocation();
		const FVector Back0 = FVector::CrossProduct(F0, Across0).GetSafeNormal();	// a left hand: fingers x (pinky to index)
		if (!F0.IsNearlyZero() && !Back0.IsNearlyZero() && !Finger.IsNearlyZero() && !Back.IsNearlyZero())
		{
			const FQuat Q0 = FRotationMatrix::MakeFromXZ(F0, Back0).ToQuat();
			const FQuat Q1 = FRotationMatrix::MakeFromXZ(Finger, Back).ToQuat();
			Rotate(Pose, Arm[CHand], Q1 * Q0.Inverse());
		}
		// The fingers close over the rim: each joint turns about the line of the knuckles, toward the palm.
		const FVector Knuckles = (CSAt(Pose, Index1).GetLocation() - CSAt(Pose, Pinky1).GetLocation()).GetSafeNormal();
		if (!Knuckles.IsNearlyZero())
		{
			for (int32 f = 0; f < 4; ++f)
			{
				for (int32 j = 0; j < 3; ++j)
				{
					const int32 Bone = Arm[CFinger0 + f * 3 + j];
					if (Bone != INDEX_NONE)
					{
						Rotate(Pose, Bone, FQuat(Knuckles, FMath::DegreesToRadians(static_cast<float>(S.CarryCurl[j]))));
					}
				}
			}
		}
	}

	// Eased in and out against the clip's own arm.
	for (int32 b = CUpper; b < CarryNum; ++b)
	{
		if (Arm[b] != INDEX_NONE)
		{
			FTransform& L = Pose[FCompactPoseBoneIndex(Arm[b])];
			L.SetRotation(FQuat::Slerp(Was[b], L.GetRotation(), W).GetNormalized());
		}
	}
}

void FTakeLocoProxy::Crouch(FCompactPose& Pose)
{
	using namespace TakeLoco;
	for (int32 b = 0; b < Num; ++b)
	{
		if (Bones[b] == INDEX_NONE)
		{
			return;		// a low LOD without the legs: no crouch
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
}
