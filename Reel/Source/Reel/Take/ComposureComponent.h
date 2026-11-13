// Julian's composure (rule 5), on the director. Owner: (a). Bodies marked "// BODY: (a)".

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ComposureComponent.generated.h"

/** The label next to the gold composure line. */
UENUM(BlueprintType)
enum class EComposureLabel : uint8
{
	None,
	Seen,	// "SEEN": a guard frame holds an exposed Julian (shown from the first frame, grace included)
	Mark	// "MARK": on a mark and not exposed
};

/** 0-100. Does not tick by itself: the director calls Step in tick step 5. */
UCLASS(ClassGroup = (Take), meta = (BlueprintSpawnableComponent))
class REEL_API UComposureComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UComposureComponent();

	UPROPERTY(EditAnywhere, Category = "Take|Composure")
	float MaxComposure = 100.f;

	/** Being SEEN costs nothing for this long (continuous), then drains. */
	UPROPERTY(EditAnywhere, Category = "Take|Composure")
	float SeenGraceSeconds = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Take|Composure")
	float SeenDrainPerSecond = 30.f;

	UPROPERTY(EditAnywhere, Category = "Take|Composure")
	float MarkRefillPerSecond = 15.f;

	/** Step 5 (director). bSeen: SEEN this frame; the continuous-seen timer grows, and past SeenGraceSeconds the
	 *  value drains at SeenDrainPerSecond; label Seen. Else bOnMarkUnexposed: +MarkRefillPerSecond, label Mark.
	 *  Else label None. Not seen resets the grace timer. Clamped to 0..MaxComposure. */
	void Step(float DeltaSeconds, bool bSeen, bool bOnMarkUnexposed);	// BODY: (a)

	/** Back to MaxComposure, label None, grace timer reset (a reload or a checkpoint). */
	void Refill();	// BODY: (a)

	float GetComposure() const;	// BODY: (a)

	/** 0..1 */
	float GetFraction() const;	// BODY: (a)

	bool IsEmpty() const;	// BODY: (a)

	EComposureLabel GetLabel() const;	// BODY: (a)

	/** "SEEN", "MARK" or "". */
	FString GetLabelText() const;	// BODY: (a)

	/** Continuous seconds SEEN (0 when not seen). */
	float GetSeenSeconds() const;	// BODY: (a)

private:
	float Value = 100.f;
	float SeenSeconds = 0.f;
	EComposureLabel Label = EComposureLabel::None;
};
