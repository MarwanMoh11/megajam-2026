"""Imports C:\\Jam\\Audio\\sfx_drone_hum.wav to /Game/Take/Audio/sfx_drone_hum, looping, and saves it.

Run inside the editor (hold the editor lock):  "$PY" -I tools/ue_mcp.py py tools/audio/import_drone_hum.py
"""
import unreal

SRC = r"C:\Jam\Audio\sfx_drone_hum.wav"
DEST = "/Game/Take/Audio"
NAME = "sfx_drone_hum"

t = unreal.AssetImportTask()
t.filename = SRC
t.destination_path = DEST
t.destination_name = NAME
t.replace_existing = True
t.automated = True
t.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])

asset = unreal.load_asset(f"{DEST}/{NAME}.{NAME}")
if asset:
    asset.set_editor_property("looping", True)
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
    print(f"TAKE_AUDIO {NAME}: imported, looping, {asset.get_editor_property('duration'):.2f} s")
else:
    print(f"TAKE_AUDIO {NAME}: import failed")
