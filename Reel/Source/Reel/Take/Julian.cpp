// Owner: (c). Constructor by the header pass; every "// BODY: (c)" is a stub to fill.

#include "Julian.h"

#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Mark.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "TakeShapes.h"
#include "TakeTypes.h"

AJulian::AJulian()
{
	PrimaryActorTick.bCanEverTick = false;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	AutoPossessAI = EAutoPossessAI::Disabled;
	AIControllerClass = nullptr;

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->InitCapsuleSize(34.f, 90.f);
	Capsule->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Capsule->SetGenerateOverlapEvents(false);
	Capsule->SetCanEverAffectNavigation(false);
	RootComponent = Capsule;

	// Body 140 cm tall from the feet (-90) up; head on top; a nose block facing +X.
	Body = FTakeShapes::CreateShape(this, TEXT("Body"), ETakeShape::Cylinder, Capsule, FVector(0.f, 0.f, -20.f), FVector(0.55f, 0.55f, 1.4f));
	Head = FTakeShapes::CreateShape(this, TEXT("Head"), ETakeShape::Sphere, Capsule, FVector(0.f, 0.f, 70.f), FVector(0.4f, 0.4f, 0.4f));
	Nose = FTakeShapes::CreateShape(this, TEXT("Nose"), ETakeShape::Cube, Capsule, FVector(22.f, 0.f, 70.f), FVector(0.15f, 0.12f, 0.1f));

	// World-space dots: absolute transform at identity, so instances added with bWorldSpace stay put while he walks.
	PathDots = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PathDots"));
	PathDots->SetupAttachment(Capsule);
	PathDots->SetUsingAbsoluteLocation(true);
	PathDots->SetUsingAbsoluteRotation(true);
	PathDots->SetUsingAbsoluteScale(true);
	FTakeShapes::SetupShape(PathDots, ETakeShape::Cube, false);
}

void AJulian::BeginPlay()
{
	Super::BeginPlay();
	// BODY: (c)
}

bool AJulian::CueTo(AMark* Mark)
{
	// BODY: (c)
	return false;
}

void AJulian::StepWalk(float DeltaSeconds)
{
	// BODY: (c)
}

bool AJulian::IsWalking() const
{
	// BODY: (c)
	return false;
}

float AJulian::GetRemainingPathLength() const
{
	// BODY: (c)
	return 0.f;
}

float AJulian::GetSecondsToArrival() const
{
	// BODY: (c)
	return 0.f;
}

float AJulian::GetBeatsToArrival(float SecondsPerBeat) const
{
	// BODY: (c)
	return 0.f;
}

const TArray<FVector>& AJulian::GetPathPoints() const
{
	// BODY: (c)
	return PathPoints;
}

AMark* AJulian::GetCuedMark() const
{
	// BODY: (c)
	return nullptr;
}

AMark* AJulian::GetStandingMark() const
{
	// BODY: (c)
	return nullptr;
}

FVector AJulian::GetFeetLocation() const
{
	// BODY: (c)
	return GetActorLocation();
}

float AJulian::GetCapsuleRadius() const
{
	// BODY: (c)
	return 34.f;
}

void AJulian::RebuildPathDots()
{
	// BODY: (c)
}
