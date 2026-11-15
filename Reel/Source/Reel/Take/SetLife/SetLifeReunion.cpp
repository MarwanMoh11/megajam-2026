// Set life piece: the reunion (the lobby's south side, ground floor). About 18-24 s.
// Two guests who have not met in years. One (A) stands toward the hall end; the other (B) moves in from the street end,
// stops and looks about. A glances at B, looks away, looks again (the double take), turns and waves; B waves back.
// They walk to each other and meet in the middle: hellos, a laugh, a line or two, then they stroll off together toward
// the great hall's doors, heads turned to each other, stop short of them and talk on. The stage walks them home.
// Kept to the south strip (y -1300..-350 on the ground floor): Julian's way in, the desk, the stair's foot, the queue's
// inside spots and the doorman are all on the north side or at y -300..330 (tools/sets/front.py ROUTES / KEEP).
#include "SetLife/SetLife.h"

namespace SetLifeReunion
{
	// tools/sets/front.py / build_auction.py: the lobby is x -3200..-1000, y -1400..1400, the ground floor at z 0 and the
	// balcony at z 450; the hall's door is the opening in the west wall at x -1000, y -350..350.
	constexpr float StripY0 = -1300.f;		// members cast between these (south of the stair, Julian's way and the desk)
	constexpr float StripY1 = -350.f;
	constexpr float GroundZMax = 250.f;		// the balcony guests (z 450) are lobby too: not them
	constexpr float MeetX0 = -2700.f;		// the meeting point is kept in here
	constexpr float MeetX1 = -1600.f;
	constexpr float MeetY0 = -1150.f;
	constexpr float MeetY1 = -600.f;
	constexpr float DestXMax = -1400.f;		// where they stroll to: no closer to the hall door than this...
	constexpr float DestYMax = -620.f;		// ...and kept 4 m off Julian's diagonal from the desk to the door
	const FVector HallDoor(-1000.f, -250.f, 0.f);
	// The cocktail tables on the south side (build_auction.py mingle()): walks go round them, not through.
	const FVector2D Tables[] = { FVector2D(-1300.f, -1000.f), FVector2D(-2950.f, -1000.f) };
	constexpr float TableClear = 80.f;		// a walk passing closer than this to a table's centre detours...
	constexpr float TableDetour = 125.f;	// ...through a point this far out from it

	struct FLine
	{
		const TCHAR* Text;
		const TCHAR* Sound;
	};
	const FLine Spot[] = {	// A, the double take (names that suit a man or a woman)
		{ TEXT("Kit? Kit Harlow, as I live and breathe!"), TEXT("sl_reunion_spot_1") },
		{ TEXT("Well, I'll be! Sandy Bell!"), TEXT("sl_reunion_spot_2") },
	};
	const FLine Reply[] = {	// B, at the meeting
		{ TEXT("Bunny! They let you out of Monte Carlo?"), TEXT("sl_reunion_reply_1") },
		{ TEXT("Good heavens! Not since before the war!"), TEXT("sl_reunion_reply_2") },
	};
	const FLine Chat[] = {	// A, laughing
		{ TEXT("You look a million dollars!"), TEXT("sl_reunion_chat_1") },
		{ TEXT("You still owe me twenty from Saratoga!"), TEXT("sl_reunion_chat_2") },
	};
	const FLine Drink[] = {	// B, pointing them toward the hall
		{ TEXT("Come on, let's find a drink."), TEXT("sl_reunion_drink_1") },
		{ TEXT("Inside. You can tell me everything."), TEXT("sl_reunion_drink_2") },
	};

	template <SIZE_T N>
	const FLine& PickLine(FSetLifeStage& S, const FLine (&Lines)[N])
	{
		return Lines[S.RandInt(0, static_cast<int32>(N) - 1)];
	}

	bool OnStrip(const FVector& P)
	{
		return P.Z < GroundZMax && P.Y > StripY0 && P.Y < StripY1;
	}

	/** The way from P to Q: Q, with a point round any cocktail table the straight line would cross first. */
	TArray<FVector> Route(const FVector& P, const FVector& Q)
	{
		TArray<FVector> Way;
		const FVector2D A(P), B(Q);
		for (const FVector2D& T : Tables)
		{
			const FVector2D AB = B - A;
			const float Len2 = AB.SizeSquared();
			if (Len2 < 1.f)
			{
				continue;
			}
			const float K = FMath::Clamp(FVector2D::DotProduct(T - A, AB) / Len2, 0.f, 1.f);
			if (K <= 0.02f || K >= 0.98f)
			{
				continue;	// the table is behind the start or past the end (a guest starting at its own table)
			}
			const FVector2D Closest = A + AB * K;
			if (FVector2D::Distance(Closest, T) < TableClear)
			{
				FVector2D Out = (Closest - T).GetSafeNormal();
				if (Out.IsNearlyZero())
				{
					Out = FVector2D(-AB.Y, AB.X).GetSafeNormal();
				}
				const FVector2D D = T + Out * TableDetour;
				Way.Add(FVector(D.X, D.Y, P.Z));
				break;	// one detour is enough on a strip this size
			}
		}
		Way.Add(FVector(Q.X, Q.Y, P.Z));
		return Way;
	}

	class FPiece : public FSetLifePiece
	{
	public:
		virtual bool Setup(FSetLifeStage& S) override
		{
			// --- casting: A toward the hall end of the south strip, B 5.5-19 m further toward the street ---
			int32 A = INDEX_NONE;
			int32 B = INDEX_NONE;
			const TArray<ACastMember*> As = S.FindFree(FSetLifeRole().Guests()
				.Where([](const ACastMember& M) { const FVector P = M.GetActorLocation(); return OnStrip(P) && P.X > -2300.f; }), 6);
			for (ACastMember* Cand : As)
			{
				const FVector PA = Cand->GetActorLocation();
				const TArray<ACastMember*> Bs = S.FindFree(FSetLifeRole().Guests()
					.Where([PA](const ACastMember& M)
					{
						const FVector P = M.GetActorLocation();
						const float D = FVector::Dist2D(P, PA);
						return OnStrip(P) && P.X < PA.X - 500.f && D > 550.f && D < 1900.f;
					}), 1);
				if (Bs.Num() > 0)
				{
					A = S.CastMember(TEXT("Spotter"), Cand);
					B = S.CastMember(TEXT("OldFriend"), Bs[0]);
					break;
				}
			}
			if (A == INDEX_NONE || B == INDEX_NONE)
			{
				return false;	// too few free guests on the lobby's south side: the stage releases anyone cast
			}

			// --- the geometry ---
			const FVector HA = S.Where(A);
			const FVector HB = S.Where(B);
			FVector M = (HA + HB) * 0.5f;
			M.X = FMath::Clamp(M.X, MeetX0, MeetX1);
			M.Y = FMath::Clamp(M.Y, MeetY0, MeetY1);
			FVector Axis = (HA - HB).GetSafeNormal2D();
			if (Axis.IsNearlyZero())
			{
				Axis = FVector(1.f, 0.f, 0.f);
			}
			const FVector MA = FVector(M.X, M.Y, HA.Z) + Axis * 55.f;	// they meet 1.1 m apart, face to face
			const FVector MB = FVector(M.X, M.Y, HB.Z) - Axis * 55.f;
			// B's first steps in: a couple of metres toward the middle, then a look about.
			const FVector Pause = HB + (MB - HB).GetSafeNormal2D() * FMath::Min(220.f, 0.35f * FVector::Dist2D(HB, MB));
			// The stroll: 3.8 m from the meeting toward the hall's door, held short of it and off Julian's way.
			const FVector Dir = (HallDoor - M).GetSafeNormal2D();
			FVector Dest = M + Dir * 380.f;
			Dest.X = FMath::Min(Dest.X, DestXMax);
			Dest.Y = FMath::Clamp(Dest.Y, MeetY0, DestYMax);
			const FVector Side(-Dir.Y, Dir.X, 0.f);
			const bool bAOnPlus = FVector::Dist2D(MA, Dest + Side * 45.f) <= FVector::Dist2D(MB, Dest + Side * 45.f);
			const FVector DA = Dest + Side * (bAOnPlus ? 45.f : -45.f);
			const FVector DB = Dest - Side * (bAOnPlus ? 45.f : -45.f);
			const FVector Doorward = Dest + Dir * 300.f;
			S.SetAnchor(M);

			// The way home from the stroll's end (round the tables).
			TArray<FVector> HomeA = Route(DA, HA);
			HomeA.Pop();
			TArray<FVector> HomeB = Route(DB, HB);
			HomeB.Pop();
			if (HomeA.Num() > 0)
			{
				S.SetReturnVia(A, HomeA);
			}
			if (HomeB.Num() > 0)
			{
				S.SetReturnVia(B, HomeB);
			}

			const FLine& L1 = PickLine(S, Spot);
			const FLine& L2 = PickLine(S, Reply);
			const FLine& L3 = PickLine(S, Chat);
			const FLine& L4 = PickLine(S, Drink);

			// --- the beat sheet ---
			// B comes in a few steps from the street end and looks about; A is idling.
			S.Gesture(A, TEXT("Listen"));
			WalkRoute(S, B, HB, Pause, 95.f);
			S.Gesture(B, TEXT("LookAround"));
			// The double take: a glance, away, and back.
			S.Wait(0.4f).Look(A, B, 0.7f);
			S.Wait(0.9f).Gesture(A, TEXT("LookAround"));
			S.Wait(0.9f).FaceTo(A, B).Look(A, B, 3.f);
			S.Wait(0.35f).GestureAt(A, TEXT("Wave"), B).Say(A, L1.Text, L1.Sound);
			// B turns, sees, waves back.
			S.Wait(0.8f).FaceTo(B, A).Look(B, A, 2.5f);
			S.Wait(0.4f).GestureAt(B, TEXT("Wave"), A);
			S.WaitGesture(B, 2.5f).WaitGesture(A, 1.f);
			// They walk to each other.
			{
				const TArray<FVector> WA = Route(HA, MA);
				const TArray<FVector> WB = Route(Pause, MB);
				S.Walk(A, WA[0], 115.f).Walk(B, WB[0], 115.f);
				S.WaitArrived(A, 9.f).WaitArrived(B, 9.f);
				if (WA.Num() > 1 || WB.Num() > 1)
				{
					S.Walk(A, WA.Last(), 115.f).Walk(B, WB.Last(), 115.f);
					S.WaitArrived(A, 6.f).WaitArrived(B, 6.f);
				}
			}
			// The greeting.
			S.FaceTo(A, B).FaceTo(B, A);
			S.Wait(0.2f).GestureAt(A, TEXT("Hello"), B).GestureAt(B, TEXT("Hello"), A);
			S.Wait(0.5f).Say(B, L2.Text, L2.Sound);
			S.Wait(1.8f).GestureAt(A, TEXT("Laugh"), B).Say(A, L3.Text, L3.Sound);
			S.Wait(0.6f).GestureAt(B, TEXT("Listen"), A);
			S.Wait(1.6f).GestureAt(B, TEXT("Laugh"), A);
			S.Wait(1.2f).GestureAtPoint(B, TEXT("Point"), Doorward).Say(B, L4.Text, L4.Sound);
			S.WaitGesture(B, 2.f);
			// Off together toward the hall, heads turned to each other.
			S.Walk(A, DA, 90.f).Walk(B, DB, 90.f);
			S.Wait(1.2f).Look(A, B, 2.2f);
			S.Wait(1.6f).Look(B, A, 2.f);
			S.WaitArrived(A, 8.f).WaitArrived(B, 8.f);
			// They stop short of the doors and talk on a moment.
			S.FaceTo(A, B).FaceTo(B, A);
			S.Wait(0.3f).GestureAt(A, TEXT("Talk"), B);
			S.Wait(2.2f).GestureAt(B, TEXT("Laugh"), A);
			S.Wait(1.6f).GestureAt(A, TEXT("Nod"), B);
			S.Wait(0.8f);
			return true;
		}

	private:
		/** Walks a role from P to Q round the tables, holding the line until it arrives. */
		static void WalkRoute(FSetLifeStage& S, int32 Role, const FVector& P, const FVector& Q, float Speed)
		{
			for (const FVector& W : Route(P, Q))
			{
				S.Walk(Role, W, Speed).WaitArrived(Role, 6.f);
			}
		}
	};
}

SETLIFE_PIECE(Reunion, SetLifeReunion::FPiece, ESetLifeZone::Lobby, 70.f, 1.f)
