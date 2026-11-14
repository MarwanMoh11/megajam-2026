// Step 5: the devices' particle effects. See TakeFX.h.
#include "TakeFX.h"

#include "Device.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TakeTypes.h"

ATakeFX::ATakeFX()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;	// after the director stepped the devices this frame
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ATakeFX::BeginPlay()
{
	Super::BeginPlay();
	WasRunning.Init(false, Cues.Num());
}

void ATakeFX::PlayCue(int32 Index)
{
	if (!Cues.IsValidIndex(Index) || !Cues[Index].System)
	{
		return;
	}
	const FTakeFXCue& Cue = Cues[Index];
	TArray<FVector> At = Cue.Points;
	if (Cue.bAtEffectPoint && Cue.Device)
	{
		At.Add(Cue.Device->GetEffectPoint() + FVector(0.f, 0.f, Cue.EffectRaise));
	}
	for (const FVector& Point : At)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Cue.System, Point, Cue.Rotation, FVector(Cue.Scale), true, true,
			ENCPoolMethod::None, true);
	}
	TAKE_LOG(TEXT("FX %s %s x%d"), *Cue.System->GetName(), Cue.Device ? *Cue.Device->GetActorNameOrLabel() : TEXT("-"), At.Num());
}

void ATakeFX::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (WasRunning.Num() != Cues.Num())
	{
		WasRunning.Init(false, Cues.Num());
	}
	for (int32 i = 0; i < Cues.Num(); ++i)
	{
		const ADevice* Device = Cues[i].Device;
		if (!Device)
		{
			continue;
		}
		const bool bRunning = Device->IsRunning();
		const bool bFire = (Cues[i].When == ETakeFXWhen::Fire) && bRunning && !WasRunning[i];
		const bool bLand = (Cues[i].When == ETakeFXWhen::Land) && Device->LandedThisFrame();
		WasRunning[i] = bRunning;
		if (bEffects && (bFire || bLand))
		{
			PlayCue(i);
		}
	}
}
