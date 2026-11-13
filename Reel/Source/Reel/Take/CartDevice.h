// A champagne cart: rolls from its service door to a crash point and lures (rule 7). Owner: (e).

#pragma once

#include "CoreMinimal.h"
#include "Device.h"
#include "CartDevice.generated.h"

/**
 * Place it at its door. Fired: the cart (Body) rolls along a straight timeline from the door to CrashOffset in
 * RollSeconds, moving on the frame it is fired; on arrival it crashes (tips over) and lures every watcher within
 * Range (12 m) of the crash point for Duration (6 s). One use. No physics.
 */
UCLASS()
class REEL_API ACartDevice : public ADevice
{
	GENERATED_BODY()

public:
	ACartDevice();

	virtual void BeginPlay() override;	// BODY: (e)

	/** Spends the use, starts the roll (first step this frame). */
	virtual bool Fire() override;	// BODY: (e)

	/** Moves the cart; lands on the frame it reaches the crash point (it tips over that frame). */
	virtual void StepDevice(float DeltaSeconds) override;	// BODY: (e)

	/** The crash point in world space (actor transform of CrashOffset). */
	virtual FVector GetEffectPoint() const override;	// BODY: (e)

	/** Crash point relative to the door (the actor), cm. A widget in the editor. */
	UPROPERTY(EditAnywhere, Category = "Take|Cart", meta = (MakeEditWidget = true))
	FVector CrashOffset = FVector(400.f, 0.f, 0.f);

	/** Time from the door to the crash, s (not in the spec: tune for timing). */
	UPROPERTY(EditAnywhere, Category = "Take|Cart")
	float RollSeconds = 1.0f;

	/** A small flat white marker on the floor at the crash point: visible in the editor, hidden in game. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> CrashMarker;

protected:
	bool bCrashed = false;
};
