# Mint stage 3: prove the pipeline

Stage 2 is done: `Reel/` exists, the editor is open on the PC's desktop, and this is a new session that should have
the `unreal-mcp` tools. This stage proves, on the untouched template, everything the Mint side of the jam depends
on: driving the editor, changing C++, packaging, and handing the project to the Windows boot. Each failure found now
costs minutes; found on Sunday it costs the entry.

**Done when:** each test below has passed or has a one-line reason it didn't; `docs/UNREAL_MCP.md` exists; the
commands in `CLAUDE.md` are the ones that worked here; times are in `docs/DEVLOG.md`; the project is mirrored onto
the Windows partition; and the work is committed and pushed. Leave the template level as you found it.

**This runbook has never been run.** Tool names and commands are best guesses from Epic's documentation. Use what
`list_toolsets` reports, and correct this file and `CLAUDE.md` when something differs. Claude Code stops waiting for
a tool call after about 120 seconds: keep each editor call short, and run builds in the background with a log.

## 0. Is the editor connected?

Call `list_toolsets` on the `unreal-mcp` server. If this session has no such tools:

- the editor must have been running before the session started: start a new session;
- the session must be opened in the repo folder, where `.mcp.json` is;
- All Toolsets must be enabled (Edit > Plugins in the editor);
- failing those, ask Marwan to click Generate Client Config (Edit > Editor Preferences > Agentic Workflows > Model
  Context Protocol, Client = Claude Code) and compare the generated `Reel/.mcp.json` with the one at the repo root.

If the tools can't be made to work on Linux within about an hour, stop: that is the tripwire in stage 1.

## 1. Learn the tools, and write them down

List the toolsets and read the descriptions of the ones a small game needs: levels and actors, Blueprints, assets
and import, Python, viewport screenshots, play-in-editor, compiling, saving, and exiting the editor. Write
`docs/UNREAL_MCP.md`, 40 lines at most: which toolset and tool does each job, with one working example call each,
and any trap you hit. Later sessions read this instead of rediscovering it.

## 2. Drive the editor

- Spawn five cubes in a row in the open level, give each a different colour, take a viewport screenshot and
  describe what you see. The description has to come from the image, not from the calls you made.
- Start play-in-editor, confirm from the tools or the log that the player character spawned, and stop it.
- Delete the cubes. Don't save the level with them in it.

## 3. The C++ loop on Linux

Live Coding, Unreal's fast C++ reload, is a Windows feature as far as the documentation says. Find out what this
build offers on Linux and measure it, because this loop sets the pace of the whole jam:

- Make a small change to a function body in the template's character class whose effect shows in play, for example
  an on-screen debug message each time the character jumps.
- Try the fastest route first: a compile or hot-reload tool in the toolsets, or the editor's own compile button
  (ask Marwan to click it through Moonlight). Check the effect in play-in-editor and time it.
- Then time the reliable route: exit the editor through its tool, build with `Build.sh`, start the editor again,
  and get the `unreal-mcp` tools back in this session. Write down exactly how the reconnect works.
- Put both timings and the recommended loop in `docs/UNREAL_MCP.md` and in the "C++ changes" section of
  `CLAUDE.md`. If the reliable route takes over two minutes, say so plainly: it argues for keeping tunable values
  in Blueprints and data, so most iteration needs no compile.
- Revert the test change.

## 4. Package on Linux (a smoke test, not the entry)

A Linux package can't be submitted, but it catches the bugs that only appear outside the editor, without a reboot.
Read `docs/MACHINE.md`: if it says to close the editor while packaging, exit it first and restart it afterwards.
Run in the background, with the log at `~/Jam/Builds/package-linux.log`, and time it:

```
"<UE>/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun -project="$HOME/Jam/megajam/Reel/Reel.uproject" -platform=Linux -clientconfig=Shipping -build -cook -stage -pak -archive -archivedirectory="$HOME/Jam/Builds" -nop4 -utf8output
```

Start the packaged game on display `:0`, check the process is still alive after 20 seconds, ask Marwan whether the
level showed and the character moved, then close it.

## 5. Hand the project to Windows

- Run `tools/sync-to-windows.sh`. It mirrors the repo onto the Windows partition at `Jam/megajam` and prints what it
  synced. If it reports the partition as read-only, Windows needs one boot ending in Restart; tell Marwan.
- Check that `Reel/Reel.uproject`, `Reel/Source/` and `setup/win-1-tools.md` are there.
- Copy the Mac's key line from `~/.ssh/authorized_keys` to `Jam/mac_key.pub` on the Windows partition (beside the
  mirror, not inside it). The Windows side uses it to let the Mac in over SSH.
- Find the Windows entry in `efibootmgr` and write its number into `docs/MACHINE.md`, so the switch command in
  `setup/0-remote.md`, part D, is ready to paste.

## 6. Record, commit, hand over

- `CLAUDE.md`: the exact commands that worked here.
- `docs/DEVLOG.md`: times for the editor build, the C++ loop both ways and the Linux package; what fought back.
- `git add` the files you changed, `git commit -m "Mint stage 3: pipeline proven on the template"`, `git push`.

End by telling Marwan, in a short table, what passed and what didn't, then what is theirs:

1. **Tonight, before sleeping:** switch to Windows (`setup/0-remote.md`, part D) and run the Windows side,
   `setup/win-1-tools.md`. Its downloads run overnight, when the PC isn't needed for building.
2. **Now:** the game. The prompts are in `README.md`.
