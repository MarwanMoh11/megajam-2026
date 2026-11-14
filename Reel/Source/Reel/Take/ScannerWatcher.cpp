// Owner: W. Constructor by the header pass; bodies by W.

#include "ScannerWatcher.h"

#include "BarClockComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Julian.h"
#include "TakeCharacter.h"
#include "TakeDirector.h"
#include "TakeShapes.h"
#include "TakeStyle.h"

namespace TakeScannerLocal
{
	/** Julian's feet count as in the frame box within this height of its floor, cm (ScannerWatcher.h). */
	constexpr float FrameFloorTolerance = 150.f;

	/** The open bar lies on the floor: its centre this high, cm (the bar is 6 cm thick). */
	constexpr float OpenBarZ = 3.f;

	/** The bar's thickness (cube scale on X and Z). */
	constexpr float BarThicknessScale = 0.06f;

	/** The tally sits this far beside the eye (local +Y), cm. */
	constexpr float TallyBesideEye = 30.f;

	/** The click volume round the eye (half extents), cm. */
	const FVector EyeClickExtent(60.f, 80.f, 60.f);

	/** A character's capsule counts as in the gate's slab from this far below the gate's floor, cm. */
	constexpr float SlabBelowFloor = 100.f;

	/** Is any character's capsule in the gate's slab right now (so the gate must not shut on it)? */
	bool AnyCharacterInGate(const AScannerWatcher* Scanner)
	{
		const ATakeDirector* Director = Scanner ? ATakeDirector::Get(Scanner) : nullptr;
		if (!Director)
		{
			return false;
		}

		const FTransform& ScannerXf = Scanner->GetActorTransform();
		const double HalfThickness = static_cast<double>(FMath::Max(Scanner->GateThickness, 1.f)) * 0.5;
		const double HalfWidth = static_cast<double>(FMath::Max(Scanner->GateHalfWidth, 1.f));
		const double Height = static_cast<double>(FMath::Max(Scanner->GateHeight, 1.f));

		for (const TObjectPtr<ATakeCharacter>& CharacterPtr : Director->Characters)
		{
			const ATakeCharacter* Who = CharacterPtr.Get();
			if (!Who)
			{
				continue;
			}
			const double Radius = static_cast<double>(Who->GetCapsuleRadius());
			const FVector Local = ScannerXf.InverseTransformPosition(Who->GetFeetLocation());
			if (FMath::Abs(Local.X) <= HalfThickness + Radius
				&& FMath::Abs(Local.Y) <= HalfWidth + Radius
				&& Local.Z > -static_cast<double>(SlabBelowFloor)
				&& Local.Z < Height)
			{
				return true;
			}
		}
		return false;
	}
}

AScannerWatcher::AScannerWatcher()
{
	Label = TEXT("SCANNER");
	LabelHeight = 300.f;
	EyeHeight = 251.f;

	// The frame: a flat rectangle on the floor in front of the gate (FrameMesh is the base's cube).
	FrameMesh->SetRelativeLocation(FVector(-90.f, 0.f, 0.f));
	FrameMesh->SetRelativeScale3D(FVector(1.8f, 3.2f, 0.01f));

	// The gate: blocks walkers (the Pawn channel) and nothing else; LayoutScanner sizes it, SetGateOpen switches it.
	GateBlock = CreateDefaultSubobject<UBoxComponent>(TEXT("GateBlock"));
	GateBlock->SetupAttachment(Root);
	GateBlock->SetBoxExtent(FVector(15.f, 160.f, 120.f));
	GateBlock->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	GateBlock->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GateBlock->SetCollisionObjectType(ECC_WorldStatic);
	GateBlock->SetCollisionResponseToAllChannels(ECR_Ignore);
	GateBlock->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	GateBlock->SetGenerateOverlapEvents(false);
	GateBlock->SetCanEverAffectNavigation(false);
	GateBlock->SetHiddenInGame(true);

	GateBar = FTakeShapes::CreateShape(this, TEXT("GateBar"), ETakeShape::Cube, Root, FVector(0.f, 0.f, 100.f),
		FVector(0.06f, 3.2f, 0.06f), FRotator::ZeroRotator, false);
	Eye = FTakeShapes::CreateShape(this, TEXT("Eye"), ETakeShape::Sphere, Root, FVector(-14.f, 0.f, 251.f),
		FVector(0.14f, 0.14f, 0.14f), FRotator::ZeroRotator, false);
}

void AScannerWatcher::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LayoutScanner();
}

void AScannerWatcher::BeginPlay()
{
	Super::BeginPlay();
	LayoutScanner();
	BuildFrameLines();

	// The gate for the state now: shut while live (it is there from the start), open otherwise.
	SetGateOpen(!IsLive());
	SyncState();
	RefreshFrameLook();
}

void AScannerWatcher::StepWatcher(float DeltaSeconds, UBarClockComponent* Clock)
{
	Super::StepWatcher(DeltaSeconds, Clock);

	// Not live: open this frame. Live: shut this frame, unless a character stands in the gate (then on the first
	// frame it is clear).
	if (!IsLive())
	{
		if (!bGateOpen)
		{
			SetGateOpen(true);
		}
	}
	else if (bGateOpen && !TakeScannerLocal::AnyCharacterInGate(this))
	{
		SetGateOpen(false);
	}
}

bool AScannerWatcher::HoldsJulian(const AJulian* Julian) const
{
	if (!Julian || !IsLive())
	{
		return false;
	}

	// The box in front of the gate: X from -FrameDepth to 0, Y within GateHalfWidth, near the floor.
	const FVector Local = GetActorTransform().InverseTransformPosition(Julian->GetFeetLocation());
	if (Local.X < -static_cast<double>(FrameDepth) || Local.X > 0.0)
	{
		return false;
	}
	if (FMath::Abs(Local.Y) > static_cast<double>(GateHalfWidth)
		|| FMath::Abs(Local.Z) > static_cast<double>(TakeScannerLocal::FrameFloorTolerance))
	{
		return false;
	}
	return HasLineOfSight(Julian);
}

bool AScannerWatcher::ApplyEffect(EDeviceEffect Effect, float Duration, const FVector& Point)
{
	bool bCaught = false;
	if (Effect == EDeviceEffect::Blind)
	{
		// The base: dark for the duration, logs, flashes BlindedLine.
		bCaught = Super::ApplyEffect(Effect, Duration, Point);
	}
	else if (Effect == EDeviceEffect::Mask && HasArrived())
	{
		AddDark(EWatcherState::Masked, Duration);
		TAKE_LOG(TEXT("%s MASKED %.1f s"), *Label, DarkSecondsLeft);
		bCaught = true;
	}

	// The gate opens on the frame the effect lands.
	if (bCaught && !IsLive() && !bGateOpen)
	{
		SetGateOpen(true);
	}
	return bCaught;
}

FVector AScannerWatcher::GetEyePoint() const
{
	return GetActorTransform().TransformPosition(EyeOffset);
}

FVector AScannerWatcher::GetAimPoint() const
{
	return GetActorTransform().TransformPosition(EyeOffset);
}

FVector AScannerWatcher::GetLabelPoint() const
{
	return GetActorLocation() + FVector(0.f, 0.f, LabelHeight);
}

void AScannerWatcher::GetFrameFarCorners(TArray<FVector>& OutLocal) const
{
	// The four floor corners of the frame box, in order round it (FramePivot's space; no rotation, 2 cm up).
	const float Depth = FMath::Max(FrameDepth, 1.f);
	const float HalfWidth = FMath::Max(GateHalfWidth, 1.f);
	OutLocal.Reset();
	OutLocal.Add(FVector(0.f, -HalfWidth, 0.f));
	OutLocal.Add(FVector(-Depth, -HalfWidth, 0.f));
	OutLocal.Add(FVector(-Depth, HalfWidth, 0.f));
	OutLocal.Add(FVector(0.f, HalfWidth, 0.f));
}

bool AScannerWatcher::HasPassed(const AJulian* Julian) const
{
	if (!Julian)
	{
		return false;
	}
	const FVector Local = GetActorTransform().InverseTransformPosition(Julian->GetFeetLocation());
	return Local.X > static_cast<double>(PassedDepth);
}

void AScannerWatcher::LayoutScanner()
{
	const float HalfWidth = FMath::Max(GateHalfWidth, 1.f);
	const float Depth = FMath::Max(FrameDepth, 1.f);
	const float Thickness = FMath::Max(GateThickness, 1.f);
	const float Height = FMath::Max(GateHeight, 1.f);

	// The frame: a flat red rectangle filling the box in front of the gate (FramePivot is 2 cm above the floor).
	FramePivot->SetRelativeLocation(FVector(0.f, 0.f, 2.f));
	FrameMesh->SetRelativeLocation(FVector(-Depth * 0.5f, 0.f, 0.f));
	FrameMesh->SetRelativeRotation(FRotator::ZeroRotator);
	FrameMesh->SetRelativeScale3D(FVector(Depth / 100.f, HalfWidth * 2.f / 100.f, 0.01f));

	// The gate's slab across X = 0, from the floor up.
	GateBlock->SetBoxExtent(FVector(Thickness * 0.5f, HalfWidth, Height * 0.5f));
	GateBlock->SetRelativeLocation(FVector(0.f, 0.f, Height * 0.5f));

	// The bar across the gate: its place and colour follow the gate.
	GateBar->SetRelativeScale3D(FVector(TakeScannerLocal::BarThicknessScale, HalfWidth * 2.f / 100.f, TakeScannerLocal::BarThicknessScale));
	GateBar->SetRelativeLocation(FVector(0.f, 0.f, bGateOpen ? TakeScannerLocal::OpenBarZ : BarHeight));

	// The eye on the lintel, the tally beside it, the click volume round it.
	Eye->SetRelativeLocation(EyeOffset);
	Tally->SetRelativeLocation(EyeOffset + FVector(0.f, TakeScannerLocal::TallyBesideEye, 0.f));
	ClickVolume->SetRelativeLocation(EyeOffset);
	ClickVolume->SetBoxExtent(TakeScannerLocal::EyeClickExtent);

	FTakeShapes::SetColour(FrameMesh, TakeColors::Red);
	FTakeShapes::SetSignal(GateBar, bGateOpen ? TakeStyle::StencilGold : TakeStyle::StencilRed);
	FTakeShapes::SetColour(GateBar, bGateOpen ? TakeColors::Gold : TakeColors::Red);
	FTakeShapes::SetSignal(Eye, TakeStyle::StencilRed);
	FTakeShapes::SetColour(Eye, TakeColors::Red);
	FTakeShapes::SetColour(Tally, TakeColors::DarkGrey);
}

void AScannerWatcher::SetGateOpen(bool bOpen)
{
	const bool bChanged = bOpen != bGateOpen;
	bGateOpen = bOpen;

	// Shut: blocks walkers (Pawn only, as set up in the constructor). Open: no collision at all.
	if (GateBlock)
	{
		GateBlock->SetCollisionEnabled(bOpen ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	}
	if (GateBar)
	{
		GateBar->SetRelativeLocation(FVector(0.f, 0.f, bOpen ? TakeScannerLocal::OpenBarZ : BarHeight));
		FTakeShapes::SetSignal(GateBar, bOpen ? TakeStyle::StencilGold : TakeStyle::StencilRed);
		FTakeShapes::SetColour(GateBar, bOpen ? TakeColors::Gold : TakeColors::Red);
	}

	if (bChanged)
	{
		TAKE_LOG(TEXT("%s gate %s"), *Label, bOpen ? TEXT("OPEN") : TEXT("SHUT"));
	}
}
