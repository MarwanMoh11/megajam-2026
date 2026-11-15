// Set life piece: whispers (the lobby and the hall; the player-reactive one). About 7-8 s.
// Julian walks past a pair of standing guests 3-6 m away, on screen and ahead of him if possible. One leans toward the
// other and glances at him, then whispers ("Who's the fellow in the white jacket?"); the partner glances too, answers
// and shrugs or laughs; they turn back to each other. Hitman's "Do I know him?", the passers-by of Sleeping Dogs.
// Purely flavour: it reads the room watch only to stay out of its way (never in house light at the front floor or the
// dais, nor within 8 m of the front row mark), and feeds nothing (no watcher, sighting, composure or room-watch call).
// Registered twice (one zone per piece in the framework): WhispersLobby and WhispersHall, sharing one global cooldown
// (25-35 s), a per-guest cooldown (60 s) and the last line pair (never the same pair twice in a row).
#include "SetLife/SetLife.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "RoomWatch.h"
#include "TakeCharacter.h"
#include "TakeDirector.h"
#include "TakeTypes.h"

namespace SetLifeWhispers
{
	// The heist's front: THE FRONT ROW mark (3100,-60); nothing within 8 m of it, nor at or beyond its x (the front
	// floor and the dais), where guests glancing at Julian is the room watch's signal.
	const FVector FrontRow(3100.f, -60.f, 0.f);
	constexpr float FrontKeepOut = 800.f;

	constexpr float MinDist = 300.f;		// pairs 3-6 m from Julian
	constexpr float MaxDist = 600.f;
	constexpr float PartnerRadius = 220.f;	// the partner stands this close to the whisperer
	constexpr float MinJulianSpeed = 60.f;	// cm/s: he is walking past, not standing about
	constexpr float GlobalCooldownMin = 25.f;
	constexpr float GlobalCooldownMax = 35.f;
	constexpr float GuestCooldown = 60.f;	// a guest who whispered (or was whispered to) rests this long

	struct FPair
	{
		const TCHAR* Ask;
		const TCHAR* AskSound;
		const TCHAR* Answer;
		const TCHAR* AnswerSound;
		const TCHAR* Reaction;	// the partner's gesture with the answer
		const TCHAR* Coda;		// the whisperer's gesture after it
	};
	const FPair Pairs[] = {
		{ TEXT("Who's the fellow in the white jacket?"), TEXT("sl_whisper_who_1"),
		  TEXT("Never seen him. New money, I'd wager."), TEXT("sl_whisper_who_2"), TEXT("Shrug"), TEXT("Nod") },
		{ TEXT("Isn't that the Ashford boy?"), TEXT("sl_whisper_ashford_1"),
		  TEXT("I heard he lost everything."), TEXT("sl_whisper_ashford_2"), TEXT("Shrug"), TEXT("Nod") },
		{ TEXT("A white jacket, after dark? How daring."), TEXT("sl_whisper_jacket_1"),
		  TEXT("Darling, don't stare."), TEXT("sl_whisper_jacket_2"), TEXT("Laugh"), TEXT("Laugh") },
		{ TEXT("Is he one of the bidders, do you think?"), TEXT("sl_whisper_bidder_1"),
		  TEXT("In that jacket? I rather doubt it."), TEXT("sl_whisper_bidder_2"), TEXT("Laugh"), TEXT("Nod") },
	};
	constexpr int32 NumPairs = static_cast<int32>(UE_ARRAY_COUNT(Pairs));

	/** Shared by both registrations, reset with the world (a take restart reloads the level). */
	struct FShared
	{
		TWeakObjectPtr<UWorld> World;
		float NextAt = 0.f;			// world time before which no whisper starts (either zone)
		int32 LastPair = INDEX_NONE;
		TMap<TWeakObjectPtr<const ACastMember>, float> RestUntil;
		TWeakObjectPtr<ARoomWatch> Room;
		bool bRoomLooked = false;
	};

	FShared& Shared(UWorld* World)
	{
		static FShared S;
		if (S.World.Get() != World)
		{
			S = FShared();
			S.World = World;
		}
		return S;
	}

	ARoomWatch* FindRoom(FShared& Sh, UWorld* World)
	{
		if (!Sh.bRoomLooked && World)
		{
			Sh.bRoomLooked = true;
			for (TActorIterator<ARoomWatch> It(World); It; ++It)
			{
				Sh.Room = *It;
				break;
			}
		}
		return Sh.Room.Get();
	}

	bool InFrontKeepOut(const FVector& P)
	{
		return P.X >= FrontRow.X || FVector::Dist2D(P, FrontRow) < FrontKeepOut;
	}

	/** The room watch's moment: Julian at the front (or about to be), or the room already noticing him. */
	bool IsRoomWatchMoment(FShared& Sh, UWorld* World, const FVector& JulianFeet)
	{
		if (InFrontKeepOut(JulianFeet))
		{
			return true;
		}
		const ARoomWatch* Room = FindRoom(Sh, World);
		return Room && (Room->IsAtFront(JulianFeet) || Room->IsMurmuring() || Room->GetExposure() > 0.f);
	}

	FVector JulianVelocity(UWorld* World)
	{
		const ATakeDirector* Director = World ? ATakeDirector::Get(World) : nullptr;
		const ATakeCharacter* Julian = Director ? Director->GetCharacter(ETakeCamera::A) : nullptr;
		return Julian ? Julian->GetVelocity() : FVector::ZeroVector;
	}

	FVector JulianHead(const FSetLifeStage& S)
	{
		return S.GetJulian() + FVector(0.f, 0.f, 165.f);
	}

	class FPiece : public FSetLifePiece
	{
	public:
		virtual bool Setup(FSetLifeStage& S) override
		{
			UWorld* World = S.GetWorld();
			if (!World)
			{
				return false;
			}
			FShared& Sh = Shared(World);
			const float Now = World->GetTimeSeconds();
			if (Now < Sh.NextAt)
			{
				return false;
			}
			const FVector J = S.GetJulian();
			const FVector V = JulianVelocity(World);
			if (J.IsZero() || V.Size2D() < MinJulianSpeed || IsRoomWatchMoment(Sh, World, J))
			{
				return false;
			}
			const FVector Heading = V.GetSafeNormal2D();

			// --- casting: a standing guest 3-6 m from Julian, on screen, with another standing guest close by ---
			FShared* ShP = &Sh;
			auto Usable = [ShP, J, Now](const ACastMember& M)
			{
				const FVector P = M.GetActorLocation();
				const float D = FVector::Dist2D(P, J);
				if (D < MinDist || D > MaxDist || FMath::Abs(P.Z - J.Z) > 250.f || InFrontKeepOut(P))
				{
					return false;
				}
				const float* Rest = ShP->RestUntil.Find(TWeakObjectPtr<const ACastMember>(&M));
				return !Rest || Now >= *Rest;
			};
			FSetLifeRole WantA = FSetLifeRole().Guests().OnScreen().NearPoint(J, MaxDist + 50.f).Where(Usable);
			WantA.MinFromJulian = MinDist;
			TArray<ACastMember*> Firsts = S.FindFree(WantA, 12);
			// Ahead of his heading first.
			Firsts.StableSort([J, Heading](const ACastMember& A, const ACastMember& B)
			{
				return FVector::DotProduct((A.GetActorLocation() - J).GetSafeNormal2D(), Heading)
					> FVector::DotProduct((B.GetActorLocation() - J).GetSafeNormal2D(), Heading);
			});
			for (ACastMember* G : Firsts)
			{
				if (FVector::DotProduct((G->GetActorLocation() - J).GetSafeNormal2D(), Heading) < -0.3f)
				{
					break;	// the rest are behind him: he has already passed them
				}
				const FVector GP = G->GetActorLocation();
				FSetLifeRole WantB = FSetLifeRole().Guests().AnyDistance().NearPoint(GP, PartnerRadius)
					.Where([G, Usable](const ACastMember& M) { return &M != G && Usable(M); });
				WantB.MinFromJulian = MinDist;
				const TArray<ACastMember*> Near = S.FindFree(WantB, 1);
				if (Near.Num() > 0)
				{
					Whisperer = S.CastMember(TEXT("Whisperer"), G);
					Partner = S.CastMember(TEXT("Partner"), Near[0]);
					break;
				}
			}
			if (Whisperer == INDEX_NONE || Partner == INDEX_NONE)
			{
				return false;	// the stage releases whoever was cast
			}

			// --- the lines: never the same pair twice in a row ---
			int32 Index = S.RandInt(0, NumPairs - 1);
			if (Index == Sh.LastPair)
			{
				Index = (Index + S.RandInt(1, NumPairs - 1)) % NumPairs;
			}
			Sh.LastPair = Index;
			const FPair& L = Pairs[Index];

			// Block the other zone's registration while this one runs; End sets the real cooldown.
			Sh.NextAt = Now + 60.f;
			const float RestTo = Now + GuestCooldown;
			Sh.RestUntil.Add(TWeakObjectPtr<const ACastMember>(S.Get(Whisperer)), RestTo);
			Sh.RestUntil.Add(TWeakObjectPtr<const ACastMember>(S.Get(Partner)), RestTo);

			const FVector PA = S.Where(Whisperer);
			const FVector PB = S.Where(Partner);
			FVector ToB = (PB - PA).GetSafeNormal2D();
			if (ToB.IsNearlyZero())
			{
				ToB = FVector(1.f, 0.f, 0.f);
			}
			// A step closer, but never into the partner.
			const float Lean = FMath::Clamp(FVector::Dist2D(PA, PB) - 75.f, 0.f, 22.f);
			S.SetAnchor((PA + PB) * 0.5f);

			const int32 A = Whisperer;
			const int32 B = Partner;
			// Julian moves: his head is read when the beat runs (the cast member's own head-turn, nothing else).
			auto Glance = [](int32 Role, float Seconds)
			{
				return [Role, Seconds](FSetLifeStage& St)
				{
					if (ACastMember* M = St.Get(Role))
					{
						M->LookAtPoint(JulianHead(St), Seconds);
					}
				};
			};

			// --- the beat sheet ---
			// He notices: a glance at Julian, a lean toward the partner (who listens).
			S.FaceTo(A, B).FaceTo(B, A).Do(Glance(A, 1.0f)).GestureAt(B, TEXT("Listen"), A);
			S.Wait(0.15f).Shift(A, ToB * Lean, 0.4f);
			// The whisper.
			S.Wait(0.85f).GestureAt(A, TEXT("Talk"), B).Say(A, L.Ask, L.AskSound);
			// The partner glances too, then answers with a shrug or a laugh.
			S.Wait(1.7f).Do(Glance(B, 0.9f));
			S.Wait(0.9f).GestureAt(B, FName(L.Reaction), A).Say(B, L.Answer, L.AnswerSound);
			// Back to each other.
			S.Wait(2.2f).FaceTo(A, B).FaceTo(B, A).GestureAt(A, FName(L.Coda), B);
			S.Wait(1.2f).GestureAt(B, TEXT("Talk"), A);
			S.Wait(1.0f);
			return true;
		}

		virtual void Tick(FSetLifeStage& S, float DeltaSeconds) override
		{
			// Julian stepped into the room watch's moment: guests glancing at him is the room's signal there.
			UWorld* World = S.GetWorld();
			if (World && IsRoomWatchMoment(Shared(World), World, S.GetJulian()))
			{
				S.Abort(TEXT("room watch"));
			}
		}

		virtual void End(FSetLifeStage& S, bool bAborted) override
		{
			if (UWorld* World = S.GetWorld())
			{
				Shared(World).NextAt = World->GetTimeSeconds() + S.Rand(GlobalCooldownMin, GlobalCooldownMax);
			}
		}

	private:
		int32 Whisperer = INDEX_NONE;
		int32 Partner = INDEX_NONE;
	};
}

SETLIFE_PIECE(WhispersLobby, SetLifeWhispers::FPiece, ESetLifeZone::Lobby, 12.f, 1.f)
SETLIFE_PIECE(WhispersHall, SetLifeWhispers::FPiece, ESetLifeZone::Hall, 12.f, 1.f)
