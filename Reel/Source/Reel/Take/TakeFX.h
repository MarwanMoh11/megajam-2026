// Step 5: the devices' particle effects (Niagara, Unreal's particle system). One actor in the level watches the
// devices and spawns each cue's system on the frame of its event: the frame a device fires (its timeline starts) or
// the frame it lands (its effect applies). tools/build_cast.py fills the cues. Effects carry no rule: no stencil,
// so they are monochrome on the film like everything that carries no meaning.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TakeFX.generated.h"

class ADevice;
class UNiagaraSystem;

/** When a cue plays. In Python: unreal.TakeFXWhen. */
UENUM(BlueprintType)
enum class ETakeFXWhen : uint8
{
	Fire,	// the frame the device starts (IsRunning turns true)
	Land	// the frame its effect applies (LandedThisFrame)
};

USTRUCT(BlueprintType)
struct FTakeFXCue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Take|FX")
	TObjectPtr<ADevice> Device;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Take|FX")
	ETakeFXWhen When = ETakeFXWhen::Land;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Take|FX")
	TObjectPtr<UNiagaraSystem> System;

	/** Also at the device's effect point (GetEffectPoint), raised by EffectRaise. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Take|FX")
	bool bAtEffectPoint = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Take|FX")
	float EffectRaise = 0.f;

	/** Fixed world points it plays at (the pyro's charges, the PA stacks). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Take|FX")
	TArray<FVector> Points;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Take|FX")
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Take|FX")
	float Scale = 1.f;
};

UCLASS()
class REEL_API ATakeFX : public AActor
{
	GENERATED_BODY()

public:
	ATakeFX();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Take|FX")
	TArray<FTakeFXCue> Cues;

	/** Master switch. */
	UPROPERTY(EditAnywhere, Category = "Take|FX")
	bool bEffects = true;

	/** Test hook: plays cue Index now (from Python in play). */
	UFUNCTION(BlueprintCallable, Category = "Take|FX")
	void PlayCue(int32 Index);

private:
	TArray<bool> WasRunning;
};
