// Basic-shape helpers: every visible thing in step 1 is an /Engine/BasicShapes mesh coloured through a dynamic
// instance of BasicShapeMaterial (vector parameter "Color"). No assets are created.
// Owner: header pass (shared, complete).

#pragma once

#include "CoreMinimal.h"

class AActor;
class USceneComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UBoxComponent;

/** /Engine/BasicShapes meshes. Cube, Sphere, Cylinder and Cone are 100 cm across with the pivot at the centre
 *  (Cone: 100 cm tall, base radius 50, tip pointing +Z). Plane is 100 x 100 cm, zero thickness, facing +Z. */
enum class ETakeShape : uint8
{
	Cube,
	Sphere,
	Cylinder,
	Cone,
	Plane
};

struct FTakeShapes
{
	/** BasicShapeMaterial's colour parameter: "Color". */
	static const FName ColorParam;

	/** The basic-shape mesh. Safe in constructors and at runtime. */
	static UStaticMesh* GetMesh(ETakeShape Shape);

	/** /Engine/BasicShapes/BasicShapeMaterial. */
	static UMaterialInterface* GetMaterial();

	/** Configures an existing mesh component as a shape: mesh, BasicShapeMaterial, no collision, no overlaps,
	 *  never affects navigation, shadow casting as asked. Works on UInstancedStaticMeshComponent too.
	 *  Collision rule (Nov 13 rework): every take prop that is not a wall or the floor (watcher bodies and frames,
	 *  the beam, tape, marks, rings, device bodies, the cart, the banner matte, the lamp's beam) is built here, so it
	 *  ignores the Pawn and Camera channels: nothing blocks walking or shoves a follow camera. Keep it NoCollision;
	 *  walls and floor come from the map script with their own collision. */
	static void SetupShape(UStaticMeshComponent* Comp, ETakeShape Shape, bool bCastShadow = true);

	/** CONSTRUCTOR ONLY: creates a default-subobject shape attached to Parent with the given relative transform. */
	static UStaticMeshComponent* CreateShape(AActor* Owner, FName Name, ETakeShape Shape, USceneComponent* Parent,
		const FVector& RelLocation = FVector::ZeroVector, const FVector& RelScale = FVector::OneVector,
		const FRotator& RelRotation = FRotator::ZeroRotator, bool bCastShadow = true);

	/** RUNTIME ONLY (BeginPlay or later): creates, attaches, registers and colours a new shape component. */
	static UStaticMeshComponent* AddShapeAtRuntime(AActor* Owner, ETakeShape Shape, USceneComponent* Parent,
		const FVector& RelLocation, const FVector& RelScale, const FLinearColor& Colour, bool bCastShadow = true);

	/** RUNTIME or OnConstruction, NEVER in a constructor: sets the shape's colour through a dynamic instance of
	 *  BasicShapeMaterial on slot 0 (created on first use, reused after). The change shows on this frame.
	 *  On an instanced component it colours every instance. Returns the dynamic instance. */
	static UMaterialInstanceDynamic* SetColour(UStaticMeshComponent* Comp, const FLinearColor& Colour);

	/** Step 4: the unlit signal material (/Game/Take/Look/M_TakeSignal, vector parameter "Color" -> emissive). */
	static UMaterialInterface* GetSignalMaterial();

	/** Step 4, RUNTIME or OnConstruction, NEVER in a constructor: tags a shape as carrying meaning for the film grade.
	 *  Stencil != 0 (TakeStyle::StencilRed / StencilGold / StencilWhite): custom depth on with that stencil value, and
	 *  slot 0 becomes a dynamic instance of the unlit signal material keeping the colour it had (so the colour on
	 *  screen is the colour set, wherever the lights are). Stencil 0: custom depth off (the material stays).
	 *  Cheap when nothing changes; SetColour keeps working on the same component afterwards. */
	static void SetSignal(UStaticMeshComponent* Comp, uint8 Stencil);

	/** Step 4, pass 3: sets "Pattern" (TakeStyle::Pattern*) on slot 0's dynamic instance; call after SetSignal. A no-op
	 *  when slot 0 is not a dynamic instance or its material has no such parameter (the shape stays solid). */
	static void SetPattern(UStaticMeshComponent* Comp, float Pattern);


	/** CONSTRUCTOR ONLY: creates a click volume: a hidden, query-only box that blocks ECC_Visibility and ignores
	 *  every other channel, generates no overlaps and never affects navigation. Make it generous: the pointer
	 *  trace (ECC_Visibility under the cursor) must find it before the floor. */
	static UBoxComponent* CreateClickVolume(AActor* Owner, FName Name, USceneComponent* Parent, const FVector& Extent,
		const FVector& RelLocation = FVector::ZeroVector);

	/** The click-volume collision setup on an existing box (also used for runtime resizes via SetBoxExtent). */
	static void SetupClickVolume(UBoxComponent* Box, const FVector& Extent);

	/** A box that is only a shape in the editor (zones): no collision at all, hidden in game, no navigation. */
	static void SetupEditorOnlyBox(UBoxComponent* Box, const FVector& Extent, const FColor& EditorColour);
};
