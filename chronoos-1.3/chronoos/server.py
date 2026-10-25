"""ChronoOS server (fase 4): solo guarda claves PUBLICAS; verifica Ed25519, deduplica y guarda."""
import os, json, hashlib, sqlite3, http.server
from pathlib import Path
from chronoos.cli import HOME
from chronoos import identity

SHOME = Path(os.environ.get("CHRONO_SERVER_HOME", HOME / "server"))
NODES = SHOME / "nodes.json"
SDB = SHOME / "server.db"

def _sdb():
    SHOME.mkdir(parents=True, exist_ok=True)
    c = sqlite3.connect(SDB)
    c.execute("CREATE TABLE IF NOT EXISTS events(node TEXT, eid INTEGER, ts INTEGER, kind TEXT, msg TEXT, sig TEXT, PRIMARY KEY(node,eid))")
    return c

def _nodes(): return json.loads(NODES.read_text()) if NODES.exists() else {}
def _save(n): SHOME.mkdir(parents=True, exist_ok=True); NODES.write_text(json.dumps(n)); os.chmod(NODES, 0o600)

def enroll(nid, pub):
    try: raw = bytes.fromhex(pub)
    except ValueError: raise SystemExit("clave publica invalida")
    if len(raw) != 32: raise SystemExit("clave publica invalida (32 bytes)")
    if hashlib.sha256(raw).hexdigest()[:16] != nid: raise SystemExit("ID y clave no coinciden: revisa que copiaste bien ambos")
    n = _nodes(); n[nid] = pub; _save(n)

def revoke(nid):
    n = _nodes()
    if nid not in n: raise SystemExit("nodo no registrado")
    del n[nid]; _save(n); print("nodo revocado")

class H(http.server.BaseHTTPRequestHandler):
    def _r(self, code, msg=""):
        self.send_response(code); self.end_headers(); self.wfile.write(msg.encode())
    def do_POST(self):
        n = int(self.headers.get("Content-Length", 0))
        if n > 1_000_000: return self._r(413, "demasiado grande")
        body = self.rfile.read(n)
        nid = self.headers.get("X-Chrono-Node", "")
        pub = _nodes().get(nid)
        if not pub: return self._r(401, "nodo no registrado")
        if not identity.verify(pub, body, self.headers.get("X-Chrono-Sig", "")): return self._r(401, "firma invalida")
        try:
            data = json.loads(body)
            assert data["node"] == nid
            rows = data["events"]
            for eid, ts, kind, msg, sig in rows:
                if not identity.verify(pub, f"{nid}|{ts}|{kind}|{msg}", sig): raise ValueError("evento alterado")
        except Exception as e:
            return self._r(400, f"lote invalido: {e}")
        c = _sdb(); before = c.total_changes
        c.executemany("INSERT OR IGNORE INTO events VALUES(?,?,?,?,?,?)", [(nid, *r) for r in rows])
        c.commit(); new = c.total_changes - before; c.close()
        self._r(200, json.dumps({"received": len(rows), "stored": new}))
    def log_message(self, *a): pass

def run(host, port):
    print(f"ChronoOS server en http://{host}:{port}  (Ctrl+C para parar)")
    http.server.ThreadingHTTPServer((host, port), H).serve_forever()

def listing():
    c = _sdb()
    rows = c.execute("SELECT node, COUNT(*), MAX(ts) FROM events GROUP BY node").fetchall(); c.close()
    print("nodos registrados:", len(_nodes()))
    for nid, n, ts in rows: print(f"  {nid}  eventos: {n}  ultimo: {ts}")

def register(sp):
    ss = sp.add_parser("server").add_subparsers(dest="sub", required=True)
    r = ss.add_parser("run"); r.add_argument("--host", default="127.0.0.1"); r.add_argument("--port", type=int, default=8787)
    r.set_defaults(fn=lambda a: run(a.host, a.port))
    e = ss.add_parser("enroll"); e.add_argument("node_id"); e.add_argument("pub")
    e.set_defaults(fn=lambda a: (enroll(a.node_id, a.pub), print("nodo registrado")))
    v = ss.add_parser("revoke"); v.add_argument("node_id"); v.set_defaults(fn=lambda a: revoke(a.node_id))
    ss.add_parser("list").set_defaults(fn=lambda a: listing())
