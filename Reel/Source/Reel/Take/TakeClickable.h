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
 * Something the pointer can click. The actor carries a click volume (FTakeShapes::CreateClickVolume) that blocks
 * ECC_Visibility; the player controller's trace under the cursor returns that actor, and Cast<ITakeClickable>
 * tells the director what it is.
 */
class ITakeClickable
{
	GENERATED_BODY()

public:
	/** Mark, Device or Watcher: the press routing in ATakeDirector::HandleClick depends on it. */
	virtual ETakeClickKind GetClickKind() const = 0;

	/** Called by ATakePlayerController::PlayerTick when the pointer starts (true) or stops (false) being over this
	 *  actor. A highlight only (brighter colour or a slight scale-up), visible on this frame. It never fires anything. */
	virtual void SetHovered(bool bHovered) = 0;
};
