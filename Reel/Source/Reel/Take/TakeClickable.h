// The pointer's contract: marks, devices and watchers implement this. Owner: header pass (shared, complete).

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TakeTypes.h"
#include "TakeClickable.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UTakeClickable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Since the Nov 13 rework there is no pointer and nothing is clicked. Devices and watchers keep this interface for
 * the highlight (SetHovered, driven by ATakeDirector::StepUse) and their click volumes stay (they block only
 * ECC_Visibility, never Pawn or Camera). Marks no longer implement it.
 */
class ITakeClickable
{
	GENERATED_BODY()

public:
	/** Device or Watcher (Mark is no longer used). */
	virtual ETakeClickKind GetClickKind() const = 0;

	/** A highlight only (brighter colour or a slight scale-up), visible on this frame. It never fires anything. */
	virtual void SetHovered(bool bHovered) = 0;
};
