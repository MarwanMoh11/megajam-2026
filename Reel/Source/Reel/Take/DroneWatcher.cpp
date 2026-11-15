// Owner: W. Constructor by the header pass; bodies by W.

#include "DroneWatcher.h"

#include "BarClockComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Julian.h"
#include "PatrolPoint.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "TakeCharacter.h"
#include "TakeDirector.h"
#include "TakeShapes.h"
#include "TakeStyle.h"

namespace TakeDroneLocal
{
	/** The disc's thickness (cylinder Z scale): 1 cm. */
	constexpr float DiscThicknessScale = 0.01f;

	/** The tally hangs this far under the body, cm. */
	constexpr float TallyBelowBody = 25.f;

	/** The label sits at least this far over the body, cm. */
	constexpr float LabelAboveBody = 80.f;

	/** The click volume round the body (half extents), cm. */
	const FVector BodyClickExtent(90.f, 90.f, 60.f);

	// --- the skin (Nov 15): a quadcopter built from shapes, animated in UpdateWorldCues, with a quiet looping hum ---
	// Built at BeginPlay only (the editor viewport keeps the plain box). Its state lives in this file's map, not in
	// the header, so the whole skin went in through Live Coding without restarting the shared editor. Promote these
	// constants to UPROPERTYs at the next header change.

	/** Rotor spin at hover, deg/s; up to (1 + SpinSpeedBoost) times that at ChaseSpeed. */
	constexpr float SpinDegPerSecond = 1150.f;
	constexpr float SpinSpeedBoost = 0.45f;

	/** Bank into the flight: degrees per cm/s of ground speed, capped. */
	constexpr float TiltPerSpeed = 0.022f;
	constexpr float MaxTiltDegrees = 17.f;

	/** How fast the felt velocity and the heading follow the real ones (per second). */
	constexpr float VelocityEaseRate = 4.f;
	constexpr float YawEaseRate = 2.5f;

	/** The hover: a slow bob (cm, Hz) and a small sway (deg); blinded, the sway is DazedSway times bigger. */
	constexpr float BobCm = 7.f;
	constexpr float BobHz = 0.45f;
	constexpr float SwayDegrees = 1.6f;
	constexpr float DazedSway = 4.f;

	/** Nav lights: a blink period (s) and how much of it they are lit. */
	constexpr float BlinkPeriod = 1.1f;
	constexpr float BlinkOn = 0.45f;

	/** The hum: quiet, spatial. Volume multiplier, pitch at hover and at ChaseSpeed, attenuation (cm). */
	const TCHAR* const HumPath = TEXT("/Game/Take/Audio/sfx_drone_hum.sfx_drone_hum");
	constexpr float HumVolume = 0.3f;
	constexpr float HumPitchHover = 0.94f;
	constexpr float HumPitchFast = 1.14f;
	constexpr float HumInnerRadius = 700.f;
	constexpr float HumFalloff = 2600.f;

	const FLinearColor HullTint(0.02f, 0.02f, 0.022f);
	const FLinearColor CanopyTint(0.055f, 0.055f, 0.06f);
	const FLinearColor ArmTint(0.045f, 0.045f, 0.048f);
	const FLinearColor MotorTint(0.09f, 0.088f, 0.085f);
	const FLinearColor BladeTint(0.2f, 0.195f, 0.19f);
	const FLinearColor PortLamp(3.f, 0.25f, 0.15f);
	const FLinearColor StarboardLamp(0.3f, 2.6f, 0.6f);
	const FLinearColor LampOff(0.02f, 0.02f, 0.02f);

	struct FDroneSkin
	{
		TWeakObjectPtr<USceneComponent> Rig;
		TArray<TWeakObjectPtr<USceneComponent>, TInlineAllocator<4>> Hubs;
		TWeakObjectPtr<UStaticMeshComponent> PortLight;
		TWeakObjectPtr<UStaticMeshComponent> StarboardLight;
		TWeakObjectPtr<UAudioComponent> Hum;
		FVector LastLocation = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		float Time = 0.f;
		float Yaw = 0.f;
		float Spin = 0.f;
		float HumPitch = HumPitchHover;
		bool bHasLast = false;
		bool bLightsLit = false;
		bool bLightsSet = false;
	};

	TMap<TWeakObjectPtr<const ADroneWatcher>, FDroneSkin>& Skins()
	{
		static TMap<TWeakObjectPtr<const ADroneWatcher>, FDroneSkin> Map;
		return Map;
	}

	/** BeginPlay: the rig at FlyHeight (hull, canopy, four arms, motors, spinning rotors, gimbal, skids, nav lights),
	 *  the old box hidden, the lens and tally moved onto the rig, the hum (silent until UpdateSkin sees it shown). */
	void BuildSkin(ADroneWatcher* Drone, USceneComponent* Root, UStaticMeshComponent* Body, UStaticMeshComponent* Lens,
		UStaticMeshComponent* Tally)
	{
		TMap<TWeakObjectPtr<const ADroneWatcher>, FDroneSkin>& Map = Skins();
		for (auto It = Map.CreateIterator(); It; ++It)
		{
			if (!It.Key().IsValid())
			{
				It.RemoveCurrent();
			}
		}
		if (!Drone || !Root)
		{
			return;
		}
		if (const FDroneSkin* Old = Map.Find(Drone); Old && Old->Rig.IsValid())
		{
			return;	// built already
		}
		FDroneSkin& Skin = Map.Add(Drone, FDroneSkin());

		USceneComponent* Rig = NewObject<USceneComponent>(Drone, TEXT("DroneSkinRig"));
		Rig->SetMobility(EComponentMobility::Movable);
		Rig->SetupAttachment(Root);
		Rig->SetRelativeLocation(FVector(0.f, 0.f, FMath::Max(Drone->FlyHeight, 1.f)));
		Rig->RegisterComponent();
		Drone->AddInstanceComponent(Rig);
		Skin.Rig = Rig;

		// Sizes in cm (the basic shapes are 100 across).
		auto Part = [Drone](ETakeShape Shape, USceneComponent* Parent, const FVector& At, const FVector& SizeCm,
			const FRotator& Rotation, const FLinearColor& Colour)
		{
			UStaticMeshComponent* Comp = FTakeShapes::AddShapeAtRuntime(Drone, Shape, Parent, At, SizeCm / 100.f, Colour, false);
			if (Comp)
			{
				Comp->SetRelativeRotation(Rotation);
			}
			return Comp;
		};

		Part(ETakeShape::Sphere, Rig, FVector(0.f, 0.f, 0.f), FVector(64.f, 50.f, 20.f), FRotator::ZeroRotator, HullTint);
		Part(ETakeShape::Sphere, Rig, FVector(6.f, 0.f, 7.f), FVector(34.f, 28.f, 14.f), FRotator::ZeroRotator, CanopyTint);

		// Four arms on the diagonals, a motor and a two-blade rotor at each tip.
		for (int32 i = 0; i < 4; ++i)
		{
			const float Angle = 45.f + 90.f * static_cast<float>(i);
			const FVector Dir(FMath::Cos(FMath::DegreesToRadians(Angle)), FMath::Sin(FMath::DegreesToRadians(Angle)), 0.f);
			Part(ETakeShape::Cube, Rig, Dir * 36.f + FVector(0.f, 0.f, 2.f), FVector(72.f, 7.f, 5.f), FRotator(0.f, Angle, 0.f), ArmTint);
			Part(ETakeShape::Cylinder, Rig, Dir * 68.f + FVector(0.f, 0.f, 4.f), FVector(15.f, 15.f, 12.f), FRotator::ZeroRotator, MotorTint);

			USceneComponent* Hub = NewObject<USceneComponent>(Drone);
			Hub->SetMobility(EComponentMobility::Movable);
			Hub->SetupAttachment(Rig);
			Hub->SetRelativeLocation(Dir * 68.f + FVector(0.f, 0.f, 11.5f));
			Hub->RegisterComponent();
			Drone->AddInstanceComponent(Hub);
			Part(ETakeShape::Cube, Hub, FVector::ZeroVector, FVector(62.f, 6.f, 1.2f), FRotator::ZeroRotator, BladeTint);
			Part(ETakeShape::Sphere, Hub, FVector(0.f, 0.f, 1.f), FVector(6.f, 6.f, 4.f), FRotator::ZeroRotator, MotorTint);
			Skin.Hubs.Add(Hub);
		}

		// The camera gimbal under the nose, two skids.
		Part(ETakeShape::Cube, Rig, FVector(16.f, 0.f, -15.f), FVector(16.f, 16.f, 11.f), FRotator::ZeroRotator, ArmTint);
		for (const float Side : {-1.f, 1.f})
		{
			Part(ETakeShape::Cube, Rig, FVector(0.f, Side * 21.f, -27.f), FVector(58.f, 3.f, 3.f), FRotator::ZeroRotator, ArmTint);
			for (const float End : {-1.f, 1.f})
			{
				Part(ETakeShape::Cube, Rig, FVector(End * 15.f, Side * 21.f, -17.f), FVector(3.f, 3.f, 20.f), FRotator::ZeroRotator, ArmTint);
			}
		}

		// Nav lights past the front motors: red to port (-Y), green to starboard.
		const FVector Port = FVector(1.f, -1.f, 0.f).GetSafeNormal() * 80.f + FVector(0.f, 0.f, 4.f);
		const FVector Starboard = FVector(1.f, 1.f, 0.f).GetSafeNormal() * 80.f + FVector(0.f, 0.f, 4.f);
		Skin.PortLight = Part(ETakeShape::Sphere, Rig, Port, FVector(6.f), FRotator::ZeroRotator, LampOff);
		Skin.StarboardLight = Part(ETakeShape::Sphere, Rig, Starboard, FVector(6.f), FRotator::ZeroRotator, LampOff);

		// The old box goes; the lens sits in the gimbal and the tally under the tail, both riding the rig.
		if (Body)
		{
			Body->SetVisibility(false);
		}
		if (Lens)
		{
			Lens->AttachToComponent(Rig, FAttachmentTransformRules::KeepWorldTransform);
			Lens->SetRelativeLocation(FVector(25.f, 0.f, -16.f));
		}
		if (Tally)
		{
			Tally->AttachToComponent(Rig, FAttachmentTransformRules::KeepWorldTransform);
			Tally->SetRelativeLocation(FVector(-20.f, 0.f, -12.f));
		}

		// The hum: quiet and spatial, on the rig.
		if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, HumPath))
		{
			UAudioComponent* Hum = NewObject<UAudioComponent>(Drone, TEXT("DroneHum"));
			Hum->bAutoActivate = false;
			Hum->SetupAttachment(Rig);
			Hum->SetSound(Sound);
			Hum->SetVolumeMultiplier(HumVolume);
			Hum->SetPitchMultiplier(HumPitchHover);
			USoundAttenuation* Attenuation = NewObject<USoundAttenuation>(Hum);
			Attenuation->Attenuation.bAttenuate = true;
			Attenuation->Attenuation.bSpatialize = true;
			Attenuation->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
			Attenuation->Attenuation.AttenuationShapeExtents = FVector(HumInnerRadius, 0.f, 0.f);
			Attenuation->Attenuation.FalloffDistance = HumFalloff;
			Attenuation->Attenuation.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
			Hum->AttenuationSettings = Attenuation;
			Hum->RegisterComponent();
			Drone->AddInstanceComponent(Hum);
			Skin.Hum = Hum;
		}
		else
		{
			TAKE_LOG(TEXT("DRONE skin: no hum at %s"), HumPath);
		}
	}

	/** Every frame (UpdateWorldCues): rotors spin (faster with speed), the body banks into its flight, turns to face
	 *  it (or Julian while it holds him), bobs and sways (dazed while blinded), nav lights blink while it sees, the hum
	 *  plays while it is shown and rises in pitch with speed. Changes show this frame. */
	void UpdateSkin(ADroneWatcher* Drone, float DeltaSeconds, float HoldAlpha, const AJulian* Julian)
	{
		FDroneSkin* Skin = Drone ? Skins().Find(Drone) : nullptr;
		USceneComponent* Rig = Skin ? Skin->Rig.Get() : nullptr;
		if (!Rig)
		{
			return;
		}

		const float Dt = FMath::Max(DeltaSeconds, 0.f);
		Skin->Time += Dt;
		const FVector Here = Drone->GetActorLocation();
		if (Skin->bHasLast && Dt > KINDA_SMALL_NUMBER)
		{
			FVector Raw = (Here - Skin->LastLocation) / Dt;
			Raw.Z = 0.f;
			if (Raw.SizeSquared() > FMath::Square(3000.f))
			{
				Raw = FVector::ZeroVector;	// a checkpoint restore: a jump, not a flight
			}
			Skin->Velocity = FMath::Lerp(Skin->Velocity, Raw, 1.f - FMath::Exp(-VelocityEaseRate * Dt));
		}
		Skin->LastLocation = Here;
		Skin->bHasLast = true;

		const FVector Velocity = Skin->Velocity;
		const float Speed = static_cast<float>(Velocity.Size2D());
		const float SpeedAlpha = FMath::Clamp(Speed / FMath::Max(Drone->ChaseSpeed, 1.f), 0.f, 1.f);

		// Heading: the way it flies; while it holds Julian, at him.
		float TargetYaw = Skin->Yaw;
		if (HoldAlpha > 0.5f && Julian)
		{
			const FVector ToJulian = Julian->GetActorLocation() - Here;
			if (ToJulian.SizeSquared2D() > 1.f)
			{
				TargetYaw = FMath::RadiansToDegrees(FMath::Atan2(ToJulian.Y, ToJulian.X));
			}
		}
		else if (Speed > 40.f)
		{
			TargetYaw = FMath::RadiansToDegrees(FMath::Atan2(Velocity.Y, Velocity.X));
		}
		Skin->Yaw = FRotator::NormalizeAxis(Skin->Yaw + FMath::FindDeltaAngleDegrees(Skin->Yaw, TargetYaw) * (1.f - FMath::Exp(-YawEaseRate * Dt)));

		// Bank into the flight (nose down the way it goes), then the hover sway on top; blinded, it reels.
		float Pitch = static_cast<float>(-Velocity.X) * TiltPerSpeed;
		float Roll = static_cast<float>(Velocity.Y) * TiltPerSpeed;
		const float Tilt = FMath::Sqrt(Pitch * Pitch + Roll * Roll);
		if (Tilt > MaxTiltDegrees)
		{
			Pitch *= MaxTiltDegrees / Tilt;
			Roll *= MaxTiltDegrees / Tilt;
		}
		const bool bDazed = Drone->HasArrived() && !Drone->IsLive();
		const float Sway = SwayDegrees * (bDazed ? DazedSway : 1.f);
		const float SwayTime = Skin->Time * (bDazed ? 2.5f : 1.f);
		Pitch += Sway * FMath::Sin(SwayTime * UE_TWO_PI * 0.31f);
		Roll += Sway * FMath::Cos(SwayTime * UE_TWO_PI * 0.43f);
		Rig->SetWorldRotation(FRotator(Pitch, 0.f, Roll).Quaternion() * FRotator(0.f, Skin->Yaw, 0.f).Quaternion());
		Rig->SetRelativeLocation(FVector(0.f, 0.f, FMath::Max(Drone->FlyHeight, 1.f) + BobCm * FMath::Sin(Skin->Time * UE_TWO_PI * BobHz)));

		// Rotors: neighbours turn opposite ways.
		Skin->Spin = FMath::Fmod(Skin->Spin + SpinDegPerSecond * (1.f + SpinSpeedBoost * SpeedAlpha) * Dt, 360.f);
		for (int32 i = 0; i < Skin->Hubs.Num(); ++i)
		{
			if (USceneComponent* Hub = Skin->Hubs[i].Get())
			{
				Hub->SetRelativeRotation(FRotator(0.f, (i % 2 == 0 ? Skin->Spin : -Skin->Spin) + 45.f * static_cast<float>(i), 0.f));
			}
		}

		// Nav lights blink while it sees; dark while blinded.
		const bool bLit = Drone->IsLive() && FMath::Fmod(Skin->Time, BlinkPeriod) < BlinkPeriod * BlinkOn;
		if (bLit != Skin->bLightsLit || !Skin->bLightsSet)
		{
			Skin->bLightsLit = bLit;
			Skin->bLightsSet = true;
			if (UStaticMeshComponent* Light = Skin->PortLight.Get())
			{
				FTakeShapes::SetLamp(Light, bLit ? PortLamp : LampOff, bLit ? TakeStyle::StencilRed : static_cast<uint8>(0));
			}
			if (UStaticMeshComponent* Light = Skin->StarboardLight.Get())
			{
				FTakeShapes::SetLamp(Light, bLit ? StarboardLamp : LampOff, 0);
			}
		}

		// The hum: on while it is shown, its pitch following the speed.
		if (UAudioComponent* Hum = Skin->Hum.Get())
		{
			const bool bShown = !Drone->IsHidden();
			if (bShown && !Hum->IsPlaying())
			{
				Hum->Play(FMath::FRand() * 3.f);
			}
			else if (!bShown && Hum->IsPlaying())
			{
				Hum->Stop();
			}
			Skin->HumPitch = FTakeShapes::Ease(Skin->HumPitch, FMath::Lerp(HumPitchHover, HumPitchFast, SpeedAlpha), 3.f, Dt);
			Hum->SetPitchMultiplier(Skin->HumPitch);
		}
	}

	/** The path's live points (null entries skipped) and each leg's bars. Returns the cycle's length in bars. */
	int32 GatherPath(const TArray<TObjectPtr<APatrolPoint>>& PathPoints, TArray<FVector, TInlineAllocator<16>>& OutPoints,
		TArray<int32, TInlineAllocator<16>>& OutLegBars)
	{
		int32 CycleBars = 0;
		for (const TObjectPtr<APatrolPoint>& PointPtr : PathPoints)
		{
			if (const APatrolPoint* PathPoint = PointPtr.Get())
			{
				const int32 LegBars = FMath::Max(1, PathPoint->Bars);
				OutPoints.Add(PathPoint->GetActorLocation());
				OutLegBars.Add(LegBars);
				CycleBars += LegBars;
			}
		}
		return CycleBars;
	}
}

ADroneWatcher::ADroneWatcher()
{
	Label = TEXT("DRONE");
	Instrument = EWatcherInstrument::Trumpet;
	LabelHeight = 600.f;
	PlanLightOffset = FVector(0.f, 0.f, -110.f);	// QA sight: the planning lift under the body, on its belly and lens

	// The cone's footprint: a flat disc on the floor (FrameMesh is the base's cube: re-shaped here).
	FTakeShapes::SetupShape(FrameMesh, ETakeShape::Cylinder, false);
	FrameMesh->SetRelativeScale3D(FVector(6.f, 6.f, 0.01f));

	Body = FTakeShapes::CreateShape(this, TEXT("Body"), ETakeShape::Cube, Root, FVector(0.f, 0.f, 520.f),
		FVector(0.9f, 0.9f, 0.18f), FRotator::ZeroRotator, false);

	// Step 6: the searchlight under the body, pointing straight down (LayoutDrone sizes its cone).
	SearchLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("SearchLight"));
	SearchLight->SetupAttachment(Root);
	SearchLight->SetRelativeLocation(FVector(0.f, 0.f, 500.f));
	SearchLight->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	SearchLight->SetMobility(EComponentMobility::Movable);
	SearchLight->SetCastShadows(false);
	SearchLight->bUseInverseSquaredFalloff = true;

	// QA sight: the lens under the body, the glint that says where the eye is.
	Lens = FTakeShapes::CreateShape(this, TEXT("Lens"), ETakeShape::Sphere, Root, FVector(0.f, 0.f, 506.f),
		FVector(0.12f, 0.12f, 0.12f), FRotator::ZeroRotator, false);
}

void ADroneWatcher::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LayoutDrone();
}

void ADroneWatcher::BeginPlay()
{
	Super::BeginPlay();
	LayoutDrone();

	// It starts over its first path point (Off until it arrives).
	SetActorLocation(EvaluatePath(nullptr));

	BuildFrameLines();
	SyncState();
	RefreshFrameLook();
	HoldAlpha = 0.f;
	TakeDroneLocal::BuildSkin(this, Root, Body, Lens, Tally);
	UpdateWorldCues(-1.f);	// QA sight: the searchlight and lens for the state now
}

void ADroneWatcher::StepWatcher(float DeltaSeconds, UBarClockComponent* Clock)
{
	// The flight is a pure function of the clock (it hovers over the first point until its arrival bar). Moved
	// before the base step, so the cone is tested this frame where it is drawn.
	if (const AActor* Target = ChaseTarget.Get())
	{
		// Step 3, the lock-on: no path; the floor point closes on the target's feet (2D) at ChaseSpeed, takes the
		// feet's height, and then stays over them. Blinded, it still follows.
		const ATakeCharacter* Character = Cast<const ATakeCharacter>(Target);
		const FVector Feet = Character ? Character->GetFeetLocation() : Target->GetActorLocation();
		FVector Here = GetActorLocation();
		const FVector ToFeet(Feet.X - Here.X, Feet.Y - Here.Y, 0.0);
		const double Remaining = ToFeet.Size();
		const double Step = static_cast<double>(FMath::Max(ChaseSpeed, 0.f)) * static_cast<double>(FMath::Max(DeltaSeconds, 0.f));
		if (Remaining <= Step || Remaining < KINDA_SMALL_NUMBER)
		{
			Here.X = Feet.X;
			Here.Y = Feet.Y;
		}
		else
		{
			Here += ToFeet / Remaining * Step;
		}
		Here.Z = Feet.Z;
		SetActorLocation(Here);
	}
	else
	{
		SetActorLocation(EvaluatePath(Clock));
	}
	Super::StepWatcher(DeltaSeconds, Clock);

	// Step 6: the searchlight is on while it sees (arrived, not blinded): the dark drone's pool goes out with it, this
	// frame (UpdateWorldCues, from SetTally after the sight test, then aims and colours it).
	if (SearchLight && SearchLight->IsVisible() != IsLive())
	{
		SearchLight->SetVisibility(IsLive());
	}
}

void ADroneWatcher::UpdateWorldCues(float DeltaSeconds)
{
	Super::UpdateWorldCues(DeltaSeconds);

	const bool bOn = IsLive();
	const bool bHolding = bOn && bTally;
	// It snaps onto him on the frame it has him; it lets go more slowly.
	HoldAlpha = bHolding ? 1.f : FTakeShapes::Ease(HoldAlpha, 0.f, HoldReleaseRate, DeltaSeconds);
	if (HoldAlpha < 0.005f)
	{
		HoldAlpha = 0.f;
	}

	if (SearchLight)
	{
		// At rest straight down, its pool the sight cone's footprint (ConeRadius); holding, onto his feet, tight.
		const float Radius = FMath::Max(ConeRadius, 1.f);
		const float Drop = FMath::Max(FlyHeight - 20.f, 1.f);
		FQuat Aim = FRotator(-90.f, 0.f, 0.f).Quaternion();
		float Outer = FMath::RadiansToDegrees(FMath::Atan2(Radius, Drop));
		if (HoldAlpha > 0.f)
		{
			const ATakeDirector* Director = ATakeDirector::Get(this);
			const AJulian* JulianActor = Director ? Director->Julian.Get() : nullptr;
			if (JulianActor)
			{
				const FVector ToFeet = JulianActor->GetFeetLocation() - SearchLight->GetComponentLocation();
				if (!ToFeet.IsNearlyZero())
				{
					Aim = FQuat::Slerp(Aim, ToFeet.Rotation().Quaternion(), HoldAlpha);
					const float HoldOuter = FMath::RadiansToDegrees(FMath::Atan2(FMath::Max(HoldPoolRadius, 20.f), static_cast<float>(ToFeet.Size())));
					Outer = FMath::Lerp(Outer, HoldOuter, HoldAlpha);
				}
			}
		}
		SearchLight->SetWorldRotation(Aim);
		FTakeShapes::SetCueCone(SearchLight, Outer, FMath::Lerp(SearchLightInnerRatio, 0.8f, HoldAlpha));
		const float Candelas = bOn ? FMath::Lerp(FMath::Max(SearchLightCandelas, 0.f), FMath::Max(HoldLightCandelas, 0.f), HoldAlpha) : 0.f;
		FTakeShapes::SetCueLight(SearchLight, Candelas, FMath::Lerp(SearchLightColour, HoldLightColour, HoldAlpha), SearchLightHaze);
	}

	// The lens: a glint while it looks, broadcast red while it has him, dark while blinded or off.
	if (Lens)
	{
		Lens->SetVisibility(HasArrived());
		const FLinearColor Glow = !bOn ? FLinearColor(0.02f, 0.02f, 0.02f) : FMath::Lerp(LensColour, LensHoldColour, HoldAlpha);
		FTakeShapes::SetLamp(Lens, Glow, HoldAlpha > 0.5f ? TakeStyle::StencilRed : static_cast<uint8>(0));
	}

	// The skin's animation and hum (Nov 15).
	const ATakeDirector* SkinDirector = ATakeDirector::Get(this);
	TakeDroneLocal::UpdateSkin(this, DeltaSeconds, HoldAlpha, SkinDirector ? SkinDirector->Julian.Get() : nullptr);
}

bool ADroneWatcher::HoldsJulian(const AJulian* Julian) const
{
	if (!Julian || !IsLive())
	{
		return false;
	}

	const FVector Feet = Julian->GetFeetLocation();
	const FVector Here = GetActorLocation();
	if (FVector::Dist2D(Feet, Here) > static_cast<double>(FMath::Max(ConeRadius, 1.f)))
	{
		return false;
	}
	if (FMath::Abs(Feet.Z - Here.Z) > static_cast<double>(ConeFloorTolerance))
	{
		return false;	// on the floor above or below the cone's
	}
	return HasLineOfSight(Julian);
}

bool ADroneWatcher::ApplyEffect(EDeviceEffect Effect, float Duration, const FVector& Point)
{
	// Blind: dark for the duration, it keeps flying (the base). Lure and Mask: nothing.
	if (Effect != EDeviceEffect::Blind)
	{
		return false;
	}
	return Super::ApplyEffect(Effect, Duration, Point);
}

bool ADroneWatcher::ChangesAtBar(int32 Bar, const UBarClockComponent* Clock) const
{
	if (!HasArrived() || IsChasing())
	{
		return false;
	}

	TArray<FVector, TInlineAllocator<16>> Points;
	TArray<int32, TInlineAllocator<16>> LegBars;
	const int32 CycleBars = TakeDroneLocal::GatherPath(PathPoints, Points, LegBars);
	if (Points.Num() < 2 || CycleBars <= 0)
	{
		return false;
	}

	// The path's time starts on its arrival bar (bar 1 when it is there from the start); that first leg is the
	// arrival's own cue, so only later leg starts count.
	const int32 StartBar = ArriveOn.IsNone() ? 1 : ArriveBar;
	const int32 BarsSince = Bar - StartBar;
	if (BarsSince <= 0)
	{
		return false;
	}

	const int32 Phase = BarsSince % CycleBars;
	int32 LegStart = 0;
	for (const int32 Bars : LegBars)
	{
		if (Phase == LegStart)
		{
			return true;
		}
		LegStart += Bars;
	}
	return false;
}

FVector ADroneWatcher::GetEyePoint() const
{
	return GetActorLocation() + FVector(0.f, 0.f, FlyHeight);
}

FVector ADroneWatcher::GetAimPoint() const
{
	return GetActorLocation() + FVector(0.f, 0.f, FlyHeight);
}

FVector ADroneWatcher::GetLabelPoint() const
{
	return GetActorLocation() + FVector(0.f, 0.f, FMath::Max(LabelHeight, FlyHeight + TakeDroneLocal::LabelAboveBody));
}

void ADroneWatcher::GetFrameFarCorners(TArray<FVector>& OutLocal) const
{
	// Four points on the disc's edge, in order round it, on the floor (FramePivot's space).
	const float Radius = FMath::Max(ConeRadius, 1.f);
	OutLocal.Reset();
	OutLocal.Add(FVector(Radius, 0.f, 0.f));
	OutLocal.Add(FVector(0.f, Radius, 0.f));
	OutLocal.Add(FVector(-Radius, 0.f, 0.f));
	OutLocal.Add(FVector(0.f, -Radius, 0.f));
}

FVector ADroneWatcher::EvaluatePath(const UBarClockComponent* Clock) const
{
	TArray<FVector, TInlineAllocator<16>> Points;
	TArray<int32, TInlineAllocator<16>> LegBars;
	const int32 CycleBars = TakeDroneLocal::GatherPath(PathPoints, Points, LegBars);
	if (Points.Num() == 0)
	{
		return HomeLocation;
	}
	if (!Clock || Points.Num() < 2 || CycleBars <= 0)
	{
		return Points[0];
	}

	// Arrived by the clock alone (so the path stays a pure function of it; the base's flag follows this frame).
	const bool bArrivedNow = ArriveOn.IsNone() || (ArriveBar > 0 && Clock->GetBar() >= ArriveBar);
	if (!bArrivedNow)
	{
		return Points[0];
	}

	const int32 StartBar = ArriveOn.IsNone() ? 1 : ArriveBar;
	const double BarsNow = Clock->GetTotalBeats() / static_cast<double>(FMath::Max(1, Clock->BeatsPerBar));
	const double BarsSince = FMath::Max(0.0, BarsNow - static_cast<double>(StartBar - 1));
	const double CyclePhase = FMath::Fmod(BarsSince, static_cast<double>(CycleBars));

	double LegStart = 0.0;
	for (int32 i = 0; i < Points.Num(); ++i)
	{
		const double Bars = static_cast<double>(LegBars[i]);
		if (CyclePhase < LegStart + Bars || i == Points.Num() - 1)
		{
			const double Alpha = FMath::Clamp((CyclePhase - LegStart) / Bars, 0.0, 1.0);
			const FVector& From = Points[i];
			const FVector& To = Points[(i + 1) % Points.Num()];
			return From + (To - From) * Alpha;
		}
		LegStart += Bars;
	}
	return Points[0];
}

void ADroneWatcher::LayoutDrone()
{
	const float Radius = FMath::Max(ConeRadius, 1.f);
	const float Height = FMath::Max(FlyHeight, 1.f);

	// The disc: the cylinder (100 across, pivot at its centre) flattened on FramePivot (2 cm above the floor).
	FramePivot->SetRelativeLocation(FVector(0.f, 0.f, 2.f));
	FrameMesh->SetRelativeLocation(FVector::ZeroVector);
	FrameMesh->SetRelativeRotation(FRotator::ZeroRotator);
	FrameMesh->SetRelativeScale3D(FVector(Radius * 2.f / 100.f, Radius * 2.f / 100.f, TakeDroneLocal::DiscThicknessScale));

	// The body at FlyHeight, the tally under it, the click volume round it.
	Body->SetRelativeLocation(FVector(0.f, 0.f, Height));
	Tally->SetRelativeLocation(FVector(0.f, 0.f, Height - TakeDroneLocal::TallyBelowBody));
	ClickVolume->SetRelativeLocation(FVector(0.f, 0.f, Height));
	ClickVolume->SetBoxExtent(TakeDroneLocal::BodyClickExtent);
	if (Lens)
	{
		Lens->SetRelativeLocation(FVector(0.f, 0.f, Height - 14.f));	// QA sight: just under the body
	}

	FTakeShapes::SetColour(FrameMesh, TakeColors::Red);
	FTakeShapes::SetColour(Body, BodyTint);
	FTakeShapes::SetColour(Tally, TakeColors::RedDark);

	// Step 6: the searchlight just under the body, its outer cone the frame's (ConeRadius on the floor), soft edge.
	if (SearchLight)
	{
		const float Half = FMath::RadiansToDegrees(FMath::Atan2(Radius, Height - 20.f));
		SearchLight->SetRelativeLocation(FVector(0.f, 0.f, Height - 20.f));
		SearchLight->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
		SearchLight->SetIntensityUnits(ELightUnits::Candelas);
		SearchLight->SetIntensity(FMath::Max(SearchLightCandelas, 0.f));
		SearchLight->SetOuterConeAngle(FMath::Clamp(Half, 1.f, 80.f));
		SearchLight->SetInnerConeAngle(FMath::Clamp(Half * FMath::Clamp(SearchLightInnerRatio, 0.f, 0.95f), 0.f, 79.f));	// QA sight
		SearchLight->SetLightColor(SearchLightColour);
		SearchLight->SetAttenuationRadius(FMath::Sqrt(Radius * Radius + Height * Height) * 1.4f);
		SearchLight->SetVolumetricScatteringIntensity(FMath::Max(SearchLightHaze, 0.f));
		SearchLight->SetCastShadows(false);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Step 3: the lock-on (W)
// ---------------------------------------------------------------------------------------------------------------------

void ADroneWatcher::SetChaseTarget(const AActor* Target)
{
	const bool bNewTarget = Target != nullptr && ChaseTarget.Get() != Target;
	const bool bWasChasing = ChaseTarget.IsValid();
	ChaseTarget = Target;

	if (bNewTarget)
	{
		TAKE_LOG(TEXT("%s locks on"), *Label);
		if (!ChaseLine.IsEmpty())
		{
			if (ATakeDirector* Director = ATakeDirector::Get(this))
			{
				Director->Flash(ChaseLine, ETakeFlashKind::Line, 3.f);
			}
		}
	}
	else if (Target == nullptr && bWasChasing)
	{
		TAKE_LOG(TEXT("%s back to its loop"), *Label);
	}
}
