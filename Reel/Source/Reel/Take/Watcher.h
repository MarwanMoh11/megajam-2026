// A watcher: anything that sees with a frame (rule 6). Abstract base. Owner: (d).
// No rule logic in its own Tick (it does not tick): the director calls StepWatcher, ApplyEffect, HoldsJulian, SetTally.
// Collision: bodies, frames and tally are FTakeShapes (NoCollision), the click volume blocks only ECC_Visibility: a
// watcher never blocks a character's walk or pushes the follow camera (subclasses keep it that way).

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

	/** The use highlight (the label colour or a slight frame scale-up), this frame. Since the Nov 13 rework it is
	 *  called by ATakeDirector::StepUse: true while the follow spot's aim assist has picked this watcher. */
	virtual void SetHovered(bool bInHovered) override;	// BODY: (d)

	/** The point the follow spot's aim assist measures to (and its beam aims at): the actor location plus AimHeight
	 *  for the guard; the beam overrides it with the middle of its segment at beam height. */
	virtual FVector GetAimPoint() const;	// BODY: (d)

	// --- director API ----------------------------------------------------------------------------------------------

	/** Step 4, before the sight test. Counts the effect down (back to Watching at 0, frame red again on that frame),
	 *  moves (patrol, lure), and refreshes the frame's look (steady: nothing on screen moves with the beat). */
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

	// --- step 2: arrivals, warnings, line of sight, frame lines (BODY: W) -----------------------------------------

	/** There from the start (ArriveOn is None), or its arrival bar has come. A watcher that has not arrived is Off:
	 *  its frame, frame lines and tally are hidden, IsLive and HoldsJulian are false, the label reads "DRONE - OFF".
	 *  Its body stays where it was placed (the drone hovers at its first path point). */
	bool HasArrived() const { return ArriveOn.IsNone() || bArrived; }

	/** The director (FireTrigger, and the checkpoint restore): this watcher arrives on the bar line that starts Bar.
	 *  Logs "<LABEL> arrives on bar N". */
	void ScheduleArrival(int32 Bar);	// BODY: W

	/** 0 = not scheduled yet. */
	int32 GetArriveBar() const { return ArriveBar; }

	/** The checkpoint restore, after ScheduleArrival: a watcher whose arrival bar is already behind the restored
	 *  clock is simply there: no "ARRIVES" line, no tally blink (review, Nov 14). */
	void MarkArrivedIfDue(const UBarClockComponent* Clock);

	/** Its tally blinks (the arrival, ArrivalBlinkBars long): the HUD and RefreshFrameLook read it. */
	bool IsArrivalBlinking() const { return ArrivalBlinkSecondsLeft > 0.f; }

	/** True if this watcher turns, starts a pass, or starts or ends a speech on the bar line that STARTS Bar. The
	 *  director plays Instrument one bar before (ATakeDirector::StepWarnings). Base: false. */
	virtual bool ChangesAtBar(int32 Bar, const UBarClockComponent* Clock) const { return false; }

	/** True for a watcher whose frame exposes Julian by itself while he is off a mark (the drone's cone, rule 5).
	 *  The red tape rule is the director's (IsJulianExposed). */
	virtual bool ExposesOffMark() const { return false; }

	/** The frame's eye, world: line traces start here and the frame lines meet here. Base: actor location plus
	 *  EyeHeight. The scanner: the red eye on its lintel. The drone: its body. */
	virtual FVector GetEyePoint() const;	// BODY: W

	/** Line of sight from the eye to Julian: true if a trace to his chest (feet + 120) or to his head (feet + 170)
	 *  is clear (ATakeDirector::IsSightBlocked: walls, floors, a dropped banner). Every subclass's HoldsJulian ends
	 *  with this. Always true when bUseLineOfSight is false or there is no director. */
	bool HasLineOfSight(const AJulian* Julian) const;	// BODY: W

	/** The frame's far corners, in FramePivot's space (base: none, so no lines). The guard: the four corners of the
	 *  far rectangle of his wedge (floor and head height, at FrameRange, at +-FrameHalfAngle). */
	virtual void GetFrameFarCorners(TArray<FVector>& OutLocal) const {}

	/** BeginPlay, and again whenever a subclass's frame changes shape: (re)builds the thin frame lines of rule 6:
	 *  one from the eye to each far corner and one between neighbouring far corners (FTakeShapes::AddShapeAtRuntime
	 *  cubes on FramePivot, FrameLineThickness thick, no shadow, NoCollision). RefreshFrameLook colours and hides
	 *  them with the frame. No far corners: no lines. */
	void BuildFrameLines();	// BODY: W

	// --- step 3 (BODY: W) ------------------------------------------------------------------------------------------

	/** The alarm (ATakeDirector, when the alarm goes live, and again after a restore): the frame turns steady
	 *  emissive white (AlarmEmissive) so it still reads when the picture bleeds to colour. */
	void SetAlarmLook(bool bOn);

	bool HasAlarmLook() const { return bAlarmLook; }

	/** Its whole actor is hidden until it arrives (the guards who walk in at the alarm): SetActorHiddenInGame(true)
	 *  in BeginPlay when ArriveOn is set, shown again on the frame it arrives (StepArrival, MarkArrivedIfDue). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Arrival")
	bool bHideUntilArrived = false;

	/** Flashed (red) on the frame it arrives ("GUARD 3: \"Nobody leaves!\""). Empty: none. Not flashed by a restore. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Arrival")
	FString ArriveLine;

	/** Step 4 (§15.3): with the alarm look on, a live frame is steady white at this emissive strength (it was 4 on
	 *  the beat falling to 1.4 between beats; the steady level sits between them). */
	UPROPERTY(EditAnywhere, Category = "Take|Watcher", meta = (ClampMin = "0"))
	float AlarmEmissive = 2.5f;

	/** Step 4, pass 3 (§15.10): the body's tint (PatrolGuard, DroneWatcher). Heavies are the darkest thing on set. */
	UPROPERTY(EditAnywhere, Category = "Take|Look")
	FLinearColor BodyTint = FLinearColor(0.02f, 0.02f, 0.022f);

	/** Step 4, pass 3 (§15.8): the frame mesh's pattern (TakeStyle::Pattern*): bands outwards from the watcher; the
	 *  beam, which is a line, uses dashes. The frame lines are always dashed. */
	UPROPERTY(EditAnywhere, Category = "Take|Look")
	float FramePattern = 2.f;

	// --- tunables ---------------------------------------------------------------------------------------------------

	/** The trigger this watcher arrives on (ATakeDirector::FireTrigger). None: it is there from the take's start. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Arrival")
	FName ArriveOn;

	/** Bars after the trigger's arrival line (the next 4-bar line) that it arrives. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Arrival", meta = (ClampMin = "0"))
	int32 ArriveExtraBars = 0;

	/** Its tally blinks for this many bars when it arrives. */
	UPROPERTY(EditAnywhere, Category = "Take|Arrival", meta = (ClampMin = "0"))
	int32 ArrivalBlinkBars = 1;

	/** The click pitch that warns one bar before it changes (ChangesAtBar), and plays solo the bar before it
	 *  arrives. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Watcher")
	EWatcherInstrument Instrument = EWatcherInstrument::None;

	/** Flashed on the frame it starts holding an exposed Julian ("CHIEF: No match. Hold there, sir."). Empty: none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Watcher")
	FString SeenLine;

	/** Flashed on the frame a Blind lands on it ("CHIEF: Who turned that on?!"). Empty: none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Watcher")
	FString BlindedLine;

	/** Height of the eye above the actor, cm (GetEyePoint). */
	UPROPERTY(EditAnywhere, Category = "Take|Watcher")
	float EyeHeight = 160.f;

	/** Walls and a dropped banner block the frame (rule 6). Off: the frame sees through everything. */
	UPROPERTY(EditAnywhere, Category = "Take|Watcher")
	bool bUseLineOfSight = true;

	/** Thickness of the frame lines, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Watcher")
	float FrameLineThickness = 3.f;

	/** Tape label on set ("GUARD", "BEAM"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Watcher")
	FString Label = TEXT("WATCHER");

	/** Height of the label point above the actor, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Watcher")
	float LabelHeight = 220.f;

	/** Height of the aim point (GetAimPoint) above the actor, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Watcher")
	float AimHeight = 120.f;

	/** Frame and tally colour while it holds an exposed Julian (solid bright red). */
	UPROPERTY(EditAnywhere, Category = "Take|Watcher")
	FLinearColor HoldingColour = FLinearColor(1.f, 0.1f, 0.06f);

	/** While hovered the frame's colour moves this far toward white (0..1). The footprint never scales. */
	UPROPERTY(EditAnywhere, Category = "Take|Watcher", meta = (ClampMin = "0", ClampMax = "1"))
	float HoverBrightness = 0.3f;

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

	/** Effects stack: the dark timer (Blind / Mask) runs on its own; subclasses keep their own (the guard's lure).
	 *  While it runs the frame is dark and not live, whatever else is happening. */
	float DarkSecondsLeft = 0.f;

	/** Which dark effect is reported while DarkSecondsLeft runs: Blinded or Masked. */
	EWatcherState DarkKind = EWatcherState::Blinded;

	/** Starts or extends the dark timer: max(time left, Duration). A Mask reports MASKED; a Blind keeps an
	 *  existing MASKED. Syncs the state and the frame look this frame. */
	void AddDark(EWatcherState Kind, float Duration);

	/** Derives State and EffectSecondsLeft from the timers: not arrived -> Off first, then dark, else the subclass's
	 *  own state. */
	virtual void SyncState();

	/** StepWatcher (base) calls this first, every frame: bArrived = ArriveOn is None, or ArriveBar > 0 and the
	 *  clock's bar >= ArriveBar. On the frame it becomes true: ArrivalBlinkSecondsLeft = ArrivalBlinkBars bars, the
	 *  frame and lines show, TAKE_LOG("<LABEL> ARRIVES"). Counts the blink down. */
	void StepArrival(float DeltaSeconds, const UBarClockComponent* Clock);	// BODY: W

	/** False until the arrival bar (only meaningful when ArriveOn is set). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	bool bArrived = false;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	int32 ArriveBar = 0;

	float ArrivalBlinkSecondsLeft = 0.f;

	bool bAlarmLook = false;

	/** The frame lines BuildFrameLines made. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> FrameLines;

private:
	/** Keeps FramePivot at scale 1: the drawn frame is the tested frame. */
	void ApplyFrameScale();
};
