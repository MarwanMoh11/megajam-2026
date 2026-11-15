// See Scene7Route.h. Owner: s7-route.
#include "Script/Scene7Route.h"
#include "Script/ScriptSequencer.h"
#include "TakeCharacter.h"
#include "TakeAudio.h"
#include "TakeGuide.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogScene7Route, Log, All);

namespace
{
	FScene7RouteStep Step(const FVector& At, float Radius, const TCHAR* Objective, const TCHAR* Speaker = TEXT(""),
		const TCHAR* Line = TEXT(""))
	{
		FScene7RouteStep S;
		S.At = At;
		S.Radius = Radius;
		S.Objective = Objective;
		S.Speaker = Speaker;
		S.Line = Line;
		return S;
	}
}

AScene7Route::AScene7Route()
{
	SceneNumber = 7;
	Pov = EScriptPov::Vance;
	Slug = FText::FromString(TEXT("INT. BACKSTAGE CORRIDOR - CONTINUOUS"));
	bPlayerWalks = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeF(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DoorF(TEXT("/Game/Take/Materials/MI_Box_Door.MI_Box_Door"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	DoorHinge = CreateDefaultSubobject<USceneComponent>(TEXT("DoorHinge"));
	DoorHinge->SetupAttachment(Root);
	DoorHinge->SetMobility(EComponentMobility::Movable);
	DoorLeaf = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorLeaf"));
	DoorLeaf->SetupAttachment(DoorHinge);
	DoorLeaf->SetStaticMesh(CubeF.Object);
	DoorLeaf->SetCollisionProfileName(TEXT("BlockAll"));
	DoorLeaf->SetCanEverAffectNavigation(false);
	DoorLeaf->SetHiddenInGame(true);
	DoorLamp = CreateDefaultSubobject<UPointLightComponent>(TEXT("DoorLamp"));
	DoorLamp->SetupAttachment(Root);
	DoorLamp->IntensityUnits = ELightUnits::Candelas;
	DoorLamp->Intensity = 0.f;
	DoorLamp->AttenuationRadius = 260.f;
	DoorLamp->SetCastShadows(false);
	DoorMaterial = DoorF.Object;

	// The route, west from the oak door to the booth (world cm; build_auction.py's plan).
	Steps = {
		Step(FVector(3650.f, -2300.f, 0.f), 200.f, TEXT("Through the door"), TEXT("NORA (V.O.)"),
			TEXT("I see you, Vance. Corridor's clear all the way west. Every guard's watching the film.")),
		Step(FVector(1700.f, -2310.f, 0.f), 260.f, TEXT("West along the backstage corridor")),
		Step(FVector(-300.f, -2300.f, 0.f), 200.f, TEXT("West along the backstage corridor"), TEXT("MAX (V.O.)"),
			TEXT("Staff door on your right. West end of the hall's dark, the rope's behind you.")),
		Step(FVector(-300.f, -1860.f, 0.f), 160.f, TEXT("Through the staff door")),
		Step(FVector(-680.f, -1240.f, 0.f), 170.f, TEXT("The projectionist's stair"), TEXT("VANCE"),
			TEXT("Reel two. Right on cue.")),
		Step(FVector(-680.f, -330.f, 450.f), 150.f, TEXT("Up the stair to the booth")),
		Step(FVector(-690.f, -60.f, 450.f), 140.f, TEXT("Into the booth")),
	};
}

void AScene7Route::BeginPlay()
{
	Super::BeginPlay();
	DoorHinge->SetWorldLocationAndRotation(DoorHingeAt, FRotator::ZeroRotator);
	DoorLeaf->SetRelativeLocation(FVector(DoorSize.X * 0.5f, 0.f, DoorSize.Z * 0.5f));
	DoorLeaf->SetRelativeScale3D(DoorSize / 100.f);
	if (DoorMaterial) { DoorLeaf->SetMaterial(0, DoorMaterial); }
	DoorLamp->SetWorldLocation(DoorLampAt);
	ShowLeaf(false);
}

// ---- helpers ------------------------------------------------------------------------------------------------------

void AScene7Route::Cue(FName Name, float Volume)
{
	const FString Path = FString::Printf(TEXT("/Game/Take/Audio/Script/S7/%s.%s"), *Name.ToString(), *Name.ToString());
	if (USoundBase* S = LoadObject<USoundBase>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		UGameplayStatics::PlaySound2D(this, S, Volume);
	}
	else if (UTakeAudioComponent* A = UTakeAudioComponent::Get(this))
	{
		A->PlayCue(Name, Volume);
	}
}

ATakeCharacter* AScene7Route::GetCole() const
{
	AScriptSequencer* S = GetSequencer();
	return S ? S->GetPovCharacter(EScriptPov::Cole) : nullptr;
}

void AScene7Route::ShowLeaf(bool bShow)
{
	DoorLeaf->SetHiddenInGame(!bShow);
	DoorLeaf->SetCollisionEnabled(bShow ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	// Scene 5's slab is the closed door the other scenes see; while the leaf shows, the slab goes.
	if (bShow)
	{
		HiddenSlabs.Reset();
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (It->ActorHasTag(OakSlabTag) && !It->IsHidden())
			{
				It->SetActorHiddenInGame(true);
				It->SetActorEnableCollision(false);
				HiddenSlabs.Add(*It);
			}
		}
	}
	else
	{
		for (AActor* A : HiddenSlabs)
		{
			if (A) { A->SetActorHiddenInGame(false); A->SetActorEnableCollision(true); }
		}
		HiddenSlabs.Reset();
	}
}

void AScene7Route::SetDoor(float Open01)
{
	const float A = FMath::InterpEaseInOut(0.f, 1.f, FMath::Clamp(Open01, 0.f, 1.f), 2.f);
	DoorHinge->SetWorldRotation(FRotator(0.f, DoorOpenYaw * A, 0.f));
}

void AScene7Route::PointGuide()
{
	if (UTakeGuideSubsystem* G = UTakeGuideSubsystem::Get(this))
	{
		if (Steps.IsValidIndex(NextStep))
		{
			Objective = Steps[NextStep].Objective;
			G->SetGoal(Steps[NextStep].At, Objective);
		}
		else
		{
			Objective = TEXT("The projector");
			G->SetGoal(ProjectorMark, Objective);
		}
	}
}

// ---- the scene ----------------------------------------------------------------------------------------------------

void AScene7Route::BeginScene()
{
	Super::BeginScene();
	NextStep = 0;
	bColeWalking = false;
	bPlayerWalks = true;
	ShowLeaf(true);
	SetDoor(0.f);
	// Scene 5's own lamp (3745, -1980) went green when Cole rigged the door in Scene 6; ours stays dark unless tuned.
	DoorLamp->SetLightColor(FLinearColor(0.1f, 1.f, 0.2f));
	DoorLamp->SetIntensity(0.f);

	if (ATakeCharacter* V = GetPovCharacter())
	{
		if (UCharacterMovementComponent* M = V->GetCharacterMovement())
		{
			if (M->MovementMode == MOVE_None) { M->SetMovementMode(MOVE_Walking); }
			SavedWalkSpeed = M->MaxWalkSpeed;
			M->MaxWalkSpeed = RouteWalkSpeed;
		}
	}
	// Cole on his side of the door (where Scene 6 leaves him; a cold jump puts him there behind the closed door).
	if (ATakeCharacter* Cole = GetCole())
	{
		const float Half = Cole->GetCapsuleComponent() ? Cole->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
		if (FVector::Dist2D(Cole->GetActorLocation(), ColeMark) > 60.f)
		{
			Cole->SetActorLocationAndRotation(ColeMark + FVector(0.f, 0.f, Half + 2.f), FRotator(0.f, ColeYaw, 0.f),
				false, nullptr, ETeleportType::TeleportPhysics);
		}
		else
		{
			Cole->SetActorRotation(FRotator(0.f, ColeYaw, 0.f));
		}
		if (UCharacterMovementComponent* M = Cole->GetCharacterMovement())
		{
			M->StopMovementImmediately();
			if (M->MovementMode == MOVE_None) { M->SetMovementMode(MOVE_Walking); }
		}
	}
	PointGuide();
	SetStage(EScene7RouteStage::Door);
	UE_LOG(LogScene7Route, Log, TEXT("S7 route: begin"));
}

void AScene7Route::EndScene()
{
	if (ATakeCharacter* V = GetPovCharacter())
	{
		if (UCharacterMovementComponent* M = V->GetCharacterMovement())
		{
			M->MaxWalkSpeed = SavedWalkSpeed > 0.f ? SavedWalkSpeed : V->WalkSpeed;
		}
	}
	if (UTakeGuideSubsystem* G = UTakeGuideSubsystem::Get(this)) { G->ClearGoal(); }
	// The other scenes see Scene 5's closed door again.
	ShowLeaf(false);
	DoorLamp->SetIntensity(0.f);
	Super::EndScene();
}

void AScene7Route::ResetScene()
{
	SetDoor(0.f);
	NextStep = 0;
	bColeWalking = false;
	Stage = EScene7RouteStage::Door;
	StageT = PrevT = 0.f;
}

bool AScene7Route::HandleSceneKey(const FKey& Key, bool bPressed)
{
	// Use does nothing on the walk (the booth part owns the projector); swallow it so the director doesn't act on it.
	return Key == EKeys::E;
}

void AScene7Route::TickScene(float DeltaSeconds)
{
	PrevT = StageT;
	StageT += DeltaSeconds;
	ATakeCharacter* V = GetPovCharacter();
	ATakeCharacter* Cole = GetCole();

	// Cole's walk to the dock: the test auto-walk, one frame at a time, until he's there.
	if (bColeWalking && Cole)
	{
		if (FVector::Dist2D(Cole->GetActorLocation(), ColeExit) > 40.f) { Cole->MoveToward(ColeExit); }
		else { bColeWalking = false; }
	}

	switch (Stage)
	{
	case EScene7RouteStage::Door:
	{
		if (At(BeepAt))
		{
			Cue(TEXT("sfx_beep"), 0.8f);
		}
		if (StageT >= DoorOpenAt) { SetDoor((StageT - DoorOpenAt) / DoorOpenSeconds); }
		if (At(DoorOpenAt)) { Cue(TEXT("sfx_s7_door_click")); }
		if (At(ColeLineAt)) { Say(TEXT("COLE"), ColeLine); }
		if (At(ColeWalkAt) && Cole) { bColeWalking = true; }
		if (StageT >= DoorOpenAt + DoorOpenSeconds)
		{
			SetDoor(1.f);
			if (StageT >= ColeWalkAt) { SetStage(EScene7RouteStage::Walk); }
		}
		break;
	}
	case EScene7RouteStage::Walk:
	{
		if (!V) { break; }
		const FVector Feet = V->GetActorLocation() - FVector(0.f, 0.f,
			V->GetCapsuleComponent() ? V->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f);
		// Steps can be reached out of order (a player who cuts a corner): take the furthest one in reach.
		for (int32 i = Steps.Num() - 1; i >= NextStep; --i)
		{
			const FScene7RouteStep& S = Steps[i];
			if (FVector::Dist2D(Feet, S.At) <= S.Radius && FMath::Abs(Feet.Z - S.At.Z) < 150.f)
			{
				for (int32 j = NextStep; j <= i; ++j)
				{
					if (!Steps[j].Line.IsEmpty()) { Say(Steps[j].Speaker, Steps[j].Line); }
				}
				NextStep = i + 1;
				PointGuide();
				break;
			}
		}
		if (FVector::Dist2D(Feet, ProjectorMark) <= ArriveRadius && Feet.Z > ArriveMinZ)
		{
			bPlayerWalks = false;
			SetStage(EScene7RouteStage::Settle);
		}
		break;
	}
	case EScene7RouteStage::Settle:
	{
		if (!V) { FinishScene(); SetStage(EScene7RouteStage::Done); break; }
		const bool bThere = FVector::Dist2D(V->GetActorLocation(), ProjectorMark) <= 12.f;
		if (!bThere && StageT < SettleMaxSeconds) { V->MoveToward(ProjectorMark); }
		// Turn to the projector as he steps on.
		const float Yaw = FMath::FixedTurn(V->GetActorRotation().Yaw, ProjectorYaw, 360.f * DeltaSeconds);
		V->SetActorRotation(FRotator(0.f, Yaw, 0.f));
		if ((bThere || StageT >= SettleMaxSeconds) && FMath::Abs(FMath::FindDeltaAngleDegrees(Yaw, ProjectorYaw)) < 3.f)
		{
			if (UCharacterMovementComponent* M = V->GetCharacterMovement()) { M->StopMovementImmediately(); }
			V->SetActorRotation(FRotator(0.f, ProjectorYaw, 0.f));
			if (AController* C = V->GetController()) { C->SetControlRotation(FRotator(0.f, ProjectorYaw, 0.f)); }
			UE_LOG(LogScene7Route, Log, TEXT("S7 route: on the projector mark (%.0f cm off)"),
				FVector::Dist2D(V->GetActorLocation(), ProjectorMark));
			SetStage(EScene7RouteStage::Done);
			FinishScene();
		}
		break;
	}
	default:
		break;
	}
}

void AScene7Route::DrawSceneHud(UCanvas* Canvas)
{
	// The guide (hud-guidance) draws the objective and marker; nothing of our own.
}
