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
	TAKE_LOG(TEXT("GI BLOOPER %d: %s, take %d, again from %s"), BlooperCount, TakeText::BlooperReasonName(Reason), TakeNumber, *CheckpointName);
}

void UTakeGameInstance::NoteNewTake()
{
	++TakeNumber;
	CheckpointName = TEXT("THE START");
	CheckpointMark.Reset();
	CheckpointBar = 1;
	bReloadingFromBlooper = false;
	TAKE_LOG(TEXT("GI NEW TAKE %d (bloopers so far %d)"), TakeNumber, BlooperCount);
}
