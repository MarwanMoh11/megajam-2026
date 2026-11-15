#include "Script/Scene7SwapMesh.h"
#include "ProceduralMeshComponent.h"

namespace Scene7SwapMesh
{
	static void Basis(const FVector& Axis, FVector& U, FVector& W)
	{
		const FVector A = Axis.GetSafeNormal();
		const FVector Ref = FMath::Abs(A.Z) < 0.9f ? FVector::UpVector : FVector::ForwardVector;
		U = FVector::CrossProduct(A, Ref).GetSafeNormal();
		W = FVector::CrossProduct(A, U).GetSafeNormal();
	}

	void FBuilder::Tri(const FVector& A, const FVector& B, const FVector& C, const FVector& NA, const FVector& NB, const FVector& NC)
	{
		FSection& Sec = S[Slot];
		const int32 I = Sec.V.Num();
		// The engine's front face: (B-A)x(C-A) points away from the normal (see GenerateBoxMesh).
		const bool bFlip = FVector::DotProduct(FVector::CrossProduct(B - A, C - A), NA + NB + NC) > 0.f;
		Sec.V.Add(A); Sec.N.Add(NA);
		Sec.V.Add(bFlip ? C : B); Sec.N.Add(bFlip ? NC : NB);
		Sec.V.Add(bFlip ? B : C); Sec.N.Add(bFlip ? NB : NC);
		// A cheap planar UV (the materials are flat colours; this only keeps the tangents sane).
		for (int32 K = 0; K < 3; ++K)
		{
			const FVector& P = Sec.V[I + K];
			Sec.UV.Add(FVector2D(P.X + P.Y, P.Z) * 0.01f);
		}
		Sec.T.Add(I); Sec.T.Add(I + 1); Sec.T.Add(I + 2);
	}

	void FBuilder::Quad(const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& N)
	{
		Tri(A, B, C, N, N, N);
		Tri(A, C, D, N, N, N);
	}

	void FBuilder::Box(const FVector& Center, const FVector& Half, const FQuat& Rot)
	{
		auto P = [&](float X, float Y, float Z) { return Center + Rot.RotateVector(FVector(X * Half.X, Y * Half.Y, Z * Half.Z)); };
		const FVector X = Rot.RotateVector(FVector::ForwardVector), Y = Rot.RotateVector(FVector::RightVector), Z = Rot.RotateVector(FVector::UpVector);
		Quad(P(1, -1, -1), P(1, 1, -1), P(1, 1, 1), P(1, -1, 1), X);
		Quad(P(-1, -1, -1), P(-1, -1, 1), P(-1, 1, 1), P(-1, 1, -1), -X);
		Quad(P(-1, 1, -1), P(-1, 1, 1), P(1, 1, 1), P(1, 1, -1), Y);
		Quad(P(-1, -1, -1), P(1, -1, -1), P(1, -1, 1), P(-1, -1, 1), -Y);
		Quad(P(-1, -1, 1), P(1, -1, 1), P(1, 1, 1), P(-1, 1, 1), Z);
		Quad(P(-1, -1, -1), P(-1, 1, -1), P(1, 1, -1), P(1, -1, -1), -Z);
	}

	void FBuilder::Cylinder(const FVector& Center, const FVector& Axis, float Radius, float Len, int32 Segs, bool bCaps)
	{
		FVector U, W;
		Basis(Axis, U, W);
		const FVector A = Axis.GetSafeNormal();
		const FVector C0 = Center - A * Len * 0.5f, C1 = Center + A * Len * 0.5f;
		for (int32 I = 0; I < Segs; ++I)
		{
			const float T0 = 2.f * PI * I / Segs, T1 = 2.f * PI * (I + 1) / Segs;
			const FVector R0 = U * FMath::Cos(T0) + W * FMath::Sin(T0);
			const FVector R1 = U * FMath::Cos(T1) + W * FMath::Sin(T1);
			Tri(C0 + R0 * Radius, C0 + R1 * Radius, C1 + R1 * Radius, R0, R1, R1);
			Tri(C0 + R0 * Radius, C1 + R1 * Radius, C1 + R0 * Radius, R0, R1, R0);
			if (bCaps)
			{
				Tri(C1, C1 + R0 * Radius, C1 + R1 * Radius, A, A, A);
				Tri(C0, C0 + R1 * Radius, C0 + R0 * Radius, -A, -A, -A);
			}
		}
	}

	void FBuilder::Tube(const FVector& Center, const FVector& Axis, float ROuter, float RInner, float Len, int32 Segs)
	{
		FVector U, W;
		Basis(Axis, U, W);
		const FVector A = Axis.GetSafeNormal();
		const FVector C0 = Center - A * Len * 0.5f, C1 = Center + A * Len * 0.5f;
		for (int32 I = 0; I < Segs; ++I)
		{
			const float T0 = 2.f * PI * I / Segs, T1 = 2.f * PI * (I + 1) / Segs;
			const FVector R0 = U * FMath::Cos(T0) + W * FMath::Sin(T0);
			const FVector R1 = U * FMath::Cos(T1) + W * FMath::Sin(T1);
			Tri(C0 + R0 * ROuter, C0 + R1 * ROuter, C1 + R1 * ROuter, R0, R1, R1);
			Tri(C0 + R0 * ROuter, C1 + R1 * ROuter, C1 + R0 * ROuter, R0, R1, R0);
			Tri(C0 + R0 * RInner, C1 + R1 * RInner, C0 + R1 * RInner, -R0, -R1, -R1);
			Tri(C0 + R0 * RInner, C1 + R0 * RInner, C1 + R1 * RInner, -R0, -R0, -R1);
			Quad(C1 + R0 * RInner, C1 + R0 * ROuter, C1 + R1 * ROuter, C1 + R1 * RInner, A);
			Quad(C0 + R0 * RInner, C0 + R1 * RInner, C0 + R1 * ROuter, C0 + R0 * ROuter, -A);
		}
	}

	void FBuilder::Cone(const FVector& Apex, const FVector& Axis, float RNear, float RFar, float Len, int32 Segs)
	{
		FVector U, W;
		Basis(Axis, U, W);
		const FVector A = Axis.GetSafeNormal();
		const FVector C1 = Apex + A * Len;
		for (int32 I = 0; I < Segs; ++I)
		{
			const float T0 = 2.f * PI * I / Segs, T1 = 2.f * PI * (I + 1) / Segs;
			const FVector R0 = U * FMath::Cos(T0) + W * FMath::Sin(T0);
			const FVector R1 = U * FMath::Cos(T1) + W * FMath::Sin(T1);
			Tri(Apex + R0 * RNear, Apex + R1 * RNear, C1 + R1 * RFar, R0, R1, R1);
			Tri(Apex + R0 * RNear, C1 + R1 * RFar, C1 + R0 * RFar, R0, R1, R0);
		}
	}

	void FBuilder::Flange(const FVector& Center, const FVector& Axis, float ROuter, float RHub, float Thick, int32 Holes, float HoleR, float HoleRing, float Phase)
	{
		FVector U, W;
		Basis(Axis, U, W);
		const FVector A = Axis.GetSafeNormal();
		const FVector F = Center + A * Thick * 0.5f, Bk = Center - A * Thick * 0.5f;
		const float Alpha = FMath::Asin(FMath::Clamp(HoleR / HoleRing, 0.f, 1.f));
		const float Rest = HoleRing * FMath::Cos(Alpha);

		// The rays: even ones plus each window's two tangents, so the windows close exactly.
		TArray<float> Rays;
		const int32 Even = 192;
		for (int32 I = 0; I < Even; ++I) { Rays.Add(2.f * PI * I / Even); }
		for (int32 K = 0; K < Holes; ++K)
		{
			const float C = Phase + 2.f * PI * K / Holes;
			Rays.Add(FMath::Fmod(C - Alpha + 4.f * PI, 2.f * PI));
			Rays.Add(FMath::Fmod(C + Alpha + 4.f * PI, 2.f * PI));
		}
		Rays.Sort();

		struct FRay { float T; float A; float B; int32 Hole; };
		TArray<FRay> R;
		for (float T : Rays)
		{
			FRay Ray{ T, Rest, Rest, -1 };
			for (int32 K = 0; K < Holes; ++K)
			{
				const float C = Phase + 2.f * PI * K / Holes;
				const float D = FMath::UnwindRadians(T - C);
				if (FMath::Abs(D) <= Alpha + 1e-4f)
				{
					const float Mid = HoleRing * FMath::Cos(D);
					const float Half = FMath::Sqrt(FMath::Max(0.f, HoleR * HoleR - FMath::Square(HoleRing * FMath::Sin(D))));
					Ray = { T, Mid - Half, Mid + Half, K };
				}
			}
			R.Add(Ray);
		}

		auto Dir = [&](float T) { return U * FMath::Cos(T) + W * FMath::Sin(T); };
		for (int32 I = 0; I < R.Num(); ++I)
		{
			const FRay& R0 = R[I];
			const FRay& R1 = R[(I + 1) % R.Num()];
			const FVector D0 = Dir(R0.T), D1 = Dir(R1.T);
			// Faces: hub..A and B..outer, front and back.
			Quad(F + D0 * RHub, F + D1 * RHub, F + D1 * R1.A, F + D0 * R0.A, A);
			Quad(F + D0 * R0.B, F + D1 * R1.B, F + D1 * ROuter, F + D0 * ROuter, A);
			Quad(Bk + D0 * RHub, Bk + D0 * R0.A, Bk + D1 * R1.A, Bk + D1 * RHub, -A);
			Quad(Bk + D0 * R0.B, Bk + D0 * ROuter, Bk + D1 * ROuter, Bk + D1 * R1.B, -A);
			// The rim and the hub's bore.
			const FVector N0 = (D0 + D1).GetSafeNormal();
			Quad(Bk + D0 * ROuter, F + D0 * ROuter, F + D1 * ROuter, Bk + D1 * ROuter, N0);
			Quad(Bk + D0 * RHub, Bk + D1 * RHub, F + D1 * RHub, F + D0 * RHub, -N0);
			// The window's walls.
			const int32 Hole = R0.Hole >= 0 ? R0.Hole : R1.Hole;
			if (Hole >= 0 && (R0.B - R0.A > 1e-3f || R1.B - R1.A > 1e-3f))
			{
				const float C = Phase + 2.f * PI * Hole / Holes;
				const FVector HC = Center + Dir(C) * HoleRing;
				const FVector PA = Center + (D0 * R0.A + D1 * R1.A) * 0.5f;
				const FVector PB = Center + (D0 * R0.B + D1 * R1.B) * 0.5f;
				Quad(Bk + D0 * R0.A, Bk + D1 * R1.A, F + D1 * R1.A, F + D0 * R0.A, (HC - PA).GetSafeNormal());
				Quad(Bk + D0 * R0.B, F + D0 * R0.B, F + D1 * R1.B, Bk + D1 * R1.B, (HC - PB).GetSafeNormal());
			}
		}
	}

	void FBuilder::Commit(UProceduralMeshComponent* Mesh) const
	{
		if (!Mesh) { return; }
		Mesh->ClearAllMeshSections();
		for (int32 I = 0; I < Count; ++I)
		{
			if (S[I].V.Num() == 0) { continue; }
			Mesh->CreateMeshSection_LinearColor(I, S[I].V, S[I].T, S[I].N, S[I].UV, {}, {}, false);
		}
	}

	// ---- the parts -------------------------------------------------------------------------------------------------

	void BuildProjector(FBuilder& B)
	{
		const FVector Y = FVector::RightVector, X = FVector::ForwardVector, Z = FVector::UpVector;

		// Pedestal: base plate, plinth, column, the column's top flange.
		B.Slot = Iron;
		B.Box(FVector(-15, 0, 4), FVector(58, 32, 4));
		B.Box(FVector(-30, 0, 11), FVector(32, 24, 3));
		B.Box(FVector(-30, 0, 50), FVector(16, 14, 36));
		B.Box(FVector(-30, 0, 88), FVector(26, 22, 2.5f));
		B.Box(FVector(-30, -14.6f, 50), FVector(10, 0.8f, 24));	// a cast panel on the column
		// Lamphouse at the back, its chimney and vents.
		B.Slot = Paint;
		B.Box(FVector(-56, 0, 118), FVector(28, 19, 24));
		B.Cylinder(FVector(-85, 0, 118), X, 15, 4, 40);
		B.Slot = Iron;
		B.Cylinder(FVector(-60, 0, 152), Z, 8, 20, 28);
		B.Cylinder(FVector(-60, 0, 164), Z, 11, 3, 28);
		for (int32 I = 0; I < 5; ++I) { B.Box(FVector(-74 + I * 7, -19.6f, 126), FVector(2.2f, 0.8f, 8)); }
		for (int32 I = 0; I < 5; ++I) { B.Box(FVector(-74 + I * 7, 19.6f, 126), FVector(2.2f, 0.8f, 8)); }
		B.Box(FVector(-56, 0, 142.5f), FVector(29, 20, 1));
		B.Slot = Chrome;
		B.Box(FVector(-48, -19.4f, 106), FVector(9, 0.5f, 2.5f));	// the maker's plate
		B.Cylinder(FVector(-80, -19.8f, 112), Y, 2.2f, 2, 16);		// the lamphouse latch

		// The head (the mechanism): the operator's door, its knob and hinges, the top plate.
		B.Slot = Paint;
		B.Box(FVector(0, -1, 116), FVector(27, 15, 24));
		B.Box(FVector(0, -16.6f, 116), FVector(22, 1.2f, 19));
		B.Slot = Chrome;
		B.Cylinder(FVector(16, -18.6f, 116), Y, 2.2f, 3, 20);
		B.Cylinder(FVector(-22.5f, -17.8f, 128), Z, 1, 5, 12);
		B.Cylinder(FVector(-22.5f, -17.8f, 104), Z, 1, 5, 12);
		B.Slot = Iron;
		B.Box(FVector(0, -1, 141), FVector(29, 17, 1.5f));
		B.Cylinder(FVector(-12, 16, 104), Y, 8, 8, 28);			// the motor, far side
		B.Cylinder(FVector(-12, 21, 104), Y, 5, 3, 20);

		// The lens: turret plate, barrel, chrome ring, glass.
		B.Box(FVector(29.5f, -1, 120), FVector(2.5f, 11, 11));
		B.Cylinder(FVector(40, -1, 120), X, 6, 22, 32);
		B.Slot = Chrome;
		B.Cylinder(FVector(52.5f, -1, 120), X, 7.5f, 5, 32);
		B.Cylinder(FVector(34, -1, 127.5f), Z, 1.2f, 3, 10);		// the focus knob
		B.Slot = Glass;
		B.Cylinder(FVector(55.1f, -1, 120), X, 5.6f, 0.4f, 32);

		// The lower magazine (closed): drum, chrome rim, knob, window; the throat up into the head.
		B.Slot = Paint;
		B.Cylinder(FVector(2, -1, 58), Y, 27, 16, 64);
		B.Slot = Chrome;
		B.Tube(FVector(2, -9.4f, 58), Y, 27.4f, 25, 1, 64);
		B.Cylinder(FVector(-22, -10.6f, 58), Y, 1.8f, 3, 16);
		B.Slot = Glass;
		B.Cylinder(FVector(2, -9.4f, 68), Y, 5, 0.6f, 24);
		B.Slot = Iron;
		B.Box(FVector(2, -1, 88), FVector(5, 4, 3.5f));

		// The upper throat (fire valve) and its rollers.
		B.Box(FVector(2, -1, 146), FVector(5, 4, 5));
		B.Slot = Chrome;
		B.Cylinder(FVector(-3, -1, 149.5f), Y, 1.6f, 9, 16);
		B.Cylinder(FVector(7, -1, 149.5f), Y, 1.6f, 9, 16);

		// The upper magazine's body (its door is a part of its own): back plate, drum wall, chrome front rim, the
		// pale inside face, the spindle and its collar, the door's hinge barrels.
		B.Slot = Paint;
		B.Cylinder(FVector(UpperCenter.X, 6.2f, UpperCenter.Z), Y, 28.5f, 1.6f, 64);
		B.Tube(FVector(UpperCenter.X, -1.2f, UpperCenter.Z), Y, 28.5f, 27, 13.2f, 64);
		B.Slot = Chrome;
		B.Tube(FVector(UpperCenter.X, -8.4f, UpperCenter.Z), Y, 28.5f, 26.6f, 1.2f, 64);
		B.Cylinder(FVector(UpperCenter.X, -2.2f, UpperCenter.Z), Y, 1.3f, 14, 16);
		B.Cylinder(FVector(UpperCenter.X, 4.2f, UpperCenter.Z), Y, 3.2f, 2, 24);
		B.Box(FVector(UpperCenter.X, -6, UpperCenter.Z), FVector(0.4f, 3, 1.8f));	// the spindle's key
		B.Cylinder(FVector(DoorHinge.X, DoorHinge.Y, UpperCenter.Z + 13), Z, 1.2f, 6, 12);
		B.Cylinder(FVector(DoorHinge.X, DoorHinge.Y, UpperCenter.Z - 13), Z, 1.2f, 6, 12);
		B.Slot = Interior;
		B.Cylinder(FVector(UpperCenter.X, 5.2f, UpperCenter.Z), Y, 27, 0.3f, 64);
		// The film's way out at the bottom of the magazine.
		B.Slot = Iron;
		B.Box(FVector(UpperCenter.X, -2, UpperCenter.Z - 28), FVector(4, 3, 1.5f));
	}

	void BuildMagazineDoor(FBuilder& B)
	{
		// Door space: the hinge at the origin, the door hangs toward -X in the XZ plane, its outside toward -Y.
		const FVector Y = FVector::RightVector;
		const FVector C(-28.5f, 0, 0);
		B.Slot = Paint;
		B.Cylinder(C, Y, 28.5f, 1.2f, 64);
		B.Slot = Chrome;
		B.Tube(C + FVector(0, -0.75f, 0), Y, 28.5f, 26.8f, 0.4f, 64);
		B.Tube(C + FVector(0, -0.8f, 11), Y, 6.6f, 5.4f, 0.6f, 32);
		B.Box(FVector(-1.6f, 0, 0), FVector(1.6f, 0.7f, 15));		// the hinge leaf
		B.Slot = Glass;
		B.Cylinder(C + FVector(0, -0.7f, 11), Y, 5.5f, 0.4f, 32);
		B.Slot = Interior;
		B.Cylinder(C + FVector(0, 0.65f, 0), Y, 27, 0.1f, 64);
	}

	void BuildDoorLatch(FBuilder& B)
	{
		// Latch space: its pivot on the door's rear edge; it turns about Y (pitch).
		B.Slot = Chrome;
		B.Box(FVector(0, -0.5f, 2.5f), FVector(1.1f, 0.5f, 4));
		B.Cylinder(FVector(0, -1.4f, 5.6f), FVector::RightVector, 1.3f, 1.8f, 16);
		B.Cylinder(FVector(0, -0.4f, 0), FVector::RightVector, 1.6f, 0.8f, 16);
	}

	void BuildReel(FBuilder& B, bool bMaster, float FilmRadius)
	{
		// Reel space: centre at the origin, the axle along Y.
		const FVector Y = FVector::RightVector;
		const float H = ReelWidth * 0.5f - 0.25f;
		B.Slot = Chrome;
		B.Flange(FVector(0, -H, 0), Y, ReelRadius, 3.2f, 0.5f, 6, 3.9f, 12.2f, 0.f);
		B.Flange(FVector(0, H, 0), Y, ReelRadius, 3.2f, 0.5f, 6, 3.9f, 12.2f, 0.f);
		B.Tube(FVector::ZeroVector, Y, 4.0f, 1.4f, ReelWidth, 32);
		B.Slot = Film;
		B.Tube(FVector::ZeroVector, Y, FilmRadius, 4.0f, ReelWidth - 1.1f, 96);
		// The label between two windows (front and back flange), long side along the rim.
		B.Slot = bMaster ? LabelRed : LabelPale;
		const float Ang = PI / 6.f, Rr = 17.2f;
		const FQuat Q(Y, -Ang);
		const FVector P(Rr * FMath::Cos(Ang), 0, Rr * FMath::Sin(Ang));
		B.Box(P + FVector(0, -H - 0.32f, 0), FVector(1.4f, 0.06f, 3.2f), Q);
		B.Box(P + FVector(0, H + 0.32f, 0), FVector(1.4f, 0.06f, 3.2f), Q);
		// A strip of tape across the film's edge (the lab's leader mark).
		B.Box(FVector(0, 0, FilmRadius + 0.05f), FVector(1.2f, ReelWidth * 0.5f - 0.6f, 0.08f));
	}

	void BuildFilmStrand(FBuilder& B)
	{
		// One centimetre of film hanging from the pivot (scale Z for its length).
		B.Slot = Film;
		B.Box(FVector(0, 0, -0.5f), FVector(0.12f, 1.75f, 0.5f));
	}

	void BuildCan(FBuilder& B)
	{
		// Can space: the bottom's centre at the origin, open toward +Z.
		const FVector Z = FVector::UpVector;
		B.Slot = Chrome;
		B.Cylinder(FVector(0, 0, 0.3f), Z, CanRadius, 0.6f, 64);
		B.Tube(FVector(0, 0, CanDepth * 0.5f), Z, CanRadius, CanRadius - 0.6f, CanDepth, 64);
		B.Tube(FVector(0, 0, CanDepth), Z, CanRadius + 0.4f, CanRadius - 0.6f, 0.6f, 64);
		B.Tube(FVector(0, 0, 1.8f), Z, CanRadius + 0.25f, CanRadius - 0.1f, 0.4f, 64);
		B.Tube(FVector(0, 0, 3.6f), Z, CanRadius + 0.25f, CanRadius - 0.1f, 0.4f, 64);
		B.Slot = Iron;
		B.Box(FVector(CanRadius + 0.4f, 0, 1.6f), FVector(0.5f, 1.6f, 1.1f));		// the latch's hinge block
	}

	void BuildCanLid(FBuilder& B)
	{
		// Lid space: the origin where the lid meets the can's rim; the top at about +1.1.
		const FVector Z = FVector::UpVector;
		B.Slot = Chrome;
		B.Cylinder(FVector(0, 0, 0.8f), Z, CanRadius + 0.8f, 0.6f, 64);
		B.Tube(FVector(0, 0, -0.4f), Z, CanRadius + 1.1f, CanRadius + 0.5f, 2.2f, 64);
		B.Tube(FVector(0, 0, 1.15f), Z, 18.f, 17.2f, 0.3f, 64);
		B.Slot = Iron;
		B.Cylinder(FVector(0, 0, 1.15f), Z, 3.f, 0.3f, 24);
		B.Slot = LabelRed;
		B.Box(FVector(0, 9.5f, 1.15f), FVector(7.5f, 3.2f, 0.08f));
	}

	void BuildCanLatch(FBuilder& B)
	{
		// Latch space: the hinge on the can's side; closed it stands up and its lip hooks over the lid.
		B.Slot = Chrome;
		B.Box(FVector(0.4f, 0, 3.0f), FVector(0.35f, 1.4f, 3.0f));
		B.Box(FVector(-0.5f, 0, 6.2f), FVector(1.25f, 1.4f, 0.3f));
		B.Cylinder(FVector(0, 0, 0), FVector::RightVector, 0.6f, 3.2f, 12);
	}

	void BuildBench(FBuilder& B)
	{
		// Bench space: the floor under its centre; the long side along X, the worker on the +Y side.
		B.Slot = Wood;
		B.Box(FVector(0, 0, BenchTop - 2), FVector(55, 26, 2));
		B.Box(FVector(0, 0, 22), FVector(50, 22, 1));
		B.Slot = Iron;
		for (int32 SX = -1; SX <= 1; SX += 2)
		{
			for (int32 SY = -1; SY <= 1; SY += 2)
			{
				B.Box(FVector(SX * 51.f, SY * 22.f, (BenchTop - 4) * 0.5f), FVector(2.5f, 2.5f, (BenchTop - 4) * 0.5f));
			}
			// A rewind at each end: post, spindle, crank.
			B.Box(FVector(SX * 49.f, -6, BenchTop + 10), FVector(1.5f, 1.5f, 10));
			B.Slot = Chrome;
			B.Cylinder(FVector(SX * 49.f, -9, BenchTop + 17), FVector::RightVector, 0.8f, 6, 12);
			B.Cylinder(FVector(SX * 49.f, -3, BenchTop + 17), FVector::RightVector, 3.5f, 1, 20);
			B.Box(FVector(SX * 49.f, -2.2f, BenchTop + 14.5f), FVector(0.6f, 0.6f, 2.5f));
			B.Slot = Iron;
		}
		B.Box(FVector(0, -22, 20), FVector(51, 1.5f, 2));
		B.Box(FVector(8, -17, BenchTop + 1.2f), FVector(6, 2.5f, 1.2f));	// the splicer
	}

	void BuildBeam(FBuilder& B, float Length)
	{
		B.Slot = Beam;
		B.Cone(FVector::ZeroVector, FVector::ForwardVector, 4.5f, Length * 0.07f, Length, 48);
	}
}
