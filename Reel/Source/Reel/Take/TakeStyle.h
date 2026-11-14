// Step 4 (the look, pass 1): every colour and size token of the film look in one place (docs/ART_STYLE.md §15.2).
// The film grade's numbers live in the material (tools/build_look.py, /Game/Take/Look/MI_FilmGrade); the overlay's
// tunables live on ATakeHUD's Film struct (TakeFilmOverlay.h). Pass 2 extends this header for the HUD's type.

#pragma once

#include "CoreMinimal.h"

namespace TakeStyle
{
	/** A §15.2 hex token as Canvas wants it (FLinearColor from sRGB; Canvas converts back to the same hex). */
	inline FLinearColor Hex(uint8 R, uint8 G, uint8 B, float Alpha = 1.f)
	{
		FLinearColor C = FLinearColor::FromSRGBColor(FColor(R, G, B));
		C.A = Alpha;
		return C;
	}

	// --- colour tokens (§15.2) ----------------------------------------------------------------------------------------
	inline const FLinearColor Ink = Hex(0x0D, 0x0C, 0x0B);
	inline const FLinearColor Plate = Hex(0x1A, 0x18, 0x15);
	inline const FLinearColor PlateMo = Hex(0x0F, 0x1A, 0x24);
	inline const FLinearColor Silver = Hex(0x7A, 0x75, 0x6D);
	inline const FLinearColor Ash = Hex(0xA3, 0x9D, 0x92);
	inline const FLinearColor Bone = Hex(0xC9, 0xC2, 0xB4);
	inline const FLinearColor Paper = Hex(0xF1, 0xEA, 0xDB);
	inline const FLinearColor Danger = Hex(0xD3, 0x30, 0x1F);
	inline const FLinearColor Julian = Hex(0xE9, 0xB9, 0x49);
	inline const FLinearColor MoShadow = Hex(0x0B, 0x17, 0x24);
	inline const FLinearColor MoMid = Hex(0x56, 0x73, 0x8A);
	inline const FLinearColor MoHigh = Hex(0xDF, 0xF4, 0xF5);

	// --- the custom-depth stencil (r.CustomDepth=3): what keeps its colour in the grade ----------------------------
	/** Danger: watchers' frames and frame lines, tally lights, red tape (the carpet too), the scanner's live bar and
	 *  eye, the beam, the drone's circle. */
	constexpr uint8 StencilRed = 1;
	/** Julian: his marks, the scanner's open bar, the reel's case, the gold side exit, the trap's mark. */
	constexpr uint8 StencilGold = 2;
	/** The alarm's white frames (§15.3): kept, with no colour calibration. */
	constexpr uint8 StencilWhite = 3;

	// --- sizes (§15.2, px at 1080 tall) -------------------------------------------------------------------------------
	constexpr float LetterboxAspect = 2.39f;
	constexpr float FilmFps = 24.f;
	constexpr float LineHair = 1.f;
	constexpr float LineThin = 2.f;

	/** §15.2's scale: s = ScreenHeight / 1080, clamped 0.8 to 2.0. */
	inline float Scale(float ScreenH) { return FMath::Clamp(ScreenH / 1080.f, 0.8f, 2.f); }

	/** The letterbox bar height for a screen (picture = width / Aspect, centred; never negative). 1920x1080: 138. */
	inline float LetterboxBar(float ScreenW, float ScreenH, float Aspect = LetterboxAspect)
	{
		return FMath::Max(0.f, FMath::RoundToFloat((ScreenH - ScreenW / FMath::Max(1.f, Aspect)) * 0.5f));
	}

	// =================================================================================================================
	// Pass 2: the HUD's tokens (§15.2 type, space, lines, motion; §15.5 layout; §15.6 stamps and cards). Every number
	// the HUD draws with comes from here; px are at 1080 tall and multiplied by Scale() (TakeHudKit rounds them).
	// =================================================================================================================

	constexpr float LineRule = 3.f;
	constexpr float LineHeavy = 6.f;

	// --- type (§15.2) --------------------------------------------------------------------------------------------------
	enum class EFace : uint8 { Limelight, FellRoman, FellItalic, JosefinSemiBold, JosefinBold, Count };

	enum class EType : uint8 { Display, Stamp, Title, Line, Body, Goal, Label, Tag, Key, Count };

	struct FTypeToken
	{
		EFace Face;
		float Px;		// em size at 1080
		float TrackEm;	// letter spacing, em (Slate's LetterSpacing / 1000)
		bool bUpper;
		float CapEm;	// cap height, em: rows centre on the caps, not the line box
	};

	inline const FTypeToken& Type(EType T)
	{
		static const FTypeToken Table[static_cast<int32>(EType::Count)] = {
			{ EFace::Limelight, 128.f, 0.04f, true, 0.70f },		// Display: PRINT IT, the title card's name
			{ EFace::Limelight, 72.f, 0.06f, true, 0.70f },			// Stamp: CLEARED, grades, x2, BLOOPER
			{ EFace::Limelight, 44.f, 0.08f, true, 0.70f },			// Title: scene titles' heads, card heads, figures
			{ EFace::FellItalic, 34.f, 0.f, false, 0.62f },			// Line: subtitles, hints, taglines
			{ EFace::FellRoman, 30.f, 0.01f, false, 0.62f },		// Body: pitch, stats words, reel rows, credits
			{ EFace::JosefinSemiBold, 26.f, 0.02f, false, 0.70f },	// Goal: the goal line, the Use prompt
			{ EFace::JosefinBold, 20.f, 0.15f, true, 0.70f },		// Label: slug, tabs, clock, composure, captions
			{ EFace::JosefinBold, 16.f, 0.12f, true, 0.70f },		// Tag: world labels, controls row, field names
			{ EFace::JosefinBold, 16.f, 0.08f, true, 0.70f },		// Key: the letter inside a keycap
		};
		return Table[static_cast<int32>(T)];
	}

	/** The imported font faces (tools/build_fonts.py), cooked with DirectoriesToAlwaysCook /Game/Take/Fonts. */
	inline const TCHAR* FacePath(EFace F)
	{
		switch (F)
		{
		case EFace::Limelight: return TEXT("/Game/Take/Fonts/FF_Limelight.FF_Limelight");
		case EFace::FellRoman: return TEXT("/Game/Take/Fonts/FF_FellRoman.FF_FellRoman");
		case EFace::FellItalic: return TEXT("/Game/Take/Fonts/FF_FellItalic.FF_FellItalic");
		case EFace::JosefinSemiBold: return TEXT("/Game/Take/Fonts/FF_JosefinSemiBold.FF_JosefinSemiBold");
		default: return TEXT("/Game/Take/Fonts/FF_JosefinBold.FF_JosefinBold");
		}
	}

	// --- space (§15.2) --------------------------------------------------------------------------------------------------
	constexpr float Unit = 8.f;
	constexpr float Margin = 48.f;		// screen margin left and right
	constexpr float CentreHalf = 352.f;	// the bars' centre column: 608-1312 at 1920

	// --- the bars' rows, as offsets from the bars' edges (§15.2: rows keep their offsets on any aspect) ------------
	constexpr float RowT1 = 40.f;		// from the screen's top
	constexpr float RowT2Up = 42.f;		// T2 (96) above the top bar's bottom edge (138)
	constexpr float GoalLineGap = 32.f;	// two goal lines: centres T2 - 16 and T2 + 16
	constexpr float RowB1Down = 36.f;	// B1 (978) below the bottom bar's top edge (942)
	constexpr float RowB2Down = 76.f;	// B2 (1018)
	constexpr float RowB3Down = 112.f;	// B3 (1054)
	constexpr float BarRowsMinBar = 120.f;	// below this bar height (ultra-wide) the rows sit over the picture

	// --- the picture's anchors (§15.5, §15.6) ---------------------------------------------------------------------------
	constexpr float SubtitleUp = 64.f;		// subtitle centre above the picture's bottom edge (878)
	constexpr float StampY = 162.f;			// stamps and scene titles: centre below the picture's top edge (300)
	constexpr float StampSideX = 520.f;		// a second stamp at the same time: this far right of centre (1480)
	constexpr float CountX = 520.f;			// x2 / x3: right of centre (1480) ...
	constexpr float CountUp = 182.f;		// ... and above the picture's bottom edge (760)

	// --- glyphs (§15.5) ---------------------------------------------------------------------------------------------------
	constexpr float GlyphSquare = 14.f;
	constexpr float GlyphTBarW = 16.f;
	constexpr float GlyphTBarH = 4.f;
	constexpr float GlyphTStemW = 4.f;
	constexpr float GlyphTStemH = 12.f;
	constexpr float PipW = 12.f;
	constexpr float PipH = 4.f;
	constexpr float PipGap = 4.f;
	constexpr float PipUnder = 8.f;			// pips under a key tab's name
	constexpr float GlyphTextGap = 8.f;		// a glyph and its word
	constexpr float PulseLow = 0.55f;		// a pulsing glyph's alpha 10 frames after the beat
	constexpr float PulseFrames = 10.f;

	// --- keycaps (§15.2) ----------------------------------------------------------------------------------------------------
	constexpr float KeyH = 28.f;
	constexpr float KeyHSmall = 24.f;		// in the controls row
	constexpr float KeyMinW = 28.f;
	constexpr float KeyPad = 8.f;
	constexpr float KeyTextGap = 8.f;		// a keycap and the word after it

	// --- T1 ---------------------------------------------------------------------------------------------------------------
	constexpr float SlateFieldGap = 24.f;
	constexpr float SlateCaptionGap = 8.f;	// caption to its value
	constexpr float TabGap = 32.f;
	constexpr float TabShakePx = 4.f;
	constexpr float TabFlightLine = 2.f;
	constexpr float TabFlightUnder = 12.f;	// the flight line's top below the name's centre
	constexpr float ClockSquare = 10.f;
	constexpr float ClockSquareGap = 6.f;
	constexpr float FinaleSquare = 12.f;
	constexpr float FinaleSquareGap = 6.f;
	constexpr float ClockPartGap = 12.f;

	// --- T2, B1, B2 ------------------------------------------------------------------------------------------------------
	constexpr float GoalMaxW = 1400.f;
	constexpr float GoalRule = 2.f;
	constexpr float GoalRuleUnder = 20.f;	// the new-goal rule below the last line's centre
	constexpr float UseMaxW = 1200.f;
	constexpr float ActionBarW = 240.f;
	constexpr float ActionBarH = 4.f;
	constexpr float ActionBarUnder = 12.f;
	constexpr float ComposureW = 800.f;
	constexpr float ComposureLabelGap = 24.f;	// "COMPOSURE" ends 24 px left of the line (536)
	constexpr float RefillMinJump = 5.f;		// a one-frame gain this big shows in paper (x2's refill, not MARK's trickle)

	// --- subtitles (§15.5) -------------------------------------------------------------------------------------------------
	constexpr float SubtitleMaxW = 1200.f;
	constexpr float SubtitlePadX = 16.f;
	constexpr float SubtitlePadY = 8.f;
	constexpr float SubtitleSpeakerGap = 12.f;
	constexpr float PlateAlpha = 0.85f;
	constexpr float LineSpacing = 1.25f;	// line pitch for wrapped text, em

	// --- the reticle (§15.4) ---------------------------------------------------------------------------------------------
	constexpr float ReticleArm = 12.f;
	constexpr float ReticleGap = 6.f;
	constexpr float ReticleAlpha = 0.8f;

	// --- tape labels and countdown tags (§15.5) ----------------------------------------------------------------------------
	constexpr float TapeH = 24.f;
	constexpr float TapePad = 8.f;
	constexpr float TapeOutline = 2.f;
	constexpr float TapeGlyphBorder = 2.f;
	constexpr float LabelGap = 4.f;
	constexpr float LabelBarClear = 16.f;
	constexpr float ReticleBoxW = 320.f;
	constexpr float ReticleBoxH = 160.f;

	// --- stamps and cards (§15.6) ------------------------------------------------------------------------------------------
	constexpr float StampFieldAlpha = 0.75f;
	constexpr float StampInnerIn = 10.f;
	constexpr float StampPadX = 24.f;
	constexpr float StampPadY = 12.f;
	constexpr float StampTilt = -4.f;
	constexpr float GradeTilt = -6.f;
	constexpr float CountTilt = -8.f;
	constexpr float CaptionGap = 8.f;		// a caption above a stamp or a title
	constexpr float CardInset = 24.f;		// card outer border inside the picture's edges
	constexpr float CardInnerIn = 8.f;
	constexpr float CardDiamond = 10.f;
	constexpr float CardPad = 64.f;
	constexpr float SceneRuleW = 320.f;
	constexpr float SceneRuleGap = 18.f;

	// --- the slate (§15.6) -------------------------------------------------------------------------------------------------
	constexpr float StickH = 28.f;
	constexpr float StripePeriod = 56.f;	// the stripe tile: one stripe in the meaning colour, one ink, 45 degrees
	constexpr float StickOpenDeg = -14.f;
	constexpr float StickCloseFrames = 6.f;
	constexpr float BoardBorder = 3.f;
	constexpr float CellIn = 8.f;
	constexpr float OpeningX = 96.f;		// from the picture's left edge
	constexpr float OpeningUp = 32.f;		// bottom above the picture's bottom edge (910)
	constexpr float OpeningW = 560.f;
	constexpr float OpeningH = 236.f;
	constexpr float BlooperW = 880.f;
	constexpr float BlooperH = 400.f;
	constexpr float PrintW = 1040.f;
	constexpr float PrintH = 480.f;
	constexpr float PrintLineUnder = 40.f;	// the how-he-left line below the board

	// --- the end pages (§15.6; columns moved left so the grade stamp never covers a value, see PLAN.md) -----------------
	constexpr float StatsCaptionRight = -320.f;	// from the screen's centre: x 640 at 1920
	constexpr float StatsValueX = -288.f;		// x 672
	constexpr float StatsMaxValueW = 420.f;
	constexpr float StatsRowPitch = 60.f;
	constexpr float GradeX = 520.f;				// right of centre: x 1480
	constexpr float GradeDY = 20.f;				// below the picture's centre: y 560

	// --- motion (§15.2), in film frames at 24 fps ---------------------------------------------------------------------------
	constexpr float FramesSnap = 4.f;
	constexpr float FramesSettle = 6.f;
	constexpr float FramesBeat = 12.f;
	constexpr float FramesHold = 36.f;
	constexpr float FramesHoldTitle = 72.f;
	constexpr float FramesBar = 48.f;

	inline int32 FilmFrame(double Seconds) { return static_cast<int32>(FMath::FloorToDouble(Seconds * FilmFps)); }
	inline float Frames(float N) { return N / FilmFps; }
}
