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

	// He starts the take standing on a mark if one is under his feet (so MARK shows from the first frame).
	StandingMark = nullptr;
	for (TActorIterator<AMark> It(GetWorld()); It; ++It)
	{
		if (FVector::Dist2D(It->GetStandPoint(), GetFeetLocation()) <= StartMarkTolerance)
		{
			StandingMark = *It;
			break;
		}
	}
}

void AJulian::StepMarks()
{
	// BODY: (m)
}

AMark* AJulian::GetStandingMark() const
{
	return StandingMark;
}
