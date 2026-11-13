// Owner: (c). Constructor by the header pass; every "// BODY: (c)" is a stub to fill.

#include "Mark.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TakeShapes.h"
#include "TakeTypes.h"

namespace TakeMarkSliceC
{
	// The disc's constructor scale (80 cm across, 2 cm thick) and how much it grows on hover.
	constexpr float DiscScaleXY = 0.8f;
	constexpr float DiscScaleZ = 0.02f;
	constexpr float HoverGrow = 1.15f;

	// One ring dot: a 14 x 7 cm dash, 3 cm tall, its top just above the disc.
	const FVector RingDotScale(0.14f, 0.07f, 0.03f);
	constexpr float RingDotZ = 2.5f;
	constexpr int32 MaxRingDots = 256;
}

AMark::AMark()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// An 80 cm gold disc, 2 cm thick, sitting on the floor.
	Disc = FTakeShapes::CreateShape(this, TEXT("Disc"), ETakeShape::Cylinder, Root, FVector(0.f, 0.f, 1.f), FVector(0.8f, 0.8f, 0.02f), FRotator::ZeroRotator, false);

	// The cued ring: instances (small flat blocks) laid out in BeginPlay; hidden until cued.
	Ring = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Ring"));
	Ring->SetupAttachment(Root);
	FTakeShapes::SetupShape(Ring, ETakeShape::Cube, false);
	Ring->SetVisibility(false);

	// The gold X under Julian, hidden until he stands here.
	CrossA = FTakeShapes::CreateShape(this, TEXT("CrossA"), ETakeShape::Cube, Root, FVector(0.f, 0.f, 3.f), FVector(1.0f, 0.12f, 0.02f), FRotator(0.f, 45.f, 0.f), false);
	CrossB = FTakeShapes::CreateShape(this, TEXT("CrossB"), ETakeShape::Cube, Root, FVector(0.f, 0.f, 3.f), FVector(1.0f, 0.12f, 0.02f), FRotator(0.f, -45.f, 0.f), false);
	CrossA->SetVisibility(false);
	CrossB->SetVisibility(false);

	// Generous click volume over the disc.
	ClickVolume = FTakeShapes::CreateClickVolume(this, TEXT("ClickVolume"), Root, FVector(70.f, 70.f, 40.f), FVector(0.f, 0.f, 40.f));
}

void AMark::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LayOutRing();
	ApplyLook();
}

void AMark::BeginPlay()
{
	Super::BeginPlay();
	bIsCued = false;
	bIsOccupied = false;
	bIsHovered = false;
	LayOutRing();
	ApplyLook();
}

void AMark::SetHovered(bool bHovered)
{
	bIsHovered = bHovered;
	ApplyLook();
}

void AMark::SetCued(bool bCued)
{
	bIsCued = bCued;
	ApplyLook();
}

void AMark::SetOccupied(bool bOccupied)
{
	bIsOccupied = bOccupied;
	ApplyLook();
}

FVector AMark::GetStandPoint() const
{
	return GetActorLocation();
}

void AMark::LayOutRing()
{
	if (!Ring || !CrossA || !CrossB)
	{
		return;
	}

	Ring->ClearInstances();
	const int32 DotCount = FMath::Clamp(RingDots, 0, TakeMarkSliceC::MaxRingDots);
	for (int32 DotIndex = 0; DotIndex < DotCount; ++DotIndex)
	{
		const float AngleDeg = 360.f * static_cast<float>(DotIndex) / static_cast<float>(DotCount);
		const float AngleRad = FMath::DegreesToRadians(AngleDeg);
		const FVector DotLocation(FMath::Cos(AngleRad) * RingRadius, FMath::Sin(AngleRad) * RingRadius, TakeMarkSliceC::RingDotZ);
		// The dash runs along the circle (its X axis on the tangent).
		const FRotator DotRotation(0.f, AngleDeg + 90.f, 0.f);
		Ring->AddInstance(FTransform(DotRotation, DotLocation, TakeMarkSliceC::RingDotScale), false);
	}

	FTakeShapes::SetColour(Ring, TakeColors::GoldBright);
	FTakeShapes::SetColour(CrossA, TakeColors::GoldBright);
	FTakeShapes::SetColour(CrossB, TakeColors::GoldBright);
}

void AMark::ApplyLook()
{
	if (!Disc || !Ring || !CrossA || !CrossB)
	{
		return;
	}

	FTakeShapes::SetColour(Disc, (bIsHovered || bIsCued) ? TakeColors::GoldBright : TakeColors::Gold);
	const float DiscXY = TakeMarkSliceC::DiscScaleXY * (bIsHovered ? TakeMarkSliceC::HoverGrow : 1.f);
	Disc->SetRelativeScale3D(FVector(DiscXY, DiscXY, TakeMarkSliceC::DiscScaleZ));

	Ring->SetVisibility(bIsCued);
	CrossA->SetVisibility(bIsOccupied);
	CrossB->SetVisibility(bIsOccupied);
}
