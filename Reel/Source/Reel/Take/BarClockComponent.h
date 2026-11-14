// The band's bar clock (rule 2), on the director. A metronome click track stands in for the band in step 1.
// Owner: (a). Header and constructor by the header pass; bodies marked "// BODY: (a)".

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TakeTypes.h"
#include "BarClockComponent.generated.h"

class UAudioComponent;
class USoundWaveProcedural;

/**
 * Bars and beats are 1-based for everyone: the take starts on bar 1, beat 1 (timecode "1.1"). At 120 BPM a beat
 * is 0.5 s and a bar (4 beats) is 2 s. Time advances only when the director calls Advance (never during the
 * blooper freeze). Everything that "keeps time" (the guard's turns, the beam's drop) reads this clock.
 * The component does not tick by itself.
 */
UCLASS(ClassGroup = (Take), meta = (BlueprintSpawnableComponent))
class REEL_API UBarClockComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBarClockComponent();

	/** Builds the click sounds (PCM in code) and the audio component that plays them. */
	virtual void BeginPlay() override;	// BODY: (a)
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;	// BODY: (a)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Clock")
	float BPM = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Clock")
	int32 BeatsPerBar = 4;

	/** The bar the take starts on (1 = the opening). SetStartBar overrides it for checkpoint restores later. */
	UPROPERTY(EditAnywhere, Category = "Take|Clock")
	int32 StartBar = 1;

	/** The metronome click on every beat (higher pitch on the downbeat). */
	UPROPERTY(EditAnywhere, Category = "Take|Clock")
	bool bMetronome = true;

	UPROPERTY(EditAnywhere, Category = "Take|Clock", meta = (ClampMin = "0", ClampMax = "1"))
	float ClickVolume = 0.5f;

	/** How long the visual pulse takes to fall from 1 (on the beat) to 0. */
	UPROPERTY(EditAnywhere, Category = "Take|Clock")
	float PulseDecaySeconds = 0.2f;

	/** Step 1 (director). Moves time on by DeltaSeconds; sets the beat/bar-crossed flags for this frame (true only
	 *  on the frame a beat or bar line is crossed); restarts the pulse and plays the click on each beat crossed. */
	void Advance(float DeltaSeconds);	// BODY: (a)

	/** Jumps the clock to the start of Bar (beat 1, phase 0) without firing beat events. For checkpoint restores. */
	void SetStartBar(int32 Bar);	// BODY: (a)

	/** Beats since bar 1 beat 1, fractional (0 at the start of the take). */
	double GetTotalBeats() const;	// BODY: (a)

	/** The current bar, 1-based. */
	int32 GetBar() const;	// BODY: (a)

	/** The current beat in the bar, 1..BeatsPerBar. */
	int32 GetBeatInBar() const;	// BODY: (a)

	/** 0..1 through the current beat. */
	float GetBeatPhase() const;	// BODY: (a)

	/** True only on the frame a beat line was crossed (includes bar lines). */
	bool BeatCrossedThisFrame() const;	// BODY: (a)

	/** True only on the frame a bar line (beat 1) was crossed. */
	bool BarCrossedThisFrame() const;	// BODY: (a)

	float GetSecondsPerBeat() const;	// BODY: (a)
	float GetSecondsPerBar() const;	// BODY: (a)

	/** 1 on the frame of a beat, falling to 0 over PulseDecaySeconds: the frames' pulse and the HUD's beat dot. */
	float GetPulse() const;	// BODY: (a)

	/** "bar.beat", e.g. "12.3": the HUD timecode and the TAKE_LOG stamp. */
	FString GetTimecode() const;	// BODY: (a)

	/** Plays the beam's hi-hat splash (a bright noise burst) at once. Called by ABeamWatcher on its drop beat. */
	void PlayAccent();	// BODY: (a)

	/** Plays one metronome click at once (bDownbeat: higher pitch). Advance calls it; exposed for tests. */
	void PlayClick(bool bDownbeat);	// BODY: (a)

	// --- step 2 (BODY: D) --------------------------------------------------------------------------------------------

	/** Plays this instrument's warning at once: a short phrase of its own pitch, clearly different from the clicks
	 *  and from each other (rule 2; stand-ins until the audio step). Bass: two low plucks (about 80 and 100 Hz).
	 *  Trumpet: two mid notes (about 520 and 660 Hz) with a brighter tone. DrumRoll: about 8 fast noise taps rising
	 *  in level. HiHat: PlayAccent. None: nothing. Each phrase lasts under one beat. Played whether or not the
	 *  metronome is muted. TAKE_LOG("WARNING <instrument>") is the caller's. */
	void PlayWarning(EWatcherInstrument Instrument);	// BODY: D

	/** The sound desk's feedback: a rising squeal, about 0.6 s. */
	void PlayFeedback();	// BODY: D

	/** A short bright two-note chime: the keycard, a checkpoint, an arrival's downbeat. */
	void PlayChime();	// BODY: D

	/** The metronome is silent for the next Bars bars from now (the feedback's drop-out; an arriving watcher's solo
	 *  bar). The clock, the beat events and the pulse carry on. A longer mute already running is kept. */
	void MuteMetronome(float Bars);	// BODY: D

	/** The band's one-bar break (rule 2): true for bars that are multiples of BreakEveryBars (8, 16, 24...). The
	 *  metronome does not click during a break bar. */
	bool IsBreakBar(int32 Bar) const;	// BODY: D

	/** The clicks are silent right now (a break bar, or MuteMetronome is running). The HUD shows "BAND BREAK". */
	bool IsMetronomeMuted() const;	// BODY: D

	// --- step 3 (BODY: D) ------------------------------------------------------------------------------------------

	/** The alarm (ATakeDirector::StartAlarm): the band stops dead. While on, the metronome's click on every beat is
	 *  replaced by an alarm tone, high on beats 1 and 3 and low on 2 and 4 (two-tone, about 880 and 660 Hz, harsh),
	 *  at AlarmVolume, and it plays through break bars and mutes. The clock, the beat events and the pulse carry on.
	 *  On the frame it is switched on one high tone plays at once. */
	void SetAlarm(bool bOn);

	bool IsAlarm() const { return bAlarm; }

	/** PRINT IT: a bright rising four-note sting, about 0.9 s. */
	void PlaySting();

	/** The clapper: one sharp dry clack (a 30 ms noise burst with a hard attack). PRINT IT and the title card. */
	void PlayClap();

	/** The curtain or a shutter landing: a low thud (about 60 Hz, 0.25 s). */
	void PlayThud();

	/** The pyro: a 0.5 s crackle of noise bursts. */
	void PlayCrackle();

	UPROPERTY(EditAnywhere, Category = "Take|Clock", meta = (ClampMin = "0", ClampMax = "1"))
	float AlarmVolume = 0.6f;

	/** The band takes a one-bar break every this many bars (0 = never). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Clock")
	int32 BreakEveryBars = 8;

	/** Level of the warning phrases against the clicks. */
	UPROPERTY(EditAnywhere, Category = "Take|Clock", meta = (ClampMin = "0", ClampMax = "1"))
	float WarningVolume = 0.8f;

private:
	/** MuteMetronome: silent until TotalBeats reaches this. */
	double MutedUntilBeats = 0.0;

	/** The warning phrases and effects, built in BuildPcm. */
	TArray<uint8> PcmBass;
	TArray<uint8> PcmTrumpet;
	TArray<uint8> PcmDrumRoll;
	TArray<uint8> PcmFeedback;
	TArray<uint8> PcmChime;

	/** Step 3, built in BuildPcm. */
	TArray<uint8> PcmAlarmHigh;
	TArray<uint8> PcmAlarmLow;
	TArray<uint8> PcmSting;
	TArray<uint8> PcmClap;
	TArray<uint8> PcmThud;
	TArray<uint8> PcmCrackle;

	bool bAlarm = false;

	/** Fills the PCM buffers below (16-bit mono at SampleRate): short decaying sine clicks and a noise burst. */
	void BuildPcm();	// BODY: (a)

	/** Sends one PCM buffer to the speakers now (e.g. queue into ClickWave playing on ClickAudio, or a fresh
	 *  one-shot USoundWaveProcedural per sound: builder's choice). */
	void PlayPcm(const TArray<uint8>& Pcm);	// BODY: (a)

	/** The first Advance after BeginPlay or SetStartBar: the beat the take starts on clicks and pulses (no events). */
	bool bFirstAdvance = true;

	/** SetStartBar ran (BeginPlay then keeps its bar instead of StartBar). */
	bool bStartBarSet = false;

	double TotalBeats = 0.0;
	bool bBeatCrossed = false;
	bool bBarCrossed = false;
	float PulseAge = 1000.f;

	int32 SampleRate = 48000;
	TArray<uint8> PcmBeat;
	TArray<uint8> PcmDownbeat;
	TArray<uint8> PcmAccent;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ClickAudio;

	UPROPERTY(Transient)
	TObjectPtr<USoundWaveProcedural> ClickWave;

	/** Voices for the warning phrases, the chime and the feedback (review, Nov 14): one queue played everything in
	 *  turn, so a phrase pushed the next click late and two warnings on one bar line came one after the other. Each
	 *  phrase goes to the voice with the least waiting; the clicks keep ClickWave to themselves. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> PhraseAudios;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USoundWaveProcedural>> PhraseWaves;

	/** How many phrase voices BeginPlay makes. */
	int32 PhraseVoices = 3;

	/** Queues one phrase on the phrase voice with the least audio waiting, at WarningVolume. */
	void PlayPhrase(const TArray<uint8>& Pcm);
};
