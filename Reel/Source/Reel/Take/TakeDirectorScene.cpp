// ATakeDirector slice (c): the scene (step 6). See TakeDirector.h for the contract.

#include "TakeDirector.h"

#include "Julian.h"
#include "Mark.h"

void ATakeDirector::StepScene(float /*DeltaSeconds*/)
{
	if (!Julian)
	{
		return;
	}

	// Marks' occupied state follows where Julian stands (AJulian sets it on arrival and on a cue; this keeps it true).
	AMark* const Standing = Julian->GetStandingMark();
	for (const TObjectPtr<AMark>& MarkPtr : Marks)
	{
		AMark* const EachMark = MarkPtr.Get();
		if (!EachMark)
		{
			continue;
		}
		const bool bShouldBeOccupied = (EachMark == Standing);
		if (EachMark->IsOccupied() != bShouldBeOccupied)
		{
			EachMark->SetOccupied(bShouldBeOccupied);
		}
	}

	// The goal mark reached: the take is cleared (the HUD's CLEAR stamp shows from this frame).
	if (Phase == ETakePhase::Rolling && Standing && Standing->bGoal)
	{
		SetPhase(ETakePhase::Cleared);
		Flash(TEXT("CLEAR - that's the one"), TakeColors::GoldBright, 3.f);
		TAKE_LOG(TEXT("CLEARED on %s"), *Standing->MarkName);
	}
}
