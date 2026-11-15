// Set life (Nov 15): the stage manager. Small rehearsed scenes ("pieces") between the guests and staff, so some side
// business is going on wherever Julian is: a waiter's spilled drink, a photographer's flash, a couple's quarrel...
// Read docs/SET_LIFE.md first. In short:
// - USetLifeSubsystem starts itself in every game world (PIE and packaged), finds the ACastMembers, and keeps 1-3
//   pieces running near Julian / on screen. Console: Reel.SetLife 0 turns it off (running pieces end, people go home).
// - A piece is a C++ class deriving FSetLifePiece in its own .cpp under Take/SetLife/, registered with SETLIFE_PIECE.
//   Its Setup casts free members (FSetLifeRole) and writes a beat sheet on the FSetLifeStage (walk, face, gesture,
//   look, say, wait...). The stage runs the beats, then gives everyone back (walks them home, faces them as they were)
//   and removes the piece's props.
// - Overheard barks (Stage.Say): a small world-anchored caption over the speaker when Julian is within ~7 m and the
//   speaker is on screen; an optional voice (a sound name under /Game/Take/Audio, 3D at the speaker).
// - Logs: "SETLIFE start <piece> cast=... at=(x,y) zone=...", "SETLIFE end <piece> ok|abort(<why>) t=...".
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Templates/Function.h"
#include "Templates/UniquePtr.h"
#include "CastMember.h"
#include "TakeShapes.h"
#include "TakeHudKit.h"
#include "SetLife.generated.h"

class AHUD;
class UCanvas;
class USceneComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class USoundAttenuation;
class USetLifeSubsystem;
class FSetLifeStage;

/** Where on the set. Street: x < -3220; lobby: -3220..-1000 (both floors); hall: x >= -1000; catwalk: z >= 700. */
enum class ESetLifeZone : uint8
{
	None,
	Street,
	Lobby,
	Hall,
	Catwalk
};

namespace SetLife
{
	REEL_API ESetLifeZone ZoneOf(const FVector& WorldPoint);
	REEL_API const TCHAR* ZoneName(ESetLifeZone Zone);
	/** The pool: guests (GuestTux, GuestTails, GuestGown, GuestGownHat) and Staff. Everyone else is never cast. */
	REEL_API bool IsPoolKit(ECastKit Kit);
}

enum class ESetLifeSeat : uint8
{
	Standing,	// default: can walk, turn and shift
	Seated,		// gestures and looks only (WalkTo / Face are ignored for them)
	Either
};

/** What a role wants. Chain the setters: FSetLifeRole().Men().NearPoint(Bar, 600).WithGlass(true). */
struct REEL_API FSetLifeRole
{
	/** Empty: any pool kit. */
	TArray<ECastKit> Kits;
	ESetLifeSeat Seat = ESetLifeSeat::Standing;
	bool bNear = false;
	FVector Near = FVector::ZeroVector;
	/** With bNear: within this of Near, cm (0: anywhere). */
	float Radius = 0.f;
	/** -1 either, 0 without a coupe in hand, 1 holding one (ACastMember::bHoldsGlass). */
	int8 Glass = -1;
	/** Must be on screen now. */
	bool bInView = false;
	/** Must be near the action: within FocusRadius of Julian, or on screen within ViewRadius of the camera. Turn it
	 *  off for a role that walks in from off screen (the waiter from the bar). */
	bool bNearFocus = true;
	/** Never cast anyone this close to Julian, cm (pieces stay clear of his path). */
	float MinFromJulian = 250.f;
	/** Only in this zone (None: the piece's own zone). */
	ESetLifeZone Zone = ESetLifeZone::None;
	/** Anything else (return true to accept). */
	TFunction<bool(const ACastMember&)> Filter;

	FSetLifeRole& Kit(ECastKit K) { Kits.AddUnique(K); return *this; }
	FSetLifeRole& Men() { return Kit(ECastKit::GuestTux).Kit(ECastKit::GuestTails); }
	FSetLifeRole& Women() { return Kit(ECastKit::GuestGown).Kit(ECastKit::GuestGownHat); }
	FSetLifeRole& Guests() { return Men().Women(); }
	FSetLifeRole& Staff() { return Kit(ECastKit::Staff); }
	FSetLifeRole& Standing() { Seat = ESetLifeSeat::Standing; return *this; }
	FSetLifeRole& Seated() { Seat = ESetLifeSeat::Seated; return *this; }
	FSetLifeRole& AnySeat() { Seat = ESetLifeSeat::Either; return *this; }
	FSetLifeRole& NearPoint(const FVector& P, float InRadius) { bNear = true; Near = P; Radius = InRadius; return *this; }
	FSetLifeRole& WithGlass(bool bWith) { Glass = bWith ? 1 : 0; return *this; }
	FSetLifeRole& OnScreen() { bInView = true; return *this; }
	FSetLifeRole& AnyDistance() { bNearFocus = false; return *this; }
	FSetLifeRole& InZone(ESetLifeZone Z) { Zone = Z; return *this; }
	FSetLifeRole& Where(TFunction<bool(const ACastMember&)> F) { Filter = MoveTemp(F); return *this; }
};

/** One piece. A new instance is made for every run, so members may hold the run's state. */
class REEL_API FSetLifePiece
{
public:
	virtual ~FSetLifePiece() = default;

	/** Cast the roles and write the beat sheet. Return false to give up: everyone cast is released, props removed,
	 *  and the scheduler tries again later. Nothing should move here (beats do that). */
	virtual bool Setup(FSetLifeStage& Stage) = 0;

	/** Every frame while it runs, after the beats (the piece clock is held during slow time: check Stage.IsHeld()). */
	virtual void Tick(FSetLifeStage& Stage, float DeltaSeconds) {}

	/** Once at the end (finished or aborted), before the props go and the cast is walked home. */
	virtual void End(FSetLifeStage& Stage, bool bAborted) {}
};

/** A registered piece type (SETLIFE_PIECE). */
struct REEL_API FSetLifePieceInfo
{
	FName Name;
	ESetLifeZone Zone = ESetLifeZone::Hall;
	/** Seconds after a run ends before it may run again (x 0.8..1.2). */
	float Cooldown = 45.f;
	/** Relative chance of being picked among the pieces that may start. */
	float Weight = 1.f;
	TFunction<TUniquePtr<FSetLifePiece>()> Make;
};

struct REEL_API FSetLifeRegistry
{
	static TArray<FSetLifePieceInfo>& Pieces();
	FSetLifeRegistry(const TCHAR* Name, ESetLifeZone Zone, float Cooldown, float Weight, TFunction<TUniquePtr<FSetLifePiece>()> Make);
};

/** In the piece's .cpp, at file scope: SETLIFE_PIECE(SpilledDrink, SetLifeSpilledDrink::FPiece, ESetLifeZone::Hall, 50.f, 1.f)
 *  Id is a plain identifier (the piece's name in logs, unique in the module); Class may be namespaced. */
#define SETLIFE_PIECE(Id, Class, Zone, Cooldown, Weight) \
	static FSetLifeRegistry GSetLifeRegistrar_##Id(TEXT(#Id), Zone, Cooldown, Weight, \
		[]() -> TUniquePtr<FSetLifePiece> { return MakeUnique<Class>(); });

/**
 * One running piece: its cast, its beat sheet and its props. The piece gets it in Setup / Tick / End.
 * Roles are ints (the index Cast returned); INDEX_NONE means "no one" and every call ignores it.
 * The beat sheet runs top to bottom: actions take no time and run on the same frame as the line before them;
 * At / Wait / WaitArrived / WaitGesture / WaitUntil hold the line. Every beat call returns the stage, so they chain.
 */
class REEL_API FSetLifeStage
{
public:
	FSetLifeStage(USetLifeSubsystem* InOwner, const FSetLifePieceInfo& InInfo, TUniquePtr<FSetLifePiece> InPiece);
	~FSetLifeStage();

	// --- context ---------------------------------------------------------------------------------------------------------
	UWorld* GetWorld() const;
	FName GetName() const { return Info.Name; }
	ESetLifeZone GetZone() const { return Info.Zone; }
	/** The piece clock, s since the start (held while the director holds time: slow time, the blooper freeze...). */
	float GetTime() const { return Elapsed; }
	bool IsHeld() const { return bHeld; }
	/** Julian's feet (zero when there is no Julian). */
	FVector GetJulian() const;
	FVector GetCamera() const;
	FVector GetCameraForward() const;
	/** P is in front of the camera, inside its view (about a 75-degree cone). */
	bool IsInView(const FVector& P) const;
	/** Within FocusRadius of Julian, or on screen within ViewRadius of the camera (what FSetLifeRole::bNearFocus asks). */
	bool IsNearFocus(const FVector& P) const;
	/** The floor under P (a trace down from 1 m above); P itself when nothing is found. */
	FVector FloorAt(const FVector& P) const;
	float Rand(float Min, float Max) const { return FMath::FRandRange(Min, Max); }
	int32 RandInt(int32 Min, int32 Max) const { return FMath::RandRange(Min, Max); }
	bool Chance(float P) const { return FMath::FRand() < P; }
	/** One of the strings, at random (repeat takes differ). */
	const FString& Pick(const TArray<FString>& Options) const;

	// --- casting (in Setup) -------------------------------------------------------------------------------------------------
	/** Free members that fit Want, best first (near Want.Near or Julian, on screen first, a little shuffled). */
	TArray<ACastMember*> FindFree(const FSetLifeRole& Want, int32 Max = 8) const;
	/** Claims the best free member that fits; INDEX_NONE if none. */
	int32 Cast(FName Role, const FSetLifeRole& Want);
	/** Claims this member (one FindFree returned); INDEX_NONE if it is not free. */
	int32 CastMember(FName Role, ACastMember* Member);
	ACastMember* Get(int32 Role) const;
	/** Where the member stands (its actor location; zero if none). */
	FVector Where(int32 Role) const;
	/** Its head (the "head" bone, or 160 cm over its feet). */
	FVector Head(int32 Role) const;
	/** Where it stood and faced when cast. */
	FVector Home(int32 Role) const;
	/** Where the piece happens (the log, and the scheduler's "is it still near Julian"). Default: the first role. */
	void SetAnchor(const FVector& P) { Anchor = P; bAnchorSet = true; }
	FVector GetAnchor() const { return Anchor; }
	/** At the end the member walks these points, then home (e.g. round the end of the bar). */
	void SetReturnVia(int32 Role, const TArray<FVector>& Points);

	// --- the beat sheet ------------------------------------------------------------------------------------------------------
	/** Hold until the piece clock reaches T, s. */
	FSetLifeStage& At(float T);
	FSetLifeStage& Wait(float Seconds);
	/** Hold until the member has stopped walking (at most MaxSeconds, then go on). */
	FSetLifeStage& WaitArrived(int32 Role, float MaxSeconds = 10.f);
	/** Hold until its gesture is over (at most MaxSeconds). */
	FSetLifeStage& WaitGesture(int32 Role, float MaxSeconds = 4.f);
	/** Hold until Cond is true (at most MaxSeconds; then go on, or end the piece with bAbortOnTimeout). */
	FSetLifeStage& WaitUntil(TFunction<bool(FSetLifeStage&)> Cond, float MaxSeconds = 10.f, bool bAbortOnTimeout = false);
	/** Walk (ACastMember::WalkTo) to a point; standing members only. */
	FSetLifeStage& Walk(int32 Role, const FVector& To, float Speed = 110.f);
	/** Walk up to Other, stopping StopShort cm before it (its position when the beat runs). */
	FSetLifeStage& WalkUpTo(int32 Role, int32 Other, float StopShort = 80.f, float Speed = 110.f);
	/** Slide the member by Offset (cm, world) over Seconds without turning: a step back, a stumble, a lean in. */
	FSetLifeStage& Shift(int32 Role, const FVector& Offset, float Seconds = 0.35f);
	FSetLifeStage& Face(int32 Role, float Yaw);
	FSetLifeStage& FaceTo(int32 Role, int32 Other);
	FSetLifeStage& FacePoint(int32 Role, const FVector& P);
	/** ACastMember::PlayGesture: Talk, Listen, Laugh, Sip, Nod, Shrug, Point, Wave, Hello, Applaud, CheckWatch,
	 *  LookAround, Bow, Refuse, Use, Lift; None stops the act. */
	FSetLifeStage& Gesture(int32 Role, FName Gesture);
	/** The gesture toward Other's head (Point and Wave reach to it; the head turns to it). */
	FSetLifeStage& GestureAt(int32 Role, FName Gesture, int32 Other);
	FSetLifeStage& GestureAtPoint(int32 Role, FName Gesture, const FVector& P);
	/** The head turns to Other's head for Seconds. */
	FSetLifeStage& Look(int32 Role, int32 Other, float Seconds = 2.5f);
	FSetLifeStage& LookAtPoint(int32 Role, const FVector& P, float Seconds = 2.5f);
	/** An overheard bark: the caption over the speaker (Seconds; 0 = from its length) and the voice Sound if that
	 *  asset exists (/Game/Take/Audio/<Sound>). Keep lines short, 1940s register. */
	FSetLifeStage& Say(int32 Role, const FString& Line, FName Sound = NAME_None, float Seconds = 0.f);
	/** Anything else, on this beat. */
	FSetLifeStage& Do(TFunction<void(FSetLifeStage&)> Fn);
	/** End the piece here (the rest of the sheet is skipped). The end of the sheet ends it anyway. */
	FSetLifeStage& Finish();

	// --- props (removed at the end) ----------------------------------------------------------------------------------------------
	/** A basic shape held by the member: attached to Bone (default the right hand), kept level, turned with the member.
	 *  Offset is in the member's frame (x forward, y right, z up), cm; SizeCm the shape's size (shapes are 100 cm).
	 *  Returns a prop id. */
	int32 HoldProp(int32 Role, ETakeShape Shape, const FVector& SizeCm, const FLinearColor& Colour,
		const FVector& Offset = FVector::ZeroVector, FName Bone = TEXT("hand_r"));
	/** A basic shape standing in the world. */
	int32 AddProp(ETakeShape Shape, const FVector& WorldLocation, const FVector& SizeCm, const FLinearColor& Colour,
		const FRotator& Rotation = FRotator::ZeroRotator);
	UStaticMeshComponent* GetProp(int32 Prop) const;
	/** Lets go of a held prop and stands it at WorldLocation / Rotation (a dropped glass). */
	void DropProp(int32 Prop, const FVector& WorldLocation, const FRotator& Rotation);
	void RemoveProp(int32 Prop);
	/** A point light that flashes up and fades over Seconds (a camera's flash bulb, a lighter). */
	void Flash(const FVector& WorldLocation, float Candelas = 4000.f, float Seconds = 0.35f,
		const FLinearColor& Colour = FLinearColor(1.f, 0.95f, 0.85f));

	/** Ends the piece now; everyone goes home. */
	void Abort(const FString& Why);

private:
	friend class USetLifeSubsystem;

	enum class EStep : uint8 { At, Wait, WaitArrived, WaitGesture, WaitUntil, Walk, WalkUpTo, Shift, Face, FaceTo,
		FacePoint, Gesture, GestureAt, GestureAtPoint, Look, LookAtPoint, Say, Do, Finish };

	struct FStep
	{
		EStep Kind = EStep::Do;
		int32 Role = INDEX_NONE;
		int32 Other = INDEX_NONE;
		FVector Point = FVector::ZeroVector;
		float A = 0.f;
		float B = 0.f;
		bool bFlag = false;
		FName Name;
		FString Text;
		TFunction<void(FSetLifeStage&)> Fn;
		TFunction<bool(FSetLifeStage&)> Cond;
	};

	struct FCast
	{
		FName Name;
		TWeakObjectPtr<ACastMember> Member;
		FVector Home = FVector::ZeroVector;
		float HomeYaw = 0.f;
		TArray<FVector> Via;
		bool bShifting = false;
		FVector ShiftFrom = FVector::ZeroVector;
		FVector ShiftTo = FVector::ZeroVector;
		float ShiftStart = 0.f;
		float ShiftSeconds = 0.f;
	};

	struct FProp
	{
		TWeakObjectPtr<UStaticMeshComponent> Mesh;
		TWeakObjectPtr<USceneComponent> Carrier;	// held: the level, turning mount on the bone
		TWeakObjectPtr<ACastMember> Holder;
	};

	struct FLight
	{
		TWeakObjectPtr<UPointLightComponent> Light;
		float Start = 0.f;
		float Seconds = 0.3f;
		float Candelas = 4000.f;
		FLinearColor Colour = FLinearColor::White;
	};

	FStep& Add(EStep Kind, int32 Role = INDEX_NONE);
	bool RunStep(FStep& Step);
	/** Runs the sheet and the piece's tick; false once the piece is over. */
	bool Step(float DeltaSeconds, bool bHold);
	void TickShifts();
	void TickProps();
	AActor* GetPropsActor();
	void Cleanup();
	bool IsSeatedRole(int32 Role) const;
	FString CastList() const;

	USetLifeSubsystem* Owner = nullptr;
	FSetLifePieceInfo Info;
	TUniquePtr<FSetLifePiece> Piece;
	TArray<FCast> Roles;
	TArray<FStep> Steps;
	TArray<FProp> Props;
	TArray<FLight> Lights;
	TWeakObjectPtr<AActor> PropsActor;
	int32 Cursor = 0;
	float StepStart = -1.f;
	float Elapsed = 0.f;
	float MaxRunSeconds = 60.f;
	bool bHeld = false;
	bool bDone = false;
	bool bAborted = false;
	FString AbortWhy;
	FVector Anchor = FVector::ZeroVector;
	bool bAnchorSet = false;
};

/**
 * The stage manager: one per game world. Finds the pool, schedules the pieces, claims and returns the cast, draws
 * the overheard captions (AHUD::OnHUDPostRender, no TakeHUD change) and plays their voices.
 */
UCLASS()
class REEL_API USetLifeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static USetLifeSubsystem* Get(const UObject* WorldContext);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	/** Free to cast: in the pool, not claimed, not driven by anyone else (the queue, the clerk), standing still. */
	bool IsFree(const ACastMember* Member) const;
	bool IsClaimed(const ACastMember* Member) const;
	/** Ends every running piece (people walk home). */
	void AbortAll(const FString& Why);
	/** A bark: caption and voice (FSetLifeStage::Say calls this). */
	void Say(ACastMember* Speaker, const FString& Line, FName Sound, float Seconds, FName Piece);

	// Tunables (code defaults; console Reel.SetLife.Max overrides MaxRunning).
	float FocusRadius = 2500.f;		// "near the action": this close to Julian...
	float ViewRadius = 3500.f;		// ...or on screen this close to the camera
	float LeaveRadius = 4000.f;		// a piece this far from Julian and off screen ends (people go home)
	float CaptionRadius = 700.f;	// barks show when Julian is this close to the speaker
	float VoiceRadius = 1500.f;		// and are voiced this close
	float FirstStartDelay = 4.f;	// s of the take before the first piece
	float StartGapMin = 3.f;		// s between two starts
	float StartGapMax = 7.f;
	float RetryDelay = 6.f;			// a piece that could not cast waits this long

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	friend class FSetLifeStage;

	struct FReturn
	{
		TWeakObjectPtr<ACastMember> Member;
		TArray<FVector> Path;
		FVector Home = FVector::ZeroVector;
		float HomeYaw = 0.f;
		bool bTurn = true;
		float Started = 0.f;
		bool bKicked = false;		// the first order goes out at once, even over a walk the piece left running
		bool bSliding = false;
		FVector SlideFrom = FVector::ZeroVector;
		float SlideStart = 0.f;
	};

	struct FBark
	{
		TWeakObjectPtr<ACastMember> Speaker;
		FString Line;
		float Start = 0.f;
		float Until = 0.f;
		bool bSeen = false;		// in Julian's earshot and not behind a wall (refreshed a few times a second)
	};

	/** 0 go, 1 hold (no new pieces, piece clocks stop), 2 end the pieces in Zone. */
	int32 GetGate(ESetLifeZone Zone, FString& OutWhy) const;
	void GatherPool();
	void Schedule(float Now);
	bool TryStart(const FSetLifePieceInfo& Info, float Now);
	void FinishStage(FSetLifeStage& Stage);
	void BeginReturn(ACastMember* Member, const FVector& Home, float HomeYaw, const TArray<FVector>& Via, bool bTurn);
	void TickReturns(float Now);
	void TickBarks(float Now);
	void DrawCaptions(AHUD* HUD, UCanvas* DebugCanvas);
	bool IsQueuePoint(const FVector& P) const;
	FVector GetJulian() const;
	bool GetCameraView(FVector& OutLocation, FVector& OutForward) const;
	bool IsInView(const FVector& P) const;
	bool IsNearFocus(const FVector& P) const;

	TArray<TWeakObjectPtr<ACastMember>> Pool;
	TSet<TWeakObjectPtr<ACastMember>> Excluded;
	TSet<TWeakObjectPtr<ACastMember>> Claimed;
	TArray<FVector> QueuePoints;
	TArray<TUniquePtr<FSetLifeStage>> Running;
	TArray<FReturn> Returns;
	TArray<FBark> Barks;
	TMap<FName, float> CooldownUntil;
	float NextStartAt = 0.f;
	float NextBarkCheck = 0.f;
	bool bGathered = false;
	bool bWasOn = true;
	FDelegateHandle HudHandle;

	UPROPERTY(Transient)
	FTakeHudKit Kit;

	UPROPERTY(Transient)
	TObjectPtr<USoundAttenuation> VoiceAttenuation;
};
