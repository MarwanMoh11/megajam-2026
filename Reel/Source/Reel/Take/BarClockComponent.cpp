// Owner: (a). Constructor by the header pass; bodies by (a).

#include "BarClockComponent.h"

#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "TakeTypes.h"

namespace TakeClockA
{
	/** One 16-bit little-endian mono sample, Sample in -1..1. */
	void AppendSample(TArray<uint8>& Out, float Sample)
	{
		const int32 Quantised = FMath::Clamp(FMath::RoundToInt(Sample * 32767.f), -32768, 32767);
		const uint16 Bits = static_cast<uint16>(static_cast<int16>(Quantised));
		Out.Add(static_cast<uint8>(Bits & 0xFF));
		Out.Add(static_cast<uint8>((Bits >> 8) & 0xFF));
	}

	/** A short decaying sine: the metronome click. */
	void BuildSineClick(TArray<uint8>& Out, int32 Rate, float Freq, float Seconds, float Amp, float DecayPerSecond)
	{
		const int32 NumSamples = FMath::Max(1, FMath::RoundToInt(static_cast<float>(Rate) * Seconds));
		const float AttackSamples = FMath::Max(1.f, static_cast<float>(Rate) * 0.001f);
		Out.Reset(NumSamples * 2);
		for (int32 Index = 0; Index < NumSamples; ++Index)
		{
			const float T = static_cast<float>(Index) / static_cast<float>(Rate);
			const float Env = FMath::Exp(-T * DecayPerSecond) * FMath::Min(1.f, static_cast<float>(Index) / AttackSamples);
			AppendSample(Out, FMath::Sin(static_cast<float>(UE_TWO_PI) * Freq * T) * Env * Amp);
		}
	}

	/** A bright noise burst (white noise, differenced for a high-passed hiss): the beam's hi-hat splash. */
	void BuildNoiseBurst(TArray<uint8>& Out, int32 Rate, float Seconds, float Amp, float DecayPerSecond)
	{
		const int32 NumSamples = FMath::Max(1, FMath::RoundToInt(static_cast<float>(Rate) * Seconds));
		const float AttackSamples = FMath::Max(1.f, static_cast<float>(Rate) * 0.001f);
		FRandomStream Rng(7351);
		float Previous = 0.f;
		Out.Reset(NumSamples * 2);
		for (int32 Index = 0; Index < NumSamples; ++Index)
		{
			const float T = static_cast<float>(Index) / static_cast<float>(Rate);
			const float Noise = Rng.FRandRange(-1.f, 1.f);
			const float Bright = (Noise - Previous) * 0.5f;
			Previous = Noise;
			const float Env = FMath::Exp(-T * DecayPerSecond) * FMath::Min(1.f, static_cast<float>(Index) / AttackSamples);
			AppendSample(Out, Bright * Env * Amp);
		}
	}

	int64 FloorBeat(double Beats)
	{
		return static_cast<int64>(FMath::FloorToDouble(Beats));
	}
}

UBarClockComponent::UBarClockComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBarClockComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!bStartBarSet)
	{
		TotalBeats = static_cast<double>(FMath::Max(1, StartBar) - 1) * static_cast<double>(FMath::Max(1, BeatsPerBar));
	}
	bBeatCrossed = false;
	bBarCrossed = false;
	PulseAge = 1000.f;
	bFirstAdvance = true;

	BuildPcm();

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	// One procedural wave that plays for the whole take: each click is queued into it (silence in between).
	ClickWave = NewObject<USoundWaveProcedural>(this);
	ClickWave->SetSampleRate(static_cast<uint32>(SampleRate));
	ClickWave->NumChannels = 1;
	ClickWave->Duration = INDEFINITELY_LOOPING_DURATION;
	ClickWave->bLooping = false;

	ClickAudio = NewObject<UAudioComponent>(OwnerActor, TEXT("TakeClickAudio"));
	ClickAudio->bAutoActivate = false;
	ClickAudio->bAutoDestroy = false;
	ClickAudio->bAllowSpatialization = false;
	ClickAudio->bIsUISound = true;
	ClickAudio->SetSound(ClickWave);
	ClickAudio->SetVolumeMultiplier(ClickVolume);
	ClickAudio->RegisterComponent();
	ClickAudio->Play();
}

void UBarClockComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ClickAudio)
	{
		ClickAudio->Stop();
		ClickAudio->DestroyComponent();
		ClickAudio = nullptr;
	}
	ClickWave = nullptr;
	Super::EndPlay(EndPlayReason);
}

void UBarClockComponent::Advance(float DeltaSeconds)
{
	bBeatCrossed = false;
	bBarCrossed = false;

	const int64 PerBar = FMath::Max(1, BeatsPerBar);
	const double Before = TotalBeats;
	const float Step = FMath::Max(0.f, DeltaSeconds);
	TotalBeats += static_cast<double>(Step) / static_cast<double>(GetSecondsPerBeat());
	PulseAge += Step;

	bool bClick = false;
	bool bClickDownbeat = false;

	// The take's first frame sits on a beat line (bar N, beat 1): click and pulse it, without beat events.
	if (bFirstAdvance)
	{
		bFirstAdvance = false;
		const int64 StartLine = TakeClockA::FloorBeat(Before);
		if (Before - static_cast<double>(StartLine) < 1e-6)
		{
			bClick = true;
			bClickDownbeat = (StartLine % PerBar) == 0;
			PulseAge = 0.f;
		}
	}

	// Every beat line in (Before, TotalBeats] is crossed this frame.
	const int64 FirstLine = TakeClockA::FloorBeat(Before) + 1;
	const int64 LastLine = TakeClockA::FloorBeat(TotalBeats);
	for (int64 Line = FirstLine; Line <= LastLine; ++Line)
	{
		bBeatCrossed = true;
		if ((Line % PerBar) == 0)
		{
			bBarCrossed = true;
		}
	}

	if (bBeatCrossed)
	{
		PulseAge = 0.f;
		bClick = true;
		bClickDownbeat = bClickDownbeat || bBarCrossed;
	}

	// One click per frame even if a hitch crossed several lines (stacked clicks would smear).
	if (bClick && bMetronome)
	{
		PlayClick(bClickDownbeat);
	}
}

void UBarClockComponent::SetStartBar(int32 Bar)
{
	TotalBeats = static_cast<double>(FMath::Max(1, Bar) - 1) * static_cast<double>(FMath::Max(1, BeatsPerBar));
	bBeatCrossed = false;
	bBarCrossed = false;
	PulseAge = 1000.f;
	bFirstAdvance = true;
	bStartBarSet = true;
}

double UBarClockComponent::GetTotalBeats() const
{
	return TotalBeats;
}

int32 UBarClockComponent::GetBar() const
{
	const int64 PerBar = FMath::Max(1, BeatsPerBar);
	return static_cast<int32>(TakeClockA::FloorBeat(TotalBeats) / PerBar) + 1;
}

int32 UBarClockComponent::GetBeatInBar() const
{
	const int64 PerBar = FMath::Max(1, BeatsPerBar);
	return static_cast<int32>(TakeClockA::FloorBeat(TotalBeats) % PerBar) + 1;
}

float UBarClockComponent::GetBeatPhase() const
{
	return static_cast<float>(TotalBeats - FMath::FloorToDouble(TotalBeats));
}

bool UBarClockComponent::BeatCrossedThisFrame() const
{
	return bBeatCrossed;
}

bool UBarClockComponent::BarCrossedThisFrame() const
{
	return bBarCrossed;
}

float UBarClockComponent::GetSecondsPerBeat() const
{
	return 60.f / FMath::Max(1.f, BPM);
}

float UBarClockComponent::GetSecondsPerBar() const
{
	return GetSecondsPerBeat() * static_cast<float>(FMath::Max(1, BeatsPerBar));
}

float UBarClockComponent::GetPulse() const
{
	if (PulseDecaySeconds <= 0.f)
	{
		return PulseAge <= 0.f ? 1.f : 0.f;
	}
	return FMath::Clamp(1.f - PulseAge / PulseDecaySeconds, 0.f, 1.f);
}

FString UBarClockComponent::GetTimecode() const
{
	return FString::Printf(TEXT("%d.%d"), GetBar(), GetBeatInBar());
}

void UBarClockComponent::PlayAccent()
{
	PlayPcm(PcmAccent);
}

void UBarClockComponent::PlayClick(bool bDownbeat)
{
	PlayPcm(bDownbeat ? PcmDownbeat : PcmBeat);
}

void UBarClockComponent::BuildPcm()
{
	TakeClockA::BuildSineClick(PcmBeat, SampleRate, 1000.f, 0.05f, 0.7f, 90.f);
	TakeClockA::BuildSineClick(PcmDownbeat, SampleRate, 1600.f, 0.06f, 0.85f, 80.f);
	TakeClockA::BuildNoiseBurst(PcmAccent, SampleRate, 0.25f, 0.8f, 18.f);
}

void UBarClockComponent::PlayPcm(const TArray<uint8>& Pcm)
{
	if (!ClickWave || !ClickAudio || Pcm.Num() == 0)
	{
		return;
	}

	// More than half a second already waiting (16-bit mono: SampleRate bytes): the device is behind, drop this one.
	if (ClickWave->GetAvailableAudioByteCount() > SampleRate)
	{
		return;
	}

	ClickAudio->SetVolumeMultiplier(ClickVolume);
	if (!ClickAudio->IsPlaying())
	{
		ClickAudio->Play();
	}
	ClickWave->QueueAudio(Pcm.GetData(), Pcm.Num());
}
