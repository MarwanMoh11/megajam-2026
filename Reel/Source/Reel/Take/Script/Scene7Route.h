// Scene 7, part 0. INT. BACKSTAGE CORRIDOR - CONTINUOUS (Vance, 35mm). Owner: s7-route. docs/script/s7.md.
// Marwan's QA (Nov 15 evening): "a narratively sensical sequence where I actually enter from the door I wanted unlocked
// and go to the film room". Vance waits at Scene 5's oak door (hall south wall, x 3650); Cole rigged its scanner from
// the backstage side in Scene 6. The lamp goes green, the door swings in, Cole is there: one line, and he goes east to
// get the dock (the exit) ready. The player walks Vance west down the backstage corridor (the film's light flickering
// through the hatches), through the staff door into the dark west end of the hall, up the projectionist's stair to the
// projection booth (the film room). On the projector mark he stops and the next part (AScene7Booth: the swap) begins.
// Nothing teleports: Vance starts where Scene 5 left him and ends where the booth part picks him up.
//
// The set (the wall's door opening, the projectionist's stair) is tools/script/scene7.py's (tag script_s7). This actor
// carries only the door leaf on its hinge; it stands anywhere (its components are placed in world space at BeginPlay).

#pragma once

#include "CoreMinimal.h"
#include "Script/ScriptScene.h"
#include "Scene7Route.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInterface;
class USoundBase;
class ATakeCharacter;

/** One waypoint of the walk: reaching it (flat radius, and within 150 cm of its height) says its line and points the
 *  guide at the next one. */
USTRUCT(BlueprintType)
struct FScene7RouteStep
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene7") FVector At = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene7") float Radius = 220.f;
	/** The guide's objective while this is the next waypoint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene7") FString Objective;
	/** Said when he reaches it (empty: nothing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene7") FString Speaker;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene7") FString Line;
};

UENUM()
enum class EScene7RouteStage : uint8
{
	Door,		// the lamp goes green, the door swings, Cole's line
	Walk,		// the player walks the route
	Settle,		// on the projector mark: he steps onto it and faces the projector
	Done
};

UCLASS()
class REEL_API AScene7Route : public AScriptScene
{
	GENERATED_BODY()

public:
	AScene7Route();

	virtual void BeginPlay() override;
	virtual void BeginScene() override;
	virtual void TickScene(float DeltaSeconds) override;
	virtual void EndScene() override;
	virtual void ResetScene() override;
	virtual bool HandleSceneKey(const FKey& Key, bool bPressed) override;
	virtual void DrawSceneHud(UCanvas* Canvas) override;

	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<USceneComponent> DoorHinge;
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UStaticMeshComponent> DoorLeaf;
	/** The rigged scanner's lamp on the hall side (red, then green when the door opens). */
	UPROPERTY(VisibleAnywhere, Category = "Scene7") TObjectPtr<UPointLightComponent> DoorLamp;

	// ---- the door (world) ----
	/** The hinge: the door opening's west jamb, at the wall's middle, floor level. The leaf runs +X from it. */
	UPROPERTY(EditAnywhere, Category = "Scene7|Door") FVector DoorHingeAt = FVector(3580.f, -2010.f, 0.f);
	/** Leaf size (cm): width along +X, thickness, height. */
	UPROPERTY(EditAnywhere, Category = "Scene7|Door") FVector DoorSize = FVector(140.f, 8.f, 260.f);
	/** Open: yaw about the hinge (negative swings it into the corridor, -Y). */
	UPROPERTY(EditAnywhere, Category = "Scene7|Door") float DoorOpenYaw = -100.f;
	UPROPERTY(EditAnywhere, Category = "Scene7|Door") TObjectPtr<UMaterialInterface> DoorMaterial;
	/** Scene 5's closed slab (and anything else with this tag): hidden while this part runs, the leaf shows instead. */
	UPROPERTY(EditAnywhere, Category = "Scene7|Door") FName OakSlabTag = TEXT("s7_oak_slab");
	UPROPERTY(EditAnywhere, Category = "Scene7|Door") FVector DoorLampAt = FVector(3745.f, -1975.f, 140.f);

	// ---- Cole ----
	/** Where Cole stands when the door opens (Scene 6 ends him there; a cold jump puts him there). */
	UPROPERTY(EditAnywhere, Category = "Scene7|Cole") FVector ColeMark = FVector(3650.f, -2290.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Scene7|Cole") float ColeYaw = 90.f;
	/** He walks here after his line (the dock door: the exit). */
	UPROPERTY(EditAnywhere, Category = "Scene7|Cole") FVector ColeExit = FVector(1700.f, -2480.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Scene7|Cole") FString ColeLine = TEXT("Door's yours, Your Grace. Projectionist's stair, west end, through the staff door. Way out's through the prop store.");

	// ---- the walk ----
	UPROPERTY(EditAnywhere, Category = "Scene7|Walk") TArray<FScene7RouteStep> Steps;
	/** Vance's walk speed on the route (cm/s; his own is 250: he's in a hurry, he doesn't run). */
	UPROPERTY(EditAnywhere, Category = "Scene7|Walk") float RouteWalkSpeed = 330.f;
	/** The projector mark (where the booth part takes him), and the yaw he faces there. */
	UPROPERTY(EditAnywhere, Category = "Scene7|Walk") FVector ProjectorMark = FVector(-745.f, 166.f, 450.f);
	UPROPERTY(EditAnywhere, Category = "Scene7|Walk") float ProjectorYaw = 52.f;
	/** Within this (flat cm, and above ArriveMinZ) the scene takes him the last steps onto the mark. */
	UPROPERTY(EditAnywhere, Category = "Scene7|Walk") float ArriveRadius = 220.f;
	UPROPERTY(EditAnywhere, Category = "Scene7|Walk") float ArriveMinZ = 330.f;

	// ---- timing ----
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float BeepAt = 0.25f;
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float DoorOpenAt = 0.45f;
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float DoorOpenSeconds = 1.1f;
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float ColeLineAt = 0.9f;
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float ColeWalkAt = 3.2f;
	UPROPERTY(EditAnywhere, Category = "Scene7|Timing") float SettleMaxSeconds = 1.6f;

protected:
	EScene7RouteStage Stage = EScene7RouteStage::Door;
	float StageT = 0.f;
	float PrevT = 0.f;
	int32 NextStep = 0;
	bool bColeWalking = false;
	float SavedWalkSpeed = 0.f;
	FString Objective;

	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> HiddenSlabs;

	bool At(float T) const { return PrevT < T && StageT >= T; }
	void SetStage(EScene7RouteStage S) { Stage = S; StageT = PrevT = 0.f; }
	void SetDoor(float Open01);
	void ShowLeaf(bool bShow);
	void PointGuide();
	void Cue(FName Name, float Volume = 1.f);
	ATakeCharacter* GetCole() const;
};
