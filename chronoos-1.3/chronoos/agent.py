"""ChronoD (fase 3+4): cola de eventos offline + sync autenticada con Ed25519."""
import os, sys, json, time, sqlite3, signal, urllib.request, urllib.error
from pathlib import Path
from chronoos.cli import HOME, _net
from chronoos import identity

DB = HOME / "queue.db"
CFG = HOME / "config.json"
PID = HOME / "chronod.pid"

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

def sync_once():
    url = _cfg().get("sync_url")
    if not url: return "sin sync_url configurado (eventos quedan en cola local)"
    c = _db()
    rows = c.execute("SELECT id,ts,kind,msg,sig FROM events WHERE synced=0 ORDER BY id LIMIT 100").fetchall()
    if not rows: c.close(); return "nada pendiente"
    nid = identity.node_id()
    body = json.dumps({"node": nid, "events": rows}).encode()
    req = urllib.request.Request(url, body, {"Content-Type": "application/json", "X-Chrono-Node": nid, "X-Chrono-Sig": identity.sign(body)})
    try:
        with urllib.request.urlopen(req, timeout=10) as r:
            if r.status == 200:
                c.executemany("UPDATE events SET synced=1 WHERE id=?", [(x[0],) for x in rows]); c.commit()
                c.close(); return f"sincronizados {len(rows)}"
    except urllib.error.HTTPError as e:
        c.close(); return f"servidor rechazo (HTTP {e.code}): {e.read().decode()[:80]}; eventos siguen en cola"
    except Exception as e:
        c.close(); return f"servidor inalcanzable, eventos en cola y se reintenta: {e}"
    c.close(); return "respuesta inesperada del servidor"

def running():
    try: os.kill(int(PID.read_text()), 0); return True
    except (OSError, ValueError, FileNotFoundError): return False

def daemon(action):
    if action == "start":
        if running(): print("ChronoD ya esta corriendo"); return
        if os.fork() > 0: print("ChronoD iniciado"); return
        HOME.mkdir(parents=True, exist_ok=True)
        os.setsid(); PID.write_text(str(os.getpid()))
        try: open("/proc/self/comm", "w").write("chronod")
        except OSError: pass
        signal.signal(signal.SIGTERM, lambda *a: (PID.unlink(missing_ok=True), sys.exit(0)))
        while True:
            sync_once(); time.sleep(30)
    elif action == "stop":
        if running(): os.kill(int(PID.read_text()), signal.SIGTERM); print("ChronoD detenido")
        else: PID.unlink(missing_ok=True); print("ChronoD no estaba corriendo")
    else: print("ChronoD:", "RUNNING" if running() else "STOPPED")

def register(sp):
    e = sp.add_parser("event"); e.add_argument("kind"); e.add_argument("msg")
    e.set_defaults(fn=lambda a: (add_event(a.kind, a.msg), print("evento guardado")))
    sp.add_parser("queue").set_defaults(fn=lambda a: print("pendientes: %d / total: %d  nodo: %s" % (*pending(), identity.node_id())))
    sp.add_parser("sync").set_defaults(fn=lambda a: print(sync_once()))
    d = sp.add_parser("daemon"); d.add_argument("action", choices=["start", "stop", "status"])
    d.set_defaults(fn=lambda a: daemon(a.action))
