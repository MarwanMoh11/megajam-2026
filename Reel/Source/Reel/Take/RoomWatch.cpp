// The room watches (QA wave 3). Owner: heist2. See RoomWatch.h.

#include "RoomWatch.h"

#include "CastMember.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "TakeAudio.h"
#include "TakeCharacter.h"
#include "TakeDirector.h"
#include "TakeShapes.h"
#include "TakeTypes.h"

namespace TakeRoomWatchLocal
{
	FBox ZoneBox(const FVector& A, const FVector& B)
	{
		return FBox(A.ComponentMin(B), A.ComponentMax(B));
	}

	void FitBox(UBoxComponent* Box, const FVector& A, const FVector& B)
	{
		if (!Box)
		{
			return;
		}
		const FBox Zone = ZoneBox(A, B);
		Box->SetWorldLocation(Zone.GetCenter());
		Box->SetWorldRotation(FRotator::ZeroRotator);
		Box->SetBoxExtent(Zone.GetExtent().ComponentMax(FVector(1.f)), false);
	}
}

ARoomWatch::ARoomWatch()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	FrontBox = CreateDefaultSubobject<UBoxComponent>(TEXT("FrontZone"));
	FrontBox->SetupAttachment(Root);
	FTakeShapes::SetupEditorOnlyBox(FrontBox, FVector(100.f), FColor(255, 170, 40));

	DaisBox = CreateDefaultSubobject<UBoxComponent>(TEXT("DaisZone"));
	DaisBox->SetupAttachment(Root);
	FTakeShapes::SetupEditorOnlyBox(DaisBox, FVector(100.f), FColor(230, 40, 40));

	MurmurLines.Add(TEXT("GUEST: \"Who's that up by the stage?\""));
	MurmurLines.Add(TEXT("GUEST: \"Is he with the auction house?\""));
	MurmurLines.Add(TEXT("GUEST: \"Sit down, man, we can't see.\""));
}

void ARoomWatch::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	TakeRoomWatchLocal::FitBox(FrontBox, FrontMin, FrontMax);
	TakeRoomWatchLocal::FitBox(DaisBox, DaisMin, DaisMax);
}

bool ARoomWatch::IsOnDais(const FVector& Feet) const
{
	return TakeRoomWatchLocal::ZoneBox(DaisMin, DaisMax).IsInsideOrOn(Feet);
}

bool ARoomWatch::IsInView(const FVector& Feet) const
{
	return IsOnDais(Feet) || TakeRoomWatchLocal::ZoneBox(FrontMin, FrontMax).IsInsideOrOn(Feet);
}

bool ARoomWatch::IsAtFront(const FVector& Feet) const
{
	const FBox Near = TakeRoomWatchLocal::ZoneBox(FrontMin, FrontMax).ExpandBy(FVector(ApproachMargin, ApproachMargin, 0.f));
	return IsInView(Feet) || Near.IsInsideOrOn(Feet);
}

void ARoomWatch::ResetRoom()
{
	Exposure = 0.f;
	bWasInView = false;
	bMurmured = false;
	bSeen = false;
	StareRenewSeconds = 0.f;
}

void ARoomWatch::GatherGuests()
{
	bGathered = true;
	Guests.Reset();
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}
	for (TActorIterator<ACastMember> It(World); It; ++It)
	{
		ACastMember* const Guest = *It;
		// The crew and Julian wear their costume pieces on their own characters: their heads are the player's.
		if (IsValid(Guest) && !Cast<ATakeCharacter>(Guest->GetAttachParentActor()))
		{
			Guests.Add(Guest);
		}
	}
}

int32 ARoomWatch::ReactNearest(const FVector& Head, int32 Count, int32 Pointers, float Seconds)
{
	if (!bGathered)
	{
		GatherGuests();
	}
	if (Count <= 0)
	{
		return 0;
	}
	struct FNear
	{
		ACastMember* Guest;
		float DistSq;
	};
	TArray<FNear> Near;
	const float RadiusSq = FMath::Square(ReactRadius);
	for (const TWeakObjectPtr<ACastMember>& Weak : Guests)
	{
		ACastMember* const Guest = Weak.Get();
		if (!Guest)
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Guest->GetActorLocation(), Head);
		if (DistSq <= RadiusSq && DistSq > FMath::Square(60.f))
		{
			Near.Add({Guest, DistSq});
		}
	}
	Near.Sort([](const FNear& A, const FNear& B) { return A.DistSq < B.DistSq; });
	const int32 Num = FMath::Min(Count, Near.Num());
	for (int32 Index = 0; Index < Num; ++Index)
	{
		if (Index < Pointers)
		{
			Near[Index].Guest->PlayGestureAt(FName(TEXT("Point")), Head);	// the cast contract (cast2)
		}
		Near[Index].Guest->LookAtPoint(Head, Seconds);
	}
	return Num;
}

ERoomReaction ARoomWatch::StepRoom(float DeltaSeconds, const FVector& Feet, const FVector& Head, bool bRoomBlind,
	ATakeDirector* Director)
{
	if (!bRoomWatches || bSeen)
	{
		return ERoomReaction::None;
	}

	const bool bInView = !bRoomBlind && IsInView(Feet);
	ERoomReaction Result = ERoomReaction::None;

	if (bInView)
	{
		// The step into view: the nearest few glance at him on this frame.
		if (!bWasInView && !bMurmured)
		{
			ReactNearest(Head, GlanceGuests, 0, FMath::Max(NoticeSeconds, 0.5f) + 0.5f);
			Result = ERoomReaction::Glance;
		}
		Exposure += DeltaSeconds * (IsOnDais(Feet) ? DaisRate : 1.f);

		if (!bMurmured && Exposure >= NoticeSeconds)
		{
			// Heads turn, two point, the murmur: all on this frame.
			bMurmured = true;
			StareRenewSeconds = StareSeconds * 0.5f;
			const int32 Turned = ReactNearest(Head, MurmurGuests, PointGuests, StareSeconds);
			if (Director)
			{
				if (Director->Audio)
				{
					Director->Audio->PlayCue(MurmurCue, MurmurVolume);
				}
				if (!bTaught && !TeachLine.IsEmpty())
				{
					Director->Flash(TeachLine, ETakeFlashKind::Line, 5.f);
				}
				else if (MurmurLines.Num() > 0)
				{
					const FString& Line = MurmurLines[MurmurLineIndex % MurmurLines.Num()];
					++MurmurLineIndex;
					if (!Line.IsEmpty())
					{
						Director->Flash(Line, ETakeFlashKind::Line, 2.5f);
					}
				}
			}
			bTaught = true;
			TAKE_LOG(TEXT("ROOM murmur: %d heads turn to Julian (%.1f s in view)"), Turned, Exposure);
			Result = ERoomReaction::Murmur;
		}
		else if (bMurmured)
		{
			// Still there: the stares are renewed so nobody looks away.
			StareRenewSeconds -= DeltaSeconds;
			if (StareRenewSeconds <= 0.f)
			{
				StareRenewSeconds = StareSeconds * 0.5f;
				ReactNearest(Head, MurmurGuests, 0, StareSeconds);
			}
		}

		if (Exposure >= SeenSeconds)
		{
			bSeen = true;
			if (Director && !SeenLine.IsEmpty())
			{
				Director->Flash(SeenLine, ETakeFlashKind::Line, 3.f);
			}
			TAKE_LOG(TEXT("ROOM saw him (%.1f s in view%s)"), Exposure, IsOnDais(Feet) ? TEXT(", on the dais") : TEXT(""));
			Result = ERoomReaction::Seen;
		}
	}
	else
	{
		Exposure = FMath::Max(0.f, Exposure - DeltaSeconds * CoolRate);
		if (bMurmured && Exposure < NoticeSeconds * 0.5f)
		{
			bMurmured = false;
			TAKE_LOG(TEXT("ROOM settles"));
		}
	}
	bWasInView = bInView;
	return Result;
}
