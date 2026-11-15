// A security shutter that falls when the alarm sounds (Scene 3). Step 3. Owner: W.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TakeTypes.h"
#include "Shutter.generated.h"

class UStaticMeshComponent;

/**
 * Place the actor on the floor in the middle of the opening it closes; yaw turns it (the panel's width runs along
 * the actor's Y). Panel is a cube Size.X thick, Size.Y wide and Size.Z tall. Open, it hangs with its foot
 * OpenHeight above the floor and has no collision at all. ATakeDirector calls StartDrop on every shutter whose
 * DropOn trigger fires (FireTrigger) and StepShutter every frame: the panel comes down in DropSeconds, and from
 * the frame it starts it blocks everything (a walker, a camera, a sight line: collision profile BlockAll), so
 * nobody slips under it. When it lands the director's clock thuds once (the shutter asks
 * ATakeDirector::Get(this)->Clock->PlayThud()).
 * SetDown is the restore: down at once, no sound.
 */
UCLASS()
class REEL_API AShutter : public AActor
{
	GENERATED_BODY()

public:
	AShutter();

	/** Sizes and colours the panel, open (editor and runtime). */
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

	/** Starts the fall on this frame (the first step is taken at once). Does nothing if it is falling or down. */
	void StartDrop();

	/** Down at once (a checkpoint restore after the alarm). */
	void SetDown();

	/** The fall. Called by the director every frame while Rolling. */
	void StepShutter(float DeltaSeconds);

	bool IsDown() const { return bDown; }

	/** The trigger that drops it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Shutter")
	FName DropOn = FName(TakeTriggers::Case);

	/** Down, it stops a walker. Off: it blocks sight lines and cameras only (review, Nov 14: a shutter that drops on
	 *  a crew member parked in a doorway would shove or trap them; only the aisle's shutter needs to stop anyone). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Shutter")
	bool bBlocksWalkers = true;

	/** Thickness (X), width (Y), height (Z), cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Shutter")
	FVector Size = FVector(12.f, 300.f, 300.f);

	/** How far above the floor its foot hangs while open, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Shutter")
	float OpenHeight = 320.f;

	UPROPERTY(EditAnywhere, Category = "Take|Shutter")
	float DropSeconds = 0.6f;

	/** A gate leaf, not a shutter (Nov 15: the aisle's steel shutter fell 8 m out of nothing). The actor stands at the
	 *  hinge post; the leaf (Size.X thick, Size.Y long, Size.Z tall) runs from the hinge along the actor's +Y when
	 *  shut. Open, it stands swung OpenSwingDegrees about the hinge (yaw, in the actor's frame) with no collision;
	 *  StartDrop swings it shut in DropSeconds and it blocks from that frame. OpenHeight is not used. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Shutter")
	bool bSwingGate = false;

	/** A gate: how far the leaf stands swung open, degrees of yaw about the hinge (the sign picks the side). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Shutter", meta = (EditCondition = "bSwingGate"))
	float OpenSwingDegrees = 90.f;

	/** A gate: the gap under the leaf, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Shutter", meta = (EditCondition = "bSwingGate"))
	float GateFootHeight = 10.f;

	/** A gate: the colour of the rail along the leaf's top. */
	UPROPERTY(EditAnywhere, Category = "Take|Shutter", meta = (EditCondition = "bSwingGate"))
	FLinearColor RailColour = FLinearColor(0.30f, 0.21f, 0.08f);

	UPROPERTY(EditAnywhere, Category = "Take|Shutter")
	FLinearColor Colour = FLinearColor(0.20f, 0.21f, 0.23f);

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> Panel;

	/** A gate's top rail: rides on Panel, shown only when bSwingGate. No collision. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> Rail;

protected:
	bool bDropping = false;
	bool bDown = false;
	float DropElapsed = 0.f;

	/** Puts the panel Alpha along its fall (0 open .. 1 on the floor; a gate: 0 swung open .. 1 shut), this frame. */
	void SetDrop(float Alpha);

	/** Sizes and colours the panel and the gate's rail, open, no collision (OnConstruction and BeginPlay). */
	void LayOutOpen();
};
