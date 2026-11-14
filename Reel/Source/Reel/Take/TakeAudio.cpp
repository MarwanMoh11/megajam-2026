// Step 6, audio: voices, sound effects, the composure loops. See TakeAudio.h.

#include "TakeAudio.h"

#include "BarClockComponent.h"
#include "ComposureComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "TakeDirector.h"

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
}

void UTakeAudioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (UAudioComponent* Audio : { VoiceAudio.Get(), HeartAudio.Get(), RingAudio.Get() })
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
	const FString Asset = Name.ToString();
	const FSoftObjectPath Path(FString::Printf(TEXT("%s/%s.%s"), *AudioFolder, *Asset, *Asset));
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
	return FMath::Max(0.f, Sound->GetDuration());
}

float UTakeAudioComponent::SpeakSubtitle(const FString& Speaker, FString& InOutLine)
{
	const FString Key = Speaker + TEXT("|") + InOutLine;
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

// --- event sites -------------------------------------------------------------------------------------------------------

void UTakeAudioComponent::OnCut()
{
	PlayCue(SwitcherCue);
}

void UTakeAudioComponent::OnBlooper()
{
	StopVoice();
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
	if (bRolling)
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

	if (UBarClockComponent* Clock = Director->Clock)
	{
		// Voice ducks the bed (never the warning phrases); backstage hears the band through the flats.
		Clock->SetVoiceDuck(IsVoicePlaying());
		Clock->SetBackstage(bBackstageLowPass && Director->GetLookCamera() == ETakeCamera::C);
	}
}
