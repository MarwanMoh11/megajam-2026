// Scene 6, INT. BACKSTAGE (Cole, raw feed). Owner: the s6-backstage session. Marwan's QA, Nov 15 evening.
// The old vault behind the hall's screen (x 4420..5300, y -1000..1000) is backstage: the film plays reversed on the
// back of the screen, ONE enforcer guards the building's door-control box on the north wall.
// Beats:
//  1. SNEAK    Cole crouches (C or Ctrl, ATakeCharacter's crouch: slower, low, silent; standing footsteps carry), stays
//              out of the guard's cone.
//  2. LURE     the counterweight lever on the west wall drops a sandbag on the thunder sheet (east wall): the guard goes
//              to look, his back to the room, for a long while; then he walks back to his post (the lever re-arms).
//  3. TAKEDOWN from behind, E: a choke-hold, played by both bodies (Scene6Rig: Cole's and the guard's own meshes
//              follow posed copies) on a side camera: the grab, the struggle, the sag, laid down.
//  4. HIDE     E by the body: Cole drags him by the shoulders into the costume trunk; the lid shuts.
//  5. RIG      out into the backstage corridor, E at the oak door's scanner (corridor side; Scene 5's door, Vance's way
//              in, s7-route's part 0): panel off, wires bridged, sparks, its lamp green; an insert of the hall-side
//              lamp going green. Flag S6DoorRigged.
//  6. EXIT     west along the corridor, E at the prop store's door (s8-escape's route to the alley fire exit): its
//              alarm box killed, the door wedged wide open. Flag S6ExitReady. The scene ends; off camera Cole is put
//              on s7-route's mark by the oak door (EndMark), where Scene 7 part 0 picks him up.
// The prop store door stands open in every other scene (the escape runs through it): Scene 6 shuts it on its start.
// Everything is self-run: the guard is an ACastMember this actor spawns and walks, the props are level actors placed by
// tools/script/scene6.py and found by tag, the keys come through HandleSceneKey.
#pragma once

#include "CoreMinimal.h"
#include "Script/ScriptScene.h"
#include "Script/Scene6Rig.h"
#include "Scene6Backstage.generated.h"

class ACastMember;
class ACameraActor;
class UPointLightComponent;
class USpotLightComponent;
class UStaticMeshComponent;
class UPoseableMeshComponent;
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
	float Range = 1000.f;		// sight cone, cm
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

	// ---- the set (world points on the floor; tools/script/scene6.py writes them) ----

	/** The guard's post by the door-control box, and the way he faces there. He scans ScanDegrees either side. */
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector GuardPost = FVector(4850.f, 800.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	float GuardYaw = -90.f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	float ScanDegrees = 35.f;
	/** The counterweight lever (Cole's use point), the thunder sheet, the guard's lookout facing it. */
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector LeverPoint = FVector(4500.f, -450.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector SheetPoint = FVector(5270.f, 560.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector LookoutPoint = FVector(5110.f, 520.f, 0.f);
	/** The costume trunk's centre (the body goes in), and the side Cole stands on to drop him in. */
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector TrunkPoint = FVector(5230.f, 120.f, 0.f);
	/** The oak door's scanner on the corridor side (its face, at chest height) and where Cole stands to work it. */
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector PanelPoint = FVector(3765.f, -2024.f, 140.f);
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector PanelStand = FVector(3765.f, -2110.f, 0.f);
	/** Where the scene leaves Cole (s7-route's part 0 starts with him here), facing EndYaw. */
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector EndMark = FVector(3650.f, -2250.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	float EndYaw = 90.f;
	/** The prop store's door off the corridor (the escape's way out): where Cole stands, its alarm box (face). */
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector ExitStand = FVector(1700.f, -2500.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector ExitAlarmPoint = FVector(1800.f, -2590.f, 152.f);
	/** The way out of the backstage room to the corridor (the HUD marker walks these, then the scanner). */
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	TArray<FVector> RouteToExit = { FVector(4850.f, -1100.f, 0.f), FVector(4500.f, -2300.f, 0.f) };
	/** The insert shot of Vance's door (Scene 5's oak door lamp): the camera's place and what it looks at. */
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector InsertCamAt = FVector(3560.f, -1700.f, 175.f);
	UPROPERTY(EditAnywhere, Category = "Scene6|Set")
	FVector InsertLookAt = FVector(3700.f, -1990.f, 130.f);

	// ---- feel ----

	UPROPERTY(EditAnywhere, Category = "Scene6|Feel")
	float TakedownReach = 165.f;
	/** Cole is "behind" when dot(guard forward, guard->Cole) is below this. */
	UPROPERTY(EditAnywhere, Category = "Scene6|Feel")
	float BehindDot = -0.25f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Feel")
	float UseReach = 175.f;
	/** Seconds in his cone until Cole is made (closer is faster). */
	UPROPERTY(EditAnywhere, Category = "Scene6|Feel")
	float SpotSeconds = 0.9f;
	/** His cone's reach is scaled by this while Cole crouches (low, in the dark). */
	UPROPERTY(EditAnywhere, Category = "Scene6|Feel")
	float CrouchSightScale = 0.6f;
	/** Cole's footsteps carry this far while he walks upright (crouched: silent). Less while the guard is lured. */
	UPROPERTY(EditAnywhere, Category = "Scene6|Feel")
	float HearRadius = 420.f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Feel")
	float LuredHearRadius = 260.f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Feel")
	float ScanSeconds = 4.f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Feel")
	float LureSpeed = 200.f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Feel")
	float LureLookSeconds = 15.f;
	/** The takedown's length: grab, struggle, sag, laid down (s). */
	UPROPERTY(EditAnywhere, Category = "Scene6|Feel")
	float ChokeSeconds = 5.2f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Feel")
	float DragSpeed = 150.f;

	// ---- look ----

	/** The back of the screen: M_OldFilm, mirrored (seen from behind), dimmed like light through fabric. */
	UPROPERTY(VisibleAnywhere, Category = "Scene6|Look")
	TObjectPtr<UStaticMeshComponent> ScreenBack;
	/** The film's light on Cole's face: a flicker, flashes on the loud moments. */
	UPROPERTY(VisibleAnywhere, Category = "Scene6|Look")
	TObjectPtr<UPointLightComponent> FlickerLight;
	/** The stunt shots' key (the takedown, the drag): above and to one side of the stunt camera, aimed at the bodies, so
	 *  they read instead of standing black against the lit screen (Marwan's QA, Nov 17). On only while that camera is. */
	UPROPERTY(VisibleAnywhere, Category = "Scene6|Look")
	TObjectPtr<USpotLightComponent> StuntKey;
	/** A soft fill from the stunt camera itself (no shadows), so the shadow side keeps its shape. */
	UPROPERTY(VisibleAnywhere, Category = "Scene6|Look")
	TObjectPtr<UPointLightComponent> StuntFill;
	/** The door-control box's lamp (red, then green). */
	UPROPERTY(VisibleAnywhere, Category = "Scene6|Look")
	TObjectPtr<UPointLightComponent> LockLamp;
	/** The fire exit's alarm lamp (red, then off). */
	UPROPERTY(VisibleAnywhere, Category = "Scene6|Look")
	TObjectPtr<UPointLightComponent> AlarmLamp;
	/** Sparks at the box while Cole bridges the wires. */
	UPROPERTY(VisibleAnywhere, Category = "Scene6|Look")
	TObjectPtr<UPointLightComponent> SparkLight;
	/** The stunt rigs' hidden posed copies (Cole, the guard). */
	UPROPERTY(VisibleAnywhere, Category = "Scene6|Look")
	TObjectPtr<UPoseableMeshComponent> ColePose;
	UPROPERTY(VisibleAnywhere, Category = "Scene6|Look")
	TObjectPtr<UPoseableMeshComponent> GuardPose;
	UPROPERTY(EditAnywhere, Category = "Scene6|Look")
	TSoftObjectPtr<UMaterialInterface> FilmMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Take/Pictures/M_OldFilm.M_OldFilm")));
	UPROPERTY(EditAnywhere, Category = "Scene6|Look")
	float FilmBrightness = 0.45f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Look")
	float FlickerCandelas = 900.f;
	/** The film's flashes go at most this far over its steady flicker (they reached 5x and blew the room to white). */
	UPROPERTY(EditAnywhere, Category = "Scene6|Look")
	float FlashCap = 1.3f;
	/** The stunt key and fill (candelas), the key's colour, how far round from the camera it stands (degrees, toward
	 *  the camera's right) and how high over the bodies' middle (cm), and its cone (outer degrees). */
	UPROPERTY(EditAnywhere, Category = "Scene6|Look")
	float StuntKeyCandelas = 450.f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Look")
	float StuntFillCandelas = 60.f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Look")
	FLinearColor StuntKeyColor = FLinearColor(1.f, 0.88f, 0.74f);
	UPROPERTY(EditAnywhere, Category = "Scene6|Look")
	float StuntKeyAngle = 38.f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Look")
	float StuntKeyRise = 150.f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Look")
	float StuntKeyCone = 34.f;
	/** The film's rhythm (only the look and the sound now): a cycle, its loud part from LoudStart. */
	UPROPERTY(EditAnywhere, Category = "Scene6|Look")
	float FilmPeriod = 9.f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Look")
	float LoudStart = 6.f;

	// ---- the moving props (engine cubes from scene6.py: they turn about an edge of the 100 cm cube) ----
	UPROPERTY(EditAnywhere, Category = "Scene6|Props")
	float LidOpenDegrees = -80.f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Props")
	float LeverPullDegrees = 60.f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Props")
	float PanelOpenDegrees = -100.f;
	UPROPERTY(EditAnywhere, Category = "Scene6|Props")
	float ExitOpenDegrees = -88.f;
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
	TArray<FName> DragCues = { TEXT("sfx_s6_drag") };
	UPROPERTY(EditAnywhere, Category = "Scene6|Sound")
	TArray<FName> LeverCues = { TEXT("sfx_s6_lever"), TEXT("sfx_winch_release") };
	UPROPERTY(EditAnywhere, Category = "Scene6|Sound")
	TArray<FName> CrashCues = { TEXT("sfx_s6_thunder_sheet"), TEXT("sfx_banner_drop") };
	UPROPERTY(EditAnywhere, Category = "Scene6|Sound")
	TArray<FName> SparkCues = { TEXT("sfx_s6_spark"), TEXT("sfx_s6_maglock") };
	UPROPERTY(EditAnywhere, Category = "Scene6|Sound")
	TArray<FName> UnlockCues = { TEXT("sfx_s6_maglock"), TEXT("sfx_chime") };
	UPROPERTY(EditAnywhere, Category = "Scene6|Sound")
	TArray<FName> AlarmOffCues = { TEXT("sfx_s6_alarm_off"), TEXT("sfx_s6_lever") };
	UPROPERTY(EditAnywhere, Category = "Scene6|Sound")
	TArray<FName> WedgeCues = { TEXT("sfx_s6_wedge"), TEXT("sfx_s6_trunk") };
	UPROPERTY(EditAnywhere, Category = "Scene6|Sound")
	TArray<FName> ColeOpenLine = { TEXT("vo_s6_cole_one") };
	UPROPERTY(EditAnywhere, Category = "Scene6|Sound")
	TArray<FName> ColeSleepLine = { TEXT("vo_s6_cole_sleep"), TEXT("vo_s6_cole_onedown") };
	UPROPERTY(EditAnywhere, Category = "Scene6|Sound")
	TArray<FName> ColeRiggedLine = { TEXT("vo_s6_cole_rigged") };
	UPROPERTY(EditAnywhere, Category = "Scene6|Sound")
	TArray<FName> ColeExitLine = { TEXT("vo_s6_cole_exit") };
	UPROPERTY(EditAnywhere, Category = "Scene6|Sound")
	TArray<FName> EnforcerHuhLine = { TEXT("vo_s6_enforcer_what") };

	// Test hooks (console commands in the .cpp).
	void TestUse() { HandleSceneKey(EKeys::E, true); HandleSceneKey(EKeys::E, false); }
	void TestCrouch(bool bOn);
	void TestPlaceCole(const FVector& Feet, float Yaw);
	FVector TestGuardFeet() const;
	float TestGuardYaw() const;

private:
	enum class EStep : uint8 { Sneak, Choke, Down, Drag, ToPanel, Rigging, ToExit, Wedging, Done, Caught };
	enum class EGuard : uint8 { Post, ToLook, Looking, Back };

	EStep Step = EStep::Sneak;
	float StepAge = 0.f;
	EGuard GuardState = EGuard::Post;
	float GuardAge = 0.f;

	UPROPERTY(Transient)
	FS6Guard G;
	bool bSpawned = false;

	// The stunt rigs and the takedown's places
	FS6Rig ColeRig;
	FS6Rig GuardRig;
	FVector ChokeAt = FVector::ZeroVector;	// the guard's feet when grabbed
	FVector ChokeFwd = FVector::ForwardVector;
	FVector ColeFrom = FVector::ZeroVector;
	FVector ColeFromFwd = FVector::ForwardVector;
	FS6Pose ColeStartPose;
	FVector BodyAt = FVector::ZeroVector;	// the laid-down body's pelvis on the floor
	FVector BodyHeadDir = FVector::ForwardVector;	// from his pelvis toward his head
	TArray<FVector> DragPath;
	float DragLen = 0.f;
	float GaitPhase = 0.f;
	int32 RouteIndex = 0;
	FVector LastColeFeet = FVector::ZeroVector;
	bool bColeCrouched = false;	// read from Cole every frame

	// Cameras
	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> StuntCam;
	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> InsertCam;

	// Being caught: why
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
	TObjectPtr<AActor> PanelCover;
	UPROPERTY(Transient)
	TObjectPtr<AActor> ExitLeaf;
	UPROPERTY(Transient)
	TObjectPtr<AActor> ExitWedge;
	FTransform LidHome, LeverHome, SandbagHome, SheetHome, PanelHome, ExitHome, WedgeHome;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FilmMID;
	float LeverAge = -1.f, CrashAge = -1.f, LidAge = -1.f, PanelAge = -1.f, ExitAge = -1.f;
	bool bLidOpen = false;
	bool bLeverArmed = true;
	float FlashLeft = 0.f;
	FLinearColor FlashColor = FLinearColor::White;
	int32 LastLoudCycle = -1;
	// HUD
	FString Objective;
	FString Toast;
	float ToastLeft = 0.f;

	void SpawnCast();
	void FindProps();
	void PlaceGuardHome();
	void SetupRigs();
	void SetCrouch(bool bOn);
	void FreezeCole(bool bFreeze);
	void ViewStunt(bool bOn);
	/** The stunt key and fill placed for the stunt camera at CamAt looking at Look, and on. */
	void LightStunt(const FVector& CamAt, const FVector& Look);

	void StepFilm(float Dt);
	void StepGuard(float Dt);
	void StepSight(float Dt);
	void StepProps(float Dt);
	void StepChoke(float Dt);
	void StepDown(float Dt);
	void StepDrag(float Dt);
	void StepRig(float Dt);
	void StepWedge(float Dt);
	void Catch(FName Reason);

	bool TryUse();
	bool IsBehind(const FVector& ColeAt) const;
	bool CanSee(const FVector& ColeAt) const;
	bool ColeMovingUpright() const;
	void StartChoke();
	void StartDrag();
	void Lure();
	void SetExitOpen(float Alpha);

	// Poses
	FS6Pose GuardLying(float Lolled) const;
	FS6Pose ColeCrouchWalk(const FVector& Feet, const FVector& Fwd, float Speed, float Drop) const;
	void LegsWalk(FS6Pose& P, const FS6Rig& Rig, const FVector& Feet, const FVector& Fwd, float Phase, float Amp, float Spread) const;

	static void TurnAboutEdge(AActor* Actor, const FTransform& Home, const FVector& HingeLocal, const FVector& AxisLocal, float Degrees);
	USoundBase* FindCue(const TArray<FName>& Cues);
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<USoundBase>> CueCache;
	TSet<FName> CueMissing;

	FVector ColeLocation() const;	// his feet
	FVector ObjectivePoint() const;
	void Play(const TArray<FName>& Cues, const FVector* At = nullptr, float Volume = 1.f);
	void SayCue(const FString& Speaker, const FString& Line, const TArray<FName>& Cues);
	void Flash(const FString& Text, float Seconds = 2.5f) { Toast = Text; ToastLeft = Seconds; }
};
