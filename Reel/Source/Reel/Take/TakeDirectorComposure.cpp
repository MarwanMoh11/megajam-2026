// ATakeDirector slice (a): exposure and composure. See TakeDirector.h for the contract.

#include "TakeDirector.h"

#include "ComposureComponent.h"
#include "Julian.h"
#include "Mark.h"
#include "RedTapeZone.h"

bool ATakeDirector::IsJulianExposed() const
{
	// BODY: (a)
	return false;
}

void ATakeDirector::StepComposure(float DeltaSeconds)
{
	// BODY: (a)
}
