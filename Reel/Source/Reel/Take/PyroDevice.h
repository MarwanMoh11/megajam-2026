// Maya's pyro: sparks and smoke that blind everything at the stage end (rule 7). Step 3. Owner: W.

#pragma once

#include "CoreMinimal.h"
#include "Device.h"
#include "PyroDevice.generated.h"

class UPointLightComponent;

/**
 * Place the actor at the firing board on Maya's lighting platform; EffectOffset (local) is the point on the stage
 * the charges go off at, and Range is measured from there (the map sets it so the dais is covered and the side
 * exit is not). One use, locked until the alarm (ADevice::LockedUntil = "Case").
 *
 * Fired: it lands on the same frame (like the sound desk): Blind for Duration (6 s) on every watcher within Range
 * of the effect point. From that frame and for Duration: a gold flare stands at each of FlareOffsets (cones or
 * spheres made at BeginPlay with FTakeShapes::AddShapeAtRuntime, hidden until fired, flickering in size on their own
 * sine, never on the beat), FlareLight burns gold at the effect point, and the board's body goes white. The clock
 * crackles (UBarClockComponent::PlayCrackle). When Duration ends the flares and the light go out for good.
 */
UCLASS()
class REEL_API APyroDevice : public ADevice
{
	GENERATED_BODY()

public:
	APyroDevice();

	/** Super, then makes the flares (hidden) and puts FlareLight at the effect point, off. */
	virtual void BeginPlay() override;

	/** Spends the use, lands this frame (MarkLanded), runs for Duration, shows the flares and the light. */
	virtual bool Fire() override;

	/** Counts the run down, flickers the flares; at the end hides them and the light, body back to BodyColour. */
	virtual void StepDevice(float DeltaSeconds) override;

	/** The actor transform applied to EffectOffset. */
	virtual FVector GetEffectPoint() const override;

	/** Every watcher within Range of the effect point, and (bBlindsLockedDrone) a drone that is locked on Julian
	 *  while he is within Range of the effect point, wherever the drone itself is: its lens is pointed straight at
	 *  the charges. Without this a pyro fired while the drone was still flying in missed it, and the drone then
	 *  arrived unblinded (the whole-take run, Nov 14). */
	virtual void GatherTargets(const TArray<TObjectPtr<AWatcher>>& All, TArray<AWatcher*>& Out) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Pyro")
	bool bBlindsLockedDrone = true;

	/** The point on the stage the charges go off at, local to the actor. A widget in the editor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Pyro", meta = (MakeEditWidget = true))
	FVector EffectOffset = FVector(1000.f, 0.f, -400.f);

	/** Where each flare stands, local to the actor (the map lays them along the front of the dais). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Pyro")
	TArray<FVector> FlareOffsets;

	/** A flare's size at rest: across, and tall, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Pyro")
	FVector2D FlareSize = FVector2D(60.f, 260.f);

	UPROPERTY(EditAnywhere, Category = "Take|Pyro")
	FLinearColor FlareColour = FLinearColor(1.f, 0.78f, 0.25f);

	/** FlareLight's intensity while it burns, candelas. */
	UPROPERTY(EditAnywhere, Category = "Take|Pyro")
	float FlareCandelas = 9000.f;

	/** Gold light at the effect point, visible only while the pyro burns. Movable, no shadows. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UPointLightComponent> FlareLight;

protected:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Flares;
};
