"""One editor, one writer: a lock for the Unreal editor, so several agents can take turns driving it.

Run with the engine's Python (stdlib only), from the repo root:
  PY="/c/Epic Games/UE_5.9/Engine/Binaries/ThirdParty/Python3/Win64/python.exe"
  "$PY" -I tools/editor_lock.py acquire <owner> [--wait 540] [--note "what for"]   # blocks; prints OK or TIMEOUT
  "$PY" -I tools/editor_lock.py release <owner>
  "$PY" -I tools/editor_lock.py status

acquire by the current owner refreshes the lock (do it every few minutes while working). A lock not refreshed for
15 minutes is stale and the next acquire takes it. Hold it for one burst of editor work (a script run, a few shots,
a save), then release: others are waiting. Exit codes: 0 got it, 2 timed out.
"""
import json
import os
import sys
import time

LOCK = r"C:\Jam\Builds\qa\editor.lock"
STALE = 15 * 60


def read():
    try:
        with open(LOCK, encoding="utf-8") as f:
            return json.load(f)
    except (OSError, ValueError):
        return None


def write(d, new=False):
    flags = os.O_WRONLY | os.O_CREAT | (os.O_EXCL if new else os.O_TRUNC)
    fd = os.open(LOCK, flags)
    with os.fdopen(fd, "w", encoding="utf-8") as f:
        json.dump(d, f)


def acquire(owner, wait, note):
    os.makedirs(os.path.dirname(LOCK), exist_ok=True)
    end = time.time() + wait
    while True:
        cur = read()
        now = time.time()
        if cur and cur.get("owner") == owner:
            cur["beat"] = now
            if note:
                cur["note"] = note
            write(cur)
            print("OK (refreshed, held %d s)" % (now - cur.get("since", now)))
            return 0
        if cur is None and not os.path.exists(LOCK):
            try:
                write({"owner": owner, "since": now, "beat": now, "note": note}, new=True)
                print("OK")
                return 0
            except FileExistsError:
                pass
        elif cur and now - cur.get("beat", 0) > STALE:
            again = read()
            if again and again.get("since") == cur.get("since") and again.get("beat") == cur.get("beat"):
                try:
                    os.remove(LOCK)
                    print("took a stale lock from %s" % cur.get("owner"))
                except OSError:
                    pass
            continue
        if now >= end:
            if cur:
                print("TIMEOUT: held by %s for %d s (%s)" % (cur.get("owner"), now - cur.get("since", now), cur.get("note", "")))
            else:
                print("TIMEOUT")
            return 2
        time.sleep(3)


def release(owner, force):
    cur = read()
    if cur is None:
        print("not held")
        return 0
    if cur.get("owner") != owner and not force:
        print("held by %s, not released" % cur.get("owner"))
        return 1
    os.remove(LOCK)
    print("released")
    return 0


def main(argv):
    if not argv or argv[0] == "status":
        cur = read()
        if cur:
            now = time.time()
            print("held by %s for %d s, last refresh %d s ago (%s)" % (
                cur.get("owner"), now - cur.get("since", now), now - cur.get("beat", now), cur.get("note", "")))
        else:
            print("free")
        return 0
    cmd, owner = argv[0], (argv[1] if len(argv) > 1 else "")
    if not owner:
        print(__doc__)
        return 1
    if cmd == "acquire":
        wait = float(argv[argv.index("--wait") + 1]) if "--wait" in argv else 540.0
        note = argv[argv.index("--note") + 1] if "--note" in argv else ""
        return acquire(owner, wait, note)
    if cmd == "release":
        return release(owner, "--force" in argv)
    print(__doc__)
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
