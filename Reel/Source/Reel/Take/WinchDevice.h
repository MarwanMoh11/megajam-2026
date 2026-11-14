// Vex's counterweight winch: the curtain drops for good, masks both flanks, and opens the stage trap (rule 7).
// Step 3. Owner: W.

#pragma once

#include "CoreMinimal.h"
#include "Device.h"
#include "WinchDevice.generated.h"

/** One curtain panel: a flat cloth hanging between two points. */
USTRUCT(BlueprintType)
struct FTakeCurtainPanel
{
	GENERATED_BODY()

	/** The two ends of the panel's foot on the floor it lands on, WORLD space, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Take|Winch")
	FVector A = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Take|Winch")
	FVector B = FVector::ZeroVector;
};

/**
 * Place the actor at the winch, backstage. One use, locked until the alarm (ADevice::LockedUntil = "Case").
 *
 * The curtain is Panels: each a flat box CurtainThickness thick and CurtainHeight tall standing on the line A-B
 * (made at BeginPlay with FTakeShapes::AddShapeAtRuntime, absolute transforms, NoCollision: a walker and a camera
 * pass through cloth). Before it is fired every panel hangs rolled up: its foot RaisedHeight above A-B.
 *
 * Fired: the panels start down on that frame and reach the floor after DropSeconds; on that frame it lands
 * (MarkLanded; the clock thuds, UBarClockComponent::PlayThud) and stays down for the rest of the take: it never
 * raises and never becomes "spent and gone". Effect is Mask but it catches no watcher by range (GatherTargets
 * returns none): it masks by geometry. BlocksSight(From, To) is true while a panel is dropping or down and the
 * segment crosses that panel's box where it hangs right now (the banner's test, per panel). The director fires
 * LandTrigger ("Winch") on the landing frame, which makes the trap's mark appear.
 *
 * RestoreUses(0) (a restore after it was fired): the curtain is down at once.
 */
UCLASS()
class REEL_API AWinchDevice : public ADevice
{
	GENERATED_BODY()

public:
	AWinchDevice();

	/** Super, then builds the panels, rolled up. */
	virtual void BeginPlay() override;

	/** Not while the curtain is dropping or down ("WINCH - the curtain is down"). */
	virtual bool CanFire(FString& OutWhyNot) const override;

	/** Spends the use, starts the drop (the first step this frame). */
	virtual bool Fire() override;

	/** The drop; lands when the panels reach the floor; nothing after that. */
	virtual void StepDevice(float DeltaSeconds) override;

	/** No watcher is caught by range. */
	virtual void GatherTargets(const TArray<TObjectPtr<AWatcher>>& All, TArray<AWatcher*>& Out) const override;

	virtual bool BlocksSight(const FVector& From, const FVector& To) const override;

	virtual void RestoreUses(int32 InUsesLeft) override;

	/** The curtain has landed. */
	bool IsCurtainDown() const { return bCurtainDown; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Winch")
	TArray<FTakeCurtainPanel> Panels;

	/** Height of every panel, cm (taller than a guard's eye and Julian's head, lower than the drone). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Winch")
	float CurtainHeight = 600.f;

	/** How far above the floor a rolled-up panel's foot hangs, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Winch")
	float RaisedHeight = 700.f;

	UPROPERTY(EditAnywhere, Category = "Take|Winch")
	float CurtainThickness = 12.f;

	UPROPERTY(EditAnywhere, Category = "Take|Winch")
	float DropSeconds = 0.7f;

	UPROPERTY(EditAnywhere, Category = "Take|Winch")
	FLinearColor CurtainColour = FLinearColor(0.33f, 0.03f, 0.04f);

protected:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> PanelMeshes;

	bool bDropping = false;
	bool bCurtainDown = false;
	float DropElapsed = 0.f;

	/** Puts every panel Alpha along its drop (0 rolled up .. 1 on the floor), this frame. */
	void SetCurtainDrop(float Alpha);
};
