import subprocess, sys, os, tempfile
CHRONO = os.path.join(os.path.dirname(os.path.abspath(__file__)), "chrono")
env = dict(os.environ, CHRONO_HOME=tempfile.mkdtemp())
def run(*a): return subprocess.run([sys.executable, CHRONO, *a], capture_output=True, text=True, env=env)
assert "ChronoOS" in run("version").stdout
assert run("status").returncode == 0
r = run("doctor"); assert ("[FAIL]" in r.stdout) == (r.returncode != 0)
assert "doctor" in run("logs").stdout
assert '"profile"' in run("caps").stdout
assert run("services").returncode == 0
assert run("event", "test", "uno").returncode == 0
run("event", "test", "dos")
assert "pendientes: 2" in run("queue").stdout
assert "sin sync_url" in run("sync").stdout
assert "pendientes: 2" in run("queue").stdout   # persiste entre procesos
print("fases 1-3 (cli y cola offline): OK")
