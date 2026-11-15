// Scene 1's forged VIP card as a HUD insert, in the HUD's own look (QA Nov 15 evening: "fix UI for the cards in the
// bottom right, looks ugly"). Header only, drawn with the kit's tokens: an ink ticket with a gold rule, a torn-off
// stub behind a perforation, the house in Fell Roman, and VOID landing on it as one of the kit's stamps.
// Owner: hud-guidance.

#pragma once

#include "CoreMinimal.h"
#include "TakeHudKit.h"
#include "TakeStyle.h"

namespace TakeTicket
{
	/** The card at the picture's bottom right. Alpha 0..1 (with Kit.Fade), Drop: px it has fallen (exit), Jitter: px
	 *  sideways (the composure flicker), VoidAge: seconds since VOID landed (< 0: not void). */
	inline void Draw(FTakeHudKit& Kit, float Alpha, float Drop, float Jitter, float VoidAge)
	{
		using namespace TakeStyle;
		if (Alpha <= 0.f)
		{
			return;
		}
		const float CW = Kit.Px(420.f), CH = Kit.Px(150.f);
		const float StubW = Kit.Px(118.f);
		const float X = Kit.W - Kit.Px(56.f) - CW + Jitter;
		const float Y = Kit.PicBottom() - Kit.Px(44.f) - CH + Drop;
		const float Was = Kit.Fade;
		Kit.Fade = Was * FMath::Clamp(Alpha, 0.f, 1.f);

		Kit.Rect(X + Kit.Px(6.f), Y + Kit.Px(8.f), CW, CH, FLinearColor(0.f, 0.f, 0.f, 0.45f));	// its shadow
		Kit.Rect(X, Y, CW, CH, FTakeHudKit::A(Ink, 0.94f));
		Kit.Frame(X, Y, CW, CH, Kit.Ln(2.f), Julian);
		Kit.Frame(X + Kit.Px(7.f), Y + Kit.Px(7.f), CW - Kit.Px(14.f), CH - Kit.Px(14.f), Kit.Ln(1.f), FTakeHudKit::A(Bone, 0.5f));

		// The stub, behind a perforation.
		const float PX = X + StubW;
		for (float PY = Y + Kit.Px(14.f); PY < Y + CH - Kit.Px(16.f); PY += Kit.Px(11.f))
		{
			Kit.Rect(PX, PY, Kit.Ln(2.f), Kit.Px(5.f), FTakeHudKit::A(Bone, 0.65f));
		}
		Kit.Text(EType::Title, TEXT("VIP"), X + StubW * 0.5f, Y + CH * 0.44f, Julian, ETakeAlign::Centre);
		Kit.Text(EType::Tag, TEXT("No. 0417"), X + StubW * 0.5f, Y + CH * 0.74f, Ash, ETakeAlign::Centre);

		// The face.
		const float FX = PX + (X + CW - PX) * 0.5f;
		const float FW = X + CW - PX;
		Kit.Text(EType::Tag, TEXT("Admit one"), FX, Y + Kit.Px(32.f), Ash, ETakeAlign::Centre);
		Kit.Text(EType::Body, TEXT("The Grand Lumiere"), FX, Y + CH * 0.47f, Paper, ETakeAlign::Centre);
		const float RuleY = Y + CH * 0.66f;
		Kit.HairH(PX + Kit.Px(30.f), RuleY, FW - Kit.Px(60.f), FTakeHudKit::A(Julian, 0.7f));
		Kit.Diamond(PX + Kit.Px(30.f), RuleY, Kit.Px(7.f), Julian);
		Kit.Diamond(X + CW - Kit.Px(30.f), RuleY, Kit.Px(7.f), Julian);
		Kit.Text(EType::Tag, TEXT("Private sale  -  9 p.m."), FX, Y + CH * 0.82f, Bone, ETakeAlign::Centre);
		Kit.Fade = Was;

		if (VoidAge >= 0.f)
		{
			FTakeStampSpec Void;
			Void.Text = TEXT("VOID");
			Void.Colour = Danger;
			Void.Tilt = -8.f;
			Kit.Stamp(Void, FVector2D(FX, Y + CH * 0.5f), VoidAge, 60.f);
		}
	}
}
