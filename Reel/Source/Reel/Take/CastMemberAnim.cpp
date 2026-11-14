// QA pass (Nov 14), package cast: see CastMemberAnim.h.
#include "CastMemberAnim.h"

#include "Animation/AnimNodeBase.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimationPoseData.h"
#include "BonePose.h"
#include "Components/SkeletalMeshComponent.h"

namespace CastRig
{
	const FName BoneNames[CastBone::Num] = {
		TEXT("root"), TEXT("pelvis"), TEXT("spine_01"), TEXT("spine_02"), TEXT("spine_03"), TEXT("spine_04"),
		TEXT("spine_05"), TEXT("neck_01"), TEXT("neck_02"), TEXT("head"),
		TEXT("clavicle_l"), TEXT("clavicle_r"), TEXT("upperarm_l"), TEXT("upperarm_r"), TEXT("lowerarm_l"),
		TEXT("lowerarm_r"), TEXT("hand_l"), TEXT("hand_r"), TEXT("middle_01_l"), TEXT("middle_01_r"),
		TEXT("thumb_01_l"), TEXT("thumb_01_r"), TEXT("thigh_l"), TEXT("thigh_r"), TEXT("calf_l"), TEXT("calf_r")};

	const FVector Up(0.f, 0.f, 1.f);

	/** The body's frame (forward, right, up; cm) in mesh space (+Y forward, -X right, +Z up). */
	FVector V(float F, float R, float U) { return FVector(-R, F, U); }

	float Ease(float X) { return FMath::SmoothStep(0.f, 1.f, X); }
	float Frac(float X) { return X - FMath::FloorToFloat(X); }
	/** 0 outside [A, B], easing up to 1 over the first quarter and down over the last. */
	float Bump(float T, float A, float B)
	{
		if (T <= A || T >= B) return 0.f;
		const float Q = (B - A) * 0.25f;
		return FMath::Min(Ease((T - A) / Q), Ease((B - T) / Q));
	}
	/** A struck note: 1 at the onset, decaying over the beat (Phase in [0, 1)). */
	float Strike(float Phase) { return FMath::Exp(-7.f * Phase); }
	/** A stick or a hand over a drum: lowest at the hit (Phase 0), up, then quickly down into the next hit. */
	float Stroke(float Phase)
	{
		return Phase < 0.7f ? Ease(Phase / 0.7f) : 1.f - Ease((Phase - 0.7f) / 0.3f);
	}

	/** Critically damped follow (stable at any step). */
	float SmoothDamp(float X, float& Vel, float Target, float SmoothTime, float Dt)
	{
		const float Omega = 2.f / FMath::Max(SmoothTime, 0.01f);
		const float K = Omega * Dt;
		const float Exp = 1.f / (1.f + K + 0.48f * K * K + 0.235f * K * K * K);
		const float Change = X - Target;
		const float Temp = (Vel + Omega * Change) * Dt;
		Vel = (Vel - Omega * Temp) * Exp;
		return Target + (Change + Temp) * Exp;
	}

	float ToF(double D) { return static_cast<float>(D); }

	/** Local-space edits to a compact pose, expressed as component-space turns. */
	struct FRig
	{
		FCompactPose& P;
		const int32* B;

		bool Has(int32 Bone) const { return B[Bone] != INDEX_NONE; }

		FTransform CSAt(int32 Index) const
		{
			FCompactPoseBoneIndex I(Index);
			FTransform T = P[I];
			FCompactPoseBoneIndex Parent = P.GetParentBoneIndex(I);
			while (Parent.IsValid())
			{
				T = T * P[Parent];
				Parent = P.GetParentBoneIndex(Parent);
			}
			return T;
		}
		FTransform CS(int32 Bone) const { return CSAt(B[Bone]); }
		FVector Loc(int32 Bone) const { return CSAt(B[Bone]).GetLocation(); }

		/** Turns Bone by Q about its own joint, Q in component space (children follow). */
		void Rotate(int32 Bone, const FQuat& Q)
		{
			if (!Has(Bone))
			{
				return;
			}
			const FCompactPoseBoneIndex I(B[Bone]);
			const FCompactPoseBoneIndex Parent = P.GetParentBoneIndex(I);
			const FQuat PR = Parent.IsValid() ? CSAt(Parent.GetInt()).GetRotation() : FQuat::Identity;
			FTransform& L = P[I];
			L.SetRotation((PR.Inverse() * Q * PR * L.GetRotation()).GetNormalized());
		}
		void Turn(int32 Bone, const FVector& Axis, float Degrees)
		{
			if (FMath::Abs(Degrees) > 0.01f)
			{
				Rotate(Bone, FQuat(Axis, FMath::DegreesToRadians(Degrees)));
			}
		}

		/** Two-bone IK: the hand to Target, the elbow toward Pole (blended with where it was by PoleW), the elbow's
		 *  hinge kept (the upper arm rolls so the forearm bends the way the clip bent it), then the hand turned so
		 *  its knuckles point along Finger and its thumb toward Thumb (by RotW). */
		void Arm(bool bRight, const FVector& Target, const FVector& Pole, float PoleW, const FVector& Finger,
			const FVector& Thumb, float RotW)
		{
			const int32 U = bRight ? CastBone::UpperR : CastBone::UpperL;
			const int32 L = bRight ? CastBone::LowerR : CastBone::LowerL;
			const int32 H = bRight ? CastBone::HandR : CastBone::HandL;
			if (!Has(U) || !Has(L) || !Has(H))
			{
				return;
			}
			const FVector A = Loc(U), Bp = Loc(L), Cp = Loc(H);
			const float L1 = ToF((Bp - A).Size()), L2 = ToF((Cp - Bp).Size());
			if (L1 < 1.f || L2 < 1.f)
			{
				return;
			}
			const FVector AT = Target - A;
			float D = ToF(AT.Size());
			if (D < 1.f)
			{
				return;
			}
			const FVector Dir = AT / D;
			D = FMath::Clamp(D, FMath::Abs(L1 - L2) + 0.5f, (L1 + L2) * 0.995f);

			FVector E0 = Bp - A;
			E0 = (E0 - Dir * FVector::DotProduct(E0, Dir)).GetSafeNormal();
			FVector P0 = (Pole - Dir * FVector::DotProduct(Pole, Dir)).GetSafeNormal();
			FVector Side = E0.IsNearlyZero() ? P0 : (P0.IsNearlyZero() ? E0 : FMath::Lerp(E0, P0, PoleW).GetSafeNormal());
			if (Side.IsNearlyZero())
			{
				Side = FVector::CrossProduct(Dir, FVector(1.f, 0.f, 0.f)).GetSafeNormal();
			}
			const float Along = (L1 * L1 - L2 * L2 + D * D) / (2.f * D);
			const float Out = FMath::Sqrt(FMath::Max(L1 * L1 - Along * Along, 0.f));
			const FVector NewB = A + Dir * Along + Side * Out;
			const FVector NewC = A + Dir * D;

			const FVector OldU = (Bp - A) / L1;
			const FVector NewU = (NewB - A).GetSafeNormal();
			const FQuat Swing = FQuat::FindBetweenNormals(OldU, NewU);
			FQuat QU = Swing;
			const FVector N0 = FVector::CrossProduct(Bp - A, Cp - Bp);
			const FVector N1 = FVector::CrossProduct(NewB - A, NewC - NewB);
			if (N0.SizeSquared() > 400.0 && N1.SizeSquared() > 400.0)
			{
				FVector H0 = Swing.RotateVector(N0.GetSafeNormal());
				FVector H1 = N1.GetSafeNormal();
				H0 = (H0 - NewU * FVector::DotProduct(H0, NewU)).GetSafeNormal();
				H1 = (H1 - NewU * FVector::DotProduct(H1, NewU)).GetSafeNormal();
				if (!H0.IsNearlyZero() && !H1.IsNearlyZero())
				{
					FQuat Roll = FQuat::FindBetweenNormals(H0, H1);
					const float MaxRoll = FMath::DegreesToRadians(100.f);
					const float Angle = ToF(Roll.GetAngle());
					if (Angle > MaxRoll)
					{
						Roll = FQuat::Slerp(FQuat::Identity, Roll, MaxRoll / Angle);
					}
					QU = Roll * Swing;
				}
			}
			Rotate(U, QU);

			const FVector B2 = Loc(L), C2 = Loc(H);
			Rotate(L, FQuat::FindBetweenNormals((C2 - B2).GetSafeNormal(), (NewC - B2).GetSafeNormal()));

			const int32 M = bRight ? CastBone::MiddleR : CastBone::MiddleL;
			const int32 T = bRight ? CastBone::ThumbR : CastBone::ThumbL;
			if (RotW > 0.001f && Has(M) && Has(T) && !Finger.IsNearlyZero() && !Thumb.IsNearlyZero())
			{
				const FVector Hp = Loc(H);
				const FVector F0 = Loc(M) - Hp, T0 = Loc(T) - Hp;
				if (FVector::CrossProduct(F0.GetSafeNormal(), T0.GetSafeNormal()).SizeSquared() > 0.01 &&
					FVector::CrossProduct(Finger.GetSafeNormal(), Thumb.GetSafeNormal()).SizeSquared() > 0.01)
				{
					const FQuat Q0(FRotationMatrix::MakeFromXY(F0, T0));
					const FQuat Q1(FRotationMatrix::MakeFromXY(Finger, Thumb));
					Rotate(H, FQuat::Slerp(FQuat::Identity, Q1 * Q0.Inverse(), FMath::Clamp(RotW, 0.f, 1.f)));
				}
			}
		}
	};

	/** One hand's goal from one layer. */
	struct FGoal
	{
		float W = 0.f;
		FVector Pos = FVector::ZeroVector;
		FVector Pole = FVector::ZeroVector;
		FVector Finger = FVector::ZeroVector;
		FVector Thumb = FVector::ZeroVector;

		void Set(float InW, const FVector& InPos, const FVector& InPole, const FVector& InFinger, const FVector& InThumb)
		{
			W = InW; Pos = InPos; Pole = InPole; Finger = InFinger; Thumb = InThumb;
		}
	};

	/** What one layer asks of the body (degrees for the turns). */
	struct FLayer
	{
		FGoal L, R;
		float LeanF = 0.f, LeanS = 0.f, Twist = 0.f;
		float HeadYaw = 0.f, HeadPitch = 0.f, HeadRoll = 0.f;
		float ShrugL = 0.f, ShrugR = 0.f;

		void Scale(float K)
		{
			L.W *= K; R.W *= K;
			LeanF *= K; LeanS *= K; Twist *= K; HeadYaw *= K; HeadPitch *= K; HeadRoll *= K; ShrugL *= K; ShrugR *= K;
		}
		void AddBody(const FLayer& O)
		{
			LeanF += O.LeanF; LeanS += O.LeanS; Twist += O.Twist; HeadYaw += O.HeadYaw; HeadPitch += O.HeadPitch;
			HeadRoll += O.HeadRoll; ShrugL += O.ShrugL; ShrugR += O.ShrugR;
		}
	};

	/** Where things are on the body this frame (component space). */
	struct FFrame
	{
		FVector C;			// between the shoulders
		FVector Head;
		FQuat HeadTurn;		// the head's turn from its reference pose (an anchor on the head follows it)
		FVector SR, SL;		// the shoulders
		FVector KneeL, KneeR, HipL, HipR;
		FVector Point;
		bool bPoint = false;
		float Energy = 1.f;
		float Phase = 0.f;	// 0..1 from the seed
		FTransform ActorToCS;

		FVector OnHead(float F, float R, float U) const { return Head + HeadTurn.RotateVector(V(F, R, U)); }
		FVector Actor(float F, float R, float U) const { return ActorToCS.TransformPosition(FVector(F, R, U)); }
	};

	float DefaultSeconds(ECastAct Act)
	{
		switch (Act)
		{
		case ECastAct::Refuse: return 1.6f;
		case ECastAct::Wave: return 1.5f;
		case ECastAct::Hello: return 1.8f;
		case ECastAct::Applaud: return 3.0f;
		case ECastAct::Talk: return 3.0f;
		case ECastAct::Listen: return 3.0f;
		case ECastAct::Laugh: return 1.6f;
		case ECastAct::Sip: return 2.4f;
		case ECastAct::Nod: return 0.9f;
		case ECastAct::Shrug: return 1.3f;
		case ECastAct::Point: return 1.6f;
		case ECastAct::CheckWatch: return 2.0f;
		case ECastAct::LookAround: return 3.2f;
		case ECastAct::Headset: return 2.0f;
		case ECastAct::Use: return 1.0f;
		case ECastAct::Lift: return 1.5f;
		case ECastAct::Bow: return 0.9f;
		case ECastAct::Speech: return -1.f;
		default: return 1.f;
		}
	}

	/** The act built from bones (no clip): T seconds in, Len long (below 0: open-ended). Weights are 0..1 before the
	 *  act's envelope. Every motion is on a smooth curve; hands lead, the spine and head follow small. */
	void BuildAct(ECastAct Act, float T, float Len, const FFrame& F, FLayer& O)
	{
		const float E = F.Energy;
		const float Ph = F.Phase * 6.2832f;
		const float End = Len > 0.f ? Len : 1e6f;
		switch (Act)
		{
		case ECastAct::Refuse:
		{
			// The stop hand: palm out at chest height, a small shake of the head, weight back.
			O.R.Set(1.f, F.C + V(42.f, 13.f, 4.f), V(-0.3f, 1.f, -0.6f), V(0.1f, 0.f, 1.f), V(0.f, -1.f, 0.f));
			O.HeadYaw = 7.f * FMath::Sin(T * 6.2832f * 2.2f) * Bump(T, 0.15f, FMath::Min(1.1f, End));
			O.LeanF = -2.5f;
			break;
		}
		case ECastAct::Wave:
		case ECastAct::Point:
		{
			// Wave: an open palm sweeps from the chest out toward the door (the act's point, else his right).
			// Point: the arm out toward the point.
			FVector Dir = F.bPoint ? (F.Point - F.SR) : V(1.f, 0.8f, -0.2f);
			Dir.Z = Act == ECastAct::Wave ? -0.25f * Dir.Size2D() : Dir.Z;
			Dir = Dir.GetSafeNormal();
			const bool bLeft = FVector::DotProduct(Dir, V(0.f, 1.f, 0.f)) < -0.2f;
			const float Side = bLeft ? -1.f : 1.f;
			FGoal& G = bLeft ? O.L : O.R;
			const FVector Shoulder = bLeft ? F.SL : F.SR;
			if (Act == ECastAct::Wave)
			{
				const float S = Ease(FMath::Clamp((T - 0.1f) / 0.75f, 0.f, 1.f));
				const FVector From = F.C + V(30.f, 6.f * Side, -16.f);
				const FVector To = Shoulder + Dir * 48.f;
				G.Set(1.f, FMath::Lerp(From, To, S), V(-0.2f, Side, -1.f), FMath::Lerp(V(1.f, 0.f, 0.f), Dir, S),
					V(0.f, Side, 0.6f));
				const FVector D2 = Dir.GetSafeNormal2D();
				O.HeadYaw = 0.5f * FMath::RadiansToDegrees(FMath::Atan2(-ToF(D2.X), ToF(D2.Y))) * S;
			}
			else
			{
				G.Set(1.f, Shoulder + Dir * 52.f, V(-0.2f, Side, -1.f), Dir, V(0.f, 0.f, 1.f));
			}
			break;
		}
		case ECastAct::Hello:
		{
			const float Sway = 5.f * FMath::Sin(T * 6.2832f * 2.f);
			O.R.Set(1.f, F.C + V(16.f, 25.f + Sway, 28.f), V(0.f, 1.f, -1.f), V(0.f, 0.2f, 1.f), V(0.f, -1.f, 0.f));
			O.HeadRoll = 3.f;
			break;
		}
		case ECastAct::Applaud:
		{
			const float Rate = 3.0f + 0.6f * F.Phase;
			const float Gap = 1.5f + 5.5f * (0.5f + 0.5f * FMath::Cos(T * 6.2832f * Rate));
			O.R.Set(1.f, F.C + V(30.f, Gap, -20.f), V(-0.2f, 1.f, -1.f), V(0.6f, -0.15f, 0.8f), V(0.f, 0.f, 1.f));
			O.L.Set(1.f, F.C + V(30.f, -Gap, -20.f), V(-0.2f, -1.f, -1.f), V(0.6f, 0.15f, 0.8f), V(0.f, 0.f, 1.f));
			O.HeadPitch = 2.f;
			break;
		}
		case ECastAct::Talk:
		case ECastAct::Speech:
		{
			// Speech rhythm: the hand drifts on slow curves and lands a beat every 0.7-0.9 s.
			const float Big = Act == ECastAct::Speech ? 1.35f : 1.f;
			const float BeatT = Frac(T / (0.7f + 0.2f * F.Phase));
			const float Land = -3.f * Strike(BeatT) * Big;
			const FVector Drift = V(4.f * FMath::Sin(T * 4.4f + Ph), 5.f * FMath::Sin(T * 2.8f), 4.f * FMath::Sin(T * 6.9f + 2.f * Ph));
			O.R.Set(1.f, F.C + V(28.f, 15.f, -19.f + Land) + Drift * E * Big, V(-0.3f, 1.f, -0.8f), V(1.f, 0.25f, 0.f),
				V(0.f, 0.4f, 1.f));
			if (E * Big > 1.1f)
			{
				const FVector Drift2 = V(3.f * FMath::Sin(T * 3.7f + 1.f), -4.f * FMath::Sin(T * 2.3f + Ph), 3.f * FMath::Sin(T * 5.3f));
				O.L.Set(0.8f, F.C + V(26.f, -15.f, -22.f) + Drift2 * E * Big, V(-0.3f, -1.f, -0.8f), V(1.f, -0.25f, 0.f),
					V(0.f, -0.4f, 1.f));
			}
			O.HeadPitch = 2.5f * FMath::Sin(T * 5.6f) * Big;
			O.LeanF = 2.f * Big;
			O.Twist = 3.f * FMath::Sin(T * 1.3f + Ph);
			break;
		}
		case ECastAct::Listen:
		{
			O.HeadRoll = 4.f * Bump(T, 0.f, End);
			O.HeadPitch = -3.f * FMath::Max(0.f, FMath::Sin(T * 2.4f)) * Bump(T, 0.6f, FMath::Min(2.2f, End));
			break;
		}
		case ECastAct::Laugh:
		{
			const float B = Bump(T, 0.f, End);
			O.LeanF = -4.f * B;
			O.HeadPitch = 6.f * B;
			O.ShrugL = O.ShrugR = 3.f * FMath::Abs(FMath::Sin(T * 6.2832f * 3.5f)) * B;
			break;
		}
		case ECastAct::Nod:
		{
			O.HeadPitch = -9.f * FMath::Abs(FMath::Sin(FMath::Clamp(T / 0.85f, 0.f, 1.f) * 6.2832f));
			break;
		}
		case ECastAct::Sip:
		{
			// From the glass held at the waist to the lips and back, the head tipping back at the top.
			const float S = Ease(FMath::Clamp(T / 0.7f, 0.f, 1.f)) * (1.f - Ease(FMath::Clamp((T - (End - 0.9f)) / 0.8f, 0.f, 1.f)));
			const FVector Hold = F.C + V(22.f, 16.f, -32.f);
			const FVector Lips = F.OnHead(13.f, 4.f, -9.f);
			O.R.Set(1.f, FMath::Lerp(Hold, Lips, S), V(0.f, 1.f, -1.f), FMath::Lerp(V(0.7f, -0.7f, 0.f), V(0.2f, -1.f, 0.f), S),
				FMath::Lerp(V(0.f, 0.f, 1.f), V(-0.5f, 0.f, 1.f), S));
			O.HeadPitch = 8.f * S * S;
			break;
		}
		case ECastAct::Shrug:
		{
			const float B = Bump(T, 0.f, End);
			O.ShrugL = O.ShrugR = 10.f * B;
			O.R.Set(1.f, F.C + V(24.f, 30.f, -30.f), V(-0.4f, 1.f, -0.6f), V(0.8f, 0.5f, 0.f), V(0.f, 1.f, 0.f));
			O.L.Set(1.f, F.C + V(24.f, -30.f, -30.f), V(-0.4f, -1.f, -0.6f), V(0.8f, -0.5f, 0.f), V(0.f, -1.f, 0.f));
			O.HeadRoll = 5.f * B;
			break;
		}
		case ECastAct::CheckWatch:
		{
			O.L.Set(1.f, F.C + V(26.f, -4.f, -12.f), V(-0.2f, -1.f, -1.f), V(0.f, 1.f, 0.f), V(-1.f, 0.f, 0.3f));
			O.HeadPitch = -18.f;
			O.HeadYaw = -10.f;
			break;
		}
		case ECastAct::LookAround:
		{
			const float S = FMath::Clamp(T / FMath::Max(End, 0.1f), 0.f, 1.f);
			O.HeadYaw = 35.f * FMath::Sin((S * 2.f - 1.f) * 1.5708f + Ph * 0.1f) * Bump(T, 0.f, End);
			O.Twist = O.HeadYaw * 0.2f;
			break;
		}
		case ECastAct::Headset:
		{
			O.R.Set(1.f, F.OnHead(1.f, 11.f, 3.f), V(-0.2f, 1.f, -0.5f), V(0.f, 0.f, 1.f), V(1.f, 0.f, 0.f));
			O.HeadRoll = -4.f;
			O.HeadPitch = -3.f;
			break;
		}
		case ECastAct::Use:
		{
			FVector Target = F.bPoint ? F.Point : F.C + V(48.f, 12.f, -18.f);
			FVector Reach = Target - F.SR;
			if (Reach.Size() > 56.0)
			{
				Target = F.SR + Reach.GetSafeNormal() * 56.f;
			}
			O.R.Set(1.f, Target, V(-0.3f, 1.f, -0.8f), (Target - F.SR).GetSafeNormal(), V(0.f, 0.f, 1.f));
			O.LeanF = 6.f;
			break;
		}
		case ECastAct::Lift:
		{
			const float Down = Bump(T, 0.f, FMath::Min(End, 0.9f));
			const float Up = Ease(FMath::Clamp((T - 0.45f) / 0.4f, 0.f, 1.f));
			const FVector Low = F.C + V(36.f, 0.f, -66.f), High = F.C + V(30.f, 0.f, -22.f);
			const FVector P = FMath::Lerp(Low, High, Up);
			O.R.Set(1.f, P + V(0.f, 11.f, 0.f), V(-0.3f, 1.f, -0.6f), V(1.f, 0.f, -0.3f), V(0.f, -1.f, 0.3f));
			O.L.Set(1.f, P + V(0.f, -11.f, 0.f), V(-0.3f, -1.f, -0.6f), V(1.f, 0.f, -0.3f), V(0.f, 1.f, 0.3f));
			O.LeanF = 30.f * Down;
			O.HeadPitch = -10.f * Down;
			break;
		}
		case ECastAct::Bow:
		{
			// Step 5's bow: about 40 degrees from the waist, the head adding a little, a hand to the waist.
			const float S = FMath::Min(Ease(FMath::Clamp(T / (0.33f * End), 0.f, 1.f)),
				1.f - Ease(FMath::Clamp((T / End - 0.55f) / 0.45f, 0.f, 1.f)));
			O.LeanF = 42.f * S * E;
			O.HeadPitch = -12.f * S * E;
			O.R.Set(S, F.C + V(14.f, 4.f, -32.f), V(-0.3f, 1.f, -0.7f), V(0.2f, -1.f, 0.f), V(0.f, 0.f, 1.f));
			break;
		}
		default:
			break;
		}
	}

	/** The band playing, built from bones (no instrument clip), loosely on the beat: each instrument on its own
	 *  subdivision and phase, never a whole-body jolt. */
	void BuildBand(ECastInstrument Inst, float Beat, const FFrame& F, FLayer& O)
	{
		const float Ph = F.Phase;
		switch (Inst)
		{
		case ECastInstrument::Piano:
		{
			// Hands over the keys (22 cm above the bench, 30 cm out), the right hand on the off-beats, the left on 1 and 3,
			// drifting along the keyboard by the phrase; a small sway.
			const float RHit = Strike(Frac(Beat + 0.5f + 0.03f * Ph));
			const float LHit = Strike(Frac(Beat * 0.5f));
			const float RAlong = 15.f + 9.f * FMath::Sin(Beat * 6.2832f / 8.f + Ph * 6.f);
			const float LAlong = -16.f + 6.f * FMath::Sin(Beat * 6.2832f / 16.f + 1.f);
			O.R.Set(1.f, F.Actor(30.f, RAlong, 25.f - 2.5f * RHit), V(-0.2f, 1.f, -1.f), V(1.f, 0.f, -0.5f), V(0.f, -1.f, 0.f));
			O.L.Set(1.f, F.Actor(30.f, LAlong, 25.f - 2.5f * LHit), V(-0.2f, -1.f, -1.f), V(1.f, 0.f, -0.5f), V(0.f, 1.f, 0.f));
			O.Twist = 3.f * FMath::Sin(Beat * 6.2832f / 8.f + Ph * 6.f);
			O.HeadPitch = -6.f - 1.5f * Strike(Frac(Beat));
			O.LeanF = 6.f;
			break;
		}
		case ECastInstrument::Drums:
		{
			// Brushes and sticks: the right hand on the ride on every beat, the left on the snare on 2 and 4.
			const float R = Stroke(Frac(Beat + 0.02f));
			const float L = Stroke(Frac(Beat * 0.5f + 0.5f));
			O.R.Set(1.f, F.Actor(18.f, 20.f, 38.f + 9.f * R), V(-0.3f, 1.f, -0.8f), V(1.f, 0.3f, -0.4f), V(0.f, -0.6f, 1.f));
			O.L.Set(1.f, F.Actor(16.f, -13.f, 25.f + 8.f * L), V(-0.3f, -1.f, -0.8f), V(1.f, -0.2f, -0.5f), V(0.f, 0.6f, 1.f));
			O.HeadPitch = -1.5f * Strike(Frac(Beat));
			O.LeanF = 4.f;
			break;
		}
		case ECastInstrument::Bass:
		{
			// The left hand walks the neck, the right plucks on every beat, a touch behind it (laid back).
			const float Pluck = Strike(Frac(Beat - 0.06f));
			const float Neck = 10.f * FMath::Sin(Beat * 6.2832f / 4.f + Ph * 6.f) + 4.f * FMath::Sin(Beat * 6.2832f / 3.f);
			O.L.Set(1.f, F.Actor(34.f, -14.f, 128.f + Neck), V(-0.3f, -1.f, -0.6f), V(0.f, 1.f, 0.2f), V(0.f, 0.f, 1.f));
			O.R.Set(1.f, F.Actor(34.f, 2.f + 4.f * Pluck, 84.f), V(-0.3f, 1.f, -0.8f), V(0.3f, 0.f, -1.f), V(1.f, 0.f, 0.f));
			O.Twist = 2.f * FMath::Sin(Beat * 6.2832f / 2.f + Ph);
			O.LeanS = 1.5f * FMath::Sin(Beat * 6.2832f / 4.f);
			O.HeadPitch = -1.5f * Strike(Frac(Beat - 0.06f));
			break;
		}
		case ECastInstrument::Trumpet:
		{
			// Three bars up, one bar down (the trumpet lowered with the head); the bell lifts on the long notes.
			const float Bar = Frac(Beat / 16.f);
			const float Rest = Bump(Bar, 0.74f, 1.0f);
			const float Lift = 4.f * FMath::Sin(Beat * 6.2832f / 8.f + Ph * 6.f);
			O.R.Set(1.f, F.OnHead(21.f, 2.f, -8.f), V(-0.3f, 1.f, -1.f), V(1.f, 0.f, 0.f), V(0.f, -1.f, 0.3f));
			O.L.Set(1.f, F.OnHead(29.f, -3.f, -10.f), V(-0.3f, -1.f, -1.f), V(1.f, 0.f, 0.f), V(0.f, 1.f, 0.3f));
			O.HeadPitch = (4.f + Lift) * (1.f - Rest) - 22.f * Rest;
			O.LeanF = -3.f * (1.f - Rest);
			break;
		}
		case ECastInstrument::Singer:
		{
			// A slow sway on two bars, a hand on the stand, the other opening on the phrase ends.
			const float S = Beat * 6.2832f / 8.f;
			O.R.Set(1.f, F.Actor(27.f, 3.f, 128.f), V(-0.3f, 1.f, -1.f), V(0.f, -0.3f, 1.f), V(0.f, -1.f, 0.f));
			const float Open = Bump(Frac(Beat / 8.f), 0.7f, 1.0f);
			O.L.Set(Open, F.C + V(24.f, -28.f, -12.f), V(-0.3f, -1.f, -0.8f), V(0.7f, -0.7f, 0.2f), V(0.f, -0.6f, 1.f));
			O.Twist = 4.f * FMath::Sin(S + Ph);
			O.LeanS = 3.f * FMath::Sin(S + 1.f);
			O.HeadRoll = 3.f * FMath::Sin(S + 2.f);
			break;
		}
		default:
			break;
		}
	}

	void BuildHold(ECastHold Hold, const FFrame& F, FLayer& O)
	{
		switch (Hold)
		{
		case ECastHold::Lap:
			// Seated fallback: palms down on the thighs, 60% of the way to the knees, elbows back and out.
			O.R.Set(1.f, FMath::Lerp(F.HipR, F.KneeR, 0.6f) + V(0.f, 0.f, 10.f), V(-0.4f, 1.f, -0.6f), V(1.f, 0.f, -0.3f), V(0.f, -1.f, 0.f));
			O.L.Set(1.f, FMath::Lerp(F.HipL, F.KneeL, 0.6f) + V(0.f, 0.f, 10.f), V(-0.4f, -1.f, -0.6f), V(1.f, 0.f, -0.3f), V(0.f, 1.f, 0.f));
			break;
		case ECastHold::Glass:
			O.R.Set(1.f, F.C + V(22.f, 16.f, -32.f), V(0.f, 1.f, -1.f), V(0.7f, -0.7f, 0.f), V(0.f, 0.f, 1.f));
			break;
		case ECastHold::ClaspFront:
			O.R.Set(1.f, F.C + V(19.f, -1.f, -47.f), V(-0.3f, 1.f, -0.4f), V(0.2f, -1.f, -0.6f), V(0.f, 0.f, 1.f));
			O.L.Set(1.f, F.C + V(20.f, 2.f, -49.f), V(-0.3f, -1.f, -0.4f), V(0.2f, 1.f, -0.6f), V(0.f, 0.f, 1.f));
			break;
		case ECastHold::ClaspBack:
			O.R.Set(1.f, F.C + V(-17.f, -2.f, -45.f), V(0.f, 1.f, 0.f), V(0.f, -1.f, -0.3f), V(-1.f, 0.f, 0.2f));
			O.L.Set(1.f, F.C + V(-17.f, 3.f, -47.f), V(0.f, -1.f, 0.f), V(0.f, 1.f, -0.3f), V(-1.f, 0.f, 0.2f));
			break;
		default:
			break;
		}
	}
}

float CastActDefaultSeconds(ECastAct Act)
{
	return CastRig::DefaultSeconds(Act);
}

// --- the instance ------------------------------------------------------------------------------------------------------

UCastAnimInstance::UCastAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FAnimInstanceProxy* UCastAnimInstance::CreateAnimInstanceProxy()
{
	return new FCastAnimProxy(this);
}

// --- the proxy ---------------------------------------------------------------------------------------------------------

void FCastAnimProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimSingleNodeInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
	if (const UCastAnimInstance* Inst = Cast<UCastAnimInstance>(InAnimInstance))
	{
		A = Inst->Acting;
	}
	if (const USkeletalMeshComponent* Comp = InAnimInstance ? InAnimInstance->GetSkelMeshComponent() : nullptr)
	{
		const FTransform& W = Comp->GetComponentTransform();
		LookCS = W.InverseTransformPosition(A.LookWorld);
		ActCS = W.InverseTransformPosition(A.ActWorld);
	}
	bCopy = false;
	if (USkeletalMeshComponent* Src = A.CopyFrom.Get())
	{
		const TArrayView<const FTransform> View = Src->GetBoneSpaceTransformsView();
		if (View.Num() > 0)
		{
			CopiedPose.Reset(View.Num());
			CopiedPose.Append(View.GetData(), View.Num());
			bCopy = true;
		}
	}
}

void FCastAnimProxy::UpdateAnimationNode(const FAnimationUpdateContext& InContext)
{
	FAnimSingleNodeInstanceProxy::UpdateAnimationNode(InContext);
	using namespace CastRig;
	const float Dt = FMath::Clamp(InContext.GetDeltaTime(), 0.f, 0.25f);
	Time += Dt;

	// A new base (another clip, or the copied pose on or off): cross-fade from the last pose.
	if (A.BaseSerial != SeenBaseSerial)
	{
		if (SeenBaseSerial != INDEX_NONE && LastOut.Num() > 0)
		{
			Snapshot = LastOut;
			SnapLeft = SnapSeconds;
		}
		SeenBaseSerial = A.BaseSerial;
	}

	// The act.
	if (A.ActSerial != SeenSerial)
	{
		SeenSerial = A.ActSerial;
		if (A.Act == ECastAct::None)
		{
			// Stop: blend out from here.
			if (CurAct != ECastAct::None)
			{
				ActLen = ActT + FMath::Max(A.BlendOut, 0.05f);
			}
		}
		else
		{
			if (ActW > 0.02f && LastOut.Num() > 0)
			{
				Snapshot = LastOut;
				SnapLeft = SnapSeconds;
			}
			CurAct = A.Act;
			CurClip = A.ActClip.Get();
			ActT = 0.f;
			ClipTime = 0.f;
			bUpperOnly = A.bActUpperBody;
			const float ClipLen = CurClip ? CurClip->GetPlayLength() / FMath::Max(A.ActRate, 0.05f) : 0.f;
			ActLen = A.ActSeconds != 0.f ? A.ActSeconds : (CurClip ? ClipLen : DefaultSeconds(CurAct));
			bClipLoop = CurClip && (ActLen < 0.f || ActLen > ClipLen + 0.05f);
		}
	}
	if (CurAct != ECastAct::None)
	{
		bUpperOnly = A.bActUpperBody;	// a crew member who starts walking mid-act keeps the act on the upper body
		ActT += Dt;
		const float In = A.BlendIn > 0.f ? Ease(ActT / A.BlendIn) : 1.f;
		const float Out = ActLen > 0.f ? Ease((ActLen - ActT) / FMath::Max(A.BlendOut, 0.01f)) : 1.f;
		ActW = FMath::Min(In, Out);
		if (CurClip)
		{
			const float L = CurClip->GetPlayLength();
			ClipTime += Dt * A.ActRate;
			ClipTime = bClipLoop && L > 0.f ? FMath::Fmod(ClipTime, L) : FMath::Min(ClipTime, L);
		}
		if (ActLen > 0.f && ActT >= ActLen)
		{
			CurAct = ECastAct::None;
			CurClip = nullptr;
			ActW = 0.f;
		}
	}
	else
	{
		ActW = 0.f;
	}

	// The look, in the body's frame, toward the point (off when it is behind).
	float TargetYaw = 0.f, TargetPitch = 0.f, TargetW = 0.f;
	const FVector D = LookCS - LastHeadCS;
	if (A.bActing && A.bLook && D.SizeSquared() > 100.0)
	{
		const float Yaw = FMath::RadiansToDegrees(ToF(FMath::Atan2(-D.X, D.Y)));
		const float Pitch = FMath::RadiansToDegrees(ToF(FMath::Atan2(D.Z, D.Size2D())));
		if (FMath::Abs(Yaw) < 115.f)
		{
			TargetYaw = FMath::Clamp(Yaw, -A.LookMaxYaw, A.LookMaxYaw);
			TargetPitch = FMath::Clamp(Pitch, -A.LookMaxPitch, A.LookMaxPitch);
			TargetW = A.LookWeight;
		}
	}
	if (TargetW > 0.f || LookW > 0.01f)
	{
		const float Keep = TargetW > 0.f ? 1.f : 0.f;
		LookYaw = SmoothDamp(LookYaw, LookYawVel, Keep > 0.f ? TargetYaw : LookYaw, A.LookSmoothSeconds, Dt);
		LookPitch = SmoothDamp(LookPitch, LookPitchVel, Keep > 0.f ? TargetPitch : LookPitch, A.LookSmoothSeconds, Dt);
	}
	LookW = SmoothDamp(LookW, LookWVel, TargetW, 0.45f, Dt);

	const float Targets[3] = {A.bActing ? A.LeanForward : 0.f, A.bActing ? A.LeanSide : 0.f, A.bActing ? A.Twist : 0.f};
	for (int32 i = 0; i < 3; ++i)
	{
		Lean[i] = SmoothDamp(Lean[i], LeanVel[i], Targets[i], 1.1f, Dt);
	}
	SnapLeft = FMath::Max(0.f, SnapLeft - Dt);
}

void FCastAnimProxy::CacheBones(const FCompactPose& Pose)
{
	const FBoneContainer& BC = Pose.GetBoneContainer();
	if (BC.GetSerialNumber() == CachedSerial && Pose.GetNumBones() == CachedNum)
	{
		return;
	}
	CachedSerial = BC.GetSerialNumber();
	CachedNum = Pose.GetNumBones();
	for (int32 b = 0; b < CastBone::Num; ++b)
	{
		const int32 MeshIndex = BC.GetPoseBoneIndexForBoneName(CastRig::BoneNames[b]);
		Bones[b] = MeshIndex != INDEX_NONE ? BC.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIndex)).GetInt() : INDEX_NONE;
	}
	// The upper body: spine_01 and everything under it.
	UpperMask.SetNumZeroed(CachedNum);
	const int32 Spine = Bones[CastBone::Spine1];
	for (int32 i = 0; i < CachedNum && Spine != INDEX_NONE; ++i)
	{
		FCompactPoseBoneIndex I(i);
		while (I.IsValid() && I.GetInt() != Spine)
		{
			I = Pose.GetParentBoneIndex(I);
		}
		UpperMask[i] = I.IsValid() ? 1 : 0;
	}
	// The head's reference rotation in component space (its face looks along +Y there).
	RefHeadCS = FQuat::Identity;
	if (Bones[CastBone::Head] != INDEX_NONE)
	{
		FQuat R = FQuat::Identity;
		for (FCompactPoseBoneIndex I(Bones[CastBone::Head]); I.IsValid(); I = Pose.GetParentBoneIndex(I))
		{
			R = Pose.GetRefPose(I).GetRotation() * R;
		}
		RefHeadCS = R;
	}
	// The pose to cross-fade from no longer matches.
	LastOut.Reset();
	Snapshot.Reset();
	SnapLeft = 0.f;
}

bool FCastAnimProxy::Evaluate(FPoseContext& Output)
{
	if (bCopy)
	{
		Output.ResetToRefPose();
		const FBoneContainer& BC = Output.Pose.GetBoneContainer();
		for (int32 i = 0; i < Output.Pose.GetNumBones(); ++i)
		{
			const FCompactPoseBoneIndex I(i);
			const int32 MeshIndex = BC.MakeMeshPoseIndex(I).GetInt();
			if (CopiedPose.IsValidIndex(MeshIndex))
			{
				Output.Pose[I] = CopiedPose[MeshIndex];
			}
		}
	}
	else
	{
		FAnimSingleNodeInstanceProxy::Evaluate(Output);
	}
	FCompactPose& Pose = Output.Pose;
	CacheBones(Pose);
	const int32 N = Pose.GetNumBones();

	// The act's clip, over the base (the root stays the base's).
	if (CurClip && ActW > 0.001f)
	{
		FPoseContext ClipContext(Output);
		FAnimationPoseData ClipData(ClipContext);
		CurClip->GetAnimationPose(ClipData, FAnimExtractContext(static_cast<double>(ClipTime), false));
		for (int32 i = 1; i < N; ++i)
		{
			const float W = bUpperOnly ? (UpperMask.IsValidIndex(i) && UpperMask[i] ? ActW : 0.f) : ActW;
			if (W > 0.f)
			{
				const FCompactPoseBoneIndex I(i);
				Pose[I].BlendWith(ClipContext.Pose[I], W);
			}
		}
	}

	ApplyLayers(Pose);

	if (SnapLeft > 0.f && Snapshot.Num() == N)
	{
		const float K = CastRig::Ease(1.f - SnapLeft / SnapSeconds);
		for (int32 i = 0; i < N; ++i)
		{
			const FCompactPoseBoneIndex I(i);
			FTransform T = Snapshot[i];
			T.BlendWith(Pose[I], K);
			Pose[I] = T;
		}
	}
	LastOut.SetNum(N);
	for (int32 i = 0; i < N; ++i)
	{
		LastOut[i] = Pose[FCompactPoseBoneIndex(i)];
	}
	return true;
}

void FCastAnimProxy::ApplyLayers(FCompactPose& Pose)
{
	using namespace CastRig;
	FRig Rig{Pose, Bones};
	if (!Rig.Has(CastBone::Head) || !Rig.Has(CastBone::Spine5))
	{
		return;
	}

	const bool bActProc = A.bActing && CurAct != ECastAct::None && !CurClip && ActW > 0.001f;
	const bool bLean = FMath::Abs(Lean[0]) + FMath::Abs(Lean[1]) + FMath::Abs(Lean[2]) > 0.05f;
	if (!A.bBendSeat && A.Hold == ECastHold::None && !A.bPlayInstrument && !bActProc && LookW < 0.01f && !bLean)
	{
		LastHeadCS = Rig.Loc(CastBone::Head);
		return;
	}

	// 1. The seated fallback: the hips held where the reference pose has them, thighs forward, shins down.
	if (A.bBendSeat)
	{
		if (Rig.Has(CastBone::Pelvis))
		{
			const FCompactPoseBoneIndex I(Bones[CastBone::Pelvis]);
			Pose[I].BlendWith(Pose.GetRefPose(I), 0.85f);
		}
		const FVector Swing(1.f, 0.f, 0.f);
		Rig.Turn(CastBone::ThighL, Swing, A.SeatHipDegrees);
		Rig.Turn(CastBone::ThighR, Swing, A.SeatHipDegrees);
		Rig.Turn(CastBone::CalfL, Swing, -A.SeatKneeDegrees);
		Rig.Turn(CastBone::CalfR, Swing, -A.SeatKneeDegrees);
	}

	// 2. What each layer asks for.
	auto MakeFrame = [&]() {
		FFrame F;
		const FTransform HeadT = Rig.CS(CastBone::Head);
		F.Head = HeadT.GetLocation();
		F.HeadTurn = HeadT.GetRotation() * RefHeadCS.Inverse();
		F.SR = Rig.Has(CastBone::UpperR) ? Rig.Loc(CastBone::UpperR) : F.Head + V(0.f, 18.f, -20.f);
		F.SL = Rig.Has(CastBone::UpperL) ? Rig.Loc(CastBone::UpperL) : F.Head + V(0.f, -18.f, -20.f);
		F.C = (F.SR + F.SL) * 0.5f;
		F.HipR = Rig.Has(CastBone::ThighR) ? Rig.Loc(CastBone::ThighR) : F.C + V(0.f, 10.f, -50.f);
		F.HipL = Rig.Has(CastBone::ThighL) ? Rig.Loc(CastBone::ThighL) : F.C + V(0.f, -10.f, -50.f);
		F.KneeR = Rig.Has(CastBone::CalfR) ? Rig.Loc(CastBone::CalfR) : F.HipR + V(45.f, 0.f, 0.f);
		F.KneeL = Rig.Has(CastBone::CalfL) ? Rig.Loc(CastBone::CalfL) : F.HipL + V(45.f, 0.f, 0.f);
		F.Point = ActCS;
		F.bPoint = A.bActPoint;
		F.Energy = A.Energy;
		F.Phase = float(A.Seed % 1000u) / 1000.f;
		F.ActorToCS = A.ActorToComponent;
		return F;
	};
	FFrame F = MakeFrame();

	FLayer Act, Band, Hold;
	if (bActProc)
	{
		BuildAct(CurAct, ActT, ActLen, F, Act);
		Act.Scale(ActW);
	}
	if (A.bPlayInstrument)
	{
		BuildBand(A.Instrument, A.Beat, F, Band);
	}
	if (A.Hold != ECastHold::None)
	{
		BuildHold(A.Hold, F, Hold);
		if (A.Hold == ECastHold::Podium)
		{
			Hold.L.Set(1.f, F.Actor(A.HoldPointActor.X, A.HoldPointActor.Y, A.HoldPointActor.Z), V(-0.3f, -1.f, -0.6f),
				V(1.f, 0.f, -0.2f), V(0.f, 1.f, 0.f));
		}
	}
	FLayer Body;
	Body.AddBody(Hold);
	Body.AddBody(Band);
	Body.AddBody(Act);
	const float LeanF = Lean[0] + Body.LeanF, LeanS = Lean[1] + Body.LeanS, Twist = Lean[2] + Body.Twist;

	// 3. The spine: the lean spread low to high, the twist high; the shoulders (a shrug).
	const FVector Fwd(-1.f, 0.f, 0.f);		// about -X tips the top forward
	const FVector RightRoll(0.f, -1.f, 0.f);	// about -Y tips the top to the right
	const float LeanShare[3] = {0.35f, 0.35f, 0.30f};
	const int32 LeanBones[3] = {CastBone::Spine1, CastBone::Spine2, CastBone::Spine3};
	for (int32 i = 0; i < 3; ++i)
	{
		Rig.Turn(LeanBones[i], Fwd, LeanF * LeanShare[i]);
		Rig.Turn(LeanBones[i], RightRoll, LeanS * LeanShare[i]);
	}
	const int32 TwistBones[4] = {CastBone::Spine2, CastBone::Spine3, CastBone::Spine4, CastBone::Spine5};
	for (int32 Bone : TwistBones)
	{
		Rig.Turn(Bone, Up, Twist * 0.25f);
	}
	Rig.Turn(CastBone::ClavR, FVector(0.f, 1.f, 0.f), Body.ShrugR);
	Rig.Turn(CastBone::ClavL, FVector(0.f, 1.f, 0.f), -Body.ShrugL);

	// The look: how far the face is from the eased target now, a quarter of it in the upper spine.
	float LookDYaw = 0.f, LookDPitch = 0.f, FaceYaw = 0.f;
	if (LookW > 0.01f)
	{
		const FQuat HeadRot = Rig.CS(CastBone::Head).GetRotation();
		const FVector Face = (HeadRot * RefHeadCS.Inverse()).RotateVector(FVector(0.f, 1.f, 0.f));
		FaceYaw = FMath::RadiansToDegrees(ToF(FMath::Atan2(-Face.X, Face.Y)));
		const float FacePitch = FMath::RadiansToDegrees(ToF(FMath::Atan2(Face.Z, Face.Size2D())));
		LookDYaw = FMath::Clamp(FMath::FindDeltaAngleDegrees(FaceYaw, LookYaw), -90.f, 90.f) * LookW;
		LookDPitch = FMath::Clamp(LookPitch - FacePitch, -45.f, 45.f) * LookW;
		Rig.Turn(CastBone::Spine4, Up, LookDYaw * 0.12f);
		Rig.Turn(CastBone::Spine5, Up, LookDYaw * 0.13f);
	}

	// 4. The arms: each layer's goal over the clip's hand, in order (hold, band, act), then IK once.
	if (Hold.R.W + Band.R.W + Act.R.W + Hold.L.W + Band.L.W + Act.L.W > 0.001f)
	{
		F = MakeFrame();	// the shoulders after the lean
		if (bActProc)
		{
			Act = FLayer();
			BuildAct(CurAct, ActT, ActLen, F, Act);
			Act.Scale(ActW);
		}
		if (A.bPlayInstrument)
		{
			Band = FLayer();
			BuildBand(A.Instrument, A.Beat, F, Band);
		}
		if (A.Hold != ECastHold::None && A.Hold != ECastHold::Podium)
		{
			Hold = FLayer();
			BuildHold(A.Hold, F, Hold);
		}
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const bool bRight = Side == 0;
			const FGoal* Goals[3] = {bRight ? &Hold.R : &Hold.L, bRight ? &Band.R : &Band.L, bRight ? &Act.R : &Act.L};
			const int32 HandBone = bRight ? CastBone::HandR : CastBone::HandL;
			if (!Rig.Has(HandBone))
			{
				continue;
			}
			FVector Pos = Rig.Loc(HandBone);
			FVector Pole = FVector::ZeroVector, Finger = FVector::ZeroVector, Thumb = FVector::ZeroVector;
			float Total = 0.f;
			// An act's clip moves the arms itself: the held postures and the built playing yield to it.
			const float Yield = CurClip ? 1.f - ActW : 1.f;
			for (const FGoal* G : Goals)
			{
				const float W = FMath::Clamp(G->W, 0.f, 1.f) * (G == Goals[2] ? 1.f : Yield);
				if (W <= 0.001f)
				{
					continue;
				}
				Pos = FMath::Lerp(Pos, G->Pos, W);
				if (Total <= 0.f)
				{
					Pole = G->Pole; Finger = G->Finger; Thumb = G->Thumb;
				}
				else
				{
					Pole = FMath::Lerp(Pole, G->Pole, W); Finger = FMath::Lerp(Finger, G->Finger, W); Thumb = FMath::Lerp(Thumb, G->Thumb, W);
				}
				Total = 1.f - (1.f - Total) * (1.f - W);
			}
			if (Total > 0.001f)
			{
				Rig.Arm(bRight, Pos, Pole.GetSafeNormal(), Total, Finger.GetSafeNormal(), Thumb.GetSafeNormal(), Total);
			}
		}
	}

	// 5. The neck and head: the rest of the look, then the layers' nods, shakes and tilts.
	const float HeadYaw = LookDYaw * 0.75f + Body.HeadYaw;
	const float HeadPitch = LookDPitch + Body.HeadPitch;
	if (FMath::Abs(HeadYaw) + FMath::Abs(HeadPitch) + FMath::Abs(Body.HeadRoll) > 0.01f)
	{
		const float FinalYaw = FMath::DegreesToRadians(FaceYaw + HeadYaw);
		const FVector FaceH(-FMath::Sin(FinalYaw), FMath::Cos(FinalYaw), 0.f);
		const FVector PitchAxis = FVector::CrossProduct(FaceH, Up).GetSafeNormal();
		struct FShare { int32 Bone; float Yaw; float Pitch; };
		const FShare Shares[3] = {{CastBone::Neck1, 0.34f, 0.35f}, {CastBone::Neck2, 0.26f, 0.2f}, {CastBone::Head, 0.40f, 0.45f}};
		for (const FShare& S : Shares)
		{
			Rig.Turn(S.Bone, Up, HeadYaw * S.Yaw);
			Rig.Turn(S.Bone, PitchAxis, HeadPitch * S.Pitch);
		}
		Rig.Turn(CastBone::Head, FaceH, -Body.HeadRoll);
	}
	LastHeadCS = Rig.Loc(CastBone::Head);
}
