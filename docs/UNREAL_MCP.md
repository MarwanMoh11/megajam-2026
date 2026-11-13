# Driving the editor through MCP (proven in stage 3, 2026-11-13)

**Connect.** The editor serves MCP at `http://127.0.0.1:8000/mcp`, about 30 s after launch (59 s on the first start after a reboot). A
session's own `unreal-mcp` tools exist only if the editor was up when the session started. After an editor restart Claude can't re-dial
them (they're a project server, not a connector): Marwan can, with `/mcp` > unreal-mcp > Reconnect. Or skip that and use
`tools/ue_mcp.py`, which talks HTTP and works at any time: `PY="/c/Epic Games/UE_5.9/Engine/Binaries/ThirdParty/Python3/Win64/python.exe"`,
then `"$PY" -I tools/ue_mcp.py tool <Toolset.Tool> '<json>'`, or `... py file.py` to run Python in the editor. Images are saved as PNGs.
Natively there are four tools: `list_toolsets`, `describe_toolsets`, `describe_tools`, and `call_tool {toolset_name, tool_name, arguments}`.

| Job | Tool (toolset.tool) | Example arguments that worked |
|---|---|---|
| Anything else | `editor_toolset.toolsets.python_scripting.PythonScriptingTools.execute_python` | `{"script":{"content":"...","language":"Python"}}`; returns the last expression; `print` goes to LogPython |
| Spawn coloured cubes | execute_python | `EditorActorSubsystem.spawn_actor_from_object(load_asset('/Engine/BasicShapes/Cube'), loc)`, then a dynamic instance of `/Engine/BasicShapes/BasicShapeMaterial` with vector param `Color` |
| Find / delete actors | `EditorToolset.LevelToolset.FindActors` / `RemoveFromScene` | `{"filter":{"name":"Stage3Cube"}}` / `{"actor":{"refPath":"/Game/...:PersistentLevel.StaticMeshActor_UAID_..."}}` |
| Place from asset or class | `EditorToolset.LevelToolset.AddToSceneFromAsset` / `AddToSceneFromClass` | `{"assetPath":"/Game/...","name":"X","xform":{...}}` (untested) |
| Viewport screenshot | `SceneCaptureToolset.SceneCaptureToolset.CaptureEditorViewport` | `{"bShowUI":false}` |
| Whole editor screenshot | `EditorToolset.EditorAppToolset.CaptureEditorImage` | `{}` (shows PIE too, with the toolbar) |
| Camera | `EditorToolset.EditorAppToolset.GetCameraTransform` / `SetCameraTransform` | `{"translation":{x,y,z},"rotation":{pitch,yaw,roll}}` (not wrapped in `transform`) |
| Play in editor | `EditorToolset.EditorAppToolset.StartPIE` / `StopPIE` / `IsPIERunning` | `{}`; StartPIE returns after a 2 s warmup (4 s in all) |
| PIE screenshot | `SceneCaptureToolset.SceneCaptureToolset.CaptureGameViewport` | `{"world":{"refPath":"/Game/ThirdPerson/UEDPIE_0_Lvl_ThirdPerson.Lvl_ThirdPerson"},"maxResolution":1000}` |
| Press keys in PIE | `InputToolset.InputToolset.ApplyInput` | `{"entries":[{"action":{"refPath":"/Game/Input/Actions/IA_Jump.IA_Jump"},"value":{"x":1,"y":0,"z":0}}],"durationSeconds":0}` |
| Read the log | `ConsoleToolset.LogsToolset.GetLogEntries` | `{"pattern":"STAGE3","maxEntries":5}` |
| Live Coding | `LiveCodingToolset.LiveCodingToolset.IsLiveCodingReady`, then `CompileLiveCoding` | `{}`; returns "Live coding succeeded" |
| Exit the editor | `EditorToolset.EditorAppToolset.ExitEditor` | `{}`; refuses if anything is unsaved unless `bDiscardUnsavedChanges` |
| Save, import, Blueprints | `LevelToolset.SaveActor`, `TextureToolset`/`StaticMeshToolset`/`SoundWaveToolset`, `BlueprintToolsets.BlueprintToolset` | not tried yet; list with `describe_toolsets` |

**C++ timings on this PC.** Function body, Live Coding: **18-24 s** from the call to "succeeded", editor stays open; PIE sees the new code
at once. Slow route (new class, header, `UPROPERTY`): `ExitEditor` 6 s, `Build.bat` 11 s for one changed `.cpp` (4.3 min clean), editor
start to MCP 29 s: **about 100 s** from exit to the change confirmed in PIE. A header change rebuilds every file including it: longer.

**Traps hit.**
- Live Coding patches memory only. The DLL on disk is the last `Build.bat` one, so the next editor start runs old code until you
  rebuild with the editor closed. Rebuild before you trust a restart.
- `FocusOnActors` returns before the camera moves, so a capture right after shows the old view. Set the camera instead.
- `GEngine->AddOnScreenDebugMessage` text didn't show in either capture. Prove code ran with `UE_LOG` and `GetLogEntries`.
- Python scripts: imports only at top level, no `time.sleep` (use `await asyncio.sleep`). Windows Python prints `\r\n`: strip before
  reusing output in bash loops.
- From Git Bash, `cmd //c "...Build.bat"` mangles the quotes. Run `Build.bat` and `RunUAT.bat` from PowerShell.
