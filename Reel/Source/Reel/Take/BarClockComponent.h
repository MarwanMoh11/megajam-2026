// The band's bar clock (rule 2), on the director. A metronome click track stands in for the band in step 1.
// Owner: (a). Header and constructor by the header pass; bodies marked "// BODY: (a)".

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
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

private:
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
};
