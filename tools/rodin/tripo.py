"""Tripo text-to-3D for the repeated filler props: balance, submit, poll (downloads the model and Tripo's own render).
Same layout and rules as generate.py: state in C:\\Jam\\Art\\Tripo\\<prop>\\task.json, prompts in tripo.json.

  "$PY" -I tools/rodin/tripo.py balance
  "$PY" -I tools/rodin/tripo.py submit palm            # refuses a prop already submitted unless --redo
  "$PY" -I tools/rodin/tripo.py poll [props...] [--wait 300]
  "$PY" -I tools/rodin/tripo.py info [props...]

API: https://developers.tripo3d.ai (v3). The key is read from TRIPO_API_KEY (process env, else HKCU\\Environment) and
never printed or logged.
"""
import json
import os
import shutil
import sys
import time
import urllib.error
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)                      # -I leaves the script's folder off sys.path
import generate as rodin                      # noqa: E402  (glb stats and the retrying fetch)

API = "https://openapi.tripo3d.ai/v3"
OUT = r"C:\Jam\Art\Tripo"


def api_key():
    k = os.environ.get("TRIPO_API_KEY")
    if not k:
        import winreg
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, "Environment") as h:
            k, _ = winreg.QueryValueEx(h, "TRIPO_API_KEY")
    if not k:
        sys.exit("TRIPO_API_KEY is not set")
    return k.strip()


def call(method, path, obj=None, timeout=90):
    h = {"Authorization": "Bearer " + api_key(), "User-Agent": rodin.UA, "Content-Type": "application/json"}
    data = json.dumps(obj).encode("utf-8") if obj is not None else None
    req = urllib.request.Request(API + path, data=data, headers=h, method=method)
    for attempt in range(4):
        try:
            with urllib.request.urlopen(req, timeout=timeout) as r:
                return r.status, json.loads(r.read().decode("utf-8") or "{}")
        except urllib.error.HTTPError as e:
            return e.code, {"error": e.read().decode("utf-8", "replace")[:600]}
        except urllib.error.URLError as e:
            if attempt == 3:
                return 0, {"error": "network: %s" % e.reason}
            time.sleep(5 * (attempt + 1))


def spec():
    with open(os.path.join(HERE, "tripo.json"), encoding="utf-8") as f:
        return json.load(f)


def load(name):
    p = os.path.join(OUT, name, "task.json")
    return json.load(open(p, encoding="utf-8")) if os.path.exists(p) else None


def save(name, t):
    os.makedirs(os.path.join(OUT, name), exist_ok=True)
    json.dump(t, open(os.path.join(OUT, name, "task.json"), "w", encoding="utf-8"), indent=2)


def balance():
    st, r = call("GET", "/account/balance")
    d = r.get("data") or {}
    print("balance:", d.get("balance"), "frozen:", d.get("frozen")) if st == 200 else print("balance:", st, r)


def submit(names, redo):
    s = spec()
    for name in names:
        if name not in s["props"]:
            print(name, ": not in tripo.json")
            continue
        if load(name):
            if not redo:
                print(name, ": already submitted (--redo for a retake)")
                continue
            n = 1
            while os.path.exists(os.path.join(OUT, "%s_take%d" % (name, n))):
                n += 1
            shutil.move(os.path.join(OUT, name), os.path.join(OUT, "%s_take%d" % (name, n)))
        body = dict(s["params"], prompt=s["props"][name] + ", " + s["style"])
        st, r = call("POST", "/generation/text-to-model", body)
        tid = (r.get("data") or {}).get("task_id")
        if st != 200 or not tid:
            print(name, ": submit failed", st, r)
            continue
        save(name, {"prop": name, "request": body, "task_id": tid, "state": "submitted",
                    "submitted": time.strftime("%Y-%m-%d %H:%M:%S")})
        print(name, ": submitted")


def poll(names, wait):
    names = names or (sorted(n for n in os.listdir(OUT) if load(n)) if os.path.isdir(OUT) else [])
    pending = [n for n in names if (load(n) or {}).get("state") == "submitted"]
    end = time.time() + wait
    while pending:
        for n in list(pending):
            t = load(n)
            st, r = call("GET", "/tasks/" + t["task_id"])
            d = r.get("data") or {}
            status = d.get("status")
            if status == "success":
                out, files = d.get("output") or {}, []
                for key, url in out.items():
                    if isinstance(url, str) and url.startswith("http"):
                        ext = os.path.splitext(url.split("?")[0])[1] or ".bin"
                        fn = key.replace("_url", "") + ext
                        if rodin.fetch(url, os.path.join(OUT, n, fn)):
                            files.append(fn)
                t.update(state="done", files=files, credits=d.get("credits_consumed"),
                         downloaded=time.strftime("%Y-%m-%d %H:%M:%S"))
                save(n, t)
                pending.remove(n)
                info([n])
            elif status in ("failed", "cancelled", "banned", "expired"):
                t.update(state="failed", error=d.get("error_message") or r)
                save(n, t)
                pending.remove(n)
                print(n, ": FAILED", t["error"])
            elif st != 200:
                print(n, ": status error", st, r)
        if not pending or time.time() >= end:
            break
        time.sleep(10)
    for n in pending:
        print(n, ": still running")


def info(names):
    names = names or (sorted(os.listdir(OUT)) if os.path.isdir(OUT) else [])
    for n in names:
        d = os.path.join(OUT, n)
        if not os.path.isdir(d):
            continue
        t = load(n) or {}
        parts = []
        for fn in sorted(os.listdir(d)):
            p = os.path.join(d, fn)
            if fn == "task.json":
                continue
            glb = fn.lower().endswith(".glb")
            tri = rodin.glb_triangles(p) if glb else None
            tex = " textures " + ",".join(rodin.glb_textures(p)) if glb else ""
            parts.append("%s %.1f MB%s%s" % (fn, os.path.getsize(p) / 1e6, " %d tris" % tri if tri else "", tex))
        print(n, "[%s, credits %s]:" % (t.get("state"), t.get("credits")), "; ".join(parts))


if __name__ == "__main__":
    a = sys.argv[1:]
    if not a:
        sys.exit(__doc__)
    wait = float(a[a.index("--wait") + 1]) if "--wait" in a else 0.0
    if "--wait" in a:
        del a[a.index("--wait"):a.index("--wait") + 2]
    names = [x for x in a[1:] if not x.startswith("--")]
    {"balance": lambda: balance(), "submit": lambda: submit(names, "--redo" in a),
     "poll": lambda: poll(names, wait), "info": lambda: info(names)}.get(a[0], lambda: sys.exit(__doc__))()
