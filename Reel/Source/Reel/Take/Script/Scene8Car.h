// Scene 8's getaway car: a silver late-60s GT coupe (generic, unbranded) with Max at the wheel. Owner: s8-alley.
// AScene8Alley spawns one at run time and drives it along its own paths; the car has no rules and no collision.
// Body: the generated mesh (BodyMesh, /Game/Take/Script/S8/SM_S8_GTCar) when it loads, else a grey-box coupe built
// from basic shapes. Nose +X, wheels on Z = 0, pivot at the ground centre; LHD: Max sits on -Y, the passenger on +Y.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Scene8Car.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class USpotLightComponent;
class UPointLightComponent;
class UAnimationAsset;

UCLASS()
class REEL_API AScene8Car : public AActor
{
	GENERATED_BODY()

public:
	AScene8Car();

	virtual void BeginPlay() override;

	/** The generated body. Unset or missing: the grey-box coupe shows instead. */
	UPROPERTY(EditAnywhere, Category = "Scene8")
	TSoftObjectPtr<UStaticMesh> BodyMesh;

	/** Max's seated body (s2-van's model). Unset: a grey-box driver. */
	UPROPERTY(EditAnywhere, Category = "Scene8")
	TSoftObjectPtr<USkeletalMesh> MaxMesh;

	/** Max's seated, looping pose or idle (single-node animation). */
	UPROPERTY(EditAnywhere, Category = "Scene8")
	TSoftObjectPtr<UAnimationAsset> MaxAnim;

	/** Max's seat, car space (the driver's hips). */
	UPROPERTY(EditAnywhere, Category = "Scene8")
	FVector DriverSeat = FVector(-30.f, -42.f, 40.f);

	/** Vance's seat, car space (his hips): the passenger side. */
	UPROPERTY(EditAnywhere, Category = "Scene8")
	FVector PassengerSeat = FVector(-30.f, 42.f, 40.f);

	/** The rear-view mirror, car space: the final shot looks back at Vance from here. */
	UPROPERTY(EditAnywhere, Category = "Scene8")
	FVector MirrorPoint = FVector(38.f, 0.f, 112.f);

	/** Where the passenger door is, car space: Vance reaches it to dive in. */
	UPROPERTY(EditAnywhere, Category = "Scene8")
	FVector PassengerDoor = FVector(-10.f, 105.f, 0.f);

	/** Headlights' intensity (candelas) at rest, and on the flare when the car rounds the corner. */
	UPROPERTY(EditAnywhere, Category = "Scene8")
	float HeadlightCd = 9000.f;

	UPROPERTY(EditAnywhere, Category = "Scene8")
	float FlareCd = 60000.f;

	/** 0..1: the headlights from rest to full flare (this frame). */
	void SetFlare(float Alpha);

	/** The passenger door swung open (0 shut .. 1 wide). Only on the grey-box body (the generated mesh is one piece). */
	void SetDoorOpen(float Alpha);

	/** Show or hide the grey-box passenger (Vance's stand-in in the long shots after he dives in) and Cole in the back. */
	void SetRiders(bool bVance, bool bCole);

	FVector GetPassengerDoorWorld() const { return GetActorTransform().TransformPosition(PassengerDoor); }
	FVector GetPassengerSeatWorld() const { return GetActorTransform().TransformPosition(PassengerSeat); }
	FVector GetMirrorWorld() const { return GetActorTransform().TransformPosition(MirrorPoint); }

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<USceneComponent> Root;

	/** Everything that leans and bounces with the drive (the body, the people, the lights). */
	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<USceneComponent> Chassis;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<USceneComponent> GreyBox;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<USceneComponent> DoorHinge;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<USpotLightComponent> HeadL;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<USpotLightComponent> HeadR;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<UPointLightComponent> Dash;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<USkeletalMeshComponent> MaxBody;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<USceneComponent> MaxProxy;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<USceneComponent> VanceProxy;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<USceneComponent> ColeProxy;

private:
	/** A grey-box person (body, head) sitting at Seat under Parent. */
	USceneComponent* MakeSitter(FName Name, const FVector& Seat, const FLinearColor& Coat);
	/** Each grey-box shape and its colour, painted on BeginPlay (dynamic instances cannot be made in a constructor). */
	TArray<TPair<UStaticMeshComponent*, FLinearColor>> Paint;
	UStaticMeshComponent* Part(FName Name, int32 Shape, USceneComponent* Parent, const FVector& Loc, const FVector& Scale, const FLinearColor& C, const FRotator& Rot = FRotator::ZeroRotator);
};
