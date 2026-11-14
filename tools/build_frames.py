"""Step 6 (Marwan, Nov 15: the red sight lines are too loud): M_SignalFaint, the faint sight frame.

Run in the editor (after tools/dress_auction.py's materials, whose M_SignalPattern block it reuses):
  "$PY" -I tools/ue_mcp.py py tools/build_frames.py

M_SignalFaint is M_SignalPattern built by the same lines of dress_auction.py, with the pattern mask Min'd with a
temporal dither of "Fade" (0..1): with the clip value at 0.5 a frame at Fade 0.22 keeps about a fifth of its pixels,
which TSR resolves to a faint line, and the pixels it keeps still write the red stencil, so the film grade keeps them
red. AWatcher sets Fade from PlanningOpacity (FTakeShapes::SetFaint).
"""
import os
import textwrap

import unreal

HERE = os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else r"C:\Jam\megajam\tools"
DRESS_PY = os.path.join(HERE, "dress_auction.py")
SRC = open(DRESS_PY, encoding="utf-8").read()

# dress_auction.py's definitions and constants only: everything before its run section (which saves the level).
CUT = SRC.rindex('\nif step("materials"):\n    build_instances()')
NS = {"__file__": DRESS_PY, "__name__": "dress_defs", "DRESS_ONLY": ("none",)}
exec(compile(SRC[:CUT], DRESS_PY, "exec"), NS)

# The M_SignalPattern block of build_materials, under the new name, ending in the dithered mask.
A = SRC.index('    s = fresh_material("M_SignalPattern")')
B = SRC.index('    save(DRESS + "/M_SignalPattern")', A)
BLOCK = SRC[A:B].replace("M_SignalPattern", "M_SignalFaint")
OLD_MASK = '    mel.connect_material_property(cu, "", unreal.MaterialProperty.MP_OPACITY_MASK)\n'
NEW_MASK = (
    '    fade = ps(s, "Fade", 0.22, -600, 900)\n'
    '    dither = mel.create_material_expression(s, unreal.MaterialExpressionMaterialFunctionCall, -350, 900)\n'
    '    dither.set_editor_property("material_function", unreal.load_asset(\n'
    '        "/Engine/Functions/Engine_MaterialFunctions02/Utility/DitherTemporalAA.DitherTemporalAA"))\n'
    '    link(fade, "", dither, ("Alpha Threshold", "AlphaThreshold"))\n'
    '    both = mel.create_material_expression(s, unreal.MaterialExpressionMin, -100, 600)\n'
    '    link(cu, "", both, "A")\n'
    '    link(dither, "", both, "B")\n'
    '    mel.connect_material_property(both, "", unreal.MaterialProperty.MP_OPACITY_MASK)\n')
assert BLOCK.count(OLD_MASK) == 1, "dress_auction.py's M_SignalPattern block changed: update build_frames.py"
BLOCK = BLOCK.replace(OLD_MASK, NEW_MASK)
exec(compile(textwrap.dedent(BLOCK) + 'save(DRESS + "/M_SignalFaint")\n', "build_frames", "exec"), NS)
print("FRAMES built /Game/Take/Dress/M_SignalFaint")
