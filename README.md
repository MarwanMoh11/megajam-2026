# Cutaway

A heist told as a film, made in four days for the **Epic AI MegaJam 2026** (theme: *Director's Cut*) by one
developer, Marwan Abudaif, directing a crew of Claude agents. Unreal Engine 5.9 MegaJam Preview, C++, Windows.

- **Play it:** the jam build is on itch.io (Windows, zip). Unzip and run `Reel.exe`.
- **How it was made:** [`docs/DEVLOG.md`](docs/DEVLOG.md), written as it happened, hour by hour, with what went wrong.
- **What it used:** [`docs/CREDITS.md`](docs/CREDITS.md) lists every outside asset with its licence, and every AI tool.

## The game

Vance thinks he's the star of a smooth 1970s thriller. His crew knows better. Over one night at the Grand Lumière
Theater, the team steals the only master reel of *Le Soleil Noir*, a lost 1974 film, before the billionaire Arthur
Pendleton locks it in his vault for good.

The theme is the camera. Vance's scenes are the **Director's Cut**: letterboxed 35mm, warm grain, sweeping angles.
Every time the story cuts to the crew, the bars and the grain drop away into the raw, unpolished feed of what it
really takes:

| Scene | Who | What you do |
|---|---|---|
| 1. The theater steps | Vance (35mm) | Queue for the door, where cards are suddenly void: palm scan only |
| 2. The florist van | Max (raw) | Tune the dials to spoof the palm scanner |
| 3. Foyer and staircase | Vance (35mm) | Pick the right lines to get through the party |
| 4. The catwalks | Nora (raw) | Clear Vance's lane from above |
| 5. The private auditorium | Vance (35mm) | Slip away along the wall during Pendleton's speech |
| 6. Backstage | Cole (raw) | Take down the enforcers, hide the bodies |
| 7. The projection booth | Vance (35mm) | Swap the reel, then fight your way out |
| 8. The rear alley | Vance (35mm) | Run for the car under fire |

**Controls:** WASD to move, mouse to look, **E** to use what's in front of you (hold for longer actions), **R** for a
new take. Each scene's goal is on screen. A mistake is a blooper, not a game over: the take restarts from that scene.

## How it was made

One person directed; the agents built. Claude Code sessions (Claude Fable orchestrating, Claude Opus agents
designing, building and reviewing) wrote the C++ gameplay, drove the Unreal editor through its MCP server, dressed
the sets, staged the cast and packaged the builds. Hyper3D Rodin made the props, ElevenLabs the voices, effects and
music, and Mixamo supplied the motion capture. The dev log records each decision, each agent's job and each thing that
broke.

The rules the agents worked under are in [`CLAUDE.md`](CLAUDE.md). Things made before the jam opened are declared in
[`docs/PRE_JAM.md`](docs/PRE_JAM.md).

## The repo

| Path | What |
|---|---|
| `Reel/` | The Unreal project (`Reel` was the working name). C++ gameplay in `Reel/Source/Reel/Take/` |
| `tools/` | Python the agents ran against the editor: set dressing, cast, props, audio, look, fonts |
| `docs/DEVLOG.md` | The dev log (the Glass Box entry) |
| `docs/SCRIPT.md` | The film's script, scene by scene; `docs/SCRIPT_ADAPT.md` maps it onto the build |
| `docs/STATUS.md` | The spec, the kill list and the build order as they evolved |
| `docs/ART_STYLE.md`, `docs/AUDIO_SCRIPT.md` | The look and the sound |
| `docs/CREDITS.md` | Assets, licences and AI tools |
| `prototypes/` | Browser grey boxes from before the engine work |
| `setup/`, `go.cmd` | The runbooks that set up the Windows PC unattended on day one |

### Building it

Windows, Unreal Engine 5.9 MegaJam Preview, Visual Studio with the Unreal workloads, Git LFS for the assets.

```powershell
git lfs install; git clone https://github.com/MarwanMoh11/megajam-2026
& "<UE_5.9>\Engine\Build\BatchFiles\Build.bat" ReelEditor Win64 Development "-Project=<repo>\Reel\Reel.uproject" -WaitMutex
```

Then open `Reel\Reel.uproject`. The packaging command is in `CLAUDE.md`.
