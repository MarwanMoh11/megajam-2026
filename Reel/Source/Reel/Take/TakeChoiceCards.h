// Dialogue choice cards (QA Nov 15 evening, hud-guidance: "the dialogue option text overflows outside the box").
// Header only, so a scene can switch to it with Live Coding. Every card is measured: its line wraps to the card's
// width, the cards grow down to fit the longest wrap, and when a row can't hold them at a readable width (a narrow or
// letterboxed picture) they stack in a column instead. Owner: hud-guidance.

#pragma once

#include "CoreMinimal.h"
#include "TakeHudKit.h"
#include "TakeStyle.h"

struct FTakeChoiceCard
{
	/** The keycap ("1"). */
	FString Key;
	/** The option's name, type.label ("PLAY ALONG"). */
	FString Head;
	/** What Vance will say, without quotes; drawn in quotes, wrapped. */
	FString Line;
};

namespace TakeChoice
{
	/** Draws the cards, Lit (-1: none) in gold. BottomY: the block's bottom edge (the hint goes under it). Caption sits
	 *  over the block, Hint under it (either may be empty). Kit.Fade applies. Returns the block's top. */
	inline float Draw(FTakeHudKit& Kit, const TArray<FTakeChoiceCard>& Cards, int32 Lit, float BottomY,
		const FString& Caption, const FString& Hint)
	{
		using namespace TakeStyle;
		const int32 N = Cards.Num();
		if (N == 0)
		{
			return BottomY;
		}
		const float Pad = Kit.Px(18.f);
		const float Gap = Kit.Px(28.f);
		const float CardMargin = Kit.Px(48.f);
		const float Avail = FMath::Max(Kit.W - 2.f * CardMargin, Kit.Px(200.f));
		const float MinCardW = Kit.Px(320.f);
		const float MaxCardW = Kit.Px(620.f);
		const float Pitch = Kit.Em(EType::Line) * 1.32f;
		const float FirstLine = Kit.Px(74.f);	// the first line's centre below the card's top (the key row is at 30)

		// The widest thing any card wants on one line.
		TArray<FString> Quoted;
		float Want = MinCardW;
		for (const FTakeChoiceCard& C : Cards)
		{
			const FString Q = FString::Printf(TEXT("\"%s\""), *C.Line);
			Quoted.Add(Q);
			const float HeadW = Kit.KeyWidth(C.Key, false) + Kit.Px(12.f) + Kit.Measure(EType::Label, C.Head);
			Want = FMath::Max(Want, FMath::Max(HeadW, Kit.Measure(EType::Line, Q)) + 2.f * Pad);
		}

		// A row when each card still gets MinCardW; otherwise a column.
		const float RowEach = (Avail - Gap * (N - 1)) / N;
		const bool bRow = RowEach >= MinCardW;
		const float CardW = bRow ? FMath::Min(FMath::Min(Want, MaxCardW), RowEach)
			: FMath::Min(FMath::Max(Want, MinCardW), FMath::Min(Avail, Kit.Px(760.f)));

		// Wrap every line to the card and take the tallest.
		TArray<TArray<FString>> Wrapped;
		int32 MostLines = 1;
		for (const FString& Q : Quoted)
		{
			TArray<FString>& L = Wrapped.AddDefaulted_GetRef();
			Kit.Wrap(EType::Line, Q, CardW - 2.f * Pad, L);
			if (L.Num() == 0)
			{
				L.Add(Q);
			}
			MostLines = FMath::Max(MostLines, L.Num());
		}
		const float CardH = FirstLine + Pitch * (MostLines - 1) + Kit.Px(44.f);
		const float BlockH = bRow ? CardH : CardH * N + Gap * 0.5f * (N - 1);
		const float Top = BottomY - BlockH;

		if (!Caption.IsEmpty())
		{
			Kit.Text(EType::Label, Caption, Kit.CX(), Top - Kit.Px(26.f), Bone, ETakeAlign::Centre);
		}
		const float RowW = CardW * N + Gap * (N - 1);
		for (int32 i = 0; i < N; ++i)
		{
			const bool bOn = Lit == i;
			const float X = bRow ? Kit.CX() - RowW * 0.5f + i * (CardW + Gap) : Kit.CX() - CardW * 0.5f;
			const float Y = bRow ? Top : Top + i * (CardH + Gap * 0.5f);
			Kit.Rect(X, Y, CardW, CardH, FLinearColor(0.f, 0.f, 0.f, bOn ? 0.78f : 0.55f));
			Kit.Frame(X, Y, CardW, CardH, Kit.Ln(bOn ? 2.f : 1.f), bOn ? Julian : Silver);
			const FLinearColor HeadC = bOn ? Julian : Bone;
			const float KX = X + Pad;
			const float KW = Kit.Key(Cards[i].Key, KX, Y + Kit.Px(30.f), FTakeInk::Of(HeadC, HeadC));
			Kit.Text(EType::Label, Cards[i].Head, KX + KW + Kit.Px(12.f), Y + Kit.Px(30.f), HeadC);
			float LineY = Y + FirstLine;
			for (const FString& L : Wrapped[i])
			{
				Kit.Text(EType::Line, L, X + Pad, LineY, bOn ? Paper : Ash);
				LineY += Pitch;
			}
		}
		if (!Hint.IsEmpty())
		{
			Kit.Text(EType::Tag, Hint, Kit.CX(), BottomY + Kit.Px(22.f), Silver, ETakeAlign::Centre);
		}
		return Top;
	}
}
