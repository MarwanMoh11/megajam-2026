// A gold floor mark: click it on camera A to cue Julian there (rule 3). Owner: (c).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TakeClickable.h"
#include "Mark.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;

/** Place it on the floor (actor location = the floor point Julian stands on). */
UCLASS()
class REEL_API AMark : public AActor, public ITakeClickable
{
	GENERATED_BODY()

public:
	AMark();

	/** Colours the disc gold, hides the ring and the X, lays out the ring's dots. */
	virtual void BeginPlay() override;	// BODY: (c)

	/** Same layout and look as BeginPlay, in the editor. */
	virtual void OnConstruction(const FTransform& Transform) override;

	// ITakeClickable
	virtual ETakeClickKind GetClickKind() const override { return ETakeClickKind::Mark; }

	/** Hover highlight: the disc brightens (GoldBright) and scales up a little; off restores it. This frame. */
	virtual void SetHovered(bool bHovered) override;	// BODY: (c)

	/** Called by AJulian::CueTo (true) and on arrival or a new cue elsewhere (false): the gold ring shows / hides
	 *  on this frame. */
	void SetCued(bool bCued);	// BODY: (c)

	/** Called by AJulian: the gold X under him shows while he stands on this mark (MARK). This frame. */
	void SetOccupied(bool bOccupied);	// BODY: (c)

	/** The floor point Julian walks to (the actor location). */
	FVector GetStandPoint() const;	// BODY: (c)

	bool IsCued() const { return bIsCued; }
	bool IsOccupied() const { return bIsOccupied; }

	/** The name the HUD and the log use ("ROPE", "PAST THE ROPE"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Mark")
	FString MarkName = TEXT("MARK");

	/** The mark past the rope: Julian standing on it clears the test (StepScene). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Mark")
	bool bGoal = false;

	/** Radius of the cued ring, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Mark")
	float RingRadius = 70.f;

	/** Number of dots in the ring. */
	UPROPERTY(EditAnywhere, Category = "Take|Mark")
	int32 RingDots = 16;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<USceneComponent> Root;

	/** The gold disc (a flat cylinder). */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> Disc;

	/** The cued ring: small gold blocks in a circle (instances relative to the actor), hidden until cued. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UInstancedStaticMeshComponent> Ring;

	/** The gold X under Julian: two flat bars at +-45 degrees, hidden until occupied. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> CrossA;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> CrossB;

	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UBoxComponent> ClickVolume;

protected:
	bool bIsCued = false;
	bool bIsOccupied = false;
	bool bIsHovered = false;

private:
	/** Lays out the ring's dots and colours the ring and the X (BeginPlay and OnConstruction). */
	void LayOutRing();

	/** Disc colour and scale, ring and X visibility, from the hovered / cued / occupied state. This frame. */
	void ApplyLook();
};
