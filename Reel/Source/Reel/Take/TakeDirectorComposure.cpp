// ATakeDirector slice (a): exposure and composure. See TakeDirector.h for the contract.

#include "TakeDirector.h"

#include "ComposureComponent.h"
#include "Julian.h"
#include "Mark.h"
#include "RedTapeZone.h"

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

	Composure->Step(DeltaSeconds, bJulianSeen, bOnMarkUnexposed);

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
