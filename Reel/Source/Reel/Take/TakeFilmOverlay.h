// Step 4 (the look, pass 1): the film overlay and the letterbox, drawn on Canvas by ATakeHUD (docs/ART_STYLE.md
// §15.4). On the film picture: grain, flicker, dust and scratches, all quantised to 24 fps; on the making-of: a
// viewfinder (corner marks, scan lines). On every camera: the 2.39:1 bars in ink, the splice at a cut, the burn as
// composure falls, the blooper's burn and the PRINT IT snap to black. The 3D picture is never moved.
// Colours and sizes come from TakeStyle.h; every number here is a tunable on the HUD (ATakeHUD::Film).

#pragma once

#include "CoreMinimal.h"
#include "TakeTypes.h"
#include "TakeFilmOverlay.generated.h"

class AHUD;
class UCanvas;
class UTexture2D;

/** What the HUD tells the overlay each frame (all read on the frame, after the world tick). */
struct FTakeFilmFrame
{
	float ScreenW = 1920.f;
	float ScreenH = 1080.f;
	double RealTime = 0.0;
	bool bFilmLook = true;			// the director's master switch
	bool bFilmPicture = true;		// the film (A, or every camera with the split off); else the making-of
	ETakeCamera LookCamera = ETakeCamera::A;
	bool bInFlight = false;			// a cut's flight is running (a new LookCamera with this set is a cut press)
	ETakePhase Phase = ETakePhase::Rolling;
	float Composure = 100.f;		// 0..100
	bool bSeen = false;			// composure draining
	bool bJulianOnScreen = false;
	FVector2D JulianScreen = FVector2D::ZeroVector;
};

USTRUCT()
struct FTakeFilmOverlay
{
	GENERATED_BODY()

	// --- switches ----------------------------------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Film")
	bool bLetterbox = true;
	UPROPERTY(EditAnywhere, Category = "Film")
	bool bGrain = true;
	UPROPERTY(EditAnywhere, Category = "Film")
	bool bFlicker = true;
	UPROPERTY(EditAnywhere, Category = "Film")
	bool bDust = true;
	UPROPERTY(EditAnywhere, Category = "Film")
	bool bScratch = true;
	UPROPERTY(EditAnywhere, Category = "Film")
	bool bSplice = true;
	UPROPERTY(EditAnywhere, Category = "Film")
	bool bViewfinder = true;
	UPROPERTY(EditAnywhere, Category = "Film")
	bool bBurn = true;
	UPROPERTY(EditAnywhere, Category = "Film")
	bool bBlooperBurn = true;
	UPROPERTY(EditAnywhere, Category = "Film")
	bool bPrintItSnap = true;

	// --- the letterbox --------------------------------------------------------------------------------------------
	/** Picture width / height inside the screen; the bars are ink. 2.39 on 1920x1080: picture 1920 x 804. */
	UPROPERTY(EditAnywhere, Category = "Film", meta = (ClampMin = "1"))
	float LetterboxAspect = 2.39f;

	// --- grain: a 256x256 grey noise tile, 1:1 in pixels, offset re-rolled 24 times a second -----------------------
	UPROPERTY(EditAnywhere, Category = "Film|Grain", meta = (ClampMin = "0", ClampMax = "1"))
	float GrainOpacity = 0.07f;
	/** For CutBurstFrames after a cut to the film (§15.4 says 0.16; held to GrainMax). */
	UPROPERTY(EditAnywhere, Category = "Film|Grain", meta = (ClampMin = "0", ClampMax = "1"))
	float GrainCutOpacity = 0.16f;
	/** On the title card and the end pages. */
	UPROPERTY(EditAnywhere, Category = "Film|Grain", meta = (ClampMin = "0", ClampMax = "1"))
	float GrainCardOpacity = 0.12f;
	/** No grain is ever drawn above this (the step 4 brief: never above 0.14). */
	UPROPERTY(EditAnywhere, Category = "Film|Grain", meta = (ClampMin = "0", ClampMax = "1"))
	float GrainMax = 0.14f;

	// --- flicker: an ink wash over the picture, re-rolled 24 times a second ----------------------------------------
	UPROPERTY(EditAnywhere, Category = "Film|Flicker", meta = (ClampMin = "0", ClampMax = "1"))
	float FlickerMax = 0.03f;
	UPROPERTY(EditAnywhere, Category = "Film|Flicker", meta = (ClampMin = "0", ClampMax = "1"))
	float FlickerCutMax = 0.05f;
	UPROPERTY(EditAnywhere, Category = "Film|Flicker", meta = (ClampMin = "0", ClampMax = "1"))
	float FlickerCardMax = 0.06f;
	/** Below LowComposure on the film (and on the blooper's first frame: BlooperFlicker). */
	UPROPERTY(EditAnywhere, Category = "Film|Flicker", meta = (ClampMin = "0", ClampMax = "1"))
	float FlickerLowMax = 0.06f;
	UPROPERTY(EditAnywhere, Category = "Film|Flicker", meta = (ClampMin = "0", ClampMax = "1"))
	float BlooperFlicker = 0.10f;

	// --- dust and scratches ---------------------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Film|Dust", meta = (ClampMin = "0", ClampMax = "24"))
	float DustPerSecond = 1.5f;
	UPROPERTY(EditAnywhere, Category = "Film|Dust", meta = (ClampMin = "0", ClampMax = "24"))
	float DustCardPerSecond = 3.f;
	UPROPERTY(EditAnywhere, Category = "Film|Dust", meta = (ClampMin = "0", ClampMax = "24"))
	float DustLowPerSecond = 4.f;
	/** Composure below this on the film: more dust and flicker (§15.4). */
	UPROPERTY(EditAnywhere, Category = "Film|Dust", meta = (ClampMin = "0", ClampMax = "100"))
	float LowComposure = 25.f;
	UPROPERTY(EditAnywhere, Category = "Film|Dust", meta = (ClampMin = "0.5"))
	float ScratchEveryMin = 6.f;
	UPROPERTY(EditAnywhere, Category = "Film|Dust", meta = (ClampMin = "0.5"))
	float ScratchEveryMax = 10.f;
	UPROPERTY(EditAnywhere, Category = "Film|Dust", meta = (ClampMin = "0.5"))
	float ScratchCardEvery = 2.f;
	UPROPERTY(EditAnywhere, Category = "Film|Dust", meta = (ClampMin = "0", ClampMax = "1"))
	float ScratchOpacity = 0.2f;

	// --- the splice at a cut (frame 0 = the frame of the press) -----------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Film|Splice", meta = (ClampMin = "0", ClampMax = "0.85"))
	float SpliceFlashOpacity = 0.35f;
	/** The ink frame-line band that rolls up the picture over SpliceBandFrames on a cut to the film, px at 1080. */
	UPROPERTY(EditAnywhere, Category = "Film|Splice", meta = (ClampMin = "0"))
	float SpliceBandPx = 32.f;
	UPROPERTY(EditAnywhere, Category = "Film|Splice", meta = (ClampMin = "1"))
	int32 SpliceBandFrames = 3;
	/** Film frames after a cut with the grain (or the scan lines) raised. */
	UPROPERTY(EditAnywhere, Category = "Film|Splice", meta = (ClampMin = "0"))
	int32 CutBurstFrames = 6;

	// --- the making-of viewfinder --------------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Film|Viewfinder", meta = (ClampMin = "0"))
	float CornerArmPx = 48.f;
	UPROPERTY(EditAnywhere, Category = "Film|Viewfinder", meta = (ClampMin = "0"))
	float CornerInsetPx = 32.f;
	UPROPERTY(EditAnywhere, Category = "Film|Viewfinder", meta = (ClampMin = "0", ClampMax = "1"))
	float CornerAlpha = 0.6f;
	UPROPERTY(EditAnywhere, Category = "Film|Viewfinder", meta = (ClampMin = "0", ClampMax = "1"))
	float ScanLineAlpha = 0.05f;
	UPROPERTY(EditAnywhere, Category = "Film|Viewfinder", meta = (ClampMin = "0", ClampMax = "1"))
	float ScanLineCutAlpha = 0.10f;

	// --- the burn as composure falls (every camera) --------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Film|Burn", meta = (ClampMin = "0", ClampMax = "100"))
	float BurnBelow = 50.f;
	UPROPERTY(EditAnywhere, Category = "Film|Burn", meta = (ClampMin = "0"))
	float BurnMinPx = 24.f;
	UPROPERTY(EditAnywhere, Category = "Film|Burn", meta = (ClampMin = "0"))
	float BurnExtraPx = 96.f;
	UPROPERTY(EditAnywhere, Category = "Film|Burn", meta = (ClampMin = "0", ClampMax = "1"))
	float BurnAlpha = 0.6f;

	// --- the blooper (frame 0 = the blooper) ---------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Film|Burn", meta = (ClampMin = "1"))
	int32 BlooperBurnFrames = 9;
	UPROPERTY(EditAnywhere, Category = "Film|Burn", meta = (ClampMin = "0"))
	int32 BlooperPaperFrames = 3;

	/** Under the HUD: the picture's film or viewfinder, the burn, the splice, then the bars. Call first in DrawHUD. */
	void DrawUnder(AHUD& Hud, UCanvas& Canvas, const FTakeFilmFrame& F);

	/** On a card (title card, end pages), after its background and before its text: the film at the card levels. */
	void DrawCard(AHUD& Hud, UCanvas& Canvas, const FTakeFilmFrame& F);

	/** Over the HUD: the blooper's burn and paper frames, the PRINT IT ink frame. Call last in DrawHUD. */
	void DrawOver(AHUD& Hud, UCanvas& Canvas, const FTakeFilmFrame& F);

	/** The bars are drawn (the HUD's old camera-A bars are not needed). */
	bool DrawsLetterbox(const FTakeFilmFrame& F) const { return F.bFilmLook && bLetterbox; }

	/** The blooper slate may show: 12 film frames after the blooper (§15.4), or at once without the blooper burn. */
	bool IsBlooperSlateDue(const FTakeFilmFrame& F) const;

	/** The bar height for this screen (0 without the letterbox). */
	float BarHeight(const FTakeFilmFrame& F) const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> GrainTex;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> RampVTex;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> RampHTex;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> BurnTex;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> ScanTex;

	// Runtime state (not saved).
	int64 FilmFrame = -1;
	FVector2D GrainOffset = FVector2D::ZeroVector;
	float FlickerRoll = 0.f;
	bool bDustThisFrame = false;
	FVector2D DustPos = FVector2D::ZeroVector;
	float DustSize = 3.f;
	bool bDustLight = false;
	double NextScratchTime = -1.0;
	double ScratchUntil = -1.0;
	float ScratchX = 0.5f;
	bool bHaveLook = false;
	ETakeCamera LastLook = ETakeCamera::A;
	bool bCutToFilm = true;
	double CutTime = -100.0;
	ETakePhase LastPhase = ETakePhase::Rolling;
	double PhaseTime = -100.0;

	void EnsureTextures();
	void Track(const FTakeFilmFrame& F);
	void Roll(const FTakeFilmFrame& F, float DustRate, double ScratchMin, double ScratchMax);
	void DrawFilm(AHUD& Hud, UCanvas& Canvas, const FTakeFilmFrame& F, float X, float Y, float W, float H,
		float Grain, float Flicker);
	void DrawEdgeBands(UCanvas& Canvas, const FLinearColor& Colour, float X, float Y, float W, float H, float Depth);
};
