"""Scene 8's final shot (s8-car-mirror): the assets the GT's cabin uses (AScene8Car builds the cabin itself at run time).

Two steps, both re-runnable:
  1. outside the editor (engine Python, stdlib only) writes C:\\Jam\\Art\\S8\\T_S8_RainDrops.png, a tileable 512 px
     rain-on-glass texture: RGB a tangent-space normal (the drops' domes, some running streaks), A the drops' mask:
       "<engine python>" -I tools/script/scene8_cabin.py
  2. inside the editor imports it and builds four materials in /Game/Take/Script/S8 (always cooked, DefaultGame.ini):
       M_S8_Paint      lit, "Color" "Rough" "Metal": the cabin's leather, walnut, chrome, carpet, the road
       M_S8_Glow       unlit, "Color" x "Power": gauges, lamp heads, far windows, oncoming headlights
       M_S8_Mirror     unlit, "Capture" (the rear-view mirror's render target), "Flip", "Bright", "Tint"
       M_S8_RainGlass  translucent glass with drops: "Tiling" (u, v), "PanSpeed", "BaseOpacity", "DropOpacity", "Glint"
       "<engine python>" -I tools/ue_mcp.py py tools/script/scene8_cabin.py
"""
import math
import os
import random
import struct
import sys
import zlib

ART = r"C:\Jam\Art\S8"
PNG = os.path.join(ART, "T_S8_RainDrops.png")
FOLDER = "/Game/Take/Script/S8"
N = 512


def write_png(path, w, h, rows):
    raw = b"".join(b"\x00" + bytes(r) for r in rows)

    def chunk(t, d):
        c = struct.pack(">I", len(d)) + t + d
        return c + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    data = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))
    with open(path, "wb") as f:
        f.write(data)


def make_drops():
    random.seed(8)
    nx = [[0.0] * N for _ in range(N)]
    ny = [[0.0] * N for _ in range(N)]
    hh = [[0.0] * N for _ in range(N)]

    def stamp(cx, cy, rx, ry):
        for dy in range(-int(ry) - 1, int(ry) + 2):
            for dx in range(-int(rx) - 1, int(rx) + 2):
                u, v = dx / rx, dy / ry
                d2 = u * u + v * v
                if d2 >= 1.0:
                    continue
                h = math.sqrt(1.0 - d2)
                x, y = (cx + dx) % N, (cy + dy) % N
                if h > hh[y][x]:
                    hh[y][x] = h
                    nx[y][x] = u * 0.9
                    ny[y][x] = v * 0.9
    for _ in range(420):                                   # round drops, mostly small
        r = random.choice([1.6, 2.0, 2.4, 3.0, 3.5, 4.5, 6.0])
        stamp(random.randrange(N), random.randrange(N), r, r * random.uniform(0.9, 1.15))
    for _ in range(26):                                    # running drops: a head and a thin wet trail above it
        cx, cy = random.randrange(N), random.randrange(N)
        r = random.uniform(3.0, 5.5)
        stamp(cx, cy, r, r * 1.3)
        for k in range(1, int(random.uniform(10, 40))):
            stamp(cx + int(math.sin(k * 0.3) * 1.5), cy - k * 2, 1.4, 2.2)
    rows = []
    for y in range(N):
        row = []
        for x in range(N):
            h = hh[y][x]
            a, b = nx[y][x], ny[y][x]
            z = math.sqrt(max(0.0, 1.0 - a * a - b * b))
            row += [int((a * 0.5 + 0.5) * 255), int((b * 0.5 + 0.5) * 255), int((z * 0.5 + 0.5) * 255),
                    int(min(1.0, h * 1.6) * 255)]
        rows.append(row)
    os.makedirs(ART, exist_ok=True)
    write_png(PNG, N, N, rows)
    print("wrote", PNG)


# ---- inside the editor ----------------------------------------------------------------------------------------------

def editor_build():
    eal = unreal.EditorAssetLibrary
    mel = unreal.MaterialEditingLibrary
    tex_path = FOLDER + "/T_S8_RainDrops"
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", PNG)
    t.set_editor_property("destination_path", FOLDER)
    t.set_editor_property("destination_name", "T_S8_RainDrops")
    t.set_editor_property("replace_existing", True)          # on purpose: a re-run replaces it
    t.set_editor_property("automated", True)
    t.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
    tex = eal.load_asset(tex_path)
    tex.set_editor_property("srgb", False)
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_VECTOR_DISPLACEMENTMAP)
    eal.save_asset(tex_path, only_if_is_dirty=False)

    def fresh(name):
        p = FOLDER + "/" + name
        if eal.does_asset_exist(p):
            m = eal.load_asset(p)
            mel.delete_all_material_expressions(m)
            return m, p
        m = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, FOLDER, unreal.Material,
                                                                    unreal.MaterialFactoryNew())
        return m, p

    def scalar(m, name, v, x, y):
        e = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, x, y)
        e.set_editor_property("parameter_name", name)
        e.set_editor_property("default_value", float(v))
        return e

    def vector(m, name, c, x, y):
        e = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, x, y)
        e.set_editor_property("parameter_name", name)
        e.set_editor_property("default_value", unreal.LinearColor(c[0], c[1], c[2], 1.0))
        return e

    def op(m, cls, a, b, x, y, an="", bn=""):
        e = mel.create_material_expression(m, cls, x, y)
        assert mel.connect_material_expressions(a, an, e, "A")
        assert mel.connect_material_expressions(b, bn, e, "B")
        return e

    def done(m, p):
        mel.recompile_material(m)
        eal.save_asset(p, only_if_is_dirty=False)
        print("built", p)

    # M_S8_Paint
    m, p = fresh("M_S8_Paint")
    mel.connect_material_property(vector(m, "Color", (0.3, 0.3, 0.3), -400, -100), "",
                                  unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(scalar(m, "Metal", 0.0, -400, 50), "", unreal.MaterialProperty.MP_METALLIC)
    mel.connect_material_property(scalar(m, "Rough", 0.5, -400, 150), "", unreal.MaterialProperty.MP_ROUGHNESS)
    done(m, p)

    # M_S8_Glow
    m, p = fresh("M_S8_Glow")
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    e = op(m, unreal.MaterialExpressionMultiply, vector(m, "Color", (1.0, 0.7, 0.4), -500, 0),
           scalar(m, "Power", 4.0, -500, 150), -250, 0)
    mel.connect_material_property(e, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    done(m, p)

    # M_S8_Mirror: the capture flipped left-right (Flip = (1, 0): u -> 1 - u), a faint tint and a darker rim
    m, p = fresh("M_S8_Mirror")
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    tc = mel.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -900, 0)
    flip = vector(m, "Flip", (1.0, 0.0, 0.0), -900, 120)
    fm = mel.create_material_expression(m, unreal.MaterialExpressionComponentMask, -700, 120)
    fm.set_editor_property("r", True)
    fm.set_editor_property("g", True)
    fm.set_editor_property("b", False)
    fm.set_editor_property("a", False)
    assert mel.connect_material_expressions(flip, "", fm, "")
    sub = op(m, unreal.MaterialExpressionSubtract, fm, tc, -550, 60)
    ab = mel.create_material_expression(m, unreal.MaterialExpressionAbs, -420, 60)
    assert mel.connect_material_expressions(sub, "", ab, "")
    cap = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -280, 0)
    cap.set_editor_property("parameter_name", "Capture")
    cap.set_editor_property("texture", eal.load_asset(tex_path))
    cap.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    assert mel.connect_material_expressions(ab, "", cap, "UVs")
    tinted = op(m, unreal.MaterialExpressionMultiply, cap, vector(m, "Tint", (0.92, 0.95, 1.0), -280, 220), -80, 0,
                an="RGB")
    lit = op(m, unreal.MaterialExpressionMultiply, tinted, scalar(m, "Bright", 1.0, -80, 200), 80, 0)
    mel.connect_material_property(lit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    done(m, p)

    # M_S8_RainGlass: two layers of the drops (one still, one running with PanSpeed), lit glints when a lamp passes
    m, p = fresh("M_S8_RainGlass")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("two_sided", True)
    for nm in ("TLM_SURFACE_FORWARD_SHADING", "TLM_SURFACE"):
        v = getattr(unreal.TranslucencyLightingMode, nm, None)
        if v is not None:
            m.set_editor_property("translucency_lighting_mode", v)
            break
    tc = mel.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -1300, 0)
    til = vector(m, "Tiling", (3.0, 3.0, 0.0), -1500, 120)       # tiles across U and V (the pane's cm / 16)
    tlm = mel.create_material_expression(m, unreal.MaterialExpressionComponentMask, -1300, 120)
    for ch, on in (("r", True), ("g", True), ("b", False), ("a", False)):
        tlm.set_editor_property(ch, on)
    assert mel.connect_material_expressions(til, "", tlm, "")
    uv = op(m, unreal.MaterialExpressionMultiply, tc, tlm, -1120, 0)
    tm = mel.create_material_expression(m, unreal.MaterialExpressionTime, -1300, 260)
    pan = vector(m, "PanSpeed", (0.0, 0.12, 0.0), -1300, 360)
    pm = mel.create_material_expression(m, unreal.MaterialExpressionComponentMask, -1120, 360)
    pm.set_editor_property("r", True)
    pm.set_editor_property("g", True)
    pm.set_editor_property("b", False)
    pm.set_editor_property("a", False)
    assert mel.connect_material_expressions(pan, "", pm, "")
    moved = op(m, unreal.MaterialExpressionMultiply, tm, pm, -980, 300)
    k = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -1120, 200)
    k.set_editor_property("r", 0.73)                           # the running layer at another scale: never lines up
    uv2a = op(m, unreal.MaterialExpressionMultiply, uv, k, -980, 150)
    uv2 = op(m, unreal.MaterialExpressionAdd, uv2a, moved, -840, 200)
    s1 = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -700, 0)
    s1.set_editor_property("parameter_name", "Drops")
    s1.set_editor_property("texture", eal.load_asset(tex_path))
    s1.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    assert mel.connect_material_expressions(uv, "", s1, "UVs")
    s2 = mel.create_material_expression(m, unreal.MaterialExpressionTextureSample, -700, 250)
    s2.set_editor_property("texture", eal.load_asset(tex_path))
    s2.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    assert mel.connect_material_expressions(uv2, "", s2, "UVs")
    nsum = op(m, unreal.MaterialExpressionAdd, s1, s2, -500, 0, an="RGB", bn="RGB")
    one = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -500, 120)
    one.set_editor_property("r", 1.0)
    nrm = op(m, unreal.MaterialExpressionSubtract, nsum, one, -360, 0)
    mel.connect_material_property(nrm, "", unreal.MaterialProperty.MP_NORMAL)
    mask = op(m, unreal.MaterialExpressionMax, s1, s2, -500, 300, an="A", bn="A")
    opac = op(m, unreal.MaterialExpressionAdd, scalar(m, "BaseOpacity", 0.06, -360, 260),
              op(m, unreal.MaterialExpressionMultiply, mask, scalar(m, "DropOpacity", 0.45, -500, 420), -360, 380),
              -200, 300)
    mel.connect_material_property(opac, "", unreal.MaterialProperty.MP_OPACITY)
    glint = op(m, unreal.MaterialExpressionMultiply, mask,
               op(m, unreal.MaterialExpressionMultiply, scalar(m, "Glint", 0.0, -500, 560),
                  vector(m, "GlintColor", (1.0, 0.62, 0.3), -500, 660), -360, 600), -200, 500)
    mel.connect_material_property(glint, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(vector(m, "Color", (0.02, 0.025, 0.03), -200, -150), "",
                                  unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(scalar(m, "Rough", 0.04, -200, -60), "", unreal.MaterialProperty.MP_ROUGHNESS)
    done(m, p)

    # MI_S8_GTCar_Inside: the GT's own material, forced one-sided. From inside the cabin the roof, doors and painted
    # windows face away and vanish; the bonnet (facing up) still shows ahead through the windscreen.
    car_mat = FOLDER + "/S8_GTCar/Materials/S8_GTCar"
    ip = FOLDER + "/MI_S8_GTCar_Inside"
    mi = eal.load_asset(ip) if eal.does_asset_exist(ip) else unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "MI_S8_GTCar_Inside", FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", eal.load_asset(car_mat))
    bpo = mi.get_editor_property("base_property_overrides")
    bpo.set_editor_property("override_two_sided", True)
    bpo.set_editor_property("two_sided", False)
    mi.set_editor_property("base_property_overrides", bpo)
    mel.update_material_instance(mi)
    eal.save_asset(ip, only_if_is_dirty=False)
    try:
        print("built", ip, "parent", type(eal.load_asset(car_mat)).__name__, "two-sided:",
              eal.load_asset(car_mat).get_editor_property("two_sided"))
    except Exception as e:                                      # noqa: BLE001
        print("built", ip, "(parent two-sided unknown:", e, ")")


unreal = sys.modules.get("unreal")

if unreal is None:
    make_drops()
else:
    editor_build()
