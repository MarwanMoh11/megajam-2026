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
| Limelight Regular; IM Fell English roman and italic (`IMFeENrm28P.ttf`, `IMFeENit28P.ttf`); Josefin Sans SemiBold and Bold (static) | The game's HUD, slates and cards: `Reel/SourceArt/Fonts/<family>/` (TTF and `OFL.txt` side by side), imported as `/Game/Take/Fonts/FF_*` | Google Fonts: `https://raw.githubusercontent.com/google/fonts/main/ofl/limelight/`, `.../ofl/imfellenglish/` (Limelight by Eduardo Tunni; IM Fell by Igino Marini); Josefin Sans (Santiago Orozco) static instances from `fonts.gstatic.com` via `fonts.googleapis.com/css?family=Josefin+Sans:600,700`, licence from `.../ofl/josefinsans/OFL.txt` | SIL Open Font License 1.1 |
| Mannequins (Manny and Quinn meshes, the unarmed animation Blueprint and its animations, `M_Mannequin`) | `Reel/Content/Characters/Mannequins`: Julian, Maya and Vex in the grey-box build | Unreal Engine's Third Person template (Epic Games), which the project was created from | Unreal Engine EULA (engine content, usable in Unreal projects) |
| Voices (48 lines), sound effects (18) and the band's music (the bed, the heist cue, the sting, four warning phrases) | `/Game/Take/Audio` (from `C:\Jam\Audio`, see `docs/AUDIO_SCRIPT.md`) | Generated with ElevenLabs (text to speech `eleven_multilingual_v2` with premade voices Bill, Callum, Lily, Charlie, George, Adam, Brian, Chris, Harry, Daniel, Liam; sound effects `eleven_text_to_sound_v2`; ElevenLabs Music `music_v1`), Starter plan, Nov 14-15 2026. Files not generated yet are synthesised stand-ins made here (`tools/audio/standins.py`) | ElevenLabs Terms of Service, paid Starter plan: commercial use of the output allowed |

## Code libraries

| Library | Used in | Source | Licence |
|---|---|---|---|
| three.js 0.160.0, with its GLTFLoader, SkeletonUtils and postprocessing passes | `prototypes/l-final-cut/index.html` (bundled) | npm `three` | MIT |
| esbuild 0.24.0 (build tool only, not shipped) | bundling the L prototype | npm `esbuild` | MIT |
| three.js 0.160.0, with its GLTFLoader, SkeletonUtils and postprocessing passes | `prototypes/style/nitrate.js`, `prototypes/style/nitrate-three.html` (from the unpkg CDN) | npm `three` | MIT |
| numpy 2.4 (audio tools only, not shipped) | `tools/audio`: stand-in synthesis, tempo measurement, resampling, loudness (BS.1770) | PyPI `numpy`, in a venv at `C:\Jamenv-audio` made from the engine's Python 3.11 | BSD 3-Clause |
| miniaudio 1.73 (pyminiaudio; audio tools only, not shipped): the MP3 decoder for ElevenLabs' 128 kbps output, chosen because ffmpeg is not installed and it needs no admin rights | `tools/audio/audio_io.py` | PyPI `miniaudio` (Irmen de Jong; wraps David Reid's miniaudio / dr_mp3) | MIT (the C library: public domain or MIT-0) |

## AI tools

| Tool | What it did |
|---|---|
| Claude Code (Anthropic) | Wrote the L prototype (rules, solver, levels, three.js game, notes) |
| Claude Code (Anthropic) | Art direction: the style board, `docs/ART_STYLE.md`, the Unreal colour-grading LUTs (`prototypes/style/make_luts.py`), and the three.js look module `prototypes/style/nitrate.js` |
| Claude Code (Anthropic): Claude Fable 5.2 orchestrating, Claude Opus 5.5 subagents designing, building and reviewing | *Print It* step 4, the look: designed `docs/ART_STYLE.md` §15 with the `design` plugin's skills (design-system, ux-copy, accessibility-review) and the `frontend-design` plugin's frontend-design skill; built the film grade (`tools/build_look.py`), the film overlay, the HUD, stamps, cards and copy, the font import (`tools/build_fonts.py`) and the set's dressing, lighting and tints (`tools/dress_auction.py`); critiqued the HUD and the world picture on screenshots with the `design` plugin's design-critique skill and applied the fixes |
| ElevenLabs (text to speech, sound effects, Music) | Step 6: every voice line, sound effect and the band's music (`tools/audio/generate.py`); no AI at runtime |
| Claude Code (Anthropic): Claude Opus 5.5 subagent | Step 6, audio: the script (`docs/AUDIO_SCRIPT.md`), the audio tools (`tools/audio`), the C++ audio layer and the clock locked to the music |
