// Owner: (a). Constructor by the header pass; every "// BODY: (a)" is a stub to fill.

#include "TakeHUD.h"

#include "BarClockComponent.h"
#include "ComposureComponent.h"
#include "Device.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "FollowSpot.h"
#include "Julian.h"
#include "Mark.h"
#include "TakeDirector.h"
#include "TakeGameInstance.h"
#include "TakeTypes.h"
#include "Watcher.h"

ATakeHUD::ATakeHUD()
{
}

void ATakeHUD::DrawHUD()
{
	Super::DrawHUD();
	// BODY: (a)
}

ATakeDirector* ATakeHUD::GetDirector()
{
	// BODY: (a)
	return ATakeDirector::Get(this);
}

FVector2D ATakeHUD::DrawShadowedText(const FString& Text, const FLinearColor& Colour, float X, float Y, float Scale,
	bool bCentre)
{
	// BODY: (a)
	return FVector2D::ZeroVector;
}
