"""ChronoOS server: Ed25519 por nodo, TLS con pin, deduplicacion; solo claves publicas."""
import os, json, hashlib, sqlite3, ssl, http.server
from pathlib import Path
from chronoos.cli import HOME
from chronoos import identity

SHOME = Path(os.environ.get("CHRONO_SERVER_HOME", HOME / "server"))
NODES = SHOME / "nodes.json"
SDB = SHOME / "server.db"
TLSD = SHOME / "tls"
LOOPBACK = ("127.0.0.1", "localhost", "::1")

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

# ---------- TLS con pin de clave publica (SPKI) ----------
def spki_pin(der):
    from cryptography import x509
    from cryptography.hazmat.primitives import serialization as S
    spki = x509.load_der_x509_certificate(der).public_key().public_bytes(S.Encoding.DER, S.PublicFormat.SubjectPublicKeyInfo)
    return hashlib.sha256(spki).hexdigest()

def current_pin():
    from cryptography import x509
    from cryptography.hazmat.primitives import serialization as S
    crt = TLSD / "server.crt"
    if not crt.exists(): raise SystemExit("sin certificado TLS: ejecuta 'chrono server run --tls' o 'chrono server init-tls'")
    return spki_pin(x509.load_pem_x509_certificate(crt.read_bytes()).public_bytes(S.Encoding.DER))

def init_tls(sans=(), rotate_key=False):
    import datetime, ipaddress, socket
    from cryptography import x509
    from cryptography.x509.oid import NameOID
    from cryptography.hazmat.primitives import hashes, serialization as S
    from cryptography.hazmat.primitives.asymmetric import ec
    TLSD.mkdir(parents=True, exist_ok=True); os.chmod(TLSD, 0o700)
    kp = TLSD / "server.key"
    key = S.load_pem_private_key(kp.read_bytes(), None) if (kp.exists() and not rotate_key) else ec.generate_private_key(ec.SECP256R1())
    fd = os.open(kp, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    with os.fdopen(fd, "wb") as f:
        f.write(key.private_bytes(S.Encoding.PEM, S.PrivateFormat.PKCS8, S.NoEncryption()))
    name = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, "chronoos-server")])
    alt = [x509.DNSName("localhost"), x509.IPAddress(ipaddress.ip_address("127.0.0.1"))]
    for s in list(sans) + [socket.gethostname()]:
        try: alt.append(x509.IPAddress(ipaddress.ip_address(s)))
        except ValueError: alt.append(x509.DNSName(s))
    now = datetime.datetime.now(datetime.timezone.utc)
    cert = (x509.CertificateBuilder().subject_name(name).issuer_name(name).public_key(key.public_key())
            .serial_number(x509.random_serial_number()).not_valid_before(now - datetime.timedelta(minutes=5))
            .not_valid_after(now + datetime.timedelta(days=365))
            .add_extension(x509.SubjectAlternativeName(alt), critical=False).sign(key, hashes.SHA256()))
    (TLSD / "server.crt").write_bytes(cert.public_bytes(S.Encoding.PEM))
    return current_pin()

class H(http.server.BaseHTTPRequestHandler):
    timeout = 10
    def _r(self, code, msg=""):
        self.send_response(code); self.end_headers(); self.wfile.write(msg.encode())
    def do_POST(self):
        try: n = int(self.headers.get("Content-Length", 0))
        except ValueError: return self._r(400, "content-length invalido")
        if n < 0 or n > 1_000_000: return self._r(413, "tamano no permitido")
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

def run(host, port, tls=False, sans=(), insecure=False):
    if not tls and host not in LOOPBACK and not insecure:
        raise SystemExit("negado: HTTP sin cifrar fuera de localhost. Usa --tls (o --insecure-http bajo tu responsabilidad)")
    httpd = http.server.ThreadingHTTPServer((host, port), H)
    if tls:
        if not (TLSD / "server.crt").exists(): init_tls(sans)
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER); ctx.minimum_version = ssl.TLSVersion.TLSv1_2
        ctx.load_cert_chain(TLSD / "server.crt", TLSD / "server.key")
        httpd.socket = ctx.wrap_socket(httpd.socket, server_side=True, do_handshake_on_connect=False)
        print(f"ChronoOS server TLS en https://{host}:{port}\npin: {current_pin()}", flush=True)
    else:
        print(f"ChronoOS server en http://{host}:{port}", flush=True)
    httpd.serve_forever()

def listing():
    c = _sdb()
    rows = c.execute("SELECT node, COUNT(*), MAX(ts) FROM events GROUP BY node").fetchall(); c.close()
    print("nodos registrados:", len(_nodes()))
    for nid, n, ts in rows: print(f"  {nid}  eventos: {n}  ultimo: {ts}")

def register(sp):
    ss = sp.add_parser("server").add_subparsers(dest="sub", required=True)
    r = ss.add_parser("run"); r.add_argument("--host", default="127.0.0.1"); r.add_argument("--port", type=int, default=8787)
    r.add_argument("--tls", action="store_true"); r.add_argument("--san", action="append", default=[])
    r.add_argument("--insecure-http", action="store_true")
    r.set_defaults(fn=lambda a: run(a.host, a.port, a.tls, a.san, a.insecure_http))
    t = ss.add_parser("init-tls"); t.add_argument("--san", action="append", default=[]); t.add_argument("--rotate-key", action="store_true")
    t.set_defaults(fn=lambda a: print("pin:", init_tls(a.san, a.rotate_key)))
    ss.add_parser("pin").set_defaults(fn=lambda a: print(current_pin()))
    e = ss.add_parser("enroll"); e.add_argument("node_id"); e.add_argument("pub")
    e.set_defaults(fn=lambda a: (enroll(a.node_id, a.pub), print("nodo registrado")))
    v = ss.add_parser("revoke"); v.add_argument("node_id"); v.set_defaults(fn=lambda a: revoke(a.node_id))
    ss.add_parser("list").set_defaults(fn=lambda a: listing())
