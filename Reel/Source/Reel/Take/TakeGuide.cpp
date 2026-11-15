// Where to go next. See TakeGuide.h. Owner: hud-guidance.

#include "TakeGuide.h"
#include "TakeShapes.h"
#include "TakeCharacter.h"
#include "CastMember.h"
#include "Script/ScriptSequencer.h"
#include "Script/ScriptScene.h"
#include "Components/SpotLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Sound/SoundBase.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogReelGuide, Log, All);

namespace TakeGuideLocal
{
	TAutoConsoleVariable<int32> CVarGuide(TEXT("Reel.Guide"), 1,
		TEXT("Where-to-go guidance: 1 on (lights, glances, crew line, fallback marker), 0 off."));

	FAutoConsoleCommandWithWorld LostCommand(TEXT("Reel.Guide.Lost"), TEXT("Guidance: act as if the player is lost now"),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			if (UTakeGuideSubsystem* G = UTakeGuideSubsystem::Get(World))
			{
				G->ForceLost();
			}
		}));

	bool On() { return CVarGuide.GetValueOnGameThread() != 0; }
}

// =====================================================================================================================
// AStageGuide
// =====================================================================================================================

AStageGuide::AStageGuide()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Pool = FTakeShapes::CreateCueSpot(this, TEXT("Pool"), Root, FVector(0.f, 0.f, 380.f), FRotator(-90.f, 0.f, 0.f), 600.f);
	Door = FTakeShapes::CreateCueRect(this, TEXT("Door"), Root, FVector(0.f, 0.f, 115.f), FRotator::ZeroRotator, 700.f);
}

FVector AStageGuide::GetGoalPoint() const
{
	return Highlight ? Highlight->GetActorLocation() : GetActorLocation();
}

void AStageGuide::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	const UWorld* World = GetWorld();
	const bool bEditorPreview = World && !World->IsGameWorld() && bPreviewInEditor && SceneNumber > 0;
	LitNow = bEditorPreview ? 1.f : 0.f;
	CallNow = 0.f;
	Apply(LitNow, 0.f, 0.0);
}

void AStageGuide::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	LitNow = FTakeShapes::Ease(LitNow, LitTarget, EaseRate, DeltaSeconds);
	CallNow = FTakeShapes::Ease(CallNow, CallTarget, EaseRate * 0.6f, DeltaSeconds);
	if (LitNow < 0.003f && LitTarget <= 0.f)
	{
		LitNow = 0.f;
	}
	const UWorld* World = GetWorld();
	Apply(LitNow, CallNow, World ? World->GetTimeSeconds() : 0.0);
}

void AStageGuide::Apply(float Lit, float Call, double Time)
{
	// The swell breathes (slow, never off): the light is asking, not blinking.
	const float Breath = 0.8f + 0.2f * FMath::Sin(static_cast<float>(Time) * 2.2f);
	const float Swell = 1.f + (FMath::Max(CallBoost, 1.f) - 1.f) * Call * Breath;
	const float Share = bTrail ? TrailShare : 1.f;

	if (Pool)
	{
		const float Height = FMath::Max(PoolHeight, 50.f);
		FVector At = GetGoalPoint();
		At.Z = Highlight ? At.Z : GetActorLocation().Z;
		Pool->SetWorldLocation(At + FVector(0.f, 0.f, Height));
		Pool->SetWorldRotation(FRotator(-90.f, 0.f, 0.f));
		Pool->SetAttenuationRadius(Height * 1.8f);
		FTakeShapes::SetCueCone(Pool, FMath::RadiansToDegrees(FMath::Atan2(FMath::Max(PoolRadius, 10.f), Height)), 0.35f);
		FTakeShapes::SetCueLight(Pool, bPool ? PoolCandelas * Share * Lit * Swell : 0.f, PoolColour);
	}
	if (Door)
	{
		Door->SetSourceWidth(DoorSize.X);
		Door->SetSourceHeight(DoorSize.Y);
		Door->SetRelativeLocation(FVector(0.f, 0.f, DoorSize.Y * 0.5f));
		Door->SetRelativeRotation(FRotator::ZeroRotator);
		FTakeShapes::SetCueLight(Door, (bDoorGlow && !bTrail) ? DoorCandelas * Lit * Swell : 0.f, PoolColour);
	}
}

// =====================================================================================================================
// UTakeGuideSubsystem
// =====================================================================================================================

UTakeGuideSubsystem* UTakeGuideSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UTakeGuideSubsystem>() : nullptr;
}

void UTakeGuideSubsystem::SetGoal(const FVector& At, const FString& Objective, const FString& CrewLine,
	const FString& CrewSpeaker)
{
	if (!bCodeGoal || !CodeGoal.Equals(At, 50.0))
	{
		bCodeCrewSaid = false;
	}
	bCodeGoal = true;
	CodeGoal = At;
	CodeObjective = Objective;
	CodeCrewLine = CrewLine;
	CodeCrewSpeaker = CrewSpeaker;
}

void UTakeGuideSubsystem::ClearGoal()
{
	bCodeGoal = false;
}

void UTakeGuideSubsystem::ForceLost()
{
	Lost = 1000.f;
	Best = 0.f;	// a warp or step closer must not count as "on his way" and clear it
	bCrewDue = false;
	bCrewSaidForGoal = true;
	UE_LOG(LogReelGuide, Log, TEXT("GUIDE forced lost (goal %s)"), bHaveGoal ? *GoalAt.ToCompactString() : TEXT("none"));
}

void UTakeGuideSubsystem::Reset(bool bNewScene)
{
	Posts.Reset();
	for (TActorIterator<AStageGuide> It(GetWorld()); It; ++It)
	{
		It->bDone = false;
		Posts.Add(*It);
	}
	if (bNewScene)
	{
		bCodeGoal = false;
	}
	bCodeCrewSaid = false;
	Goal = nullptr;
	bHaveGoal = false;
	LastGoalAt = FVector(1e9);
	Lost = 0.f;
	MarkerAlpha = 0.f;
	bCrewDue = false;
	bCrewSaidForGoal = false;
	bGlancedLost = false;
}

void UTakeGuideSubsystem::Glance(const FVector& At, const FVector& From, float Radius, int32 Count) const
{
	if (Radius <= 0.f || Count <= 0)
	{
		return;
	}
	TArray<TPair<float, ACastMember*>> Near;
	for (TActorIterator<ACastMember> It(GetWorld()); It; ++It)
	{
		ACastMember* M = *It;
		if (!M || M->IsHidden())
		{
			continue;
		}
		const float D = static_cast<float>(FVector::Dist(M->GetActorLocation(), From));
		if (D > 60.f && D < Radius)
		{
			Near.Emplace(D, M);
		}
	}
	Near.Sort([](const TPair<float, ACastMember*>& A, const TPair<float, ACastMember*>& B) { return A.Key < B.Key; });
	for (int32 i = 0; i < FMath::Min(Count, Near.Num()); ++i)
	{
		// Staggered a little by distance in how long they hold it: a glance, not a drill.
		Near[i].Value->LookAtPoint(At + FVector(0.f, 0.f, 120.f), 2.2f + 0.4f * i);
	}
}

void UTakeGuideSubsystem::Tick(float DeltaSeconds)
{
	UWorld* World = GetWorld();
	AScriptSequencer* Seq = AScriptSequencer::Get(World);
	AScriptScene* Scene = Seq && Seq->IsRunning() ? Seq->GetActiveScene() : nullptr;
	const bool bOn = TakeGuideLocal::On() && Scene && Seq->GetEndSeconds() < 0.f;

	if (Scene != LastScene.Get() || (Scene && Scene->GetRetakes() != LastRetakes))
	{
		Reset(Scene != LastScene.Get());
		LastScene = Scene;
		LastRetakes = Scene ? Scene->GetRetakes() : -1;
	}

	// ---- the active step: the lowest step in this scene with a goal not yet done ----------------------------------------
	int32 ActiveStep = MAX_int32;
	AStageGuide* GoalPost = nullptr;
	if (bOn)
	{
		const float SceneT = Scene->GetSceneSeconds();
		for (const TWeakObjectPtr<AStageGuide>& W : Posts)
		{
			AStageGuide* P = W.Get();
			if (!P || P->SceneNumber != Scene->SceneNumber || P->bTrail)
			{
				continue;
			}
			if (!P->bDone && !P->DoneFlag.IsNone() && Seq->Flags.Contains(P->DoneFlag))
			{
				P->bDone = true;
			}
			const bool bReady = (P->RequireFlag.IsNone() || Seq->Flags.Contains(P->RequireFlag)) && SceneT >= P->StartDelay;
			if (!P->bDone && bReady && P->Step < ActiveStep)
			{
				ActiveStep = P->Step;
				GoalPost = P;
			}
		}
	}

	ATakeCharacter* Char = Scene ? Seq->GetPovCharacter(Seq->GetPov()) : nullptr;
	FName RetakeReason;
	float RetakeAge = 0.f;
	const bool bWalking = bOn && Char && Scene->bPlayerWalks && !Seq->IsRetaking(RetakeReason, RetakeAge);

	// Reached: a post with no DoneFlag is done when he gets there.
	if (GoalPost && Char && GoalPost->DoneFlag.IsNone())
	{
		const FVector D = Char->GetActorLocation() - GoalPost->GetGoalPoint();
		if (D.Size2D() < GoalPost->ReachRadius && FMath::Abs(D.Z) < 300.f)
		{
			GoalPost->bDone = true;
			UE_LOG(LogReelGuide, Log, TEXT("GUIDE scene %d step %d reached (%s)"), Scene->SceneNumber, GoalPost->Step,
				*GoalPost->GetName());
			GoalPost = nullptr;	// the next step lights on the next frame
		}
	}

	// ---- the goal the marker and the wander check use: the scene's own, else the post's -------------------------------
	bHaveGoal = bOn && (bCodeGoal || GoalPost);
	if (bHaveGoal)
	{
		GoalAt = bCodeGoal ? CodeGoal : GoalPost->GetGoalPoint();
		GoalObjective = bCodeGoal ? CodeObjective : GoalPost->Objective;
	}
	Goal = GoalPost;

	const float CallNow = bHaveGoal ? FMath::Clamp((Lost - Seq->GetTune(TEXT("Guide.LostSeconds"), 11.f)) / 2.f, 0.f, 1.f) : 0.f;
	for (const TWeakObjectPtr<AStageGuide>& W : Posts)
	{
		AStageGuide* P = W.Get();
		if (!P)
		{
			continue;
		}
		const bool bLit = bOn && GoalPost && P->SceneNumber == Scene->SceneNumber && P->Step == ActiveStep;
		P->SetTarget(bLit ? 1.f : 0.f, (bLit && !P->bTrail) ? CallNow : 0.f);
	}

	if (!bHaveGoal || !Char)
	{
		Lost = 0.f;
		MarkerAlpha = FTakeShapes::Ease(MarkerAlpha, 0.f, 6.f, DeltaSeconds);
		return;
	}

	// ---- a new goal: start the clock, and the nearest cast glance at it -------------------------------------------------
	const float Dist = static_cast<float>(FVector::Dist2D(Char->GetActorLocation(), GoalAt));
	if (!GoalAt.Equals(LastGoalAt, 80.0))
	{
		LastGoalAt = GoalAt;
		Best = Dist;
		Lost = 0.f;
		bCrewDue = false;
		bCrewSaidForGoal = bCodeGoal ? bCodeCrewSaid : false;
		bGlancedLost = false;
		if (GoalPost)
		{
			Glance(GoalAt, Char->GetActorLocation(), GoalPost->GlanceRadius, FMath::Min(GoalPost->GlanceCount, 2));
		}
		UE_LOG(LogReelGuide, Log, TEXT("GUIDE scene %d goal %s (%s) %.0f cm away"), Scene->SceneNumber,
			*GoalAt.ToCompactString(), *GoalObjective, Dist);
	}

	// ---- lost: wandering without getting closer ---------------------------------------------------------------------------
	const float LostSeconds = Seq->GetTune(TEXT("Guide.LostSeconds"), 11.f);
	const float MarkerSeconds = Seq->GetTune(TEXT("Guide.MarkerSeconds"), 17.f);
	const float Reach = GoalPost ? GoalPost->ReachRadius : 200.f;
	if (!bWalking || Dist < Reach)
	{
		// Held for a beat, or there: no clock.
	}
	else if (Dist < Best - 150.f)
	{
		Best = Dist;
		Lost = FMath::Max(0.f, FMath::Min(Lost, MarkerSeconds) - 4.f);	// on his way: the marker fades after a few steps
	}
	else
	{
		const bool bMoving = Char->GetVelocity().Size2D() > 40.f;
		const float Rate = bMoving ? 1.f : (bSubtitleUp ? 0.f : Seq->GetTune(TEXT("Guide.IdleRate"), 0.3f));
		Lost += DeltaSeconds * Rate;
		Best = FMath::Min(Best, Dist);
	}

	if (Lost >= LostSeconds && !bGlancedLost)
	{
		bGlancedLost = true;
		if (GoalPost)
		{
			Glance(GoalAt, Char->GetActorLocation(), GoalPost->GlanceRadius, GoalPost->GlanceCount);
		}
		const bool bHasLine = bCodeGoal ? !CodeCrewLine.IsEmpty() : (GoalPost && !GoalPost->CrewLine.IsEmpty());
		bCrewDue = bHasLine && !bCrewSaidForGoal;
		UE_LOG(LogReelGuide, Log, TEXT("GUIDE lost %.0f s: glance%s"), Lost, bCrewDue ? TEXT(", crew line") : TEXT(""));
	}
	// The marker shows from the first second, every scene (Marwan's QA Nov 16: "visuals as to where to go should be shown
	// immediately always"). It used to wait for MarkerSeconds of wandering. The lost clock still drives the glance and
	// the crew line.
	const bool bMarker = bWalking && Dist >= Reach;
	MarkerAlpha = FTakeShapes::Ease(MarkerAlpha, bMarker ? 1.f : 0.f, bMarker ? 1.5f : 3.f, DeltaSeconds);
}

FTakeGuideView UTakeGuideSubsystem::GetView() const
{
	FTakeGuideView V;
	V.bShow = bHaveGoal && MarkerAlpha > 0.01f;
	V.Goal = GoalAt;
	V.Objective = GoalObjective;
	V.Alpha = MarkerAlpha;
	return V;
}

bool UTakeGuideSubsystem::TakeCrewLine(FString& OutSpeaker, FString& OutLine, USoundBase*& OutSound)
{
	if (!bCrewDue)
	{
		return false;
	}
	bCrewDue = false;
	bCrewSaidForGoal = true;
	if (bCodeGoal)
	{
		bCodeCrewSaid = true;
		OutSpeaker = CodeCrewSpeaker;
		OutLine = CodeCrewLine;
		OutSound = nullptr;
		return !OutLine.IsEmpty();
	}
	const AStageGuide* P = Goal.Get();
	if (!P || P->CrewLine.IsEmpty())
	{
		return false;
	}
	OutSpeaker = P->CrewSpeaker;
	OutLine = P->CrewLine;
	OutSound = P->CrewSound;
	return true;
}
