// Owner: (u). Step 4 pass 2: the frame, the flashes and the bars (docs/ART_STYLE.md §15.5). The picture layer is in
// TakeHUDPicture.cpp, the cards in TakeHUDCards.cpp.

#include "TakeHUD.h"

#include "BarClockComponent.h"
#include "ComposureComponent.h"
#include "Device.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "Julian.h"
#include "Mark.h"
#include "TakeCharacter.h"
#include "TakeAudio.h"
#include "TakeDirector.h"
#include "TakeGameInstance.h"
#include "TakePlayerController.h"


ATakeHUD::ATakeHUD()
{
	using namespace TakeStyle;
	SceneNames = { TEXT("THE DOOR"), TEXT("THE FLOOR"), TEXT("THE GETAWAY") };
	TitlePitch = {
		TEXT("Tonight the Aurelia Auction House sells the only print of a lost film: the Director's Cut."),
		TEXT("You direct the crew that steals it, live, in one take.") };
	TitleCast = {
		TEXT("1|JULIAN|the actor, on the floor"),
		TEXT("2|MAYA|lights, up above"),
		TEXT("3|VEX|props and sound, backstage") };
	TitleRules = {
		TEXT("Get JULIAN in, lift the keycard, take the reel and get him out."),
		TEXT("[danger] Red sees him and burns his composure.   [julian] Gold is his: his marks, the reel, the way out.") };
	CreditLines = {
		TEXT("PRINT IT"),
		TEXT("A heist shot in one take."),
		TEXT("Directed by you."),
		TEXT("Made in Unreal Engine 5.9 for the Epic AI MegaJam 2026. Theme: Director's Cut."),
		TEXT("By Marwan, solo, with Claude agents."),
		TEXT("Thank you for playing.") };
}

void ATakeHUD::DrawHUD()
{
	using namespace TakeStyle;
	Super::DrawHUD();
	if (!Canvas)
	{
		using namespace TakeStyle;
		return;
	}
	ATakeDirector* Dir = GetDirector();
	if (!Dir)
	{
		using namespace TakeStyle;
		return;
	}

	const FTakeFilmFrame FilmFrame = MakeFilmFrame(*Dir, Canvas->ClipX, Canvas->ClipY);
	const float BarH = Film.DrawsLetterbox(FilmFrame) ? Film.BarHeight(FilmFrame)
		: TakeStyle::LetterboxBar(Canvas->ClipX, Canvas->ClipY, Film.LetterboxAspect);
	Kit.Begin(Canvas, FilmFrame.RealTime, FilmFrame.SecondsSinceBeat, BarH);
	TrackState(*Dir);
	ReadFlashes(*Dir);

	// Under everything: the picture's film or viewfinder, the burn, the splice, the bars in ink.
	Film.DrawUnder(*this, *Canvas, FilmFrame);
	if (!Film.DrawsLetterbox(FilmFrame) || Kit.Bar > BarH + 0.5f)
	{
		using namespace TakeStyle;
		// The look is off, or the screen is wider than 2.39:1: the bars are still the HUD's lanes (MinBar tall).
		Kit.Rect(0.f, 0.f, Kit.W, Kit.Bar, Ink);
		Kit.Rect(0.f, Kit.PicBottom(), Kit.W, Kit.Bar, Ink);
	}

	if (Dir->Phase == ETakePhase::Title)
	{
		using namespace TakeStyle;
		DrawTitleCard(*Dir, FilmFrame);
		DrawB1(*Dir, true);
		Film.DrawOver(*this, *Canvas, FilmFrame);
		return;
	}
	if (Dir->Phase == ETakePhase::Printed)
	{
		using namespace TakeStyle;
		DrawEnds(*Dir, FilmFrame);
		Film.DrawOver(*this, *Canvas, FilmFrame);
		return;
	}

	DrawPicture(*Dir, FilmFrame);
	DrawBars(*Dir);
	// Over the HUD: the blooper's burn and paper frames (and ink over the bars while frozen).
	Film.DrawOver(*this, *Canvas, FilmFrame);
	if (Dir->Phase == ETakePhase::BlooperFreeze && Film.IsBlooperSlateDue(FilmFrame))
	{
		using namespace TakeStyle;
		DrawBlooperSlate(*Dir);
		DrawB1(*Dir, true);
	}
}

// --- state the HUD keeps on its own clock -----------------------------------------------------------------------------------

void ATakeHUD::TrackState(const ATakeDirector& Dir)
{
	using namespace TakeStyle;
	if (Dir.Phase != LastPhase)
	{
		using namespace TakeStyle;
		LastPhase = Dir.Phase;
		PhaseSince = Kit.Now;
		if (Dir.Phase != ETakePhase::Rolling && Dir.Phase != ETakePhase::Cleared)
		{
			// A blooper, the title card or the ends: what was being said belongs to the take that stopped.
			Subtitles.Reset();
			Stamps.Reset();
			GoalFlash = FTakeShown();
		}
	}
	const ETakeCamera Look = Dir.GetLookCamera();
	if (Look != LastLook)
	{
		using namespace TakeStyle;
		LastLook = Look;
		LookSince = Kit.Now;
	}
	if (Dir.Objective != LastGoal)
	{
		using namespace TakeStyle;
		LastGoal = Dir.Objective;
		GoalSince = Kit.Now;
	}
	const float Composure = Dir.Composure ? Dir.Composure->GetComposure() : 100.f;
	if (LastComposure >= 0.f && Composure - LastComposure >= RefillMinJump)
	{
		using namespace TakeStyle;
		RefillFrom = LastComposure;
		RefillTo = Composure;
		RefillSince = Kit.Now;
	}
	LastComposure = Composure;
}

void ATakeHUD::ReadFlashes(const ATakeDirector& Dir)
{
	using namespace TakeStyle;
	for (const FTakeFlash& Flash : Dir.Flashes)
	{
		using namespace TakeStyle;
		if (Flash.Serial <= LastSerial)
		{
			continue;
		}
		LastSerial = Flash.Serial;
		FTakeShown Shown;
		Shown.Text = Flash.Text;
		Shown.Kind = Flash.Kind;
		Shown.Since = Kit.Now;
		Shown.Tab = Flash.Tab;
		switch (Flash.Kind)
		{
		case ETakeFlashKind::Refusal:
			Refusal = Shown;
			break;
		case ETakeFlashKind::Line:
		{
			FTakeSubtitle& Sub = Subtitles.AddDefaulted_GetRef();
			FTakeHudKit::SplitSpeaker(Flash.Text, Sub.Speaker, Sub.Line);
			Sub.Hold = SubtitleBaseSeconds + SubtitlePerCharSeconds * static_cast<float>(Sub.Line.Len());
			// Keep the line on screen and at most SubtitleQueueMax waiting: the oldest waiting one gives way.
			while (Subtitles.Num() > 1 + FMath::Max(0, SubtitleQueueMax))
			{
				Subtitles.RemoveAt(1);
			}
			break;
		}
		case ETakeFlashKind::Stamp:
		case ETakeFlashKind::Count:
			Stamps.Add(Shown);
			break;
		default:
			GoalFlash = Shown;	// a newer flash replaces an older one
			break;
		}
	}
	// A director that starts its serials again (a fresh director with this HUD): follow it down.
	if (Dir.FlashSerial < LastSerial)
	{
		using namespace TakeStyle;
		LastSerial = Dir.FlashSerial;
	}

	// Subtitles: the head holds its time, then the next one shows on that frame.
	while (Subtitles.Num() > 0)
	{
		using namespace TakeStyle;
		FTakeSubtitle& Head = Subtitles[0];
		if (Head.Since < 0.0)
		{
			Head.Since = Kit.Now;
			// Step 6: the line is spoken as it comes on screen, and holds for at least its voice.
			if (Dir.Audio)
			{
				const float Voice = Dir.Audio->SpeakSubtitle(Head.Speaker, Head.Line);
				if (Voice > 0.f)
				{
					Head.Hold = FMath::Max(Head.Hold, Voice + Dir.Audio->SubtitleVoicePad);
				}
			}
		}
		if (Kit.Now - Head.Since < Head.Hold)
		{
			break;
		}
		Subtitles.RemoveAt(0);
	}
	Stamps.RemoveAll([this](const FTakeShown& Shown)
	{
		using namespace TakeStyle;
		return Age(Shown.Since) > Frames(StampTitleHoldFrames + FramesSettle);
	});
}

bool ATakeHUD::IsControlsAnswer() const
{
	using namespace TakeStyle;
	// Enter during play answers with the controls (TakeDirectorPresses): B3 lights up instead of a second legend in B1.
	return IsRefusalShowing() && Refusal.Text.StartsWith(TEXT("[WASD]"));
}

FString ATakeHUD::NoFinalStop(const FString& In)
{
	using namespace TakeStyle;
	// One rule for T2 and B1: no final full stop (inner ones stay).
	return In.EndsWith(TEXT(".")) && !In.EndsWith(TEXT("..")) ? In.LeftChop(1) : In;
}

bool ATakeHUD::IsRefusalShowing() const
{
	using namespace TakeStyle;
	return !Refusal.Text.IsEmpty() && Age(Refusal.Since) < RefusalHoldSeconds;
}

FString ATakeHUD::SceneName(int32 Scene) const
{
	using namespace TakeStyle;
	return SceneNames.IsValidIndex(Scene - 1) ? SceneNames[Scene - 1] : FString();
}

// --- the bars ------------------------------------------------------------------------------------------------------------------

void ATakeHUD::DrawBars(const ATakeDirector& Dir)
{
	using namespace TakeStyle;
	DrawSlateStrip(Dir);
	DrawKeyTabs(Dir);
	DrawClock(Dir);
	DrawGoal(Dir);
	DrawB1(Dir, false);
	DrawComposure(Dir);
	if (Dir.Phase == ETakePhase::Rolling || Dir.Phase == ETakePhase::Cleared)
	{
		using namespace TakeStyle;
		// The only controls legend in play: paper while the opening slate runs (a cold start) and when Enter asks.
		const bool bLoud = IsControlsAnswer() || (Dir.bTitleIsSlate && Dir.TitleSecondsLeft > 0.f);
		FTakeInk Controls = bLoud ? FTakeInk::Of(Paper, Paper) : FTakeInk::Of(Ash, Ash);
		Controls.bSmallKeys = true;
		Kit.Rich(EType::Tag, ControlsHint, Kit.CX(), Kit.RowB3(), Kit.W - 2.f * Kit.Px(Margin), 1, Controls);
	}
}

void ATakeHUD::DrawSlateStrip(const ATakeDirector& Dir)
{
	using namespace TakeStyle;
	const UTakeGameInstance* GI = Dir.GetTakeGameInstance();
	const float Y = Kit.RowT1();
	float X = Kit.Px(Margin);
	X += Kit.Text(EType::Tag, TEXT("SCENE"), X, Y, Ash) + Kit.Px(SlateCaptionGap);
	const FString SceneValue = FString::Printf(TEXT("%d  %s"), Dir.Scene, *SceneName(Dir.Scene));
	X += Kit.Text(EType::Label, SceneValue, X, Y, Paper) + Kit.Px(SlateFieldGap);
	X += Kit.Text(EType::Tag, TEXT("TAKE"), X, Y, Ash) + Kit.Px(SlateCaptionGap);
	Kit.Text(EType::Label, FString::FromInt(GI ? GI->TakeNumber : 1), X, Y, Paper);
}

void ATakeHUD::DrawKeyTabs(const ATakeDirector& Dir)
{
	using namespace TakeStyle;
	const ETakeCamera Ids[] = { ETakeCamera::A, ETakeCamera::B, ETakeCamera::C };
	const ETakeCamera Active = Dir.GetLookCamera();
	const ATakePlayerController* TakePC = Cast<ATakePlayerController>(GetOwningPlayerController());
	const bool bInFlight = TakePC && TakePC->IsInFlight();
	const float Y = Kit.RowT1();

	float Widths[3] = { 0.f, 0.f, 0.f };
	float Total = 0.f;
	int32 Shown = 0;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		using namespace TakeStyle;
		if (!Dir.GetCharacter(Ids[Index]))
		{
			continue;
		}
		Widths[Index] = Kit.KeyWidth(TakeText::CameraKey(Ids[Index]), false) + Kit.Px(KeyTextGap)
			+ Kit.Measure(EType::Label, TakeText::CrewName(Ids[Index]));
		Total += Widths[Index] + (Shown > 0 ? Kit.Px(TabGap) : 0.f);
		++Shown;
	}

	float X = FMath::RoundToFloat(Kit.CX() - Total * 0.5f);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		using namespace TakeStyle;
		if (Widths[Index] <= 0.f)
		{
			continue;
		}
		const ETakeCamera Id = Ids[Index];
		const bool bActive = Id == Active;

		// A refused press shakes its tab 4 px left and right over 6 frames.
		float Shake = 0.f;
		if (IsRefusalShowing() && Refusal.Tab == Index + 1)
		{
			static const float Steps[] = { 1.f, -1.f, 1.f, -1.f, 0.5f, -0.5f };
			const int32 F = FilmFrame(Age(Refusal.Since));
			Shake = F >= 0 && F < 6 ? Steps[F] * Kit.Px(TabShakePx) : 0.f;
		}
		const float TX = X + Shake;

		FTakeInk Cap = bActive ? FTakeInk::Of(Paper, Paper) : FTakeInk::Of(Silver, Silver);
		if (bActive)
		{
			Cap.KeyFill = Paper;
			Cap.KeyLetter = Ink;
		}
		const float KW = Kit.Key(TakeText::CameraKey(Id), TX, Y, Cap);
		const float NameX = TX + KW + Kit.Px(KeyTextGap);
		const float NameW = Kit.Text(EType::Label, TakeText::CrewName(Id), NameX, Y, bActive ? Paper : Silver);

		// The flight: a paper line under the target's name grows over the flight's length.
		if (bInFlight && bActive)
		{
			const float Grow = FMath::Clamp(TakePC->GetFlightAlpha(), 0.f, 1.f);
			Kit.Rect(NameX, Y + Kit.Px(TabFlightUnder), FMath::RoundToFloat(NameW * Grow), Kit.Ln(TabFlightLine), Paper);
		}

		// Pips: one per use of each limited device this crew member works.
		if (Id != ETakeCamera::A)
		{
			TArray<bool, TInlineAllocator<8>> Pips;
			for (const TObjectPtr<ADevice>& Device : Dir.Devices)
			{
				if (Device && Device->FiredFrom == Id && Device->Uses > 0)
				{
					for (int32 Use = 0; Use < Device->Uses; ++Use)
					{
						Pips.Add(Use < Device->UsesLeft);
					}
				}
			}
			if (Pips.Num() > 0)
			{
				const float PipStep = Kit.Px(PipW) + Kit.Px(PipGap);
				const float PipsW = PipStep * static_cast<float>(Pips.Num()) - Kit.Px(PipGap);
				float PX = FMath::RoundToFloat(NameX + NameW * 0.5f - PipsW * 0.5f);
				const float PY = Y + Kit.Em(EType::Label) * 0.35f + Kit.Px(PipUnder) + Kit.Px(PipH) * 0.5f;
				for (const bool bUnused : Pips)
				{
					Kit.Pip(PX, PY, bUnused, Paper, Silver);
					PX += PipStep;
				}
			}
		}
		X += Widths[Index] + Kit.Px(TabGap);
	}
}

void ATakeHUD::DrawClock(const ATakeDirector& Dir)
{
	using namespace TakeStyle;
	const UBarClockComponent* Clock = Dir.Clock;
	if (!Clock)
	{
		using namespace TakeStyle;
		return;
	}
	const float Y = Kit.RowT1();
	const float Right = Kit.W - Kit.Px(Margin);

	if (Dir.bAlarm && Dir.bAlarmLive)
	{
		using namespace TakeStyle;
		// The finale count: ALARM, the bars left, a square for each bar (the current one blinks on the beat).
		const int32 Count = FMath::Max(1, Dir.FinaleBars);
		const int32 Left = FMath::Clamp(Dir.GetAlarmBarsLeft(), 0, Count);
		const float Sq = Kit.Px(FinaleSquare);
		const float Step = Sq + Kit.Px(FinaleSquareGap);
		float X = Right - (Step * static_cast<float>(Count) - Kit.Px(FinaleSquareGap));
		const float SqY = FMath::RoundToFloat(Y - Sq * 0.5f);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			if (Index < Left)
			{
				const float Blink = Index == Left - 1 ? Kit.Pulse() : 1.f;
				Kit.Rect(X, SqY, Sq, Sq, FTakeHudKit::A(Danger, Blink));
			}
			else
			{
				Kit.Frame(X, SqY, Sq, Sq, Kit.Ln(LineThin), Silver);
			}
			X += Step;
		}
		const float NumRight = Right - (Step * static_cast<float>(Count) - Kit.Px(FinaleSquareGap)) - Kit.Px(ClockPartGap);
		const bool bLastBar = Left <= 1;
		const FString Numeral = bLastBar ? FString::Printf(TEXT("%.1f"), FMath::Max(0.f, Dir.GetAlarmSecondsLeft()))
			: FString::FromInt(Left);
		const float NumW = Kit.Text(EType::Title, Numeral, NumRight, Y, bLastBar ? Danger : Paper, ETakeAlign::Right);
		Kit.Text(EType::Label, TEXT("ALARM"), NumRight - NumW - Kit.Px(ClockPartGap), Y, Danger, ETakeAlign::Right);
		return;
	}

	// The bar clock: BAR, the bar's number, four beat squares (BREAK on the band's break).
	float PartsLeft = Right;
	if (Clock->IsMetronomeMuted())
	{
		using namespace TakeStyle;
		PartsLeft -= Kit.Text(EType::Label, TEXT("BREAK"), Right, Y, Bone, ETakeAlign::Right);
	}
	else
	{
		const int32 Beat = Clock->GetBeatInBar();	// 1..4
		const float Sq = Kit.Px(ClockSquare);
		const float Step = Sq + Kit.Px(ClockSquareGap);
		float X = Right - (Step * 4.f - Kit.Px(ClockSquareGap));
		PartsLeft = X;
		const float SqY = FMath::RoundToFloat(Y - Sq * 0.5f);
		for (int32 Index = 0; Index < 4; ++Index)
		{
			if (Index < Beat)
			{
				Kit.Rect(X, SqY, Sq, Sq, Paper);
			}
			else
			{
				Kit.Frame(X, SqY, Sq, Sq, Kit.Ln(LineHair), Silver);
			}
			X += Step;
		}
	}
	const float NumRight = PartsLeft - Kit.Px(ClockPartGap);
	const float NumW = Kit.Text(EType::Label, FString::FromInt(Clock->GetBar()), NumRight, Y, Paper, ETakeAlign::Right);
	Kit.Text(EType::Tag, TEXT("BAR"), NumRight - NumW - Kit.Px(SlateCaptionGap), Y, Ash, ETakeAlign::Right);
}

void ATakeHUD::DrawGoal(const ATakeDirector& Dir)
{
	using namespace TakeStyle;
	const float Y = Kit.RowT2();
	const float MaxW = Kit.Px(GoalMaxW);

	// A flash takes the goal line's place for FlashHoldSeconds: a square for arrivals, a T for his wins.
	if (!GoalFlash.Text.IsEmpty() && Age(GoalFlash.Since) < FlashHoldSeconds)
	{
		using namespace TakeStyle;
		FString Line = GoalFlash.Text;
		if (GoalFlash.Kind == ETakeFlashKind::Danger)
		{
			Line = TEXT("[danger] ") + Line;
		}
		else if (GoalFlash.Kind == ETakeFlashKind::Julian)
		{
			Line = TEXT("[julian] ") + Line;
		}
		Kit.Rich(EType::Goal, NoFinalStop(Line), Kit.CX(), Y, MaxW, 1, FTakeInk::Of(Paper, Paper));
		return;
	}
	if (Dir.Objective.IsEmpty())
	{
		using namespace TakeStyle;
		return;
	}
	float LastY = Y;
	const float Widest = Kit.Rich(EType::Goal, NoFinalStop(Dir.Objective), Kit.CX(), Y, MaxW, 2, FTakeInk::Of(Paper, Paper), &LastY);

	// A new goal: a bone rule under it draws out from the centre over GoalRuleFrames.
	const float Grow = FMath::Clamp(static_cast<float>(FilmFrame(Age(GoalSince)) + 1) / GoalRuleFrames, 0.f, 1.f);
	const float RuleW = FMath::RoundToFloat(Widest * Grow);
	Kit.Rect(FMath::RoundToFloat(Kit.CX() - RuleW * 0.5f), LastY + Kit.Px(GoalRuleUnder), RuleW, Kit.Ln(GoalRule), Bone);
}

void ATakeHUD::DrawB1(const ATakeDirector& Dir, bool bRefusalsOnly)
{
	using namespace TakeStyle;
	const float Y = Kit.RowB1();
	const float MaxW = Kit.Px(UseMaxW);
	if (IsRefusalShowing() && !IsControlsAnswer())
	{
		using namespace TakeStyle;
		Kit.Rich(EType::Goal, NoFinalStop(Refusal.Text), Kit.CX(), Y, MaxW, 1, FTakeInk::Of(Bone, Bone));
		return;
	}
	if (bRefusalsOnly || Dir.Phase != ETakePhase::Rolling)
	{
		using namespace TakeStyle;
		return;
	}
	const ATakePlayerController* TakePC = Cast<ATakePlayerController>(GetOwningPlayerController());
	if (TakePC && TakePC->IsInFlight())
	{
		using namespace TakeStyle;
		return;
	}

	// Julian's camera: his mark action and its gold progress bar.
	if (Dir.CurrentCamera == ETakeCamera::A)
	{
		using namespace TakeStyle;
		const AMark* ActionMark = Dir.Julian ? Dir.Julian->GetStandingMark() : nullptr;
		if (ActionMark && ActionMark->IsActionMark() && !ActionMark->IsActionDone())
		{
			Kit.Text(EType::Goal, ActionMark->ActionVerb, Kit.CX(), Y, Paper, ETakeAlign::Centre);
			const float BW = Kit.Px(ActionBarW);
			const float BH = Kit.Ln(ActionBarH);
			const float BX = FMath::RoundToFloat(Kit.CX() - BW * 0.5f);
			const float BY = Y + Kit.Px(ActionBarUnder);
			Kit.Rect(BX, BY + BH * 0.5f, BW, Kit.Ln(LineHair), Silver);
			Kit.Rect(BX, BY, FMath::RoundToFloat(BW * FMath::Clamp(ActionMark->GetActionAlpha(), 0.f, 1.f)), BH,
				TakeStyle::Julian);
		}
		return;
	}

	// The crew: the Use prompt. Ready: paper; in reach but not ready: bone with a silver keycap and the reason.
	const FTakeUseTarget& Use = Dir.CurrentUse;
	if (!Use.Prompt.IsEmpty())
	{
		using namespace TakeStyle;
		Kit.Rich(EType::Goal, NoFinalStop(Use.Prompt), Kit.CX(), Y, MaxW, 1,
			Use.bCanFire ? FTakeInk::Of(Paper, Paper) : FTakeInk::Of(Bone, Silver));
	}
}

void ATakeHUD::DrawComposure(const ATakeDirector& Dir)
{
	using namespace TakeStyle;
	const UComposureComponent* Comp = Dir.Composure;
	const float Fraction = Comp ? FMath::Clamp(Comp->GetFraction(), 0.f, 1.f) : 1.f;
	const EComposureLabel Label = Comp ? Comp->GetLabel() : EComposureLabel::None;
	const float Y = Kit.RowB2();
	const float LW = Kit.Px(ComposureW);
	const float LX = FMath::RoundToFloat(Kit.CX() - LW * 0.5f);
	const float LH = Kit.Ln(LineHeavy);
	const float LY = FMath::RoundToFloat(Y - LH * 0.5f);

	Kit.Text(EType::Label, TEXT("COMPOSURE"), LX - Kit.Px(ComposureLabelGap), Y, Bone, ETakeAlign::Right);
	Kit.Rect(LX, FMath::RoundToFloat(Y - Kit.Ln(LineHair) * 0.5f), LW, Kit.Ln(LineHair), Silver);

	// SEEN: the fill swaps danger and gold every 6 frames; otherwise gold.
	FLinearColor Fill = TakeStyle::Julian;
	if (Label == EComposureLabel::Seen && (FilmFrame(Kit.Now) / static_cast<int32>(FramesSettle)) % 2 == 0)
	{
		using namespace TakeStyle;
		Fill = Danger;
	}
	const float FillW = FMath::RoundToFloat(LW * Fraction);
	Kit.Rect(LX, LY, FillW, LH, Fill);

	// A refill (x2): the gained part in paper for 12 frames.
	if (Age(RefillSince) < Frames(FramesBeat) && RefillTo > RefillFrom)
	{
		using namespace TakeStyle;
		const float From = FMath::RoundToFloat(LW * FMath::Clamp(RefillFrom / 100.f, 0.f, 1.f));
		Kit.Rect(LX + From, LY, FMath::Max(0.f, FillW - From), LH, Paper);
	}

	const float SX = LX + LW + Kit.Px(ComposureLabelGap);
	const float Half = Kit.Px(GlyphSquare) * 0.5f;
	if (Label == EComposureLabel::Seen)
	{
		using namespace TakeStyle;
		Kit.GlyphDanger(SX + Half, Y, Kit.Pulse(), false);
		Kit.Text(EType::Label, Comp->GetLabelText(), SX + Half * 2.f + Kit.Px(GlyphTextGap), Y, Danger);
	}
	else if (Label == EComposureLabel::Mark)
	{
		using namespace TakeStyle;
		Kit.GlyphJulian(SX + Kit.GlyphW() * 0.5f, Y, 1.f);
		Kit.Text(EType::Label, Comp->GetLabelText(), SX + Kit.GlyphW() + Kit.Px(GlyphTextGap), Y, TakeStyle::Julian);
	}
}

// --- plumbing -----------------------------------------------------------------------------------------------------------------

ATakeDirector* ATakeHUD::GetDirector()
{
	using namespace TakeStyle;
	if (!CachedDirector.IsValid())
	{
		using namespace TakeStyle;
		CachedDirector = ATakeDirector::Get(this);
	}
	return CachedDirector.Get();
}

FTakeFilmFrame ATakeHUD::MakeFilmFrame(const ATakeDirector& Dir, float ScreenW, float ScreenH) const
{
	using namespace TakeStyle;
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
		using namespace TakeStyle;
		F.SecondsSinceBeat = Dir.Clock->GetBeatPhase() * Dir.Clock->GetSecondsPerBeat();
	}
	if (Dir.Julian && Canvas)
	{
		using namespace TakeStyle;
		const FVector Projected = Canvas->Project(Dir.Julian->GetActorLocation());
		F.bJulianOnScreen = Projected.Z > 0.0 && Projected.X >= 0.0 && Projected.X <= ScreenW && Projected.Y >= 0.0
			&& Projected.Y <= ScreenH;
		F.JulianScreen = FVector2D(Projected.X, Projected.Y);
	}
	return F;
}
