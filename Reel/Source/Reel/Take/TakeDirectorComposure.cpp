// ATakeDirector slice (a): exposure and composure. See TakeDirector.h for the contract.

#include "TakeDirector.h"

#include "ComposureComponent.h"
#include "Julian.h"
#include "Mark.h"
#include "RedTapeZone.h"
#include "Watcher.h"

namespace TakeComposureA
{
	const TCHAR* LabelName(EComposureLabel InLabel)
	{
		switch (InLabel)
		{
		case EComposureLabel::Seen: return TEXT("SEEN");
		case EComposureLabel::Mark: return TEXT("MARK");
		default: return TEXT("none");
		}
	}
}

bool ATakeDirector::IsJulianExposed() const
{
	if (!Julian)
	{
		return false;
	}

	const FVector Feet = Julian->GetFeetLocation();
	for (const TObjectPtr<ARedTapeZone>& Zone : TapeZones)
	{
		if (Zone && Zone->Contains(Feet))
		{
			return true;
		}
	}

	// Rule 5: off a mark inside the drone's cone (any watcher that ExposesOffMark and holds him).
	if (Julian->GetStandingMark() == nullptr)
	{
		const AJulian* JulianActor = Julian.Get();
		for (const TObjectPtr<AWatcher>& WatcherPtr : Watchers)
		{
			const AWatcher* W = WatcherPtr.Get();
			if (W && W->ExposesOffMark() && W->HoldsJulian(JulianActor))
			{
				return true;
			}
		}
	}
	return false;
}

void ATakeDirector::StepComposure(float DeltaSeconds)
{
	if (!Composure)
	{
		return;
	}

	const bool bOnMarkUnexposed = Julian && Julian->GetStandingMark() != nullptr && !bJulianExposed;
	const EComposureLabel LabelBefore = Composure->GetLabel();
	const bool bWasEmpty = Composure->IsEmpty();

	const float SeenSecondsBefore = Composure->GetSeenSeconds();
	Composure->Step(DeltaSeconds, bJulianSeen, bOnMarkUnexposed);

	// The grace has just run out: he is caught, and whoever holds him says so ("Security!"). A turning frame that
	// only sweeps across him never gets this far.
	const float Grace = Composure->SeenGraceSeconds;
	if (bJulianSeen && SeenSecondsBefore < Grace && Composure->GetSeenSeconds() >= Grace)
	{
		for (const TObjectPtr<AWatcher>& WatcherPtr : Watchers)
		{
			const AWatcher* const Holder = WatcherPtr.Get();
			if (Holder && Holder->HasTally() && !Holder->SeenLine.IsEmpty())
			{
				Flash(Holder->SeenLine, TakeColors::Red, 2.5f);
			}
		}
	}

	const EComposureLabel LabelAfter = Composure->GetLabel();
	if (LabelAfter != LabelBefore)
	{
		TAKE_LOG(TEXT("COMPOSURE %s -> %s (%.0f)"), TakeComposureA::LabelName(LabelBefore),
			TakeComposureA::LabelName(LabelAfter), Composure->GetComposure());
	}
	if (!bWasEmpty && Composure->IsEmpty())
	{
		TAKE_LOG(TEXT("COMPOSURE hits 0"));
	}
}
