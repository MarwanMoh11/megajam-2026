# Art style: Nitrate

**Status: locked by Marwan on Thu Nov 12.** The look is locked; the board's shapes are not. Its stick figures,
boxes and canned animations were a mock-up of the treatment only. Characters will be full models, rigged and
animated, with this treatment applied on top. The three.js prototypes use the same look through
`prototypes/style/nitrate.js` (demo: `prototypes/style/nitrate-three.html`).

Nitrate is a silent film from 1915 to 1927 (nitrate was the film stock of the era). The image is toned black and
white in a 4:3 gate, and colour appears only where it's stencilled on by hand: red for danger, gold for the hero and the
payoff. It works for both candidates. K's cross-cutting is the era's own "last-minute rescue", and L's timed gags (safe,
piano, anvil) are its stunt comedy. Where K and L differ, this file says so.

Words used below: **value** is how light or dark something is, ignoring colour. **Toning** is the era's way of
colouring a black-and-white print: the dark and middle tones take a colour, and the whites stay clean. A **LUT**
(look-up table) is a small image Unreal uses to remap every colour on screen; here it does the toning.

## 1. Five rules (when in doubt, these win)

1. **Value tells the story.** The hero is the lightest thing on screen, the heavies are the darkest, and the set sits
   in between. Section 3 gives the numbers, and section 13 checks them in a screenshot.
2. **Colour is only for meaning.** Stencil red means danger, now: a strike window, a falling crusher, a room about to be
   lost. Stencil gold means the hero, a bonus, or the Director's Cut. Nothing else on screen is saturated. Generated
   textures never keep their colour.
3. **One tone per location.** K: the Rooftop is night blue, the Mill is lamplight amber, the Railway is dusk green.
   L: every level is silver (neutral), so the crushers' red is the only colour. A cut changes the tone in the same
   frame.
4. **Steady frame, film at the edges.** Grain, flicker and dust sit on top, quietly. The 3D image never shakes or
   weaves during play. The film effects get loud only at cuts and at the end of a take.
5. **Desktop and Web look the same.** No Lumen, no Nanite, no Film Grain setting and no Cinematic DoF, because the
   mobile renderer used by the Web build has none of them. Section 10 has the evidence.

## 2. Where K and L differ

| | K. Meanwhile... | L. The Tight Edit |
|---|---|---|
| Tone | Three tones, one per room (Night, Amber, Green) | Silver for every level |
| Frame | 4:3 gate; the other rooms show as round cameos in the side margins | 4:3 gate at the top; the edit bench (timeline) runs full width below it |
| The cut | Hard cut to the new room, one white flash frame, a "Meanwhile..." intertitle over the lower third for about a second while play continues | Each take ends with an iris-out on the star, then a result intertitle card |
| Danger read | The heavy in its strike window turns stencil red; a room's danger bar turns red and flashes from 75% | A red disc appears on the floor under a crusher 0.6 s before impact, and the crusher's front face turns red |
| Characters | Hero, heavy (one mesh, reused), two helpless ones | The star (the hero mesh, reused) |

## 3. Palette

| Role | Hex | Where it goes |
|---|---|---|
| Ink | `#0D0C0B` | Heavies, intertitle card backgrounds, the gate and margins |
| Silver | `#7A756D` | The set: mid-grey reference |
| Bone | `#C9C2B4` | Lit tops, painted backdrops |
| Paper | `#F1EADB` | The hero, helpless ones, all intertitle and HUD text |
| Stencil red | `#D3301F` | Danger only (rule 2) |
| Stencil gold | `#E9B949` | Hero scarf, "Just in time!", near-miss numbers, the Director's Cut medal |

Tones, as dark / mid / light stops (the LUTs in `prototypes/style/luts/` are built from exactly these):

| Tone | Dark | Mid | Light | Used for |
|---|---|---|---|---|
| Night | `#0B1420` | `#5F7387` | `#EEF0EA` | K: the Rooftop |
| Amber | `#1A1108` | `#9B7A4D` | `#F6EBD3` | K: the Mill |
| Green | `#0E1712` | `#6C806D` | `#EEF0E4` | K: the Railway |
| Silver | `#0D0C0B` | `#7A756D` | `#F1EADB` | L: all levels; title screen and menus |

**The value ladder** (material base colour, in Unreal's linear 0-1 units; the sRGB column is roughly what a lit
screenshot should show, before toning):

| Role | Base colour band (linear) | Lit screenshot (sRGB 0-255) |
|---|---|---|
| Hero costume | 0.70 – 0.85 | 215 – 245 |
| Helpless ones | 0.55 – 0.75 | 195 – 230 |
| Set: props, walls | 0.10 – 0.30 | 90 – 150 |
| Floors | 0.12 – 0.22 | 95 – 130 |
| Backdrops (unlit) | 0.30 – 0.45 | 150 – 180 |
| Heavies | 0.01 – 0.03 | 20 – 45 |

## 4. Materials

One master material, so every generated asset goes through the same treatment.

**`M_Nitrate`** (Material Domain Surface, Shading Model Default Lit, Blend Mode Opaque):
- Parameters: `BaseTexture` (the generated albedo), `BandLow` and `BandHigh` (scalars from the ladder above),
  `Contrast` (scalar, default 1.0), `NormalTexture` (optional), `NormalStrength` (default 0.6).
- Base Color = `lerp(BandLow, BandHigh, saturate(pow(Desaturation(BaseTexture, 1), Contrast)))`. The Desaturation
  node at fraction 1 throws away the generated colour; the lerp squeezes what's left into the role's band, so a
  generated texture can add detail but can never move a heavy out of black or a hero out of white.
- Metallic 0, Specular 0.3, Roughness 0.75 everywhere. No metals: with reflections off they read as noise.
- Normal: the generated normal map at `NormalStrength`, or none.
- Ignore any generated roughness, metallic or AO maps. They carry baked lighting, which fights the key light.

**Instances** (make all of these on day one, even before assets exist; grey boxes use them too):

| Instance | BandLow | BandHigh | Notes |
|---|---|---|---|
| `MI_Hero` | 0.70 | 0.85 | Hero and star costume |
| `MI_Helpless` | 0.55 | 0.75 | K only |
| `MI_Heavy` | 0.01 | 0.03 | All heavies |
| `MI_Set` | 0.10 | 0.30 | Props, walls, crushers at rest |
| `MI_Floor` | 0.12 | 0.22 | Floors and ground |
| `MI_SetDark` | 0.04 | 0.12 | Posts, beams, rails, hats, trim |

**Unlit materials** (Shading Model Unlit, Emissive Color only, so the colour is exact under any light):
- `M_Backdrop`: painted sky and skyline flats. Emissive = `lerp(0.30, 0.45, Desaturation(Texture, 1))`.
- `M_StencilRed`: emissive `#D3301F` (in linear, about 0.65, 0.03, 0.013). For heavies in their strike window,
  crusher front faces in their danger window, and the floor tell disc in L.
- `M_StencilGold`: emissive `#E9B949`. For the hero's scarf and the medal.
- `M_Window` (K, the Mill): emissive 0.9 grey, for windows that should glow.

**Switching to red is a material swap on the same frame the AI decides to strike** (rule from CLAUDE.md: nothing waits
for a timer). Use `SetMaterial` on the heavy's body slot, or a scalar parameter that lerps base colour to red, not a
fade.

## 5. Lighting

**Project settings (once):** Rendering → Global Illumination → **Dynamic Global Illumination Method: None**;
Reflections → **Reflection Method: None**. This turns Lumen off on desktop too, so desktop matches Web, and the hard,
bounce-free light is part of the look. Rendering → Mobile → **Mobile HDR: on**: without it, the mobile renderer
supports no post-process at all (see section 10).

**Per room (K) or per level (L):**
- **One Directional Light**, Mobility Movable, Light Color pure white, Intensity left at its default to start.
  Rotation for K rooms: **Pitch -40, Yaw 135, Roll 0**, with the room's camera looking along +X. The light then comes
  from behind the set and from the left, and shadows fall long, towards the camera and to the right. For L levels,
  use **Pitch -55, Yaw 30**: light from the front left, so the star and crushers are lit from the camera side.
- **Source Angle 0.3** for hard-edged shadows. (On the mobile renderer, Source Angle isn't supported for movable
  lights, so Web shadows are hard anyway.)
- **One Sky Light**, Mobility Movable, Source Type SLS Captured Scene, Intensity Scale 0.5 to start. This is the fill.
- **Tuning, by numbers, not taste:** take a screenshot. Change the Post Process **Exposure Compensation** until a lit
  floor reads sRGB 95-130. Then change the Sky Light **Intensity Scale** until floor in shadow reads 45-70 (about half
  the lit floor). Shadows should be a clear, readable shape.
- **Optional, K's Mill only:** real holes in the back wall where the windows are, so the directional light throws
  window-shaped patches on the floor. On the directional light, **Light Shafts** (supported on mobile) give the beams.
- **Optional depth:** an Exponential Height Fog with a low **Fog Density** (start 0.02) and neutral grey colour.
  Volumetric Fog stays off; the mobile renderer doesn't have it.

## 6. Camera

Each K room has its own **Cine Camera Actor**; L has one per level.
- **Filmback:** Sensor Width 24.89 mm, Sensor Height 18.67 mm (a 4:3 sensor). Under Camera Options, tick
  **Constrain Aspect Ratio**: Unreal then pillarboxes the 4:3 picture on a 16:9 screen, and the HUD lives in the black
  side margins.
- **Lens:** K rooms, **Current Focal Length 19 mm**. L levels, **20 mm**. (These match the board's framing: about 52°
  and 50° of vertical view.)
- **Focus Settings → Focus Method: Disable.** No depth of field: Cinematic DoF doesn't exist on the mobile renderer, and
  a blurred background hurts the read.
- **Placement, K:** 680 cm above the floor, pitched down 32°, about 820 cm back from the hero's start mark, hero a
  little below frame centre.
- **Placement, L:** 390 cm above the floor, pitched down 13°, about 1050 cm in front of the star's path, the whole path
  in frame with 1 m of floor to spare at each end.
- **The camera doesn't move during play.** The only camera motion allowed is a short hit shake: a Camera Shake with a
  Perlin noise pattern, 0.15 s, small amplitude, on the hero landing a hit (K) or a crusher impact (L).
- **A cut is instant:** `SetViewTargetWithBlend` with Blend Time 0, called in the same frame as the key press.

## 7. Post-process settings

One **Post Process Volume** with **Infinite Extent (Unbound)** ticked holds everything shared. Each Cine Camera
overrides only the LUT, in its own Post Process settings, with Post Process Blend Weight 1. Since the cut switches
cameras, the tone changes in the same frame.

| Category | Setting | Value |
|---|---|---|
| Exposure | Metering Mode | Manual |
| Exposure | Exposure Compensation | Tuned per section 5 (start 0) |
| Exposure | Apply Physical Camera Exposure | Off |
| Lens → Bloom | Method | Standard |
| Lens → Bloom | Intensity | 0.4 |
| Lens → Bloom | Threshold | 1.0 |
| Lens → Chromatic Aberration | Intensity | 0 |
| Lens → Lens Flares | Intensity | 0 |
| Lens → Image Effects | Vignette Intensity | 0.6 |
| Color Grading → Misc | Color Grading LUT | Unbound volume: `LUT_Nitrate_Silver`. Each K camera: its room's LUT |
| Color Grading → Misc | Color Grading LUT Intensity | 1.0 |
| Color Grading | Global / Shadows / Midtones / Highlights | Leave at defaults. **Don't set Global Saturation to 0**: it would grey out the stencil red too |
| Film | Slope, Toe, Shoulder, Black Clip, White Clip | Defaults. If the image looks flat, raise Toe to 0.6, and change nothing else |
| Film Grain | Film Grain Intensity | 0 (grain is the UI overlay in section 8, so both builds match) |
| Rendering Features → Motion Blur | Amount | 0 |

**LUT textures:** run `python3 prototypes/style/make_luts.py` (stdlib only), or use the PNGs already in
`prototypes/style/luts/`. Import them, then on each one set **Mip Gen Settings: NoMipMaps** and **Texture Group:
Color Lookup Table** (per Epic's LUT page). Unreal's LUT format is a 16×16×16 cube unwrapped to 256×16, which is what
the script writes. **First apply `LUT_Neutral` and check the frame doesn't change.** If it does, the channel layout is
wrong for this engine build: stop and report. The toned LUTs leave saturated pixels alone, which is how the stencil red
and gold survive the grade.

**Fallback if LUTs misbehave (for example, on the Web build):** remove the LUT and use Color Grading → Misc → **Scene
Color Tint** with a pale version of the room's tone (Night `#DDE6F2`, Amber `#F2E6D2`, Green `#E2EDE2`, Silver
`#FFFFFF`). The materials are already grey, so this still gives a toned black and white, with slightly tinted whites.

## 8. Film overlay (UI, both builds)

A full-screen UMG widget, `WBP_FilmGate`, sits above the 3D view and below the HUD. UMG is drawn identically by both
renderers, which is why the film effects live here.

- **Gate:** a black mask with the 4:3 picture area cut out, corner radius 2.5% of screen height, sized and centred to
  match the constrained 4:3 view.
- **Grain:** an Image with a User Interface material (Blend Mode Translucent). A 256×256 tiling grey noise texture,
  tiled about 6 times across the screen, UV offset jumped to a random position 24 times a second (use
  `floor(Time*24)/24` as the seed). Grey centred on 0.5, opacity 0.10. At 0.10 grain must be visible on a still frame
  and invisible in motion; if it isn't visible, raise it to 0.14 and no further.
- **Flicker:** a full-screen black Image whose opacity is re-rolled 24 times a second between 0 and 0.04.
- **Dust and scratches:** at most two dust specks a second (2-6 px, dark or light), and one vertical scratch about
  every 8 s, lasting 2 frames. Never more.
- **Splice at a cut (K):** a paper-white full-picture flash at 85% opacity on the cut frame only, plus a black frame-line
  bar that grows from the bottom of the gate to 4% of its height and back over 3 frames.
- **Iris (L, end of take):** a black mask with a circular hole centred on the star, closing from full screen to 0 in
  0.45 s, ease-out.
- All film effects tick at 24 fps (quantised time). Gameplay and the HUD run at full frame rate.

## 9. UI and type

**Fonts** (all SIL Open Font License 1.1: free to embed in a game and ship; put each `OFL.txt` next to the font and
list it in `docs/CREDITS.md`):

| Font | Use | Get it |
|---|---|---|
| Limelight | Titles, "Just in time!", medals, big numbers | `https://raw.githubusercontent.com/google/fonts/main/ofl/limelight/Limelight-Regular.ttf` |
| IM Fell English (roman and italic) | Intertitle text | `.../ofl/imfellenglish/IMFeENrm28P.ttf` and `IMFeENit28P.ttf` |
| Josefin Sans (SemiBold, Bold) | HUD labels, key prompts, small caps | fonts.google.com/specimen/Josefin+Sans, "Download family", use the files in `static/`. Don't import the variable `[wght]` file |

Import each TTF as a Font Face, group them in one Composite Font per family, and use only these three families.

**Sizes at 1080p** (UMG units scale with the screen): intertitle text 52 px italic; "Just in time!" 84 px; room
labels and bar labels 20 px Josefin Bold, letter-spaced 0.15 em, uppercase; score 48 px Limelight; key prompt 24 px
Josefin Bold in a 2 px Paper outline box.

**Colours:** text is always Paper on Ink. Gold only for the bonus, the medal and near-miss numbers. Red only for
danger states.

**Intertitle card** (made in UMG, no texture needed): an Ink rectangle, two inset Paper borders (3 px at 12 px in, 1 px
at 20 px in), small Paper diamonds on the corners, centred italic text. K's "Meanwhile, at the Mill..." card spans 60%
of the picture width and sits at the bottom of the picture. Full-picture cards are only used between scenes, never
over play.

**HUD layout, K:** left margin: cameo of the next room (a circle, 20% of screen height across) with its name below and
its danger bar below that. Right margin: the same for the third room. Bottom of the left margin: "REEL 1" and the
score in feet of film ("1,250 feet"). Bottom of the right margin: the SPACE / CUT prompt. Above the picture, centred:
the current room's name, small.

**HUD layout, L:** the 4:3 picture fills the top 72% of the screen; the edit bench fills the bottom 25%, full width,
on Ink. Tracks look like film strips: dark strip, small sprocket holes along both edges, clips as lighter runs of
frames. The star's clear time is a white grease-pencil tick; a close shave is a red grease-pencil bracket with the gap
in seconds; the playhead is red. Left margin: "TAKE 3". Right margin: the two targets, "The Studio Cut: survive the
take" and "The Director's Cut: 2 shaves under 0.25 s", with the count in Limelight.

**Medals:** the Studio Cut is a Silver/Bone disc with the title in IM Fell; the Director's Cut is a gold disc with
"DC" in Limelight.

## 10. The Web build

From Epic's *Mobile Rendering Features Reference* (UE 5.6 docs; the 5.9 page wasn't published when this was
written), the mobile renderer supports: Bloom (Standard), Image Effects (vignette), Chromatic Aberration, Color
Grading, Film (tonemapper), Post Process Materials, manual exposure, Exponential Height Fog, Light Shafts, one
directional light with cascaded shadow maps, Unlit and Default Lit materials, and UI materials. It does **not**
support: Film Grain, Cinematic Depth of Field, Motion Blur, Local Exposure, Lumen GI and reflections, Nanite,
Virtual Shadow Maps, Volumetric Fog, Contact Shadows, or Source Angle on movable lights. With Mobile HDR off, almost
all post-process is gone.

**What Nitrate loses on Web:** bloom on the brightest whites is weaker, and the Mill's light shafts may need to become
translucent cards. Everything else in this file is either supported or lives in UMG. Check on the first Web package:
LUT toning applied (if not, use the Scene Color Tint fallback in section 7), stencil red still red, grain moving.

## 11. Feedback language

Every row happens on the frame of the event. Sound direction is in section 11b.

| Event | Game | What you see | What you hear |
|---|---|---|---|
| Cut pressed | K | Hard cut, white flash frame, frame-line bar, "Meanwhile..." card for ~1 s; the new room's tone | Projector clack + splice tick |
| Cut back within the late window | K | "JUST IN TIME!" in Limelight gold over the picture top, scales 1.35 → 1.0 in 0.18 s; "+250 feet of film" under it in IM Fell | Piano glissando up, short |
| Heavy enters strike window | K | Heavy's body goes stencil red | Low piano tremolo note |
| Hero lands a hit | K | Heavy knocked back; three short Paper impact lines; 0.15 s hit shake | Slapstick bonk |
| Room danger ≥ 75% | K | That room's cameo ring and bar flash red (about 3 Hz) | Its music layer turns agitato |
| A helpless one is lost ("killed off") | K | Its cameo irises out; card "Written out of the picture." | Single low piano chord |
| Play pressed | L | Clapper card flashes for 2 frames; "ROLLING" with a blinking red dot | Clapper snap, projector starts |
| Crusher wind-up (0.6 s before impact) | L | Red disc on the floor under it; its front face turns red | Rope creak, rising |
| Crusher impact | L | Dust puffs; 0.15 s shake | Thud + bell |
| Close shave (< 0.25 s) | L | Gold "0.08 s!" above the crusher, rising and fading over 1.1 s; "CLOSE SHAVE" under it; the target count updates | Cymbal tick + audience gasp sting |
| Star crushed | L | Freeze, iris-out, card "CUT! MEDIC!" | Record-scratch stop of the piano |
| Take survived | L | Iris-out on the star, result card with the medal | Short fanfare chord |

### 11b. Audio direction

A live accompanist in a 1920s picture house: solo piano carries everything, and in-world sound is cartoon foley.
No voices.
- **Music:** stride and ragtime piano for action; agitato (fast tremolo, "hurry music") for danger; a small theatre
  organ or chamber band only for the title and the ending. In K, add one agitato layer per room in danger.
- **Bed:** a quiet, constant projector whirr under everything. It stops dead on "CUT!".
- **Foley:** bonks, whooshes, wood and metal thuds, a slide whistle at most once per minute.
- **Search terms for the WeLoveIndies library:** `silent film piano`, `ragtime`, `stride piano`, `honky tonk`,
  `agitato`, `chase piano`, `vaudeville`, `1920s`, `theatre organ`, `film projector`, `projector loop`, `clapperboard`,
  `slide whistle`, `slapstick`, `cartoon hit`, `whoosh`. If nothing fits, try `jazz age`, `saloon piano`, `old movie`.

## 12. Generating assets on-style

The treatment (section 4) does most of the unifying, so prompts aim for clean shapes and neutral, unlit textures.
Always add to the end of a prompt: *"matte, neutral flat lighting, no baked shadows, no text, no logos, plain
background"*. Don't name films, actors or studios in prompts, and don't recreate any famous silent star's look (for
example, no bowler-cane-moustache tramp, no man hanging from a clock).

**Tripo (characters).** Template:
> `[character], 1920s silent film costume, full body, A-pose with arms 45 degrees from the body, stylised
> proportions with a slightly large head, simple large shapes, matte cloth, neutral expression, game-ready, low
> detail, matte, neutral flat lighting, no baked shadows, no text, no logos, plain background`

- Hero / star: `a lean young everyman in a light grey three-piece suit, flat cap, short scarf tied at the neck`
- Heavy: `a burly henchman in a dark double-breasted suit, bowler hat, very broad shoulders, big hands`
- Helpless A (K): `a young person in a light 1920s drop-waist dress, short bobbed hair`
- Helpless B (K): `an elderly inventor in a light waistcoat and shirtsleeves, round spectacles`

Settings: use auto-rigging, ask for a mid-poly result (around 15-20k faces if the option exists), and export FBX. The
scarf gets its own material slot (`M_StencilGold`) if Tripo splits it out; if it doesn't, add a simple scarf mesh.

**Rodin (props).** Template:
> `[prop], 1920s, chunky simplified silhouette like a stage prop, slightly exaggerated proportions, single object,
> centred, matte painted wood and iron, matte, neutral flat lighting, no baked shadows, no text, no logos, plain
> background`

L props: `a heavy black safe with a round dial`, `an upright piano`, `a blacksmith's anvil`, `a wooden barrel`, `a
director's folding chair`, `a hand-cranked film camera on a wooden tripod`. K props: `a brick chimney stack`, `a
rooftop water tower on four legs`, `a sawmill buzz saw on a wooden bench`, `a sawn log`, `a railway semaphore signal`,
`the front half of a small steam locomotive`, `stacked wooden crates`.

**PBRgen (materials).** Template:
> `[surface], 1920s, seamless tileable, medium scale, strong surface detail, flat even lighting, no baked shadows,
> no colour cast`

Surfaces: `tar paper roof with overlapping seams` (Rooftop floor), `worn wide wooden floorboards` (Mill floor),
`sooty brick wall` (walls everywhere), `railway ballast gravel with wooden sleepers` (Railway ground), `old
cobblestones` (L street), `painted theatre canvas with visible brush strokes` (backdrops, facades).
Only the albedo and normal maps are used (section 4).

**fal (concept art, backdrops, the itch cover).** Backdrop template:
> `painted theatre backdrop of [a 1920s city skyline at night | a sawmill interior back wall | flat farmland with a
> distant water tower | a studio backlot street of false-front buildings], flat matte painting, greyscale, soft,
> low contrast, no people, no text, wide`

The backdrops go on flat planes with `M_Backdrop`. Generated images must not appear in-game with the people or text a
model sometimes adds: check every one.

## 13. Asset budget and order

The order is built so the game looks like Nitrate after step 0 alone, and every later step only adds. Generation
starts after the fun gate (Sat 12:00) and stops at content lock (Sun 10:00).

| Step | What | Count | Tool | Rough time |
|---|---|---|---|---|
| 0 | The grade on grey boxes: project settings, `M_Nitrate` and its instances, LUTs, the post-process volume, `WBP_FilmGate`, the three fonts | — | Editor | 1.5 h |
| 1 | The hero (also L's star) and one heavy | 2 characters | Tripo | 30 min |
| 2 | The danger props. L: safe, piano, anvil. K: buzz saw, locomotive front | 3 (L) or 2 (K) | Rodin | 30 min |
| 3 | Floors and walls | 4 materials (K: roof, floorboards, brick, ballast. L: cobbles, brick, canvas, floorboards) | PBRgen | 30 min |
| 4 | Set dressing | 6 props (K: chimney, water tower, crates, log, signal, vent. L: barrel, chair, camera, crates, lamp post, signal) | Rodin | 50 min |
| 5 | K only: the two helpless ones | 2 characters | Tripo | 30 min |
| 6 | Backdrops | 3 (K) or 1 (L) | fal | 20 min |

**Totals.** K: 4 characters, 8 props, 4 materials, 3 backdrops (19 generated assets). L: 1 character (the hero mesh
reused), 9 props, 4 materials, 1 backdrop (15). Stop at these numbers. Anything more is polish, after the bug bash.

**Each asset, on import:** assign the role instance (section 4), check the scale against a 180 cm grey box, run the
silhouette check below, list it in `docs/CREDITS.md` with the tool, and save. Use Rodin for at least one shipped prop
(Hyper3D prize pool, see `docs/JAM_PLAN.md`).

## 14. Checks that replace taste

Run these on a screenshot after any lighting, material or post change:
1. **Value ladder:** sample pixels. Hero costume 200 or more (sRGB, average of R, G and B); heavies 45 or less; lit
   floor 95-130; floor in shadow 45-70.
2. **Stencil survives:** a heavy in its strike window samples within 15 of `#D3301F` on each channel.
3. **Silhouette:** with every character set to unlit black on a white background, each must be identifiable at 64
   px tall: the hero by the flat cap and scarf, the heavy by the bowler hat and shoulders.
4. **Thumbnail:** scale the frame down to 315×250. The hero, the red heavy and the room's tone must still be obvious.
5. **Neutral LUT:** with `LUT_Neutral` applied the frame is unchanged (once per engine build).
6. **Web:** on the first Web package, repeat checks 1, 2 and 4 in Chrome.

Only Marwan can judge whether grain, flicker and the overall mood feel right. Report those as "needs your eye", not as
done.

## Sources

- Epic, *Mobile Rendering Features Reference*, UE 5.6 documentation (feature support on the mobile renderer).
- Epic, *Post Process Effects*, *Color Grading and the Filmic Tonemapper*, *Using Look-up Tables for Color Grading*,
  *Auto Exposure*, *Bloom*, *Cinematic Cameras* (setting names as they appear in the Details panel).
- Fonts: Google Fonts, SIL OFL 1.1.
- Past Epic MegaJam winners were checked only by title (2021-2023 results pages). The pages found don't describe how
  the games look, so nothing here relies on them.
