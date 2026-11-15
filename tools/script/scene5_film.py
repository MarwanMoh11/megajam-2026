"""Scene 5: LE SOLEIL NOIR, the long looping film on the auditorium screen (QA Nov 15 evening). Owner: s5-auditorium.

Inside the editor, holding the editor lock, after tools/script/scene5_film_frames.cs wrote the atlas:
    "$PY" -I tools/ue_mcp.py py tools/script/scene5_film.py 110
Makes, in /Game/Take/Script/S5 (re-runnable: the texture re-imported, the material rebuilt in place, both saved):
  T_S5_FilmAtlas  the film's 26 shots, 4 x 9 cells of 1024 x 455 (C:\\Jam\\Builds\\s5\\film\\T_S5_FilmAtlas.png)
  M_S5_Film       unlit emissive old print: the cells in order, ShotSeconds each, looping (Shots x ShotSeconds:
                  about 2 min 15 s), a slow push in on each shot, a dip through black at each cut, and the old print's
                  flicker, scratches, dust, grain, gate weave and vignette. The same scalars as the old M_OldFilm, so
                  AHouseDimmer drives it unchanged: Play (0 black, 1 running), Brightness, T0 (world seconds when Play
                  went to 1: the film starts on its first frame at the blackout), Aspect
scene5.py puts M_S5_Film on the level's HouseDimmer (FilmMaterial) with FilmLeaderSkipSeconds 0.
"""
import os

import unreal

DEST = "/Game/Take/Script/S5"
SRC = r"C:\Jam\Builds\s5\film\T_S5_FilmAtlas.png"
SHOTS_TXT = r"C:\Jam\Builds\s5\film\shots.txt"
NOISE = "/Engine/EngineMaterials/Good64x64TilingNoiseHighFreq"
EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
REPORT = []

FILM_HLSL = r"""
float t = (T0 >= 0.0) ? max(T - T0, 0.0) : T;
float ss = max(ShotSecs, 0.5);
float n = max(Shots, 1.0);
float lt = fmod(t, ss * n);
float k = floor(lt / ss);
float u = frac(lt / ss);
float fr = floor(t * 24.0);
float h1 = frac(sin(fr * 12.9898) * 43758.5453);
float h2 = frac(sin(fr * 78.233 + 1.7) * 12543.1234);
float2 uv = UV + float2(sin(t * 3.1) * 0.0015 + (h1 - 0.5) * 0.002, sin(t * 2.3) * 0.002 + (h2 - 0.5) * 0.003);
float zoom = 1.0 - 0.05 * u;
float2 suv = (uv - 0.5) * zoom + 0.5;
suv = clamp(suv, 0.002, 0.998);
float2 cell = float2(fmod(k, 4.0), floor(k / 4.0));
float2 auv = (cell + suv) / float2(4.0, 9.0);
float3 s3 = Texture2DSampleLevel(Atlas, AtlasSampler, auv, 0).rgb;
float l = dot(s3, float3(0.299, 0.587, 0.114));
l = saturate((l - 0.45) * 1.25 + 0.5);
float dip = smoothstep(0.0, 0.06, u) * smoothstep(1.0, 0.95, u);
float3 col = float3(l, l, l) * lerp(0.15, 1.0, dip);
col *= 0.9 + 0.1 * h1 + 0.03 * sin(t * 47.0);
for (int i = 0; i < 3; i++) {
    float seg = floor(t * (1.5 + i));
    float hs = frac(sin((seg + i * 17.0) * 91.17) * 4373.13);
    if (hs > 0.55) {
        float xs = frac(hs * 7.31) + (h2 - 0.5) * 0.003;
        float w = 0.0007 + 0.0012 * frac(hs * 3.7);
        float ln = smoothstep(w, 0.0, abs(uv.x - xs)) * (0.6 + 0.4 * frac(sin(uv.y * 40.0 + seg) * 99.0));
        col = lerp(col, (i == 1) ? float3(0.03, 0.03, 0.03) : float3(0.85, 0.85, 0.85), ln * 0.6);
    }
}
float dn = Texture2DSampleLevel(Noise, NoiseSampler, uv * float2(Asp, 1.0) * 1.7 + float2(h1, h2) * 7.0, 0).r;
col = lerp(col, (h1 > 0.5) ? float3(0.02, 0.02, 0.02) : float3(0.8, 0.8, 0.8), smoothstep(0.92, 0.985, dn) * 0.8);
float gn = Texture2DSampleLevel(Noise, NoiseSampler, uv * float2(Asp, 1.0) * 7.0 + float2(h2, h1) * 13.0, 0).r;
col *= 0.9 + 0.2 * gn;
float2 d = (UV - 0.5) * 2.0;
col *= 1.0 - 0.4 * pow(saturate(dot(d, d) * 0.5), 1.5);
return col * float3(1.0, 0.97, 0.9) * Brightness * saturate(Play);
"""


def expr(m, cls, x, y, **kw):
    e = MEL.create_material_expression(m, cls, x, y)
    for k, v in kw.items():
        e.set_editor_property(k, v)
    return e


def scalar(m, name, v, x, y):
    return expr(m, unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=float(v))


def tex_object(m, tex, x, y, param=None, linear_grey=False):
    e = expr(m, unreal.MaterialExpressionTextureObjectParameter if param else unreal.MaterialExpressionTextureObject, x, y)
    if param:
        e.set_editor_property("parameter_name", param)
    e.set_editor_property("texture", tex)
    srgb = tex.get_editor_property("srgb")
    cs = tex.get_editor_property("compression_settings")
    S = unreal.MaterialSamplerType
    if cs == unreal.TextureCompressionSettings.TC_GRAYSCALE:
        e.set_editor_property("sampler_type", S.SAMPLERTYPE_GRAYSCALE if srgb else S.SAMPLERTYPE_LINEAR_GRAYSCALE)
    else:
        e.set_editor_property("sampler_type", S.SAMPLERTYPE_COLOR if srgb else S.SAMPLERTYPE_LINEAR_COLOR)
    return e


def import_atlas():
    t = unreal.AssetImportTask()
    t.filename = SRC
    t.destination_path = DEST
    t.destination_name = "T_S5_FilmAtlas"
    t.replace_existing = True
    t.automated = True
    t.save = False
    AT.import_asset_tasks([t])
    tex = EAL.load_asset(DEST + "/T_S5_FilmAtlas")
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
    tex.set_editor_property("srgb", True)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_CINEMATIC)
    tex.set_editor_property("never_stream", True)
    EAL.save_loaded_asset(tex, False)
    return tex


def build(tex, shots):
    path = DEST + "/M_S5_Film"
    if EAL.does_asset_exist(path):
        m = EAL.load_asset(path)
        MEL.delete_all_material_expressions(m)
    else:
        m = AT.create_asset("M_S5_Film", DEST, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    ins = [("UV", expr(m, unreal.MaterialExpressionTextureCoordinate, -900, -500)),
           ("T", expr(m, unreal.MaterialExpressionTime, -900, -440)),
           ("T0", scalar(m, "T0", -1.0, -900, -380)),
           ("Play", scalar(m, "Play", 0.0, -900, -320)),
           ("Brightness", scalar(m, "Brightness", 1.0, -900, -260)),
           ("ShotSecs", scalar(m, "ShotSeconds", 5.2, -900, -200)),
           ("Shots", scalar(m, "Shots", float(shots), -900, -140)),
           ("Asp", scalar(m, "Aspect", 2.33, -900, -80)),
           ("Atlas", tex_object(m, tex, -900, 0, param="FilmAtlas")),
           ("Noise", tex_object(m, EAL.load_asset(NOISE), -900, 120))]
    c = expr(m, unreal.MaterialExpressionCustom, -400, 0)
    c.set_editor_property("code", FILM_HLSL)
    c.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    c.set_editor_property("description", "S5Film")
    arr = []
    for (n, _) in ins:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        arr.append(ci)
    c.set_editor_property("inputs", arr)
    for (n, src) in ins:
        MEL.connect_material_expressions(src, "", c, n)
    MEL.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m)
    EAL.save_asset(path)
    return m


shots = 26
if os.path.exists(SHOTS_TXT):
    with open(SHOTS_TXT, encoding="utf-8") as f:
        shots = len([ln for ln in f if ln.strip()])
if not os.path.exists(SRC):
    REPORT.append("no atlas at " + SRC + ": run scene5_film_frames.cs first")
else:
    tex = import_atlas()
    m = build(tex, shots)
    REPORT.append("S5 film: T_S5_FilmAtlas %s, M_S5_Film built (%d shots x 5.2 s = %.0f s loop)"
                  % (tex.blueprint_get_size_x() if hasattr(tex, "blueprint_get_size_x") else "?", shots, shots * 5.2))
"\n".join(REPORT)
