"""Imports the Scene 5 WAVs (C:\\Jam\\Audio\\script\\s5, made by scene5_audio.py) into /Game/Take/Audio/Script/S5.

Inside the editor, holding the editor lock:
  "$PY" -I tools/ue_mcp.py py tools/script/scene5_audio_import.py 110
Replaces existing assets and saves them; only mus_s5_score loops.
"""
import os

import unreal

SRC = r"C:\Jam\Audio\script\s5"
DEST = "/Game/Take/Audio/Script/S5"
LOOPING = {"mus_s5_score"}

tasks = []
for f in sorted(os.listdir(SRC)):
    if not f.endswith(".wav"):
        continue
    t = unreal.AssetImportTask()
    t.filename = os.path.join(SRC, f)
    t.destination_path = DEST
    t.destination_name = f[:-4]
    t.replace_existing = True
    t.automated = True
    t.save = True
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

done = []
for t in tasks:
    n = t.destination_name
    a = unreal.load_asset(f"{DEST}/{n}.{n}")
    if not a:
        print(f"S5_AUDIO missing after import: {n}")
        continue
    a.set_editor_property("looping", n in LOOPING)
    unreal.EditorAssetLibrary.save_loaded_asset(a, only_if_is_dirty=False)
    done.append(f"{n} ({a.get_editor_property('duration'):.2f} s{', loop' if n in LOOPING else ''})")
print(f"S5_AUDIO imported {len(done)} of {len(tasks)} into {DEST}: {', '.join(done)}")
