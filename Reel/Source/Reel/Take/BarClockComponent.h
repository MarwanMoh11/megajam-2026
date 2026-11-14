// The band's bar clock (rule 2), on the director. A metronome click track stands in for the band in step 1.
// Owner: (a). Header and constructor by the header pass; bodies marked "// BODY: (a)".

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TakeTypes.h"
#include "BarClockComponent.generated.h"

class UAudioComponent;
class USoundWave;
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

	// --- step 6: the band (docs/AUDIO_SCRIPT.md) ---------------------------------------------------------------------
	// The band is a looping bed plus one-shot warning phrases, imported into /Game/Take/Audio (UTakeAudioComponent
	// finds them by name). With the bed imported, the click track goes and the clock follows the music: it starts
	// the bed at the clock's place in the loop (bar N plays loop position (N-1) mod BedBars), and each frame steers
	// TotalBeats toward the music's playback position. A sound that is not imported falls back to the synthesised
	// stand-in above, so the game runs with any subset of the audio.

	/** The band's looping bed: a whole number of bars at BPM. */
	UPROPERTY(EditAnywhere, Category = "Take|Band")
	FName BedSound = TEXT("mus_bed");

	/** The bed's length in bars (0: from its duration). */
	UPROPERTY(EditAnywhere, Category = "Take|Band")
	int32 BedBars = 0;

	/** The heist cue from the alarm: its last bar ends on the deadline (StartHeistCue). */
	UPROPERTY(EditAnywhere, Category = "Take|Band")
	FName HeistSound = TEXT("mus_heist");

	/** The heist cue's length in bars (0: from its duration). */
	UPROPERTY(EditAnywhere, Category = "Take|Band")
	int32 HeistBars = 0;

	/** The warning phrases (rule 2), one bar ahead, never ducked. */
	UPROPERTY(EditAnywhere, Category = "Take|Band")
	FName BassPhrase = TEXT("band_bass");

	UPROPERTY(EditAnywhere, Category = "Take|Band")
	FName TrumpetPhrase = TEXT("band_trumpet");

	UPROPERTY(EditAnywhere, Category = "Take|Band")
	FName DrumRollPhrase = TEXT("band_drumroll");

	/** The beam's drop beat (PlayAccent). */
	UPROPERTY(EditAnywhere, Category = "Take|Band")
	FName HiHatSplash = TEXT("band_hihat");

	/** The sounds behind PlaySting, PlayClap, PlayChime, PlayFeedback, PlayThud, PlayCrackle and the alarm. */
	UPROPERTY(EditAnywhere, Category = "Take|Band")
	FName StingSound = TEXT("mus_sting");

	UPROPERTY(EditAnywhere, Category = "Take|Band")
	FName ClapSound = TEXT("sfx_clapper");

	UPROPERTY(EditAnywhere, Category = "Take|Band")
	FName ChimeSound = TEXT("sfx_chime");

	UPROPERTY(EditAnywhere, Category = "Take|Band")
	FName FeedbackSound = TEXT("sfx_feedback");

	UPROPERTY(EditAnywhere, Category = "Take|Band")
	FName ThudSound = TEXT("sfx_thud");

	UPROPERTY(EditAnywhere, Category = "Take|Band")
	FName CrackleSound = TEXT("sfx_pyro");

	UPROPERTY(EditAnywhere, Category = "Take|Band")
	FName AlarmSound = TEXT("sfx_alarm");

	UPROPERTY(EditAnywhere, Category = "Take|Band", meta = (ClampMin = "0", ClampMax = "2"))
	float BedVolume = 0.8f;

	/** The music's level on the band's one-bar break (every BreakEveryBars). */
	UPROPERTY(EditAnywhere, Category = "Take|Band", meta = (ClampMin = "0", ClampMax = "1"))
	float BreakBedLevel = 0.2f;

	/** The music's level while a voice speaks. */
	UPROPERTY(EditAnywhere, Category = "Take|Band", meta = (ClampMin = "0", ClampMax = "1"))
	float VoiceDuckLevel = 0.45f;

	/** Fade time for the break, the knock-out and the duck. */
	UPROPERTY(EditAnywhere, Category = "Take|Band")
	float BandFadeSeconds = 0.06f;

	UPROPERTY(EditAnywhere, Category = "Take|Band", meta = (ClampMin = "0", ClampMax = "2"))
	float PhraseVolume = 1.f;

	/** The clock follows the music's playback position (off: the clock runs on game time and the music free-runs). */
	UPROPERTY(EditAnywhere, Category = "Take|Band")
	bool bClockFollowsMusic = true;

	/** Added to the music's reported position (the mixer reports what it has rendered, a buffer ahead of the ear:
	 *  a negative value moves the clock later). */
	UPROPERTY(EditAnywhere, Category = "Take|Band")
	float MusicLatencySeconds = -0.02f;

	/** The most the clock may run faster or slower than real time to catch the music, as a fraction (0.1 = 10%). */
	UPROPERTY(EditAnywhere, Category = "Take|Band", meta = (ClampMin = "0", ClampMax = "1"))
	float MaxDriftCorrection = 0.1f;

	/** Further apart than this many beats, the clock jumps to the music (forward) or waits for it (behind). */
	UPROPERTY(EditAnywhere, Category = "Take|Band")
	float SnapBeats = 0.5f;

	/** Camera C (backstage) hears the band through the flats. */
	UPROPERTY(EditAnywhere, Category = "Take|Band")
	float BackstageLowPassHz = 900.f;

	/** The blooper's tape-stop: the music falls away over this long under the tape-stop sound. */
	UPROPERTY(EditAnywhere, Category = "Take|Band")
	float TapeStopFadeSeconds = 0.35f;

	/** The alarm (ATakeDirector::StartAlarm): the heist cue plays so that its last bar ends on DeadlineBar (the bar
	 *  the clock runs out on); from the cue's first bar, or at once from the matching point if that bar is past. */
	void StartHeistCue(int32 DeadlineBar);

	/** A blooper: the music stops (under the tape-stop sound). The clock itself stops with the freeze. */
	void TapeStop();

	/** A voice is speaking: the music ducks. UTakeAudioComponent sets it every frame. */
	void SetVoiceDuck(bool bOn);

	/** The view is backstage: the music is low-passed. UTakeAudioComponent sets it every frame. */
	void SetBackstage(bool bOn);

	/** The bed is imported and playing: the clicks are off and the clock follows it. */
	bool HasBand() const { return bBandLoaded; }

private:
	/** MuteMetronome: silent until TotalBeats reaches this. */
	double MutedUntilBeats = 0.0;

	// --- step 6 state ---
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MusicAudio;

	/** The bed was found at BeginPlay. */
	bool bBandLoaded = false;
	/** What MusicAudio plays now: 0 none, 1 the bed, 2 the heist cue. */
	int32 MusicKind = 0;
	/** Clock beats at the music file's second 0 (the loop the bed started in; the heist cue's first bar). */
	double MusicBaseBeats = 0.0;
	/** The music file's length in seconds. */
	float MusicDuration = 0.f;
	/** The last reported position: seconds into the file (loops unwrapped), and when it came (FPlatformTime). */
	double MusicReportSeconds = -1.0;
	double MusicReportTime = 0.0;
	float LastRawPercent = 0.f;
	int32 MusicWraps = 0;
	/** The heist cue waits for its first bar: the clock beat it starts on (< 0: not pending). */
	double HeistStartBeats = -1.0;
	float AppliedBandLevel = -1.f;
	bool bVoiceDuck = false;
	bool bBackstage = false;
	bool bAppliedBackstage = false;
	/** Times this take the music was moved back onto the clock. Past MaxReseeks the clock stops following (the
	 *  position reports disagree with the seek) and logs it once: the music then free-runs from where it is. */
	int32 Reseeks = 0;
	bool bFollowGivenUp = false;

	void StartMusic(int32 Kind, double StartBeats);
	void StopMusic(float FadeSeconds);
	void OnMusicPercent(const UAudioComponent* Component, const USoundWave* Wave, const float Percent);
	/** The music's position in clock beats now (extrapolated from the last report), or < 0 if unknown. */
	double GetMusicBeats() const;
	void UpdateBandLevel();
	/** Plays the named sound once (2D, at Volume), true if it is imported. */
	bool PlayNamed(FName Name, float Volume);
	int32 BarsOf(float Seconds, int32 Override) const;

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
