"""ChronoOS fase 4: identidad Ed25519 por nodo + updates firmados con rollback."""
import os, re, json, hashlib, tarfile, shutil
from pathlib import Path
from cryptography.hazmat.primitives.asymmetric import ed25519
from cryptography.hazmat.primitives import serialization as S
from cryptography.exceptions import InvalidSignature
from chronoos.cli import HOME, VERSION, log_event

IDK = HOME / "node.ed25519"
TRUST = HOME / "trust" / "publisher.pub"
REL = HOME / "releases"
CUR = HOME / "current"
PREV = HOME / "previous"

def _b(d): return d if isinstance(d, bytes) else d.encode()
def _raw_priv(k): return k.private_bytes(S.Encoding.Raw, S.PrivateFormat.Raw, S.NoEncryption())
def _raw_pub(k): return k.public_key().public_bytes(S.Encoding.Raw, S.PublicFormat.Raw)

def _write_secret(path, data):
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, "w") as f: f.write(data)

def load_key():
    if not IDK.exists():
        HOME.mkdir(parents=True, exist_ok=True)
        _write_secret(IDK, _raw_priv(ed25519.Ed25519PrivateKey.generate()).hex())
    return ed25519.Ed25519PrivateKey.from_private_bytes(bytes.fromhex(IDK.read_text().strip()))

def pubkey_hex(): return _raw_pub(load_key()).hex()
def node_id(): return hashlib.sha256(bytes.fromhex(pubkey_hex())).hexdigest()[:16]
def sign(data): return load_key().sign(_b(data)).hex()

def verify(pub_hex, data, sig_hex):
    try:
        ed25519.Ed25519PublicKey.from_public_bytes(bytes.fromhex(pub_hex)).verify(bytes.fromhex(sig_hex), _b(data))
        return True
    except (InvalidSignature, ValueError):
        return False

# ---------- publicador y updates ----------
def keygen(outdir):
    d = Path(outdir); d.mkdir(parents=True, exist_ok=True)
    k = ed25519.Ed25519PrivateKey.generate()
    _write_secret(d / "publisher.key", _raw_priv(k).hex())
    (d / "publisher.pub").write_text(_raw_pub(k).hex())
    print("publicador creado en", d)
    print("pub:", _raw_pub(k).hex())
    print("GUARDA publisher.key OFFLINE (fuera del telefono y del servidor). Quien la tenga puede firmar updates.")

def sign_file(file, keyfile):
    k = ed25519.Ed25519PrivateKey.from_private_bytes(bytes.fromhex(Path(keyfile).read_text().strip()))
    Path(str(file) + ".sig").write_text(k.sign(Path(file).read_bytes()).hex())
    print("firmado:", str(file) + ".sig")

def trust(pub):
    if not re.fullmatch(r"[0-9a-f]{64}", pub): raise SystemExit("clave publica invalida (64 hex)")
    TRUST.parent.mkdir(parents=True, exist_ok=True); TRUST.write_text(pub)
    log_event("update", "publicador de confianza actualizado"); print("publicador de confianza guardado")

def _report(msg):
    log_event("update", msg)
    try:
        from chronoos import agent; agent.add_event("update", msg)
    except Exception: pass

def verify_file(path):
    path = Path(path); sig = Path(str(path) + ".sig")
    if not path.exists(): raise SystemExit(f"no existe {path}")
    if not TRUST.exists(): raise SystemExit("sin publicador de confianza: usa 'chrono update trust PUB'")
    if not sig.exists():
        _report(f"rechazada {path.name}: sin firma"); raise SystemExit("falta el .sig: update sin firma rechazada")
    if not verify(TRUST.read_text().strip(), path.read_bytes(), sig.read_text().strip()):
        _report(f"rechazada {path.name}: firma invalida"); raise SystemExit("FIRMA INVALIDA: update rechazada")

def _ver(v): return tuple(int(x) for x in v.split("-")[0].split("."))

def current_version():
    f = CUR / "manifest.json"
    return json.loads(f.read_text())["version"] if f.exists() else VERSION

def apply(path):
    verify_file(path)
    with tarfile.open(path) as tf:
        members = tf.getmembers()
        for m in members:
            n = Path(m.name)
            if n.is_absolute() or ".." in n.parts or not (m.isfile() or m.isdir()):
                _report(f"rechazada {Path(path).name}: contenido inseguro"); raise SystemExit(f"contenido inseguro ({m.name}): rechazada")
        try:
            ver = str(json.load(tf.extractfile("manifest.json"))["version"])
            assert re.fullmatch(r"\d+(\.\d+)*", ver)
        except Exception:
            raise SystemExit("paquete sin manifest.json valido (version numerica)")
        cur = current_version()
        if _ver(ver) <= _ver(cur):
            _report(f"rechazada {ver}: no es mayor que {cur}"); raise SystemExit(f"version {ver} no es mayor que la actual ({cur}): rechazada")
        dest = REL / ver
        if dest.exists(): shutil.rmtree(dest)
        dest.mkdir(parents=True)
        try: tf.extractall(dest, members=members, filter="data")
        except TypeError: tf.extractall(dest, members=members)
    prev = os.path.realpath(CUR) if CUR.is_symlink() else ""
    tmp = HOME / ".cur.tmp"; tmp.unlink(missing_ok=True)
    os.symlink(dest, tmp); os.replace(tmp, CUR); PREV.write_text(prev)
    _report(f"update aplicada {cur} -> {ver}"); print(f"update {ver} verificada y preparada en {dest}")

def rollback():
    if not PREV.exists(): raise SystemExit("no hay update previa para revertir")
    prev = PREV.read_text().strip()
    if prev:
        tmp = HOME / ".cur.tmp"; tmp.unlink(missing_ok=True); os.symlink(prev, tmp); os.replace(tmp, CUR)
    else:
        CUR.unlink(missing_ok=True)
    PREV.unlink(); _report("rollback realizado"); print("revertido a", current_version())

def status():
    t = TRUST.read_text().strip()[:16] + "..." if TRUST.exists() else "ninguno"
    print(f"version actual: {current_version()}   publicador de confianza: {t}")

def register(sp):
    sp.add_parser("node").set_defaults(fn=lambda a: print(f"id:  {node_id()}\npub: {pubkey_hex()}   (clave PUBLICA: segura de compartir)"))
    pb = sp.add_parser("publisher").add_subparsers(dest="sub", required=True)
    k = pb.add_parser("keygen"); k.add_argument("outdir"); k.set_defaults(fn=lambda a: keygen(a.outdir))
    us = sp.add_parser("update").add_subparsers(dest="sub", required=True)
    s = us.add_parser("sign"); s.add_argument("file"); s.add_argument("--key", required=True); s.set_defaults(fn=lambda a: sign_file(a.file, a.key))
    t = us.add_parser("trust"); t.add_argument("pub"); t.set_defaults(fn=lambda a: trust(a.pub))
    v = us.add_parser("verify"); v.add_argument("file"); v.set_defaults(fn=lambda a: (verify_file(a.file), print("firma valida")))
    ap = us.add_parser("apply"); ap.add_argument("file"); ap.set_defaults(fn=lambda a: apply(a.file))
    us.add_parser("rollback").set_defaults(fn=lambda a: rollback())
    us.add_parser("status").set_defaults(fn=lambda a: status())
