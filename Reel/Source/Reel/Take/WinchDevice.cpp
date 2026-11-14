// Vex's counterweight winch and the curtain (step 3). Owner: W. See WinchDevice.h for the contract.

#include "WinchDevice.h"

#include "BarClockComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TakeDirector.h"
#include "TakeShapes.h"

AWinchDevice::AWinchDevice()
{
	DeviceName = TEXT("WINCH");
	Effect = EDeviceEffect::Mask;
	Range = 0.f;
	Duration = 0.f;
	Uses = 1;
	FiredFrom = ETakeCamera::C;
	UseVerb = TEXT("Let the counterweight go: the curtain drops");
	LockedUntil = FName(TakeTriggers::Case);
	LandTrigger = FName(TakeTriggers::Winch);
}

namespace TakeWinchLocal
{
	/** The drop's progress (0..1) after Elapsed seconds of a DropSecs drop (0 = at once). */
	float DropAlpha(float Elapsed, float DropSecs)
	{
		return DropSecs > KINDA_SMALL_NUMBER ? FMath::Clamp(Elapsed / DropSecs, 0.f, 1.f) : 1.f;
	}

	/** Does the segment From-To cross the unit cube mesh (100 cm, pivot in the middle) under Transform? */
	bool SegmentCrossesCube(const FTransform& Transform, const FVector& From, const FVector& To)
	{
		const FVector LocalFrom = Transform.InverseTransformPosition(From);
		const FVector LocalTo = Transform.InverseTransformPosition(To);
		const FBox CubeBox(FVector(-50.0), FVector(50.0));
		if (CubeBox.IsInside(LocalFrom) || CubeBox.IsInside(LocalTo))
		{
			return true;
		}
		const FVector Span = LocalTo - LocalFrom;
		if (Span.IsNearlyZero())
		{
			return false;
		}
		return FMath::LineBoxIntersection(CubeBox, LocalFrom, LocalTo, Span);
	}
}

void AWinchDevice::BeginPlay()
{
	Super::BeginPlay();	// UsesLeft, the body's colour, the use ring

	bRunning = false;
	bLanded = false;
	RunSeconds = 0.f;
	bDropping = false;
	bCurtainDown = false;
	DropElapsed = 0.f;

	// One cloth panel per Panels entry, absolute transforms (the actor's place backstage does not matter).
	for (const TObjectPtr<UStaticMeshComponent>& OldPanel : PanelMeshes)
	{
		if (UStaticMeshComponent* Old = OldPanel.Get())
		{
			Old->DestroyComponent();
		}
	}
	PanelMeshes.Reset();

	for (int32 Index = 0; Index < Panels.Num(); ++Index)
	{
		UStaticMeshComponent* Mesh = FTakeShapes::AddShapeAtRuntime(this, ETakeShape::Cube, Root, FVector::ZeroVector,
			FVector::OneVector, CurtainColour, false);
		if (Mesh)
		{
			Mesh->SetMobility(EComponentMobility::Movable);
			Mesh->SetUsingAbsoluteLocation(true);
			Mesh->SetUsingAbsoluteRotation(true);
			Mesh->SetUsingAbsoluteScale(true);
		}
		// Kept even if null, so PanelMeshes[i] is always Panels[i].
		PanelMeshes.Add(Mesh);
	}

	SetCurtainDrop(0.f);
}

bool AWinchDevice::CanFire(FString& OutWhyNot) const
{
	// Locked first (the base says why); then once it is dropping or down it never fires again.
	if (!IsLocked() && (bDropping || bCurtainDown))
	{
		OutWhyNot = DeviceName + TEXT(" - the curtain is down");
		return false;
	}
	return Super::CanFire(OutWhyNot);
}

bool AWinchDevice::Fire()
{
	// Spends the use, bRunning, FiredFrame, "FIRE WINCH".
	if (!Super::Fire())
	{
		return false;
	}

	// The drop starts this frame (StepDevice skips the fire frame's time so it is not taken twice).
	bDropping = true;
	bCurtainDown = false;
	const UWorld* WinchWorld = GetWorld();
	DropElapsed = WinchWorld ? WinchWorld->GetDeltaSeconds() : 0.f;
	SetCurtainDrop(TakeWinchLocal::DropAlpha(DropElapsed, DropSeconds));
	return true;
}

void AWinchDevice::StepDevice(float DeltaSeconds)
{
	// Nothing before the fire and nothing after the landing: the curtain stays down for the rest of the take.
	if (!bDropping)
	{
		return;
	}

	if (!FiredThisFrame())
	{
		DropElapsed += DeltaSeconds;
		RunSeconds += DeltaSeconds;
	}

	const float Alpha = TakeWinchLocal::DropAlpha(DropElapsed, DropSeconds);
	SetCurtainDrop(Alpha);
	if (Alpha < 1.f)
	{
		return;
	}

	// Landed: once, for good. The director fires LandTrigger ("Winch") on this frame.
	bDropping = false;
	bCurtainDown = true;
	bRunning = false;
	MarkLanded();

	const ATakeDirector* Director = ATakeDirector::Get(this);
	if (UBarClockComponent* Clock = Director ? Director->Clock.Get() : nullptr)
	{
		Clock->PlayThud();
	}
	TAKE_LOG(TEXT("WINCH curtain down"));
}

void AWinchDevice::GatherTargets(const TArray<TObjectPtr<AWatcher>>& All, TArray<AWatcher*>& Out) const
{
	// It masks by geometry (BlocksSight), never by range.
	Out.Reset();
}

bool AWinchDevice::BlocksSight(const FVector& From, const FVector& To) const
{
	if (!bDropping && !bCurtainDown)
	{
		return false;
	}

	// Each panel where it hangs right now, tested in the cube mesh's own space (the banner's test).
	for (const TObjectPtr<UStaticMeshComponent>& PanelPtr : PanelMeshes)
	{
		const UStaticMeshComponent* Mesh = PanelPtr.Get();
		if (Mesh && TakeWinchLocal::SegmentCrossesCube(Mesh->GetComponentTransform(), From, To))
		{
			return true;
		}
	}
	return false;
}

void AWinchDevice::RestoreUses(int32 InUsesLeft)
{
	Super::RestoreUses(InUsesLeft);
	if (Uses <= 0)
	{
		return;
	}

	// The restore runs after BeginPlay: a spent winch means the curtain came down before the checkpoint. Down at
	// once, no sound, no landing (the director restores the trigger itself).
	bRunning = false;
	bDropping = false;
	if (UsesLeft <= 0)
	{
		bCurtainDown = true;
		DropElapsed = DropSeconds;
		SetCurtainDrop(1.f);
		TAKE_LOG(TEXT("WINCH curtain down (restored)"));
	}
	else
	{
		bCurtainDown = false;
		DropElapsed = 0.f;
		SetCurtainDrop(0.f);
	}
}

void AWinchDevice::SetCurtainDrop(float Alpha)
{
	const float Drop = FMath::Clamp(Alpha, 0.f, 1.f);
	const float Height = FMath::Max(CurtainHeight, 1.f);
	const float Thickness = FMath::Max(CurtainThickness, 1.f);
	const float FootLift = RaisedHeight * (1.f - Drop);

	for (int32 Index = 0; Index < PanelMeshes.Num(); ++Index)
	{
		UStaticMeshComponent* Mesh = PanelMeshes[Index].Get();
		if (!Mesh || !Panels.IsValidIndex(Index))
		{
			continue;
		}

		// The cube's X runs along A-B, Y is the cloth's thickness, Z its height; the foot sits on the line A-B
		// (at the middle of the two ends' heights) plus FootLift.
		const FTakeCurtainPanel& Panel = Panels[Index];
		const FVector Along = Panel.B - Panel.A;
		const float Length = FMath::Max(static_cast<float>(Along.Size()), 1.f);
		const FVector Along2D(Along.X, Along.Y, 0.0);
		const float Yaw = Along2D.IsNearlyZero() ? 0.f : static_cast<float>(Along2D.Rotation().Yaw);

		FVector Centre = (Panel.A + Panel.B) * 0.5;
		Centre.Z += FootLift + Height * 0.5f;

		Mesh->SetWorldLocationAndRotation(Centre, FRotator(0.f, Yaw, 0.f));
		Mesh->SetWorldScale3D(FVector(Length / 100.f, Thickness / 100.f, Height / 100.f));
	}
}
