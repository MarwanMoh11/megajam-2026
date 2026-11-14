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
class ULocalLightComponent;
class USpotLightComponent;
class UPointLightComponent;
class URectLightComponent;

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

	/** Step 6: /Game/Take/Dress/M_SignalFaint (tools/build_frames.py): M_SignalPattern dithered to "Fade" coverage, so
	 *  a frame reads faint and still writes its stencil (the grade keeps its red). Null if it is missing. */
	static UMaterialInterface* GetFaintMaterial();

	/** Step 6, RUNTIME only: slot 0 becomes a dynamic instance of the faint material (keeping its colour; call
	 *  SetPattern and SetColour after), custom depth on with Stencil, "Fade" set. Without the asset it stays as it is.
	 *  SetSignal turns it back into the solid signal material. */
	static void SetFaint(UStaticMeshComponent* Comp, uint8 Stencil, float Fade);


	/** CONSTRUCTOR ONLY: creates a click volume: a hidden, query-only box that blocks ECC_Visibility and ignores
	 *  every other channel, generates no overlaps and never affects navigation. Make it generous: the pointer
	 *  trace (ECC_Visibility under the cursor) must find it before the floor. */
	static UBoxComponent* CreateClickVolume(AActor* Owner, FName Name, USceneComponent* Parent, const FVector& Extent,
		const FVector& RelLocation = FVector::ZeroVector);

	/** The click-volume collision setup on an existing box (also used for runtime resizes via SetBoxExtent). */
	static void SetupClickVolume(UBoxComponent* Box, const FVector& Extent);

	/** A box that is only a shape in the editor (zones): no collision at all, hidden in game, no navigation. */
	static void SetupEditorOnlyBox(UBoxComponent* Box, const FVector& Extent, const FColor& EditorColour);

	// --- QA sight: world cues (lights and lamps instead of drawn overlays) ------------------------------------------

	/** The shared cue-light setup: movable, no shadows (static or dynamic), candelas, no light functions, culled past
	 *  CueLightMaxDrawDistance with a fade, no volumetric haze until asked (SetCueLight). Point and spot lights get
	 *  inverse-squared falloff. Constructor or runtime. */
	static void SetupCueLight(ULocalLightComponent* Light, float AttenuationRadius);

	/** CONSTRUCTOR ONLY: a cue spot light (SetupCueLight), off until SetCueLight turns it on. */
	static USpotLightComponent* CreateCueSpot(AActor* Owner, FName Name, USceneComponent* Parent,
		const FVector& RelLocation, const FRotator& RelRotation, float AttenuationRadius);

	/** CONSTRUCTOR ONLY: a cue point light (SetupCueLight), off until SetCueLight turns it on. */
	static UPointLightComponent* CreateCuePoint(AActor* Owner, FName Name, USceneComponent* Parent,
		const FVector& RelLocation, float AttenuationRadius);

	/** CONSTRUCTOR ONLY: a cue rect light (SetupCueLight), off until SetCueLight turns it on. */
	static URectLightComponent* CreateCueRect(AActor* Owner, FName Name, USceneComponent* Parent,
		const FVector& RelLocation, const FRotator& RelRotation, float AttenuationRadius);

	/** RUNTIME: a cue light's strength (candelas), colour and haze (volumetric scattering) this frame; at or near 0
	 *  candelas it is hidden, so it costs nothing. Cheap when nothing changes. */
	static void SetCueLight(ULocalLightComponent* Light, float Candelas, const FLinearColor& Colour, float Haze = 0.f);

	/** RUNTIME: a cue spot's cone (outer half-angle, degrees; inner = outer * InnerRatio). Only touches the light when
	 *  it moves by a quarter of a degree or more (a cone change rebuilds the light's render state). */
	static void SetCueCone(USpotLightComponent* Light, float OuterDegrees, float InnerRatio);

	/** RUNTIME or OnConstruction: back to the plain lit basic-shape material (BasicShapeMaterial), custom depth off,
	 *  keeping the colour it had; undoes SetSignal / SetFaint. A no-op when it is already plain. */
	static void SetPlain(UStaticMeshComponent* Comp);

	/** RUNTIME or OnConstruction: a small lamp: the unlit signal material showing Colour as its glow (values above 1
	 *  glow brighter), solid pattern; custom depth with Stencil (0: none, so the grade treats it as any other pixel). */
	static void SetLamp(UStaticMeshComponent* Comp, const FLinearColor& Colour, uint8 Stencil);

	/** Cue lights stop drawing this far from the view, cm (with a 20% fade); 0 = never culled. */
	static constexpr float CueLightMaxDrawDistance = 5000.f;

	/** Moves Current toward Target at Rate per second (exponential; Rate <= 0 or DeltaSeconds < 0 snaps). */
	static float Ease(float Current, float Target, float Rate, float DeltaSeconds);
};
