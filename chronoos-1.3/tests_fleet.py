import subprocess, sys, os, tempfile, time, json, sqlite3, csv
CHRONO = os.path.join(os.path.dirname(os.path.abspath(__file__)), "chrono")
t = tempfile.mkdtemp(); sh = t + "/srv"; os.makedirs(sh)
env = dict(os.environ, CHRONO_HOME=t + "/h", CHRONO_SERVER_HOME=sh)
def run(*a): return subprocess.run([sys.executable, CHRONO, *a], capture_output=True, text=True, env=env)
N = 1000; now = int(time.time())
nodes = {f"{i:016x}": "00" * 32 for i in range(N)}                    # 1000 registered nodes
json.dump(nodes, open(sh + "/nodes.json", "w"))
db = sqlite3.connect(sh + "/server.db")
db.execute("CREATE TABLE events(node TEXT, eid INTEGER, ts INTEGER, kind TEXT, msg TEXT, sig TEXT, PRIMARY KEY(node,eid))")
rows = []
for i in range(N):
    nid = f"{i:016x}"
    if i < 700:   rows += [(nid, k, now - 60 * k, "k", f"m{k}", "s") for k in range(3)]       # active
    elif i < 900: rows += [(nid, 0, now - 7200, "k", "old", "s")]                              # silent
    # 900..999: no events
rows += [("ffff000000000000", 0, now - 10, "k", "gone", "s")]                                  # revoked: events but not registered
db.executemany("INSERT INTO events VALUES(?,?,?,?,?,?)", rows); db.commit(); db.close()
t0 = time.time(); r = run("fleet", "summary"); dt = time.time() - t0
assert r.returncode == 0 and "nodes: 1001" in r.stdout, r.stdout + r.stderr
for line in ("active     700", "silent     200", "no-events  100", "revoked    1"): assert line in r.stdout, r.stdout
assert dt < 5, f"too slow: {dt:.1f}s"
assert run("fleet", "list", "--status", "revoked").stdout.count("ffff000000000000") == 1
out = run("fleet", "list", "--limit", "5").stdout; assert "more" in out
assert "gone" in run("fleet", "show", "ffff000000000000").stdout
assert "no events" in run("fleet", "show", "doesnotexist").stdout
f = t + "/fleet.csv"; assert run("fleet", "export", f).returncode == 0
assert len(list(csv.reader(open(f)))) == 1002
print(f"FLEET: TODAS LAS PRUEBAS OK (1001 nodos, resumen en {dt:.2f}s)")
