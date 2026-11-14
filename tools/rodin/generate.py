"""Hyper3D Rodin text-to-3D for the hero props: submit, poll, download. Resumable: each prop's state lives in
C:\\Jam\\Art\\Rodin\\<prop>\\task.json, so a rerun picks up where the last one stopped.

Run with the engine's Python (stdlib only), from the repo root:
  PY="/c/Epic Games/UE_5.9/Engine/Binaries/ThirdParty/Python3/Win64/python.exe"
  "$PY" -I tools/rodin/generate.py balance
  "$PY" -I tools/rodin/generate.py submit podium pedestal ...   # refuses a prop already submitted unless --redo
  "$PY" -I tools/rodin/generate.py poll [props...] [--wait 300]  # downloads each prop when its jobs are Done
  "$PY" -I tools/rodin/generate.py info [props...]               # files and triangle counts of what is on disk

--redo moves the current take to <prop>_take<n> and submits again (a retake keeps the old files).
Prompts and request fields are in props.json next to this file. API: https://docs.hyper3d.ai (Rodin Gen-2.5).
The key is read from HYPER3D_API_KEY (process env, else HKCU\\Environment) and never printed or logged.
"""
import json
import os
import shutil
import struct
import sys
import time
import urllib.error
import urllib.request
import uuid

API = "https://api.hyper3d.com/api/v2"
HERE = os.path.dirname(os.path.abspath(__file__))
OUT = r"C:\Jam\Art\Rodin"
UA = "rodin-props-tool/1.0"


def api_key():
    k = os.environ.get("HYPER3D_API_KEY")
    if not k:
        import winreg
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, "Environment") as h:
            k, _ = winreg.QueryValueEx(h, "HYPER3D_API_KEY")
    if not k:
        sys.exit("HYPER3D_API_KEY is not set")
    return k.strip()


def call(method, path, data=None, headers=None, timeout=90):
    """(http status, parsed json). Errors come back as {"error": body}; the key never appears in output."""
    h = {"Authorization": "Bearer " + api_key(), "User-Agent": UA, "Accept": "application/json"}
    h.update(headers or {})
    req = urllib.request.Request(API + path, data=data, headers=h, method=method)
    for attempt in range(4):       # DNS and connection blips: nothing reached the server, so a retry is safe
        try:
            with urllib.request.urlopen(req, timeout=timeout) as r:
                raw = r.read().decode("utf-8")
                return (200 if r.status in (200, 201) else r.status), (json.loads(raw) if raw.strip() else {})
        except urllib.error.HTTPError as e:
            return e.code, {"error": e.read().decode("utf-8", "replace")[:600],
                            "retry_after": e.headers.get("Retry-After")}
        except urllib.error.URLError as e:
            if attempt == 3:
                return 0, {"error": "network: %s" % e.reason}
            time.sleep(5 * (attempt + 1))


def post_json(path, obj):
    return call("POST", path, json.dumps(obj).encode("utf-8"), {"Content-Type": "application/json"})


def multipart(fields):
    b = "----rodin" + uuid.uuid4().hex
    body = "".join('--%s\r\nContent-Disposition: form-data; name="%s"\r\n\r\n%s\r\n' % (b, k, v) for k, v in fields)
    body += "--%s--\r\n" % b
    return body.encode("utf-8"), "multipart/form-data; boundary=" + b


def spec():
    # RODIN_PROPS_FILE: another prop list with the same shape, so parallel agents each keep their own file
    path = os.environ.get("RODIN_PROPS_FILE") or os.path.join(HERE, "props.json")
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def task_path(name):
    return os.path.join(OUT, name, "task.json")


def load(name):
    p = task_path(name)
    if not os.path.exists(p):
        return None
    with open(p, encoding="utf-8") as f:
        return json.load(f)


def save(name, t):
    os.makedirs(os.path.join(OUT, name), exist_ok=True)
    with open(task_path(name), "w", encoding="utf-8") as f:
        json.dump(t, f, indent=2)


def balance():
    st, r = call("GET", "/check_balance")
    print("balance:", r.get("balance") if st == 200 else (st, r))
    return r.get("balance") if st == 200 else None


def submit(names, redo):
    s = spec()
    for name in names:
        if name not in s["props"]:
            print(name, ": not in props.json")
            continue
        if load(name):
            if not redo:
                print(name, ": already submitted (--redo for a retake)")
                continue
            n = 1
            while os.path.exists(os.path.join(OUT, "%s_take%d" % (name, n))):
                n += 1
            shutil.move(os.path.join(OUT, name), os.path.join(OUT, "%s_take%d" % (name, n)))
        prompt = s["props"][name] + ", " + s["style"]
        fields = [("prompt", prompt)] + [(k, v) for k, v in s["params"].items()]
        body, ctype = multipart(fields)
        st, r = call("POST", "/rodin", body, {"Content-Type": ctype})
        if st != 200 or not r.get("uuid") or r.get("error"):
            print(name, ": submit failed", st, r)
            continue
        save(name, {"prop": name, "prompt": prompt, "params": s["params"], "uuid": r["uuid"],
                    "subscription_key": r["jobs"]["subscription_key"], "consumed": r.get("consumed"),
                    "submitted": time.strftime("%Y-%m-%d %H:%M:%S"), "state": "submitted"})
        print(name, ": submitted, consumed", r.get("consumed"))


def fetch(url, path, tries=4):
    """Downloads url to path, retrying DNS and connection blips (this PC's DNS drops now and then)."""
    for attempt in range(tries):
        try:
            req = urllib.request.Request(url, headers={"User-Agent": UA})
            with urllib.request.urlopen(req, timeout=300) as src, open(path, "wb") as dst:
                shutil.copyfileobj(src, dst)
            return True
        except (urllib.error.URLError, OSError):
            time.sleep(5 * (attempt + 1))
    return False


def download(name, t):
    st, r = post_json("/download", {"task_uuid": t["uuid"]})
    if st != 200 or "list" not in r:
        print(name, ": download list failed", st, r)
        return False
    d = os.path.join(OUT, name)
    files = []
    for item in r["list"]:
        fn = os.path.basename(item["name"].replace("\\", "/")) or "file"
        if not fetch(item["url"], os.path.join(d, fn)):
            print(name, ": download of", fn, "failed; poll again later")
            return False
        files.append(fn)
    t["files"], t["state"], t["downloaded"] = files, "done", time.strftime("%Y-%m-%d %H:%M:%S")
    save(name, t)
    return True


def poll(names, wait):
    names = names or sorted(n for n in os.listdir(OUT) if load(n)) if os.path.isdir(OUT) else []
    pending = [n for n in names if (load(n) or {}).get("state") == "submitted"]
    end = time.time() + wait
    while pending:
        for n in list(pending):
            t = load(n)
            st, r = post_json("/status", {"subscription_key": t["subscription_key"]})
            if st == 429:
                time.sleep(float(r.get("retry_after") or 10))
                continue
            jobs = r.get("jobs") or []
            states = [j.get("status") for j in jobs]
            if jobs and all(s == "Done" for s in states):
                if download(n, t):
                    print(n, ": done ->", ", ".join(load(n)["files"]))
                    info([n])
                pending.remove(n)
            elif any(s == "Failed" for s in states):
                t["state"] = "failed"
                save(n, t)
                print(n, ": FAILED", r)
                pending.remove(n)
            elif st != 200:
                print(n, ": status error", st, r)
        if not pending or time.time() >= end:
            break
        time.sleep(10)
    for n in pending:
        print(n, ": still running")


def image_size(b):
    """(w, h) of PNG or JPEG bytes, else None."""
    if b[:4] == bytes((0x89, 0x50, 0x4E, 0x47)):
        return struct.unpack(">II", b[16:24])
    if b[:2] == bytes((0xFF, 0xD8)):
        i = 2
        while i + 9 < len(b):
            if b[i] != 0xFF:
                i += 1
                continue
            m, ln = b[i + 1], struct.unpack(">H", b[i + 2:i + 4])[0]
            if m in (0xC0, 0xC1, 0xC2):
                h, w = struct.unpack(">HH", b[i + 5:i + 9])
                return w, h
            i += 2 + ln
    return None


def glb_textures(path):
    """Sizes of the images embedded in a .glb, e.g. ['2048x2048', ...]."""
    with open(path, "rb") as f:
        data = f.read()
    ln = struct.unpack("<I", data[12:16])[0]
    g = json.loads(data[20:20 + ln].decode("utf-8"))
    bin0 = 20 + ln + 8
    out = []
    for im in g.get("images", []):
        bv = g["bufferViews"][im["bufferView"]] if "bufferView" in im else None
        if bv:
            o = bin0 + bv.get("byteOffset", 0)
            sz = image_size(data[o:o + bv["byteLength"]])
            out.append("%dx%d" % sz if sz else "?")
    return out


def glb_triangles(path):
    """Triangle count of a .glb from its JSON chunk (indexed or not, triangle-list primitives)."""
    with open(path, "rb") as f:
        magic, _, _ = struct.unpack("<4sII", f.read(12))
        if magic != b"glTF":
            return None
        ln, kind = struct.unpack("<I4s", f.read(8))
        g = json.loads(f.read(ln).decode("utf-8"))
    tris = 0
    for m in g.get("meshes", []):
        for p in m.get("primitives", []):
            if p.get("mode", 4) != 4:
                continue
            acc = p.get("indices", p["attributes"].get("POSITION"))
            tris += g["accessors"][acc]["count"] // 3
    return tris


def info(names):
    names = names or (sorted(os.listdir(OUT)) if os.path.isdir(OUT) else [])
    for n in names:
        d = os.path.join(OUT, n)
        if not os.path.isdir(d):
            continue
        t = load(n) or {}
        parts = []
        for fn in sorted(os.listdir(d)):
            if fn == "task.json":
                continue
            p = os.path.join(d, fn)
            tri = glb_triangles(p) if fn.lower().endswith(".glb") else None
            tex = " textures " + ",".join(glb_textures(p)) if fn.lower().endswith(".glb") else ""
            parts.append("%s %.1f MB%s%s" % (fn, os.path.getsize(p) / 1e6, " %d tris" % tri if tri else "", tex))
        print(n, "[%s, consumed %s]:" % (t.get("state"), t.get("consumed")), "; ".join(parts))


def main(argv):
    if not argv:
        sys.exit(__doc__)
    cmd, rest = argv[0], argv[1:]
    redo = "--redo" in rest
    wait = 0.0
    if "--wait" in rest:
        wait = float(rest[rest.index("--wait") + 1])
        rest = rest[:rest.index("--wait")] + rest[rest.index("--wait") + 2:]
    names = [a for a in rest if not a.startswith("--")]
    if cmd == "balance":
        balance()
    elif cmd == "submit":
        submit(names, redo)
    elif cmd == "poll":
        poll(names, wait)
    elif cmd == "info":
        info(names)
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main(sys.argv[1:])
