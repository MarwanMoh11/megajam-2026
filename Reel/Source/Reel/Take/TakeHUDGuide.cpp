// The HUD's part of the guidance (TakeGuide.h): the crew line goes to the subtitles when the lane is free, and the soft
// marker fades in at the goal, or at the picture's edge pointing to it, with the objective beside it. Drawn under the
// scene's own HUD, the splice and the retake. Owner: hud-guidance.

#include "TakeHUD.h"
#include "TakeGuide.h"
#include "TakeStyle.h"
#include "Script/ScriptSequencer.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Canvas.h"
#include "HAL/IConsoleManager.h"
#include "TakeChoiceCards.h"
#include "TakeTicketCard.h"
#include "TakeRigCard.h"

namespace TakeHudGuideLocal
{
	// Shots of the shared HUD pieces without their scene: 1 the choice cards (Scene 3's lines plus a long one),
	// 2 the VIP ticket, 3 the ticket voided, 4 Scene 4's rig cards. 0 off.
	TAutoConsoleVariable<int32> CVarHudTest(TEXT("Reel.Hud.Test"), 0,
		TEXT("HUD test draw: 1 choice cards, 2 VIP ticket, 3 VIP ticket voided, 4 rig cards, 0 off."));
}

void ATakeHUD::DrawGuide()
{
	using namespace TakeStyle;
	if (const int32 Test = TakeHudGuideLocal::CVarHudTest.GetValueOnGameThread(); Test > 0 && Canvas)
	{
		if (Test == 1)
		{
			TArray<FTakeChoiceCard> Cards;
			Cards.Add({ TEXT("1"), TEXT("PLAY ALONG"), TEXT("Keep your voice down, darling. I'm incognito.") });
			Cards.Add({ TEXT("2"), TEXT("BRUSH OFF"), TEXT("You have the wrong man. I have never been to Zurich, and I do not read Cahiers.") });
			TakeChoice::Draw(Kit, Cards, 0, Kit.PicBottom() - Kit.Px(282.f), TEXT("THE CRITIC  -  your answer"),
				TEXT("1 / 2   or   arrows + E"));
		}
		else if (Test == 4)
		{
			const float Bar = 1.f - static_cast<float>(FMath::Fmod(Kit.Now, 6.0)) / 6.f;
			TakeRigCard::Draw(Kit, FVector2D(Kit.W * 0.62f, Kit.H * 0.55f), 60.f, -150.f, TEXT("1"), TEXT("THE SPOTLIGHT CLAMP"),
				TEXT("Loosen it: the lamp swings into the EAST DOOR guard's eyes."), Julian, -1.f);
			TakeRigCard::Draw(Kit, FVector2D(Kit.W * 0.40f, Kit.H * 0.72f), -530.f, -40.f, FString(),
				TEXT("THE SPOTLIGHT  -  EAST DOOR BLINDED"), TEXT("Guard 1 is yelling up at it. The bulb won't last."), Danger, Bar);
		}
		else
		{
			const float Age = static_cast<float>(FMath::Fmod(Kit.Now, 3.0));
			TakeTicket::Draw(Kit, 1.f, 0.f, 0.f, Test == 3 ? Age : -1.f);
		}
	}
	UTakeGuideSubsystem* Guide = UTakeGuideSubsystem::Get(this);
	AScriptSequencer* Seq = AScriptSequencer::Get(this);
	if (!Guide || !Seq || !Seq->IsRunning() || !Canvas)
	{
		return;
	}

	// The crew line: only into a free lane, so it never cuts a scene's own line.
	Guide->SetSubtitleUp(Subtitles.Num() > 0);
	FString Speaker, Line;
	USoundBase* Sound = nullptr;
	if (Subtitles.Num() == 0 && Guide->TakeCrewLine(Speaker, Line, Sound))
	{
		Seq->OnSay(Speaker, Line, Sound);
	}

	const FTakeGuideView V = Guide->GetView();
	if (!V.bShow || !PlayerOwner || !PlayerOwner->PlayerCameraManager)
	{
		return;
	}
	const float A = FMath::Clamp(V.Alpha, 0.f, 1.f) * 0.9f;
	const FVector Target = V.Goal + FVector(0.f, 0.f, 110.f);
	const FVector CamLoc = PlayerOwner->PlayerCameraManager->GetCameraLocation();
	const FRotator CamRot = PlayerOwner->PlayerCameraManager->GetCameraRotation();
	const FVector Local = CamRot.UnrotateVector(Target - CamLoc);	// X ahead, Y right, Z up

	// The picture, less a margin: the marker never sits on the bars or the very edge.
	const float Inset = Kit.Px(70.f);
	const float L = Inset, R = Kit.W - Inset;
	const float T = Kit.PicY + Inset, B = Kit.PicBottom() - Inset;
	const float Bob = Kit.Px(4.f) * FMath::Sin(static_cast<float>(Kit.Now) * 2.4f);
	const FLinearColor Gold = FTakeHudKit::A(Julian, A);
	const FLinearColor Shade = FLinearColor(0.f, 0.f, 0.f, 0.5f * A);

	FVector2D P = FVector2D::ZeroVector;
	bool bOnPicture = false;
	if (Local.X > 10.0)
	{
		const FVector S = Canvas->Project(Target, false);
		P = FVector2D(S.X, S.Y);
		bOnPicture = P.X >= L && P.X <= R && P.Y >= T && P.Y <= B;
	}

	if (bOnPicture)
	{
		// On the picture: a small gold diamond over the goal, the objective under it.
		const float Y = P.Y + Bob;
		Kit.Diamond(P.X, Y, Kit.Px(22.f), Shade);
		Kit.Diamond(P.X, Y, Kit.Px(16.f), Gold);
		Kit.Diamond(P.X, Y, Kit.Px(8.f), FLinearColor(0.f, 0.f, 0.f, 0.6f * A));
		if (!V.Objective.IsEmpty())
		{
			const float TW = Kit.Measure(EType::Tag, V.Objective);
			const float TY = Y + Kit.Px(30.f);
			Kit.Rect(P.X - TW * 0.5f - Kit.Px(10.f), TY - Kit.Px(13.f), TW + Kit.Px(20.f), Kit.Px(26.f), Shade);
			Kit.Text(EType::Tag, V.Objective, P.X, TY, FTakeHudKit::A(Paper, A), ETakeAlign::Centre);
		}
		return;
	}

	// Off the picture or behind him: a chevron on the picture's edge, pointing the way, the objective inside it.
	// Behind him: point to the side it is on, a little low (turn round), never straight down.
	FVector2D Dir(Local.Y >= 0.0 ? 1.f : -1.f, 0.25f);
	if (Local.X > 10.0)
	{
		// In front but off the frame: the projected point's direction from the centre is the honest one.
		const FVector S = Canvas->Project(Target, false);
		Dir = FVector2D(S.X - Kit.CX(), S.Y - (Kit.PicY + Kit.PicH * 0.5f));
	}
	Dir.Normalize();
	const FVector2D C(Kit.CX(), Kit.PicY + Kit.PicH * 0.5f);
	const float HalfW = (R - L) * 0.5f, HalfH = (B - T) * 0.5f;
	const float Scale = FMath::Min(Dir.X != 0.f ? HalfW / FMath::Abs(Dir.X) : 1e6f, Dir.Y != 0.f ? HalfH / FMath::Abs(Dir.Y) : 1e6f);
	const FVector2D Edge = C + Dir * Scale;
	const float Deg = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
	const float Arm = Kit.Px(22.f), Thick = Kit.Ln(4.f);
	for (const float Side : { 140.f, 220.f })
	{
		Kit.Push(Edge, Deg + Side, 1.f);
		Kit.Rect(Edge.X, Edge.Y - Thick * 0.5f, Arm, Thick, Gold);
		Kit.Pop();
	}
	if (!V.Objective.IsEmpty())
	{
		const float TW = Kit.Measure(EType::Tag, V.Objective);
		FVector2D TP = Edge - Dir * Kit.Px(56.f);
		TP.X = FMath::Clamp(TP.X, L + TW * 0.5f + Kit.Px(10.f), R - TW * 0.5f - Kit.Px(10.f));
		TP.Y = FMath::Clamp(TP.Y, T + Kit.Px(14.f), B - Kit.Px(14.f));
		Kit.Rect(TP.X - TW * 0.5f - Kit.Px(10.f), TP.Y - Kit.Px(13.f), TW + Kit.Px(20.f), Kit.Px(26.f), Shade);
		Kit.Text(EType::Tag, V.Objective, TP.X, TP.Y, FTakeHudKit::A(Paper, A), ETakeAlign::Centre);
	}
}
