// Owner: (a). Constructor by the header pass; bodies by (a).

#include "TakeHUD.h"

#include "BarClockComponent.h"
#include "ComposureComponent.h"
#include "Device.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "FollowSpot.h"
#include "Julian.h"
#include "Mark.h"
#include "TakeDirector.h"
#include "TakeGameInstance.h"
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
	const float Ui = TextScale * FMath::Max(0.75f, ScreenH / 900.f);
	const float Margin = 18.f * Ui;
	const float LineH = 24.f * Ui;

	const ETakeCamera Cam = Dir->CurrentCamera;
	const bool bOnA = Cam == ETakeCamera::A;
	UBarClockComponent* BarClock = Dir->Clock;
	UComposureComponent* Comp = Dir->Composure;
	const UTakeGameInstance* GI = Dir->GetTakeGameInstance();
	const int32 TakeN = GI ? GI->TakeNumber : 1;
	const FString Tc = BarClock ? BarClock->GetTimecode() : FString(TEXT("-.-"));

	// --- Camera look -----------------------------------------------------------------------------------------------
	float TopBarH = 0.f;
	float BottomTop = ScreenH;	// top of the bottom band (letterbox bar on A, a strip on B/C)
	const FLinearColor HeadColour = bOnA ? TakeColors::White : TakeColors::WorkBlue;

	if (bOnA)
	{
		const float Aspect = FMath::Max(1.f, LetterboxAspect);
		const float BarH = FMath::Max(0.f, (ScreenH - ScreenW / Aspect) * 0.5f);
		TopBarH = BarH;
		BottomTop = ScreenH - BarH;
		DrawRect(TakeColors::Black, 0.f, 0.f, ScreenW, BarH);
		DrawRect(TakeColors::Black, 0.f, BottomTop, ScreenW, BarH);

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

	DrawShadowedText(FString::Printf(TEXT("TAKE %d"), TakeN), TakeColors::White, DotX + 18.f * Ui, Margin, Ui, false);

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

	// --- The count at Julian's cued mark (every camera) ------------------------------------------------------------
	AJulian* Jul = Dir->Julian;
	if (Jul && BarClock && Jul->IsWalking())
	{
		if (AMark* Cued = Jul->GetCuedMark())
		{
			const FVector Proj = Project(Cued->GetStandPoint() + FVector(0.f, 0.f, 40.f));
			if (Proj.Z > 0.f)
			{
				const int32 Beats = FMath::Max(0, FMath::CeilToInt(Jul->GetBeatsToArrival(BarClock->GetSecondsPerBeat())));
				DrawShadowedText(FString::Printf(TEXT("in %d"), Beats), TakeColors::GoldBright, Proj.X, Proj.Y - LineH,
					Ui * 1.2f, true);
			}
		}
	}

	// --- Labels on set (B and C only) ------------------------------------------------------------------------------
	if (!bOnA)
	{
		for (const TObjectPtr<AWatcher>& Watcher : Dir->Watchers)
		{
			if (!Watcher)
			{
				continue;
			}
			const FVector Proj = Project(Watcher->GetLabelPoint());
			if (Proj.Z <= 0.f)
			{
				continue;
			}
			const FLinearColor WColour = Watcher->HasTally() ? TakeColors::Red
				: (Watcher->IsLive() ? TakeColors::White : TakeColors::Grey);
			DrawShadowedText(FString::Printf(TEXT("%s - %s"), *Watcher->GetLabel(), *Watcher->GetStateText()), WColour,
				Proj.X, Proj.Y, Ui, true);
		}

		for (const TObjectPtr<ADevice>& Device : Dir->Devices)
		{
			if (!Device)
			{
				continue;
			}
			const FVector Proj = Project(Device->GetLabelPoint());
			if (Proj.Z <= 0.f)
			{
				continue;
			}
			const FLinearColor DColour = Device->FiredFrom == Cam ? TakeColors::Gold : TakeColors::Grey;
			DrawShadowedText(Device->GetPipsText(), DColour, Proj.X, Proj.Y, Ui, true);
		}
	}

	// --- How-to-play line (bottom of the bottom band) --------------------------------------------------------------
	const TCHAR* HowTo = bOnA
		? TEXT("Click a gold mark to cue Julian   [2] Maya")
		: (Cam == ETakeCamera::B
			? TEXT("Click a device to fire, a watcher to spot him   [1] Julian")
			: TEXT("Click a device to fire   [1] Julian   [2] Maya"));
	const float HowToY = ScreenH - Margin - LineH;
	DrawShadowedText(HowTo, TakeColors::Grey, ScreenW * 0.5f, HowToY, Ui * 0.9f, true);

	// --- Flash lines: centred low, newest at the bottom, just above the bottom band ---------------------------------
	{
		const int32 Count = Dir->Flashes.Num();
		float FlashY = BottomTop - Margin - LineH * 1.2f;
		for (int32 Index = Count - 1; Index >= 0; --Index)
		{
			const FTakeFlash& Line = Dir->Flashes[Index];
			const float Alpha = FMath::Clamp(Line.SecondsLeft / 0.4f, 0.f, 1.f);
			DrawShadowedText(Line.Text, TakeHudA::WithAlpha(Line.Colour, Alpha), ScreenW * 0.5f, FlashY, Ui * 1.1f, true);
			FlashY -= LineH * 1.3f;
		}
	}

	// --- Blooper slate ----------------------------------------------------------------------------------------------
	if (Dir->Phase == ETakePhase::BlooperFreeze)
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
		const float BoxY = FMath::Max(TopBarH, ScreenH * 0.2f);
		DrawRect(TakeHudA::WithAlpha(TakeColors::Black, 0.5f), BoxX, BoxY, BoxW, BoxH);
		TakeHudA::DrawFrame(*this, TakeColors::Gold, BoxX, BoxY, BoxW, BoxH, 5.f * Ui);
		DrawTextWithFont(TEXT("CLEAR"), LargeFont, TakeColors::Gold, ScreenW * 0.5f, BoxY + Pad, Big, true);
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
