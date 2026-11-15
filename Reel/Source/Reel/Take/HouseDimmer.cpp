// Maya's house dimmer (QA pass, Nov 14). Owner: heist. See HouseDimmer.h for the trick and the contract.

#include "HouseDimmer.h"

#include "CastMember.h"
#include "Components/LightComponent.h"
#include "Components/LocalLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Light.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "TakeCharacter.h"
#include "TakeDirector.h"
#include "TakeShapes.h"
#include "Watcher.h"

namespace TakeHouseDimmerLocal
{
	/** The projector's body: dark enamel. */
	const FLinearColor ProjectorBodyColour(0.05f, 0.05f, 0.05f);

	/** The board while the house comes back up: half way to white. */
	constexpr float RisingWhite = 0.4f;

	/** The film's flicker: two fast sines out of step, and a dip as a splice runs through the gate. Never the beat. */
	float FlickerScale(float RunSeconds, float Amount)
	{
		const float Wobble = 0.5f * (FMath::Sin(RunSeconds * 37.f) + FMath::Sin(RunSeconds * 23.f + 1.3f));	// -1..1
		float Scale = 1.f - Amount * (0.5f + 0.5f * Wobble);
		if (FMath::Frac(RunSeconds * 1.37f) < 0.05f)
		{
			Scale *= 0.55f;
		}
		return FMath::Max(0.f, Scale);
	}

	FBox HouseBox(const FVector& A, const FVector& B)
	{
		return FBox(A.ComponentMin(B), A.ComponentMax(B));
	}
}

AHouseDimmer::AHouseDimmer()
{
	DeviceName = TEXT("HOUSE DIMMER");
	Effect = EDeviceEffect::Blind;
	Range = 3500.f;
	Duration = 0.75f;	// one glance at the screen; renewed while the house is dark
	Uses = 0;
	FiredFrom = ETakeCamera::B;
	UseVerb = TEXT("Bring the house lights down for the preview");
	UseRadius = 100.f;	// small: the pyro board's ring is 1.4 m away
	LockedUntil = FName(TakeTriggers::Scene2);	// W3 heist2: the dark is the only way to the dais, keycard included
	LockedWhy = TEXT("for the preview, once Julian is in the hall");
	FireLine = TEXT("MAYA: \"House lights down. Rolling the preview.\"");
	BodyColour = FLinearColor(0.30f, 0.22f, 0.12f);	// walnut, until the board's model

	// A dimmer board: a tall bank of levers, 30 cm deep, 1.2 m across, 1.8 m tall. A plain box for now.
	if (Body)
	{
		Body->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
		Body->SetRelativeScale3D(FVector(0.3f, 1.2f, 1.8f));
	}

	// The master lever on the board's face (towards the use point), waist high: thrown while the house is down.
	bShowHandle = true;
	HandleOffset = FVector(18.f, 0.f, 110.f);

	ProjectorBody = FTakeShapes::CreateShape(this, TEXT("ProjectorBody"), ETakeShape::Cube, Root, ProjectorOffset,
		FVector(0.6f, 0.35f, 0.4f), FRotator::ZeroRotator, false);

	ProjectorLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("ProjectorLight"));
	ProjectorLight->SetupAttachment(Root);
	ProjectorLight->SetMobility(EComponentMobility::Movable);
	ProjectorLight->SetCastShadows(false);
	ProjectorLight->SetVisibility(false);

	ScreenGlow = FTakeShapes::CreateCueRect(this, TEXT("ScreenGlow"), Root, ScreenOffset, FRotator::ZeroRotator, 3500.f);
}

void AHouseDimmer::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);	// the use ring
	PlaceProjector();
}

void AHouseDimmer::BeginPlay()
{
	Super::BeginPlay();	// UsesLeft, the body's colour, the use ring

	bRunning = false;
	bLanded = false;
	RunSeconds = 0.f;
	DarkSecondsLeft = 0.f;
	GlanceSecondsLeft = 0.f;
	HouseLevel = 1.f;
	AppliedLevel = 1.f;
	HouseLights.Reset();
	HouseBaseIntensity.Reset();

	PlaceProjector();
	if (ProjectorBody)
	{
		FTakeShapes::SetColour(ProjectorBody, TakeHouseDimmerLocal::ProjectorBodyColour);
	}

	// The picture: a thin pale panel on the screen's face, hidden until the preview. No shadows, NoCollision.
	if (UStaticMeshComponent* Old = ScreenPanel.Get())
	{
		Old->DestroyComponent();
	}
	ScreenPanel = nullptr;
	// W3 heist2: a plane on the screen's face (its +Z towards the hall, its X up), the old film on it when it exists.
	const FVector Facing = GetScreenFacing();
	const FVector PanelScale(FMath::Max(ScreenSize.Y, 1.f) / 100.f,
		FMath::Max(ScreenSize.X, 1.f) / 100.f * (bScreenMirror ? -1.f : 1.f), 1.f);
	ScreenPanel = FTakeShapes::AddShapeAtRuntime(this, ETakeShape::Plane, Root, ScreenOffset, PanelScale, ScreenColour, false);
	FilmMID = nullptr;
	bHasFilm = false;
	if (ScreenPanel)
	{
		ScreenPanel->SetMobility(EComponentMobility::Movable);
		const FQuat Face = FRotationMatrix::MakeFromZX(Facing, FVector::UpVector).ToQuat();
		const FQuat Roll(Facing, FMath::DegreesToRadians(ScreenPictureRoll));
		ScreenPanel->SetWorldLocationAndRotation(GetScreenCentre() + Facing * 2.f, Roll * Face);
		if (UMaterialInterface* const Film = FilmMaterial.LoadSynchronous())
		{
			FilmMID = ScreenPanel->CreateDynamicMaterialInstance(0, Film);
			bHasFilm = FilmMID != nullptr;
		}
		TAKE_LOG(TEXT("%s: the screen shows %s"), *DeviceName,
			bHasFilm ? *FilmMaterial.ToSoftObjectPath().ToString() : TEXT("the stand-in panel (no M_OldFilm)"));
	}
	SetPreviewVisible(false);
}

bool AHouseDimmer::CanFire(FString& OutWhyNot) const
{
	const ATakeDirector* const Director = ATakeDirector::Get(this);
	if (Director && !Director->IsHeistOn())
	{
		OutWhyNot = DeviceName + TEXT(" is not in this cut");
		return false;
	}
	if (Director && Director->HasFired(FName(TakeTriggers::Case)))
	{
		OutWhyNot = DeviceName + TEXT(" - the preview is over");
		return false;
	}
	if (bRunning)
	{
		OutWhyNot = IsHouseDark()
			? FString::Printf(TEXT("%s - the preview is running (%d s)"), *DeviceName, FMath::CeilToInt(DarkSecondsLeft))
			: DeviceName + TEXT(" - the house is coming up");
		return false;
	}
	return Super::CanFire(OutWhyNot);	// locked until the keycard
}

bool AHouseDimmer::Fire()
{
	// CanFire, bRunning, RunSeconds = 0, FiredFrame, "FIRE HOUSE DIMMER".
	if (!Super::Fire())
	{
		return false;
	}

	// Lands on this frame: StepDevices (step 3, same frame) gives every hall watcher its first glance.
	MarkLanded();
	DarkSecondsLeft = FMath::Max(PreviewSeconds, 1.f);
	GlanceSecondsLeft = FMath::Max(GlanceRefreshSeconds, 0.05f);

	// This frame: the lever's clunk (the house drops), the beam and the picture, the board goes white.
	GatherHouseLights();
	HouseLevel = FMath::Clamp(FirstDropLevel, DimLevel, 1.f);
	ApplyHouseLevel(HouseLevel);
	SetPreviewVisible(true);
	SetFilm(1.f);
	ApplyProjectorFlicker();
	if (Body)
	{
		BodyMID = FTakeShapes::SetColour(Body, TakeColors::White);
	}
	if (bTurnHeads)
	{
		TurnHeads();
	}
	TAKE_LOG(TEXT("%s down: %d house lights to %.0f%%, preview %.1f s"), *DeviceName, HouseLights.Num(),
		DimLevel * 100.f, DarkSecondsLeft);
	return true;
}

void AHouseDimmer::StepDevice(float DeltaSeconds)
{
	StepLook(DeltaSeconds);	// the pilot lamp and the lever (ADevice, package devices): every frame, first

	if (!bRunning)
	{
		return;
	}

	// The fire frame's time is not taken twice (Fire ran in step 2 of this frame).
	const float Dt = FiredThisFrame() ? 0.f : DeltaSeconds;
	RunSeconds += Dt;

	if (DarkSecondsLeft > 0.f)
	{
		// Dark: the house eases down, the film flickers, every hall watcher keeps looking at the screen.
		DarkSecondsLeft -= Dt;
		const float DownSpeed = (1.f - DimLevel) / FMath::Max(DimSeconds, 0.05f);
		HouseLevel = FMath::FInterpConstantTo(HouseLevel, DimLevel, Dt, DownSpeed);

		GlanceSecondsLeft -= Dt;
		if (GlanceSecondsLeft <= 0.f)
		{
			GlanceSecondsLeft = FMath::Max(GlanceRefreshSeconds, 0.05f);
			Glance();
		}

		if (DarkSecondsLeft <= 0.f)
		{
			EndDark();
		}
		else
		{
			ApplyProjectorFlicker();
		}
	}
	else
	{
		// Rising: the house eases back to where it was; the watchers look back within one glance.
		const float UpSpeed = 1.f / FMath::Max(RaiseSeconds, 0.05f);
		HouseLevel = FMath::FInterpConstantTo(HouseLevel, 1.f, Dt, UpSpeed);
		if (HouseLevel >= 1.f - KINDA_SMALL_NUMBER)
		{
			HouseLevel = 1.f;
			bRunning = false;
			RefreshBodyLook();
			TAKE_LOG(TEXT("%s: the house is up"), *DeviceName);
		}
	}
	ApplyHouseLevel(HouseLevel);
}

void AHouseDimmer::GatherTargets(const TArray<TObjectPtr<AWatcher>>& All, TArray<AWatcher*>& Out) const
{
	Super::GatherTargets(All, Out);	// every watcher within Range of the hall's middle
	const FBox House = TakeHouseDimmerLocal::HouseBox(HouseMin, HouseMax);
	Out.RemoveAll([&House](const AWatcher* W)
	{
		// The beam is a laser: it does not watch films. A watcher outside the hall (backstage) cannot see the screen.
		return !W || W->IsInstantBlooper() || !House.IsInsideOrOn(W->GetActorLocation());
	});
}

FVector AHouseDimmer::GetEffectPoint() const
{
	return GetActorTransform().TransformPosition(EffectOffset);
}

FString AHouseDimmer::GetPipsText() const
{
	if (IsHouseDark())
	{
		return FString::Printf(TEXT("%s  DARK %d"), *DeviceName, FMath::CeilToInt(DarkSecondsLeft));
	}
	return DeviceName + (bRunning ? FString(TEXT("  RISING")) : FString(TEXT("  UP")));
}

void AHouseDimmer::EndPreviewIn(float Seconds)
{
	if (!IsHouseDark())
	{
		return;
	}
	DarkSecondsLeft = FMath::Min(DarkSecondsLeft, FMath::Max(Seconds, 0.f));
	if (DarkSecondsLeft <= 0.f)
	{
		EndDark();
		return;
	}
	TAKE_LOG(TEXT("%s: the house comes up in %.1f s"), *DeviceName, DarkSecondsLeft);
}

void AHouseDimmer::HoldPreviewFor(float Seconds)
{
	if (IsHouseDark() && Seconds > DarkSecondsLeft)
	{
		DarkSecondsLeft = Seconds;
	}
}

void AHouseDimmer::RestoreHouseNow()
{
	const bool bWasRunning = bRunning;
	bRunning = false;
	DarkSecondsLeft = 0.f;
	SetPreviewVisible(false);
	HouseLevel = 1.f;
	ApplyHouseLevel(1.f);
	RefreshBodyLook();
	if (bWasRunning)
	{
		TAKE_LOG(TEXT("%s: the house back up at once (blooper)"), *DeviceName);
	}
}

FVector AHouseDimmer::GetScreenCentre() const
{
	return GetActorTransform().TransformPosition(ScreenOffset);
}

FVector AHouseDimmer::GetScreenFacing() const
{
	const FVector Facing = ScreenNormal.GetSafeNormal();
	return Facing.IsNearlyZero() ? -GetActorForwardVector() : Facing;
}

void AHouseDimmer::SetFilm(float Play)
{
	if (bHasFilm && FilmMID)
	{
		FilmMID->SetScalarParameterValue(TEXT("Play"), Play);
		FilmMID->SetScalarParameterValue(TEXT("Brightness"), FilmBrightness);
		if (Play > 0.f)
		{
			// The leader's countdown starts on this frame (T0: world seconds, the material's Time); the frame's shape.
			FilmMID->SetScalarParameterValue(TEXT("T0"), GetWorld() ? GetWorld()->GetTimeSeconds() : -1.f);
			FilmMID->SetScalarParameterValue(TEXT("Aspect"), FMath::Max(ScreenSize.X, 1.f) / FMath::Max(ScreenSize.Y, 1.f));
		}
	}
	else if (ScreenPanel && Play > 0.f)
	{
		FTakeShapes::SetColour(ScreenPanel, ScreenColour * FMath::Min(FilmBrightness * Play, 1.f));
	}
}

void AHouseDimmer::PlaceProjector()
{
	const FTransform& Where = GetActorTransform();
	const FVector Lens = Where.TransformPosition(ProjectorOffset);
	const FVector Screen = Where.TransformPosition(ScreenOffset);
	const FVector ToScreen = Screen - Lens;
	const FRotator Aim = ToScreen.IsNearlyZero() ? GetActorRotation() : ToScreen.Rotation();

	if (ProjectorBody)
	{
		ProjectorBody->SetWorldLocationAndRotation(Lens, Aim);
	}
	if (ProjectorLight)
	{
		ProjectorLight->SetWorldLocationAndRotation(Lens, Aim);
		ProjectorLight->SetIntensityUnits(ELightUnits::Candelas);
		ProjectorLight->SetIntensity(ProjectorCandelas);
		ProjectorLight->SetLightColor(ProjectorColour);
		ProjectorLight->SetOuterConeAngle(ProjectorConeDegrees);
		ProjectorLight->SetInnerConeAngle(ProjectorConeDegrees * 0.7f);
		ProjectorLight->SetAttenuationRadius(FMath::Max(1000.f, static_cast<float>(ToScreen.Size()) * 1.25f));
		ProjectorLight->SetVolumetricScatteringIntensity(ProjectorScattering);
		ProjectorLight->SetCastShadows(false);
	}
	if (ScreenGlow)
	{
		// On the screen's face, facing the hall (a rect light shines along its +X), the screen's size.
		const FVector Facing = GetScreenFacing();
		ScreenGlow->SetWorldLocationAndRotation(Screen + Facing * 20.f, Facing.Rotation());
		ScreenGlow->SetSourceWidth(FMath::Max(ScreenSize.X, 10.f));
		ScreenGlow->SetSourceHeight(FMath::Max(ScreenSize.Y, 10.f));
	}
}

void AHouseDimmer::GatherHouseLights()
{
	HouseLights.Reset();
	HouseBaseIntensity.Reset();
	AppliedLevel = 1.f;

	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}
	const FBox House = TakeHouseDimmerLocal::HouseBox(HouseMin, HouseMax);
	for (TActorIterator<ALight> It(World); It; ++It)
	{
		ALight* const LightActor = *It;
		// A light riding someone (Julian's key on his camera boom) is his, not the house's.
		if (!IsValid(LightActor) || LightActor->GetAttachParentActor())
		{
			continue;
		}
		ULightComponent* const Comp = LightActor->GetLightComponent();
		if (!Comp || !Comp->IsA<ULocalLightComponent>() || Comp->GetMobility() == EComponentMobility::Static
			|| !Comp->IsVisible() || !House.IsInsideOrOn(Comp->GetComponentLocation()))
		{
			continue;
		}
		HouseLights.Add(Comp);
		HouseBaseIntensity.Add(Comp->Intensity);
	}
}

void AHouseDimmer::ApplyHouseLevel(float Level)
{
	if (FMath::IsNearlyEqual(Level, AppliedLevel, 0.0005f))
	{
		return;
	}
	AppliedLevel = Level;
	for (int32 Index = 0; Index < HouseLights.Num(); ++Index)
	{
		if (ULightComponent* const Comp = HouseLights[Index].Get())
		{
			Comp->SetIntensity(HouseBaseIntensity[Index] * Level);
		}
	}
}

void AHouseDimmer::SetPreviewVisible(bool bVisible)
{
	if (ProjectorLight)
	{
		ProjectorLight->SetVisibility(bVisible);
	}
	if (ScreenPanel)
	{
		ScreenPanel->SetVisibility(bVisible);
	}
	if (ScreenGlow && !bVisible)
	{
		FTakeShapes::SetCueLight(ScreenGlow, 0.f, ScreenGlowColour);
	}
}

void AHouseDimmer::ApplyProjectorFlicker()
{
	if (ProjectorLight)
	{
		ProjectorLight->SetIntensity(ProjectorCandelas * TakeHouseDimmerLocal::FlickerScale(RunSeconds, FlickerAmount));
	}
	if (ScreenGlow)
	{
		// The film's light on the front rows: the same flicker, a little slower to settle.
		FTakeShapes::SetCueLight(ScreenGlow,
			ScreenGlowCandelas * TakeHouseDimmerLocal::FlickerScale(RunSeconds * 0.9f, FlickerAmount * 1.5f), ScreenGlowColour);
	}
}

int32 AHouseDimmer::Glance()
{
	const ATakeDirector* const Director = ATakeDirector::Get(this);
	if (!Director)
	{
		return 0;
	}
	TArray<AWatcher*> Targets;
	GatherTargets(Director->Watchers, Targets);
	const FVector Point = GetEffectPoint();
	int32 Caught = 0;
	for (AWatcher* const W : Targets)
	{
		if (W && W->ApplyEffect(Effect, Duration, Point))
		{
			++Caught;
		}
	}
	return Caught;
}

void AHouseDimmer::TurnHeads()
{
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}
	const FBox House = TakeHouseDimmerLocal::HouseBox(HouseMin, HouseMax);
	const FVector Screen = GetScreenCentre();
	int32 Turned = 0;
	for (TActorIterator<ACastMember> It(World); It; ++It)
	{
		ACastMember* const Guest = *It;
		// The crew and Julian wear their costume pieces on their own characters: their heads are the player's.
		if (!IsValid(Guest) || Cast<ATakeCharacter>(Guest->GetAttachParentActor())
			|| !House.IsInsideOrOn(Guest->GetActorLocation()))
		{
			continue;
		}
		// W3 heist2: at the screen where stage put it, for the whole preview (cast2's ScreenPoint is a fixed guess).
		Guest->LookAtPoint(Screen, FMath::Max(PreviewSeconds, 1.f) + RaiseSeconds);	// the cast contract (cast2)
		++Turned;
	}
	TAKE_LOG(TEXT("%s: %d heads turn to the screen"), *DeviceName, Turned);
}

void AHouseDimmer::EndDark()
{
	DarkSecondsLeft = 0.f;
	SetPreviewVisible(false);
	SetFilm(0.f);
	if (Body)
	{
		BodyMID = FTakeShapes::SetColour(Body, FMath::Lerp(BodyColour, TakeColors::White, TakeHouseDimmerLocal::RisingWhite));
	}
	TAKE_LOG(TEXT("%s: the preview ends, the house comes up"), *DeviceName);
}
