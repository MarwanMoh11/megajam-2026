// A crew device: one of three effects on every watcher in its range (rule 7). Abstract base. Owner: (u).
// It does not tick: the director calls Fire (step 2) and StepDevice (step 3), and applies the effect on landing.
// Since the Nov 13 rework it is fired by Maya or Vex standing in its use ring and pressing Use (E): see
// ATakeDirector::FindUseTarget. It is no longer clicked (ITakeClickable and ClickVolume stay, unused by the input).
// Collision: every shape here comes from FTakeShapes (NoCollision): it never blocks a character or the follow camera.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TakeClickable.h"
#include "TakeTypes.h"
#include "Device.generated.h"

class UBoxComponent;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class AWatcher;

UCLASS(Abstract)
class REEL_API ADevice : public AActor, public ITakeClickable
{
	GENERATED_BODY()

public:
	ADevice();

	/** UsesLeft = Uses; colours the body; LayOutUseRing; ring dim (not in reach). */
	virtual void BeginPlay() override;	// BODY: (u)

	/** LayOutUseRing in the editor, so the map shows where to stand. Subclasses that override call Super. */
	virtual void OnConstruction(const FTransform& Transform) override;	// BODY: (u)

	// ITakeClickable
	virtual ETakeClickKind GetClickKind() const override { return ETakeClickKind::Device; }

	/** Body highlight (body brightens), this frame. No longer driven by a pointer; SetInReach is the use highlight. */
	virtual void SetHovered(bool bInHovered) override;	// BODY: (u)

	// --- use (the Nov 13 rework) ---------------------------------------------------------------------------------

	/** The floor point Maya or Vex stands near to use it: the actor transform applied to UseOffset. */
	FVector GetUsePoint() const;	// BODY: (u)

	/** Called by ATakeDirector::StepUse when this becomes (true) or stops being (false) the current character's use
	 *  target: the use ring goes GoldBright (dim Grey when false), this frame. */
	void SetInReach(bool bInReach);	// BODY: (u)

	bool IsInReach() const { return bIsInReach; }

	// --- director API ----------------------------------------------------------------------------------------------

	/** Can it fire now? False with a short reason for the flash ("CART - used", "BANNER - still down"). */
	virtual bool CanFire(FString& OutWhyNot) const;	// BODY: (u)

	/** Step 2 (ATakeDirector::FireDevice), only after CanFire: spends a use (unless free), starts its timeline, and
	 *  changes something visible this frame (a pip goes out; the cart moves its first step; the banner starts down).
	 *  Logs "FIRE <name>". Returns false if it could not fire. */
	virtual bool Fire();	// BODY: (u)

	/** Step 3: advances the timeline by DeltaSeconds and sets LandedThisFrame on the frame the effect lands. */
	virtual void StepDevice(float DeltaSeconds);	// BODY: (u)

	/** True only on the frame the effect lands (the spot: the frame it is fired; the cart: the crash; the banner:
	 *  the matte reaching the bottom). */
	virtual bool LandedThisFrame() const;	// BODY: (u)

	/** The watchers this landing affects: by default every watcher with DistanceToPoint(GetEffectPoint()) <= Range.
	 *  The follow spot: only its target. */
	virtual void GatherTargets(const TArray<TObjectPtr<AWatcher>>& All, TArray<AWatcher*>& Out) const;	// BODY: (u)

	/** The point ranges are measured from and lures walk to (default: the actor location). */
	virtual FVector GetEffectPoint() const;	// BODY: (u)

	/** "CART  o" for one use left, "CART  -" when spent; the spot: "SPOT  READY" / "SPOT  BUSY 2.1". */
	virtual FString GetPipsText() const;	// BODY: (u)

	/** Its timeline is running (fired and not finished). */
	bool IsRunning() const { return bRunning; }

	/** World point over the device where the HUD draws its pips. */
	virtual FVector GetLabelPoint() const;	// BODY: (u)

	// --- step 2 (BODY: D) --------------------------------------------------------------------------------------------

	/** Does this device's matte block a watcher's line of sight from From to To right now (rule 7, Mask: "solid
	 *  geometry that blocks frames")? Base: false. The banner: true while its matte is down (or dropping past the
	 *  line) and the segment crosses the matte's box. ATakeDirector::IsSightBlocked asks every device. */
	virtual bool BlocksSight(const FVector& From, const FVector& To) const { return false; }

	/** The checkpoint restore (ATakeDirector::RestoreCheckpoint, after every BeginPlay): UsesLeft = InUsesLeft
	 *  (clamped to 0..Uses), and the device looks spent if it is (pips, body). Free devices (Uses 0) ignore it. */
	virtual void RestoreUses(int32 InUsesLeft);	// BODY: D

	// --- step 3 (BODY: W) ------------------------------------------------------------------------------------------

	/** Set: CanFire is false until the director's HasFired(LockedUntil), with the reason DeviceName + " - " +
	 *  LockedWhy ("PYRO - locked until the alarm"). The pyro and the winch are locked until the Case trigger. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Device")
	FName LockedUntil;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Device")
	FString LockedWhy = TEXT("locked until the alarm");

	/** True while LockedUntil is set and has not fired (no director: not locked). CanFire asks it first. */
	bool IsLocked() const;

	/** Set: ATakeDirector::StepDevices fires this trigger on the frame the device lands (the winch: "Winch", which
	 *  makes the trap's mark appear). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Device")
	FName LandTrigger;

	/** Flashed (white) by ATakeDirector::FireDevice on the frame it fires ("VEX: \"Curtain coming down!\""). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Device")
	FString FireLine;

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

	/** Who uses it: B Maya, C Vex (the test box has no Vex, so its cart is Maya's). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Device")
	ETakeCamera FiredFrom = ETakeCamera::B;

	/** Feet within this 2D distance of the use point can use it, cm. Also the use ring's radius. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Device")
	float UseRadius = 220.f;

	/** ...and within this height of it, cm: the ring can't be worked from the floor below or above. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Device")
	float UseHeight = 150.f;

	/** The use point, local to the actor (Z on the floor). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Device")
	FVector UseOffset = FVector::ZeroVector;

	/** The prompt's verb: "[E] " + UseVerb ("Release the cart"; the spot adds the watcher: "[E] Spot GUARD"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Device")
	FString UseVerb = TEXT("Use");

	/** Dashes in the use ring. */
	UPROPERTY(EditAnywhere, Category = "Take|Device")
	int32 UseRingDots = 32;

	/** Width of a use-ring dash across the circle, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Device")
	float UseRingDashWidth = 10.f;

	/** Height of a use-ring dash above the floor, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Device")
	float UseRingDashHeight = 2.f;

	/** Share of each dash's slot along the circle that is dash (the rest is gap), 0-1. */
	UPROPERTY(EditAnywhere, Category = "Take|Device")
	float UseRingDashFill = 0.55f;

	/** Height of the pips label above the actor, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Device")
	float LabelHeight = 200.f;

	/** The body's colour (hover brightens it). */
	UPROPERTY(EditAnywhere, Category = "Take|Device")
	FLinearColor BodyColour = TakeColors::WorkBlue;

	/** Step 4, pass 3 (§15.9 MI_Crew_Ring): the use ring in reach, mo.high cyan-white: gold is Julian's. Lit
	 *  BasicShapeMaterial: the ring is an instanced mesh, and the unlit signal material has no instanced-mesh usage
	 *  (setting it at runtime crashed the GPU on the editor's first frame). */
	UPROPERTY(EditAnywhere, Category = "Take|Look")
	FLinearColor RingReachColour = FLinearColor(0.738f, 0.905f, 0.913f);

	/** ...and out of reach (silver x 0.5). */
	UPROPERTY(EditAnywhere, Category = "Take|Look")
	FLinearColor RingIdleColour = FLinearColor(0.097f, 0.089f, 0.076f);

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

	/** The use ring: flat dashes (a few cm tall) on the floor in a circle of UseRadius around the use point, so the
	 *  player sees where to stand. Absolute rotation (stays flat whatever the device's rotation). NoCollision. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UInstancedStaticMeshComponent> UseRing;

protected:
	/** Places UseRing at UseOffset and lays its UseRingDots dashes out at UseRadius; colours it for bIsInReach. */
	void LayOutUseRing();	// BODY: (u)

	bool bRunning = false;
	bool bLanded = false;
	float RunSeconds = 0.f;
	bool bHovered = false;
	bool bIsInReach = false;

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
