// Set life piece: the toast (the hall, by the bar and the cocktail tables). About 12-22 s.
// A standing group of 3-4 guests near the bar (glass holders first). If the bartender is free he comes round the end
// of the bar with a tray and the host takes a fresh coupe. The host calls for attention and raises a toast, the others
// face him, the glass holders sip together on the cue, one says "Hear, hear!", one laughs, another claps; a guest or
// two standing within ~4 m glance over and one nods. Then the group talks on and the waiter goes back to the bar.
// The bartender is a normal claimed role: the stage never puts one member in two pieces, so this piece and the
// spilled drink can never both have him; when he is busy the toast runs without a waiter.
#include "SetLife/SetLife.h"

#include "Components/StaticMeshComponent.h"

namespace SetLifeToast
{
	// The bar (tools/sets/hall.py bar(): x 800..1600, y -1560..-1470, the customers' side is +Y; build_cast.py stands
	// the bartender behind it at (1200, -1700)). Same numbers as the spilled drink.
	const FVector BarFront(1200.f, -1300.f, 0.f);
	constexpr float BarX0 = 800.f;
	constexpr float BarX1 = 1600.f;
	constexpr float BarBackY = -1540.f;		// behind the bar: y below this, x between the ends
	constexpr float BarEndBackY = -1660.f;	// round the end: from behind...
	constexpr float BarEndOutY = -1390.f;	// ...to the customers' side
	constexpr float BarClearY = -1400.f;	// a waiter's mark between the ends stays on the customers' side of this

	const FLinearColor Silver(0.55f, 0.54f, 0.50f);
	const FLinearColor GlassTone(0.80f, 0.76f, 0.62f);

	struct FLine
	{
		const TCHAR* Text;
		const TCHAR* Sound;
	};
	const FLine Offer[] = {
		{ TEXT("Champagne? Fresh off the ice."), TEXT("sl_toast_waiter_1") },
		{ TEXT("A fresh glass, with the house's compliments."), TEXT("sl_toast_waiter_2") },
	};
	const FLine Call[] = {
		{ TEXT("Friends! A moment, if you please."), TEXT("sl_toast_call_1") },
		{ TEXT("I say, a word, everyone!"), TEXT("sl_toast_call_2") },
	};
	const FLine Toast[] = {
		{ TEXT("To Lot Forty-One, and the fool who pays for it!"), TEXT("sl_toast_toast_1") },
		{ TEXT("To the old girl: may she fetch a king's ransom!"), TEXT("sl_toast_toast_2") },
	};
	const FLine Reply[] = {
		{ TEXT("Hear, hear!"), TEXT("sl_toast_reply_1") },
		{ TEXT("Here's how!"), TEXT("sl_toast_reply_2") },
	};
	const FLine Quip[] = {
		{ TEXT("Ha! Well said!"), TEXT("sl_toast_laugh_1") },
		{ TEXT("Ha! Not me, I hope!"), TEXT("sl_toast_laugh_2") },
	};

	template <int32 N>
	const FLine& PickLine(const FSetLifeStage& S, const FLine (&Lines)[N])
	{
		return Lines[S.RandInt(0, N - 1)];
	}

	class FPiece : public FSetLifePiece
	{
	public:
		virtual bool Setup(FSetLifeStage& S) override
		{
			// --- casting: a standing group of 3-4 near the bar, seeded on a glass holder if there is one ---
			TArray<ACastMember*> Group;
			for (int32 Pass = 0; Pass < 2 && Group.Num() == 0; ++Pass)
			{
				FSetLifeRole Seed = FSetLifeRole().Guests().NearPoint(BarFront, 1500.f);
				if (Pass == 0)
				{
					Seed.WithGlass(true);
				}
				for (ACastMember* G : S.FindFree(Seed, 10))
				{
					TArray<ACastMember*> Near = S.FindFree(FSetLifeRole().Guests().AnyDistance()
						.NearPoint(G->GetActorLocation(), 280.f)
						.Where([G](const ACastMember& M) { return &M != G; }), 6);
					if (Near.Num() < 2)
					{
						continue;
					}
					// Glass holders first (they can sip on the cue), keep FindFree's order otherwise.
					Near.StableSort([](const ACastMember& A, const ACastMember& B) { return A.bHoldsGlass && !B.bHoldsGlass; });
					Group.Add(G);
					for (int32 i = 0; i < Near.Num() && Group.Num() < 4; ++i)
					{
						Group.Add(Near[i]);
					}
					break;
				}
			}
			if (Group.Num() < 3)
			{
				return false;
			}
			for (int32 i = 0; i < Group.Num(); ++i)
			{
				const int32 R = S.CastMember(*FString::Printf(TEXT("Guest%d"), i + 1), Group[i]);
				if (R != INDEX_NONE)
				{
					Roles.Add(R);
					bGlass.Add(Group[i]->bHoldsGlass);
				}
			}
			if (Roles.Num() < 3)
			{
				return false;	// the stage releases whoever was cast
			}

			// The waiter is optional: the bartender if no other piece has him.
			Waiter = S.Cast(TEXT("Waiter"), FSetLifeRole().Staff().AnyDistance().NearPoint(BarFront, 1600.f));

			// The host: with a waiter, someone without a glass (he takes the fresh one); without, a glass holder.
			int32 HostSlot = 0;
			for (int32 i = 0; i < Roles.Num(); ++i)
			{
				if (bGlass[i] == (Waiter == INDEX_NONE))
				{
					HostSlot = i;
					break;
				}
			}
			Host = Roles[HostSlot];
			bHostHeld = bGlass[HostSlot];
			TArray<int32> Others;
			TArray<bool> OthersGlass;
			for (int32 i = 0; i < Roles.Num(); ++i)
			{
				if (i != HostSlot)
				{
					Others.Add(Roles[i]);
					OthersGlass.Add(bGlass[i]);
				}
			}
			const bool bHostSips = bHostHeld || Waiter != INDEX_NONE;

			// --- the geometry ---
			FVector Centre = FVector::ZeroVector;
			for (const int32 R : Roles)
			{
				Centre += S.Where(R);
			}
			Centre /= static_cast<float>(Roles.Num());
			const FVector PH = S.Where(Host);
			FVector OutDir = (PH - Centre).GetSafeNormal2D();
			if (OutDir.IsNearlyZero())
			{
				OutDir = FVector(0.f, 1.f, 0.f);
			}
			S.SetAnchor(Centre);

			// Onlookers: one or two standing guests within ~4 m, outside the group.
			for (ACastMember* M : S.FindFree(FSetLifeRole().Guests().AnyDistance().NearPoint(Centre, 400.f), 2))
			{
				const int32 R = S.CastMember(*FString::Printf(TEXT("Onlooker%d"), Onlookers.Num() + 1), M);
				if (R != INDEX_NONE)
				{
					Onlookers.Add(R);
				}
			}

			// The waiter's way: out round the bar's nearer end (and back that way at the end), then up beside the host.
			TArray<FVector> Way;
			FVector Approach = FVector::ZeroVector;
			FVector Out = FVector::ZeroVector;
			if (Waiter != INDEX_NONE)
			{
				const FVector PW = S.Where(Waiter);
				Approach = PH + OutDir * 95.f;
				Approach.Z = PW.Z;
				if (Approach.X > BarX0 - 60.f && Approach.X < BarX1 + 60.f)
				{
					Approach.Y = FMath::Max(Approach.Y, BarClearY);
				}
				if (PW.Y < BarBackY && PW.X > BarX0 - 20.f && PW.X < BarX1 + 20.f)
				{
					const float EndX = FMath::Abs(Approach.X - BarX0) < FMath::Abs(Approach.X - BarX1) ? BarX0 - 70.f : BarX1 + 70.f;
					Way.Add(FVector(EndX, BarEndBackY, PW.Z));
					Way.Add(FVector(EndX, BarEndOutY, PW.Z));
					S.SetReturnVia(Waiter, { Way[0] });
				}
				Out = Way.Num() > 0 ? Way.Last() : Approach + OutDir * 150.f;

				// The tray with two coupes (stem and bowl), carried in the right hand. One is the host's.
				S.HoldProp(Waiter, ETakeShape::Cylinder, FVector(38.f, 38.f, 1.5f), Silver, FVector(10.f, 0.f, 3.f));
				TrayStem = S.HoldProp(Waiter, ETakeShape::Cylinder, FVector(1.f, 1.f, 8.f), GlassTone, FVector(15.f, -7.f, 7.75f));
				TrayBowl = S.HoldProp(Waiter, ETakeShape::Sphere, FVector(9.f, 9.f, 4.5f), GlassTone, FVector(15.f, -7.f, 13.5f));
				S.HoldProp(Waiter, ETakeShape::Cylinder, FVector(1.f, 1.f, 8.f), GlassTone, FVector(5.f, 8.f, 7.75f));
				S.HoldProp(Waiter, ETakeShape::Sphere, FVector(9.f, 9.f, 4.5f), GlassTone, FVector(5.f, 8.f, 13.5f));
			}

			const FLine& LOffer = PickLine(S, Offer);
			const FLine& LCall = PickLine(S, Call);
			const FLine& LToast = PickLine(S, Toast);
			const FLine& LReply = PickLine(S, Reply);
			const FLine& LQuip = PickLine(S, Quip);
			const int32 First = Others[0];
			const int32 Second = Others[1];

			// --- the beat sheet ---
			// The group chats; the waiter comes over.
			S.GestureAt(Host, TEXT("Talk"), First).GestureAt(First, TEXT("Listen"), Host);
			if (Waiter != INDEX_NONE)
			{
				for (const FVector& P : Way)
				{
					S.Walk(Waiter, P, 130.f).WaitArrived(Waiter, 8.f);
				}
				S.Walk(Waiter, Approach, 125.f).GestureAt(Host, TEXT("Talk"), Second).WaitArrived(Waiter, 9.f);
				S.FaceTo(Waiter, Host).FaceTo(Host, Waiter);
				S.Wait(0.3f).Say(Waiter, LOffer.Text, LOffer.Sound).Gesture(Waiter, TEXT("Bow"));
				S.Wait(0.9f).Do([this](FSetLifeStage& St) { ReachForGlass(St); });
				S.Wait(0.55f).Do([this](FSetLifeStage& St) { TakeGlass(St); });
				S.Wait(0.3f).GestureAt(Host, TEXT("Nod"), Waiter);
				S.Wait(0.6f).Walk(Waiter, Out, 120.f);
			}
			else
			{
				S.Wait(1.5f);
			}

			// The call for attention: the host turns to the group, they turn to him; neighbours glance over.
			S.FacePoint(Host, Centre).Gesture(Host, TEXT("Hello")).Say(Host, LCall.Text, LCall.Sound);
			for (const int32 R : Others)
			{
				S.FaceTo(R, Host).GestureAt(R, TEXT("Listen"), Host);
			}
			for (const int32 R : Onlookers)
			{
				S.Look(R, Host, 4.5f);
			}

			// The toast.
			S.Wait(1.6f).Gesture(Host, TEXT("Speech")).Say(Host, LToast.Text, LToast.Sound);
			for (const int32 R : Others)
			{
				S.GestureAt(R, TEXT("Listen"), Host);
			}

			// The cue: "Hear, hear!", and every glass goes up together.
			S.Wait(2.6f).Say(First, LReply.Text, LReply.Sound);
			S.Gesture(Host, bHostSips ? TEXT("Sip") : TEXT("Nod"));
			for (int32 i = 0; i < Others.Num(); ++i)
			{
				if (OthersGlass[i])
				{
					S.Gesture(Others[i], TEXT("Sip"));
				}
				else
				{
					S.GestureAt(Others[i], TEXT("Nod"), Host);
				}
			}
			S.WaitGesture(Host, 3.f);

			// One laughs, another claps; a neighbour nods.
			S.Wait(0.2f).GestureAt(First, TEXT("Laugh"), Host).Say(First, LQuip.Text, LQuip.Sound);
			S.GestureAt(Second, TEXT("Applaud"), Host);
			if (Others.Num() > 2)
			{
				S.GestureAt(Others[2], TEXT("Nod"), Host);
			}
			if (Onlookers.Num() > 0)
			{
				S.GestureAt(Onlookers[0], TEXT("Nod"), Host);
			}
			S.Wait(0.4f).GestureAt(Host, TEXT("Laugh"), First);

			// The group talks on.
			S.Wait(2.4f).GestureAt(Host, TEXT("Talk"), Second).GestureAt(Second, TEXT("Listen"), Host);
			S.GestureAt(First, TEXT("Talk"), Host);
			if (Waiter != INDEX_NONE)
			{
				S.WaitArrived(Waiter, 6.f);
			}
			S.Wait(2.f);
			return true;
		}

	private:
		/** The host reaches for the coupe on the tray. */
		void ReachForGlass(FSetLifeStage& S)
		{
			ACastMember* H = S.Get(Host);
			if (!H)
			{
				return;
			}
			const UStaticMeshComponent* Bowl = S.GetProp(TrayBowl);
			const FVector At = Bowl ? Bowl->GetComponentLocation() : S.Where(Waiter) + FVector(0.f, 0.f, 110.f);
			H->PlayGestureAt(TEXT("Use"), At);
		}

		/** The coupe leaves the tray for the host's hand (the free one if he already holds a glass). */
		void TakeGlass(FSetLifeStage& S)
		{
			S.RemoveProp(TrayStem);
			S.RemoveProp(TrayBowl);
			const FName Bone = bHostHeld ? FName(TEXT("hand_l")) : FName(TEXT("hand_r"));
			S.HoldProp(Host, ETakeShape::Cylinder, FVector(1.f, 1.f, 8.f), GlassTone, FVector(3.f, 0.f, 2.f), Bone);
			S.HoldProp(Host, ETakeShape::Sphere, FVector(9.f, 9.f, 4.5f), GlassTone, FVector(3.f, 0.f, 7.5f), Bone);
		}

		TArray<int32> Roles;
		TArray<bool> bGlass;
		TArray<int32> Onlookers;
		int32 Host = INDEX_NONE;
		int32 Waiter = INDEX_NONE;
		int32 TrayStem = INDEX_NONE;
		int32 TrayBowl = INDEX_NONE;
		bool bHostHeld = false;
	};
}

SETLIFE_PIECE(Toast, SetLifeToast::FPiece, ESetLifeZone::Hall, 60.f, 1.f)
