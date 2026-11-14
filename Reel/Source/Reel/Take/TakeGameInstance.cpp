// Owner: (e). Constructor by the header pass; every "// BODY: (e)" is a stub to fill.

#include "TakeGameInstance.h"

UTakeGameInstance::UTakeGameInstance()
{
}

void UTakeGameInstance::NoteBlooper(EBlooperReason Reason)
{
	++BlooperCount;
	LastBlooperReason = Reason;
	bReloadingFromBlooper = true;
	++BloopersAtCheckpoint;
	TAKE_LOG(TEXT("GI BLOOPER %d: %s, take %d, again from %s"), BlooperCount, TakeText::BlooperReasonName(Reason), TakeNumber, *CheckpointName);
}

void UTakeGameInstance::NoteNewTake()
{
	++TakeNumber;
	CheckpointName = TEXT("THE STREET");
	CheckpointMark.Reset();
	CheckpointBar = 1;
	CheckpointScene = 1;
	CheckpointUses.Reset();
	CheckpointTriggers.Reset();
	CrewPlaces.Reset();
	BloopersAtCheckpoint = 0;
	bReloadingFromBlooper = false;
	// Step 3: the ends start over (the title card stays shown: once a session).
	Cuts = 0;
	CheckpointCuts = 0;
	BlooperReel.Reset();
	RestoredScenes.Reset();
	TAKE_LOG(TEXT("GI NEW TAKE %d (bloopers so far %d)"), TakeNumber, BlooperCount);
}
