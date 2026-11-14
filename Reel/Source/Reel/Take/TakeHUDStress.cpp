// QA hud (Marwan's note 7): composure is the picture, not a meter. One eased stress value, from composure on one
// curve, drives the overlay's cues (FTakeFilmOverlay::DrawStress: wash, vignette, burn, dirt, pulse) and the film
// grade's "Stress" scalar (package look adds it to the material; a material without it ignores the override). A
// watcher holding Julian right now adds a pull toward its side. Real time throughout: a slowed world (the tutorial's
// slow motion) does not slow the picture's answer.

#include "TakeHUD.h"

#include "Camera/PlayerCameraManager.h"
#include "ComposureComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TakeDirector.h"
#include "Watcher.h"

namespace TakeHudStress
{
	/** Value eased toward Target with time constant Seconds over Dt (exponential, frame-rate independent). */
	float Ease(float Value, float Target, float Dt, float Seconds)
	{
		if (Dt <= 0.f)
		{
			return Value;
		}
		const float K = 1.f - FMath::Exp(-Dt / FMath::Max(0.01f, Seconds));
		const float Next = Value + (Target - Value) * K;
		return FMath::Abs(Next - Target) < 0.001f ? Target : Next;
	}
}

void ATakeHUD::UpdateStress(const ATakeDirector& Dir, FTakeFilmFrame& F)
{
	using namespace TakeHudStress;
	const float Dt = StressTime >= 0.0 ? FMath::Clamp(static_cast<float>(F.RealTime - StressTime), 0.f, 0.1f) : 0.f;
	StressTime = F.RealTime;

	// The take's picture: rolling, cleared, or frozen on a blooper (the drained frame holds). Cards are calm.
	const bool bPlay = Dir.Phase == ETakePhase::Rolling || Dir.Phase == ETakePhase::Cleared
		|| Dir.Phase == ETakePhase::BlooperFreeze;
	const float Fraction = Dir.Composure ? FMath::Clamp(Dir.Composure->GetFraction(), 0.f, 1.f) : 1.f;
	const float Target = FMath::Pow(1.f - Fraction, FMath::Max(0.1f, StressCurve));
	Stress = bPlay ? Ease(Stress, Target, Dt, Target > Stress ? StressRiseSeconds : StressFallSeconds) : 0.f;

	// Who holds him, and from where: each watcher with its tally lit, as a direction on screen from the camera's
	// heading (to the side: left or right edge; ahead: the top edge, behind: the bottom, both weighted down).
	const bool bHeld = Dir.Phase == ETakePhase::Rolling && Dir.bJulianSeen;
	FVector2D Toward = FVector2D::ZeroVector;
	const APlayerController* PC = GetOwningPlayerController();
	if (bHeld && PC && PC->PlayerCameraManager)
	{
		const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
		const float Yaw = static_cast<float>(FMath::DegreesToRadians(PC->PlayerCameraManager->GetCameraRotation().Yaw));
		const FVector Fwd(FMath::Cos(Yaw), FMath::Sin(Yaw), 0.f);
		const FVector Right(-FMath::Sin(Yaw), FMath::Cos(Yaw), 0.f);
		for (const TObjectPtr<AWatcher>& WatcherPtr : Dir.Watchers)
		{
			const AWatcher* Watcher = WatcherPtr.Get();
			if (!Watcher || !Watcher->HasTally())
			{
				continue;
			}
			const FVector D = Watcher->GetActorLocation() - CamLoc;
			const float Ahead = static_cast<float>(FVector::DotProduct(D, Fwd));
			const float Side = static_cast<float>(FVector::DotProduct(D, Right));
			if (FMath::IsNearlyZero(Ahead) && FMath::IsNearlyZero(Side))
			{
				continue;
			}
			const float Angle = FMath::Atan2(Side, Ahead);	// 0 ahead, positive to the right
			Toward += FVector2D(FMath::Sin(Angle), -FMath::Cos(Angle) * SeenVerticalWeight);
		}
		Toward = Toward.GetClampedToMaxSize(1.0);
	}
	const bool bFresh = SeenWeight < 0.05f;
	SeenWeight = bPlay ? Ease(SeenWeight, bHeld ? 1.f : 0.f, Dt, bHeld ? SeenRiseSeconds : SeenFallSeconds) : 0.f;
	if (bHeld)
	{
		// A new hold points at once; while held the direction follows the watcher smoothly. Let go, it stays put
		// while the cue fades.
		const float K = Dt > 0.f ? 1.f - FMath::Exp(-Dt / 0.12f) : 0.f;
		SeenDir = bFresh ? Toward : SeenDir + (Toward - SeenDir) * static_cast<double>(K);
	}

	F.Stress = bComposureInPicture ? Stress : 0.f;
	F.SeenWeight = bComposureInPicture ? SeenWeight : 0.f;
	F.SeenDir = SeenDir;

	// The grade: the same stress, written when it changes or the director made a new instance.
	if (bDriveGradeStress && !StressParameter.IsNone())
	{
		UMaterialInstanceDynamic* MID = Dir.GradeMID.Get();
		if (MID && (MID != StressMID.Get() || FMath::Abs(Stress - AppliedStress) > 0.002f))
		{
			MID->SetScalarParameterValue(StressParameter, Stress);
			StressMID = MID;
			AppliedStress = Stress;
		}
	}
}
