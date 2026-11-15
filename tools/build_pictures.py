"""Pictures for every frame in the building, and the old film for the big screen (wave 3, agent pictures).

Makes, in /Game/Take/Pictures (no outside image service; every picture is rendered here from the game's own set and
cast, then treated in a material as a 1940s print or an oil portrait):
  T_Pic_<name>    1024 px square captures (PNG sources in C:\\Jam\\Builds\\qa\\pictures\\src), DXT1, max 1024
  M_Picture       lit picture material: sepia / muted colour, vignette, grain, canvas weave, a varnish sheen. The frame's
                  aspect comes from the plane's custom primitive data 0 (crops the square picture), a grain seed from 1
  MI_Pic_<name>   one per picture, with its treatment (oil, print, muted, card)
  T_Film_Digits   the leader's numbers 8..2 (4 x 2 atlas)
  M_OldFilm       unlit emissive old print for the big screen: leader countdown, then stills cut every StillSecs, with
                  flicker, scratches, dust, grain and gate weave, black and white. Scalars Play (0 black, 1 running),
                  Brightness, T0 (world seconds when Play went to 1, so the countdown starts on cue; -1 = free running),
                  StillSecs, Aspect (screen width / height)

Run inside the editor, one step per call (each well under the MCP's 110 s), with a first line naming the step:
  PY="/c/Epic Games/UE_5.9/Engine/Binaries/ThirdParty/Python3/Win64/python.exe"
  { echo "STEP = 'mat'"; cat tools/build_pictures.py; } > "$TMP/p.py" && "$PY" -I tools/ue_mcp.py py "$TMP/p.py"
Steps, in order: mat; rig; shoot; import; compose_rig; compose; import; film; mi. ('rig' places the capture stages and
lights, 'shoot' renders them on the next call, after the editor has ticked, posed the cast and streamed textures, then
removes them.) Optional: ONLY = ['por_julian', ...] on another first line limits rig/shoot/compose to those shots.
Re-runnable: textures are re-imported over themselves, materials rebuilt in place. The level is never saved here.
"""
import math
import os

import unreal

STEP = globals().get("STEP", "mat")
ONLY = globals().get("ONLY", None)

DEST = "/Game/Take/Pictures"
OUT = r"C:\Jam\Builds\qa\pictures"
SRC = os.path.join(OUT, "src")
RIG_TAG = "pictures_rig"
STUDIO = (0.0, 0.0, -20000.0)            # the capture stage, far under the level
COMPOSE = (60000.0, 0.0, -20000.0)       # the poster and lobby-card table
HERO_FWD = 90.0                           # the hero meshes face local +Y (the mannequin's convention)
LIGHT = 60.0                            # candelas of a key light at gain 1 (calibrated by eye on the first shots)
RES = 1024

EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
HEROES = {n: "/Game/Take/Cast/Heroes/%s/SKM_Hero_%s" % (n, n) for n in ("Julian", "Maya", "Vex")}
IDLE = "/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle"
CANVAS_DK, CANVAS_LT = "/Game/Take/Dress/MI_Canvas_Dark", "/Game/Take/Dress/MI_Canvas_Light"
CUBE, PLANE = "/Engine/BasicShapes/Cube", "/Engine/BasicShapes/Plane"
NOISE = "/Engine/EngineMaterials/Good64x64TilingNoiseHighFreq"

# --- treatments (M_Picture's parameters) ---------------------------------------------------------------------------
TREAT = {
    "oil":   dict(Sat=0.62, Sepia=0.22, Contrast=1.12, Grain=0.03, Mottle=0.30, Canvas=1.0, Vig=0.55, Bright=1.05,
                  Glow=0.22, Rough=0.30, Tint=(1.10, 0.92, 0.70)),
    "print": dict(Sat=0.0, Sepia=0.80, Contrast=1.25, Grain=0.10, Mottle=0.08, Canvas=0.0, Vig=0.60, Bright=1.10,
                  Glow=0.22, Rough=0.22, Tint=(1.12, 0.95, 0.74)),
    "muted": dict(Sat=0.42, Sepia=0.35, Contrast=1.15, Grain=0.08, Mottle=0.10, Canvas=0.0, Vig=0.55, Bright=1.10,
                  Glow=0.22, Rough=0.24, Tint=(1.10, 0.94, 0.76)),
    "card":  dict(Sat=0.80, Sepia=0.12, Contrast=1.05, Grain=0.05, Mottle=0.10, Canvas=0.0, Vig=0.22, Bright=1.0,
                  Glow=0.25, Rough=0.40, Tint=(1.06, 0.96, 0.82)),
    "flat":  dict(Sat=1.0, Sepia=0.0, Contrast=1.0, Grain=0.0, Mottle=0.0, Canvas=0.0, Vig=0.0, Bright=1.0,
                  Glow=0.0, Rough=0.5, Tint=(1.0, 1.0, 1.0)),
}

# --- the shots -----------------------------------------------------------------------------------------------------
# studio: a hero (or three) on the stage in front of a painted backdrop. room: a camera in the dressed level, the cast
# placed in shot, lit by our own key, fill and rim (the level's own light is far too dark for a capture).
# face: degrees the hero turns away from the camera. gain: light multiplier. shape: "any", or "P"/"L" (composites).
SHOTS = {
    "por_julian": dict(kind="studio", slot=0, treat="oil", heroes=[("Julian", (0, 0), 22)], dist=250, cz=158,
                       lz=150, fov=17, back=CANVAS_DK, gain=1.0, warm=(1.0, 0.82, 0.62)),
    "por_maya":   dict(kind="studio", slot=1, treat="oil", heroes=[("Maya", (0, 0), -20)], dist=240, cz=150,
                       lz=142, fov=17, back=CANVAS_LT, gain=0.55, warm=(1.0, 0.86, 0.70)),
    "por_vex":    dict(kind="studio", slot=2, treat="oil", heroes=[("Vex", (0, 0), 28)], dist=250, cz=156,
                       lz=148, fov=17, back=CANVAS_DK, gain=1.0, warm=(0.95, 0.85, 0.75)),
    "st_trio":    dict(kind="studio", slot=3, treat="print", heroes=[("Maya", (-80, 30), -18), ("Julian", (0, 0), 0),
                       ("Vex", (82, 35), 20)], dist=520, cz=150, lz=118, fov=32, back=CANVAS_DK, gain=1.4,
                       warm=(1.0, 0.9, 0.78)),
    "st_lobby":   dict(kind="room", treat="print", cam=(-3100, -250, 170), look=(-2100, 100, 140), fov=55,
                       heroes=[("Julian", (-2560, -170, 0), 10)], gain=1.6),
    "st_street":  dict(kind="room", treat="print", cam=(-4700, -1100, 180), look=(-3300, 150, 230), fov=55,
                       heroes=[("Julian", (-3750, -260, 0), 25), ("Maya", (-3600, 420, 0), -30)], gain=2.2, wide=True),
    "st_office":  dict(kind="room", treat="muted", cam=(-880, 2140, 620), look=(-250, 2560, 580), fov=55,
                       heroes=[("Vex", (-380, 2470, 450), 15)], gain=1.4),
    "st_projection": dict(kind="room", treat="print", cam=(1050, 2150, 620), look=(1750, 2560, 580), fov=55,
                          heroes=[("Maya", (1600, 2450, 450), -15)], gain=1.4),
    "st_hall":    dict(kind="room", treat="muted", cam=(-850, -900, 420), look=(3500, 300, 200), fov=65,
                       heroes=[], gain=3.0, wide=True),
    "st_stock":   dict(kind="room", treat="print", cam=(2450, 2150, 175), look=(3400, 2700, 120), fov=55,
                       heroes=[("Vex", (2950, 2420, 0), 30)], gain=1.6),
    "st_backstage": dict(kind="room", treat="print", cam=(300, -2420, 170), look=(1500, -2300, 140), fov=55,
                         heroes=[("Maya", (1050, -2330, 0), -10)], gain=1.5),
    "st_gallery": dict(kind="room", treat="muted", cam=(-700, 2200, 175), look=(300, 2650, 150), fov=55,
                       heroes=[("Julian", (-100, 2480, 0), 20)], gain=1.5),
}
# composites: a still (flat) with period lettering, captured from above as base colour
CARDS = {
    "poster":       dict(src="st_trio", shape="P", treat="card"),
    "card_lobby":   dict(src="st_lobby", shape="L", treat="card", caption="HE CAME TO BID ON A DEAD MAN'S FILM"),
    "card_street":  dict(src="st_street", shape="L", treat="card", caption="ONE NIGHT. ONE PRINT. NO INVITATION."),
    "card_stock":   dict(src="st_stock", shape="L", treat="card", caption="SOME PICTURES ARE WORTH STEALING"),
}
FILM = ["st_street", "st_lobby", "st_hall", "st_trio", "st_office", "st_projection", "st_backstage", "st_stock"]


def log(msg):
    unreal.log("pictures: " + msg)
    with open(os.path.join(OUT, "steps.txt"), "a", encoding="utf-8") as f:
        f.write(msg + "\n")
    return msg


def world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def wanted(name):
    return ONLY is None or name in ONLY


# --- materials -----------------------------------------------------------------------------------------------------
def new_material(name):
    path = DEST + "/" + name
    if EAL.does_asset_exist(path):
        m = EAL.load_asset(path)
        MEL.delete_all_material_expressions(m)
        return m
    return AT.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())


def expr(m, cls, x, y, **kw):
    e = MEL.create_material_expression(m, cls, x, y)
    for k, v in kw.items():
        e.set_editor_property(k, v)
    return e


def scalar(m, name, v, x, y, cpd=None):
    e = expr(m, unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=float(v))
    if cpd is not None:
        e.set_editor_property("use_custom_primitive_data", True)
        e.set_editor_property("primitive_data_index", cpd)
    return e


def sampler_for(tex):
    cs, srgb = tex.get_editor_property("compression_settings"), tex.get_editor_property("srgb")
    S = unreal.MaterialSamplerType
    if cs == unreal.TextureCompressionSettings.TC_GRAYSCALE:
        return S.SAMPLERTYPE_GRAYSCALE if srgb else S.SAMPLERTYPE_LINEAR_GRAYSCALE
    if cs == unreal.TextureCompressionSettings.TC_ALPHA:
        return S.SAMPLERTYPE_ALPHA
    if cs == unreal.TextureCompressionSettings.TC_MASKS:
        return S.SAMPLERTYPE_MASKS
    return S.SAMPLERTYPE_COLOR if srgb else S.SAMPLERTYPE_LINEAR_COLOR


def tex_object(m, tex, x, y, param=None):
    if param:
        e = expr(m, unreal.MaterialExpressionTextureObjectParameter, x, y, parameter_name=param)
    else:
        e = expr(m, unreal.MaterialExpressionTextureObject, x, y)
    e.set_editor_property("texture", tex)
    e.set_editor_property("sampler_type", sampler_for(tex))
    return e


def custom(m, code, ins, out, x, y, desc):
    c = expr(m, unreal.MaterialExpressionCustom, x, y)
    c.set_editor_property("code", code)
    c.set_editor_property("output_type", out)
    c.set_editor_property("description", desc)
    arr = []
    for (n, _) in ins:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        arr.append(ci)
    c.set_editor_property("inputs", arr)
    for (n, src) in ins:
        MEL.connect_material_expressions(src, "", c, n)
    return c


PICTURE_HLSL = r"""
float2 c = (Asp >= 1.0) ? float2(1.0, 1.0 / Asp) : float2(Asp, 1.0);
float2 uv = (UV - 0.5) * c * Zoom + 0.5;
float3 t = Texture2DSample(Tex, TexSampler, uv).rgb;
t = saturate((t - 0.5) * Contrast + 0.5);
float l = dot(t, float3(0.299, 0.587, 0.114));
t = lerp(float3(l, l, l), t, Sat);
t = lerp(t, l * Tint.rgb, Sepia);
float2 q = float2(UV.x * max(Asp, 0.2), UV.y);
float n1 = Texture2DSample(Noise, NoiseSampler, q * 23.0 + Seed * 0.731).r;
float n2 = Texture2DSample(Noise, NoiseSampler, q * float2(1.3, 3.1) + Seed * 0.377).r;
float wv = sin(q.x * 6.2832 * Weave) * sin(UV.y * 6.2832 * Weave);
t *= 1.0 + Grain * (n1 - 0.5) * 2.0 + Mottle * (n2 - 0.5) + Canvas * wv * 0.10;
float2 d = (UV - 0.5) * 2.0;
t *= 1.0 - Vig * pow(saturate(dot(d, d) * 0.5), 1.4);
return saturate(t * Bright);
"""


def build_picture_material():
    m = new_material("M_Picture")
    m.set_editor_property("used_with_instanced_static_meshes", True)
    noise = EAL.load_asset(NOISE)
    tex = tex_object(m, noise, -900, -300, param="Picture")
    nz = tex_object(m, noise, -900, -100)
    uv = expr(m, unreal.MaterialExpressionTextureCoordinate, -900, 50)
    tint = expr(m, unreal.MaterialExpressionVectorParameter, -900, 150, parameter_name="Tint",
                default_value=unreal.LinearColor(1.1, 0.92, 0.7, 1.0))
    t = TREAT["print"]
    ins = [("Tex", tex), ("Noise", nz), ("UV", uv), ("Tint", tint),
           ("Asp", scalar(m, "Aspect", 1.5, -700, -400, cpd=0)), ("Seed", scalar(m, "Seed", 0.0, -700, -340, cpd=1))]
    y = -280
    for k in ("Sat", "Sepia", "Contrast", "Grain", "Mottle", "Canvas", "Vig", "Bright"):
        ins.append((k, scalar(m, k, t[k], -700, y)))
        y += 60
    ins.append(("Weave", scalar(m, "Weave", 150.0, -700, y)))
    ins.append(("Zoom", scalar(m, "Zoom", 0.96, -700, y + 60)))
    c = custom(m, PICTURE_HLSL, ins, unreal.CustomMaterialOutputType.CMOT_FLOAT3, -350, 0, "Picture")
    MEL.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
    glow = scalar(m, "Glow", 0.2, -350, 250)
    mul = expr(m, unreal.MaterialExpressionMultiply, -150, 250)
    MEL.connect_material_expressions(c, "", mul, "A")
    MEL.connect_material_expressions(glow, "", mul, "B")
    MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.connect_material_property(scalar(m, "Rough", 0.3, -350, 350), "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.recompile_material(m)
    EAL.save_asset(DEST + "/M_Picture")
    return m


def make_mi(name, tex_name, treat, extra=None):
    path = DEST + "/" + name
    if EAL.does_asset_exist(path):
        mi = EAL.load_asset(path)
    else:
        mi = AT.create_asset(name, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, EAL.load_asset(DEST + "/M_Picture"))
    tex = EAL.load_asset(DEST + "/" + tex_name)
    MEL.set_material_instance_texture_parameter_value(mi, "Picture", tex)
    p = dict(TREAT[treat])
    p.update(extra or {})
    for k, v in p.items():
        if k == "Tint":
            MEL.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(v[0], v[1], v[2], 1.0))
        else:
            MEL.set_material_instance_scalar_parameter_value(mi, k, float(v))
    MEL.update_material_instance(mi)
    EAL.save_asset(path)
    return mi


# --- the old film ----------------------------------------------------------------------------------------------------
FILM_HLSL = r"""
float t = (T0 >= 0.0) ? max(T - T0, 0.0) : T;
float lead = 7.0;
float ss = max(StillSecs, 0.2);
float cyc = lead + 8.0 * ss;
float lt = fmod(t, cyc);
float fr = floor(t * 24.0);
float h1 = frac(sin(fr * 12.9898) * 43758.5453);
float h2 = frac(sin(fr * 78.233 + 1.7) * 12543.1234);
float2 uv = UV + float2(sin(t * 3.1) * 0.0015 + (h1 - 0.5) * 0.0025, sin(t * 2.3) * 0.002 + (h2 - 0.5) * 0.004);
float2 p = (uv - 0.5) * float2(Asp, 1.0);
float3 col = float3(0, 0, 0);
if (lt < lead) {
    float n = floor(lt);
    float s = frac(lt);
    float r = length(p);
    float an = frac(atan2(p.x, -p.y) / 6.2831853 + 1.0);
    float g = (an < s) ? 0.36 : 0.62;
    g = lerp(g, 0.92, smoothstep(0.007, 0.0, abs(r - 0.42)));
    g = lerp(g, 0.92, smoothstep(0.007, 0.0, abs(r - 0.35)));
    g = lerp(g, 0.08, smoothstep(0.005, 0.0, min(abs(p.x), abs(p.y))) * step(r, 0.47));
    float2 dq = p / 0.56 + 0.5;
    if (dq.x > 0.0 && dq.x < 1.0 && dq.y > 0.0 && dq.y < 1.0) {
        float2 cell = float2(fmod(n, 4.0), floor(n / 4.0));
        float dg = Texture2DSampleLevel(Digits, DigitsSampler, (cell + dq) / float2(4.0, 2.0), 0).r;
        g = lerp(g, 0.03, saturate(dg * 1.4));
    }
    if (n >= 6.0 && s > 0.12) g = 0.02;
    col = float3(g, g, g);
} else {
    float k = floor((lt - lead) / ss);
    float2 cr = (Asp >= 1.0) ? float2(1.0, 1.0 / Asp) : float2(Asp, 1.0);
    float2 suv = (uv - 0.5) * cr + 0.5;
    float3 s3;
    if (k < 1.0) s3 = Texture2DSampleLevel(S0, S0Sampler, suv, 0).rgb;
    else if (k < 2.0) s3 = Texture2DSampleLevel(S1, S1Sampler, suv, 0).rgb;
    else if (k < 3.0) s3 = Texture2DSampleLevel(S2, S2Sampler, suv, 0).rgb;
    else if (k < 4.0) s3 = Texture2DSampleLevel(S3, S3Sampler, suv, 0).rgb;
    else if (k < 5.0) s3 = Texture2DSampleLevel(S4, S4Sampler, suv, 0).rgb;
    else if (k < 6.0) s3 = Texture2DSampleLevel(S5, S5Sampler, suv, 0).rgb;
    else if (k < 7.0) s3 = Texture2DSampleLevel(S6, S6Sampler, suv, 0).rgb;
    else s3 = Texture2DSampleLevel(S7, S7Sampler, suv, 0).rgb;
    float l = dot(s3, float3(0.299, 0.587, 0.114));
    l = saturate((l - 0.45) * 1.35 + 0.5);
    col = float3(l, l, l);
}
col *= 0.88 + 0.12 * h1 + 0.03 * sin(t * 47.0);
for (int i = 0; i < 3; i++) {
    float seg = floor(t * (1.5 + i));
    float hs = frac(sin((seg + i * 17.0) * 91.17) * 4373.13);
    if (hs > 0.4) {
        float xs = frac(hs * 7.31) + (h2 - 0.5) * 0.003;
        float w = 0.0007 + 0.0012 * frac(hs * 3.7);
        float ln = smoothstep(w, 0.0, abs(uv.x - xs)) * (0.6 + 0.4 * frac(sin(uv.y * 40.0 + seg) * 99.0));
        col = lerp(col, (i == 1) ? float3(0.03, 0.03, 0.03) : float3(0.85, 0.85, 0.85), ln * 0.7);
    }
}
float dn = Texture2DSampleLevel(Noise, NoiseSampler, uv * float2(Asp, 1.0) * 1.7 + float2(h1, h2) * 7.0, 0).r;
col = lerp(col, (h1 > 0.5) ? float3(0.02, 0.02, 0.02) : float3(0.8, 0.8, 0.8), smoothstep(0.9, 0.98, dn) * 0.85);
float gn = Texture2DSampleLevel(Noise, NoiseSampler, uv * float2(Asp, 1.0) * 7.0 + float2(h2, h1) * 13.0, 0).r;
col *= 0.88 + 0.24 * gn;
float2 d = (UV - 0.5) * 2.0;
col *= 1.0 - 0.4 * pow(saturate(dot(d, d) * 0.5), 1.5);
return col * float3(1.0, 0.97, 0.9) * Brightness * saturate(Play);
"""


def build_film():
    m = new_material("M_OldFilm")
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("used_with_instanced_static_meshes", True)
    ins = [("UV", expr(m, unreal.MaterialExpressionTextureCoordinate, -900, -500)),
           ("T", expr(m, unreal.MaterialExpressionTime, -900, -440)),
           ("T0", scalar(m, "T0", -1.0, -900, -380)),
           ("Play", scalar(m, "Play", 0.0, -900, -320)),
           ("Brightness", scalar(m, "Brightness", 1.0, -900, -260)),
           ("StillSecs", scalar(m, "StillSecs", 1.6, -900, -200)),
           ("Asp", scalar(m, "Aspect", 1.37, -900, -140)),
           ("Digits", tex_object(m, EAL.load_asset(DEST + "/T_Film_Digits"), -900, -80)),
           ("Noise", tex_object(m, EAL.load_asset(NOISE), -900, 0))]
    for i, n in enumerate(FILM):
        ins.append(("S%d" % i, tex_object(m, EAL.load_asset(DEST + "/T_Pic_" + n), -900, 80 + 80 * i,
                                          param="Still%d" % i)))
    c = custom(m, FILM_HLSL, ins, unreal.CustomMaterialOutputType.CMOT_FLOAT3, -400, 0, "OldFilm")
    MEL.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m)
    EAL.save_asset(DEST + "/M_OldFilm")
    return m


# --- capture rigs --------------------------------------------------------------------------------------------------
def tag(a):
    a.set_editor_property("tags", [unreal.Name(RIG_TAG)])
    a.set_folder_path("Pictures_Rig")
    return a


def v(p):
    return unreal.Vector(float(p[0]), float(p[1]), float(p[2]))


def box(loc, size, mat, yaw=0.0):
    a = tag(EAS.spawn_actor_from_class(unreal.StaticMeshActor, v(loc), unreal.Rotator(roll=0, pitch=0, yaw=yaw)))
    c = a.static_mesh_component
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    c.set_static_mesh(EAL.load_asset(CUBE))
    c.set_material(0, EAL.load_asset(mat))
    a.set_actor_scale3d(unreal.Vector(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    return a


def light(loc, cd, rgb=(1.0, 0.9, 0.8), radius=900.0, shadows=False):
    a = tag(EAS.spawn_actor_from_class(unreal.PointLight, v(loc)))
    c = a.point_light_component
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    c.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    c.set_intensity(float(cd))
    c.set_attenuation_radius(float(radius))
    c.set_light_color(unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    c.set_cast_shadows(bool(shadows))
    c.set_editor_property("source_radius", 12.0)
    return a


def hero(name, loc, look_from, face=0.0):
    yaw = math.degrees(math.atan2(look_from[1] - loc[1], look_from[0] - loc[0])) - HERO_FWD + face
    a = tag(EAS.spawn_actor_from_class(unreal.SkeletalMeshActor, v(loc), unreal.Rotator(roll=0, pitch=0, yaw=yaw)))
    c = a.skeletal_mesh_component
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    mesh = EAL.load_asset(HEROES[name])
    try:
        c.set_skeletal_mesh_asset(mesh)
    except Exception:                                           # noqa: BLE001
        c.set_editor_property("skeletal_mesh_asset", mesh)
    try:                                                        # an idle pose instead of the bind pose, if it takes
        c.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
        c.override_animation_data(EAL.load_asset(IDLE), True, False, 0.7, 0.0)
        c.set_update_animation_in_editor(True)
    except Exception as e:                                      # noqa: BLE001
        log("anim: %s" % e)
    return a


def rot_dir(d, deg):
    a = math.radians(deg)
    return (d[0] * math.cos(a) - d[1] * math.sin(a), d[0] * math.sin(a) + d[1] * math.cos(a))


def lights_for(cam, target, gain, warm=(1.0, 0.86, 0.68), wide=False):
    dx, dy = target[0] - cam[0], target[1] - cam[1]
    L = math.hypot(dx, dy) or 1.0
    back = (-dx / L, -dy / L)                                  # from the subject toward the camera
    k = rot_dir(back, 50.0)
    light((target[0] + k[0] * 190, target[1] + k[1] * 190, target[2] + 110), LIGHT * gain, warm, 900, True)
    f = rot_dir(back, -35.0)
    light((target[0] + f[0] * 260, target[1] + f[1] * 260, target[2] + 20), LIGHT * gain * 0.22, (0.75, 0.82, 1.0),
          900)
    r = rot_dir((-back[0], -back[1]), 35.0)
    light((target[0] + r[0] * 140, target[1] + r[1] * 140, target[2] + 160), LIGHT * gain * 0.7, (1.0, 0.95, 0.9),
          700)
    if wide:                                                    # lamps down the length of a wide shot
        for s in (0.25, 0.5, 0.75, 1.0):
            light((cam[0] + dx * s, cam[1] + dy * s, target[2] + 300), LIGHT * gain * 0.9, warm, 1800)


def studio_frame(s):
    o = (STUDIO[0] + 2500.0 * s["slot"], STUDIO[1], STUDIO[2])
    cam = (o[0], o[1] - s["dist"], o[2] + s["cz"])
    return o, cam, (o[0], o[1], o[2] + s["lz"])


def rig_shot(name, s):
    if s["kind"] == "studio":
        o, cam, look = studio_frame(s)
        box((o[0], o[1] + 260, o[2] + 250), (900, 10, 700), s["back"])           # the painted backdrop
        box((o[0], o[1], o[2] - 5), (900, 900, 10), CANVAS_DK)                    # the floor
        for (n, (x, y), face) in s["heroes"]:
            hero(n, (o[0] + x, o[1] + y, o[2]), cam, face)
        lights_for(cam, (look[0], look[1], o[2] + 150), s["gain"], s.get("warm", (1.0, 0.86, 0.68)))
        light((o[0] - 120, o[1] + 200, o[2] + 120), LIGHT * 0.08 * s["gain"], (1.0, 0.8, 0.6), 400)  # backdrop pool
    else:
        for (n, p, face) in s["heroes"]:
            hero(n, p, s["cam"], face)
        tgt = s["heroes"][0][1] if s["heroes"] else s["look"]
        lights_for(s["cam"], (tgt[0], tgt[1], tgt[2] + 140 if s["heroes"] else s["look"][2]), s["gain"],
                   wide=s.get("wide", False))


def clear_rig():
    n = 0
    for a in EAS.get_all_level_actors():
        if RIG_TAG in [str(t) for t in a.get_editor_property("tags")]:
            EAS.destroy_actor(a)
            n += 1
    return n


_RT = {}


def capture(name, pos, look, fov, source, w=RES, h=RES, ortho=None, flags=()):
    key = (w, h)
    if key not in _RT:
        _RT[key] = unreal.RenderingLibrary.create_render_target2d(
            world(), w, h, unreal.TextureRenderTargetFormat.RTF_RGBA8, unreal.LinearColor(0, 0, 0, 1), False)
    rt = _RT[key]
    r = unreal.MathLibrary.find_look_at_rotation(v(pos), v(look))
    cam = EAS.spawn_actor_from_class(unreal.SceneCapture2D, v(pos), r)
    try:
        c = cam.capture_component2d
        c.set_editor_property("texture_target", rt)
        c.set_editor_property("capture_source", source)
        c.set_editor_property("capture_every_frame", False)
        c.set_editor_property("capture_on_movement", False)
        c.set_editor_property("fov_angle", float(fov))
        if ortho:
            c.set_editor_property("projection_type", unreal.CameraProjectionMode.ORTHOGRAPHIC)
            c.set_editor_property("ortho_width", float(ortho))
        if flags:
            c.set_editor_property("show_flag_settings", [unreal.EngineShowFlagsSetting(f, on) for (f, on) in flags])
        c.capture_scene()
        unreal.RenderingLibrary.export_render_target(world(), rt, SRC, name + ".png")
    finally:
        EAS.destroy_actor(cam)
    return os.path.join(SRC, name + ".png")


def shoot(name, s):
    if s["kind"] == "studio":
        _, cam, look = studio_frame(s)
        return capture(name, cam, look, s["fov"], unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR,
                       flags=(("Fog", False), ("VolumetricFog", False)))
    return capture(name, s["cam"], s["look"], s["fov"], unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR,
                   flags=(("Fog", False), ("VolumetricFog", False)))   # our lamps lit the fog into halos


# --- composites: poster, lobby cards, the leader's digits --------------------------------------------------------------
def plane(loc, w, h, mi, aspect):
    """A Plane facing -Y (toward the compose camera, which sees +X on its left), u to screen right, v down."""
    a = tag(EAS.spawn_actor_from_class(unreal.StaticMeshActor, v(loc),
                                       unreal.MathLibrary.make_rot_from_xy(unreal.Vector(-1, 0, 0),
                                                                           unreal.Vector(0, 0, -1))))
    c = a.static_mesh_component
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    c.set_static_mesh(EAL.load_asset(PLANE))
    c.set_material(0, mi)
    a.set_actor_scale3d(unreal.Vector(w / 100.0, h / 100.0, 1.0))
    c.set_custom_primitive_data_float(0, float(aspect))
    return a


def text(t, loc, size, rgb, font=None):
    a = tag(EAS.spawn_actor_from_class(unreal.TextRenderActor, v(loc), unreal.Rotator(roll=0, pitch=0, yaw=-90)))
    c = a.text_render
    c.set_text(t)
    c.set_editor_property("world_size", float(size))
    c.set_editor_property("text_render_color", unreal.Color(r=int(rgb[0] * 255), g=int(rgb[1] * 255),
                                                            b=int(rgb[2] * 255), a=255))
    c.set_editor_property("horizontal_alignment", unreal.HorizTextAligment.EHTA_CENTER)
    c.set_editor_property("vertical_alignment", unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)
    if font:
        c.set_editor_property("font", font)
    return a


def flat(src):
    return make_mi("MI_PicFlat_" + src, "T_Pic_" + src, "flat")


def solid(name, rgb):
    """A plain colour: the flat treatment of a texture, tinted to rgb with brightness 0 grain (white noise -> grey)."""
    return make_mi("MI_PicSolid_" + name, "T_Film_White" if EAL.does_asset_exist(DEST + "/T_Film_White")
                   else "T_Pic_st_trio", "flat", dict(Sat=0.0, Sepia=1.0, Tint=rgb, Contrast=0.0, Bright=2.0))


CREAM, INK, RED = (0.86, 0.78, 0.60), (0.08, 0.06, 0.05), (0.42, 0.06, 0.07)


def compose_rig(name, k, slot):
    o = (COMPOSE[0] + 600.0 * slot, COMPOSE[1], COMPOSE[2])
    if name == "digits":                                        # 4 x 2 cells, 100 cm each, 8..2 on black
        plane((o[0], o[1] + 5, o[2]), 400, 200, solid("black", (0.0, 0.0, 0.0)), 2.0)
        for i, dgt in enumerate("8765432"):
            text(dgt, (o[0] + 150 - 100 * (i % 4), o[1] - 2, o[2] + 50 - 100 * (i // 4)), 88, (1, 1, 1))
        return o
    src = flat(k["src"])
    if k["shape"] == "P":                                       # the one-sheet: a 133 x 200 safe area
        plane((o[0], o[1] + 5, o[2]), 220, 220, solid("ink", INK), 1.0)
        plane((o[0], o[1] + 3, o[2] - 5), 128, 128, src, 1.0)
        plane((o[0], o[1] + 2, o[2] + 80), 133, 38, solid("red", RED), 3.5)
        text("THE DIRECTOR'S", (o[0], o[1] - 1, o[2] + 89), 13, CREAM)
        text("CUT", (o[0], o[1] - 1, o[2] + 72), 20, CREAM)
        text("JULIAN  \u00b7  MAYA  \u00b7  VEX", (o[0], o[1] - 1, o[2] - 76), 7.5, CREAM)
        text("AN AURELIA PICTURE  \u00b7  1941", (o[0], o[1] - 1, o[2] - 88), 5.5, (0.7, 0.62, 0.45))
    else:                                                       # a lobby card: a 200 x 133 safe area
        plane((o[0], o[1] + 5, o[2]), 220, 220, solid("cream", CREAM), 1.0)
        plane((o[0] - 28, o[1] + 3, o[2] + 6), 132, 108, src, 132 / 108.0)
        plane((o[0], o[1] + 4, o[2]), 204, 136, solid("red", RED), 1.5)
        plane((o[0] - 28, o[1] + 3.5, o[2] + 6), 138, 114, solid("ink", INK), 1.2)
        text("THE", (o[0] + 70, o[1] - 1, o[2] + 40), 9, CREAM)
        text("DIRECTOR'S", (o[0] + 70, o[1] - 1, o[2] + 24), 10, CREAM)
        text("CUT", (o[0] + 70, o[1] - 1, o[2] + 4), 22, CREAM)
        text("1941", (o[0] + 70, o[1] - 1, o[2] - 18), 7, (0.8, 0.7, 0.5))
        text(k["caption"], (o[0] - 28, o[1] - 1, o[2] - 56), 5.2, CREAM)
    return o


def compose_shoot(name, slot):
    o = (COMPOSE[0] + 600.0 * slot, COMPOSE[1], COMPOSE[2])
    if name == "digits":
        return capture("digits", (o[0], o[1] - 400, o[2]), o, 90, unreal.SceneCaptureSource.SCS_BASE_COLOR,
                       w=1024, h=512, ortho=400)
    return capture(name, (o[0], o[1] - 400, o[2]), o, 90, unreal.SceneCaptureSource.SCS_BASE_COLOR, ortho=200)


# --- textures --------------------------------------------------------------------------------------------------------
def import_png(path, name):
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", path)
    t.set_editor_property("destination_path", DEST)
    t.set_editor_property("destination_name", name)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("automated", True)
    t.set_editor_property("save", False)
    AT.import_asset_tasks([t])
    tex = EAL.load_asset(DEST + "/" + name)
    tex.set_editor_property("max_texture_size", 1024)
    tex.set_editor_property("compression_no_alpha", True)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
    if name == "T_Film_Digits":
        tex.set_editor_property("srgb", False)
    EAL.save_asset(DEST + "/" + name)
    return tex


def step_import():
    done = []
    for f in sorted(os.listdir(SRC)):
        if not f.endswith(".png"):
            continue
        n = f[:-4]
        if ONLY is not None and n not in ONLY:
            continue
        import_png(os.path.join(SRC, f), "T_Film_Digits" if n == "digits" else "T_Pic_" + n)
        done.append(n)
    return done


def step_mi():
    out = []
    for n, s in SHOTS.items():
        if EAL.does_asset_exist(DEST + "/T_Pic_" + n):
            make_mi("MI_Pic_" + n, "T_Pic_" + n, s["treat"])
            out.append(n)
    for n, k in CARDS.items():
        if EAL.does_asset_exist(DEST + "/T_Pic_" + n):
            make_mi("MI_Pic_" + n, "T_Pic_" + n, k["treat"], dict(Zoom=1.0))
            out.append(n)
    return out


# --- the step runner -------------------------------------------------------------------------------------------------
if not EAL.does_directory_exist(DEST):
    EAL.make_directory(DEST)
os.makedirs(SRC, exist_ok=True)

if STEP == "mat":
    build_picture_material()
    RESULT = log("mat: M_Picture built")
elif STEP == "rig":
    clear_rig()
    names = [n for n in SHOTS if wanted(n)]
    for n in names:
        rig_shot(n, SHOTS[n])
    RESULT = log("rig: %s" % names)
elif STEP == "shoot":
    names = [n for n in SHOTS if wanted(n)]
    paths = [shoot(n, SHOTS[n]) for n in names]
    RESULT = log("shoot: %d shots, rig cleared %d" % (len(paths), clear_rig()))
elif STEP == "compose_rig":
    clear_rig()
    names = [n for n in list(CARDS) + ["digits"] if wanted(n)]
    for i, n in enumerate(list(CARDS) + ["digits"]):
        if n in names:
            compose_rig(n, CARDS.get(n), i)
    RESULT = log("compose_rig: %s" % names)
elif STEP == "compose":
    names = [n for n in list(CARDS) + ["digits"] if wanted(n)]
    for i, n in enumerate(list(CARDS) + ["digits"]):
        if n in names:
            compose_shoot(n, i)
    RESULT = log("compose: %s, rig cleared %d" % (names, clear_rig()))
elif STEP == "import":
    RESULT = log("import: %s" % step_import())
elif STEP == "film":
    build_film()
    RESULT = log("film: M_OldFilm built")
elif STEP == "mi":
    RESULT = log("mi: %s" % step_mi())
elif STEP == "clear":
    RESULT = log("clear: %d rig actors" % clear_rig())
else:
    RESULT = "unknown STEP " + STEP
RESULT
