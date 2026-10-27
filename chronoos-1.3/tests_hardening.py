import subprocess, sys, os, tempfile, time, json, sqlite3, socket, re
CHRONO = os.path.join(os.path.dirname(os.path.abspath(__file__)), "chrono")
t = tempfile.mkdtemp(); nh, sh = t + "/node", t + "/srv"
def env(home=nh): return dict(os.environ, CHRONO_HOME=home, CHRONO_SERVER_HOME=sh)
def run(*a, home=nh): return subprocess.run([sys.executable, CHRONO, *a], capture_output=True, text=True, env=env(home))
def free_port():
    s = socket.socket(); s.bind(("127.0.0.1", 0)); p = s.getsockname()[1]; s.close(); return p
def cfg(**k): json.dump(k, open(nh + "/config.json", "w"))
procs = []
try:
    # HTTP expuesto fuera de localhost sin --tls: el servidor se niega
    r = run("server", "run", "--host", "0.0.0.0", "--port", "1")
    assert r.returncode != 0 and "negado" in r.stderr, (r.stdout, r.stderr)
    # servidor TLS + pin
    port = free_port()
    procs.append(subprocess.Popen([sys.executable, CHRONO, "server", "run", "--tls", "--port", str(port)], env=env(t + "/shome"), stdout=subprocess.DEVNULL))
    time.sleep(2)
    pin = run("server", "pin").stdout.strip(); assert re.fullmatch(r"[0-9a-f]{64}", pin), pin
    run("event", "a", "uno")
    info = run("node").stdout
    nid = re.search(r"id:\s+(\w+)", info).group(1); pub = re.search(r"pub:\s+(\w+)", info).group(1)
    run("server", "enroll", nid, pub)
    cfg(sync_url=f"https://127.0.0.1:{port}/", server_pin="0" * 64)          # pin incorrecto (intermediario)
    assert "NO coincide" in run("sync").stdout and "pendientes: 1" in run("queue").stdout
    cfg(sync_url=f"https://127.0.0.1:{port}/")                                 # https sin pin
    assert "requiere 'server_pin'" in run("sync").stdout
    cfg(sync_url=f"https://127.0.0.1:{port}/", server_pin=pin)                 # pin correcto
    assert "sincronizados 1" in run("sync").stdout
    assert "eventos: 1" in run("server", "list").stdout
    run("event", "b", "dos")
    cfg(sync_url="http://192.0.2.1:8787/")                                     # HTTP fuera de localhost
    assert "no permitido" in run("sync").stdout and "pendientes: 1" in run("queue").stdout
    # permisos: doctor detecta y harden corrige
    hh = t + "/h"; run("event", "x", "y", home=hh)
    os.chmod(hh + "/queue.db", 0o644)
    assert "[FAIL] claves y datos privados" in run("doctor", home=hh).stdout
    assert "permisos ajustados" in run("harden", home=hh).stdout
    assert "[OK]   claves y datos privados" in run("doctor", home=hh).stdout
    # retencion: borra sincronizados antiguos, conserva pendientes
    c = sqlite3.connect(nh + "/queue.db"); old = int(time.time()) - 90 * 86400
    c.execute("INSERT INTO events(ts,kind,msg,sig,synced) VALUES(?,?,?,?,1)", (old, "k", "m", "s")); c.commit(); c.close()
    assert "1 eventos sincronizados antiguos eliminados" in run("maintenance", "--keep-days", "30").stdout
    assert "pendientes: 1" in run("queue").stdout
    # daemon en primer plano (systemd): se llama chronod en /proc y limpia su PID
    cfg(sync_url="http://127.0.0.1:1/")
    d = subprocess.Popen([sys.executable, CHRONO, "daemon", "run"], env=env(nh)); procs.append(d)
    time.sleep(1.5)
    assert open(f"/proc/{d.pid}/comm").read().strip() == "chronod"
    assert "RUNNING" in run("daemon", "status").stdout
    d.terminate(); d.wait(5)
    assert not os.path.exists(nh + "/chronod.pid")
    print("HARDENING: TODAS LAS PRUEBAS OK")
finally:
    for p in procs:
        if p.poll() is None: p.terminate()
