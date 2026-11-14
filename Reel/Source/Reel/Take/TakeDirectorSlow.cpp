// QA time (Nov 14, Marwan's note 2): slow time, for the switching tutorial. The contract is the "QA time" block in
// TakeDirector.h. The world slows through the global time dilation; the bar clock slows with it (the director advances
// it on the slowed time) and holds the music (UBarClockComponent::SetSlowAmount); voices and the HUD run on real time.

#include "TakeDirector.h"

#include "BarClockComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TakeAudio.h"
#include "TakeTypes.h"

namespace TakeSlowLocal
{
	/** Smoothstep: eases in and out. */
	float Ease(float Alpha)
	{
		const float A = FMath::Clamp(Alpha, 0.f, 1.f);
		return A * A * (3.f - 2.f * A);
	}

	/** The scale the reasons ask for (the slowest), 1 with none. */
	float Target(const TMap<FName, float>& Reasons)
	{
		float Scale = 1.f;
		for (const TPair<FName, float>& Each : Reasons)
		{
			Scale = FMath::Min(Scale, Each.Value);
		}
		return Scale;
	}

	/** At this scale (or slower) the music is fully held; between it and 1 the hold is partial. */
	constexpr float FullHoldScale = 0.15f;
}

void ATakeDirector::BeginSlow(FName Reason, float Scale, float EaseSeconds)
{
	if (!bSlowTimeEnabled)
	{
		return;
	}
	if (Phase != ETakePhase::Rolling || bReloadRequested)
	{
		TAKE_LOG(TEXT("SLOW %s not begun: the take is not rolling"), *Reason.ToString());
		return;
	}
	const float Floor = FMath::Clamp(MinSlowScale, 0.01f, 1.f);
	const float Clamped = FMath::Clamp(Scale, Floor, 1.f);
	if (SlowReasons.Num() == 0)
	{
		SlowHeldSeconds = 0.f;
	}
	const bool bAgain = SlowReasons.Contains(Reason);
	SlowReasons.Add(Reason, Clamped);	// a second BeginSlow with this reason replaces its scale

	SlowEaseFrom = SlowScaleNow;
	SlowEaseTo = TakeSlowLocal::Target(SlowReasons);
	SlowEaseSeconds = FMath::Max(0.f, EaseSeconds);
	SlowEaseElapsed = 0.f;
	TAKE_LOG(TEXT("SLOW %s %s: x%.2f over %.2f s (now x%.2f)"), *Reason.ToString(), bAgain ? TEXT("again") : TEXT("begins"),
		Clamped, SlowEaseSeconds, SlowScaleNow);
}

void ATakeDirector::EndSlow(FName Reason, float EaseSeconds)
{
	if (SlowReasons.Remove(Reason) == 0)
	{
		return;	// not on: nothing to end
	}
	SlowEaseFrom = SlowScaleNow;
	SlowEaseTo = TakeSlowLocal::Target(SlowReasons);
	SlowEaseSeconds = FMath::Max(0.f, EaseSeconds);
	SlowEaseElapsed = 0.f;
	TAKE_LOG(TEXT("SLOW %s ends: back to x%.2f over %.2f s"), *Reason.ToString(), SlowEaseTo, SlowEaseSeconds);
}

bool ATakeDirector::IsSlowed() const
{
	return SlowReasons.Num() > 0 || SlowScaleNow < 0.999f;
}

float ATakeDirector::VoiceSecondsLeft() const
{
	return Audio ? Audio->GetVoiceSecondsLeft() : 0.f;
}

void ATakeDirector::StepSlow(float RealDeltaSeconds)
{
	const float Dt = FMath::Max(0.f, RealDeltaSeconds);
	float Applied = SlowScaleNow;

	// A blooper (the freeze), the end sequence, the title card or a reload on its way: normal speed at once, so the
	// freeze and the slate run their real length. The reload then starts a fresh world at normal speed anyway.
	const bool bMustReset = Phase == ETakePhase::BlooperFreeze || Phase == ETakePhase::Printed
		|| Phase == ETakePhase::Title || bReloadRequested;
	if (bMustReset && (SlowReasons.Num() > 0 || SlowScaleNow != 1.f || SlowEaseTo != 1.f))
	{
		TAKE_LOG(TEXT("SLOW reset (%d reasons)"), SlowReasons.Num());
		SlowReasons.Reset();
		SlowScaleNow = 1.f;
		SlowEaseFrom = 1.f;
		SlowEaseTo = 1.f;
		SlowEaseSeconds = 0.f;
		SlowEaseElapsed = 0.f;
		Applied = -1.f;	// apply below
	}

	// The safety net (off by default).
	if (SlowReasons.Num() > 0)
	{
		SlowHeldSeconds += Dt;
		if (MaxSlowHoldSeconds > 0.f && SlowHeldSeconds > MaxSlowHoldSeconds)
		{
			TAKE_LOG(TEXT("SLOW ends by itself after %.1f s (no EndSlow)"), SlowHeldSeconds);
			SlowReasons.Reset();
			SlowEaseFrom = SlowScaleNow;
			SlowEaseTo = 1.f;
			SlowEaseSeconds = 0.35f;
			SlowEaseElapsed = 0.f;
		}
	}

	// The ease, on real time.
	if (SlowScaleNow != SlowEaseTo)
	{
		SlowEaseElapsed += Dt;
		const float Alpha = SlowEaseSeconds > 0.f ? SlowEaseElapsed / SlowEaseSeconds : 1.f;
		SlowScaleNow = Alpha >= 1.f ? SlowEaseTo : FMath::Lerp(SlowEaseFrom, SlowEaseTo, TakeSlowLocal::Ease(Alpha));
	}
	if (SlowScaleNow != Applied)
	{
		UGameplayStatics::SetGlobalTimeDilation(this, SlowScaleNow);
	}

	// The band: held under the filter while slowed, back in on the grid after (the clock's side).
	if (Clock)
	{
		const float Amount = FMath::Clamp((1.f - SlowScaleNow) / (1.f - TakeSlowLocal::FullHoldScale), 0.f, 1.f);
		Clock->SetSlowAmount(Amount, Dt);
	}
}
