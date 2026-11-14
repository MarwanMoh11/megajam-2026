// Owner: (u). Step 4 pass 2: the cards and slates (docs/ART_STYLE.md §15.6): the title card, the blooper slate,
// PRINT IT, the stats and grade, the blooper reel and credits.

#include "TakeHUD.h"

#include "Engine/Canvas.h"
#include "TakeDirector.h"
#include "TakeGameInstance.h"


namespace TakeHudCards
{
	/** A slate's board split into a head row and a row of cells. */
	void SlateCells(FTakeHudKit& Kit, float X, float Y, float W, float H, const TArray<FString>& Captions,
		const TArray<FString>& Values, TakeStyle::EType ValueType)
	{
		using namespace TakeStyle;
		const int32 N = FMath::Max(1, Captions.Num());
		const float CellW = W / static_cast<float>(N);
		for (int32 Index = 0; Index < N; ++Index)
		{
			const float CellX = FMath::RoundToFloat(X + CellW * static_cast<float>(Index));
			if (Index > 0)
			{
				Kit.HairV(CellX, Y, H, Bone);
			}
			if (Captions.IsValidIndex(Index))
			{
				Kit.CellCaption(CellX, Y, Captions[Index]);
			}
			if (Values.IsValidIndex(Index))
			{
				Kit.Text(ValueType, Values[Index], CellX + Kit.Px(CellIn), Y + H * 0.6f, Paper);
			}
		}
	}
}

// --- the title card ----------------------------------------------------------------------------------------------------------

void ATakeHUD::DrawTitleCard(const ATakeDirector& Dir, const FTakeFilmFrame& F)
{
	using namespace TakeStyle;
	Kit.CardField();
	Film.DrawCard(*this, *Canvas, F);
	Kit.Rect(0.f, 0.f, Kit.W, Kit.Bar, Ink);	// the bars stay clean
	Kit.Rect(0.f, Kit.PicBottom(), Kit.W, Kit.Bar, Ink);
	const FVector4f Box = Kit.Card();
	const float CX = Kit.CX();

	// Heights of the blocks, then the gaps shrink if the picture is short.
	const float CapDisplay = TakeStyle::Type(EType::Display).CapEm * Kit.Em(EType::Display);
	const float LineH = Kit.Em(EType::Line) * LineSpacing;
	const float BodyH = Kit.Em(EType::Body) * LineSpacing;
	const float CastH = Kit.Px(KeyH) + Kit.Px(Unit) + LineH;
	const float Blocks = CapDisplay + LineH + BodyH * TitlePitch.Num() + CastH + BodyH * TitleRules.Num()
		+ Kit.Px(KeyHSmall) + Kit.Px(KeyH);
	const float Gaps[] = { 28.f, 24.f, 56.f, 56.f, 52.f, 32.f };	// pitch | cast | rules | controls: 3u more between groups
	float GapSum = 0.f;
	for (const float G : Gaps)
	{
		using namespace TakeStyle;
		GapSum += Kit.Px(G);
	}
	const float Squeeze = GapSum > 0.f ? FMath::Clamp((Box.W - Blocks) / GapSum, 0.f, 1.f) : 1.f;
	float Y = Box.Y;
	auto Next = [&Y, &Gaps, Squeeze, this](float BlockH, int32 GapIndex) -> float
	{
		const float Centre = Y + BlockH * 0.5f;
		Y += BlockH + (GapIndex >= 0 ? Kit.Px(Gaps[GapIndex]) * Squeeze : 0.f);
		return Centre;
	};

	Kit.Text(EType::Display, GameTitle, CX, Next(CapDisplay, 0), Paper, ETakeAlign::Centre);
	Kit.Text(EType::Line, TitleTagline, CX, Next(LineH, 1), Bone, ETakeAlign::Centre);
	for (int32 Index = 0; Index < TitlePitch.Num(); ++Index)
	{
		using namespace TakeStyle;
		Kit.Text(EType::Body, TitlePitch[Index], CX, Next(BodyH, Index == TitlePitch.Num() - 1 ? 2 : -1), Paper,
			ETakeAlign::Centre);
	}

	// The cast row: three columns, keycap and name over the job.
	{
		const float RowTop = Y;
		const float ColW = Box.Z / static_cast<float>(FMath::Max(1, TitleCast.Num()));
		for (int32 Index = 0; Index < TitleCast.Num(); ++Index)
		{
			TArray<FString> Parts;
			TitleCast[Index].ParseIntoArray(Parts, TEXT("|"), false);
			if (Parts.Num() < 3)
			{
				continue;
			}
			const float ColCX = Box.X + ColW * (static_cast<float>(Index) + 0.5f);
			const float NameY = RowTop + Kit.Px(KeyH) * 0.5f;
			const float RowW = Kit.KeyWidth(Parts[0], false) + Kit.Px(KeyTextGap) + Kit.Measure(EType::Label, Parts[1]);
			const float KX = FMath::RoundToFloat(ColCX - RowW * 0.5f);
			const float KW = Kit.Key(Parts[0], KX, NameY, FTakeInk::Of(Paper, Paper));
			Kit.Text(EType::Label, Parts[1], KX + KW + Kit.Px(KeyTextGap), NameY, Paper);
			Kit.Text(EType::Line, Parts[2], ColCX, RowTop + Kit.Px(KeyH) + Kit.Px(Unit) + LineH * 0.5f, Bone,
				ETakeAlign::Centre);
		}
		Next(CastH, 3);
	}

	for (int32 Index = 0; Index < TitleRules.Num(); ++Index)
	{
		using namespace TakeStyle;
		Kit.Rich(EType::Body, TitleRules[Index], CX, Next(BodyH, Index == TitleRules.Num() - 1 ? 4 : -1), Box.Z, 1,
			FTakeInk::Of(Paper, Paper));
	}

	FTakeInk Controls = FTakeInk::Of(Ash, Paper);
	Controls.bSmallKeys = true;
	Kit.Rich(EType::Tag, TitleControls, CX, Next(Kit.Px(KeyHSmall), 5), Box.Z, 1, Controls);

	// The prompt: its keycap's outline pulses on a 120 BPM beat of real time (the band holds on the title card).
	const float SavedBeat = Kit.SinceBeat;
	Kit.SinceBeat = static_cast<float>(FMath::Fmod(Kit.Now, 0.5));
	FTakeInk Prompt = FTakeInk::Of(Paper, FTakeHudKit::A(Paper, Kit.Pulse()));
	Prompt.KeyLetter = Paper;
	Kit.Rich(EType::Goal, TitlePrompt, CX, Next(Kit.Px(KeyH), -1), Box.Z, 1, Prompt);
	Kit.SinceBeat = SavedBeat;
}

// --- the blooper slate ----------------------------------------------------------------------------------------------------------

void ATakeHUD::DrawBlooperSlate(const ATakeDirector& Dir)
{
	using namespace TakeStyle;
	const UTakeGameInstance* GI = Dir.GetTakeGameInstance();
	Kit.Rect(0.f, Kit.PicY, Kit.W, Kit.PicH, Ink);

	const float SW = Kit.Px(BlooperW);
	const float SH = Kit.Px(BlooperH);
	const float X = FMath::RoundToFloat(Kit.CX() - SW * 0.5f);
	const float Top = FMath::RoundToFloat(Kit.PicY + Kit.PicH * 0.5f - SH * 0.5f);
	const float Stick = Kit.Px(StickH);
	const float SlateAge = Age(PhaseSince) - Frames(static_cast<float>(Film.BlooperBurnFrames + Film.BlooperPaperFrames));
	Kit.Sticks(X, Top, SW, Danger, SlateAge);

	const float BY = Top + 2.f * Stick;
	const float BH = SH - 2.f * Stick;
	Kit.Board(X, BY, SW, BH);
	const float HeadH = FMath::RoundToFloat(BH * 0.42f);
	const float RowH = FMath::RoundToFloat((BH - HeadH) * 0.5f);
	Kit.Text(EType::Stamp, TEXT("BLOOPER"), Kit.CX(), BY + HeadH * 0.5f, Danger, ETakeAlign::Centre);
	Kit.HairH(X, BY + HeadH, SW, Bone);
	Kit.HairH(X, BY + HeadH + RowH, SW, Bone);

	Kit.CellCaption(X, BY + HeadH, TEXT("WHAT"));
	Kit.Text(EType::Label, TakeText::BlooperReasonLine(Dir.LastBlooperReason), X + Kit.Px(CellIn),
		BY + HeadH + RowH * 0.62f, Paper);
	const FString AgainFrom = GI ? GI->CheckpointName : FString(TEXT("THE STREET"));
	TakeHudCards::SlateCells(Kit, X, BY + HeadH + RowH, SW, RowH, { TEXT("AGAIN FROM"), TEXT("TAKE") },
		{ AgainFrom, FString::FromInt(GI ? GI->TakeNumber : 1) }, EType::Label);

	DrawSubtitleNow(TEXT("DIRECTOR"), TEXT("Cut!"), true);
}

// --- the ends --------------------------------------------------------------------------------------------------------------------

void ATakeHUD::DrawEnds(const ATakeDirector& Dir, const FTakeFilmFrame& F)
{
	using namespace TakeStyle;
	Kit.Rect(0.f, 0.f, Kit.W, Kit.H, Ink);
	Film.DrawCard(*this, *Canvas, F);

	if (Dir.EndPage <= 0)
	{
		using namespace TakeStyle;
		DrawPrintIt(Dir);
	}
	else
	{
		const FVector4f Box = Kit.Card();
		if (Dir.EndPage == 1)
		{
			DrawStats(Dir, Box);
		}
		else
		{
			DrawReel(Dir, Box);
		}
	}

	FTakeInk Keys = FTakeInk::Of(Ash, Paper);
	Keys.bSmallKeys = true;
	Kit.Rich(EType::Tag, Dir.EndPage < 2 ? EndKeys : EndKeysLast, Kit.CX(), Kit.RowB3(), Kit.W, 1, Keys);
	DrawB1(Dir, true);
}

void ATakeHUD::DrawPrintIt(const ATakeDirector& Dir)
{
	using namespace TakeStyle;
	const UTakeGameInstance* GI = Dir.GetTakeGameInstance();
	const float SW = Kit.Px(PrintW);
	const float SH = Kit.Px(PrintH);
	const float X = FMath::RoundToFloat(Kit.CX() - SW * 0.5f);
	const float Top = FMath::RoundToFloat(Kit.H * 0.5f - SH * 0.5f);
	const float Stick = Kit.Px(StickH);
	Kit.Sticks(X, Top, SW, TakeStyle::Julian, Dir.EndPageSeconds);

	const float BY = Top + 2.f * Stick;
	const float BH = SH - 2.f * Stick;
	Kit.Board(X, BY, SW, BH);
	const float HeadH = FMath::RoundToFloat(BH * 0.55f);
	Kit.Text(EType::Display, TEXT("PRINT IT"), Kit.CX(), BY + HeadH * 0.5f, TakeStyle::Julian, ETakeAlign::Centre);
	Kit.HairH(X, BY + HeadH, SW, Bone);
	TakeHudCards::SlateCells(Kit, X, BY + HeadH, SW, BH - HeadH, { TEXT("SCENE"), TEXT("TAKE"), TEXT("DIRECTOR") },
		{ FString::FromInt(Dir.Scene), FString::FromInt(GI ? GI->TakeNumber : 1), DirectorWord }, EType::Title);

	// How he left, under the board; then the director's line.
	TArray<FString> Lines;
	Kit.Wrap(EType::Line, Dir.PrintLine, SW, Lines);
	float LineY = Top + SH + Kit.Px(PrintLineUnder);
	for (const FString& Each : Lines)
	{
		using namespace TakeStyle;
		Kit.Text(EType::Line, Each, Kit.CX(), LineY, Paper, ETakeAlign::Centre);
		LineY += Kit.Em(EType::Line) * LineSpacing;
	}
	DrawSubtitleNow(TEXT("DIRECTOR"), TEXT("Print it!"), true);
}

void ATakeHUD::DrawStats(const ATakeDirector& Dir, const FVector4f& Box)
{
	using namespace TakeStyle;
	const UTakeGameInstance* GI = Dir.GetTakeGameInstance();
	const float CX = Kit.CX();
	const float TitleCap = TakeStyle::Type(EType::Title).CapEm * Kit.Em(EType::Title);
	Kit.Text(EType::Title, TEXT("YOUR TAKE"), CX, Box.Y + TitleCap * 0.5f, Paper, ETakeAlign::Centre);

	struct FRow
	{
		FString Caption;
		FString Value;
		EType ValueType = EType::Title;
		FString After;	// type.line, bone, after the value
	};
	TArray<FRow> Rows;
	const int32 Whole = FMath::Max(0, FMath::FloorToInt(Dir.FinishSeconds));
	Rows.Add({ TEXT("TAKE"), FString::FromInt(GI ? GI->TakeNumber : 1), EType::Title, FString() });
	Rows.Add({ TEXT("RUNNING TIME"), FString::Printf(TEXT("%d:%02d"), Whole / 60, Whole % 60), EType::Title,
		TEXT("bloopers spliced out") });
	Rows.Add({ TEXT("CUTS"), FString::FromInt(GI ? GI->Cuts : 0), EType::Title, FString() });
	Rows.Add({ TEXT("BLOOPERS"), FString::FromInt(GI ? GI->BlooperReel.Num() : 0), EType::Title, FString() });
	Rows.Add({ TEXT("OUT BY"), Dir.bSideExit ? TEXT("The gold side exit") : TEXT("The stage trap"), EType::Body, FString() });
	int32 Built = 0;
	int32 Restored = 0;
	TArray<FString> SceneLines;
	for (const FTakeDeletedScene& Deleted : Dir.DeletedScenes)
	{
		using namespace TakeStyle;
		if (!Deleted.bBuilt)
		{
			continue;
		}
		++Built;
		const FString Name = FTakeHudKit::TitleCase(Deleted.Name);
		if (GI && GI->RestoredScenes.Contains(Deleted.Id))
		{
			++Restored;
			SceneLines.Add(FString::Printf(TEXT("[julian] Restored: %s"), *Name));
		}
		else
		{
			SceneLines.Add(FString::Printf(TEXT("Missed: %s. %s"), *Name, *FTakeHudKit::Sentence(Deleted.SlateLine, true)));
		}
	}
	Rows.Add({ TEXT("DELETED SCENES"), FString::Printf(TEXT("%d of %d"), Restored, Built), EType::Title, FString() });

	// Rows appear one every StatsRowSeconds, at full opacity on their frame.
	const float CaptionRight = CX + Kit.Px(StatsCaptionRight);
	const float ValueX = CX + Kit.Px(StatsValueX);
	const float Pitch = Kit.Px(StatsRowPitch);
	float Y = Box.Y + TitleCap + Pitch * 1.2f;
	int32 Shown = 0;
	for (const FRow& Row : Rows)
	{
		using namespace TakeStyle;
		if (Dir.EndPageSeconds < StatsRowSeconds * static_cast<float>(Shown))
		{
			break;
		}
		Kit.Text(EType::Tag, Row.Caption, CaptionRight, Y, Ash, ETakeAlign::Right);
		const float ValueW = Kit.Text(Row.ValueType, Row.Value, ValueX, Y, Paper);
		if (!Row.After.IsEmpty())
		{
			Kit.Text(EType::Line, Row.After, ValueX + ValueW + Kit.Px(Unit) * 2.f, Y, Bone);
		}
		Y += Pitch;
		++Shown;
	}
	for (const FString& Line : SceneLines)
	{
		using namespace TakeStyle;
		if (Dir.EndPageSeconds < StatsRowSeconds * static_cast<float>(Shown))
		{
			break;
		}
		Kit.Rich(EType::Body, Line, CX, Y, Box.Z, 2, FTakeInk::Of(Paper, Paper));
		Y += Pitch;
		++Shown;
	}

	// The grade lands GradeDelayFrames after the page opens, and stays.
	const float GradeAge = Dir.EndPageSeconds - Frames(GradeDelayFrames);
	if (GradeAge >= 0.f)
	{
		using namespace TakeStyle;
		FTakeStampSpec Spec;
		Spec.Text = TakeText::GradeName(Dir.Grade);
		Spec.Tilt = GradeTilt;
		Spec.bField = false;
		Spec.Colour = Dir.Grade == ETakeGrade::DirectorsCut ? TakeStyle::Julian : Paper;
		Spec.bInner = Dir.Grade != ETakeGrade::RoughCut;
		Spec.bDiamonds = Dir.Grade == ETakeGrade::DirectorsCut;
		const FVector2D At(CX + Kit.Px(GradeX), Kit.PicY + Kit.PicH * 0.5f + Kit.Px(GradeDY));
		const FVector2D Size = Kit.StampSize(Spec);
		Kit.Text(EType::Tag, TEXT("THE GRADE"), At.X, At.Y - Size.Y * 0.5f - Kit.Px(Unit) * 4.f, Ash, ETakeAlign::Centre);
		Kit.Stamp(Spec, At, GradeAge, 1.0e6f);
	}
}

void ATakeHUD::DrawReel(const ATakeDirector& Dir, const FVector4f& Box)
{
	using namespace TakeStyle;
	const UTakeGameInstance* GI = Dir.GetTakeGameInstance();
	const float CX = Kit.CX();
	const float TitleCap = TakeStyle::Type(EType::Title).CapEm * Kit.Em(EType::Title);
	const float HeaderBottom = Box.Y + TitleCap + Kit.Px(Unit) * 5.f;
	const float BandTop = HeaderBottom + Kit.Px(Unit) * 2.f;
	const float BandBottom = Box.Y + Box.W;

	struct FItem
	{
		EType Type = EType::Body;
		FString Text;
		FLinearColor Colour = Paper;
		float GapBefore = 0.f;
	};
	TArray<FItem> Items;
	const float Gap = Kit.Px(Unit) * 2.f;
	Items.Add({ EType::Title, TEXT("THE BLOOPER REEL"), Paper, 0.f });
	if (!GI || GI->BlooperReel.Num() == 0)
	{
		using namespace TakeStyle;
		Items.Add({ EType::Body, TEXT("No bloopers. One clean take."), Paper, Gap });
	}
	else
	{
		for (const FTakeBlooperRecord& Record : GI->BlooperReel)
		{
			Items.Add({ EType::Tag, FString::Printf(TEXT("TAKE %d  BAR %d"), Record.Take, Record.Bar), Ash, Gap });
			FString Line = FTakeHudKit::Sentence(TakeText::BlooperReasonLine(Record.Reason), true);
			if (!Record.CaughtBy.IsEmpty())
			{
				Line += FString::Printf(TEXT(" Caught by the %s."), *Record.CaughtBy);
			}
			Line += FString::Printf(TEXT(" Again from %s."), *Record.CheckpointName.ToLower());
			TArray<FString> Wrapped;
			Kit.Wrap(EType::Body, Line, Box.Z, Wrapped);
			for (const FString& Each : Wrapped)
			{
				Items.Add({ EType::Body, Each, Paper, 0.f });
			}
		}
	}
	for (int32 Index = 0; Index < CreditLines.Num(); ++Index)
	{
		using namespace TakeStyle;
		Items.Add({ Index == 0 ? EType::Title : EType::Body, CreditLines[Index], Paper, Index == 0 ? Gap * 4.f : Gap * 0.5f });
	}

	// Offsets down the roll; it starts 40% down the band and rolls until its last line reaches the band's middle.
	TArray<float> Offsets;
	float Cursor = 0.f;
	for (const FItem& Item : Items)
	{
		using namespace TakeStyle;
		Cursor += Item.GapBefore;
		Offsets.Add(Cursor + Kit.Em(Item.Type) * 0.5f);
		Cursor += Kit.Em(Item.Type) * LineSpacing;
	}
	const float Start = BandTop + (BandBottom - BandTop) * 0.4f;
	const float MaxRoll = FMath::Max(0.f, Start + (Offsets.Num() > 0 ? Offsets.Last() : 0.f) - (BandTop + BandBottom) * 0.5f);
	const float Rolled = FMath::Min(FMath::Max(0.f, Dir.EndPageSeconds) * ReelScrollSpeed * Kit.S, MaxRoll);
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		using namespace TakeStyle;
		const float ItemY = Start + Offsets[Index] - Rolled;
		const float Half = Kit.Em(Items[Index].Type) * 0.6f;
		if (ItemY - Half >= BandTop && ItemY + Half <= BandBottom)	// whole lines only: the roll is clipped by the band
		{
			Kit.Text(Items[Index].Type, Items[Index].Text, CX, ItemY, Items[Index].Colour, ETakeAlign::Centre);
		}
	}

	Kit.Text(EType::Title, TEXT("PRINT IT"), CX, Box.Y + TitleCap * 0.5f, Paper, ETakeAlign::Centre);
	Kit.Text(EType::Tag, TakeText::GradeName(Dir.Grade), CX, Box.Y + TitleCap + Kit.Px(Unit) * 3.f, Ash, ETakeAlign::Centre);
}
