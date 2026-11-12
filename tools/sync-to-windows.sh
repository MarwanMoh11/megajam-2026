#!/usr/bin/env bash
# Mirror the working tree onto the Windows partition, so the Windows boot can package it.
# The Windows side is a build mirror: nothing is edited or committed there.
# Usage: tools/sync-to-windows.sh        (reads the mount point from .winroot, or from $WIN_ROOT)
set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
WIN_ROOT="${WIN_ROOT:-}"
if [ -z "$WIN_ROOT" ] && [ -f "$REPO/.winroot" ]; then WIN_ROOT="$(cat "$REPO/.winroot")"; fi
if [ -z "$WIN_ROOT" ]; then
  echo "No Windows mount point. Write it into $REPO/.winroot (for example /media/$USER/Windows)."; exit 1
fi
if [ ! -d "$WIN_ROOT/Windows" ]; then
  echo "$WIN_ROOT is not a mounted Windows partition (no Windows folder in it). Mount it first."; exit 1
fi

DEST="$WIN_ROOT/Jam/megajam"
mkdir -p "$DEST" 2>/dev/null || true
if ! touch "$DEST/.write-test" 2>/dev/null; then
  echo "The Windows partition is read-only. Boot Windows once and leave it with Restart, not Shut down."; exit 1
fi
rm -f "$DEST/.write-test"

# Generated folders are left out in both directions, so the Windows build cache survives each sync.
rsync -rt --delete --info=stats1 \
  --exclude '.git/' --exclude '.winroot' \
  --exclude 'Binaries/' --exclude 'Intermediate/' --exclude 'Saved/' --exclude 'DerivedDataCache/' \
  "$REPO/" "$DEST/"

{
  echo "commit: $(git -C "$REPO" rev-parse --short HEAD)"
  echo "uncommitted files: $(git -C "$REPO" status --porcelain | wc -l)"
  echo "synced: $(date '+%Y-%m-%d %H:%M %Z')"
} > "$DEST/SYNCED.txt"
cat "$DEST/SYNCED.txt"
