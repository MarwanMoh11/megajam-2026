// Step 5, characters and props; QA pass (Nov 14), package cast: acting and variety. See CastMember.h.
#include "CastMember.h"

#include "AnimationRuntime.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimationAsset.h"
#include "Animation/Skeleton.h"
#include "BarClockComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Crc.h"
#include "Misc/PackageName.h"
#include "TakeCharacter.h"
#include "TakeDirector.h"
#include "TakeShapes.h"
#include "TakeTypes.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/Package.h"
#include "Watcher.h"

static TAutoConsoleVariable<int32> CVarCastActing(
	TEXT("Reel.Cast.Acting"), 1,
	TEXT("1: the cast acts (looks, gestures, conversation, applause, the band playing). 0: step 5's still cast."),
	ECVF_Default);

namespace CastKits
{
	enum class EKind : uint8 { Costume, Floor, Held };

	/** One piece: a basic shape on a bone, placed in the body's frame (X forward, Y right, Z up, cm) relative to the
	 *  bone's reference-pose position (no bone: on the actor's root, a standing member's feet or a seated one's seat
	 *  top; it never moves). Held: on a hand, in the hand's frame (X along the knuckles, Y toward the thumb, Z out of
	 *  the right palm), its Z along the knuckles (Axis 0) or the thumb (Axis 1). */
	struct FPiece
	{
		ETakeShape Shape;
		const TCHAR* Bone;
		FVector Loc;
		FRotator Rot;
		FVector Size;
		FLinearColor Colour;
		EKind Kind = EKind::Costume;
		uint8 Axis = 1;
	};

	FLinearColor Grey(float V) { return FLinearColor(V, V, V); }
	FLinearColor Lin(float R, float G, float B) { return FLinearColor(R, G, B); }
	FLinearColor Scaled(const FLinearColor& C, float K, float Lo = 0.008f, float Hi = 0.40f)
	{
		return FLinearColor(FMath::Clamp(C.R * K, Lo, Hi), FMath::Clamp(C.G * K, Lo, Hi), FMath::Clamp(C.B * K, Lo, Hi));
	}

	/** A stable number in [0, 1) from a seed and a salt (a hash finaliser, so the draws are independent). */
	float Draw(uint32 Seed, uint32 Salt)
	{
		uint32 X = Seed + Salt * 0x9E3779B9u;
		X ^= X >> 16; X *= 0x85EBCA6Bu; X ^= X >> 13; X *= 0xC2B2AE35u; X ^= X >> 16;
		return float(X >> 8) / 16777216.f;
	}
	template <int32 N>
	const FLinearColor& Pick(const FLinearColor (&Colours)[N], uint32 Seed, uint32 Salt)
	{
		return Colours[FMath::Min(int32(Draw(Seed, Salt) * N), N - 1)];
	}

	// The tones. Faces and shirts stay well under Julian's white; a muted period palette for the accents.
	const FLinearColor Wood = Grey(0.05f);
	const FLinearColor Metal = Lin(0.20f, 0.17f, 0.11f);
	const FLinearColor Skin = Grey(0.28f);
	const FLinearColor BassWood = Lin(0.12f, 0.06f, 0.03f);
	const FLinearColor Keys = Grey(0.40f);
	const FLinearColor Pearl = Lin(0.42f, 0.40f, 0.34f);
	const FLinearColor GlassTone = Lin(0.34f, 0.32f, 0.24f);
	const FLinearColor Skins[] = {
		Lin(0.55f, 0.36f, 0.27f), Lin(0.50f, 0.30f, 0.24f), Lin(0.45f, 0.30f, 0.20f), Lin(0.36f, 0.22f, 0.14f),
		Lin(0.28f, 0.16f, 0.09f), Lin(0.18f, 0.10f, 0.06f), Lin(0.10f, 0.055f, 0.035f)};
	const FLinearColor Hairs[] = {
		Lin(0.012f, 0.010f, 0.009f), Lin(0.04f, 0.025f, 0.015f), Lin(0.08f, 0.045f, 0.025f), Lin(0.14f, 0.05f, 0.02f),
		Lin(0.33f, 0.24f, 0.11f), Lin(0.22f, 0.21f, 0.20f), Lin(0.40f, 0.39f, 0.37f), Lin(0.03f, 0.02f, 0.015f)};
	const FLinearColor Accents[] = {
		Lin(0.010f, 0.010f, 0.012f), Lin(0.14f, 0.018f, 0.025f), Lin(0.015f, 0.07f, 0.035f), Lin(0.012f, 0.018f, 0.06f),
		Lin(0.38f, 0.34f, 0.25f), Lin(0.30f, 0.20f, 0.06f), Lin(0.09f, 0.025f, 0.07f), Lin(0.32f, 0.16f, 0.16f)};

	bool IsQuinnKit(ECastKit Kit)
	{
		return Kit == ECastKit::GuestGown || Kit == ECastKit::GuestGownHat || Kit == ECastKit::BandSinger;
	}
	bool IsBandKit(ECastKit Kit)
	{
		return Kit == ECastKit::BandBass || Kit == ECastKit::BandTrumpet || Kit == ECastKit::BandDrums
			|| Kit == ECastKit::BandPiano || Kit == ECastKit::BandSinger;
	}

	/** Who this person is beyond the kit: drawn once from the seed, so the same in every run. */
	struct FLook
	{
		FLinearColor Skin, Hair, Shirt, Accent;
		int32 HairStyle = 0;	// 0 short, 1 slicked, 2 balding, 3 bun, 4 waves, 5 victory rolls, 6 none
		bool bShell = true;
		bool bGlasses = false, bMoustache = false, bWideMoustache = false, bPocket = false, bFlower = false;
		bool bPearls = false, bStole = false, bHat = false, bCummerbund = false, bWhiteTie = false, bHairFlower = false;
		float SkirtLength = 1.f;
		bool bSeated = false, bInstrumentProps = false, bGlass = false;
	};

	FLook MakeLook(uint32 Seed, ECastKit Kit, bool bVary, bool bShell)
	{
		FLook L;
		const bool bWoman = IsQuinnKit(Kit);
		if (!bVary)
		{
			L.bShell = false;
			L.HairStyle = bWoman ? 3 : 0;
			return L;
		}
		L.bShell = bShell;
		L.Skin = Pick(Skins, Seed, 11);
		L.Hair = Pick(Hairs, Seed, 12);
		L.Shirt = Draw(Seed, 13) < 0.6f ? Lin(0.40f, 0.39f, 0.36f) : Lin(0.37f, 0.34f, 0.27f);
		L.Accent = Pick(Accents, Seed, 14);
		const float H = Draw(Seed, 15);
		L.HairStyle = bWoman ? (H < 0.34f ? 3 : H < 0.7f ? 4 : 5) : (H < 0.45f ? 0 : H < 0.8f ? 1 : 2);
		L.bGlasses = Draw(Seed, 16) < (bWoman ? 0.12f : 0.25f);
		L.bMoustache = !bWoman && Draw(Seed, 17) < 0.35f;
		L.bWideMoustache = Draw(Seed, 18) < 0.3f;
		L.bPocket = !bWoman && Draw(Seed, 19) < 0.55f;
		L.bFlower = !bWoman && Draw(Seed, 20) < 0.25f;
		L.bPearls = bWoman && Draw(Seed, 21) < 0.5f;
		L.bStole = bWoman && Draw(Seed, 22) < 0.35f;
		L.bHairFlower = bWoman && Draw(Seed, 23) < 0.3f;
		L.bHat = Draw(Seed, 24) < 0.45f;
		L.bCummerbund = Draw(Seed, 25) < 0.3f;
		L.bWhiteTie = Draw(Seed, 26) < 0.5f;
		L.SkirtLength = Draw(Seed, 27) < 0.7f ? 1.f : 0.82f;
		return L;
	}

	// --- pieces (the head sizes assume the skin shell: hair and hats sit a little proud of the mannequin's head) -------

	void Shell(TArray<FPiece>& Out, const FLook& L)
	{
		if (L.bShell)
		{
			Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(1.f, 0.f, 8.f), FRotator::ZeroRotator, FVector(21.5f, 18.f, 23.f), L.Skin});
		}
	}
	void Hair(TArray<FPiece>& Out, int32 Style, const FLinearColor& C)
	{
		const FRotator Z = FRotator::ZeroRotator;
		switch (Style)
		{
		case 1: Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(-1.f, 0.f, 12.2f), Z, FVector(22.6f, 19.3f, 16.f), C}); break;
		case 2: Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(-5.f, 0.f, 8.5f), Z, FVector(17.f, 19.4f, 15.f), C}); break;
		case 3:
			Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(-1.5f, 0.f, 11.f), Z, FVector(22.5f, 19.2f, 19.f), C});
			Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(-12.5f, 0.f, 12.f), Z, FVector(10.f, 10.f, 10.f), C});
			break;
		case 4:
			Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(-2.5f, 0.f, 9.5f), Z, FVector(24.f, 21.f, 22.5f), C});
			Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(-8.f, 0.f, 1.f), Z, FVector(15.f, 20.f, 12.f), C});
			break;
		case 5:
			Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(-1.5f, 0.f, 11.f), Z, FVector(22.5f, 19.2f, 19.f), C});
			Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(3.f, 5.f, 18.5f), Z, FVector(10.f, 7.f, 7.f), C});
			Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(3.f, -5.f, 18.5f), Z, FVector(10.f, 7.f, 7.f), C});
			break;
		case 6: break;
		default: Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(-1.5f, 0.f, 11.f), Z, FVector(22.5f, 19.2f, 19.f), C}); break;
		}
	}
	void Face(TArray<FPiece>& Out, const FLook& L)
	{
		if (L.bGlasses)
		{
			Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(11.8f, 3.4f, 8.6f), FRotator(90.f, 0.f, 0.f), FVector(3.4f, 3.4f, 0.5f), Grey(0.02f)});
			Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(11.8f, -3.4f, 8.6f), FRotator(90.f, 0.f, 0.f), FVector(3.4f, 3.4f, 0.5f), Grey(0.02f)});
		}
		if (L.bMoustache)
		{
			Out.Add({ETakeShape::Cube, TEXT("head"), FVector(11.f, 0.f, 3.2f), FRotator::ZeroRotator, FVector(1.2f, L.bWideMoustache ? 8.f : 5.2f, 1.1f), L.Hair});
		}
	}
	void BowTie(TArray<FPiece>& Out, const FLinearColor& C) { Out.Add({ETakeShape::Cube, TEXT("neck_01"), FVector(8.f, 0.f, -4.f), FRotator::ZeroRotator, FVector(3.f, 11.f, 4.5f), C}); }
	void ShirtFront(TArray<FPiece>& Out, const FLinearColor& C) { Out.Add({ETakeShape::Cube, TEXT("spine_05"), FVector(11.f, 0.f, -4.f), FRotator(-8.f, 0.f, 0.f), FVector(2.f, 12.f, 22.f), C}); }
	void Tails(TArray<FPiece>& Out, const FLinearColor& C) { Out.Add({ETakeShape::Cube, TEXT("pelvis"), FVector(-13.f, 0.f, -20.f), FRotator(6.f, 0.f, 0.f), FVector(2.5f, 28.f, 48.f), C}); }
	void TopHat(TArray<FPiece>& Out, const FLinearColor& C, float Crown)
	{
		Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(0.f, 0.f, 20.5f), FRotator::ZeroRotator, FVector(33.f, 31.f, 1.5f), C});
		Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(0.f, 0.f, 20.5f + Crown * 0.5f), FRotator::ZeroRotator, FVector(22.f, 21.f, Crown), C});
	}
	void Fedora(TArray<FPiece>& Out, const FLinearColor& C)
	{
		Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(0.f, 0.f, 19.5f), FRotator(-4.f, 0.f, 0.f), FVector(32.f, 30.f, 1.2f), C});
		Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(-0.5f, 0.f, 25.f), FRotator::ZeroRotator, FVector(22.f, 21.f, 11.f), C});
	}
	void LadyHat(TArray<FPiece>& Out, const FLinearColor& C, const FLinearColor& Feather)
	{
		Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(1.f, 0.f, 21.f), FRotator(-8.f, 0.f, 0.f), FVector(17.f, 16.f, 6.f), C});
		Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(1.f, 0.f, 18.5f), FRotator(-8.f, 0.f, 0.f), FVector(25.f, 24.f, 1.f), C});
		Out.Add({ETakeShape::Cube, TEXT("head"), FVector(-5.f, 6.f, 26.f), FRotator(25.f, 0.f, 0.f), FVector(1.f, 2.f, 12.f), Feather});
	}
	void PeakedCap(TArray<FPiece>& Out, const FLinearColor& C, float Crown)
	{
		Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(0.5f, 0.f, 17.f + Crown * 0.5f), FRotator::ZeroRotator, FVector(23.f, 22.f, Crown), C});
		Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(1.5f, 0.f, 18.f + Crown), FRotator::ZeroRotator, FVector(28.f, 27.f, 2.5f), C});
		Out.Add({ETakeShape::Cube, TEXT("head"), FVector(12.5f, 0.f, 17.5f), FRotator(-18.f, 0.f, 0.f), FVector(8.f, 17.f, 1.2f), C});
	}
	void Epaulettes(TArray<FPiece>& Out, const FLinearColor& C)
	{
		Out.Add({ETakeShape::Cube, TEXT("clavicle_r"), FVector(0.f, 14.f, 4.f), FRotator::ZeroRotator, FVector(11.f, 11.f, 2.5f), C});
		Out.Add({ETakeShape::Cube, TEXT("clavicle_l"), FVector(0.f, -14.f, 4.f), FRotator::ZeroRotator, FVector(11.f, 11.f, 2.5f), C});
	}
	void Belt(TArray<FPiece>& Out, const FLinearColor& C) { Out.Add({ETakeShape::Cylinder, TEXT("pelvis"), FVector(1.f, 0.f, 3.f), FRotator::ZeroRotator, FVector(26.f, 37.f, 5.f), C}); }
	void Headset(TArray<FPiece>& Out, const FLinearColor& C)
	{
		Out.Add({ETakeShape::Cube, TEXT("head"), FVector(0.f, 0.f, 19.f), FRotator::ZeroRotator, FVector(3.f, 19.f, 2.f), C});
		Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(0.f, 9.5f, 8.f), FRotator::ZeroRotator, FVector(6.f, 4.f, 7.f), C});
		Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(0.f, -9.5f, 8.f), FRotator::ZeroRotator, FVector(6.f, 4.f, 7.f), C});
	}
	/** A gown. Standing: a long A-line from the waist. Seated: a lap over the thighs and a drape from the knees (never a
	 *  cone under the seat: Marwan's "upside-down cone chairs"). */
	void Gown(TArray<FPiece>& Out, const FLinearColor& C, const FLook& L)
	{
		if (L.bSeated)
		{
			// --- W3 cast2 --- seated: no skirt shapes at all. Every rigid shape tried here (a cone, a board at the shins, an
			// egg on the lap, a tube round the shins) read as a box or a bucket between the knees; the body already wears
			// the gown's colour (Paint Tint), so the seated woman is the tinted figure with her hair, hat and jewellery.
		}
		else
		{
			const float Len = L.SkirtLength, W = 0.8f + 0.2f * Len;
			Out.Add({ETakeShape::Cone, TEXT("pelvis"), FVector(0.f, 0.f, 8.f - 50.f * Len), FRotator::ZeroRotator, FVector(64.f * W, 68.f * W, 100.f * Len), C});
		}
	}
	void Extras(TArray<FPiece>& Out, const FLook& L, const FLinearColor& Tint)
	{
		if (L.bPocket) Out.Add({ETakeShape::Cube, TEXT("spine_05"), FVector(11.5f, -9.5f, -2.f), FRotator::ZeroRotator, FVector(1.2f, 4.f, 2.5f), Draw(GetTypeHash(L.Accent.R), 3) < 0.5f ? L.Shirt : L.Accent});
		if (L.bFlower) Out.Add({ETakeShape::Sphere, TEXT("spine_05"), FVector(11.5f, -10.5f, 5.f), FRotator::ZeroRotator, FVector(2.6f, 2.6f, 2.6f), L.Accent});
		if (L.bPearls) Out.Add({ETakeShape::Cylinder, TEXT("neck_01"), FVector(2.f, 0.f, -1.5f), FRotator::ZeroRotator, FVector(14.f, 15.f, 1.4f), Pearl});
		if (L.bStole) Out.Add({ETakeShape::Cylinder, TEXT("spine_05"), FVector(-1.f, 0.f, 6.f), FRotator::ZeroRotator, FVector(28.f, 42.f, 4.5f), L.Accent});	// W3 cast2: thinner (read as a neck brace)
		if (L.bHairFlower) Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(0.f, 8.5f, 15.f), FRotator::ZeroRotator, FVector(4.f, 3.f, 4.f), L.Accent});
	}
	void Glass(TArray<FPiece>& Out)
	{
		Out.Add({ETakeShape::Cylinder, TEXT("hand_r"), FVector(6.5f, 1.5f, 3.f), FRotator::ZeroRotator, FVector(0.9f, 0.9f, 8.f), GlassTone, EKind::Held, 1});
		Out.Add({ETakeShape::Sphere, TEXT("hand_r"), FVector(6.5f, 7.5f, 3.f), FRotator::ZeroRotator, FVector(9.f, 9.f, 4.5f), GlassTone, EKind::Held, 1});
	}

	void Build(ECastKit Kit, const FLinearColor& Tint, const FLook& L, TArray<FPiece>& Out)
	{
		const FLinearColor Dark = Scaled(Tint, 0.25f);
		const FLinearColor Coat = Scaled(Tint, 0.8f);
		const FLinearColor HairC = L.bShell ? L.Hair : Scaled(Tint, 0.25f);
		const FLinearColor ShirtC = L.bShell ? L.Shirt : FLinearColor(FMath::Min(Tint.R + 0.12f, 0.40f), FMath::Min(Tint.G + 0.12f, 0.40f), FMath::Min(Tint.B + 0.12f, 0.40f));
		const FLinearColor Tie = L.bShell ? (L.bWhiteTie ? L.Shirt : Grey(0.01f)) : Dark;
		switch (Kit)
		{
		case ECastKit::GuestTux:
			Shell(Out, L); Hair(Out, L.HairStyle, HairC); Face(Out, L);
			ShirtFront(Out, ShirtC); BowTie(Out, L.bCummerbund ? L.Accent : Grey(0.01f));
			if (L.bCummerbund) Out.Add({ETakeShape::Cylinder, TEXT("pelvis"), FVector(1.f, 0.f, 13.f), FRotator::ZeroRotator, FVector(27.f, 36.f, 9.f), L.Accent});
			Extras(Out, L, Tint);
			break;
		case ECastKit::GuestTails:
			Shell(Out, L); Face(Out, L); ShirtFront(Out, ShirtC); BowTie(Out, Tie); Tails(Out, Coat);
			if (L.bHat || !L.bShell) TopHat(Out, Grey(0.012f), 20.f); else Hair(Out, L.HairStyle, HairC);
			Extras(Out, L, Tint);
			break;
		case ECastKit::GuestGown:
			Shell(Out, L); Hair(Out, L.HairStyle, HairC); Face(Out, L); Gown(Out, Tint, L); Extras(Out, L, Tint);
			break;
		case ECastKit::GuestGownHat:
		{
			FLook W = L;
			W.bStole = true;
			Shell(Out, W); Hair(Out, W.HairStyle == 5 ? 4 : W.HairStyle, HairC); Gown(Out, Tint, W);
			LadyHat(Out, W.bShell ? Scaled(W.Accent, 0.6f, 0.008f, 0.3f) : Dark, W.bShell ? W.Accent : Dark);
			Extras(Out, W, Tint);
			break;
		}
		case ECastKit::Staff:
			Shell(Out, L); Hair(Out, L.HairStyle, HairC); Face(Out, L); ShirtFront(Out, ShirtC); BowTie(Out, Grey(0.01f));
			Out.Add({ETakeShape::Cube, TEXT("spine_03"), FVector(10.5f, 0.f, 0.f), FRotator(-4.f, 0.f, 0.f), FVector(2.f, 26.f, 28.f), Dark});	// the waistcoat
			break;
		case ECastKit::Guard:
			Shell(Out, L); Face(Out, L); PeakedCap(Out, Tint, 7.f); Epaulettes(Out, Tint); Belt(Out, Tint);
			break;
		case ECastKit::Doorman:
		{
			FLook D = L;
			D.bMoustache = D.bShell;
			D.bWideMoustache = true;
			Shell(Out, D); Face(Out, D); PeakedCap(Out, Tint, 11.f); Epaulettes(Out, Tint);
			Out.Add({ETakeShape::Cylinder, TEXT("pelvis"), FVector(-1.f, 0.f, -24.f), FRotator::ZeroRotator, FVector(31.f, 40.f, 56.f), Tint});	// the greatcoat's skirts
			break;
		}
		case ECastKit::Vance:
		{
			FLook V = L;
			V.bMoustache = V.bShell;
			V.bWideMoustache = false;
			V.Hair = Grey(0.22f);
			V.bGlasses = false;
			Shell(Out, V); Face(Out, V); ShirtFront(Out, V.bShell ? V.Shirt : ShirtC); BowTie(Out, V.bShell ? V.Shirt : Dark);
			Tails(Out, Tint); TopHat(Out, Tint, 26.f);
			Out.Add({ETakeShape::Cylinder, nullptr, FVector(10.f, 27.f, 46.f), FRotator(0.f, 0.f, -6.f), FVector(2.5f, 2.5f, 92.f), Tint});		// the cane
			Out.Add({ETakeShape::Sphere, nullptr, FVector(10.f, 32.f, 92.f), FRotator::ZeroRotator, FVector(5.f, 5.f, 5.f), V.bShell ? Metal : Tint});
			break;
		}
		case ECastKit::Bodyguard:
			Shell(Out, L);
			Out.Add({ETakeShape::Sphere, TEXT("spine_05"), FVector(-1.f, 0.f, 3.f), FRotator::ZeroRotator, FVector(27.f, 54.f, 20.f), Tint});		// the shoulders
			Out.Add({ETakeShape::Cube, TEXT("head"), FVector(11.5f, 0.f, 8.f), FRotator::ZeroRotator, FVector(2.f, 15.f, 3.5f), Grey(0.005f)});	// dark glasses
			break;
		case ECastKit::BandBass:
		case ECastKit::BandTrumpet:
		case ECastKit::BandDrums:
		case ECastKit::BandPiano:
		{
			Shell(Out, L); Face(Out, L);
			if (L.bHat || !L.bShell) Fedora(Out, L.bShell ? Lin(0.05f, 0.04f, 0.03f) : Scaled(Tint, 1.5f)); else Hair(Out, L.HairStyle, HairC);
			ShirtFront(Out, ShirtC); BowTie(Out, Grey(0.01f));
			const bool bStand = !L.bInstrumentProps;
			if (Kit == ECastKit::BandBass && bStand)
			{
				// The upright bass: a waisted body (two flattened bouts), a fingerboard up a long neck, the scroll, the pin.
				Out.Add({ETakeShape::Sphere, nullptr, FVector(32.f, -8.f, 50.f), FRotator::ZeroRotator, FVector(20.f, 56.f, 72.f), BassWood});
				Out.Add({ETakeShape::Sphere, nullptr, FVector(32.f, -8.f, 100.f), FRotator::ZeroRotator, FVector(18.f, 40.f, 52.f), BassWood});
				Out.Add({ETakeShape::Cube, nullptr, FVector(43.f, -8.f, 115.f), FRotator::ZeroRotator, FVector(2.f, 6.f, 120.f), Wood});
				Out.Add({ETakeShape::Cylinder, nullptr, FVector(31.f, -8.f, 150.f), FRotator::ZeroRotator, FVector(6.f, 6.f, 72.f), BassWood});
				Out.Add({ETakeShape::Sphere, nullptr, FVector(31.f, -8.f, 190.f), FRotator::ZeroRotator, FVector(7.f, 7.f, 11.f), BassWood});
				Out.Add({ETakeShape::Cube, nullptr, FVector(43.f, -8.f, 62.f), FRotator::ZeroRotator, FVector(2.f, 16.f, 3.f), Skin});
				Out.Add({ETakeShape::Cylinder, nullptr, FVector(32.f, -8.f, 8.f), FRotator::ZeroRotator, FVector(2.f, 2.f, 16.f), Metal});
			}
			else if (Kit == ECastKit::BandTrumpet)
			{
				// Held at the lips (it moves with the head), so it stays even with real props on the riser.
				Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(26.f, 0.f, -6.f), FRotator(90.f, 0.f, 0.f), FVector(3.5f, 3.5f, 34.f), Metal});
				Out.Add({ETakeShape::Cube, TEXT("head"), FVector(22.f, 0.f, -2.f), FRotator::ZeroRotator, FVector(7.f, 3.f, 7.f), Metal});
				Out.Add({ETakeShape::Cone, TEXT("head"), FVector(45.f, 0.f, -6.f), FRotator(90.f, 0.f, 0.f), FVector(14.f, 14.f, 14.f), Metal});
				Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(52.f, 0.f, -6.f), FRotator::ZeroRotator, FVector(9.f, 9.f, 9.f), Dark});
			}
			else if (Kit == ECastKit::BandDrums)
			{
				Out.Add({ETakeShape::Cylinder, TEXT("hand_r"), FVector(13.f, 1.f, 1.f), FRotator::ZeroRotator, FVector(1.3f, 1.3f, 40.f), Lin(0.20f, 0.14f, 0.08f), EKind::Held, 0});
				Out.Add({ETakeShape::Cylinder, TEXT("hand_l"), FVector(13.f, 1.f, -1.f), FRotator::ZeroRotator, FVector(1.3f, 1.3f, 40.f), Lin(0.20f, 0.14f, 0.08f), EKind::Held, 0});
				if (bStand)
				{
					Out.Add({ETakeShape::Cylinder, nullptr, FVector(0.f, 0.f, -25.f), FRotator::ZeroRotator, FVector(34.f, 34.f, 50.f), Wood});
					Out.Add({ETakeShape::Cylinder, nullptr, FVector(62.f, 0.f, -22.f), FRotator(90.f, 0.f, 0.f), FVector(56.f, 56.f, 36.f), Wood});
					Out.Add({ETakeShape::Cylinder, nullptr, FVector(81.f, 0.f, -22.f), FRotator(90.f, 0.f, 0.f), FVector(52.f, 52.f, 1.f), Skin});
					Out.Add({ETakeShape::Cylinder, nullptr, FVector(32.f, -20.f, 12.f), FRotator(0.f, 0.f, 8.f), FVector(36.f, 36.f, 14.f), Skin});
					Out.Add({ETakeShape::Cylinder, nullptr, FVector(55.f, 12.f, 30.f), FRotator(20.f, 0.f, 0.f), FVector(30.f, 30.f, 22.f), Wood});
					Out.Add({ETakeShape::Cylinder, nullptr, FVector(30.f, -45.f, -5.f), FRotator::ZeroRotator, FVector(2.5f, 2.5f, 90.f), Metal});
					Out.Add({ETakeShape::Cylinder, nullptr, FVector(30.f, -45.f, 40.f), FRotator::ZeroRotator, FVector(36.f, 36.f, 1.5f), Metal});
					Out.Add({ETakeShape::Cylinder, nullptr, FVector(30.f, -45.f, 42.5f), FRotator::ZeroRotator, FVector(36.f, 36.f, 1.5f), Metal});
					Out.Add({ETakeShape::Cylinder, nullptr, FVector(48.f, 42.f, 0.f), FRotator::ZeroRotator, FVector(2.5f, 2.5f, 110.f), Metal});
					Out.Add({ETakeShape::Cylinder, nullptr, FVector(48.f, 42.f, 55.f), FRotator(0.f, 0.f, 14.f), FVector(50.f, 50.f, 1.5f), Metal});
				}
			}
			else if (Kit == ECastKit::BandPiano && bStand)
			{
				Out.Add({ETakeShape::Cube, nullptr, FVector(0.f, 0.f, -25.f), FRotator::ZeroRotator, FVector(36.f, 70.f, 50.f), Wood});
				Out.Add({ETakeShape::Cube, nullptr, FVector(64.f, 0.f, 15.f), FRotator::ZeroRotator, FVector(52.f, 140.f, 130.f), Wood});
				Out.Add({ETakeShape::Cube, nullptr, FVector(64.f, 0.f, 81.f), FRotator::ZeroRotator, FVector(58.f, 146.f, 3.f), Wood});
				Out.Add({ETakeShape::Cube, nullptr, FVector(30.f, 0.f, 22.f), FRotator::ZeroRotator, FVector(24.f, 124.f, 6.f), Keys});
			}
			break;
		}
		case ECastKit::BandSinger:
		{
			FLook S = L;
			S.HairStyle = S.HairStyle == 3 ? 4 : S.HairStyle;
			S.bHairFlower = S.bShell;
			Shell(Out, S); Hair(Out, S.HairStyle, HairC); Gown(Out, Tint, S); Extras(Out, S, Tint);
			if (!S.bInstrumentProps)
			{
				Out.Add({ETakeShape::Cylinder, nullptr, FVector(30.f, 0.f, 1.f), FRotator::ZeroRotator, FVector(26.f, 26.f, 2.f), Metal});
				Out.Add({ETakeShape::Cylinder, nullptr, FVector(30.f, 0.f, 72.f), FRotator::ZeroRotator, FVector(2.5f, 2.5f, 142.f), Metal});
				Out.Add({ETakeShape::Sphere, nullptr, FVector(27.f, 0.f, 147.f), FRotator(-30.f, 0.f, 0.f), FVector(6.f, 6.f, 10.f), Dark});
			}
			break;
		}
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
		if (L.bGlass)
		{
			Glass(Out);
		}
	}

	// --- crowd pass (Nov 15): a guest with an explicit look (FCastLook, from build_cast.py) --------------------------------
	// The body is painted (M_CrowdBody: skin, hands, sleeves, gloves, shirt front, trousers, shoes, hair, face), so no skin
	// shell and no shirt-front piece; the pieces only add what stands proud of the body. Offsets on "head" are from the
	// head bone (the mannequin's face is about CrowdFaceX forward of it, the eyes CrowdEyeZ up).
	const float CrowdFaceX = 10.5f;
	const float CrowdEyeZ = 7.f;

	bool IsGuestKit(ECastKit Kit)
	{
		return Kit == ECastKit::GuestTux || Kit == ECastKit::GuestTails || Kit == ECastKit::GuestGown || Kit == ECastKit::GuestGownHat;
	}

	void CrowdHair(TArray<FPiece>& Out, int32 Style, const FLinearColor& C, bool bHat)
	{
		const FRotator Z = FRotator::ZeroRotator;
		auto Cap = [&]() { Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(-3.5f, 0.f, 9.5f), Z, FVector(21.f, 18.8f, 18.5f), C}); };
		switch (Style)
		{
		case 5: if (!bHat) Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(2.f, 0.f, 16.5f), Z, FVector(15.f, 14.f, 7.f), C}); break;
		case 10: Cap(); if (!bHat) Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(-12.5f, 0.f, 12.f), Z, FVector(10.f, 10.f, 10.f), C}); break;
		case 11: Cap(); Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(-7.f, 0.f, 2.f), Z, FVector(14.f, 21.f, 13.f), C}); break;
		case 12:
			Cap();
			if (!bHat)
			{
				Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(3.f, 5.f, 18.f), Z, FVector(10.f, 7.f, 7.f), C});
				Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(3.f, -5.f, 18.f), Z, FVector(10.f, 7.f, 7.f), C});
			}
			break;
		case 13: Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(-3.f, 0.f, 7.f), Z, FVector(22.f, 21.f, 21.f), C}); break;
		case 14: Cap(); if (!bHat) Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(-4.f, 0.f, 21.f), Z, FVector(13.f, 12.f, 14.f), C}); break;
		case 15: Cap(); Out.Add({ETakeShape::Cube, TEXT("head"), FVector(-8.5f, 0.f, -5.f), FRotator(-6.f, 0.f, 0.f), FVector(6.f, 17.f, 22.f), C}); break;
		case 16: Cap(); Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(-11.f, 0.f, 0.f), Z, FVector(9.f, 13.f, 8.f), C}); break;
		default: break;		// the men's short styles are painted on the head
		}
	}

	void CrowdHat(TArray<FPiece>& Out, const FCastLook& K)
	{
		const FLinearColor C = K.HatColour;
		const FLinearColor Band = Scaled(C, 0.35f, 0.004f, 0.2f);
		switch (K.Hat)
		{
		case 1: Fedora(Out, C); Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(-0.5f, 0.f, 21.3f), FRotator::ZeroRotator, FVector(22.6f, 21.6f, 2.2f), Band}); break;
		case 2: TopHat(Out, C, 20.f); break;
		case 3:
			Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(0.f, 0.f, 19.5f), FRotator::ZeroRotator, FVector(31.f, 29.f, 1.8f), C});
			Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(-0.5f, 0.f, 24.5f), FRotator::ZeroRotator, FVector(21.f, 20.f, 10.f), C});
			Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(-0.5f, 0.f, 21.f), FRotator::ZeroRotator, FVector(21.6f, 20.6f, 2.2f), K.Accent});
			break;
		case 4:
			Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(0.f, 0.f, 18.5f), FRotator::ZeroRotator, FVector(29.f, 27.f, 1.2f), C});
			Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(0.f, 0.f, 21.f), FRotator::ZeroRotator, FVector(21.f, 20.f, 16.f), C});
			break;
		case 11: Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(2.f, 3.f, 20.5f), FRotator(-8.f, 0.f, 12.f), FVector(12.f, 12.f, 5.5f), C}); break;
		case 12:
			Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(1.f, 0.f, 18.5f), FRotator(-6.f, 0.f, 0.f), FVector(42.f, 40.f, 0.8f), C});
			Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(0.f, 0.f, 21.5f), FRotator(-6.f, 0.f, 0.f), FVector(19.f, 18.f, 6.f), C});
			Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(0.f, 0.f, 19.4f), FRotator(-6.f, 0.f, 0.f), FVector(19.6f, 18.6f, 1.6f), K.Accent});
			break;
		case 13: LadyHat(Out, C, K.Accent); break;
		case 14:
			Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(-1.f, 0.f, 15.5f), FRotator::ZeroRotator, FVector(23.f, 21.f, 17.f), C});
			Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(9.5f, 0.f, 17.f), FRotator::ZeroRotator, FVector(2.6f, 2.6f, 2.6f), Metal});
			break;
		default: break;
		}
	}

	void BuildGuest(ECastKit Kit, const FLinearColor& Tint, const FCastLook& K, bool bSeated, bool bGlass, TArray<FPiece>& Out)
	{
		const bool bWoman = IsQuinnKit(Kit);
		const FRotator Z = FRotator::ZeroRotator;
		const float FX = CrowdFaceX + 0.6f, EZ = CrowdEyeZ;
		CrowdHair(Out, K.HairStyle, K.Hair, K.Hat != 0);
		const int32 HatFrom = Out.Num();
		CrowdHat(Out, K);
		for (int32 i = HatFrom; i < Out.Num(); ++i)
		{
			Out[i].Loc.Z -= 4.5f;	// the hats were placed on the skin shell; the painted head is lower
		}
		const FLinearColor Rim = K.Glasses == 2 ? Lin(0.035f, 0.018f, 0.008f) : Grey(0.02f);
		if (K.Glasses == 1)
		{
			Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(FX, 3.3f, EZ), FRotator(90.f, 0.f, 0.f), FVector(3.4f, 3.4f, 0.5f), Rim});
			Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(FX, -3.3f, EZ), FRotator(90.f, 0.f, 0.f), FVector(3.4f, 3.4f, 0.5f), Rim});
		}
		else if (K.Glasses == 2)
		{
			Out.Add({ETakeShape::Cube, TEXT("head"), FVector(FX, 3.4f, EZ + 0.3f), Z, FVector(0.8f, 4.6f, 3.4f), Rim});
			Out.Add({ETakeShape::Cube, TEXT("head"), FVector(FX, -3.4f, EZ + 0.3f), Z, FVector(0.8f, 4.6f, 3.4f), Rim});
		}
		else if (K.Glasses == 3)
		{
			Out.Add({ETakeShape::Cylinder, TEXT("head"), FVector(FX, 3.3f, EZ), FRotator(90.f, 0.f, 0.f), FVector(3.6f, 3.6f, 0.5f), Metal});
		}
		switch (K.Neck)
		{
		case 0: BowTie(Out, Grey(0.01f)); break;
		case 1: BowTie(Out, K.Shirt); break;
		case 2: BowTie(Out, K.Accent); break;
		case 3:
			Out.Add({ETakeShape::Cube, TEXT("neck_01"), FVector(8.5f, 0.f, -4.5f), Z, FVector(2.5f, 4.f, 3.5f), K.Accent});
			Out.Add({ETakeShape::Cube, TEXT("spine_05"), FVector(11.8f, 0.f, -9.f), FRotator(-8.f, 0.f, 0.f), FVector(1.2f, 5.f, 24.f), K.Accent});
			break;
		case 11: Out.Add({ETakeShape::Cylinder, TEXT("neck_01"), FVector(2.f, 0.f, -1.5f), Z, FVector(14.f, 15.f, 1.4f), Pearl}); break;
		case 12: Out.Add({ETakeShape::Cylinder, TEXT("neck_01"), FVector(1.f, 0.f, 3.f), Z, FVector(11.5f, 12.f, 1.8f), K.Accent}); break;
		case 13: Out.Add({ETakeShape::Cylinder, TEXT("neck_01"), FVector(2.5f, 0.f, -2.5f), FRotator(-10.f, 0.f, 0.f), FVector(15.f, 16.f, 0.8f), Metal}); break;
		default: break;
		}
		if (K.bEarrings)
		{
			const FLinearColor E = K.Neck == 11 ? Pearl : Metal;
			Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(1.5f, 8.2f, 1.5f), Z, FVector(1.9f, 1.9f, 2.4f), E});
			Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(1.5f, -8.2f, 1.5f), Z, FVector(1.9f, 1.9f, 2.4f), E});
		}
		if (K.Wrap == 1) Out.Add({ETakeShape::Sphere, TEXT("spine_05"), FVector(-7.f, 0.f, 3.f), FRotator(-15.f, 0.f, 0.f), FVector(13.f, 44.f, 20.f), K.WrapColour});	// over the back and shoulders (a disc read as a neck ring)
		else if (K.Wrap == 2) Out.Add({ETakeShape::Sphere, TEXT("spine_05"), FVector(-2.f, 0.f, 9.f), Z, FVector(25.f, 39.f, 10.f), K.WrapColour});
		if (K.Chest == 1 || K.Chest == 3) Out.Add({ETakeShape::Cube, TEXT("spine_05"), FVector(11.5f, -9.5f, -2.f), Z, FVector(1.2f, 4.f, 2.5f), K.Shirt.R > 0.3f && K.Chest == 3 ? K.Accent : K.Shirt});
		if (K.Chest == 2 || K.Chest == 3) Out.Add({ETakeShape::Sphere, TEXT("spine_05"), FVector(11.5f, -10.5f, 5.f), Z, FVector(2.8f, 2.8f, 2.8f), K.Accent});
		if (K.Chest == 4) Out.Add({ETakeShape::Sphere, TEXT("spine_05"), FVector(10.5f, -6.f, 3.f), Z, FVector(2.6f, 1.2f, 2.6f), Metal});
		if (K.bHairFlower) Out.Add({ETakeShape::Sphere, TEXT("head"), FVector(-1.f, 8.f, 14.f), Z, FVector(4.f, 3.f, 4.f), K.Accent});
		if (K.bCummerbund && !bWoman) Out.Add({ETakeShape::Cylinder, TEXT("pelvis"), FVector(1.f, 0.f, 13.f), Z, FVector(27.f, 36.f, 9.f), K.Accent});
		if (Kit == ECastKit::GuestTails) Tails(Out, Scaled(Tint, 0.8f));
		if (bWoman && !bSeated)
		{
			FLook G;
			G.bSeated = false;
			G.SkirtLength = 1.f;
			Gown(Out, K.Lower, G);
		}
		if (bGlass)
		{
			Glass(Out);
		}
	}

	/** Shared colour instances of BasicShapeMaterial, so equal pieces draw together. */
	UMaterialInstanceDynamic* SharedColour(const FLinearColor& Colour)
	{
		static TMap<uint32, TWeakObjectPtr<UMaterialInstanceDynamic>> Cache;
		const uint32 Hash = Colour.ToFColor(false).DWColor();
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

	/** PieceBones' mark for a piece on the actor's root (it never moves to the crew's copy). */
	const FName RootPiece(TEXT("__Root"));
}

// --- every member in the running world: groups, neighbours, Vance, the clip cache -------------------------------------

struct FCastWorld
{
	static TArray<TWeakObjectPtr<ACastMember>>& All() { static TArray<TWeakObjectPtr<ACastMember>> A; return A; }
	static TWeakObjectPtr<UWorld>& World() { static TWeakObjectPtr<UWorld> W; return W; }
	static bool& Grouped() { static bool b = false; return b; }
	static TMap<FName, TWeakObjectPtr<UAnimSequenceBase>>& Clips() { static TMap<FName, TWeakObjectPtr<UAnimSequenceBase>> C; return C; }
	static TSet<FName>& Missing() { static TSet<FName> M; return M; }

	static void Enter(ACastMember* M)
	{
		UWorld* W = M->GetWorld();
		if (World().Get() != W)
		{
			All().Reset();
			Grouped() = false;
			Missing().Reset();	// clips imported since the last run are found
			World() = W;
		}
		All().AddUnique(M);
		Grouped() = false;
	}
	static void Leave(ACastMember* M) { All().Remove(M); }

	static ACastMember* Find(ECastKit Kit)
	{
		for (const TWeakObjectPtr<ACastMember>& P : All())
		{
			if (P.IsValid() && P->Kit == Kit)
			{
				return P.Get();
			}
		}
		return nullptr;
	}

	/** Conversation groups (standing guests within GroupRadius, chained) and seated neighbours, once per world. */
	static void EnsureGroups()
	{
		if (Grouped())
		{
			return;
		}
		Grouped() = true;
		TArray<ACastMember*> Standing, Seated;
		for (const TWeakObjectPtr<ACastMember>& P : All())
		{
			ACastMember* M = P.Get();
			if (!M || M->bDressParent || M->bWalker || M->bBandKit || (!M->bSeated && M->Kit == ECastKit::Staff))
			{
				continue;
			}
			(M->bSeated ? Seated : Standing).Add(M);
		}
		const int32 N = Standing.Num();
		TArray<int32> Parent;
		Parent.SetNum(N);
		for (int32 i = 0; i < N; ++i)
		{
			Parent[i] = i;
		}
		auto Root = [&Parent](int32 i) { while (Parent[i] != i) { Parent[i] = Parent[Parent[i]]; i = Parent[i]; } return i; };
		for (int32 i = 0; i < N; ++i)
		{
			for (int32 j = i + 1; j < N; ++j)
			{
				const float R = FMath::Min(Standing[i]->GroupRadius, Standing[j]->GroupRadius);
				if (FVector::Dist2D(Standing[i]->GetActorLocation(), Standing[j]->GetActorLocation()) < R)
				{
					Parent[Root(i)] = Root(j);
				}
			}
		}
		TMap<int32, TArray<ACastMember*>> Clusters;
		for (int32 i = 0; i < N; ++i)
		{
			Clusters.FindOrAdd(Root(i)).Add(Standing[i]);
		}
		for (TPair<int32, TArray<ACastMember*>>& C : Clusters)
		{
			C.Value.Sort([](const ACastMember& X, const ACastMember& Y) { return X.GetFName().LexicalLess(Y.GetFName()); });
			const uint32 Key = FCrc::StrCrc32(*C.Value[0]->GetFName().ToString());
			for (ACastMember* M : C.Value)
			{
				M->Group.Reset();
				for (ACastMember* Other : C.Value)
				{
					M->Group.Add(Other);
				}
				M->GroupKey = Key;
				M->TurnSeconds = 4.f + 3.f * CastKits::Draw(Key, 7);
			}
		}
		for (ACastMember* M : Seated)
		{
			M->Neighbours.Reset();
			TArray<TPair<float, ACastMember*>> Near;
			for (ACastMember* Other : Seated)
			{
				const float D = FVector::Dist2D(M->GetActorLocation(), Other->GetActorLocation());
				if (Other != M && D < 140.f)
				{
					Near.Add(TPair<float, ACastMember*>(D, Other));
				}
			}
			Near.Sort([](const TPair<float, ACastMember*>& X, const TPair<float, ACastMember*>& Y) { return X.Key < Y.Key; });
			for (int32 i = 0; i < FMath::Min(2, Near.Num()); ++i)
			{
				M->Neighbours.Add(Near[i].Value);
			}
		}
	}
};

// --- setup -------------------------------------------------------------------------------------------------------------

// chars-locomotion: the template's two-handed pistol aim (on the mannequin skeleton: every cast mesh plays it).
namespace CastGun
{
	UAnimSequenceBase* AimClip()
	{
		static TWeakObjectPtr<UAnimSequenceBase> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<UAnimSequenceBase>(nullptr,
				TEXT("/Game/Characters/Mannequins/Anims/Pistol/MF_Pistol_Idle_ADS.MF_Pistol_Idle_ADS"));
		}
		return Cached.Get();
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

	// chars-locomotion: the pistol, hidden until SetArmed (or bArmed on the placed member).
	Gun = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Gun"));
	Gun->SetupAttachment(Body);
	Gun->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Gun->SetGenerateOverlapEvents(false);
	Gun->SetCanEverAffectNavigation(false);
	Gun->bReceivesDecals = false;
	Gun->SetHiddenInGame(true);
	Gun->SetVisibility(false);
	GunMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Take/Props/Guns/SM_Pistol.SM_Pistol")));
	// hand_r on the mannequin skeleton: +X runs back up the forearm, +Y out of the palm's thumb side, +Z out of the
	// back of the hand. The grip sits across the palm with the barrel along the fingers (tools/build_gun.py probe).
	GunGrip = FTransform(FRotator(0.f, 180.f, 0.f), FVector(-9.f, 2.f, -1.5f));
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

UCastAnimInstance* ACastMember::GetAnim() const
{
	return Body ? Cast<UCastAnimInstance>(Body->GetAnimInstance()) : nullptr;
}

float ACastMember::Draw(uint32 Salt) const
{
	return CastKits::Draw(Seed, Salt);
}

bool ACastMember::IsActingNow() const
{
	return bActing && CVarCastActing.GetValueOnGameThread() != 0;
}

void ACastMember::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UWorld* World = GetWorld();
	if (!World || World->IsGameWorld())
	{
		return;
	}
	// The editor shows the costume and the first frame of the pose; a character's own mesh is dressed only in play.
	bDressParent = Cast<ACharacter>(GetAttachParentActor()) != nullptr;
	bWalker = !bDressParent && GetAttachParentActor() != nullptr;
	SetupBody();
	if (!bDressParent)
	{
		BuildPieces(true);
	}
	ApplyGun();
}

void ACastMember::BeginPlay()
{
	Super::BeginPlay();
	bDressParent = Cast<ACharacter>(GetAttachParentActor()) != nullptr;
	bWalker = !bDressParent && GetAttachParentActor() != nullptr;
	FCastWorld::Enter(this);
	SetupBody();
	BuildPieces(true);
	if (bWalker)
	{
		HideParentStandIn();
		LastParentLocation = GetAttachParentActor()->GetActorLocation();
	}
	if (bDressParent)
	{
		if (USkeletalMeshComponent* Wearer = GetWearer())
		{
			Body->AddTickPrerequisiteComponent(Wearer);
		}
	}
	TargetYaw = static_cast<float>(GetActorRotation().Yaw);
	bBowFiredLast = false;
	ApplyGun();
	LastOwnLocation = GetActorLocation();
	bHasOwnLocation = true;
	const float Now = GetWorld()->GetTimeSeconds();
	NextThink = Now + FMath::FRand() * ThinkInterval;
	NextLookChange = Now + FMath::FRand() * 3.f;
	NextLean = Now + 2.f + FMath::FRand() * 6.f;
	NextIdleAct = Now + 4.f + FMath::FRand() * 10.f;
	SetActorTickInterval(bWalker || bBandKit || bDressParent ? 0.f : FMath::Min(ThinkInterval, 0.1f));
}

void ACastMember::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FCastWorld::Leave(this);
	Super::EndPlay(EndPlayReason);
}

bool ACastMember::UsesCrowdLook() const
{
	return Look.bExplicit && (Look.ForSeed < 0 || Look.ForSeed == LookSeed) && bVary && HeroMesh.IsNull() && !bDressParent && !bWalker && CastKits::IsGuestKit(Kit);
}

bool ACastMember::PaintCrowdBody()
{
	USkeletalMesh* Mesh = Body ? Body->GetSkeletalMeshAsset() : nullptr;
	if (!CrowdBodyMaterial || !Mesh)
	{
		return false;
	}
	UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Body->GetMaterial(0));
	if (!MID || MID->Parent != CrowdBodyMaterial)
	{
		MID = UMaterialInstanceDynamic::Create(CrowdBodyMaterial, Body);
	}
	if (!MID)
	{
		return false;
	}
	for (int32 Slot = 0; Slot < Body->GetNumMaterials(); ++Slot)
	{
		Body->SetMaterial(Slot, MID);
	}
	// The joints at the reference pose, in the mesh's space (the material's pre-skinned position is in the same space).
	const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
	auto Joint = [&](const TCHAR* Name, const TCHAR* Param) {
		const int32 Index = Ref.FindBoneIndex(FName(Name));
		if (Index != INDEX_NONE)
		{
			const FVector P = FAnimationRuntime::GetComponentSpaceTransformRefPose(Ref, Index).GetLocation();
			MID->SetVectorParameterValue(Param, FLinearColor(float(P.X), float(P.Y), float(P.Z)));
		}
	};
	Joint(TEXT("head"), TEXT("HeadPos"));
	Joint(TEXT("neck_01"), TEXT("NeckPos"));
	Joint(TEXT("spine_05"), TEXT("ChestPos"));
	Joint(TEXT("pelvis"), TEXT("PelvisPos"));
	Joint(TEXT("upperarm_r"), TEXT("ShoulderPos"));
	Joint(TEXT("lowerarm_r"), TEXT("ElbowPos"));
	Joint(TEXT("hand_r"), TEXT("WristPos"));
	Joint(TEXT("foot_r"), TEXT("FootPos"));

	const bool bWoman = CastKits::IsQuinnKit(Kit);
	int32 Cover = 1;		// 0 bald, 1 full, 2 balding, 3 receding, 4 a woman's, 5 full with a parting
	switch (Look.HairStyle)
	{
	case 1: Cover = 5; break;
	case 2: Cover = 2; break;
	case 3: Cover = 0; break;
	case 4: Cover = 3; break;
	default: Cover = Look.HairStyle >= 10 ? 4 : 1; break;
	}
	MID->SetVectorParameterValue(TEXT("Skin"), Look.Skin);
	MID->SetVectorParameterValue(TEXT("Upper"), Tint);
	MID->SetVectorParameterValue(TEXT("Lower"), Look.Lower);
	MID->SetVectorParameterValue(TEXT("Shirt"), Look.Shirt);
	MID->SetVectorParameterValue(TEXT("Glove"), Look.GloveColour);
	MID->SetVectorParameterValue(TEXT("Shoe"), Look.Shoes);
	MID->SetVectorParameterValue(TEXT("HairColour"), Look.Hair);
	MID->SetVectorParameterValue(TEXT("Lips"), bWoman ? Look.Lips : CastKits::Scaled(Look.Skin, 0.6f, 0.004f, 0.4f));
	MID->SetVectorParameterValue(TEXT("Accent"), Look.Accent);
	MID->SetVectorParameterValue(TEXT("Style1"), FLinearColor(float(Look.Gloves), Look.bBareArms ? 1.f : 0.f, float(Look.Neckline)));
	MID->SetVectorParameterValue(TEXT("Style2"), FLinearColor(float(Cover), float(Look.FacialHair), bWoman ? 0.f : 1.f));
	MID->SetVectorParameterValue(TEXT("Style3"), FLinearColor(Look.bSash ? 1.f : 0.f, bWoman ? 1.f : 0.f, 0.f));
	return true;
}

void ACastMember::SetupBody()
{
	Seed = LookSeed >= 0 ? uint32(LookSeed) * 2654435761u + 17u : FCrc::StrCrc32(*GetFName().ToString());
	bBandKit = CastKits::IsBandKit(Kit);
	const int32 KeepAct = Acting.ActSerial, KeepBase = Acting.BaseSerial;
	Acting = FCastActing();
	Acting.ActSerial = KeepAct + 1;
	Acting.BaseSerial = KeepBase;
	Acting.Seed = Seed;
	Acting.bActing = IsActingNow();

	if (bDressParent)
	{
		// The crew: our body is a copy of the character's, shown only during an act (TickCrew).
		Body->SetVisibility(false);
		Body->SetComponentTickEnabled(false);
		return;
	}

	static const TCHAR* Manny = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple");
	static const TCHAR* Quinn = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple");
	USkeletalMesh* Mesh = HeroMesh.IsNull() ? nullptr : HeroMesh.LoadSynchronous();
	if (!Mesh)
	{
		Mesh = LoadObject<USkeletalMesh>(nullptr, bQuinn || CastKits::IsQuinnKit(Kit) ? Quinn : Manny);
	}
	if (Body->GetSkeletalMeshAsset() != Mesh)
	{
		Body->SetSkeletalMeshAsset(Mesh);
	}
	Body->SetVisibility(true);
	const float Breadth = UsesCrowdLook() ? FMath::Clamp(Look.Width, 0.8f, 1.3f) : 1.f;
	Body->SetRelativeScale3D(FVector(BodyScale * Breadth, BodyScale * Breadth, BodyScale));
	Body->SetCastShadow(bBodyShadow);
	if (Body->LDMaxDrawDistance > 0.f)
	{
		Body->SetCachedMaxDrawDistance(Body->LDMaxDrawDistance);
	}
	if (!HeroMesh.IsNull() && Body->OverrideMaterials.Num() > 0)
	{
		Body->EmptyOverrideMaterials();	// --- W3 cast2 --- a hero wears its own material, not the mannequin's tint MIDs
	}
	if (HeroMesh.IsNull() && !(UsesCrowdLook() && PaintCrowdBody()))
	{
		// The tint, as the characters do it (M_Mannequin's "Paint Tint"); no glowing logo on the chest.
		for (int32 Slot = 0; Slot < Body->GetNumMaterials(); ++Slot)
		{
			if (Body->GetMaterial(Slot))
			{
				if (UMaterialInstanceDynamic* MID = Body->CreateAndSetMaterialInstanceDynamic(Slot))
				{
					MID->SetVectorParameterValue(TEXT("Paint Tint"), Tint);
					MID->SetVectorParameterValue(TEXT("LogoTint"), Tint * 0.5f);
					MID->SetScalarParameterValue(TEXT("EmissivePower"), 0.f);
				}
			}
		}
	}

	// What it plays at rest: a mocap clip when one exists, else step 5's idle (bent at the hips when seated).
	RestBase = nullptr;
	bRestIsClip = false;
	bSeatClip = false;
	bSeatFitted = false;
	const int32 Pick3 = FMath::Min(int32(Draw(31) * 3.f), 2);
	if (bWalker)
	{
		RestBase = WalkBlendSpace;
		Acting.bActUpperBody = true;
	}
	else if (bBandKit)
	{
		static const FName Keys[] = {TEXT("Bass"), TEXT("Trumpet"), TEXT("Drums"), TEXT("Piano"), TEXT("Sing")};
		static const ECastInstrument Insts[] = {ECastInstrument::Bass, ECastInstrument::Trumpet, ECastInstrument::Drums, ECastInstrument::Piano, ECastInstrument::Singer};
		const int32 Index = int32(Kit) - int32(ECastKit::BandBass);
		if (UAnimSequenceBase* Clip = bBandClips ? FindClip(Keys[Index]) : nullptr)	// W3 cast2: bBandClips
		{
			RestBase = Clip;
			bRestIsClip = true;
			bSeatClip = bSeated;
		}
		else
		{
			RestBase = PoseAnim;
			Acting.Instrument = Insts[Index];
			Acting.bPlayInstrument = true;
			Acting.BandShift = BandReach;	// --- W3 cast2 ---
			Acting.bBendSeat = bSeated;
		}
	}
	else if (bSeated)
	{
		const FName A(TEXT("SitIdle_A")), B(TEXT("SitIdle_B"));
		UAnimSequenceBase* Clip = Draw(32) < 0.5f ? FindClip(A) : FindClip(B);
		Clip = Clip ? Clip : (FindClip(A) ? FindClip(A) : FindClip(B));
		if (Clip)
		{
			RestBase = Clip;
			bRestIsClip = bSeatClip = true;
		}
		else
		{
			RestBase = SeatedAnim ? SeatedAnim.Get() : PoseAnim.Get();
			Acting.bBendSeat = true;
			Acting.Hold = ECastHold::Lap;
		}
	}
	else
	{
		static const FName Idles[] = {TEXT("StandIdle_A"), TEXT("StandIdle_B"), TEXT("StandIdle_C")};
		UAnimSequenceBase* Clip = nullptr;
		for (int32 i = 0; i < 3 && !Clip; ++i)
		{
			Clip = FindClip(Idles[(Pick3 + i) % 3]);
		}
		RestBase = Clip ? Clip : PoseAnim.Get();
		bRestIsClip = Clip != nullptr;
	}
	Acting.SeatHipDegrees = SeatedHipDegrees;
	Acting.SeatKneeDegrees = SeatedKneeDegrees;
	switch (Kit)
	{
	case ECastKit::Bodyguard: Acting.Hold = ECastHold::ClaspFront; ScanAmplitude = 30.f; ScanPeriod = 9.f; break;
	case ECastKit::Doorman: Acting.Hold = ECastHold::ClaspBack; ScanAmplitude = 25.f; ScanPeriod = 7.f; break;
	case ECastKit::Vance: Acting.Hold = ECastHold::ClaspBack; ScanAmplitude = 30.f; ScanPeriod = 8.f; break;
	case ECastKit::Guard: ScanAmplitude = 30.f + 10.f * Draw(33); ScanPeriod = 5.5f + 2.f * Draw(34); break;
	default: break;
	}
	if (bHoldsGlass && !bSeated && !bBandKit && !bWalker)
	{
		Acting.Hold = ECastHold::Glass;
	}
	Acting.HoldPointActor = PodiumHand;
	Acting.Energy = Kit == ECastKit::Vance ? 1.3f : 0.85f + 0.35f * Draw(35);

	Body->SetRelativeLocation(FVector(0.f, 0.f, bSeated ? -SeatedDrop * BodyScale + SeatOffsetZ : 0.f));

	// The idle loops from its own start at its own rate (drawn from the seed), so no two are in step. Without acting a
	// seated body, or a band player built from bones, holds its first frame as step 5 had it.
	const UAnimSequenceBase* Seq = Cast<UAnimSequenceBase>(RestBase);
	const bool bStill = !bWalker && ((!bIdle && !bRestIsClip) || (!IsActingNow() && (bSeated || Acting.bPlayInstrument)));
	const float Start = bStill ? (bSeated ? SeatedTime : PoseTime) : (Seq ? Draw(1) * Seq->GetPlayLength() : 0.f);
	const float Rate = bStill ? 0.f : 1.f + (2.f * Draw(2) - 1.f) * IdleRateSpread;
	PlayBase(RestBase, true, Rate, Start);
	bOnWalkSpace = RestBase == WalkBlendSpace;
	Body->bEnableUpdateRateOptimizations = !bWalker && !bBandKit;	// far guests update less often
	FitSeat();
	FramesAlive = 0;
	bFrozen = false;
}

void ACastMember::PlayBase(UAnimationAsset* Asset, bool bLoop, float Rate, float StartTime)
{
	if (!Body || !Body->GetSkeletalMeshAsset())
	{
		return;
	}
	Body->AnimationData.AnimToPlay = Asset;
	Body->AnimationData.bSavedLooping = bLoop;
	Body->AnimationData.bSavedPlaying = Rate != 0.f;
	Body->AnimationData.SavedPosition = StartTime;
	Body->AnimationData.SavedPlayRate = Rate != 0.f ? Rate : 1.f;
	UCastAnimInstance* Inst = GetAnim();
	if (!Inst)
	{
		Body->SetAnimInstanceClass(UCastAnimInstance::StaticClass());	// it starts from AnimationData
		Inst = GetAnim();
	}
	else
	{
		if (Inst->GetCurrentAsset() != Asset)
		{
			Inst->SetAnimationAsset(Asset, bLoop, Rate != 0.f ? Rate : 1.f);
		}
		Inst->SetPosition(StartTime, false);
		Inst->SetPlayRate(Rate != 0.f ? Rate : 1.f);
		Inst->SetPlaying(Rate != 0.f);
	}
	++Acting.BaseSerial;
	Acting.ActorToComponent = Body->GetRelativeTransform().Inverse();
	PushActing();
}

void ACastMember::PlayRestBase()
{
	const UAnimSequenceBase* Seq = Cast<UAnimSequenceBase>(RestBase);
	const float Rate = 1.f + (2.f * Draw(2) - 1.f) * IdleRateSpread;
	PlayBase(RestBase, true, Rate, Seq ? Draw(1) * Seq->GetPlayLength() : 0.f);
	bOnWalkSpace = RestBase == WalkBlendSpace;
}

void ACastMember::FitSeat()
{
	if (!bSeated || bDressParent || bWalker || !Body->GetSkeletalMeshAsset() || !GetAnim())
	{
		return;
	}
	// The pose of this frame on the bones now, then the hip joints onto the seat (or the clip's hip height).
	Body->TickAnimation(0.f, false);
	Body->RefreshBoneTransforms();
	// --- QA integrator --- no pose yet (construction script in the editor): GetBoneLocation would index an empty
	// array and crash the editor. Leave bSeatFitted false so Tick fits the seat once the pose exists.
	if (Body->GetComponentSpaceTransforms().Num() <= FMath::Max(Body->GetBoneIndex(TEXT("thigh_l")), Body->GetBoneIndex(TEXT("thigh_r"))))
	{
		return;
	}
	FVector Hip(0.f, 0.f, SeatedDrop);
	bool bMeasured = false;
	if (Body->GetBoneIndex(TEXT("thigh_l")) != INDEX_NONE && Body->GetBoneIndex(TEXT("thigh_r")) != INDEX_NONE)
	{
		Hip = (Body->GetBoneLocation(TEXT("thigh_l"), EBoneSpaces::ComponentSpace) + Body->GetBoneLocation(TEXT("thigh_r"), EBoneSpaces::ComponentSpace)) * 0.5f;
		Hip.Z -= SeatThighRadius;
		bMeasured = Hip.Z > 10.f && Hip.Z < 120.f;
	}
	if (bSeatClip && SeatHipHeight > 0.f)
	{
		Hip.Z = SeatHipHeight;
	}
	else if (!bMeasured)
	{
		Hip = FVector(0.f, 0.f, SeatedDrop);
	}
	// Mesh space (forward +Y, right -X) to the actor's frame: the hips at SeatHipForward on the seat's centre line.
	const float S = BodyScale;
	const float HipForward = static_cast<float>(Hip.Y) * S, HipRight = static_cast<float>(-Hip.X) * S;
	Body->SetRelativeLocation(FVector(SeatHipForward - HipForward, -HipRight, -static_cast<float>(Hip.Z) * S + SeatOffsetZ));
	// --- W3 cast2 --- the bent legs fitted to this body and this seat: the feet flat on the floor in front of it.
	if (Acting.bBendSeat && bFitLegs)
	{
		const FReferenceSkeleton& Ref = Body->GetSkeletalMeshAsset()->GetRefSkeleton();
		auto RefLoc = [&Ref](const TCHAR* Name, FVector& Out) {
			const int32 Index = Ref.FindBoneIndex(FName(Name));
			if (Index == INDEX_NONE)
			{
				return false;
			}
			Out = FAnimationRuntime::GetComponentSpaceTransformRefPose(Ref, Index).GetLocation();
			return true;
		};
		FVector RHip, RKnee, RAnkle;
		if (RefLoc(TEXT("thigh_l"), RHip) && RefLoc(TEXT("calf_l"), RKnee) && RefLoc(TEXT("foot_l"), RAnkle))
		{
			const float Thigh = static_cast<float>((RKnee - RHip).Size()) * S;
			const float Shin = static_cast<float>((RAnkle - RKnee).Size()) * S;
			const float Ankle = FMath::Max(static_cast<float>(RAnkle.Z), 4.f) * S;
			const FVector Seat = GetActorLocation();
			const FVector Probe = Seat + GetActorForwardVector() * (SeatHipForward + Thigh + 6.f);
			float FloorZ = static_cast<float>(Seat.Z) - SeatDefaultHeight;
			FHitResult Hit;
			FCollisionQueryParams Query(SCENE_QUERY_STAT(CastSeatFloor), false, this);
			if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, Probe + FVector(0.f, 0.f, 5.f), Probe - FVector(0.f, 0.f, 160.f), ECC_Visibility, Query)
				&& Hit.ImpactPoint.Z < Seat.Z - 20.f)
			{
				FloorZ = static_cast<float>(Hit.ImpactPoint.Z);
			}
			// The knee is level with the hip joint when the thighs lie flat: the shin and the ankle must cover the rest.
			const float HipZ = static_cast<float>(Seat.Z) + SeatThighRadius * S + SeatOffsetZ - FloorZ;
			const float Need = HipZ - Ankle;
			float Drop = 0.f, Lean = SeatShinForward;
			const float Rest = Need - Shin * FMath::Cos(FMath::DegreesToRadians(Lean));
			if (Rest >= 0.f)
			{
				// Short legs: the thighs slope down to the knee (the feet still short of the floor beyond the limit).
				Drop = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(Rest / FMath::Max(Thigh, 1.f), 0.f, FMath::Sin(FMath::DegreesToRadians(SeatMaxThighDrop)))));
			}
			else
			{
				// Long legs: the shins lean out first, then the knees rise.
				const float C = Need / FMath::Max(Shin, 1.f);
				Lean = C >= FMath::Cos(FMath::DegreesToRadians(SeatMaxShinForward)) ? FMath::RadiansToDegrees(FMath::Acos(FMath::Min(C, 1.f))) : SeatMaxShinForward;
				Lean = FMath::Max(Lean, SeatShinForward);
				const float Left = Need - Shin * FMath::Cos(FMath::DegreesToRadians(Lean));
				Drop = Left < 0.f ? FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(Left / FMath::Max(Thigh, 1.f), -0.34f, 0.f))) : 0.f;
			}
			Acting.SeatHipDegrees = SeatedHipDegrees - Drop;
			Acting.SeatKneeDegrees = Acting.SeatHipDegrees - Lean;
		}
	}
	Acting.ActorToComponent = Body->GetRelativeTransform().Inverse();
	PushActing();
	bSeatFitted = true;
}

UAnimSequenceBase* ACastMember::FindClip(FName Key) const
{
	if (!bUseMocap || Key.IsNone())
	{
		return nullptr;
	}
	UAnimSequenceBase* Clip = nullptr;
	if (const TWeakObjectPtr<UAnimSequenceBase>* Found = FCastWorld::Clips().Find(Key))
	{
		Clip = Found->Get();
	}
	if (!Clip)
	{
		if (FCastWorld::Missing().Contains(Key))
		{
			return nullptr;
		}
		const FString Name = Key.ToString();
		const FString Package = MocapPath + Name;
		if (FPackageName::IsValidLongPackageName(Package) && FPackageName::DoesPackageExist(Package))
		{
			Clip = LoadObject<UAnimSequenceBase>(nullptr, *(Package + TEXT(".") + Name));
		}
		if (!Clip)
		{
			FCastWorld::Missing().Add(Key);
			return nullptr;
		}
		FCastWorld::Clips().Add(Key, Clip);
	}
	// Only a clip on the body's own skeleton (the template mannequin's): anything else would twist it.
	const USkeletalMeshComponent* On = bDressParent ? GetWearer() : Body.Get();
	const USkeletalMesh* Mesh = On ? On->GetSkeletalMeshAsset() : nullptr;
	if (!Mesh || Clip->GetSkeleton() != Mesh->GetSkeleton())
	{
		return nullptr;
	}
	return Clip;
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

	// A hero mesh (ours or the character's) wears its own clothes: no costume shells.
	const ATakeCharacter* Character = bDressParent ? Cast<ATakeCharacter>(GetAttachParentActor()) : nullptr;
	const bool bHero = !HeroMesh.IsNull() || (Character && !Character->HeroMesh.IsNull());

	CastKits::FLook Drawn = CastKits::MakeLook(Seed, Kit, bVary && !bDressParent, bSkinShell);
	Drawn.bSeated = bSeated;
	Drawn.bInstrumentProps = bInstrumentProps;
	Drawn.bGlass = bHoldsGlass && !bSeated && !bDressParent && !bWalker;
	TArray<CastKits::FPiece> Defs;
	if (UsesCrowdLook())
	{
		CastKits::BuildGuest(Kit, Tint, Look, bSeated, Drawn.bGlass, Defs);
	}
	else
	{
		CastKits::Build(Kit, Tint, Drawn, Defs);
	}

	const FQuat MeshFromBody(FRotator(0.f, 90.f, 0.f));		// body X (forward) -> mesh +Y, body Y (right) -> mesh -X
	FTransform BodyRest = Wearer->GetRelativeTransform();
	BodyRest.SetLocation(FVector::ZeroVector);
	auto RefCS = [&Ref](const TCHAR* Name, FTransform& Out) {
		const int32 Index = Ref.FindBoneIndex(FName(Name));
		if (Index == INDEX_NONE)
		{
			return false;
		}
		Out = FAnimationRuntime::GetComponentSpaceTransformRefPose(Ref, Index);
		return true;
	};
	for (const CastKits::FPiece& Def : Defs)
	{
		const bool bFloor = Def.Bone == nullptr;
		if (bHero && Def.Kind == CastKits::EKind::Costume && !bFloor)
		{
			continue;
		}
		const FName Bone = bFloor ? NAME_None : FName(Def.Bone);
		FTransform BoneCS = FTransform::Identity;
		FTransform PieceCS;
		if (!bFloor && !RefCS(Def.Bone, BoneCS))
		{
			continue;
		}
		if (Def.Kind == CastKits::EKind::Held)
		{
			// The hand's frame at the reference pose: X to the middle knuckle, Y toward the thumb.
			const bool bRight = Bone == FName(TEXT("hand_r"));
			FTransform Knuckle, Thumb;
			if (!RefCS(bRight ? TEXT("middle_01_r") : TEXT("middle_01_l"), Knuckle) || !RefCS(bRight ? TEXT("thumb_01_r") : TEXT("thumb_01_l"), Thumb))
			{
				continue;
			}
			const FVector X = Knuckle.GetLocation() - BoneCS.GetLocation();
			const FVector Y = Thumb.GetLocation() - BoneCS.GetLocation();
			const FQuat Hand(FRotationMatrix::MakeFromXY(X, Y));
			const FQuat Along = FQuat::FindBetweenNormals(FVector(0.f, 0.f, 1.f), Def.Axis == 0 ? FVector(1.f, 0.f, 0.f) : FVector(0.f, 1.f, 0.f));
			PieceCS = FTransform(Hand * Along, BoneCS.GetLocation() + Hand.RotateVector(Def.Loc), Def.Size / 100.f);
		}
		else
		{
			const FVector Offset(-Def.Loc.Y, Def.Loc.X, Def.Loc.Z);
			PieceCS = FTransform(MeshFromBody * FQuat(Def.Rot), (bFloor ? FVector::ZeroVector : BoneCS.GetLocation()) + Offset, Def.Size / 100.f);
		}
		const FTransform Rel = bFloor ? PieceCS : PieceCS.GetRelativeTransform(BoneCS);

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
		const bool bOnRoot = bFloor && !bDressParent;
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

void ACastMember::PushActing()
{
	if (UCastAnimInstance* Inst = GetAnim())
	{
		Inst->Acting = Acting;
	}
}

FVector ACastMember::HeadWorld() const
{
	const USkeletalMeshComponent* Mesh = bDressParent ? GetWearer() : Body.Get();
	if (Mesh && Mesh->GetSkeletalMeshAsset() && Mesh->GetBoneIndex(TEXT("head")) != INDEX_NONE)
	{
		return Mesh->GetSocketLocation(TEXT("head"));
	}
	return GetActorLocation() + FVector(0.f, 0.f, bSeated ? 75.f : 160.f);
}

// --- the contract ------------------------------------------------------------------------------------------------------

ACastMember* ACastMember::FindOn(const AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}
	if (ACastMember* Self = const_cast<ACastMember*>(Cast<ACastMember>(Actor)))
	{
		return Self;
	}
	TArray<AActor*> Attached;
	Actor->GetAttachedActors(Attached);
	for (AActor* Each : Attached)
	{
		if (ACastMember* Member = Cast<ACastMember>(Each))
		{
			return Member;
		}
	}
	return nullptr;
}

void ACastMember::WalkTo(const FVector& Target, float Speed)
{
	if (bWalker || bDressParent || bSeated || bBandKit)
	{
		return;
	}
	WalkTarget = FVector(Target.X, Target.Y, GetActorLocation().Z);
	WalkSpeedGoal = FMath::Max(Speed, 10.f);
	bWalkingTo = FVector::Dist2D(WalkTarget, GetActorLocation()) > 2.f;
	if (!bWalkingTo)
	{
		return;
	}
	StoppedAt = -1.f;
	if (!bOnWalkSpace && WalkBlendSpace)
	{
		// The idle's own time carries into the blend space's idle, and the switch cross-fades.
		PlayBase(WalkBlendSpace, true, 1.f, 0.f);
		bOnWalkSpace = true;
	}
	SetActorTickInterval(0.f);
}

void ACastMember::StopWalking()
{
	bWalkingTo = false;
}

void ACastMember::FaceYaw(float Yaw)
{
	TargetYaw = FRotator::NormalizeAxis(Yaw);
	bTurning = FMath::Abs(FMath::FindDeltaAngleDegrees(static_cast<float>(GetActorRotation().Yaw), TargetYaw)) > 0.5f;
	if (bTurning && !bWalker && !bDressParent)
	{
		SetActorTickInterval(0.f);
	}
}

void ACastMember::TickOwnWalk(float DeltaSeconds)
{
	const float Dt = FMath::Min(DeltaSeconds, 0.1f);
	FVector Loc = GetActorLocation();
	float DesiredYaw = TargetYaw;
	if (bWalkingTo)
	{
		FVector To = WalkTarget - Loc;
		To.Z = 0.f;
		const float Dist = static_cast<float>(To.Size());
		// Ease in at WalkAccel, ease out to stop on the point.
		const float Want = FMath::Min(WalkSpeedGoal, FMath::Sqrt(2.f * WalkAccel * Dist));
		WalkSpeedNow = FMath::FInterpConstantTo(WalkSpeedNow, Want, Dt, WalkAccel);
		const float Step = FMath::Min(WalkSpeedNow * Dt, Dist);
		if (Dist > 1.f)
		{
			Loc += To / Dist * Step;
			SetActorLocation(Loc);
			if (Dist > 8.f)
			{
				DesiredYaw = static_cast<float>(To.Rotation().Yaw);
				TargetYaw = DesiredYaw;
			}
		}
		if (Dist - Step < 1.5f && WalkSpeedNow < 25.f)
		{
			bWalkingTo = false;
			WalkSpeedNow = 0.f;
			StoppedAt = GetWorld()->GetTimeSeconds();
		}
	}
	else
	{
		WalkSpeedNow = FMath::FInterpConstantTo(WalkSpeedNow, 0.f, Dt, WalkAccel * 2.f);
	}
	if (bOnWalkSpace)
	{
		if (UAnimSingleNodeInstance* Inst = Body->GetSingleNodeInstance())
		{
			const float Shown = FMath::Max(WalkSpeedNow, ExternalUntil > 0.f ? ExternalSpeed : 0.f);
			Inst->SetBlendSpacePosition(FVector(Shown < 5.f ? 0.f : Shown, 0.f, 0.f));
		}
	}
	// Turning: eased toward the way it goes (or FaceYaw), never faster than TurnRate.
	const float Yaw = static_cast<float>(GetActorRotation().Yaw);
	const float Delta = FMath::FindDeltaAngleDegrees(Yaw, DesiredYaw);
	if (FMath::Abs(Delta) > 0.3f)
	{
		const float Turn = FMath::Clamp(Delta * FMath::Min(1.f, 6.f * Dt), -TurnRate * Dt, TurnRate * Dt);
		SetActorRotation(FRotator(0.f, Yaw + Turn, 0.f));
		bTurning = true;
	}
	else
	{
		bTurning = false;
	}
	// Back to its own idle a moment after it stops.
	if (!bWalkingTo && bOnWalkSpace && RestBase != WalkBlendSpace && StoppedAt >= 0.f && GetWorld()->GetTimeSeconds() - StoppedAt > 0.35f)
	{
		PlayRestBase();
		StoppedAt = -1.f;
	}
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
	const float Speed = static_cast<float>(FVector::Dist2D(Now, LastParentLocation)) / DeltaSeconds;
	LastParentLocation = Now;
	WalkSpeedSmoothed = FMath::FInterpTo(WalkSpeedSmoothed, FMath::Min(Speed, 600.f), DeltaSeconds, 12.f);
	if (UAnimSingleNodeInstance* Inst = Body->GetSingleNodeInstance())
	{
		Inst->SetBlendSpacePosition(FVector(WalkSpeedSmoothed < 8.f ? 0.f : WalkSpeedSmoothed, 0.f, 0.f));
	}
}

// --- chars-locomotion (Nov 15 evening): the pistol ----------------------------------------------------------------------

void ACastMember::ApplyGun()
{
	if (!Gun)
	{
		return;
	}
	USkeletalMeshComponent* Wearer = GetWearer();
	Wearer = Wearer ? Wearer : Body.Get();
	UStaticMesh* Mesh = (bArmed && !GunMesh.IsNull()) ? GunMesh.LoadSynchronous() : nullptr;
	if (!bArmed || !Wearer || !Mesh)
	{
		Gun->SetHiddenInGame(true);
		Gun->SetVisibility(false);
		return;
	}
	if (Gun->GetStaticMesh() != Mesh)
	{
		Gun->SetStaticMesh(Mesh);
	}
	if (Gun->GetAttachParent() != Wearer || Gun->GetAttachSocketName() != GunSocket)
	{
		Gun->AttachToComponent(Wearer, FAttachmentTransformRules::SnapToTargetNotIncludingScale, GunSocket);
	}
	Gun->SetRelativeTransform(GunGrip);
	Gun->SetHiddenInGame(false);
	Gun->SetVisibility(true);
}

void ACastMember::SetArmed(bool bInArmed)
{
	bArmed = bInArmed;
	if (!bArmed && IsAiming())
	{
		StartAct(ECastAct::None, 0.f, false, FVector::ZeroVector);
	}
	ApplyGun();
	TAKE_LOG(TEXT("CAST %s %s"), *GetActorNameOrLabel(), bArmed ? TEXT("draws a pistol") : TEXT("puts the pistol away"));
}

void ACastMember::AimAt(const FVector& WorldPoint)
{
	if (bDressParent)
	{
		TAKE_LOG(TEXT("CAST %s: the crew do not aim"), *GetActorNameOrLabel());
		return;
	}
	if (!bArmed)
	{
		SetArmed(true);
	}
	if (!IsAiming())
	{
		StartAct(ECastAct::Aim, -1.f, !WorldPoint.IsZero(), WorldPoint);
	}
	if (!WorldPoint.IsZero())
	{
		Acting.ActWorld = WorldPoint;
		LookAtPoint(WorldPoint, 1.5f);
		if (!bWalker && !bWalkingTo)
		{
			FaceYaw(static_cast<float>((WorldPoint - GetActorLocation()).Rotation().Yaw));
		}
	}
}

void ACastMember::StopAiming()
{
	if (IsAiming())
	{
		StartAct(ECastAct::None, 0.f, false, FVector::ZeroVector);
	}
}

FVector ACastMember::GetMuzzleLocation() const
{
	if (Gun && bArmed && Gun->GetStaticMesh())
	{
		return Gun->GetComponentTransform().TransformPosition(GunMuzzle);
	}
	const USkeletalMeshComponent* Wearer = GetWearer();
	Wearer = Wearer ? Wearer : Body.Get();
	return Wearer ? Wearer->GetSocketLocation(GunSocket) : GetActorLocation();
}

ECastAct ACastMember::ActFromName(FName Gesture) const
{
	struct FEntry { const TCHAR* Name; ECastAct Act; };
	static const FEntry Entries[] = {
		{TEXT("Refuse"), ECastAct::Refuse}, {TEXT("Stop"), ECastAct::Refuse}, {TEXT("Wave"), ECastAct::Wave},
		{TEXT("WaveThrough"), ECastAct::Wave}, {TEXT("Hello"), ECastAct::Hello}, {TEXT("Applaud"), ECastAct::Applaud},
		{TEXT("Clap"), ECastAct::Applaud}, {TEXT("Talk"), ECastAct::Talk}, {TEXT("Listen"), ECastAct::Listen},
		{TEXT("Laugh"), ECastAct::Laugh}, {TEXT("Sip"), ECastAct::Sip}, {TEXT("Drink"), ECastAct::Sip},
		{TEXT("Nod"), ECastAct::Nod}, {TEXT("Shrug"), ECastAct::Shrug}, {TEXT("Point"), ECastAct::Point},
		{TEXT("CheckWatch"), ECastAct::CheckWatch}, {TEXT("LookAround"), ECastAct::LookAround},
		{TEXT("Headset"), ECastAct::Headset}, {TEXT("Use"), ECastAct::Use}, {TEXT("Lift"), ECastAct::Lift},
		{TEXT("Bow"), ECastAct::Bow}, {TEXT("Speech"), ECastAct::Speech}, {TEXT("Aim"), ECastAct::Aim}};
	for (const FEntry& Entry : Entries)
	{
		if (Gesture == FName(Entry.Name))	// FName compares without case
		{
			return Entry.Act;
		}
	}
	return ECastAct::None;
}

FName ACastMember::ClipKeyFor(ECastAct Act) const
{
	const bool bSit = bSeated;
	switch (Act)
	{
	case ECastAct::Refuse: return TEXT("Refuse");
	case ECastAct::Wave: case ECastAct::Hello: return TEXT("Wave");
	case ECastAct::Applaud: return bSit ? FName(TEXT("SitClap")) : FName(TEXT("Clap"));
	case ECastAct::Talk:
	{
		if (bSit)
		{
			return TEXT("SitTalk");
		}
		static const FName Talks[] = {TEXT("Talk_A"), TEXT("Talk_B"), TEXT("Talk_C")};
		return Talks[(Acting.ActSerial + int32(Seed % 3u)) % 3];
	}
	case ECastAct::Listen: return TEXT("Listen");
	case ECastAct::Laugh: return TEXT("Laugh");
	case ECastAct::Sip: return bSit ? FName(TEXT("SitDrink")) : FName(TEXT("Sip"));
	case ECastAct::Shrug: return TEXT("Shrug");
	case ECastAct::CheckWatch: return TEXT("CheckWatch");
	case ECastAct::LookAround: return TEXT("LookAround");
	case ECastAct::Headset: return TEXT("Headset");
	case ECastAct::Use: return TEXT("Use");
	case ECastAct::Lift: return TEXT("Lift");
	case ECastAct::Bow: return TEXT("Bow");
	case ECastAct::Speech: return Draw(36) < 0.5f ? FName(TEXT("Speech_A")) : FName(TEXT("Speech_B"));
	default: return NAME_None;	// Nod, Point: built from bones
	}
}

void ACastMember::StartAct(ECastAct Act, float Seconds, bool bPoint, const FVector& Point)
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (Act == ECastAct::None)
	{
		Acting.Act = ECastAct::None;
		++Acting.ActSerial;
		ActUntil = Now + Acting.BlendOut;
		bTalking = false;
		PushActing();
		return;
	}
	const FName Key = ClipKeyFor(Act);
	UAnimSequenceBase* Clip = Act == ECastAct::Aim ? CastGun::AimClip() : FindClip(Key);
	if (!Clip && Act == ECastAct::Speech)
	{
		Clip = FindClip(TEXT("Speech_A"));
		Clip = Clip ? Clip : FindClip(TEXT("Speech_B"));
	}
	const bool bSeatedClip = Clip && Key.ToString().StartsWith(TEXT("Sit"));
	const AActor* Parent = GetAttachParentActor();
	const bool bMoving = bWalkingTo || (Parent && Parent->GetVelocity().Size2D() > 20.f) || (bWalker && WalkSpeedSmoothed > 20.f);
	Acting.Act = Act;
	Acting.ActClip = Clip;
	Acting.ActSeconds = Seconds;
	Acting.bActPoint = bPoint;
	Acting.ActWorld = Point;
	Acting.ActRate = 0.94f + 0.12f * Draw(50 + Acting.ActSerial);
	Acting.bActUpperBody = Act == ECastAct::Aim || bWalker || bMoving || bBandKit || (bSeated && !(bSeatedClip && bSeatClip));
	Acting.BlendIn = bDressParent ? 0.15f : 0.3f;
	Acting.BlendOut = bDressParent ? 0.3f : 0.4f;
	if (Act == ECastAct::Aim)
	{
		Acting.ActRate = 1.f;
		Acting.BlendIn = 0.15f;	// the gun comes up fast
		Acting.BlendOut = 0.3f;
	}
	if (Act == ECastAct::Bow)
	{
		Acting.BlendIn = 0.1f;
		Acting.Energy = BowDegrees / 42.f;
		if (!Clip && Seconds == 0.f)
		{
			Acting.ActSeconds = BowSeconds;
		}
	}
	++Acting.ActSerial;
	const float Len = Acting.ActSeconds > 0.f ? Acting.ActSeconds
		: Acting.ActSeconds < 0.f ? 1e6f
		: Clip ? Clip->GetPlayLength() / Acting.ActRate : CastActDefaultSeconds(Act);
	ActUntil = Now + FMath::Max(Len, 0.1f);
	bTalking = Act == ECastAct::Talk || Act == ECastAct::Speech;
	PushActing();
}

void ACastMember::PlayGesture(FName Gesture)
{
	PlayGestureAt(Gesture, FVector::ZeroVector);
	// PlayGestureAt with a zero point means "no point": handled there.
}

void ACastMember::PlayGestureAt(FName Gesture, const FVector& WorldPoint)
{
	const bool bPoint = !WorldPoint.IsZero();
	if (Gesture == FName(TEXT("LookAt")))
	{
		if (bPoint)
		{
			LookAtPoint(WorldPoint, 2.5f);
		}
		else if (bWalker || bDressParent)
		{
			// A watcher looks at Julian (PatrolGuard plays it the frame he starts holding him).
			LookOverrideUntil = (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f) + 2.5f;
			bLookOverrideJulian = true;
		}
		else
		{
			// A guest turns to the screen (the house dimmer's preview).
			LookAtPoint(ScreenPoint, ScreenLookSeconds);
		}
		NextThink = 0.f;
		return;
	}
	const ECastAct Act = ActFromName(Gesture);
	if (Gesture == FName(TEXT("None")))
	{
		if (bDressParent) { EndCrewAct(); } else { StartAct(ECastAct::None, 0.f, false, FVector::ZeroVector); }
		return;
	}
	if (Act == ECastAct::None)
	{
		TAKE_LOG(TEXT("CAST %s: no gesture \"%s\""), *GetActorNameOrLabel(), *Gesture.ToString());
		return;
	}
	if (bDressParent)
	{
		StartCrewAct(Act, bPoint, WorldPoint);
		return;
	}
	StartAct(Act, Act == ECastAct::Applaud ? ApplauseSeconds * (0.8f + 0.4f * FMath::FRand()) : 0.f, bPoint, WorldPoint);
	if (bPoint)
	{
		LookAtPoint(WorldPoint, FMath::Max(ActUntil - static_cast<float>(GetWorld()->GetTimeSeconds()), 1.f));
	}
}

void ACastMember::LookAtPoint(const FVector& WorldPoint, float Seconds)
{
	LookOverridePoint = WorldPoint;
	bLookOverrideJulian = false;
	LookOverrideUntil = (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f) + Seconds;
	Acting.bLook = true;
	Acting.LookWorld = WorldPoint;
	PushActing();
}

bool ACastMember::IsGesturing() const
{
	const UWorld* World = GetWorld();
	return World && World->GetTimeSeconds() < ActUntil;
}

void ACastMember::CrowdGesture(const UObject* WorldContext, FName Gesture, FVector Center, float Radius, float Share)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}
	static int32 Event = 0;
	++Event;
	for (const TWeakObjectPtr<ACastMember>& P : FCastWorld::All())
	{
		ACastMember* M = P.Get();
		if (!M || M->GetWorld() != World || M->bWalker || M->bDressParent || M->bBandKit || !M->IsActingNow()
			|| FVector::Dist(M->GetActorLocation(), Center) > Radius || M->Draw(900 + Event) >= Share)
		{
			continue;
		}
		const ECastAct Act = M->ActFromName(Gesture);
		if (Act == ECastAct::None)
		{
			continue;
		}
		M->PendingAct = Act;
		M->PendingAt = World->GetTimeSeconds() + 0.1f + 0.7f * M->Draw(901 + Event);
		M->PendingSeconds = Act == ECastAct::Applaud ? M->ApplauseSeconds * (0.75f + 0.5f * M->Draw(902 + Event)) : 0.f;
		M->NextThink = 0.f;
	}
}

void ACastMember::BowNow()
{
	if (bDressParent)
	{
		StartCrewAct(ECastAct::Bow, false, FVector::ZeroVector);
		TAKE_LOG(TEXT("CAST BOW %s"), *GetActorNameOrLabel());
	}
}

// --- every frame -------------------------------------------------------------------------------------------------------

void ACastMember::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	++FramesAlive;
	const float Now = GetWorld()->GetTimeSeconds();
	if (bDressParent)
	{
		TickCrew(DeltaSeconds);
		return;
	}
	if (bWalker)
	{
		TickWalk(DeltaSeconds);
	}
	// chars-locomotion: moved by a scene with SetActorLocation (not WalkTo): the legs walk at that speed. Not a body
	// lying down or dragged (pitched or rolled), and not a placement (more than 2.5 m in one tick).
	if (!bWalker && !bSeated && !bBandKit && bHasOwnLocation && !bWalkingTo && DeltaSeconds > 0.f)
	{
		const FVector L = GetActorLocation();
		const FRotator R = GetActorRotation();
		const float D = static_cast<float>(FVector::Dist2D(L, LastOwnLocation));
		const bool bUpright = FMath::Abs(R.Pitch) < 20.f && FMath::Abs(R.Roll) < 20.f;
		if (bUpright && D < 250.f && D / DeltaSeconds > 15.f)
		{
			ExternalSpeed = FMath::FInterpTo(ExternalSpeed, FMath::Min(D / DeltaSeconds, 600.f), FMath::Min(DeltaSeconds, 0.1f), 12.f);
			ExternalUntil = Now + 0.25f;
			if (!bOnWalkSpace && WalkBlendSpace)
			{
				PlayBase(WalkBlendSpace, true, 1.f, 0.f);
				bOnWalkSpace = true;
			}
			StoppedAt = -1.f;
		}
		else if (ExternalUntil > 0.f && Now > ExternalUntil)
		{
			ExternalSpeed = 0.f;
			ExternalUntil = -1.f;
			StoppedAt = Now;
		}
	}
	if (bWalkingTo || bTurning || (bOnWalkSpace && RestBase != WalkBlendSpace))
	{
		TickOwnWalk(DeltaSeconds);
	}
	LastOwnLocation = GetActorLocation();
	bHasOwnLocation = true;
	if (bSeated && !bSeatFitted)
	{
		FitSeat();
	}
	// Step 5's still cast without acting: a seated body or a built band player freezes on its first frame.
	if (!IsActingNow() && !bFrozen && FramesAlive > 3 && !bWalker && (bSeated || Acting.bPlayInstrument))
	{
		bFrozen = true;
	}
	else if (IsActingNow() && bFrozen)
	{
		bFrozen = false;
		SetupBody();	// the idles start again
	}
	const bool bAnimate = UpdateGate();
	if (bAnimate && Now >= NextThink)
	{
		Think(Now);
		NextThink = Now + (bBandKit ? 0.f : ThinkInterval * (0.7f + 0.6f * FMath::FRand()));
	}
	const bool bEveryFrame = bWalker || bWalkingTo || bTurning || bBandKit || ExternalUntil > 0.f || (bOnWalkSpace && RestBase != WalkBlendSpace);
	const float Interval = bEveryFrame ? 0.f : FMath::Min(ThinkInterval, 0.1f);
	if (!FMath::IsNearlyEqual(PrimaryActorTick.TickInterval, Interval))
	{
		SetActorTickInterval(Interval);
	}
}

bool ACastMember::UpdateGate()
{
	bool bWant = !bFrozen;
	if (bWant && !bWalker && !bWalkingTo)
	{
		const APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0);
		const FVector Eye = Cam ? Cam->GetCameraLocation() : GetActorLocation();
		const float Dist = static_cast<float>(FVector::Dist(Eye, GetActorLocation()));
		bWant = Dist < AnimateDistance && (Dist < 500.f || Body->WasRecentlyRendered(0.3f));
	}
	if (bWant != bBodyAnimating)
	{
		bBodyAnimating = bWant;
		Body->SetComponentTickEnabled(bWant);
	}
	// With its tick off a body no longer picks its LOD: do it here, refreshing the bones only on a change.
	if (!bWant && Body->UpdateLODStatus())
	{
		Body->RefreshBoneTransforms();
	}
	return bWant;
}

// --- thinking ----------------------------------------------------------------------------------------------------------

void ACastMember::Think(float Now)
{
	Acting.bActing = IsActingNow();
	if (!Acting.bActing)
	{
		Acting.bLook = false;
		PushActing();
		return;
	}
	FCastWorld::EnsureGroups();
	if (PendingAct != ECastAct::None && Now >= PendingAt)
	{
		StartAct(PendingAct, PendingSeconds, false, FVector::ZeroVector);
		PendingAct = ECastAct::None;
	}
	if (bBandKit)
	{
		ThinkBand();
	}
	else if (bWalker)
	{
		ThinkWatcher(Now);
	}
	else if (bSeated)
	{
		ThinkSeated(Now);
	}
	else
	{
		ThinkStanding(Now);
	}
	ThinkLook(Now);
	PushActing();
}

void ACastMember::ThinkLook(float Now)
{
	ATakeDirector* Director = ATakeDirector::Get(this);
	ATakeCharacter* Julian = Director ? Director->GetCharacter(ETakeCamera::A) : nullptr;
	auto JulianHead = [Julian]() {
		return Julian->GetMesh() ? Julian->GetMesh()->GetSocketLocation(TEXT("head")) : Julian->GetActorLocation() + FVector(0.f, 0.f, 70.f);
	};

	// A glance at Julian as he walks past (not every guest: GlanceChance, each a little late).
	if (Julian && !bWalker)
	{
		const float D = static_cast<float>(FVector::Dist2D(Julian->GetActorLocation(), GetActorLocation()));
		const bool bPassing = D < GlanceRadius && Julian->GetVelocity().Size2D() > 40.f && !Julian->IsHidden();
		if (bPassing && !bJulianNear)
		{
			bJulianNear = true;
			if (Draw(200 + GlanceCount++) < GlanceChance)
			{
				GlanceFrom = Now + 0.2f + 0.5f * FMath::FRand();
				GlanceUntil = GlanceFrom + 1.4f + 1.4f * FMath::FRand();
			}
		}
		else if (D > GlanceRadius * 1.3f)
		{
			bJulianNear = false;
		}
	}

	bool bLook = false;
	FVector Point = FVector::ZeroVector;
	if (Now < LookOverrideUntil)
	{
		if (bLookOverrideJulian && Julian)
		{
			Point = JulianHead();
			bLook = true;
		}
		else if (!bLookOverrideJulian)
		{
			Point = LookOverridePoint;
			bLook = true;
		}
	}
	else if (Julian && Now >= GlanceFrom && Now < GlanceUntil)
	{
		Point = JulianHead();
		bLook = true;
	}
	else
	{
		switch (LookMode)
		{
		case 1: Point = LookFixed; bLook = true; break;
		case 2:
		{
			const ACastMember* Vance = FCastWorld::Find(ECastKit::Vance);
			Point = Vance ? Vance->HeadWorld() : PodiumPoint;
			bLook = true;
			break;
		}
		case 3:
			if (const ACastMember* Other = LookMember.Get())
			{
				Point = Other->HeadWorld();
				bLook = true;
			}
			break;
		case 4:
			if (Julian)
			{
				Point = JulianHead();
				bLook = true;
			}
			break;
		default:
			break;
		}
	}
	Acting.bLook = bLook;
	if (bLook)
	{
		Acting.LookWorld = Point;
	}
}

void ACastMember::ThinkSeated(float Now)
{
	const ACastMember* Vance = FCastWorld::Find(ECastKit::Vance);
	const bool bSpeech = Vance && Vance->bTalking;
	if (Now >= NextLookChange)
	{
		const float R = FMath::FRand();
		ACastMember* Neighbour = Neighbours.Num() > 0 ? Neighbours[FMath::RandRange(0, Neighbours.Num() - 1)].Get() : nullptr;
		if (bSpeech ? R < 0.85f : R < 0.45f)
		{
			LookMode = 2;
		}
		else if (Neighbour && R < (bSpeech ? 0.95f : 0.75f))
		{
			LookMode = 3;
			LookMember = Neighbour;
			// A word with the neighbour, who looks back.
			if (!bSpeech && !IsGesturing() && FMath::FRand() < 0.35f)
			{
				StartAct(ECastAct::Talk, 2.f + 2.f * FMath::FRand(), false, FVector::ZeroVector);
				if (!Neighbour->IsGesturing())
				{
					Neighbour->LookMode = 3;
					Neighbour->LookMember = this;
					Neighbour->NextLookChange = Now + 3.5f;
				}
			}
		}
		else if (R < 0.85f)
		{
			LookMode = 1;
			LookFixed = GetActorLocation() + GetActorForwardVector() * 600.f + FVector(0.f, 0.f, 60.f + 80.f * FMath::FRand())
				+ GetActorRightVector() * (FMath::FRand() * 400.f - 200.f);
		}
		else
		{
			LookMode = 0;
		}
		NextLookChange = Now + 2.5f + 5.f * FMath::FRand();
	}
	if (Now >= NextLean)
	{
		const float K = bSeatClip ? 0.5f : 1.f;
		Acting.LeanForward = K * (FMath::FRand() * 8.f - 2.f);
		Acting.LeanSide = K * (FMath::FRand() * 6.f - 3.f);
		Acting.Twist = K * (FMath::FRand() * 10.f - 5.f);
		NextLean = Now + 6.f + 8.f * FMath::FRand();
	}
}

void ACastMember::ThinkStanding(float Now)
{
	if (bWalkingTo)
	{
		LookMode = 1;
		LookFixed = WalkTarget + FVector(0.f, 0.f, 160.f);
		return;
	}
	const bool bStaff = Kit == ECastKit::Staff;
	const int32 Size = Group.Num();
	if (Size >= 2 && !bStaff)
	{
		// A conversation: the group shares a clock, so all of them agree whose turn it is.
		const float Offset = float(GroupKey % 1000u) / 1000.f * TurnSeconds;
		const int32 Turn = FMath::FloorToInt((Now + Offset) / TurnSeconds);
		const int32 SpeakerIndex = FMath::Min(int32(CastKits::Draw(GroupKey, uint32(Turn)) * Size), Size - 1);
		ACastMember* Speaker = Group[SpeakerIndex].Get();
		if (Turn != LastTurn)
		{
			LastTurn = Turn;
			if (Speaker == this)
			{
				if (!IsGesturing())
				{
					StartAct(ECastAct::Talk, TurnSeconds - 0.6f, false, FVector::ZeroVector);
				}
				ACastMember* Other = Group[(SpeakerIndex + 1 + FMath::RandRange(0, Size - 2)) % Size].Get();
				LookMode = Other ? 3 : 0;
				LookMember = Other;
				NextLookChange = Now + 1.8f + 1.5f * FMath::FRand();
			}
			else
			{
				LookMode = Speaker ? 3 : 0;
				LookMember = Speaker;
				NextLookChange = Now + TurnSeconds;
				const float R = FMath::FRand();
				if (!IsGesturing() && PendingAct == ECastAct::None)
				{
					PendingAct = R < 0.25f ? ECastAct::Nod : R < 0.35f ? ECastAct::Laugh : R < 0.5f ? ECastAct::Listen : ECastAct::None;
					PendingAt = Now + 0.6f + 1.8f * FMath::FRand();
					PendingSeconds = 0.f;
				}
			}
		}
		else if (Speaker == this && Now >= NextLookChange)
		{
			ACastMember* Other = Group[(SpeakerIndex + 1 + FMath::RandRange(0, Size - 2)) % Size].Get();
			LookMode = Other ? 3 : 0;
			LookMember = Other;
			NextLookChange = Now + 1.8f + 1.5f * FMath::FRand();
		}
	}
	else if (Now >= NextLookChange)
	{
		// Alone, or in the queue: looks about (the door, the street), sometimes the time.
		const float R = FMath::FRand();
		if (R < 0.5f)
		{
			LookMode = 1;
			LookFixed = GetActorLocation() + GetActorForwardVector().RotateAngleAxis(FMath::FRand() * 120.f - 60.f, FVector::UpVector) * 500.f
				+ FVector(0.f, 0.f, 150.f + 60.f * FMath::FRand());
		}
		else
		{
			LookMode = 0;
		}
		NextLookChange = Now + 3.f + 5.f * FMath::FRand();
		if (!bStaff && !IsGesturing() && Now >= NextIdleAct)
		{
			StartAct(FMath::FRand() < 0.55f ? ECastAct::LookAround : ECastAct::CheckWatch, 0.f, false, FVector::ZeroVector);
			NextIdleAct = Now + 8.f + 10.f * FMath::FRand();
		}
	}
	if (bHoldsGlass && !IsGesturing() && !bTalking && Now >= NextIdleAct)
	{
		StartAct(ECastAct::Sip, 0.f, false, FVector::ZeroVector);
		NextIdleAct = Now + 9.f + 13.f * FMath::FRand();
	}
	if (Now >= NextLean)
	{
		const float K = bRestIsClip ? 0.4f : 1.f;
		Acting.LeanForward = K * (FMath::FRand() * 4.f - 1.f);
		Acting.LeanSide = K * (FMath::FRand() * 4.f - 2.f);
		Acting.Twist = K * (FMath::FRand() * 8.f - 4.f);
		NextLean = Now + 5.f + 7.f * FMath::FRand();
	}
}

void ACastMember::ThinkWatcher(float Now)
{
	// Vance: speech gestures while he speaks, a hand on the podium; the hall applauds when he stops.
	if (Kit == ECastKit::Vance)
	{
		const AWatcher* Watcher = Cast<AWatcher>(GetAttachParentActor());
		const bool bSpeaking = Watcher && Watcher->GetStateText().Contains(TEXT("SPEAK"));
		if (bSpeaking && !bVanceSpoke)
		{
			Acting.Hold = ECastHold::Podium;
			StartAct(ECastAct::Speech, -1.f, false, FVector::ZeroVector);
		}
		else if (!bSpeaking && bVanceSpoke)
		{
			Acting.Hold = ECastHold::ClaspBack;
			StartAct(ECastAct::None, 0.f, false, FVector::ZeroVector);
			CrowdGesture(this, TEXT("Applaud"), GetActorLocation(), 3500.f, SpeechCount == 0 ? ApplauseShare : ApplauseShare * 0.5f);
			++SpeechCount;
		}
		bVanceSpoke = bSpeaking;
	}
	// The head scans, lingering at each end; a guard on the move scans narrower.
	const float Moving = WalkSpeedSmoothed > 30.f ? 0.8f : 1.f;
	const float S = FMath::Sin((Now / FMath::Max(ScanPeriod, 1.f) + Draw(37)) * 6.2832f);
	const float Angle = ScanAmplitude * Moving * FMath::Sign(S) * FMath::Sqrt(FMath::Abs(S));
	const FVector Ahead = GetActorForwardVector().RotateAngleAxis(Angle, FVector::UpVector);
	LookMode = 1;
	LookFixed = HeadWorld() + Ahead * 600.f - FVector(0.f, 0.f, Kit == ECastKit::Vance && bTalking ? 60.f : 30.f);
}

void ACastMember::ThinkBand()
{
	const ATakeDirector* Director = ATakeDirector::Get(this);
	if (Director && Director->Clock)
	{
		Acting.Beat = float(Director->Clock->GetBar() * 4 + Director->Clock->GetBeatInBar()) + Director->Clock->GetBeatPhase();
	}
	else
	{
		Acting.Beat = GetWorld()->GetTimeSeconds() * 2.f;
	}
	LookMode = 0;
}

// --- the crew ----------------------------------------------------------------------------------------------------------

void ACastMember::StartCrewAct(ECastAct Act, bool bPoint, const FVector& Point)
{
	USkeletalMeshComponent* Wearer = GetWearer();
	if (!Wearer || !Wearer->GetSkeletalMeshAsset() || !Wearer->GetAttachParent())
	{
		return;
	}
	if (!bCrewAct)
	{
		// Our body becomes the character's: same mesh, materials and place, its pose copied every frame.
		if (Body->GetSkeletalMeshAsset() != Wearer->GetSkeletalMeshAsset())
		{
			Body->SetSkeletalMeshAsset(Wearer->GetSkeletalMeshAsset());
		}
		if (Body->GetAttachParent() != Wearer->GetAttachParent())
		{
			Body->AttachToComponent(Wearer->GetAttachParent(), FAttachmentTransformRules::KeepRelativeTransform);
		}
		Body->SetRelativeTransform(Wearer->GetRelativeTransform());
		for (int32 Slot = 0; Slot < Wearer->GetNumMaterials(); ++Slot)
		{
			Body->SetMaterial(Slot, Wearer->GetMaterial(Slot));
		}
		Body->LightingChannels = Wearer->LightingChannels;
		Body->SetCastShadow(Wearer->CastShadow);
		Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		Body->SetComponentTickEnabled(true);
		Acting.CopyFrom = Wearer;
		if (!GetAnim())
		{
			Body->AnimationData.AnimToPlay = nullptr;
			Body->SetAnimInstanceClass(UCastAnimInstance::StaticClass());
		}
		++Acting.BaseSerial;
	}
	if (Act != ECastAct::None)
	{
		StartAct(Act, 0.f, bPoint, Point);
		CrewActUntil = ActUntil + Acting.BlendOut;
	}
	PushHeroRest();
	if (!bCrewAct)
	{
		// The first frame is the character's pose already: no flash of the reference pose.
		Body->TickAnimation(0.f, false);
		Body->RefreshBoneTransforms();
		WearerTickOption = uint8(Wearer->VisibilityBasedAnimTickOption);
		Wearer->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		ReattachPieces(Body);
		Body->SetVisibility(true);
		Wearer->SetVisibility(false);
		bCrewAct = true;
	}
	TAKE_LOG(TEXT("CAST ACT %s %s"), *GetActorNameOrLabel(), *UEnum::GetValueAsString(Act));
}

void ACastMember::EndCrewAct()
{
	USkeletalMeshComponent* Wearer = GetWearer();
	if (bCrewAct && Wearer)
	{
		ReattachPieces(Wearer);
		Wearer->SetVisibility(true);
		Wearer->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption(WearerTickOption);
	}
	Body->SetVisibility(false);
	Body->SetComponentTickEnabled(false);
	Acting.CopyFrom = nullptr;
	Acting.Act = ECastAct::None;
	++Acting.ActSerial;
	PushActing();
	bCrewAct = false;
}

void ACastMember::TickCrew(float DeltaSeconds)
{
	ATakeDirector* Director = ATakeDirector::Get(this);
	const float Now = GetWorld()->GetTimeSeconds();
	const ATakeCharacter* Character = Cast<ATakeCharacter>(GetAttachParentActor());

	// Only Julian bows (The Bow is his deleted scene).
	const bool bFired = Kit == ECastKit::Julian && Director && BowTrigger != NAME_None && Director->HasFired(BowTrigger);
	if (bFired && !bBowFiredLast)
	{
		BowNow();
	}
	bBowFiredLast = bFired;

	// A hand to the headset when a voice line starts, on the character the player is on.
	// --- W3 cast2 --- only the crew who wear a headset (Maya, Vex): on Julian the hand to the ear read as a half wave.
	if (Director && Character && bHeadsetOnVoice && Kit != ECastKit::Julian && IsActingNow())
	{
		const float Left = Director->VoiceSecondsLeft();
		if (Left > 0.f && VoiceLeftLast <= 0.f && Now >= NextHeadset && (!bCrewAct || Now >= CrewActUntil) && Director->GetLookCamera() == Character->Id)
		{
			StartCrewAct(ECastAct::Headset, false, FVector::ZeroVector);
			NextHeadset = Now + 6.f;
		}
		VoiceLeftLast = Left;
	}

	// --- W3 cast2 --- a lead on a hero mesh keeps the copied body (the arms brought to rest over its animation).
	const bool bKeep = WantsHeroRest();
	if (bKeep && !bCrewAct)
	{
		StartCrewAct(ECastAct::None, false, FVector::ZeroVector);
	}
	else if (!bKeep && bCrewAct && Now >= CrewActUntil && Acting.Act == ECastAct::None)
	{
		EndCrewAct();
	}
	if (!bCrewAct)
	{
		return;
	}
	PushHeroRest();
	const float Speed = Character ? static_cast<float>(Character->GetVelocity().Size2D()) : 0.f;
	if (Acting.Act == ECastAct::Bow && Speed > BowCancelSpeed)
	{
		StartAct(ECastAct::None, 0.f, false, FVector::ZeroVector);
		CrewActUntil = Now + Acting.BlendOut;
	}
	Acting.bActUpperBody = Speed > 20.f;
	PushActing();
	if (Now >= CrewActUntil && !bKeep)
	{
		EndCrewAct();
		TAKE_LOG(TEXT("CAST ACT END %s"), *GetActorNameOrLabel());
	}
}

bool ACastMember::WantsHeroRest() const
{
	const ATakeCharacter* Character = Cast<ATakeCharacter>(GetAttachParentActor());
	return bHeroRestArms && IsActingNow() && Character && !Character->HeroMesh.IsNull();
}

void ACastMember::PushHeroRest()
{
	const bool bOn = WantsHeroRest();
	Acting.HeroArmIn = bOn ? HeroArmIn : 0.f;
	Acting.HeroElbowBend = bOn ? HeroElbowBend : 0.f;
	Acting.HeroPalmTurn = bOn ? HeroPalmTurn : 0.f;
}
