// Owner: (a). Constructor by the header pass; bodies by (a).

#include "BarClockComponent.h"

#include "Components/AudioComponent.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundWaveProcedural.h"
#include "TakeAudio.h"
#include "TakeTypes.h"
// QA time: the warnings ask the director where Julian and the watchers are; the beam's splash plays at the beam.
#include "BeamWatcher.h"
#include "Julian.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "TakeDirector.h"
#include "Watcher.h"

namespace TakeClockA
{
	/** Step 6 phase 2: after the music starts (a take, a restore, the heist cue), its playback reports are ignored for
	 *  this long, so the last reports of the sound it replaced cannot pass for it (OnMusicPercent). */
	constexpr double MusicSettleSeconds = 0.5;

	/** One 16-bit little-endian mono sample, Sample in -1..1. */
	void AppendSample(TArray<uint8>& Out, float Sample)
	{
		const int32 Quantised = FMath::Clamp(FMath::RoundToInt(Sample * 32767.f), -32768, 32767);
		const uint16 Bits = static_cast<uint16>(static_cast<int16>(Quantised));
		Out.Add(static_cast<uint8>(Bits & 0xFF));
		Out.Add(static_cast<uint8>((Bits >> 8) & 0xFF));
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

	// Step 6: the band's music. The bed starts on the first Advance, at the clock's place in its loop.
	MusicAudio = NewObject<UAudioComponent>(OwnerActor, TEXT("TakeMusicAudio"));
	MusicAudio->bAutoActivate = false;
	MusicAudio->bAutoDestroy = false;
	MusicAudio->bAllowSpatialization = false;
	MusicAudio->bIsUISound = true;
	MusicAudio->RegisterComponent();
	MusicAudio->OnAudioPlaybackPercentNative.AddUObject(this, &UBarClockComponent::OnMusicPercent);
	UTakeAudioComponent* Audio = OwnerActor->FindComponentByClass<UTakeAudioComponent>();
	bBandLoaded = Audio && Audio->FindSound(BedSound) != nullptr;
	MusicKind = 0;
	HeistStartBeats = -1.0;
	Reseeks = 0;
	bFollowGivenUp = false;
	AppliedBandLevel = -1.f;
	bAppliedBackstage = false;

	// QA time: the warnings' memory, the beam splash's attenuation, slow time.
	for (int32 Slot = 0; Slot < NumInstrumentSlots; ++Slot)
	{
		LastWarningBeats[Slot] = -1.0;
		LastCueSound[Slot] = NAME_None;
		bInStretch[Slot] = false;
	}
	PendingCues.Reset();
	CueRng.GenerateNewSeed();
	AccentAttenuation = NewObject<USoundAttenuation>(this);
	AccentAttenuation->Attenuation.bAttenuate = true;
	AccentAttenuation->Attenuation.bSpatialize = true;
	AccentAttenuation->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
	AccentAttenuation->Attenuation.AttenuationShapeExtents = FVector(FMath::Max(0.f, AccentInnerRadius), 0.f, 0.f);
	AccentAttenuation->Attenuation.FalloffDistance = FMath::Max(1.f, AccentFalloffDistance);
	AccentAttenuation->Attenuation.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
	SlowAmount = 0.f;
	SlowMuffle = 0.f;
	bMusicHeld = false;
	bRejoinPending = false;
	AppliedLowPassHz = -1.f;
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
	if (MusicAudio)
	{
		MusicAudio->OnAudioPlaybackPercentNative.RemoveAll(this);
		MusicAudio->Stop();
		MusicAudio->DestroyComponent();
		MusicAudio = nullptr;
	}
	MusicKind = 0;
	Super::EndPlay(EndPlayReason);
}

void UBarClockComponent::Advance(float DeltaSeconds)
{
	bBeatCrossed = false;
	bBarCrossed = false;

	const int64 PerBar = FMath::Max(1, BeatsPerBar);
	const double Before = TotalBeats;
	const float Step = FMath::Max(0.f, DeltaSeconds);
	double StepBeats = static_cast<double>(Step) / static_cast<double>(GetSecondsPerBeat());

	// Step 6: the clock follows the music. Small drift is taken up by running at most MaxDriftCorrection faster or
	// slower; a bigger gap (an audio hitch, a restart) moves the music back onto the clock instead, so the clock
	// never jumps and never waits.
	// QA time: not while slow time holds the music (the clock runs on the slowed game time with the world).
	if (MusicKind != 0 && bClockFollowsMusic && !bFollowGivenUp && !bFirstAdvance && !bMusicHeld)
	{
		const double Music = GetMusicBeats();
		if (Music >= 0.0)
		{
			const double Error = Music - (TotalBeats + StepBeats);
			// QA time: on a long frame (under 10 fps: the editor throttled in the background) the music's start and
			// its reports lag by frames, so a gap there is not a real hitch: steer only, never reseek. Reseeking there
			// gave up the follow at the take's start (integration look, Nov 15: five reseeks at about 3 fps).
			const bool bLongFrame = FApp::GetDeltaTime() > 0.1;
			if (!bLongFrame && FMath::Abs(Error) > static_cast<double>(FMath::Max(0.05f, SnapBeats)))
			{
				++Reseeks;
				UE_LOG(LogTemp, Warning, TEXT("TAKE band: music %.2f beats off the clock at %s, reseek %d"), Error,
					*GetTimecode(), Reseeks);
				if (Reseeks > 4)
				{
					bFollowGivenUp = true;
					UE_LOG(LogTemp, Warning, TEXT("TAKE band: the clock stops following the music this take"));
				}
				else
				{
					StartMusic(MusicKind, MusicBaseBeats);
				}
			}
			else
			{
				const double Cap = static_cast<double>(MaxDriftCorrection) * StepBeats;
				StepBeats = FMath::Max(0.0, StepBeats + FMath::Clamp(Error, -Cap, Cap));
			}
		}
	}

	TotalBeats += StepBeats;

	// A beat line this frame: the stand-in alarm tone's cue. Nothing clicks: the metronome is gone, the band's music is
	// the only thing that keeps time, and nothing on screen moves with the beat.
	bool bBeatLine = false;

	// The take's first frame sits on a beat line (bar N, beat 1): the alarm tone's cue, without beat events.
	if (bFirstAdvance)
	{
		bFirstAdvance = false;
		// Step 6: the band comes in where the clock is (not under the alarm: the heist cue has its own start).
		if (bBandLoaded && !bAlarm && MusicKind == 0)
		{
			StartMusic(1, 0.0);
		}
		const int64 StartLine = TakeClockA::FloorBeat(Before);
		if (Before - static_cast<double>(StartLine) < 1e-6)
		{
			bBeatLine = true;
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
		bBeatLine = true;
	}

	// Step 3: the alarm replaces the click on every beat (high on beats 1 and 3, low on 2 and 4) and never rests:
	// it plays through break bars, mutes and a switched-off metronome.
	// Step 6: the heist cue comes in on its first bar; a cue that has played out leaves the clock on game time.
	if (HeistStartBeats >= 0.0 && TotalBeats >= HeistStartBeats - 1e-6)
	{
		const double CueBase = HeistStartBeats;
		HeistStartBeats = -1.0;
		StartMusic(2, CueBase);
	}
	if (MusicKind == 2 && MusicAudio && !MusicAudio->IsPlaying() && MusicReportSeconds >= 0.0)
	{
		MusicKind = 0;
	}

	// QA time: Julian's stretches near each watcher (bar lines), the phrases waiting out their few ms, and the music
	// back in after slow time, on a beat line.
	if (bBarCrossed && bSmartWarnings)
	{
		UpdateStretches();
	}
	FirePendingCues();
	if (bMusicHeld && bRejoinPending && SlowAmount <= 0.f && (bBeatCrossed || !bSlowRejoinOnBeat))
	{
		RejoinMusic();
	}
	UpdateBandLevel();

	if (bBeatLine && bAlarm)
	{
		// The stand-in alarm tone on every beat, only while there is no heist cue to play.
		UTakeAudioComponent* Audio = GetOwner() ? GetOwner()->FindComponentByClass<UTakeAudioComponent>() : nullptr;
		if (!Audio || !Audio->FindSound(HeistSound))
		{
			const bool bHigh = (GetBeatInBar() % 2) == 1;
			PlayPcm(bHigh ? PcmAlarmHigh : PcmAlarmLow);
		}
	}
}

void UBarClockComponent::SetStartBar(int32 Bar)
{
	TotalBeats = static_cast<double>(FMath::Max(1, Bar) - 1) * static_cast<double>(FMath::Max(1, BeatsPerBar));
	bBeatCrossed = false;
	bBarCrossed = false;
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

FString UBarClockComponent::GetTimecode() const
{
	return FString::Printf(TEXT("%d.%d"), GetBar(), GetBeatInBar());
}

void UBarClockComponent::PlayAccent()
{
	if (bSmartWarnings)
	{
		// QA time: a mechanical event, heard at its source: the beam nearest Julian (there is one).
		const ATakeDirector* Director = Cast<ATakeDirector>(GetOwner());
		const AJulian* Hero = Director ? Director->Julian.Get() : nullptr;
		const ABeamWatcher* Nearest = nullptr;
		double NearestDistSq = TNumericLimits<double>::Max();
		if (Director)
		{
			for (const AWatcher* EachWatcher : Director->Watchers)
			{
				const ABeamWatcher* Beam = Cast<ABeamWatcher>(EachWatcher);
				if (!Beam)
				{
					continue;
				}
				const double DistSq = Hero ? FVector::DistSquared(Beam->GetAimPoint(), Hero->GetFeetLocation()) : 0.0;
				if (!Nearest || DistSq < NearestDistSq)
				{
					Nearest = Beam;
					NearestDistSq = DistSq;
				}
			}
		}
		if (Nearest)
		{
			PlayAccentAt(Nearest->GetAimPoint());
			return;
		}
	}
	if (!PlayNamed(HiHatSplash, bSmartWarnings ? AccentVolume : PhraseVolume))
	{
		PlayPcm(PcmAccent);
	}
}

void UBarClockComponent::BuildPcm()
{
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
	if (bSmartWarnings)
	{
		// QA time (note 9): only when it tells the player something, a variant, varied (see the header).
		if (Instrument == EWatcherInstrument::HiHat)
		{
			PlayAccent();
			return;
		}
		if (Instrument == EWatcherInstrument::None)
		{
			return;
		}
		const float Level = WarningGate(Instrument);
		if (Level <= 0.f)
		{
			return;
		}
		const bool bBass = Instrument == EWatcherInstrument::Bass;
		const bool bTrumpet = Instrument == EWatcherInstrument::Trumpet;
		USoundBase* Sound = PickCue(Instrument, bBass ? BassPhrase : (bTrumpet ? TrumpetPhrase : DrumRollPhrase),
			bBass ? BassVariants : (bTrumpet ? TrumpetVariants : DrumRollVariants));
		if (Sound)
		{
			PlayVaried(Sound, PhraseVolume * Level, bBass || bTrumpet, false, FVector::ZeroVector);
		}
		else
		{
			PlayPhrase(bBass ? PcmBass : (bTrumpet ? PcmTrumpet : PcmDrumRoll));
		}
		return;
	}

	// Played whether or not the metronome is muted (PlayPcm does not look at the mute).
	// Step 6: the band's own phrase if it is imported, else the synthesised stand-in.
	switch (Instrument)
	{
	case EWatcherInstrument::Bass:
		if (!PlayNamed(BassPhrase, PhraseVolume))
		{
			PlayPhrase(PcmBass);
		}
		break;
	case EWatcherInstrument::Trumpet:
		if (!PlayNamed(TrumpetPhrase, PhraseVolume))
		{
			PlayPhrase(PcmTrumpet);
		}
		break;
	case EWatcherInstrument::DrumRoll:
		if (!PlayNamed(DrumRollPhrase, PhraseVolume))
		{
			PlayPhrase(PcmDrumRoll);
		}
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
	if (!PlayNamed(FeedbackSound, 1.f))
	{
		PlayPhrase(PcmFeedback);
	}
}

void UBarClockComponent::PlayChime()
{
	if (!PlayNamed(ChimeSound, 1.f))
	{
		PlayPhrase(PcmChime);
	}
}

void UBarClockComponent::MuteMetronome(float Bars)
{
	if (Bars <= 0.f)
	{
		return;
	}
	// Counted from the beat line the clock is on, so the mute ends exactly on a beat line (a solo bar started on a
	// bar line ends on the next bar line, whose downbeat clicks again). Step 6: rounded up to a bar line, so the
	// band knocked out mid-bar by the feedback comes back in on the next bar line, in phase (rule 2).
	const double PerBar = static_cast<double>(FMath::Max(1, BeatsPerBar));
	const double Until = FMath::FloorToDouble(TotalBeats) + static_cast<double>(Bars) * PerBar;
	MutedUntilBeats = FMath::Max(MutedUntilBeats, FMath::CeilToDouble(Until / PerBar - 1e-6) * PerBar);
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
	// The frame it is switched on: the band stops dead, and the alarm (one high tone at once as its stand-in).
	if (bOn && !bWasOn)
	{
		StopMusic(0.f);
		if (!PlayNamed(AlarmSound, 1.f))
		{
			PlayPcm(PcmAlarmHigh);
		}
	}
	if (!bOn && bWasOn)
	{
		// PRINT IT: the heist cue makes way for the sting.
		StopMusic(0.15f);
	}
}

void UBarClockComponent::PlaySting()
{
	if (!PlayNamed(StingSound, 1.f))
	{
		PlayPhrase(PcmSting);
	}
}

void UBarClockComponent::PlayClap()
{
	if (!PlayNamed(ClapSound, 1.f))
	{
		PlayPhrase(PcmClap);
	}
}

void UBarClockComponent::PlayThud()
{
	if (!PlayNamed(ThudSound, 1.f))
	{
		PlayPhrase(PcmThud);
	}
}

void UBarClockComponent::PlayCrackle()
{
	if (!PlayNamed(CrackleSound, 1.f))
	{
		PlayPhrase(PcmCrackle);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Step 6: the band
// ---------------------------------------------------------------------------------------------------------------------

bool UBarClockComponent::PlayNamed(FName Name, float Volume)
{
	UTakeAudioComponent* Audio = GetOwner() ? GetOwner()->FindComponentByClass<UTakeAudioComponent>() : nullptr;
	USoundBase* Sound = Audio ? Audio->FindSound(Name) : nullptr;
	if (!Sound)
	{
		return false;
	}
	UGameplayStatics::PlaySound2D(this, Sound, Volume);
	return true;
}

int32 UBarClockComponent::BarsOf(float Seconds, int32 Override) const
{
	if (Override > 0)
	{
		return Override;
	}
	return FMath::Max(1, FMath::RoundToInt(Seconds / FMath::Max(0.01f, GetSecondsPerBar())));
}

void UBarClockComponent::StartMusic(int32 Kind, double BaseBeats)
{
	// QA Nov 15 evening: the script take's music is UTakeAudioComponent's scene mix. The band's bed under the script
	// doubled Scene 1's jazz, ran under Scene 5's score, dipped every eighth bar and died at the first retake.
	if (const ATakeDirector* Director = Cast<ATakeDirector>(GetOwner()))
	{
		if (Director->bScriptDriven)
		{
			MusicKind = 0;
			if (MusicAudio && MusicAudio->IsPlaying())
			{
				MusicAudio->Stop();
			}
			return;
		}
	}
	UTakeAudioComponent* Audio = GetOwner() ? GetOwner()->FindComponentByClass<UTakeAudioComponent>() : nullptr;
	USoundWave* Wave = Audio ? Cast<USoundWave>(Audio->FindSound(Kind == 2 ? HeistSound : BedSound)) : nullptr;
	if (!MusicAudio || !Wave || Wave->Duration <= 0.f)
	{
		MusicKind = 0;
		return;
	}
	const double PerBar = static_cast<double>(FMath::Max(1, BeatsPerBar));
	const double SecondsPerBeat = static_cast<double>(GetSecondsPerBeat());
	double StartSeconds = 0.0;
	if (Kind == 1)
	{
		// Bar N of the take plays the loop at bar ((N - 1) mod BedBars): the loop this beat is in starts at file second 0.
		const double LoopBeats = static_cast<double>(BarsOf(Wave->Duration, BedBars)) * PerBar;
		MusicBaseBeats = FMath::FloorToDouble(TotalBeats / LoopBeats) * LoopBeats;
		StartSeconds = (TotalBeats - MusicBaseBeats) * SecondsPerBeat;
		Wave->bLooping = true;
	}
	else
	{
		MusicBaseBeats = BaseBeats;
		StartSeconds = FMath::Max(0.0, (TotalBeats - BaseBeats) * SecondsPerBeat);
		Wave->bLooping = false;
		if (StartSeconds >= static_cast<double>(Wave->Duration))
		{
			MusicKind = 0;
			return;
		}
	}
	MusicKind = Kind;
	MusicDuration = Wave->Duration;
	MusicReportSeconds = -1.0;
	MusicReportTime = FPlatformTime::Seconds();	// reports are ignored for MusicSettleSeconds from here (OnMusicPercent)
	MusicWraps = 0;
	LastRawPercent = -1.f;	// not primed: the first moving report after the settle only primes
	MusicAudio->Stop();
	MusicAudio->SetSound(Wave);
	MusicAudio->SetVolumeMultiplier(1.f);
	// QA time: in from silence over the band fade, at its level now (it may be muffled by slow time), rather than at
	// full level for a moment and then down.
	const float StartLevel = ComputeBandLevel();
	MusicAudio->FadeIn(FMath::Max(0.01f, BandFadeSeconds), FMath::Max(0.f, StartLevel), static_cast<float>(StartSeconds));
	AppliedBandLevel = StartLevel;
	AppliedLowPassHz = -1.f;
	bAppliedBackstage = !bBackstage;	// the filter is re-applied below
	UpdateBandLevel();
}

void UBarClockComponent::StopMusic(float FadeSeconds)
{
	MusicKind = 0;
	HeistStartBeats = -1.0;
	MusicReportSeconds = -1.0;
	if (!MusicAudio || !MusicAudio->IsPlaying())
	{
		return;
	}
	if (FadeSeconds > 0.f)
	{
		MusicAudio->FadeOut(FadeSeconds, 0.f);
	}
	else
	{
		MusicAudio->Stop();
	}
}

void UBarClockComponent::OnMusicPercent(const UAudioComponent* Component, const USoundWave* Wave, const float Percent)
{
	if (Component != MusicAudio || MusicKind == 0)
	{
		return;
	}
	// Step 6 phase 2: a sound that was just stopped (the bed as the alarm cuts to the heist cue, or the bed before a
	// restore restarts it) reports its last position for a few more audio buffers, and a source that has not started
	// yet repeats its seek point. Read as the music, either sent the clock into a burst of reseeks until it gave up.
	// So: only the current sound counts; nothing counts for MusicSettleSeconds after StartMusic (which sets
	// MusicReportTime; no report has been taken while MusicReportSeconds < 0); a report that has not moved since the
	// last one is ignored; and the first moving report only primes the next comparison.
	if (Wave != Cast<USoundWave>(MusicAudio->Sound))
	{
		return;
	}
	const double Now = FPlatformTime::Seconds();
	if (MusicReportSeconds < 0.0 && Now - MusicReportTime < TakeClockA::MusicSettleSeconds)
	{
		return;
	}
	if (Percent == LastRawPercent)
	{
		return;
	}
	// The mixer counts frames from the file's start (the seek included). A looping file may count on past 1 or
	// wrap to 0: a drop of more than half the file between two taken reports is a wrap.
	if (MusicReportSeconds >= 0.0 && MusicKind == 1 && Percent < LastRawPercent - 0.5f)
	{
		++MusicWraps;
	}
	const bool bPrimed = LastRawPercent >= 0.f;
	LastRawPercent = Percent;
	if (!bPrimed)
	{
		return;
	}
	MusicReportSeconds = (static_cast<double>(MusicWraps) + static_cast<double>(Percent)) * static_cast<double>(MusicDuration);
	MusicReportTime = Now;
}

double UBarClockComponent::GetMusicBeats() const
{
	if (MusicReportSeconds < 0.0)
	{
		return -1.0;
	}
	const double Age = FPlatformTime::Seconds() - MusicReportTime;
	if (Age > 0.5)
	{
		return -1.0;	// no news (a silent or stalled source): the clock runs on game time
	}
	const double Seconds = MusicReportSeconds + Age + static_cast<double>(MusicLatencySeconds);
	return MusicBaseBeats + Seconds / static_cast<double>(GetSecondsPerBeat());
}

float UBarClockComponent::ComputeBandLevel() const
{
	float Level = 1.f;
	if (MusicKind == 1)
	{
		if (TotalBeats < MutedUntilBeats)
		{
			Level = 0.f;	// the feedback's knock-out, an arriving watcher's solo bar
		}
		else if (IsBreakBar(GetBar()))
		{
			Level = BreakBedLevel;
		}
	}
	if (bVoiceDuck)
	{
		Level *= VoiceDuckLevel;
	}
	if (SlowMuffle > 0.f)
	{
		Level *= FMath::Lerp(1.f, FMath::Clamp(SlowBedLevel, 0.f, 1.f), FMath::Clamp(SlowMuffle, 0.f, 1.f));
	}
	return Level * BedVolume;
}

void UBarClockComponent::UpdateBandLevel()
{
	if (!MusicAudio || MusicKind == 0)
	{
		return;
	}
	const float Level = ComputeBandLevel();
	if (FMath::Abs(Level - AppliedBandLevel) > 0.001f)
	{
		MusicAudio->AdjustVolume(FMath::Max(0.f, BandFadeSeconds), FMath::Max(0.f, Level));
		AppliedBandLevel = Level;
	}
	// The low-pass: backstage (through the flats) and slow time (far away), the lower of the two; 0 = off.
	// QA time: slow time sweeps it on a log scale from 20 kHz down to SlowLowPassHz.
	float WantHz = 0.f;
	if (bBackstage)
	{
		WantHz = FMath::Max(20.f, BackstageLowPassHz);
	}
	if (SlowMuffle > 0.001f)
	{
		const float Alpha = FMath::Clamp(SlowMuffle, 0.f, 1.f);
		const float SlowHz = FMath::Pow(2.f, FMath::Lerp(FMath::Log2(20000.f), FMath::Log2(FMath::Max(100.f, SlowLowPassHz)), Alpha));
		WantHz = WantHz > 0.f ? FMath::Min(WantHz, SlowHz) : SlowHz;
	}
	const bool bOnChanged = (WantHz > 0.f) != (AppliedLowPassHz > 0.f);
	const bool bHzChanged = WantHz > 0.f && FMath::Abs(WantHz - AppliedLowPassHz) > 0.01f * FMath::Max(1.f, AppliedLowPassHz);
	if (bOnChanged || bHzChanged || bBackstage != bAppliedBackstage)
	{
		MusicAudio->SetLowPassFilterEnabled(WantHz > 0.f);
		if (WantHz > 0.f)
		{
			MusicAudio->SetLowPassFilterFrequency(WantHz);
		}
		AppliedLowPassHz = WantHz;
		bAppliedBackstage = bBackstage;
	}
}

void UBarClockComponent::StartHeistCue(int32 DeadlineBar)
{
	UTakeAudioComponent* Audio = GetOwner() ? GetOwner()->FindComponentByClass<UTakeAudioComponent>() : nullptr;
	USoundWave* Wave = Audio ? Cast<USoundWave>(Audio->FindSound(HeistSound)) : nullptr;
	if (!Wave)
	{
		return;
	}
	// The cue's last bar is the bar before DeadlineBar (the clock runs out as DeadlineBar begins).
	const int32 FirstBar = DeadlineBar - BarsOf(Wave->Duration, HeistBars);
	const double StartBeats = static_cast<double>(FirstBar - 1) * static_cast<double>(FMath::Max(1, BeatsPerBar));
	if (TotalBeats >= StartBeats)
	{
		StartMusic(2, StartBeats);
	}
	else
	{
		HeistStartBeats = StartBeats;
	}
}

void UBarClockComponent::TapeStop()
{
	StopMusic(FMath::Max(0.f, TapeStopFadeSeconds));
}

void UBarClockComponent::SetVoiceDuck(bool bOn)
{
	if (bVoiceDuck != bOn)
	{
		bVoiceDuck = bOn;
		UpdateBandLevel();
	}
}

void UBarClockComponent::SetBackstage(bool bOn)
{
	if (bBackstage != bOn)
	{
		bBackstage = bOn;
		UpdateBandLevel();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// QA time (Nov 14): warnings that tell the player something (note 9), and the music under slow time (note 2)
// ---------------------------------------------------------------------------------------------------------------------

float UBarClockComponent::RangeFor(EWatcherInstrument Instrument) const
{
	switch (Instrument)
	{
	case EWatcherInstrument::Bass:
		return BassRange;
	case EWatcherInstrument::Trumpet:
		return TrumpetRange;
	case EWatcherInstrument::DrumRoll:
		return DrumRollRange;
	default:
		return 0.f;
	}
}

float UBarClockComponent::WarningGate(EWatcherInstrument Instrument)
{
	const int32 Slot = static_cast<int32>(Instrument);
	if (Slot <= 0 || Slot >= NumInstrumentSlots)
	{
		return 0.f;
	}
	const ATakeDirector* Director = Cast<ATakeDirector>(GetOwner());
	const AJulian* Hero = Director ? Director->Julian.Get() : nullptr;
	if (!Hero)
	{
		return 1.f;	// nothing to measure against: as before
	}
	const FVector Feet = Hero->GetFeetLocation();
	const int32 NextBar = GetBar() + 1;
	bool bArrival = false;
	float Nearest = TNumericLimits<float>::Max();
	for (const AWatcher* EachWatcher : Director->Watchers)
	{
		if (!EachWatcher || EachWatcher->Instrument != Instrument)
		{
			continue;
		}
		if (EachWatcher->GetArriveBar() > 0 && EachWatcher->GetArriveBar() == NextBar)
		{
			bArrival = true;
		}
		if (EachWatcher->HasArrived())
		{
			Nearest = FMath::Min(Nearest, EachWatcher->DistanceToPoint(Feet));
		}
	}

	const double Now = TotalBeats;
	const double SecondsPerBeat = static_cast<double>(GetSecondsPerBeat());
	if (bArrival)
	{
		// News: a watcher walks in on the next bar line. Always, in full (its solo bar).
		LastWarningBeats[Slot] = Now;
		return 1.f;
	}

	const float Range = RangeFor(Instrument);
	if (Range > 0.f && Nearest > Range)
	{
		UE_LOG(LogTemp, Log, TEXT("TAKE band: warning %d held at %s (out of range: %.0f cm)"), Slot, *GetTimecode(), Nearest);
		return 0.f;	// it does not matter to Julian where he is
	}
	if (!bInStretch[Slot])
	{
		// The first turn since he came into its range: at once, in full.
		bInStretch[Slot] = true;
		LastWarningBeats[Slot] = Now;
		UE_LOG(LogTemp, Log, TEXT("TAKE band: warning %d played at %s (first in range: %.0f cm)"), Slot, *GetTimecode(), Nearest);
		return 1.f;
	}
	const float Cooldown = Nearest <= CloseRange ? CloseCooldownSeconds : WarningCooldownSeconds;
	const double Since = LastWarningBeats[Slot] < 0.0 ? TNumericLimits<double>::Max() : (Now - LastWarningBeats[Slot]) * SecondsPerBeat;
	if (Since < static_cast<double>(Cooldown) - 0.01)
	{
		UE_LOG(LogTemp, Log, TEXT("TAKE band: warning %d held at %s (cooldown: %.1f of %.1f s)"), Slot, *GetTimecode(), Since, Cooldown);
		return 0.f;	// he heard it a moment ago
	}
	LastWarningBeats[Slot] = Now;
	UE_LOG(LogTemp, Log, TEXT("TAKE band: warning %d played at %s (repeat after %.1f s, %.0f cm)"), Slot, *GetTimecode(), Since, Nearest);
	return FMath::Clamp(RepeatLevel, 0.f, 1.f);
}

void UBarClockComponent::UpdateStretches()
{
	const ATakeDirector* Director = Cast<ATakeDirector>(GetOwner());
	const AJulian* Hero = Director ? Director->Julian.Get() : nullptr;
	if (!Hero)
	{
		return;
	}
	const FVector Feet = Hero->GetFeetLocation();
	float Nearest[NumInstrumentSlots];
	for (int32 Slot = 0; Slot < NumInstrumentSlots; ++Slot)
	{
		Nearest[Slot] = TNumericLimits<float>::Max();
	}
	for (const AWatcher* EachWatcher : Director->Watchers)
	{
		const int32 Slot = EachWatcher ? static_cast<int32>(EachWatcher->Instrument) : 0;
		if (Slot > 0 && Slot < NumInstrumentSlots && EachWatcher->HasArrived())
		{
			Nearest[Slot] = FMath::Min(Nearest[Slot], EachWatcher->DistanceToPoint(Feet));
		}
	}
	for (int32 Slot = 1; Slot < NumInstrumentSlots; ++Slot)
	{
		const float Range = RangeFor(static_cast<EWatcherInstrument>(Slot));
		if (bInStretch[Slot] && Range > 0.f && Nearest[Slot] > Range * FMath::Max(1.f, RangeExitFactor))
		{
			bInStretch[Slot] = false;
		}
	}
}

USoundBase* UBarClockComponent::PickCue(EWatcherInstrument Instrument, FName Phrase, const TArray<FName>& Variants)
{
	UTakeAudioComponent* Audio = GetOwner() ? GetOwner()->FindComponentByClass<UTakeAudioComponent>() : nullptr;
	if (!Audio)
	{
		return nullptr;
	}
	TArray<FName, TInlineAllocator<8>> Names;
	TArray<USoundBase*, TInlineAllocator<8>> Sounds;
	const auto Consider = [&Names, &Sounds, Audio](FName Name)
	{
		if (USoundBase* Sound = Audio->FindSound(Name))
		{
			Names.Add(Name);
			Sounds.Add(Sound);
		}
	};
	Consider(Phrase);
	for (const FName& Name : Variants)
	{
		if (Name != Phrase)
		{
			Consider(Name);
		}
	}
	if (Sounds.Num() == 0)
	{
		return nullptr;
	}
	const int32 Slot = FMath::Clamp(static_cast<int32>(Instrument), 0, NumInstrumentSlots - 1);
	if (Sounds.Num() > 1)
	{
		// Never the one it played last.
		const int32 Last = Names.IndexOfByKey(LastCueSound[Slot]);
		if (Last != INDEX_NONE)
		{
			Names.RemoveAt(Last);
			Sounds.RemoveAt(Last);
		}
	}
	const int32 Pick = CueRng.RandRange(0, Sounds.Num() - 1);
	LastCueSound[Slot] = Names[Pick];
	return Sounds[Pick];
}

void UBarClockComponent::PlayVaried(USoundBase* Sound, float Volume, bool bPitched, bool bAtLocation, const FVector& Where)
{
	if (!Sound)
	{
		return;
	}
	FTakePendingCue Cue;
	Cue.Sound = Sound;
	Cue.Volume = FMath::Max(0.f, Volume) * FMath::Pow(10.f, -CueRng.FRandRange(0.f, FMath::Max(0.f, VolumeJitterDb)) / 20.f);
	const float Cents = CueRng.FRandRange(-1.f, 1.f) * FMath::Max(0.f, bPitched ? PitchedJitterCents : UnpitchedJitterCents);
	Cue.Pitch = FMath::Pow(2.f, Cents / 1200.f);
	Cue.bAtLocation = bAtLocation;
	Cue.Location = Where;
	const float Late = CueRng.FRandRange(0.f, FMath::Clamp(TimingJitterSeconds, 0.f, 0.1f));
	Cue.AtBeats = TotalBeats + static_cast<double>(Late) / static_cast<double>(GetSecondsPerBeat());
	PendingCues.Add(Cue);
	if (Late < 0.004f)
	{
		FirePendingCues();	// nothing to wait for: this frame
	}
}

void UBarClockComponent::FirePendingCues()
{
	for (int32 Index = 0; Index < PendingCues.Num(); )
	{
		const FTakePendingCue& Cue = PendingCues[Index];
		// Due, or stale (more than a beat out: the clock was moved under it).
		if (Cue.AtBeats > TotalBeats + 1e-6 && Cue.AtBeats - TotalBeats < 1.0)
		{
			++Index;
			continue;
		}
		if (USoundBase* Sound = Cue.Sound.Get())
		{
			if (Cue.bAtLocation)
			{
				UGameplayStatics::PlaySoundAtLocation(this, Sound, Cue.Location, FRotator::ZeroRotator, Cue.Volume, Cue.Pitch,
					0.f, AccentAttenuation);
			}
			else
			{
				UGameplayStatics::PlaySound2D(this, Sound, Cue.Volume, Cue.Pitch);
			}
		}
		PendingCues.RemoveAt(Index);
	}
}

void UBarClockComponent::PlayAccentAt(const FVector& Where)
{
	USoundBase* Sound = PickCue(EWatcherInstrument::HiHat, HiHatSplash, HiHatVariants);
	if (!Sound)
	{
		PlayPcm(PcmAccent);	// the stand-in has no place in the world
		return;
	}
	if (AccentAttenuation)
	{
		AccentAttenuation->Attenuation.AttenuationShapeExtents = FVector(FMath::Max(0.f, AccentInnerRadius), 0.f, 0.f);
		AccentAttenuation->Attenuation.FalloffDistance = FMath::Max(1.f, AccentFalloffDistance);
	}
	PlayVaried(Sound, AccentVolume, false, true, Where);
}

void UBarClockComponent::SetSlowAmount(float Amount, float RealDeltaSeconds)
{
	SlowAmount = FMath::Clamp(Amount, 0.f, 1.f);
	if (SlowAmount > 0.f)
	{
		// Slowing (or slowed): the music is held, under the filter, and the clock stops following it.
		if (!bMusicHeld && MusicKind != 0)
		{
			UE_LOG(LogTemp, Log, TEXT("TAKE band: music held for slow time at %s"), *GetTimecode());
		}
		bMusicHeld = bMusicHeld || MusicKind != 0;
		bRejoinPending = false;
		SlowMuffle = FMath::Max(SlowMuffle, SlowAmount);
	}
	else if (bMusicHeld)
	{
		// Time is back: the music comes in again where the clock is, on the next beat line (Advance) or now.
		bRejoinPending = true;
		if (MusicKind == 0 || !MusicAudio)
		{
			bMusicHeld = false;	// nothing playing to bring back (a blooper stopped it; the heist cue starts itself)
			bRejoinPending = false;
		}
		else if (!bSlowRejoinOnBeat)
		{
			RejoinMusic();
		}
	}
	else if (SlowMuffle > 0.f)
	{
		// Back in: the level and the filter open.
		SlowMuffle = SlowRejoinFadeSeconds > 0.f
			? FMath::Max(0.f, SlowMuffle - FMath::Max(0.f, RealDeltaSeconds) / SlowRejoinFadeSeconds) : 0.f;
	}
	UpdateBandLevel();
}

void UBarClockComponent::RejoinMusic()
{
	bMusicHeld = false;
	bRejoinPending = false;
	if (MusicKind == 0 || !MusicAudio)
	{
		return;
	}
	// The music played on at its own speed under the slow while the clock went slow with the world. It restarts
	// where the clock is (as a restore does), still muffled: SetSlowAmount then opens it over SlowRejoinFadeSeconds.
	const int32 Kind = MusicKind;
	StartMusic(Kind, MusicBaseBeats);
	// A music start is corrected once in its first bar (STATUS, step 6 phase 2): that one does not count toward
	// giving up the follow, or a few slows in a take would leave the clock on game time.
	Reseeks = FMath::Max(0, Reseeks - 1);
	UE_LOG(LogTemp, Log, TEXT("TAKE band: music back in at %s after slow time"), *GetTimecode());
}
