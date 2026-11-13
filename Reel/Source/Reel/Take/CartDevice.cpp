// Owner: (u). Constructor by the header pass; every "// BODY: (u)" is a stub to fill.

#include "CartDevice.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TakeShapes.h"

ACartDevice::ACartDevice()
{
	DeviceName = TEXT("CART");
	UseVerb = TEXT("Release the cart");
	Effect = EDeviceEffect::Lure;
	Range = 1200.f;
	Duration = 6.f;
	Uses = 1;
	FiredFrom = ETakeCamera::B;
	BodyColour = TakeColors::White;

	// The cart: a 100 x 60 x 90 block at its door (Body moves along the roll; the actor stays at the door).
	Body->SetRelativeLocation(FVector(0.f, 0.f, 45.f));
	Body->SetRelativeScale3D(FVector(1.0f, 0.6f, 0.9f));

	CrashMarker = FTakeShapes::CreateShape(this, TEXT("CrashMarker"), ETakeShape::Cube, Root, CrashOffset + FVector(0.f, 0.f, 1.f), FVector(0.4f, 0.4f, 0.02f), FRotator(0.f, 45.f, 0.f), false);
	CrashMarker->SetHiddenInGame(true);
}

void ACartDevice::BeginPlay()
{
	Super::BeginPlay();

	bCrashed = false;
	if (Body)
	{
		BodyHome = Body->GetRelativeLocation();
	}
}

bool ACartDevice::Fire()
{
	if (!Super::Fire())
	{
		return false;
	}

	// The first step of the roll, this frame (StepDevice skips the fire frame's time so it is not taken twice).
	bCrashed = false;
	const UWorld* CartWorld = GetWorld();
	RunSeconds = CartWorld ? CartWorld->GetDeltaSeconds() : 0.f;
	PlaceCart(GetRollAlpha(), false);
	return true;
}

void ACartDevice::StepDevice(float DeltaSeconds)
{
	if (!bRunning || bCrashed)
	{
		return;
	}

	if (!FiredThisFrame())
	{
		RunSeconds += DeltaSeconds;
	}

	const float Alpha = GetRollAlpha();
	if (Alpha < 1.f)
	{
		PlaceCart(Alpha, false);
		return;
	}

	// The crash: it tips over at the crash point and the lure lands this frame.
	bCrashed = true;
	bRunning = false;
	PlaceCart(1.f, true);
	MarkLanded();
	TAKE_LOG(TEXT("%s CRASHES"), *DeviceName);
}

FVector ACartDevice::GetEffectPoint() const
{
	return GetActorTransform().TransformPosition(CrashOffset);
}

float ACartDevice::GetRollAlpha() const
{
	if (RollSeconds <= KINDA_SMALL_NUMBER)
	{
		return 1.f;
	}
	return FMath::Clamp(RunSeconds / RollSeconds, 0.f, 1.f);
}

void ACartDevice::PlaceCart(float Alpha, bool bTipped)
{
	if (!Body)
	{
		return;
	}

	const FVector Flat(CrashOffset.X, CrashOffset.Y, 0.f);
	const double TravelYaw = Flat.IsNearlyZero() ? 0.0 : Flat.Rotation().Yaw;

	FVector CartLocation = BodyHome + CrashOffset * Alpha;
	FRotator CartRotation(0.0, TravelYaw, 0.0);
	if (bTipped)
	{
		// On its side: rolled 90 degrees about its travel axis, resting on what was its flank.
		CartRotation.Roll = 90.0;
		const FVector Scale = Body->GetRelativeScale3D();
		CartLocation.Z = BodyHome.Z + CrashOffset.Z - Scale.Z * 50.0 + Scale.Y * 50.0;
	}
	Body->SetRelativeLocationAndRotation(CartLocation, CartRotation);
}
