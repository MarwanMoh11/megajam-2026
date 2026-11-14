// Owner: (u). Step 4 pass 2: the picture layer (docs/ART_STYLE.md §15.4 reticle, §15.5 labels and subtitles, §15.6
// stamps, scene titles and the opening slate).

#include "TakeHUD.h"

#include "Camera/PlayerCameraManager.h"
#include "Device.h"
#include "Engine/Canvas.h"
#include "TakeDirector.h"
#include "TakeGameInstance.h"
#include "TakePlayerController.h"
#include "Watcher.h"

using namespace TakeStyle;

namespace TakeHudLabels
{
	/** One world label waiting for its place (§15.5's label rule). */
	struct FCand
	{
		FVector World = FVector::ZeroVector;
		FVector2D Screen = FVector2D::ZeroVector;
		float Dist = 0.f;
		int32 Prio = 6;
		FString Name;
		FString State;
		bool bWatcher = false;
		bool bLive = false;
		float Countdown = -1.f;
		const ADevice* Device = nullptr;
		int32 Count = 1;
	};

	bool Overlaps(const FVector4f& A, const FVector4f& B, float Gap)
	{
		return A.X < B.X + B.Z + Gap && B.X < A.X + A.Z + Gap && A.Y < B.Y + B.W + Gap && B.Y < A.Y + A.W + Gap;
	}

	FString Plural(const FString& Name, int32 Count)
	{
		if (Count <= 1)
		{
			return Name;
		}
		return FString::Printf(TEXT("%d %s%s"), Count, *Name, Name.EndsWith(TEXT("S")) ? TEXT("") : TEXT("S"));
	}
}

void ATakeHUD::DrawPicture(const ATakeDirector& Dir, const FTakeFilmFrame& F)
{
	DrawReticle(Dir);
	DrawLabels(Dir);
	if (Dir.bTitleIsSlate && Dir.TitleSecondsLeft > 0.f)
	{
		Kit.Fade = FMath::Clamp(Dir.TitleSecondsLeft / Frames(FramesSettle), 0.f, 1.f);
		DrawOpeningSlate(Dir, Dir.TitleSeconds - Dir.TitleSecondsLeft, Kit.Fade);
		Kit.Fade = 1.f;
	}
	DrawStamps(Dir);
	DrawSubtitles(F);
}

void ATakeHUD::DrawReticle(const ATakeDirector& Dir)
{
	// The crew's cameras only (§15.4: Julian has nothing to aim); kept with the split off, since Maya still aims.
	const ATakePlayerController* TakePC = Cast<ATakePlayerController>(GetOwningPlayerController());
	if (Dir.Phase != ETakePhase::Rolling || Dir.GetLookCamera() == ETakeCamera::A || (TakePC && TakePC->IsInFlight()))
	{
		return;
	}
	const bool bUseUp = Dir.CurrentUse.bCanFire && !Dir.CurrentUse.Prompt.IsEmpty();
	const FLinearColor C = bUseUp ? TakeStyle::Julian : FTakeHudKit::A(Paper, ReticleAlpha);
	const float CX = Kit.CX();
	const float CY = FMath::RoundToFloat(Kit.H * 0.5f);
	const float T = Kit.Ln(LineThin);
	const float Gap = Kit.Px(ReticleGap) * 0.5f;
	const float Arm = Kit.Px(ReticleArm);
	Kit.Rect(CX - Gap - Arm, CY - T * 0.5f, Arm, T, C);
	Kit.Rect(CX + Gap, CY - T * 0.5f, Arm, T, C);
	Kit.Rect(CX - T * 0.5f, CY - Gap - Arm, T, Arm, C);
	Kit.Rect(CX - T * 0.5f, CY + Gap, T, Arm, C);
}

void ATakeHUD::DrawLabels(const ATakeDirector& Dir)
{
	using namespace TakeHudLabels;
	const APlayerController* ViewPC = GetOwningPlayerController();
	if (!ViewPC || !ViewPC->PlayerCameraManager || Dir.Phase == ETakePhase::BlooperFreeze)
	{
		return;
	}
	const FVector CamLoc = ViewPC->PlayerCameraManager->GetCameraLocation();
	const ETakeCamera Look = Dir.GetLookCamera();
	const bool bFilmCam = Look == ETakeCamera::A;
	const bool bTagsOnly = bFilmCam && !bLabelsOnFilmCamera;	// Julian's camera: countdown tags only
	const float Clear = Kit.Px(LabelBarClear);

	auto OnPicture = [this, Clear](const FVector& P) -> TOptional<FVector2D>
	{
		const FVector S2 = Project(P, false);
		if (S2.Z <= 0.0 || S2.X < 0.0 || S2.X > Kit.W || S2.Y < Kit.PicY + Clear || S2.Y > Kit.PicBottom() - Clear)
		{
			return {};
		}
		return FVector2D(S2.X, S2.Y);
	};

	TArray<FCand> Cands;
	for (const TObjectPtr<AWatcher>& Watcher : Dir.Watchers)
	{
		if (!Watcher || Watcher->IsHidden())
		{
			continue;
		}
		const FVector P = Watcher->GetLabelPoint();
		const float Dist = static_cast<float>(FVector::Dist(P, CamLoc));
		const bool bArrived = Watcher->HasArrived();
		if (Dist > LabelMaxDistance || (!bArrived && Dist > OffLabelMaxDistance))
		{
			continue;
		}
		const EWatcherState St = Watcher->GetState();
		const bool bEffect = bArrived
			&& (St == EWatcherState::Blinded || St == EWatcherState::Lured || St == EWatcherState::Masked);
		if (bTagsOnly && !bEffect)
		{
			continue;
		}
		const TOptional<FVector2D> Screen = OnPicture(P);
		if (!Screen.IsSet())
		{
			continue;
		}
		FCand& C = Cands.AddDefaulted_GetRef();
		C.World = P;
		C.Screen = Screen.GetValue();
		C.Dist = Dist;
		C.bWatcher = true;
		C.bLive = bArrived && Watcher->IsLive();
		C.Name = Watcher->GetLabel();
		C.State = bArrived ? Watcher->GetStateText() : FString(TEXT("OFF"));
		if (C.State == TEXT("WATCHING") || C.State == TEXT("LIVE"))
		{
			C.State.Reset();	// the default: a live watcher is its name and the pulsing square
		}
		C.Countdown = bEffect ? Watcher->GetEffectSecondsLeft() : -1.f;
		C.Prio = Watcher.Get() == Dir.CurrentUse.Watcher.Get() ? 1 : (Watcher->HasTally() ? 2 : (C.bLive ? 3 : 5));
	}
	if (!bTagsOnly)
	{
		for (const TObjectPtr<ADevice>& Device : Dir.Devices)
		{
			if (!Device)
			{
				continue;
			}
			const FVector P = Device->GetLabelPoint();
			const float Dist = static_cast<float>(FVector::Dist(P, CamLoc));
			const TOptional<FVector2D> Screen = Dist <= LabelMaxDistance ? OnPicture(P) : TOptional<FVector2D>();
			if (!Screen.IsSet())
			{
				continue;
			}
			FCand& C = Cands.AddDefaulted_GetRef();
			C.World = P;
			C.Screen = Screen.GetValue();
			C.Dist = Dist;
			C.Name = Device->DeviceName;
			C.Device = Device.Get();
			C.Prio = Device.Get() == Dir.CurrentUse.Device.Get() ? 1 : (Device->FiredFrom == Look ? 4 : 6);
		}
	}

	// Priority, then the nearer; then same name and state within LabelMergeDistance share one label.
	Cands.Sort([](const FCand& L, const FCand& R) { return L.Prio != R.Prio ? L.Prio < R.Prio : L.Dist < R.Dist; });
	for (int32 I = 0; I < Cands.Num(); ++I)
	{
		for (int32 J = Cands.Num() - 1; J > I; --J)
		{
			if (Cands[I].bWatcher && Cands[J].bWatcher && Cands[I].Name == Cands[J].Name
				&& Cands[I].State == Cands[J].State && !bTagsOnly
				&& FVector::Dist(Cands[I].World, Cands[J].World) <= LabelMergeDistance)
			{
				Cands[I].Count += Cands[J].Count;
				Cands.RemoveAt(J);
			}
		}
	}

	const float TapeHPx = Kit.Px(TakeStyle::TapeH);
	const float Pad = Kit.Px(TapePad);
	const float Gap = Kit.Px(GlyphTextGap);
	const float LabelGapPx = Kit.Px(LabelGap);
	const FVector4f Reticle(Kit.CX() - Kit.Px(ReticleBoxW) * 0.5f, Kit.H * 0.5f - Kit.Px(ReticleBoxH) * 0.5f,
		Kit.Px(ReticleBoxW), Kit.Px(ReticleBoxH));
	const int32 Limit = bTagsOnly ? MaxCountdownTags : MaxLabels;
	int32 Others = 0;
	TArray<FVector4f, TInlineAllocator<8>> Placed;
	// The subtitle row is taken: a label never sits under a spoken line.
	if (Subtitles.Num() > 0 && Subtitles[0].Since >= 0.0)
	{
		const float SubH = 2.f * Kit.Px(SubtitlePadY) + 2.f * Kit.Em(EType::Line) * LineSpacing;
		Placed.Add(FVector4f(Kit.CX() - Kit.Px(SubtitleMaxW) * 0.5f, Kit.PicBottom() - Kit.Px(SubtitleUp) - SubH * 0.5f,
			Kit.Px(SubtitleMaxW), SubH));
	}

	for (const FCand& C : Cands)
	{
		const bool bP1 = C.Prio == 1;
		if (!bP1 && Others >= Limit)
		{
			continue;
		}

		// Size: [glyph] NAME [STATE] [pips], or the film's countdown tag: [off] 3.2.
		const FString Name = Plural(C.Name, C.Count);
		const FString Seconds = FString::Printf(TEXT("%.1f"), FMath::Max(0.f, C.Countdown));
		float ContentW = 0.f;
		if (bTagsOnly)
		{
			ContentW = Kit.Px(GlyphSquare) + Gap + Kit.Measure(EType::Tag, Seconds);
		}
		else
		{
			ContentW = (C.bWatcher ? Kit.Px(GlyphSquare) + Gap : 0.f) + Kit.Measure(EType::Tag, Name)
				+ (C.State.IsEmpty() ? 0.f : Gap + Kit.Measure(EType::Tag, C.State));
			if (C.Device && C.Device->Uses > 0)
			{
				ContentW += Gap + (Kit.Px(PipW) + Kit.Px(PipGap)) * static_cast<float>(C.Device->Uses) - Kit.Px(PipGap);
			}
		}
		const float BoxW = FMath::RoundToFloat(ContentW + 2.f * Pad);
		float X = FMath::RoundToFloat(FMath::Clamp(C.Screen.X - BoxW * 0.5f, Clear, Kit.W - Clear - BoxW));
		float Y = FMath::RoundToFloat(C.Screen.Y - Kit.Px(LabelLiftPx) - TapeHPx);
		Y = FMath::Clamp(Y, Kit.PicY + Kit.Px(LabelBarClear), Kit.PicBottom() - Kit.Px(LabelBarClear) - TapeHPx);

		FVector4f Box(X, Y, BoxW, TapeHPx);
		if (!bP1 && !bFilmCam && Overlaps(Box, Reticle, 0.f))
		{
			continue;	// only the priority-1 label may sit by the reticle
		}
		bool bFree = false;
		for (int32 Lift = 0; Lift <= LabelMaxLifts; ++Lift)
		{
			bool bHit = false;
			for (const FVector4f& Other : Placed)
			{
				bHit = bHit || Overlaps(Box, Other, LabelGapPx);
			}
			if (!bHit)
			{
				bFree = Box.Y >= Kit.PicY + Kit.Px(LabelBarClear);
				break;
			}
			Box.Y -= TapeHPx + LabelGapPx;
		}
		if (!bFree)
		{
			continue;
		}
		Placed.Add(Box);
		if (!bP1)
		{
			++Others;
		}

		const float CY = Box.Y + TapeHPx * 0.5f;
		float CX = Box.X + Pad;
		if (bTagsOnly)
		{
			Kit.Rect(Box.X, Box.Y, Box.Z, Box.W, FTakeHudKit::A(TakeStyle::Plate, PlateAlpha));
			Kit.GlyphOff(CX + Kit.Px(GlyphSquare) * 0.5f, CY, Paper);
			Kit.Text(EType::Tag, Seconds, CX + Kit.Px(GlyphSquare) + Gap, CY, Paper);
			continue;
		}

		// Camera tape: a bone strip, ink type; priority 1 gets a paper outline.
		if (bP1)
		{
			const float O = Kit.Ln(TapeOutline);
			Kit.Frame(Box.X - O, Box.Y - O, Box.Z + 2.f * O, Box.W + 2.f * O, O, Paper);
		}
		Kit.Rect(Box.X, Box.Y, Box.Z, Box.W, Bone);
		if (C.bWatcher)
		{
			if (C.bLive)
			{
				Kit.GlyphDanger(CX + Kit.Px(GlyphSquare) * 0.5f, CY, Kit.Pulse(), true);
			}
			else
			{
				Kit.GlyphOff(CX + Kit.Px(GlyphSquare) * 0.5f, CY, Ink);
			}
			CX += Kit.Px(GlyphSquare) + Gap;
		}
		CX += Kit.Text(EType::Tag, Name, CX, CY, Ink);
		if (!C.State.IsEmpty())
		{
			CX += Gap + Kit.Text(EType::Tag, C.State, CX + Gap, CY, Ink);
		}
		if (C.Device && C.Device->Uses > 0)
		{
			CX += Gap;
			for (int32 Use = 0; Use < C.Device->Uses; ++Use)
			{
				Kit.Pip(CX, CY, Use < C.Device->UsesLeft, Ink, Ink);
				CX += Kit.Px(PipW) + Kit.Px(PipGap);
			}
		}
	}
}

void ATakeHUD::DrawSubtitles(const FTakeFilmFrame& F)
{
	if (Subtitles.Num() > 0 && Subtitles[0].Since >= 0.0)
	{
		DrawSubtitleNow(Subtitles[0].Speaker, Subtitles[0].Line, F.bFilmPicture || !F.bFilmLook);
	}
}

void ATakeHUD::DrawSubtitleNow(const FString& Speaker, const FString& Line, bool bFilm)
{
	if (Line.IsEmpty())
	{
		return;
	}
	const float CY = Kit.PicBottom() - Kit.Px(SubtitleUp);
	const float PadX = Kit.Px(SubtitlePadX);
	const float PadY = Kit.Px(SubtitlePadY);
	const float SpeakerW = Speaker.IsEmpty() ? 0.f : Kit.Measure(EType::Tag, Speaker) + Kit.Px(SubtitleSpeakerGap);
	TArray<FString> Lines;
	Kit.Wrap(EType::Line, Line, Kit.Px(SubtitleMaxW) - 2.f * PadX - SpeakerW, Lines);
	while (Lines.Num() > 2)
	{
		Lines[1] += TEXT(" ") + Lines.Pop();	// two lines at most; a very long line runs wide
	}
	float LinesW = 0.f;
	for (const FString& Each : Lines)
	{
		LinesW = FMath::Max(LinesW, Kit.Measure(EType::Line, Each));
	}
	const float Pitch = Kit.Em(EType::Line) * LineSpacing;
	const float BoxW = FMath::RoundToFloat(2.f * PadX + SpeakerW + LinesW);
	const float BoxH = FMath::RoundToFloat(2.f * PadY + Pitch * static_cast<float>(Lines.Num()));
	const float X = FMath::RoundToFloat(Kit.CX() - BoxW * 0.5f);
	Kit.Rect(X, FMath::RoundToFloat(CY - BoxH * 0.5f), BoxW, BoxH,
		FTakeHudKit::A(bFilm ? TakeStyle::Plate : PlateMo, PlateAlpha));
	float LineY = CY - Pitch * 0.5f * static_cast<float>(Lines.Num() - 1);
	if (!Speaker.IsEmpty())
	{
		Kit.Text(EType::Tag, Speaker, X + PadX, LineY, Bone);
	}
	for (const FString& Each : Lines)
	{
		Kit.Text(EType::Line, Each, X + PadX + SpeakerW, LineY, Paper);
		LineY += Pitch;
	}
}

void ATakeHUD::DrawStamps(const ATakeDirector& Dir)
{
	const FVector2D Centre(Kit.CX(), Kit.PicY + Kit.Px(StampY));
	const FVector2D Side(Kit.CX() + Kit.Px(StampSideX), Centre.Y);
	bool bCentreTaken = false;

	// The scene title: a caption, the title, bone rules above and below; no field, no border, no tilt.
	if (!Dir.bTitleIsSlate && !Dir.Title.IsEmpty() && Dir.TitleSecondsLeft > 0.f)
	{
		FString Caption;
		FString Title;
		FTakeHudKit::SplitCaption(Dir.Title, Caption, Title);
		const float TitleAge = Dir.TitleSeconds - Dir.TitleSecondsLeft;
		Kit.Fade = FMath::Clamp(Dir.TitleSecondsLeft / Frames(FramesSettle), 0.f, 1.f);
		Kit.Push(Centre, 0.f, FTakeHudKit::LandingScale(TitleAge));
		const float Cap = TakeStyle::Type(EType::Stamp).CapEm * Kit.Em(EType::Stamp);
		const float RuleW = Kit.Px(SceneRuleW);
		const float RuleX = FMath::RoundToFloat(Centre.X - RuleW * 0.5f);
		const float Above = FMath::RoundToFloat(Centre.Y - Cap * 0.5f - Kit.Px(SceneRuleGap));
		const float Below = FMath::RoundToFloat(Centre.Y + Cap * 0.5f + Kit.Px(SceneRuleGap));
		Kit.HairH(RuleX, Above, RuleW, Bone);
		Kit.HairH(RuleX, Below, RuleW, Bone);
		Kit.Text(EType::Tag, Caption, Centre.X, Above - Kit.Px(SceneRuleGap) - Kit.Em(EType::Tag) * 0.4f, Bone,
			ETakeAlign::Centre);
		Kit.Text(EType::Stamp, Title, Centre.X, Centre.Y, Paper, ETakeAlign::Centre);
		Kit.Pop();
		Kit.Fade = 1.f;
		bCentreTaken = true;
	}

	for (const FTakeShown& Shown : Stamps)
	{
		FTakeStampSpec Spec;
		Spec.Colour = TakeStyle::Julian;
		FVector2D At = Centre;
		float Hold = StampHoldFrames;
		if (Shown.Kind == ETakeFlashKind::Count)
		{
			Spec.Text = Shown.Text;
			Spec.Tilt = CountTilt;
			At = FVector2D(Kit.CX() + Kit.Px(CountX), Kit.PicBottom() - Kit.Px(CountUp));
		}
		else
		{
			FTakeHudKit::SplitCaption(Shown.Text, Spec.Caption, Spec.Text);
			Hold = Spec.Caption.IsEmpty() ? StampHoldFrames : StampTitleHoldFrames;
			if (bCentreTaken)
			{
				At = Side;	// two at once: the second one beside, never over Julian
			}
			bCentreTaken = true;
		}
		Kit.Stamp(Spec, At, Age(Shown.Since), Hold);
	}
}

void ATakeHUD::DrawOpeningSlate(const ATakeDirector& Dir, float SlateAge, float Alpha)
{
	const UTakeGameInstance* GI = Dir.GetTakeGameInstance();
	const float SW = Kit.Px(OpeningW);
	const float SH = Kit.Px(OpeningH);
	const float X = Kit.Px(OpeningX);
	const float Top = FMath::RoundToFloat(Kit.PicBottom() - Kit.Px(OpeningUp) - SH);
	const float Stick = Kit.Px(StickH);
	Kit.Sticks(X, Top, SW, Paper, SlateAge);

	const float BY = Top + 2.f * Stick;
	const float BH = SH - 2.f * Stick;
	Kit.Board(X, BY, SW, BH);
	const float Row1 = FMath::RoundToFloat(BH * 0.4f);
	Kit.HairH(X, BY + Row1, SW, Bone);

	FString Venue;
	FString Lot;
	FTakeHudKit::SplitCaption(Dir.VenueLine, Venue, Lot);
	if (Venue.IsEmpty())
	{
		const int32 Dash = Lot.Find(TEXT(" - "));
		if (Dash > 0)
		{
			Venue = Lot.Left(Dash).TrimStartAndEnd();
			Lot = Lot.Mid(Dash + 3).TrimStartAndEnd();
		}
	}
	Kit.CellCaption(X, BY, Venue);
	Kit.Text(EType::Label, Lot, X + Kit.Px(CellIn), BY + Row1 * 0.66f, Paper);

	const FString Captions[] = { TEXT("SCENE"), TEXT("TAKE"), TEXT("DIRECTOR") };
	const FString Values[] = { FString::FromInt(Dir.Scene), FString::FromInt(GI ? GI->TakeNumber : 1), DirectorWord };
	const float CellW = SW / 3.f;
	const float CellY = BY + Row1;
	const float CellH = BH - Row1;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const float CellX = FMath::RoundToFloat(X + CellW * static_cast<float>(Index));
		if (Index > 0)
		{
			Kit.HairV(CellX, CellY, CellH, Bone);
		}
		Kit.CellCaption(CellX, CellY, Captions[Index]);
		Kit.Text(EType::Title, Values[Index], CellX + CellW * 0.5f, CellY + CellH * 0.6f, Paper, ETakeAlign::Centre);
	}
}
