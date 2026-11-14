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

	// The picture: the film's dirt, or the making-of's viewfinder.
	if (F.bFilmPicture)
	{
		Roll(F, bLow ? DustLowPerSecond : DustPerSecond, ScratchEveryMin, ScratchEveryMax);
		float Flicker = bCutBurst ? FlickerCutMax : (bLow ? FlickerLowMax : FlickerMax);
		if (bBlooper && BlooperFrame == 0)
		{
			Flicker = BlooperFlicker;
			FlickerRoll = 1.f;
		}
		DrawFilm(Hud, Canvas, F, PX, PY, PW, PH, bCutBurst && bCutToFilm ? GrainCutOpacity : GrainOpacity, Flicker);
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

	// The burn as composure falls (every camera): a danger band along the picture's four edges.
	if (bBurn && F.Composure < BurnBelow && BurnBelow > 0.f)
	{
		const float K = FMath::Clamp(1.f - F.Composure / BurnBelow, 0.f, 1.f);
		const float Alpha = BurnAlpha * K;	// steady at the composure's level: no beat
		DrawEdgeBands(Canvas, WithAlpha(TakeStyle::Danger, Alpha), PX, PY, PW, PH, (BurnMinPx + BurnExtraPx * K) * S);
	}

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
