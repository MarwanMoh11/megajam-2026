// Step 5, characters and props. See CastMember.h.
#include "CastMember.h"

#include "AnimationRuntime.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Crc.h"
#include "TakeDirector.h"
#include "TakeShapes.h"
#include "TakeTypes.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/Package.h"

namespace CastKits
{
	/** One costume piece: a basic shape on a bone, placed in the body's frame (X forward, Y right, Z up, cm) relative
	 *  to the bone's reference-pose position (NAME_None: the actor's location, a standing member's feet or a seated
	 *  one's seat top; the piece then stands still on the actor's root), sized in cm. */
	struct FPiece
	{
		ETakeShape Shape;
		const TCHAR* Bone;
		FVector Loc;
		FRotator Rot;
		FVector Size;
		FLinearColor Colour;
	};

	FLinearColor Grey(float V) { return FLinearColor(V, V, V); }
	FLinearColor Scaled(const FLinearColor& C, float K, float Lo = 0.008f, float Hi = 0.40f)
	{
		return FLinearColor(FMath::Clamp(C.R * K, Lo, Hi), FMath::Clamp(C.G * K, Lo, Hi), FMath::Clamp(C.B * K, Lo, Hi));
	}

	// The tones (§15.10): pieces stay in the wearer's band; nothing above 0.40 but Julian; no hue.
	const FLinearColor Wood = Grey(0.05f);		// the bass, the piano, the drum shells
	const FLinearColor Metal = Grey(0.20f);		// pewter, never brass: gold is Julian's
	const FLinearColor Skin = Grey(0.28f);		// drum heads, the keys
	const FLinearColor BassWood = Grey(0.10f);	// the bass a step lighter than the suits, so its shape reads
	const FLinearColor Keys = Grey(0.40f);		// the piano's keys: the light line that says "piano" either side of him

	void Hair(TArray<FPiece>& Out, const FLinearColor& C) { Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(-3.5f, 0.f, 11.f), FRotator::ZeroRotator, FVector(19.f, 17.5f, 15.f), C}); }
	void Bun(TArray<FPiece>& Out, const FLinearColor& C) { Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(-11.f, 0.f, 9.f), FRotator::ZeroRotator, FVector(10.f, 10.f, 10.f), C}); }
	void BowTie(TArray<FPiece>& Out, const FLinearColor& C) { Out.Add({ETakeShape::Cube, TEXT("neck_01"), FVector(8.f, 0.f, -4.f), FRotator::ZeroRotator, FVector(3.f, 11.f, 4.5f), C}); }
	void ShirtFront(TArray<FPiece>& Out, const FLinearColor& C) { Out.Add({ETakeShape::Cube, TEXT("spine_05"), FVector(11.f, 0.f, -4.f), FRotator(-8.f, 0.f, 0.f), FVector(2.f, 12.f, 22.f), C}); }
	void Tails(TArray<FPiece>& Out, const FLinearColor& C) { Out.Add({ETakeShape::Cube, TEXT("pelvis"), FVector(-13.f, 0.f, -20.f), FRotator(6.f, 0.f, 0.f), FVector(2.5f, 28.f, 48.f), C}); }
	void TopHat(TArray<FPiece>& Out, const FLinearColor& C, float Crown)
	{
		Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(0.f, 0.f, 18.f), FRotator::ZeroRotator, FVector(32.f, 30.f, 1.5f), C});
		Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(0.f, 0.f, 18.f + Crown * 0.5f), FRotator::ZeroRotator, FVector(21.f, 20.f, Crown), C});
	}
	void Fedora(TArray<FPiece>& Out, const FLinearColor& C)
	{
		Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(0.f, 0.f, 16.5f), FRotator(-4.f, 0.f, 0.f), FVector(31.f, 29.f, 1.2f), C});
		Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(-0.5f, 0.f, 22.f), FRotator::ZeroRotator, FVector(21.f, 20.f, 11.f), C});
	}
	void PeakedCap(TArray<FPiece>& Out, const FLinearColor& C, float Crown)
	{
		Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(0.5f, 0.f, 15.f + Crown * 0.5f), FRotator::ZeroRotator, FVector(22.f, 21.f, Crown), C});
		Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(1.5f, 0.f, 16.f + Crown), FRotator(0.f, 0.f, 0.f), FVector(27.f, 26.f, 2.5f), C});
		Out.Add({ETakeShape::Cube, TEXT("head"), FVector(12.f, 0.f, 15.5f), FRotator(-18.f, 0.f, 0.f), FVector(8.f, 17.f, 1.2f), C});
	}
	void Epaulettes(TArray<FPiece>& Out, const FLinearColor& C)
	{
		Out.Add({ETakeShape::Cube, TEXT("clavicle_r"), FVector(0.f, 14.f, 4.f), FRotator::ZeroRotator, FVector(11.f, 11.f, 2.5f), C});
		Out.Add({ETakeShape::Cube, TEXT("clavicle_l"), FVector(0.f, -14.f, 4.f), FRotator::ZeroRotator, FVector(11.f, 11.f, 2.5f), C});
	}
	void Belt(TArray<FPiece>& Out, const FLinearColor& C) { Out.Add({ETakeShape::Cylinder, TEXT("pelvis"), FVector(1.f, 0.f, 3.f), FRotator::ZeroRotator, FVector(26.f, 37.f, 5.f), C}); }
	void Skirt(TArray<FPiece>& Out, const FLinearColor& C) { Out.Add({ETakeShape::Cone, TEXT("pelvis"), FVector(0.f, 0.f, -42.f), FRotator::ZeroRotator, FVector(64.f, 68.f, 100.f), C}); }
	void Headset(TArray<FPiece>& Out, const FLinearColor& C)
	{
		Out.Add({ETakeShape::Cube, TEXT("head"), FVector(0.f, 0.f, 19.f), FRotator::ZeroRotator, FVector(3.f, 19.f, 2.f), C});
		Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(0.f, 9.5f, 8.f), FRotator::ZeroRotator, FVector(6.f, 4.f, 7.f), C});
		Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(0.f, -9.5f, 8.f), FRotator::ZeroRotator, FVector(6.f, 4.f, 7.f), C});
	}

	void Build(ECastKit Kit, const FLinearColor& Tint, TArray<FPiece>& Out)
	{
		const FLinearColor Dark = Scaled(Tint, 0.25f);
		const FLinearColor Light = FLinearColor(FMath::Min(Tint.R + 0.12f, 0.40f), FMath::Min(Tint.G + 0.12f, 0.40f), FMath::Min(Tint.B + 0.12f, 0.40f));
		const FLinearColor Coat = Scaled(Tint, 0.8f);
		switch (Kit)
		{
		case ECastKit::GuestTux:
			Hair(Out, Dark); ShirtFront(Out, Light); BowTie(Out, Dark);
			break;
		case ECastKit::GuestTails:
			ShirtFront(Out, Light); BowTie(Out, Dark); Tails(Out, Coat); TopHat(Out, Dark, 20.f);
			break;
		case ECastKit::GuestGown:
			Hair(Out, Dark); Bun(Out, Dark); Skirt(Out, Tint);
			break;
		case ECastKit::GuestGownHat:
			Hair(Out, Dark); Skirt(Out, Tint);
			Out.Add({ETakeShape::Cylinder, TEXT("spine_05"), FVector(-1.f, 0.f, 4.f), FRotator::ZeroRotator, FVector(30.f, 44.f, 7.f), Light});	// the stole
			Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(1.f, 0.f, 18.f), FRotator(-6.f, 0.f, 0.f), FVector(18.f, 17.f, 6.f), Dark});		// the hat
			Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(1.f, 0.f, 15.5f), FRotator(-6.f, 0.f, 0.f), FVector(25.f, 24.f, 1.f), Dark});
			break;
		case ECastKit::Staff:
			Hair(Out, Dark); ShirtFront(Out, Light); BowTie(Out, Dark);
			Out.Add({ETakeShape::Cube, TEXT("spine_03"), FVector(10.5f, 0.f, 0.f), FRotator(-4.f, 0.f, 0.f), FVector(2.f, 26.f, 28.f), Dark});		// the waistcoat
			break;
		case ECastKit::Guard:
			PeakedCap(Out, Tint, 7.f); Epaulettes(Out, Tint); Belt(Out, Tint);
			break;
		case ECastKit::Doorman:
			PeakedCap(Out, Tint, 11.f); Epaulettes(Out, Tint);
			Out.Add({ETakeShape::Cylinder, TEXT("pelvis"), FVector(-1.f, 0.f, -24.f), FRotator::ZeroRotator, FVector(31.f, 40.f, 56.f), Tint});	// the greatcoat's skirts
			break;
		case ECastKit::Vance:
			Tails(Out, Tint); TopHat(Out, Tint, 26.f);
			Out.Add({ETakeShape::Cylinder, nullptr, FVector(10.f, 27.f, 46.f), FRotator(0.f, 0.f, -6.f), FVector(2.5f, 2.5f, 92.f), Tint});		// the cane
			Out.Add({ETakeShape::Sphere, nullptr, FVector(10.f, 32.f, 92.f), FRotator::ZeroRotator, FVector(5.f, 5.f, 5.f), Tint});
			break;
		case ECastKit::Bodyguard:
			Out.Add({ETakeShape::Sphere, TEXT("spine_05"), FVector(-1.f, 0.f, 3.f), FRotator::ZeroRotator, FVector(27.f, 54.f, 20.f), Tint});		// the shoulders
			Out.Add({ETakeShape::Cube, TEXT("head"), FVector(10.f, 0.f, 7.f), FRotator::ZeroRotator, FVector(2.f, 15.f, 3.5f), Grey(0.005f)});		// dark glasses
			break;
		case ECastKit::BandBass:
			Fedora(Out, Scaled(Tint, 1.5f)); BowTie(Out, Dark);
			// The upright bass: a waisted body (two flattened bouts), a fingerboard up a long neck, the scroll, the pin.
			Out.Add({ETakeShape::Sphere, nullptr, FVector(32.f, -8.f, 50.f), FRotator::ZeroRotator, FVector(20.f, 56.f, 72.f), BassWood});
			Out.Add({ETakeShape::Sphere, nullptr, FVector(32.f, -8.f, 100.f), FRotator::ZeroRotator, FVector(18.f, 40.f, 52.f), BassWood});
			Out.Add({ETakeShape::Cube, nullptr, FVector(43.f, -8.f, 115.f), FRotator::ZeroRotator, FVector(2.f, 6.f, 120.f), Wood});
			Out.Add({ETakeShape::Cylinder, nullptr, FVector(31.f, -8.f, 150.f), FRotator::ZeroRotator, FVector(6.f, 6.f, 72.f), BassWood});
			Out.Add({ETakeShape::Sphere, nullptr, FVector(31.f, -8.f, 190.f), FRotator::ZeroRotator, FVector(7.f, 7.f, 11.f), BassWood});
			Out.Add({ETakeShape::Cube, nullptr, FVector(43.f, -8.f, 62.f), FRotator::ZeroRotator, FVector(2.f, 16.f, 3.f), Skin});			// the bridge
			Out.Add({ETakeShape::Cylinder, nullptr, FVector(32.f, -8.f, 8.f), FRotator::ZeroRotator, FVector(2.f, 2.f, 16.f), Metal});
			break;
		case ECastKit::BandTrumpet:
			Fedora(Out, Scaled(Tint, 1.5f)); BowTie(Out, Dark);
			Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(26.f, 0.f, -6.f), FRotator(90.f, 0.f, 0.f), FVector(3.5f, 3.5f, 34.f), Metal});	// the lead pipe
			Out.Add({ETakeShape::Cube, TEXT("head"), FVector(22.f, 0.f, -2.f), FRotator::ZeroRotator, FVector(7.f, 3.f, 7.f), Metal});				// the valves
			Out.Add({ETakeShape::Cone, TEXT("head"), FVector(45.f, 0.f, -6.f), FRotator(90.f, 0.f, 0.f), FVector(14.f, 14.f, 14.f), Metal});		// the bell
			Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(52.f, 0.f, -6.f), FRotator::ZeroRotator, FVector(9.f, 9.f, 9.f), Dark});				// the mute
			break;
		case ECastKit::BandDrums:
			Fedora(Out, Scaled(Tint, 1.5f)); BowTie(Out, Dark);
			Out.Add({ETakeShape::Cylinder, nullptr, FVector(0.f, 0.f, -25.f), FRotator::ZeroRotator, FVector(34.f, 34.f, 50.f), Wood});			// the stool
			Out.Add({ETakeShape::Cylinder, nullptr, FVector(62.f, 0.f, -22.f), FRotator(90.f, 0.f, 0.f), FVector(56.f, 56.f, 36.f), Wood});		// the bass drum
			Out.Add({ETakeShape::Cylinder, nullptr, FVector(81.f, 0.f, -22.f), FRotator(90.f, 0.f, 0.f), FVector(52.f, 52.f, 1.f), Skin});		// its front head
			Out.Add({ETakeShape::Cylinder, nullptr, FVector(32.f, -20.f, 12.f), FRotator(0.f, 0.f, 8.f), FVector(36.f, 36.f, 14.f), Skin});		// the snare
			Out.Add({ETakeShape::Cylinder, nullptr, FVector(55.f, 12.f, 30.f), FRotator(20.f, 0.f, 0.f), FVector(30.f, 30.f, 22.f), Wood});		// the tom
			Out.Add({ETakeShape::Cylinder, nullptr, FVector(30.f, -45.f, -5.f), FRotator::ZeroRotator, FVector(2.5f, 2.5f, 90.f), Metal});		// the hi-hat
			Out.Add({ETakeShape::Cylinder, nullptr, FVector(30.f, -45.f, 40.f), FRotator::ZeroRotator, FVector(36.f, 36.f, 1.5f), Metal});
			Out.Add({ETakeShape::Cylinder, nullptr, FVector(30.f, -45.f, 42.5f), FRotator::ZeroRotator, FVector(36.f, 36.f, 1.5f), Metal});
			Out.Add({ETakeShape::Cylinder, nullptr, FVector(48.f, 42.f, 0.f), FRotator::ZeroRotator, FVector(2.5f, 2.5f, 110.f), Metal});		// the ride
			Out.Add({ETakeShape::Cylinder, nullptr, FVector(48.f, 42.f, 55.f), FRotator(0.f, 0.f, 14.f), FVector(50.f, 50.f, 1.5f), Metal});
			break;
		case ECastKit::BandPiano:
			Fedora(Out, Scaled(Tint, 1.5f)); BowTie(Out, Dark);
			Out.Add({ETakeShape::Cube, nullptr, FVector(0.f, 0.f, -25.f), FRotator::ZeroRotator, FVector(36.f, 70.f, 50.f), Wood});				// the bench
			Out.Add({ETakeShape::Cube, nullptr, FVector(64.f, 0.f, 15.f), FRotator::ZeroRotator, FVector(52.f, 140.f, 130.f), Wood});				// the case
			Out.Add({ETakeShape::Cube, nullptr, FVector(64.f, 0.f, 81.f), FRotator::ZeroRotator, FVector(58.f, 146.f, 3.f), Wood});				// the lid
			Out.Add({ETakeShape::Cube, nullptr, FVector(30.f, 0.f, 22.f), FRotator::ZeroRotator, FVector(24.f, 124.f, 6.f), Keys});				// the keys
			break;
		case ECastKit::BandSinger:
			Hair(Out, Dark); Bun(Out, Dark); Skirt(Out, Tint);
			Out.Add({ETakeShape::Cylinder, nullptr, FVector(30.f, 0.f, 1.f), FRotator::ZeroRotator, FVector(26.f, 26.f, 2.f), Metal});			// the stand's base
			Out.Add({ETakeShape::Cylinder, nullptr, FVector(30.f, 0.f, 72.f), FRotator::ZeroRotator, FVector(2.5f, 2.5f, 142.f), Metal});
			Out.Add({ETakeShape::Sphere, nullptr, FVector(27.f, 0.f, 147.f), FRotator(-30.f, 0.f, 0.f), FVector(6.f, 6.f, 10.f), Dark});		// the microphone
			break;
		case ECastKit::Julian:
			BowTie(Out, Grey(0.02f));	// no hair: he stays the white figure
			break;
		case ECastKit::Maya:
			Headset(Out, Grey(0.06f)); Belt(Out, Grey(0.10f));
			break;
		case ECastKit::Vex:
			Headset(Out, Grey(0.06f));
			Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(-1.f, 0.f, 13.f), FRotator::ZeroRotator, FVector(21.f, 20.f, 17.f), Grey(0.08f)});	// the beanie
			break;
		default:
			break;
		}
	}

	/** Shared colour instances of BasicShapeMaterial, so equal pieces draw together. Runtime and editor pieces are
	 *  transient, so the instances live in the transient package. */
	UMaterialInstanceDynamic* SharedColour(const FLinearColor& Colour)
	{
		static TMap<uint32, TWeakObjectPtr<UMaterialInstanceDynamic>> Cache;
		const FColor Key = Colour.ToFColor(false);
		const uint32 Hash = Key.DWColor();
		if (TWeakObjectPtr<UMaterialInstanceDynamic>* Found = Cache.Find(Hash))
		{
			if (Found->IsValid())
			{
				return Found->Get();
			}
		}
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(FTakeShapes::GetMaterial(), GetTransientPackage());
		if (MID)
		{
			MID->SetVectorParameterValue(TEXT("Color"), Colour);
			Cache.Add(Hash, MID);
		}
		return MID;
	}

	/** A stable number in [0, 1) from a seed and a salt (a hash finaliser, so the draws are independent). */
	float Draw(uint32 Seed, uint32 Salt)
	{
		uint32 X = Seed + Salt * 0x9E3779B9u;
		X ^= X >> 16; X *= 0x85EBCA6Bu; X ^= X >> 13; X *= 0xC2B2AE35u; X ^= X >> 16;
		return float(X >> 8) / 16777216.f;
	}

	/** PieceBones' mark for a piece on the actor's root (it never moves to the poseable copy). */
	const FName RootPiece(TEXT("__Root"));

	bool IsQuinnKit(ECastKit Kit)
	{
		return Kit == ECastKit::GuestGown || Kit == ECastKit::GuestGownHat || Kit == ECastKit::BandSinger;
	}
}

ACastMember::ACastMember()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;	// after the director advanced the clock and fired its triggers

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));	// the mannequin faces +Y in its own space
	Body->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Body->SetGenerateOverlapEvents(false);
	Body->SetCanEverAffectNavigation(false);
	Body->bReceivesDecals = false;
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	static ConstructorHelpers::FObjectFinder<UAnimationAsset> Idle(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
	static ConstructorHelpers::FObjectFinder<UAnimationAsset> Walk(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/BS_Idle_Walk_Run.BS_Idle_Walk_Run"));
	PoseAnim = Idle.Object;
	SeatedAnim = Idle.Object;
	SeatedTime = 0.f;
	WalkBlendSpace = Walk.Object;
}

USkeletalMeshComponent* ACastMember::GetWearer() const
{
	if (bDressParent)
	{
		const ACharacter* Character = Cast<ACharacter>(GetAttachParentActor());
		return Character ? Character->GetMesh() : nullptr;
	}
	return Body;
}

void ACastMember::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UWorld* World = GetWorld();
	if (!World || World->IsGameWorld())
	{
		return;
	}
	// The editor shows the costume on our own mannequin; a character's own mesh is dressed only in play.
	bDressParent = Cast<ACharacter>(GetAttachParentActor()) != nullptr;
	bWalker = !bDressParent && GetAttachParentActor() != nullptr;
	SetupBody();
	if (!bDressParent)
	{
		BuildPieces(true);
	}
}

void ACastMember::BeginPlay()
{
	Super::BeginPlay();
	bDressParent = Cast<ACharacter>(GetAttachParentActor()) != nullptr;
	bWalker = !bDressParent && GetAttachParentActor() != nullptr;
	SetupBody();
	BuildPieces(true);
	if (bWalker)
	{
		HideParentStandIn();
		LastParentLocation = GetAttachParentActor()->GetActorLocation();
	}
	bBowFiredLast = false;
	BowTime = -1.f;
}

void ACastMember::SetupBody()
{
	if (bDressParent)
	{
		Body->SetSkeletalMeshAsset(nullptr);
		Body->SetVisibility(false);
		Body->SetComponentTickEnabled(false);
		return;
	}
	static const TCHAR* Manny = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple");
	static const TCHAR* Quinn = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple");
	const bool bUseQuinn = bQuinn || CastKits::IsQuinnKit(Kit);
	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, bUseQuinn ? Quinn : Manny);
	if (Body->GetSkeletalMeshAsset() != Mesh)
	{
		Body->SetSkeletalMeshAsset(Mesh);
	}
	Body->SetVisibility(true);
	Body->SetRelativeScale3D(FVector(BodyScale));
	Body->SetRelativeLocation(FVector(0.f, 0.f, bSeated ? -SeatedDrop : 0.f));
	Body->SetCastShadow(bBodyShadow);
	// The body's draw distance (set per member by tools/build_cast.py; 0 = always) goes to its pieces too.
	if (Body->LDMaxDrawDistance > 0.f)
	{
		Body->SetCachedMaxDrawDistance(Body->LDMaxDrawDistance);
	}

	// The tint, as the characters do it (M_Mannequin's "Paint Tint").
	for (int32 Slot = 0; Slot < Body->GetNumMaterials(); ++Slot)
	{
		if (Body->GetMaterial(Slot))
		{
			if (UMaterialInstanceDynamic* MID = Body->CreateAndSetMaterialInstanceDynamic(Slot))
			{
				MID->SetVectorParameterValue(TEXT("Paint Tint"), Tint);
				// No glowing cyan logo on the chest: it would be the only hue on a guest at the alarm.
				MID->SetVectorParameterValue(TEXT("LogoTint"), Tint * 0.5f);
				MID->SetScalarParameterValue(TEXT("EmissivePower"), 0.f);
			}
		}
	}

	if (bWalker && WalkBlendSpace)
	{
		Body->OverrideAnimationData(WalkBlendSpace, true, true, 0.f, 1.f);
		Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		Body->SetComponentTickEnabled(true);
	}
	else
	{
		UAnimationAsset* Anim = bSeated ? SeatedAnim.Get() : PoseAnim.Get();
		const float Time = bSeated ? SeatedTime : PoseTime;
		const UAnimSequenceBase* Sequence = Cast<UAnimSequenceBase>(Anim);
		bIdling = bIdle && !bSeated && Sequence && Sequence->GetPlayLength() > 0.f;
		if (bIdling)
		{
			// Like an NPC in a stealth game: the idle looped, each from its own time and at its own rate (drawn once from
			// the name, so the same in every run), so no two breathe together. Never on the band's beat.
			const uint32 Seed = FCrc::StrCrc32(*GetFName().ToString());
			const float Start = CastKits::Draw(Seed, 1) * Sequence->GetPlayLength();
			const float Rate = 1.f + (2.f * CastKits::Draw(Seed, 2) - 1.f) * IdleRateSpread;
			Body->OverrideAnimationData(Anim, true, true, Start, Rate);
			Body->bEnableUpdateRateOptimizations = true;	// far ones update less often
			Body->SetComponentTickEnabled(true);
		}
		else if (Anim)
		{
			Body->OverrideAnimationData(Anim, false, false, Time, 0.f);
		}
	}
	FramesAlive = 0;
}

void ACastMember::BuildPieces(bool bTransient)
{
	for (UStaticMeshComponent* Old : Pieces)
	{
		if (IsValid(Old))
		{
			Old->DestroyComponent();
		}
	}
	Pieces.Reset();
	PieceBones.Reset();

	USkeletalMeshComponent* Wearer = GetWearer();
	if (!Wearer || !Wearer->GetSkinnedAsset())
	{
		return;
	}
	const FReferenceSkeleton& Ref = Wearer->GetSkinnedAsset()->GetRefSkeleton();

	TArray<CastKits::FPiece> Defs;
	CastKits::Build(Kit, Tint, Defs);

	const FQuat MeshFromBody(FRotator(0.f, 90.f, 0.f));		// body X (forward) -> mesh +Y, body Y (right) -> mesh -X
	// A piece on no bone stands on the floor (an instrument, a stand, a stool, the cane): on our own body it goes on the
	// actor's root, so the seated pose never moves it. It is placed from the actor's own location (a seated
	// member's seat top, a standing one's feet), not the seated body's origin SeatedDrop below the seat.
	FTransform BodyRest = Wearer->GetRelativeTransform();
	BodyRest.SetLocation(FVector::ZeroVector);
	for (const CastKits::FPiece& Def : Defs)
	{
		const FName Bone = Def.Bone ? FName(Def.Bone) : NAME_None;
		FTransform BoneCS = FTransform::Identity;
		if (Bone != NAME_None)
		{
			const int32 Index = Ref.FindBoneIndex(Bone);
			if (Index == INDEX_NONE)
			{
				continue;
			}
			BoneCS = FAnimationRuntime::GetComponentSpaceTransformRefPose(Ref, Index);
		}
		const FVector Offset(-Def.Loc.Y, Def.Loc.X, Def.Loc.Z);
		const FTransform PieceCS(MeshFromBody * FQuat(Def.Rot), (Bone != NAME_None ? BoneCS.GetLocation() : FVector::ZeroVector) + Offset, Def.Size / 100.f);
		const FTransform Rel = (Bone != NAME_None) ? PieceCS.GetRelativeTransform(BoneCS) : PieceCS;

		const EObjectFlags Flags = bTransient ? (RF_Transient | RF_DuplicateTransient | RF_TextExportTransient) : RF_NoFlags;
		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this, NAME_None, Flags);
		Comp->SetStaticMesh(FTakeShapes::GetMesh(Def.Shape));
		Comp->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Comp->SetGenerateOverlapEvents(false);
		Comp->SetCanEverAffectNavigation(false);
		Comp->SetCastShadow(false);
		Comp->bReceivesDecals = false;
		Comp->LightingChannels = Wearer->LightingChannels;
		if (Wearer->LDMaxDrawDistance > 0.f)
		{
			Comp->LDMaxDrawDistance = Wearer->LDMaxDrawDistance;
			Comp->SetCachedMaxDrawDistance(Wearer->LDMaxDrawDistance);
		}
		const bool bOnRoot = Bone == NAME_None && !bDressParent;
		Comp->SetupAttachment(bOnRoot ? static_cast<USceneComponent*>(Root.Get()) : static_cast<USceneComponent*>(Wearer), Bone);
		Comp->SetRelativeTransform(bOnRoot ? Rel * BodyRest : Rel);
		Comp->SetMaterial(0, CastKits::SharedColour(Def.Colour));
		Comp->RegisterComponent();
		Pieces.Add(Comp);
		PieceBones.Add(bOnRoot ? CastKits::RootPiece : Bone);
	}
}

void ACastMember::HideParentStandIn()
{
	AActor* Parent = GetAttachParentActor();
	if (!Parent)
	{
		return;
	}
	TArray<UStaticMeshComponent*> Comps;
	Parent->GetComponents(Comps);
	for (UStaticMeshComponent* Comp : Comps)
	{
		const FString Name = Comp->GetName();
		if (Name == TEXT("Body") || Name == TEXT("Cap"))
		{
			Comp->SetHiddenInGame(true);	// collision and sight blocking stay as they were
		}
	}
}

void ACastMember::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	++FramesAlive;
	if (bDressParent)
	{
		TickBow(DeltaSeconds);
		return;
	}
	if (bWalker)
	{
		TickWalk(DeltaSeconds);
		return;
	}
	// A standing member idles on its own; a frozen one needs no animation once the pose is on the bones.
	if (FramesAlive == 3 && !bIdling)
	{
		Freeze();
	}
	// With its tick off a frozen body no longer picks its LOD: do it here, refreshing the bones only on a change.
	if (FramesAlive > 3 && !bIdling)
	{
		USkinnedMeshComponent* Shown = bPosed ? static_cast<USkinnedMeshComponent*>(PoseMesh.Get()) : static_cast<USkinnedMeshComponent*>(Body.Get());
		if (Shown && Shown->UpdateLODStatus())
		{
			Shown->RefreshBoneTransforms();
		}
	}
}

void ACastMember::Freeze()
{
	if (bSeated)
	{
		MakePosed();
	}
	Body->SetComponentTickEnabled(false);
}

void ACastMember::TickWalk(float DeltaSeconds)
{
	const AActor* Parent = GetAttachParentActor();
	if (!Parent || DeltaSeconds <= 0.f)
	{
		return;
	}
	// The guards who walk in at the alarm are hidden until they arrive: so is their body.
	if (IsHidden() != Parent->IsHidden())
	{
		SetActorHiddenInGame(Parent->IsHidden());
	}
	const FVector Now = Parent->GetActorLocation();
	const float Speed = FVector::Dist2D(Now, LastParentLocation) / DeltaSeconds;
	LastParentLocation = Now;
	WalkSpeedSmoothed = FMath::FInterpTo(WalkSpeedSmoothed, FMath::Min(Speed, 600.f), DeltaSeconds, 12.f);
	if (UAnimSingleNodeInstance* Inst = Body->GetSingleNodeInstance())
	{
		Inst->SetBlendSpacePosition(FVector(0.f, WalkSpeedSmoothed < 8.f ? 0.f : WalkSpeedSmoothed, 0.f));
	}
}

void ACastMember::BowNow()
{
	USkeletalMeshComponent* Wearer = GetWearer();
	if (!Wearer || !Wearer->GetSkinnedAsset())
	{
		return;
	}
	EnsurePoseMesh(Wearer);

	Wearer->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	BowTime = 0.f;
	BowCancelAlpha = 1.f;
	ApplyBowPose(0.f);
	ReattachPieces(PoseMesh);
	PoseMesh->SetVisibility(true);
	Wearer->SetVisibility(false);
	TAKE_LOG(TEXT("CAST BOW %s"), *GetActorNameOrLabel());
}

void ACastMember::ApplyBowPose(float Alpha)
{
	USkeletalMeshComponent* Wearer = GetWearer();
	if (!PoseMesh || !Wearer)
	{
		return;
	}
	PoseMesh->CopyPoseFromSkeletalComponent(Wearer);
	// Mesh space: forward is +Y, so a turn about -X tips the top of the spine forward.
	const FVector Axis(-1.f, 0.f, 0.f);
	struct FBend { const TCHAR* Bone; float Degrees; };
	const FBend Bends[] = {
		{TEXT("spine_01"), BowDegrees * 0.30f},
		{TEXT("spine_02"), BowDegrees * 0.25f},
		{TEXT("spine_03"), BowDegrees * 0.25f},
		{TEXT("spine_04"), BowDegrees * 0.20f},
		{TEXT("head"), BowHeadDegrees},
	};
	for (const FBend& Bend : Bends)
	{
		BendBone(FName(Bend.Bone), Axis, Bend.Degrees * Alpha);
	}
	PoseMesh->RefreshBoneTransforms();
}

void ACastMember::EndBow()
{
	USkeletalMeshComponent* Wearer = GetWearer();
	BowTime = -1.f;
	if (!Wearer)
	{
		return;
	}
	ReattachPieces(Wearer);
	Wearer->SetVisibility(true);
	Wearer->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
	if (PoseMesh)
	{
		PoseMesh->SetVisibility(false);
	}
	TAKE_LOG(TEXT("CAST BOW END %s"), *GetActorNameOrLabel());
}

void ACastMember::TickBow(float DeltaSeconds)
{
	const ATakeDirector* Director = ATakeDirector::Get(this);
	// Only Julian bows (The Bow is his deleted scene); Maya and Vex wear their kit and nothing more.
	const bool bFired = Kit == ECastKit::Julian && Director && BowTrigger != NAME_None && Director->HasFired(BowTrigger);
	if (bFired && !bBowFiredLast)
	{
		BowNow();
	}
	bBowFiredLast = bFired;
	if (BowTime < 0.f)
	{
		return;
	}
	BowTime += DeltaSeconds;
	const AActor* Parent = GetAttachParentActor();
	if (Parent && Parent->GetVelocity().Size2D() > BowCancelSpeed)
	{
		BowCancelAlpha = FMath::Max(0.f, BowCancelAlpha - DeltaSeconds / 0.15f);
	}
	const float T = BowTime / FMath::Max(BowSeconds, 0.1f);
	if (T >= 1.f || BowCancelAlpha <= 0.f)
	{
		EndBow();
		return;
	}
	const float Down = FMath::SmoothStep(0.f, 0.33f, T);
	const float Up = 1.f - FMath::SmoothStep(0.55f, 1.f, T);
	ApplyBowPose(FMath::Min(Down, Up) * BowCancelAlpha);
}

void ACastMember::EnsurePoseMesh(USkeletalMeshComponent* Source)
{
	if (!Source || !Source->GetSkinnedAsset())
	{
		return;
	}
	if (!PoseMesh)
	{
		PoseMesh = NewObject<UPoseableMeshComponent>(this, TEXT("PoseMesh"), RF_Transient);
		PoseMesh->SetSkinnedAssetAndUpdate(Source->GetSkinnedAsset());
		PoseMesh->SetupAttachment(Source->GetAttachParent());
		PoseMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		PoseMesh->SetCanEverAffectNavigation(false);
		PoseMesh->bReceivesDecals = false;
		PoseMesh->RegisterComponent();
		// Its bones change only when we bend them (and we refresh them then): without this it refreshed every
		// visible copy every frame (about 1 ms for the 44 seated guests).
		PoseMesh->SetComponentTickEnabled(false);
	}
	PoseMesh->SetRelativeTransform(Source->GetRelativeTransform());
	for (int32 Slot = 0; Slot < Source->GetNumMaterials(); ++Slot)
	{
		PoseMesh->SetMaterial(Slot, Source->GetMaterial(Slot));
	}
	PoseMesh->LightingChannels = Source->LightingChannels;
	if (Source->LDMaxDrawDistance > 0.f)
	{
		PoseMesh->LDMaxDrawDistance = Source->LDMaxDrawDistance;
		PoseMesh->SetCachedMaxDrawDistance(Source->LDMaxDrawDistance);
	}
	PoseMesh->SetCastShadow(Source->CastShadow);
	PoseMesh->MarkRenderStateDirty();
}

void ACastMember::BendBone(FName Bone, const FVector& Axis, float Degrees)
{
	if (!PoseMesh || PoseMesh->GetBoneIndex(Bone) == INDEX_NONE)
	{
		return;
	}
	FTransform CS = PoseMesh->GetBoneTransformByName(Bone, EBoneSpaces::ComponentSpace);
	CS.SetRotation(FQuat(Axis, FMath::DegreesToRadians(Degrees)) * CS.GetRotation());
	PoseMesh->SetBoneTransformByName(Bone, CS, EBoneSpaces::ComponentSpace);
}

void ACastMember::ReattachPieces(USceneComponent* Target)
{
	if (!Target)
	{
		return;
	}
	for (int32 i = 0; i < Pieces.Num(); ++i)
	{
		if (Pieces[i] && PieceBones[i] != CastKits::RootPiece)
		{
			Pieces[i]->AttachToComponent(Target, FAttachmentTransformRules::KeepRelativeTransform, PieceBones[i]);
		}
	}
}

void ACastMember::MakePosed()
{
	EnsurePoseMesh(Body);
	if (!PoseMesh)
	{
		return;
	}
	PoseMesh->CopyPoseFromSkeletalComponent(Body);
	if (bSeated)
	{
		// Mesh space (forward +Y): a turn about +X swings a hanging thigh forward; the knee turns the shin back down.
		const FVector Swing(1.f, 0.f, 0.f);
		BendBone(TEXT("thigh_l"), Swing, SeatedHipDegrees);
		BendBone(TEXT("thigh_r"), Swing, SeatedHipDegrees);
		BendBone(TEXT("calf_l"), Swing, -SeatedKneeDegrees);
		BendBone(TEXT("calf_r"), Swing, -SeatedKneeDegrees);
	}
	PoseMesh->RefreshBoneTransforms();
	ReattachPieces(PoseMesh);
	PoseMesh->SetVisibility(true);
	Body->SetVisibility(false);
	bPosed = true;
}
