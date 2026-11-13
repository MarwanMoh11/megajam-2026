// Owner: (d). Constructor by the header pass; bodies by (d).

#include "RedTapeZone.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TakeShapes.h"
#include "TakeTypes.h"

ARedTapeZone::ARedTapeZone()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Area = CreateDefaultSubobject<UBoxComponent>(TEXT("Area"));
	Area->SetupAttachment(Root);
	FTakeShapes::SetupEditorOnlyBox(Area, Extent, FColor(230, 20, 20));

	// Four flat strips (cubes squashed to tape); OnConstruction lays them along the edges of Extent.
	const FVector Flat(1.f, 1.f, 0.01f);
	TapeNorth = FTakeShapes::CreateShape(this, TEXT("TapeNorth"), ETakeShape::Cube, Root, FVector(Extent.X, 0.f, 1.f), Flat, FRotator::ZeroRotator, false);
	TapeSouth = FTakeShapes::CreateShape(this, TEXT("TapeSouth"), ETakeShape::Cube, Root, FVector(-Extent.X, 0.f, 1.f), Flat, FRotator::ZeroRotator, false);
	TapeEast = FTakeShapes::CreateShape(this, TEXT("TapeEast"), ETakeShape::Cube, Root, FVector(0.f, Extent.Y, 1.f), Flat, FRotator::ZeroRotator, false);
	TapeWest = FTakeShapes::CreateShape(this, TEXT("TapeWest"), ETakeShape::Cube, Root, FVector(0.f, -Extent.Y, 1.f), Flat, FRotator::ZeroRotator, false);
}

void ARedTapeZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LayoutTape();
}

void ARedTapeZone::BeginPlay()
{
	Super::BeginPlay();
	LayoutTape();
}

void ARedTapeZone::LayoutTape()
{
	const float HalfX = FMath::Max(1.f, Extent.X);
	const float HalfY = FMath::Max(1.f, Extent.Y);
	const float Height = FMath::Max(1.f, Extent.Z);
	const float Width = FMath::Max(1.f, TapeWidth);

	// The outline covers the counted volume: floor up to 2 * Extent.Z.
	if (Area)
	{
		Area->SetRelativeLocation(FVector(0.f, 0.f, Height));
		Area->SetBoxExtent(FVector(HalfX, HalfY, Height));
	}

	// Cube mesh is 100 cm: scale = size / 100. North/South run along Y at +-X; East/West along X at +-Y.
	// The N/S strips are a tape width longer so the corners close.
	const float Thick = 0.01f;
	const FVector AlongY(Width / 100.f, (2.f * HalfY + Width) / 100.f, Thick);
	const FVector AlongX((2.f * HalfX) / 100.f, Width / 100.f, Thick);

	struct FStrip
	{
		UStaticMeshComponent* Comp;
		FVector Location;
		FVector Scale;
	};
	const FStrip Strips[] = {
		{ TapeNorth, FVector(HalfX, 0.f, 1.f), AlongY },
		{ TapeSouth, FVector(-HalfX, 0.f, 1.f), AlongY },
		{ TapeEast, FVector(0.f, HalfY, 1.f), AlongX },
		{ TapeWest, FVector(0.f, -HalfY, 1.f), AlongX },
	};
	for (const FStrip& Strip : Strips)
	{
		if (!Strip.Comp)
		{
			continue;
		}
		Strip.Comp->SetRelativeLocation(Strip.Location);
		Strip.Comp->SetRelativeScale3D(Strip.Scale);
		FTakeShapes::SetColour(Strip.Comp, TakeColors::Red);
	}
}

bool ARedTapeZone::Contains(const FVector& WorldPoint) const
{
	// The zone's own frame, yaw only. Extent is in cm before the actor's scale, and the strips scale with the actor,
	// so the tested area scales with it too.
	const FVector Delta = WorldPoint - GetActorLocation();
	const FVector Local = FRotator(0.f, GetActorRotation().Yaw, 0.f).UnrotateVector(Delta);
	const FVector Scaled = Extent * GetActorScale3D().GetAbs();
	return FMath::Abs(Local.X) <= Scaled.X
		&& FMath::Abs(Local.Y) <= Scaled.Y
		&& Delta.Z >= -50.f
		&& Delta.Z <= Scaled.Z * 2.f;
}
