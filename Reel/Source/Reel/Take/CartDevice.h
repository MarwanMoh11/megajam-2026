// A champagne cart: rolls from its service door to a crash point and lures (rule 7). Owner: (u).

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

	virtual void BeginPlay() override;	// BODY: (u)

	/** Spends the use, starts the roll (first step this frame). */
	virtual bool Fire() override;	// BODY: (u)

	/** Moves the cart; lands on the frame it reaches the crash point (it tips over that frame). */
	virtual void StepDevice(float DeltaSeconds) override;	// BODY: (u)

	/** The crash point in world space (actor transform of CrashOffset). */
	virtual FVector GetEffectPoint() const override;	// BODY: (u)

	/** Scene 4 (s4-catwalk): back at its door, upright, its use restored: for a retake or a jump. */
	void ResetCart();

	/** Crash point relative to the door (the actor), cm. A widget in the editor. */
	UPROPERTY(EditAnywhere, Category = "Take|Cart", meta = (MakeEditWidget = true))
	FVector CrashOffset = FVector(400.f, 0.f, 0.f);

	/** Time from the door to the crash, s (not in the spec: tune for timing). */
	UPROPERTY(EditAnywhere, Category = "Take|Cart")
	float RollSeconds = 1.0f;

	/** A small flat white marker on the floor at the crash point: visible in the editor, hidden in game. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> CrashMarker;

	/** QA devices: how far the cart tips over at the crash, about its way, degrees (90: on its side). */
	UPROPERTY(EditAnywhere, Category = "Take|Cart")
	float TipDegrees = 90.f;

protected:
	bool bCrashed = false;

private:
	/** QA devices: lifts or drops C so its bounds' bottom sits RiseZ above where it sat at the door, this frame. */
	void RestOnFloor(USceneComponent* C, float RestBottomZ, float RiseZ);

	/** The bottoms of the box and the model at the door, world Z (BeginPlay). */
	float BodyRestBottom = 0.f;
	float ModelRestBottom = 0.f;

	/** 0 at the door .. 1 at the crash point, from RunSeconds and RollSeconds. */
	float GetRollAlpha() const;

	/** Puts the cart Alpha along its roll, facing its way, tipped on its side if bTipped, this frame. */
	void PlaceCart(float Alpha, bool bTipped);

	/** Body's relative location at the door (BeginPlay). */
	FVector BodyHome = FVector(0.f, 0.f, 45.f);
};
