# Scene 7's reel swap (s7-reelswap): its two materials and the placed AScene7Swap in the film room.
# Run inside the editor, under the editor lock, from the repo root:
#   "$PY" -I tools/ue_mcp.py py tools/script/scene7_swap.py
# Idempotent: the materials are rebuilt in place, the actor (label S7_ReelSwap) is moved, not duplicated.
import unreal

DIR = "/Game/Take/Props/Swap"
# The film room (s7-route's layout note): x -1000..-600, y -200..800, floor z 450; the port glass on the east wall.
# The actor's origin is the floor under the projector, +X the lens (east). The bench sits behind the operator (-Y).
SWAP_LOC = unreal.Vector(-705.0, 230.0, 450.0)
SWAP_YAW = 0.0
LABEL = "S7_ReelSwap"

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def fresh(name):
    path = DIR + "/" + name
    if eal.does_asset_exist(path):
        m = eal.load_asset(path)
        mel.delete_all_material_expressions(m)
    else:
        m = at.create_asset(name, DIR, unreal.Material, unreal.MaterialFactoryNew())
    return m


def vparam(m, name, rgb, x, y):
    e = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    return e


def sparam(m, name, v, x, y):
    e = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", v)
    return e


def surface():
    m = fresh("M_Swap_Surface")
    c = vparam(m, "Color", (0.5, 0.5, 0.5), -400, -100)
    me = sparam(m, "Metallic", 0.0, -400, 50)
    ro = sparam(m, "Roughness", 0.5, -400, 150)
    mel.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(me, "", unreal.MaterialProperty.MP_METALLIC)
    mel.connect_material_property(ro, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(m)
    eal.save_asset(DIR + "/M_Swap_Surface", only_if_is_dirty=False)
    return m


def beam():
    # The projector's beam: unlit, additive, two-sided; bright down the middle, gone at the cone's edges (fresnel),
    # soft where it meets the wall (depth fade).
    m = fresh("M_Swap_Beam")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    c = vparam(m, "Color", (1.0, 0.93, 0.8), -900, -200)
    g = sparam(m, "Glow", 0.05, -900, -50)
    fr = mel.create_material_expression(m, unreal.MaterialExpressionFresnel, -900, 100)
    fr.set_editor_property("exponent", 1.6)
    om = mel.create_material_expression(m, unreal.MaterialExpressionOneMinus, -700, 100)
    mel.connect_material_expressions(fr, "", om, "")
    df = mel.create_material_expression(m, unreal.MaterialExpressionDepthFade, -700, 250)
    df.set_editor_property("fade_distance_default", 40.0)
    m1 = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -550, -150)
    mel.connect_material_expressions(c, "", m1, "A")
    mel.connect_material_expressions(g, "", m1, "B")
    m2 = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -400, -50)
    mel.connect_material_expressions(m1, "", m2, "A")
    mel.connect_material_expressions(om, "", m2, "B")
    m3 = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -250, 50)
    mel.connect_material_expressions(m2, "", m3, "A")
    mel.connect_material_expressions(df, "", m3, "B")
    mel.connect_material_property(m3, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m)
    eal.save_asset(DIR + "/M_Swap_Beam", only_if_is_dirty=False)
    return m


def place(surf, bm):
    actor = None
    for a in eas.get_all_level_actors():
        if a.get_class().get_name() == "Scene7Swap":
            actor = a
            break
    if actor is None:
        actor = eas.spawn_actor_from_class(unreal.Scene7Swap, SWAP_LOC, unreal.Rotator(0.0, 0.0, SWAP_YAW))
    actor.set_actor_label(LABEL)
    actor.set_actor_location_and_rotation(SWAP_LOC, unreal.Rotator(0.0, 0.0, SWAP_YAW), False, False)
    actor.set_editor_property("tags", [unreal.Name("s7_swap")])   # not script_s7: scene7.py clears that tag
    actor.set_editor_property("surface_material", surf)
    actor.set_editor_property("beam_material", bm)
    return actor


surf = surface()
bm = beam()
a = place(surf, bm)
les.save_current_level()
print("swap at", a.get_actor_location(), a.get_actor_rotation(), "use spot", a.get_use_spot() if hasattr(a, "get_use_spot") else "-")
