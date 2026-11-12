# Credits

Every asset not made in this repo, with its source and licence, plus the AI tools used. The submission form asks for
both. Add a row when you import something.

## Assets

| Asset | Used in | Source | Licence |
|---|---|---|---|
| RobotExpressive.glb (stand-in rigged star, until our own model exists) | `prototypes/l-final-cut/index.html` (embedded) | three.js examples, `https://threejs.org/examples/models/gltf/RobotExpressive/RobotExpressive.glb`. Model by Tomás Laulhé (Quaternius); modifications by Don McCurdy | CC0 1.0 |
| Limelight font | `prototypes/l-final-cut/index.html` (embedded, latin subset) | Google Fonts | SIL Open Font License 1.1 |
| IM Fell English font (roman and italic) | `prototypes/l-final-cut/index.html` (embedded, latin subset) | Google Fonts | SIL Open Font License 1.1 |
| Josefin Sans font | `prototypes/l-final-cut/index.html` (embedded, latin subset) | Google Fonts | SIL Open Font License 1.1 |
| RobotExpressive.glb (same stand-in) | `prototypes/style/nitrate-three.html` (loaded at runtime, not stored in the repo) | three.js examples, same URL as above | CC0 1.0 |
| Limelight, IM Fell English, Josefin Sans fonts | `prototypes/style/nitrate.js` and `nitrate-three.html` (loaded from Google Fonts at runtime) | Google Fonts | SIL Open Font License 1.1 |
| Limelight, IM Fell English, Josefin Sans, Fraunces, Archivo, Londrina Solid, Jost fonts | `prototypes/style/board.html` (embedded; the art direction board, not shipped) | Google Fonts | SIL Open Font License 1.1 |

## Code libraries

| Library | Used in | Source | Licence |
|---|---|---|---|
| three.js 0.160.0, with its GLTFLoader, SkeletonUtils and postprocessing passes | `prototypes/l-final-cut/index.html` (bundled) | npm `three` | MIT |
| esbuild 0.24.0 (build tool only, not shipped) | bundling the L prototype | npm `esbuild` | MIT |
| three.js 0.160.0, with its GLTFLoader, SkeletonUtils and postprocessing passes | `prototypes/style/nitrate.js`, `prototypes/style/nitrate-three.html` (from the unpkg CDN) | npm `three` | MIT |

## AI tools

| Tool | What it did |
|---|---|
| Claude Code (Anthropic) | Wrote the L prototype (rules, solver, levels, three.js game, notes) |
| Claude Code (Anthropic) | Art direction: the style board, `docs/ART_STYLE.md`, the Unreal colour-grading LUTs (`prototypes/style/make_luts.py`), and the three.js look module `prototypes/style/nitrate.js` |
