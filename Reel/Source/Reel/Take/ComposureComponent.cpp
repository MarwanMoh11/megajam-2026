// Owner: (a). Constructor by the header pass; every "// BODY: (a)" is a stub to fill.

#include "ComposureComponent.h"

#include "TakeTypes.h"

UComposureComponent::UComposureComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UComposureComponent::Step(float DeltaSeconds, bool bSeen, bool bOnMarkUnexposed)
{
	// BODY: (a)
}

void UComposureComponent::Refill()
{
	// BODY: (a)
	Value = MaxComposure;
	SeenSeconds = 0.f;
	Label = EComposureLabel::None;
}

float UComposureComponent::GetComposure() const
{
	// BODY: (a)
	return Value;
}

float UComposureComponent::GetFraction() const
{
	// BODY: (a)
	return 1.f;
}

bool UComposureComponent::IsEmpty() const
{
	// BODY: (a)
	return false;
}

EComposureLabel UComposureComponent::GetLabel() const
{
	// BODY: (a)
	return Label;
}

FString UComposureComponent::GetLabelText() const
{
	// BODY: (a)
	return FString();
}

float UComposureComponent::GetSeenSeconds() const
{
	// BODY: (a)
	return 0.f;
}
