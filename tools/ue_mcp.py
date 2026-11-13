"""Talk to the Unreal editor's MCP server over HTTP, without the session's own MCP connection.

Use it when the `unreal-mcp` tools are missing from a Claude Code session (the editor was started or restarted
after the session began). Run it with the engine's bundled Python:

  PY="/c/Epic Games/UE_5.9/Engine/Binaries/ThirdParty/Python3/Win64/python.exe"
  "$PY" -I tools/ue_mcp.py list                                   # the 4 top-level tools
  "$PY" -I tools/ue_mcp.py toolsets                               # one line per toolset
  "$PY" -I tools/ue_mcp.py describe EditorToolset.EditorAppToolset.StartPIE
  "$PY" -I tools/ue_mcp.py tool EditorToolset.EditorAppToolset.StartPIE '{}'
  "$PY" -I tools/ue_mcp.py py script.py                           # run a Python file inside the editor

Images in a result are saved as shot_<time>_<n>.png in the current folder and their paths printed.
An optional last argument is the timeout in seconds (default 110).
"""
import base64, json, os, sys, time, urllib.request

URL = "http://127.0.0.1:8000/mcp"


def post(body, sid=None, timeout=110):
    req = urllib.request.Request(URL, data=json.dumps(body).encode(), method="POST")
    req.add_header("Content-Type", "application/json")
    req.add_header("Accept", "application/json, text/event-stream")
    if sid:
        req.add_header("Mcp-Session-Id", sid)
    with urllib.request.urlopen(req, timeout=timeout) as r:
        sid = r.headers.get("Mcp-Session-Id") or sid
        raw = r.read().decode("utf-8", "replace")
    if not raw.strip():
        return None, sid
    if raw.lstrip().startswith("{"):
        return json.loads(raw), sid
    msg = None
    for line in raw.splitlines():  # server-sent events: keep the last result
        if line.startswith("data:"):
            try:
                m = json.loads(line[5:].strip())
            except ValueError:
                continue
            if "result" in m or "error" in m:
                msg = m
    return msg, sid


def session():
    _, sid = post({"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {
        "protocolVersion": "2025-06-18", "capabilities": {},
        "clientInfo": {"name": "ue_mcp.py", "version": "1"}}})
    post({"jsonrpc": "2.0", "method": "notifications/initialized"}, sid)
    return sid


def call(sid, name, args, timeout=110):
    res, _ = post({"jsonrpc": "2.0", "id": 2, "method": "tools/call",
                   "params": {"name": name, "arguments": args}}, sid, timeout)
    return res


def show(res):
    if res is None:
        print("(no response)")
        return
    if "error" in res:
        print("ERROR", json.dumps(res["error"]))
        return
    r = res["result"]
    for i, c in enumerate(r.get("content", [])):
        if c.get("type") == "text":
            print(c["text"])
        elif c.get("type") == "image":
            ext = c.get("mimeType", "image/png").split("/")[-1]
            path = os.path.abspath("shot_%d_%d.%s" % (int(time.time()), i, ext))
            with open(path, "wb") as f:
                f.write(base64.b64decode(c["data"]))
            print("[image saved: %s]" % path)
        else:
            print("[%s content]" % c.get("type"))
    if r.get("isError"):
        print("(isError)")


def toolset_call(sid, full_name, args, timeout):
    toolset, tool = full_name.rsplit(".", 1)
    return call(sid, "call_tool", {"toolset_name": toolset, "tool_name": tool, "arguments": args}, timeout)


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    a = [x.strip() for x in sys.argv[1:]]
    cmd = a[0]
    sid = session()
    if cmd == "list":
        res, _ = post({"jsonrpc": "2.0", "id": 2, "method": "tools/list"}, sid)
        for t in res["result"]["tools"]:
            print(t["name"], "-", (t.get("description") or "").split("\n")[0])
    elif cmd == "toolsets":
        res = call(sid, "list_toolsets", {})
        text = "".join(c.get("text", "") for c in res["result"]["content"])
        for line in text.splitlines():
            if line.startswith("- ") and ": " in line and "." in line.split(":")[0]:
                print(line[:140])
    elif cmd == "describe":
        show(call(sid, "describe_tools", {"tool_names": a[1:]}))
    elif cmd == "tool":
        args = json.loads(a[2]) if len(a) > 2 else {}
        timeout = float(a[3]) if len(a) > 3 else 110
        show(toolset_call(sid, a[1], args, timeout))
    elif cmd == "py":
        with open(a[1], encoding="utf-8") as f:
            src = f.read()
        timeout = float(a[2]) if len(a) > 2 else 110
        show(toolset_call(sid, "editor_toolset.toolsets.python_scripting.PythonScriptingTools.execute_python",
                          {"script": {"content": src, "language": "Python"}}, timeout))
    else:
        print(__doc__)


if __name__ == "__main__":
    main()
