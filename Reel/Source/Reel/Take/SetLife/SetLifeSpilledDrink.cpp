// Set life piece: the spilled drink (the hall, by the bar and the cocktail tables). About 15-25 s.
// The bartender carries a tray out round the end of the bar. A gentleman talking with a partner steps back into
// him, a glass goes over (it lies on the floor in a spreading claret pool), the gentleman turns and apologises, the
// partner laughs, the waiter nods it off and goes back to the bar, the pair talk on.
// The worked example in docs/SET_LIFE.md: copy this file to write a new piece.
#include "SetLife/SetLife.h"

#include "Components/StaticMeshComponent.h"

namespace SetLifeSpilledDrink
{
	// The bar (tools/sets/hall.py bar(): x 800..1600, y -1560..-1470, the customers' side is +Y; build_cast.py stands
	// the bartender behind it at (1200, -1700)).
	const FVector BarFront(1200.f, -1300.f, 0.f);
	constexpr float BarX0 = 800.f;
	constexpr float BarX1 = 1600.f;
	constexpr float BarBackY = -1540.f;		// behind the bar: y below this, x between the ends
	constexpr float BarEndBackY = -1660.f;	// round the end: from behind...
	constexpr float BarEndOutY = -1390.f;	// ...to the customers' side

	const FLinearColor Silver(0.55f, 0.54f, 0.50f);
	const FLinearColor GlassTone(0.80f, 0.76f, 0.62f);
	const FLinearColor Claret(0.16f, 0.015f, 0.03f);

	struct FLine
	{
		const TCHAR* Text;
		const TCHAR* Sound;
	};
	const FLine Sorry[] = {
		{ TEXT("Oh! Terribly sorry, old boy."), TEXT("sl_spill_sorry_1") },
		{ TEXT("I say! Frightfully sorry."), TEXT("sl_spill_sorry_2") },
	};
	const FLine Tease[] = {
		{ TEXT("Steady on, Reggie!"), TEXT("sl_spill_tease_1") },
		{ TEXT("Ha! Two left feet, as ever."), TEXT("sl_spill_tease_2") },
	};
	const FLine Waive[] = {
		{ TEXT("Quite all right, sir."), TEXT("sl_spill_waiter_1") },
		{ TEXT("No harm done, sir. I'll fetch another."), TEXT("sl_spill_waiter_2") },
	};

	class FPiece : public FSetLifePiece
	{
	public:
		virtual bool Setup(FSetLifeStage& S) override
		{
			// --- casting: a gentleman with someone close by (a conversation) near the bar, and the bartender ---
			int32 Guest = INDEX_NONE;
			int32 Partner = INDEX_NONE;
			for (ACastMember* G : S.FindFree(FSetLifeRole().Men().NearPoint(BarFront, 1300.f), 10))
			{
				const TArray<ACastMember*> Near = S.FindFree(FSetLifeRole().Guests().AnyDistance()
					.NearPoint(G->GetActorLocation(), 260.f)
					.Where([G](const ACastMember& M) { return &M != G; }), 1);
				if (Near.Num() > 0)
				{
					Guest = S.CastMember(TEXT("Guest"), G);
					Partner = S.CastMember(TEXT("Partner"), Near[0]);
					break;
				}
			}
			if (Guest == INDEX_NONE || Partner == INDEX_NONE)
			{
				return false;	// the stage releases whoever was cast
			}
			Waiter = S.Cast(TEXT("Waiter"), FSetLifeRole().Staff().AnyDistance().NearPoint(BarFront, 1600.f));
			if (Waiter == INDEX_NONE)
			{
				return false;
			}

			// --- the geometry: the waiter passes just behind the guest's back ---
			const FVector PG = S.Where(Guest);
			const FVector PP = S.Where(Partner);
			const FVector PW = S.Where(Waiter);
			FVector Away = (PG - PP).GetSafeNormal2D();
			if (Away.IsNearlyZero())
			{
				Away = FVector(1.f, 0.f, 0.f);
			}
			const FVector Side(-Away.Y, Away.X, 0.f);
			Bump = PG + Away * 70.f;
			TArray<FVector> Way;
			if (PW.Y < BarBackY && PW.X > BarX0 - 20.f && PW.X < BarX1 + 20.f)
			{
				// Behind the bar: out round the end nearer the guest (and back the same way at the end).
				const float EndX = FMath::Abs(Bump.X - BarX0) < FMath::Abs(Bump.X - BarX1) ? BarX0 - 70.f : BarX1 + 70.f;
				Way.Add(FVector(EndX, BarEndBackY, PW.Z));
				Way.Add(FVector(EndX, BarEndOutY, PW.Z));
				S.SetReturnVia(Waiter, { Way[0] });
			}
			const FVector Out = Way.Num() > 0 ? Way.Last() : PW;
			const FVector A1 = Bump + Side * 240.f;
			const FVector A2 = Bump - Side * 240.f;
			const FVector Approach = FVector::Dist2D(A1, Out) < FVector::Dist2D(A2, Out) ? A1 : A2;
			SpillAt = S.FloorAt(PG + Away * 55.f + (Approach - Bump).GetSafeNormal2D() * 15.f);
			S.SetAnchor(PG);

			// --- props: a tray with two coupes, carried in the right hand ---
			S.HoldProp(Waiter, ETakeShape::Cylinder, FVector(38.f, 38.f, 1.5f), Silver, FVector(10.f, 0.f, 3.f));
			Glass = S.HoldProp(Waiter, ETakeShape::Cylinder, FVector(6.f, 6.f, 11.f), GlassTone, FVector(15.f, -7.f, 9.5f));
			S.HoldProp(Waiter, ETakeShape::Cylinder, FVector(6.f, 6.f, 11.f), GlassTone, FVector(5.f, 8.f, 9.5f));

			const FLine& L1 = Sorry[S.RandInt(0, static_cast<int32>(UE_ARRAY_COUNT(Sorry)) - 1)];
			const FLine& L2 = Tease[S.RandInt(0, static_cast<int32>(UE_ARRAY_COUNT(Tease)) - 1)];
			const FLine& L3 = Waive[S.RandInt(0, static_cast<int32>(UE_ARRAY_COUNT(Waive)) - 1)];
			const FVector BumpAt = Bump;
			const int32 W = Waiter;

			// --- the beat sheet ---
			S.FaceTo(Guest, Partner).FaceTo(Partner, Guest).GestureAt(Guest, TEXT("Talk"), Partner);
			for (const FVector& P : Way)
			{
				S.Walk(Waiter, P, 130.f).WaitArrived(Waiter, 12.f);
			}
			S.Walk(Waiter, Approach, 130.f).WaitArrived(Waiter, 14.f);
			S.Walk(Waiter, Bump, 100.f).GestureAt(Guest, TEXT("Talk"), Partner);
			S.WaitUntil([W, BumpAt](FSetLifeStage& St) { return FVector::Dist2D(St.Where(W), BumpAt) < 90.f; }, 6.f);
			// The step back, and the glass goes over.
			S.Shift(Guest, Away * 32.f, 0.3f).Wait(0.28f);
			S.Do([this](FSetLifeStage& St) { Spill(St); });
			S.LookAtPoint(Waiter, SpillAt, 1.6f).LookAtPoint(Partner, SpillAt, 1.2f);
			S.Wait(0.35f).FaceTo(Guest, Waiter);
			S.Wait(0.45f).Say(Guest, L1.Text, L1.Sound).Gesture(Guest, TEXT("Shrug"));
			S.Wait(0.7f).GestureAt(Partner, TEXT("Laugh"), Guest).Say(Partner, L2.Text, L2.Sound);
			S.Wait(1.4f).FaceTo(Waiter, Guest).Gesture(Waiter, TEXT("Nod")).Say(Waiter, L3.Text, L3.Sound);
			// Back to the bar; the pair talk on.
			S.Wait(2.2f).FaceTo(Guest, Partner).GestureAt(Guest, TEXT("Talk"), Partner);
			S.Walk(Waiter, Out, 120.f);
			S.Wait(0.8f).GestureAt(Partner, TEXT("Nod"), Guest);
			S.WaitArrived(Waiter, 14.f);
			return true;
		}

		virtual void Tick(FSetLifeStage& S, float DeltaSeconds) override
		{
			// The pool spreads over 0.6 s.
			if (UStaticMeshComponent* Pool = S.GetProp(PoolProp))
			{
				const float T = FMath::Clamp((S.GetTime() - SpilledAt) / 0.6f, 0.f, 1.f);
				const float E = 1.f - FMath::Square(1.f - T);
				Pool->SetWorldScale3D(FVector(FMath::Lerp(0.12f, 0.62f, E), FMath::Lerp(0.12f, 0.44f, E), 0.006f));
			}
		}

	private:
		void Spill(FSetLifeStage& S)
		{
			S.DropProp(Glass, SpillAt + FVector(0.f, 0.f, 3.f), FRotator(90.f, S.Rand(0.f, 360.f), 0.f));
			PoolProp = S.AddProp(ETakeShape::Cylinder, SpillAt + FVector(0.f, 0.f, 0.4f), FVector(12.f, 12.f, 0.6f), Claret,
				FRotator(0.f, S.Rand(0.f, 360.f), 0.f));
			SpilledAt = S.GetTime();
		}

		int32 Waiter = INDEX_NONE;
		int32 Glass = INDEX_NONE;
		int32 PoolProp = INDEX_NONE;
		float SpilledAt = 0.f;
		FVector Bump = FVector::ZeroVector;
		FVector SpillAt = FVector::ZeroVector;
	};
}

SETLIFE_PIECE(SpilledDrink, SetLifeSpilledDrink::FPiece, ESetLifeZone::Hall, 55.f, 1.f)
