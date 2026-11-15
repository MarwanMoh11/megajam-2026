// Scene 6, INT. BACKSTAGE CORRIDOR & RIGGING (Cole, raw feed). Owner: the s6-backstage session. docs/script/s6.md.
// The vault room behind the hall's screen (x 4420..5300, y -1000..1000) is backstage: the film plays reversed on the
// back of the screen, two ENFORCERS hold the way up to the booth door, a projectionist stands at the door.
// Beats: (1) take Enforcer A down from behind, only while the film is loud (gunfight, explosion), and he goes in the
// costume trunk; (2) pull the counterweight lever: the sandbag hits the thunder sheet and Enforcer B leaves the stairs
// to look; (3) up the stairs, knock out the projectionist from behind; (4) the mag-lock override opens the booth door.
// Everything here is self-run: the enforcers are ACastMembers this actor spawns and walks (no director watchers), the
// props are level actors placed by tools/script/scene6.py and found by tag, the keys come through HandleSceneKey.
#pragma once

#include "CoreMinimal.h"
#include "Script/ScriptScene.h"
#include "Scene6Backstage.generated.h"

class ACastMember;
class UPointLightComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class USoundBase;

/** One person on the set Cole has to get past. */
USTRUCT()
struct FS6Guard
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<ACastMember> Body;

	FVector Home = FVector::ZeroVector;
	float HomeYaw = 0.f;
	float Range = 800.f;		// sight cone, cm
	float HalfAngle = 40.f;		// degrees
	float Alert = 0.f;			// 0..1, 1 = Cole is made
	bool bDown = false;			// knocked out
	FString Name;
};

UCLASS()
class REEL_API AScene6Backstage : public AScriptScene
{
	GENERATED_BODY()

public:
	AScene6Backstage();

	virtual void BeginPlay() override;
	virtual void BeginScene() override;
	virtual void TickScene(float DeltaSeconds) override;
	virtual void EndScene() override;
	virtual void ResetScene() override;
	virtual bool HandleSceneKey(const FKey& Key, bool bPressed) override;
	virtual void DrawSceneHud(UCanvas* Canvas) override;

	// ---- the set (world points; tools/script/scene6.py may overwrite them) ----

	/** Enforcer A's patrol: walks between these two, pausing at each end. */
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector PatrolA0 = FVector(4700.f, -620.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector PatrolA1 = FVector(4700.f, 640.f, 0.f);
	/** Enforcer B's post: the top of the stairs, facing down them. */
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector PostB = FVector(5120.f, 380.f, 250.f);
	/** The projectionist at the booth door, his back to the stairs. */
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector PostProjectionist = FVector(5000.f, 880.f, 250.f);
	/** The costume trunk (where A is dragged), the lever's use point, the thunder sheet, B's lookout by it. */
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector TrunkPoint = FVector(4530.f, 360.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector LeverPoint = FVector(4520.f, -450.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector SheetPoint = FVector(4560.f, 920.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector LookoutB = FVector(4700.f, 740.f, 0.f);
	/** The mag-lock panel by the booth door (its lamp goes red to green), and the door's hinge side. */
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector PanelPoint = FVector(4890.f, 990.f, 380.f);
	/** The stair: x range, from (y0, z 0) up to (y1, StairTopZ); the landing runs on to the north wall. */
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector2D StairX = FVector2D(5040.f, 5200.f);
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector2D StairY = FVector2D(-200.f, 300.f);
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector2D LandingX = FVector2D(4880.f, 5200.f);
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	float StairTopZ = 250.f;

	// ---- the film's soundtrack: loud moments are cover ----

	/** One cycle of the film: quiet, a build (the warning), then loud. */
	UPROPERTY(EditAnywhere, Category = "Scene6|Film")
	float FilmPeriod = 9.f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Film")
	float LoudStart = 6.f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Film")
	float BuildSeconds = 1.5f;
	/** The first cycle starts this far in, so the first loud moment comes ~4 s after the line. */
	UPROPERTY(EditAnywhere, Category = "Scene6|Film")
	float FilmOffset = 1.0f;

	// ---- feel ----

	UPROPERTY(EditAnywhere, Category = "Scene6|Feel")
	float TakedownReach = 175.f;
	/** Cole is "behind" when dot(guard forward, guard->Cole) is below this. */
	UPROPERTY(EditAnywhere, Category = "Scene6|Feel")
	float BehindDot = -0.15f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Feel")
	float UseReach = 170.f;
	/** Seconds in a cone until he's made (closer is faster). */
	UPROPERTY(EditAnywhere, Category = "Scene6|Feel")
	float SpotSeconds = 0.7f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Feel")
	float PatrolSpeed = 140.f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Feel")
	float PatrolPause = 2.5f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Feel")
	float LureSpeed = 230.f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Feel")
	float LureLookSeconds = 9.f;
	/** The projectionist looks round every GlanceEvery s, for GlanceHold s. */
	UPROPERTY(EditAnywhere, Category = "Scene6|Feel")
	float GlanceEvery = 6.5f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Feel")
	float GlanceHold = 1.6f;

	// ---- look ----

	/** The back of the screen: M_OldFilm, mirrored (seen from behind), dimmed like light through fabric. */
	UPROPERTY(VisibleAnywhere, Category = "Scene6|Look")
	TObjectPtr<UStaticMeshComponent> ScreenBack;
	/** The film's light on Cole's face: a flicker, flashes on the loud moments. */
	UPROPERTY(VisibleAnywhere, Category = "Scene6|Look")
	TObjectPtr<UPointLightComponent> FlickerLight;
	/** The mag-lock's lamp (red, then green). */
	UPROPERTY(VisibleAnywhere, Category = "Scene6|Look")
	TObjectPtr<UPointLightComponent> LockLamp;
	UPROPERTY(EditAnywhere, Category = "Scene6|Look")
	TSoftObjectPtr<UMaterialInterface> FilmMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Take/Pictures/M_OldFilm.M_OldFilm")));
	UPROPERTY(EditAnywhere, Category = "Scene6|Look")
	float FilmBrightness = 0.45f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Look")
	float FlickerCandelas = 900.f;

	// ---- the moving props (engine cubes from scene6.py: they turn about an edge of the 100 cm cube) ----
	UPROPERTY(EditAnywhere, Category = "Scene6|Props")
	float LidOpenDegrees = -75.f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Props")
	float LeverPullDegrees = 60.f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Props")
	float DoorOpenDegrees = -85.f;
	/** How far the sandbag falls onto the thunder sheet, cm. */
	UPROPERTY(EditAnywhere, Category = "Scene6|Props")
	float SandbagDrop = 170.f;

	// ---- sound (names under /Game/Take/Audio; the S6 folder first if it has them) ----
	UPROPERTY(EditAnywhere, Category = "Scene6|Sound")
	TArray<FName> GunfightCues = { TEXT("sfx_s6_gunfight"), TEXT("sfx_glass_smash") };
	UPROPERTY(EditAnywhere, Category = "Scene6|Sound")
	TArray<FName> ExplosionCues = { TEXT("sfx_s6_explosion"), TEXT("sfx_pyro") };
	UPROPERTY(EditAnywhere, Category = "Scene6|Sound")
	TArray<FName> TakedownCues = { TEXT("sfx_s6_choke"), TEXT("sfx_thud") };
	UPROPERTY(EditAnywhere, Category = "Scene6|Sound")
	TArray<FName> LeverCues = { TEXT("sfx_s6_lever"), TEXT("sfx_winch_release") };
	UPROPERTY(EditAnywhere, Category = "Scene6|Sound")
	TArray<FName> CrashCues = { TEXT("sfx_s6_thunder_sheet"), TEXT("sfx_banner_drop") };
	UPROPERTY(EditAnywhere, Category = "Scene6|Sound")
	TArray<FName> UnlockCues = { TEXT("sfx_s6_maglock"), TEXT("sfx_chime") };
	UPROPERTY(EditAnywhere, Category = "Scene6|Sound")
	TArray<FName> ColeOpenLine = { TEXT("vo_s6_cole_open") };
	UPROPERTY(EditAnywhere, Category = "Scene6|Sound")
	TArray<FName> ColeDoorLine = { TEXT("vo_s6_cole_door") };
	UPROPERTY(EditAnywhere, Category = "Scene6|Sound")
	TArray<FName> EnforcerHuhLine = { TEXT("vo_s6_enforcer_what") };

private:
	enum class EStep : uint8 { Patrol, Dragging, Lever, Lured, Projectionist, Panel, Done, Caught };

	EStep Step = EStep::Patrol;
	float StepAge = 0.f;

	UPROPERTY(Transient)
	FS6Guard A;
	UPROPERTY(Transient)
	FS6Guard B;
	UPROPERTY(Transient)
	FS6Guard P;
	bool bSpawned = false;

	// A's patrol
	int32 PatrolLeg = 1;	// walking toward PatrolA1 (1) or PatrolA0 (0)
	float PauseLeft = 0.f;
	// The drag into the trunk
	FVector DragFrom = FVector::ZeroVector;
	float DownAge = -1.f;	// A tipping over
	// B's lure route
	TArray<FVector> BRoute;
	int32 BRouteIndex = 0;
	float BLookLeft = 0.f;
	bool bBReturning = false;
	// The projectionist
	float GlanceClock = 0.f;
	bool bGlancing = false;
	float PDownAge = -1.f;
	// Being caught: who turned, and why
	FName CaughtReason;
	float CaughtAge = 0.f;
	// The props
	UPROPERTY(Transient)
	TObjectPtr<AActor> TrunkLid;
	UPROPERTY(Transient)
	TObjectPtr<AActor> LeverHandle;
	UPROPERTY(Transient)
	TObjectPtr<AActor> Sandbag;
	UPROPERTY(Transient)
	TObjectPtr<AActor> Sheet;
	UPROPERTY(Transient)
	TObjectPtr<AActor> DoorLeaf;
	FTransform LidHome, LeverHome, SandbagHome, SheetHome, DoorHome;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FilmMID;
	float LeverAge = -1.f, CrashAge = -1.f, DoorAge = -1.f, LidAge = -1.f;
	bool bLeverPulled = false, bUnlocked = false;
	float FlashLeft = 0.f;
	FLinearColor FlashColor = FLinearColor::White;
	int32 LastLoudCycle = -1;
	// HUD
	FString Objective;
	FString Toast;
	float ToastLeft = 0.f;

	void SpawnCast();
	ACastMember* SpawnOne(const FVector& At, float Yaw, uint8 Kit, const FString& Label, const FLinearColor& Tint);
	void FindProps();
	void PlaceGuardsHome();

	// The film's clock: 0..1 loudness, the build before it, and which cue this cycle is.
	float FilmT() const;
	bool IsLoud() const;
	bool IsBuilding() const;
	bool CycleIsExplosion() const;

	void StepFilm(float Dt);
	void StepPatrolA(float Dt);
	void StepDrag(float Dt);
	void StepLure(float Dt);
	void StepProjectionist(float Dt);
	void StepProps(float Dt);
	void StepSight(FS6Guard& G, float Dt, bool bLooking);
	void Catch(FS6Guard& G, FName Reason);

	bool TryUse();
	bool IsBehind(const FS6Guard& G, const FVector& ColeAt) const;
	bool CanSee(const FS6Guard& G, const FVector& ColeAt) const;
	void KnockDown(FS6Guard& G);
	float FloorZ(const FVector& At) const;
	static void TurnAboutEdge(AActor* Actor, const FTransform& Home, const FVector& HingeLocal, const FVector& AxisLocal, float Degrees);
	class USoundBase* FindCue(const TArray<FName>& Cues);
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<USoundBase>> CueCache;
	TSet<FName> CueMissing;
	FVector ColeDragFrom = FVector::ZeroVector;

	FVector ColeLocation() const;
	void Play(const TArray<FName>& Cues, const FVector* At = nullptr, float Volume = 1.f);
	void SayCue(const FString& Speaker, const FString& Line, const TArray<FName>& Cues);
	void Flash(const FString& Text, float Seconds = 2.5f) { Toast = Text; ToastLeft = Seconds; }
};
