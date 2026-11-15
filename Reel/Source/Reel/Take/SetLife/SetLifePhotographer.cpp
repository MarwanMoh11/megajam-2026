// Set life piece: the society photographer (the street, the press pen on the south pavement). About 16-20 s.
// L.A. Noire's press men and the red-carpet flashbulbs of period films. A gentleman from the press pen (a GuestTux with
// a press camera at his chest: body, lens, flash gun and its round reflector) beckons the guests standing by the pen.
// They step to a mark with the Aurelia's front at their backs, turn to him and pose close (one waves); he raises the
// camera, the bulb pops (a short, strong, warm light), they laugh and blink, "Lovely! One more for the Herald.", a
// second flash, the spent bulb drops on the pavement, the guests stroll on down the pavement and chat, and he looks
// round for his next subject.
// Staged on the south pavement only (y < -150): clear of the queue (y 300..800), the carpet and the doorman's post
// (-3290, 0), the scanner and the way in (y ~320). Nobody crosses the press pen (lamps, tripods, the bulb crate at
// y -385..-440): the pose is played in the strip between the carpet and the pen, the walk-on goes round its east end.
#include "SetLife/SetLife.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"

namespace SetLifePhotographer
{
	// The press pen (tools/sets/front.py "press"): lamps at (-3985/-3615, -400), tripod cameras at (-3875/-3700, ~-390),
	// a bulb crate at (-3790, -418). The three onlookers in front of it stand at y -300 (Cast_Guest_006/008/010).
	const FVector PenFront(-3800.f, -300.f, 0.f);
	constexpr float CastRadius = 600.f;		// who counts as "at the pen"
	constexpr float SouthOfCarpetY = -150.f;	// never cast or stage north of this (the carpet, the queue)

	// The marks (world x, y; z from the floor).
	const FVector2D PhotoSpot(-3830.f, -318.f);		// the photographer, a step in front of the west tripod
	const FVector2D MarkCentre(-3530.f, -290.f);	// the guests pose here, the facade behind them, 3 m from him
	constexpr float SlotHalf = 46.f;				// each guest this far either side of the centre (along Y)
	constexpr float CloseIn = 13.f;					// then each shifts this much toward the other
	const FVector2D PassNorth(-3830.f, -232.f);		// a guest from the west end passes the photographer here
	const FVector2D OnNorth(-3565.f, -740.f);		// the stroll on: down the pavement past the pen's east lamp
	const FVector2D OnSouth(-3485.f, -790.f);
	const FVector2D BackVia(-3545.f, -265.f);		// the way home: back up past the lamp...
	const FVector2D BackWest(-3800.f, -232.f);		// ...and north of the photographer's place for the west end

	const FLinearColor CameraBlack(0.025f, 0.025f, 0.03f);
	const FLinearColor Chrome(0.62f, 0.60f, 0.55f);
	const FLinearColor BulbGlass(0.90f, 0.92f, 0.95f);
	const FLinearColor FlashWarm(1.f, 0.93f, 0.80f);
	constexpr float FlashCandelas = 10000.f;
	constexpr float FlashSeconds = 0.3f;

	struct FLine
	{
		const TCHAR* Text;
		const TCHAR* Sound;
	};
	const FLine Beckon[] = {
		{ TEXT("You there! One for the society page?"), TEXT("sl_photo_beckon_1") },
		{ TEXT("Hold it! A picture for the papers?"), TEXT("sl_photo_beckon_2") },
	};
	const FLine Reply[] = {
		{ TEXT("Us? Oh, how thrilling!"), TEXT("sl_photo_reply_1") },
		{ TEXT("Is my hair all right?"), TEXT("sl_photo_reply_2") },
	};
	const FLine Pose[] = {
		{ TEXT("Closer, now. Chins up!"), TEXT("sl_photo_pose_1") },
		{ TEXT("Big smiles. Hold it..."), TEXT("sl_photo_pose_2") },
	};
	const FLine Blink[] = {
		{ TEXT("Ha! I'm seeing stars!"), TEXT("sl_photo_blink_1") },
		{ TEXT("Oh! Spots before my eyes!"), TEXT("sl_photo_blink_2") },
	};
	const FLine Again[] = {
		{ TEXT("Lovely! One more for the Herald."), TEXT("sl_photo_again_1") },
		{ TEXT("Swell! Once more, for the Herald."), TEXT("sl_photo_again_2") },
	};
	const FLine Thanks[] = {
		{ TEXT("Do send us a print!"), TEXT("sl_photo_thanks_1") },
		{ TEXT("Page six, darling, page six!"), TEXT("sl_photo_thanks_2") },
	};
	const FLine Next[] = {
		{ TEXT("Who's next? Smile for the Herald!"), TEXT("sl_photo_next_1") },
		{ TEXT("Anybody else? Don't be shy!"), TEXT("sl_photo_next_2") },
	};

	template <SIZE_T N>
	const FLine& PickLine(const FSetLifeStage& S, const FLine (&Lines)[N])
	{
		return Lines[S.RandInt(0, static_cast<int32>(N) - 1)];
	}

	class FPiece : public FSetLifePiece
	{
	public:
		virtual bool Setup(FSetLifeStage& S) override
		{
			// --- casting: a gentleman at the pen (build_cast.py picks the press guests' kits at random, so a man in tails
			// will do, and if the pen holds only ladies, a lady); one or two guests beside it, not in the queue ---
			auto AtPen = [](const ACastMember& M) { return M.GetActorLocation().Y < SouthOfCarpetY; };
			int32 Ph = S.Cast(TEXT("Photographer"), FSetLifeRole().Men().NearPoint(PenFront, CastRadius).Where(AtPen));
			if (Ph == INDEX_NONE)
			{
				Ph = S.Cast(TEXT("Photographer"), FSetLifeRole().Guests().NearPoint(PenFront, CastRadius).Where(AtPen));
			}
			if (Ph == INDEX_NONE)
			{
				return false;
			}
			ACastMember* PhMember = S.Get(Ph);
			const TArray<ACastMember*> Found = S.FindFree(FSetLifeRole().Guests().NearPoint(PenFront, CastRadius)
				.Where([PhMember](const ACastMember& M) { return &M != PhMember && M.GetActorLocation().Y < SouthOfCarpetY; }), 2);
			TArray<int32> Subjects;
			for (ACastMember* M : Found)
			{
				const int32 R = S.CastMember(Found.Num() > 1 && Subjects.Num() == 0 ? TEXT("SubjectA") : TEXT("SubjectB"), M);
				if (R != INDEX_NONE)
				{
					Subjects.Add(R);
				}
			}
			if (Subjects.Num() == 0)
			{
				return false;	// the stage releases the photographer
			}
			// West (smaller X) takes the north slot and walks the north side of the photographer; east takes the south.
			Subjects.Sort([&S](int32 L, int32 R) { return S.Where(L).X < S.Where(R).X; });
			const bool bPair = Subjects.Num() > 1;
			const int32 West = Subjects[0];
			const int32 East = bPair ? Subjects[1] : Subjects[0];
			const int32 A = East;	// speaks the reply and the thanks
			const int32 B = West;	// waves, blinks (the same guest when alone)

			// --- the geometry ---
			const float Z = S.Where(Ph).Z;
			auto Floor = [&S, Z](const FVector2D& P) { return S.FloorAt(FVector(P.X, P.Y, Z)); };
			const FVector Spot = Floor(PhotoSpot);
			const FVector Centre = Floor(MarkCentre);
			Fwd = (Centre - Spot).GetSafeNormal2D();
			const FVector Side(-Fwd.Y, Fwd.X, 0.f);		// his right
			const FVector SlotWest = bPair ? Floor(MarkCentre + FVector2D(0.f, SlotHalf)) : Centre;
			const FVector SlotEast = bPair ? Floor(MarkCentre - FVector2D(0.f, SlotHalf)) : Centre;
			const FVector OnWest = Floor(OnNorth);
			const FVector OnEast = Floor(OnSouth);
			const bool bWestPasses = S.Where(West).X < PhotoSpot.X + 60.f;
			const FVector Pass = Floor(PassNorth);
			for (int32 R : Subjects)
			{
				TArray<FVector> Via = { Floor(BackVia) };
				if (S.Home(R).X < -3700.f)
				{
					Via.Add(Floor(BackWest));
				}
				S.SetReturnVia(R, Via);
			}
			S.SetAnchor((Spot + Centre) * 0.5f);

			// --- props: a press camera at his chest (spine_05; the hand would swing it about) ---
			const bool bChest = PhMember && PhMember->Body && PhMember->Body->DoesSocketExist(TEXT("spine_05"));
			const FName Bone = bChest ? FName(TEXT("spine_05")) : NAME_None;
			const float Up = bChest ? 0.f : 128.f;		// no chest bone: from the feet
			S.HoldProp(Ph, ETakeShape::Cube, FVector(12.f, 18.f, 15.f), CameraBlack, FVector(24.f, 0.f, -8.f + Up), Bone);
			const int32 Lens = S.HoldProp(Ph, ETakeShape::Cylinder, FVector(8.f, 8.f, 7.f), CameraBlack,
				FVector(33.f, 0.f, -9.f + Up), Bone);
			S.HoldProp(Ph, ETakeShape::Cylinder, FVector(3.5f, 3.5f, 20.f), Chrome, FVector(25.f, 12.f, 2.f + Up), Bone);
			const int32 Dish = S.HoldProp(Ph, ETakeShape::Cylinder, FVector(20.f, 20.f, 1.5f), Chrome,
				FVector(28.f, 12.f, 15.f + Up), Bone);
			Bulb = S.HoldProp(Ph, ETakeShape::Sphere, FVector(5.f, 5.f, 6.5f), BulbGlass, FVector(30.f, 12.f, 15.f + Up), Bone);
			// The lens and the reflector face forward (a cylinder stands on its axis: tip it over).
			for (int32 Id : { Lens, Dish })
			{
				if (UStaticMeshComponent* Mesh = S.GetProp(Id))
				{
					Mesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
				}
			}
			const FVector CameraAt = Spot + Fwd * 30.f + FVector(0.f, 0.f, 128.f);
			CameraGuess = CameraAt + Side * 12.f + FVector(0.f, 0.f, 15.f);
			BulbDropAt = S.FloorAt(Spot + Fwd * 28.f + Side * 22.f) + FVector(0.f, 0.f, 3.f);

			const FLine& L1 = PickLine(S, Beckon);
			const FLine& L2 = PickLine(S, Reply);
			const FLine& L3 = PickLine(S, Pose);
			const FLine& L4 = PickLine(S, Blink);
			const FLine& L5 = PickLine(S, Again);
			const FLine& L6 = PickLine(S, Thanks);
			const FLine& L7 = PickLine(S, Next);

			// --- the beat sheet ---
			// He spots them and beckons; they look round.
			S.FaceTo(Ph, A).GestureAt(Ph, TEXT("Wave"), A).Say(Ph, L1.Text, L1.Sound);
			S.Look(A, Ph, 2.2f).Look(B, Ph, 2.2f);
			S.Wait(1.1f).GestureAt(A, TEXT("Laugh"), Ph).Say(A, L2.Text, L2.Sound);
			// To the marks (the west guest round the north side of him), he to his spot.
			S.Wait(0.8f).Walk(Ph, Spot, 70.f);
			if (bPair)
			{
				S.Walk(East, SlotEast, 100.f);
			}
			if (bWestPasses)
			{
				S.Walk(West, Pass, 110.f).WaitArrived(West, 4.f);
			}
			S.Walk(West, SlotWest, 110.f).WaitArrived(West, 6.f).WaitArrived(East, 4.f).WaitArrived(Ph, 2.f);
			// The pose: they face him and stand close, one waves; he raises the camera.
			S.FacePoint(A, Spot).FacePoint(B, Spot).FacePoint(Ph, Centre);
			S.Wait(0.35f);
			if (bPair)
			{
				const FVector Toward = (SlotEast - SlotWest).GetSafeNormal2D();
				S.Shift(West, Toward * CloseIn, 0.4f).Shift(East, -Toward * CloseIn, 0.4f);
			}
			S.GestureAt(B, TEXT("Wave"), Ph).Say(Ph, L3.Text, L3.Sound).Gesture(Ph, TEXT("Lift"));
			S.WaitGesture(Ph, 1.8f);
			// Pop! They blink and laugh.
			S.Do([this](FSetLifeStage& St) { Pop(St); }).GestureAtPoint(Ph, TEXT("Use"), CameraGuess);
			S.Wait(0.2f).Shift(B, -Fwd * 7.f, 0.15f).GestureAt(B, TEXT("Laugh"), A).Gesture(A, TEXT("Laugh"));
			S.Wait(0.35f).Say(B, L4.Text, L4.Sound);
			S.Wait(1.3f).Say(Ph, L5.Text, L5.Sound).Gesture(Ph, TEXT("Nod"));
			S.Wait(1.0f).Shift(B, Fwd * 7.f, 0.3f).Look(A, Ph, 1.5f).Look(B, Ph, 1.5f);
			S.Wait(0.6f).Do([this](FSetLifeStage& St) { Pop(St); }).GestureAtPoint(Ph, TEXT("Use"), CameraGuess);
			// The spent bulb drops; they thank him and stroll on down the pavement.
			S.Wait(0.4f).Do([this](FSetLifeStage& St) { St.DropProp(Bulb, BulbDropAt, FRotator(0.f, St.Rand(0.f, 360.f), 90.f)); });
			S.Gesture(A, TEXT("Laugh")).GestureAt(B, TEXT("Laugh"), A);
			S.Wait(0.8f).GestureAt(A, TEXT("Wave"), Ph).Say(A, L6.Text, L6.Sound);
			S.Wait(1.4f).Walk(West, OnWest, 100.f);
			if (bPair)
			{
				S.Walk(East, OnEast, 100.f);
			}
			// He looks for the next subject.
			S.Wait(0.6f).Gesture(Ph, TEXT("LookAround")).Say(Ph, L7.Text, L7.Sound);
			S.WaitArrived(West, 6.f).WaitArrived(East, 6.f);
			if (bPair)
			{
				S.FaceTo(West, East).FaceTo(East, West).GestureAt(West, TEXT("Talk"), East);
				S.Wait(1.3f).GestureAt(East, TEXT("Laugh"), West);
			}
			else
			{
				S.Gesture(West, TEXT("CheckWatch"));
			}
			S.Wait(1.2f);
			return true;
		}

	private:
		/** The bulb pops: a short, strong, warm light at the bulb (where it is now). */
		void Pop(FSetLifeStage& S)
		{
			const UStaticMeshComponent* B = S.GetProp(Bulb);
			const FVector At = B ? B->GetComponentLocation() : CameraGuess;
			S.Flash(At + Fwd * 12.f, FlashCandelas, FlashSeconds, FlashWarm);
		}

		int32 Bulb = INDEX_NONE;
		FVector Fwd = FVector(1.f, 0.f, 0.f);
		FVector CameraGuess = FVector::ZeroVector;
		FVector BulbDropAt = FVector::ZeroVector;
	};
}

SETLIFE_PIECE(Photographer, SetLifePhotographer::FPiece, ESetLifeZone::Street, 60.f, 1.f)
