// ATakeDirector slice (c): the scene (step 6). See TakeDirector.h for the contract.

#include "TakeDirector.h"

#include "Julian.h"
#include "Mark.h"
#include "TakeGameInstance.h"

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

	// A checkpoint mark Julian has come within reach of (walking counts): the next blooper starts there.
	UTakeGameInstance* const GI = GetTakeGameInstance();
	if (Phase == ETakePhase::Rolling && GI)
	{
		const FVector Feet = Julian->GetFeetLocation();
		for (const TObjectPtr<AMark>& MarkPtr : Marks)
		{
			const AMark* const EachMark = MarkPtr.Get();
			if (!EachMark || !EachMark->bCheckpoint || GI->CheckpointMark == EachMark->MarkName)
			{
				continue;
			}
			const FVector Point = EachMark->GetStandPoint();
			if (FMath::Abs(Point.Z - Feet.Z) <= EachMark->CheckpointHeight
				&& FVector::Dist2D(Point, Feet) <= EachMark->CheckpointRadius)
			{
				GI->CheckpointMark = EachMark->MarkName;
				GI->CheckpointName = EachMark->MarkName.ToUpper();
				Flash(FString::Printf(TEXT("CHECKPOINT - %s"), *GI->CheckpointName), TakeColors::GoldBright, 2.f);
				TAKE_LOG(TEXT("CHECKPOINT saved at %s"), *EachMark->MarkName);
			}
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
