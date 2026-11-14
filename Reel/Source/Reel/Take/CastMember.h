// Step 5, characters and props; QA pass (Nov 14), package cast: acting and variety.
//
// One member of the cast, costumed from basic shapes on a template mannequin (Manny or Quinn), or on a generated hero
// mesh (HeroMesh: no costume shells). tools/build_cast.py places them all:
// - A guest, staff, a queue member or a band player: its own body. It plays mocap clips when they exist
//   (/Game/Take/Anim/Mocap, tools/mocap_clips.json: idles from a random start at a varied rate, seated idles, the
//   instruments, gestures blended in and out) and falls back to the template idle with acts built from bones
//   (UCastAnimInstance, CastMemberAnim.h). Seated members sit with their thighs on the seat top (fitted from the clip).
//   Each one thinks a few times a second: looks (the podium, the speaker, a neighbour, Julian as he passes),
//   conversation turns in a group, a sip, a weight shift, applause when Vance finishes speaking. WalkTo walks it on
//   the template's walk blend space (the queue).
// - Attached to a watcher (a PatrolGuard: guards, the doorman, Vance, the bodyguard): its body replaces the grey
//   cylinder and walks on the blend space at the watcher's speed, scanning with the head; Vance gestures while he
//   speaks; the doorman's stop hand and wave come from PlayGesture.
// - Attached to a character (Julian, Maya, Vex): it dresses the character's own mesh. For an act (PlayGesture: Use,
//   Lift, Shrug, Headset, Bow) its own body takes the character's pose each frame, plays the act over it, and hands
//   back when done. Julian's bow at The Bow is one of these acts.
// Only people near the camera and on screen animate (AnimateDistance); the console variable Reel.Cast.Acting 0 (or
// bActing off) freezes the acting as step 5 had it. Nothing here is a rule: no collision, no stencil.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CastMemberAnim.h"
#include "CastMember.generated.h"

class USkeletalMeshComponent;
class USkeletalMesh;
class UStaticMeshComponent;
class UAnimationAsset;
class UAnimSequenceBase;
class UCastAnimInstance;

/** The costume (and instrument) a cast member wears. In Python: unreal.CastKit. */
UENUM(BlueprintType)
enum class ECastKit : uint8
{
	None,
	GuestTux,		// dinner jacket: shirt front, bow tie, hair
	GuestTails,		// tailcoat, white tie, sometimes a top hat
	GuestGown,		// a gown and hair (Quinn)
	GuestGownHat,	// a gown, a stole and a small hat (Quinn)
	Staff,			// bartender, attendant: shirt front, bow tie, waistcoat
	Guard,			// peaked cap, epaulettes, belt
	Doorman,		// tall peaked cap, epaulettes, a long coat's skirts
	Vance,			// tall top hat, tails, a cane
	Bodyguard,		// broad shoulders, dark glasses, bald
	BandBass,		// an upright bass
	BandTrumpet,	// a muted trumpet at the lips
	BandDrums,		// a drum kit (seated)
	BandPiano,		// an upright piano (seated)
	BandSinger,		// a gown; a microphone on a stand (Quinn)
	Julian,			// bow tie on his white jacket
	Maya,			// headset and tool belt
	Vex				// headset and beanie
};

UCLASS()
class REEL_API ACastMember : public AActor
{
	GENERATED_BODY()

public:
	ACastMember();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	// --- the contract (COMMON.md): scene1, heist and devices call these ------------------------------------------------

	/** Walks to Target (2D, on the walk blend space) at Speed cm/s, easing in and out, facing the way it goes. Not for
	 *  seated members, watchers or the crew (they ignore it). */
	UFUNCTION(BlueprintCallable, Category = "Take|Cast")
	void WalkTo(const FVector& Target, float Speed = 110.f);

	UFUNCTION(BlueprintCallable, Category = "Take|Cast")
	bool IsWalking() const { return bWalkingTo; }

	/** Turns (eased, at TurnRate) to face Yaw, degrees (world). */
	UFUNCTION(BlueprintCallable, Category = "Take|Cast")
	void FaceYaw(float Yaw);

	/** Refuse (the stop hand), Wave, Hello, Applaud, Talk, Listen, Laugh, Sip, Nod, Shrug, Point, CheckWatch,
	 *  LookAround, Headset, Use, Lift, Bow, Speech; LookAt (the head and spine turn to Julian); None stops the act.
	 *  A mocap clip when one exists, else built from bones. On a crew member's dresser: the character's act. */
	UFUNCTION(BlueprintCallable, Category = "Take|Cast")
	void PlayGesture(FName Gesture);

	// --- more of the same ---------------------------------------------------------------------------------------------

	/** PlayGesture toward a world point (Wave sweeps toward it, Point and Use reach to it, LookAt looks at it). */
	UFUNCTION(BlueprintCallable, Category = "Take|Cast")
	void PlayGestureAt(FName Gesture, const FVector& WorldPoint);

	/** The head and the top of the spine turn to WorldPoint for Seconds (eased), over whatever plays. */
	UFUNCTION(BlueprintCallable, Category = "Take|Cast")
	void LookAtPoint(const FVector& WorldPoint, float Seconds = 2.5f);

	UFUNCTION(BlueprintCallable, Category = "Take|Cast")
	void StopWalking();

	/** An act is playing. */
	UFUNCTION(BlueprintCallable, Category = "Take|Cast")
	bool IsGesturing() const;

	/** The cast member on Actor: the actor itself, or the one attached to it (a watcher, a character). Null if none. */
	static ACastMember* FindOn(const AActor* Actor);

	/** Guests and staff within Radius of Center play Gesture, each with probability Share, a little out of step. */
	UFUNCTION(BlueprintCallable, Category = "Take|Cast", meta = (WorldContext = "WorldContext"))
	static void CrowdGesture(const UObject* WorldContext, FName Gesture, FVector Center, float Radius, float Share = 0.75f);

	/** Test hook (Python in play): Julian bows now. */
	UFUNCTION(BlueprintCallable, Category = "Take|Cast")
	void BowNow();

	// --- the costume ---------------------------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Cast")
	ECastKit Kit = ECastKit::GuestTux;

	/** The body's colour (the jacket, the gown): tools/build_cast.py picks it from a muted 1940s palette. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Cast")
	FLinearColor Tint = FLinearColor(0.28f, 0.28f, 0.28f);

	/** Quinn's mesh instead of Manny's (the gowns force it). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast")
	bool bQuinn = false;

	UPROPERTY(EditAnywhere, Category = "Take|Cast")
	bool bSeated = false;

	/** Uniform scale of the body and its pieces (height and build; build_cast.py varies it per person). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast")
	float BodyScale = 1.f;

	/** Seed of the variety (hair, hats, glasses, skin, accents...). -1: from the actor's name. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Variety")
	int32 LookSeed = -1;

	/** Variety on (off: step 5's costumes in the body's tint). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Variety")
	bool bVary = true;

	/** A skin-toned head over the mannequin's (variety's skin tones). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Variety")
	bool bSkinShell = true;

	/** Holds a champagne coupe in the right hand (and sips from it). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Variety")
	bool bHoldsGlass = false;

	/** Real instrument props stand where this player plays (tools/sets/hall_band.json): no shape instrument, stand or
	 *  stool (the trumpet in the hands stays). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast")
	bool bInstrumentProps = false;

	/** A generated, rigged mesh on the mannequin skeleton (package heroes): replaces the mannequin, no tint, no costume
	 *  shells (instruments and props stay). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast")
	TSoftObjectPtr<USkeletalMesh> HeroMesh;

	// --- poses and clips -----------------------------------------------------------------------------------------------

	/** The standing fallback idle (no StandIdle clip). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Pose")
	TObjectPtr<UAnimationAsset> PoseAnim;

	/** Where the fallback idle starts when not idling (s). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Pose")
	float PoseTime = 0.f;

	/** The seated fallback: this clip at SeatedTime, bent at the hips and knees (no SitIdle clip). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Pose")
	TObjectPtr<UAnimationAsset> SeatedAnim;

	UPROPERTY(EditAnywhere, Category = "Take|Cast|Pose")
	float SeatedTime = 0.12f;

	/** The seated fallback when the seat cannot be fitted from the clip: the body this far below the seat top, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Pose")
	float SeatedDrop = 86.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cast|Pose")
	float SeatedHipDegrees = 90.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cast|Pose")
	float SeatedKneeDegrees = 90.f;

	/** The seated clip's hip height (tools/mocap_clips.json hip_height): the body is placed so it meets the seat top.
	 *  0: fitted from the clip itself (the hip joints SeatThighRadius above the seat top). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Seat")
	float SeatHipHeight = 0.f;

	/** The thighs' half thickness: the hip joints sit this far above the seat top when fitted, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Seat")
	float SeatThighRadius = 8.f;

	/** Where the hip joints sit relative to the seat centre (the actor), forward cm (negative: toward the back). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Seat")
	float SeatHipForward = -3.f;

	/** Added to the seated body's height, cm (tuning). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Seat")
	float SeatOffsetZ = 0.f;

	// --- W3 cast2 ---
	/** The seated fallback's legs fitted per person: the thighs on the cushion, the feet flat on the floor (found by a
	 *  trace in front of the seat), whatever the body's height. Off: SeatedHipDegrees / SeatedKneeDegrees for everyone. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Seat")
	bool bFitLegs = true;

	/** The shins' natural lean forward from vertical when they reach the floor, degrees. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Seat")
	float SeatShinForward = 8.f;

	/** At most this far the thighs slope down (short legs) or the shins lean forward (long legs), degrees. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Seat")
	float SeatMaxThighDrop = 14.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cast|Seat")
	float SeatMaxShinForward = 32.f;

	/** The seat top's height over the floor when the trace finds no floor, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Seat")
	float SeatDefaultHeight = 45.f;

	/** The walk (watchers, WalkTo). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Pose")
	TObjectPtr<UAnimationAsset> WalkBlendSpace;

	/** Standing: the idle loops from its own start at its own rate (1 +/- IdleRateSpread). Off: still at PoseTime. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Pose")
	bool bIdle = true;

	UPROPERTY(EditAnywhere, Category = "Take|Cast|Pose", meta = (ClampMin = "0", ClampMax = "0.5"))
	float IdleRateSpread = 0.1f;

	/** Play the mocap clips that exist (off: everything built from bones). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Acting")
	bool bUseMocap = true;

	/** Where the clips are: <MocapPath><Key>.<Key>. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Acting")
	FString MocapPath = TEXT("/Game/Take/Anim/Mocap/");

	/** The body casts a shadow (the pieces never do). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast")
	bool bBodyShadow = true;

	// --- acting --------------------------------------------------------------------------------------------------------

	/** Looks, gestures, conversation, applause (with Reel.Cast.Acting 1). Off: step 5's still cast. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Acting")
	bool bActing = true;

	/** Beyond this from the camera (or off screen), the body holds its pose and does not animate, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Acting")
	float AnimateDistance = 2600.f;

	/** How often a member decides what to do next, s (spread +/- 30% per member). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Acting")
	float ThinkInterval = 0.15f;

	/** Julian walking past within this, cm: a glance (GlanceChance of the time). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Acting")
	float GlanceRadius = 380.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cast|Acting", meta = (ClampMin = "0", ClampMax = "1"))
	float GlanceChance = 0.55f;

	/** Standing guests this close form a conversation group, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Acting")
	float GroupRadius = 230.f;

	/** PlayGesture("LookAt") with no point, on a guest: the guest looks here for ScreenLookSeconds (the screen at the
	 *  end of the dais, for the house dimmer's preview); on a watcher it looks at Julian. World cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Acting")
	FVector ScreenPoint = FVector(4390.f, 0.f, 520.f);

	UPROPERTY(EditAnywhere, Category = "Take|Cast|Acting")
	float ScreenLookSeconds = 10.f;

	/** Where the seated look when Vance is not found (the podium), world cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Acting")
	FVector PodiumPoint = FVector(3890.f, -480.f, 175.f);

	/** Vance's left hand on the podium while he speaks, his frame (x forward, y right, z up from his feet), cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Acting")
	FVector PodiumHand = FVector(50.f, -16.f, 121.f);

	/** Turning to FaceYaw or to the walk's way, degrees per second. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Acting")
	float TurnRate = 160.f;

	/** The walk's ease: speeding up and slowing down, cm/s^2. */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Acting")
	float WalkAccel = 220.f;

	/** Clap seconds when Vance finishes speaking, and the share of the hall that claps (first speech; later ones half). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Acting")
	float ApplauseSeconds = 3.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cast|Acting", meta = (ClampMin = "0", ClampMax = "1"))
	float ApplauseShare = 0.8f;

	// --- Julian's bow ----------------------------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, Category = "Take|Cast|Bow")
	FName BowTrigger = TEXT("Bow");

	UPROPERTY(EditAnywhere, Category = "Take|Cast|Bow")
	float BowSeconds = 0.9f;

	/** The bow built from bones (no Bow clip): degrees at the bottom (the head adds about 12). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Bow")
	float BowDegrees = 42.f;

	UPROPERTY(EditAnywhere, Category = "Take|Cast|Bow")
	float BowHeadDegrees = 12.f;

	/** The crew's act blends out once the character moves faster than this, cm/s (the bow; any act while walking
	 *  plays on the upper body only). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Bow")
	float BowCancelSpeed = 60.f;

	/** A crew member touches the headset when a voice line starts (the one the player is on). */
	UPROPERTY(EditAnywhere, Category = "Take|Cast|Acting")
	bool bHeadsetOnVoice = true;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<USceneComponent> Root;

	/** The mannequin (on the crew: a copy shown only during an act). */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<USkeletalMeshComponent> Body;

private:
	/** The mesh the pieces go on: Body, or the attach parent character's mesh. */
	USkeletalMeshComponent* GetWearer() const;
	UCastAnimInstance* GetAnim() const;

	void SetupBody();
	void BuildPieces(bool bTransient);
	void HideParentStandIn();
	void ReattachPieces(USceneComponent* Target);

	/** Plays Asset as the body's base (looped from StartTime at Rate), cross-fading from the last pose. */
	void PlayBase(UAnimationAsset* Asset, bool bLoop, float Rate, float StartTime);
	/** The idle (or seated idle, or instrument) this member returns to. */
	void PlayRestBase();
	/** The seated body placed so its thighs (or the clip's hip height) meet the seat top. */
	void FitSeat();

	/** A clip under MocapPath, null if missing or on another skeleton (cached). */
	UAnimSequenceBase* FindClip(FName Key) const;

	void StartAct(ECastAct Act, float Seconds, bool bPoint, const FVector& Point);
	ECastAct ActFromName(FName Gesture) const;
	FName ClipKeyFor(ECastAct Act) const;

	bool IsActingNow() const;
	bool UpdateGate();
	void Think(float Now);
	void ThinkLook(float Now);
	void ThinkSeated(float Now);
	void ThinkStanding(float Now);
	void ThinkWatcher(float Now);
	void ThinkBand();
	void PushActing();

	void TickWalk(float DeltaSeconds);		// on a watcher: the blend space at the parent's speed
	void TickOwnWalk(float DeltaSeconds);	// WalkTo and FaceYaw
	void TickCrew(float DeltaSeconds);
	void StartCrewAct(ECastAct Act, bool bPoint, const FVector& Point);
	void EndCrewAct();

	FVector HeadWorld() const;
	float Draw(uint32 Salt) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Pieces;
	TArray<FName> PieceBones;

	/** What the body is told (copied to the anim instance). */
	UPROPERTY(Transient)
	FCastActing Acting;

	UPROPERTY(Transient)
	TObjectPtr<UAnimationAsset> RestBase;

	bool bDressParent = false;
	bool bWalker = false;
	bool bBandKit = false;
	bool bRestIsClip = false;		// a mocap clip is the rest base (idle, seated idle, instrument)
	bool bSeatClip = false;			// the seated rest base is a seated clip (no bend)
	bool bSeatFitted = false;
	bool bOnWalkSpace = false;
	bool bBodyAnimating = true;
	bool bFrozen = false;
	int32 FramesAlive = 0;
	uint32 Seed = 0;

	// Walking (WalkTo) and turning (FaceYaw).
	bool bWalkingTo = false;
	bool bTurning = false;
	FVector WalkTarget = FVector::ZeroVector;
	float WalkSpeedGoal = 110.f;
	float WalkSpeedNow = 0.f;
	float TargetYaw = 0.f;
	float StoppedAt = -1.f;

	// On a watcher.
	FVector LastParentLocation = FVector::ZeroVector;
	float WalkSpeedSmoothed = 0.f;
	bool bVanceSpoke = false;
	int32 SpeechCount = 0;

	// Thinking.
	float NextThink = 0.f;
	float ActUntil = -1.f;
	float NextLookChange = 0.f;
	float NextLean = 0.f;
	float NextIdleAct = 0.f;
	float LookOverrideUntil = -1.f;
	FVector LookOverridePoint = FVector::ZeroVector;
	bool bLookOverrideJulian = false;
	float GlanceFrom = -1.f, GlanceUntil = -1.f;
	bool bJulianNear = false;
	int32 GlanceCount = 0;
	int32 LookMode = 0;			// 0 none, 1 a point, 2 the podium (Vance), 3 a member, 4 Julian
	FVector LookFixed = FVector::ZeroVector;
	TWeakObjectPtr<ACastMember> LookMember;
	float ScanPeriod = 7.f;
	float ScanAmplitude = 30.f;
	int32 LastTurn = -1;
	ECastAct PendingAct = ECastAct::None;
	float PendingAt = -1.f;
	float PendingSeconds = 0.f;

	// Groups (built once per world from all members).
	friend struct FCastWorld;
	TArray<TWeakObjectPtr<ACastMember>> Group;	// conversation group, me included, sorted
	TArray<TWeakObjectPtr<ACastMember>> Neighbours;
	uint32 GroupKey = 0;
	float TurnSeconds = 5.f;
	bool bTalking = false;

	// The crew.
	bool bCrewAct = false;
	float CrewActUntil = -1.f;
	bool bBowFiredLast = false;
	float VoiceLeftLast = 0.f;
	float NextHeadset = 0.f;
	uint8 WearerTickOption = 0;
};
