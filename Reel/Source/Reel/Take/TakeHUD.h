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
class UMaterialInstanceDynamic;

/** A line the HUD is showing from the director's flashes (its own timing, from the frame it first saw it). */
struct FTakeShown
{
	FString Text;
	ETakeFlashKind Kind = ETakeFlashKind::Note;
	double Since = -100.0;
	int32 Tab = 0;
	float Hold = 0.f;	// QA hud: a key prompt flash holds this long (its Flash seconds), real time
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
	 * QA hud: with bMinimalHud (the default) the bars keep only the fading goal line, the prompt lane and the alarm's
	 * seconds; composure is the picture (UpdateStress, then Film.DrawStress over the picture, under the HUD).
	 */
	virtual void DrawHUD() override;	// BODY: (u)

	// --- words (§15.7) ---------------------------------------------------------------------------------------------------

	/** The controls row, B3, during play (gone in the minimal HUD unless bShowControlsRow). */
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

	// --- QA hud: the minimal HUD (Marwan's note 7) -----------------------------------------------------------------------
	// Nothing on screen unless the player needs it this second: subtitles; one small prompt at a time near the bottom
	// centre (why a press was refused, the Use prompt with the device's uses, Julian's mark action, a cut key the first
	// times it is taught); the goal line when it changes, then it fades; at the alarm, the seconds left. Composure is
	// the picture (FTakeFilmOverlay's Stress cues and the grade's Stress scalar), not a meter.

	/** Master switch. Off: the full step-4 HUD (every lane below comes back). On: only what the switches allow. */
	UPROPERTY(EditAnywhere, Category = "Take|Minimal")
	bool bMinimalHud = true;

	/** With bMinimalHud on: bring back the slate strip (SCENE, name, TAKE). */
	UPROPERTY(EditAnywhere, Category = "Take|Minimal")
	bool bShowSlateStrip = false;

	/** With bMinimalHud on: bring back the key tabs, their device pips and the flight line. */
	UPROPERTY(EditAnywhere, Category = "Take|Minimal")
	bool bShowKeyTabs = false;

	/** With bMinimalHud on: bring back the running TIME. */
	UPROPERTY(EditAnywhere, Category = "Take|Minimal")
	bool bShowRunningTime = false;

	/** With bMinimalHud on: the alarm shows ALARM, the seconds and the draining bar, not the seconds alone. */
	UPROPERTY(EditAnywhere, Category = "Take|Minimal")
	bool bShowFullAlarm = false;

	/** With bMinimalHud on: bring back the composure line with its SEEN square and MARK T. */
	UPROPERTY(EditAnywhere, Category = "Take|Minimal")
	bool bShowComposureLine = false;

	/** With bMinimalHud on: bring back the controls row (the title card always shows the controls). */
	UPROPERTY(EditAnywhere, Category = "Take|Minimal")
	bool bShowControlsRow = false;

	/** With bMinimalHud on: bring back the tape labels on the crew's cameras. */
	UPROPERTY(EditAnywhere, Category = "Take|Minimal")
	bool bShowWorldLabels = false;

	/** With bMinimalHud on: bring back the countdown tags on Julian's camera. */
	UPROPERTY(EditAnywhere, Category = "Take|Minimal")
	bool bShowCountdownTags = false;

	/** With bMinimalHud on: the goal line stays up instead of fading. */
	UPROPERTY(EditAnywhere, Category = "Take|Minimal")
	bool bShowGoalAlways = false;

	/** With bMinimalHud on: the crew's reticle all the time, not only while a device is in reach. */
	UPROPERTY(EditAnywhere, Category = "Take|Minimal")
	bool bShowReticleAlways = false;

	/** A new goal holds this long (real seconds), then fades over GoalFadeSeconds. */
	UPROPERTY(EditAnywhere, Category = "Take|Minimal", meta = (ClampMin = "0"))
	float GoalHoldSeconds = 5.f;

	UPROPERTY(EditAnywhere, Category = "Take|Minimal", meta = (ClampMin = "0.01"))
	float GoalFadeSeconds = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Take|Minimal", meta = (ClampMin = "0.01"))
	float GoalFadeInSeconds = 0.2f;

	/** After a blooper (and when a take starts) the goal comes back for this long. */
	UPROPERTY(EditAnywhere, Category = "Take|Minimal", meta = (ClampMin = "0"))
	float GoalAfterBlooperSeconds = 3.f;

	/** A goal that starts with a cut key ("[2] Cut to MAYA ...") is a lesson: it holds in the prompt lane while it is
	 *  the goal, until the player has made that cut on cue this many times; after that it is an ordinary goal line.
	 *  A Note flash that starts with a keycap is a prompt too: the prompt lane, for its Flash seconds (real time). */
	UPROPERTY(EditAnywhere, Category = "Take|Minimal", meta = (ClampMin = "0"))
	int32 KeyTeachTimes = 3;

	/** Enter's answer during play (the controls) holds this long in the prompt lane. */
	UPROPERTY(EditAnywhere, Category = "Take|Minimal", meta = (ClampMin = "0.1"))
	float ControlsAnswerSeconds = 4.f;

	/** Prompts fade out over their last this many seconds. */
	UPROPERTY(EditAnywhere, Category = "Take|Minimal", meta = (ClampMin = "0.01"))
	float PromptFadeSeconds = 0.25f;

	/** The prompt lane's centre, as a fraction down the bottom bar. */
	UPROPERTY(EditAnywhere, Category = "Take|Minimal", meta = (ClampMin = "0", ClampMax = "1"))
	float PromptRowInBar = 0.4f;

	/** The goal line's centre, as a fraction down the top bar. */
	UPROPERTY(EditAnywhere, Category = "Take|Minimal", meta = (ClampMin = "0", ClampMax = "1"))
	float GoalRowInBar = 0.55f;

	// --- QA hud: composure as the picture ------------------------------------------------------------------------------------

	/** Composure drawn as the picture: the overlay's vignette, wash, burn, dirt and the seen pull (Film, "Stress"). */
	UPROPERTY(EditAnywhere, Category = "Take|Composure")
	bool bComposureInPicture = true;

	/** Write the stress to the director's film grade instance each frame (harmless if the material lacks it). */
	UPROPERTY(EditAnywhere, Category = "Take|Composure")
	bool bDriveGradeStress = true;

	UPROPERTY(EditAnywhere, Category = "Take|Composure")
	FName StressParameter = TEXT("Stress");

	/** Stress = (1 - composure / max) ^ StressCurve: under 1 it shows early (70: 0.43, 40: 0.70, 15: 0.89 at 0.7). */
	UPROPERTY(EditAnywhere, Category = "Take|Composure", meta = (ClampMin = "0.1", ClampMax = "3"))
	float StressCurve = 0.7f;

	/** The picture follows composure down this fast (time constant, real seconds) ... */
	UPROPERTY(EditAnywhere, Category = "Take|Composure", meta = (ClampMin = "0.01"))
	float StressRiseSeconds = 0.12f;

	/** ... and recovers this slowly (a refill breathes back). */
	UPROPERTY(EditAnywhere, Category = "Take|Composure", meta = (ClampMin = "0.01"))
	float StressFallSeconds = 0.6f;

	/** A watcher holding Julian: the cue comes in this fast and leaves this slowly (time constants, real seconds). */
	UPROPERTY(EditAnywhere, Category = "Take|Composure", meta = (ClampMin = "0.01"))
	float SeenRiseSeconds = 0.08f;

	UPROPERTY(EditAnywhere, Category = "Take|Composure", meta = (ClampMin = "0.01"))
	float SeenFallSeconds = 0.5f;

	/** How much a watcher ahead or behind pulls toward the top or bottom edge (1: as much as one to the side). */
	UPROPERTY(EditAnywhere, Category = "Take|Composure", meta = (ClampMin = "0", ClampMax = "1"))
	float SeenVerticalWeight = 0.5f;

	/** The stress the picture shows now, 0..1 (eased, real time): for anything else that wants to follow it. */
	float GetStress() const { return Stress; }

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
	// QA hud: the minimal HUD (TakeHUD.cpp)
	bool Shows(bool bSwitch) const { return !bMinimalHud || bSwitch; }
	float RefusalHold() const;
	float GoalRow() const;
	float PromptRow() const;
	float GoalAlpha(const ATakeDirector& Dir) const;
	bool IsKeyTeaching() const { return bMinimalHud && bTeaching; }
	void DrawPromptLane(const ATakeDirector& Dir, bool bRefusalsOnly);
	void DrawUsePrompt(const FTakeUseTarget& Use, float Y);
	void DrawAlarmNumber(const ATakeDirector& Dir);
	static FString GoalKey(const FString& In);
	static int32 LeadingCutKey(const FString& In);
	static float FadeOut(float InAge, float Hold, float FadeSeconds);
	// QA hud: composure as the picture (TakeHUDStress.cpp): eases Stress and the seen cue, writes the grade's scalar.
	void UpdateStress(const ATakeDirector& Dir, FTakeFilmFrame& F);
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

	// --- QA hud ---
	FTakeShown KeyPrompt;			// a Note flash that starts with a keycap: the prompt lane, for its own seconds
	bool bGoalBrief = false;		// the goal came back after a blooper or a take's start: GoalAfterBlooperSeconds
	bool bTeaching = false;			// the goal is a cut key being taught (it holds in the prompt lane)
	int32 TeachKey = 0;				// that key (1-3), until the player cuts to its camera
	int32 TaughtCount[4] = { 0, 0, 0, 0 };	// cuts made on cue, per key
	float Stress = 0.f;
	float SeenWeight = 0.f;
	FVector2D SeenDir = FVector2D::ZeroVector;
	double StressTime = -1.0;
	float AppliedStress = -1.f;
	TWeakObjectPtr<UMaterialInstanceDynamic> StressMID;
};
