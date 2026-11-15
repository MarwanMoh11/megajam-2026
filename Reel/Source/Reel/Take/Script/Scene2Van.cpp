// See Scene2Van.h. Owner: the s2-van session.

#include "Script/Scene2Van.h"
#include "Script/ScriptSequencer.h"
#include "TakeHudKit.h"
#include "TakeStyle.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogScene2, Log, All);

namespace Scene2Local
{
	const FName FlagThrough(TEXT("S1PalmThrough"));
	const FLinearColor Phosphor(0.36f, 1.f, 0.55f, 1.f);	// Max's CRT green
	const FLinearColor PhosphorDim(0.10f, 0.32f, 0.16f, 1.f);
	const FLinearColor ScanAmber(1.f, 0.62f, 0.16f, 1.f);	// the scanner's wave
	const FLinearColor Alarm(0.95f, 0.20f, 0.12f, 1.f);
	const FLinearColor Glass(0.02f, 0.04f, 0.03f, 0.86f);	// panel field

	FLinearColor A_(const FLinearColor& C, float Alpha) { return FLinearColor(C.R, C.G, C.B, C.A * Alpha); }
	float Ease(float A) { A = FMath::Clamp(A, 0.f, 1.f); return A * A * (3.f - 2.f * A); }
	FQuat LookQuat(const FVector& From, const FVector& At)
	{
		const FVector D = (At - From).GetSafeNormal();
		return FRotationMatrix::MakeFromX(D.IsNearlyZero() ? FVector::ForwardVector : D).ToQuat();
	}
	void Line(UCanvas* C, const FVector2D& A, const FVector2D& B, const FLinearColor& Col, float Thick)
	{
		FCanvasLineItem L(A, B);
		L.SetColor(Col);
		L.LineThickness = Thick;
		L.BlendMode = SE_BLEND_Translucent;
		C->DrawItem(L);
	}
	void Fill(UCanvas* C, float X, float Y, float W, float H, const FLinearColor& Col)
	{
		FCanvasTileItem T(FVector2D(X, Y), FVector2D(W, H), Col);
		T.BlendMode = SE_BLEND_Translucent;
		C->DrawItem(T);
	}
}

AScene2Van::AScene2Van()
{
	SceneNumber = 2;
	Pov = EScriptPov::Max;
	bPlayerWalks = false;
	Slug = FText::FromString(TEXT("INT. FLORIST VAN - CONTINUOUS"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Cam = CreateDefaultSubobject<UCameraComponent>(TEXT("Cam"));
	Cam->SetupAttachment(Root);
	Cam->SetUsingAbsoluteLocation(true);
	Cam->SetUsingAbsoluteRotation(true);
	Cam->FieldOfView = 62.f;

	FeedCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("FeedCapture"));
	FeedCapture->SetupAttachment(Root);
	FeedCapture->SetUsingAbsoluteLocation(true);
	FeedCapture->SetUsingAbsoluteRotation(true);
	FeedCapture->bCaptureEveryFrame = false;
	FeedCapture->bCaptureOnMovement = false;
	FeedCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
}

void AScene2Van::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (!SceneCamera) { SceneCamera = this; }	// the van is a camera, not a character
}

float AScene2Van::Tune(const TCHAR* Name, float Default) const
{
	const AScriptSequencer* S = GetSequencer();
	return S ? S->GetTune(FName(*FString::Printf(TEXT("S2.%s"), Name)), Default) : Default;
}

float AScene2Van::HesitationLimit() const
{
	return Tune(TEXT("HesitationSeconds"), HesitationSeconds) + FMath::Min(Retakes, 3) * Tune(TEXT("RetakeBonus"), RetakeBonus);
}

void AScene2Van::Play(USoundBase* S) const
{
	if (S) { UGameplayStatics::PlaySound2D(this, S); }
}

bool AScene2Van::CallScene1(FName Function) const
{
	for (TActorIterator<AScriptScene> It(GetWorld()); It; ++It)
	{
		AScriptScene* S = *It;
		if (!S || S->SceneNumber != 1) { continue; }
		UFunction* F = S->FindFunction(Function);
		if (F && F->NumParms == 0)
		{
			S->ProcessEvent(F, nullptr);
			UE_LOG(LogScene2, Log, TEXT("S2 called %s on %s"), *Function.ToString(), *S->GetName());
			return true;
		}
		UE_LOG(LogScene2, Warning, TEXT("S2: scene 1 (%s) has no %s()"), *S->GetName(), *Function.ToString());
		return false;
	}
	UE_LOG(LogScene2, Warning, TEXT("S2: no scene 1 actor for %s()"), *Function.ToString());
	return false;
}

void AScene2Van::ResetScene()
{
	Super::ResetScene();
	SetPhase(EScene2Phase::Opening);
	Freq = 1.5f;
	Gain = 0.22f;
	Seed = FMath::FRandRange(0.f, 6.f);
	Sync = 0.f;
	Hesitation = 0.f;
	bKeyA = bKeyD = bKeyL = bKeyR = false;
	FreqHeld = GainHeld = 0;
	SinceDial = SinceSpark = SinceLock = 99.f;
	LastDialSound = -1.f;
	bSaidClose = bSaidBouncerA = bSaidBouncerB = false;
	bPalmCalled = false;
	ConsumeFlag(Scene2Local::FlagThrough);
	UpdateTarget();
	PoseOpening(0.f);
	CallScene1(TEXT("StageForVan"));	// Vance at the glass, hand hovering, for a cold Jump 2 or a retake
}

void AScene2Van::BeginScene()
{
	Super::BeginScene();
	if (!FeedTarget)
	{
		FeedTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("S2_FeedRT"));
		FeedTarget->ClearColor = FLinearColor::Black;
		FeedTarget->InitCustomFormat(512, 384, PF_B8G8R8A8, false);
		FeedTarget->UpdateResourceImmediate(true);
	}
	FeedCapture->TextureTarget = FeedTarget;
	FeedCapture->FOVAngle = FeedFov;
	FeedCapture->SetWorldLocationAndRotation(FeedFrom, Scene2Local::LookQuat(FeedFrom, FeedAt));
	FeedCapture->bCaptureEveryFrame = true;
	PoseOpening(0.f);
	StopHum();
	if (AmbHum) { HumComp = UGameplayStatics::SpawnSound2D(this, AmbHum, 0.8f); }
	if (GetRetakes() > 0)	// a retake skips the street: straight back to the dials (instant retry)
	{
		const float OpenLen = OpeningBeats.X + OpeningBeats.Y + OpeningBeats.Z + OpeningBeats.W;
		PoseOpening(OpenLen);
		SetPhase(EScene2Phase::Tune);
		Say(TEXT("MAX"), TEXT("Again. Three seconds, Vance. Don't touch that glass."), LineRetake);
	}
}

void AScene2Van::StopHum()
{
	if (HumComp) { HumComp->FadeOut(0.25f, 0.f); HumComp = nullptr; }
}

void AScene2Van::EndScene()
{
	StopHum();
	FeedCapture->bCaptureEveryFrame = false;
	Super::EndScene();
}

void AScene2Van::PoseOpening(float T)
{
	using namespace Scene2Local;
	const float B0 = OpeningBeats.X, B1 = OpeningBeats.Y, B2 = OpeningBeats.Z, B3 = OpeningBeats.W;
	FVector P;
	FQuat Q;
	float Fov = StreetFov;
	const FQuat AtSteps = LookQuat(StepsPos, StepsLook);
	const FQuat ToVan = LookQuat(StepsPos, WindshieldLook);
	const FQuat Through = LookQuat(WindshieldPos, InsidePos);
	const FQuat OnMax = LookQuat(MaxCamPos, MaxLook);
	if (T < B0)	// pan on the steps, from the queue to the van across the street
	{
		P = StepsPos;
		Q = FQuat::Slerp(AtSteps, ToVan, Ease(T / B0));
	}
	else if (T < B0 + B1)	// cross the wet street to the windshield
	{
		const float A = Ease((T - B0) / B1);
		P = FMath::Lerp(StepsPos, WindshieldPos, A);
		Q = FQuat::Slerp(ToVan, Through, Ease(FMath::Clamp((A - 0.6f) / 0.4f, 0.f, 1.f)));
	}
	else if (T < B0 + B1 + B2)	// push through the fogged glass into the cab
	{
		const float A = Ease((T - B0 - B1) / B2);
		P = FMath::Lerp(WindshieldPos, InsidePos, A);
		Q = Through;
		Fov = FMath::Lerp(StreetFov, VanFov, A);
	}
	else	// settle on Max
	{
		const float A = Ease((T - B0 - B1 - B2) / FMath::Max(B3, 0.01f));
		P = FMath::Lerp(InsidePos, MaxCamPos, A);
		Q = FQuat::Slerp(Through, OnMax, A);
		Fov = VanFov;
	}
	Cam->SetWorldLocationAndRotation(P, Q);
	Cam->SetFieldOfView(Fov);
}

void AScene2Van::UpdateTarget()
{
	// The scanner's handshake wave drifts: slow enough to track with a held dial, never still.
	const float T = (Phase == EScene2Phase::Tune ? PhaseAge : 0.f);
	TargetFreq = 3.6f + 1.1f * FMath::Sin(0.22f * T + Seed) + 0.28f * FMath::Sin(0.8f * T + 2.f * Seed);
	TargetGain = 0.62f + 0.2f * FMath::Sin(0.4f * T + 1.f + Seed);
}

bool AScene2Van::InWindow() const
{
	const float Widen = 1.f + 0.2f * FMath::Min(Retakes, 3);
	return FMath::Abs(Freq - TargetFreq) <= Tune(TEXT("FreqTolerance"), FreqTolerance) * Widen
		&& FMath::Abs(Gain - TargetGain) <= Tune(TEXT("GainTolerance"), GainTolerance) * Widen;
}

void AScene2Van::TryPatch()
{
	if (Phase != EScene2Phase::Tune) { return; }
	if (Sync >= 1.f)
	{
		SetPhase(EScene2Phase::Locked);
		SinceLock = 0.f;
		Play(SfxLock);
		UE_LOG(LogScene2, Log, TEXT("S2 BZZT-LOCK at %.1f s, hesitation %.2f"), GetSceneSeconds(), Hesitation);
		return;
	}
	SinceSpark = 0.f;
	Hesitation = FMath::Min(1.f, Hesitation + Tune(TEXT("BadPatchCost"), BadPatchCost));
	Play(SfxSpark);
	Say(TEXT("MAX"), TEXT("Not yet, not yet! Sync first!"), LineNotYet);
}

bool AScene2Van::HandleSceneKey(const FKey& Key, bool bPressed)
{
	const bool bSkip = Key == EKeys::SpaceBar || Key == EKeys::Enter || Key == EKeys::E || Key == EKeys::Gamepad_FaceButton_Bottom;
	if (Phase == EScene2Phase::Opening)
	{
		if (bSkip && bPressed) { PhaseAge = OpeningBeats.X + OpeningBeats.Y + OpeningBeats.Z + OpeningBeats.W; }
		return bSkip;
	}
	if (Phase != EScene2Phase::Tune) { return bSkip; }

	if (Key == EKeys::A || Key == EKeys::D || Key == EKeys::Left || Key == EKeys::Right)
	{
		bool& Flag = Key == EKeys::A ? bKeyA : Key == EKeys::D ? bKeyD : Key == EKeys::Left ? bKeyL : bKeyR;
		Flag = bPressed;
		FreqHeld = static_cast<int8>((bKeyD ? 1 : 0) - (bKeyA ? 1 : 0));
		GainHeld = static_cast<int8>((bKeyR ? 1 : 0) - (bKeyL ? 1 : 0));
		if (bPressed)	// a nudge on the press frame, so the needle moves now
		{
			if (Key == EKeys::A || Key == EKeys::D) { Freq = FMath::Clamp(Freq + (Key == EKeys::D ? 0.06f : -0.06f), 1.f, 6.f); }
			else { Gain = FMath::Clamp(Gain + (Key == EKeys::Right ? 0.015f : -0.015f), 0.1f, 1.f); }
			SinceDial = 0.f;
			Play(SfxDial);
		}
		return true;
	}
	if (bSkip)
	{
		if (bPressed) { TryPatch(); }
		return true;
	}
	return false;
}

void AScene2Van::TickScene(float DeltaSeconds)
{
	using namespace Scene2Local;
	PhaseAge += DeltaSeconds;
	SinceDial += DeltaSeconds;
	SinceSpark += DeltaSeconds;
	SinceLock += DeltaSeconds;
	const float OpenLen = OpeningBeats.X + OpeningBeats.Y + OpeningBeats.Z + OpeningBeats.W;

	switch (Phase)
	{
	case EScene2Phase::Opening:
		PoseOpening(PhaseAge);
		if (PhaseAge >= OpenLen)
		{
			PoseOpening(OpenLen);
			SetPhase(EScene2Phase::Tune);
			Say(TEXT("MAX"), TEXT("Palm reader? Seriously?! Give me three seconds, Vance. Do not touch that glass yet!"), LineOpen);
		}
		break;

	case EScene2Phase::Tune:
	{
		// a breath of handheld drift on the settled shot: the van idles
		const FVector Wobble(0.f, 0.f, 0.6f * FMath::Sin(PhaseAge * 9.f) + 0.3f * FMath::Sin(PhaseAge * 23.f));
		Cam->SetWorldLocation(MaxCamPos + Wobble);

		if (FreqHeld != 0) { Freq = FMath::Clamp(Freq + FreqHeld * Tune(TEXT("FreqRate"), FreqRate) * DeltaSeconds, 1.f, 6.f); SinceDial = 0.f; }
		if (GainHeld != 0) { Gain = FMath::Clamp(Gain + GainHeld * Tune(TEXT("GainRate"), GainRate) * DeltaSeconds, 0.1f, 1.f); SinceDial = 0.f; }
		if ((FreqHeld != 0 || GainHeld != 0) && PhaseAge - LastDialSound > 0.12f) { Play(SfxDial); LastDialSound = PhaseAge; }

		UpdateTarget();
		const float FillRate = 1.f / FMath::Max(0.1f, Tune(TEXT("SyncSeconds"), SyncSeconds));
		Sync = InWindow() ? FMath::Min(1.f, Sync + FillRate * DeltaSeconds) : FMath::Max(0.f, Sync - 0.8f * DeltaSeconds);
		if (Sync > 0.6f && !bSaidClose) { bSaidClose = true; Say(TEXT("MAX"), TEXT("Come on, come on... that's it, hold it..."), LineClose); }

		Hesitation = FMath::Min(1.f, Hesitation + DeltaSeconds / FMath::Max(1.f, HesitationLimit()));
		if (Hesitation > 0.45f && !bSaidBouncerA) { bSaidBouncerA = true; Say(TEXT("BOUNCER"), TEXT("Sir? The glass."), LineBouncerA); }
		if (Hesitation > 0.75f && !bSaidBouncerB) { bSaidBouncerB = true; Say(TEXT("BOUNCER"), TEXT("Is there a problem, monsieur?"), LineBouncerB); }
		if (Hesitation >= 1.f)
		{
			SetPhase(EScene2Phase::Done);
			FailScene(TEXT("Hesitated"));
		}
		break;
	}

	case EScene2Phase::Locked:
		if (PhaseAge - DeltaSeconds < 0.3f && PhaseAge >= 0.3f)
		{
			Play(SfxToggle);
			Say(TEXT("MAX"), TEXT("You're a French duke now. Drop the hand."), LineLock);
		}
		if (PhaseAge >= Tune(TEXT("LockHold"), 2.6f))
		{
			SetPhase(EScene2Phase::Vance);
			FeedCapture->bCaptureEveryFrame = false;
			StopHum();
			CutTo(EScriptPov::Vance, VanceCamera);
			bPalmCalled = CallScene1(TEXT("PalmAccepted"));
			if (!bPalmCalled) { Say(TEXT("BOUNCER"), TEXT("Enjoy the screening, Your Grace.")); }
		}
		break;

	case EScene2Phase::Vance:
		if (ConsumeFlag(FlagThrough) || PhaseAge >= Tune(TEXT("VanceTimeout"), bPalmCalled ? VanceTimeout : 3.f))
		{
			SetPhase(EScene2Phase::Done);
			FinishScene();
		}
		break;

	default:
		break;
	}
}

// ---- HUD ----------------------------------------------------------------------------------------------------------------

void AScene2Van::DrawSceneHud(UCanvas* Canvas)
{
	using namespace Scene2Local;
	using namespace TakeStyle;
	if (!Canvas) { return; }
	FTakeHudKit& K = Kit;
	K.Begin(Canvas, FPlatformTime::Seconds(), 0.f);
	const float S = K.S;

	if (Phase == EScene2Phase::Opening)
	{
		// the windshield's fog as the camera pushes through it: a bloom of condensation, peaking on the glass
		const float Push0 = OpeningBeats.X + OpeningBeats.Y - 0.35f;
		const float Push1 = OpeningBeats.X + OpeningBeats.Y + OpeningBeats.Z;
		const float A = PhaseAge < Push0 || PhaseAge > Push1 + 0.4f ? 0.f
			: FMath::Sin(PI * FMath::Clamp((PhaseAge - Push0) / (Push1 + 0.4f - Push0), 0.f, 1.f));
		if (A > 0.f)
		{
			Fill(Canvas, 0.f, 0.f, K.W, K.H, FLinearColor(0.78f, 0.84f, 0.86f, 0.82f * A));
			for (int32 I = 0; I < 26; ++I)	// drips on the glass
			{
				const float X = FMath::Frac(FMath::Sin(I * 12.9898f) * 43758.5453f) * K.W;
				const float Y0 = FMath::Frac(FMath::Sin(I * 78.233f) * 12345.678f) * K.H * 0.6f;
				const float Len = (40.f + 90.f * FMath::Frac(I * 0.37f)) * S;
				Fill(Canvas, X, Y0, 2.f * S, Len, FLinearColor(0.45f, 0.52f, 0.55f, 0.55f * A));
			}
		}
		K.Rich(EType::Tag, TEXT("[SPACE] skip"), K.W - K.Px(110.f), K.H - K.Px(40.f), K.Px(200.f), 1, FTakeInk::Of(A_(Paper, 0.6f), A_(Paper, 0.6f)));
		return;
	}
	if (Phase != EScene2Phase::Tune && Phase != EScene2Phase::Locked) { return; }

	const float M = K.Px(48.f);
	const float PanelW = K.Px(600.f);
	// the main CRT: the live feed of Vance's hand over the scanner
	const float FeedH = PanelW * 0.75f;
	DrawFeed(Canvas, M, K.Px(96.f), PanelW, FeedH, S);

	// Vance's hesitation, under the feed
	{
		const float Y = K.Px(96.f) + FeedH + K.Px(30.f);
		const bool bHot = Hesitation > 0.75f;
		const float Pulse = bHot ? 0.5f + 0.5f * FMath::Sin(K.Now * 14.f) : 1.f;
		K.Text(EType::Tag, TEXT("Vance's hesitation"), M, Y, bHot ? Alarm : Bone);
		const float BY = Y + K.Px(18.f);
		Fill(Canvas, M, BY, PanelW, K.Px(16.f), Glass);
		Fill(Canvas, M, BY, PanelW * Hesitation, K.Px(16.f), FLinearColor(Alarm.R, Alarm.G, Alarm.B, 0.55f + 0.45f * Pulse));
		K.Frame(M, BY, PanelW, K.Px(16.f), K.Ln(1.f), A_(Bone, 0.7f));
		if (SinceSpark < 0.5f) { K.Text(EType::Tag, TEXT("+ suspicious"), M + PanelW * Hesitation + K.Px(8.f), BY + K.Px(8.f), Alarm); }
	}

	// the scope and dials, right side
	const float SX = K.W - M - PanelW;
	DrawScope(Canvas, SX, K.Px(96.f), PanelW, K.Px(300.f), S);

	// SYNC meter
	const float SyY = K.Px(96.f) + K.Px(300.f) + K.Px(22.f);
	{
		const bool bReady = Sync >= 1.f;
		const float Blink = bReady ? 0.5f + 0.5f * FMath::Sin(K.Now * 12.f) : 0.f;
		K.Text(EType::Tag, bReady ? TEXT("Sync: locked on") : TEXT("Sync"), SX, SyY, bReady ? Phosphor : Bone);
		const float BY = SyY + K.Px(18.f);
		Fill(Canvas, SX, BY, PanelW, K.Px(16.f), Glass);
		Fill(Canvas, SX, BY, PanelW * Sync, K.Px(16.f), FLinearColor(Phosphor.R, Phosphor.G, Phosphor.B, 0.6f + 0.4f * Blink));
		K.Frame(SX, BY, PanelW, K.Px(16.f), K.Ln(1.f), A_(Bone, 0.7f));
	}

	// the dials
	const float DY = SyY + K.Px(190.f);
	const float R = K.Px(92.f);
	DrawDial(Canvas, SX + PanelW * 0.25f, DY, R, (Freq - 1.f) / 5.f, TEXT("Frequency"), TEXT("A"), TEXT("D"), FreqHeld != 0 || (SinceDial < 0.15f && (bKeyA || bKeyD)), S);
	DrawDial(Canvas, SX + PanelW * 0.75f, DY, R, (Gain - 0.1f) / 0.9f, TEXT("Gain"), TEXT("LEFT"), TEXT("RIGHT"), GainHeld != 0, S);

	// the bypass patch: the cable and its jack, and the toggle Max slams
	{
		const float PY = DY + R + K.Px(70.f);
		const bool bReady = Sync >= 1.f && Phase == EScene2Phase::Tune;
		const bool bLocked = Phase == EScene2Phase::Locked;
		const FLinearColor Ink = bLocked ? Phosphor : bReady ? Paper : A_(Ash, 0.6f);
		const FTakeInk Words = FTakeInk::Of(Ink, Ink);
		const FString Prompt = bLocked ? TEXT("Bypass patched") : bReady ? TEXT("[SPACE] Patch the bypass cable") : TEXT("[SPACE] Patch the bypass (sync first)");
		K.Rich(EType::Goal, Prompt, SX + PanelW * 0.5f, PY, PanelW, 1, Words);
		// the toggle: down until the lock, then slammed up
		const float TX = SX + PanelW - K.Px(26.f), TY = DY - R;
		Fill(Canvas, TX - K.Px(18.f), TY, K.Px(36.f), K.Px(60.f), Glass);
		K.Frame(TX - K.Px(18.f), TY, K.Px(36.f), K.Px(60.f), K.Ln(1.f), A_(Bone, 0.8f));
		const float Up = bLocked ? Ease(SinceLock / 0.12f) : 0.f;
		const FVector2D Pivot(TX, TY + K.Px(30.f));
		const FVector2D Tip = Pivot + FVector2D(0.f, K.Px(26.f) * (1.f - 2.f * Up));
		Line(Canvas, Pivot, Tip, bLocked ? Phosphor : Bone, K.Ln(6.f));
	}

	// the teaching line, top centre: readable in ten seconds
	if (Phase == EScene2Phase::Tune)
	{
		const FTakeInk Teach = FTakeInk::Of(Paper, Phosphor);
		K.Rich(EType::Goal, TEXT("Match the scanner's amber wave:  [A] [D] frequency   [LEFT] [RIGHT] gain   then [SPACE] patch"),
			K.CX(), K.Px(44.f), K.W - 2.f * M, 1, Teach);
	}

	// BZZT-LOCK: a green flash and the stamp; a spark: a red flash
	if (SinceLock < 0.35f) { Fill(Canvas, 0.f, 0.f, K.W, K.H, FLinearColor(0.5f, 1.f, 0.6f, 0.45f * (1.f - SinceLock / 0.35f))); }
	if (Phase == EScene2Phase::Locked)
	{
		FTakeStampSpec Spec;
		Spec.Text = TEXT("BZZT-LOCK");
		Spec.Colour = Phosphor;
		K.Stamp(Spec, FVector2D(SX + PanelW * 0.5f, K.Px(96.f) + K.Px(150.f)), SinceLock, 40.f);
	}
	if (SinceSpark < 0.25f) { Fill(Canvas, 0.f, 0.f, K.W, K.H, FLinearColor(1.f, 0.25f, 0.1f, 0.3f * (1.f - SinceSpark / 0.25f))); }
}

void AScene2Van::DrawFeed(UCanvas* C, float X, float Y, float W, float H, float S)
{
	using namespace Scene2Local;
	using namespace TakeStyle;
	FTakeHudKit& K = Kit;
	Fill(C, X - 10.f * S, Y - 10.f * S, W + 20.f * S, H + 20.f * S, FLinearColor(0.05f, 0.05f, 0.045f, 0.95f));	// the bezel
	if (FeedTarget && FeedTarget->GetResource())
	{
		FCanvasTileItem T(FVector2D(X, Y), FeedTarget->GetResource(), FVector2D(W, H), FLinearColor(0.78f, 1.f, 0.82f, 1.f));
		T.BlendMode = SE_BLEND_Opaque;
		C->DrawItem(T);
	}
	else
	{
		Fill(C, X, Y, W, H, FLinearColor(0.02f, 0.03f, 0.02f, 1.f));
		K.Text(EType::Label, TEXT("No signal"), X + W * 0.5f, Y + H * 0.5f, Phosphor, ETakeAlign::Centre);
	}
	for (float LY = Y; LY < Y + H; LY += 3.f * S)	// scanlines
	{
		Fill(C, X, LY, W, FMath::Max(1.f, S), FLinearColor(0.f, 0.f, 0.f, 0.22f));
	}
	const float Roll = FMath::Frac(static_cast<float>(K.Now) * 0.23f);	// the rolling bar
	Fill(C, X, Y + Roll * (H - 30.f * S), W, 30.f * S, FLinearColor(0.6f, 1.f, 0.7f, 0.06f));
	K.Frame(X, Y, W, H, K.Ln(2.f), A_(Phosphor, 0.5f));
	K.Text(EType::Tag, TEXT("Cam 3  front podium"), X + K.Px(14.f), Y + K.Px(20.f), Phosphor);
	if (FMath::Frac(static_cast<float>(K.Now)) < 0.6f)
	{
		K.Text(EType::Tag, TEXT("Rec"), X + W - K.Px(14.f), Y + K.Px(20.f), Alarm, ETakeAlign::Right);
	}
	const int32 Sec = 7 + static_cast<int32>(GetSceneSeconds());
	K.Text(EType::Tag, FString::Printf(TEXT("23:41:%02d"), Sec % 60), X + K.Px(14.f), Y + H - K.Px(18.f), Phosphor);
}

void AScene2Van::DrawScope(UCanvas* C, float X, float Y, float W, float H, float S)
{
	using namespace Scene2Local;
	using namespace TakeStyle;
	FTakeHudKit& K = Kit;
	Fill(C, X, Y, W, H, Glass);
	for (int32 I = 1; I < 10; ++I) { Line(C, FVector2D(X + W * I / 10.f, Y), FVector2D(X + W * I / 10.f, Y + H), A_(PhosphorDim, 0.8f), 1.f); }
	for (int32 I = 1; I < 6; ++I) { Line(C, FVector2D(X, Y + H * I / 6.f), FVector2D(X + W, Y + H * I / 6.f), A_(PhosphorDim, 0.8f), 1.f); }
	K.Frame(X, Y, W, H, K.Ln(2.f), A_(Phosphor, 0.5f));
	K.Text(EType::Tag, TEXT("Scanner handshake"), X + K.Px(14.f), Y + K.Px(20.f), ScanAmber);
	K.Text(EType::Tag, TEXT("Your signal"), X + W - K.Px(14.f), Y + K.Px(20.f), Phosphor, ETakeAlign::Right);

	const float Ph = static_cast<float>(K.Now) * 4.f;
	const float CY = Y + H * 0.55f;
	const float Amp = H * 0.38f;
	const int32 N = 120;
	const bool bIn = InWindow();
	auto Wave = [&](float F, float G, const FLinearColor& Col, float Thick, float Jitter)
	{
		FVector2D Prev;
		for (int32 I = 0; I <= N; ++I)
		{
			const float U = I / static_cast<float>(N);
			const float J = Jitter * FMath::Sin(U * 97.f + Ph * 7.f);
			const FVector2D P(X + U * W, CY - (G * Amp) * FMath::Sin(2.f * PI * F * U + Ph) + J);
			if (I > 0) { Line(C, Prev, P, Col, Thick); }
			Prev = P;
		}
	};
	Wave(TargetFreq, TargetGain, ScanAmber, 3.f * S, 2.f * S);
	Wave(Freq, Gain, bIn ? Phosphor : A_(Phosphor, 0.75f), (bIn ? 3.5f : 2.f) * S, 0.f);
}

void AScene2Van::DrawDial(UCanvas* C, float CX, float CY, float R, float V, const FString& Label, const FString& KeyL,
	const FString& KeyR, bool bHeld, float S)
{
	using namespace Scene2Local;
	using namespace TakeStyle;
	FTakeHudKit& K = Kit;
	const FLinearColor Ring = bHeld ? Phosphor : Bone;
	// the knob: a filled disc of short radial strokes, a rim, ticks over 270 degrees, the needle
	const int32 Seg = 40;
	for (int32 I = 0; I < Seg; ++I)
	{
		const float A0 = 2.f * PI * I / Seg, A1 = 2.f * PI * (I + 1) / Seg;
		Line(C, FVector2D(CX + R * FMath::Cos(A0), CY + R * FMath::Sin(A0)), FVector2D(CX + R * FMath::Cos(A1), CY + R * FMath::Sin(A1)), Ring, K.Ln(2.f));
		Line(C, FVector2D(CX, CY), FVector2D(CX + R * 0.92f * FMath::Cos(A0), CY + R * 0.92f * FMath::Sin(A0)), Glass, K.Ln(R * 0.16f / S));
	}
	for (int32 I = 0; I <= 10; ++I)
	{
		const float A = FMath::DegreesToRadians(135.f + 27.f * I);
		Line(C, FVector2D(CX + R * 1.06f * FMath::Cos(A), CY + R * 1.06f * FMath::Sin(A)),
			FVector2D(CX + R * 1.18f * FMath::Cos(A), CY + R * 1.18f * FMath::Sin(A)), A_(Bone, 0.8f), K.Ln(2.f));
	}
	const float A = FMath::DegreesToRadians(135.f + 270.f * FMath::Clamp(V, 0.f, 1.f));
	Line(C, FVector2D(CX, CY), FVector2D(CX + R * 0.85f * FMath::Cos(A), CY + R * 0.85f * FMath::Sin(A)), bHeld ? Phosphor : ScanAmber, K.Ln(5.f));
	K.Diamond(CX, CY, K.Px(10.f), Ring);
	K.Text(EType::Label, Label, CX, CY + R + K.Px(26.f), Ring, ETakeAlign::Centre);
	const FTakeInk Keys = FTakeInk::Of(Ring, Ring);
	K.Rich(EType::Tag, FString::Printf(TEXT("[%s]"), *KeyL), CX - R - K.Px(34.f), CY, K.Px(120.f), 1, Keys);
	K.Rich(EType::Tag, FString::Printf(TEXT("[%s]"), *KeyR), CX + R + K.Px(34.f), CY, K.Px(120.f), 1, Keys);
}

