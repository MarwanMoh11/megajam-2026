// Scene 8's chasing guard: an armed guard who runs after Vance and Cole down the corridors and fires. Owner: s8-escape.
// A puppet: AScene8Alley spawns them at run time, moves them along Vance's own trail (so they take the stairs and the
// doors he took, with no navigation) and calls Fire. The guard has no rules and no collision; nothing it does can hurt
// anyone (the shots spark on walls beside Vance). Rigged guard (SKM_NPC_Guard on the mannequin skeleton) when it
// loads, jogging with a pistol or aiming; else a grey-box guard. Root at the feet, facing +X.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Scene8Chaser.generated.h"

class UAnimationAsset;
class UPointLightComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class REEL_API AScene8Chaser : public AActor
{
	GENERATED_BODY()

public:
	AScene8Chaser();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Scene8|Chaser")
	TSoftObjectPtr<USkeletalMesh> GuardMesh;

	/** Played while he runs, and while he stands and aims. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Chaser")
	TSoftObjectPtr<UAnimationAsset> RunAnim;

	UPROPERTY(EditAnywhere, Category = "Scene8|Chaser")
	TSoftObjectPtr<UAnimationAsset> AimAnim;

	/** A pistol model for his right hand (chars-locomotion's gun when it exists); unset or missing: a grey-box pistol. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Chaser")
	TSoftObjectPtr<UStaticMesh> GunMesh;

	/** The hand bone the pistol sits in. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Chaser")
	FName HandBone = TEXT("hand_r");

	/** The muzzle in the gun's own space (SM_Pistol: grip at the origin, barrel +X), cm. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Chaser")
	FVector GunMuzzle = FVector(20.f, 0.f, 7.1f);

	/** Seconds the muzzle flash and the tracer stay up. */
	UPROPERTY(EditAnywhere, Category = "Scene8|Chaser")
	float FlashSeconds = 0.06f;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<USceneComponent> Root;

	/** The grey-box guard (body, head, arm, pistol): hidden when the rigged guard loads. */
	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<USceneComponent> Box;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<UStaticMeshComponent> Gun;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<UStaticMeshComponent> MuzzleFlash;

	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<UPointLightComponent> MuzzleLight;

	/** A thin bright streak from the muzzle to where the bullet hit, for a frame or two. */
	UPROPERTY(VisibleAnywhere, Category = "Scene8")
	TObjectPtr<UStaticMeshComponent> Tracer;

	/** Running (jog cycle) or standing to aim. Changes the animation on this frame; no-op when unchanged. */
	void SetRunning(bool bRun);
	bool IsRunning() const { return bRunning; }

	/** The pistol's muzzle, world space. */
	FVector GetMuzzle() const;

	/** The flash, the light and the tracer to Hit, this frame. */
	void Fire(const FVector& Hit);

	/** Shown and placed, or hidden (between takes). */
	void Show(bool bShow);

private:
	bool bRunning = false;
	bool bRigged = false;
	bool bGunMesh = false;
	float FlashAge = 1.f;
	void ApplyAnim();
};
