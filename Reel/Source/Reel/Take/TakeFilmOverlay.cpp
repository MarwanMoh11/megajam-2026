// Step 4 (the look, pass 1): the film overlay. See TakeFilmOverlay.h and docs/ART_STYLE.md §15.4.

#include "TakeFilmOverlay.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "GameFramework/HUD.h"
#include "TakeStyle.h"
#include "TextureResource.h"

namespace TakeFilmLocal
{
	constexpr float Fps = TakeStyle::FilmFps;

	/** A transient texture filled by Fill(x, y) -> FColor (BGRA8, sRGB). */
	template <typename FillFn>
	UTexture2D* MakeTexture(int32 W, int32 H, bool bWrap, bool bNearest, FillFn Fill)
	{
		UTexture2D* Tex = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
		if (!Tex || !Tex->GetPlatformData() || Tex->GetPlatformData()->Mips.Num() == 0)
		{
			return nullptr;
		}
		Tex->SRGB = true;
		Tex->Filter = bNearest ? TF_Nearest : TF_Bilinear;
		Tex->AddressX = bWrap ? TA_Wrap : TA_Clamp;
		Tex->AddressY = bWrap ? TA_Wrap : TA_Clamp;
		FTexture2DMipMap& Mip = Tex->GetPlatformData()->Mips[0];
		FColor* Pixels = static_cast<FColor*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
		for (int32 Y = 0; Y < H; ++Y)
		{
			for (int32 X = 0; X < W; ++X)
			{
				Pixels[Y * W + X] = Fill(X, Y);
			}
		}
		Mip.BulkData.Unlock();
		Tex->UpdateResource();
		return Tex;
	}

	void PaintTile(UCanvas& Canvas, UTexture2D* Tex, float X, float Y, float W, float H, FVector2D UV0, FVector2D UV1,
		const FLinearColor& Colour)
	{
		if (!Tex || !Tex->GetResource() || W <= 0.f || H <= 0.f || Colour.A <= 0.f)
		{
			return;
		}
		FCanvasTileItem Item(FVector2D(X, Y), Tex->GetResource(), FVector2D(W, H), UV0, UV1, Colour);
		Item.BlendMode = SE_BLEND_Translucent;
		Canvas.DrawItem(Item);
	}

	void PaintRect(UCanvas& Canvas, const FLinearColor& Colour, float X, float Y, float W, float H)
	{
		if (W <= 0.f || H <= 0.f || Colour.A <= 0.f)
		{
			return;
		}
		FCanvasTileItem Item(FVector2D(X, Y), FVector2D(W, H), Colour);
		Item.BlendMode = SE_BLEND_Translucent;
		Canvas.DrawItem(Item);
	}

	FLinearColor WithAlpha(FLinearColor C, float A)
	{
		C.A = FMath::Clamp(A, 0.f, 1.f);
		return C;
	}
}

using namespace TakeFilmLocal;

float FTakeFilmOverlay::BarHeight(const FTakeFilmFrame& F) const
{
	return DrawsLetterbox(F) ? TakeStyle::LetterboxBar(F.ScreenW, F.ScreenH, LetterboxAspect) : 0.f;
}

void FTakeFilmOverlay::EnsureTextures()
{
	if (!GrainTex)
	{
		FRandomStream Noise(1924);
		GrainTex = MakeTexture(256, 256, true, true, [&Noise](int32, int32)
		{
			// Grey centred on 0.5 (a little wider than uniform so single specks read on a still frame).
			const float V = FMath::Clamp(0.5f + (Noise.FRand() + Noise.FRand() - 1.f) * 0.5f, 0.f, 1.f);
			const uint8 G = static_cast<uint8>(V * 255.f);
			return FColor(G, G, G, 255);
		});
	}
	if (!RampVTex)
	{
		// Solid at v = 0 (the edge), clear at v = 1 (inside): a falloff that stays strong near the edge.
		RampVTex = MakeTexture(1, 64, false, false, [](int32, int32 Y)
		{
			const float T = Y / 63.f;
			return FColor(255, 255, 255, static_cast<uint8>(FMath::Square(1.f - T) * 255.f));
		});
	}
	if (!RampHTex)
	{
		RampHTex = MakeTexture(64, 1, false, false, [](int32 X, int32)
		{
			const float T = X / 63.f;
			return FColor(255, 255, 255, static_cast<uint8>(FMath::Square(1.f - T) * 255.f));
		});
	}
	if (!BurnTex)
	{
		// The film burn: a paper core, a danger ring, clear outside.
		const FLinearColor PaperL = TakeStyle::Paper;
		const FLinearColor DangerL = TakeStyle::Danger;
		BurnTex = MakeTexture(128, 128, false, false, [PaperL, DangerL](int32 X, int32 Y)
		{
			const float DX = (X + 0.5f) / 64.f - 1.f;
			const float DY = (Y + 0.5f) / 64.f - 1.f;
			const float R = FMath::Sqrt(DX * DX + DY * DY);
			const float Ring = FMath::Clamp((R - 0.35f) / 0.35f, 0.f, 1.f);
			const FLinearColor C = FMath::Lerp(PaperL, DangerL, Ring);
			const float A = R < 0.7f ? 1.f : FMath::Clamp(1.f - (R - 0.7f) / 0.3f, 0.f, 1.f);
			FColor Out = C.ToFColor(true);
			Out.A = static_cast<uint8>(A * 255.f);
			return Out;
		});
	}
	if (!ScanTex)
	{
		ScanTex = MakeTexture(1, 4, true, true, [](int32, int32 Y)
		{
			return FColor(0, 0, 0, Y == 0 ? 255 : 0);
		});
	}
	if (!VignetteTex)
	{
		// QA hud: white, alpha 0 inside VignetteClear of the half-size, rising on a smoothstep to 1 at r = 1, so the
		// whole border (edge midpoints r = 1, corners more) is opaque and the fill outside it meets it without a seam.
		const float Clear = FMath::Clamp(VignetteClear, 0.f, 0.95f);
		VignetteTex = MakeTexture(256, 256, false, false, [Clear](int32 X, int32 Y)
		{
			const float DX = (X + 0.5f) / 128.f - 1.f;
			const float DY = (Y + 0.5f) / 128.f - 1.f;
			const float R = FMath::Sqrt(DX * DX + DY * DY);
			const float T = FMath::Clamp((R - Clear) / (1.f - Clear), 0.f, 1.f);
			const float A = T * T * (3.f - 2.f * T);
			return FColor(255, 255, 255, static_cast<uint8>(FMath::RoundToInt(A * 255.f)));
		});
	}
}

void FTakeFilmOverlay::Track(const FTakeFilmFrame& F)
{
	// A cut press: the look camera changes while a flight starts (a snap, a blooper's restore, gets no splice).
	if (!bHaveLook)
	{
		bHaveLook = true;
		LastLook = F.LookCamera;
	}
	else if (F.LookCamera != LastLook)
	{
		LastLook = F.LookCamera;
		if (F.bInFlight)
		{
			CutTime = F.RealTime;
			bCutToFilm = F.bFilmPicture;
		}
	}
	if (F.Phase != LastPhase)
	{
		LastPhase = F.Phase;
		PhaseTime = F.RealTime;
	}
}

void FTakeFilmOverlay::Roll(const FTakeFilmFrame& F, float DustRate, double ScratchMin, double ScratchMax)
{
	const int64 Frame = static_cast<int64>(FMath::FloorToDouble(F.RealTime * Fps));
	if (Frame == FilmFrame)
	{
		return;
	}
	FilmFrame = Frame;
	FRandomStream Rng(static_cast<int32>(Frame * 7919 + 17));
	GrainOffset = FVector2D(Rng.FRand(), Rng.FRand());
	FlickerRoll = Rng.FRand();
	bDustThisFrame = Rng.FRand() < DustRate / Fps;
	DustPos = FVector2D(Rng.FRand(), Rng.FRand());
	DustSize = Rng.FRandRange(2.f, 5.f);
	bDustLight = Rng.FRand() < 0.5f;

	if (NextScratchTime < 0.0 || NextScratchTime > F.RealTime + ScratchMax)
	{
		NextScratchTime = F.RealTime + Rng.FRandRange(ScratchMin, ScratchMax);
	}
	if (F.RealTime >= NextScratchTime)
	{
		ScratchUntil = F.RealTime + 2.0 / Fps;
		ScratchX = Rng.FRandRange(0.08f, 0.92f);
		NextScratchTime = F.RealTime + Rng.FRandRange(ScratchMin, ScratchMax);
	}
}

void FTakeFilmOverlay::DrawFilm(AHUD& Hud, UCanvas& Canvas, const FTakeFilmFrame& F, float X, float Y, float W,
	float H, float Grain, float Flicker)
{
	const float S = TakeStyle::Scale(F.ScreenH);
	if (bGrain && GrainTex)
	{
		// 1:1 in pixels: the tile repeats every 256 px; the offset jumps every film frame.
		const float Alpha = FMath::Min(Grain, GrainMax);
		const FVector2D UV0 = GrainOffset;
		const FVector2D UV1 = GrainOffset + FVector2D(W / 256.f, H / 256.f);
		PaintTile(Canvas, GrainTex, X, Y, W, H, UV0, UV1, FLinearColor(1.f, 1.f, 1.f, Alpha));
	}
	if (bFlicker)
	{
		PaintRect(Canvas, WithAlpha(TakeStyle::Ink, FlickerRoll * Flicker), X, Y, W, H);
	}
	if (bDust && bDustThisFrame)
	{
		const float Size = FMath::Max(1.f, FMath::RoundToFloat(DustSize * S));
		const FLinearColor Colour = bDustLight ? WithAlpha(TakeStyle::Paper, 0.5f) : WithAlpha(TakeStyle::Ink, 0.7f);
		PaintRect(Canvas, Colour, X + DustPos.X * (W - Size), Y + DustPos.Y * (H - Size), Size, Size);
	}
	if (bScratch && F.RealTime < ScratchUntil)
	{
		// One px, full picture height; it drifts 4 px over its two frames.
		const float Drift = static_cast<float>((ScratchUntil - F.RealTime) * Fps) * 2.f * S;
		PaintRect(Canvas, WithAlpha(TakeStyle::Paper, ScratchOpacity), X + ScratchX * W + Drift, Y, FMath::Max(1.f, S), H);
	}
}

void FTakeFilmOverlay::DrawEdgeBands(UCanvas& Canvas, const FLinearColor& Colour, float X, float Y, float W, float H,
	float Depth)
{
	if (Depth <= 0.f || Colour.A <= 0.f)
	{
		return;
	}
	PaintTile(Canvas, RampVTex, X, Y, W, Depth, FVector2D(0.f, 0.f), FVector2D(1.f, 1.f), Colour);
	PaintTile(Canvas, RampVTex, X, Y + H - Depth, W, Depth, FVector2D(0.f, 1.f), FVector2D(1.f, 0.f), Colour);
	PaintTile(Canvas, RampHTex, X, Y, Depth, H, FVector2D(0.f, 0.f), FVector2D(1.f, 1.f), Colour);
	PaintTile(Canvas, RampHTex, X + W - Depth, Y, Depth, H, FVector2D(1.f, 0.f), FVector2D(0.f, 1.f), Colour);
}

void FTakeFilmOverlay::DrawUnder(AHUD& Hud, UCanvas& Canvas, const FTakeFilmFrame& F)
{
	if (!F.bFilmLook)
	{
		return;
	}
	EnsureTextures();
	Track(F);

	const float S = TakeStyle::Scale(F.ScreenH);
	const float Bar = BarHeight(F);
	const float PX = 0.f;
	const float PY = Bar;
	const float PW = F.ScreenW;
	const float PH = FMath::Max(0.f, F.ScreenH - 2.f * Bar);
	const double SinceCut = F.RealTime - CutTime;
	const int32 CutFrame = static_cast<int32>(FMath::FloorToDouble(SinceCut * Fps));
	const bool bCutBurst = SinceCut >= 0.0 && CutFrame < CutBurstFrames;
	const bool bLow = F.Composure < LowComposure;
	const bool bBlooper = F.Phase == ETakePhase::BlooperFreeze;
	const int32 BlooperFrame = static_cast<int32>(FMath::FloorToDouble((F.RealTime - PhaseTime) * Fps));
	// QA hud: the dirt rises with the stress on one curve (bStressDirt); off, the old step below LowComposure.
	const float Stress = FMath::Clamp(F.Stress, 0.f, 1.f);

	// The picture: the film's dirt, or the making-of's viewfinder.
	if (F.bFilmPicture)
	{
		const float Dust = bStressDirt ? FMath::Lerp(DustPerSecond, DustLowPerSecond, Stress)
			: (bLow ? DustLowPerSecond : DustPerSecond);
		Roll(F, Dust, ScratchEveryMin, ScratchEveryMax);
		const float CalmFlicker = bStressDirt ? FMath::Lerp(FlickerMax, FlickerLowMax, Stress)
			: (bLow ? FlickerLowMax : FlickerMax);
		float Flicker = bCutBurst ? FMath::Max(FlickerCutMax, CalmFlicker) : CalmFlicker;
		if (bBlooper && BlooperFrame == 0)
		{
			Flicker = BlooperFlicker;
			FlickerRoll = 1.f;
		}
		const float CalmGrain = GrainOpacity + (bStressDirt ? GrainStressExtra * Stress : 0.f);
		DrawFilm(Hud, Canvas, F, PX, PY, PW, PH, bCutBurst && bCutToFilm ? FMath::Max(GrainCutOpacity, CalmGrain) : CalmGrain,
			Flicker);
	}
	else if (bViewfinder)
	{
		const float ScanAlpha = bCutBurst ? ScanLineCutAlpha : ScanLineAlpha;
		PaintTile(Canvas, ScanTex, PX, PY, PW, PH, FVector2D(0.f, 0.f), FVector2D(1.f, PH / 4.f),
			FLinearColor(1.f, 1.f, 1.f, ScanAlpha));

		// Corner marks: they start 12% further from the centre on the cut and close by frame 3.
		float Open = 0.f;
		if (bSplice && SinceCut >= 0.0 && SinceCut < 3.0 / Fps)
		{
			Open = 0.12f * (1.f - static_cast<float>(SinceCut * Fps / 3.0));
		}
		const float Arm = FMath::RoundToFloat(CornerArmPx * S);
		const float Inset = FMath::RoundToFloat(CornerInsetPx * S);
		const float Thick = FMath::Max(1.f, FMath::RoundToFloat(TakeStyle::LineThin * S));
		const FLinearColor Mark = WithAlpha(TakeStyle::MoHigh, CornerAlpha);
		const float CX = PX + PW * 0.5f;
		const float CY = PY + PH * 0.5f;
		for (int32 Corner = 0; Corner < 4; ++Corner)
		{
			const float SX = (Corner & 1) ? 1.f : -1.f;
			const float SY = (Corner & 2) ? 1.f : -1.f;
			const float HalfW = (PW * 0.5f - Inset) * (1.f + Open);
			const float HalfH = (PH * 0.5f - Inset) * (1.f + Open);
			const float KX = CX + SX * HalfW;	// the corner's point
			const float KY = CY + SY * HalfH;
			PaintRect(Canvas, Mark, SX < 0.f ? KX : KX - Arm, SY < 0.f ? KY : KY - Thick, Arm, Thick);
			PaintRect(Canvas, Mark, SX < 0.f ? KX : KX - Thick, SY < 0.f ? KY : KY - Arm, Thick, Arm);
		}
	}

	// QA hud: the burn as composure falls moved to DrawStress (every camera, with or without the film look).

	// The splice: a paper flash on the press frame (one film frame), and on a cut to the film an ink frame-line band
	// rolling from the picture's bottom edge to its top over SpliceBandFrames.
	if (bSplice && SinceCut >= 0.0)
	{
		if (SinceCut < 1.0 / Fps)
		{
			PaintRect(Canvas, WithAlpha(TakeStyle::Paper, SpliceFlashOpacity), PX, PY, PW, PH);
		}
		const double BandSeconds = SpliceBandFrames / Fps;
		if (bCutToFilm && SinceCut < BandSeconds)
		{
			const float Band = FMath::RoundToFloat(SpliceBandPx * S);
			const float T = static_cast<float>(SinceCut / BandSeconds);
			const float BandY = FMath::Lerp(PY + PH - Band, PY, T);
			PaintRect(Canvas, TakeStyle::Ink, PX, BandY, PW, Band);
		}
	}

	// The bars, last: clean ink.
	if (Bar > 0.f)
	{
		PaintRect(Canvas, TakeStyle::Ink, 0.f, 0.f, F.ScreenW, Bar);
		PaintRect(Canvas, TakeStyle::Ink, 0.f, F.ScreenH - Bar, F.ScreenW, Bar);
	}
}

void FTakeFilmOverlay::DrawCard(AHUD& Hud, UCanvas& Canvas, const FTakeFilmFrame& F)
{
	if (!F.bFilmLook)
	{
		return;
	}
	EnsureTextures();
	Track(F);
	Roll(F, DustCardPerSecond, ScratchCardEvery * 0.75, ScratchCardEvery * 1.25);
	DrawFilm(Hud, Canvas, F, 0.f, 0.f, F.ScreenW, F.ScreenH, GrainCardOpacity, FlickerCardMax);
}

bool FTakeFilmOverlay::IsBlooperSlateDue(const FTakeFilmFrame& F) const
{
	if (!F.bFilmLook || !bBlooperBurn || F.Phase != ETakePhase::BlooperFreeze || LastPhase != ETakePhase::BlooperFreeze)
	{
		return true;
	}
	return (F.RealTime - PhaseTime) * Fps >= static_cast<double>(BlooperBurnFrames + BlooperPaperFrames);
}

void FTakeFilmOverlay::DrawOver(AHUD& Hud, UCanvas& Canvas, const FTakeFilmFrame& F)
{
	if (!F.bFilmLook)
	{
		return;
	}
	EnsureTextures();
	Track(F);
	const float Bar = BarHeight(F);
	const float PY = Bar;
	const float PW = F.ScreenW;
	const float PH = FMath::Max(0.f, F.ScreenH - 2.f * Bar);
	const double Since = F.RealTime - PhaseTime;
	const int32 Frame = static_cast<int32>(FMath::FloorToDouble(Since * Fps));

	// The blooper: a film burn grows from Julian to the picture's diagonal, then paper over the whole picture.
	if (bBlooperBurn && F.Phase == ETakePhase::BlooperFreeze && Frame >= 0)
	{
		if (Frame < BlooperBurnFrames)
		{
			const FVector2D Origin = F.bJulianOnScreen ? F.JulianScreen : FVector2D(PW * 0.5f, PY + PH * 0.5f);
			const float Diagonal = FMath::Sqrt(PW * PW + PH * PH);
			const float Radius = Diagonal * static_cast<float>(Frame + 1) / static_cast<float>(BlooperBurnFrames);
			// The texture's opaque part reaches 0.7 of its half-size: scale so that reaches Radius.
			const float Half = Radius / 0.7f;
			PaintTile(Canvas, BurnTex, Origin.X - Half, Origin.Y - Half, Half * 2.f, Half * 2.f, FVector2D(0.f, 0.f),
				FVector2D(1.f, 1.f), FLinearColor::White);
		}
		else if (Frame < BlooperBurnFrames + BlooperPaperFrames)
		{
			PaintRect(Canvas, TakeStyle::Paper, 0.f, PY, PW, PH);
		}
		if (Bar > 0.f)
		{
			PaintRect(Canvas, TakeStyle::Ink, 0.f, 0.f, F.ScreenW, Bar);
			PaintRect(Canvas, TakeStyle::Ink, 0.f, F.ScreenH - Bar, F.ScreenW, Bar);
		}
	}

	// PRINT IT: frame 0 is ink over the whole screen, bars included.
	if (bPrintItSnap && F.Phase == ETakePhase::Printed && Frame == 0)
	{
		PaintRect(Canvas, TakeStyle::Ink, 0.f, 0.f, F.ScreenW, F.ScreenH);
	}
}

// --- QA hud: composure as the picture ------------------------------------------------------------------------------------

float FTakeFilmOverlay::HeartbeatAt(const FTakeFilmFrame& F) const
{
	if (!bHeartbeat)
	{
		return 0.f;
	}
	const float Stress = FMath::Clamp(F.Stress, 0.f, 1.f);
	const float Amp = HeartbeatDepth * FMath::SmoothStep(HeartbeatFrom, 1.f, Stress);
	if (Amp <= 0.f)
	{
		return 0.f;
	}
	// Lub-dub: two soft bumps early in each beat, the second smaller; the rate climbs with the stress.
	const double Bpm = static_cast<double>(FMath::Lerp(HeartbeatBpmCalm, HeartbeatBpmTight, Stress));
	const double Beats = F.RealTime * Bpm / 60.0;
	const float P = static_cast<float>(Beats - FMath::FloorToDouble(Beats));
	const float Lub = FMath::Exp(-FMath::Square((P - 0.05f) / 0.06f));
	const float Dub = 0.6f * FMath::Exp(-FMath::Square((P - 0.24f) / 0.06f));
	return Amp * FMath::Min(1.f, Lub + Dub);
}

void FTakeFilmOverlay::DrawVignette(UCanvas& Canvas, const FVector2D& Centre, float HalfW, float HalfH, float Alpha,
	float X, float Y, float W, float H)
{
	if (Alpha <= 0.f || HalfW <= 1.f || HalfH <= 1.f || W <= 0.f || H <= 0.f)
	{
		return;
	}
	const FLinearColor Colour = WithAlpha(TakeStyle::Ink, Alpha);
	const float X0 = static_cast<float>(Centre.X) - HalfW;
	const float X1 = static_cast<float>(Centre.X) + HalfW;
	const float Y0 = static_cast<float>(Centre.Y) - HalfH;
	const float Y1 = static_cast<float>(Centre.Y) + HalfH;

	// The texture, clipped to the picture (its UVs cut to match), so nothing spills into the bars.
	const float IX0 = FMath::Max(X0, X);
	const float IX1 = FMath::Min(X1, X + W);
	const float IY0 = FMath::Max(Y0, Y);
	const float IY1 = FMath::Min(Y1, Y + H);
	if (IX1 > IX0 && IY1 > IY0)
	{
		const FVector2D UV0((IX0 - X0) / (X1 - X0), (IY0 - Y0) / (Y1 - Y0));
		const FVector2D UV1((IX1 - X0) / (X1 - X0), (IY1 - Y0) / (Y1 - Y0));
		PaintTile(Canvas, VignetteTex, IX0, IY0, IX1 - IX0, IY1 - IY0, UV0, UV1, Colour);
	}

	// The picture outside the texture: solid at the same alpha (above, below, then left and right between them).
	const float Top = FMath::Clamp(Y0, Y, Y + H);
	const float Bottom = FMath::Clamp(Y1, Y, Y + H);
	PaintRect(Canvas, Colour, X, Y, W, Top - Y);
	PaintRect(Canvas, Colour, X, Bottom, W, Y + H - Bottom);
	const float Left = FMath::Clamp(X0, X, X + W);
	const float Right = FMath::Clamp(X1, X, X + W);
	PaintRect(Canvas, Colour, X, Top, Left - X, Bottom - Top);
	PaintRect(Canvas, Colour, Right, Top, X + W - Right, Bottom - Top);
}

void FTakeFilmOverlay::DrawStress(UCanvas& Canvas, const FTakeFilmFrame& F, float PicY, float PicH)
{
	const float Stress = FMath::Clamp(F.Stress, 0.f, 1.f);
	const float Seen = bSeenPull ? FMath::Clamp(F.SeenWeight, 0.f, 1.f) : 0.f;
	if ((Stress <= 0.002f && Seen <= 0.002f) || PicH <= 0.f || F.ScreenW <= 0.f)
	{
		return;
	}
	EnsureTextures();
	const float S = TakeStyle::Scale(F.ScreenH);
	const float PX = 0.f;
	const float PY = PicY;
	const float PW = F.ScreenW;
	const float PH = PicH;
	const float Beat = HeartbeatAt(F);
	const FVector2D Pull = F.SeenDir.SizeSquared() > 1.0 ? F.SeenDir.GetSafeNormal() : F.SeenDir;

	// 1. The colour cools and drains (the grade's Stress does the draining; the overlay adds a navy wash).
	if (bStressWash && WashMaxAlpha > 0.f)
	{
		PaintRect(Canvas, WithAlpha(TakeStyle::MoShadow, WashMaxAlpha * Stress), PX, PY, PW, PH);
	}

	// 2. The vignette closes in; held, it is at least SeenVignetteAlpha and its clear centre slides away from the
	//    watcher, so the dark gathers on his side.
	if (bStressVignette)
	{
		float Alpha = VignetteMaxAlpha * Stress;
		float Scale = FMath::Lerp(VignetteScaleCalm, VignetteScaleTight, Stress);
		if (Seen > 0.f)
		{
			Alpha = FMath::Max(Alpha, SeenVignetteAlpha * Seen);
			Scale = FMath::Min(Scale, FMath::Lerp(VignetteScaleCalm, SeenVignetteScale, Seen));
		}
		Alpha = FMath::Clamp(Alpha * (1.f + Beat), 0.f, 0.95f);
		const FVector2D Centre(PX + PW * 0.5f - static_cast<float>(Pull.X) * Seen * SeenPull * PW * 0.5f,
			PY + PH * 0.5f - static_cast<float>(Pull.Y) * Seen * SeenPull * PH * 0.5f);
		DrawVignette(Canvas, Centre, PW * 0.5f * Scale, PH * 0.5f * Scale, Alpha, PX, PY, PW, PH);
	}

	// 3. The edges burn warm from BurnFrom, deeper and stronger as it rises, with the pulse.
	if (bBurn)
	{
		const float K = FMath::SmoothStep(BurnFrom, 1.f, Stress);
		if (K > 0.f)
		{
			DrawEdgeBands(Canvas, WithAlpha(TakeStyle::Danger, FMath::Min(1.f, BurnAlpha * K * (1.f + Beat))), PX, PY, PW,
				PH, (BurnMinPx + BurnExtraPx * K) * S);
		}
	}

	// 4. Held: a warm glow on the edge(s) facing the watcher, so the player knows which way to break the line.
	if (Seen > 0.f && !Pull.IsNearlyZero())
	{
		const float Depth = FMath::Min(SeenGlowPx * S, FMath::Min(PW, PH) * 0.5f);
		const float Base = SeenGlowAlpha * Seen;
		const float L = Base * FMath::Max(0.f, static_cast<float>(-Pull.X));
		const float R = Base * FMath::Max(0.f, static_cast<float>(Pull.X));
		const float T = Base * FMath::Max(0.f, static_cast<float>(-Pull.Y));
		const float B = Base * FMath::Max(0.f, static_cast<float>(Pull.Y));
		const FLinearColor Warm = TakeStyle::Danger;
		PaintTile(Canvas, RampHTex, PX, PY, Depth, PH, FVector2D(0.f, 0.f), FVector2D(1.f, 1.f), WithAlpha(Warm, L));
		PaintTile(Canvas, RampHTex, PX + PW - Depth, PY, Depth, PH, FVector2D(1.f, 0.f), FVector2D(0.f, 1.f), WithAlpha(Warm, R));
		PaintTile(Canvas, RampVTex, PX, PY, PW, Depth, FVector2D(0.f, 0.f), FVector2D(1.f, 1.f), WithAlpha(Warm, T));
		PaintTile(Canvas, RampVTex, PX, PY + PH - Depth, PW, Depth, FVector2D(0.f, 1.f), FVector2D(1.f, 0.f), WithAlpha(Warm, B));
	}
}
