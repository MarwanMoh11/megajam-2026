// What survives a level reload (rule 9). Set as GameInstanceClass in DefaultEngine.ini. Owner: (e).

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "TakeTypes.h"
#include "TakeGameInstance.generated.h"

/** Where a crew character stood when a blooper fired (the reload puts them back: nobody watches the crew, so the
 *  walk the player gave them is not part of what a blooper costs). */
USTRUCT()
struct FTakeCrewPlace
{
	GENERATED_BODY()

	/** The actor location (the capsule's centre). */
	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	float Yaw = 0.f;

	UPROPERTY()
	float LookYaw = 0.f;

	UPROPERTY()
	float LookPitch = 0.f;
};

/** One blooper, for the blooper reel under the end credits (rule 11; text only until stills exist). */
USTRUCT()
struct FTakeBlooperRecord
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Take = 1;

	/** The bar it fired on. */
	UPROPERTY()
	int32 Bar = 1;

	UPROPERTY()
	int32 Scene = 1;

	UPROPERTY()
	EBlooperReason Reason = EBlooperReason::None;

	/** "THE ROPE": where the take went again from. */
	UPROPERTY()
	FString CheckpointName;

	/** Who caught him, as its tape label ("GUARD", "DRONE"); empty for the beam and the clock. */
	UPROPERTY()
	FString CaughtBy;
};

UCLASS()
class REEL_API UTakeGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UTakeGameInstance();

	/** A blooper: BlooperCount +1, LastBlooperReason = Reason, bReloadingFromBlooper = true. Logs. */
	void NoteBlooper(EBlooperReason Reason);	// BODY: (e)

	/** R: TakeNumber +1, the checkpoint back to the start, bReloadingFromBlooper = false. Logs. */
	void NoteNewTake();	// BODY: (e)

	/** "TAKE 3" on the HUD. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Take")
	int32 TakeNumber = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Take")
	int32 BlooperCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Take")
	EBlooperReason LastBlooperReason = EBlooperReason::None;

	/** Named on the slate: "BLOOPER - AGAIN FROM THE START". Step 1 has only the start. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Take")
	FString CheckpointName = TEXT("THE START");

	/** MarkName of the checkpoint mark the next blooper restarts at. Empty: where Julian was placed in the map. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Take")
	FString CheckpointMark;

	/** The bar the checkpoint restores the clock to (1 = the opening). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Take")
	int32 CheckpointBar = 1;

	// --- step 2: the rest of what a checkpoint keeps (rule 9). Written by ATakeDirector::SaveCheckpoint, read by
	// ATakeDirector::RestoreCheckpoint, cleared by NoteNewTake. BODY: S ---------------------------------------------

	/** The scene the checkpoint is in (1 The Door, 2 The Floor). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Take")
	int32 CheckpointScene = 1;

	/** Uses left on every device when the checkpoint was saved, by the device's object name (GetName()). A device
	 *  not in the map keeps its full uses. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Take")
	TMap<FString, int32> CheckpointUses;

	/** The triggers that had fired when the checkpoint was saved, each with the bar it fired on (arrival bars are
	 *  worked out from it again). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Take")
	TMap<FName, int32> CheckpointTriggers;

	/** Maya's and Vex's places at the last blooper (ATakeDirector::SaveCrewPlaces), put back on the reload's first
	 *  tick (RestoreCrewPlaces). A new take clears it: they start where the map put them. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TMap<ETakeCamera, FTakeCrewPlace> CrewPlaces;

	/** One bit per character (1 << ETakeCamera): its cover-story line has been shown this session. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Take")
	uint8 IntroShownMask = 0;

	/** Bloopers at the current checkpoint (reset when a new one is saved and by a new take): two or more and the
	 *  crew hint (rule 9). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Take")
	int32 BloopersAtCheckpoint = 0;

	// --- step 3: the ends (BODY: S). NoteNewTake clears all of it except bTitleShown ---------------------------------

	/** The title card has been shown this session (ATakeDirector shows it once, before the first take). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Take")
	bool bTitleShown = false;

	/** Cuts made this take (ATakeDirector::HandleCut, each flight started). A blooper's restore puts it back to
	 *  CheckpointCuts: the bloopers are spliced out (rule 11). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Take")
	int32 Cuts = 0;

	/** Cuts when the checkpoint was saved (SaveCheckpoint). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Take")
	int32 CheckpointCuts = 0;

	/** Every blooper of this take, oldest first (StepBlooperCheck adds one): the blooper reel. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TArray<FTakeBlooperRecord> BlooperReel;

	/** The ids of the deleted scenes restored this take (ATakeDirector::RestoreDeletedScene). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Take")
	TArray<FName> RestoredScenes;

	/** True from a blooper until the reloaded level has started (so it can say "Action!"). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Take")
	bool bReloadingFromBlooper = false;
};
