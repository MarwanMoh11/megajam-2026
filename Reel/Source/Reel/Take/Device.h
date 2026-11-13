// A crew device: one of three effects on every watcher in its range (rule 7). Abstract base. Owner: (e).
// It does not tick: the director calls Fire (step 2) and StepDevice (step 3), and applies the effect on landing.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TakeClickable.h"
#include "TakeTypes.h"
#include "Device.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class AWatcher;

UCLASS(Abstract)
class REEL_API ADevice : public AActor, public ITakeClickable
{
	GENERATED_BODY()

public:
	ADevice();

	/** UsesLeft = Uses; colours the body. */
	virtual void BeginPlay() override;	// BODY: (e)

	// ITakeClickable
	virtual ETakeClickKind GetClickKind() const override { return ETakeClickKind::Device; }

	/** Hover highlight (body brightens), this frame. Range rings are out of step 1. */
	virtual void SetHovered(bool bInHovered) override;	// BODY: (e)

	// --- director API ----------------------------------------------------------------------------------------------

	/** Can it fire now? False with a short reason for the flash ("CART - used", "BANNER - still down"). */
	virtual bool CanFire(FString& OutWhyNot) const;	// BODY: (e)

	/** Step 2 (ATakeDirector::FireDevice), only after CanFire: spends a use (unless free), starts its timeline, and
	 *  changes something visible this frame (a pip goes out; the cart moves its first step; the banner starts down).
	 *  Logs "FIRE <name>". Returns false if it could not fire. */
	virtual bool Fire();	// BODY: (e)

	/** Step 3: advances the timeline by DeltaSeconds and sets LandedThisFrame on the frame the effect lands. */
	virtual void StepDevice(float DeltaSeconds);	// BODY: (e)

	/** True only on the frame the effect lands (the spot: the frame it is fired; the cart: the crash; the banner:
	 *  the matte reaching the bottom). */
	virtual bool LandedThisFrame() const;	// BODY: (e)

	/** The watchers this landing affects: by default every watcher with DistanceToPoint(GetEffectPoint()) <= Range.
	 *  The follow spot: only its target. */
	virtual void GatherTargets(const TArray<TObjectPtr<AWatcher>>& All, TArray<AWatcher*>& Out) const;	// BODY: (e)

	/** The point ranges are measured from and lures walk to (default: the actor location). */
	virtual FVector GetEffectPoint() const;	// BODY: (e)

	/** "CART  o" for one use left, "CART  -" when spent; the spot: "SPOT  READY" / "SPOT  BUSY 2.1". */
	virtual FString GetPipsText() const;	// BODY: (e)

	/** Its timeline is running (fired and not finished). */
	bool IsRunning() const { return bRunning; }

	/** World point over the device where the HUD draws its pips. */
	virtual FVector GetLabelPoint() const;	// BODY: (e)

	// --- tunables ---------------------------------------------------------------------------------------------------

	/** HUD and log name ("SPOT", "CART", "BANNER"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Device")
	FString DeviceName = TEXT("DEVICE");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Device")
	EDeviceEffect Effect = EDeviceEffect::Blind;

	/** Range around the effect point, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Device")
	float Range = 1000.f;

	/** How long the effect lasts on each watcher it catches, s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Device")
	float Duration = 4.f;

	/** Uses per take; 0 = free. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Device")
	int32 Uses = 1;

	/** The camera it is fired from (the test box has no camera C, so its cart is fired from B). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Device")
	ETakeCamera FiredFrom = ETakeCamera::B;

	/** Height of the pips label above the actor, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Device")
	float LabelHeight = 200.f;

	/** The body's colour (hover brightens it). */
	UPROPERTY(EditAnywhere, Category = "Take|Device")
	FLinearColor BodyColour = TakeColors::WorkBlue;

	UPROPERTY(VisibleInstanceOnly, Transient, BlueprintReadOnly, Category = "Take|State")
	int32 UsesLeft = 1;

	// --- components -------------------------------------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<USceneComponent> Root;

	/** The device's prop (subclasses size and colour it). */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UBoxComponent> ClickVolume;

protected:
	bool bRunning = false;
	bool bLanded = false;
	float RunSeconds = 0.f;
	bool bHovered = false;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BodyMID;

	/** Colours the body for its state (BodyColour, brighter while hovered), this frame. */
	void RefreshBodyLook();

	/** The effect lands on this frame: LandedThisFrame() is true until the frame ends. */
	void MarkLanded();

	/** Fire() ran on this frame (so StepDevice does not take the fire frame's step twice). */
	bool FiredThisFrame() const;

private:
	uint64 LandedFrame = 0;
	uint64 FiredFrame = 0;
};
