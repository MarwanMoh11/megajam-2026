"""Preview server for the Rodin props: serves C:\\Jam\\Art\\Rodin and preview.html on 127.0.0.1, and saves the
contact sheets the page renders (POST /save/<prop>/<file>.png) next to each prop's GLB.

  "$PY" -I tools/rodin/preview.py [port]      # default 8961; then open http://127.0.0.1:8961/preview.html?p=podium,reel
"""
import http.server
import os
import re
import sys

OUT = r"C:\Jam\Art\Rodin"
HERE = os.path.dirname(os.path.abspath(__file__))


class H(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *a, **k):
        super().__init__(*a, directory=OUT, **k)

    def do_GET(self):
        if self.path.split("?")[0] == "/preview.html":
            body = open(os.path.join(HERE, "preview.html"), "rb").read()
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return
        super().do_GET()

    def do_POST(self):
        m = re.fullmatch(r"/save/([A-Za-z0-9_]+)/([A-Za-z0-9_]+\.png)", self.path)
        if not m or not os.path.isdir(os.path.join(OUT, m.group(1))):
            self.send_error(400)
            return
        n = int(self.headers.get("Content-Length", "0"))
        with open(os.path.join(OUT, m.group(1), m.group(2)), "wb") as f:
            f.write(self.rfile.read(n))
        self.send_response(204)
        self.end_headers()

    def log_message(self, *a):
        pass


if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8961
    http.server.ThreadingHTTPServer(("127.0.0.1", port), H).serve_forever()
