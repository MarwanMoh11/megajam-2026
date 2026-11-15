// Geometry for the reel swap's props, built in code (no imported meshes): the 1970s booth projector with its upper and
// lower magazines, the 35 mm reels (six round windows in each flange), the silver film canister, its lid and latch,
// the rewind bench, and the beam. Each part is a procedural mesh; each material slot is one Scene7SwapMesh::ESlot.
// Owner: s7-reelswap.

#pragma once

#include "CoreMinimal.h"

class UProceduralMeshComponent;

namespace Scene7SwapMesh
{
	/** Material slots (AScene7Swap gives each one a colour). */
	enum ESlot : int32
	{
		Paint,		// the projector's enamel
		Iron,		// dark cast iron and black parts
		Chrome,		// bright metal: reels, knobs, the can
		Glass,		// lens glass and the door's window
		Film,		// the film pack on a reel
		LabelRed,	// the master's red label
		LabelPale,	// the dummy's pale label
		Wood,		// the bench top
		Interior,	// inside the magazine
		Beam,		// the light beam (additive)
		Count
	};

	struct FSection
	{
		TArray<FVector> V;
		TArray<int32> T;
		TArray<FVector> N;
		TArray<FVector2D> UV;
	};

	struct FBuilder
	{
		FSection S[Count];
		ESlot Slot = Paint;

		void Tri(const FVector& A, const FVector& B, const FVector& C, const FVector& NA, const FVector& NB, const FVector& NC);
		/** A flat quad; the winding is fixed up so the face points along N. */
		void Quad(const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& N);
		void Box(const FVector& Center, const FVector& Half, const FQuat& Rot = FQuat::Identity);
		/** A cylinder along Axis from Center - Axis*Len/2 to Center + Axis*Len/2. */
		void Cylinder(const FVector& Center, const FVector& Axis, float Radius, float Len, int32 Segs = 32, bool bCaps = true);
		/** A pipe (annulus) with walls inside and out. */
		void Tube(const FVector& Center, const FVector& Axis, float ROuter, float RInner, float Len, int32 Segs = 48);
		/** A cone from Apex along Axis, radius RFar at distance Len (open both ends). */
		void Cone(const FVector& Apex, const FVector& Axis, float RNear, float RFar, float Len, int32 Segs = 32);
		/** A reel flange: a disc of Thick with a hub hole and Holes round windows on a ring. */
		void Flange(const FVector& Center, const FVector& Axis, float ROuter, float RHub, float Thick, int32 Holes, float HoleR, float HoleRing, float Phase = 0.f);

		void Commit(UProceduralMeshComponent* Mesh) const;
	};

	/** The parts (in each part's own space; see the .cpp for where the pivots are). */
	void BuildProjector(FBuilder& B);
	void BuildMagazineDoor(FBuilder& B);
	void BuildDoorLatch(FBuilder& B);
	void BuildReel(FBuilder& B, bool bMaster, float FilmRadius);
	void BuildFilmStrand(FBuilder& B);
	void BuildCan(FBuilder& B);
	void BuildCanLid(FBuilder& B);
	void BuildCanLatch(FBuilder& B);
	void BuildBench(FBuilder& B);
	void BuildBeam(FBuilder& B, float Length);

	// ---- the measurements the actor needs (cm, projector space: +X the lens, -Y the operator's side, Z up) ----
	/** The upper magazine's spindle (the reel's centre). */
	inline const FVector UpperCenter(2.f, -1.f, 178.f);
	inline constexpr float MagRadius = 27.f;
	/** The magazine's open face (operator side) and the door's hinge (front edge, vertical axis). */
	inline constexpr float MagFaceY = -9.f;
	inline const FVector DoorHinge(2.f + 28.5f, -9.8f, 178.f);
	/** The door latch's place on the door (door space: the door hangs from the hinge toward -X). */
	inline const FVector DoorLatchLocal(-55.5f, -1.6f, 0.f);
	/** The reel: flange radius, the flanges' gap, and where the reel sits on the spindle (Y). */
	inline constexpr float ReelRadius = 21.f;
	inline constexpr float ReelWidth = 4.6f;
	inline constexpr float ReelY = -2.f;
	/** The lens's front, where the beam starts. */
	inline const FVector LensFront(55.f, -1.f, 120.f);
	/** The fire-valve throat under the upper magazine (where the film goes down into the head). */
	inline const FVector Throat(2.f, -2.f, 150.f);
	/** The bench top's height, and the can's size. */
	inline constexpr float BenchTop = 92.f;
	inline constexpr float CanRadius = 23.f;
	inline constexpr float CanDepth = 5.5f;
}
