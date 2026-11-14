// Owner: header pass (shared, complete).

#include "TakeShapes.h"

#include "Components/BoxComponent.h"
#include "Components/LocalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
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

UMaterialInterface* FTakeShapes::GetSignalMaterial()
{
	static TWeakObjectPtr<UMaterialInterface> Cached;
	if (!Cached.IsValid())
	{
		// Pass 3's patterned signal material (tools/dress_auction.py); pass 1's solid one if it is missing.
		Cached = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Take/Dress/M_SignalPattern.M_SignalPattern"),
			nullptr, LOAD_NoWarn);
		if (!Cached.IsValid())
		{
			Cached = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Take/Look/M_TakeSignal.M_TakeSignal"));
		}
	}
	return Cached.Get();
}

void FTakeShapes::SetSignal(UStaticMeshComponent* Comp, uint8 Stencil)
{
	if (!Comp)
	{
		return;
	}
	Comp->SetCustomDepthStencilValue(Stencil);
	Comp->SetRenderCustomDepth(Stencil != 0);
	if (Stencil == 0)
	{
		return;
	}
	UMaterialInterface* const Signal = GetSignalMaterial();
	if (!Signal)
	{
		return;
	}
	UMaterialInstanceDynamic* const Old = Cast<UMaterialInstanceDynamic>(Comp->GetMaterial(0));
	if (Old && Old->Parent == Signal)
	{
		return;
	}
	FLinearColor Colour = FLinearColor::White;
	if (Old)
	{
		Old->GetVectorParameterValue(FHashedMaterialParameterInfo(ColorParam), Colour);
	}
	if (UMaterialInstanceDynamic* const MID = Comp->CreateDynamicMaterialInstance(0, Signal))
	{
		MID->SetVectorParameterValue(ColorParam, Colour);
	}
}

UMaterialInterface* FTakeShapes::GetFaintMaterial()
{
	static TWeakObjectPtr<UMaterialInterface> Cached;
	if (!Cached.IsValid())
	{
		Cached = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Take/Dress/M_SignalFaint.M_SignalFaint"), nullptr,
			LOAD_NoWarn);
	}
	return Cached.Get();
}

void FTakeShapes::SetFaint(UStaticMeshComponent* Comp, uint8 Stencil, float Fade)
{
	static const FName FadeParam(TEXT("Fade"));
	UMaterialInterface* const Faint = GetFaintMaterial();
	if (!Comp || !Faint)
	{
		return;
	}
	Comp->SetCustomDepthStencilValue(Stencil);
	Comp->SetRenderCustomDepth(Stencil != 0);
	UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Comp->GetMaterial(0));
	if (!MID || MID->Parent != Faint)
	{
		FLinearColor Colour = FLinearColor::White;
		if (MID)
		{
			MID->GetVectorParameterValue(FHashedMaterialParameterInfo(ColorParam), Colour);
		}
		MID = Comp->CreateDynamicMaterialInstance(0, Faint);
		if (MID)
		{
			MID->SetVectorParameterValue(ColorParam, Colour);
		}
	}
	if (MID)
	{
		MID->SetScalarParameterValue(FadeParam, Fade);
	}
}

void FTakeShapes::SetPattern(UStaticMeshComponent* Comp, float Pattern)
{
	static const FName PatternParam(TEXT("Pattern"));
	if (UMaterialInstanceDynamic* const MID = Comp ? Cast<UMaterialInstanceDynamic>(Comp->GetMaterial(0)) : nullptr)
	{
		MID->SetScalarParameterValue(PatternParam, Pattern);
	}
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

// ---------------------------------------------------------------------------------------------------------------------
// QA sight: world cues
// ---------------------------------------------------------------------------------------------------------------------

void FTakeShapes::SetupCueLight(ULocalLightComponent* Light, float AttenuationRadius)
{
	if (!Light)
	{
		return;
	}
	// Cheap by construction: movable, no shadows of any kind, no translucency lighting, culled with distance.
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetCastShadows(false);
	Light->CastStaticShadows = false;
	Light->CastDynamicShadows = false;
	Light->bAffectTranslucentLighting = false;
	Light->IntensityUnits = ELightUnits::Candelas;
	Light->Intensity = 0.f;
	Light->AttenuationRadius = FMath::Max(AttenuationRadius, 10.f);
	Light->VolumetricScatteringIntensity = 0.f;
	Light->MaxDrawDistance = CueLightMaxDrawDistance;
	Light->MaxDistanceFadeRange = CueLightMaxDrawDistance * 0.2f;
	if (UPointLightComponent* const Point = Cast<UPointLightComponent>(Light))
	{
		Point->bUseInverseSquaredFalloff = true;
	}
	Light->SetVisibility(false);
}

USpotLightComponent* FTakeShapes::CreateCueSpot(AActor* Owner, FName Name, USceneComponent* Parent,
	const FVector& RelLocation, const FRotator& RelRotation, float AttenuationRadius)
{
	USpotLightComponent* Light = Owner->CreateDefaultSubobject<USpotLightComponent>(Name);
	if (Parent)
	{
		Light->SetupAttachment(Parent);
	}
	Light->SetRelativeLocation(RelLocation);
	Light->SetRelativeRotation(RelRotation);
	SetupCueLight(Light, AttenuationRadius);
	return Light;
}

UPointLightComponent* FTakeShapes::CreateCuePoint(AActor* Owner, FName Name, USceneComponent* Parent,
	const FVector& RelLocation, float AttenuationRadius)
{
	UPointLightComponent* Light = Owner->CreateDefaultSubobject<UPointLightComponent>(Name);
	if (Parent)
	{
		Light->SetupAttachment(Parent);
	}
	Light->SetRelativeLocation(RelLocation);
	SetupCueLight(Light, AttenuationRadius);
	return Light;
}

URectLightComponent* FTakeShapes::CreateCueRect(AActor* Owner, FName Name, USceneComponent* Parent,
	const FVector& RelLocation, const FRotator& RelRotation, float AttenuationRadius)
{
	URectLightComponent* Light = Owner->CreateDefaultSubobject<URectLightComponent>(Name);
	if (Parent)
	{
		Light->SetupAttachment(Parent);
	}
	Light->SetRelativeLocation(RelLocation);
	Light->SetRelativeRotation(RelRotation);
	SetupCueLight(Light, AttenuationRadius);
	return Light;
}

void FTakeShapes::SetCueLight(ULocalLightComponent* Light, float Candelas, const FLinearColor& Colour, float Haze)
{
	if (!Light)
	{
		return;
	}
	const bool bOn = Candelas > 0.5f;
	if (Light->IsVisible() != bOn)
	{
		Light->SetVisibility(bOn);
	}
	if (!bOn)
	{
		return;
	}
	if (!FMath::IsNearlyEqual(Light->Intensity, Candelas, 0.25f))
	{
		Light->SetIntensity(Candelas);
	}
	if (Light->LightColor != Colour.ToFColor(true))
	{
		Light->SetLightColor(Colour);
	}
	const float UseHaze = FMath::Max(Haze, 0.f);
	if (!FMath::IsNearlyEqual(Light->VolumetricScatteringIntensity, UseHaze, 0.005f))
	{
		Light->SetVolumetricScatteringIntensity(UseHaze);
	}
}

void FTakeShapes::SetCueCone(USpotLightComponent* Light, float OuterDegrees, float InnerRatio)
{
	if (!Light)
	{
		return;
	}
	const float Outer = FMath::Clamp(OuterDegrees, 1.f, 80.f);
	const float Inner = FMath::Clamp(Outer * FMath::Clamp(InnerRatio, 0.f, 0.98f), 0.f, Outer - 0.5f);
	if (FMath::Abs(Light->OuterConeAngle - Outer) >= 0.25f)
	{
		Light->SetOuterConeAngle(Outer);
	}
	if (FMath::Abs(Light->InnerConeAngle - Inner) >= 0.25f)
	{
		Light->SetInnerConeAngle(Inner);
	}
}

void FTakeShapes::SetPlain(UStaticMeshComponent* Comp)
{
	if (!Comp)
	{
		return;
	}
	if (Comp->bRenderCustomDepth)
	{
		Comp->SetRenderCustomDepth(false);
	}
	UMaterialInterface* const Basic = GetMaterial();
	UMaterialInstanceDynamic* const Old = Cast<UMaterialInstanceDynamic>(Comp->GetMaterial(0));
	if (!Basic || (Old && Old->Parent == Basic))
	{
		return;
	}
	FLinearColor Colour = FLinearColor::White;
	if (Old)
	{
		Old->GetVectorParameterValue(FHashedMaterialParameterInfo(ColorParam), Colour);
	}
	if (UMaterialInstanceDynamic* const MID = Comp->CreateDynamicMaterialInstance(0, Basic))
	{
		MID->SetVectorParameterValue(ColorParam, Colour);
	}
}

void FTakeShapes::SetLamp(UStaticMeshComponent* Comp, const FLinearColor& Colour, uint8 Stencil)
{
	static const FName PatternParam(TEXT("Pattern"));
	if (!Comp)
	{
		return;
	}
	UMaterialInterface* const Signal = GetSignalMaterial();
	UMaterialInterface* const Wanted = Signal ? Signal : GetMaterial();
	UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Comp->GetMaterial(0));
	if (!MID || MID->Parent != Wanted)
	{
		MID = Comp->CreateDynamicMaterialInstance(0, Wanted);
	}
	if (Comp->CustomDepthStencilValue != Stencil)
	{
		Comp->SetCustomDepthStencilValue(Stencil);
	}
	const bool bDepth = Stencil != 0;
	if (static_cast<bool>(Comp->bRenderCustomDepth) != bDepth)
	{
		Comp->SetRenderCustomDepth(bDepth);
	}
	if (MID)
	{
		MID->SetScalarParameterValue(PatternParam, 0.f);	// TakeStyle::PatternSolid
		MID->SetVectorParameterValue(ColorParam, Colour);
	}
}

float FTakeShapes::Ease(float Current, float Target, float Rate, float DeltaSeconds)
{
	if (Rate <= 0.f || DeltaSeconds < 0.f)
	{
		return Target;
	}
	return FMath::Lerp(Target, Current, FMath::Exp(-Rate * DeltaSeconds));
}
