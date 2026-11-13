// Owner: (a). Constructor by the header pass; every "// BODY: (a)" is a stub to fill.

#include "BarClockComponent.h"

#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "TakeTypes.h"

UBarClockComponent::UBarClockComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBarClockComponent::BeginPlay()
{
	Super::BeginPlay();
	// BODY: (a)
}

void UBarClockComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// BODY: (a)
	Super::EndPlay(EndPlayReason);
}

void UBarClockComponent::Advance(float DeltaSeconds)
{
	// BODY: (a)
}

void UBarClockComponent::SetStartBar(int32 Bar)
{
	// BODY: (a)
}

double UBarClockComponent::GetTotalBeats() const
{
	// BODY: (a)
	return 0.0;
}

int32 UBarClockComponent::GetBar() const
{
	// BODY: (a)
	return 1;
}

int32 UBarClockComponent::GetBeatInBar() const
{
	// BODY: (a)
	return 1;
}

float UBarClockComponent::GetBeatPhase() const
{
	// BODY: (a)
	return 0.f;
}

bool UBarClockComponent::BeatCrossedThisFrame() const
{
	// BODY: (a)
	return false;
}

bool UBarClockComponent::BarCrossedThisFrame() const
{
	// BODY: (a)
	return false;
}

float UBarClockComponent::GetSecondsPerBeat() const
{
	// BODY: (a)
	return 0.5f;
}

float UBarClockComponent::GetSecondsPerBar() const
{
	// BODY: (a)
	return 2.f;
}

float UBarClockComponent::GetPulse() const
{
	// BODY: (a)
	return 0.f;
}

FString UBarClockComponent::GetTimecode() const
{
	// BODY: (a)
	return TEXT("1.1");
}

void UBarClockComponent::PlayAccent()
{
	// BODY: (a)
}

void UBarClockComponent::PlayClick(bool bDownbeat)
{
	// BODY: (a)
}

void UBarClockComponent::BuildPcm()
{
	// BODY: (a)
}

void UBarClockComponent::PlayPcm(const TArray<uint8>& Pcm)
{
	// BODY: (a)
}
