// A rig's card on a crew feed (Scene 4's spotlight clamp and sandbag line): a leader from the rig to an ink card with
// its key, its name and what it does, in the HUD's own type and colours (QA Nov 15 evening: "fix UI for the cards in
// the bottom right, looks ugly"). Header only; the line is measured and wrapped, the card kept inside the picture at
// any size. Owner: hud-guidance.

#pragma once

#include "CoreMinimal.h"
#include "TakeHudKit.h"
#include "TakeStyle.h"

namespace TakeRigCard
{
	/** Anchor: the rig on screen. DX, DY: where the card sits from it, in 1080p px (DX < 0: the card to its left).
	 *  Key: its keycap ("" for none). Bar: 0..1 a draining timer under the line (< 0: none). TopClear / BottomClear:
	 *  px kept free above and below the picture (the feed's bands). */
	inline void Draw(FTakeHudKit& Kit, const FVector2D& Anchor, float DX, float DY, const FString& Key,
		const FString& Title, const FString& Line, const FLinearColor& Col, float Bar,
		float TopClear = 76.f, float BottomClear = 86.f)
	{
		using namespace TakeStyle;
		const float Pad = Kit.Px(18.f);
		const float CardMargin = Kit.Px(24.f);
		const float BW = FMath::Min(Kit.Px(480.f), Kit.W - 2.f * CardMargin);
		const float IW = BW - 2.f * Pad - Kit.Px(4.f);

		// The head: the keycap, then the name in caps (smaller caps when it would not fit).
		const FTakeInk KeyInk = FTakeInk::Of(Col, Col);
		const float KeyW = Key.IsEmpty() ? 0.f : Kit.KeyWidth(Key, false) + Kit.Px(12.f);
		const EType HeadType = Kit.Measure(EType::Label, Title) + KeyW <= IW ? EType::Label : EType::Tag;

		TArray<FString> Lines;
		Kit.Wrap(EType::Body, Line, IW, Lines);
		const float Pitch = Kit.Em(EType::Body) * 1.3f;
		const float HeadH = Kit.Px(40.f);
		const float BarH = Bar >= 0.f ? Kit.Px(18.f) : 0.f;
		const float BH = Pad + HeadH + Kit.Px(14.f) + Pitch * static_cast<float>(FMath::Max(Lines.Num(), 1)) + BarH + Pad * 0.6f;

		const float Top = Kit.PicY + Kit.Px(TopClear);
		const float Bottom = Kit.PicBottom() - Kit.Px(BottomClear);
		const float X = FMath::RoundToFloat(FMath::Clamp(Anchor.X + Kit.Px(DX), CardMargin, FMath::Max(CardMargin, Kit.W - CardMargin - BW)));
		const float Y = FMath::RoundToFloat(FMath::Clamp(Anchor.Y + Kit.Px(DY), Top, FMath::Max(Top, Bottom - BH)));

		// The leader: a hairline from the rig to the card's near edge, a diamond on the rig.
		const FVector2D End(DX < 0.f ? X + BW : X, Y + BH * 0.5f);
		const FVector2D D = End - Anchor;
		const float Len = static_cast<float>(D.Size());
		if (Len > Kit.Px(12.f))
		{
			const float Thick = Kit.Ln(2.f);
			Kit.Push(Anchor, FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X)), 1.f);
			Kit.Rect(Anchor.X, Anchor.Y - Thick * 0.5f, Len, Thick, FTakeHudKit::A(Col, 0.75f));
			Kit.Pop();
		}
		Kit.Diamond(Anchor.X, Anchor.Y, Kit.Px(18.f), FLinearColor(0.f, 0.f, 0.f, 0.55f));
		Kit.Diamond(Anchor.X, Anchor.Y, Kit.Px(12.f), Col);

		// The card: a shadow, an ink field, a hairline frame in its colour and a rule down its left edge.
		Kit.Rect(X + Kit.Px(5.f), Y + Kit.Px(6.f), BW, BH, FLinearColor(0.f, 0.f, 0.f, 0.35f));
		Kit.Rect(X, Y, BW, BH, FTakeHudKit::A(Ink, 0.9f));
		Kit.Frame(X, Y, BW, BH, Kit.Ln(1.f), FTakeHudKit::A(Col, 0.7f));
		Kit.Rect(X, Y, Kit.Px(4.f), BH, Col);

		const float LX = X + Kit.Px(4.f) + Pad;
		const float HeadY = Y + Pad + HeadH * 0.5f - Kit.Px(4.f);
		float TX = LX;
		if (!Key.IsEmpty())
		{
			TX += Kit.Key(Key, LX, HeadY, KeyInk) + Kit.Px(12.f);
		}
		Kit.Text(HeadType, Title, TX, HeadY, Col);
		const float RuleY = Y + Pad + HeadH;
		Kit.HairH(LX, RuleY, IW, FTakeHudKit::A(Bone, 0.3f));

		float LY = RuleY + Kit.Px(14.f) + Pitch * 0.5f;
		for (const FString& L : Lines)
		{
			Kit.Text(EType::Body, L, LX, LY, Paper);
			LY += Pitch;
		}
		if (Bar >= 0.f)
		{
			const float BY = LY - Pitch * 0.5f + Kit.Px(8.f);
			const float BarThick = Kit.Px(4.f);
			Kit.Rect(LX, BY, IW, BarThick, FTakeHudKit::A(Bone, 0.18f));
			Kit.Rect(LX, BY, FMath::RoundToFloat(IW * FMath::Clamp(Bar, 0.f, 1.f)), BarThick, Col);
		}
	}
}
