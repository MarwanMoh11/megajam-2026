"""Put the Blender-made Vance (tools/vance/build_vance.py) in the game, in the editor, under the editor lock:
  "<engine python>" -I tools/ue_mcp.py py tools/vance/import_vance.py 110

The lead's mesh is /Game/Take/Cast/Heroes/Julian/SKM_Hero_Julian (camera A's HeroMesh, its CastMember's, and
Scene8Car's VanceMesh all point there). This keeps that path: the Rodin mesh is copied to SKM_Hero_Julian_Rodin once
(the fallback: point HeroMesh back at it), then the FBX is imported over SKM_Hero_Julian on SK_Mannequin.
"""
import unreal

FBX = r"C:\Jam\Art\Vance\SKM_Vance.fbx"
DIR = "/Game/Take/Cast/Heroes/Julian"
NAME = "SKM_Hero_Julian"
BACKUP = DIR + "/SKM_Hero_Julian_Rodin"
MAT_DIR = "/Game/Take/Cast/Heroes/Vance"
SKELETON = "/Game/Characters/Mannequins/Meshes/SK_Mannequin"
EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
out = []

if not EAL.does_asset_exist(BACKUP):
    out.append("backup %s" % bool(EAL.duplicate_asset(DIR + "/" + NAME, BACKUP)))
    EAL.save_asset(BACKUP)

skel = unreal.load_asset(SKELETON)
t = unreal.AssetImportTask()
t.set_editor_property("filename", FBX)
t.set_editor_property("destination_path", DIR)
t.set_editor_property("destination_name", NAME)
t.set_editor_property("replace_existing", True)          # on purpose: Vance replaces the Rodin mesh at this path
t.set_editor_property("automated", True)
t.set_editor_property("save", True)
ui = unreal.FbxImportUI()
ui.set_editor_property("import_as_skeletal", True)
ui.set_editor_property("import_mesh", True)
ui.set_editor_property("import_animations", False)
ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
ui.set_editor_property("skeleton", skel)
ui.set_editor_property("create_physics_asset", False)
ui.set_editor_property("import_materials", True)
ui.set_editor_property("import_textures", False)
t.set_editor_property("options", ui)
AT.import_asset_tasks([t])
paths = [str(p) for p in (t.get_editor_property("imported_object_paths") or [])]
out.append("imported: " + ", ".join(paths))

skm = unreal.load_asset(DIR + "/" + NAME)
out.append("skeleton: %s" % skm.get_editor_property("skeleton").get_path_name())
b = skm.get_bounds()
out.append("bounds origin %s extent %s" % (b.origin, b.box_extent))
mats = skm.get_editor_property("materials")
for m in mats:
    mi = m.get_editor_property("material_interface")
    out.append("slot %s -> %s" % (m.get_editor_property("material_slot_name"), mi.get_path_name() if mi else None))
EAL.save_asset(DIR + "/" + NAME)
"\n".join(out)
