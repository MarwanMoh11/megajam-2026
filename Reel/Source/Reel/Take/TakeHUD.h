// Everything on screen, drawn on the Canvas as text and flat rects (no UMG assets). Owner: (u).
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
	 *    along the bottom. The look follows Director->CurrentCamera, which changes on a flight's last frame.
	 *  - A beat dot that flashes with Clock->GetPulse().
	 *  - No pointer (the cursor is hidden). A centre reticle: a small cross or ring ReticleSize px across, white;
	 *    gold while Director->CurrentUse.Watcher is set (the spot has a watcher picked). Hidden during a flight.
	 *  - The use prompt: Director->CurrentUse.Prompt, centred just below the reticle, gold when bCanFire, grey when
	 *    not. Empty prompt (nothing in reach, Julian, a flight): nothing drawn.
	 *  - The crew strip, bottom centre above the hint: "1 JULIAN   2 MAYA   3 VEX", one entry per Id that
	 *    Director->GetCharacter finds (a missing Vex is drawn dim or left out), the current one
	 *    (Director->CurrentCamera) lit GoldBright, the others grey.
	 *  - During a flight (PC->IsInFlight()): a big "CUT -> MAYA" (PC->GetFlightTarget()->GetCrewName()) in LargeFont at
	 *    SlateTextScale across the middle, from the press frame to the landing frame.
	 *  - On B and C only (labels only on set): each watcher's label and state over it ("GUARD - LURED"), each
	 *    device's name and pips over it (ADevice::GetPipsText: the spot shows READY or BUSY 2.1).
	 *  - The flash lines (Director->Flashes), centred low, newest at the bottom.
	 *  - One controls hint line, the same on every character: ControlsHint, grey, bottom centre.
	 *  - Phase BlooperFreeze: a red slate across the middle, "BLOOPER - AGAIN FROM THE START" (checkpoint name from
	 *    the game instance), the reason, and "Cut!".
	 *  - Phase Cleared: a gold "CLEAR" stamp.
	 * The PC is GetOwningPlayerController() cast to ATakePlayerController (or Director->PC).
	 */
	virtual void DrawHUD() override;	// BODY: (u)

	/** The one controls line at the bottom. */
	UPROPERTY(EditAnywhere, Category = "Take|HUD")
	FString ControlsHint = TEXT("WASD walk   Mouse look   E use   1 2 3 switch   R new take");

	/** The centre reticle's size, px. */
	UPROPERTY(EditAnywhere, Category = "Take|HUD")
	float ReticleSize = 8.f;

	/** Camera A's letterbox: 2.39:1. */
	UPROPERTY(EditAnywhere, Category = "Take|HUD")
	float LetterboxAspect = 2.39f;

	UPROPERTY(EditAnywhere, Category = "Take|HUD")
	float TextScale = 1.4f;

	UPROPERTY(EditAnywhere, Category = "Take|HUD")
	float SlateTextScale = 2.5f;

	/** Height of the composure line, px. */
	UPROPERTY(EditAnywhere, Category = "Take|HUD")
	float ComposureLineHeight = 6.f;

	// --- step 2 (BODY: D; the layers are listed in the orchestrator's brief to D) ------------------------------------

	/** The objective line's text scale (top centre, under the bar). */
	UPROPERTY(EditAnywhere, Category = "Take|HUD")
	float ObjectiveScale = 1.6f;

	/** The scene title's text scale (LargeFont). */
	UPROPERTY(EditAnywhere, Category = "Take|HUD")
	float TitleScale = 2.2f;

	/** Watcher labels also show on Julian's camera (dimmer), not only on Maya's and Vex's. */
	UPROPERTY(EditAnywhere, Category = "Take|HUD")
	bool bLabelsOnJulian = true;

	/** Labels and pips farther than this from the camera are not drawn, cm (Maya's and Vex's cameras). */
	UPROPERTY(EditAnywhere, Category = "Take|HUD")
	float LabelMaxDistance = 4000.f;

	/** ...and on Julian's camera, where they are only a reminder, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|HUD")
	float JulianLabelMaxDistance = 2200.f;

	/** A watcher that has not arrived yet shows its "OFF" label only this close, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|HUD")
	float OffLabelMaxDistance = 1500.f;

	/** Two labels closer than this on screen (px on a 900 px tall screen: across, down) are stacked, not overlapped. */
	UPROPERTY(EditAnywhere, Category = "Take|HUD")
	FVector2D LabelStackSize = FVector2D(170.f, 20.f);

	/** The action bar's size (the lift), px. */
	UPROPERTY(EditAnywhere, Category = "Take|HUD")
	FVector2D ActionBarSize = FVector2D(260.f, 10.f);

	// --- step 3 (BODY: D; the layers are listed in the orchestrator's brief to D) ------------------------------------

	/** The title card and the credits: the game's name (a working title until Marwan picks one). */
	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	FString GameTitle = TEXT("PRINT IT");

	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	FString TitleTagline = TEXT("a heist shot in one take");

	/** How to play, on the title card (one line each). Empty at BeginPlay: filled with the default lines. */
	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	TArray<FString> TitleLines;

	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	FString TitlePrompt = TEXT("[ENTER]  ACTION");

	/** The credits under the blooper reel (one line each). Empty at BeginPlay: filled with the default lines. */
	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	TArray<FString> CreditLines;

	/** The title card's and the PRINT IT slate's name scale (LargeFont). */
	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	float BigTitleScale = 4.f;

	/** The blooper reel rolls up this many px (on a 900 px tall screen) a second. */
	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	float ReelScrollSpeed = 28.f;

	/** The alarm's red wash at the screen's edges: its strongest alpha, on the beat. */
	UPROPERTY(EditAnywhere, Category = "Take|HUD", meta = (ClampMin = "0", ClampMax = "1"))
	float AlarmEdgeAlpha = 0.45f;

	/** The alarm clock's text scale ("ALARM  5 BARS"). */
	UPROPERTY(EditAnywhere, Category = "Take|HUD")
	float AlarmClockScale = 2.4f;

protected:
	/** The director, cached (ATakeDirector::Get). */
	ATakeDirector* GetDirector();	// BODY: (u)

	/** Text with a dark drop shadow, centred on X when bCentre. Returns the drawn size. */
	FVector2D DrawShadowedText(const FString& Text, const FLinearColor& Colour, float X, float Y, float Scale,
		bool bCentre);	// BODY: (u)

	/** DrawShadowedText with a chosen font (the slate and stamp use LargeFont). */
	FVector2D DrawTextWithFont(const FString& Text, UFont* Font, const FLinearColor& Colour, float X, float Y,
		float Scale, bool bCentre);

	UPROPERTY(Transient)
	TObjectPtr<UFont> SmallFont;

	UPROPERTY(Transient)
	TObjectPtr<UFont> LargeFont;

	TWeakObjectPtr<ATakeDirector> CachedDirector;
};
