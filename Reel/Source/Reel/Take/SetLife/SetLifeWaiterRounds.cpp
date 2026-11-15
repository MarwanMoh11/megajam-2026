// Set life piece: waiter rounds (the hall). About 15-30 s. The background filler: short cooldown, double weight, so
// something is moving in the hall while the bigger pieces cool down.
// A waiter (Staff) comes out round the end of the bar with a tray of four coupes and walks 2-3 stops among the
// standing guests near Julian / on screen (cocktail tables, the reserved rows' ends). At each stop the guest turns to
// him; a guest without a glass usually reaches for one (a glass leaves the tray and appears in the guest's hand), a
// guest who already has one (or now and then one who hasn't) declines with the stop hand. A line now and then. Then
// he heads back to the bar; the stage walks him home round the bar's end and removes the tray and the given glasses.
// Stays off the centre aisle, the cross aisle, the rows, the front floor and the dais (tools/sets/hall_zones.json).
#include "SetLife/SetLife.h"

#include "Components/StaticMeshComponent.h"

namespace SetLifeWaiterRounds
{
	// The bar (tools/sets/hall.py: x 800..1600, y -1560..-1470, customers' side +Y; the bartender at (1200, -1700)).
	constexpr float BarX0 = 800.f;
	constexpr float BarX1 = 1600.f;
	constexpr float BarBackY = -1540.f;		// behind the bar: y below this, x between the ends
	constexpr float BarEndBackY = -1660.f;	// round the end: from behind...
	constexpr float BarEndOutY = -1390.f;	// ...to the customers' side

	// The hall has one Staff member (build_cast.py STAFF_AT: the bartender; the other two are in the lobby). The
	// SpilledDrink piece uses him too: any other Staff in the hall is preferred, and the bartender is only the
	// fallback (claims keep the two pieces from sharing him at once). false: the piece runs only with another waiter.
	constexpr bool bBartenderFallback = true;

	constexpr float ExitSpeed = 130.f;		// round the bar's end
	constexpr float WalkSpeed = 115.f;		// between the stops
	constexpr float StandOff = 85.f;		// the waiter stops this far from the guest
	constexpr float MaxFromStart = 1100.f;	// the first stop is at most this far from where he comes out
	constexpr float MaxLeg = 700.f;			// stop to stop
	constexpr float MinGap = 160.f;			// two stops' guests at least this far apart
	constexpr float StopSeconds = 3.9f;		// a stop's beats, about
	constexpr float TargetSeconds = 30.f;

	const FLinearColor Silver(0.55f, 0.54f, 0.50f);
	const FLinearColor Champagne(0.86f, 0.80f, 0.56f);

	/** An axis-aligned box on the floor plan, world cm. */
	struct FArea
	{
		float X0, X1, Y0, Y1;
		bool Has(const FVector& P) const { return P.X >= X0 && P.X <= X1 && P.Y >= Y0 && P.Y <= Y1; }
	};
	// The rows with the centre and cross aisles (rows x 1900..3150, blockers |y| <= 614; Julian's seat and stand
	// point are in here), the front floor and the dais (x 3185..4400, |y| <= 1200), the centre line from the lobby
	// doors to the rows (Julian's way in), and the bar.
	const FArea Rows{ 1850.f, 3170.f, -650.f, 650.f };
	const FArea Front{ 3170.f, 4500.f, -1300.f, 1300.f };
	const FArea CentreLine{ -1000.f, 1850.f, -200.f, 200.f };
	const FArea Bar{ 790.f, 1610.f, -1570.f, -1460.f };
	const FArea BehindBar{ 760.f, 1640.f, -1900.f, -1460.f };

	/** A guest or a stand point may be here. */
	bool IsOpenSpot(const FVector& P)
	{
		return !Rows.Has(P) && !Front.Has(P) && !CentreLine.Has(P) && !BehindBar.Has(P);
	}

	/** A straight walk from A to B keeps out of the rows, the front, the bar and Julian's feet. */
	bool IsOpenLeg(const FVector& A, const FVector& B, const FVector& Julian)
	{
		const int32 N = FMath::Max(1, FMath::CeilToInt(static_cast<float>(FVector::Dist2D(A, B)) / 40.f));
		for (int32 i = 0; i <= N; ++i)
		{
			const FVector P = FMath::Lerp(A, B, static_cast<float>(i) / N);
			if (Rows.Has(P) || Front.Has(P) || Bar.Has(P))
			{
				return false;
			}
			if (!Julian.IsZero() && FVector::Dist2D(P, Julian) < 120.f)
			{
				return false;
			}
		}
		return true;
	}

	bool IsBehindBar(const FVector& P)
	{
		return P.Y < BarBackY && P.X > BarX0 - 40.f && P.X < BarX1 + 40.f;
	}

	bool IsLady(const ACastMember& M)
	{
		return M.Kit == ECastKit::GuestGown || M.Kit == ECastKit::GuestGownHat;
	}

	struct FLine
	{
		const TCHAR* Text;
		const TCHAR* Sound;
	};
	const FLine OfferMadam{ TEXT("Champagne, madam?"), TEXT("sl_waiter_offer_madam") };
	const FLine OfferSir{ TEXT("Champagne, sir?"), TEXT("sl_waiter_offer_sir") };
	const FLine OfferAny[] = {
		{ TEXT("Compliments of the house."), TEXT("sl_waiter_offer_house") },
		{ TEXT("A glass before the bidding?"), TEXT("sl_waiter_offer_bidding") },
	};
	const FLine Accept[] = {
		{ TEXT("Don't mind if I do."), TEXT("sl_waiter_accept_1") },
		{ TEXT("How very kind."), TEXT("sl_waiter_accept_2") },
	};
	const FLine Decline[] = {
		{ TEXT("No thank you, I'm driving the Bentley."), TEXT("sl_waiter_decline_bentley") },
		{ TEXT("Not for me, thank you."), TEXT("sl_waiter_decline_2") },
	};
	const FLine DeclineHasGlass{ TEXT("I've one already, thank you."), TEXT("sl_waiter_decline_glass") };
	const FLine VeryGoodSir{ TEXT("Very good, sir."), TEXT("sl_waiter_verygood_sir") };
	const FLine VeryGoodMadam{ TEXT("Very good, madam."), TEXT("sl_waiter_verygood_madam") };

	template <SIZE_T N>
	const FLine& PickLine(const FSetLifeStage& S, const FLine (&Lines)[N])
	{
		return Lines[S.RandInt(0, static_cast<int32>(N) - 1)];
	}

	class FPiece : public FSetLifePiece
	{
	public:
		virtual bool Setup(FSetLifeStage& S) override
		{
			const FVector Julian = S.GetJulian();

			// --- the waiter: any free Staff in the hall, the bartender last ---
			ACastMember* WaiterMember = nullptr;
			const TArray<ACastMember*> StaffFound = S.FindFree(FSetLifeRole().Staff().AnyDistance(), 8);
			for (ACastMember* M : StaffFound)
			{
				if (!IsBehindBar(M->GetActorLocation()))
				{
					WaiterMember = M;
					break;
				}
			}
			if (!WaiterMember && bBartenderFallback && StaffFound.Num() > 0)
			{
				WaiterMember = StaffFound[0];
			}
			if (!WaiterMember)
			{
				return false;	// no free Staff: never a guest
			}
			const FVector PW = WaiterMember->GetActorLocation();
			const bool bFromBar = IsBehindBar(PW);
			// Out from behind the bar: round the end nearer the guest, to the customers' side.
			auto ExitFor = [PW, bFromBar](const FVector& Toward) -> FVector
			{
				if (!bFromBar)
				{
					return PW;
				}
				const float EndX = FMath::Abs(Toward.X - BarX0) < FMath::Abs(Toward.X - BarX1) ? BarX0 - 70.f : BarX1 + 70.f;
				return FVector(EndX, BarEndOutY, PW.Z);
			};

			// --- the stops: standing guests near Julian / on screen, off his path, within reach of the bar ---
			const TArray<ACastMember*> Cands = S.FindFree(FSetLifeRole().Guests()
				.Where([ExitFor](const ACastMember& M)
				{
					const FVector P = M.GetActorLocation();
					return IsOpenSpot(P) && FVector::Dist2D(P, ExitFor(P)) <= MaxFromStart;
				}), 16);
			if (Cands.Num() < 2)
			{
				return false;
			}

			TArray<ACastMember*> Chosen;
			TArray<FVector> Stands;
			FVector Start = FVector::ZeroVector;
			auto StandFor = [](const FVector& G, const FVector& From)
			{
				FVector Dir = (From - G).GetSafeNormal2D();
				if (Dir.IsNearlyZero())
				{
					Dir = FVector(-1.f, 0.f, 0.f);
				}
				return G + Dir * StandOff;
			};
			// The first: the best placed guest (near Julian, on screen) whose way in is clear.
			for (ACastMember* G : Cands)
			{
				const FVector PG = G->GetActorLocation();
				const FVector From = ExitFor(PG);
				const FVector SP = StandFor(PG, From);
				if (IsOpenSpot(SP) && IsOpenLeg(From, SP, Julian) && IsOpenLeg(SP, From, FVector::ZeroVector))
				{
					Start = From;
					Chosen.Add(G);
					Stands.Add(SP);
					break;
				}
			}
			if (Chosen.Num() == 0)
			{
				return false;
			}
			// Then the nearest guest to the last stop, a few metres on, up to three stops.
			while (Chosen.Num() < 3)
			{
				const FVector Last = Chosen.Last()->GetActorLocation();
				ACastMember* Best = nullptr;
				FVector BestStand = FVector::ZeroVector;
				float BestD = TNumericLimits<float>::Max();
				for (ACastMember* G : Cands)
				{
					if (Chosen.Contains(G))
					{
						continue;
					}
					const FVector PG = G->GetActorLocation();
					const float D = static_cast<float>(FVector::Dist2D(PG, Last));
					if (D < MinGap || D > MaxLeg || D >= BestD)
					{
						continue;
					}
					bool bClear = true;
					for (const ACastMember* C : Chosen)
					{
						bClear &= FVector::Dist2D(PG, C->GetActorLocation()) >= MinGap;
					}
					const FVector SP = StandFor(PG, Stands.Last());
					// ...and his straight way back to the bar from there is clear too.
					if (bClear && IsOpenSpot(SP) && IsOpenLeg(Stands.Last(), SP, Julian) && IsOpenLeg(SP, Start, FVector::ZeroVector))
					{
						Best = G;
						BestStand = SP;
						BestD = D;
					}
				}
				if (!Best)
				{
					break;
				}
				Chosen.Add(Best);
				Stands.Add(BestStand);
			}

			// --- the round's length: out, the legs, the stops, a little of the way back; trim to about 30 s ---
			TArray<FVector> Way;
			if (bFromBar)
			{
				Way.Add(FVector(Start.X, BarEndBackY, PW.Z));
				Way.Add(Start);
			}
			auto Estimate = [&](int32 NumStops)
			{
				float T = 1.5f;
				FVector P = PW;
				for (const FVector& W : Way)
				{
					T += static_cast<float>(FVector::Dist2D(P, W)) / ExitSpeed;
					P = W;
				}
				for (int32 i = 0; i < NumStops; ++i)
				{
					T += static_cast<float>(FVector::Dist2D(P, Stands[i])) / WalkSpeed + StopSeconds;
					P = Stands[i];
				}
				return T;
			};
			while (Chosen.Num() > 2 && Estimate(Chosen.Num()) > TargetSeconds)
			{
				Chosen.Pop();
				Stands.Pop();
			}
			if (Chosen.Num() < 2 || Estimate(Chosen.Num()) > TargetSeconds + 4.f)
			{
				return false;
			}

			// --- cast ---
			Waiter = S.CastMember(TEXT("Waiter"), WaiterMember);
			if (Waiter == INDEX_NONE)
			{
				return false;
			}
			TArray<int32> Guests;
			TArray<bool> bLady;
			TArray<bool> bHasGlass;
			for (int32 i = 0; i < Chosen.Num(); ++i)
			{
				bLady.Add(IsLady(*Chosen[i]));
				bHasGlass.Add(Chosen[i]->bHoldsGlass);
				const int32 G = S.CastMember(*FString::Printf(TEXT("Guest%d"), i + 1), Chosen[i]);
				if (G == INDEX_NONE)
				{
					return false;	// the stage releases whoever was cast
				}
				Guests.Add(G);
			}
			S.SetAnchor(S.Where(Guests[0]));
			if (bFromBar)
			{
				S.SetReturnVia(Waiter, { Way[1], Way[0] });
			}

			// --- props: a tray with four coupes in the right hand ---
			S.HoldProp(Waiter, ETakeShape::Cylinder, FVector(38.f, 38.f, 1.5f), Silver, FVector(10.f, 0.f, 3.f));
			const FVector2D Slots[] = { FVector2D(4.f, 7.f), FVector2D(4.f, -7.f), FVector2D(17.f, 7.f), FVector2D(17.f, -7.f) };
			for (const FVector2D& Slot : Slots)
			{
				TrayGlasses.Add(S.HoldProp(Waiter, ETakeShape::Cylinder, FVector(6.f, 6.f, 11.f), Champagne,
					FVector(Slot.X, Slot.Y, 9.5f)));
			}

			// Who takes one: a guest with a coupe already declines; one without usually takes (at least one does).
			TArray<bool> bTakes;
			bool bAnyTakes = false;
			for (int32 i = 0; i < Guests.Num(); ++i)
			{
				const bool bTake = !bHasGlass[i] && S.Chance(0.7f);
				bTakes.Add(bTake);
				bAnyTakes |= bTake;
			}
			if (!bAnyTakes)
			{
				const int32 First = bHasGlass.Find(false);
				if (First != INDEX_NONE)
				{
					bTakes[First] = true;
				}
			}

			// --- the beat sheet ---
			for (const FVector& P : Way)
			{
				S.Walk(Waiter, P, ExitSpeed).WaitArrived(Waiter, 8.f);
			}
			FVector From = Way.Num() > 0 ? Way.Last() : PW;
			for (int32 i = 0; i < Guests.Num(); ++i)
			{
				const int32 G = Guests[i];
				const FVector SP = Stands[i];
				const FVector PG = S.Where(G);
				const FVector TrayAt = SP + (PG - SP).GetSafeNormal2D() * 25.f + FVector(0.f, 0.f, 105.f);
				const float Leg = static_cast<float>(FVector::Dist2D(From, SP));
				From = SP;

				S.Walk(Waiter, SP, WalkSpeed).WaitArrived(Waiter, Leg / WalkSpeed + 3.f);
				S.FaceTo(Waiter, G).FaceTo(G, Waiter).Look(G, Waiter, 3.f);
				S.Wait(0.35f);
				if (i == 0 || S.Chance(0.5f))
				{
					const FLine& Offer = S.Chance(0.55f) ? (bLady[i] ? OfferMadam : OfferSir) : PickLine(S, OfferAny);
					S.Say(Waiter, Offer.Text, Offer.Sound);
				}
				S.Wait(0.55f);
				if (bTakes[i])
				{
					// Reaches to the tray; a glass leaves it and is in the hand.
					S.GestureAtPoint(G, TEXT("Use"), TrayAt).Wait(0.45f);
					S.Do([this, G](FSetLifeStage& St) { GiveGlass(St, G); });
					S.Wait(0.35f).GestureAt(G, TEXT("Nod"), Waiter);
					if (S.Chance(0.45f))
					{
						const FLine& L = PickLine(S, Accept);
						S.Say(G, L.Text, L.Sound);
					}
					S.Wait(0.45f).Gesture(Waiter, TEXT("Nod")).Wait(0.8f);
				}
				else
				{
					// A polite no: the stop hand.
					S.Gesture(G, TEXT("Refuse"));
					if (bHasGlass[i] ? S.Chance(0.35f) : S.Chance(0.5f))
					{
						const FLine& L = bHasGlass[i] ? DeclineHasGlass : PickLine(S, Decline);
						S.Say(G, L.Text, L.Sound);
					}
					S.Wait(0.6f).Gesture(Waiter, TEXT("Nod"));
					if (S.Chance(0.3f))
					{
						const FLine& L = bLady[i] ? VeryGoodMadam : VeryGoodSir;
						S.Say(Waiter, L.Text, L.Sound);
					}
					S.Wait(0.9f);
				}
			}
			// Back toward the bar; the stage walks him the rest of the way (round the bar's end) once this ends.
			S.Walk(Waiter, Way.Num() > 0 ? Way.Last() : PW, WalkSpeed).Wait(1.5f);
			return true;
		}

	private:
		/** One coupe off the tray, into the guest's right hand (the stage removes both at the end). */
		void GiveGlass(FSetLifeStage& S, int32 Guest)
		{
			if (TrayGlasses.Num() > 0)
			{
				S.RemoveProp(TrayGlasses.Pop());
			}
			S.HoldProp(Guest, ETakeShape::Cylinder, FVector(6.f, 6.f, 11.f), Champagne, FVector(4.f, 0.f, 6.f));
		}

		int32 Waiter = INDEX_NONE;
		TArray<int32> TrayGlasses;
	};
}

SETLIFE_PIECE(WaiterRounds, SetLifeWaiterRounds::FPiece, ESetLifeZone::Hall, 12.f, 2.f)
