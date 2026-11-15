"""One editor, one writer: a lock for the Unreal editor, so several agents can take turns driving it.

Run with the engine's Python (stdlib only), from the repo root:
  PY="/c/Epic Games/UE_5.9/Engine/Binaries/ThirdParty/Python3/Win64/python.exe"
  "$PY" -I tools/editor_lock.py acquire <owner> [--wait 540] [--note "what for"]   # blocks; prints OK or TIMEOUT
  "$PY" -I tools/editor_lock.py release <owner>
  "$PY" -I tools/editor_lock.py status

acquire by the current owner refreshes the lock (do it every few minutes while working). A lock not refreshed for
15 minutes is stale and the next acquire takes it.
Waiters queue first-come first-served (C:\Jam\Builds\qa\editor.queue): a free lock goes only to the head of the
queue, so keep acquire running (--wait) to hold your place; status shows the queue. Hold it for one burst of editor work (a script run, a few shots,
a save), then release: others are waiting. Exit codes: 0 got it, 2 timed out.
"""
import json
import os
import sys
import time

LOCK = r"C:\Jam\Builds\qa\editor.lock"
STALE = 15 * 60
QUEUE = r"C:\Jam\Builds\qa\editor.queue"   # FIFO of waiters: a free lock goes to the head (Nov 15)
QMTX = QUEUE + ".mtx"
GONE = 30   # a waiter that has not polled for this many seconds has left the queue


def _qedit(fn):
    """Read-modify-write the queue under a tiny file mutex. fn(list) -> list."""
    for _ in range(200):
        try:
            fd = os.open(QMTX, os.O_WRONLY | os.O_CREAT | os.O_EXCL)
            os.close(fd)
            break
        except FileExistsError:
            try:
                if time.time() - os.path.getmtime(QMTX) > 10:
                    os.remove(QMTX)
            except OSError:
                pass
            time.sleep(0.05)
    try:
        try:
            with open(QUEUE, encoding="utf-8") as f:
                q = json.load(f)
        except (OSError, ValueError):
            q = []
        now = time.time()
        q = [w for w in q if now - w.get("seen", 0) <= GONE]
        q = fn(q)
        with open(QUEUE, "w", encoding="utf-8") as f:
            json.dump(q, f)
        return q
    finally:
        try:
            os.remove(QMTX)
        except OSError:
            pass


def _queue_touch(owner, note, front=False):
    def f(q):
        now = time.time()
        for w in q:
            if w["owner"] == owner:
                w["seen"] = now
                return q
        e = {"owner": owner, "at": now, "seen": now, "note": note}
        if front:
            q.insert(0, e)   # restarts only (the coordinator)
        else:
            q.append(e)
        return q
    return _qedit(f)


def _queue_leave(owner):
    _qedit(lambda q: [w for w in q if w["owner"] != owner])


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


def acquire(owner, wait, note, front=False):
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
        q = _queue_touch(owner, note, front)
        my_turn = bool(q) and q[0]["owner"] == owner
        if cur is None and not os.path.exists(LOCK) and my_turn:
            try:
                write({"owner": owner, "since": now, "beat": now, "note": note}, new=True)
                _queue_leave(owner)
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
            _queue_leave(owner)
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
        q = _qedit(lambda q: q)
        if q:
            print("queue: " + ", ".join("%s (%d s)" % (w["owner"], time.time() - w["at"]) for w in q))
        return 0
    cmd, owner = argv[0], (argv[1] if len(argv) > 1 else "")
    if not owner:
        print(__doc__)
        return 1
    if cmd == "acquire":
        wait = float(argv[argv.index("--wait") + 1]) if "--wait" in argv else 540.0
        note = argv[argv.index("--note") + 1] if "--note" in argv else ""
        return acquire(owner, wait, note, "--front" in argv)
    if cmd == "release":
        return release(owner, "--force" in argv)
    print(__doc__)
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
