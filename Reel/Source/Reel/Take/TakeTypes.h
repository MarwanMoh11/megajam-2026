// Shared types for the take: enums, the press struct, colours, LogTake and TAKE_LOG.
// Owner: header pass (shared). Builders read this file; they do not change it.

#pragma once

#include "CoreMinimal.h"
#include "TakeTypes.generated.h"

class AActor;
class ADevice;
class AWatcher;

DECLARE_LOG_CATEGORY_EXTERN(LogTake, Log, All);

/** "bar.beat" of the running take's clock, e.g. "3.2" ("-.-" when no director is running). Defined in TakeDirector.cpp. */
FString TakeLogStamp();

/**
 * One line to LogTake, prefixed with the frame number (GFrameCounter) and the clock's bar.beat:
 *     LogTake: [f1234 3.2] CUT -> B
 * Format must be a TEXT("...") literal (it goes through FString::Printf, so the format is checked at compile time).
 * Every press handler and every visible reaction logs through this macro: the press and its reaction carry the
 * same f number, and that is how "same frame" gets verified. UE_LOG is compiled out of Shipping: that is fine.
 */
#define TAKE_LOG(Format, ...) \
	UE_LOG(LogTake, Log, TEXT("[f%llu %s] %s"), (unsigned long long)GFrameCounter, *TakeLogStamp(), *FString::Printf(Format, ##__VA_ARGS__))

/** The three cameras. A is the film (Julian), B is Maya's making-of camera, C is Vex's (no C in the test box).
 *  In Python it is unreal.TakeCameraId (unreal.TakeCamera is the camera actor class). */
UENUM(BlueprintType, meta = (ScriptName = "TakeCameraId"))
enum class ETakeCamera : uint8
{
	A,
	B,
	C
};

/** What a device does to every watcher in its range (rule 7). */
UENUM(BlueprintType)
enum class EDeviceEffect : uint8
{
	Blind,	// the frame goes dark for the duration
	Lure,	// the watcher walks to the device's point, looks at it, and walks back
	Mask	// a matte blocks frames and the beam (in step 1: switches the beam off; the guard ignores it)
};

/** The take's phase. Rolling: everything steps. BlooperFreeze: nothing steps for 1 s, then the level reloads.
 *  Cleared: Julian reached the goal mark; the world keeps running, no blooper can fire, R still starts a new take. */
UENUM(BlueprintType)
enum class ETakePhase : uint8
{
	Rolling,
	BlooperFreeze,
	Cleared,
	Title,		// step 3: the title card before the session's first take; nothing steps until Confirm or Use
	Printed		// step 3: Julian is out with the reel: PRINT IT, the stats and grade, the blooper reel; R: a new take
};

/** Why a blooper fired. None = no blooper (pending slot empty / no blooper yet this session). */
UENUM(BlueprintType)
enum class EBlooperReason : uint8
{
	None,
	ComposureZero,
	BeamTouched,
	ClockRanOut,	// step 3: the finale's 8 bars ran out (rule 9)
	SeenAtCase		// --- QA heist: he took the reel with the house lights up, in front of the whole hall
};

/** The cut the take earns (rule 11, the grade). */
UENUM(BlueprintType)
enum class ETakeGrade : uint8
{
	RoughCut,
	FinalCut,
	DirectorsCut
};

/** What a clickable actor is, for the press routing in rules 3 and 4. */
/** Step 6 (Marwan, Nov 15: the red sight lines are too loud): when the watchers' sight frames are drawn. The sight
 *  tests never change; the beam's laser is a thing in the world and always shows. */
UENUM(BlueprintType)
enum class ETakeFrameDisplay : uint8
{
	Never,		// no frame at all
	Planning,	// hidden at rest; faint while a crew member plans a device on it; solid red while it sees Julian
	Always		// step 4's frames, always on
};

UENUM(BlueprintType)
enum class ETakeClickKind : uint8
{
	None,
	Mark,
	Device,
	Watcher
};

/** One queued press. */
UENUM(BlueprintType)
enum class ETakePressType : uint8
{
	Cut,		// keys 1/2/3 or TakeCut: switch to Julian / Maya / Vex (a flight)
	Use,		// E, left mouse button or TakeUse: fire the device in reach (the director's CurrentUse)
	NewTake,	// R or TakeNewTake
	Confirm		// step 3: Enter, Space or TakeConfirm: starts the take from the title card, turns the end sequence's page
};

/** A watcher's state, shown on its tape label on B and C ("GUARD - LURED"). */
UENUM(BlueprintType)
enum class EWatcherState : uint8
{
	Watching,	// frame live
	Off,		// not live yet (the beam before its first live bar)
	Dropped,	// the beam's one-beat drop
	Blinded,
	Lured,
	Masked
};

/** The click-track pitch that warns one bar before a watcher turns, passes or speaks (rule 2; the band's
 *  instruments replace these in the audio step). None = no warning. */
UENUM(BlueprintType)
enum class EWatcherInstrument : uint8
{
	None,
	Bass,		// the guard: a low pluck
	Trumpet,	// the drone: two mid notes
	DrumRoll,	// Vance's speech: a short roll
	HiHat		// the beam (its drop already splashes on the drop beat itself)
};

/** Trigger names the C++ scene script knows (step 2). The map's ASceneTrigger boxes and marks fire them by name;
 *  watchers arrive on them (AWatcher::ArriveOn). Make an FName at the point of use: FName(TakeTriggers::Scene2). */
namespace TakeTriggers
{
	inline const TCHAR* const Door = TEXT("Door");			// Julian is through the scanner and inside the front doors
	inline const TCHAR* const Scene2 = TEXT("Scene2");		// he is on the hall floor: Scene 2 starts
	inline const TCHAR* const PastRope = TEXT("PastRope");	// he is past the rope, on the dais
	inline const TCHAR* const Keycard = TEXT("Keycard");	// he lifted Vance's keycard: Scene 3 starts
	// Step 3, Scene 3 (The Getaway).
	inline const TCHAR* const Case = TEXT("Case");			// he opened the case and has the reel: the alarm
	inline const TCHAR* const Winch = TEXT("Winch");		// Vex's winch landed: the curtain is down, the stage trap is open
	inline const TCHAR* const Bow = TEXT("Bow");			// the deleted scene The Bow (also its Id in DeletedScenes)
	inline const TCHAR* const ExitTrap = TEXT("ExitTrap");	// out through the stage trap: PRINT IT
	inline const TCHAR* const ExitSide = TEXT("ExitSide");	// out through the gold side exit: PRINT IT
}

/**
 * One press, queued by ATakePlayerController during input handling (TG_PrePhysics) and consumed by
 * ATakeDirector::StepPresses in the same frame. Exec test commands go through the same queue.
 */
USTRUCT(BlueprintType)
struct FTakePress
{
	GENERATED_BODY()

	UPROPERTY()
	ETakePressType Type = ETakePressType::Cut;

	/** Cut: the character (= camera id) to switch to. */
	UPROPERTY()
	ETakeCamera Camera = ETakeCamera::A;

	/** "key", "mouse" or "exec": for the log. */
	UPROPERTY()
	FString Source;

	/** GFrameCounter when queued (for the log). */
	uint64 Frame = 0;
};

/**
 * What a Use press would do right now for one character: ATakeDirector::FindUseTarget fills it, the director keeps
 * this frame's result in CurrentUse (the HUD's prompt line, the device ring and watcher highlights, the Use press).
 * All empty (Device null, Prompt empty, bCanFire false) = nothing in reach.
 */
USTRUCT(BlueprintType)
struct FTakeUseTarget
{
	GENERATED_BODY()

	/** The device in reach (null = none). */
	UPROPERTY()
	TObjectPtr<ADevice> Device = nullptr;

	/** The follow spot only: the watcher the aim assist picked (null = none picked). */
	UPROPERTY()
	TObjectPtr<AWatcher> Watcher = nullptr;

	/** The HUD's prompt line: "[E] Release the cart", "CART - used", "Look at a watcher". Empty = draw nothing. */
	UPROPERTY()
	FString Prompt;

	/** A Use press now would fire (false: the press flashes Prompt instead). */
	UPROPERTY()
	bool bCanFire = false;
};

/** Shared colours (linear). Used for BasicShapeMaterial's "Color" parameter and for the HUD. */
/** Where the HUD shows a flash (docs/ART_STYLE.md 15.5): T2 with no glyph, T2 with the danger square, T2 with Julian's
 *  T, B1 (a refused press, bone), the subtitle row ("SPEAKER: \"line\""), a julian stamp ("CAPTION|TEXT"), x2 / x3. */
enum class ETakeFlashKind : uint8
{
	Note,
	Danger,
	Julian,
	Refusal,
	Line,
	Stamp,
	Count
};

namespace TakeColors
{
	inline const FLinearColor Gold(1.0f, 0.62f, 0.08f);
	inline const FLinearColor GoldBright(1.0f, 0.85f, 0.35f);
	inline const FLinearColor Red(0.85f, 0.03f, 0.02f);
	inline const FLinearColor RedDark(0.12f, 0.01f, 0.01f);	// a blinded / masked frame
	inline const FLinearColor WorkBlue(0.25f, 0.45f, 0.8f);
	inline const FLinearColor White(0.95f, 0.95f, 0.95f);
	inline const FLinearColor Grey(0.35f, 0.35f, 0.38f);
	inline const FLinearColor DarkGrey(0.08f, 0.08f, 0.09f);
	inline const FLinearColor Black(0.0f, 0.0f, 0.0f);
}

/** Short names for the HUD and the log. */
namespace TakeText
{
	inline const TCHAR* CameraLetter(ETakeCamera C)
	{
		switch (C)
		{
		case ETakeCamera::A: return TEXT("A");
		case ETakeCamera::B: return TEXT("B");
		default: return TEXT("C");
		}
	}

	/** The key that cuts to this camera: "1", "2", "3". */
	inline const TCHAR* CameraKey(ETakeCamera C)
	{
		switch (C)
		{
		case ETakeCamera::A: return TEXT("1");
		case ETakeCamera::B: return TEXT("2");
		default: return TEXT("3");
		}
	}

	/** Whose camera it is: "JULIAN", "MAYA", "VEX". */
	inline const TCHAR* CrewName(ETakeCamera C)
	{
		switch (C)
		{
		case ETakeCamera::A: return TEXT("JULIAN");
		case ETakeCamera::B: return TEXT("MAYA");
		default: return TEXT("VEX");
		}
	}

	inline const TCHAR* EffectName(EDeviceEffect E)
	{
		switch (E)
		{
		case EDeviceEffect::Blind: return TEXT("BLIND");
		case EDeviceEffect::Lure: return TEXT("LURE");
		default: return TEXT("MASK");
		}
	}

	inline const TCHAR* WatcherStateName(EWatcherState S)
	{
		switch (S)
		{
		case EWatcherState::Watching: return TEXT("WATCHING");
		case EWatcherState::Off: return TEXT("OFF");
		case EWatcherState::Dropped: return TEXT("DROPPED");
		case EWatcherState::Blinded: return TEXT("BLINDED");
		case EWatcherState::Lured: return TEXT("LURED");
		default: return TEXT("MASKED");
		}
	}

	inline const TCHAR* InstrumentName(EWatcherInstrument I)
	{
		switch (I)
		{
		case EWatcherInstrument::Bass: return TEXT("BASS");
		case EWatcherInstrument::Trumpet: return TEXT("TRUMPET");
		case EWatcherInstrument::DrumRoll: return TEXT("DRUM ROLL");
		case EWatcherInstrument::HiHat: return TEXT("HI-HAT");
		default: return TEXT("NONE");
		}
	}

	/** The blooper's reason as the slate and the reel say it (BlooperReasonName stays the log's tag). */
	inline const TCHAR* BlooperReasonLine(EBlooperReason R)
	{
		switch (R)
		{
		case EBlooperReason::ComposureZero: return TEXT("Composure ran out");
		case EBlooperReason::BeamTouched: return TEXT("Walked into the beam");
		case EBlooperReason::ClockRanOut: return TEXT("The clock ran out");
		case EBlooperReason::SeenAtCase: return TEXT("The whole room saw him");	// --- QA heist (W3 heist2: the front, the keycard or the case)
		default: return TEXT("");
		}
	}

	inline const TCHAR* BlooperReasonName(EBlooperReason R)
	{
		switch (R)
		{
		case EBlooperReason::ComposureZero: return TEXT("COMPOSURE ZERO");
		case EBlooperReason::BeamTouched: return TEXT("BEAM TOUCHED");
		case EBlooperReason::ClockRanOut: return TEXT("THE CLOCK RAN OUT");
		case EBlooperReason::SeenAtCase: return TEXT("THE ROOM SAW HIM");	// --- QA heist
		default: return TEXT("NONE");
		}
	}

	inline const TCHAR* PhaseName(ETakePhase P)
	{
		switch (P)
		{
		case ETakePhase::Rolling: return TEXT("ROLLING");
		case ETakePhase::BlooperFreeze: return TEXT("BLOOPER_FREEZE");
		case ETakePhase::Title: return TEXT("TITLE");
		case ETakePhase::Printed: return TEXT("PRINTED");
		default: return TEXT("CLEARED");
		}
	}

	inline const TCHAR* GradeName(ETakeGrade G)
	{
		switch (G)
		{
		case ETakeGrade::DirectorsCut: return TEXT("DIRECTOR'S CUT");
		case ETakeGrade::FinalCut: return TEXT("FINAL CUT");
		default: return TEXT("ROUGH CUT");
		}
	}
}
