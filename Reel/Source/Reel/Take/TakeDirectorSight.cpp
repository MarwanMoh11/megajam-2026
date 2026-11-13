// ATakeDirector slice (d): the watchers' sight tests (step 4). See TakeDirector.h for the contract.

#include "TakeDirector.h"

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
		const bool bHolds = JulianActor != nullptr && W->HoldsJulian(JulianActor);

		if (W->IsInstantBlooper())
		{
			// The beam: touching it while live is a blooper at once (the end-of-frame check fires it).
			if (bHolds)
			{
				if (Phase == ETakePhase::Rolling && PendingBlooper == EBlooperReason::None)
				{
					TAKE_LOG(TEXT("%s TOUCHED"), *W->GetLabel());
				}
				RequestBlooper(EBlooperReason::BeamTouched);
			}
			W->SetTally(bHolds);
			continue;
		}

		// The guard: his frame holding an exposed Julian is SEEN.
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
