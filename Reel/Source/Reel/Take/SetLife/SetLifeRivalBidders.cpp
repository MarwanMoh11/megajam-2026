// Set life piece: the rival bidders (the hall, the seated rows). About 13-16 s.
// Two seated gentlemen-or-ladies one row apart: the one in front (Front) turns his head to the one behind (Back) and
// points; Back answers with a dismissive talk and a shrug; three barks between them. The neighbours within ~2 m look
// round at them, one laughs or nods; a guest in the row ahead turns and shushes them with the stop hand. Then everyone
// faces the dais again. Hitman's Showstopper auction (overheard bidders), the auction-floor needle of period films.
// Seated members are never moved or turned (the framework ignores Walk / Face for them): looks and gestures only.
// The heist's dark ends hall pieces (USetLifeSubsystem::GetGate); the seated just look back to the podium themselves.
#include "SetLife/SetLife.h"

namespace SetLifeRivalBidders
{
	// The rows (tools/sets/hall_zones.json rows x 1900..3150; hall_seats.json: rows ~100 cm apart in X, seats ~62 cm
	// apart in Y, yaw 0 = facing +X, the dais). The bar stools and the phone bidders' chairs are left out by X and yaw.
	constexpr float RowsX0 = 1850.f;
	constexpr float RowsX1 = 3200.f;
	constexpr float RowStepMin = 60.f;		// "one row apart": the one behind is 60..150 cm further from the dais
	constexpr float RowStepMax = 150.f;
	constexpr float SameFileY = 50.f;		// ...and nearly straight behind (less than one seat to the side)
	constexpr float ShushSideY = 130.f;		// the shusher: in the row ahead of Front, within two seats to the side
	constexpr float NeighbourRadius = 200.f;

	struct FLine
	{
		const TCHAR* Text;
		const TCHAR* Sound;
	};
	const FLine Needle[] = {
		{ TEXT("You took the Degas from me, Pemberton."), TEXT("sl_rivals_needle_1") },
		{ TEXT("Not this time, Pemberton. Not this lot."), TEXT("sl_rivals_needle_2") },
	};
	const FLine Brush[] = {
		{ TEXT("May the deeper pocket win, old man."), TEXT("sl_rivals_brush_1") },
		{ TEXT("Your pockets aren't that deep, old man."), TEXT("sl_rivals_brush_2") },
	};
	const FLine Vow[] = {
		{ TEXT("I'll go to the last penny. Mark me."), TEXT("sl_rivals_vow_1") },
		{ TEXT("We shall see whose pocket runs dry."), TEXT("sl_rivals_vow_2") },
	};
	const FLine Aside[] = {
		{ TEXT("Ha! Every season, those two."), TEXT("sl_rivals_aside_1") },
		{ TEXT("Here we go again."), TEXT("sl_rivals_aside_2") },
	};
	const FLine Shush[] = {
		{ TEXT("Shh!"), TEXT("sl_rivals_shush_1") },
		{ TEXT("Shh! Do you mind?"), TEXT("sl_rivals_shush_2") },
	};

	template <SIZE_T N>
	const FLine& PickLine(const FSetLifeStage& S, const FLine (&Lines)[N])
	{
		return Lines[S.RandInt(0, static_cast<int32>(N) - 1)];
	}

	/** A seat in the rows, facing the dais. */
	bool IsRowSeat(const ACastMember& M)
	{
		const FVector P = M.GetActorLocation();
		const float Yaw = FRotator::NormalizeAxis(M.GetActorRotation().Yaw);
		return M.bSeated && P.X > RowsX0 && P.X < RowsX1 && FMath::Abs(Yaw) < 30.f;
	}

	class FPiece : public FSetLifePiece
	{
	public:
		virtual bool Setup(FSetLifeStage& S) override
		{
			// --- casting: Front (on screen if we can), Back straight behind him, a shusher ahead, the neighbours ---
			TArray<ACastMember*> Fronts = S.FindFree(FSetLifeRole().Guests().Seated().OnScreen()
				.Where([](const ACastMember& M) { return IsRowSeat(M); }), 10);
			if (Fronts.Num() == 0)
			{
				Fronts = S.FindFree(FSetLifeRole().Guests().Seated()
					.Where([](const ACastMember& M) { return IsRowSeat(M); }), 10);
			}

			ACastMember* PickF = nullptr;
			ACastMember* PickB = nullptr;
			ACastMember* PickS = nullptr;
			TArray<ACastMember*> PickN;
			for (ACastMember* F : Fronts)
			{
				const FVector PF = F->GetActorLocation();
				const TArray<ACastMember*> Behind = S.FindFree(FSetLifeRole().Guests().Seated().AnyDistance()
					.NearPoint(PF, RowStepMax + 20.f)
					.Where([F, PF](const ACastMember& M)
					{
						const FVector P = M.GetActorLocation();
						const float Dx = PF.X - P.X;
						return &M != F && IsRowSeat(M) && Dx > RowStepMin && Dx < RowStepMax
							&& FMath::Abs(P.Y - PF.Y) < SameFileY;
					}), 1);
				if (Behind.Num() == 0)
				{
					continue;
				}
				ACastMember* B = Behind[0];
				const TArray<ACastMember*> Ahead = S.FindFree(FSetLifeRole().Guests().Seated().AnyDistance()
					.NearPoint(PF, 220.f)
					.Where([F, PF](const ACastMember& M)
					{
						const FVector P = M.GetActorLocation();
						const float Dx = P.X - PF.X;
						return &M != F && IsRowSeat(M) && Dx > RowStepMin && Dx < RowStepMax
							&& FMath::Abs(P.Y - PF.Y) < ShushSideY;
					}), 1);
				ACastMember* Sh = Ahead.Num() > 0 ? Ahead[0] : nullptr;
				const FVector Mid = (PF + B->GetActorLocation()) * 0.5f;
				const TArray<ACastMember*> Near = S.FindFree(FSetLifeRole().Guests().Seated().AnyDistance()
					.NearPoint(Mid, NeighbourRadius)
					.Where([F, B, Sh](const ACastMember& M) { return &M != F && &M != B && &M != Sh && IsRowSeat(M); }), 4);
				// A scene needs an audience: at least two onlookers (the shusher counts).
				if (Near.Num() + (Sh ? 1 : 0) < 2)
				{
					continue;
				}
				PickF = F;
				PickB = B;
				PickS = Sh;
				PickN = Near;
				break;
			}
			if (!PickF)
			{
				return false;
			}

			const int32 Front = S.CastMember(TEXT("Front"), PickF);
			const int32 Back = S.CastMember(TEXT("Back"), PickB);
			if (Front == INDEX_NONE || Back == INDEX_NONE)
			{
				return false;	// the stage releases whoever was cast
			}
			const int32 Shusher = PickS ? S.CastMember(TEXT("Shusher"), PickS) : INDEX_NONE;
			TArray<int32> Neighbours;
			for (ACastMember* N : PickN)
			{
				const int32 R = S.CastMember(TEXT("Neighbour"), N);
				if (R != INDEX_NONE)
				{
					Neighbours.Add(R);
				}
			}
			if (Neighbours.Num() + (Shusher != INDEX_NONE ? 1 : 0) < 1)
			{
				return false;
			}

			// --- the geometry: the pair's heads, the podium ---
			const FVector HeadF = S.Head(Front);
			const FVector HeadB = S.Head(Back);
			const FVector Pair = (HeadF + HeadB) * 0.5f;
			const FVector Podium = PickF->PodiumPoint;
			S.SetAnchor((S.Where(Front) + S.Where(Back)) * 0.5f);

			const FLine& L1 = PickLine(S, Needle);
			const FLine& L2 = PickLine(S, Brush);
			const FLine& L3 = PickLine(S, Vow);
			const FLine& L4 = PickLine(S, Aside);
			const FLine& L5 = PickLine(S, Shush);
			// One neighbour reacts out loud (laugh or nod), the first one (nearest the pair, on screen first).
			const int32 Wag = Neighbours.Num() > 0 ? Neighbours[0] : INDEX_NONE;
			const bool bWagLaughs = S.Chance(0.6f);

			// --- the beat sheet ---
			// Front turns round to Back and points.
			S.Look(Front, Back, 4.f);
			S.Wait(0.35f).GestureAt(Front, TEXT("Point"), Back).Say(Front, L1.Text, L1.Sound);
			S.Look(Back, Front, 6.f);
			// The neighbours look round, one after another.
			for (int32 i = 0; i < Neighbours.Num(); ++i)
			{
				S.Wait(i == 0 ? 0.6f : S.Rand(0.15f, 0.4f)).LookAtPoint(Neighbours[i], Pair, 7.5f);
			}
			// Back brushes it off.
			S.At(2.6f).GestureAt(Back, TEXT("Talk"), Front).Say(Back, L2.Text, L2.Sound);
			S.Wait(1.3f).GestureAt(Back, TEXT("Shrug"), Front);
			S.Wait(0.6f).Look(Front, Back, 4.f);
			if (Wag != INDEX_NONE)
			{
				S.GestureAt(Wag, bWagLaughs ? TEXT("Laugh") : TEXT("Nod"), Front);
				if (bWagLaughs)
				{
					S.Say(Wag, L4.Text, L4.Sound);
				}
			}
			// Front's vow.
			S.At(5.6f).GestureAt(Front, TEXT("Talk"), Back).Say(Front, L3.Text, L3.Sound);
			S.Wait(0.8f).GestureAt(Back, TEXT("Listen"), Front);
			// The row ahead has had enough.
			if (Shusher != INDEX_NONE)
			{
				S.At(8.2f).LookAtPoint(Shusher, Pair, 3.2f);
				S.Wait(0.3f).GestureAt(Shusher, TEXT("Refuse"), Front).Say(Shusher, L5.Text, L5.Sound);
				S.Wait(0.9f).Look(Front, Shusher, 1.6f).Look(Back, Shusher, 1.6f);
				S.Wait(0.6f).Gesture(Back, TEXT("Shrug"));
				S.At(11.0f);
			}
			else
			{
				S.At(8.6f).GestureAt(Back, TEXT("Nod"), Front);
				S.At(9.6f);
			}
			// Everyone faces the dais again.
			S.LookAtPoint(Front, Podium, 4.f).LookAtPoint(Back, Podium, 4.f);
			if (Shusher != INDEX_NONE)
			{
				S.Wait(0.25f).LookAtPoint(Shusher, Podium, 4.f);
			}
			for (int32 i = 0; i < Neighbours.Num(); ++i)
			{
				S.Wait(S.Rand(0.1f, 0.35f)).LookAtPoint(Neighbours[i], Podium, 4.f);
			}
			S.Wait(0.8f).Gesture(Front, TEXT("Nod"));
			S.Wait(2.6f);
			return true;
		}
	};
}

SETLIFE_PIECE(RivalBidders, SetLifeRivalBidders::FPiece, ESetLifeZone::Hall, 70.f, 1.f)
