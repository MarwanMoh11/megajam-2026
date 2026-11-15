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
class AFollowSpot;
class AMark;
class ATakeDirector;
class AScannerWatcher;
class UBoxComponent;
class UPointLightComponent;
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
	Through,	// past the scanner: the director's own Scene 1 lines (and the lobby's, below)
	Lobby,		// W3 opening: in the lobby, the clerk has greeted him; the desk is the next thing
	Done		// signed in (or gone on into the hall): nothing more from the opening
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

	/** W3 opening: the searchlight is about to fire at Target, and the doorman at the scanner will react and speak on
	 *  its landing frame (ATakeDirector::HandleUse then holds back the spot's own @fire line, so his comes first). */
	bool ClaimsSpotLine(const AFollowSpot* Spot, const AActor* Target) const;

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
	FString RefuseLine = TEXT("DOORMAN: \"No match. Hold there, sir.\"");	// W3 opening: said by the man at the scanner

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

	// --- W3 opening: nobody skips the line (Marwan, Nov 15 note 3) ---------------------------------------------------
	// An invisible wall on the Pawn channel only (cameras, sight and aim pass) across the queue lane and the pavement
	// north of it, from just behind the guest ahead of Julian to the facade: he can stand on his place but cannot walk
	// past, round or through the line. At the front it stands just past the gold mark until the scanner is blind. Off
	// for good once he is through. Pushing into it: the guest ahead (or the doorman) turns to him and says so.

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Hold")
	bool bHoldLine = true;

	/** The wall's near face stands this far behind the place ahead of Julian's (X, cm). */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Hold")
	float HoldGap = 15.f;

	/** ...but never closer than this in front of his own place (X, cm): he can always stand on it. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Hold")
	float HoldMinLead = 60.f;

	/** At the front, the wall stands this far past the gold mark (X, cm) until the scanner is blind. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Hold")
	float FrontHoldLead = 70.f;

	/** The wall's far end (world X: the facade) and its sides (world Y), cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Hold")
	float HoldFarX = -3230.f;

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Hold")
	float HoldMinY = 190.f;

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Hold")
	float HoldMaxY = 1000.f;

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Hold")
	float HoldHeight = 260.f;

	/** Julian this close to the wall (cm, past his capsule) and walking into it: the line tells him (once a while). */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Hold")
	float NudgeDistance = 15.f;

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Hold")
	float NudgeCooldown = 6.f;

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Hold")
	FString NudgeGuestLine = TEXT("GUEST: \"Wait your turn, pal.\"");

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Hold")
	FString NudgeDoormanLine = TEXT("DOORMAN: \"One at a time, sir.\"");

	// --- W3 opening: the doorman at the scanner (Marwan, Nov 15 note 3) ----------------------------------------------
	// A uniformed man (an ACastMember spawned in play, kit Doorman) stands beside the scanner's gate, facing the line.
	// He waves each guest through, refuses Julian (RefuseLine, the stop hand), and on the frame Maya's searchlight
	// lands he turns to the light, shields his eyes and says LightLine, in shot. The scanner's own BlindedLine is
	// muted while he is there (he says it).

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Doorman")
	bool bDoorman = true;

	/** Where he stands (world, feet) and faces (degrees). */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Doorman", meta = (MakeEditWidget = true))
	FVector DoormanPoint = FVector(-3650.f, 150.f, 0.f);	// round B: about 1.2 m from the detector post, in front of the gate

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Doorman")
	float DoormanYaw = 137.f;	// toward the gold mark

	/** When the light lands and the player is on Maya, her look eases onto him this fast (s, real time; 0: no pan). */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Doorman", meta = (ClampMin = "0"))
	float LightPanSeconds = 0.45f;

	/** The speaker's lamp: a small warm light (no shadows) in front of the doorman and the clerk, at RestCandelas,
	 *  rising to SpeakCandelas while he speaks (SpeakSeconds) so the eye finds who is talking. 0 and 0: none. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Doorman", meta = (ClampMin = "0"))
	float LampRestCandelas = 25.f;

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Doorman", meta = (ClampMin = "0"))
	float LampSpeakCandelas = 90.f;

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Doorman", meta = (ClampMin = "0"))
	float LampRadius = 320.f;

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Doorman", meta = (ClampMin = "0"))
	float SpeakSeconds = 3.f;

	/** The lamp stands this far in front of his face and above his feet, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Doorman")
	FVector LampOffset = FVector(70.f, 0.f, 200.f);

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Doorman")
	FLinearColor DoormanTint = FLinearColor(0.07f, 0.09f, 0.18f);

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Doorman")
	FString LightLine = TEXT("DOORMAN: \"Who turned that light on?!\"");

	/** His act when the light hits (Refuse: a raised palm, read as a hand against the glare). */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Doorman")
	FName ShieldGesture = TEXT("Refuse");

	/** He reacts at real speed under slow time (like the voices), for this long, s. Off: he slows with the world. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Doorman")
	bool bReactAtRealSpeed = true;

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Doorman", meta = (ClampMin = "0"))
	float ReactSeconds = 2.5f;

	/** The beam lands this close to him (cm): he is dazzled and looks up at the lamp; farther, he looks where it lands. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Doorman")
	float DazzleRadius = 450.f;

	/** Maya's searchlight line after his (untaught takes; on the first take her lesson line follows instead). */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Doorman")
	FString MayaFireLine = TEXT("MAYA: \"Searchlight's on the scanner. Walk him through.\"");

	// --- W3 opening: the lobby (Marwan, Nov 15 note 4) ---------------------------------------------------------------
	// Through the doors, the clerk at the reception desk greets Julian: he turns to him, raises a hand and speaks (the
	// subtitle names him), and the camera eases toward him once a session (it steers the walk; the mouse takes it
	// back). The goal line names one thing: sign in at the DESK. On the desk's mark Julian takes paddle 41 (Lift),
	// the clerk answers, and the director's own lines (post the crew, then the hall) take over. Optional: walking on
	// into the hall ends it quietly. Off: as before.

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Lobby")
	bool bLobbyGreet = true;

	/** The clerk: the standing cast member nearest ClerkFrom (within ClerkFindRadius); moved to ClerkPoint (facing
	 *  ClerkYaw) at the take's start when ClerkPoint is set (the desk moved; the cast build still stands him at the
	 *  old one). World, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Lobby")
	FVector ClerkFrom = FVector(-2050.f, 1100.f, 0.f);

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Lobby")
	FVector ClerkPoint = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Lobby")
	float ClerkYaw = 180.f;

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Lobby")
	float ClerkFindRadius = 120.f;

	/** The mark Julian signs in on (its MarkName). */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Lobby")
	FString DeskMarkName = TEXT("THE DESK");

	/** Julian's feet this close to the desk's mark sign him in, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Lobby")
	float DeskRadius = 90.f;

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Lobby")
	FString GreetLine = TEXT("CLERK: \"Good evening, sir. Bidders sign in here, at the desk.\"");

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Lobby")
	FString DeskLine = TEXT("CLERK: \"Paddle forty-one, sir. The sale is through the hall doors.\"");

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Lobby")
	FString PaddleNote = TEXT("Paddle 41: JULIAN is a bidder");

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Lobby")
	FString DeskObjective = TEXT("Sign JULIAN in at the reception DESK: the clerk has a bidder's paddle");

	/** The camera's ease toward the clerk, s (real time), and the clerk's head height over his feet, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Queue|Lobby", meta = (ClampMin = "0"))
	float LobbyPanSeconds = 0.9f;

	UPROPERTY(EditAnywhere, Category = "Take|Queue|Lobby")
	float ClerkHeadHeight = 150.f;

	/** Bits in UTakeGameInstance::IntroShownMask (the camera ids use bits 0..2). */
	static constexpr uint8 BitLobbyPan = 1u << 7;

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

	// --- W3 opening ---
	/** The hold wall: placed, switched and the nudge (StepGuests). */
	void StepHold(ATakeDirector& Director);

	/** The doorman: spawned; waves guests through; the light reaction on the searchlight's landing frame. */
	void SpawnDoorman(ATakeDirector& Director);
	void StepDoorman(ATakeDirector& Director);

	/** The lobby's greeting and the desk (beats Lobby, Done). */
	void StepLobby(ATakeDirector& Director);

	/** The clerk found (and moved to the new desk) at setup. */
	void FindClerk();

	AMark* FindDeskMark(const ATakeDirector& Director) const;

	UPROPERTY(Transient)
	TObjectPtr<UBoxComponent> HoldBox;

	UPROPERTY(Transient)
	TObjectPtr<ACastMember> Doorman;

	/** The speaker lamps (doorman, clerk) and when each stops speaking (real time). */
	UPROPERTY(Transient)
	TObjectPtr<class UPointLightComponent> DoormanLamp;

	UPROPERTY(Transient)
	TObjectPtr<class UPointLightComponent> ClerkLamp;

	double DoormanSpeaksUntil = 0.0;
	double ClerkSpeaksUntil = 0.0;

	UPointLightComponent* MakeLamp(const ACastMember* Who, float Yaw);
	void StepLamps(ATakeDirector& Director);

	UPROPERTY(Transient)
	TWeakObjectPtr<ACastMember> Clerk;

	UPROPERTY(Transient)
	TWeakObjectPtr<AFollowSpot> Searchlight;

	bool bHoldDone = false;
	bool bHoldOn = false;
	double LastNudgeAt = -100.0;
	double ReactUntil = -1.0;
	double ClerkFaceAt = 0.0;
	int32 WavedGuest = INDEX_NONE;
	// --- end W3 opening

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
