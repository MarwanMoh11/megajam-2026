"""Inside the editor (hold the editor lock): import tools/script/voice_fill.py's WAVs, replacing and saving.
  "$PY" -I tools/ue_mcp.py py tools/script/voice_fill_import.py
"""
import os
import unreal

OUT = r"C:\Jam\Audio\script\fill"
tasks = []
for f in sorted(os.listdir(OUT)):
    if not f.endswith(".wav"):
        continue
    n = f[:-4]
    t = unreal.AssetImportTask()
    t.filename = os.path.join(OUT, f)
    t.destination_path = "/Game/Take/Audio/Script/Fill" if n.startswith("vo_fill_") else "/Game/Take/Audio"
    t.destination_name = n
    t.replace_existing = True
    t.automated = True
    t.save = True
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
ok = sum(1 for t in tasks if unreal.EditorAssetLibrary.does_asset_exist(t.destination_path + "/" + t.destination_name))
unreal.log("VOICE_FILL imported %d of %d" % (ok, len(tasks)))
