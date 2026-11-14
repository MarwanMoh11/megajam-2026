// Scene 1's queue and its taught first switch (QA notes 1, 2, 10). Owner: scene1.
//
// THE BEAT SHEET (Scene 1, The Door: about 40 s done right, from the slate to the doors)
//  0  0 s    The slate. JULIAN stands in the queue for the scanner, three guests ahead of him, five behind. The goal
//            line: wait your turn. The DIRECTOR's cover line for Julian plays over it.
//  1  2-11 s The line moves. Every StepSeconds the guest at the front steps into the scanner, is passed (a beat), walks
//            in through the gate and the doors, and everyone behind steps up one place. The gap in front of Julian is
//            his cue: the player walks him up to keep his place. Dawdling costs nothing but time: the guests behind
//            him wait behind him, nobody passes him (they block like people, so he cannot cut the line either).
//  2  ~11 s  His turn: he is on the front place (the gold mark) and the scanner is clear. The scanner refuses him:
//            CHIEF "No match. Hold there, sir." and Julian shrugs.
//  3  ~13 s  The aside: JULIAN, on his earpiece, "Not on the list. Maya, I need a distraction." As he says her name,
//            time slows (ATakeDirector::BeginSlow) and the camera pans by itself up to MAYA at the searchlight on the
//            terrace over the doors (ATakePlayerController::StartGlance), so the player sees who she is and where.
//  4         Taught: MAYA "Cut to me with 2. My searchlight blinds the scanner." The goal line: [2]. The slow holds
//            until her line is done and the player has pressed 2 (2 works on any frame; WASD, the mouse or 1 hand the
//            camera back to Julian at once).
//  5         The flight to Maya (it runs on real time, so the slowed world does not stretch it). She lands: her look
//            eases down onto the scanner and she says "Nobody watches the crew. Swing my light onto the scanner: E."
//            The goal line: [E]. The slow holds until her line is done and the scanner is blinded.
//  6         E: the scanner is blinded, the gate opens (as before). Her searchlight line plays, then "It's blind. Cut
//            back to Julian with 1." The goal line: [1]. The slow holds until that line is done and the player cuts.
//  7         The flight back. On landing the slow eases out: the scanner's blind (6 s of world time, untouched by the
//            slow) counts down on the goal line and Julian walks through the gate. The Door, as before.
// A beat is taught (slowed, panned, spoken) only the first time in a session: its bit in the game instance's
// IntroShownMask (bits 3..6: the camera ids use 0..2) says it was. A later take (R or a blooper back to the street)
// starts Julian RetakeJulianPlace places from the front, the guests before him already inside, and plays the
// refusal and the aside only. No press is ever held back and nothing fails for dawdling; the slow also gives up
// after TutorHoldAfterLineSeconds of waiting, or TutorMaxSeconds in all.
// Old flow: ATakeDirector::bQueueScene1 off. The guests ahead of Julian are put inside at once (the lane is clear)
// and the line stands still: walk up the lane to the gold mark, as before the QA pass.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "QueueLine.generated.h"

class ACastMember;
class ATakeDirector;
class AScannerWatcher;
class UCapsuleComponent;

/** Where Scene 1 stands: the queue, then the beats of the taught first switch. */
UENUM()
enum class EQueueBeat : uint8
{
	Queue,		// waiting in line
	Refuse,		// the scanner's line
	Aside,		// Julian's earpiece line (the slow and the glance start with it when the switch is taught)
	AskCut,		// [2]
	AskUse,		// [E], on Maya
	AskBack,	// [1]
	Through		// the director's own Scene 1 lines from here
};

/** One guest in the line (or gone in). */
USTRUCT()
struct FQueueGuest
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<ACastMember> Member;

	/** Blocks Julian (the Pawn channel only) where the guest stands, so he queues like everyone. Off once in. */
	UPROPERTY()
	TObjectPtr<UCapsuleComponent> Blocker;

	/** The place it is walking to or standing on (-1: none). */
	int32 TargetPlace = -1;

	/** Going in: -1 not yet; 0 to the scan point; 1..WayIn.Num() the way in; WayIn.Num() + 1 the inside spot. */
	int32 Leg = -1;

	/** Seconds left standing at the scan point. */
	float ScanLeft = 0.f;

	/** Its inside spot (an index into InsideSpots, -1 none yet). */
	int32 InsideSpot = -1;

	/** Where its last WalkTo goes, how long ago it was given (world time) and how long the walk should take. */
	FVector LegTarget = FVector::ZeroVector;
	float OrderAge = 0.f;
	float OrderExpect = 0.f;

	bool bInside = false;
};

/**
 * The queue for the scanner on the street, and the beats of Scene 1 after it (see the beat sheet above).
 * tools/build_auction.py places one at the origin with its places; the guests are the ACastMembers that
 * tools/build_cast.py puts on the places' placeholders (found by distance on the first step). ATakeDirector calls
 * StepLine from StepScene, GetObjective from UpdateObjective, and IsTeaching from StepIntroLines.
 */
UCLASS()
class REEL_API AQueueLine : public AActor
{
	GENERATED_BODY()

public:
	AQueueLine();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** The first frame only: Julian onto his place in the line (all actors have begun play), then the tick stops. */
	virtual void Tick(float DeltaSeconds) override;

	/** The level's queue line (the one that began play last in this world), or null. */
	static AQueueLine* Get(const UObject* WorldContextObject);

	/** Step 6 (ATakeDirector::StepScene), every rolling frame: the line, then the beats. DeltaSeconds is the world's
	 *  (a slowed world slows the line); the beats' timers run on real time. */
	void StepLine(ATakeDirector& Director, float DeltaSeconds);

	/** The goal line while the queue or a beat has something to say; empty: the director's own Scene 1 lines. */
	FString GetObjective(const ATakeDirector& Director) const;

	/** A taught beat is running (the director holds back Maya's cover line: the tutor's lines introduce her). */
	bool IsTeaching() const { return bTeachingNow; }

	EQueueBeat GetBeat() const { return Beat; }

	// --- level data (tools/build_auction.py, Scene 1). Points are local to the actor, which stands at the origin ----

	/** The places in the line, front first. Place 0 is the gold mark where the front guest waits for the scanner. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue", meta = (MakeEditWidget = true))
	TArray<FVector> Places;

	/** Julian's place on the session's first take (0 = the front). The guests stand on the other places. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue", meta = (ClampMin = "0"))
	int32 JulianPlace = 3;

	/** His place on every later take of the session: the guests before it are already inside (instant retry). */
	UPROPERTY(EditAnywhere, Category = "Take|Queue", meta = (ClampMin = "0"))
	int32 RetakeJulianPlace = 1;

	/** Julian faces this way on his place at the take's start (degrees; 0 = east, up the lane to the doors). */
	UPROPERTY(EditAnywhere, Category = "Take|Queue")
	float JulianStartYaw = 0.f;

	/** Where a guest stands to be scanned: in the scanner's frame, before the gate. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue", meta = (MakeEditWidget = true))
	FVector ScanPoint = FVector::ZeroVector;

	/** The way in from the scan point: through the gate and the doors. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue", meta = (MakeEditWidget = true))
	TArray<FVector> WayIn;

	/** Where the guests who went in stand, one each in turn (repeats are spread by InsideSpread). */
	UPROPERTY(EditAnywhere, Category = "Take|Queue", meta = (MakeEditWidget = true))
	TArray<FVector> InsideSpots;

	// --- the line's tunables ---------------------------------------------------------------------------------------

	/** The first guest steps into the scanner this long after the take starts rolling, s (world time). */
	UPROPERTY(EditAnywhere, Category = "Take|Queue", meta = (ClampMin = "0"))
	float FirstStepSeconds = 2.f;

	/** Then the line steps up every this many seconds (world time). */
	UPROPERTY(EditAnywhere, Category = "Take|Queue", meta = (ClampMin = "0.5"))
	float StepSeconds = 3.f;

	/** A guest stands this long at the scan point, s. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue", meta = (ClampMin = "0"))
	float ScanSeconds = 1.2f;

	/** The guests' walking speed, cm/s (the cast contract's WalkTo). */
	UPROPERTY(EditAnywhere, Category = "Take|Queue", meta = (ClampMin = "10"))
	float GuestWalkSpeed = 110.f;

	/** The gesture a guest makes at the scanner (the cast contract: Refuse, Wave, Applaud, Talk, LookAt, Sip). */
	UPROPERTY(EditAnywhere, Category = "Take|Queue")
	FName ScanGesture = TEXT("Talk");

	/** Julian's gesture when the scanner refuses him (ATakeCharacter::PlayGesture: Use, Lift, Shrug). */
	UPROPERTY(EditAnywhere, Category = "Take|Queue")
	FName RefuseGesture = TEXT("Shrug");

	/** A cast member this close to a place (2D, cm) on the first step is that place's guest. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue", meta = (ClampMin = "1"))
	float GuestFindRadius = 60.f;

	/** Julian counts as on a place this close to it (2D, feet, cm). */
	UPROPERTY(EditAnywhere, Category = "Take|Queue", meta = (ClampMin = "10"))
	float PlaceRadius = 70.f;

	/** A guest counts as arrived on its place this close (2D, cm). */
	UPROPERTY(EditAnywhere, Category = "Take|Queue", meta = (ClampMin = "1"))
	float GuestArriveRadius = 25.f;

	/** Repeated inside spots are spread this far apart, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue")
	float InsideSpread = 90.f;

	/** The guests block Julian like people (a capsule on the Pawn channel only; cameras, sight and the aim pass). */
	UPROPERTY(EditAnywhere, Category = "Take|Queue")
	bool bGuestsBlock = true;

	UPROPERTY(EditAnywhere, Category = "Take|Queue", meta = (ClampMin = "10"))
	float BlockRadius = 40.f;

	/** The guest behind Julian stops this far from his feet (2D, cm): more than its blocker and his capsule. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue", meta = (ClampMin = "0"))
	float KeepClearOfJulian = 95.f;

	UPROPERTY(EditAnywhere, Category = "Take|Queue", meta = (ClampMin = "10"))
	float BlockHalfHeight = 90.f;

	// --- the beats (the words go on screen as subtitles; C:\Jam\Builds\qa\lines\scene1.json lists them for voicing) ---

	/** Teach the first switch (slow, pan, lines) the first time each beat comes up in a session. Off: the refusal and
	 *  the aside only, then the director's own lines. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Tutor")
	bool bTeach = true;

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Tutor")
	FString RefuseLine = TEXT("CHIEF: \"No match. Hold there, sir.\"");

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Tutor")
	FString AsideLine = TEXT("JULIAN: \"Not on the list. Maya, I need a distraction.\"");

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Tutor")
	FString CutLine = TEXT("MAYA: \"Cut to me with 2. My searchlight blinds the scanner.\"");

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Tutor")
	FString UseLine = TEXT("MAYA: \"Nobody watches the crew. Swing my light onto the scanner: E.\"");

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Tutor")
	FString BackLine = TEXT("MAYA: \"It's blind. Cut back to Julian with 1.\"");

	/** The world's speed while a beat is taught (ATakeDirector::BeginSlow's Scale). */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Tutor", meta = (ClampMin = "0.05", ClampMax = "1"))
	float TutorSlowScale = 0.15f;

	/** The camera frames Maya this high above her actor (her capsule's centre), cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Tutor")
	float GlanceTargetHeight = 40.f;

	/** Maya's look eases onto the scanner over this long (real time), s. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Tutor", meta = (ClampMin = "0"))
	float LookPanSeconds = 0.8f;

	/** A line counts as spoken after at least this long, and no sooner than its voice ends (real time). An unvoiced
	 *  line counts as LineBaseSeconds + LinePerCharSeconds a character. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Tutor", meta = (ClampMin = "0"))
	float LineBaseSeconds = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Tutor", meta = (ClampMin = "0"))
	float LinePerCharSeconds = 0.05f;

	/** The voice starts when the HUD shows the subtitle: a line is never done before this long, s. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Tutor", meta = (ClampMin = "0"))
	float LineVoiceStartSeconds = 0.35f;

	/** An unvoiced line (no voice rose after it) is done this long after its reading time, s. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Tutor", meta = (ClampMin = "0"))
	float LineUnvoicedPad = 0.4f;

	/** A line is done this long after its reading time whatever the voice does (the beats never hang), s. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Tutor", meta = (ClampMin = "1"))
	float LineMaxExtraSeconds = 8.f;

	/** Maya's own searchlight line (the audio's, on the fire): the back line waits this long for it at least, s. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Tutor", meta = (ClampMin = "0"))
	float FireLineSeconds = 2.2f;

	/** The slow gives up after waiting this long for the press once the line is done (real time), s. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Tutor", meta = (ClampMin = "0"))
	float TutorHoldAfterLineSeconds = 10.f;

	/** And never lasts longer than this in all (real time), s. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Tutor", meta = (ClampMin = "0"))
	float TutorMaxSeconds = 40.f;

	/** Bits in UTakeGameInstance::IntroShownMask (the camera ids use bits 0..2). */
	static constexpr uint8 BitQueueWalked = 1u << 3;
	static constexpr uint8 BitTaughtCut = 1u << 4;
	static constexpr uint8 BitTaughtUse = 1u << 5;
	static constexpr uint8 BitTaughtBack = 1u << 6;

protected:
	/** The first step: finds the guests, makes their blockers, builds the line; on a later take or the old flow, puts
	 *  the guests before Julian's place inside at once. */
	void SetupLine(ATakeDirector& Director);

	/** Julian onto his place (a later take: RetakeJulianPlace), facing JulianStartYaw. */
	void PlaceJulian(ATakeDirector* Director);

	/** The line: the front steps in on the timer, everyone behind steps up, the guests going in walk on. */
	void StepGuests(ATakeDirector& Director, float DeltaSeconds);

	/** The beats after the queue. */
	void StepBeats(ATakeDirector& Director);

	void EnterBeat(ATakeDirector& Director, EQueueBeat NewBeat);

	/** Flashes Line as a subtitle and starts its clock (LineDone). */
	void SayLine(ATakeDirector& Director, const FString& Line);

	/** The last line said is done: its time has passed and no voice is playing. */
	bool LineDone(const ATakeDirector& Director) const;

	/** BeginSlow / EndSlow on a change only. */
	void SetSlow(ATakeDirector& Director, bool bSlow);

	/** The beat is taught this session for the first time (and marks it taught). */
	bool TakeTeachBit(ATakeDirector& Director, uint8 Bit);

	/** Julian's index in Line (-1: not in it). */
	int32 JulianIndex() const { return Line.Find(INDEX_NONE); }

	/** The guest walks to Point (once per new target). */
	void WalkGuest(FQueueGuest& Guest, const FVector& Point);

	/** The guest is put at Point at once (a later take: already inside). */
	void PutGuest(FQueueGuest& Guest, const FVector& Point, float Yaw);

	/** The guest has reached its last WalkTo's point (or stopped walking a moment after it was given). */
	bool GuestArrived(const FQueueGuest& Guest) const;

	/** No guest is walking to or standing at the scan point. */
	bool ScanPointClear() const;

	/** The guest's blocker on or off (on: it follows the guest). */
	void SetBlocker(FQueueGuest& Guest, bool bOn);

	/** Real seconds since the world began (undilated). */
	double RealNow() const;

	/** The next inside spot (spread when they repeat). */
	FVector NextInsideSpot(FQueueGuest& Guest);

	FVector ToWorld(const FVector& Local) const;

	AScannerWatcher* FindScanner(const ATakeDirector& Director) const;

	UPROPERTY(Transient)
	TArray<FQueueGuest> Guests;

	/** The line, front first: guest indices, INDEX_NONE for Julian. */
	TArray<int32> Line;

	EQueueBeat Beat = EQueueBeat::Queue;

	bool bSetUp = false;
	bool bJulianPlaced = false;
	bool bOldFlow = false;
	bool bSlowing = false;
	bool bTeachingNow = false;
	bool bUseStarted = false;
	bool bBackSaid = false;
	bool bBackTaught = false;
	bool bLineVoiceSeen = false;

	float StepTimer = 0.f;
	int32 InsideCount = 0;

	/** Real time (the world's real seconds) the beat, the last line and the slow started at. */
	double BeatStartedAt = 0.0;
	double LineSaidAt = 0.0;
	float LineSeconds = 0.f;
	double SlowStartedAt = 0.0;
	double LineDoneSince = -1.0;
};
