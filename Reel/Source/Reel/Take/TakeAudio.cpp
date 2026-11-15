// Step 6, audio: voices, sound effects, the composure loops. See TakeAudio.h.

#include "TakeAudio.h"

#include "BarClockComponent.h"
#include "ComposureComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "TakeDirector.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Script/ScriptSequencer.h"
#include "Sound/SoundAttenuation.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY_STATIC(LogReelAudio, Log, All);

static TAutoConsoleVariable<int32> CVarAudioLog(
	TEXT("Reel.Audio.Log"), 0,
	TEXT("1: log the script take's mix once a second (scene, score, duck, hall, voice, every playing audio component)."));

namespace TakeAudioTable
{
	struct FRow
	{
		const TCHAR* Id;		// the file and asset name
		const TCHAR* Speaker;	// as the HUD shows it
		const TCHAR* Text;		// the subtitle, word for word what the voice says
		const TCHAR* Key;		// "SPEAKER|line the game flashes" (a variant answers that line) or "@event"
	};

	static const FRow Rows[] = {
#define TAKE_AUDIO_ROW(Id, Speaker, Text, Key) { TEXT(Id), TEXT(Speaker), TEXT(Text), TEXT(Key) },
#include "TakeAudioLines.inl"
#undef TAKE_AUDIO_ROW
		{ nullptr, nullptr, nullptr, nullptr }
	};

	constexpr int32 NumRows = UE_ARRAY_COUNT(Rows) - 1;

	/** The rewind whirr comes this long into the blooper freeze and carries on through the reload. */
	constexpr float RewindDelaySeconds = 0.65f;
}

UTakeAudioComponent::UTakeAudioComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bTickEvenWhenPaused = false;

	// The script take's mix (QA Nov 15 evening). Scene 1 has its own muffled jazz and rain, Scene 5 its own score.
	auto Mix = [this](int32 Scene, const TCHAR* Music, float Volume, float LowPass, float Hall, float HallLowPass)
	{
		FTakeSceneMix& Row = SceneMix.AddDefaulted_GetRef();
		Row.Scene = Scene;
		Row.Music = Music ? FName(Music) : NAME_None;
		Row.MusicVolume = Volume;
		Row.MusicLowPassHz = LowPass;
		Row.HallVolume = Hall;
		Row.HallLowPassHz = HallLowPass;
	};
	Mix(0, TEXT("mus_bed"), 0.6f, 0.f, 0.f, 0.f);							// the title card
	Mix(1, nullptr, 0.f, 0.f, 0.f, 0.f);									// outside: the scene's own jazz through the doors
	Mix(2, TEXT("mus_bed"), 0.4f, 700.f, 0.f, 0.f);						// the van: the gala across the street
	Mix(3, TEXT("Hall/mus_hall_quartet"), 0.55f, 0.f, 1.f, 0.f);			// the foyer and the main hall
	Mix(4, TEXT("Hall/mus_hall_quartet"), 0.4f, 2500.f, 0.75f, 3000.f);	// the same hall from the rafters
	Mix(5, nullptr, 0.f, 0.f, 0.f, 0.f);									// the auditorium: its own score
	Mix(6, TEXT("Score/mus_sneak"), 0.45f, 0.f, 0.f, 0.f);					// backstage
	Mix(7, TEXT("Score/mus_sneak"), 0.45f, 0.f, 0.f, 0.f);					// the booth (carries on from 6)
	Mix(702, TEXT("Score/mus_chase"), 0.5f, 0.f, 0.f, 0.f);				// Scene 7 part 2: the alarm, the run to the security room
	Mix(703, TEXT("Score/mus_chase"), 0.5f, 0.f, 0.f, 0.f);				// Scene 7 part 3: the fight (carries on from part 2)
	Mix(8, TEXT("Score/mus_chase"), 0.6f, 0.f, 0.f, 0.f);					// the escape (carries on from the fight)
	Mix(99, TEXT("mus_bed"), 0.7f, 0.f, 0.f, 0.f);							// the credits: the band plays them out

	auto Scatter = [this](std::initializer_list<const TCHAR*> Names, float MinGap, float MaxGap, float Volume,
		float MinDistance, float MaxDistance)
	{
		FTakeHallScatter& Row = HallScatter.AddDefaulted_GetRef();
		for (const TCHAR* Name : Names) { Row.Sounds.Add(FName(Name)); }
		Row.MinGap = MinGap;
		Row.MaxGap = MaxGap;
		Row.Volume = Volume;
		Row.MinDistance = MinDistance;
		Row.MaxDistance = MaxDistance;
	};
	Scatter({ TEXT("Hall/sfx_hall_clink_1"), TEXT("Hall/sfx_hall_clink_2"), TEXT("Hall/sfx_hall_clink_3"),
		TEXT("Hall/sfx_hall_clink_4") }, 2.5f, 6.f, 0.8f, 300.f, 1400.f);
	Scatter({ TEXT("Hall/sfx_hall_laugh_1"), TEXT("Hall/sfx_hall_laugh_2"), TEXT("Hall/sfx_hall_laugh_3") },
		6.f, 14.f, 0.6f, 500.f, 1800.f);
	Scatter({ TEXT("Hall/sfx_hall_steps_1"), TEXT("Hall/sfx_hall_steps_2"), TEXT("Hall/sfx_hall_steps_3") },
		4.f, 9.f, 0.7f, 250.f, 900.f);
	Scatter({ TEXT("Hall/sfx_hall_tray_1"), TEXT("Hall/sfx_hall_tray_2") }, 9.f, 18.f, 0.6f, 400.f, 1500.f);
}

UTakeAudioComponent* UTakeAudioComponent::Get(const UObject* WorldContextObject)
{
	ATakeDirector* Director = ATakeDirector::Get(WorldContextObject);
	return Director ? Director->Audio.Get() : nullptr;
}

ATakeDirector* UTakeAudioComponent::GetDirector() const
{
	return Cast<ATakeDirector>(GetOwner());
}

void UTakeAudioComponent::BeginPlay()
{
	Super::BeginPlay();
	BuildTable();

	if (AActor* OwnerActor = GetOwner())
	{
		VoiceAudio = NewObject<UAudioComponent>(OwnerActor, TEXT("TakeVoiceAudio"));
		VoiceAudio->bAutoActivate = false;
		VoiceAudio->bAutoDestroy = false;
		VoiceAudio->bAllowSpatialization = false;
		VoiceAudio->bIsUISound = true;
		VoiceAudio->RegisterComponent();
	}
	HeartAudio = MakeLoop(HeartbeatLoop);
	RingAudio = MakeLoop(RingingLoop);
	HeartLevel = 0.f;
	RingLevel = 0.f;
	PendingCutSeconds = -1.f;

	// The script take's mix: two score players (the crossfade), the hall's two loops, the one-shots' attenuation.
	if (AActor* OwnerActor = GetOwner())
	{
		for (int32 i = 0; i < 2; ++i)
		{
			UAudioComponent* Score = NewObject<UAudioComponent>(OwnerActor);
			Score->bAutoActivate = false;
			Score->bAutoDestroy = false;
			Score->bAllowSpatialization = false;
			Score->bIsUISound = true;
			Score->RegisterComponent();
			ScoreAudio[i] = Score;
			ScoreCue[i] = NAME_None;
			ScoreLevel[i] = 0.f;
			ScoreTarget[i] = 0.f;
		}
	}
	MurmurAudio = MakeLoop(HallMurmur);
	RoomAudio = MakeLoop(HallRoom);
	HallAttenuation = NewObject<USoundAttenuation>(this);
	{
		FSoundAttenuationSettings& A = HallAttenuation->Attenuation;
		A.bAttenuate = true;
		A.bSpatialize = true;
		A.AttenuationShape = EAttenuationShape::Sphere;
		A.AttenuationShapeExtents = FVector(400.f, 0.f, 0.f);
		A.FalloffDistance = 2600.f;
		A.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
	}
	ScatterWait.Init(1.f, HallScatter.Num());
	for (float& Wait : ScatterWait) { Wait = FMath::FRandRange(0.5f, 3.f); }
	ScoreNow = 0;
	TapeStopT = -1.f;
	HallLevel = HallTarget = 0.f;
	Duck = 1.f;
	MixScene = nullptr;
	MixKey = -1;
	bMixRetaking = false;
}

void UTakeAudioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (UAudioComponent* Audio : { VoiceAudio.Get(), HeartAudio.Get(), RingAudio.Get(), ScoreAudio[0].Get(),
		ScoreAudio[1].Get(), MurmurAudio.Get(), RoomAudio.Get() })
	{
		if (Audio)
		{
			Audio->Stop();
			Audio->DestroyComponent();
		}
	}
	VoiceAudio = nullptr;
	HeartAudio = nullptr;
	RingAudio = nullptr;
	ScoreAudio[0] = ScoreAudio[1] = nullptr;
	MurmurAudio = nullptr;
	RoomAudio = nullptr;
	Super::EndPlay(EndPlayReason);
}

void UTakeAudioComponent::BuildTable()
{
	RowsByKey.Reset();
	NextVariant.Reset();
	// First the keys the game asks by (a line and its variants, an event's lines), then each row's own words, so a
	// subtitle that SayEvent flashed finds exactly the row it came from.
	for (int32 Row = 0; Row < TakeAudioTable::NumRows; ++Row)
	{
		RowsByKey.FindOrAdd(TakeAudioTable::Rows[Row].Key).Add(Row);
	}
	for (int32 Row = 0; Row < TakeAudioTable::NumRows; ++Row)
	{
		const FString Own = FString(TakeAudioTable::Rows[Row].Speaker) + TEXT("|") + TakeAudioTable::Rows[Row].Text;
		if (!RowsByKey.Contains(Own))
		{
			RowsByKey.Add(Own).Add(Row);
		}
	}
}

USoundBase* UTakeAudioComponent::FindSound(FName Name)
{
	if (Name.IsNone() || NotImported.Contains(Name))
	{
		return nullptr;
	}
	if (const TObjectPtr<USoundBase>* Found = Loaded.Find(Name))
	{
		return Found->Get();
	}
	// A subfolder is allowed ("Hall/amb_hall_murmur" is <AudioFolder>/Hall/amb_hall_murmur.amb_hall_murmur).
	const FString Asset = Name.ToString();
	int32 Slash = INDEX_NONE;
	const FString Object = Asset.FindLastChar(TEXT('/'), Slash) ? Asset.Mid(Slash + 1) : Asset;
	const FSoftObjectPath Path(FString::Printf(TEXT("%s/%s.%s"), *AudioFolder, *Asset, *Object));
	USoundBase* Sound = Cast<USoundBase>(Path.TryLoad());
	if (!Sound)
	{
		NotImported.Add(Name);
		return nullptr;
	}
	Loaded.Add(Name, Sound);
	return Sound;
}

bool UTakeAudioComponent::PlayCue(FName Name, float Volume)
{
	USoundBase* Sound = FindSound(Name);
	if (!Sound)
	{
		return false;
	}
	UGameplayStatics::PlaySound2D(this, Sound, Volume * SfxVolume);
	return true;
}

float UTakeAudioComponent::PlayRow(int32 Row)
{
	if (Row < 0 || Row >= TakeAudioTable::NumRows || !VoiceAudio)
	{
		return 0.f;
	}
	USoundBase* Sound = FindSound(FName(TakeAudioTable::Rows[Row].Id));
	if (!Sound)
	{
		return 0.f;
	}
	VoiceAudio->Stop();
	VoiceAudio->SetSound(Sound);
	VoiceAudio->SetVolumeMultiplier(VoiceVolume);
	VoiceAudio->Play();
	// QA time: for VoiceSecondsLeft (real time: a voice plays at normal speed under slow time).
	VoiceStartSeconds = FPlatformTime::Seconds();
	VoiceLengthSeconds = FMath::Max(0.f, Sound->GetDuration());
	return VoiceLengthSeconds;
}

float UTakeAudioComponent::SpeakSubtitle(const FString& Speaker, FString& InOutLine)
{
	const FString Key = Speaker + TEXT("|") + InOutLine;
	// A script scene's Say line brings its own voice (PlayScriptVoice): never a second one from the old table.
	if (!LastScriptKey.IsEmpty() && Key == LastScriptKey)
	{
		return 0.f;
	}
	const TArray<int32>* Rows = RowsByKey.Find(Key);
	if (!Rows || Rows->Num() == 0)
	{
		return 0.f;
	}
	int32& Next = NextVariant.FindOrAdd(Key);
	const int32 Row = (*Rows)[Next % Rows->Num()];
	++Next;
	// A variant shows its own words: the subtitle is always what is said.
	InOutLine = TakeAudioTable::Rows[Row].Text;
	return PlayRow(Row);
}

float UTakeAudioComponent::SpeakNow(const FString& Speaker, const FString& Line)
{
	FString Shown = Line;
	return SpeakSubtitle(Speaker, Shown);
}

bool UTakeAudioComponent::SayEvent(const FString& Event)
{
	const FString Key = TEXT("@") + Event;
	const TArray<int32>* Rows = RowsByKey.Find(Key);
	ATakeDirector* Director = GetDirector();
	if (!Rows || Rows->Num() == 0 || !Director)
	{
		return false;
	}
	int32& Next = NextVariant.FindOrAdd(Key);
	const TakeAudioTable::FRow& Row = TakeAudioTable::Rows[(*Rows)[Next % Rows->Num()]];
	++Next;
	// The HUD's subtitle queue shows it and, as it comes on screen, voices it (SpeakSubtitle by its own words).
	Director->Flash(FString::Printf(TEXT("%s: \"%s\""), Row.Speaker, Row.Text), ETakeFlashKind::Line, 3.f);
	return true;
}

void UTakeAudioComponent::PlayScriptVoice(USoundBase* Sound, const FString& Speaker, const FString& Line)
{
	LastScriptKey = Speaker.ToUpper() + TEXT("|") + Line;
	if (!VoiceAudio)
	{
		return;
	}
	VoiceAudio->Stop();
	if (!Sound)
	{
		return;
	}
	VoiceAudio->SetSound(Sound);
	VoiceAudio->SetVolumeMultiplier(VoiceVolume);
	VoiceAudio->Play();
	VoiceStartSeconds = FPlatformTime::Seconds();
	VoiceLengthSeconds = FMath::Max(0.f, Sound->GetDuration());
	UE_LOG(LogReelAudio, Log, TEXT("AUDIO voice %s %.1f s (%s)"), *Sound->GetName(), VoiceLengthSeconds, *Speaker);
}

void UTakeAudioComponent::StopVoice()
{
	if (VoiceAudio)
	{
		VoiceAudio->Stop();
	}
}

bool UTakeAudioComponent::IsVoicePlaying() const
{
	return VoiceAudio && VoiceAudio->IsPlaying();
}

float UTakeAudioComponent::GetVoiceSecondsLeft() const
{
	if (!IsVoicePlaying())
	{
		return 0.f;
	}
	const double Elapsed = FPlatformTime::Seconds() - VoiceStartSeconds;
	return FMath::Max(0.f, VoiceLengthSeconds - static_cast<float>(Elapsed));
}

// --- event sites -------------------------------------------------------------------------------------------------------

void UTakeAudioComponent::OnCut()
{
	PlayCue(SwitcherCue);
}

void UTakeAudioComponent::OnBlooper()
{
	StopVoice();
	// The script take: its score winds down like a tape (StepScriptMix) and comes back as the retake rolls.
	StartTapeStop();
	if (ATakeDirector* Director = GetDirector())
	{
		if (Director->Clock)
		{
			Director->Clock->TapeStop();
		}
	}
	for (const FName& Cue : BlooperCues)
	{
		PlayCue(Cue);
	}
	PendingCutSeconds = FMath::Max(0.f, BlooperCutDelay);
	if (USoundBase* Rewind = FindSound(RewindCue))
	{
		// Through the reload: the next take's clapper and "Action!" come in under its tail.
		if (UWorld* World = GetWorld())
		{
			FTimerHandle Handle;
			TWeakObjectPtr<UWorld> WeakWorld = World;
			const float Volume = SfxVolume;
			World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([WeakWorld, Rewind, Volume]()
			{
				if (WeakWorld.IsValid())
				{
					UGameplayStatics::SpawnSound2D(WeakWorld.Get(), Rewind, Volume, 1.f, 0.f, nullptr, true, true);
				}
			}), TakeAudioTable::RewindDelaySeconds, false);
		}
	}
}

void UTakeAudioComponent::OnTakeStart(bool bAfterBlooper)
{
	ATakeDirector* Director = GetDirector();
	if (Director && Director->Clock)
	{
		Director->Clock->PlayClap();
	}
}

void UTakeAudioComponent::OnTrigger(FName Trigger)
{
	SayEvent(TEXT("trigger_") + Trigger.ToString());
}

void UTakeAudioComponent::OnDeviceFired(FName FireSound, const FString& DeviceName, bool bHasFireLine)
{
	PlayCue(FireSound);
	if (!bHasFireLine)
	{
		SayEvent(TEXT("fire_") + DeviceName);
	}
}

void UTakeAudioComponent::OnAlarm()
{
	PlayCue(ShuttersCue);
}

void UTakeAudioComponent::OnPrintIt()
{
	SpeakNow(TEXT("DIRECTOR"), TEXT("Print it!"));
}

// --- loops and the mix -------------------------------------------------------------------------------------------------

UAudioComponent* UTakeAudioComponent::MakeLoop(FName Name)
{
	USoundBase* Sound = FindSound(Name);
	AActor* OwnerActor = GetOwner();
	if (!Sound || !OwnerActor)
	{
		return nullptr;
	}
	if (USoundWave* Wave = Cast<USoundWave>(Sound))
	{
		Wave->bLooping = true;
	}
	UAudioComponent* Audio = NewObject<UAudioComponent>(OwnerActor);
	Audio->bAutoActivate = false;
	Audio->bAutoDestroy = false;
	Audio->bAllowSpatialization = false;
	Audio->bIsUISound = true;
	Audio->SetSound(Sound);
	Audio->RegisterComponent();
	return Audio;
}

void UTakeAudioComponent::StepLoop(UAudioComponent* Audio, float& Level, float Target, float DeltaTime)
{
	if (!Audio)
	{
		return;
	}
	Level = FMath::FInterpConstantTo(Level, Target, DeltaTime, FMath::Max(0.01f, LoopFadePerSecond));
	if (Level <= 0.001f)
	{
		if (Audio->IsPlaying())
		{
			Audio->Stop();
		}
		return;
	}
	Audio->SetVolumeMultiplier(Level);
	if (!Audio->IsPlaying())
	{
		Audio->Play();
	}
}

void UTakeAudioComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ATakeDirector* Director = GetDirector();
	if (!Director)
	{
		return;
	}

	// QA time: slow time eases, resets and holds the music on the real frame time (this tick's DeltaTime is slowed).
	Director->StepSlow(FApp::GetDeltaTime());

	if (PendingCutSeconds >= 0.f)
	{
		PendingCutSeconds -= DeltaTime;
		if (PendingCutSeconds < 0.f)
		{
			SpeakNow(TEXT("DIRECTOR"), TEXT("Cut!"));
		}
	}

	const bool bRolling = Director->Phase == ETakePhase::Rolling;
	float HeartTarget = 0.f;
	float RingTarget = 0.f;
	if (bRolling && !Director->bScriptDriven)	// the old take's heartbeat/ringing never play under the script
	{
		HeartTarget = Director->bJulianSeen ? HeartbeatVolume : 0.f;
		const float Composure = Director->Composure ? Director->Composure->GetComposure() : 100.f;
		if (RingingBelow > 0.f && Composure < RingingBelow)
		{
			RingTarget = RingingVolume * FMath::Clamp(1.f - Composure / RingingBelow, 0.f, 1.f);
		}
	}
	StepLoop(HeartAudio, HeartLevel, HeartTarget, DeltaTime);
	StepLoop(RingAudio, RingLevel, RingTarget, DeltaTime);

	StepScriptMix(FApp::GetDeltaTime());

	if (UBarClockComponent* Clock = Director->Clock)
	{
		// Voice ducks the bed (never the warning phrases); backstage hears the band through the flats.
		Clock->SetVoiceDuck(IsVoicePlaying());
		Clock->SetBackstage(bBackstageLowPass && Director->GetLookCamera() == ETakeCamera::C);
	}
}

// --- the script take's mix (QA Nov 15 evening) -------------------------------------------------------------------------
// The bug Marwan heard: the script's scenes ran on top of the old take's band. The bar clock started the jazz bed on
// the take's first frame and never stopped it for a scene (so it doubled Scene 1's own muffled jazz and ran under
// Scene 5's score), dipped it to 20% every eighth bar (the old take's band breaks), never ducked it under a scene's
// voice (Say played its own component; the duck only knew the old table's voice), and a retake's tape-stop killed it
// for good (the old take reloaded the level and started the band again; the script's retake doesn't). The clock is
// silent while the script runs (UBarClockComponent::StartMusic) and this mix owns the music instead.

void UTakeAudioComponent::LogMix(const TCHAR* Why) const
{
	const int32 Old = 1 - ScoreNow;
	UE_LOG(LogReelAudio, Log,
		TEXT("AUDIO mix %s: scene %d | score %s %.2f->%.2f lp %.0f%s | fading %s %.2f | duck %.2f | hall %.2f->%.2f lp %.0f | voice %s"),
		Why, MixKey, *ScoreCue[ScoreNow].ToString(), ScoreLevel[ScoreNow], ScoreTarget[ScoreNow], ScoreLowPass,
		TapeStopT >= 0.f ? TEXT(" TAPESTOP") : TEXT(""), *ScoreCue[Old].ToString(), ScoreLevel[Old], Duck, HallLevel,
		HallTarget, HallLowPass, IsVoicePlaying() && VoiceAudio->Sound ? *VoiceAudio->Sound->GetName() : TEXT("-"));
}

void UTakeAudioComponent::StartTapeStop()
{
	if (TapeStopT < 0.f && ScoreAudio[ScoreNow] && ScoreAudio[ScoreNow]->IsPlaying())
	{
		TapeIndex = ScoreNow;
		TapeStopT = 0.f;
		ScoreTarget[ScoreNow] = 0.f;
		LogMix(TEXT("retake: tape-stop"));
	}
}

void UTakeAudioComponent::StartScore(FName Cue, float Volume, float LowPassHz, bool bRestart)
{
	FName Want = Cue;
	USoundBase* Sound = Want.IsNone() ? nullptr : FindSound(Want);
	if (!Want.IsNone() && !Sound && !MusicFallback.IsNone())
	{
		UE_LOG(LogReelAudio, Warning, TEXT("AUDIO score %s is not imported: %s instead"), *Want.ToString(),
			*MusicFallback.ToString());
		Want = MusicFallback;
		Sound = FindSound(Want);
	}
	ScoreLowPass = LowPassHz;
	UAudioComponent* Current = ScoreAudio[ScoreNow];
	const bool bTaping = TapeStopT >= 0.f && TapeIndex == ScoreNow;
	if (Sound && Current && !bRestart && !bTaping && ScoreCue[ScoreNow] == Want && Current->IsPlaying())
	{
		// The same cue: it plays on through the cut, at the new scene's level and filter.
		ScoreTarget[ScoreNow] = Volume * ScoreVolume;
	}
	else
	{
		ScoreTarget[ScoreNow] = 0.f;	// the cue playing fades out (or finishes its tape-stop)
		if (Sound)
		{
			ScoreNow = 1 - ScoreNow;
			if (UAudioComponent* Next = ScoreAudio[ScoreNow])
			{
				if (USoundWave* Wave = Cast<USoundWave>(Sound))
				{
					Wave->bLooping = true;
				}
				if (TapeStopT >= 0.f && TapeIndex == ScoreNow)
				{
					TapeStopT = -1.f;	// (a cue change mid tape-stop: this player is wanted again)
				}
				Next->Stop();
				Next->SetSound(Sound);
				Next->SetPitchMultiplier(1.f);
				Next->SetVolumeMultiplier(0.001f);
				Next->Play();
			}
			ScoreCue[ScoreNow] = Want;
			ScoreLevel[ScoreNow] = 0.f;
			ScoreTarget[ScoreNow] = Volume * ScoreVolume;
		}
	}
	for (int32 i = 0; i < 2; ++i)
	{
		if (UAudioComponent* Score = ScoreAudio[i])
		{
			const float Hz = i == ScoreNow ? ScoreLowPass : 0.f;
			Score->SetLowPassFilterEnabled(Hz > 0.f);
			if (Hz > 0.f)
			{
				Score->SetLowPassFilterFrequency(Hz);
			}
		}
	}
}

void UTakeAudioComponent::ApplySceneMix(int32 Key, bool bRestart)
{
	MixKey = Key;
	const FTakeSceneMix* Row = SceneMix.FindByPredicate([Key](const FTakeSceneMix& R) { return R.Scene == Key; });
	StartScore(Row ? Row->Music : NAME_None, Row ? Row->MusicVolume : 0.f, Row ? Row->MusicLowPassHz : 0.f, bRestart);
	HallTarget = Row ? Row->HallVolume : 0.f;
	HallLowPass = Row ? Row->HallLowPassHz : 0.f;
	for (UAudioComponent* Loop : { MurmurAudio.Get(), RoomAudio.Get() })
	{
		if (Loop)
		{
			Loop->SetLowPassFilterEnabled(HallLowPass > 0.f);
			if (HallLowPass > 0.f)
			{
				Loop->SetLowPassFilterFrequency(HallLowPass);
			}
		}
	}
	LogMix(bRestart ? TEXT("retake rolls") : TEXT("scene"));
}

void UTakeAudioComponent::StepScriptMix(float RealDeltaSeconds)
{
	AScriptSequencer* Seq = AScriptSequencer::Get(this);
	if (!Seq || !Seq->IsRunning())
	{
		return;	// the old take: the bar clock's band
	}
	const float Dt = FMath::Max(0.f, RealDeltaSeconds);

	// Which mix: the scene, the title card (0), the credits (99); a retake winds the score down and restarts it.
	FName Reason;
	float RetakeAge = 0.f;
	const bool bRetaking = Seq->IsRetaking(Reason, RetakeAge);
	AScriptScene* Scene = Seq->GetActiveScene();
	int32 Key = Seq->GetEndSeconds() >= 0.f ? 99 : (Scene ? Scene->SceneNumber : 0);
	if (Scene && Key != 99)
	{
		// A scene in parts (an actor tag "Part=N", Scene 7's four): a row for N * 100 + part wins over the scene's.
		for (const FName& Tag : Scene->Tags)
		{
			const FString T = Tag.ToString();
			if (T.StartsWith(TEXT("Part=")))
			{
				const int32 PartKey = Key * 100 + FCString::Atoi(*T.Mid(5));
				if (SceneMix.ContainsByPredicate([PartKey](const FTakeSceneMix& R) { return R.Scene == PartKey; }))
				{
					Key = PartKey;
				}
				break;
			}
		}
	}
	if (bRetaking)
	{
		if (!bMixRetaking)
		{
			StartTapeStop();
		}
		bMixRetaking = true;
	}
	else if (bMixRetaking)
	{
		bMixRetaking = false;
		MixScene = Scene;
		ApplySceneMix(Key, true);
	}
	else if (Key != MixKey || Scene != MixScene.Get())
	{
		MixScene = Scene;
		ApplySceneMix(Key, false);
	}

	// The duck: every voice through VoiceAudio (the scenes' Say, the old table's lines).
	Duck = FMath::FInterpConstantTo(Duck, IsVoicePlaying() ? ScoreDuckLevel : 1.f, Dt, FMath::Max(0.1f, DuckPerSecond));
	const float DuckDepth = FMath::Max(0.01f, 1.f - ScoreDuckLevel);
	const float HallDuck = FMath::Lerp(1.f, HallDuckLevel, FMath::Clamp((1.f - Duck) / DuckDepth, 0.f, 1.f));

	// The score's two players: the cue fading in or holding, the other fading out; the retake's tape-stop.
	const float FadeRate = 1.f / FMath::Max(0.05f, CrossfadeSeconds);
	for (int32 i = 0; i < 2; ++i)
	{
		UAudioComponent* Score = ScoreAudio[i];
		if (!Score)
		{
			continue;
		}
		if (TapeStopT >= 0.f && i == TapeIndex)
		{
			const float A = FMath::Clamp(TapeStopT / FMath::Max(0.05f, RetakeTapeStopSeconds), 0.f, 1.f);
			Score->SetPitchMultiplier(FMath::Lerp(1.f, 0.3f, A));
			Score->SetVolumeMultiplier(FMath::Max(0.001f, ScoreLevel[i] * Duck * (1.f - A)));
			if (A >= 1.f)
			{
				Score->Stop();
				Score->SetPitchMultiplier(1.f);
				ScoreLevel[i] = 0.f;
				ScoreTarget[i] = 0.f;
				if (i != ScoreNow || bMixRetaking)
				{
					ScoreCue[i] = NAME_None;
				}
				TapeStopT = -1.f;
			}
			continue;
		}
		ScoreLevel[i] = FMath::FInterpConstantTo(ScoreLevel[i], ScoreTarget[i], Dt, FadeRate);
		if (ScoreLevel[i] <= 0.001f && ScoreTarget[i] <= 0.f)
		{
			if (Score->IsPlaying())
			{
				Score->Stop();
			}
			if (i != ScoreNow)
			{
				ScoreCue[i] = NAME_None;
			}
			continue;
		}
		Score->SetVolumeMultiplier(FMath::Max(0.001f, ScoreLevel[i] * Duck));
	}
	if (TapeStopT >= 0.f)
	{
		TapeStopT += Dt;
	}

	// The main hall's bed.
	HallLevel = FMath::FInterpConstantTo(HallLevel, HallTarget, Dt, FadeRate);
	UAudioComponent* const HallLoops[] = { MurmurAudio.Get(), RoomAudio.Get() };
	const float HallLoopVolumes[] = { HallMurmurVolume, HallRoomVolume };
	for (int32 i = 0; i < 2; ++i)
	{
		UAudioComponent* Loop = HallLoops[i];
		if (!Loop)
		{
			continue;
		}
		if (HallLevel <= 0.001f)
		{
			if (Loop->IsPlaying())
			{
				Loop->Stop();
			}
			continue;
		}
		Loop->SetVolumeMultiplier(FMath::Max(0.001f, HallLevel * HallLoopVolumes[i] * HallDuck));
		if (!Loop->IsPlaying())
		{
			Loop->Play(FMath::FRandRange(0.f, 5.f));	// not from the file's top every time
		}
	}
	if (HallLevel > 0.05f && !bMixRetaking)
	{
		StepHallScatter(Dt);
	}

	if (CVarAudioLog.GetValueOnGameThread() != 0)
	{
		MixLogWait -= Dt;
		if (MixLogWait <= 0.f)
		{
			MixLogWait = 1.f;
			LogMix(TEXT("tick"));
			FString Playing;
			int32 Count = 0;
			for (TObjectIterator<UAudioComponent> It; It; ++It)
			{
				const UAudioComponent* Audio = *It;
				if (Audio && Audio->GetWorld() == GetWorld() && Audio->IsPlaying() && Audio->Sound)
				{
					++Count;
					Playing += FString::Printf(TEXT(" %s@%s(%.2f)"), *Audio->Sound->GetName(),
						Audio->GetOwner() ? *Audio->GetOwner()->GetName() : TEXT("-"), Audio->VolumeMultiplier);
				}
			}
			UE_LOG(LogReelAudio, Log, TEXT("AUDIO playing %d:%s"), Count, *Playing);
		}
	}
}

void UTakeAudioComponent::StepHallScatter(float RealDeltaSeconds)
{
	if (ScatterWait.Num() != HallScatter.Num())
	{
		ScatterWait.Init(1.f, HallScatter.Num());
	}
	APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!Camera)
	{
		return;
	}
	const FVector Ear = Camera->GetCameraLocation();
	const float DuckDepth = FMath::Max(0.01f, 1.f - ScoreDuckLevel);
	const float HallDuck = FMath::Lerp(1.f, HallDuckLevel, FMath::Clamp((1.f - Duck) / DuckDepth, 0.f, 1.f));
	for (int32 i = 0; i < HallScatter.Num(); ++i)
	{
		const FTakeHallScatter& Row = HallScatter[i];
		ScatterWait[i] -= RealDeltaSeconds;
		if (ScatterWait[i] > 0.f || Row.Sounds.Num() == 0)
		{
			continue;
		}
		ScatterWait[i] = FMath::FRandRange(FMath::Max(0.2f, Row.MinGap), FMath::Max(Row.MinGap, Row.MaxGap) + 0.01f);
		USoundBase* Sound = FindSound(Row.Sounds[FMath::RandRange(0, Row.Sounds.Num() - 1)]);
		if (!Sound)
		{
			continue;
		}
		const float Yaw = FMath::FRandRange(0.f, 2.f * PI);
		const float Distance = FMath::FRandRange(Row.MinDistance, FMath::Max(Row.MinDistance, Row.MaxDistance));
		const FVector Where = Ear + FVector(FMath::Cos(Yaw), FMath::Sin(Yaw), 0.f) * Distance
			+ FVector(0.f, 0.f, FMath::FRandRange(-120.f, 40.f));
		UAudioComponent* Shot = UGameplayStatics::SpawnSoundAtLocation(this, Sound, Where, FRotator::ZeroRotator,
			Row.Volume * HallLevel * HallDuck, FMath::FRandRange(0.94f, 1.06f), 0.f, HallAttenuation);
		if (Shot && HallLowPass > 0.f)
		{
			Shot->SetLowPassFilterEnabled(true);
			Shot->SetLowPassFilterFrequency(HallLowPass);
		}
	}
}
