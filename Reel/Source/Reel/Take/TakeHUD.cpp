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
#include "Script/ScriptSequencer.h"


ATakeHUD::ATakeHUD()
{
	using namespace TakeStyle;
	// The script (docs/SCRIPT.md): the title card is THE DIRECTOR'S CUT, the crew are Vance, Max, Nora and Cole.
	GameTitle = TEXT("THE DIRECTOR'S CUT");
	ControlsHint = TEXT("[WASD] Walk   [MOUSE] Look   [E] Use   [R] Retake the scene");
	TitleTagline = TEXT("A 1970s heist thriller. Only one of them thinks it's a movie.");
	SceneNames = { TEXT("THE MARQUEE"), TEXT("THE VAN"), TEXT("THE FOYER"), TEXT("THE CATWALK"), TEXT("THE AUDITORIUM"),
		TEXT("BACKSTAGE"), TEXT("THE BOOTH"), TEXT("THE ALLEY") };
	TitlePitch = {
		TEXT("Arthur Pendleton bought the stolen master print of Le Soleil Noir to lock it away forever."),
		TEXT("Tonight, at the Grand Lumiere, the crew steals it back. You cut between them.") };
	TitleCast = {
		TEXT("A|VANCE|the star, in his own movie"),
		TEXT("B|NORA|the eye, up in the rafters"),
		TEXT("C|COLE|the muscle, backstage") };
	TitleRules = {
		TEXT("Vance sees a 35mm thriller. The crew see the raw feed."),
		TEXT("Blow a scene and the film burns: the scene goes again from the top.") };
	TitleControls = TEXT("[WASD] Walk   [MOUSE] Look   [E] Use   [R] Retake the scene   [ESC] Quit");
	// --- end W3 opening
	CreditLines = {
		TEXT("THE DIRECTOR'S CUT"),
		TEXT("A 1970s heist thriller."),
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

	FTakeFilmFrame FilmFrame = MakeFilmFrame(*Dir, Canvas->ClipX, Canvas->ClipY);
	UpdateStress(*Dir, FilmFrame);	// QA hud: composure as the picture (and the grade's Stress scalar)
	const float BarH = Film.DrawsLetterbox(FilmFrame) ? Film.BarHeight(FilmFrame)
		: (FilmFrame.bScriptRaw ? 0.f : TakeStyle::LetterboxBar(Canvas->ClipX, Canvas->ClipY, Film.LetterboxAspect));
	Kit.Begin(Canvas, FilmFrame.RealTime, BarH);
	TrackState(*Dir);
	ReadFlashes(*Dir);

	// Under everything: the picture's film or viewfinder, the burn, the splice, the bars in ink.
	Film.DrawUnder(*this, *Canvas, FilmFrame);
	if (!FilmFrame.bScriptRaw && (!Film.DrawsLetterbox(FilmFrame) || Kit.Bar > BarH + 0.5f))
	{
		using namespace TakeStyle;
		// The look is off, or the screen is wider than 2.39:1: the bars are still the HUD's lanes (MinBar tall).
		Kit.Rect(0.f, 0.f, Kit.W, Kit.Bar, Ink);
		Kit.Rect(0.f, Kit.PicBottom(), Kit.W, Kit.Bar, Ink);
	}
	// QA hud: composure is the picture: over the picture, under every word, on every camera, film look or not.
	if (bComposureInPicture && Dir->Phase != ETakePhase::Title && Dir->Phase != ETakePhase::Printed)
	{
		Film.DrawStress(*Canvas, FilmFrame, Kit.PicY, Kit.PicH);
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
	// The script's layer: the scene's own HUD, the slug, the splice, the retake, the end of the film.
	DrawScript(*Dir, FilmFrame);
}

// --- state the HUD keeps on its own clock -----------------------------------------------------------------------------------

void ATakeHUD::TrackState(const ATakeDirector& Dir)
{
	using namespace TakeStyle;
	if (Dir.Phase != LastPhase)
	{
		using namespace TakeStyle;
		const ETakePhase Before = LastPhase;
		LastPhase = Dir.Phase;
		PhaseSince = Kit.Now;
		if (Dir.Phase != ETakePhase::Rolling && Dir.Phase != ETakePhase::Cleared)
		{
			// A blooper, the title card or the ends: what was being said belongs to the take that stopped.
			Subtitles.Reset();
			Stamps.Reset();
			GoalFlash = FTakeShown();
			KeyPrompt = FTakeShown();
		}
		// QA hud: the take rolls again (Action!, or back from a blooper): the goal comes back for a moment.
		if (Dir.Phase == ETakePhase::Rolling)
		{
			GoalSince = Kit.Now;
			bGoalBrief = Before == ETakePhase::BlooperFreeze;
		}
	}
	const ETakeCamera Look = Dir.GetLookCamera();
	if (Look != LastLook)
	{
		using namespace TakeStyle;
		LastLook = Look;
		LookSince = Kit.Now;
		// QA hud: the lesson was learnt: the player cut to the camera it taught (the key is the camera's tab number).
		if (TeachKey >= 1 && TeachKey <= 3 && static_cast<int32>(Look) + 1 == TeachKey)
		{
			using namespace TakeStyle;
			++TaughtCount[TeachKey];
			TeachKey = 0;
		}
	}
	// QA hud: a goal is new when its words change (a countdown's digits ticking inside it are the same goal).
	const FString GoalWords = GoalKey(Dir.Objective);
	if (GoalWords != LastGoal)
	{
		using namespace TakeStyle;
		LastGoal = GoalWords;
		GoalSince = Kit.Now;
		bGoalBrief = false;
		// A goal that leads with a cut key is a lesson until the player has made that cut KeyTeachTimes times on cue.
		const int32 CutKey = LeadingCutKey(Dir.Objective);
		bTeaching = CutKey >= 1 && CutKey <= 3 && TaughtCount[CutKey] < KeyTeachTimes;
		TeachKey = bTeaching ? CutKey : 0;
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
			// QA hud: in the minimal HUD a note that leads with a keycap ("[2] Cut to MAYA") is a prompt: the bottom
			// lane, for the seconds it was flashed for, on real time (so a slowed world cannot stretch or cut it).
			if (bMinimalHud && Flash.Kind == ETakeFlashKind::Note && Flash.Text.TrimStart().StartsWith(TEXT("[")))
			{
				using namespace TakeStyle;
				Shown.Hold = FMath::Max(Flash.Seconds, FlashHoldSeconds);
				KeyPrompt = Shown;
				break;
			}
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
	return !Refusal.Text.IsEmpty() && Age(Refusal.Since) < RefusalHold();
}

float ATakeHUD::RefusalHold() const
{
	using namespace TakeStyle;
	// QA hud: in the minimal HUD, Enter's answer (the controls) is the only legend in play, so it holds longer.
	return bMinimalHud && Refusal.Text.StartsWith(TEXT("[WASD]")) ? FMath::Max(RefusalHoldSeconds, ControlsAnswerSeconds)
		: RefusalHoldSeconds;
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
	// QA hud: the minimal HUD keeps the goal line, the prompt lane and the alarm's seconds; each switch brings one back.
	if (Shows(bShowSlateStrip))
	{
		DrawSlateStrip(Dir);
	}
	if (Shows(bShowKeyTabs))
	{
		DrawKeyTabs(Dir);
	}
	DrawClock(Dir);
	DrawGoal(Dir);
	if (!bMinimalHud || Dir.Phase != ETakePhase::BlooperFreeze)
	{
		DrawB1(Dir, false);	// QA hud: on a blooper the lane is drawn once, with the slate (never twice over itself)
	}
	if (Shows(bShowComposureLine))
	{
		DrawComposure(Dir);
	}
	if (Shows(bShowControlsRow) && (Dir.Phase == ETakePhase::Rolling || Dir.Phase == ETakePhase::Cleared))
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

	if (Dir.bAlarm && Dir.bAlarmLive && !Shows(bShowFullAlarm))
	{
		using namespace TakeStyle;
		DrawAlarmNumber(Dir);	// QA hud: the one number the player needs
		return;
	}
	if (!(Dir.bAlarm && Dir.bAlarmLive) && !Shows(bShowRunningTime))
	{
		using namespace TakeStyle;
		return;	// QA hud: no running time in the minimal HUD
	}
	if (Dir.bAlarm && Dir.bAlarmLive)
	{
		using namespace TakeStyle;
		// The finale count: ALARM, the seconds left (whole seconds, rounded up; tenths in the last two) and a steady
		// bar that drains with them. Nothing blinks or keeps the beat.
		const int32 Count = FMath::Max(1, Dir.FinaleBars);
		const float Total = FMath::Max(0.1f, static_cast<float>(Count) * Clock->GetSecondsPerBar());
		const float SecondsLeft = FMath::Clamp(Dir.GetAlarmSecondsLeft(), 0.f, Total);
		const float Sq = Kit.Px(FinaleSquare);
		const float BarW = (Sq + Kit.Px(FinaleSquareGap)) * static_cast<float>(Count) - Kit.Px(FinaleSquareGap);
		const float BarX = Right - BarW;
		const float SqY = FMath::RoundToFloat(Y - Sq * 0.5f);
		const float FillW = FMath::RoundToFloat(BarW * SecondsLeft / Total);
		if (FillW > 0.f)
		{
			Kit.Rect(BarX, SqY, FillW, Sq, Danger);
		}
		Kit.Frame(BarX, SqY, BarW, Sq, Kit.Ln(LineThin), Silver);
		const float NumRight = BarX - Kit.Px(ClockPartGap);
		const bool bLastBar = SecondsLeft <= 2.f;
		const FString Numeral = bLastBar ? FString::Printf(TEXT("%.1f"), SecondsLeft)
			: FString::FromInt(FMath::CeilToInt(SecondsLeft));
		const float NumW = Kit.Text(EType::Title, Numeral, NumRight, Y, bLastBar ? Danger : Paper, ETakeAlign::Right);
		Kit.Text(EType::Label, TEXT("ALARM"), NumRight - NumW - Kit.Px(ClockPartGap), Y, Danger, ETakeAlign::Right);
		return;
	}

	// The take's running time on its clock, whole seconds. Nothing on screen keeps the beat (no beat squares, no
	// BREAK): the band is heard, as the warning, not watched.
	const int32 Whole = FMath::Max(0, FMath::FloorToInt(static_cast<float>(Clock->GetTotalBeats()) * Clock->GetSecondsPerBeat()));
	const float NumW = Kit.Text(EType::Label, FString::Printf(TEXT("%d:%02d"), Whole / 60, Whole % 60), Right, Y, Paper, ETakeAlign::Right);
	Kit.Text(EType::Tag, TEXT("TIME"), Right - NumW - Kit.Px(SlateCaptionGap), Y, Ash, ETakeAlign::Right);
}

void ATakeHUD::DrawGoal(const ATakeDirector& Dir)
{
	using namespace TakeStyle;
	const float Y = GoalRow();
	const float MaxW = Kit.Px(GoalMaxW);

	// A flash takes the goal line's place for FlashHoldSeconds: a square for arrivals, a T for his wins.
	if (!GoalFlash.Text.IsEmpty() && Age(GoalFlash.Since) < FlashHoldSeconds)
	{
		using namespace TakeStyle;
		if (bMinimalHud)
		{
			using namespace TakeStyle;
			// QA hud: no glyphs; the words carry the meaning in colour (gold his, red danger), and fade out.
			const FLinearColor Words = GoalFlash.Kind == ETakeFlashKind::Julian ? TakeStyle::Julian
				: (GoalFlash.Kind == ETakeFlashKind::Danger ? Danger : Paper);
			Kit.Fade = FMath::Min(FMath::Clamp(Age(GoalFlash.Since) / GoalFadeInSeconds, 0.f, 1.f),
				FadeOut(Age(GoalFlash.Since), FlashHoldSeconds, GoalFadeSeconds));
			Kit.Rich(EType::Goal, NoFinalStop(GoalFlash.Text), Kit.CX(), Y, MaxW, 1, FTakeInk::Of(Words, Words));
			Kit.Fade = 1.f;
			return;
		}
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
	// QA hud: the minimal goal line appears when the goal changes, holds, and fades; a lesson sits in the prompt lane.
	const float Alpha = bMinimalHud ? GoalAlpha(Dir) : 1.f;
	if (Alpha <= 0.f || IsKeyTeaching())
	{
		using namespace TakeStyle;
		return;
	}
	Kit.Fade = Alpha;
	float LastY = Y;
	const float Widest = Kit.Rich(EType::Goal, NoFinalStop(Dir.Objective), Kit.CX(), Y, MaxW, 2, FTakeInk::Of(Paper, Paper), &LastY);

	// A new goal: a bone rule under it draws out from the centre over GoalRuleFrames.
	const float Grow = FMath::Clamp(static_cast<float>(FilmFrame(Age(GoalSince)) + 1) / GoalRuleFrames, 0.f, 1.f);
	const float RuleW = FMath::RoundToFloat(Widest * Grow);
	Kit.Rect(FMath::RoundToFloat(Kit.CX() - RuleW * 0.5f), LastY + Kit.Px(GoalRuleUnder), RuleW, Kit.Ln(GoalRule), Bone);
	Kit.Fade = 1.f;
}

// --- QA hud: the minimal HUD -----------------------------------------------------------------------------------------------

float ATakeHUD::GoalRow() const
{
	using namespace TakeStyle;
	return bMinimalHud ? FMath::RoundToFloat(Kit.Bar * GoalRowInBar) : Kit.RowT2();
}

float ATakeHUD::PromptRow() const
{
	using namespace TakeStyle;
	return bMinimalHud ? FMath::RoundToFloat(Kit.PicBottom() + Kit.Bar * PromptRowInBar) : Kit.RowB1();
}

float ATakeHUD::FadeOut(float InAge, float Hold, float FadeSeconds)
{
	using namespace TakeStyle;
	// 1 until FadeSeconds before Hold, then linear to 0 at Hold.
	return FMath::Clamp((Hold - InAge) / FMath::Max(0.01f, FadeSeconds), 0.f, 1.f);
}

float ATakeHUD::GoalAlpha(const ATakeDirector& Dir) const
{
	using namespace TakeStyle;
	// The cleared scene's "[R] New take" and the blooper's freeze: the first holds, the second shows nothing.
	if (Dir.Phase == ETakePhase::Cleared || bShowGoalAlways)
	{
		using namespace TakeStyle;
		return 1.f;
	}
	if (Dir.Phase != ETakePhase::Rolling)
	{
		using namespace TakeStyle;
		return 0.f;
	}
	const float Since = Age(GoalSince);
	const float Hold = bGoalBrief ? GoalAfterBlooperSeconds : GoalHoldSeconds;
	const float In = FMath::Clamp(Since / FMath::Max(0.01f, GoalFadeInSeconds), 0.f, 1.f);
	return FMath::Min(In, FadeOut(Since, Hold + GoalFadeSeconds, GoalFadeSeconds));
}

FString ATakeHUD::GoalKey(const FString& In)
{
	using namespace TakeStyle;
	FString Out;
	Out.Reserve(In.Len());
	for (const TCHAR Ch : In)
	{
		using namespace TakeStyle;
		if (!FChar::IsDigit(Ch) && Ch != TEXT('.'))
		{
			Out.AppendChar(Ch);
		}
	}
	return Out;
}

int32 ATakeHUD::LeadingCutKey(const FString& In)
{
	using namespace TakeStyle;
	const FString Trimmed = In.TrimStart();
	if (Trimmed.Len() >= 3 && Trimmed[0] == TEXT('[') && Trimmed[2] == TEXT(']') && Trimmed[1] >= TEXT('1')
		&& Trimmed[1] <= TEXT('3'))
	{
		using namespace TakeStyle;
		return static_cast<int32>(Trimmed[1] - TEXT('0'));
	}
	return 0;
}

void ATakeHUD::DrawAlarmNumber(const ATakeDirector& Dir)
{
	using namespace TakeStyle;
	const UBarClockComponent* Clock = Dir.Clock;
	const int32 Count = FMath::Max(1, Dir.FinaleBars);
	const float Total = Clock ? FMath::Max(0.1f, static_cast<float>(Count) * Clock->GetSecondsPerBar()) : 1000.f;
	const float SecondsLeft = FMath::Clamp(Dir.GetAlarmSecondsLeft(), 0.f, Total);
	const bool bLast = SecondsLeft <= 2.f;
	const FString Numeral = bLast ? FString::Printf(TEXT("%.1f"), SecondsLeft) : FString::FromInt(FMath::CeilToInt(SecondsLeft));
	// The seconds alone, top right in the bar; red for the last two, with tenths.
	Kit.Text(EType::Title, Numeral, Kit.W - Kit.Px(Margin), FMath::RoundToFloat(Kit.Bar * 0.5f), bLast ? Danger : Paper,
		ETakeAlign::Right);
}

void ATakeHUD::DrawUsePrompt(const FTakeUseTarget& Use, float Y)
{
	using namespace TakeStyle;
	// Ready: paper; in reach but not ready: bone with silver keycaps. The device's uses as pips after the words.
	FTakeInk PromptInk = Use.bCanFire ? FTakeInk::Of(Paper, Paper) : FTakeInk::Of(Bone, Silver);
	PromptInk.bSmallKeys = true;
	const FString Words = NoFinalStop(Use.Prompt);
	const float MaxW = Kit.Px(UseMaxW);
	const ADevice* Device = Use.Device.Get();
	const int32 Uses = Device ? Device->Uses : 0;
	const bool bPips = Uses > 0 && Device->UsesLeft > 0;
	const float PipStep = Kit.Px(PipW) + Kit.Px(PipGap);
	const float PipsW = bPips ? Kit.Px(KeyTextGap) * 2.f + PipStep * static_cast<float>(Uses) - Kit.Px(PipGap) : 0.f;
	const float WordsW = FMath::Min(Kit.RichWidth(EType::Goal, Words, PromptInk), MaxW);
	const float Left = FMath::RoundToFloat(Kit.CX() - (WordsW + PipsW) * 0.5f);
	Kit.Rich(EType::Goal, Words, Left + WordsW * 0.5f, Y, MaxW, 1, PromptInk);
	if (bPips)
	{
		using namespace TakeStyle;
		float PX = Kit.Snap(Left + WordsW + Kit.Px(KeyTextGap) * 2.f);
		for (int32 Index = 0; Index < Uses; ++Index)
		{
			using namespace TakeStyle;
			Kit.Pip(PX, Y, Index < Device->UsesLeft, Use.bCanFire ? Paper : Bone, Silver);
			PX += PipStep;
		}
	}
}

void ATakeHUD::DrawPromptLane(const ATakeDirector& Dir, bool bRefusalsOnly)
{
	using namespace TakeStyle;
	// One small prompt at a time near the bottom centre, only while it applies. In order: why a press was refused (or
	// Enter's answer, the controls), a flashed key prompt, then by camera: Julian's mark action, else a cut key being
	// taught; the crew: a cut key being taught, else the Use prompt (with the device's uses as pips).
	const float Y = PromptRow();
	const float MaxW = Kit.Px(UseMaxW);
	if (IsRefusalShowing())
	{
		using namespace TakeStyle;
		FTakeInk RefusalInk = FTakeInk::Of(Bone, Bone);
		RefusalInk.bSmallKeys = true;
		Kit.Fade = FadeOut(Age(Refusal.Since), RefusalHold(), PromptFadeSeconds);
		Kit.Rich(EType::Goal, NoFinalStop(Refusal.Text), Kit.CX(), Y, IsControlsAnswer() ? Kit.W - 2.f * Kit.Px(Margin) : MaxW,
			1, RefusalInk);
		Kit.Fade = 1.f;
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
	FTakeInk Lesson = FTakeInk::Of(Paper, Paper);
	Lesson.bSmallKeys = true;
	if (!KeyPrompt.Text.IsEmpty() && Age(KeyPrompt.Since) < KeyPrompt.Hold)
	{
		using namespace TakeStyle;
		Kit.Fade = FadeOut(Age(KeyPrompt.Since), KeyPrompt.Hold, PromptFadeSeconds);
		Kit.Rich(EType::Goal, NoFinalStop(KeyPrompt.Text), Kit.CX(), Y, MaxW, 1, Lesson);
		Kit.Fade = 1.f;
		return;
	}
	const bool bLesson = IsKeyTeaching() && !Dir.Objective.IsEmpty();

	if (Dir.CurrentCamera == ETakeCamera::A)
	{
		using namespace TakeStyle;
		const AMark* ActionMark = Dir.Julian ? Dir.Julian->GetStandingMark() : nullptr;
		if (ActionMark && ActionMark->IsActionMark() && !ActionMark->IsActionDone())
		{
			using namespace TakeStyle;
			Kit.Text(EType::Goal, ActionMark->ActionVerb, Kit.CX(), Y, Paper, ETakeAlign::Centre);
			const float BW = Kit.Px(ActionBarW);
			const float BH = Kit.Ln(ActionBarH);
			const float BX = FMath::RoundToFloat(Kit.CX() - BW * 0.5f);
			const float BY = Y + Kit.Px(ActionBarUnder);
			Kit.Rect(BX, BY + BH * 0.5f, BW, Kit.Ln(LineHair), Silver);
			Kit.Rect(BX, BY, FMath::RoundToFloat(BW * FMath::Clamp(ActionMark->GetActionAlpha(), 0.f, 1.f)), BH,
				TakeStyle::Julian);
			return;
		}
		if (bLesson)
		{
			using namespace TakeStyle;
			Kit.Rich(EType::Goal, NoFinalStop(Dir.Objective), Kit.CX(), Y, MaxW, 1, Lesson);
		}
		return;
	}

	// The crew: a lesson always names another camera ("[1] Cut back to JULIAN"), so it wins over this one's device.
	if (bLesson)
	{
		using namespace TakeStyle;
		Kit.Rich(EType::Goal, NoFinalStop(Dir.Objective), Kit.CX(), Y, MaxW, 1, Lesson);
		return;
	}
	if (!Dir.CurrentUse.Prompt.IsEmpty())
	{
		using namespace TakeStyle;
		DrawUsePrompt(Dir.CurrentUse, Y);
	}
}

void ATakeHUD::DrawB1(const ATakeDirector& Dir, bool bRefusalsOnly)
{
	using namespace TakeStyle;
	if (bMinimalHud)
	{
		using namespace TakeStyle;
		DrawPromptLane(Dir, bRefusalsOnly);	// QA hud
		return;
	}
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
		Kit.GlyphDanger(SX + Half, Y, 1.f, false);
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
	// The script: Vance is the Final Cut (bars, grain, the film); the crew are the Uncut Feed (crisp, raw, no bars).
	if (Dir.bScriptDriven)
	{
		const AScriptSequencer* Seq = AScriptSequencer::Get(this);
		F.bScriptRaw = Seq && Seq->IsRunning() && Seq->GetPov() != EScriptPov::Vance;
		F.bFilmPicture = Dir.bFilmLook && !F.bScriptRaw;
	}
	const ATakePlayerController* TakePC = Cast<ATakePlayerController>(PlayerOwner);
	F.bInFlight = TakePC && TakePC->IsInFlight();
	F.Phase = Dir.Phase;
	F.Composure = Dir.Composure ? Dir.Composure->GetComposure() : 100.f;
	F.bSeen = Dir.bJulianSeen;
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
