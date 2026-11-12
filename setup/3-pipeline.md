# Stage 3: prove the pipeline, end to end

Stage 2 is done: `Reel\` exists, the editor is open, and this is a new session that should have the `unreal-mcp`
tools. This stage does once, on the untouched template, everything Sunday night depends on: drive the editor, change
C++, package for Windows, package for the browser, and build the zip. Each failure found now costs minutes; found on
Sunday it costs the entry.

**Done when:** each test below has passed or has a one-line reason it didn't; `docs/UNREAL_MCP.md` exists; the
commands in `CLAUDE.md` are the ones that worked on this PC; times and sizes are in `docs/DEVLOG.md`; and the work
is committed and pushed. Leave the template level as you found it.

**This runbook has never been run.** Tool names and commands are best guesses from Epic's documentation. Use what
`list_toolsets` reports, and correct this file and `CLAUDE.md` when something differs. Claude Code stops waiting for
a tool call after about 120 seconds: keep each editor call short, and run builds in the background with a log.

## 0. Is the editor connected?

Call `list_toolsets` on the `unreal-mcp` server. If this session has no such tools:

- the editor must have been running before the session started: start a new session;
- the session must be opened in the repo folder, where `.mcp.json` is;
- All Toolsets must be enabled (Edit > Plugins in the editor);
- failing those, ask Marwan to click Generate Client Config (Edit > Editor Preferences > Agentic Workflows > Model
  Context Protocol, Client = Claude Code) and compare the generated `Reel\.mcp.json` with the one at the repo root.

## 1. Learn the tools, and write them down

List the toolsets and read the descriptions of the ones a small game needs: levels and actors, Blueprints, assets
and import, Python, viewport screenshots, play-in-editor, Live Coding, saving, and exiting the editor. Write
`docs/UNREAL_MCP.md`, 40 lines at most: which toolset and tool does each job, with one working example call each,
and any trap you hit. Later sessions read this instead of rediscovering it.

## 2. Drive the editor

- Spawn five cubes in a row in the open level, give each a different colour, take a viewport screenshot and
  describe what you see. The description has to come from the image, not from the calls you made.
- Start play-in-editor, confirm from the tools or the log that the player character spawned, and stop it.
- Delete the cubes. Don't save the level with them in it.

## 3. C++ round trip

Make a small change to a function body in the template's character class whose effect shows in play, for example an
on-screen debug message each time the character jumps. Avoid changing constructor defaults for this test: Live
Coding does not reliably re-apply them to Blueprint children that already exist. Compile with Live Coding (its
toolset if there is one; otherwise ask Marwan to press Ctrl+Alt+F11 in the editor), check the effect in
play-in-editor, and time it. Then revert the change and compile again.

## 4. Package for Windows

Read `docs/MACHINE.md`: if it says to close the editor while packaging, save, exit the editor first and restart it
afterwards. Run in the background, with the log at `C:\Jam\Builds\package-win.log`, and time it:

```
"<UE>\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="<repo>\Reel\Reel.uproject" -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -prereqs -archive -archivedirectory="C:\Jam\Builds" -nop4 -utf8output
```

- `-prereqs` bundles the Visual C++ runtime installer, so the game starts on a judge's PC that lacks it.
- Start the packaged `.exe` windowed, check the process is still alive after 20 seconds, ask Marwan whether the
  level showed and the character moved, then close it.
- Zip the packaged folder with 7-Zip as `C:\Jam\Builds\Test_Reel.zip` and report its size. The jam's limit is 1 GB.

## 5. Package for the browser (stop after 30 minutes)

A browser build is a bonus that gets an entry more plays. It is experimental in this Preview. Start a 30-minute
clock at the first command; when it runs out, write down how far it got and stop.

- Use the platform name stage 2 wrote in `docs/MACHINE.md`. Same command as above with that platform, Shipping, its
  own log. If the command line won't take it, ask Marwan to run it from the editor: Platforms > Web (or Wasm) >
  Shipping > Package Project.
- Start the result with the `Play_*.bat` in the packaged folder (opening `index.html` from disk fails by design)
  and ask Marwan whether it plays in Chrome.
- Check the limits: under 500 MB in total, under 200 MB per file, `index.html` at the top level of the zip.
- Write the outcome under "Not decided" in `docs/STATUS.md`: Web ships, or Windows only and why.

## 6. Only if `docs/MACHINE.md` says "RTX award possible" (10 minutes at most)

Check whether NVIDIA's DLSS plugin has a build for this engine version. Write the answer and the link in
`docs/MACHINE.md`. Install nothing now; it is a Saturday decision.

## 7. Record, commit, hand over

- `CLAUDE.md`: replace the commands under "Commands" with the exact ones that worked here.
- `docs/DEVLOG.md`: times for the editor build, a Live Coding compile, the Windows package and the Web package; zip
  sizes; what fought back. Sunday's schedule is built from these numbers.
- `git add -A`, `git commit -m "Stage 3: pipeline proven on the template"`, `git push` (same LFS rule as stage 2).

End by telling Marwan, in a short table, what passed and what didn't, then the two things that are theirs:

1. **Rehearse the upload** (10 minutes, in a browser): on itch.io create a new project and leave it as a **Draft**,
   upload `Test_Reel.zip` (and the Web zip marked "playable in the browser", if it worked), open the draft page, then
   delete the test uploads. Keep the draft: it becomes the real page, and its devlog is the Glass Box entry.
2. **Start the game:** paste prompt 4 from `README.md`.
