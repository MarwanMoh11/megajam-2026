// What survives a level reload (rule 9). Set as GameInstanceClass in DefaultEngine.ini. Owner: (e).

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "TakeTypes.h"
#include "TakeGameInstance.generated.h"

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

	/** The bar the checkpoint restores the clock to (1 = the opening). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Take")
	int32 CheckpointBar = 1;

	/** True from a blooper until the reloaded level has started (so it can say "Action!"). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Take")
	bool bReloadingFromBlooper = false;
};
