// Owner: header pass (shared, complete).

#include "TakeShapes.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

const FName FTakeShapes::ColorParam(TEXT("Color"));

UStaticMesh* FTakeShapes::GetMesh(ETakeShape Shape)
{
	const TCHAR* Path = TEXT("/Engine/BasicShapes/Cube.Cube");
	switch (Shape)
	{
	case ETakeShape::Cube: Path = TEXT("/Engine/BasicShapes/Cube.Cube"); break;
	case ETakeShape::Sphere: Path = TEXT("/Engine/BasicShapes/Sphere.Sphere"); break;
	case ETakeShape::Cylinder: Path = TEXT("/Engine/BasicShapes/Cylinder.Cylinder"); break;
	case ETakeShape::Cone: Path = TEXT("/Engine/BasicShapes/Cone.Cone"); break;
	case ETakeShape::Plane: Path = TEXT("/Engine/BasicShapes/Plane.Plane"); break;
	}
	return LoadObject<UStaticMesh>(nullptr, Path);
}

UMaterialInterface* FTakeShapes::GetMaterial()
{
	return LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
}

void FTakeShapes::SetupShape(UStaticMeshComponent* Comp, ETakeShape Shape, bool bCastShadow)
{
	if (!Comp)
	{
		return;
	}
	Comp->SetStaticMesh(GetMesh(Shape));
	Comp->SetMaterial(0, GetMaterial());
	Comp->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Comp->SetGenerateOverlapEvents(false);
	Comp->SetCanEverAffectNavigation(false);
	Comp->SetCastShadow(bCastShadow);
	Comp->bReceivesDecals = false;
}

UStaticMeshComponent* FTakeShapes::CreateShape(AActor* Owner, FName Name, ETakeShape Shape, USceneComponent* Parent,
	const FVector& RelLocation, const FVector& RelScale, const FRotator& RelRotation, bool bCastShadow)
{
	UStaticMeshComponent* Comp = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
	SetupShape(Comp, Shape, bCastShadow);
	if (Parent)
	{
		Comp->SetupAttachment(Parent);
	}
	Comp->SetRelativeLocation(RelLocation);
	Comp->SetRelativeRotation(RelRotation);
	Comp->SetRelativeScale3D(RelScale);
	return Comp;
}

UStaticMeshComponent* FTakeShapes::AddShapeAtRuntime(AActor* Owner, ETakeShape Shape, USceneComponent* Parent,
	const FVector& RelLocation, const FVector& RelScale, const FLinearColor& Colour, bool bCastShadow)
{
	if (!Owner)
	{
		return nullptr;
	}
	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(Owner);
	SetupShape(Comp, Shape, bCastShadow);
	Comp->SetupAttachment(Parent ? Parent : Owner->GetRootComponent());
	Comp->SetRelativeLocation(RelLocation);
	Comp->SetRelativeScale3D(RelScale);
	Comp->RegisterComponent();
	Owner->AddInstanceComponent(Comp);
	SetColour(Comp, Colour);
	return Comp;
}

UMaterialInstanceDynamic* FTakeShapes::SetColour(UStaticMeshComponent* Comp, const FLinearColor& Colour)
{
	if (!Comp)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Comp->GetMaterial(0));
	if (!MID)
	{
		MID = Comp->CreateDynamicMaterialInstance(0, GetMaterial());
	}
	if (MID)
	{
		MID->SetVectorParameterValue(ColorParam, Colour);
	}
	return MID;
}

UBoxComponent* FTakeShapes::CreateClickVolume(AActor* Owner, FName Name, USceneComponent* Parent, const FVector& Extent,
	const FVector& RelLocation)
{
	UBoxComponent* Box = Owner->CreateDefaultSubobject<UBoxComponent>(Name);
	if (Parent)
	{
		Box->SetupAttachment(Parent);
	}
	Box->SetRelativeLocation(RelLocation);
	SetupClickVolume(Box, Extent);
	return Box;
}

void FTakeShapes::SetupClickVolume(UBoxComponent* Box, const FVector& Extent)
{
	if (!Box)
	{
		return;
	}
	Box->InitBoxExtent(Extent);
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionObjectType(ECC_WorldDynamic);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Box->SetGenerateOverlapEvents(false);
	Box->SetCanEverAffectNavigation(false);
	Box->SetHiddenInGame(true);
	Box->ShapeColor = FColor(255, 190, 40);
}

void FTakeShapes::SetupEditorOnlyBox(UBoxComponent* Box, const FVector& Extent, const FColor& EditorColour)
{
	if (!Box)
	{
		return;
	}
	Box->InitBoxExtent(Extent);
	Box->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Box->SetGenerateOverlapEvents(false);
	Box->SetCanEverAffectNavigation(false);
	Box->SetHiddenInGame(true);
	Box->ShapeColor = EditorColour;
}
