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

## 15. Print It: the look (step 4)

Written Nov 14 for *Print It*, the walking heist in `docs/STATUS.md`. Where this section differs from §1-14, this
section wins; where it differs from STATUS, STATUS wins. Sizes are at 1920x1080. "Frame" in a timing means a film
frame at 24 fps (41.7 ms); at the band's 120 BPM a beat is 12 frames and a bar is 48.

### 15.1 The idea

*Print It* is a film and its making-of, cut against each other on one key press. Julian's camera is the print:
Ink-to-Paper monochrome in a 2.39:1 frame with grain, where only red (danger) and gold (Julian) keep their colour;
Maya's and Vex's cameras are the crew's video monitors: steel blue, clean, with viewfinder marks and tape labels.
Every word, stamp and card on screen is one family (Ink plates, Paper type, inset rules), so the HUD reads as the
film's own titles and slates, and colour anywhere on screen always means danger or Julian.

### 15.2 Tokens

**Colour.** Hex is what the screen shows. "Display 0-1" is the hex divided by 255: use it for maths in a
post-process material set to After Tonemapping (15.3). "Linear" is Unreal's linear RGB: use it for material base
colours, light colours and Canvas `FLinearColor` (Canvas converts linear to the screen).

| Token | Hex | Display 0-1 | Linear | Role |
|---|---|---|---|---|
| `ink` | `#0D0C0B` | 0.051, 0.047, 0.043 | 0.0040, 0.0037, 0.0033 | Letterbox bars, card fields, the film's black, the heavies' value |
| `plate` | `#1A1815` | 0.102, 0.094, 0.082 | 0.0103, 0.0091, 0.0075 | Text plates over the film picture, alpha 0.85 |
| `plate.mo` | `#0F1A24` | 0.059, 0.102, 0.141 | 0.0048, 0.0103, 0.0176 | Text plates over the making-of picture, alpha 0.85 |
| `silver` | `#7A756D` | 0.478, 0.459, 0.427 | 0.1946, 0.1779, 0.1529 | Film mid grey; inactive key tabs (20 px bold or larger only), hairlines, empty pips |
| `ash` | `#A39D92` | 0.639, 0.616, 0.573 | 0.3663, 0.3372, 0.2874 | Small secondary text: the controls row, field captions |
| `bone` | `#C9C2B4` | 0.788, 0.761, 0.706 | 0.5841, 0.5395, 0.4564 | Secondary text, card inner rules |
| `paper` | `#F1EADB` | 0.945, 0.918, 0.859 | 0.8796, 0.8228, 0.7084 | All primary text and borders, the film's white, Julian's value |
| `danger` | `#D3301F` | 0.827, 0.188, 0.122 | 0.6514, 0.0296, 0.0137 | Stencil red: watchers' frames, tape, tally lights, SEEN, the burn, the blooper slate |
| `julian` | `#E9B949` | 0.914, 0.725, 0.286 | 0.8148, 0.4851, 0.0666 | Stencil gold: his marks, the reel, the side exit, composure, gold stamps |
| `alarm` | `#FFFFFF` | 1, 1, 1 | 1, 1, 1 | The watchers' frames after the alarm (emissive, pulsing) |
| `mo.shadow` | `#0B1724` | 0.043, 0.090, 0.141 | 0.0033, 0.0086, 0.0176 | Making-of grade, dark stop (steel blue) |
| `mo.mid` | `#56738A` | 0.337, 0.451, 0.541 | 0.0931, 0.1714, 0.2542 | Making-of grade, mid stop |
| `mo.high` | `#DFF4F5` | 0.875, 0.957, 0.961 | 0.7379, 0.9047, 0.9131 | Making-of grade, light stop (cyan-white fluorescent); viewfinder marks |

The film's tone is §3's Silver tone exactly: `ink` / `silver` / `paper` as dark / mid / light. Red and gold are the
only saturated tokens; nothing else on screen may be saturated (§1 rule 2).

**Type.** Only the three §9 families. Tracking is given in em and in Slate's units (thousandths of an em, the
`LetterSpacing` field of `FSlateFontInfo`).

| Token | Font | Size | Tracking | Case | Used for |
|---|---|---|---|---|---|
| `type.display` | Limelight | 128 | 0.04 em / 40 | UPPER | PRINT IT, the title card's name |
| `type.stamp` | Limelight | 72 | 0.06 em / 60 | UPPER | CLEARED, the grade stamps, x2 / x3, DELETED SCENE RESTORED, the blooper slate's head |
| `type.title` | Limelight | 44 | 0.08 em / 80 | UPPER | Scene titles, card heads, stats figures, the finale's count |
| `type.line` | IM Fell English Italic | 34 | 0 | Sentence | Spoken lines (subtitles), hints, card taglines ("Keep that in!") |
| `type.body` | IM Fell English Roman | 30 | 0.01 em / 10 | Sentence | The title card's pitch, stats rows, blooper reel rows, credits |
| `type.goal` | Josefin Sans SemiBold | 26 | 0.02 em / 20 | Sentence | The goal line, the Use prompt |
| `type.label` | Josefin Sans Bold | 20 | 0.15 em / 150 | UPPER | Camera slug, key tabs, the clock, the composure label, card captions |
| `type.tag` | Josefin Sans Bold | 16 | 0.12 em / 120 | UPPER | World labels (watchers, devices, tape), the controls row, slate field names |
| `type.key` | Josefin Sans Bold | 16 | 0.08 em / 80 | UPPER | The letter inside a keycap |

**Keycap** (every key named on screen): a box 28 px tall, at least 28 px wide, 8 px padding each side, `line.thin`
Paper border, no fill in the bars, `plate` fill over the picture; `type.key` Paper centred. Written `[E]` in the copy
deck below; the brackets become the box.

**Space.** One unit `u` = 8 px. Steps: 8, 16, 24, 32, 48, 64. Screen margin left and right: 48 px. Everything in px
is multiplied by `s = ScreenHeight / 1080` (clamped 0.8 to 2.0) and rounded to whole pixels; lines never go below
1 px.

**The letterbox grid** (2.39:1 inside 16:9): picture 1920 x 804 at y 138 to 942; bars 138 px top and bottom. On
another aspect, picture height = width / 2.39, centred, and the HUD rows keep their offsets from the bars' edges.
Keep as built: the camera renders the full screen and the HUD paints the bars in `ink`.

| Row | Centre y | Holds |
|---|---|---|
| T1 | 40 | Slate strip (left), key tabs with device pips (centre), clock (right) |
| T2 | 96 | The goal line (centre) |
| picture | 138-942 | The world, world labels, subtitles (centre y 878), stamps and cards (centre 540) |
| B1 | 978 | The Use prompt and its refusals (centre) |
| B2 | 1018 | Composure (centre) |
| B3 | 1054 | The controls row (centre) |

Columns in the bars: left 48-608, centre 608-1312, right 1312-1872.

**Lines.** `line.hair` 1 px (card inner rules, scan lines), `line.thin` 2 px (keycaps, viewfinder marks, card inner
border, reticle), `line.rule` 3 px (card outer border), `line.heavy` 6 px (the composure line, stamp borders).

**Motion** (in film frames at 24 fps; HUD and gameplay still run at the full frame rate).

| Token | Frames | Used for |
|---|---|---|
| `motion.flash` | 1 | The cut's Paper flash |
| `motion.splice` | 3 | The frame-line roll at a cut to the film |
| `motion.snap` | 4 | A stamp landing; the viewfinder closing at a cut to the making-of |
| `motion.settle` | 6 | Grain burst after a cut; every exit fade |
| `motion.beat` | 12 | Beat pulses (pips, the REC dot, SEEN, the alarm's frames) |
| `motion.hold` | 36 | How long a stamp stays (1.5 s) |
| `motion.bar` | 48 | The alarm's bleed (2 s, the current `BleedSeconds`) |

**The entry rule:** anything that appears does so at full opacity on the frame of its event. Only exits fade
(`motion.settle`, linear). Stamps land by scale, never by fade.

### 15.3 The grade

One post-process material (a material that runs on the whole rendered image) in the unbound Post Process Volume,
Blendable Location **After Tonemapping**: the input is display-encoded 0-1, so every number here is in display
0-1 and the hex stops are used as they are. Check once: with `GradeStrength = 0` the frame matches the ungraded
frame. If the input turns out to be linear in this build, wrap the maths in `pow(x, 1/2.2)` and back.

Parameters live as one Material Parameter Collection (a set of named values C++ can set each frame),
`MPC_Grade`: `GradeMode` (0 film, 1 making-of), `GradeStrength` (0-1), `AlarmPulse` (0-1), `bHueKeyFallback`.
The HUD or the director sets `GradeMode` on the frame of the cut press: a flight is seen in the look it lands in.

**Shared.** `Y = dot(rgb, (0.2126, 0.7152, 0.0722))`.

**The film (camera 1).**
1. Levels: `Y1 = saturate((Y - 0.03) / (0.90 - 0.03))` (black point 0.03, white point 0.90).
2. Contrast, a soft S: `Y2 = lerp(Y1, Y1*Y1*(3 - 2*Y1), 0.6)` (`FilmContrast` 0.6).
3. Tone, three stops: `Y2` 0 = `ink`, 0.5 = `silver`, 1 = `paper` (linear interpolation between neighbours).
4. Vignette, picture only: with `p = ((u - 0.5) / 0.5, (v - 0.5) / 0.3722)` and `r = length(p) / 1.414`,
   multiply by `1 - 0.30 * smoothstep(0.5, 1.0, r)` (corners at 70%).
5. Output `lerp(original, graded, GradeStrength)`. Tint: none beyond the Silver stops (they are the warmth).

**The making-of (cameras 2 and 3).**
1. Levels: `Y1 = saturate((Y - 0.02) / 0.93)`. No contrast curve (flat video).
2. Tone: `Y1` 0 = `mo.shadow`, 0.5 = `mo.mid`, 1 = `mo.high`.
3. No vignette.
4. Output `lerp(original, toned, 0.9 * GradeStrength)`: 10% of the source colour stays so tints still separate
   the people.

**What keeps its colour** (both modes), marked with the custom-depth stencil (a per-object number the
post-process can read; enable Custom Depth-Stencil Pass "Enabled with Stencil" in Project Settings):

| Stencil | Means | Output |
|---|---|---|
| 1 | Danger: frames, tape, tally lights, the scanner's bar | Original colour, saturation x1.15, value floor: if `Y < 0.20`, scale up to `Y = 0.20` |
| 2 | Julian's gold: marks, the reel's case, the gold side exit, the trap's mark, the scanner's open bar | Original colour, saturation x1.15, value floor `Y >= 0.45` |

The floors keep red tape readable in shadow and keep gold always lighter than red (15.8). Vignette still applies.
**Fallback** (`bHueKeyFallback`, for anything that cannot write custom depth, such as line batches or translucent
meshes): a pixel with saturation over 0.45 and hue within 20 degrees of `danger` or `julian` keeps its colour.
With the dressing in 15.9 nothing else on the set is that saturated.

**The alarm's bleed.** On the frame the finale goes live (after the hold), `GradeStrength` runs 1 to 0 over
`motion.bar` (48 frames, 2 s) on a smoothstep curve, on every camera. After it, saturation 1.15 on the whole
picture (`AlarmSaturation` 1.15, down from 1.25: full colour after monochrome is already the jolt). On the same
frame the watchers' frames switch from `danger` to `alarm` white, emissive 4, and pulse: full on each beat frame,
falling linearly to 35% over 10 frames, holding until the next beat (`AlarmPulse` drives it). Tally lights and the
tape stay red. A blooper or restart puts `GradeStrength` back to 1 on the restart frame, no fade.

**The "all film" switch:** `bAllFilm` on the director (default off). On: `GradeMode` stays 0 on every camera, and
cameras 2 and 3 get the film overlay (15.4) instead of the viewfinder. The HUD, the world labels and the stencils do
not change. This is the switch Marwan judges the split with.

**Engine settings** (unbound volume, unchanged from §7 except): Exposure manual as built; Bloom 0.4, threshold 1.0;
Vignette Intensity 0 (the material does it); Film Grain 0 (the overlay does it); Color Saturation 1 (the
material does it); Motion Blur 0.

### 15.4 The overlay

Drawn by `TakeHUD` on Canvas, under the HUD text, inside the picture rect only. The bars stay clean `ink`. All film
effects tick at 24 fps (`floor(Time * 24)`).

**Film (camera 1, and every camera with `bAllFilm`).**

| Effect | Play | After a cut (6 frames) | Cards and end pages |
|---|---|---|---|
| Grain: 256x256 grey noise texture tiled 1:1 in pixels, offset re-rolled each frame, translucent, centred on 0.5 | opacity 0.07 | 0.16 | 0.12 |
| Flicker: an `ink` rect over the picture, opacity re-rolled each frame | 0 to 0.03 | 0 to 0.05 | 0 to 0.06 |
| Dust: rects 2-5 px, `ink` at 0.7 or `paper` at 0.5, each lives 1 frame | at most 1.5 a second | same | 3 a second |
| Scratch: a 1 px vertical `paper` line at 0.2, full picture height, x drifts 4 px | every 6-10 s, 2 frames | same | every 2 s |

No reticle on the film: Julian has nothing to aim.

**Making-of (cameras 2 and 3):** a viewfinder instead of film. Why: it is the crew's video camera, so a clean
picture keeps the device labels and the red frames readable, and a different overlay is the cut's second signal
after the grade.
- Corner marks: four L shapes, arms 48 px, `line.thin`, `mo.high` at 0.6, inset 32 px from the picture's corners.
- Reticle (replaces today's "+"): a cross of four 12 px arms with a 6 px gap at the centre, `line.thin`, `paper`
  at 0.8. It turns `julian` gold while a device can be used (the Use prompt is up): gold here means "this helps
  Julian".
- Scan lines: a 1 px `ink` line every 4 px at 0.05 (a tiled 1x4 texture). No grain, flicker or dust.

**The splice at a cut** (frame 0 is the frame of the press).

| Frame | To the film | To the making-of |
|---|---|---|
| 0 | Grade and overlay swap; a `paper` flash over the picture at 0.35; the HUD's active key tab changes | Same, flash at 0.35 |
| 0-2 | An `ink` frame-line band, 32 px tall (4% of the picture), rolls from the picture's bottom edge to its top | Corner marks start 12% further from the centre and close to their place by frame 3 |
| 0-5 | Grain at 0.16 | Scan lines at 0.10 |

A refused press (during a flight) gets no splice: only the refusal in the bar (15.5). The splice never covers more
than 35% opacity and never lasts past frame 5, so it cannot hide play.

**The burn as composure falls** (every camera: composure is Julian's, and the take runs while you are on the
crew). Below 50 composure, a `danger` band along all four picture edges, drawn with a 1x64 ramp texture (solid at
the edge, clear inside): width `24 + 96 * (1 - c/50)` px, alpha `0.6 * (1 - c/50)` (c = composure). While SEEN
(draining) it adds +0.15 alpha on each beat, falling over 10 frames. Below 25 on the film camera, dust rises to 4 a
second and flicker to 0 to 0.06.

**At a blooper** (during today's freeze; frame 0 is the blooper):
- 0: the picture holds; flicker 0.10.
- 0-8: a film burn grows from Julian's screen position to the picture's diagonal: a radial ramp texture, `paper`
  core, `danger` ring, clear outside.
- 9-11: `paper` over the whole picture.
- 12 on: the blooper slate (15.6) on `ink`, until the restart. The restart frame is clean: no burn, grade at 1.

**At the end of a take** (PRINT IT): frame 0 is `ink` over the whole screen, bars included, with the clapper. From
frame 1 the end pages run with the film overlay at the "cards and end pages" levels on every camera.

### 15.5 The HUD

**One rule for where things go.** Each lane answers one question, and the lanes never move between cameras:
the top bar is *where am I and what next*, the bottom bar is *what can my hands do and how is Julian*, the picture
holds the world, its labels, the subtitles and the film's own stamps. A cut changes the picture, never where you
look for information.

**Glyphs** (built from rects; used in the HUD, the labels and the title card):

| Glyph | Shape | Colour | Motion |
|---|---|---|---|
| `glyph.danger` | Filled square 14x14 (the tally) | `danger` | Pulses on each beat: alpha 1.0 on the beat frame, 0.55 by frame 10 |
| `glyph.julian` | T: a 16x4 bar over a 4x12 stem (an actor's tape mark) | `julian` | Still, never pulses |
| `glyph.off` | Square outline 14x14, `line.thin` | `silver` | Still |
| `glyph.pip` | Rect 12x4 (a device use) | `paper` unused, `silver` outline spent | Still |

**Layout at 1920x1080** (rows from 15.2).

| Element | Lane | Anchor and offset | Type | Colour | States |
|---|---|---|---|---|---|
| Slate strip | T1 left | Left x 48, centre y 40; 24 px between fields | Captions `type.tag`, values `type.label` | Captions `ash`, values `paper` | "SCENE 2  THE FLOOR     TAKE 1". Changes on the scene's first frame. Replaces "A - JULIAN" and the separate take and scene texts. |
| Key tabs | T1 centre | Centred on x 960, 32 px between tabs | Keycap + name `type.label` | Active: keycap filled `paper`, letter `ink`, name `paper`. Inactive: keycap outline `silver`, name `silver` | The target goes active on the press frame. During the flight a 2 px `paper` line under its name grows to full width over the flight's length (this replaces "CUT -> MAYA"). Refused press: the pressed tab shifts 4 px left and right over 6 frames, and B1 says why. Under MAYA and VEX: one `glyph.pip` per limited-use device, 4 px apart, 8 px under the name. |
| Bar clock | T1 right | Right edge x 1872, centre y 40 | "BAR" `type.tag`, number `type.label`, then 4 beat squares 10x10, 6 px apart | Caption `ash`, number `paper`; squares `paper` filled up to this beat, `silver` outline after | The next square fills on each beat frame. On a band break (every 8th bar) the squares give way to "BREAK" in `bone`. Replaces the REC dot, "REC", the timecode and the gold "BAND BREAK". |
| Finale count | T1 right, from the alarm going live | Right edge x 1872 | "ALARM" `type.label`, numeral `type.title`, 8 squares 12x12, 6 px apart | "ALARM" `danger`; numeral `paper`; squares `danger` filled for bars left, `silver` outline spent | The current bar's square blinks on each beat. In the last bar the numeral shows seconds to one decimal ("1.4") in `danger`. |
| Goal line | T2 | Centred x 960, y 96; max 1400 px wide, at most 2 lines (centres y 80 and 112) | `type.goal`, keycaps inline | `paper`; names of labelled things in capitals | A new goal appears on its frame with a 2 px `bone` rule under it that draws out from the centre over 12 frames. |
| Flash | T2, in place of the goal line | Same | `type.goal` after a glyph | `paper` text; `glyph.danger` for arrivals, `glyph.julian` for his wins (keycard, reel, trap open, checkpoints), none for neutral | Holds 48 frames, then the goal line is back. A newer flash replaces an older one. No fades. |
| Use prompt | B1 | Centred x 960, y 978; max 1200 px | `type.goal` + keycap | Ready: `paper`, keycap `paper`. In reach, not ready: text `bone`, keycap `silver`, with the reason | On Julian's camera B1 shows his mark action instead ("Lifting the keycard") with a 240x4 `julian` progress bar 12 px under it. |
| Press refusals | B1, in place of the prompt | Same | `type.goal` | `bone` | Every press that does nothing says why here, for 36 frames: cut during a flight, E with nothing in reach, E on Julian. |
| Composure | B2 | Line x 560-1360 (800 x 6), centre y 1018; "COMPOSURE" right-aligned to x 536; state from x 1384 | Label and state `type.label` | Fill `julian` from the left; spent part a 1 px `silver` hairline; label `bone` | SEEN: `glyph.danger` + "SEEN" in `danger`, and the fill alternates `danger` / `julian` every 6 frames. MARK: `glyph.julian` + "MARK" in `julian`. Otherwise no state. Refill (x2): the gained part shows in `paper` for 12 frames. |
| Controls row | B3 | Centred x 960, y 1054 | `type.tag`, keycaps 24 px tall | `ash` | On during play; hidden under cards; the end pages put their own keys here. |
| Subtitles | Picture | Centred x 960, y 878 (64 px above the picture's edge); max 1200 px, 2 lines | Speaker `type.tag`, line `type.line` | Speaker `bone`, line `paper`, on `plate` (film) or `plate.mo` (making-of) at 0.85, padding 16 x 8 | Spoken lines, the crew's intro lines and hints. Queue; each holds 2 s + 0.06 s a character. |
| Reticle | Picture centre | Making-of only (15.4) | | `paper` 0.8; `julian` while Use is up | |
| World labels | Picture | 24 px above the actor's head | See the label rule | | |

**Film camera against the making-of:** the bars and everything in them are identical. Only the picture changes:
grade, overlay, reticle (making-of only) and world labels (below).

**The label rule** (fixes the clutter noted at step 2, several watcher and device labels piled up on Maya's camera):
1. Only watchers and devices get labels, and only within 40 m of the camera and on screen.
2. Priority: (1) the watcher the reticle has picked, or the device in reach; (2) watchers that see Julian now;
   (3) other live watchers; (4) the current crew member's devices; (5) watchers that are off, blinded, lured or
   masked; (6) the other crew member's devices. Ties go to the nearer.
3. At most 4 labels, plus the priority-1 label.
4. Stacking: a label that overlaps a higher-priority one (4 px gap) moves up by its own height + 4 px, at most
   twice; if it still overlaps, it hides. Labels stay inside the picture 16 px clear of the bars, and only the
   priority-1 label may enter the 320 x 160 box round the reticle.
5. Same name, same state, within 3 m: one label, "2 GUARDS".
6. Labels appear and hide on the frame, no fades.

**Tape labels (making-of):** the label is a strip of camera tape: a `bone` plate 24 px tall, 8 px padding, no
outline (priority 1 gets a 2 px `paper` outline), text `type.tag` in `ink`. Left of the name: `glyph.danger` with a
2 px `ink` border when live, `glyph.off` when not. The state word shows only when it is not the default:
"BLINDED 3.2", "LURED 4.0", "MASKED", "OFF", "DROPPED", "SPEAKING", "RETURNING". A live, watching watcher is its
name and the pulsing square, nothing more. Devices show their name and their pips; unlimited devices (the
searchlight, the follow spot) show no pips.

**Julian's camera (the film): no tape labels.** STATUS rule 1 ("labels only on set") stands: the red frames,
SEEN and the burn carry the danger. The one exception, because the look must not hide a rule: a watcher that is
blinded, lured or masked shows a countdown tag (`glyph.off` + seconds, `type.tag` `paper` on `plate`), no name, at
most 3, so Julian's walk can be timed from his own camera.

### 15.6 Stamps and cards

One family. **Stamps** land over play and must not hide it; **cards** appear when play has stopped. The **slate**
(a clapperboard) is the family's one bold piece, used three times: opening, blooper, PRINT IT.

| Part | Stamp (over play) | Card (play stopped) |
|---|---|---|
| Field | `plate` at 0.75 inside the border (at most 6% of the picture) | `ink` 1.0 |
| Outer border | `line.heavy` in the stamp's colour | `line.rule` `paper`, 24 px inside the picture's edges |
| Inner rule | `line.thin`, 10 px inside, same colour | `line.hair` `bone`, 8 px inside |
| Corners | None | 10 px diamonds (a square tile turned 45 degrees), `paper`, on the inner rule's corners |
| Padding | 24 x 12 | 64 |
| Tilt | -4 degrees (grade stamps -6, x2 / x3 -8) | None |
| Landing | Frame 0 at scale 1.30 and full opacity, frame 2 at 0.96, frame 4 at 1.00 | Whole on its frame |
| Exit | 6-frame fade after `motion.hold` (36), or 72 for titles | Cut, on Enter or the restart |
| Colour means | `julian` = his win, `paper` = neutral, `danger` = failure | Type `paper`; the slate's sticks take the meaning colour |

**The slate.** Two sticks, 28 px tall and as wide as the board, filled with a 45-degree stripe tile (a 56 x 56
procedural texture: one stripe in the meaning colour, one `ink`). The top stick is hinged at its left end and
starts at -14 degrees; it closes to 0 over frames 0-6 and lands on frame 6 with the clapper sound. Under them, the
board: `ink` field, 3 px `paper` border, rows split by 1 px `bone` hairlines, a caption (`type.tag` `ash`) in the
top-left of each cell 8 px in, the value under it.

| Member | When | Where and size | Copy | Tokens |
|---|---|---|---|---|
| Opening slate | Every take start (after the title card, after R) | Picture lower left: x 96, bottom at y 910; 560 x 236. A card-type board, but small and away from Julian, so play runs | Row 1: caption "AURELIA AUCTION HOUSE", value "LOT 41: THE DIRECTOR'S CUT". Row 2 cells: "SCENE" 1, "TAKE" 3, "DIRECTOR" YOU. On the clap: subtitle DIRECTOR "Action!" | Sticks `paper`; values `type.title` `paper` (row 1 value `type.label`). Holds 96 frames, 6-frame exit |
| Scene title | A scene's first frame (with its checkpoint) | Picture centre x 960, y 300 | Caption "SCENE 2" (`type.tag` `bone`), title "THE FLOOR" (`type.stamp` `paper`), 1 px `bone` rules 320 px wide above and below | No field, no border, no tilt; stamp landing; holds 72 |
| Blooper slate | Frame 12 of the blooper burn (15.4) | Picture centre, 880 x 400, on `ink` | Head "BLOOPER". Cells: "WHAT" (the reason, e.g. "Walked into the beam"), "AGAIN FROM" THE ROPE, "TAKE" 1. Subtitle DIRECTOR "Cut!" | Sticks `danger`; head `type.stamp` `danger`; cells `type.label` `paper` |
| CLEARED | The lift lands | Picture x 960, y 300 | "CLEARED"; subtitle DIRECTOR "That's the one." | Stamp, `julian` |
| x2 / x3 | One device catches two or more | Picture x 1480, y 760 | "x2" | Stamp, `julian`, `type.stamp`, -8 degrees |
| DELETED SCENE RESTORED | The Bow | Picture x 960, y 300 | Caption "DELETED SCENE RESTORED" (`type.label` `julian`) over the stamp "THE BOW"; subtitle DIRECTOR "Keep that in!" | Stamp, `julian`; holds 72 |
| PRINT IT | The finish | Frame 0 all `ink`; the slate centred, 1040 x 480 | Head "PRINT IT"; under the board, one line: how he left (`PrintLineTrap` / `PrintLineSide`); subtitle DIRECTOR "Print it!" | Sticks `julian`; head `type.display` `julian`; line `type.line` `paper` |
| Grade stamps | The stats page, 24 frames after it opens | Right column, centre x 1420, y 560; caption "THE GRADE" (`type.tag` `ash`) above | "ROUGH CUT" / "FINAL CUT" / "DIRECTOR'S CUT" | ROUGH CUT: `paper`, outer border only. FINAL CUT: `paper`, outer + inner. DIRECTOR'S CUT: `julian`, outer + inner + corner diamonds. All `type.stamp`, -6 degrees |
| Title card | Once a session, before the first take | Card in the picture; bars stay | Name, tagline, pitch (2 lines), cast row (3 columns: keycap, name, job), the rule line drawn with the glyphs, the controls, the prompt (copy in 15.7) | Name `type.display` `paper`; tagline `type.line` `bone`; pitch `type.body` `paper`; cast names `type.label` `paper`, jobs `type.line` `bone`; prompt `type.goal` with its keycap's outline pulsing on the beat |
| Stats page | After PRINT IT | Card | Head "YOUR TAKE"; rows as caption (right-aligned at x 880) and value (from x 912) | Head `type.title` `paper`; captions `type.tag` `ash`; figures `type.title` `paper`, words `type.body` `paper`; a restored scene gets `glyph.julian` |
| Blooper reel and credits | After the stats | Card; the roll scrolls inside the inner rule and is clipped by it | Head "THE BLOOPER REEL"; each blooper a caption "TAKE 1  BAR 62" and a line "Walked into the beam. Caught by the BEAM. Again from the rope."; then the credits | Head `type.title`; captions `type.tag` `ash`; lines `type.body` `paper`; `ReelScrollSpeed` as built |

End pages put their keys in B3: "[ENTER] Next   [R] New take   [ESC] Quit" (keycaps `paper`, words `ash`).

### 15.7 The copy deck

**Rules for every string.** One name per thing, the name STATUS uses, the same in the goal line, the label and the
prompt (FOLLOW SPOT, SOUND DESK, CHAMPAGNE CART, PYRO BOARD, CURTAIN WINCH, the stage trap, the gold side exit).
Anything that has a label on screen is written in capitals in a sentence, so it can be found. `[E]` means a
keycap. No spaced dashes or "NAME - state" joins: structure (caption and value, glyph and word) does that job.
Spoken lines are drawn as speaker and line, so "NAME:" prefixes and quotation marks are split off by the HUD, not
retyped. Nobody runs, so no line says "run". "Cut" is the verb for 1/2/3 everywhere. R is always "New take".

Path abbreviations: `HUD.h/.cpp` = `Reel/Source/Reel/Take/TakeHUD.*`, `Dir*` = `TakeDirector*.cpp` in the same
folder, `TT` = `TakeTypes.h`, `BA` = `tools/build_auction.py`.

| File | Now | New |
|---|---|---|
| HUD.h | `WASD walk   Mouse look   E use   1 2 3 switch   R new take` | `[WASD] WALK   [MOUSE] LOOK   [E] USE   [1] [2] [3] CUT   [R] NEW TAKE` |
| HUD.h | `PRINT IT` (title) | keep |
| HUD.h | `a heist shot in one take` | `A heist shot in one take.` |
| HUD.h, Dir | `[ENTER]  ACTION`, `[ENTER] or [E]: action` | `[ENTER] Action!` (E still works; one prompt, not two) |
| HUD.cpp | Title line: `Tonight the Aurelia Auction House sells...` | keep |
| HUD.cpp | Title line: `You direct the crew that steals it, live, in one take.` | keep |
| HUD.cpp | `[1] JULIAN, the actor   [2] MAYA, lights   [3] VEX, props` | Three columns: `[1] JULIAN` / `the actor, on the floor`; `[2] MAYA` / `lights, up above`; `[3] VEX` / `props and sound, backstage` |
| HUD.cpp | `WASD walk   Mouse look   E use what a crew member stands at   R new take` | `[WASD] Walk   [MOUSE] Look   [E] Use what the crew stands at   [R] New take` |
| HUD.cpp | `Get Julian in, lift the keycard, take the reel, get him out. Being seen burns his composure.` | Two lines: `Get JULIAN in, lift the keycard, take the reel and get him out.` then, with the glyphs: `[danger] Red sees him and burns his composure.   [julian] Gold is his: his marks, the reel, the way out.` |
| HUD.cpp | Credits: `PRINT IT  -  a heist shot in one take` | Two lines: `PRINT IT`, `A heist shot in one take.` |
| HUD.cpp | Credits: `Directed by you.`, `Made in Unreal Engine 5.9...`, `By Marwan, solo, with Claude agents.`, `Thank you for playing.` | keep (names stay as they are) |
| HUD.cpp | `[ENTER] next      [R] another take      [ESC] quit` | `[ENTER] Next   [R] New take   [ESC] Quit` |
| HUD.cpp | `[R] another take      [ESC] quit` | `[R] New take   [ESC] Quit` |
| HUD.cpp | `PRINT IT` (end slate) | keep |
| HUD.cpp | `DIRECTOR: "Print it!"` | Subtitle: DIRECTOR / `Print it!` |
| HUD.cpp | `YOUR TAKE` | keep |
| HUD.cpp | `TAKE  %d` | Caption `TAKE`, value `%d` |
| HUD.cpp | `TIME  %d:%02d   (bloopers spliced out)` | Caption `RUNNING TIME`, value `%d:%02d`, then `bloopers spliced out` in `type.line` `bone` |
| HUD.cpp | `CUTS  %d`, `BLOOPERS  %d` | Captions `CUTS`, `BLOOPERS`; values `%d` |
| HUD.cpp | `OUT BY  THE GOLD SIDE EXIT` / `OUT BY  THE STAGE TRAP` | Caption `OUT BY`, value `The gold side exit` / `The stage trap` |
| HUD.cpp | `DELETED SCENES  %d / %d` | Caption `DELETED SCENES`, value `%d of %d` |
| HUD.cpp | `RESTORED   %s` | `[julian] Restored: The Bow` |
| HUD.cpp | `MISSED   %s: %s` | `Missed: The Bow. At the gold side exit, bow to the blinded drone.` |
| HUD.cpp | `THE GRADE` | keep (caption) |
| HUD.cpp | `PRINT IT   -   %s` (reel header) | Two lines: `PRINT IT`, then the grade (`FINAL CUT`) as a caption |
| HUD.cpp | `THE BLOOPER REEL` | keep |
| HUD.cpp | `No bloopers. One clean take.` | keep |
| HUD.cpp | `TAKE %d    bar %d    AGAIN FROM %s    %s   -   %s` | Caption `TAKE %d  BAR %d`; line `%reason. Caught by the %WATCHER. Again from %checkpoint.` (checkpoint in sentence case here: "the rope") |
| HUD.cpp | `REC` | removed |
| HUD.cpp | `%s - %s` (`A - JULIAN`) | removed; slate strip `SCENE %d  %SCENENAME     TAKE %d` |
| HUD.cpp | `BAND BREAK` (gold) | `BREAK` (`bone`) |
| HUD.cpp | `CUT -> %s` | removed (the key tab shows the flight) |
| HUD.cpp | `%d %s` (key tabs) | `[%d] %s` as keycap and name |
| HUD.cpp | `%s - OFF`, `%s - %s` (watcher labels) | Tape label: name, then the state word only when not the default (15.5) |
| HUD.cpp | `ALARM   %d BARS` and `%.1f s` | `ALARM` + numeral + squares; last bar `%.1f` |
| HUD.cpp, Dir | `THE START` (checkpoint name) | `THE STREET` |
| HUD.cpp | `BLOOPER - AGAIN FROM %s`, reason, `Cut!` | Blooper slate: head `BLOOPER`; cells `WHAT`, `AGAIN FROM`, `TAKE`; subtitle DIRECTOR / `Cut!` |
| HUD.cpp | `CLEAR` (stamp) | `CLEARED` |
| Dir | `ACTION!` (flash) | Subtitle DIRECTOR / `Action!` on the slate's clap |
| Dir | `%s   TAKE %d` (Scene 1 title and take) | The opening slate (15.6) |
| Dir | `AURELIA AUCTION HOUSE  -  LOT 41: THE DIRECTOR'S CUT` | Caption `AURELIA AUCTION HOUSE`, value `LOT 41: THE DIRECTOR'S CUT` |
| Dir, BA | `SCENE 1 - THE DOOR`, `SCENE 2 - THE FLOOR`, `SCENE 3 - THE GETAWAY` | Caption `SCENE 1`, title `THE DOOR` (and so on) |
| Dir | `Cut! Again from %s` | removed (the blooper slate says it) |
| DirBlooper | `TAKE %d` (flash on a new take) | removed (the opening slate shows the take) |
| Dir | `Shutters down. The whole floor is red tape. Get him out.` | keep |
| Dir | `PrintLineTrap`, `PrintLineSide` | keep |
| DirComposure | `SEEN`, `MARK` | keep |
| DirDevices | `%s - can't fire now` | `%s can't fire now` |
| DirDevices | `[E] ` + verb (+ watcher) | keep (`[E] Put the follow spot on the GUARD`) |
| DirDevices | `Look at a watcher` | `Look at a watcher to aim the follow spot` |
| DirDevices | `x%d` | keep, as a stamp |
| DirFinale | `DELETED SCENE RESTORED: %s` | Caption `DELETED SCENE RESTORED`, stamp `%s` |
| DirFinale | `[R] another take` | `[R] New take` |
| DirFinale | `That is a print.  [R] another take` | `That's a print.  [R] New take` |
| DirPresses | `WASD walk   E use   1 2 3 cut   R new take` | Same as the controls row |
| DirPresses | `Mid-cut` (both) | `Wait for the cut to land` |
| DirPresses | `Already on %s` | keep |
| DirPresses | `[2] MAYA or [3] VEX work the set` | `JULIAN has nothing to use. [2] MAYA and [3] VEX work the set.` |
| DirPresses | `Nothing in reach - stand in a device's ring` | `Nothing in reach. Stand in a device's ring.` |
| DirScene | `CLEAR - that's the one` | Stamp `CLEARED` + subtitle DIRECTOR / `That's the one.` |
| DirScene | `%s - arrives on bar %d` | `[danger] %s arrives on bar %d` |
| DirScene | `%s arrives on the next bar` | `[danger]` + keep |
| DirScene | `KEYCARD LIFTED`, `HE HAS THE REEL`, `THE STAGE TRAP IS OPEN` | `[julian]` + keep |
| DirScene | `CHECKPOINT - %s` | `[julian] CHECKPOINT: %s` |
| DirScene | `CLEARED.   [R] new take` | `Cleared.  [R] New take` |
| DirScene | `The case trips the alarm: %d bars to get out. Post the crew first: [3] VEX at the curtain winch, [2] MAYA at the pyro board. Then stand JULIAN o...` | `The case trips the alarm: %d bars to get out. Post [3] VEX at the CURTAIN WINCH and [2] MAYA at the PYRO BOARD first.` |
| DirScene | `Walk VEX east ... (His sound desk buys the gold side exit instead.)` | `Walk VEX east along the corridor to the CURTAIN WINCH. Or save his SOUND DESK for the gold side exit.` |
| DirScene | `[E]  Let the counterweight go: the curtain hides him from the guards and opens the stage trap` | `[E] Drop the curtain: it hides JULIAN from the guards and opens the stage trap` |
| DirScene | `Run VEX to the curtain winch at the east end of the corridor` | `Walk VEX to the CURTAIN WINCH at the east end of the corridor` |
| DirScene | `ALARM.   [3] VEX: the winch drops the curtain... [2] MAYA: the pyro blinds the stage.` | `Alarm. [3] VEX: drop the curtain to open the stage trap. [2] MAYA: the pyro blinds the stage.` |
| DirScene | `RUN. The open trap: the gold mark upstage, behind the lots table.` | `Go. The open trap is the gold mark upstage, behind the lots table.` |
| DirScene | `[1]  JULIAN: get him to the open trap` | `[1] JULIAN: get him to the open trap` |
| DirScene | `No invitation: keep off the red carpet. Walk JULIAN ... gold mark   (WASD + mouse)` | Same without `(WASD + mouse)` (the controls row has it) |
| DirScene | `[2]  Cut to MAYA on the canopy. She has the searchlight.` | `[2] Cut to MAYA on the terrace. She has the SEARCHLIGHT.` |
| DirScene | `Stand in the searchlight's gold ring` | `Stand MAYA in the SEARCHLIGHT's ring` (device rings are no longer gold, 15.9) |
| DirScene | `The rope. Blind or mask the BEAM and the GUARD:   [2] MAYA spot, banner   [3] VEX cart` | `The rope. Blind or mask the BEAM and the GUARD: [2] MAYA has the FOLLOW SPOT and the BANNER, [3] VEX the CHAMPAGNE CART.` |
| DirScene | Every other goal line (lines 386-577) | keep; collapse runs of spaces to two; keycaps single-spaced |
| BA | `device_name`: `CART`, `DAIS CART`, `DESK`, `PYRO`, `WINCH` | `CHAMPAGNE CART`, `DAIS CART`, `SOUND DESK`, `PYRO BOARD`, `CURTAIN WINCH` (`SEARCHLIGHT`, `FOLLOW SPOT`, `BANNER` keep) |
| BA | `action_verb`: `LIFTING THE KEYCARD`, `OPENING THE CASE`, `DROPPING THROUGH THE TRAP`, `TAKING A BOW`, `OUT THE GOLD DOOR` | Sentence case for B1: `Lifting the keycard`, `Opening the case`, `Dropping through the stage trap`, `Taking a bow`, `Out by the gold side exit` |
| BA | Spoken lines (`seen_line`, `arrive_line`, `intro_line`) | keep the words; the HUD splits the speaker and drops the quotes |
| BA | Signs `QUEUE`, `INVITED GUESTS`, `PRESS`, `CREW ONLY`, `CURTAIN WINCH`, `THE AURELIA QUINTET`, `LOT 41: THE DIRECTOR'S CUT`, `SECURITY` | keep words; colour `bone` (15.9) |
| BA | Signs `STAGE TRAP`, `EXIT` (gold side door), `LIVE BROADCAST` | keep words; `STAGE TRAP` and `EXIT` gold (stencil 2), `LIVE BROADCAST` red (stencil 1) |
| TT | Blooper reasons `COMPOSURE ZERO`, `BEAM TOUCHED`, `THE CLOCK RAN OUT` | `Composure ran out`, `Walked into the beam`, `The clock ran out` |
| TT | Watcher states `WATCHING`, `LIVE` | Not shown (the default) |
| TT | `OFF`, `DROPPED`, `BLINDED`, `LURED`, `MASKED`, `RETURNING`, `SPEAKING` | keep |
| TT | Grades `ROUGH CUT`, `FINAL CUT`, `DIRECTOR'S CUT` | keep |

### 15.8 Accessibility

**Contrast** (WCAG 2.1 relative luminance, computed from the hex values). Large text is 24 px and up, or 18.7 px
and up in bold; Josefin Sans Bold (700) at 20 px counts as large. Over the picture, the worst case is taken: a
plate over a `paper`-white pixel.

| Text role | Foreground | Background | Ratio | Needs | Pass |
|---|---|---|---|---|---|
| Goal line, prompts, values, body (bars and cards) | `paper` | `ink` | 16.3 | 4.5 | Yes |
| Secondary text, speakers, refusals | `bone` | `ink` | 11.0 | 4.5 | Yes |
| Controls row, captions (16 px) | `ash` | `ink` | 7.3 | 4.5 | Yes |
| Inactive key tabs (20 px bold) | `silver` | `ink` | 4.3 | 3.0 (large) | Yes; fails 4.5, so `silver` never carries text under 20 px bold |
| Gold text: MARK, captions, stamps on cards | `julian` | `ink` | 10.7 | 4.5 | Yes |
| Red text: SEEN, ALARM (20 px bold), BLOOPER (72 px) | `danger` | `ink` | 3.9 | 3.0 (large) | Yes; fails 4.5, so red text is never under 20 px bold |
| Tape labels | `ink` | `bone` | 11.0 | 4.5 | Yes |
| Subtitles and film countdown tags, worst case | `paper` | `plate` 0.85 over `paper` | 9.8 | 4.5 | Yes |
| Speaker over the picture, worst case | `bone` | same | 6.6 | 4.5 | Yes |
| Stamps over play, worst case | `paper` / `julian` | `plate` 0.75 over `paper` | 7.1 / 4.6 | 3.0 (large) | Yes |
| (not allowed) red stamp over play | `danger` | `plate` 0.75 over `paper` | 1.7 | 3.0 | No: so no `danger` stamp is ever drawn over play; the blooper slate is a card on `ink` |

Non-text (WCAG 1.4.11, 3:1): `glyph.danger` on `ink` 3.9, `glyph.julian` 10.7, `glyph.off` and the composure
hairline (`silver`) 4.3, pips and keycaps (`paper`) 16.3: all pass. `glyph.danger` on a `bone` tape is 2.8, which
fails, so on tape it gets the 2 px `ink` border (`ink` on `bone` 11.0).

**Danger and Julian without hue.** Every red thing differs from every gold thing in at least four ways that do not
need colour vision, and all of them survive the monochrome film if the stencil ever fails.

| Cue | Danger (red) | Julian (gold) |
|---|---|---|
| Value | Dark-mid: luma 81 of 255; the grade floors it at 0.20 | Light: luma 187; the grade floors it at 0.45. Luminance ratio gold to red 2.7:1 |
| Shape in the HUD | Square (the tally) | T (a tape mark) |
| Line in the world | Frames dashed (20 cm on, 10 cm off, from the eye outwards); tape hatched at 45 degrees, red and `ink` in 20 cm stripes | Marks are solid T-shaped plates; the case, the trap's mark and the exit frame are solid |
| Motion | Pulses on every beat (frames, tally, glyph, SEEN) | Never moves, never pulses |
| At the alarm | Frames turn `alarm` white, still dashed, pulsing | Unchanged |
| Protanopia (Vienot simulation) | `#595922`, luminance 0.09 | `#BFBF4A`, luminance 0.49 (ratio 3.8:1) |
| Deuteranopia (Vienot simulation) | `#7F7F00`, luminance 0.20 | `#C9C945`, luminance 0.54 (ratio 2.4:1) |

For deuteranopia the value gap alone is modest, which is why shape, line and motion must all ship, not just one.

**Flashing** (WCAG 2.3.1, no more than three flashes a second): the film flicker changes the picture by 6% at
most (under the 10% that counts as a flash); the cut's 0.35 `paper` flash can happen at most once per flight
(presses during a flight are dropped); beat pulses, the alarm's white frames and SEEN's fill swap run at 2 per
second. The pyro's flash (a set event, out of scope here) must stay at one flash per fire.

**Other:** every function is on keys; no colour is the only carrier of any state; text never goes below 16 px at
1080 (scale clamps at 0.8, so 13 px at 864 lines and below; judges are expected at 1080 or above). Time limits are
the game's challenge (the band's bars), which WCAG exempts as a real-time event.

### 15.9 Dressing the set

Design by value first: the film shows the set in monochrome and the making-of in steel blue, so hue only shows
after the alarm's bleed. Hue is kept low (no channel more than about twice another) so that at full colour the red
and gold still own the picture. All of it is material instances and boxes made by `tools/build_auction.py`.

**Materials** (linear base colour, roughness; replace or retune the `make_mi` list at the top of the dressing).

| Instance | Base colour (linear) | Rough. | Where | Replaces |
|---|---|---|---|---|
| `MI_Front_Street` | 0.025, 0.025, 0.027 | 0.35 | Road: low roughness so the lamps streak on it | `m_street` |
| `MI_Front_Pave` | 0.11, 0.105, 0.10 | 0.8 | Pavement, kerb | `m_pave` |
| `MI_Front_Facade` | 0.20, 0.19, 0.18 | 0.7 | Outside walls, stone | street walls in `m_wall` |
| `MI_Front_Canopy` | 0.035, 0.033, 0.03 | 0.5 | Canopy over the doors, door leaves | `m_door` (was green) |
| `MI_Front_Glass` | 0.02, 0.025, 0.03 | 0.1 | Windows | `m_window` (was blue) |
| `MI_Front_Marble` | 0.22, 0.21, 0.20 | 0.25 | Lobby floor, the grand stair | lobby `m_floor` |
| `MI_Front_Wall` | 0.16, 0.15, 0.14 | 0.85 | Lobby and hall walls | `m_wall` (was blue-grey) |
| `MI_Front_Panel` | 0.06, 0.05, 0.045 | 0.6 | Dark wood: wainscot, desk fronts, bench frames | new |
| `MI_Front_Parquet` | 0.10, 0.09, 0.08 | 0.45 | Hall floor | hall `m_floor` (0.045, too dark for a lit floor of 95-130) |
| `MI_Front_Trim` | 0.34, 0.33, 0.31 | 0.35 | Pewter: cornices, rails, picture frames, rope posts, column caps. Never brass: gold is Julian's | `m_trim` |
| `MI_Front_Velvet` | 0.06, 0.035, 0.045 | 1.0 | Bench seats, sofas, the rope, the house curtain, the aisle runner | `m_sofa`, `m_rope` (red was a false danger) |
| `MI_Front_Linen` | 0.45, 0.43, 0.40 | 0.9 | Tablecloths, the lots table, canvases in frames | new |
| `MI_Front_Plinth` | 0.40, 0.39, 0.37 | 0.6 | Exhibit plinths, the podium, the scanner's arch | `m_case` |
| `MI_Front_Stage` | 0.05, 0.045, 0.04 | 0.5 | Dais deck and steps | `m_dais` |
| `MI_Front_Foliage` | 0.05, 0.07, 0.05 | 0.9 | Plants | `m_plant` (was green) |
| `MI_Back_Ply` | 0.20, 0.17, 0.13 | 0.9 | The raw backs of the flats | backstage `m_wall` |
| `MI_Back_Batten` | 0.28, 0.24, 0.18 | 0.9 | Braces and battens on the flats | new |
| `MI_Back_Concrete` | 0.12, 0.12, 0.12 | 0.9 | Backstage floor, the loading dock | backstage `m_floor` |
| `MI_Back_Steel` | 0.08, 0.08, 0.085 | 0.4 | Catwalk, Maya's platform, truss, rails, road cases | `m_deck` |
| `MI_Back_Tape` | 0.65, 0.63, 0.58 | 0.9 | Camera tape: edge strips, spike marks, tape on rails | new |
| `MI_Back_Cable` | 0.012, 0.012, 0.012 | 0.5 | Cable runs, sandbags' ties, stage weights | `m_tire` |
| `MI_Back_Tube` | unlit, emissive `mo.high` x 6 | | Fluorescent work tubes (practicals, no light of their own) | `m_glow` (was saturated blue) |
| `MI_Crew_Ring` | unlit, emissive `mo.high` x 2 | | Device reach rings on the floor | the gold device rings (gold is Julian's) |
| `MI_Exit_Sign` | unlit, emissive `bone` x 1.5 | | Fire exit signs | `m_exit` (was saturated green) |
| `MI_Red` | `danger` linear, emissive 0.3 x the same, stencil 1 | 0.6 | Watchers' frames (dashed: 20 cm on, 10 cm off by world distance from the eye), tally lights, the scanner's bar | `m_red` |
| `MI_Red_Hatch` | 45-degree stripes, 20 cm, `danger` and `ink`, emissive 0.3 on the red, stencil 1 | 0.8 | Red tape (every `RedTapeZone`), the carpet's edges | the tape's `m_red` |
| `MI_Red_Carpet` | 0.33, 0.03, 0.04, stencil 1 | 0.95 | The red carpet (it is red tape, STATUS step 2) | `m_carpet` |
| `MI_Gold` | `julian` linear, emissive 0.5 x the same, stencil 2 | 0.4 | Marks (solid T plates), the case, the trap's mark, the gold side exit's frame, the scanner's open bar | `m_gold` |
| Heads of stand-ins | 0.45, 0.43, 0.40 | 0.8 | Placeholder people's spheres | `m_head` |

Signs (TextRender): default colour `bone` linear instead of today's gold-ish (1.0, 0.85, 0.45); `EXIT` at the gold
side door and `STAGE TRAP` in `julian` (stencil 2); `LIVE BROADCAST` in `danger` (stencil 1); `SECURITY` in `bone`.

**Small boxes per zone** (with `deco()`, none blocking a sight line, the aim or a camera):

| Zone | Add | Reads as |
|---|---|---|
| Street and queue | A 15 cm kerb along the road; pewter stanchions every 1.2 m along the queue with `MI_Front_Velvet` ropes; a press wall at PRESS (a 4 x 2.4 m `MI_Front_Linen` flat); two potted bay trees at the doors (cylinder pot, sphere crown); canopy edge trim | A gala entrance with a queue |
| Lobby | Wainscot (`MI_Front_Panel`, 0.9 m tall, 4 cm proud) on every wall; a pewter cornice rail at 3.6 m; a chandelier of five `MI_Exit_Sign`-style emissive spheres over the atrium; the reception desk faced in panel | An old auction house |
| Hall | Pilasters every 6 m on the long walls (0.6 x 0.3 m boxes, pewter caps); framed canvases between them (pewter frame, linen canvas); wall sconces (small emissive `bone` boxes at 2.4 m, no lights); a velvet runner along each side aisle (never the centre aisle: that is the drone's) | A sale room |
| Dais and stage | Pewter lip trim on the dais edge; footlights (a row of small emissive `bone` boxes along the front, 40 cm apart); the pyro charges as small `MI_Back_Cable` cylinders; the house curtain in velvet | A stage |
| Backstage corridor | On the flats' backs: a batten frame every 2.4 m and a diagonal brace (a rotated box) with a stage weight at its foot; cable runs along the wall base; road cases stacked in twos (steel with pewter edge strips); `MI_Back_Tape` spike marks (two crossed 30 x 4 cm strips) on the floor; a fluorescent tube every 6 m on the wall at 2.2 m; signs `PROPS` and `STAGE RIGHT` in `bone` | The back of a film set |
| Catwalk and Maya's platform | Steel floor with a `MI_Back_Tape` strip along both edges; truss boxes overhead; black lamp housings hung on the rail every 2 m (cylinders, unlit); cable loops; tape wrapped on the rails at the posts | A lighting rig |

**Lighting per zone.** The film reads value, so each zone gets one key pool where Julian walks, a cool rim that
cuts the heavies out of the dark, and haze where a beam should show. Colours as built: `WARM` (1.0, 0.82, 0.60),
about 3200 K; `COOL` (0.55, 0.70, 1.0), about 9000 K; `WORK` (0.70, 0.92, 1.0), cyan-white, about 8000 K; `GOLDL`
(1.0, 0.70, 0.20), a gold gel. Shadows: **at most four casters in any view**.

| Zone | Key | Rim or back | Practicals | Haze | Shadow casters in view |
|---|---|---|---|---|---|
| Street | `Key_Scanner`, `WARM`, 420 cd (keep) | The moon, `COOL`, from behind the building | Lamp heads, canopy lamps (as built) | Volumetric 1.5 on `Key_Scanner` | `Key_Scanner`; the searchlight when it burns: 2 |
| Lobby | `Key_Stair`, `WARM` (keep); chandelier point light (keep, no shadows) | None | Sconces | Low | `Key_Stair`: 1 |
| Hall floor | `Key_Aisle` x3 and `Key_Tape`, top-down pools (keep, no shadows); `Key_Rope` (shadows) | `Rim_Hall` x2, `COOL`, from the dais end (keep) | Sconces | 2.0 in the keys: shafts | `Key_Rope`, the follow spot when it burns, `Key_Podium` and `Key_Reel` seen down the hall: 4 |
| Dais | `Key_Podium` on Vance, `WARM` (shadows); `Key_Reel`, `GOLDL` (shadows); `Fill_Dais` (keep, low) | **Add** `Back_Dais`: one spot behind the podium, 4 m up, `COOL`, aimed at the house, 900 cd, 40 degree cone, haze 2.0, no shadows: the noir backlight that rims Vance and the bodyguard | Footlights (emissive only) | 1.2 | 3, or 4 with the follow spot |
| Backstage | `WORK` lamps (keep, no shadows); tubes are emissive only | None | Tubes | None: video is clear | 0 |
| Catwalk, platform | `WORK` lamps (keep) | The hall's rims reach up | Lamp housings | From the hall | 0, or 1 with the follow spot |

Exposure stays manual (as built). Tune `KEY_CD`, `RIM_CD` and the exposure until 15.11 check 1 passes, not by eye.

### 15.10 Characters by tint

Tints are linear base colours on the template mannequins (`tint` on the character; the watchers' and stand-ins'
material instances). The ladder: Julian alone at the top, the crew and guests in the middle, every watcher at the
bottom.

| Who | Tint (linear) | Band | Note |
|---|---|---|---|
| Julian | 0.80, 0.78, 0.72 | Hero 0.70-0.85 | The white dinner jacket: the lightest thing in every film frame |
| Maya | 0.38, 0.40, 0.42 | Crew 0.30-0.45 | Light grey; was saturated blue (0.10, 0.35, 0.80) |
| Vex | 0.16, 0.17, 0.18 | Crew 0.15-0.20 | Dark grey, a value step from Maya; was saturated green (0.10, 0.55, 0.30) |
| Guests and stand-ins (bodies) | 0.20, 0.28 or 0.35, neutral | Set 0.20-0.35 | Three values in rotation, never above 0.40 so Julian stays lightest; heads 0.45 |
| The band | 0.04, 0.04, 0.04 | Dark | Suits; they bob, they are not heavies |
| Doorman, chief, guard, the alarm's guards | 0.02, 0.02, 0.022 | Heavies 0.01-0.03 | |
| Bodyguard | 0.012, 0.012, 0.012 | Darkest | The last heavy before the reel |
| Vance | 0.03, 0.028, 0.026 | Heavy | Read by `Key_Podium` and `Back_Dais`, not by his own value |
| The drone's body | 0.02, 0.02, 0.02 | Heavy | Its tally is `MI_Red` |

No character carries a stencil: Julian is white, not gold.

### 15.11 Checks

Run on a 1920x1080 screenshot after any grade, lighting or material change (a scripted PIE frame or the package's
`play2.ps1` shots). Sample a 5 x 5 px patch, average R, G and B (sRGB 0-255).

| # | Check | Where (default follow framing, Scene 2 at the floor mark, camera 1) | Pass |
|---|---|---|---|
| 1 | Hero | Julian's back between the shoulder blades (about x 960, y 700) | 200 or more |
| 1 | Heavies | The nearest guard's torso | 45 or less |
| 1 | Lit floor | Floor inside an aisle key pool | 95-130 |
| 1 | Floor in shadow | Floor between two bench rows | 45-70 |
| 1 | Black point | The darkest patch of the picture | Within 5 of the bars' `ink` (13, 12, 11) |
| 2 | Stencil red | Red tape at the rope, in `Key_Rope`'s pool | R 150 or more, G and B 70 or less |
| 2 | Red in shadow | Red tape outside any key | R at least 2.2 x G, average 51 or more |
| 2 | Stencil gold | The hall's gold mark | R 200 or more, B 110 or less, average 115 or more |
| 3 | Making-of | The same wall on camera 2 | B at least R + 25 |
| 3 | Making-of highlight | A work tube | 225 or more, B at least R |
| 4 | Thumbnail | The whole frame scaled to 315 x 250 | Still shows: the letterbox, Julian as the lightest shape, one red frame or tape, one gold mark |
| 5 | Grade off | `GradeStrength` 0 against the grade removed | Unchanged (once per engine build) |
| 6 | Labels | Maya's platform view (as `step2_10_maya_platform.png`) | At most 5 labels, none overlapping, none in the bars |
| 7 | Alarm | 2.5 s after the finale goes live, camera 1 | Full colour; frames white and pulsing; the gold mark still passes check 2 |

Only Marwan can judge grain, flicker, the burn and the overall mood; report those as "needs your eye".

### 15.12 Filled in by Claude

Decisions the sources did not settle (Marwan can overrule any of them):
- The same 2.39:1 bars on all three cameras, painted by the HUD; the HUD never moves between cameras.
- The grade as an After Tonemapping post-process material with `MPC_Grade`; the film's tone is §3's Silver stops;
  levels, contrast 0.6, vignette 0.30; the making-of at 90% strength with no vignette.
- Stencil 1 red and 2 gold, with value floors (0.20 red, 0.45 gold) and a hue-key fallback.
- The bleed as `GradeStrength` 1 to 0 over 2 s on a smoothstep; `AlarmSaturation` 1.15 (was 1.25); the frames'
  white pulse at emissive 4, falling to 35% within each beat.
- The making-of overlay is a viewfinder (corner marks, reticle, scan lines), not film; the reticle only on the
  making-of, gold while Use is up.
- Grain 0.07 in play (§8 said 0.10), louder only after cuts and on cards and end pages.
- The splice, the burn (on every camera), and the blooper's burn-to-paper sequence.
- Lanes: top bar for where and what next, bottom bar for hands and Julian; flashes take the goal line's place;
  every refused press explained in B1.
- Dropped: the REC dot, the timecode, "A - JULIAN", "CUT -> MAYA", the gold "BAND BREAK".
- Device uses as pips under the MAYA and VEX key tabs.
- The label rule; making-of labels as camera tape (`bone` with `ink` text); on the film, countdown tags only.
- Glyphs: a square for danger, a T for Julian; red frames dashed, red tape hatched, red pulses, gold never moves.
- The slate as the one bold piece; the opening slate in the picture's lower left while play runs.
- Renamed devices (CHAMPAGNE CART, SOUND DESK, PYRO BOARD, CURTAIN WINCH), `THE STREET` for the first
  checkpoint, reworded blooper reasons, "terrace" for "canopy", no "run".
- Device rings, the rope, the curtain, the exit signs, plants, doors and glass lose their colour; signs go `bone`.
- Character values: crew and guests in the middle, the band dark, Julian alone at the top; the new `Back_Dais` rim.

**Conflicts in the sources and how they were settled:**
- §8 puts the overlay in a UMG widget; the brief and the build draw on Canvas: Canvas.
- §9 keeps gold for "the bonus"; STATUS makes gold Julian's: STATUS. So composure stays gold, but the band break,
  device rings and the active key tab lose it.
- Step 2 showed labels on Julian's camera "dimmer"; STATUS rule 1 says labels only on set. Now that the split is
  built, rule 1 wins, except the countdown tags, because the look must not hide a rule.
- §7's engine vignette (0.6, built 0.55) moves into the material so it covers only the film.
- Step 3's 60% saturation stand-in is replaced by the grade.

### 15.13 Only Marwan can judge

- The split against `bAllFilm`: does the cut to steel-blue video read as "the making-of", or as a different game?
- Grain, flicker, dust, the burn's strength and the vignette.
- The opening slate in the lower left while he can already walk: charming, or in the way?
- Tape labels: readable at a glance on Maya's platform, and is the film camera fair without them?
- Hatched tape and dashed frames: still clearly "red tape" and "camera frames"?
- The bleed at the alarm and the white pulse.
- The words: the director's lines, "Walked into the beam", the title card's two rule lines.
- The hall's and the backstage's darkness after the new floor values and `Back_Dais`.

## Sources

- Epic, *Mobile Rendering Features Reference*, UE 5.6 documentation (feature support on the mobile renderer).
- Epic, *Post Process Effects*, *Color Grading and the Filmic Tonemapper*, *Using Look-up Tables for Color Grading*,
  *Auto Exposure*, *Bloom*, *Cinematic Cameras* (setting names as they appear in the Details panel).
- Fonts: Google Fonts, SIL OFL 1.1.
- Past Epic MegaJam winners were checked only by title (2021-2023 results pages). The pages found don't describe how
  the games look, so nothing here relies on them.
