// Everything on screen, drawn on the Canvas as text and flat rects (no UMG assets). Owner: (a).
// Reads everything through ATakeDirector's public state and the actors' getters; never changes game state.
// Add drawing helpers as free functions in a uniquely named namespace in TakeHUD.cpp (no header change needed).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TakeHUD.generated.h"

class ATakeDirector;
class UFont;

UCLASS()
class REEL_API ATakeHUD : public AHUD
{
	GENERATED_BODY()

public:
	ATakeHUD();

	/**
	 * Drawn every frame after the world tick (so after the director's late tick): whatever a press changed this
	 * frame is on screen this frame. Layers, in order:
	 *  - Camera look: on A, black letterbox bars to LetterboxAspect with "A - JULIAN" + timecode (bar.beat) + "TAKE n"
	 *    top left, a red REC dot top right, the gold composure line and its label (SEEN in red / MARK in gold) in the
	 *    bottom bar. On B, no bars: "B - MAYA" top left in work-light blue, timecode, take, and the composure line
	 *    along the bottom. This is the visible change of look on a cut.
	 *  - A beat dot that flashes with Clock->GetPulse().
	 *  - At Julian's cued mark (while he walks, every camera): the count "in 6" (beats to arrival, rounded up).
	 *  - On B and C only (labels only on set): each watcher's label and state over it ("GUARD - LURED"), each
	 *    device's name and pips over it (ADevice::GetPipsText: the spot shows READY or BUSY 2.1).
	 *  - The flash lines (Director->Flashes), centred low, newest at the bottom.
	 *  - One how-to-play line per camera: A "Click a gold mark to cue Julian   [2] Maya", B "Click a device to fire,
	 *    a watcher to spot him   [1] Julian".
	 *  - Phase BlooperFreeze: a red slate across the middle, "BLOOPER - AGAIN FROM THE START" (checkpoint name from
	 *    the game instance), the reason, and "Cut!".
	 *  - Phase Cleared: a gold "CLEAR" stamp.
	 */
	virtual void DrawHUD() override;	// BODY: (a)

	/** Camera A's letterbox: 2.39:1. */
	UPROPERTY(EditAnywhere, Category = "Take|HUD")
	float LetterboxAspect = 2.39f;

	UPROPERTY(EditAnywhere, Category = "Take|HUD")
	float TextScale = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Take|HUD")
	float SlateTextScale = 2.5f;

	/** Height of the composure line, px. */
	UPROPERTY(EditAnywhere, Category = "Take|HUD")
	float ComposureLineHeight = 6.f;

protected:
	/** The director, cached (ATakeDirector::Get). */
	ATakeDirector* GetDirector();	// BODY: (a)

	/** Text with a dark drop shadow, centred on X when bCentre. Returns the drawn size. */
	FVector2D DrawShadowedText(const FString& Text, const FLinearColor& Colour, float X, float Y, float Scale,
		bool bCentre);	// BODY: (a)

	/** DrawShadowedText with a chosen font (the slate and stamp use LargeFont). */
	FVector2D DrawTextWithFont(const FString& Text, UFont* Font, const FLinearColor& Colour, float X, float Y,
		float Scale, bool bCentre);

	UPROPERTY(Transient)
	TObjectPtr<UFont> SmallFont;

	UPROPERTY(Transient)
	TObjectPtr<UFont> LargeFont;

	TWeakObjectPtr<ATakeDirector> CachedDirector;
};
