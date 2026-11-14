// Step 4 (the look, pass 2): the HUD's drawing kit. See TakeHudKit.h.

#include "TakeHudKit.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Engine/Texture2D.h"
#include "EngineFontServices.h"
#include "Fonts/CompositeFont.h"
#include "Fonts/FontCache.h"
#include "TakeTypes.h"


namespace TakeKitLocal
{
	enum class ERun : uint8 { Word, Key, Danger, Julian };

	struct FRun
	{
		ERun Kind = ERun::Word;
		FString Text;
		float W = 0.f;
		int32 Spaces = 0;	// whitespace before it in the source
	};

	/** Splits a rich line into words, keycaps ([E], [WASD], [1]) and glyphs ([danger], [julian]). */
	void Parse(const FString& In, TArray<FRun>& Out)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		const int32 N = In.Len();
		int32 I = 0;
		int32 Spaces = 0;
		auto Add = [&Out, &Spaces](ERun Kind, const FString& Text)
		{
			FRun& R = Out.AddDefaulted_GetRef();
			R.Kind = Kind;
			R.Text = Text;
			R.Spaces = Out.Num() == 1 ? 0 : Spaces;
			Spaces = 0;
		};
		while (I < N)
		{
			const TCHAR Ch = In[I];
			if (FChar::IsWhitespace(Ch))
			{
				++Spaces;
				++I;
				continue;
			}
			if (Ch == TEXT('['))
			{
				const int32 Close = In.Find(TEXT("]"), ESearchCase::CaseSensitive, ESearchDir::FromStart, I + 1);
				if (Close > I + 1)
				{
					const FString Inner = In.Mid(I + 1, Close - I - 1);
					if (Inner == TEXT("danger") || Inner == TEXT("julian"))
					{
						Add(Inner == TEXT("danger") ? ERun::Danger : ERun::Julian, FString());
						I = Close + 1;
						continue;
					}
					bool bKey = Inner.Len() <= 8;
					for (const TCHAR K : Inner)
					{
						bKey = bKey && (FChar::IsUpper(K) || FChar::IsDigit(K));
					}
					if (bKey)
					{
						Add(ERun::Key, Inner);
						I = Close + 1;
						continue;
					}
				}
			}
			int32 End = I + 1;
			while (End < N && !FChar::IsWhitespace(In[End]) && In[End] != TEXT('['))
			{
				++End;
			}
			Add(ERun::Word, In.Mid(I, End - I));
			I = End;
		}
	}

	/** A transient texture filled by Fill(x, y) (as TakeFilmOverlay.cpp makes its tiles). */
	template <typename FillFn>
	UTexture2D* MakeTexture(int32 W, int32 H, FillFn Fill)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		UTexture2D* Tex = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
		if (!Tex || !Tex->GetPlatformData() || Tex->GetPlatformData()->Mips.Num() == 0)
		{
			return nullptr;
		}
		Tex->SRGB = true;
		Tex->Filter = TF_Bilinear;
		Tex->AddressX = TA_Wrap;
		Tex->AddressY = TA_Wrap;
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
}


// --- the frame --------------------------------------------------------------------------------------------------------

void FTakeHudKit::Begin(UCanvas* InCanvas, double InRealTime, float InBarHeight)
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	Canvas = InCanvas;
	Now = InRealTime;
	if (!Canvas)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		return;
	}
	W = Canvas->ClipX;
	H = Canvas->ClipY;
	// ClipX/Y are screen pixels divided by the canvas's DPI scale (UCanvas::Update): size everything from the pixels.
	DPIScale = Canvas->Canvas ? FMath::Max(0.25f, Canvas->Canvas->GetDPIScale()) : 1.f;
	S = HudScale(H * DPIScale) / DPIScale;
	// The lanes never shrink below MinBar: on a screen wider than 2.39:1 the picture is cropped instead.
	Bar = FMath::Max(FMath::Max(0.f, InBarHeight), Px(MinBar));
	PicY = Bar;
	PicH = FMath::Max(0.f, H - 2.f * Bar);
	Fade = 1.f;
	if (!FMath::IsNearlyEqual(LoggedScreenH, H) || !FMath::IsNearlyEqual(LoggedDpi, DPIScale))
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		LoggedScreenH = H;
		LoggedDpi = DPIScale;
		UE_LOG(LogTake, Log, TEXT("HUD SCALE: canvas %.0f x %.0f, DPI scale %.2f, screen %.0f x %.0f, s %.3f, bar %.0f px"),
			W, H, DPIScale, W * DPIScale, H * DPIScale, S * DPIScale, Bar * DPIScale);
	}
}

float FTakeHudKit::RowT1() const
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	return Px(TakeStyle::RowT1);
}

float FTakeHudKit::RowT2() const
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	return Bar - Px(RowT2Up);
}

float FTakeHudKit::RowB1() const
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	return BottomRow(RowB1Down);
}

float FTakeHudKit::RowB2() const
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	return BottomRow(RowB2Down);
}

float FTakeHudKit::RowB3() const
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	return BottomRow(RowB3Down);
}

float FTakeHudKit::BottomRow(float Down) const
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	const float B3 = H - Bar + Px(RowB3Down);
	const float Over = FMath::Max(0.f, B3 + Px(KeyHSmall) * 0.5f + Px(BottomSafe) - H);
	return H - Bar + Px(Down) - Over;
}

// --- primitives ---------------------------------------------------------------------------------------------------------

void FTakeHudKit::Rect(float X, float Y, float InW, float InH, const FLinearColor& C) const
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	if (!Canvas || InW <= 0.f || InH <= 0.f || C.A * Fade <= 0.f)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		return;
	}
	FCanvasTileItem Item(FVector2D(X, Y), FVector2D(InW, InH), A(C, Fade));
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

void FTakeHudKit::Frame(float X, float Y, float InW, float InH, float Thick, const FLinearColor& C) const
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	Rect(X, Y, InW, Thick, C);
	Rect(X, Y + InH - Thick, InW, Thick, C);
	Rect(X, Y + Thick, Thick, InH - 2.f * Thick, C);
	Rect(X + InW - Thick, Y + Thick, Thick, InH - 2.f * Thick, C);
}

void FTakeHudKit::Diamond(float CentreX, float CentreY, float Size, const FLinearColor& C) const
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	if (!Canvas || Size <= 0.f || C.A * Fade <= 0.f)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		return;
	}
	FCanvasTileItem Item(FVector2D(CentreX - Size * 0.5f, CentreY - Size * 0.5f), FVector2D(Size, Size), A(C, Fade));
	Item.Rotation = FRotator(0.f, 45.f, 0.f);
	Item.PivotPoint = FVector2D(0.5f, 0.5f);
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

void FTakeHudKit::EnsureStripes()
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	if (StripeTex)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		return;
	}
	const int32 P = static_cast<int32>(StripePeriod);
	StripeTex = MakeTexture(P, P, [P](int32 X, int32 Y)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		return ((X + Y) % P) < P / 2 ? FColor(255, 255, 255, 255) : FColor(255, 255, 255, 0);
	});
}

void FTakeHudKit::Stripes(float X, float Y, float InW, float InH, const FLinearColor& Meaning)
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	Rect(X, Y, InW, InH, Ink);
	EnsureStripes();
	if (!Canvas || !StripeTex || !StripeTex->GetResource())
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		return;
	}
	const float Period = FMath::Max(1.f, Px(StripePeriod));
	FCanvasTileItem Item(FVector2D(X, Y), StripeTex->GetResource(), FVector2D(InW, InH), FVector2D(0.f, 0.f),
		FVector2D(InW / Period, InH / Period), A(Meaning, Fade));
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

void FTakeHudKit::Push(const FVector2D& Pivot, float Degrees, float InScale) const
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	if (!Canvas || !Canvas->Canvas)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		return;
	}
	const FMatrix M = FTranslationMatrix(FVector(-Pivot.X, -Pivot.Y, 0.0))
		* FScaleMatrix(FVector(InScale, InScale, 1.0))
		* FRotationMatrix(FRotator(0.0, Degrees, 0.0))
		* FTranslationMatrix(FVector(Pivot.X, Pivot.Y, 0.0));
	Canvas->Canvas->PushRelativeTransform(M);
}

void FTakeHudKit::Pop() const
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	if (Canvas && Canvas->Canvas)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		Canvas->Canvas->PopTransform();
	}
}

// --- type ----------------------------------------------------------------------------------------------------------------

void FTakeHudKit::EnsureFonts()
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	if (bFontsTried)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		return;
	}
	bFontsTried = true;
	int32 Missing = 0;
	for (int32 Index = 0; Index < static_cast<int32>(EFace::Count); ++Index)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		UFontFace* Face = LoadObject<UFontFace>(nullptr, FacePath(static_cast<EFace>(Index)));
		Faces.Add(Face);
		UFont* FontObject = nullptr;
		if (Face)
		{
			FontObject = NewObject<UFont>(GetTransientPackage(), NAME_None, RF_Transient);
			FontObject->FontCacheType = EFontCacheType::Runtime;
			FTypefaceEntry& Entry = FontObject->CompositeFont.DefaultTypeface.Fonts.AddDefaulted_GetRef();
			Entry.Name = TEXT("Regular");
			Entry.Font = FFontData(Face);
		}
		else
		{
			++Missing;
		}
		FontObjects.Add(FontObject);
	}
	if (Missing > 0)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		UE_LOG(LogTake, Warning, TEXT("HUD FONTS: %d of %d font faces missing in /Game/Take/Fonts: the engine font stands in"),
			Missing, static_cast<int32>(EFace::Count));
	}
	else
	{
		UE_LOG(LogTake, Log, TEXT("HUD FONTS: Limelight, IM Fell English, Josefin Sans loaded"));
	}
}

const FSlateFontInfo& FTakeHudKit::Font(TakeStyle::EType T)
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	EnsureFonts();
	const FTypeToken& Tok = Type(T);
	// Slate sizes are points at 96 DPI (1 pt = 4/3 canvas units); the font cache renders them at the canvas's DPI
	// scale, so the em is Tok.Px * S canvas units = Tok.Px * s screen pixels.
	const float Pt = FMath::Max(4.f, FMath::RoundToFloat(Tok.Px * S * 0.75f * 4.f) * 0.25f);
	const uint32 CacheKey = (static_cast<uint32>(T) << 24) | static_cast<uint32>(FMath::RoundToInt(Pt * 4.f));
	if (const FSlateFontInfo* Found = FontInfos.Find(CacheKey))
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		return *Found;
	}
	const int32 FaceIndex = static_cast<int32>(Tok.Face);
	const UFont* FontObject = FontObjects.IsValidIndex(FaceIndex) ? FontObjects[FaceIndex].Get() : nullptr;
	if (!FontObject && GEngine)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		FontObject = GEngine->GetMediumFont();	// the engine's runtime Roboto stands in (logged once in EnsureFonts)
	}
	return FontInfos.Add(CacheKey, FSlateFontInfo(FontObject, Pt));
}

FString FTakeHudKit::Cased(TakeStyle::EType T, const FString& In) const
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	return Type(T).bUpper ? In.ToUpper() : In;
}

float FTakeHudKit::Measure(TakeStyle::EType T, const FString& InText)
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	if (InText.IsEmpty())
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		return 0.f;
	}
	const FString Str = Cased(T, InText);
	const FSlateFontInfo Info = Font(T);
	const float Tr = Track(T) * DPIScale;	// the character list is in screen pixels
	if (!FEngineFontServices::IsInitialized())
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		return static_cast<float>(Str.Len()) * Em(T) * 0.55f;
	}
	const TSharedPtr<FSlateFontCache> Cache = FEngineFontServices::Get().GetFontCache();
	if (!Cache.IsValid())
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		return static_cast<float>(Str.Len()) * Em(T) * 0.55f;
	}
	// The same sum FCanvasSimpleTextItem::DrawStringInternal_RuntimeCache walks: kerning, advance, spacing adjust.
	FCharacterList& List = Cache->GetCharacterList(Info, DPIScale);
	float X = 0.f;
	TCHAR Prev = 0;
	for (int32 Index = 0; Index < Str.Len(); ++Index)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		const TCHAR Ch = Str[Index];
		const FCharacterEntry& Entry = List.GetCharacter(Ch, Info.FontFallback);
		const bool bWhite = !Entry.Valid || FChar::IsWhitespace(Ch);
		const float Advance = static_cast<float>(Entry.XAdvance);
		if (!bWhite && Index > 0)
		{
			X += static_cast<float>(List.GetKerning(Prev, Ch, Info.FontFallback));
		}
		X += Advance + Tr;
		Prev = Ch;
	}
	return FMath::Max(0.f, X - Tr) / DPIScale;
}

float FTakeHudKit::Text(TakeStyle::EType T, const FString& InText, float X, float CentreY, const FLinearColor& C, ETakeAlign Align)
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	if (InText.IsEmpty())
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		return 0.f;
	}
	const float TextW = Measure(T, InText);
	if (!Canvas || C.A * Fade <= 0.f)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		return TextW;
	}
	const FString Str = Cased(T, InText);
	const FSlateFontInfo Info = Font(T);
	float Left = X;
	if (Align == ETakeAlign::Centre)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		Left = X - TextW * 0.5f;
	}
	else if (Align == ETakeAlign::Right)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		Left = X - TextW;
	}
	// Caps centred on CentreY: the item's top is the baseline minus (line height + descender).
	float Top = CentreY - Em(T) * 0.6f;
	if (FEngineFontServices::IsInitialized())
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		const TSharedPtr<FSlateFontCache> Cache = FEngineFontServices::Get().GetFontCache();
		if (Cache.IsValid())
		{
			FCharacterList& List = Cache->GetCharacterList(Info, DPIScale);
			const float Baseline = CentreY + Type(T).CapEm * Em(T) * 0.5f;
			Top = Baseline - (static_cast<float>(List.GetMaxHeight()) + static_cast<float>(List.GetBaseline())) / DPIScale;
		}
	}
	FCanvasTextItem Item(FVector2D(Snap(Left), Snap(Top)), FText::FromString(Str), Info, A(C, Fade));
	Item.HorizSpacingAdjust = Track(T) * DPIScale;	// screen pixels, added to the glyph advances
	Canvas->DrawItem(Item);
	return TextW;
}

// --- keycaps and glyphs ----------------------------------------------------------------------------------------------------

float FTakeHudKit::KeyWidth(const FString& KeyText, bool bSmall)
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	return FMath::Max(Px(KeyMinW), Measure(EType::Key, KeyText) + 2.f * Px(KeyPad));
}

float FTakeHudKit::Key(const FString& KeyText, float X, float CentreY, const FTakeInk& InInk)
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	const float KW = KeyWidth(KeyText, InInk.bSmallKeys);
	const float KH = Px(InInk.bSmallKeys ? KeyHSmall : KeyH);
	const float Y = FMath::RoundToFloat(CentreY - KH * 0.5f);
	Rect(X, Y, KW, KH, InInk.KeyFill);
	Frame(X, Y, KW, KH, Ln(LineThin), InInk.KeyLine);
	Text(EType::Key, KeyText, X + KW * 0.5f, CentreY, InInk.KeyLetter, ETakeAlign::Centre);
	return KW;
}

void FTakeHudKit::GlyphDanger(float CentreX, float CentreY, float Alpha, bool bInkBorder) const
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	const float Size = Px(GlyphSquare);
	const float X = FMath::RoundToFloat(CentreX - Size * 0.5f);
	const float Y = FMath::RoundToFloat(CentreY - Size * 0.5f);
	if (bInkBorder)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		const float B = Ln(TapeGlyphBorder);
		Rect(X - B, Y - B, Size + 2.f * B, Size + 2.f * B, Ink);
	}
	Rect(X, Y, Size, Size, A(Danger, Alpha));
}

void FTakeHudKit::GlyphJulian(float CentreX, float CentreY, float Alpha) const
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	const float BW = Px(GlyphTBarW);
	const float BH = Ln(GlyphTBarH);
	const float SW = Ln(GlyphTStemW);
	const float SH = Px(GlyphTStemH);
	const float Top = FMath::RoundToFloat(CentreY - (BH + SH) * 0.5f);
	Rect(FMath::RoundToFloat(CentreX - BW * 0.5f), Top, BW, BH, A(TakeStyle::Julian, Alpha));
	Rect(FMath::RoundToFloat(CentreX - SW * 0.5f), Top + BH, SW, SH, A(TakeStyle::Julian, Alpha));
}

void FTakeHudKit::GlyphOff(float CentreX, float CentreY, const FLinearColor& C) const
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	const float Size = Px(GlyphSquare);
	Frame(FMath::RoundToFloat(CentreX - Size * 0.5f), FMath::RoundToFloat(CentreY - Size * 0.5f), Size, Size,
		Ln(LineThin), C);
}

void FTakeHudKit::Pip(float X, float CentreY, bool bUnused, const FLinearColor& On, const FLinearColor& Spent) const
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	const float PW = Px(PipW);
	const float PH = Ln(PipH);
	const float Y = FMath::RoundToFloat(CentreY - PH * 0.5f);
	if (bUnused)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		Rect(X, Y, PW, PH, On);
	}
	else
	{
		Frame(X, Y, PW, PH, 1.f, Spent);
	}
}

// --- rich lines -------------------------------------------------------------------------------------------------------------

namespace TakeKitLocal
{
	float GapBefore(const FRun& Prev, const FRun& Run, float SpaceW, float KeyGap)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		if (Run.Spaces <= 0)
		{
			return 0.f;
		}
		const bool bTokenSide = Prev.Kind != ERun::Word || Run.Kind != ERun::Word;
		const float One = bTokenSide ? FMath::Max(SpaceW, KeyGap) : SpaceW;
		return Run.Spaces >= 3 ? One * 2.f : One;	// a run of three or more spaces reads as two
	}
}

float FTakeHudKit::RichWidth(TakeStyle::EType T, const FString& InText, const FTakeInk& InInk)
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	TArray<FRun> Runs;
	Parse(InText, Runs);
	const float SpaceW = Measure(T, TEXT("n")) * 0.55f + Track(T);
	float X = 0.f;
	for (int32 Index = 0; Index < Runs.Num(); ++Index)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		FRun& Run = Runs[Index];
		Run.W = Run.Kind == ERun::Word ? Measure(T, Run.Text)
			: (Run.Kind == ERun::Key ? KeyWidth(Run.Text, InInk.bSmallKeys) : GlyphW());
		X += (Index > 0 ? GapBefore(Runs[Index - 1], Run, SpaceW, Px(KeyTextGap)) : 0.f) + Run.W;
	}
	return X;
}

float FTakeHudKit::Rich(TakeStyle::EType T, const FString& InText, float CentreX, float CentreY, float MaxW, int32 MaxLines,
	const FTakeInk& InInk, float* OutLastLineY, int32* OutLines)
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	TArray<FRun> Runs;
	Parse(InText, Runs);
	if (Runs.Num() == 0)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		return 0.f;
	}
	const float SpaceW = Measure(T, TEXT("n")) * 0.55f + Track(T);
	const float KeyGap = Px(KeyTextGap);
	for (FRun& Run : Runs)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		Run.W = Run.Kind == ERun::Word ? Measure(T, Run.Text)
			: (Run.Kind == ERun::Key ? KeyWidth(Run.Text, InInk.bSmallKeys) : GlyphW());
	}

	// Greedy wrap: a run starts a new line when it would pass MaxW, until the last allowed line takes the rest.
	TArray<int32> LineStart;
	TArray<float> LineW;
	LineStart.Add(0);
	LineW.Add(0.f);
	for (int32 Index = 0; Index < Runs.Num(); ++Index)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		const bool bLineEmpty = LineStart.Last() == Index;
		const float Gap = bLineEmpty ? 0.f : GapBefore(Runs[Index - 1], Runs[Index], SpaceW, KeyGap);
		if (!bLineEmpty && LineW.Last() + Gap + Runs[Index].W > MaxW && LineStart.Num() < FMath::Max(1, MaxLines))
		{
			LineStart.Add(Index);
			LineW.Add(Runs[Index].W);
			continue;
		}
		LineW.Last() += Gap + Runs[Index].W;
	}

	const int32 Lines = LineStart.Num();
	const float Pitch = Em(T) * LineSpacing;
	float Widest = 0.f;
	float LineY = CentreY - Pitch * 0.5f * static_cast<float>(Lines - 1);
	for (int32 Line = 0; Line < Lines; ++Line)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		const int32 First = LineStart[Line];
		const int32 End = Line + 1 < Lines ? LineStart[Line + 1] : Runs.Num();
		float X = FMath::RoundToFloat(CentreX - LineW[Line] * 0.5f);
		Widest = FMath::Max(Widest, LineW[Line]);
		for (int32 Index = First; Index < End; ++Index)
		{
			const FRun& Run = Runs[Index];
			if (Index > First)
			{
				X += GapBefore(Runs[Index - 1], Run, SpaceW, KeyGap);
			}
			switch (Run.Kind)
			{
			case ERun::Word: Text(T, Run.Text, X, LineY, InInk.Text); break;
			case ERun::Key: Key(Run.Text, X, LineY, InInk); break;
			case ERun::Danger: GlyphDanger(X + Run.W * 0.5f, LineY, InInk.Text.A, false); break;
			case ERun::Julian: GlyphJulian(X + Run.W * 0.5f, LineY, InInk.Text.A); break;
			}
			X += Run.W;
		}
		if (Line + 1 < Lines)
		{
			LineY += Pitch;
		}
	}
	if (OutLastLineY)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		*OutLastLineY = LineY;
	}
	if (OutLines)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		*OutLines = Lines;
	}
	return Widest;
}

void FTakeHudKit::Wrap(TakeStyle::EType T, const FString& InText, float MaxW, TArray<FString>& OutLines)
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	TArray<FString> Words;
	InText.ParseIntoArrayWS(Words);
	FString Building;
	for (const FString& Word : Words)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		const FString Candidate = Building.IsEmpty() ? Word : Building + TEXT(" ") + Word;
		if (!Building.IsEmpty() && Measure(T, Candidate) > MaxW)
		{
			OutLines.Add(Building);
			Building = Word;
		}
		else
		{
			Building = Candidate;
		}
	}
	if (!Building.IsEmpty())
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		OutLines.Add(Building);
	}
}

// --- the family --------------------------------------------------------------------------------------------------------------

FVector2D FTakeHudKit::StampSize(const FTakeStampSpec& Spec)
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	const float TextW = Measure(Spec.Type, Spec.Text);
	const float Cap = Type(Spec.Type).CapEm * Em(Spec.Type);
	const float Outer = Spec.bOuter ? Ln(LineHeavy) : 0.f;
	const float Inner = Spec.bInner ? Px(StampInnerIn) + Ln(LineThin) : 0.f;
	return FVector2D(TextW + 2.f * (Px(StampPadX) + Outer + Inner), Cap + 2.f * (Px(StampPadY) + Outer + Inner));
}

float FTakeHudKit::LandingScale(float Age)
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	const int32 F = FilmFrame(Age);
	switch (F)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
	case 0: return 1.30f;
	case 1: return 1.13f;
	case 2: return 0.96f;
	case 3: return 0.98f;
	default: return 1.f;
	}
}

float FTakeHudKit::ExitAlpha(float Age, float HoldFrames)
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	const float F = static_cast<float>(FilmFrame(Age));
	if (F < HoldFrames)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		return 1.f;
	}
	return FMath::Clamp(1.f - (F - HoldFrames + 1.f) / FramesSettle, 0.f, 1.f);
}

void FTakeHudKit::Stamp(const FTakeStampSpec& Spec, const FVector2D& Centre, float Age, float HoldFrames)
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	if (Age < 0.f || Spec.Text.IsEmpty())
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		return;
	}
	const float Alpha = ExitAlpha(Age, HoldFrames);
	if (Alpha <= 0.f)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		return;
	}
	const FVector2D Size = StampSize(Spec);
	const float X = FMath::RoundToFloat(Centre.X - Size.X * 0.5f);
	const float Y = FMath::RoundToFloat(Centre.Y - Size.Y * 0.5f);
	const FLinearColor Colour = A(Spec.Colour, Alpha);
	Push(Centre, Spec.Tilt, LandingScale(Age));
	if (Spec.bField)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		Rect(X, Y, Size.X, Size.Y, A(TakeStyle::Plate, StampFieldAlpha * Alpha));
	}
	const float Outer = Spec.bOuter ? Ln(LineHeavy) : 0.f;
	if (Spec.bOuter)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		Frame(X, Y, Size.X, Size.Y, Outer, Colour);
	}
	if (Spec.bInner)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		const float In = Outer + Px(StampInnerIn);
		Frame(X + In, Y + In, Size.X - 2.f * In, Size.Y - 2.f * In, Ln(LineThin), Colour);
		if (Spec.bDiamonds)
		{
			const float D = Px(CardDiamond);
			Diamond(X + In, Y + In, D, Colour);
			Diamond(X + Size.X - In, Y + In, D, Colour);
			Diamond(X + In, Y + Size.Y - In, D, Colour);
			Diamond(X + Size.X - In, Y + Size.Y - In, D, Colour);
		}
	}
	Text(Spec.Type, Spec.Text, Centre.X, Centre.Y, Colour, ETakeAlign::Centre);
	if (!Spec.Caption.IsEmpty())
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		Text(EType::Label, Spec.Caption, Centre.X, Y - Px(CaptionGap) - Em(EType::Label) * 0.4f, Colour, ETakeAlign::Centre);
	}
	Pop();
}

void FTakeHudKit::CardField() const
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	Rect(0.f, PicY, W, PicH, Ink);
}

FVector4f FTakeHudKit::Card()
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	const float In = Px(CardInset);
	const float X = In;
	const float Y = PicY + In;
	const float CW = W - 2.f * In;
	const float CH = PicH - 2.f * In;
	const float Rule = Ln(LineRule);
	Frame(X, Y, CW, CH, Rule, Paper);
	const float In2 = Rule + Px(CardInnerIn);
	const float IX = X + In2;
	const float IY = Y + In2;
	const float IW = CW - 2.f * In2;
	const float IH = CH - 2.f * In2;
	Frame(IX, IY, IW, IH, Ln(LineHair), Bone);
	const float D = Px(CardDiamond);
	Diamond(IX, IY, D, Paper);
	Diamond(IX + IW, IY, D, Paper);
	Diamond(IX, IY + IH, D, Paper);
	Diamond(IX + IW, IY + IH, D, Paper);
	const float Pad = Px(CardPad);
	return FVector4f(IX + Pad, IY + Pad, IW - 2.f * Pad, IH - 2.f * Pad);
}

void FTakeHudKit::Sticks(float X, float Y, float InW, const FLinearColor& Meaning, float Age)
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	const float SH = Px(StickH);
	const float Thin = Ln(LineThin);
	Stripes(X, Y + SH, InW, SH, Meaning);
	Frame(X, Y + SH, InW, SH, Thin, Paper);
	const float Closed = FMath::Clamp(static_cast<float>(FilmFrame(FMath::Max(0.f, Age))) / StickCloseFrames, 0.f, 1.f);
	Push(FVector2D(X, Y + SH), FMath::Lerp(StickOpenDeg, 0.f, Closed), 1.f);
	Stripes(X, Y, InW, SH, Meaning);
	Frame(X, Y, InW, SH, Thin, Paper);
	Pop();
}

void FTakeHudKit::Board(float X, float Y, float InW, float InH) const
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	Rect(X, Y, InW, InH, Ink);
	Frame(X, Y, InW, InH, Ln(BoardBorder), Paper);
}

void FTakeHudKit::CellCaption(float X, float Y, const FString& Caption)
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	Text(EType::Tag, Caption, X + Px(CellIn), Y + Px(CellIn) + Em(EType::Tag) * 0.5f, Ash);
}

void FTakeHudKit::HairH(float X, float Y, float InW, const FLinearColor& C) const
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	Rect(X, Y, InW, Ln(LineHair), C);
}

void FTakeHudKit::HairV(float X, float Y, float InH, const FLinearColor& C) const
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	Rect(X, Y, Ln(LineHair), InH, C);
}

// --- words ----------------------------------------------------------------------------------------------------------------------

void FTakeHudKit::SplitSpeaker(const FString& In, FString& OutSpeaker, FString& OutLine)
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	OutSpeaker.Reset();
	OutLine = In.TrimStartAndEnd();
	int32 Colon = INDEX_NONE;
	if (OutLine.FindChar(TEXT(':'), Colon) && Colon > 0 && Colon <= 24)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		const FString Head = OutLine.Left(Colon).TrimStartAndEnd();
		bool bName = !Head.IsEmpty();
		for (const TCHAR Ch : Head)
		{
			bName = bName && (FChar::IsUpper(Ch) || FChar::IsDigit(Ch) || Ch == TEXT(' ') || Ch == TEXT('.')
				|| Ch == TEXT('\'') || Ch == TEXT('-'));
		}
		if (bName)
		{
			OutSpeaker = Head;
			OutLine = OutLine.Mid(Colon + 1).TrimStartAndEnd();
		}
	}
	if (OutLine.Len() >= 2 && OutLine.StartsWith(TEXT("\"")) && OutLine.EndsWith(TEXT("\"")))
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		OutLine = OutLine.Mid(1, OutLine.Len() - 2);
	}
}

void FTakeHudKit::SplitCaption(const FString& In, FString& OutCaption, FString& OutText)
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	int32 Split = INDEX_NONE;
	if (In.FindChar(TEXT('|'), Split))
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		OutCaption = In.Left(Split).TrimStartAndEnd();
		OutText = In.Mid(Split + 1).TrimStartAndEnd();
		return;
	}
	const int32 Dash = In.Find(TEXT(" - "));
	if (Dash > 0 && In.StartsWith(TEXT("SCENE")))
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		OutCaption = In.Left(Dash).TrimStartAndEnd();
		OutText = In.Mid(Dash + 3).TrimStartAndEnd();
		return;
	}
	OutCaption.Reset();
	OutText = In;
}

FString FTakeHudKit::TitleCase(const FString& In)
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	FString Out = In.ToLower();
	bool bStart = true;
	for (int32 Index = 0; Index < Out.Len(); ++Index)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		if (bStart && FChar::IsAlpha(Out[Index]))
		{
			Out[Index] = FChar::ToUpper(Out[Index]);
		}
		bStart = FChar::IsWhitespace(Out[Index]);
	}
	return Out;
}

FString FTakeHudKit::Sentence(const FString& In, bool bFullStop)
{
	using namespace TakeStyle;
	using namespace TakeKitLocal;
	FString Out = In.TrimStartAndEnd();
	if (Out.IsEmpty())
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		return Out;
	}
	Out[0] = FChar::ToUpper(Out[0]);
	if (bFullStop)
	{
		using namespace TakeStyle;
		using namespace TakeKitLocal;
		const TCHAR Last = Out[Out.Len() - 1];
		if (Last != TEXT('.') && Last != TEXT('!') && Last != TEXT('?'))
		{
			Out.AppendChar(TEXT('.'));
		}
	}
	return Out;
}
