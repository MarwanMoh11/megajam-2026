# Crowd pass (Nov 15): makes /Game/Take/Cast/Crowd/M_CrowdBody, the guests' painted mannequin. Run in the editor:
#     "<engine python>" -I tools/ue_mcp.py py tools/crowd_material.py
# One material, a dynamic instance per guest (ACastMember::PaintCrowdBody sets the colours, the styles and the joints
# at the reference pose). It paints by the vertex's pre-skinned (reference pose) position, so the regions stay put
# whatever the body plays: skin on the head, neck and hands; sleeves or bare arms; short or opera gloves; a man's shirt
# front, collar and cuffs, trousers below the jacket's hem; a gown's neckline and sash; shoes; painted hair (full,
# receding, balding, bald, a woman's), brows, eyes, lips, moustaches, beards, goatees and sideburns. No textures.
# FaceA/FaceB/FaceC are the face's layout around the head joint (cm, Manny's size): tune them here, re-run.
import unreal

PATH, NAME = "/Game/Take/Cast/Crowd", "M_CrowdBody"
MEL = unreal.MaterialEditingLibrary

HLSL = r"""
float3 q = float3(abs(P.x), P.y, P.z);
float3 s = float3(abs(ShoulderPos.x), ShoulderPos.y, ShoulderPos.z);
float3 e = float3(abs(ElbowPos.x), ElbowPos.y, ElbowPos.z);
float3 w = float3(abs(WristPos.x), WristPos.y, WristPos.z);
float k = max((HeadPos.z - FootPos.z) / 154.4, 0.5);
float man = Style2.z;
float z = P.z;
float front = P.y - ChestPos.y;

float3 se = e - s; float tu = dot(q - s, se) / dot(se, se); float du = length(q - (s + saturate(tu) * se));
float3 ew = w - e; float tf = dot(q - e, ew) / dot(ew, ew); float df = length(q - (e + saturate(tf) * ew));
float3 hd = normalize(ew);
bool hand = dot(q - w, hd) > -0.5 * k && length(q - w) < 24.0 * k;
bool fore = !hand && df < 8.5 * k && tf > -0.05;
bool upper = !hand && !fore && du < 9.5 * k && tu > 0.12 && q.x > 15.0 * k;

float3 col = Upper;
if (man > 0.5) { if (z < PelvisPos.z - 14.0 * k) col = Lower; }
else
{
    if (z < PelvisPos.z + 6.0 * k) col = Lower;
    if (Style3.x > 0.5 && z >= PelvisPos.z + 6.0 * k && z < PelvisPos.z + 11.0 * k) col = Accent;
}
if (z < FootPos.z + 3.0 * k) col = Shoe;

if (man > 0.5)
{
    float vb = NeckPos.z - 30.0 * k;
    if (front > 3.0 * k && z > vb && z < NeckPos.z - 3.0 * k && q.x < (z - vb) * 0.24) col = Shirt;
    if (z >= NeckPos.z - 5.0 * k && z < NeckPos.z + 1.5 * k && q.x < 7.0 * k && (front > -2.0 * k || z > NeckPos.z - 1.0 * k)) col = Shirt;
    if (z >= NeckPos.z + 1.5 * k) col = Skin;
}
else
{
    if (z > NeckPos.z - 4.0 * k) col = Skin;
    if (Style1.z > 0.5 && Style1.z < 1.5)
    {
        float vb = NeckPos.z - 22.0 * k;
        if (front > 3.0 * k && z > vb && q.x < (z - vb) * 0.45) col = Skin;
    }
    if (Style1.z > 1.5 && z > ChestPos.z - 3.0 * k) col = Skin;
}

if (upper || fore)
{
    col = Style1.y > 0.5 ? Skin : Upper;
    if (man > 0.5 && fore && tf > 0.86) col = Shirt;
    if (Style1.x > 1.5 && (fore || tu > 0.55)) col = Glove;
}
if (hand) col = Style1.x > 0.5 ? Glove : Skin;

if (z >= NeckPos.z + 1.5 * k)
{
    float ez = HeadPos.z + FaceA.x * k;
    float es = FaceA.y * k;
    bool face = P.y > HeadPos.y + FaceA.z * k;
    float back = P.y - HeadPos.y;
    float hl = ez + FaceC.x * k;
    float cover = Style2.x;
    bool hair = false;
    if ((cover > 0.5 && cover < 1.5) || cover > 4.5)
        hair = z > hl + (cover > 4.5 ? 1.0 * k : 0.0) || (back < -1.0 * k && z > HeadPos.z - 1.0 * k) || (q.x > 6.0 * k && back < 4.0 * k && z > ez + 0.5 * k);
    else if (cover > 1.5 && cover < 2.5)
        hair = back < 3.0 * k && z > HeadPos.z - 1.0 * k && z < HeadPos.z + 11.0 * k;
    else if (cover > 2.5 && cover < 3.5)
        hair = z > hl + 2.5 * k || (back < -1.0 * k && z > HeadPos.z - 1.0 * k) || (q.x > 6.0 * k && back < 3.0 * k && z > ez + 0.5 * k);
    else if (cover > 3.5)
        hair = z > hl - 0.5 * k || (back < 2.0 * k && z > NeckPos.z + 2.0 * k) || (q.x > 6.0 * k && back < 5.0 * k && z > ez - 3.0 * k);
    if (hair) col = HairColour;

    float fh = Style2.y;
    if (fh > 2.5 && fh < 3.5 && z < ez - FaceB.z * k + 0.6 * k && back > 1.0 * k) col = HairColour;
    if (fh > 4.5 && q.x > 6.0 * k && back > -2.0 * k && back < 5.0 * k && z > ez - 5.0 * k && z < ez + 1.0 * k) col = HairColour;

    if (face)
    {
        if (length(float2(q.x - es, z - ez)) < FaceB.x * k) col = float3(0.012, 0.010, 0.010);
        if (abs(q.x - es) < 1.8 * k && abs(z - (ez + FaceB.y * k)) < 0.45 * k) col = HairColour;
        float mz = ez - FaceB.z * k;
        if (q.x < 2.2 * k && abs(z - mz) < 0.55 * k) col = Lips;
        bool mous = fh > 0.5 && fh < 4.5;
        float mw = fh < 1.5 ? 2.6 : 3.3;
        float mh = fh < 1.5 ? 0.35 : 0.75;
        if (mous && q.x < mw * k && abs(z - (mz + 1.3 * k)) < mh * k) col = HairColour;
        if (fh > 3.5 && fh < 4.5 && q.x < 2.0 * k && z < mz - 0.7 * k && z > mz - 4.5 * k) col = HairColour;
    }
}
return col;
"""

# name: default (Manny's joints; mid colours; the face layout)
PARAMS = [
    ("HeadPos", (0.0, 0.7, 162.6)), ("NeckPos", (0.0, -1.6, 152.8)), ("ChestPos", (0.0, 0.5, 141.1)),
    ("PelvisPos", (0.0, 2.3, 95.9)), ("ShoulderPos", (-19.0, -2.6, 143.6)), ("ElbowPos", (-35.0, -1.9, 120.9)),
    ("WristPos", (-47.8, 15.7, 104.5)), ("FootPos", (-14.1, -1.0, 8.2)),
    ("Skin", (0.45, 0.30, 0.20)), ("Upper", (0.05, 0.05, 0.06)), ("Lower", (0.02, 0.02, 0.02)),
    ("Shirt", (0.40, 0.39, 0.36)), ("Glove", (0.38, 0.37, 0.34)), ("Shoe", (0.01, 0.01, 0.01)),
    ("HairColour", (0.03, 0.02, 0.015)), ("Lips", (0.20, 0.02, 0.03)), ("Accent", (0.14, 0.02, 0.03)),
    ("Style1", (0.0, 0.0, 0.0)), ("Style2", (1.0, 0.0, 1.0)), ("Style3", (0.0, 0.0, 0.0)),
    ("FaceA", (7.0, 3.3, 6.0)), ("FaceB", (0.85, 2.3, 7.5)), ("FaceC", (4.5, 0.0, 0.0)),
]
ROUGHNESS = 0.72

full = PATH + "/" + NAME
if unreal.EditorAssetLibrary.does_asset_exist(full):
    mat = unreal.load_asset(full)
    MEL.delete_all_material_expressions(mat)
else:
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("used_with_skeletal_mesh", True)

pos_cls = getattr(unreal, "MaterialExpressionPreSkinnedPosition")
pos = MEL.create_material_expression(mat, pos_cls, -1400, -200)
interp = MEL.create_material_expression(mat, unreal.MaterialExpressionVertexInterpolator, -1100, -200)
MEL.connect_material_expressions(pos, "", interp, "")
custom = MEL.create_material_expression(mat, unreal.MaterialExpressionCustom, -300, 0)
custom.set_editor_property("code", HLSL)
custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
custom.set_editor_property("description", "CrowdPaint")
inputs = []
for n in ["P"] + [p[0] for p in PARAMS]:
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", n)
    inputs.append(ci)
custom.set_editor_property("inputs", inputs)
MEL.connect_material_expressions(interp, "", custom, "P")
for i, (n, v) in enumerate(PARAMS):
    vp = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -800, -600 + i * 60)
    vp.set_editor_property("parameter_name", n)
    vp.set_editor_property("default_value", unreal.LinearColor(v[0], v[1], v[2], 1.0))
    MEL.connect_material_expressions(vp, "", custom, n)
MEL.connect_material_property(custom, "", unreal.MaterialProperty.MP_BASE_COLOR)
rough = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 200)
rough.set_editor_property("r", ROUGHNESS)
MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset(full, only_if_is_dirty=False)
"M_CrowdBody: %d expressions" % MEL.get_num_material_expressions(mat)
