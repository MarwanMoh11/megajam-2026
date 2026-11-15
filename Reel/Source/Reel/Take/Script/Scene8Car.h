// Scene 8's getaway car: a silver late-60s GT coupe (generic, unbranded) with Max at the wheel. Owner: s8-alley;
// the body and the final in-car shot (the Finale API): s8-car-mirror.
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
class UCameraComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;

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

	/** The rear-view mirror's glass, car space: it hangs from the top of the windscreen. */
	UPROPERTY(EditAnywhere, Category = "Scene8")
	FVector MirrorPoint = FVector(6.f, 0.f, 121.f);

	/** Where the passenger door is, car space: Vance reaches it to dive in. */
	UPROPERTY(EditAnywhere, Category = "Scene8")
	FVector PassengerDoor = FVector(-10.f, 105.f, 0.f);

	/** Headlights' intensity (candelas) at rest, and on the flare when the car rounds the corner. */
	UPROPERTY(EditAnywhere, Category = "Scene8")
	float HeadlightCd = 2500.f;

	UPROPERTY(EditAnywhere, Category = "Scene8")
	float FlareCd = 60000.f;

	/** 0..1: the headlights from rest to full flare (this frame). */
	void SetFlare(float Alpha);

	/** The passenger door swung open (0 shut .. 1 wide). Only on the grey-box body (the generated mesh is one piece). */
	void SetDoorOpen(float Alpha);

	/** Show or hide the grey-box passenger (Vance's stand-in in the long shots after he dives in) and Cole in the back. */
	void SetRiders(bool bVance, bool bCole);

	// ---- the final shot (Scene 8's Mirror beat): inside the moving car in the rain ----------------------------------
	// BeginFinale builds the cabin (seats, dash, wheel, glass with rain, the mirror with a live capture of Vance's
	// face), sits Vance (his own model on the seated clip, the canister in his lap) and Cole, moves the car to
	// FinaleSpot on an endless road, and turns its own camera on: the scene cuts to the car. TickFinale runs two shots:
	// A, from the dash, Vance with the canister, streetlights sliding over him; B, over his shoulder onto the mirror:
	// his face in it, the tie straightened, "Cut. Print it." (the scene says the line at PrintAt). EndFinale undoes it.

	void BeginFinale();
	/** T: seconds since BeginFinale. */
	void TickFinale(float T, float DeltaSeconds);
	void EndFinale();
	bool IsInFinale() const { return bFinale; }

	/** The whole final shot, s: the scene finishes (smash to black) at this time. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	float FinaleSeconds = 8.2f;

	/** Shot A (the dash, Vance with the canister) runs this long, then shot B (the mirror). */
	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	float ShotASeconds = 3.4f;

	/** Vance straightens his tie (finale time, s). */
	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	float TieAt = 3.6f;

	/** "Cut. Print it." (finale time, s). */
	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	float PrintAt = 5.6f;

	/** Where the car drives in the final shot (world; an empty, dark stretch: only the road and what the car brings
	 *  along show). Zero: it stays where the drive ended. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	FVector FinaleSpot = FVector(0.f, -60000.f, -21.f);

	/** The speed the lamps, lane marks and lights slide past at, cm/s. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	float RoadSpeed = 1700.f;

	/** Street lamps along the kerb (passenger side), cm apart. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	float LampSpacing = 1900.f;

	/** Each street lamp's spot, candelas, and its colour (sodium). */
	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	float LampCd = 9000.f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	FLinearColor LampColour = FLinearColor(1.f, 0.55f, 0.22f);

	/** The steady key on Vance's face (the instruments' glow), candelas. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	float FaceKeyCd = 14.f;

	/** Shot A: the camera and the point it looks at (car space), field of view. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	FVector ShotACam = FVector(22.f, 6.f, 104.f);

	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	FVector ShotALook = FVector(-40.f, 40.f, 82.f);

	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	float ShotAFov = 44.f;

	/** Shot B: the camera (over Vance's inner shoulder, car space) looking at the mirror, field of view. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	FVector ShotBCam = FVector(-86.f, 14.f, 114.f);

	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	float ShotBFov = 34.f;

	/** The lens: f-stop of the shallow focus (both shots). */
	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	float Fstop = 1.8f;

	/** The mirror's capture: field of view on Vance's face. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	float MirrorCaptureFov = 26.f;

	/** Vance's and Cole's seated bodies (their own models, on the seated clips). */
	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	TSoftObjectPtr<USkeletalMesh> VanceMesh;

	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	TSoftObjectPtr<USkeletalMesh> ColeMesh;

	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	TSoftObjectPtr<UAnimationAsset> SitAnim;

	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	TSoftObjectPtr<UAnimationAsset> ColeSitAnim;

	/** The tie: a standing clip played on the seated body in shot B (only his top half is ever in frame). */
	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	TSoftObjectPtr<UAnimationAsset> TieAnim;

	/** The seated clip's hips over the model's root at scale 1, cm; the standing clip's (for the tie). */
	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	float SitHipHeight = 58.f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	float StandHipHeight = 96.f;

	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	TSoftObjectPtr<UMaterialInterface> PaintMaterial;

	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	TSoftObjectPtr<UMaterialInterface> GlowMaterial;

	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	TSoftObjectPtr<UMaterialInterface> MirrorMaterial;

	UPROPERTY(EditAnywhere, Category = "Scene8|Finale")
	TSoftObjectPtr<UMaterialInterface> GlassMaterial;

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

	// ---- the final shot's parts (made at BeginFinale, hidden at EndFinale) ----
	UPROPERTY(VisibleAnywhere, Category = "Scene8|Finale")
	TObjectPtr<UCameraComponent> FinaleCam;

	UPROPERTY(VisibleAnywhere, Category = "Scene8|Finale")
	TObjectPtr<USceneCaptureComponent2D> MirrorCapture;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> MirrorTarget;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> Cabin;		// everything inside the car: under Chassis (bounces with it)

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> Street;		// the road and what slides past: under Root (still)

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> MirrorHead;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> MirrorGlass;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> VanceBody;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> ColeBody;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> Movers;		// slide past at RoadSpeed, wrapping (lamps, marks, glows)

	UPROPERTY(Transient)
	TArray<TObjectPtr<USpotLightComponent>> LampSpots;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> GlassMids;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> FaceKey;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> Oncoming;

private:
	bool bFinale = false;
	bool bShotB = false;
	bool bTie = false;
	float Bob = 0.f;
	FVector PreFinaleLocation = FVector::ZeroVector;
	FRotator PreFinaleRotation = FRotator::ZeroRotator;
	TArray<float> MoverSpan;		// each mover's wrap length along X, cm (negative: an oncoming one, twice the speed)
	TArray<float> MoverX0;			// each mover's X at T = 0
	void BuildFinale();
	void SetShot(bool bB);
	UStaticMeshComponent* Bit(int32 Shape, USceneComponent* Parent, const FVector& Loc, const FVector& SizeCm,
		const FLinearColor& C, float Rough, float Metal, const FRotator& Rot = FRotator::ZeroRotator, bool bShadow = false);
	UStaticMeshComponent* Glow(USceneComponent* Parent, const FVector& Loc, float DiameterCm, const FLinearColor& C,
		float Power);
	UStaticMeshComponent* Pane(USceneComponent* Parent, const FVector& Loc, const FVector2D& SizeCm, const FRotator& Rot,
		const FVector2D& Pan, float Tiling);
	USkeletalMeshComponent* Sitter(const TSoftObjectPtr<USkeletalMesh>& Mesh, const TSoftObjectPtr<UAnimationAsset>& Anim,
		const FVector& Hips);

	/** A grey-box person (body, head) sitting at Seat under Parent. */
	USceneComponent* MakeSitter(FName Name, const FVector& Seat, const FLinearColor& Coat);
	/** Each grey-box shape and its colour, painted on BeginPlay (dynamic instances cannot be made in a constructor). */
	TArray<TPair<UStaticMeshComponent*, FLinearColor>> Paint;
	UStaticMeshComponent* Part(FName Name, int32 Shape, USceneComponent* Parent, const FVector& Loc, const FVector& Scale, const FLinearColor& C, const FRotator& Rot = FRotator::ZeroRotator);
};
