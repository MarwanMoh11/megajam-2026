# Stage 0: reach the PC from the MacBook

The PC upstairs dual-boots Linux Mint and Windows. The game is built on Mint; Windows is booted only to package,
because the jam accepts Windows builds and not Linux ones. Everything after this stage is done from the MacBook:

- **SSH** carries the agents. The Claude app on the Mac starts sessions that run on the PC.
- **Sunshine** (on the PC) and **Moonlight** (on the Mac) carry the screen, for the editor and for playing the game.

Both are free and open source, and on a home network neither needs an account or a cloud relay.

**Nothing here has been run on this PC.** Commands are from each project's documentation.

## A. At the PC's keyboard, in Mint (5 minutes, the only trip upstairs)

1. Open a terminal:
   ```
   sudo apt update && sudo apt install -y openssh-server
   hostname -I | awk '{print $1}'; whoami
   ```
   Note the two lines it prints: the PC's address and your username.
2. **Start the long download now.** In Firefox, open
   https://forums.unrealengine.com/t/unreal-engine-5-9-megajam-preview/2852840 , follow the "here for Linux" link,
   sign in to Epic and download the Linux build of the 5.9 MegaJam Preview into `~/Downloads`. It is tens of GB.
3. So you never have to come back up:
   - Menu > Login Window > Users: turn on **automatic login** for your user. Sunshine only runs once you are logged
     in, so without this you can't reach the desktop after a reboot.
   - Menu > Power Management: never suspend, never turn the screen off on idle.
   - Leave the monitor connected (it can be switched off at its button) and the PC on wired Ethernet if you can.

## B. On the MacBook

Tell the main Claude session the address and username; it writes the SSH config and makes a key. By hand it is:

1. A key just for this PC:
   ```bash
   ssh-keygen -t ed25519 -f ~/.ssh/id_ed25519_jampc -N ""
   ```
2. Two entries in `~/.ssh/config`. Both boots share one address but have different host keys; `HostKeyAlias` keeps
   them apart so SSH doesn't raise a "host identification has changed" alarm when you switch.
   ```
   Host jam-mint
     HostName <address>
     User <mint username>
     IdentityFile ~/.ssh/id_ed25519_jampc
     HostKeyAlias jam-mint

   Host jam-win
     HostName <address>
     User <windows username>
     IdentityFile ~/.ssh/id_ed25519_jampc
     HostKeyAlias jam-win
   ```
3. Install the key on Mint. It asks for your PC password once:
   ```bash
   ssh-copy-id -i ~/.ssh/id_ed25519_jampc.pub jam-mint
   ```
4. Check it, then install the few packages that need your password (the agents never type it):
   ```bash
   ssh -t jam-mint 'sudo apt install -y git git-lfs gh curl rsync p7zip-full efibootmgr vulkan-tools'
   ```
5. Sign the PC in to GitHub and clone the repo. `gh` prints a one-time code; open the address it shows on the Mac:
   ```bash
   ssh -t jam-mint 'gh auth login -p https -w && gh auth setup-git && gh repo clone MarwanMoh11/megajam-2026 ~/Jam/megajam'
   ```
6. In the Claude app: the environment selector under the prompt box > **New environment** > **SSH** tab > Host list >
   **Add SSH connection...** Name `jam-mint`, SSH host `jam-mint`. Then pick the folder `~/Jam/megajam` and create
   the environment. The app installs Claude Code on the PC the first time it connects.
7. Moonlight, for the screen:
   ```bash
   brew install --cask moonlight
   ```

## C. Pair the screen (after stage 1 has installed Sunshine)

1. On the Mac, open `https://<address>:47990` and accept the certificate warning (Sunshine signs its own). Create
   Sunshine's username and password.
2. Open Moonlight. The PC appears by itself, or add it by address. Click it: Moonlight shows a four-digit PIN.
3. Enter the PIN on Sunshine's PIN page. Start the **Desktop** stream.

In Moonlight's settings choose 1080p at 60 fps to start with, and raise it if the network keeps up.

## D. Switching to Windows and back without going upstairs

- **Mint to Windows:** find the Windows entry with `efibootmgr` (it is called "Windows Boot Manager"), then
  ```
  ssh -t jam-mint 'sudo efibootmgr -n <its four-digit number> && sudo reboot'
  ```
  `-n` sets the next boot only; nothing permanent changes.
- **Windows to Mint:** restart Windows. The normal boot order takes it back to Mint. **Always use Restart, never
  Shut down:** a Windows shutdown leaves its partition locked and Mint can then only read it.
- Reaching Windows from the Mac needs its own one-time setup: `setup/win-1-tools.md`.

## If it doesn't work

- **`ssh` times out:** the PC and the Mac must be on the same network; check the address again with `hostname -I`.
- **Moonlight shows a black screen:** the PC needs a connected display and a logged-in desktop session.
- **The stream stutters:** lower the bitrate in Moonlight, or move the PC to Ethernet.
- **Sunshine won't capture at all:** RustDesk is the fallback. It is open source too, but slower, so judge game feel
  at the PC itself.
