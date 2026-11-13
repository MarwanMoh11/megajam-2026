// ATakeCharacter: Julian, Maya and Vex (the Nov 13 rework: you walk them yourself). Owner: (m).
// Maya and Vex are placed as plain ATakeCharacter with Id B or C; Julian is the AJulian subclass (Id A).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TakeTypes.h"
#include "TakeCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UMaterialInstanceDynamic;

/**
 * A third-person character that NO controller ever possesses (AutoPossessPlayer / AutoPossessAI disabled, no AI
 * controller class). ATakePlayerController feeds the one the player is on directly: Move() and AddLook() from its
 * PlayerTick. GetCharacterMovement()->bRunPhysicsWithNoController = true, so AddMovementInput works without a
 * controller. bOrientRotationToMovement = true at TurnRate: the body turns to where it walks, the look does not.
 *
 * Its look is its own (LookYaw / LookPitch), not a control rotation: the boom uses ABSOLUTE rotation and the
 * character writes it from the look (AddLook/SetLook at once, and again every Tick). So every character always has a
 * valid follow view, whether or not the player is on it, and the flight can read the new character's view.
 *
 * Tick order (set up by ATakeDirector::EnsurePlayerController): the player controller (input) -> this actor and its
 * movement component (AddTickPrerequisiteActor(PC) on both) -> the director (AddTickPrerequisiteComponent on the
 * movement component). So a press, the move it causes, and the rules that see it share one frame.
 *
 * Collision: the capsule uses the Pawn profile. Every take prop (FTakeShapes) is NoCollision, so nothing but walls
 * and the floor blocks the walk or pushes the follow camera (the boom probes ECC_Camera).
 */
UCLASS()
class REEL_API ATakeCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	/** Header pass: capsule (Pawn profile), movement defaults, boom and follow camera, mannequin mesh and anim class
	 *  by ConstructorHelpers (left empty if not found), mesh at Z = -half height, yaw -90. */
	ATakeCharacter();

	/** Applies the tunables (ApplyTunables), makes the tint MID (ApplyTint), SetLook(actor yaw, StartLookPitch) so
	 *  the follow camera starts behind him. */
	virtual void BeginPlay() override;	// BODY: (m)

	/** Same tunables and tint in the editor, so the map shows the tinted mannequins and the boom length. */
	virtual void OnConstruction(const FTransform& Transform) override;	// BODY: (m)

	/** Every frame: Boom->SetWorldRotation(FRotator(LookPitch, LookYaw, 0)). Nothing else (no rule logic here). */
	virtual void Tick(float DeltaSeconds) override;	// BODY: (m)

	// --- look ---------------------------------------------------------------------------------------------------------

	/** Adds to the look: yaw wraps, pitch clamps to [MinPitch, MaxPitch]; the boom is rotated at once (same frame). */
	void AddLook(float YawDeg, float PitchDeg);	// BODY: (m)

	/** Sets the look (pitch clamped), boom rotated at once. Used at BeginPlay, by TakeLook and TakeAim. */
	void SetLook(float Yaw, float Pitch);	// BODY: (m)

	float GetLookYaw() const { return LookYaw; }
	float GetLookPitch() const { return LookPitch; }

	// --- walking ------------------------------------------------------------------------------------------------------

	/** This frame's walk input from the player controller: Axes.X forward, Axes.Y right, each -1..1, relative to
	 *  LookYaw (pitch ignored). Calls AddMovementInput this frame (the movement component, which ticks after the
	 *  controller, moves him on this same frame). Zero axes: no input (he brakes at WalkBraking). */
	void Move(const FVector2D& Axes);	// BODY: (m)

	/** The test auto-walk (TakeWalkTo): AddMovementInput toward WorldPoint in 2D, full scale, this frame. The caller
	 *  stops calling it once the feet are within its tolerance. */
	void MoveToward(const FVector& WorldPoint);	// BODY: (m)

	/** 2D speed below StandSpeed (standing still: marks count it). */
	bool IsStanding() const;	// BODY: (m)

	/** His feet: actor location minus the capsule's scaled half height. Tape, frames, marks and the beam test this. */
	FVector GetFeetLocation() const;	// BODY: (m)

	/** The capsule's scaled radius, cm (the beam's touch test). */
	float GetCapsuleRadius() const;	// BODY: (m)

	// --- the follow view ----------------------------------------------------------------------------------------------

	/** The follow camera's world location, rotation and FOV right now (the flight starts from and lands on this). */
	void GetFollowView(FVector& OutLocation, FRotator& OutRotation, float& OutFOV) const;	// BODY: (m)

	/** The follow camera's world location (the Use aim assist measures from here). */
	FVector GetAimOrigin() const;	// BODY: (m)

	/** The follow camera's forward vector (the reticle's direction). */
	FVector GetAimDirection() const;	// BODY: (m)

	/** "JULIAN", "MAYA", "VEX" (TakeText::CrewName(Id)). */
	const TCHAR* GetCrewName() const { return TakeText::CrewName(Id); }

	// --- tunables (all re-applied in BeginPlay / OnConstruction by ApplyTunables) ------------------------------------

	/** Whose character and camera this is: A Julian, B Maya, C Vex. Keys 1/2/3 switch to it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take|Character")
	ETakeCamera Id = ETakeCamera::A;

	/** The mannequin's colour (the material's TintParameter). */
	UPROPERTY(EditAnywhere, Category = "Take|Character")
	FLinearColor Tint = FLinearColor::White;

	/** The vector parameter the tint is written to (M_Mannequin calls it "Paint Tint"). */
	UPROPERTY(EditAnywhere, Category = "Take|Character")
	FName TintParameter = TEXT("Paint Tint");

	/** cm/s (MaxWalkSpeed). Nobody runs. */
	UPROPERTY(EditAnywhere, Category = "Take|Character")
	float WalkSpeed = 250.f;

	/** cm/s^2 (MaxAcceleration): low, for the weighted start. */
	UPROPERTY(EditAnywhere, Category = "Take|Character")
	float WalkAcceleration = 600.f;

	/** cm/s^2 (BrakingDecelerationWalking): the weighted stop. */
	UPROPERTY(EditAnywhere, Category = "Take|Character")
	float WalkBraking = 900.f;

	/** Body turn rate toward the walk direction, deg/s (RotationRate.Yaw). */
	UPROPERTY(EditAnywhere, Category = "Take|Character")
	float TurnRate = 400.f;

	/** Boom TargetArmLength, cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Camera")
	float BoomLength = 360.f;

	/** Boom SocketOffset (over the right shoulder), cm. */
	UPROPERTY(EditAnywhere, Category = "Take|Camera")
	FVector ShoulderOffset = FVector(0.f, 50.f, 60.f);

	/** Boom CameraLagSpeed (camera lag is on). */
	UPROPERTY(EditAnywhere, Category = "Take|Camera")
	float CameraLagSpeed = 9.f;

	/** Look pitch limits, degrees (negative looks down). */
	UPROPERTY(EditAnywhere, Category = "Take|Camera")
	float MinPitch = -65.f;

	UPROPERTY(EditAnywhere, Category = "Take|Camera")
	float MaxPitch = 40.f;

	/** The follow camera's field of view, degrees. */
	UPROPERTY(EditAnywhere, Category = "Take|Camera")
	float CameraFOV = 70.f;

	/** Look pitch at the start of a take, degrees. */
	UPROPERTY(EditAnywhere, Category = "Take|Camera")
	float StartLookPitch = -12.f;

	/** Below this 2D speed he counts as standing, cm/s. */
	UPROPERTY(EditAnywhere, Category = "Take|Character")
	float StandSpeed = 10.f;

	// --- components ---------------------------------------------------------------------------------------------------

	/** Absolute rotation (SetUsingAbsoluteRotation(true)), bUsePawnControlRotation false, bDoCollisionTest true
	 *  (ProbeChannel ECC_Camera), bEnableCameraLag true. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Take")
	TObjectPtr<USpringArmComponent> Boom;

	/** On the boom's socket. The character's view (a view target finds it: bFindCameraComponentWhenViewTarget). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Take")
	TObjectPtr<UCameraComponent> FollowCamera;

protected:
	/** Movement (MaxWalkSpeed, MaxAcceleration, BrakingDecelerationWalking, RotationRate), boom (TargetArmLength,
	 *  SocketOffset, CameraLagSpeed) and camera (FieldOfView) from the tunables. */
	void ApplyTunables();	// BODY: (m)

	/** Creates (once) a dynamic material instance for every material slot of the mesh and sets TintParameter = Tint.
	 *  No mesh: does nothing. */
	void ApplyTint();	// BODY: (m)

	/** Boom->SetWorldRotation(FRotator(LookPitch, LookYaw, 0)): the follow camera moves with it this frame. */
	void ApplyLookToBoom();

	/** Friction while braking (BrakingFriction, with bUseSeparateBrakingFriction on). 0: the stop is WalkBraking
	 *  alone (250 cm/s stops in about 0.28 s at 900). The engine default (GroundFriction 8 x 2) stops in about 0.1 s. */
	UPROPERTY(EditAnywhere, Category = "Take|Character")
	float WalkBrakingFriction = 0.f;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	float LookYaw = 0.f;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Take|State")
	float LookPitch = 0.f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> TintMIDs;
};
