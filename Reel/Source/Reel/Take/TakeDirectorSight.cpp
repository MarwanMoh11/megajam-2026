// ATakeDirector slice (d): the watchers' sight tests (step 4). See TakeDirector.h for the contract.

#include "TakeDirector.h"

#include "BeamWatcher.h"
#include "Julian.h"
#include "Watcher.h"

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
