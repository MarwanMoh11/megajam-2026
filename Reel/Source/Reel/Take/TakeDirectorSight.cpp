// ATakeDirector slice (d): the watchers' sight tests (step 4). See TakeDirector.h for the contract.

#include "TakeDirector.h"

#include "BeamWatcher.h"
#include "CollisionQueryParams.h"
#include "Device.h"
#include "Engine/World.h"
#include "Julian.h"
#include "TakeCharacter.h"
#include "Watcher.h"

namespace TakeSightW
{
	/** A watcher's SeenLine flash. */
	constexpr float SeenLineSeconds = 2.5f;
}

void ATakeDirector::StepSight(float DeltaSeconds)
{
	bJulianSeen = false;

	for (const TObjectPtr<AWatcher>& WatcherPtr : Watchers)
	{
		AWatcher* W = WatcherPtr.Get();
		if (!W)
		{
			continue;
		}

		const AJulian* JulianActor = Julian.Get();

		if (W->IsInstantBlooper())
		{
			// The beam: touching it while live is a blooper at once (the end-of-frame check fires it). Swept from
			// where his feet were before this frame's walk, so a long frame cannot carry him through.
			bool bTouches = false;
			if (JulianActor)
			{
				if (const ABeamWatcher* BeamActor = Cast<ABeamWatcher>(W))
				{
					bTouches = BeamActor->TouchesJulianSwept(JulianActor, JulianFeetBeforeWalk);
				}
				else
				{
					bTouches = W->HoldsJulian(JulianActor);
				}
			}
			if (bTouches)
			{
				if (Phase == ETakePhase::Rolling && PendingBlooper == EBlooperReason::None)
				{
					TAKE_LOG(TEXT("%s TOUCHED"), *W->GetLabel());
				}
				RequestBlooper(EBlooperReason::BeamTouched);
			}
			W->SetTally(bTouches);
			continue;
		}

		// The guard: his frame holding an exposed Julian is SEEN.
		const bool bHolds = JulianActor != nullptr && W->HoldsJulian(JulianActor);
		const bool bNowTally = bHolds && bJulianExposed;
		const bool bWasTally = W->HasTally();
		W->SetTally(bNowTally);
		if (bNowTally && !bWasTally)
		{
			TAKE_LOG(TEXT("SEEN by %s (frames Julian)"), *W->GetLabel());
		}
		else if (!bNowTally && bWasTally)
		{
			TAKE_LOG(TEXT("SEEN ends (%s lost him)"), *W->GetLabel());
		}
		if (bNowTally)
		{
			bJulianSeen = true;
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Step 2 (W)
// ---------------------------------------------------------------------------------------------------------------------

bool ATakeDirector::IsSightBlocked(const FVector& From, const FVector& To) const
{
	// A dropped banner (or any device that blocks sight) first: no trace needed.
	for (const TObjectPtr<ADevice>& DevicePtr : Devices)
	{
		const ADevice* Device = DevicePtr.Get();
		if (Device && Device->BlocksSight(From, To))
		{
			return true;
		}
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// Walls and floors: anything on the Visibility channel that is not a take actor.
	FCollisionQueryParams Params(FName(TEXT("TakeSight")), false, this);
	for (const TObjectPtr<AActor>& IgnorePtr : SightIgnore)
	{
		if (const AActor* Ignored = IgnorePtr.Get())
		{
			Params.AddIgnoredActor(Ignored);
		}
	}
	// The characters (Julian is the trace's target) and the watchers (their click volumes block Visibility) are in
	// SightIgnore already; added again so a short SightIgnore can never hide Julian behind himself.
	for (const TObjectPtr<ATakeCharacter>& CharacterPtr : Characters)
	{
		if (const ATakeCharacter* Who = CharacterPtr.Get())
		{
			Params.AddIgnoredActor(Who);
		}
	}
	for (const TObjectPtr<AWatcher>& WatcherPtr : Watchers)
	{
		if (const AWatcher* Other = WatcherPtr.Get())
		{
			Params.AddIgnoredActor(Other);
		}
	}
	return World->LineTraceTestByChannel(From, To, ECC_Visibility, Params);
}
