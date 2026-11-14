// Maya's house dimmer: the hall's lights down for the preview of Lot 41, every eye on the screen. Owner: heist (QA
// pass, Nov 14). The director's side is TakeDirectorHeist.cpp (ATakeDirector::BeginHeist, StepHeist).
//
// THE TRICK (Marwan's QA note 6: "It doesn't make sense everyone sitting and I just walk up in front of everyone to
// the film and steal it unless a smart trick is employed").
//  - Who the crew are: the gala's own lighting and broadcast crew. The programme has a preview of Lot 41 before the
//    bidding, a glimpse of the lost film on the screen over the dais with the house lights down. Maya runs the
//    room's lights, so she calls the preview whenever she likes. Vex (props) built a dummy: the same can, the same
//    weight, a newsreel inside.
//  - The one new thing to do: after the keycard, Maya throws the HOUSE DIMMER at the west rail of her platform (E).
//    On that frame the projector on her platform lights the screen and the house lights start down; Vance
//    announces the glimpse, and every head in the hall turns to the screen (every hall watcher is blind while it
//    runs; the guests play their LookAt gesture).
//  - In the dark, Julian stands on the case mark (1.2 s) and swaps the reel for the dummy. A second later the house
//    comes up and the case is not empty.
//  - What trips the alarm: Vance, presenting Lot 41 as the lights come up, opens the can and finds a newsreel. That
//    is the old Case trigger, so the getaway is unchanged: the alarm, the shutters, the guards, the winch and the
//    pyro unlocked, the stage trap or the gold door.
//  - Without the trick: the case in full light in front of the whole hall. Half-way into the swap the room sees
//    him: Vance's line and the blooper THE ROOM SAW HIM.
//  - Taught by one goal line ("[2] MAYA: dim the HOUSE LIGHTS. In the dark, JULIAN swaps the reel at the case") and
//    one spoken line (Vex's, as the keycard lands). Cost to a take: Maya's few steps, the press, a cut back, and the
//    second before the house comes up: about 6 to 10 s.
//  - References: Uncharted 4's auction (the room held while the lights go, the cross taken in the dark); the pinch in
//    Ocean's Eleven (the room's own show is the cover).
//  - The switch: ATakeDirector::bHeistTrick. Off, or no dimmer in the level: the old flow (the case trips the alarm).
//
// The device: free (Uses 0), locked until the Keycard trigger, refused once the case is open and while a preview
// runs. Fired: lands on the same frame (StepDevices gives every hall watcher its first glance, and the stamp), then
// keeps every hall watcher looking at the screen (Blind for Duration, refreshed every GlanceRefreshSeconds) while
// the house is dark, so they look back within Duration of the house coming up. The house lights are found at fire
// time: every placed ALight with a movable or stationary local light inside HouseMin..HouseMax that is not attached
// to another actor (Julian's key light rides his camera boom and stays). Their intensity eases to DimLevel and back.

#pragma once

#include "CoreMinimal.h"
#include "Device.h"
#include "HouseDimmer.generated.h"

class ULightComponent;
class USpotLightComponent;
class UStaticMeshComponent;

UCLASS()
class REEL_API AHouseDimmer : public ADevice
{
	GENERATED_BODY()

public:
	AHouseDimmer();

	/** Super (the use ring), then places the projector and aims it at the screen, so the map shows the beam's line. */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Super (UsesLeft, the body, the ring), then the projector off and the screen panel made (hidden). */
	virtual void BeginPlay() override;

	/** Refused: the trick is off ("not in this cut"), the case is open ("the preview is over"), a preview is running
	 *  or the house is still coming up; then the base rules (locked until the keycard). */
	virtual bool CanFire(FString& OutWhyNot) const override;

	/** This frame: the house lights are gathered and drop to FirstDropLevel, the projector's beam and the screen
	 *  show, the body goes white, the guests turn to the screen. Lands this frame (MarkLanded). */
	virtual bool Fire() override;

	/** Dark: eases the house to DimLevel, flickers the projector, refreshes every hall watcher's glance; at the end
	 *  of the preview the beam goes out and the house eases back up over RaiseSeconds. */
	virtual void StepDevice(float DeltaSeconds) override;

	/** Every watcher within Range of the effect point and inside HouseMin..HouseMax, except the beam: a laser does
	 *  not watch films. */
	virtual void GatherTargets(const TArray<TObjectPtr<AWatcher>>& All, TArray<AWatcher*>& Out) const override;

	/** The actor transform applied to EffectOffset (the middle of the hall). */
	virtual FVector GetEffectPoint() const override;

	/** "HOUSE DIMMER  UP", "HOUSE DIMMER  DARK 7", "HOUSE DIMMER  RISING". */
	virtual FString GetPipsText() const override;

	/** The preview is on: the house is down (or going down) and every hall watcher is on the screen. */
	bool IsHouseDark() const { return bRunning && DarkSecondsLeft > 0.f; }

	/** Seconds of dark left (0 when the house is up or coming up). */
	float GetDarkSecondsLeft() const { return IsHouseDark() ? DarkSecondsLeft : 0.f; }

	/** The swap: the house comes up within Seconds (only ever sooner). 0: the beam goes out on this frame. */
	void EndPreviewIn(float Seconds);

	/** Maya holds the house while Julian is half way through the swap: dark for at least Seconds more (only ever
	 *  longer, only while dark). */
	void HoldPreviewFor(float Seconds);

	/** A blooper: the house lights back to where they were, the beam and the screen off, at once. */
	void RestoreHouseNow();

	// --- the preview -----------------------------------------------------------------------------------------------

	/** How long the house stays down when nobody swaps, s. Enough for a cut back to Julian and a walk to the case. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Dimmer", meta = (ClampMin = "1"))
	float PreviewSeconds = 12.f;

	/** The house lights' share of their own intensity while the preview runs (0 black, 1 untouched). */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer", meta = (ClampMin = "0", ClampMax = "1"))
	float DimLevel = 0.08f;

	/** The house drops to this share on the fire frame itself (the lever's clunk), then eases on to DimLevel. */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer", meta = (ClampMin = "0", ClampMax = "1"))
	float FirstDropLevel = 0.55f;

	/** From full to DimLevel, s. */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer", meta = (ClampMin = "0.05"))
	float DimSeconds = 0.8f;

	/** From DimLevel back to full when the preview ends, s. */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer", meta = (ClampMin = "0.05"))
	float RaiseSeconds = 1.f;

	/** While dark, every hall watcher's glance (Blind for Duration) is renewed this often, s. Keep it below Duration. */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer", meta = (ClampMin = "0.05"))
	float GlanceRefreshSeconds = 0.5f;

	/** Julian on the case mark with the house up: the room sees him once the swap is this far along (0-1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Dimmer", meta = (ClampMin = "0", ClampMax = "1"))
	float SeenAtCaseAlpha = 0.5f;

	/** After the swap the house comes up this soon (Maya sees it on her monitor), s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Dimmer", meta = (ClampMin = "0"))
	float LightsUpAfterSwapSeconds = 1.f;

	/** The guests in the hall play ACastMember::PlayGesture("LookAt") as the preview starts. */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer")
	bool bTurnHeads = true;

	/** The house: lights inside this world box are dimmed, and guests inside it turn their heads (the map sets it to
	 *  the great hall, floor to ceiling). */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer")
	FVector HouseMin = FVector(-1000.f, -2000.f, -100.f);

	UPROPERTY(EditAnywhere, Category = "Take|Dimmer")
	FVector HouseMax = FVector(4400.f, 2000.f, 1500.f);

	/** The hall's middle, local to the actor: Range is measured from here. A widget in the editor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Dimmer", meta = (MakeEditWidget = true))
	FVector EffectOffset = FVector(1200.f, 600.f, -450.f);

	// --- the projector and the screen ------------------------------------------------------------------------------

	/** The projector's lens, local to the actor (the map puts it on the platform's east rail). A widget. */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer", meta = (MakeEditWidget = true))
	FVector ProjectorOffset = FVector(280.f, 40.f, 160.f);

	/** The middle of the screen's face, local to the actor (the map: the dark screen on the hall's end wall). */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer", meta = (MakeEditWidget = true))
	FVector ScreenOffset = FVector(3900.f, 640.f, 150.f);

	/** The picture on the screen: across and tall, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer")
	FVector2D ScreenSize = FVector2D(1400.f, 600.f);

	/** The picture's yaw relative to the actor: 0 lays it across the actor's Y axis (the east wall, with the actor at
	 *  yaw 0). */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer")
	float ScreenYaw = 0.f;

	/** The picture's colour (a pale silver: the projector's light makes it glow). */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer")
	FLinearColor ScreenColour = FLinearColor(0.80f, 0.78f, 0.70f);

	/** The projector's light, candelas (a screen 36 m away). */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer", meta = (ClampMin = "0"))
	float ProjectorCandelas = 40000.f;

	/** The beam's half angle, degrees (the inner cone is 70% of it). */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer", meta = (ClampMin = "1", ClampMax = "40"))
	float ProjectorConeDegrees = 7.f;

	/** How much the beam shows in the hall's haze. */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer", meta = (ClampMin = "0"))
	float ProjectorScattering = 1.f;

	UPROPERTY(EditAnywhere, Category = "Take|Dimmer")
	FLinearColor ProjectorColour = FLinearColor(0.95f, 0.92f, 0.82f);

	/** The film's flicker: the beam's intensity dips by up to this share (never on the beat). */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer", meta = (ClampMin = "0", ClampMax = "1"))
	float FlickerAmount = 0.2f;

	// --- what the scene says (subtitles; listed in C:\Jam\Builds\qa\lines\heist.json for the voices) -------------

	/** As the keycard lands (the spoken half of the lesson). */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer|Lines")
	FString TeachLine = TEXT("VEX: \"Not with the whole room watching. Maya: house lights down, roll the preview. Every eye goes to the screen.\"");

	/** As the house goes down. */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer|Lines")
	FString PreviewLine = TEXT("VANCE: \"Ladies and gentlemen: a first glimpse of Lot 41.\"");

	/** The swap is done in the dark. */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer|Lines")
	FString SwapLine = TEXT("MAYA: \"Clean swap. Bringing the house up.\"");

	/** The house comes up and Vance opens the dummy (the alarm's own line follows). */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer|Lines")
	FString DiscoverLine = TEXT("VANCE: \"And here it is... a newsreel?!\"");

	/** The preview ended with no swap. */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer|Lines")
	FString NoSwapLine = TEXT("MAYA: \"House is up. Get him beside the case and I'll roll it again.\"");

	/** The room sees him at the case with the house up (the blooper follows). */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer|Lines")
	FString SeenLine = TEXT("VANCE: \"Sir! Step away from that case!\"");

	/** The crew hint after two bloopers at the keycard's checkpoint. */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer|Lines")
	FString HintLine = TEXT("MAYA: \"Not in the light. Get him beside the case, cut to me, I take the house down, then swap it.\"");

	/** The case mark's word over its progress bar while the trick is on. */
	UPROPERTY(EditAnywhere, Category = "Take|Dimmer|Lines")
	FString SwapVerb = TEXT("Swapping the reel for the dummy");

	// --- components ---------------------------------------------------------------------------------------------------

	/** The preview's beam: from the projector to the screen. Movable, no shadows, visible only while dark. */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<USpotLightComponent> ProjectorLight;

	/** The projector itself: a plain box for now (the model is package devices'). */
	UPROPERTY(VisibleAnywhere, Category = "Take")
	TObjectPtr<UStaticMeshComponent> ProjectorBody;

protected:
	/** Places ProjectorBody and ProjectorLight at ProjectorOffset, aimed at the screen; sets the light's numbers. */
	void PlaceProjector();

	/** Finds the house lights (see the file comment) and remembers each one's intensity now. */
	void GatherHouseLights();

	/** Every house light at its remembered intensity x Level (only when Level changed). */
	void ApplyHouseLevel(float Level);

	/** The beam and the picture on or off, this frame. */
	void SetPreviewVisible(bool bVisible);

	/** The beam's intensity for RunSeconds (the film's flicker). */
	void ApplyProjectorFlicker();

	/** Every hall watcher looks at the screen for Duration (ApplyEffect Blind). Returns how many it caught. */
	int32 Glance();

	/** The guests in the house turn to the screen (ACastMember::PlayGesture("LookAt")). */
	void TurnHeads();

	/** The dark ends: the beam and the picture go out, the house starts back up. Logs. */
	void EndDark();

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> ScreenPanel;

	TArray<TWeakObjectPtr<ULightComponent>> HouseLights;
	TArray<float> HouseBaseIntensity;

	float HouseLevel = 1.f;
	float AppliedLevel = 1.f;
	float DarkSecondsLeft = 0.f;
	float GlanceSecondsLeft = 0.f;
};
