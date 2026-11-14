// Owner: (m). Constructor by the header pass; every "// BODY: (m)" is a stub to fill.

#include "Julian.h"

#include "EngineUtils.h"
#include "Mark.h"
#include "TakeDirector.h"
#include "TakeTypes.h"

AJulian::AJulian()
{
	Id = ETakeCamera::A;
	Tint = FLinearColor::White;
}

void AJulian::BeginPlay()
{
	Super::BeginPlay();

	// He starts the take standing on a mark if one is under his feet (so MARK shows from the first frame). The
	// director may not have gathered its marks yet, so look in the world; the nearest within tolerance wins.
	StandingMark = nullptr;
	const FVector Feet = GetFeetLocation();
	double BestDist = StartMarkTolerance;
	for (TActorIterator<AMark> It(GetWorld()); It; ++It)
	{
		const double Dist = FVector::Dist2D(It->GetStandPoint(), Feet);
		if (Dist <= BestDist)
		{
			BestDist = Dist;
			StandingMark = *It;
		}
	}

	if (StandingMark)
	{
		StandingMark->SetOccupied(true);
		TAKE_LOG(TEXT("ON MARK %s (start)"), *StandingMark->MarkName);
	}
}

void AJulian::StepMarks()
{
	const ATakeDirector* Director = ATakeDirector::Get(this);
	if (!Director)
	{
		return;
	}

	// Standing still with his feet inside a mark's StandRadius (2D): the nearest such mark is his.
	AMark* NewMark = nullptr;
	if (IsStanding())
	{
		const FVector Feet = GetFeetLocation();
		double BestDist = TNumericLimits<double>::Max();
		for (const TObjectPtr<AMark>& MarkPtr : Director->Marks)
		{
			AMark* const EachMark = MarkPtr.Get();
			if (!EachMark || !EachMark->HasAppeared())
			{
				continue;
			}
			const double Dist = FVector::Dist2D(EachMark->GetStandPoint(), Feet);
			if (Dist <= EachMark->StandRadius && Dist < BestDist)
			{
				BestDist = Dist;
				NewMark = EachMark;
			}
		}
	}

	if (NewMark == StandingMark.Get())
	{
		return;
	}

	AMark* const OldMark = StandingMark.Get();
	StandingMark = NewMark;
	if (OldMark)
	{
		OldMark->SetOccupied(false);
		TAKE_LOG(TEXT("OFF MARK %s"), *OldMark->MarkName);
	}
	if (NewMark)
	{
		NewMark->SetOccupied(true);	// the gold X shows this frame
		TAKE_LOG(TEXT("ON MARK %s"), *NewMark->MarkName);
	}
}

AMark* AJulian::GetStandingMark() const
{
	return StandingMark;
}
