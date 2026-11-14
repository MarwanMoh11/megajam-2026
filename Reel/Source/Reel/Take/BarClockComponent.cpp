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

namespace TakeClockD
{
	/** Appends Seconds of silence. */
	void AppendSilence(TArray<uint8>& Out, int32 Rate, float Seconds)
	{
		const int32 NumSamples = FMath::Max(0, FMath::RoundToInt(static_cast<float>(Rate) * Seconds));
		for (int32 Index = 0; Index < NumSamples; ++Index)
		{
			TakeClockA::AppendSample(Out, 0.f);
		}
	}

	/**
	 * Appends one note: Harmonics partials (each HarmonicFalloff times the one below, normalised), a 5 ms attack,
	 * an exponential decay and a short linear release at the end (no click when it stops). Few harmonics and a fast
	 * decay give a pluck; many harmonics and a slow decay give a brassy note.
	 */
	void AppendTone(TArray<uint8>& Out, int32 Rate, float Freq, float Seconds, float Amp, float DecayPerSecond,
		int32 Harmonics, float HarmonicFalloff)
	{
		const int32 NumSamples = FMath::Max(1, FMath::RoundToInt(static_cast<float>(Rate) * Seconds));
		const float AttackSamples = FMath::Max(1.f, static_cast<float>(Rate) * 0.005f);
		const float ReleaseSamples = FMath::Max(1.f, static_cast<float>(Rate) * 0.015f);
		const int32 Partials = FMath::Max(1, Harmonics);
		float WeightSum = 0.f;
		float Weight = 1.f;
		for (int32 H = 0; H < Partials; ++H)
		{
			WeightSum += Weight;
			Weight *= HarmonicFalloff;
		}
		WeightSum = FMath::Max(WeightSum, KINDA_SMALL_NUMBER);

		for (int32 Index = 0; Index < NumSamples; ++Index)
		{
			const float T = static_cast<float>(Index) / static_cast<float>(Rate);
			float Wave = 0.f;
			float PartialWeight = 1.f;
			for (int32 H = 1; H <= Partials; ++H)
			{
				Wave += FMath::Sin(static_cast<float>(UE_TWO_PI) * Freq * static_cast<float>(H) * T) * PartialWeight;
				PartialWeight *= HarmonicFalloff;
			}
			Wave /= WeightSum;
			const float Attack = FMath::Min(1.f, static_cast<float>(Index) / AttackSamples);
			const float Release = FMath::Min(1.f, static_cast<float>(NumSamples - 1 - Index) / ReleaseSamples);
			const float Env = FMath::Exp(-T * DecayPerSecond) * Attack * Release;
			TakeClockA::AppendSample(Out, Wave * Env * Amp);
		}
	}

	/** Appends one snare-like tap (plain white noise, darker than the hi-hat's differenced hiss) in a SlotSeconds
	 *  slot: TapSeconds of decaying noise, then silence to the end of the slot. */
	void AppendNoiseTap(TArray<uint8>& Out, int32 Rate, float SlotSeconds, float TapSeconds, float Amp,
		float DecayPerSecond, FRandomStream& Rng)
	{
		const int32 SlotSamples = FMath::Max(1, FMath::RoundToInt(static_cast<float>(Rate) * SlotSeconds));
		const int32 TapSamples = FMath::Clamp(FMath::RoundToInt(static_cast<float>(Rate) * TapSeconds), 1, SlotSamples);
		const float AttackSamples = FMath::Max(1.f, static_cast<float>(Rate) * 0.001f);
		for (int32 Index = 0; Index < SlotSamples; ++Index)
		{
			if (Index >= TapSamples)
			{
				TakeClockA::AppendSample(Out, 0.f);
				continue;
			}
			const float T = static_cast<float>(Index) / static_cast<float>(Rate);
			const float Env = FMath::Exp(-T * DecayPerSecond) * FMath::Min(1.f, static_cast<float>(Index) / AttackSamples);
			TakeClockA::AppendSample(Out, Rng.FRandRange(-1.f, 1.f) * Env * Amp);
		}
	}

	/** A rising squeal: a sine sweeping exponentially from FromHz to ToHz with a little vibrato and a touch of its
	 *  second harmonic, getting louder as it rises. */
	void BuildSqueal(TArray<uint8>& Out, int32 Rate, float FromHz, float ToHz, float Seconds, float Amp)
	{
		const int32 NumSamples = FMath::Max(1, FMath::RoundToInt(static_cast<float>(Rate) * Seconds));
		const float AttackSamples = FMath::Max(1.f, static_cast<float>(Rate) * 0.02f);
		const float ReleaseSamples = FMath::Max(1.f, static_cast<float>(Rate) * 0.04f);
		const float Ratio = ToHz / FMath::Max(FromHz, 1.f);
		double Phase = 0.0;
		Out.Reset(NumSamples * 2);
		for (int32 Index = 0; Index < NumSamples; ++Index)
		{
			const float T = static_cast<float>(Index) / static_cast<float>(Rate);
			const float Alpha = static_cast<float>(Index) / static_cast<float>(NumSamples);
			const float Vibrato = 1.f + 0.015f * FMath::Sin(static_cast<float>(UE_TWO_PI) * 7.f * T);
			const float Freq = FromHz * FMath::Pow(Ratio, Alpha) * Vibrato;
			Phase += static_cast<double>(UE_TWO_PI) * static_cast<double>(Freq) / static_cast<double>(Rate);
			const float Wave = (FMath::Sin(static_cast<float>(Phase)) + 0.3f * FMath::Sin(static_cast<float>(Phase * 2.0))) / 1.3f;
			const float Attack = FMath::Min(1.f, static_cast<float>(Index) / AttackSamples);
			const float Release = FMath::Min(1.f, static_cast<float>(NumSamples - 1 - Index) / ReleaseSamples);
			const float Swell = FMath::Lerp(0.35f, 1.f, Alpha);
			TakeClockA::AppendSample(Out, Wave * Attack * Release * Swell * Amp);
		}
	}
}

namespace TakeClockStep3
{
	/** A harsh square-ish tone: the odd partials 1, 3, 5... (each 1/k), a fast attack, a flat sustain and a short
	 *  linear release, so tones on consecutive beats stay distinct. The raw odd-partial sum peaks near 0.93. */
	void AppendOddTone(TArray<uint8>& Out, int32 Rate, float Freq, float Seconds, float Amp, int32 OddPartials,
		float AttackSeconds, float ReleaseSeconds)
	{
		const int32 NumSamples = FMath::Max(1, FMath::RoundToInt(static_cast<float>(Rate) * Seconds));
		const float AttackSamples = FMath::Max(1.f, static_cast<float>(Rate) * AttackSeconds);
		const float ReleaseSamples = FMath::Max(1.f, static_cast<float>(Rate) * ReleaseSeconds);
		const int32 Partials = FMath::Max(1, OddPartials);
		const float Nyquist = 0.5f * static_cast<float>(Rate);
		for (int32 Index = 0; Index < NumSamples; ++Index)
		{
			const float T = static_cast<float>(Index) / static_cast<float>(Rate);
			float Wave = 0.f;
			for (int32 P = 0; P < Partials; ++P)
			{
				const float K = static_cast<float>(2 * P + 1);
				if (Freq * K >= Nyquist)
				{
					break;
				}
				Wave += FMath::Sin(static_cast<float>(UE_TWO_PI) * Freq * K * T) / K;
			}
			const float Attack = FMath::Min(1.f, static_cast<float>(Index) / AttackSamples);
			const float Release = FMath::Min(1.f, static_cast<float>(NumSamples - 1 - Index) / ReleaseSamples);
			TakeClockA::AppendSample(Out, Wave * Attack * Release * Amp);
		}
	}

	/** The clapper: Seconds of noise (half white, half differenced for the bite), a near-instant attack and a fast
	 *  exponential decay. Dry: no tail. */
	void BuildClack(TArray<uint8>& Out, int32 Rate, float Seconds, float Amp, float DecayPerSecond)
	{
		const int32 NumSamples = FMath::Max(1, FMath::RoundToInt(static_cast<float>(Rate) * Seconds));
		const float AttackSamples = FMath::Max(1.f, static_cast<float>(Rate) * 0.0003f);
		const float ReleaseSamples = FMath::Max(1.f, static_cast<float>(Rate) * 0.003f);
		FRandomStream Rng(2203);
		float Previous = 0.f;
		Out.Reset(NumSamples * 2);
		for (int32 Index = 0; Index < NumSamples; ++Index)
		{
			const float T = static_cast<float>(Index) / static_cast<float>(Rate);
			const float Noise = Rng.FRandRange(-1.f, 1.f);
			const float Wave = 0.5f * Noise + 0.5f * (Noise - Previous);
			Previous = Noise;
			const float Attack = FMath::Min(1.f, static_cast<float>(Index) / AttackSamples);
			const float Release = FMath::Min(1.f, static_cast<float>(NumSamples - 1 - Index) / ReleaseSamples);
			const float Env = FMath::Exp(-T * DecayPerSecond) * Attack * Release;
			TakeClockA::AppendSample(Out, Wave * Env * Amp);
		}
	}

	/** A low thud: a sine falling fast from StartHz to EndHz (plus a touch of its second partial so small speakers
	 *  carry it), a 2 ms attack and an exponential decay to a short release. */
	void BuildThud(TArray<uint8>& Out, int32 Rate, float StartHz, float EndHz, float Seconds, float Amp)
	{
		const int32 NumSamples = FMath::Max(1, FMath::RoundToInt(static_cast<float>(Rate) * Seconds));
		const float AttackSamples = FMath::Max(1.f, static_cast<float>(Rate) * 0.002f);
		const float ReleaseSamples = FMath::Max(1.f, static_cast<float>(Rate) * 0.02f);
		double Phase = 0.0;
		Out.Reset(NumSamples * 2);
		for (int32 Index = 0; Index < NumSamples; ++Index)
		{
			const float T = static_cast<float>(Index) / static_cast<float>(Rate);
			const float Freq = EndHz + (StartHz - EndHz) * FMath::Exp(-T * 30.f);
			Phase += static_cast<double>(UE_TWO_PI) * static_cast<double>(Freq) / static_cast<double>(Rate);
			const float Wave = (FMath::Sin(static_cast<float>(Phase)) + 0.25f * FMath::Sin(static_cast<float>(Phase * 2.0))) / 1.25f;
			const float Attack = FMath::Min(1.f, static_cast<float>(Index) / AttackSamples);
			const float Release = FMath::Min(1.f, static_cast<float>(NumSamples - 1 - Index) / ReleaseSamples);
			const float Env = FMath::Exp(-T * 9.f) * Attack * Release;
			TakeClockA::AppendSample(Out, Wave * Env * Amp);
		}
	}

	/** The pyro's crackle: Bursts short noise pops (1.5-6 ms each, random level) scattered over Seconds, fading a
	 *  little towards the end. */
	void BuildCrackle(TArray<uint8>& Out, int32 Rate, float Seconds, float Amp, int32 Bursts)
	{
		const int32 NumSamples = FMath::Max(1, FMath::RoundToInt(static_cast<float>(Rate) * Seconds));
		TArray<float> Mix;
		Mix.SetNumZeroed(NumSamples);
		FRandomStream Rng(9187);
		for (int32 Burst = 0; Burst < FMath::Max(1, Bursts); ++Burst)
		{
			const int32 Start = Rng.RandRange(0, NumSamples - 1);
			const int32 Length = FMath::Max(1, FMath::RoundToInt(static_cast<float>(Rate) * Rng.FRandRange(0.0015f, 0.006f)));
			const float Level = Rng.FRandRange(0.3f, 1.f);
			const float Decay = 1.f / FMath::Max(1.f, static_cast<float>(Length) * 0.35f);
			for (int32 Offset = 0; Offset < Length && Start + Offset < NumSamples; ++Offset)
			{
				Mix[Start + Offset] += Rng.FRandRange(-1.f, 1.f) * Level * FMath::Exp(-static_cast<float>(Offset) * Decay);
			}
		}
		Out.Reset(NumSamples * 2);
		for (int32 Index = 0; Index < NumSamples; ++Index)
		{
			const float Alpha = static_cast<float>(Index) / static_cast<float>(NumSamples);
			const float Fade = Alpha < 0.7f ? 1.f : FMath::Lerp(1.f, 0.35f, (Alpha - 0.7f) / 0.3f);
			TakeClockA::AppendSample(Out, FMath::Clamp(Mix[Index], -1.f, 1.f) * Fade * Amp);
		}
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

	// The phrase voices: the same kind of endless procedural wave, one each.
	PhraseAudios.Reset();
	PhraseWaves.Reset();
	for (int32 Voice = 0; Voice < FMath::Max(1, PhraseVoices); ++Voice)
	{
		USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(this);
		Wave->SetSampleRate(static_cast<uint32>(SampleRate));
		Wave->NumChannels = 1;
		Wave->Duration = INDEFINITELY_LOOPING_DURATION;
		Wave->bLooping = false;

		UAudioComponent* Audio = NewObject<UAudioComponent>(OwnerActor);
		Audio->bAutoActivate = false;
		Audio->bAutoDestroy = false;
		Audio->bAllowSpatialization = false;
		Audio->bIsUISound = true;
		Audio->SetSound(Wave);
		Audio->SetVolumeMultiplier(ClickVolume);
		Audio->RegisterComponent();
		Audio->Play();

		PhraseWaves.Add(Wave);
		PhraseAudios.Add(Audio);
	}
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
	for (UAudioComponent* Audio : PhraseAudios)
	{
		if (Audio)
		{
			Audio->Stop();
			Audio->DestroyComponent();
		}
	}
	PhraseAudios.Reset();
	PhraseWaves.Reset();
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

	// Step 3: the alarm replaces the click on every beat (high on beats 1 and 3, low on 2 and 4) and never rests:
	// it plays through break bars, mutes and a switched-off metronome.
	if (bClick && bAlarm)
	{
		const bool bHigh = (GetBeatInBar() % 2) == 1;
		PlayPcm(bHigh ? PcmAlarmHigh : PcmAlarmLow);
		return;
	}

	// One click per frame even if a hitch crossed several lines (stacked clicks would smear). A band break or a
	// MuteMetronome silences it; the beat events and the pulse above still happen.
	if (bClick && bMetronome && !IsMetronomeMuted())
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
	MutedUntilBeats = 0.0;
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

	// Step 2: the warning phrases (each under one beat, 0.5 s at 120 BPM) and the effects, at WarningVolume.
	const float Level = FMath::Clamp(WarningVolume, 0.f, 1.f);

	// Bass (the guard): two low plucks, 80 then 100 Hz, with enough harmonics to carry on small speakers.
	PcmBass.Reset();
	TakeClockD::AppendTone(PcmBass, SampleRate, 80.f, 0.2f, Level, 9.f, 5, 0.6f);
	TakeClockD::AppendSilence(PcmBass, SampleRate, 0.03f);
	TakeClockD::AppendTone(PcmBass, SampleRate, 100.f, 0.2f, Level, 9.f, 5, 0.6f);

	// Trumpet (the drone): two sustained bright mid notes, 520 then 660 Hz (a saw-like stack of partials).
	PcmTrumpet.Reset();
	TakeClockD::AppendTone(PcmTrumpet, SampleRate, 520.f, 0.17f, Level * 0.8f, 2.f, 8, 0.75f);
	TakeClockD::AppendSilence(PcmTrumpet, SampleRate, 0.03f);
	TakeClockD::AppendTone(PcmTrumpet, SampleRate, 660.f, 0.2f, Level * 0.8f, 2.f, 8, 0.75f);

	// DrumRoll (Vance's speech): eight fast snare taps, 50 ms apart, rising from soft to full.
	PcmDrumRoll.Reset();
	{
		FRandomStream RollRng(4129);
		const int32 Taps = 8;
		for (int32 Tap = 0; Tap < Taps; ++Tap)
		{
			const float Rise = FMath::Lerp(0.25f, 1.f, static_cast<float>(Tap) / static_cast<float>(Taps - 1));
			TakeClockD::AppendNoiseTap(PcmDrumRoll, SampleRate, 0.05f, 0.04f, Level * Rise, 60.f, RollRng);
		}
	}

	// The sound desk's feedback: a 0.6 s squeal rising from 1.2 to 3.2 kHz.
	TakeClockD::BuildSqueal(PcmFeedback, SampleRate, 1200.f, 3200.f, 0.6f, Level * 0.7f);

	// The chime: two bell-like notes rising a fourth (E6 then A6), short then ringing.
	PcmChime.Reset();
	TakeClockD::AppendTone(PcmChime, SampleRate, 1318.5f, 0.12f, Level * 0.7f, 6.f, 3, 0.35f);
	TakeClockD::AppendTone(PcmChime, SampleRate, 1760.f, 0.32f, Level * 0.7f, 7.f, 3, 0.35f);

	// Step 3. The alarm: a harsh two-tone, 880 and 660 Hz, 0.18 s each (a 0.5 s beat leaves a clear gap), at
	// AlarmVolume. It replaces the click on the click voice.
	const float AlarmLevel = FMath::Clamp(AlarmVolume, 0.f, 1.f);
	PcmAlarmHigh.Reset();
	TakeClockStep3::AppendOddTone(PcmAlarmHigh, SampleRate, 880.f, 0.18f, AlarmLevel, 7, 0.003f, 0.025f);
	PcmAlarmLow.Reset();
	TakeClockStep3::AppendOddTone(PcmAlarmLow, SampleRate, 660.f, 0.18f, AlarmLevel, 7, 0.003f, 0.025f);

	// PRINT IT: a bright rising four-note sting (G5 B5 D6, then a ringing G6), 0.9 s in all.
	PcmSting.Reset();
	TakeClockD::AppendTone(PcmSting, SampleRate, 784.f, 0.14f, Level * 0.75f, 3.f, 6, 0.6f);
	TakeClockD::AppendTone(PcmSting, SampleRate, 987.8f, 0.14f, Level * 0.75f, 3.f, 6, 0.6f);
	TakeClockD::AppendTone(PcmSting, SampleRate, 1174.7f, 0.14f, Level * 0.75f, 3.f, 6, 0.6f);
	TakeClockD::AppendTone(PcmSting, SampleRate, 1568.f, 0.48f, Level * 0.8f, 3.5f, 6, 0.6f);

	// The clapper: one dry 30 ms clack.
	TakeClockStep3::BuildClack(PcmClap, SampleRate, 0.03f, Level, 110.f);

	// The curtain or a shutter landing: a 0.25 s thud falling from about 110 Hz to 55 Hz (about 60 Hz most of it).
	TakeClockStep3::BuildThud(PcmThud, SampleRate, 110.f, 55.f, 0.25f, Level);

	// The pyro: 0.5 s of crackle.
	TakeClockStep3::BuildCrackle(PcmCrackle, SampleRate, 0.5f, Level * 0.8f, 45);
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

// ---------------------------------------------------------------------------------------------------------------------
// Step 2 (header pass stubs; BODY: D)
// ---------------------------------------------------------------------------------------------------------------------

void UBarClockComponent::PlayWarning(EWatcherInstrument Instrument)
{
	// Played whether or not the metronome is muted (PlayPcm does not look at the mute).
	switch (Instrument)
	{
	case EWatcherInstrument::Bass:
		PlayPhrase(PcmBass);
		break;
	case EWatcherInstrument::Trumpet:
		PlayPhrase(PcmTrumpet);
		break;
	case EWatcherInstrument::DrumRoll:
		PlayPhrase(PcmDrumRoll);
		break;
	case EWatcherInstrument::HiHat:
		PlayAccent();
		break;
	default:
		break;
	}
}

void UBarClockComponent::PlayFeedback()
{
	PlayPhrase(PcmFeedback);
}

void UBarClockComponent::PlayChime()
{
	PlayPhrase(PcmChime);
}

void UBarClockComponent::MuteMetronome(float Bars)
{
	if (Bars <= 0.f)
	{
		return;
	}
	// Counted from the beat line the clock is on, so the mute ends exactly on a beat line (a solo bar started on a
	// bar line ends on the next bar line, whose downbeat clicks again).
	const double PerBar = static_cast<double>(FMath::Max(1, BeatsPerBar));
	const double Until = FMath::FloorToDouble(TotalBeats) + static_cast<double>(Bars) * PerBar;
	MutedUntilBeats = FMath::Max(MutedUntilBeats, Until);
}

bool UBarClockComponent::IsBreakBar(int32 Bar) const
{
	return BreakEveryBars > 0 && Bar > 0 && (Bar % BreakEveryBars) == 0;
}

bool UBarClockComponent::IsMetronomeMuted() const
{
	// Step 3: the alarm never rests, so no band break (and no "BAND BREAK" on the HUD) while it sounds.
	if (bAlarm)
	{
		return false;
	}
	return IsBreakBar(GetBar()) || TotalBeats < MutedUntilBeats;
}

void UBarClockComponent::PlayPhrase(const TArray<uint8>& Pcm)
{
	if (Pcm.Num() == 0)
	{
		return;
	}

	// The voice with the least waiting: a second warning on the same bar line sounds with the first, not after it.
	int32 Best = INDEX_NONE;
	int32 BestWaiting = MAX_int32;
	for (int32 Voice = 0; Voice < PhraseWaves.Num(); ++Voice)
	{
		if (PhraseWaves[Voice] && PhraseAudios.IsValidIndex(Voice) && PhraseAudios[Voice])
		{
			const int32 Waiting = PhraseWaves[Voice]->GetAvailableAudioByteCount();
			if (Waiting < BestWaiting)
			{
				BestWaiting = Waiting;
				Best = Voice;
			}
		}
	}
	if (Best == INDEX_NONE)
	{
		PlayPcm(Pcm);	// no phrase voice (no owner at BeginPlay): the click queue is better than silence
		return;
	}
	// More than a second already waiting on every voice: the device is behind, drop this one.
	if (BestWaiting > SampleRate * 2)
	{
		return;
	}
	if (!PhraseAudios[Best]->IsPlaying())
	{
		PhraseAudios[Best]->Play();
	}
	PhraseWaves[Best]->QueueAudio(Pcm.GetData(), Pcm.Num());
}

// ---------------------------------------------------------------------------------------------------------------------
// Step 3 (BODY: D)
// ---------------------------------------------------------------------------------------------------------------------

void UBarClockComponent::SetAlarm(bool bOn)
{
	const bool bWasOn = bAlarm;
	bAlarm = bOn;
	// The frame it is switched on: one high tone at once, on the click voice (the band stops dead under it).
	if (bOn && !bWasOn)
	{
		PlayPcm(PcmAlarmHigh);
	}
}

void UBarClockComponent::PlaySting()
{
	PlayPhrase(PcmSting);
}

void UBarClockComponent::PlayClap()
{
	PlayPhrase(PcmClap);
}

void UBarClockComponent::PlayThud()
{
	PlayPhrase(PcmThud);
}

void UBarClockComponent::PlayCrackle()
{
	PlayPhrase(PcmCrackle);
}
