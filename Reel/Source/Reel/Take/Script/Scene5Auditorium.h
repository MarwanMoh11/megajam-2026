// Scene 5. INT. PRIVATE AUDITORIUM - NIGHT (Vance, 35mm). Owner: s5-auditorium. docs/SCRIPT.md, docs/script/s5.md.
//
// The beats, in order (QA Nov 15 evening: the mouth, the walk out of the row, the long film, no beat timing):
//  1. SPEECH  the house is low; PENDLETON (the level's old auctioneer "Vance" PatrolGuard: top hat, tails, cane) at the
//             brass podium in one hard spotlight says his line, hands clasped, his head carrying it: small dips on
//             the voice's stressed beats (Scene5Lipsync.inl, the voice's loudness baked by tools/script/scene5_lipsync.py),
//             a slow turn across the house, a slight tilt (no mouth piece and no Speech clip since QA Nov 17). Part
//             way through, a cutaway: Vance in his aisle seat in the row, listening; then Pendleton again from a
//             second angle. [Space]/[Enter] skips. A retake skips straight to the sneak.
//  2. CUT     a second before the line ends Pendleton points at the booth; on its end the house lights CUT to pitch
//             black (AHouseDimmer::BeginScreening(0)), the breaker's chunk and one timpani hit, the projector's beam
//             from the booth, the long looping film (M_S5_Film, tools/script/scene5_film*.: LE SOLEIL NOIR's titles,
//             scenes and a few Easter eggs) and the score.
//  3. RISE    the seat shot in the dark: VANCE (into cuff) "Lights are down. I'm moving." He stands (cut on the
//             action to the exit shot from the side aisle), slips out of the row into the aisle, walked by the scene.
//             COLE (earpiece) answers with the way: the oak door by the stage, the torches. [Space] skips to the
//             aisle. Then the follow camera and the player's walk.
//  4. SNEAK   Vance walks (slow, low) along the shadowy south wall to the heavy oak door. An usher and a guard watch
//             the film most of the time (torch lowered: blind); every so often each turns and sweeps the aisle with
//             a raised torch for a few seconds, warned by the HUD ("turning"). Moving inside a raised torch fills
//             SEEN; full = FailScene. VipAlibi (Scene 3) spends itself on the first full SEEN instead ("Pardon me,
//             sir"). Standing still in a beam is safe.
//  5. DOOR    at the door: he stops, the deadbolt buzzes red; VANCE (into cuff): "Booth door is mag-locked from the
//             inside. Cole, you're up." -> FinishScene (the sequencer cuts to Cole). The film keeps running until
//             StopFilm or AHouseDimmer::EndScreening (Scene 7's alarm).
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
class UAnimSequenceBase;

/** One watcher with a torch. Most of the time it faces the screen (AwayYaw, torch lowered: blind); every cycle it
 *  turns and sweeps the aisle from YawA to YawB and back (torch raised: it sees) for CheckSeconds, then turns back. */
USTRUCT(BlueprintType)
struct FScene5Watch
{
	GENERATED_BODY()

	/** The body (an ACastMember standing alone, placed by scene5.py). The torch rides at its hand. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script")
	TObjectPtr<AActor> Who;

	/** The check's sweep, world yaw, degrees: from YawA to YawB and back. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script")
	float YawA = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script")
	float YawB = 90.f;

	/** Where it looks while it watches the film, world yaw (toward the screen). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script")
	float AwayYaw = 0.f;

	/** Watching the film (blind), s. Long and readable: the player's window. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script", meta = (ClampMin = "1"))
	float AwaySeconds = 8.f;

	/** Sweeping the aisle with the torch raised, s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script", meta = (ClampMin = "0.5"))
	float CheckSeconds = 3.5f;

	/** Each turn (to the aisle, back to the film), s. The torch comes up during the turn to the aisle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script", meta = (ClampMin = "0.2"))
	float TurnSeconds = 0.9f;

	/** Unused since QA Nov 15 evening (the old sine sweep's period); kept so placed data loads. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script", meta = (ClampMin = "0.5"))
	float SweepSeconds = 6.f;

	/** 0..1: where in its cycle it starts when the sneak begins (0: at the start of watching the film). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script")
	float Phase = 0.f;

	/** The subtitle's speaker when this one sees him, and the HUD's name for it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Script")
	FString Speaker = TEXT("USHER");
};

/** Where a watcher is in its cycle. */
enum class EScene5Look : uint8
{
	Film,		// facing the screen, torch down: blind
	TurnIn,		// turning to the aisle, the torch coming up: sees in the second half
	Check,		// sweeping the aisle: sees
	TurnOut		// turning back to the film: blind
};

UENUM()
enum class EScene5Beat : uint8
{
	Speech,
	Rise,
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

	/** The hook for Scene 7's alarm (s7-route): the film and the beam off and Scene 5's score out, at once. bHouseUp:
	 *  the house lights come back up (true), or the hall stays as dark as it is (false). Safe to call any time. */
	UFUNCTION(BlueprintCallable, Category = "Script")
	void StopFilm(bool bHouseUp = true);

	// ---- the set (scene5.py) -------------------------------------------------------------------------------------

	/** Pendleton: the old auctioneer's PatrolGuard (or its CastMember). He points at the booth before the cut. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	TObjectPtr<AActor> Pendleton;

	/** The hall's house dimmer (projector, film, house lights). Null: the first one in the level. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	TObjectPtr<AHouseDimmer> Dimmer;

	UPROPERTY(EditAnywhere, Category = "Script|S5")
	TArray<FScene5Watch> Watches;

	/** The cutaway during the speech, and the dark seat shot after the blackout: Vance in his seat in the row. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	TObjectPtr<AActor> SeatCamera;

	/** Pendleton again after the cutaway, from a second angle (never SceneCamera: a cut back to the scene's own camera
	 *  puts Vance back on the StartSpot). Null: the cutaway is skipped. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	TObjectPtr<AActor> PendletonCamera2;

	/** The exit shot from the side aisle: Vance stands, slips out of the row and turns to the wall. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	TObjectPtr<AActor> ExitCamera;

	/** Scene 4's last shot (QA Nov 16): Vance sits down in his seat on camera, before this scene opens. */
	void SeatVanceNow() { SitVance(true); }
	/** Keeps Vance in his seat while he is seated: every tick of the seat shots (QA Nov 16: something switched his
	 *  movement back on with his capsule off, he fell through the floor and the seat shots showed an empty chair). */
	void HoldVanceSeated();
	/** Places the seat camera from the seat (SeatCamOffset, turned by SeatYaw) at SeatCamFov: the wide seat shot. */
	void FrameSeatCamera();

	/** The seat camera's place from the seat's floor point (seat space: +X ahead of the chair, +Z up), and its FOV.
	 *  QA Nov 16 (Marwan): "zoom out the camera more at the seating area". */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	FVector SeatCamOffset = FVector(200.f, -190.f, 300.f);
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float SeatCamFov = 56.f;

	/** Vance's seat: the chair's floor point (world) and the way it faces. He sits there through the speech. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	FVector SeatAt = FVector(2550.f, -583.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float SeatYaw = 0.f;
	/** The seated body's offset from SeatAt (world cm; z lifts the capsule). Tuned by eye in the seat shot. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	FVector SitOffset = FVector(-8.f, 0.f, 0.f);
	/** The seated clip (a mocap idle on the mannequin skeleton). */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	TSoftObjectPtr<UAnimSequenceBase> SitClip = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Take/Anim/Mocap/SitIdle_A.SitIdle_A")));

	/** The walk out: he stands at StandAt (in the leg room in front of his chair), walks to RowExit (the row's end
	 *  in the side aisle), then to AisleAt, where the player takes him. World, floor. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	FVector StandAt = FVector(2600.f, -583.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	FVector RowExit = FVector(2600.f, -800.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	FVector AisleAt = FVector(2700.f, -1150.f, 0.f);

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
	/** Rising out of the row (into his cuff), and the crew's answer on the earpiece. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	FString GoLine = TEXT("Lights are down. I'm moving. Heading for the stage door.");
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	FString ReplySpeaker = TEXT("COLE");
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	FString ReplyLine = TEXT("Copy. Oak door, south wall, right by the stage. It's the only way up to the booth. Ushers have torches: move while they watch the picture.");
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
	/** The old LE SOLEIL NOIR text over the screen after the cut, s (0: none; the film has its own title card). */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float TitleSeconds = 0.f;

	/** The cutaway to Vance in his seat: starts at this share of the speech, lasts CutawaySeconds. */
	UPROPERTY(EditAnywhere, Category = "Script|S5", meta = (ClampMin = "0", ClampMax = "1"))
	float CutawayAt = 0.42f;
	UPROPERTY(EditAnywhere, Category = "Script|S5", meta = (ClampMin = "0"))
	float CutawaySeconds = 2.4f;

	/** The rise: Vance's line this long after the blackout; he stands (the cut to ExitCamera) at StandUpAt; the walk
	 *  to the aisle gives up after RiseMaxSeconds (he is put at AisleAt). */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float GoLineAt = 0.9f;
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float StandUpAt = 2.4f;
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float RiseMaxSeconds = 9.f;
	/** The walk out of the row, cm/s. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float RiseSpeed = 200.f;
	/** The screen's cool bounce on Vance's face through the rise (the hall is black), candelas (0: none). */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float VanceFillCandelas = 40.f;

	/** Pendleton's old mouth: a dark lip-shaped piece on his head bone that opened on the words. Off since Marwan's QA
	 *  (Nov 17: lips moving under a still face); his head carries the line instead (TalkHead). Offset from the head
	 *  bone at the reference pose: X forward, Y left, Z up, cm. */
	UPROPERTY(EditAnywhere, Category = "Script|S5|Mouth")
	bool bLipSync = false;
	/** How much Pendleton's head carries the line (ACastMember::SetTalkHead): dips on the voice's stressed beats (the
	 *  baked curve, tools/script/scene5_lipsync.py), a slow turn across the house, a slight tilt. 0: still. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float TalkHead = 1.f;
	UPROPERTY(EditAnywhere, Category = "Script|S5|Mouth")
	FVector MouthOffset = FVector(10.6f, 0.f, -1.6f);
	/** Closed: width, depth, height, cm. */
	UPROPERTY(EditAnywhere, Category = "Script|S5|Mouth")
	FVector MouthSize = FVector(4.6f, 1.2f, 0.45f);
	/** Fully open adds this much height (cm), and the piece drops by half of it (the jaw). */
	UPROPERTY(EditAnywhere, Category = "Script|S5|Mouth")
	float MouthOpenCm = 2.4f;
	/** Lips spread (ee, s) add this much width, rounded (oo, o) take it away, cm. */
	UPROPERTY(EditAnywhere, Category = "Script|S5|Mouth")
	float MouthWideCm = 1.2f;
	UPROPERTY(EditAnywhere, Category = "Script|S5|Mouth")
	FLinearColor MouthColour = FLinearColor(0.05f, 0.008f, 0.008f);
	/** The mouth runs this far ahead of the sound (the eye reads the jaw a frame early), s. */
	UPROPERTY(EditAnywhere, Category = "Script|S5|Mouth")
	float MouthLead = 0.05f;
	/** Pendleton's arms during the speech: ACastMember::PlayGesture with this act (None: no act). None since Marwan's
	 *  QA (Nov 17): the Speech clips (a rallying crowd-pleaser) were over the top; his hands stay clasped and the head
	 *  does the talking. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	FName SpeechGesture = NAME_None;

	/** The sneak is crouched (ATakeCharacter::SetCrouching: CrouchSpeed, about 150 cm/s); off: SneakSpeed upright. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	bool bCrouchSneak = true;
	/** Vance's walk while sneaking upright, cm/s. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float SneakSpeed = 260.f;
	/** Faster than this counts as moving, cm/s. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float MovingSpeed = 40.f;
	/** Moving in a raised torch fills SEEN in this long, s (it drains at half speed). */
	UPROPERTY(EditAnywhere, Category = "Script|S5", meta = (ClampMin = "0.05"))
	float SeenSeconds = 0.8f;
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float TorchRange = 1700.f;
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float TorchHalfDegrees = 16.f;
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float TorchCandelas = 9000.f;
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float TorchHeight = 130.f;
	/** A lowered torch (the watcher on the film): this share of TorchCandelas, pitched down at its feet. */
	UPROPERTY(EditAnywhere, Category = "Script|S5", meta = (ClampMin = "0", ClampMax = "1"))
	float LoweredTorchShare = 0.15f;
	/** The HUD warns this long before a watcher turns to the aisle, s. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float TurnWarnSeconds = 1.5f;
	/** After the alibi saves him, no torch sees him for this long, s. */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float AlibiGraceSeconds = 3.f;
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float ScoreVolume = 0.8f;
	/** One timpani hit lands with the blackout (no beat timing since QA Nov 15 evening). */
	UPROPERTY(EditAnywhere, Category = "Script|S5")
	float TimpaniVolume = 1.f;

	// ---- sound (loaded by path; a missing one is skipped) ------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Script|S5|Sound")
	TSoftObjectPtr<USoundBase> PendletonSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Take/Audio/Script/S5/vo_s5_pendleton.vo_s5_pendleton")));
	UPROPERTY(EditAnywhere, Category = "Script|S5|Sound")
	TSoftObjectPtr<USoundBase> VanceSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Take/Audio/Script/S5/vo_s5_vance_cuff.vo_s5_vance_cuff")));
	UPROPERTY(EditAnywhere, Category = "Script|S5|Sound")
	TSoftObjectPtr<USoundBase> GoSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Take/Audio/Script/S5/vo_s5_vance_go.vo_s5_vance_go")));
	UPROPERTY(EditAnywhere, Category = "Script|S5|Sound")
	TSoftObjectPtr<USoundBase> ReplySound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Take/Audio/Script/S5/vo_s5_cole_reply.vo_s5_cole_reply")));
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
	void StepRise();
	void BeginSneak();
	void ArriveAtDoor();
	void StepTorches(float DeltaSeconds);
	bool IsLit(const FVector& At, int32& OutWho) const;
	/** Watcher I's look now: its state, its yaw, and the seconds until the next state. */
	EScene5Look WatchLook(int32 I, float& OutYaw, float& OutLeft) const;
	/** The mouth: made and attached on the speech's first frame, then opened from the baked envelope. */
	void EnsureMouth();
	void StepMouth();
	/** Vance into his seat (the sit clip on his mesh, his capsule off) or back on his feet at StandAt. */
	void SitVance(bool bSit);
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
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> PendletonMouth;
	/** The screen's bounce on Vance's face in the dark seat and exit shots (made at run time, HouseKeep). */
	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> VanceFill;

	EScene5Beat Phase = EScene5Beat::Speech;
	float PhaseTime = 0.f;
	float SpeechEnd = 11.f;
	float SpeechPlayStart = 0.f;	// PhaseTime when the voice started (the mouth's clock); <0: no voice
	bool bPointed = false;
	bool bCutaway = false;		// the speech's cutaway to Vance has been
	bool bCutBack = false;		// and back to Pendleton
	bool bSeated = false;		// Vance in his seat
	bool bStood = false;
	bool bGoSaid = false;
	bool bReplySaid = false;
	float ReplyAt = 0.f;
	int32 RiseLeg = 0;			// 0 to RowExit, 1 to AisleAt, 2 there
	float RiseStuck = 0.f;
	FVector RiseLastAt = FVector::ZeroVector;
	FTransform MouthHeadRef;	// the head bone at the reference pose, component space
	bool bMouthPlaced = false;
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
