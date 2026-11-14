// Owner: (u). Constructor by the header pass; every "// BODY: (u)" is a stub to fill.

#include "BannerDevice.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TakeShapes.h"

ABannerDevice::ABannerDevice()
{
	DeviceName = TEXT("BANNER");
	UseVerb = TEXT("Drop the banner");
	Effect = EDeviceEffect::Mask;
	Range = 600.f;
	Duration = 10.f;
	Uses = 1;
	FiredFrom = ETakeCamera::B;
	BodyColour = TakeColors::DarkGrey;

	// Place the actor at the batten, up in the flies. The matte (Body) hangs under it, rolled up (Up phase).
	Batten = FTakeShapes::CreateShape(this, TEXT("Batten"), ETakeShape::Cube, Root, FVector::ZeroVector, FVector(0.1f, MatteSize.Y / 100.f + 0.2f, 0.1f));

	Body->SetRelativeLocation(FVector(0.f, 0.f, -MatteSize.Z * 0.5f));
	Body->SetRelativeScale3D(MatteSize / 100.f);

	ClickVolume->SetRelativeLocation(FVector(0.f, 0.f, -MatteSize.Z * 0.5f));
	ClickVolume->InitBoxExtent(FVector(60.f, MatteSize.Y * 0.5f + 20.f, MatteSize.Z * 0.5f + 20.f));
}

void ABannerDevice::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LayoutBanner();
}

void ABannerDevice::BeginPlay()
{
	Super::BeginPlay();

	BannerPhase = EBannerPhase::Up;
	PhaseSeconds = 0.f;
	LayoutBanner();
}

bool ABannerDevice::CanFire(FString& OutWhyNot) const
{
	switch (BannerPhase)
	{
	case EBannerPhase::Dropping:
	case EBannerPhase::Down:
		OutWhyNot = DeviceName + TEXT(" is still down");
		return false;
	case EBannerPhase::Raising:
		OutWhyNot = DeviceName + TEXT(" is winching back up");
		return false;
	default:
		break;
	}
	return Super::CanFire(OutWhyNot);
}

bool ABannerDevice::Fire()
{
	if (!Super::Fire())
	{
		return false;
	}

	// The drop starts this frame (StepDevice skips the fire frame's time so it is not taken twice).
	BannerPhase = EBannerPhase::Dropping;
	const UWorld* BannerWorld = GetWorld();
	PhaseSeconds = BannerWorld ? BannerWorld->GetDeltaSeconds() : 0.f;
	SetMatteDrop(DropSeconds > KINDA_SMALL_NUMBER ? FMath::Clamp(PhaseSeconds / DropSeconds, 0.f, 1.f) : 1.f);
	return true;
}

void ABannerDevice::StepDevice(float DeltaSeconds)
{
	if (!bRunning)
	{
		return;
	}

	if (!FiredThisFrame())
	{
		PhaseSeconds += DeltaSeconds;
		RunSeconds += DeltaSeconds;
	}

	switch (BannerPhase)
	{
	case EBannerPhase::Dropping:
	{
		const float DropAlpha = DropSeconds > KINDA_SMALL_NUMBER ? FMath::Clamp(PhaseSeconds / DropSeconds, 0.f, 1.f) : 1.f;
		SetMatteDrop(DropAlpha);
		if (DropAlpha >= 1.f)
		{
			// Down: the Mask lands this frame, then it holds for Duration.
			BannerPhase = EBannerPhase::Down;
			PhaseSeconds = 0.f;
			MarkLanded();
			TAKE_LOG(TEXT("%s DOWN"), *DeviceName);
		}
		break;
	}
	case EBannerPhase::Down:
		if (PhaseSeconds >= Duration)
		{
			BannerPhase = EBannerPhase::Raising;
			PhaseSeconds = 0.f;
			TAKE_LOG(TEXT("%s WINCHING UP"), *DeviceName);
		}
		break;
	case EBannerPhase::Raising:
	{
		const float RaiseAlpha = RaiseSeconds > KINDA_SMALL_NUMBER ? FMath::Clamp(PhaseSeconds / RaiseSeconds, 0.f, 1.f) : 1.f;
		SetMatteDrop(1.f - RaiseAlpha);
		if (RaiseAlpha >= 1.f)
		{
			bRunning = false;
			PhaseSeconds = 0.f;
			BannerPhase = (Uses > 0 && UsesLeft <= 0) ? EBannerPhase::Spent : EBannerPhase::Up;
			TAKE_LOG(TEXT("%s UP%s"), *DeviceName, BannerPhase == EBannerPhase::Spent ? TEXT(", spent") : TEXT(""));
		}
		break;
	}
	default:
		break;
	}
}

FVector ABannerDevice::GetEffectPoint() const
{
	return GetActorTransform().TransformPosition(EffectOffset);
}

void ABannerDevice::LayoutBanner()
{
	if (Batten)
	{
		Batten->SetRelativeScale3D(FVector(0.1f, MatteSize.Y / 100.f + 0.2f, 0.1f));
		FTakeShapes::SetColour(Batten, BattenColour);
	}
	if (Body)
	{
		Body->SetRelativeScale3D(MatteSize / 100.f);
		FTakeShapes::SetColour(Body, BodyColour);
	}
	if (ClickVolume)
	{
		ClickVolume->SetBoxExtent(FVector(60.f, MatteSize.Y * 0.5f + 20.f, MatteSize.Z * 0.5f + 20.f));
	}
	SetMatteDrop(0.f);
}

void ABannerDevice::SetMatteDrop(float Alpha)
{
	const FVector MatteLocation(0.0, 0.0, -MatteSize.Z * 0.5 - DropDistance * FMath::Clamp(Alpha, 0.f, 1.f));
	if (Body)
	{
		Body->SetRelativeLocation(MatteLocation);
	}
	if (ClickVolume)
	{
		ClickVolume->SetRelativeLocation(MatteLocation);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Step 2 (header pass stubs; BODY: D)
// ---------------------------------------------------------------------------------------------------------------------

bool ABannerDevice::BlocksSight(const FVector& From, const FVector& To) const
{
	if (!Body)
	{
		return false;
	}
	if (BannerPhase != EBannerPhase::Dropping && BannerPhase != EBannerPhase::Down && BannerPhase != EBannerPhase::Raising)
	{
		return false;
	}

	// The matte where it hangs right now: test the segment in the cube mesh's own space (the basic cube is 100 cm
	// across, pivot at the centre), so a rotated banner is tested as the thin slab it is, not its world bounds.
	const FTransform& MatteTransform = Body->GetComponentTransform();
	const FVector LocalFrom = MatteTransform.InverseTransformPosition(From);
	const FVector LocalTo = MatteTransform.InverseTransformPosition(To);
	const FBox MatteBox(FVector(-50.0), FVector(50.0));
	if (MatteBox.IsInside(LocalFrom) || MatteBox.IsInside(LocalTo))
	{
		return true;
	}
	const FVector Span = LocalTo - LocalFrom;
	if (Span.IsNearlyZero())
	{
		return false;
	}
	return FMath::LineBoxIntersection(MatteBox, LocalFrom, LocalTo, Span);
}

void ABannerDevice::RestoreUses(int32 InUsesLeft)
{
	Super::RestoreUses(InUsesLeft);
	if (Uses <= 0)
	{
		return;
	}

	// The restore runs after BeginPlay, so the banner is Up and still: a spent one is rolled up for good.
	bRunning = false;
	PhaseSeconds = 0.f;
	RunSeconds = 0.f;
	BannerPhase = UsesLeft <= 0 ? EBannerPhase::Spent : EBannerPhase::Up;
	SetMatteDrop(0.f);
}
