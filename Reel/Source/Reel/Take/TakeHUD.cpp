// Owner: (u). Constructor by the header pass; bodies by (u).

#include "TakeHUD.h"

#include "BarClockComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "ComposureComponent.h"
#include "Device.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "FollowSpot.h"
#include "Julian.h"
#include "Mark.h"
#include "Misc/ScopeExit.h"
#include "TakeCharacter.h"
#include "TakeDirector.h"
#include "TakeGameInstance.h"
#include "TakePlayerController.h"
#include "TakeTypes.h"
#include "Watcher.h"

namespace TakeHudA
{
	/** A filled disc from horizontal 1 px rects. */
	void DrawDisc(AHUD& Hud, const FLinearColor& Colour, float CentreX, float CentreY, float Radius)
	{
		const int32 R = FMath::Max(1, FMath::RoundToInt(Radius));
		for (int32 Row = -R; Row <= R; ++Row)
		{
			const float HalfSpan = FMath::Sqrt(static_cast<float>(R * R - Row * Row));
			Hud.DrawRect(Colour, CentreX - HalfSpan, CentreY + static_cast<float>(Row), HalfSpan * 2.f, 1.f);
		}
	}

	/** A rectangle outline of the given thickness. */
	void DrawFrame(AHUD& Hud, const FLinearColor& Colour, float X, float Y, float W, float H, float Thickness)
	{
		Hud.DrawRect(Colour, X, Y, W, Thickness);
		Hud.DrawRect(Colour, X, Y + H - Thickness, W, Thickness);
		Hud.DrawRect(Colour, X, Y, Thickness, H);
		Hud.DrawRect(Colour, X + W - Thickness, Y, Thickness, H);
	}

	FLinearColor WithAlpha(const FLinearColor& Colour, float Alpha)
	{
		return FLinearColor(Colour.R, Colour.G, Colour.B, Alpha);
	}
}

namespace TakeHudEnds
{
	/** One line of the blooper reel and credits roll (page 2 of the ends). */
	struct FRollLine
	{
		FString Text;
		FLinearColor Colour = FLinearColor::White;
		/** Times the HUD's text scale. */
		float ScaleMul = 1.f;
		/** Empty lines before this one. */
		float GapBefore = 0.f;
	};

	/** The grade's colour: DIRECTOR'S CUT bright gold, FINAL CUT white, ROUGH CUT grey. */
	FLinearColor GradeColour(ETakeGrade Grade)
	{
		switch (Grade)
		{
		case ETakeGrade::DirectorsCut: return TakeColors::GoldBright;
		case ETakeGrade::FinalCut: return TakeColors::White;
		default: return TakeColors::Grey;
		}
	}
}

ATakeHUD::ATakeHUD()
{
}

void ATakeHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	ATakeDirector* Dir = GetDirector();
	if (!Dir)
	{
		return;
	}

	if (!SmallFont && GEngine)
	{
		SmallFont = GEngine->GetMediumFont();
	}
	if (!LargeFont && GEngine)
	{
		LargeFont = GEngine->GetLargeFont();
	}

	const float ScreenW = Canvas->ClipX;
	const float ScreenH = Canvas->ClipY;
	const float ScreenFactor = FMath::Max(0.75f, ScreenH / 900.f);	// 1 at 900 px tall
	const float Ui = TextScale * ScreenFactor;
	const float Margin = 18.f * Ui;
	const float LineH = 24.f * Ui;

	// Camera A's letterbox bar height, worked out on every camera (the objective sits under it on A, B and C alike).
	const float LetterboxBarH = FMath::Max(0.f, (ScreenH - ScreenW / FMath::Max(1.f, LetterboxAspect)) * 0.5f);

	const ETakeCamera Cam = Dir->CurrentCamera;
	const bool bOnA = Cam == ETakeCamera::A;
	UBarClockComponent* BarClock = Dir->Clock;
	UComposureComponent* Comp = Dir->Composure;
	const UTakeGameInstance* GI = Dir->GetTakeGameInstance();
	const int32 TakeN = GI ? GI->TakeNumber : 1;
	const FString Tc = BarClock ? BarClock->GetTimecode() : FString(TEXT("-.-"));

	// --- Step 4: the film overlay: under every HUD layer, and its blooper burn / PRINT IT snap over all of them ------
	const FTakeFilmFrame FilmFrame = MakeFilmFrame(*Dir, ScreenW, ScreenH);
	Film.DrawUnder(*this, *Canvas, FilmFrame);
	ON_SCOPE_EXIT
	{
		if (Canvas)
		{
			Film.DrawOver(*this, *Canvas, FilmFrame);
		}
	};
	const bool bFilmBars = Film.DrawsLetterbox(FilmFrame);

	// --- Step 3: the title card's and the credits' default lines (the fields stay editable; filled once) ------------
	if (TitleLines.Num() == 0)
	{
		TitleLines.Add(TEXT("Tonight the Aurelia Auction House sells the only print of a lost film: the Director's Cut."));
		TitleLines.Add(TEXT("You direct the crew that steals it, live, in one take."));
		TitleLines.Add(TEXT("[1] JULIAN, the actor        [2] MAYA, lights        [3] VEX, props"));
		TitleLines.Add(TEXT("WASD walk      Mouse look      E use what a crew member stands at      R new take"));
		TitleLines.Add(TEXT("Get Julian in, lift the keycard, take the reel, get him out. Being seen burns his composure."));
	}
	if (CreditLines.Num() == 0)
	{
		CreditLines.Add(GameTitle + TEXT("  -  ") + TitleTagline);
		CreditLines.Add(TEXT("Directed by you."));
		CreditLines.Add(TEXT("Made in Unreal Engine 5.9 for the Epic AI MegaJam 2026. Theme: Director's Cut."));
		CreditLines.Add(TEXT("By Marwan, solo, with Claude agents."));
		CreditLines.Add(TEXT("Thank you for playing."));
	}

	// --- Step 3: helpers for the full-screen cards -------------------------------------------------------------------
	// The scale that fits InText into InMaxW px (it only ever shrinks).
	auto FitScale = [this](const FString& InText, UFont* InFont, float InScale, float InMaxW) -> float
	{
		float FitW = 0.f;
		float FitH = 0.f;
		GetTextSize(InText, FitW, FitH, InFont, InScale);
		return (FitW > InMaxW && FitW > 0.f) ? InScale * (InMaxW / FitW) : InScale;
	};
	// Greedy word wrap of InText into lines no wider than InMaxW px.
	auto WrapText = [this](const FString& InText, UFont* InFont, float InScale, float InMaxW, TArray<FString>& OutWrapped)
	{
		TArray<FString> Words;
		InText.ParseIntoArray(Words, TEXT(" "), true);
		FString Building;
		for (const FString& Word : Words)
		{
			const FString Candidate = Building.IsEmpty() ? Word : Building + TEXT(" ") + Word;
			float CandW = 0.f;
			float CandH = 0.f;
			GetTextSize(Candidate, CandW, CandH, InFont, InScale);
			if (CandW > InMaxW && !Building.IsEmpty())
			{
				OutWrapped.Add(Building);
				Building = Word;
			}
			else
			{
				Building = Candidate;
			}
		}
		if (!Building.IsEmpty())
		{
			OutWrapped.Add(Building);
		}
	};
	// The flash lines, centred, newest at InBottomY and older ones upwards, never above InTopLimit.
	auto DrawFlashStack = [&](float InBottomY, float InTopLimit)
	{
		float StackY = InBottomY;
		for (int32 FlashIndex = Dir->Flashes.Num() - 1; FlashIndex >= 0; --FlashIndex)
		{
			if (StackY < InTopLimit)
			{
				break;
			}
			const FTakeFlash& StackFlash = Dir->Flashes[FlashIndex];
			const float StackAlpha = FMath::Clamp(StackFlash.SecondsLeft / 0.4f, 0.f, 1.f);
			DrawShadowedText(StackFlash.Text, TakeHudA::WithAlpha(StackFlash.Colour, StackAlpha), ScreenW * 0.5f, StackY,
				Ui * 1.1f, true);
			StackY -= LineH * 1.3f;
		}
	};

	// --- Step 3: the title card (phase Title). None of the normal HUD under it; the flash lines over it ------------
	if (Dir->Phase == ETakePhase::Title)
	{
		DrawRect(TakeHudA::WithAlpha(TakeColors::Black, 0.72f), 0.f, 0.f, ScreenW, ScreenH);
		Film.DrawCard(*this, *Canvas, FilmFrame);
		const float CardMaxW = ScreenW * 0.9f;
		const float CardMidX = ScreenW * 0.5f;
		float CardY = ScreenH * 0.13f;

		const float GameNameScale = FitScale(GameTitle, LargeFont, BigTitleScale * ScreenFactor, CardMaxW);
		const FVector2D GameNameDrawn = DrawTextWithFont(GameTitle, LargeFont, TakeColors::GoldBright, CardMidX, CardY,
			GameNameScale, true);
		CardY += static_cast<float>(GameNameDrawn.Y) + 4.f * Ui;

		const FVector2D TaglineDrawn = DrawShadowedText(TitleTagline, TakeColors::Gold, CardMidX, CardY,
			FitScale(TitleTagline, SmallFont, Ui * 1.2f, CardMaxW), true);
		CardY += FMath::Max(static_cast<float>(TaglineDrawn.Y), LineH) + LineH * 1.3f;

		for (const FString& CardLine : TitleLines)
		{
			const FVector2D CardLineDrawn = DrawShadowedText(CardLine, TakeColors::White, CardMidX, CardY,
				FitScale(CardLine, SmallFont, Ui, CardMaxW), true);
			CardY += FMath::Max(static_cast<float>(CardLineDrawn.Y), LineH) + LineH * 0.55f;
		}

		// The prompt near the bottom, pulsing gently on real time (the clock holds on the title card).
		const UWorld* CardWorld = GetWorld();
		const float CardNow = CardWorld ? static_cast<float>(CardWorld->GetRealTimeSeconds()) : 0.f;
		const float PromptAlpha = 0.6f + 0.4f * (0.5f + 0.5f * FMath::Sin(CardNow * UE_TWO_PI * 0.7f));
		const float PromptY = FMath::Max(CardY + LineH * 1.5f, ScreenH * 0.8f);
		DrawTextWithFont(TitlePrompt, LargeFont, TakeHudA::WithAlpha(TakeColors::Gold, PromptAlpha), CardMidX, PromptY,
			FitScale(TitlePrompt, LargeFont, TitleScale * ScreenFactor * 0.7f, CardMaxW), true);

		DrawFlashStack(PromptY - LineH * 1.5f, CardY);
		return;
	}

	// --- Step 3: the ends (phase Printed): solid black, nothing of the normal HUD, the page, the footer -------------
	if (Dir->Phase == ETakePhase::Printed)
	{
		DrawRect(TakeColors::Black, 0.f, 0.f, ScreenW, ScreenH);
		Film.DrawCard(*this, *Canvas, FilmFrame);
		const float EndMaxW = ScreenW * 0.9f;
		const float EndMidX = ScreenW * 0.5f;

		// The footer, and one flash row above it (a press on the end screens shows its answer there).
		const float FooterY = ScreenH - Margin - LineH;
		const FString FooterText = Dir->EndPage < 2
			? FString(TEXT("[ENTER] next      [R] another take      [ESC] quit"))
			: FString(TEXT("[R] another take      [ESC] quit"));
		DrawShadowedText(FooterText, TakeColors::Grey, EndMidX, FooterY, FitScale(FooterText, SmallFont, Ui * 0.95f, EndMaxW),
			true);
		const float FlashRowY = FooterY - LineH * 1.4f;
		DrawFlashStack(FlashRowY, FlashRowY);
		const float EndBottom = FlashRowY - Margin * 0.3f;	// nothing of a page is drawn below this

		const FString GradeText = TakeText::GradeName(Dir->Grade);

		if (Dir->EndPage <= 0)
		{
			// Page 0: the PRINT IT slate.
			float SlateY = ScreenH * 0.24f;
			const FString PrintItText = TEXT("PRINT IT");
			const FVector2D PrintItDrawn = DrawTextWithFont(PrintItText, LargeFont, TakeColors::GoldBright, EndMidX, SlateY,
				FitScale(PrintItText, LargeFont, BigTitleScale * ScreenFactor, EndMaxW), true);
			SlateY += static_cast<float>(PrintItDrawn.Y) + LineH * 0.8f;

			const FString DirectorSays = TEXT("DIRECTOR: \"Print it!\"");
			const FVector2D DirectorSaysDrawn = DrawShadowedText(DirectorSays, TakeColors::White, EndMidX, SlateY,
				FitScale(DirectorSays, SmallFont, Ui * 1.3f, EndMaxW), true);
			SlateY += FMath::Max(static_cast<float>(DirectorSaysDrawn.Y), LineH) + LineH * 0.8f;

			TArray<FString> PrintRows;
			const float PrintScale = Ui * 1.1f;
			WrapText(Dir->PrintLine, SmallFont, PrintScale, EndMaxW, PrintRows);
			for (const FString& PrintRow : PrintRows)
			{
				if (SlateY > EndBottom)
				{
					break;
				}
				const FVector2D PrintRowDrawn = DrawShadowedText(PrintRow, TakeColors::White, EndMidX, SlateY,
					FitScale(PrintRow, SmallFont, PrintScale, EndMaxW), true);
				SlateY += FMath::Max(static_cast<float>(PrintRowDrawn.Y), LineH) + LineH * 0.3f;
			}
		}
		else if (Dir->EndPage == 1)
		{
			// Page 1: the stats, one row every 0.3 s, then the grade.
			float StatsY = ScreenH * 0.07f;
			const FString StatsHeader = TEXT("YOUR TAKE");
			const FVector2D StatsHeaderDrawn = DrawTextWithFont(StatsHeader, LargeFont, TakeColors::Gold, EndMidX, StatsY,
				FitScale(StatsHeader, LargeFont, TitleScale * ScreenFactor, EndMaxW), true);
			StatsY += static_cast<float>(StatsHeaderDrawn.Y) + LineH * 0.8f;

			TArray<FString> RowTexts;
			TArray<FLinearColor> RowColours;
			RowTexts.Add(FString::Printf(TEXT("TAKE  %d"), TakeN));
			RowColours.Add(TakeColors::White);
			const int32 FinishWhole = FMath::Max(0, FMath::FloorToInt(Dir->FinishSeconds));
			RowTexts.Add(FString::Printf(TEXT("TIME  %d:%02d   (bloopers spliced out)"), FinishWhole / 60, FinishWhole % 60));
			RowColours.Add(TakeColors::White);
			RowTexts.Add(FString::Printf(TEXT("CUTS  %d"), GI ? GI->Cuts : 0));
			RowColours.Add(TakeColors::White);
			RowTexts.Add(FString::Printf(TEXT("BLOOPERS  %d"), GI ? GI->BlooperReel.Num() : 0));
			RowColours.Add(TakeColors::White);
			RowTexts.Add(Dir->bSideExit ? FString(TEXT("OUT BY  THE GOLD SIDE EXIT")) : FString(TEXT("OUT BY  THE STAGE TRAP")));
			RowColours.Add(TakeColors::White);

			int32 BuiltCount = 0;
			int32 RestoredCount = 0;
			for (const FTakeDeletedScene& Deleted : Dir->DeletedScenes)
			{
				if (!Deleted.bBuilt)
				{
					continue;
				}
				++BuiltCount;
				if (GI && GI->RestoredScenes.Contains(Deleted.Id))
				{
					++RestoredCount;
				}
			}
			RowTexts.Add(FString::Printf(TEXT("DELETED SCENES  %d / %d"), RestoredCount, BuiltCount));
			RowColours.Add(TakeColors::White);
			for (const FTakeDeletedScene& Deleted : Dir->DeletedScenes)
			{
				if (!Deleted.bBuilt)
				{
					continue;
				}
				if (GI && GI->RestoredScenes.Contains(Deleted.Id))
				{
					RowTexts.Add(FString::Printf(TEXT("RESTORED   %s"), *Deleted.Name));
					RowColours.Add(TakeColors::Gold);
				}
				else
				{
					RowTexts.Add(FString::Printf(TEXT("MISSED   %s: %s"), *Deleted.Name, *Deleted.SlateLine));
					RowColours.Add(TakeColors::Grey);
				}
			}

			// Left-aligned in a centred column as wide as the widest row (so rows do not shift as they appear).
			const float RowScaleBase = Ui * 1.1f;
			float ColumnW = 0.f;
			for (const FString& RowText : RowTexts)
			{
				float RowW = 0.f;
				float RowH = 0.f;
				GetTextSize(RowText, RowW, RowH, SmallFont, RowScaleBase);
				ColumnW = FMath::Max(ColumnW, RowW);
			}
			const float RowScale = ColumnW > EndMaxW ? RowScaleBase * (EndMaxW / ColumnW) : RowScaleBase;
			ColumnW = FMath::Min(ColumnW, EndMaxW);
			const float ColumnX = FMath::Max(Margin, (ScreenW - ColumnW) * 0.5f);

			// The grade block under the rows; the rows share what height is left.
			const float GradeScale = FitScale(GradeText, LargeFont, TitleScale * ScreenFactor * 1.3f, EndMaxW);
			float GradeW = 0.f;
			float GradeH = 0.f;
			GetTextSize(GradeText, GradeW, GradeH, LargeFont, GradeScale);
			const float GradeBlockH = LineH * 1.6f + GradeH;
			const int32 RowCount = RowTexts.Num();
			const float RowsSpace = EndBottom - StatsY - GradeBlockH - LineH * 0.6f;
			const float RowStep = FMath::Clamp(RowsSpace / static_cast<float>(FMath::Max(1, RowCount)), LineH * 0.9f,
				LineH * 1.4f);

			const float RowInterval = 0.3f;
			for (int32 RowIndex = 0; RowIndex < RowCount; ++RowIndex)
			{
				if (Dir->EndPageSeconds < RowInterval * static_cast<float>(RowIndex))
				{
					break;
				}
				DrawShadowedText(RowTexts[RowIndex], RowColours[RowIndex], ColumnX, StatsY + RowStep * static_cast<float>(RowIndex),
					RowScale, false);
			}

			if (Dir->EndPageSeconds >= RowInterval * static_cast<float>(RowCount))
			{
				const float GradeTopY = StatsY + RowStep * static_cast<float>(RowCount) + LineH * 0.6f;
				const FVector2D GradeLabelDrawn = DrawShadowedText(TEXT("THE GRADE"), TakeColors::Grey, EndMidX, GradeTopY,
					Ui * 0.95f, true);
				DrawTextWithFont(GradeText, LargeFont, TakeHudEnds::GradeColour(Dir->Grade), EndMidX,
					GradeTopY + static_cast<float>(GradeLabelDrawn.Y) + LineH * 0.3f, GradeScale, true);
			}
		}
		else
		{
			// Page 2: a fixed header, then the blooper reel and the credits rolling up between it and the footer.
			const FString ReelHeader = FString::Printf(TEXT("PRINT IT   -   %s"), *GradeText);
			const float ReelHeaderY = Margin * 1.5f;
			const FVector2D ReelHeaderDrawn = DrawTextWithFont(ReelHeader, LargeFont, TakeColors::GoldBright, EndMidX, ReelHeaderY,
				FitScale(ReelHeader, LargeFont, TitleScale * ScreenFactor, EndMaxW), true);
			const float BandTop = ReelHeaderY + static_cast<float>(ReelHeaderDrawn.Y) + Margin;
			const float BandBottom = EndBottom;

			TArray<TakeHudEnds::FRollLine> RollLines;
			auto AddRoll = [&RollLines](const FString& InRollText, const FLinearColor& InRollColour, float InRollScale,
				float InRollGap)
			{
				TakeHudEnds::FRollLine& NewRoll = RollLines.AddDefaulted_GetRef();
				NewRoll.Text = InRollText;
				NewRoll.Colour = InRollColour;
				NewRoll.ScaleMul = InRollScale;
				NewRoll.GapBefore = InRollGap;
			};

			AddRoll(TEXT("THE BLOOPER REEL"), TakeColors::Gold, 1.3f, 0.f);
			if (!GI || GI->BlooperReel.Num() == 0)
			{
				AddRoll(TEXT("No bloopers. One clean take."), TakeColors::White, 1.f, 0.5f);
			}
			else
			{
				for (int32 ReelIndex = 0; ReelIndex < GI->BlooperReel.Num(); ++ReelIndex)
				{
					const FTakeBlooperRecord& Record = GI->BlooperReel[ReelIndex];
					FString RecordText = FString::Printf(TEXT("TAKE %d    bar %d    AGAIN FROM %s    %s"), Record.Take, Record.Bar,
						*Record.CheckpointName, TakeText::BlooperReasonName(Record.Reason));
					if (!Record.CaughtBy.IsEmpty())
					{
						RecordText += FString::Printf(TEXT("   -   %s"), *Record.CaughtBy);
					}
					AddRoll(RecordText, TakeColors::White, 1.f, ReelIndex == 0 ? 0.5f : 0.f);
				}
			}
			for (int32 CreditIndex = 0; CreditIndex < CreditLines.Num(); ++CreditIndex)
			{
				AddRoll(CreditLines[CreditIndex], CreditIndex == 0 ? TakeColors::GoldBright : TakeColors::White,
					CreditIndex == 0 ? 1.2f : 1.f, CreditIndex == 0 ? 2.f : 0.f);
			}

			// Each line's offset from the roll's first line.
			const float RollStep = LineH * 1.35f;
			TArray<float> RollOffsets;
			float RollCursor = 0.f;
			for (const TakeHudEnds::FRollLine& RollLine : RollLines)
			{
				RollCursor += RollLine.GapBefore * RollStep;
				RollOffsets.Add(RollCursor);
				RollCursor += RollStep;
			}

			// The roll starts with its first line 40% down the band and moves up until the last line reaches the middle.
			const float RollStartY = BandTop + (BandBottom - BandTop) * 0.4f;
			const float LastOffset = RollOffsets.Num() > 0 ? RollOffsets.Last() : 0.f;
			const float MaxRoll = FMath::Max(0.f, RollStartY + LastOffset - ScreenH * 0.5f);
			const float Rolled = FMath::Min(FMath::Max(0.f, Dir->EndPageSeconds) * ReelScrollSpeed * ScreenFactor, MaxRoll);
			for (int32 RollIndex = 0; RollIndex < RollLines.Num(); ++RollIndex)
			{
				const TakeHudEnds::FRollLine& RollLine = RollLines[RollIndex];
				const float RollY = RollStartY + RollOffsets[RollIndex] - Rolled;
				const float RollScale = FitScale(RollLine.Text, SmallFont, Ui * RollLine.ScaleMul, EndMaxW);
				const float RollH = LineH * RollLine.ScaleMul;
				if (RollY < BandTop || RollY + RollH > BandBottom)
				{
					continue;	// outside the band between the header and the footer
				}
				DrawShadowedText(RollLine.Text, RollLine.Colour, EndMidX, RollY, RollScale, true);
			}
		}
		return;
	}

	// --- Step 3: the alarm's red wash along the picture's four edges, under every other layer ---------------------
	const bool bAlarmShown = Dir->Phase == ETakePhase::Rolling && Dir->bAlarm;
	if (bAlarmShown)
	{
		const float WashPulse = BarClock ? BarClock->GetPulse() : 0.f;
		const float WashAlpha = FMath::Clamp(AlarmEdgeAlpha * (0.35f + 0.65f * WashPulse) * (Dir->bAlarmLive ? 1.f : 0.6f),
			0.f, 1.f);
		const FLinearColor WashColour = TakeHudA::WithAlpha(TakeColors::Red, WashAlpha);
		const float WashDepth = ScreenH * 0.07f;
		// On A the picture sits between the letterbox bars: the wash hugs the picture, not the black bars.
		const float PictureTop = (bOnA || bFilmBars) ? LetterboxBarH : 0.f;
		const float PictureBottom = (bOnA || bFilmBars) ? ScreenH - LetterboxBarH : ScreenH;
		const float SideH = FMath::Max(0.f, PictureBottom - PictureTop - WashDepth * 2.f);
		DrawRect(WashColour, 0.f, PictureTop, ScreenW, WashDepth);
		DrawRect(WashColour, 0.f, PictureBottom - WashDepth, ScreenW, WashDepth);
		DrawRect(WashColour, 0.f, PictureTop + WashDepth, WashDepth, SideH);
		DrawRect(WashColour, ScreenW - WashDepth, PictureTop + WashDepth, WashDepth, SideH);
	}

	// --- Camera look -----------------------------------------------------------------------------------------------
	float TopBarH = 0.f;
	float BottomTop = ScreenH;	// top of the bottom band (letterbox bar on A, a strip on B/C)
	const FLinearColor HeadColour = bOnA ? TakeColors::White : TakeColors::WorkBlue;

	if (bOnA)
	{
		const float BarH = LetterboxBarH;
		TopBarH = BarH;
		BottomTop = ScreenH - BarH;
		if (!bFilmBars)
		{
			DrawRect(TakeColors::Black, 0.f, 0.f, ScreenW, BarH);
			DrawRect(TakeColors::Black, 0.f, BottomTop, ScreenW, BarH);
		}

		// REC dot, top right.
		const float RecR = 8.f * Ui;
		const float RecX = ScreenW - Margin - 60.f * Ui;
		TakeHudA::DrawDisc(*this, TakeColors::Red, RecX, Margin + LineH * 0.5f, RecR);
		DrawShadowedText(TEXT("REC"), TakeColors::Red, RecX + RecR + 6.f * Ui, Margin, Ui, false);
	}
	else
	{
		// B/C: a dark strip along the bottom for the composure line and the how-to line.
		BottomTop = ScreenH - (LineH * 2.f + Margin * 1.5f);
		DrawRect(TakeHudA::WithAlpha(TakeColors::Black, 0.55f), 0.f, BottomTop, ScreenW, ScreenH - BottomTop);
	}

	// Top left: camera name, timecode, take.
	const FString CamName = FString::Printf(TEXT("%s - %s"), TakeText::CameraLetter(Cam), TakeText::CrewName(Cam));
	float HeadX = Margin;
	const FVector2D NameSize = DrawShadowedText(CamName, HeadColour, HeadX, Margin, Ui, false);
	HeadX += NameSize.X + 24.f * Ui;
	const FVector2D TcSize = DrawShadowedText(Tc, TakeColors::White, HeadX, Margin, Ui, false);

	// Beat dot right after the timecode: grows and brightens with the pulse.
	const float Pulse = BarClock ? BarClock->GetPulse() : 0.f;
	const float DotR = (5.f + 4.f * Pulse) * Ui;
	const float DotX = HeadX + TcSize.X + 14.f * Ui;
	TakeHudA::DrawDisc(*this, FMath::Lerp(TakeColors::Grey, TakeColors::GoldBright, Pulse), DotX,
		Margin + LineH * 0.5f, DotR);

	const float TakeX = DotX + 18.f * Ui;
	const FVector2D TakeSize = DrawShadowedText(FString::Printf(TEXT("TAKE %d"), TakeN), TakeColors::White, TakeX, Margin, Ui,
		false);

	// Step 2: the scene after the take, and "BAND BREAK" under the beat dot while the clicks are silent.
	DrawShadowedText(FString::Printf(TEXT("SCENE %d"), Dir->Scene), TakeColors::White,
		TakeX + static_cast<float>(TakeSize.X) + 24.f * Ui, Margin, Ui, false);
	if (BarClock && BarClock->IsMetronomeMuted())
	{
		DrawShadowedText(TEXT("BAND BREAK"), TakeColors::Gold, DotX - DotR, Margin + LineH, Ui * 0.85f, false);
	}

	// Composure line with its label, in the bottom band.
	{
		const float Fraction = Comp ? Comp->GetFraction() : 1.f;
		const EComposureLabel CompLabel = Comp ? Comp->GetLabel() : EComposureLabel::None;
		const float LineW = ScreenW * 0.5f;
		const float LineX = (ScreenW - LineW) * 0.5f;
		const float LineY = BottomTop + Margin;
		const float LineThick = ComposureLineHeight * Ui;
		const FLinearColor Fill = CompLabel == EComposureLabel::Seen ? TakeColors::Red : TakeColors::Gold;
		DrawRect(TakeColors::DarkGrey, LineX, LineY, LineW, LineThick);
		DrawRect(Fill, LineX, LineY, LineW * Fraction, LineThick);
		if (Comp && CompLabel != EComposureLabel::None)
		{
			const FLinearColor LabelColour = CompLabel == EComposureLabel::Seen ? TakeColors::Red : TakeColors::Gold;
			DrawShadowedText(Comp->GetLabelText(), LabelColour, LineX + LineW + 14.f * Ui,
				LineY + LineThick * 0.5f - LineH * 0.5f, Ui, false);
		}
	}

	// --- The reticle and the use prompt; in a flight, the big "CUT -> MAYA" instead --------------------------------
	ATakePlayerController* TakePC = Cast<ATakePlayerController>(GetOwningPlayerController());
	if (!TakePC)
	{
		TakePC = Dir->PC;
	}
	const float MidX = ScreenW * 0.5f;
	const float MidY = ScreenH * 0.5f;
	// The next free line under the reticle (the use prompt, then the action bar).
	float UnderReticleY = MidY + FMath::Max(3.f, ReticleSize * Ui * 0.5f) + 14.f * Ui;
	if (TakePC && TakePC->IsInFlight())
	{
		if (const ATakeCharacter* FlightTarget = TakePC->GetFlightTarget())
		{
			DrawTextWithFont(FString::Printf(TEXT("CUT -> %s"), FlightTarget->GetCrewName()), LargeFont, TakeColors::White,
				MidX, ScreenH * 0.42f, SlateTextScale * Ui * 0.5f, true);
		}
	}
	else
	{
		const FTakeUseTarget& Use = Dir->CurrentUse;
		const FLinearColor ReticleColour = Use.Watcher ? TakeColors::GoldBright
			: TakeHudA::WithAlpha(TakeColors::White, 0.8f);
		const float Half = FMath::Max(3.f, ReticleSize * Ui * 0.5f);
		const float Thick = FMath::Max(1.f, FMath::RoundToFloat(1.5f * Ui));
		DrawRect(ReticleColour, MidX - Half, MidY - Thick * 0.5f, Half * 2.f, Thick);
		DrawRect(ReticleColour, MidX - Thick * 0.5f, MidY - Half, Thick, Half * 2.f);
		if (!Use.Prompt.IsEmpty())
		{
			const FVector2D PromptSize = DrawShadowedText(Use.Prompt, Use.bCanFire ? TakeColors::GoldBright : TakeColors::Grey,
				MidX, UnderReticleY, Ui * 1.1f, true);
			UnderReticleY += static_cast<float>(PromptSize.Y) + 6.f * Ui;
		}
	}

	// --- Step 2: the action bar (the lift), when Julian stands on an action mark that is not done yet ---------------
	{
		const AMark* ActionMark = Dir->Julian ? Dir->Julian->GetStandingMark() : nullptr;
		if (ActionMark && ActionMark->IsActionMark() && !ActionMark->IsActionDone())
		{
			const bool bSeen = Dir->bJulianSeen;
			const FLinearColor VerbColour = bSeen ? TakeColors::Red : TakeColors::GoldBright;
			const FVector2D VerbSize = DrawShadowedText(ActionMark->ActionVerb, VerbColour, MidX, UnderReticleY, Ui, true);
			const float BarW = FMath::Max(1.f, static_cast<float>(ActionBarSize.X) * ScreenFactor);
			const float BarH = FMath::Max(1.f, static_cast<float>(ActionBarSize.Y) * ScreenFactor);
			const float BarX = MidX - BarW * 0.5f;
			const float BarY = UnderReticleY + static_cast<float>(VerbSize.Y) + 4.f * Ui;
			const float Alpha = FMath::Clamp(ActionMark->GetActionAlpha(), 0.f, 1.f);
			DrawRect(TakeHudA::WithAlpha(bSeen ? TakeColors::RedDark : TakeColors::DarkGrey, 0.85f), BarX, BarY, BarW, BarH);
			if (Alpha > 0.f)
			{
				DrawRect(bSeen ? TakeColors::Red : TakeColors::Gold, BarX, BarY, BarW * Alpha, BarH);
			}
			TakeHudA::DrawFrame(*this, bSeen ? TakeColors::Red : TakeColors::Gold, BarX, BarY, BarW, BarH, FMath::Max(1.f, Ui));
		}
	}

	// --- The crew strip, top centre: "1 JULIAN   2 MAYA   3 VEX", the current one lit ------------------------------
	float CrewStripBottom = Margin + LineH;	// the objective goes below this
	{
		const ETakeCamera Ids[] = { ETakeCamera::A, ETakeCamera::B, ETakeCamera::C };
		const float Gap = 28.f * Ui;
		FString Texts[3];
		float Widths[3] = { 0.f, 0.f, 0.f };
		float TotalW = 0.f;
		int32 Shown = 0;
		for (int32 Index = 0; Index < 3; ++Index)
		{
			if (!Dir->GetCharacter(Ids[Index]))
			{
				continue;
			}
			Texts[Index] = FString::Printf(TEXT("%d %s"), Index + 1, TakeText::CrewName(Ids[Index]));
			float TextH = 0.f;
			GetTextSize(Texts[Index], Widths[Index], TextH, SmallFont, Ui);
			CrewStripBottom = FMath::Max(CrewStripBottom, Margin + TextH);
			TotalW += Widths[Index] + (Shown > 0 ? Gap : 0.f);
			++Shown;
		}
		float StripX = (ScreenW - TotalW) * 0.5f;
		for (int32 Index = 0; Index < 3; ++Index)
		{
			if (Texts[Index].IsEmpty())
			{
				continue;
			}
			DrawShadowedText(Texts[Index], Ids[Index] == Cam ? TakeColors::GoldBright : TakeColors::Grey, StripX, Margin,
				Ui, false);
			StripX += Widths[Index] + Gap;
		}
	}

	// --- Labels on set: watchers on every camera (dimmer and smaller on A, with bLabelsOnJulian), device pips on B
	//     and C only; nothing behind the camera or farther than LabelMaxDistance from it ------------------------------
	{
		FVector CamLoc = FVector::ZeroVector;
		FVector CamFwd = FVector::ZeroVector;
		const APlayerController* ViewPC = GetOwningPlayerController();
		const bool bHaveCam = ViewPC && ViewPC->PlayerCameraManager;
		if (bHaveCam)
		{
			CamLoc = ViewPC->PlayerCameraManager->GetCameraLocation();
			CamFwd = ViewPC->PlayerCameraManager->GetCameraRotation().Vector();
		}
		const float MaxDist = FMath::Max(0.f, bOnA ? JulianLabelMaxDistance : LabelMaxDistance);
		auto ShouldDrawWithin = [&](const FVector& Point, float Limit) -> bool
		{
			if (!bHaveCam)
			{
				return true;
			}
			const FVector ToPoint = Point - CamLoc;
			return ToPoint.SizeSquared() <= FMath::Square(static_cast<double>(Limit))
				&& FVector::DotProduct(ToPoint, CamFwd) > 0.0;
		};
		auto ShouldDrawAt = [&](const FVector& Point) -> bool
		{
			return ShouldDrawWithin(Point, MaxDist);
		};

		// Labels that would land on top of each other are stacked downwards (Vance and the bodyguard from the bridge).
		TArray<FVector2D, TInlineAllocator<16>> PlacedLabels;
		const FVector2D StackSize = LabelStackSize * ScreenFactor;
		auto PlaceLabel = [&](float X, float Y) -> float
		{
			for (int32 Try = 0; Try < 6; ++Try)
			{
				bool bClash = false;
				for (const FVector2D& Placed : PlacedLabels)
				{
					if (FMath::Abs(Placed.X - X) < StackSize.X && FMath::Abs(Placed.Y - Y) < StackSize.Y)
					{
						bClash = true;
						break;
					}
				}
				if (!bClash)
				{
					break;
				}
				Y += StackSize.Y;
			}
			PlacedLabels.Add(FVector2D(X, Y));
			return Y;
		};

		if (!bOnA || bLabelsOnJulian)
		{
			const float LabelScale = bOnA ? Ui * 0.8f : Ui;
			const bool bBlinkOn = BarClock ? BarClock->GetBeatPhase() < 0.5f : true;
			for (const TObjectPtr<AWatcher>& Watcher : Dir->Watchers)
			{
				if (!Watcher || Watcher->IsHidden())
				{
					continue;	// the guards who walk in at the alarm show no label before they arrive
				}
				const FVector LabelPoint = Watcher->GetLabelPoint();
				if (!ShouldDrawAt(LabelPoint)
					|| (!Watcher->HasArrived() && !ShouldDrawWithin(LabelPoint, FMath::Min(MaxDist, OffLabelMaxDistance))))
				{
					continue;
				}
				const FVector Proj = Project(LabelPoint);
				if (Proj.Z <= 0.f)
				{
					continue;
				}

				FString WText;
				FLinearColor WColour;
				if (!Watcher->HasArrived())
				{
					WText = FString::Printf(TEXT("%s - OFF"), *Watcher->GetLabel());
					WColour = TakeColors::Grey;
				}
				else
				{
					WText = FString::Printf(TEXT("%s - %s"), *Watcher->GetLabel(), *Watcher->GetStateText());
					WColour = Watcher->HasTally() ? Watcher->HoldingColour
						: (Watcher->IsLive() ? TakeColors::White : TakeColors::Grey);
				}
				float WAlpha = bOnA ? (Watcher->HasTally() ? 0.9f : 0.6f) : 1.f;
				if (Watcher->IsArrivalBlinking() && !bBlinkOn)
				{
					WAlpha *= 0.15f;
				}
				DrawShadowedText(WText, TakeHudA::WithAlpha(WColour, WAlpha), Proj.X, PlaceLabel(Proj.X, Proj.Y), LabelScale, true);
			}
		}

		if (!bOnA)
		{
			for (const TObjectPtr<ADevice>& Device : Dir->Devices)
			{
				if (!Device)
				{
					continue;
				}
				const FVector LabelPoint = Device->GetLabelPoint();
				if (!ShouldDrawAt(LabelPoint))
				{
					continue;
				}
				const FVector Proj = Project(LabelPoint);
				if (Proj.Z <= 0.f)
				{
					continue;
				}
				const FLinearColor DColour = Device->FiredFrom == Cam ? TakeColors::Gold : TakeColors::Grey;
				DrawShadowedText(Device->GetPipsText(), DColour, Proj.X, PlaceLabel(Proj.X, Proj.Y), Ui, true);
			}
		}
	}

	// --- Step 2: the objective, top centre under camera A's letterbox bar (the same height on B and C), white on a
	//     dark strip so it reads over anything ------------------------------------------------------------------------
	float ObjectiveBottom = CrewStripBottom;
	if (!Dir->Objective.IsEmpty())
	{
		const float ObjScale = ObjectiveScale * ScreenFactor;
		float ObjW = 0.f;
		float ObjH = 0.f;
		GetTextSize(Dir->Objective, ObjW, ObjH, SmallFont, ObjScale);
		const float PadX = 14.f * ScreenFactor;
		const float PadY = 5.f * ScreenFactor;
		const float StripY = FMath::Max(LetterboxBarH, CrewStripBottom) + Margin * 0.5f;
		const float StripW = ObjW + PadX * 2.f;
		const float StripH = ObjH + PadY * 2.f;
		DrawRect(TakeHudA::WithAlpha(TakeColors::Black, 0.6f), (ScreenW - StripW) * 0.5f, StripY, StripW, StripH);
		DrawShadowedText(Dir->Objective, TakeColors::White, ScreenW * 0.5f, StripY + PadY, ObjScale, true);
		ObjectiveBottom = StripY + StripH;
	}

	// --- Step 3: the alarm clock, centred under the objective: "ALARM" in the hold, then "ALARM   N BARS" and the
	//     seconds; red, alternating red and white with the beat in the last two bars -----------------------------------
	float AlarmClockBottom = 0.f;	// the flash lines and the scene title stay clear of it
	if (bAlarmShown)
	{
		const float AlarmTextScale = AlarmClockScale * ScreenFactor;
		const float AlarmClockY = FMath::Max(ObjectiveBottom, CrewStripBottom) + Margin * 0.5f;
		const int32 AlarmBarsLeft = Dir->GetAlarmBarsLeft();
		const bool bLastBars = Dir->bAlarmLive && AlarmBarsLeft <= 2;
		const bool bWhiteBeat = bLastBars && BarClock && (BarClock->GetBeatInBar() % 2) == 0;
		const FLinearColor AlarmColour = bWhiteBeat ? TakeColors::White : TakeColors::Red;
		const FString AlarmText = Dir->bAlarmLive
			? FString::Printf(TEXT("ALARM   %d %s"), AlarmBarsLeft, AlarmBarsLeft == 1 ? TEXT("BAR") : TEXT("BARS"))
			: FString(TEXT("ALARM"));
		const FVector2D AlarmDrawn = DrawTextWithFont(AlarmText, LargeFont, AlarmColour, ScreenW * 0.5f, AlarmClockY,
			AlarmTextScale, true);
		AlarmClockBottom = AlarmClockY + static_cast<float>(AlarmDrawn.Y);
		if (Dir->bAlarmLive)
		{
			const FVector2D AlarmSecondsDrawn = DrawShadowedText(
				FString::Printf(TEXT("%.1f s"), FMath::Max(0.f, Dir->GetAlarmSecondsLeft())), AlarmColour, ScreenW * 0.5f,
				AlarmClockBottom + 2.f * Ui, Ui * 1.1f, true);
			AlarmClockBottom += 2.f * Ui + static_cast<float>(AlarmSecondsDrawn.Y);
		}
	}

	// --- How-to-play line (bottom of the bottom band) --------------------------------------------------------------
	const float HowToY = ScreenH - Margin - LineH;
	DrawShadowedText(ControlsHint, TakeColors::Grey, ScreenW * 0.5f, HowToY, Ui * 0.9f, true);

	// --- Flash lines: centred low, newest at the bottom, just above the bottom band ---------------------------------
	{
		const int32 Count = Dir->Flashes.Num();
		float FlashY = BottomTop - Margin - LineH * 1.2f;
		for (int32 Index = Count - 1; Index >= 0; --Index)
		{
			// Never up into the objective (or the alarm clock): the oldest lines give way.
			if (FlashY < FMath::Max(ObjectiveBottom, AlarmClockBottom) + Margin * 0.5f)
			{
				break;
			}
			const FTakeFlash& Line = Dir->Flashes[Index];
			const float Alpha = FMath::Clamp(Line.SecondsLeft / 0.4f, 0.f, 1.f);
			DrawShadowedText(Line.Text, TakeHudA::WithAlpha(Line.Colour, Alpha), ScreenW * 0.5f, FlashY, Ui * 1.1f, true);
			FlashY -= LineH * 1.3f;
		}
	}

	// --- Blooper slate ----------------------------------------------------------------------------------------------
	if (Dir->Phase == ETakePhase::BlooperFreeze && Film.IsBlooperSlateDue(FilmFrame))
	{
		const float SlateH = ScreenH * 0.36f;
		const float SlateY = (ScreenH - SlateH) * 0.5f;
		DrawRect(TakeHudA::WithAlpha(TakeColors::Red, 0.92f), 0.f, SlateY, ScreenW, SlateH);
		TakeHudA::DrawFrame(*this, TakeColors::White, 0.f, SlateY, ScreenW, SlateH, 4.f * Ui);

		const FString Checkpoint = GI ? GI->CheckpointName : FString(TEXT("THE START"));
		const float Big = SlateTextScale * Ui * 0.5f;
		float TextY = SlateY + SlateH * 0.12f;
		const FVector2D TitleSize = DrawTextWithFont(FString::Printf(TEXT("BLOOPER - AGAIN FROM %s"), *Checkpoint), LargeFont,
			TakeColors::White, ScreenW * 0.5f, TextY, Big, true);
		TextY += TitleSize.Y + 10.f * Ui;
		const FVector2D ReasonSize = DrawShadowedText(TakeText::BlooperReasonName(Dir->LastBlooperReason),
			TakeColors::White, ScreenW * 0.5f, TextY, Ui * 1.2f, true);
		TextY += ReasonSize.Y + 10.f * Ui;
		DrawTextWithFont(TEXT("Cut!"), LargeFont, TakeColors::White, ScreenW * 0.5f, TextY, Big, true);
	}

	// --- CLEAR stamp ------------------------------------------------------------------------------------------------
	float ClearStampBottom = 0.f;	// the scene title goes below it
	if (Dir->Phase == ETakePhase::Cleared)
	{
		const float Big = SlateTextScale * Ui * 0.6f;
		float StampW = 0.f;
		float StampH = 0.f;
		GetTextSize(TEXT("CLEAR"), StampW, StampH, LargeFont, Big);
		const float Pad = 16.f * Ui;
		const float BoxW = StampW + Pad * 2.f;
		const float BoxH = StampH + Pad * 2.f;
		const float BoxX = (ScreenW - BoxW) * 0.5f;
		const float BoxY = FMath::Max3(TopBarH, ScreenH * 0.2f, ObjectiveBottom + Margin * 0.5f);
		DrawRect(TakeHudA::WithAlpha(TakeColors::Black, 0.5f), BoxX, BoxY, BoxW, BoxH);
		TakeHudA::DrawFrame(*this, TakeColors::Gold, BoxX, BoxY, BoxW, BoxH, 5.f * Ui);
		DrawTextWithFont(TEXT("CLEAR"), LargeFont, TakeColors::Gold, ScreenW * 0.5f, BoxY + Pad, Big, true);
		ClearStampBottom = BoxY + BoxH;
	}

	// --- Step 2: the scene title, gold, centred at a third of the height, fading over its last 0.5 s ----------------
	if (!Dir->Title.IsEmpty() && Dir->TitleSecondsLeft > 0.f)
	{
		const float TitleTextScale = TitleScale * ScreenFactor;
		float TitleW = 0.f;
		float TitleH = 0.f;
		GetTextSize(Dir->Title, TitleW, TitleH, LargeFont, TitleTextScale);
		float TitleY = ScreenH / 3.f - TitleH * 0.5f;
		TitleY = FMath::Max(TitleY, ObjectiveBottom + Margin * 0.5f);	// never over the objective
		if (AlarmClockBottom > 0.f)
		{
			TitleY = FMath::Max(TitleY, AlarmClockBottom + Margin * 0.5f);	// "ALARM" under the alarm clock
		}
		if (ClearStampBottom > 0.f)
		{
			TitleY = FMath::Max(TitleY, ClearStampBottom + Margin * 0.5f);	// "SCENE 2 CLEARED" under the stamp
		}
		const float TitleAlpha = FMath::Clamp(Dir->TitleSecondsLeft / 0.5f, 0.f, 1.f);
		DrawTextWithFont(Dir->Title, LargeFont, TakeHudA::WithAlpha(TakeColors::Gold, TitleAlpha), ScreenW * 0.5f, TitleY,
			TitleTextScale, true);
	}
}

ATakeDirector* ATakeHUD::GetDirector()
{
	if (!CachedDirector.IsValid())
	{
		CachedDirector = ATakeDirector::Get(this);
	}
	return CachedDirector.Get();
}

FVector2D ATakeHUD::DrawShadowedText(const FString& Text, const FLinearColor& Colour, float X, float Y, float Scale,
	bool bCentre)
{
	return DrawTextWithFont(Text, SmallFont, Colour, X, Y, Scale, bCentre);
}

FVector2D ATakeHUD::DrawTextWithFont(const FString& Text, UFont* Font, const FLinearColor& Colour, float X, float Y,
	float Scale, bool bCentre)
{
	if (!Canvas || Text.IsEmpty())
	{
		return FVector2D::ZeroVector;
	}
	UFont* UseFont = Font ? Font : (GEngine ? GEngine->GetMediumFont() : nullptr);
	float TextW = 0.f;
	float TextH = 0.f;
	GetTextSize(Text, TextW, TextH, UseFont, Scale);
	const float DrawX = bCentre ? X - TextW * 0.5f : X;
	const float Offset = FMath::Max(1.f, 2.f * Scale);
	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, Colour.A * 0.85f), DrawX + Offset, Y + Offset, UseFont, Scale);
	DrawText(Text, Colour, DrawX, Y, UseFont, Scale);
	return FVector2D(TextW, TextH);
}

FTakeFilmFrame ATakeHUD::MakeFilmFrame(const ATakeDirector& Dir, float ScreenW, float ScreenH) const
{
	FTakeFilmFrame F;
	F.ScreenW = ScreenW;
	F.ScreenH = ScreenH;
	const UWorld* World = GetWorld();
	F.RealTime = World ? static_cast<double>(World->GetRealTimeSeconds()) : 0.0;
	F.bFilmLook = Dir.bFilmLook;
	F.bFilmPicture = Dir.IsFilmPicture();
	F.LookCamera = Dir.GetLookCamera();
	const ATakePlayerController* TakePC = Cast<ATakePlayerController>(PlayerOwner);
	F.bInFlight = TakePC && TakePC->IsInFlight();
	F.Phase = Dir.Phase;
	F.Composure = Dir.Composure ? Dir.Composure->GetComposure() : 100.f;
	F.bSeen = Dir.bJulianSeen;
	if (Dir.Clock)
	{
		F.SecondsSinceBeat = Dir.Clock->GetBeatPhase() * Dir.Clock->GetSecondsPerBeat();
	}
	if (Dir.Julian && Canvas)
	{
		const FVector Projected = Canvas->Project(Dir.Julian->GetActorLocation());
		F.bJulianOnScreen = Projected.Z > 0.0 && Projected.X >= 0.0 && Projected.X <= ScreenW && Projected.Y >= 0.0
			&& Projected.Y <= ScreenH;
		F.JulianScreen = FVector2D(Projected.X, Projected.Y);
	}
	return F;
}
