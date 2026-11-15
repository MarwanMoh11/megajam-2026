// A crew device: one of three effects on every watcher in its range (rule 7). Abstract base. Owner: (u).
// It does not tick: the director calls Fire (step 2) and StepDevice (step 3), and applies the effect on landing.
// Since the Nov 13 rework it is fired by Maya or Vex standing in its use ring and pressing Use (E): see
// ATakeDirector::FindUseTarget. It is no longer clicked (ITakeClickable and ClickVolume stay, unused by the input).
// Collision: every shape here comes from FTakeShapes (NoCollision): it never blocks a character or the follow camera.
// QA Nov 14 (notes 10, 17, 18): a period model stands in for the box (ModelMesh), a pilot lamp and a warm pool say
// "in reach" instead of the floor ring (bShowUseRing, off), and a part moves on the press (the handle, or the model).

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
class UPointLightComponent;
class UStaticMesh;
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
	 *  target: the pilot lamp and its pool go bright (dim when false), this frame (and the ring, if shown). */
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

	/** Step 6: the sound it makes on the frame it fires, a sound in /Game/Take/Audio (None: the device makes its own,
	 *  as the pyro board's crackle and the sound desk's feedback do through the bar clock). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Device")
	FName FireSound;

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

	// --- QA devices (Nov 14): the period model ----------------------------------------------------------------------

	/** The period model shown in place of the box body (a generated mesh in /Game/Take/Props/Devices). Set, with
	 *  bUseModel on: Model shows it with the mesh's own material (never recoloured), and the box (Body) is hidden
	 *  (bModelHidesBody) but keeps its job. Unset: the box, as before. */
	UPROPERTY(EditAnywhere, Category = "Take|Model")
	TSoftObjectPtr<UStaticMesh> ModelMesh;

	/** Switch: off shows the box again, whatever ModelMesh says. */
	UPROPERTY(EditAnywhere, Category = "Take|Model")
	bool bUseModel = true;

	/** Where the model stands, local to the actor: the floor point under the middle of the mesh's footprint (the
	 *  mesh's own pivot does not matter). The banner's is its winch on Maya's platform. */
	UPROPERTY(EditAnywhere, Category = "Take|Model", meta = (MakeEditWidget = true))
	FVector ModelOffset = FVector::ZeroVector;

	/** The model's turn, local to the actor (the follow spot and the searchlight swing from here). */
	UPROPERTY(EditAnywhere, Category = "Take|Model")
	FRotator ModelRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, Category = "Take|Model")
	FVector ModelScale = FVector::OneVector;

	/** The box hides while the model shows. Off for the banner: its box is the cloth that drops. */
	UPROPERTY(EditAnywhere, Category = "Take|Model")
	bool bModelHidesBody = true;

	/** Switch: the old dashed ring on the floor around the use point (note 18: off, the pilot lamp says it now). */
	UPROPERTY(EditAnywhere, Category = "Take|Model")
	bool bShowUseRing = false;

	/** The gesture the user plays on the press (ATakeCharacter::PlayGesture). None: no gesture. */
	UPROPERTY(EditAnywhere, Category = "Take|Model")
	FName UseGesture = FName(TEXT("Use"));

	// --- QA devices: the pilot lamp and its pool ---------------------------------------------------------------------

	/** Switch: a small lamp on the device and a soft warm pool around it. Dim while it can be used, bright while the
	 *  active crew member is in reach (and while it runs), dark when spent or locked (it lights when it unlocks). */
	UPROPERTY(EditAnywhere, Category = "Take|Pilot")
	bool bShowPilot = true;

	/** The lamp, local to the actor. */
	UPROPERTY(EditAnywhere, Category = "Take|Pilot", meta = (MakeEditWidget = true))
	FVector PilotOffset = FVector(0.f, 0.f, 110.f);

	/** The lamp's diameter, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Pilot")
	float PilotSize = 5.f;

	UPROPERTY(EditAnywhere, Category = "Take|Pilot")
	FLinearColor PilotColour = FLinearColor(1.f, 0.52f, 0.16f);

	/** The lamp's glow (emissive multiplier) while it can be used and nobody is in reach. */
	UPROPERTY(EditAnywhere, Category = "Take|Pilot")
	float PilotIdleGlow = 0.8f;

	/** ...while the active crew member is in reach, or it runs. */
	UPROPERTY(EditAnywhere, Category = "Take|Pilot")
	float PilotReachGlow = 8.f;

	/** While it runs the lamp flickers at this rate, Hz (0: steady; the sound desk's meters jump). */
	UPROPERTY(EditAnywhere, Category = "Take|Pilot")
	float PilotRunFlickerHz = 0.f;

	/** The pool's light at the lamp, candelas, out of reach and in reach. No shadows. */
	UPROPERTY(EditAnywhere, Category = "Take|Pilot")
	float PoolIdleCandelas = 3.f;

	UPROPERTY(EditAnywhere, Category = "Take|Pilot")
	float PoolReachCandelas = 30.f;

	/** The pool's reach, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Pilot")
	float PoolRadius = 260.f;

	// --- QA devices: the moving part (a lever that throws, or a crank that turns) -----------------------------------

	/** Switch: a brass lever with a black knob, pivoting at HandleOffset. It runs along the pivot's +Z. */
	UPROPERTY(EditAnywhere, Category = "Take|Handle")
	bool bShowHandle = false;

	/** The pivot, local to the actor. */
	UPROPERTY(EditAnywhere, Category = "Take|Handle", meta = (MakeEditWidget = true))
	FVector HandleOffset = FVector(0.f, 0.f, 90.f);

	/** The pivot's turn at rest, local to the actor. */
	UPROPERTY(EditAnywhere, Category = "Take|Handle")
	FRotator HandleRest = FRotator::ZeroRotator;

	/** Added to the rest turn when thrown (on the press frame it is already partly thrown). */
	UPROPERTY(EditAnywhere, Category = "Take|Handle")
	FRotator HandleThrow = FRotator(-70.f, 0.f, 0.f);

	/** Seconds to throw fully (it comes back at the same rate when the device is ready again). */
	UPROPERTY(EditAnywhere, Category = "Take|Handle")
	float HandleThrowSeconds = 0.2f;

	/** While its part moves (the banner dropping or raising, the curtain falling) the lever also turns about the
	 *  pivot's X at this rate, degrees a second: a crank on a winch wheel. 0: it only throws. */
	UPROPERTY(EditAnywhere, Category = "Take|Handle")
	float HandleSpinDegrees = 0.f;

	/** Interactable props (Nov 15): the throw eases out and runs this far past its end before it settles (a share of
	 *  the throw: 0 a plain ease-out, 0.15 a slam that bounces back a little). */
	UPROPERTY(EditAnywhere, Category = "Take|Handle")
	float HandleOvershoot = 0.12f;

	/** ...and on the press frame it is already this far through its throw (0-1). */
	UPROPERTY(EditAnywhere, Category = "Take|Handle")
	float HandleFirstStep = 0.35f;

	/** Interactable props: the press shakes the model, a damped rock about its foot, degrees (0: none; the follow
	 *  spot and the carts move their own models). */
	UPROPERTY(EditAnywhere, Category = "Take|Model")
	float PressJoltDegrees = 0.f;

	UPROPERTY(EditAnywhere, Category = "Take|Model")
	float PressJoltSeconds = 0.45f;

	/** A spent one-use device keeps its lever thrown (and its lamp dark). */
	UPROPERTY(EditAnywhere, Category = "Take|Handle")
	bool bHandleStaysThrownWhenSpent = true;

	UPROPERTY(EditAnywhere, Category = "Take|Handle")
	float HandleLength = 40.f;

	UPROPERTY(EditAnywhere, Category = "Take|Handle")
	float HandleThickness = 3.f;

	UPROPERTY(EditAnywhere, Category = "Take|Handle")
	float HandleKnobSize = 7.f;

	UPROPERTY(EditAnywhere, Category = "Take|Handle")
	FLinearColor HandleColour = FLinearColor(0.42f, 0.27f, 0.08f);

	UPROPERTY(EditAnywhere, Category = "Take|Handle")
	FLinearColor HandleKnobColour = FLinearColor(0.02f, 0.018f, 0.016f);

	// --- QA devices: a rope or cable (the banner's to the batten, the curtain's up to the grid) ----------------------

	UPROPERTY(EditAnywhere, Category = "Take|Rope")
	bool bShowRope = false;

	/** The rope's ends, local to the actor. */
	UPROPERTY(EditAnywhere, Category = "Take|Rope", meta = (MakeEditWidget = true))
	FVector RopeFrom = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Take|Rope", meta = (MakeEditWidget = true))
	FVector RopeTo = FVector(0.f, 0.f, 300.f);

	UPROPERTY(EditAnywhere, Category = "Take|Rope")
	float RopeThickness = 2.5f;

	UPROPERTY(EditAnywhere, Category = "Take|Rope")
	FLinearColor RopeColour = FLinearColor(0.16f, 0.12f, 0.07f);

	/** Is a model showing (ModelMesh set and loaded, bUseModel on)? */
	bool HasModel() const { return bHasModel; }

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

	/** QA devices: the period model (ModelMesh). NoCollision, no navigation: never blocks a sight trace or the camera. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> Model;

	/** QA devices: the pilot lamp (unlit, glows) and its warm pool (a small point light, no shadows). */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> PilotLamp;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UPointLightComponent> PilotPool;

	/** QA devices: the lever's pivot, the lever and its knob. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<USceneComponent> HandlePivot;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> HandleBar;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> HandleKnob;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> Rope;

protected:
	/** Places UseRing at UseOffset and lays its UseRingDots dashes out at UseRadius; colours it for bIsInReach.
	 *  bShowUseRing off: the ring is emptied and hidden. */
	void LayOutUseRing();	// BODY: (u)

	// --- QA devices -------------------------------------------------------------------------------------------------

	/** Loads ModelMesh onto Model (or clears it), hides or shows the box, lays out the pilot, the handle and the
	 *  rope. From OnConstruction and BeginPlay. */
	void ApplyLook();

	/** Puts the model where it stands, turned by ExtraTurn (local to the actor, about the model's anchor) and moved by
	 *  ExtraMove. The anchor (ModelOffset) is the floor point under the middle of the mesh's footprint. */
	void PlaceModel(const FQuat& ExtraTurn = FQuat::Identity, const FVector& ExtraMove = FVector::ZeroVector);

	/** Every frame from StepDevice (every subclass calls it first): the pilot's glow, the lever's throw and spin. */
	void StepLook(float DeltaSeconds);

	/** On the press frame (Fire, the follow spot's FireAt): the lever jumps part of its throw, the lamp goes bright,
	 *  and the user reaches for it (UseGesture). */
	void OnFiredLook();

	/** Does the crank turn now? Default: while running. The banner: dropping or raising; the curtain: falling. */
	virtual bool IsHandleTurning() const { return bRunning; }

	/** Which way the crank turns now: 1 or -1 (the banner winds back the other way as it raises). */
	virtual float HandleSpinSign() const { return 1.f; }

	/** Should the lever be thrown now? Default: while running, or spent (bHandleStaysThrownWhenSpent). */
	virtual bool IsHandleThrown() const;

	/** Is the box body showing a basic shape that may be recoloured (no model over it, no generated mesh on it)? */
	bool BodyIsBox() const;

	/** The pilot's glow for the state now: 0 spent (and not running) or locked; PilotReachGlow running or in reach
	 *  (flickering at PilotRunFlickerHz while running); PilotIdleGlow otherwise. */
	float PilotGlowNow() const;

	/** Sets the lamp's emissive colour and the pool's light for Glow, this frame (a no-op when unchanged). */
	void SetPilotGlow(float Glow);

	/** Turns the lever's pivot for HandleAlpha and HandleSpin, this frame. */
	void PoseHandle();

	/** The press jolt's rock for its clock now, about the model's own side axis (identity when done). */
	FQuat GetJoltTurn() const;

	bool bHasModel = false;
	float JoltClock = -1.f;
	float HandleAlpha = 0.f;
	float HandleSpin = 0.f;
	float PilotGlowShown = -1.f;
	float PilotClock = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PilotMID;

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
