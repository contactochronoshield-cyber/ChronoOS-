#!/usr/bin/env python3
import os, sys, json, platform, shutil, socket, time, argparse
from pathlib import Path

VERSION = "1.3.0-dev"
HOME = Path(os.environ.get("CHRONO_HOME", Path.home() / ".chronoos"))
LOG = HOME / "events.log"

def log_event(kind, msg):
    HOME.mkdir(parents=True, exist_ok=True)
    with open(LOG, "a") as f:
        f.write(json.dumps({"ts": int(time.time()), "kind": kind, "msg": msg}) + "\n")

def detect_profile():
    env = os.environ
    if "TERMUX_VERSION" in env or "com.termux" in env.get("PREFIX", ""):
        return "mobile-android-termux"
    s = platform.system()
    if s == "Linux":
        if Path("/proc/device-tree/model").exists() or platform.machine().startswith(("arm", "aarch")):
            return "linux-edge-or-server"
        return "linux-server"
    return {"Darwin": "macos-client", "Windows": "windows-client"}.get(s, "unknown")

def _net():
    try:
        socket.create_connection(("1.1.1.1", 53), timeout=1.5).close()
        return "online"
    except OSError:
        return "offline"

def capabilities():
    du = shutil.disk_usage(str(Path.home()))
    return {
        "platform": platform.system(), "arch": platform.machine(),
        "profile": detect_profile(), "python": platform.python_version(),
        "cpus": os.cpu_count(), "disk_free_gb": round(du.free / 1e9, 1),
        "root": hasattr(os, "geteuid") and os.geteuid() == 0,
        "has_systemd": Path("/run/systemd/system").exists(),
        "has_gpu_nvidia": shutil.which("nvidia-smi") is not None,
        "has_wireguard": shutil.which("wg") is not None,
        "has_git": shutil.which("git") is not None,
        "network": _net(),
    }

def _writable():
    try:
        HOME.mkdir(parents=True, exist_ok=True)
        (HOME / ".t").write_text("x"); (HOME / ".t").unlink()
        return True
    except OSError:
        return False

def cmd_version(a): print(f"ChronoOS {VERSION}")

def cmd_status(a):
    c = capabilities()
    print(f"ChronoOS {VERSION}  perfil: {c['profile']}  red: {c['network']}")
    print("ChronoD:", "RUNNING" if (HOME / "chronod.pid").exists() else "STOPPED (agente no implementado aun)")
    print("Init:", "systemd" if c["has_systemd"] else "UNKNOWN")

def cmd_doctor(a):
    c = capabilities(); ok = True
    checks = [("python>=3.8", sys.version_info >= (3, 8)),
              ("disco libre >1GB", c["disk_free_gb"] > 1),
              ("directorio de estado escribible", _writable())]
    for name, passed in checks:
        print(("[OK]  " if passed else "[FAIL]"), name); ok &= passed
    for k in ("has_git", "has_wireguard", "has_systemd", "has_gpu_nvidia"):
        print("[info]", k, "->", c[k])
    log_event("doctor", "ok" if ok else "fail")
    sys.exit(0 if ok else 1)

def cmd_services(a):
    c = capabilities()
    svc = {"wireguard (wg)": c["has_wireguard"], "systemd": c["has_systemd"],
           "gpu nvidia": c["has_gpu_nvidia"], "chronod": (HOME / "chronod.pid").exists()}
    for k, v in svc.items():
        print(f"{k:18} {'disponible' if v else 'no disponible'}")

def cmd_logs(a):
    if not LOG.exists():
        print("Sin eventos aun."); return
    for line in LOG.read_text().splitlines()[-a.n:]:
        e = json.loads(line)
        print(time.strftime("%F %T", time.localtime(e["ts"])), e["kind"], e["msg"])

def cmd_caps(a): print(json.dumps(capabilities(), indent=2))

def main():
    p = argparse.ArgumentParser(prog="chrono", description="ChronoOS Anywhere")
    sp = p.add_subparsers(dest="cmd")
    for n, f in [("version", cmd_version), ("status", cmd_status), ("doctor", cmd_doctor),
                 ("services", cmd_services), ("caps", cmd_caps)]:
        sp.add_parser(n).set_defaults(fn=f)
    l = sp.add_parser("logs"); l.add_argument("-n", type=int, default=20); l.set_defaults(fn=cmd_logs)
    try:
        from chronoos import agent, server, identity
        agent.register(sp); server.register(sp); identity.register(sp)
    except ImportError as e:
        print('aviso: ChronoD/server/update no disponibles (falta dependencia):', e, file=sys.stderr)
    a = p.parse_args()
    if not a.cmd:
        p.print_help(); return
    a.fn(a)

if __name__ == "__main__":
    main()
