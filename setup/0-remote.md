# From the MacBook: follow the sessions, see the screen

Claude Code runs on the PC itself. The MacBook reaches it two ways, and neither needs SSH or any port opened by
hand:

- **Remote Control** carries the Claude sessions: they run on the PC and you read and steer them from the Mac.
- **Sunshine** (on the PC) and **Moonlight** (on the Mac) carry the PC's screen, keyboard and mouse.

None of this has been run on this PC yet.

## A. The sessions

Open https://claude.ai/code in the Mac's browser, signed in to the same Claude account as on the PC (the Claude
phone app shows them too). Sessions on the PC have a computer icon with a green dot:

| Name | What it is |
|---|---|
| `jam-setup` | The setup session that `go.cmd` started. |
| `jam-pipeline` | The session the setup starts by itself for stage 3, once the editor is running. |
| `jam-pc` | The door: a Remote Control server left running on the PC. Start new sessions here, for example the one that builds the game. |

A session only exists while its window on the PC stays open and the PC stays on. If `jam-pc` is gone (after a
restart, say), open a terminal on the PC through Moonlight and run:

```
cd C:\Jam\megajam
claude remote-control --name jam-pc --permission-mode bypassPermissions --spawn=same-dir
```

## B. The screen

1. Install Moonlight on the Mac:

   ```bash
   brew install --cask moonlight
   ```

2. In the Mac's browser open `https://<address>:47990`, with the PC address the setup session printed (it is also
   in `docs/MACHINE.md`). The browser warns about the certificate, because Sunshine signs its own: continue. Create
   a Sunshine username and password of your choice.
3. Open Moonlight. The PC appears by itself, or add it by its address. Click it: Moonlight shows a four-digit PIN.
4. In the Sunshine page, **PIN** tab: enter the PIN and any device name.
5. In Moonlight, click the PC > **Desktop**. Quit the stream with Ctrl+Option+Shift+Q.

Sunshine runs as a Windows service, so the stream works at the sign-in screen and shows Windows' permission prompts.
It streams a real display: the PC's monitor has to stay connected. There is no shared clipboard; when something
long has to be typed on the PC, ask a session to do it.

## If it doesn't work

- **No sessions at claude.ai/code:** Remote Control needs a Claude subscription sign-in on the PC (not an API key)
  and the same account on the Mac. On the PC, type `/remote-control` in the session to see why it isn't connected.
  Until it works, use the sessions in their terminal windows through Moonlight.
- **Moonlight shows a black screen:** the monitor is off or unplugged.
- **The Sunshine page doesn't open:** ask a session for the PC's current address; it can change after a restart.
