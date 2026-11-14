"""Step 4 pass 2: import the HUD's three font families as Font Face assets (docs/ART_STYLE.md 9, 15.2).

Run inside the editor (tools/ue_mcp.py, the editor's Python). Sources: Reel/SourceArt/Fonts/<family>/*.ttf, each with
its OFL.txt (outside Content, so the editor never auto-imports them). Output: /Game/Take/Fonts/FF_*.uasset, loading
policy Inline (the font data lives in the asset: nothing extra to stage), no UFont (the HUD builds its composite
fonts in C++, TakeHudKit.cpp). Cooked by DirectoriesToAlwaysCook=/Game/Take/Fonts in DefaultGame.ini.
Safe to re-run: replaces the assets in place.
"""
import os

import unreal

ROOT = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "SourceArt", "Fonts")
DEST = "/Game/Take/Fonts"
FACES = [
    ("FF_Limelight", "Limelight/Limelight-Regular.ttf"),
    ("FF_FellRoman", "IMFellEnglish/IMFeENrm28P.ttf"),
    ("FF_FellItalic", "IMFellEnglish/IMFeENit28P.ttf"),
    ("FF_JosefinSemiBold", "JosefinSans/JosefinSans-SemiBold.ttf"),
    ("FF_JosefinBold", "JosefinSans/JosefinSans-Bold.ttf"),
]

tasks = []
for name, rel in FACES:
    src = os.path.normpath(os.path.join(ROOT, rel))
    if not os.path.exists(src):
        raise RuntimeError("missing font source: " + src)
    factory = unreal.FontFileImportFactory()
    factory.set_editor_property("batch_create_font_asset", unreal.BatchCreateFontAsset.NO)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", src)
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", False)
    task.set_editor_property("factory", factory)
    tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

for name, _ in FACES:
    path = DEST + "/" + name
    face = unreal.EditorAssetLibrary.load_asset(path)
    if not face:
        raise RuntimeError("import failed: " + path)
    face.set_editor_property("loading_policy", unreal.FontLoadingPolicy.INLINE)
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    print("FONT", path, face.get_class().get_name(), face.get_editor_property("loading_policy"))
