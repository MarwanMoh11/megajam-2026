"""Editor side (phase 2): imports every WAV in C:\\Jam\\Audio into /Game/Take/Audio, replacing and saving.

Run inside the editor:  "$PY" -I tools/ue_mcp.py py tools/audio/import_audio.py
Only the ids in the manifest are imported (not stand-in copies or raw files). Loops are flagged looping; the band's
music is kept uncompressed-in-memory (no streaming) so its seek and loop point are sample-accurate. The C++ falls
back to the synthesised stand-ins for anything missing, so a partial import still plays.
"""
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else r"C:\Jam\megajam\tools\audio"
sys.path.insert(0, HERE)
import manifest as M  # noqa: E402

SRC = r"C:\Jam\Audio"
DEST = "/Game/Take/Audio"
LOOPING = {"mus_bed"} | {s[0] for s in M.SFX if s[4]}
MUSIC = {m[0] for m in M.MUSIC if m[2]}            # the bed and the heist cue

tasks = []
for iid in M.all_ids():
    path = os.path.join(SRC, iid + ".wav")
    if not os.path.exists(path):
        continue
    t = unreal.AssetImportTask()
    t.filename = path
    t.destination_path = DEST
    t.destination_name = iid
    t.replace_existing = True
    t.automated = True
    t.save = True
    tasks.append(t)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

done, flagged = 0, 0
for iid in M.all_ids():
    asset = unreal.load_asset(f"{DEST}/{iid}.{iid}")
    if not asset:
        continue
    done += 1
    changed = False
    if iid in LOOPING:
        asset.set_editor_property("looping", True)
        changed = True
    if iid in MUSIC:
        try:
            asset.set_editor_property("loading_behavior", unreal.SoundWaveLoadingBehavior.FORCE_INLINE)
            changed = True
        except Exception as e:  # property names move between versions: report, do not stop
            print(f"TAKE_AUDIO {iid}: loading_behavior not set ({e})")
    if changed:
        unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
        flagged += 1
print(f"TAKE_AUDIO imported {len(tasks)} files; {done} assets in {DEST}; {flagged} flagged (loop/inline)")
