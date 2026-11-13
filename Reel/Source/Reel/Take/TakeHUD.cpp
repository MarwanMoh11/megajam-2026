// Owner: (u). Constructor by the header pass; bodies by (u).

#include "TakeHUD.h"

#include "BarClockComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "ComposureComponent.h"
#include "Device.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "FollowSpot.h"
#include "Julian.h"
#include "Mark.h"
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

	// --- Camera look -----------------------------------------------------------------------------------------------
	float TopBarH = 0.f;
	float BottomTop = ScreenH;	// top of the bottom band (letterbox bar on A, a strip on B/C)
	const FLinearColor HeadColour = bOnA ? TakeColors::White : TakeColors::WorkBlue;

	if (bOnA)
	{
		const float BarH = LetterboxBarH;
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
				if (!Watcher)
				{
					continue;
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

	// --- How-to-play line (bottom of the bottom band) --------------------------------------------------------------
	const float HowToY = ScreenH - Margin - LineH;
	DrawShadowedText(ControlsHint, TakeColors::Grey, ScreenW * 0.5f, HowToY, Ui * 0.9f, true);

	// --- Flash lines: centred low, newest at the bottom, just above the bottom band ---------------------------------
	{
		const int32 Count = Dir->Flashes.Num();
		float FlashY = BottomTop - Margin - LineH * 1.2f;
		for (int32 Index = Count - 1; Index >= 0; --Index)
		{
			// Never up into the objective: the oldest lines give way.
			if (FlashY < ObjectiveBottom + Margin * 0.5f)
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
