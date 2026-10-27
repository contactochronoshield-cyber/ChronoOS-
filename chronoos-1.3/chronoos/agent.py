"""ChronoD: cola offline + sync autenticada (Ed25519) por HTTPS con pin o loopback; mantenimiento y hardening."""
import os, sys, json, time, sqlite3, signal, hmac, ssl, http.client
from urllib.parse import urlparse
from pathlib import Path
from chronoos.cli import HOME, LOG
from chronoos import identity

DB = HOME / "queue.db"
CFG = HOME / "config.json"
PID = HOME / "chronod.pid"
LOOPBACK = ("127.0.0.1", "localhost", "::1")

def _db():
    HOME.mkdir(parents=True, exist_ok=True)
    c = sqlite3.connect(DB)
    c.execute("CREATE TABLE IF NOT EXISTS events(id INTEGER PRIMARY KEY, ts INTEGER, kind TEXT, msg TEXT, sig TEXT, synced INTEGER DEFAULT 0)")
    return c

def add_event(kind, msg):
    ts = int(time.time()); c = _db()
    sig = identity.sign(f"{identity.node_id()}|{ts}|{kind}|{msg}")
    c.execute("INSERT INTO events(ts,kind,msg,sig) VALUES(?,?,?,?)", (ts, kind, msg, sig))
    c.commit(); c.close()

def pending():
    c = _db(); n = c.execute("SELECT COUNT(*) FROM events WHERE synced=0").fetchone()[0]
    t = c.execute("SELECT COUNT(*) FROM events").fetchone()[0]; c.close(); return n, t

def _cfg():
    return json.loads(CFG.read_text()) if CFG.exists() else {}

def _post(url, body, headers, cfg):
    u = urlparse(url); host = u.hostname or ""
    if u.scheme == "https":
        pin = (cfg.get("server_pin") or "").lower().replace(":", "")
        if not pin: raise ValueError("https requiere 'server_pin' en config.json (se obtiene con 'chrono server pin' en el servidor)")
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ctx.minimum_version = ssl.TLSVersion.TLSv1_2
        ctx.check_hostname = False; ctx.verify_mode = ssl.CERT_NONE   # la confianza la da el pin, no una CA
        conn = http.client.HTTPSConnection(host, u.port or 443, timeout=10, context=ctx)
        conn.connect()
        from chronoos.server import spki_pin
        if not hmac.compare_digest(spki_pin(conn.sock.getpeercert(binary_form=True)), pin):
            conn.close(); raise ValueError("el certificado del servidor NO coincide con server_pin (posible intermediario)")
    elif u.scheme == "http":
        if host not in LOOPBACK and not cfg.get("allow_insecure"):
            raise ValueError("HTTP sin cifrar no permitido fuera de localhost: usa https + server_pin")
        conn = http.client.HTTPConnection(host, u.port or 80, timeout=10)
    else:
        raise ValueError("sync_url debe empezar por http:// o https://")
    try:
        conn.request("POST", u.path or "/", body, headers)
        r = conn.getresponse()
        return r.status, r.read().decode(errors="replace")
    finally:
        conn.close()

def sync_once():
    cfg = _cfg(); url = cfg.get("sync_url")
    if not url: return "sin sync_url configurado (eventos quedan en cola local)"
    c = _db()
    rows = c.execute("SELECT id,ts,kind,msg,sig FROM events WHERE synced=0 ORDER BY id LIMIT 100").fetchall()
    if not rows: c.close(); return "nada pendiente"
    nid = identity.node_id()
    body = json.dumps({"node": nid, "events": rows}).encode()
    headers = {"Content-Type": "application/json", "X-Chrono-Node": nid, "X-Chrono-Sig": identity.sign(body)}
    try:
        status, text = _post(url, body, headers, cfg)
    except ValueError as e:
        c.close(); return f"conexion bloqueada por seguridad: {e}; eventos siguen en cola"
    except Exception as e:
        c.close(); return f"servidor inalcanzable, eventos en cola y se reintenta: {e}"
    if status == 200:
        c.executemany("UPDATE events SET synced=1 WHERE id=?", [(x[0],) for x in rows]); c.commit()
        c.close(); return f"sincronizados {len(rows)}"
    c.close(); return f"servidor rechazo (HTTP {status}): {text[:80]}; eventos siguen en cola"

def maintenance(keep_days=30, keep_log=1000):
    cutoff = int(time.time()) - keep_days * 86400
    c = _db(); n = c.execute("DELETE FROM events WHERE synced=1 AND ts<?", (cutoff,)).rowcount
    c.commit(); c.execute("VACUUM"); c.close()
    cut = 0
    if LOG.exists():
        lines = LOG.read_text().splitlines()
        if len(lines) > keep_log:
            LOG.write_text("\n".join(lines[-keep_log:]) + "\n"); cut = len(lines) - keep_log
    return f"mantenimiento: {n} eventos sincronizados antiguos eliminados, {cut} lineas de log recortadas"

def harden():
    HOME.mkdir(parents=True, exist_ok=True); os.chmod(HOME, 0o700); n = 0
    for p in HOME.iterdir():
        if p.name in ("releases", "current") or p.is_symlink(): continue
        try:
            if p.is_dir():
                os.chmod(p, 0o700)
                for f in p.iterdir():
                    if f.is_file() and not f.is_symlink(): os.chmod(f, 0o600); n += 1
            else:
                os.chmod(p, 0o600)
            n += 1
        except OSError: pass
    return f"permisos ajustados ({n} elementos): directorio 700, archivos 600"

def running():
    try: os.kill(int(PID.read_text()), 0); return True
    except (OSError, ValueError, FileNotFoundError): return False

def _loop():
    HOME.mkdir(parents=True, exist_ok=True)
    PID.write_text(str(os.getpid()))
    try: open("/proc/self/comm", "w").write("chronod")   # para que chronoctl lo encuentre
    except OSError: pass
    def _term(*a): PID.unlink(missing_ok=True); sys.exit(0)
    signal.signal(signal.SIGTERM, _term); signal.signal(signal.SIGINT, _term)
    last = 0
    while True:
        try: sync_once()
        except Exception: pass
        if time.time() - last > 86400:
            try: maintenance()
            except Exception: pass
            last = time.time()
        time.sleep(30)

def daemon(action):
    if action in ("start", "run"):
        if running(): print("ChronoD ya esta corriendo"); return
        if action == "start":
            if os.fork() > 0: print("ChronoD iniciado"); return
            os.setsid()
        _loop()
    elif action == "stop":
        if running(): os.kill(int(PID.read_text()), signal.SIGTERM); print("ChronoD detenido")
        else: PID.unlink(missing_ok=True); print("ChronoD no estaba corriendo")
    else: print("ChronoD:", "RUNNING" if running() else "STOPPED")

def register(sp):
    e = sp.add_parser("event"); e.add_argument("kind"); e.add_argument("msg")
    e.set_defaults(fn=lambda a: (add_event(a.kind, a.msg), print("evento guardado")))
    sp.add_parser("queue").set_defaults(fn=lambda a: print("pendientes: %d / total: %d  nodo: %s" % (*pending(), identity.node_id())))
    sp.add_parser("sync").set_defaults(fn=lambda a: print(sync_once()))
    d = sp.add_parser("daemon"); d.add_argument("action", choices=["start", "stop", "status", "run"])
    d.set_defaults(fn=lambda a: daemon(a.action))
    m = sp.add_parser("maintenance"); m.add_argument("--keep-days", type=int, default=30)
    m.set_defaults(fn=lambda a: print(maintenance(a.keep_days)))
    sp.add_parser("harden").set_defaults(fn=lambda a: print(harden()))
