// Scene 5. INT. PRIVATE AUDITORIUM - NIGHT (Vance, 35mm). Owner: s5-auditorium. docs/SCRIPT.md, docs/script/s5.md.
//
// The beats, in order:
//  1. SPEECH  the house is low; PENDLETON (the level's old auctioneer "Vance" PatrolGuard: top hat, tails, cane) at the
//             brass podium in one hard spotlight says his line. Vance stands at his aisle seat and cannot walk yet.
//             [Space]/[Enter] skips. A retake skips straight to the blackout.
//  2. CUT     a second before the line ends Pendleton points at the booth; on its end the house lights CUT to pitch
//             black (AHouseDimmer::BeginScreening(0)), the breaker's chunk, the projector's beam from the booth, the
//             film and the title LE SOLEIL NOIR on the screen, the score and the first timpani hit.
//  3. SNEAK   Vance walks (slow, low) along the shadowy south wall to the heavy oak door. The timpani hits every
//             BeatSeconds; each hit flares the screen and the HUD's drum mark. Torches (an usher at the front, a guard
//             at the rope) sweep the floor: inside a torch beam, moving outside the boom's window fills SEEN; full =
//             FailScene. VipAlibi (Scene 3) spends itself on the first full SEEN instead ("Pardon me, sir").
//  4. DOOR    at the door: he stops, the deadbolt buzzes red; VANCE (into cuff): "Booth door is mag-locked from the
//             inside. Cole, you're up." -> FinishScene (the sequencer cuts to Cole). The film keeps running.
// Placed by tools/script/scene5.py (tag script_s5).

#pragma once

#include "CoreMinimal.h"
#include "Script/ScriptScene.h"
#include "Scene5Auditorium.generated.h"

class AHouseDimmer;
class UAudioComponent;
class USoundBase;
class USpotLightComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/** One watcher with a torch that sweeps between two yaws. */
USTRUCT(BlueprintType)
struct FScene5Watch
{
	GENERATED_BODY()

	/** The body (an ACastMember standing alone, placed by scene5.py). The torch rides at its hand. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script")
	TObjectPtr<AActor> Who;

	/** The sweep's two ends, world yaw, degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script")
	float YawA = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script")
	float YawB = 90.f;

	/** One full sweep there and back, s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script", meta = (ClampMin = "0.5"))
	float SweepSeconds = 6.f;

	/** 0..1: where in the sweep it starts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script")
	float Phase = 0.f;

	/** The subtitle's speaker when this one sees him. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script")
	FString Speaker = TEXT("USHER");
};

UENUM()
enum class EScene5Beat : uint8
{
	Speech,
	Sneak,
	Caught,
	Door,
	Done
};

UCLASS()
class REEL_API AScene5Auditorium : public AScriptScene
{
	GENERATED_BODY()

public:
	AScene5Auditorium();

	virtual void BeginScene() override;
	virtual void TickScene(float DeltaSeconds) override;
	virtual void EndScene() override;
	virtual void ResetScene() override;
	virtual bool HandleSceneKey(const FKey& Key, bool bPressed) override;
	virtual void DrawSceneHud(UCanvas* Canvas) override;

	/** For Scene 6 (Cole's override): the deadbolt's lamp goes green (true) or back to red. */
	UFUNCTION(BlueprintCallable, Category = "Script")
	void SetDoorUnlocked(bool bUnlocked);

	// ---- the set (scene5.py) -------------------------------------------------------------------------------------

	/** Pendleton: the old auctioneer's PatrolGuard (or its CastMember). He points at the booth before the cut. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	TObjectPtr<AActor> Pendleton;

	/** The hall's house dimmer (projector, film, house lights). Null: the first one in the level. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	TObjectPtr<AHouseDimmer> Dimmer;

	UPROPERTY(EditAnywhere, Category = "Script|S5")
	TArray<FScene5Watch> Watches;

	/** Where Pendleton points: the projection booth, world. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	FVector BoothPoint = FVector(745.f, -600.f, 1060.f);

	/** The spot on Pendleton: from, to (world). */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	FVector PendletonSpotFrom = FVector(3560.f, -480.f, 900.f);
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	FVector PendletonSpotTo = FVector(3890.f, -480.f, 150.f);

	/** The oak door: Vance arrives within DoorRadius of DoorMark (world, on the floor in front of it). */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	FVector DoorMark = FVector(3650.f, -1900.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float DoorRadius = 140.f;

	/** The deadbolt's red lamp, world. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	FVector DoorLampAt = FVector(3745.f, -1980.f, 140.f);

	/** The title on the screen's face, world, and its facing yaw (180: readable from the house). */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	FVector TitleAt = FVector(4380.f, 0.f, 640.f);
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float TitleYaw = 180.f;

	// ---- the beats -------------------------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, Category = "Script|S5")
	FString PendletonLine = TEXT("Tonight, you witness history. Le Soleil Noir. The only surviving print in existence. Tomorrow, it enters my private vault, never to be seen by common eyes again.");
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	FString VanceLine = TEXT("Booth door is mag-locked from the inside. Cole, you're up.");
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	FString WhoLine = TEXT("Hey. Who's moving back there?");
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	FString AlibiLine = TEXT("Oh. Pardon me, sir. Enjoy the picture.");
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	FString SeenLine = TEXT("You! Stop right there!");
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	FString Title = TEXT("LE SOLEIL NOIR");
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	FString SubTitle = TEXT("1974");

	/** The speech when its sound is missing, s. */
	UPROPERTY(EditAnywhere, Category = "Script|S5", meta = (ClampMin = "1"))
	float SpeechSeconds = 11.f;
	/** Pendleton points this long before the cut, s. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float PointLead = 1.2f;
	/** The house during the speech (his spot stays full). */
	UPROPERTY(EditAnywhere, Category = "Script|S5", meta = (ClampMin = "0", ClampMax = "1"))
	float SpeechHouseLevel = 0.12f;
	/** The house once the film rolls: pitch black. */
	UPROPERTY(EditAnywhere, Category = "Script|S5", meta = (ClampMin = "0", ClampMax = "1"))
	float DarkHouseLevel = 0.f;
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float PendletonSpotCandelas = 60000.f;
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float PendletonSpotDegrees = 9.f;
	/** The title holds on the screen this long after the cut, s. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float TitleSeconds = 9.f;

	/** The timpani: one hit every BeatSeconds; moving is safe from MoveBefore s before a hit to MoveAfter s after. */
	UPROPERTY(EditAnywhere, Category = "Script|S5", meta = (ClampMin = "0.4"))
	float BeatSeconds = 1.6f;
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float MoveBefore = 0.15f;
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float MoveAfter = 0.6f;
	/** Vance's walk while sneaking (low), cm/s. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float SneakSpeed = 260.f;
	/** Faster than this counts as moving, cm/s. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float MovingSpeed = 40.f;
	/** Moving off the boom in a torch fills SEEN in this long, s (it drains at half speed). */
	UPROPERTY(EditAnywhere, Category = "Script|S5", meta = (ClampMin = "0.05"))
	float SeenSeconds = 0.45f;
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float TorchRange = 1700.f;
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float TorchHalfDegrees = 16.f;
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float TorchCandelas = 9000.f;
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float TorchHeight = 130.f;
	/** After the alibi saves him, no torch sees him for this long, s. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float AlibiGraceSeconds = 3.f;
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float ScoreVolume = 0.8f;
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float TimpaniVolume = 1.f;

	// ---- sound (loaded by path; a missing one is skipped) ------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Script|S5|Sound")
	TSoftObjectPtr<USoundBase> PendletonSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Take/Audio/Script/S5/vo_s5_pendleton.vo_s5_pendleton")));
	UPROPERTY(EditAnywhere, Category = "Script|S5|Sound")
	TSoftObjectPtr<USoundBase> VanceSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Take/Audio/Script/S5/vo_s5_vance_cuff.vo_s5_vance_cuff")));
	UPROPERTY(EditAnywhere, Category = "Script|S5|Sound")
	TSoftObjectPtr<USoundBase> WhoSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Take/Audio/Script/S5/vo_s5_usher_who.vo_s5_usher_who")));
	UPROPERTY(EditAnywhere, Category = "Script|S5|Sound")
	TSoftObjectPtr<USoundBase> AlibiSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Take/Audio/Script/S5/vo_s5_usher_alibi.vo_s5_usher_alibi")));
	UPROPERTY(EditAnywhere, Category = "Script|S5|Sound")
	TSoftObjectPtr<USoundBase> SeenSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Take/Audio/Script/S5/vo_s5_guard_seen.vo_s5_guard_seen")));
	UPROPERTY(EditAnywhere, Category = "Script|S5|Sound")
	TSoftObjectPtr<USoundBase> ScoreSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Take/Audio/Script/S5/mus_s5_score.mus_s5_score")));
	UPROPERTY(EditAnywhere, Category = "Script|S5|Sound")
	TSoftObjectPtr<USoundBase> TimpaniSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Take/Audio/Script/S5/sfx_s5_timpani.sfx_s5_timpani")));
	UPROPERTY(EditAnywhere, Category = "Script|S5|Sound")
	TSoftObjectPtr<USoundBase> LightsCutSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Take/Audio/Script/S5/sfx_s5_lights_cut.sfx_s5_lights_cut")));
	UPROPERTY(EditAnywhere, Category = "Script|S5|Sound")
	TSoftObjectPtr<USoundBase> ProjectorSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Take/Audio/Script/S5/sfx_s5_projector.sfx_s5_projector")));
	UPROPERTY(EditAnywhere, Category = "Script|S5|Sound")
	TSoftObjectPtr<USoundBase> MagLockSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Take/Audio/Script/S5/sfx_s5_maglock.sfx_s5_maglock")));
	/** The title's material (unlit glowing text). Missing: the engine's text material. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	TSoftObjectPtr<UMaterialInterface> TitleMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Take/Script/S5/M_S5_TitleText.M_S5_TitleText")));

	// ---- components ---------------------------------------------------------------------------------------------------
	UPROPERTY(VisibleAnywhere, Category = "Script|S5")
	TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category = "Script|S5")
	TObjectPtr<USpotLightComponent> PendletonSpot;
	UPROPERTY(VisibleAnywhere, Category = "Script|S5")
	TObjectPtr<UPointLightComponent> DoorLamp;
	UPROPERTY(VisibleAnywhere, Category = "Script|S5")
	TObjectPtr<UStaticMeshComponent> DoorBulb;
	UPROPERTY(VisibleAnywhere, Category = "Script|S5")
	TObjectPtr<UTextRenderComponent> TitleText;
	UPROPERTY(VisibleAnywhere, Category = "Script|S5")
	TObjectPtr<UTextRenderComponent> SubTitleText;

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

	/** The spot, the lamp and the title at their world places (from the properties above). */
	void PlaceParts();
	void UpdateLamp();

	void Blackout();
	void OnBeat();
	void ArriveAtDoor();
	void StepTorches(float DeltaSeconds);
	bool IsLit(const FVector& At, int32& OutWho) const;
	bool InBoomWindow() const;
	void SetPhase(EScene5Beat NewPhase);
	void SetWalkSpeed(float Speed);
	void RestoreWalkSpeed();
	void ShowTitle(bool bShow);
	void StopScore(float FadeSeconds);
	void SayLine(const FString& Speaker, const FString& Line, const TSoftObjectPtr<USoundBase>& Sound);
	float SoundSeconds(const TSoftObjectPtr<USoundBase>& Sound, float Fallback) const;
	void PlayOneShot(const TSoftObjectPtr<USoundBase>& Sound, float Volume = 1.f);
	class ACastMember* GetPendletonCast() const;
	void EnsureTorches();

	UPROPERTY(Transient)
	TArray<TObjectPtr<USpotLightComponent>> Torches;
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ScoreComp;
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> SpeechComp;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BulbMID;

	EScene5Beat Phase = EScene5Beat::Speech;
	float PhaseTime = 0.f;
	float SpeechEnd = 11.f;
	bool bPointed = false;
	float BeatT = 0.f;			// seconds since the last hit
	double LastBeatReal = -100.0;	// real time of the last hit (the HUD's flash)
	float SneakTime = 0.f;
	float Seen = 0.f;
	int32 SeenBy = -1;
	bool bLitNow = false;
	bool bMovingNow = false;
	bool bWhoSaid = false;
	float GraceLeft = 0.f;
	float DoorEnd = 0.f;
	bool bVanceSaid = false;
	bool bDoorUnlocked = false;
	float SavedWalkSpeed = -1.f;
	TArray<FTransform> WatchStarts;
};
