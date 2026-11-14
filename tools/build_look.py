# Builds the film look's assets in /Game/Take/Look from text (step 4; docs/ART_STYLE.md §15.3). Run it in the editor:
#   "<engine python>" -I tools/ue_mcp.py py tools/build_look.py
# tools/build_auction.py runs it too when the assets are missing, and puts MI_FilmGrade in the level's unbound
# post-process volume. Safe to re-run: the materials are emptied and rebuilt, the instances get these numbers again.
#
#   M_FilmGrade    the grade: a post-process material at Scene Color After DOF (before TSR, so the stencil's edges are
#                  anti-aliased with the picture and do not crawl). The volume's tone curve is off while the film
#                  look is on (ATakeDirector::StepLook), so the material does the tone itself: a filmic curve gives the
#                  display-referred picture §15.3 is written for, the grade runs in display 0-1, and the result is
#                  decoded back to linear for the tonemapper's sRGB encode.
#   MI_FilmGrade   its numbers (§15.3). Tune them here or in the editor; C++ drives Bleed and MakingOf per frame.
#   M_TakeSignal   unlit, vector parameter Color -> emissive: everything that keeps its colour (stencil 1, 2, 3).
#   MI_Signal_Red, MI_Signal_Gold   for the map's own meshes (the carpet, the reel's case, the gold side exit).
import unreal

LOOK = "/Game/Take/Look"
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()

# --- §15.2 tokens, display 0-1 ---------------------------------------------------------------------------------------
INK = (0.051, 0.047, 0.043)
SILVER = (0.478, 0.459, 0.427)
PAPER = (0.945, 0.918, 0.859)
MO_SHADOW = (0.043, 0.090, 0.141)
MO_MID = (0.337, 0.451, 0.541)
MO_HIGH = (0.875, 0.957, 0.961)

# --- §15.3 numbers, and the calibration (measured on a screenshot: a live red frame within 15 of #D3301F) -----------
SCALARS = {
    "Bleed": 0.0,            # 0 = graded, 1 = the full-colour picture (the alarm; C++ runs it on a smoothstep)
    "MakingOf": 0.0,         # 0 = film, 1 = making-of (C++ sets it on the frame of the cut press)
    "ColourExposure": 0.45,   # the scene into the material's filmic curve (calibrated against the step 3 picture)
    "FilmBlack": 0.03,       # film levels: black point, white point, soft-S contrast
    "FilmWhite": 0.90,
    "FilmContrast": 0.6,
    "Vignette": 0.30,        # film only: corners at 70%
    "MoBlack": 0.02,         # making-of levels: flat video, no contrast curve
    "MoRange": 0.93,
    "MoKeep": 0.9,           # 10% of the source colour stays on the making-of
    "StencilSat": 1.15,      # what keeps its colour: saturation x1.15
    "KeptGain": 1.7,         # a signal colour at full strength (TakeColors::Red, Gold) reaches its token
    "RedFloor": 0.20,        # ...and a value floor: red never darker than Y 0.20
    "GoldFloor": 0.45,       # gold never darker than Y 0.45 (always lighter than red)
    "AlarmSat": 1.0,         # after the bleed: saturation on the whole picture
    "KeepStencil": 1.0,      # 0 = everything graded (a check)
    "OutputScale": 0.6,      # measured with DebugFlat: after the grade the engine shows linear x1.65 (display x1.25)
    "DebugFlat": 0.0,        # 1 = known values instead of the picture (a check of what the engine does after the grade)
}
VECTORS = {
    "Ink": INK, "Silver": SILVER, "Paper": PAPER,
    "MoShadow": MO_SHADOW, "MoMid": MO_MID, "MoHigh": MO_HIGH,
    "RedToken": (0.864, 0.167, 0.095),   # danger #D3301F (0.827, 0.188, 0.122) before the film overlay: its grain
                                         # (0.07 toward grey) and flicker bring it back to the token on screen
    "GoldToken": (0.959, 0.753, 0.274),  # julian #E9B949 (0.914, 0.725, 0.286), the same way
}

HLSL = r"""
float3 lumw = float3(0.2126, 0.7152, 0.0722);
float3 c = max(Scene.rgb, 0.0);
float s = floor(Stencil.r + 0.5);
float isRed = (abs(s - 1.0) < 0.5) ? 1.0 : 0.0;
float isGold = (abs(s - 2.0) < 0.5) ? 1.0 : 0.0;
float isWhite = (abs(s - 3.0) < 0.5) ? 1.0 : 0.0;
// the custom-depth pass ignores what stands in front: keep only where the tagged surface is the one seen
float seen = (CustomDepth.r <= SceneDepth.r + max(1.0, SceneDepth.r * 0.002)) ? 1.0 : 0.0;
float kept = saturate(isRed + isGold + isWhite) * KeepStencil * seen;
isRed *= seen;
isGold *= seen;
float3 x = c * ColourExposure;
float3 tm = saturate((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14));
float3 o = lerp(tm * 12.92, 1.055 * pow(max(tm, 1e-6), 1.0 / 2.4) - 0.055, step(0.0031308, tm));
float Y = dot(o, lumw);
float f1 = saturate((Y - FilmBlack) / max(0.001, FilmWhite - FilmBlack));
float f2 = lerp(f1, f1 * f1 * (3.0 - 2.0 * f1), FilmContrast);
float3 film = lerp(lerp(Ink.rgb, Silver.rgb, saturate(f2 * 2.0)), lerp(Silver.rgb, Paper.rgb, saturate(f2 * 2.0 - 1.0)), step(0.5, f2));
float m1 = saturate((Y - MoBlack) / max(0.001, MoRange));
float3 mo = lerp(lerp(MoShadow.rgb, MoMid.rgb, saturate(m1 * 2.0)), lerp(MoMid.rgb, MoHigh.rgb, saturate(m1 * 2.0 - 1.0)), step(0.5, m1));
mo = lerp(o, mo, MoKeep);
float3 graded = lerp(film, mo, MakingOf);
// what keeps its colour: a saturated red or gold source shows its token's hue at the source's value (the token
// itself once the source is as bright as the token), anything less saturated on it (a grey tally, a white pulse)
// keeps its own colour at saturation x StencilSat; then the value floors (red Y >= 0.20, gold Y >= 0.45)
float mx = max(o.r, max(o.g, o.b));
float satv = (mx - min(o.r, min(o.g, o.b))) / max(mx, 1e-4);
float3 tok = isRed * RedToken.rgb + isGold * GoldToken.rgb;
float tokMax = max(tok.r, max(tok.g, tok.b));
float3 tokScaled = tok * min(1.0, mx * KeptGain / max(tokMax, 1e-4));
float w = saturate((satv - 0.15) / 0.35) * saturate(isRed + isGold);
float3 sc = lerp(max(lerp(float3(Y, Y, Y), o, StencilSat), 0.0), tokScaled, w);
float floorY = isRed * RedFloor + isGold * GoldFloor;
sc *= max(1.0, floorY / max(dot(sc, lumw), 1e-4));
sc = saturate(sc);
graded = lerp(graded, sc, kept);
float2 p = float2((UV.x - 0.5) / 0.5, (UV.y - 0.5) / 0.3722);
float r = length(p) / 1.414;
graded *= 1.0 - Vignette * (1.0 - MakingOf) * smoothstep(0.5, 1.0, r);
float3 alarm = lerp(saturate(lerp(float3(Y, Y, Y), o, AlarmSat)), sc, kept);
float3 d = saturate(lerp(graded, alarm, Bleed));
// a check of the pipeline after the grade: top half ten greys 0..1, bottom half the true red and gold tokens
float3 dbg = lerp(float3(1.0, 1.0, 1.0) * (floor(UV.x * 10.0) / 9.0), lerp(float3(0.827, 0.188, 0.122), float3(0.914, 0.725, 0.286), step(0.5, UV.x)), step(0.5, UV.y));
d = lerp(d, dbg, DebugFlat);
return lerp(d / 12.92, pow((d + 0.055) / 1.055, 2.4), step(0.04045, d)) * OutputScale;
"""


def enum_member(enum_cls, *names):
    for n in names:
        if hasattr(enum_cls, n):
            return getattr(enum_cls, n)
    raise RuntimeError("no %s in %s: %s" % (names, enum_cls, [m for m in dir(enum_cls) if m.isupper()]))


def fresh_material(name):
    path = LOOK + "/" + name
    if eal.does_asset_exist(path):
        m = eal.load_asset(path)
        mel.delete_all_material_expressions(m)
    else:
        m = at.create_asset(name, LOOK, unreal.Material, unreal.MaterialFactoryNew())
    return m


def instance(name, parent, scalars=None, vectors=None):
    path = LOOK + "/" + name
    if eal.does_asset_exist(path):
        mi = eal.load_asset(path)
    else:
        mi = at.create_asset(name, LOOK, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    for k, v in (scalars or {}).items():
        mel.set_material_instance_scalar_parameter_value(mi, k, float(v))
    for k, v in (vectors or {}).items():
        mel.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
    mel.update_material_instance(mi)
    eal.save_asset(path, only_if_is_dirty=False)
    return mi


# --- M_TakeSignal ------------------------------------------------------------------------------------------------------
sig = fresh_material("M_TakeSignal")
sig.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
col = mel.create_material_expression(sig, unreal.MaterialExpressionVectorParameter, -400, 0)
col.set_editor_property("parameter_name", "Color")
col.set_editor_property("default_value", unreal.LinearColor(0.85, 0.03, 0.02, 1.0))
mel.connect_material_property(col, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(sig)
eal.save_asset(LOOK + "/M_TakeSignal", only_if_is_dirty=False)

# --- M_FilmGrade -------------------------------------------------------------------------------------------------------
g = fresh_material("M_FilmGrade")
g.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
g.set_editor_property("blendable_location", enum_member(unreal.BlendableLocation, "BL_SCENE_COLOR_AFTER_DOF",
                                                         "BL_SCENECOLOR_AFTER_DOF", "SCENE_COLOR_AFTER_DOF"))
cu = mel.create_material_expression(g, unreal.MaterialExpressionCustom, 0, 0)
cu.set_editor_property("code", HLSL)
cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
cu.set_editor_property("description", "FilmGrade")
names = ["Scene", "Stencil", "SceneDepth", "CustomDepth", "UV"] + list(SCALARS.keys()) + list(VECTORS.keys())
inputs = []
for n in names:
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", n)
    inputs.append(ci)
cu.set_editor_property("inputs", inputs)

scene = mel.create_material_expression(g, unreal.MaterialExpressionSceneTexture, -700, -300)
scene.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
stencil = mel.create_material_expression(g, unreal.MaterialExpressionSceneTexture, -700, -150)
stencil.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_CUSTOM_STENCIL)
sdepth = mel.create_material_expression(g, unreal.MaterialExpressionSceneTexture, -1000, -300)
sdepth.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_SCENE_DEPTH)
cdepth = mel.create_material_expression(g, unreal.MaterialExpressionSceneTexture, -1000, -150)
cdepth.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_CUSTOM_DEPTH)
assert mel.connect_material_expressions(sdepth, "Color", cu, "SceneDepth")
assert mel.connect_material_expressions(cdepth, "Color", cu, "CustomDepth")
uv = mel.create_material_expression(g, unreal.MaterialExpressionScreenPosition, -700, 0)
assert mel.connect_material_expressions(scene, "Color", cu, "Scene")
assert mel.connect_material_expressions(stencil, "Color", cu, "Stencil")
if not mel.connect_material_expressions(uv, "ViewportUV", cu, "UV"):
    assert mel.connect_material_expressions(uv, "", cu, "UV")
y = 150
for k, v in SCALARS.items():
    e = mel.create_material_expression(g, unreal.MaterialExpressionScalarParameter, -700, y)
    e.set_editor_property("parameter_name", k)
    e.set_editor_property("default_value", float(v))
    assert mel.connect_material_expressions(e, "", cu, k)
    y += 60
for k, v in VECTORS.items():
    e = mel.create_material_expression(g, unreal.MaterialExpressionVectorParameter, -700, y)
    e.set_editor_property("parameter_name", k)
    e.set_editor_property("default_value", unreal.LinearColor(v[0], v[1], v[2], 1.0))
    assert mel.connect_material_expressions(e, "", cu, k)
    y += 120
mel.connect_material_property(cu, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(g)
eal.save_asset(LOOK + "/M_FilmGrade", only_if_is_dirty=False)

# --- the instances -----------------------------------------------------------------------------------------------------
instance("MI_FilmGrade", g, SCALARS, VECTORS)
instance("MI_Signal_Red", sig, vectors={"Color": (0.85, 0.03, 0.02)})     # TakeColors::Red
instance("MI_Signal_Gold", sig, vectors={"Color": (1.0, 0.62, 0.08)})     # TakeColors::Gold
print("LOOK built: M_FilmGrade, MI_FilmGrade, M_TakeSignal, MI_Signal_Red, MI_Signal_Gold in " + LOOK)
