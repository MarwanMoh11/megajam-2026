// A banner drop: a matte that masks (rule 7). Owner: (u).

#pragma once

#include "CoreMinimal.h"
#include "Device.h"
#include "BannerDevice.generated.h"

/** Where the banner is on its timeline. */
enum class EBannerPhase : uint8
{
	Up,
	Dropping,
	Down,
	Raising,
	Spent
};

/**
 * Place it at the batten (top). Fired: the matte (Body, a flat box) drops DropDistance in DropSeconds, starting on
 * the frame it is fired; when it is down it lands: Mask for Duration (10 s) on every watcher within Range of the
 * effect point (in the test box: the beam); then it winches back up in RaiseSeconds. One use.
 */
UCLASS()
class REEL_API ABannerDevice : public ADevice
{
	GENERATED_BODY()

public:
	ABannerDevice();

	/** Sizes the matte from MatteSize (editor and runtime). */
	virtual void OnConstruction(const FTransform& Transform) override;	// BODY: (u)

	virtual void BeginPlay() override;	// BODY: (u)

	virtual bool CanFire(FString& OutWhyNot) const override;	// BODY: (u)

	/** Spends the use, starts the drop (first step this frame). */
	virtual bool Fire() override;	// BODY: (u)

	/** Drop -> lands when down -> holds Duration -> raises -> spent. */
	virtual void StepDevice(float DeltaSeconds) override;	// BODY: (u)

	/** Actor transform of EffectOffset. */
	virtual FVector GetEffectPoint() const override;	// BODY: (u)

	/** Step 2: true while the matte is Dropping, Down or Raising and the segment From -> To crosses the matte's
	 *  box where it hangs right now (Body's world bounds; FMath::LineBoxIntersection or a slab test). */
	virtual bool BlocksSight(const FVector& From, const FVector& To) const override;	// BODY: D

	/** Step 2: a restored spent banner starts Spent (rolled up, no drop left). */
	virtual void RestoreUses(int32 InUsesLeft) override;	// BODY: D

	/** The matte's size: width (Y), thickness (X), height (Z), cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Banner")
	FVector MatteSize = FVector(10.f, 400.f, 250.f);

	/** How far the matte falls, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Banner")
	float DropDistance = 300.f;

	UPROPERTY(EditAnywhere, Category = "Take|Banner")
	float DropSeconds = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Take|Banner")
	float RaiseSeconds = 1.0f;

	/** Centre of the masked area relative to the actor, cm (range is measured from here). A widget in the editor. */
	UPROPERTY(EditAnywhere, Category = "Take|Banner", meta = (MakeEditWidget = true))
	FVector EffectOffset = FVector(0.f, 0.f, -300.f);

	/** The batten the matte hangs from (a thin bar). */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> Batten;

	/** The batten's colour. */
	UPROPERTY(EditAnywhere, Category = "Take|Banner")
	FLinearColor BattenColour = TakeColors::Grey;

protected:
	EBannerPhase BannerPhase = EBannerPhase::Up;
	float PhaseSeconds = 0.f;

	/** QA devices: the winch's crank turns while the banner drops (one way) and while it raises (the other). */
	virtual bool IsHandleTurning() const override { return BannerPhase == EBannerPhase::Dropping || BannerPhase == EBannerPhase::Raising; }
	virtual float HandleSpinSign() const override { return BannerPhase == EBannerPhase::Raising ? -1.f : 1.f; }

private:
	/** Sizes and colours the batten, the matte and its click volume, matte rolled up. From OnConstruction and BeginPlay. */
	void LayoutBanner();

	/** Puts the matte (and its click volume) Alpha along its drop (0 up .. 1 down), this frame. */
	void SetMatteDrop(float Alpha);
};
