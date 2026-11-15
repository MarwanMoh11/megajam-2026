// The HUD's script layer (docs/SCRIPT_ADAPT.md). Owner: the Spine session.
// Drawn over the picture while AScriptSequencer runs: the active scene's own HUD, the slug line on a scene's first
// cut, the splice (a few frames of white flash and a frame line rolling through), the retake (the frame burns and the
// blooper card), and the end of the film ("Cut. Print it." then the credits over film sprockets).

#include "TakeHUD.h"
#include "TakeDirector.h"
#include "TakeStyle.h"
#include "Script/ScriptSequencer.h"
#include "Engine/Canvas.h"

namespace TakeScriptHud
{
	static FString ReasonLine(FName Reason)
	{
		const FString R = Reason.ToString();
		if (R == TEXT("Retake") || R == TEXT("Test")) { return TEXT("Again, from the top of the scene."); }
		if (R == TEXT("Seen")) { return TEXT("You were seen."); }
		if (R == TEXT("Caught")) { return TEXT("Caught on camera. Not our camera."); }
		if (R == TEXT("Time")) { return TEXT("Too slow. The moment's gone."); }
		if (R == TEXT("Suspicion")) { return TEXT("They're onto you."); }
		if (R == TEXT("Hit")) { return TEXT("That wasn't in the script."); }
		// A scene's own reason: shown as written, underscores to spaces.
		return R.Replace(TEXT("_"), TEXT(" "));
	}
}

void ATakeHUD::DrawScript(const ATakeDirector& Dir, const FTakeFilmFrame& F)
{
	using namespace TakeStyle;
	AScriptSequencer* Seq = AScriptSequencer::Get(this);
	if (!Seq || !Seq->IsRunning() || !Canvas) { return; }
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;

	// ---- the end of the film --------------------------------------------------------------------------------------
	const float End = Seq->GetEndSeconds();
	if (End >= 0.f)
	{
		Kit.Rect(0.f, 0.f, W, H, Ink);
		// Sprockets down both edges, rolling.
		const float HoleW = Kit.Px(28.f), HoleH = Kit.Px(20.f), Pitch = Kit.Px(48.f);
		const float Roll = FMath::Fmod(End * Kit.Px(90.f), Pitch);
		for (float Y = -Pitch + Roll; Y < H; Y += Pitch)
		{
			Kit.Rect(Kit.Px(18.f), Y, HoleW, HoleH, Plate);
			Kit.Rect(W - Kit.Px(18.f) - HoleW, Y, HoleW, HoleH, Plate);
		}
		Kit.Rect(Kit.Px(60.f), 0.f, Kit.Ln(2.f), H, Plate);
		Kit.Rect(W - Kit.Px(60.f), 0.f, Kit.Ln(2.f), H, Plate);
		if (End < Seq->PrintItSeconds)
		{
			const float A = FMath::Clamp(End / 0.25f, 0.f, 1.f);
			Kit.Text(EType::Title, TEXT("Cut."), Kit.CX(), H * 0.42f, Paper * FLinearColor(1, 1, 1, A), ETakeAlign::Centre);
			const float B = FMath::Clamp((End - 0.8f) / 0.25f, 0.f, 1.f);
			Kit.Text(EType::Display, TEXT("PRINT IT."), Kit.CX(), H * 0.56f, Julian * FLinearColor(1, 1, 1, B), ETakeAlign::Centre);
			return;
		}
		// The credits roll up from the bottom over CreditsSeconds, then hold the last line.
		const float T = End - Seq->PrintItSeconds;
		const float LineH = Kit.Em(EType::Line) * 1.6f;
		const float Total = LineH * Seq->Credits.Num() + H;
		const float Speed = Total / FMath::Max(1.f, Seq->CreditsSeconds);
		float Y = H - FMath::Min(T * Speed, Total - H * 0.5f);
		for (const FString& Line : Seq->Credits)
		{
			if (Y > -LineH && Y < H + LineH && !Line.IsEmpty())
			{
				const bool bHead = Line == Line.ToUpper();
				Kit.Text(bHead ? EType::Goal : EType::Line, Line, Kit.CX(), Y, bHead ? Julian : Bone, ETakeAlign::Centre);
			}
			Y += LineH;
		}
		if (T > Seq->CreditsSeconds * 0.5f)
		{
			Kit.Text(EType::Label, TEXT("[ENTER] Roll it again"), Kit.CX(), H - Kit.Px(40.f), Silver, ETakeAlign::Centre);
		}
		return;
	}

	// ---- the active scene's own HUD -------------------------------------------------------------------------------
	if (AScriptScene* Scene = Seq->GetActiveScene())
	{
		Scene->DrawSceneHud(Canvas);
	}

	// ---- the slug line --------------------------------------------------------------------------------------------
	float SlugAge = 0.f;
	const FText Slug = Seq->GetSlug(SlugAge);
	if (!Slug.IsEmpty())
	{
		const float In = FMath::Clamp((SlugAge - 0.15f) / 0.2f, 0.f, 1.f);
		const float Out = FMath::Clamp((Seq->SlugSeconds - SlugAge) / 0.5f, 0.f, 1.f);
		const float A = In * Out;
		const FString S = Slug.ToString().ToUpper();
		const float Y = Kit.PicY + Kit.PicH * 0.12f;
		const float TW = Kit.Measure(EType::Goal, S);
		Kit.Rect(Kit.CX() - TW * 0.5f - Kit.Px(16.f), Y - Kit.Em(EType::Goal) * 0.7f, TW + Kit.Px(32.f),
			Kit.Em(EType::Goal) * 1.4f, Ink * FLinearColor(1, 1, 1, 0.65f * A));
		Kit.Text(EType::Goal, S, Kit.CX(), Y, Paper * FLinearColor(1, 1, 1, A), ETakeAlign::Centre);
	}

	// ---- the splice: a white flash, a black frame line rolling down, a scratch -----------------------------------
	const float Sp = Seq->GetSpliceAlpha();
	if (Sp > 0.f)
	{
		if (Sp > 0.75f)
		{
			Kit.Rect(0.f, 0.f, W, H, Paper * FLinearColor(1, 1, 1, 0.9f));
		}
		else
		{
			const float Roll = (1.f - Sp / 0.75f);
			const float LineY = H * Roll;
			Kit.Rect(0.f, LineY - Kit.Px(36.f), W, Kit.Px(36.f), Ink * FLinearColor(1, 1, 1, 0.85f));
			Kit.Rect(0.f, 0.f, W, H, Paper * FLinearColor(1, 1, 1, 0.25f * Sp));
			const float X = W * (0.3f + 0.4f * FMath::Frac(static_cast<float>(F.RealTime) * 7.3f));
			Kit.Rect(X, 0.f, Kit.Ln(2.f), H, Paper * FLinearColor(1, 1, 1, 0.6f));
		}
	}

	// ---- the retake: the frame burns, then the blooper card ------------------------------------------------------
	FName Reason;
	float Age = 0.f;
	if (Seq->IsRetaking(Reason, Age))
	{
		// The burn: a hot hole opening from the centre, then the card.
		const float Burn = FMath::Clamp(Age / 0.45f, 0.f, 1.f);
		const float R = FMath::Max(W, H) * Burn;
		Kit.Rect(Kit.CX() - R * 0.5f, H * 0.5f - R * 0.35f, R, R * 0.7f, Hex(0xF5, 0xB0, 0x40) * FLinearColor(1, 1, 1, 0.85f));
		Kit.Rect(Kit.CX() - R * 0.35f, H * 0.5f - R * 0.22f, R * 0.7f, R * 0.44f, Paper);
		if (Age > 0.45f)
		{
			Kit.Rect(0.f, 0.f, W, H, Ink);
			Kit.Text(EType::Display, TEXT("CUT!"), Kit.CX(), H * 0.42f, Danger, ETakeAlign::Centre);
			Kit.Text(EType::Line, TakeScriptHud::ReasonLine(Reason), Kit.CX(), H * 0.58f, Paper, ETakeAlign::Centre);
			if (AScriptScene* Scene = Seq->GetActiveScene())
			{
				Kit.Text(EType::Label, FString::Printf(TEXT("SCENE %d   TAKE %d"), Scene->SceneNumber, Scene->GetRetakes() + 1),
					Kit.CX(), H * 0.68f, Silver, ETakeAlign::Centre);
			}
		}
	}
}
