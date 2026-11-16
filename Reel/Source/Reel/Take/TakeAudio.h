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
class USoundAttenuation;
class USoundBase;
class ATakeDirector;

/** The script take's mix for one scene (QA Nov 15 evening: the score under each scene, the main hall's bed). */
USTRUCT()
struct FTakeSceneMix
{
	GENERATED_BODY()

	/** The script scene (1-8); 0 = the title card, 99 = the end credits; N * 100 + P = part P of scene N (tag Part=P). */
	UPROPERTY(EditAnywhere, Category = "Take|Audio")
	int32 Scene = 0;

	/** The score: a sound under AudioFolder, a subfolder allowed ("Hall/mus_hall_quartet"); None = no score (the
	 *  scene has its own). The same cue in the next scene plays on through the cut. */
	UPROPERTY(EditAnywhere, Category = "Take|Audio")
	FName Music;

	UPROPERTY(EditAnywhere, Category = "Take|Audio", meta = (ClampMin = "0", ClampMax = "2"))
	float MusicVolume = 0.5f;

	/** A low-pass on the score, Hz (heard through a wall, from the rafters); 0 = open. */
	UPROPERTY(EditAnywhere, Category = "Take|Audio")
	float MusicLowPassHz = 0.f;

	/** The main hall's bed (murmur, room tone, clinks, laughs, steps, trays) at this level; 0 = off. */
	UPROPERTY(EditAnywhere, Category = "Take|Audio", meta = (ClampMin = "0", ClampMax = "2"))
	float HallVolume = 0.f;

	/** A low-pass on the hall's bed, Hz; 0 = open. */
	UPROPERTY(EditAnywhere, Category = "Take|Audio")
	float HallLowPassHz = 0.f;
};

/** One kind of the hall's scattered one-shots: a random one of Sounds every MinGap..MaxGap s, somewhere around the
 *  listener between MinDistance and MaxDistance (3D, so they come from all round). */
USTRUCT()
struct FTakeHallScatter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Take|Audio")
	TArray<FName> Sounds;

	UPROPERTY(EditAnywhere, Category = "Take|Audio")
	float MinGap = 3.f;

	UPROPERTY(EditAnywhere, Category = "Take|Audio")
	float MaxGap = 8.f;

	UPROPERTY(EditAnywhere, Category = "Take|Audio", meta = (ClampMin = "0", ClampMax = "2"))
	float Volume = 0.7f;

	UPROPERTY(EditAnywhere, Category = "Take|Audio")
	float MinDistance = 400.f;

	UPROPERTY(EditAnywhere, Category = "Take|Audio")
	float MaxDistance = 1600.f;
};

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

	// --- the script take's mix (QA Nov 15 evening) --------------------------------------------------------------------
	// The script's scenes don't use the old take's band (the bar clock stays silent while the script runs): this
	// plays the score per scene, carries a cue through the cut when the next scene uses it too, ducks it under every
	// voice (the scenes' Say lines play through VoiceAudio), tape-stops it on a retake and brings it back as the
	// retake rolls, and lays the main hall's bed under scenes 3 and 4. Console: Reel.Audio.Log 1 logs the mix.

	/** The mix per scene (Scene 0 = the title card, 99 = the credits). A scene with no row: no score, no hall. */
	UPROPERTY(EditAnywhere, Category = "Take|Audio|Script")
	TArray<FTakeSceneMix> SceneMix;

	/** A score that is not imported plays this instead (None: silence). */
	UPROPERTY(EditAnywhere, Category = "Take|Audio|Script")
	FName MusicFallback = TEXT("mus_bed");

	/** Every score at this level (times the row's MusicVolume). */
	UPROPERTY(EditAnywhere, Category = "Take|Audio|Script", meta = (ClampMin = "0", ClampMax = "2"))
	float ScoreVolume = 1.f;

	/** Under a voice the score drops to this, and the hall's bed to HallDuckLevel. */
	UPROPERTY(EditAnywhere, Category = "Take|Audio|Script", meta = (ClampMin = "0", ClampMax = "1"))
	float ScoreDuckLevel = 0.45f;

	UPROPERTY(EditAnywhere, Category = "Take|Audio|Script", meta = (ClampMin = "0", ClampMax = "1"))
	float HallDuckLevel = 0.3f;	// was 0.7: the hall's murmur well under a scripted line (Nov 16)

	/** Ducking in and out, level units a second. */
	UPROPERTY(EditAnywhere, Category = "Take|Audio|Script")
	float DuckPerSecond = 4.f;

	/** A cue changing at a cut: the old one fades out and the new one in over this, s. */
	UPROPERTY(EditAnywhere, Category = "Take|Audio|Script")
	float CrossfadeSeconds = 1.2f;

	/** A retake: the score winds down like a tape stopping (pitch and level) over this, s. */
	UPROPERTY(EditAnywhere, Category = "Take|Audio|Script")
	float RetakeTapeStopSeconds = 0.5f;

	/** The hall's loops (2D): the crowd's murmur and the room's tone. */
	UPROPERTY(EditAnywhere, Category = "Take|Audio|Script")
	FName HallMurmur = TEXT("Hall/amb_hall_murmur");

	UPROPERTY(EditAnywhere, Category = "Take|Audio|Script", meta = (ClampMin = "0", ClampMax = "2"))
	float HallMurmurVolume = 0.55f;

	UPROPERTY(EditAnywhere, Category = "Take|Audio|Script")
	FName HallRoom = TEXT("Hall/amb_hall_room");

	UPROPERTY(EditAnywhere, Category = "Take|Audio|Script", meta = (ClampMin = "0", ClampMax = "2"))
	float HallRoomVolume = 0.4f;

	/** The hall's one-shots: glasses, laughs, steps, trays. */
	UPROPERTY(EditAnywhere, Category = "Take|Audio|Script")
	TArray<FTakeHallScatter> HallScatter;

	/** The scenes' Say: plays Sound through VoiceAudio (one line at a time; the score ducks under it). Speaker and
	 *  Line are the subtitle's, so the HUD doesn't voice that subtitle a second time from the old table. */
	void PlayScriptVoice(USoundBase* Sound, const FString& Speaker, const FString& Line);

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

	/** QA time: real seconds left of the voice playing now, 0 when none (ATakeDirector::VoiceSecondsLeft). */
	float GetVoiceSecondsLeft() const;

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

	/** QA time: when the voice now playing started (FPlatformTime, real seconds) and its length. */
	double VoiceStartSeconds = 0.0;
	float VoiceLengthSeconds = 0.f;

	UAudioComponent* MakeLoop(FName Name);
	void StepLoop(UAudioComponent* Audio, float& Level, float Target, float DeltaTime);

	// --- the script take's mix ---
	/** Two score players for the crossfade: [ScoreNow] is the cue playing, the other one fades out. */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ScoreAudio[2];
	FName ScoreCue[2];
	float ScoreLevel[2] = { 0.f, 0.f };
	float ScoreTarget[2] = { 0.f, 0.f };
	int32 ScoreNow = 0;
	float ScoreLowPass = 0.f;
	float TapeStopT = -1.f;		// >= 0: the retake's tape-stop is running
	int32 TapeIndex = 0;		// the score player winding down

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MurmurAudio;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> RoomAudio;

	UPROPERTY(Transient)
	TObjectPtr<USoundAttenuation> HallAttenuation;

	float HallLevel = 0.f;
	float HallTarget = 0.f;
	float HallLowPass = 0.f;
	float Duck = 1.f;
	TArray<float> ScatterWait;

	/** What the mix last saw of the sequencer: the scene, retaking, the end. */
	TWeakObjectPtr<AActor> MixScene;
	int32 MixKey = -1;
	bool bMixRetaking = false;
	float MixLogWait = 0.f;
	FString LastScriptKey;

	void StepScriptMix(float RealDeltaSeconds);
	void StartTapeStop();
	void ApplySceneMix(int32 Key, bool bRestart);
	void StartScore(FName Cue, float Volume, float LowPassHz, bool bRestart);
	void StepHallScatter(float RealDeltaSeconds);
	void LogMix(const TCHAR* Why) const;
};
