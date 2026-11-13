// A watcher: anything that sees with a frame (rule 6). Abstract base. Owner: (d).
// No rule logic in its own Tick (it does not tick): the director calls StepWatcher, ApplyEffect, HoldsJulian, SetTally.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TakeClickable.h"
#include "TakeTypes.h"
#include "Watcher.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UBarClockComponent;
class AJulian;

/** Place on the floor: the actor location is the floor point under it. */
UCLASS(Abstract)
class REEL_API AWatcher : public AActor, public ITakeClickable
{
	GENERATED_BODY()

public:
	AWatcher();

	/** Creates the frame's dynamic material (red), the tally (dark), remembers its home pose. */
	virtual void BeginPlay() override;	// BODY: (d)

	// ITakeClickable
	virtual ETakeClickKind GetClickKind() const override { return ETakeClickKind::Watcher; }

	/** Hover highlight (the label colour or a slight frame scale-up), this frame. */
	virtual void SetHovered(bool bInHovered) override;	// BODY: (d)

	// --- director API ----------------------------------------------------------------------------------------------

	/** Step 4, before the sight test. Counts the effect down (back to Watching at 0, frame red again on that frame),
	 *  moves (patrol, lure), and pulses the frame on the beat (FramePivot scale 1 + PulseScale * Clock->GetPulse()). */
	virtual void StepWatcher(float DeltaSeconds, UBarClockComponent* Clock);	// BODY: (d)

	/** The geometric sight test (step 4, StepSight): does the frame hold Julian right now? Walls ignored in step 1.
	 *  False while not live (blinded, masked, dropped, off). */
	virtual bool HoldsJulian(const AJulian* Julian) const;	// BODY: (d)

	/** Step 3 (StepDevices), on the frame a device lands. Blind: State Blinded for Duration, frame dark this frame.
	 *  Lure / Mask: as the subclass defines. Returns true if this watcher was caught (its state changed). Logs. */
	virtual bool ApplyEffect(EDeviceEffect Effect, float Duration, const FVector& Point);	// BODY: (d)

	/** Called by StepSight every frame. Holding an exposed Julian: frame solid bright red + tally light on, this
	 *  frame; otherwise the normal frame and the tally off. */
	virtual void SetTally(bool bHolding);	// BODY: (d)

	/** The beam: touching it is an instant blooper. The guard: catching an exposed Julian is SEEN. */
	virtual bool IsInstantBlooper() const { return false; }

	/** 2D distance from Point to this watcher (cm), for device ranges. The beam measures to its segment. */
	virtual float DistanceToPoint(const FVector& Point) const;	// BODY: (d)

	/** Frame active right now. */
	virtual bool IsLive() const;	// BODY: (d)

	/** "WATCHING", "LURED 4.2"...: the state part of the label. */
	virtual FString GetStateText() const;	// BODY: (d)

	EWatcherState GetState() const { return State; }
	float GetEffectSecondsLeft() const { return EffectSecondsLeft; }
	bool HasTally() const { return bTally; }
	const FString& GetLabel() const { return Label; }

	/** World point over the watcher where the HUD draws its label. */
	virtual FVector GetLabelPoint() const;	// BODY: (d)

	// --- tunables ---------------------------------------------------------------------------------------------------

	/** Tape label on set ("GUARD", "BEAM"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Watcher")
	FString Label = TEXT("WATCHER");

	/** The frame grows by this fraction on each beat. */
	UPROPERTY(EditAnywhere, Category = "Take|Watcher")
	float PulseScale = 0.06f;

	/** Height of the label point above the actor, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Watcher")
	float LabelHeight = 220.f;

	// --- components -------------------------------------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<USceneComponent> Root;

	/** The frame hangs off this; StepWatcher scales it for the beat pulse. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<USceneComponent> FramePivot;

	/** The frame: red. Subclasses size and place it (guard: a flattened cone on the floor; beam: a thin line). */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> FrameMesh;

	/** The red tally light (a small sphere), lit while it holds an exposed Julian. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> Tally;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UBoxComponent> ClickVolume;

protected:
	/** Frame colour for the current state (red live, RedDark blinded/masked, hidden when dropped/off is the
	 *  subclass's call), applied this frame. */
	virtual void RefreshFrameLook();	// BODY: (d)

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	EWatcherState State = EWatcherState::Watching;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	float EffectSecondsLeft = 0.f;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	bool bTally = false;

	bool bHovered = false;

	/** The lure's point (where the device landed). */
	FVector LurePoint = FVector::ZeroVector;

	/** Where it stood and faced at BeginPlay. */
	FVector HomeLocation = FVector::ZeroVector;
	float HomeYaw = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FrameMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> TallyMID;
};
