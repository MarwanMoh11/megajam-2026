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
| The same mannequins again for the whole cast (step 5): `SKM_Manny_Simple`, `SKM_Quinn_Simple`, poses from `MM_Idle` and `MF_Pistol_Idle_ADS`, the walk from `BS_Idle_Walk_Run` | Every guest, the band, the guards, the doorman, Vance and the bodyguard (`ACastMember`, placed by `tools/build_cast.py`); costumes and instruments are the engine's basic shapes (`/Engine/BasicShapes`) assembled in code | Unreal Engine's Third Person template and engine content (Epic Games) | Unreal Engine EULA |
| Niagara system templates `SimpleExplosion`, `DirectionalBurst`, `RadialBurst` | Duplicated as `/Game/Take/FX/NS_Take_Pyro`, `NS_Take_Sparks`, `NS_Take_Glass` (the pyro, the sound desk's feedback, the champagne carts' glass) | Unreal Engine's Niagara plugin, `/Niagara/DefaultAssets/Templates/Systems` (Epic Games) | Unreal Engine EULA |
| Voices (48 lines), sound effects (18) and the band's music (the bed, the heist cue, the sting, four warning phrases) | `/Game/Take/Audio` (from `C:\Jam\Audio`, see `docs/AUDIO_SCRIPT.md`) | Generated with ElevenLabs (text to speech `eleven_multilingual_v2` with premade voices Bill, Callum, Lily, Charlie, Chris, Harry, and voices made with ElevenLabs Voice Design (`eleven_multilingual_ttv_v2`) for the chief, Vance, the doorman, the broadcast producer and the alarm guards; sound effects `eleven_text_to_sound_v2`; ElevenLabs Music `music_v1`), Starter plan, Nov 14-15 2026. Files not generated yet are synthesised stand-ins made here (`tools/audio/standins.py`) | ElevenLabs Terms of Service, paid Starter plan: commercial use of the output allowed |
| 23 props: the podium, the case's pedestal and reel, the champagne carts, the sound desk, the lobby chandelier, the grand piano, and the repeated furniture (hall chairs, columns, cocktail tables, the bar and back bar, stools, picture frames, busts and urns, stanchions, sofas, road cases, crates, cloak rails, film cameras, light stands) | `/Game/Take/Props` (from `C:\Jam\Art\Rodin`), placed by `tools/rodin/import_props.py`; each prop's prompt and task id are listed under *Hyper3D Rodin assets* below | Generated with Hyper3D Rodin (Deemos), API `Gen-2.5-Low`, PBR textures, Nov 14 2026, 11.5 API credits | Hyper3D Copyright License (https://hyper3d.ai/legal/copyright-license): purchased content may be modified, published and sold in a product; no attribution required |

## Code libraries

| Library | Used in | Source | Licence |
|---|---|---|---|
| three.js 0.160.0, with its GLTFLoader, SkeletonUtils and postprocessing passes | `prototypes/l-final-cut/index.html` (bundled) | npm `three` | MIT |
| esbuild 0.24.0 (build tool only, not shipped) | bundling the L prototype | npm `esbuild` | MIT |
| three.js 0.160.0, with its GLTFLoader, SkeletonUtils and postprocessing passes | `prototypes/style/nitrate.js`, `prototypes/style/nitrate-three.html` (from the unpkg CDN) | npm `three` | MIT |
| numpy 2.4 (audio tools only, not shipped) | `tools/audio`: stand-in synthesis, tempo measurement, resampling, loudness (BS.1770) | PyPI `numpy`, in a venv at `C:\Jam\venv-audio` made from the engine's Python 3.11 | BSD 3-Clause |
| miniaudio 1.73 (pyminiaudio; audio tools only, not shipped): the MP3 decoder for ElevenLabs' 128 kbps output, chosen because ffmpeg is not installed and it needs no admin rights | `tools/audio/audio_io.py` | PyPI `miniaudio` (Irmen de Jong; wraps David Reid's miniaudio / dr_mp3) | MIT (the C library: public domain or MIT-0) |

## AI tools

| Tool | What it did |
|---|---|
| Claude Code (Anthropic) | Wrote the L prototype (rules, solver, levels, three.js game, notes) |
| Claude Code (Anthropic) | Art direction: the style board, `docs/ART_STYLE.md`, the Unreal colour-grading LUTs (`prototypes/style/make_luts.py`), and the three.js look module `prototypes/style/nitrate.js` |
| Claude Code (Anthropic): Claude Fable 5.2 orchestrating, Claude Opus 5.5 subagents designing, building and reviewing | *Print It* step 4, the look: designed `docs/ART_STYLE.md` §15 with the `design` plugin's skills (design-system, ux-copy, accessibility-review) and the `frontend-design` plugin's frontend-design skill; built the film grade (`tools/build_look.py`), the film overlay, the HUD, stamps, cards and copy, the font import (`tools/build_fonts.py`) and the set's dressing, lighting and tints (`tools/dress_auction.py`); critiqued the HUD and the world picture on screenshots with the `design` plugin's design-critique skill and applied the fixes |
| Claude Code (Anthropic): Claude Opus 5.5, two sessions in turn | *Print It* step 5, characters and props: the cast as costumed mannequins (`ACastMember`: costume kits from basic shapes on the bones, frozen and seated poses, the walk, the standing idle, Julian's bow), the band's instruments and its rim light, the devices' Niagara effects (`ATakeFX`), `tools/build_cast.py` that places them, and the checks (scripted play-in-editor tests, frame time, the packaged runs with real keys) |
| ElevenLabs (text to speech, Voice Design, sound effects, Music) | Step 6: every voice line, sound effect and the band's music (`tools/audio/generate.py`); no AI at runtime |
| Claude Code (Anthropic): Claude Opus 5.5 subagent | Step 6, audio: the script (`docs/AUDIO_SCRIPT.md`), the audio tools (`tools/audio`), the C++ audio layer and the clock locked to the music |
| Hyper3D Rodin (Deemos), text to 3D | The 23 props in `/Game/Take/Props` (`tools/rodin/generate.py`); no AI at runtime |
| Claude Code (Anthropic): Claude Opus 5.5 | The props pass: the Rodin prompts and client (`tools/rodin`), the import, materials and placement script (`tools/rodin/import_props.py`: the meshes fit to the grey boxes, which keep their collision), and the set's procedural surfaces in `tools/dress_auction.py` (parquet, marble, damask, the aisle runners, plywood, concrete: value patterns in the M_Dress material, no textures) |

## Hyper3D Rodin assets

Every prompt also carried this style line: "1950s mid-century, chunky simplified silhouette like a stage prop, slightly exaggerated proportions, single object, centred, matte, neutral flat lighting, no baked shadows, no text, no logos, plain background". Request: tier Gen-2.5-Low, texture_mode high, mesh_mode Raw, quality_override 15000, geometry_file_format glb, material PBR, preview_render true.

| Asset | Rodin task id | Prompt |
|---|---|---|
| `/Game/Take/Props/Backbar/SM_Backbar` | `bbc7fb4f-cf7b-4237-8609-8f56dc1bdd1d` | a back-bar shelving unit, wide and tall, three shelves full of dark green glass bottles, a plain mirror panel behind them, dark walnut wood |
| `/Game/Take/Props/Bar/SM_Bar` | `173c2609-8a7d-40c7-b502-3ee25274567d` | a short straight section of a hotel bar counter, about twice as long as it is tall, a front of vertical rounded wooden ribs, a flat dark top with a rounded edge and a foot rail, dark walnut wood and pewter grey metal |
| `/Game/Take/Props/Bust/SM_Bust` | `062cecc9-1a71-461e-bf8b-ec22cabc500f` | a classical stone bust of a man on a short square plinth, pale grey stone |
| `/Game/Take/Props/Camera/SM_Camera` | `75177f08-9753-4332-b717-c06f6cd1a8e9` | a vintage movie camera with two round film reels on top, on a wooden tripod, black metal |
| `/Game/Take/Props/Cart/SM_Cart` | `e0c9f2a9-d2a0-4838-8e5f-ab7fc7701902` | a hotel room-service trolley with two shelves, a push handle at one end and four small castor wheels, an ice bucket holding two champagne bottles on the top shelf, chrome metal frame and white linen cloth |
| `/Game/Take/Props/Chair/SM_Chair` | `4c84af48-2195-453f-81c7-17e25382fadf` | an auction room side chair, upholstered padded seat and straight padded back, slim tapered dark wood legs, charcoal fabric |
| `/Game/Take/Props/Chandelier/SM_Chandelier` | `7908ba2c-34e6-4661-b28c-936f71ae8093` | a sputnik chandelier, a small central sphere with straight thin rods radiating in all directions, each rod ending in a round frosted bulb, hanging from a short ceiling rod, pewter grey metal and white bulbs |
| `/Game/Take/Props/Coatrack/SM_Coatrack` | `d6149b49-5019-4fd2-b983-7a00d569168d` | a cloakroom clothes rail on wheels with dark coats and hats hanging from hangers, black metal |
| `/Game/Take/Props/Cocktail/SM_Cocktail` | `a961a864-96e3-4340-a24c-8df07cbfd323` | a tall round cocktail bar table on a single slim pedestal stem with a heavy round base, dark wood top, pewter grey metal stem |
| `/Game/Take/Props/Column/SM_Column` | `cbe79ec8-a205-4ee5-a73a-0fa85b92b4e0` | a tall slender classical fluted column, about six times as tall as it is wide, with a plain square base and a simple square capital, pale grey stone |
| `/Game/Take/Props/Crate/SM_Crate` | `624d939e-c1ae-43a2-a332-59d0439c6cd4` | a wooden shipping crate made of planks with diagonal cross braces, plain unpainted wood |
| `/Game/Take/Props/Desk/SM_Desk` | `12743c9f-6879-4a84-a763-3dd9816bf06b` | a radio broadcast sound mixing console on a sturdy wooden desk, a sloped control panel with rows of large round knobs and two round needle meters, a thick bundle of cables at the back, dark grey metal and dark wood |
| `/Game/Take/Props/Frame/SM_Frame` | `4b5a01f8-ee5f-4134-b84f-ada6d5f94794` | an empty ornate rectangular picture frame with carved moulding, landscape format, open centre, dark pewter grey metal |
| `/Game/Take/Props/Lightstand/SM_Lightstand` | `38022819-ec0c-4e8c-820b-a84730fdd9c2` | a vintage film studio spotlight, a big round fresnel lamp head with barn doors, on a tall three-legged stand, black metal |
| `/Game/Take/Props/Pedestal/SM_Pedestal` | `39a0838f-6bc1-4a64-adeb-ab23df23abf5` | a museum display pedestal, a square column with vertical fluting on a stepped square base and a flat square top, pale grey painted wood |
| `/Game/Take/Props/Piano/SM_Piano` | `36132c8a-70e7-435e-902e-a8068b39d930` | a black lacquered baby grand piano with the lid propped half open on a thick stick, black and white keys, three short thick legs |
| `/Game/Take/Props/Podium/SM_Podium` | `583acdb5-3d07-49d5-810e-8a05f74139b2` | an auctioneer's lectern podium, tall box body with a slanted reading top and a narrow ledge, fluted front panel, square plinth base, dark walnut wood with pale grey pewter trim |
| `/Game/Take/Props/Reel/SM_Reel` | `40da3957-261d-4169-af22-884f624f7a9a` | a large vintage film reel with six round holes in each flange, standing upright on edge in a small low cradle stand, plain silver metal |
| `/Game/Take/Props/Roadcase/SM_Roadcase` | `f3e17e9d-672f-4f5a-9642-ceaec1db7e03` | a black theatrical road case on castor wheels with metal corner protectors, edge trim and latches |
| `/Game/Take/Props/Sofa/SM_Sofa` | `8a9909bf-cb0d-4b40-98db-a19a395efc7e` | a three-seat sofa, low rounded back and arms, tapered dark wood legs, charcoal velvet |
| `/Game/Take/Props/Stanchion/SM_Stanchion` | `4817c4bb-1539-45a7-a8c0-e448bc1d43b8` | a crowd control stanchion post, a slim pole with a round ball top and a heavy round base, pewter grey metal |
| `/Game/Take/Props/Stool/SM_Stool` | `eb7f6f30-8456-492a-9141-5f2f0558081c` | a bar stool with a round padded seat, a chrome pedestal stem, a foot ring and a round base, charcoal leather |
| `/Game/Take/Props/Vase/SM_Vase` | `68897d02-8b48-4dc1-b468-72d7fe7faed1` | a large classical urn vase with two handles on a short square plinth, pale grey stone |
