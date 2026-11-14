// Step 6, audio: the voices, the sound effects and the composure loops, on the director. The band (the bed, the
// warning phrases, the heist cue) is the bar clock's (UBarClockComponent), because the clock follows the music.
//
// Every sound is a USoundWave in /Game/Take/Audio named like its file in C:\Jam\Audio (tools/audio). A sound that is
// not imported yet is skipped, and the clock's synthesised stand-in plays where there is one, so the game runs with
// any subset imported. The voice table (which file says which subtitle) is generated from tools/audio/manifest.py
// into TakeAudioLines.inl; docs/AUDIO_SCRIPT.md is the script.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TakeAudio.generated.h"

class UAudioComponent;
class USoundBase;
class ATakeDirector;

UCLASS(ClassGroup = (Take), meta = (BlueprintSpawnableComponent))
class REEL_API UTakeAudioComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTakeAudioComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** The director's audio, or null (no director in this world). */
	static UTakeAudioComponent* Get(const UObject* WorldContextObject);

	/** Where the sounds are imported. A sound is <AudioFolder>/<name>.<name>. */
	UPROPERTY(EditAnywhere, Category = "Take|Audio")
	FString AudioFolder = TEXT("/Game/Take/Audio");

	UPROPERTY(EditAnywhere, Category = "Take|Audio", meta = (ClampMin = "0", ClampMax = "2"))
	float VoiceVolume = 1.f;

	UPROPERTY(EditAnywhere, Category = "Take|Audio", meta = (ClampMin = "0", ClampMax = "2"))
	float SfxVolume = 1.f;

	/** A voiced subtitle holds at least its voice's length plus this. */
	UPROPERTY(EditAnywhere, Category = "Take|Audio")
	float SubtitleVoicePad = 0.35f;

	/** The cut's switcher click, on the frame of the press. */
	UPROPERTY(EditAnywhere, Category = "Take|Audio")
	FName SwitcherCue = TEXT("sfx_switcher_click");

	/** A blooper: these play on the frame the freeze starts (the music stops under the tape-stop). */
	UPROPERTY(EditAnywhere, Category = "Take|Audio")
	TArray<FName> BlooperCues = { TEXT("sfx_tape_stop"), TEXT("sfx_film_burn"), TEXT("sfx_buzzer") };

	/** The director's "Cut!" comes this long after the freeze starts (the blooper slate draws its subtitle). */
	UPROPERTY(EditAnywhere, Category = "Take|Audio")
	float BlooperCutDelay = 0.3f;

	/** The take after a blooper starts with the rewind whirr; every take starts with the clapper. */
	UPROPERTY(EditAnywhere, Category = "Take|Audio")
	FName RewindCue = TEXT("sfx_rewind");

	/** The alarm's shutters rolling down (once, at the alarm). */
	UPROPERTY(EditAnywhere, Category = "Take|Audio")
	FName ShuttersCue = TEXT("sfx_shutters");

	/** Composure (STATUS, Audio): a heartbeat while a watcher holds him, and a ringing that rises as it falls. */
	UPROPERTY(EditAnywhere, Category = "Take|Audio")
	FName HeartbeatLoop = TEXT("sfx_heartbeat");

	UPROPERTY(EditAnywhere, Category = "Take|Audio")
	FName RingingLoop = TEXT("sfx_ringing");

	UPROPERTY(EditAnywhere, Category = "Take|Audio", meta = (ClampMin = "0", ClampMax = "2"))
	float HeartbeatVolume = 0.8f;

	UPROPERTY(EditAnywhere, Category = "Take|Audio", meta = (ClampMin = "0", ClampMax = "2"))
	float RingingVolume = 0.5f;

	/** The ringing starts below this composure and is at RingingVolume at 0. */
	UPROPERTY(EditAnywhere, Category = "Take|Audio")
	float RingingBelow = 60.f;

	/** How fast the heartbeat and the ringing fade in and out (volume units a second). */
	UPROPERTY(EditAnywhere, Category = "Take|Audio")
	float LoopFadePerSecond = 4.f;

	/** The making-of camera C (Vex, backstage) hears the band through the flats (STATUS, Audio: low-pass only here). */
	UPROPERTY(EditAnywhere, Category = "Take|Audio")
	bool bBackstageLowPass = true;

	/** The sound <AudioFolder>/<Name>, loaded once and cached; null if it is not imported. */
	USoundBase* FindSound(FName Name);

	/** Plays a sound effect now, 2D. False if it is not imported (the caller's stand-in, if any, then plays). */
	bool PlayCue(FName Name, float Volume = 1.f);

	/** The HUD calls this as a subtitle comes on screen: plays its voice and returns its length in seconds (0: no
	 *  voice). A line with recorded variants plays the next one in turn and changes InOutLine to its words. */
	float SpeakSubtitle(const FString& Speaker, FString& InOutLine);

	/** A voice without a subtitle (the blooper slate's and PRINT IT's "Cut!" and "Print it!" draw their own). */
	float SpeakNow(const FString& Speaker, const FString& Line);

	/** A line the script ties to an event ("@<event>" in the table, e.g. a trigger or a device's name): flashed on
	 *  the director as a subtitle, which voices it. False if the table has no line for it. */
	bool SayEvent(const FString& Event);

	void StopVoice();
	bool IsVoicePlaying() const;

	// --- the director's calls (one line at each event site) ---------------------------------------------------------

	/** A cut was taken (the press frame). */
	void OnCut();

	/** The blooper freeze started: the music's tape-stop, the film burn, the buzzer, "Cut!". */
	void OnBlooper();

	/** A take starts rolling: the clapper (and before it the rewind whirr, after a blooper). */
	void OnTakeStart(bool bAfterBlooper);

	/** A scene trigger fired (TakeTriggers): its line, if the script has one ("@trigger_<Name>"). */
	void OnTrigger(FName Trigger);

	/** A device fired: its sound, and its crew line if the device has no FireLine of its own ("@fire_<NAME>"). */
	void OnDeviceFired(FName FireSound, const FString& DeviceName, bool bHasFireLine);

	/** The alarm went off (not on a checkpoint restore): the shutters. */
	void OnAlarm();

	/** PRINT IT: "Print it!" (the clapper and the sting are the clock's). */
	void OnPrintIt();

private:
	ATakeDirector* GetDirector() const;

	/** Rows of the generated table, by key: "SPEAKER|line" (the line as the game shows it) or "@event". */
	TMap<FString, TArray<int32>> RowsByKey;
	/** The next variant to play, by key. */
	TMap<FString, int32> NextVariant;
	void BuildTable();

	/** Plays row Row's voice. Returns its seconds (0 if not imported). */
	float PlayRow(int32 Row);

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<USoundBase>> Loaded;
	TSet<FName> NotImported;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> VoiceAudio;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> HeartAudio;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> RingAudio;

	float HeartLevel = 0.f;
	float RingLevel = 0.f;

	/** A delayed voice (the blooper's "Cut!"): seconds left, < 0 = none. Counted in real time (the freeze). */
	float PendingCutSeconds = -1.f;

	UAudioComponent* MakeLoop(FName Name);
	void StepLoop(UAudioComponent* Audio, float& Level, float Target, float DeltaTime);
};
