// Owner: (a). Constructor by the header pass; bodies by (a).

#include "ComposureComponent.h"

#include "TakeTypes.h"

UComposureComponent::UComposureComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UComposureComponent::Step(float DeltaSeconds, bool bSeen, bool bOnMarkUnexposed)
{
	const float Dt = FMath::Max(0.f, DeltaSeconds);
	if (bSeen)
	{
		// Only the part of this frame past the grace drains.
		SeenSeconds += Dt;
		const float DrainSeconds = FMath::Min(Dt, FMath::Max(0.f, SeenSeconds - SeenGraceSeconds));
		Value -= SeenDrainPerSecond * DrainSeconds;
		Label = EComposureLabel::Seen;
	}
	else
	{
		SeenSeconds = 0.f;
		if (bOnMarkUnexposed)
		{
			Value += MarkRefillPerSecond * Dt;
			Label = EComposureLabel::Mark;
		}
		else
		{
			Label = EComposureLabel::None;
		}
	}
	Value = FMath::Clamp(Value, 0.f, MaxComposure);
}

void UComposureComponent::Refill()
{
	Value = MaxComposure;
	SeenSeconds = 0.f;
	Label = EComposureLabel::None;
}

float UComposureComponent::GetComposure() const
{
	return Value;
}

float UComposureComponent::GetFraction() const
{
	return MaxComposure > 0.f ? FMath::Clamp(Value / MaxComposure, 0.f, 1.f) : 0.f;
}

bool UComposureComponent::IsEmpty() const
{
	return Value <= 0.f;
}

EComposureLabel UComposureComponent::GetLabel() const
{
	return Label;
}

FString UComposureComponent::GetLabelText() const
{
	switch (Label)
	{
	case EComposureLabel::Seen: return TEXT("SEEN");
	case EComposureLabel::Mark: return TEXT("MARK");
	default: return FString();
	}
}

float UComposureComponent::GetSeenSeconds() const
{
	return SeenSeconds;
}
