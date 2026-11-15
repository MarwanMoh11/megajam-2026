// The script scenes' HUD widgets (QA Nov 16, Marwan: "proper UI for telling you to crouch and the like, nothing plain
// text"). Built on the HUD's kit (TakeHudKit.h: the game's type, keycaps, plates, diamonds), so every scene's prompts
// look like one system: the objective at the top centre, the action prompt at the bottom centre, the stance icon, the
// guard's alert and the waypoint. Header only (inline): scenes include it and draw with a kit begun for the frame.
#pragma once

#include "CoreMinimal.h"
#include "TakeHudKit.h"
#include "TakeStyle.h"

namespace ScriptHudWidgets
{
	/** How a widget is coloured: its accent (rule, keycaps, glyph) and its words. */
	enum class ETone : uint8 { Neutral, Action, Good, Warn, Danger, Muted };

	inline FLinearColor ToneColour(ETone Tone)
	{
		switch (Tone)
		{
		case ETone::Action: return TakeStyle::Julian;
		case ETone::Good: return TakeStyle::Hex(0x7F, 0xC8, 0x8A);
		case ETone::Warn: return TakeStyle::Hex(0xE8, 0x8A, 0x2E);
		case ETone::Danger: return TakeStyle::Danger;
		case ETone::Muted: return TakeStyle::Ash;
		default: return TakeStyle::Paper;
		}
	}

	/** An ink plate with a coloured rule down its left edge (the family's caption plate). */
	inline void Plate(FTakeHudKit& K, float X, float Y, float W, float H, const FLinearColor& Accent, float Alpha)
	{
		K.Rect(K.Snap(X), K.Snap(Y), K.Snap(W), K.Snap(H), FTakeHudKit::A(TakeStyle::Plate, 0.78f * Alpha));
		K.Rect(K.Snap(X), K.Snap(Y), K.Ln(4.f), K.Snap(H), FTakeHudKit::A(Accent, Alpha));
	}

	/** A rich line ([E] as keycaps) on a plate, centred on CentreX / CentreY. Returns the plate's height. */
	inline float RichPlate(FTakeHudKit& K, TakeStyle::EType Type, const FString& Line, float CentreX, float CentreY,
		ETone Tone, float Alpha, float MaxW)
	{
		if (Line.IsEmpty() || Alpha <= 0.f)
		{
			return 0.f;
		}
		const FLinearColor Accent = ToneColour(Tone);
		const FLinearColor Words = Tone == ETone::Muted ? TakeStyle::Ash : TakeStyle::Paper;
		FTakeInk Ink = FTakeInk::Of(FTakeHudKit::A(Words, Alpha), FTakeHudKit::A(Accent, Alpha));
		Ink.KeyFill = FTakeHudKit::A(TakeStyle::Ink, 0.9f * Alpha);
		const float PadX = K.Px(22.f), PadY = K.Px(12.f);
		const float TextW = FMath::Min(K.RichWidth(Type, Line, Ink), MaxW);
		const int32 Lines = K.RichWidth(Type, Line, Ink) > MaxW ? 2 : 1;
		const float LineH = K.Em(Type) * 1.5f;
		const float BoxW = TextW + 2.f * PadX;
		const float BoxH = LineH * Lines + 2.f * PadY;
		Plate(K, CentreX - BoxW * 0.5f, CentreY - BoxH * 0.5f, BoxW, BoxH, Accent, Alpha);
		K.Rich(Type, Line, CentreX + K.Px(2.f), CentreY, MaxW, 2, Ink);
		return BoxH;
	}

	/** The objective: top centre, under the (now thin or absent) top bar. */
	inline void Objective(FTakeHudKit& K, const FString& Line, float Alpha = 1.f)
	{
		const float Y = K.Px(64.f) + K.Em(TakeStyle::EType::Goal);
		RichPlate(K, TakeStyle::EType::Goal, Line, K.CX(), Y, ETone::Action, Alpha, K.W * 0.62f);
	}

	/** A second line under the objective (a hint or a warning), smaller. */
	inline void SubObjective(FTakeHudKit& K, const FString& Line, ETone Tone, float Alpha = 1.f)
	{
		const float Y = K.Px(64.f) + K.Em(TakeStyle::EType::Goal) * 3.4f;
		RichPlate(K, TakeStyle::EType::Label, Line, K.CX(), Y, Tone, Alpha, K.W * 0.56f);
	}

	/** The action prompt: bottom centre, one at a time ("[E] Drag him into the costume trunk"). */
	inline void Prompt(FTakeHudKit& K, const FString& Line, ETone Tone, float Alpha = 1.f)
	{
		RichPlate(K, TakeStyle::EType::Goal, Line, K.CX(), K.H * 0.76f, Tone, Alpha, K.W * 0.5f);
	}

	/** A toast in the middle of the picture ("CRASH!", "ALARM OFF"). */
	inline void Toast(FTakeHudKit& K, const FString& Line, ETone Tone, float Alpha)
	{
		// A word or two lands big; a sentence reads at the objective's size.
		const TakeStyle::EType Type = Line.Len() <= 24 ? TakeStyle::EType::Title : TakeStyle::EType::Goal;
		RichPlate(K, Type, Line, K.CX(), K.H * 0.36f, Tone, Alpha, K.W * 0.6f);
	}

	/** A small figure: standing (tall) or crouched (low, knees bent), in Colour, its feet on FootY. Scale: 1 = 34 px. */
	inline void Figure(FTakeHudKit& K, float CentreX, float FootY, bool bCrouched, const FLinearColor& Colour, float Scale = 1.f)
	{
		const float U = K.Px(1.f) * Scale;
		if (!bCrouched)
		{
			K.Diamond(CentreX, FootY - 31.f * U, 9.f * U, Colour);					// head
			K.Rect(CentreX - 4.f * U, FootY - 25.f * U, 8.f * U, 13.f * U, Colour);	// body
			K.Rect(CentreX - 4.f * U, FootY - 12.f * U, 3.f * U, 12.f * U, Colour);	// legs
			K.Rect(CentreX + 1.f * U, FootY - 12.f * U, 3.f * U, 12.f * U, Colour);
		}
		else
		{
			K.Diamond(CentreX + 5.f * U, FootY - 20.f * U, 9.f * U, Colour);		// head, forward and low
			K.Rect(CentreX - 6.f * U, FootY - 15.f * U, 12.f * U, 7.f * U, Colour);	// back, bent over
			K.Rect(CentreX - 6.f * U, FootY - 8.f * U, 3.f * U, 8.f * U, Colour);	// knees down
			K.Rect(CentreX + 3.f * U, FootY - 8.f * U, 3.f * U, 8.f * U, Colour);
		}
	}

	/** The stance, bottom left: only a small yellow figure, standing or crouched (QA Nov 16, Marwan: "just a simple yellow
	 *  logo"), on a soft ink shadow so it reads on any floor. */
	inline void StanceIcon(FTakeHudKit& K, bool bCrouched)
	{
		const float X = K.Px(64.f);
		const float FootY = K.H - K.Px(56.f);
		const float Shadow = K.Px(2.f);
		Figure(K, X + Shadow, FootY + Shadow, bCrouched, FTakeHudKit::A(TakeStyle::Ink, 0.7f), 1.4f);
		Figure(K, X, FootY, bCrouched, TakeStyle::Julian, 1.4f);
	}

	/** Over a guard's head: ? (curious) growing into ! (onto you). Level 0..1. */
	inline void Alert(FTakeHudKit& K, const FVector2D& At, float Level)
	{
		const bool bBang = Level >= 0.6f;
		const FLinearColor C = bBang ? TakeStyle::Danger : TakeStyle::Julian;
		const float Size = K.Px(26.f + 18.f * FMath::Clamp(Level, 0.f, 1.f));
		K.Diamond(At.X, At.Y, Size + K.Px(6.f), FTakeHudKit::A(TakeStyle::Ink, 0.85f));
		K.Diamond(At.X, At.Y, Size, C);
		K.Text(TakeStyle::EType::Title, bBang ? TEXT("!") : TEXT("?"), At.X, At.Y, TakeStyle::Ink, ETakeAlign::Centre);
	}

	/** The waypoint: a gold diamond on the next thing to use, its distance under it; dimmer on the screen's edge. */
	inline void Waypoint(FTakeHudKit& K, const FVector2D& At, bool bOffScreen, float Metres)
	{
		const float A = bOffScreen ? 0.55f : 0.95f;
		K.Diamond(At.X, At.Y, K.Px(20.f), FTakeHudKit::A(TakeStyle::Ink, 0.8f * A));
		K.Diamond(At.X, At.Y, K.Px(14.f), FTakeHudKit::A(TakeStyle::Julian, A));
		const FString Dist = FString::Printf(TEXT("%.0f M"), Metres);
		const float TW = K.Measure(TakeStyle::EType::Tag, Dist);
		const float TY = At.Y + K.Px(24.f);
		K.Rect(At.X - TW * 0.5f - K.Px(6.f), TY - K.Px(10.f), TW + K.Px(12.f), K.Px(20.f), FTakeHudKit::A(TakeStyle::Ink, 0.7f * A));
		K.Text(TakeStyle::EType::Tag, Dist, At.X, TY, FTakeHudKit::A(TakeStyle::Paper, A), ETakeAlign::Centre);
	}
}
