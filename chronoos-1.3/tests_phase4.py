import subprocess, sys, os, tempfile, time, json, sqlite3, socket, re, tarfile, io
CHRONO = os.path.join(os.path.dirname(os.path.abspath(__file__)), "chrono")
t = tempfile.mkdtemp(); nh, sh = t + "/node", t + "/srv"
def run(*a, home=nh):
    e = dict(os.environ, CHRONO_HOME=home, CHRONO_SERVER_HOME=sh)
    return subprocess.run([sys.executable, CHRONO, *a], capture_output=True, text=True, env=e)
def ok(r, s=""): assert r.returncode == 0 and s in r.stdout, (r.stdout, r.stderr)
def bad(r, s): assert r.returncode != 0 and s in r.stderr, (r.stdout, r.stderr)
s = socket.socket(); s.bind(("127.0.0.1", 0)); port = s.getsockname()[1]; s.close()
srv = subprocess.Popen([sys.executable, CHRONO, "server", "run", "--port", str(port)],
        env=dict(os.environ, CHRONO_HOME=t + "/shome", CHRONO_SERVER_HOME=sh), stdout=subprocess.DEVNULL)
time.sleep(1)
try:
    # --- identidad y sync ---
    run("event", "a", "uno"); run("event", "b", "dos")
    json.dump({"sync_url": f"http://127.0.0.1:{port}/"}, open(nh + "/config.json", "w"))
    assert oct(os.stat(nh + "/node.ed25519").st_mode)[-3:] == "600"
    assert "rechazo (HTTP 401)" in run("sync").stdout
    info = run("node").stdout
    nid = re.search(r"id:\s+(\w+)", info).group(1); pub = re.search(r"pub:\s+(\w+)", info).group(1)
    bad(run("server", "enroll", "0" * 16, pub), "no coinciden")
    ok(run("server", "enroll", nid, pub), "registrado")
    ok(run("sync"), "sincronizados 2"); ok(run("queue"), "pendientes: 0")
    # replay
    c = sqlite3.connect(nh + "/queue.db"); c.execute("UPDATE events SET synced=0"); c.commit(); c.close()
    ok(run("sync"), "sincronizados 2")
    assert sqlite3.connect(sh + "/server.db").execute("SELECT COUNT(*) FROM events").fetchone()[0] == 2
    # evento alterado tras firmar
    c = sqlite3.connect(nh + "/queue.db"); c.execute("UPDATE events SET synced=0, msg='FALSIFICADO' WHERE id=1"); c.commit(); c.close()
    assert "HTTP 400" in run("sync").stdout
    # el servidor NO guarda secretos: solo claves publicas
    assert json.load(open(sh + "/nodes.json"))[nid] == pub
    # revocacion
    c = sqlite3.connect(nh + "/queue.db"); c.execute("UPDATE events SET synced=0, msg='uno' WHERE id=1"); c.commit(); c.close()
    ok(run("server", "revoke", nid), "revocado")
    assert "HTTP 401" in run("sync").stdout
    # --- updates firmados ---
    pk = t + "/pub"; ok(run("publisher", "keygen", pk), "publicador creado")
    other = t + "/other"; run("publisher", "keygen", other)
    def pkg(name, ver, evil=False):
        f = f"{t}/{name}.tar.gz"
        with tarfile.open(f, "w:gz") as tf:
            for fn, data in [("manifest.json", json.dumps({"version": ver}).encode()), ("chrono.txt", b"codigo " + ver.encode())] + ([("../evil", b"x")] if evil else []):
                ti = tarfile.TarInfo(fn); ti.size = len(data); tf.addfile(ti, io.BytesIO(data))
        return f
    p1 = pkg("u131", "1.3.1"); p2 = pkg("u132", "1.3.2"); p0 = pkg("u130", "1.3.0"); pe = pkg("uevil", "1.3.9", True)
    bad(run("update", "apply", p1), "sin publicador")
    ok(run("update", "trust", open(pk + "/publisher.pub").read()))
    bad(run("update", "apply", p1), "sin firma")                                  # sin .sig
    for p in (p1, p2, p0, pe): ok(run("update", "sign", p, "--key", pk + "/publisher.key"))
    ok(run("update", "verify", p1), "firma valida")
    # firma de otro publicador
    q = pkg("ufake", "1.4.0"); run("update", "sign", q, "--key", other + "/publisher.key")
    bad(run("update", "apply", q), "FIRMA INVALIDA")
    # archivo alterado despues de firmar
    open(p1, "ab").write(b"x"); bad(run("update", "apply", p1), "FIRMA INVALIDA")
    p1 = pkg("u131", "1.3.1"); run("update", "sign", p1, "--key", pk + "/publisher.key")
    ok(run("update", "apply", p1), "verificada"); ok(run("update", "status"), "1.3.1")
    bad(run("update", "apply", p0), "no es mayor")                                 # anti-downgrade
    bad(run("update", "apply", pe), "inseguro")                                    # path traversal firmado
    ok(run("update", "apply", p2), "1.3.2")
    ok(run("update", "rollback"), "1.3.1"); ok(run("update", "status"), "1.3.1")  # recuperacion
    bad(run("update", "rollback"), "no hay update previa")                         # un solo nivel, falla limpio
    h2 = t + "/node2"                                                              # rollback a la version base
    run("update", "trust", open(pk + "/publisher.pub").read(), home=h2)
    ok(run("update", "apply", p1, home=h2), "verificada"); ok(run("update", "rollback", home=h2), "1.3.0-dev")
    # eventos de update quedan registrados
    assert "rechazada" in run("logs", "-n", "30").stdout
    print("FASE 4: TODAS LAS PRUEBAS OK")
finally:
    srv.terminate()
