// Step 4 (the look, pass 2): the HUD's drawing kit (docs/ART_STYLE.md §15.2, §15.5, §15.6). One system: the three type
// roles with their tracking, keycaps, glyphs, plates, stamps, cards and the slate, all from TakeStyle.h tokens. The
// HUD never draws a literal colour or size; it asks the kit. Canvas only (no UMG): text goes through FCanvasTextItem
// with an FSlateFontInfo built from the imported font faces (tools/build_fonts.py).

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "TakeStyle.h"
#include "TakeHudKit.generated.h"

class UCanvas;
class UFont;
class UFontFace;
class UTexture2D;

enum class ETakeAlign : uint8 { Left, Centre, Right };

/** How a rich line is inked: its words, and the keycaps written [E] inside it. */
struct FTakeInk
{
	FLinearColor Text = TakeStyle::Paper;
	FLinearColor KeyLine = TakeStyle::Paper;
	FLinearColor KeyLetter = TakeStyle::Paper;
	FLinearColor KeyFill = FLinearColor::Transparent;
	bool bSmallKeys = false;

	static FTakeInk Of(const FLinearColor& Words, const FLinearColor& Keys)
	{
		FTakeInk I;
		I.Text = Words;
		I.KeyLine = Keys;
		I.KeyLetter = Keys;
		return I;
	}
};

/** One stamp of the §15.6 family. */
struct FTakeStampSpec
{
	FString Text;
	/** Above the stamp, type.label in the stamp's colour ("DELETED SCENE RESTORED"); empty for none. */
	FString Caption;
	FLinearColor Colour = TakeStyle::Julian;
	TakeStyle::EType Type = TakeStyle::EType::Stamp;
	float Tilt = TakeStyle::StampTilt;
	bool bField = true;
	bool bOuter = true;
	bool bInner = true;
	bool bDiamonds = false;
};

USTRUCT()
struct FTakeHudKit
{
	GENERATED_BODY()

	// --- the frame ---------------------------------------------------------------------------------------------------
	/** Call first each DrawHUD: the screen, the scale s, the picture rect, real time and the beat. */
	void Begin(UCanvas* InCanvas, double InRealTime, float InSecondsSinceBeat, float InBarHeight);

	UCanvas* Canvas = nullptr;
	/** Canvas units per px-at-1080: the screen's s (from its pixel height, §15.2) divided by the canvas's DPI scale.
	 *  A game viewport on a 120% Windows desktop draws its canvas at DPI scale 1.2 (UCanvas::Update divides ClipX/Y by
	 *  it, FCanvas scales back up); the editor's PIE viewport uses 1.0. Sizing from the pixels keeps the HUD identical. */
	float S = 1.f;
	float W = 1920.f;
	float H = 1080.f;
	float Bar = 138.f;
	float PicY = 138.f;
	float PicH = 804.f;
	double Now = 0.0;
	float SinceBeat = 1.f;
	/** Multiplies every alpha the kit draws: set it around a composite to fade it out whole, then back to 1. */
	float Fade = 1.f;

	/** px at 1080 -> canvas units on this screen, rounded to whole screen pixels. */
	float Px(float V) const { return FMath::RoundToFloat(V * S * DPIScale) / DPIScale; }
	/** A position rounded to whole screen pixels. */
	float Snap(float V) const { return FMath::RoundToFloat(V * DPIScale) / DPIScale; }
	/** A line's thickness: rounded, never under 1 px. */
	float Ln(float V) const { return FMath::Max(1.f, FMath::RoundToFloat(V * S * DPIScale)) / DPIScale; }
	float CX() const { return W * 0.5f; }
	float PicBottom() const { return PicY + PicH; }

	// The bars' rows (§15.2), kept as offsets from the bars' edges on any aspect.
	float RowT1() const;
	float RowT2() const;
	float RowB1() const;
	float RowB2() const;
	float RowB3() const;

	// --- primitives ----------------------------------------------------------------------------------------------------
	void Rect(float X, float Y, float InW, float InH, const FLinearColor& C) const;
	void Frame(float X, float Y, float InW, float InH, float Thick, const FLinearColor& C) const;
	void Diamond(float CentreX, float CentreY, float Size, const FLinearColor& C) const;
	/** The slate's 45-degree stripe tile over an ink field. */
	void Stripes(float X, float Y, float InW, float InH, const FLinearColor& Meaning);
	/** Rotate (degrees, negative = counter-clockwise) and scale everything drawn until Pop() about Pivot. */
	void Push(const FVector2D& Pivot, float Degrees, float Scale) const;
	void Pop() const;

	// --- type ------------------------------------------------------------------------------------------------------------
	/** The em size of a type role on this screen, px. */
	float Em(TakeStyle::EType T) const { return Px(TakeStyle::Type(T).Px); }
	/** Width of Text drawn in T (case and tracking applied), px. */
	float Measure(TakeStyle::EType T, const FString& InText);
	/** Draws Text with its caps centred on CentreY; X is its left, centre or right edge. Returns the width. */
	float Text(TakeStyle::EType T, const FString& InText, float X, float CentreY, const FLinearColor& C,
		ETakeAlign Align = ETakeAlign::Left);

	// --- keycaps and glyphs (§15.2, §15.5) --------------------------------------------------------------------------------
	float KeyWidth(const FString& KeyText, bool bSmall);
	/** A keycap with its left edge at X; returns its width. */
	float Key(const FString& KeyText, float X, float CentreY, const FTakeInk& Ink);
	/** A beat pulse: 1.0 on the beat frame, PulseLow by frame 10, held to the next beat. */
	float Pulse() const;
	void GlyphDanger(float CentreX, float CentreY, float Alpha, bool bInkBorder) const;
	void GlyphJulian(float CentreX, float CentreY, float Alpha) const;
	void GlyphOff(float CentreX, float CentreY, const FLinearColor& C) const;
	/** A device use: filled On when unused, a 1 px outline in Spent when spent. X is the left edge. */
	void Pip(float X, float CentreY, bool bUnused, const FLinearColor& On, const FLinearColor& Spent) const;
	float GlyphW() const { return Px(TakeStyle::GlyphTBarW); }

	// --- rich lines: text with [E] keycaps and [danger] / [julian] glyphs --------------------------------------------------
	/** Width of the line unwrapped. */
	float RichWidth(TakeStyle::EType T, const FString& InText, const FTakeInk& Ink);
	/** Draws the line centred on CentreX, wrapped to MaxW in at most MaxLines lines, the block centred on CentreY.
	 *  Returns the widest line's width; OutLastLineY gets the last line's centre. */
	float Rich(TakeStyle::EType T, const FString& InText, float CentreX, float CentreY, float MaxW, int32 MaxLines,
		const FTakeInk& Ink, float* OutLastLineY = nullptr, int32* OutLines = nullptr);
	/** Word wrap of plain text into lines no wider than MaxW. */
	void Wrap(TakeStyle::EType T, const FString& InText, float MaxW, TArray<FString>& OutLines);

	// --- the family (§15.6) ------------------------------------------------------------------------------------------------
	/** Stamp box size (border to border) before tilt and landing. */
	FVector2D StampSize(const FTakeStampSpec& Spec);
	/** A stamp landing at Centre: Age seconds since its event, held HoldFrames film frames, then a 6-frame exit. */
	void Stamp(const FTakeStampSpec& Spec, const FVector2D& Centre, float Age, float HoldFrames);
	/** The landing scale: 1.30 on frame 0, 0.96 on frame 2, 1.00 from frame 4. */
	static float LandingScale(float Age);
	/** 1 while held, then linear to 0 over motion.settle; 0 after. */
	static float ExitAlpha(float Age, float HoldFrames);
	/** A card over the whole picture: ink field, paper outer rule 24 px in, bone inner hairline, paper diamonds.
	 *  Returns the content box (inside the padding) as X, Y, W, H. */
	FVector4f Card();
	/** The card's field only (the film overlay goes between it and the border). */
	void CardField() const;
	/** A slate's two sticks across W at (X, Y): the top one hinged at its left end, closing over frames 0-6. */
	void Sticks(float X, float Y, float InW, const FLinearColor& Meaning, float Age);
	/** A slate's board: ink field, 3 px paper border. */
	void Board(float X, float Y, float InW, float InH) const;
	/** A board cell's caption, top-left 8 px in (type.tag, ash). */
	void CellCaption(float X, float Y, const FString& Caption);
	void HairH(float X, float Y, float InW, const FLinearColor& C) const;
	void HairV(float X, float Y, float InH, const FLinearColor& C) const;

	// --- words --------------------------------------------------------------------------------------------------------------
	/** "SPEAKER: \"line\"" -> SPEAKER and line (quotes dropped). No speaker: OutSpeaker empty. */
	static void SplitSpeaker(const FString& In, FString& OutSpeaker, FString& OutLine);
	/** "CAPTION|TEXT" or "SCENE 2 - THE FLOOR" -> caption and text. No caption: OutCaption empty. */
	static void SplitCaption(const FString& In, FString& OutCaption, FString& OutText);
	/** "THE BOW" -> "The Bow". */
	static FString TitleCase(const FString& In);
	/** First letter upper, the rest as given; adds a full stop when there is no end punctuation. */
	static FString Sentence(const FString& In, bool bFullStop);

	static FLinearColor A(const FLinearColor& C, float Alpha) { return FLinearColor(C.R, C.G, C.B, C.A * Alpha); }

private:
	const FSlateFontInfo& Font(TakeStyle::EType T);
	float Track(TakeStyle::EType T) const { return TakeStyle::Type(T).TrackEm * Em(T); }
	FString Cased(TakeStyle::EType T, const FString& In) const;
	void EnsureFonts();
	void EnsureStripes();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UFontFace>> Faces;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> StripeTex;

	/** One runtime-cache UFont per face (FCanvasTextItem draws only with a UFont behind its FSlateFontInfo). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UFont>> FontObjects;
	TMap<uint32, FSlateFontInfo> FontInfos;
	bool bFontsTried = false;
	float LoggedScreenH = -1.f;
	float LoggedDpi = -1.f;

	/** A bottom-bar row: Down below the bar's top edge, lifted together if B3's keycaps would leave the screen. */
	float BottomRow(float Down) const;
	float DPIScale = 1.f;
};
