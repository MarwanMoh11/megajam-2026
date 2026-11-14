// Everything on screen, drawn on the Canvas (no UMG assets). Owner: (u). Step 4 pass 2: docs/ART_STYLE.md §15.5-15.7.
// Reads everything through ATakeDirector's public state and the actors' getters; never changes game state.
// Every colour, size and timing comes from TakeStyle.h through the kit (TakeHudKit.h); the tunables are below.
// Drawing is split over three files: TakeHUD.cpp (the frame, the bars, flashes), TakeHUDPicture.cpp (reticle,
// labels, subtitles, stamps, the opening slate), TakeHUDCards.cpp (title card, blooper slate, PRINT IT, end pages).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TakeFilmOverlay.h"
#include "TakeHudKit.h"
#include "TakeTypes.h"
#include "TakeHUD.generated.h"

class ATakeDirector;

/** A line the HUD is showing from the director's flashes (its own timing, from the frame it first saw it). */
struct FTakeShown
{
	FString Text;
	ETakeFlashKind Kind = ETakeFlashKind::Note;
	double Since = -100.0;
	int32 Tab = 0;
};

/** A spoken line waiting for, or holding, the subtitle row. */
struct FTakeSubtitle
{
	FString Speaker;
	FString Line;
	float Hold = 2.f;
	double Since = -1.0;	// < 0: queued, not shown yet
};

UCLASS()
class REEL_API ATakeHUD : public AHUD
{
	GENERATED_BODY()

public:
	ATakeHUD();

	/**
	 * Drawn every frame after the world tick, so whatever a press changed this frame is on screen this frame.
	 * Order: the film overlay's under-layer (picture dirt or viewfinder, burn, splice, bars); then by phase:
	 *  - Title: the title card (a card over the picture; the bars stay ink).
	 *  - Printed: the PRINT IT slate, the stats and grade, the blooper reel and credits (cards, keys in B3).
	 *  - Rolling / Cleared / BlooperFreeze: the picture layer (reticle on the crew's cameras, world labels by the
	 *    label rule, subtitles, the opening slate, scene titles and stamps), then the bars: T1 slate strip, key tabs
	 *    with pips, bar clock or finale count; T2 goal line or flash; B1 Use prompt, Julian's mark action or a
	 *    refusal; B2 composure; B3 controls. Then the overlay's over-layer (blooper burn, PRINT IT snap), then the
	 *    blooper slate when it is due, and B1 refusals over everything.
	 */
	virtual void DrawHUD() override;	// BODY: (u)

	// --- words (§15.7) ---------------------------------------------------------------------------------------------------

	/** The controls row, B3, during play. */
	UPROPERTY(EditAnywhere, Category = "Take|HUD")
	FString ControlsHint = TEXT("[WASD] Walk   [MOUSE] Look   [E] Use   [1] [2] [3] Cut   [R] New take");

	/** The scenes' names for the slate strip and the slates, index 0 = Scene 1. */
	UPROPERTY(EditAnywhere, Category = "Take|HUD")
	TArray<FString> SceneNames;

	UPROPERTY(EditAnywhere, Category = "Take|HUD")
	FString DirectorWord = TEXT("YOU");

	// --- timings (§15.2 motion, §15.5) --------------------------------------------------------------------------------------

	/** A flash holds the goal line's place this long (48 film frames). */
	UPROPERTY(EditAnywhere, Category = "Take|HUD", meta = (ClampMin = "0.1"))
	float FlashHoldSeconds = 2.f;

	/** A refused press says why in B1 this long (36 film frames). */
	UPROPERTY(EditAnywhere, Category = "Take|HUD", meta = (ClampMin = "0.1"))
	float RefusalHoldSeconds = 1.5f;

	/** A subtitle holds SubtitleBaseSeconds + SubtitlePerCharSeconds a character. */
	UPROPERTY(EditAnywhere, Category = "Take|HUD", meta = (ClampMin = "0.1"))
	float SubtitleBaseSeconds = 2.f;

	UPROPERTY(EditAnywhere, Category = "Take|HUD", meta = (ClampMin = "0"))
	float SubtitlePerCharSeconds = 0.06f;

	/** Lines waiting behind the one on screen; a newer line pushes out the oldest waiting one. */
	UPROPERTY(EditAnywhere, Category = "Take|HUD", meta = (ClampMin = "0"))
	int32 SubtitleQueueMax = 2;

	/** Stamps over play hold this many film frames (titles and DELETED SCENE RESTORED: StampTitleHoldFrames). */
	UPROPERTY(EditAnywhere, Category = "Take|HUD", meta = (ClampMin = "1"))
	float StampHoldFrames = 36.f;

	UPROPERTY(EditAnywhere, Category = "Take|HUD", meta = (ClampMin = "1"))
	float StampTitleHoldFrames = 72.f;

	/** A new goal's bone rule draws out from the centre over this many film frames. */
	UPROPERTY(EditAnywhere, Category = "Take|HUD", meta = (ClampMin = "1"))
	float GoalRuleFrames = 12.f;

	// --- the label rule (§15.5) ------------------------------------------------------------------------------------------------

	/** Off (default, STATUS rule 1): Julian's camera shows only countdown tags. On: tape labels on the film too. */
	UPROPERTY(EditAnywhere, Category = "Take|Labels")
	bool bLabelsOnFilmCamera = false;

	/** Only watchers and devices this close to the camera get a label, cm (40 m). */
	UPROPERTY(EditAnywhere, Category = "Take|Labels", meta = (ClampMin = "0"))
	float LabelMaxDistance = 4000.f;

	/** A watcher that has not arrived yet shows its OFF label only this close, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Labels", meta = (ClampMin = "0"))
	float OffLabelMaxDistance = 1500.f;

	/** At most this many labels, plus the priority-1 label (the picked watcher or the device in reach). */
	UPROPERTY(EditAnywhere, Category = "Take|Labels", meta = (ClampMin = "0"))
	int32 MaxLabels = 4;

	/** Julian's camera: at most this many countdown tags (blinded, lured, masked). */
	UPROPERTY(EditAnywhere, Category = "Take|Labels", meta = (ClampMin = "0"))
	int32 MaxCountdownTags = 3;

	/** Watchers with the same name and state this close share one label ("2 GUARDS"), cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Labels", meta = (ClampMin = "0"))
	float LabelMergeDistance = 300.f;

	/** An overlapping label moves up at most this many times, then hides. */
	UPROPERTY(EditAnywhere, Category = "Take|Labels", meta = (ClampMin = "0"))
	int32 LabelMaxLifts = 2;

	/** A label's bottom edge this far above the actor's label point, px at 1080. */
	UPROPERTY(EditAnywhere, Category = "Take|Labels", meta = (ClampMin = "0"))
	float LabelLiftPx = 8.f;

	// --- the ends (step 3, reworded by §15.7) -----------------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	FString GameTitle = TEXT("PRINT IT");

	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	FString TitleTagline = TEXT("A heist shot in one take.");

	/** The title card's pitch, one line each (type.body). */
	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	TArray<FString> TitlePitch;

	/** The cast row: "key|NAME|job", three columns. */
	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	TArray<FString> TitleCast;

	/** The rule lines; [danger] and [julian] draw the glyphs. */
	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	TArray<FString> TitleRules;

	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	FString TitleControls = TEXT("[WASD] Walk   [MOUSE] Look   [E] Use   [1] [2] [3] Cut   [R] New take");

	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	FString TitlePrompt = TEXT("[ENTER] Action!");

	/** The credits after the blooper reel (the first line is drawn as a title). */
	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	TArray<FString> CreditLines;

	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	FString EndKeys = TEXT("[ENTER] Next   [R] New take   [ESC] Quit");

	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	FString EndKeysLast = TEXT("[R] New take   [ESC] Quit");

	/** The blooper reel rolls up this many px (at 1080) a second. */
	UPROPERTY(EditAnywhere, Category = "Take|Ends")
	float ReelScrollSpeed = 34.f;

	/** The stats rows appear one every this many seconds; the grade stamp lands GradeDelayFrames after the page. */
	UPROPERTY(EditAnywhere, Category = "Take|Ends", meta = (ClampMin = "0"))
	float StatsRowSeconds = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Take|Ends", meta = (ClampMin = "0"))
	float GradeDelayFrames = 24.f;

	/** Step 4: the film overlay and the letterbox (TakeFilmOverlay.h): every number and switch is here. */
	UPROPERTY(EditAnywhere, Category = "Take|Film")
	FTakeFilmOverlay Film;

protected:
	/** Step 4: what the film overlay needs this frame. */
	FTakeFilmFrame MakeFilmFrame(const ATakeDirector& Dir, float ScreenW, float ScreenH) const;

	/** The director, cached (ATakeDirector::Get). */
	ATakeDirector* GetDirector();	// BODY: (u)

	// TakeHUD.cpp
	void ReadFlashes(const ATakeDirector& Dir);
	void TrackState(const ATakeDirector& Dir);
	void DrawBars(const ATakeDirector& Dir);
	void DrawSlateStrip(const ATakeDirector& Dir);
	void DrawKeyTabs(const ATakeDirector& Dir);
	void DrawClock(const ATakeDirector& Dir);
	void DrawGoal(const ATakeDirector& Dir);
	void DrawB1(const ATakeDirector& Dir, bool bRefusalsOnly);
	void DrawComposure(const ATakeDirector& Dir);
	bool IsRefusalShowing() const;
	bool IsControlsAnswer() const;
	static FString NoFinalStop(const FString& In);
	float Age(double Since) const { return static_cast<float>(Kit.Now - Since); }
	FString SceneName(int32 Scene) const;

	// TakeHUDPicture.cpp
	void DrawPicture(const ATakeDirector& Dir, const FTakeFilmFrame& F);
	void DrawReticle(const ATakeDirector& Dir);
	void DrawLabels(const ATakeDirector& Dir);
	void DrawSubtitles(const FTakeFilmFrame& F);
	void DrawStamps(const ATakeDirector& Dir);
	void DrawOpeningSlate(const ATakeDirector& Dir, float SlateAge, float Alpha);
	void DrawSubtitleNow(const FString& Speaker, const FString& Line, bool bFilm);

	// TakeHUDCards.cpp
	void DrawTitleCard(const ATakeDirector& Dir, const FTakeFilmFrame& F);
	void DrawBlooperSlate(const ATakeDirector& Dir);
	void DrawEnds(const ATakeDirector& Dir, const FTakeFilmFrame& F);
	void DrawPrintIt(const ATakeDirector& Dir);
	void DrawStats(const ATakeDirector& Dir, const FVector4f& Box);
	void DrawReel(const ATakeDirector& Dir, const FVector4f& Box);

	UPROPERTY(Transient)
	FTakeHudKit Kit;

	TWeakObjectPtr<ATakeDirector> CachedDirector;

	// What the HUD is showing, on its own clock (real time, so a freeze or a pause cannot stall a line).
	int32 LastSerial = 0;
	FTakeShown GoalFlash;
	FTakeShown Refusal;
	TArray<FTakeShown> Stamps;
	TArray<FTakeSubtitle> Subtitles;
	FString LastGoal;
	double GoalSince = -100.0;
	float LastComposure = -1.f;
	float RefillFrom = 0.f;
	float RefillTo = 0.f;
	double RefillSince = -100.0;
	ETakePhase LastPhase = ETakePhase::Rolling;
	double PhaseSince = 0.0;
	ETakeCamera LastLook = ETakeCamera::A;
	double LookSince = -100.0;
};
