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
}
