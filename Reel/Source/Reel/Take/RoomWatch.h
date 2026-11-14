// The room watches (QA wave 3, Marwan's second feedback, item 6). Owner: heist2. The director's side is
// TakeDirectorHeist.cpp (StepHeist calls StepRoom every rolling frame while the heist trick is on).
//
// "He should not be able to walk up casually just because the guards are distracted: the ordinary guests should see
// him too and react." A hundred seated bidders face the dais. In house light, Julian on the floor between the front
// row and the dais (the FRONT zone) or on the dais (the DAIS zone) is out of place, and the room notices:
//  - the frame he steps in: the few guests nearest him glance at him (the first cue, same frame);
//  - after NoticeSeconds (about 1 s) in view: the nearest guests turn to him, two of them point, a murmur rises
//    (the sound cue and a guest's line, on that frame); the first time in a take, Vex's line teaches the answer;
//  - after SeenSeconds (2.5 s): the blooper THE ROOM SAW HIM (EBlooperReason::SeenAtCase).
// Out of the zones (at his bidder's place, in the aisle, among the crowd) he is one of them: the count cools off.
// With the house dark (Maya's dimmer: the room is watching the preview on the screen) nobody sees him at all.
// Distracting the guards does nothing here: only the dark crosses the front.
//
// The zones come from tools/sets/hall_zones.json ("front_floor", "dais"; stage writes it), placed by heist2's QA block
// in the Scene 3 region of tools/build_auction.py. The switch is bRoomWatches (off: the old rules, guards only).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomWatch.generated.h"

class ACastMember;
class ATakeDirector;
class UBoxComponent;

/** What the room did this frame. */
enum class ERoomReaction : uint8
{
	None,		// nothing new
	Glance,		// he stepped into view: the nearest few glance at him
	Murmur,		// NoticeSeconds in view: heads turn, a murmur
	Seen		// SeenSeconds in view: the blooper
};

UCLASS()
class REEL_API ARoomWatch : public AActor
{
	GENERATED_BODY()

public:
	ARoomWatch();

	/** Fits the two editor-only boxes to the zones, so the map shows them. */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** The switch: off, the seated guests never see Julian (the old rules: guards, drone, beam, Vance only). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Room")
	bool bRoomWatches = true;

	/** The floor between the front row and the dais, world cm (z: from the floor to head height). */
	UPROPERTY(EditAnywhere, Category = "Take|Room")
	FVector FrontMin = FVector(3100.f, -1200.f, -100.f);

	UPROPERTY(EditAnywhere, Category = "Take|Room")
	FVector FrontMax = FVector(3600.f, 1200.f, 250.f);

	/** The dais, world cm (z: from below its top to head height above it). */
	UPROPERTY(EditAnywhere, Category = "Take|Room")
	FVector DaisMin = FVector(3600.f, -1300.f, -100.f);

	UPROPERTY(EditAnywhere, Category = "Take|Room")
	FVector DaisMax = FVector(4400.f, 1300.f, 300.f);

	/** The front rows: this much around the FRONT zone (on the floor) counts as "at the front", where the goal line
	 *  names the dimmer. cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Room", meta = (ClampMin = "0"))
	float ApproachMargin = 450.f;

	/** In view this long, the nearest guests turn to him and a murmur rises, s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Room", meta = (ClampMin = "0.1"))
	float NoticeSeconds = 1.f;

	/** In view this long, the room has seen him: the blooper, s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Room", meta = (ClampMin = "0.2"))
	float SeenSeconds = 2.5f;

	/** On the dais the count runs this much faster (he is up on the stage, lit). */
	UPROPERTY(EditAnywhere, Category = "Take|Room", meta = (ClampMin = "0.1"))
	float DaisRate = 1.25f;

	/** Out of view (or in the dark) the count falls this fast, seconds a second. Below NoticeSeconds / 2 the murmur
	 *  is over and can rise again. */
	UPROPERTY(EditAnywhere, Category = "Take|Room", meta = (ClampMin = "0"))
	float CoolRate = 1.f;

	/** The guests nearest him who glance as he steps into view. */
	UPROPERTY(EditAnywhere, Category = "Take|Room", meta = (ClampMin = "0"))
	int32 GlanceGuests = 3;

	/** The guests nearest him who turn to him at the murmur. */
	UPROPERTY(EditAnywhere, Category = "Take|Room", meta = (ClampMin = "0"))
	int32 MurmurGuests = 14;

	/** Of those, the nearest this many point at him. */
	UPROPERTY(EditAnywhere, Category = "Take|Room", meta = (ClampMin = "0"))
	int32 PointGuests = 2;

	/** Only guests this close react, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Room", meta = (ClampMin = "100"))
	float ReactRadius = 2500.f;

	/** How long the heads stay on him after the murmur, s (renewed while he stays in view). */
	UPROPERTY(EditAnywhere, Category = "Take|Room", meta = (ClampMin = "0.2"))
	float StareSeconds = 3.f;

	/** The murmur's sound (UTakeAudioComponent::PlayCue; harmless while it is not imported). */
	UPROPERTY(EditAnywhere, Category = "Take|Room")
	FName MurmurCue = TEXT("sfx_murmur");

	UPROPERTY(EditAnywhere, Category = "Take|Room", meta = (ClampMin = "0", ClampMax = "2"))
	float MurmurVolume = 0.8f;

	/** What a guest says at the murmur (subtitles, in turn; listed in C:\Jam\Builds\qa\lines\heist2.json). */
	UPROPERTY(EditAnywhere, Category = "Take|Room|Lines")
	TArray<FString> MurmurLines;

	/** The first murmur of a take: the lesson (Vex, over the earpiece). */
	UPROPERTY(EditAnywhere, Category = "Take|Room|Lines")
	FString TeachLine = TEXT("VEX: \"Not in front of the whole room. Back in the crowd. Maya: take the house lights down.\"");

	/** The room has seen him (the blooper follows). */
	UPROPERTY(EditAnywhere, Category = "Take|Room|Lines")
	FString SeenLine = TEXT("VANCE: \"Sir! Step back from the stage!\"");

	/** Julian (feet) is in a watched zone. */
	bool IsInView(const FVector& Feet) const;

	/** Julian (feet) is on the dais. */
	bool IsOnDais(const FVector& Feet) const;

	/** Julian (feet) is at the front: in a zone or within ApproachMargin of the front zone. */
	bool IsAtFront(const FVector& Feet) const;

	/** One rolling frame. bRoomBlind: the house is dark (everyone is on the screen). Head: where the guests look.
	 *  Plays the reactions itself (glances, LookAtPoint, PlayGestureAt, the murmur cue and line through Director);
	 *  returns what happened. Seen is returned once; the caller requests the blooper. */
	ERoomReaction StepRoom(float DeltaSeconds, const FVector& Feet, const FVector& Head, bool bRoomBlind,
		ATakeDirector* Director);

	/** A new take or a restore: the count at 0, nobody murmuring. */
	void ResetRoom();

	/** Seconds of being seen, 0..SeenSeconds. */
	float GetExposure() const { return Exposure; }

	bool IsMurmuring() const { return bMurmured; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<USceneComponent> Root;

	/** Editor-only: the FRONT zone (amber) and the DAIS zone (red). */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UBoxComponent> FrontBox;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UBoxComponent> DaisBox;

	/** Every guest in the level once (not the crew's costume pieces). */
	void GatherGuests();

	/** The Count guests nearest Head turn their heads to it for Seconds; the nearest Pointers of them point. */
	int32 ReactNearest(const FVector& Head, int32 Count, int32 Pointers, float Seconds);

	TArray<TWeakObjectPtr<ACastMember>> Guests;
	bool bGathered = false;
	float Exposure = 0.f;
	bool bWasInView = false;
	bool bMurmured = false;
	bool bSeen = false;
	bool bTaught = false;
	int32 MurmurLineIndex = 0;
	float StareRenewSeconds = 0.f;
};
