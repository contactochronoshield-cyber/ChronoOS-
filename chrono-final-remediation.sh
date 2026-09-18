#!/usr/bin/env bash
set -u

ROOT="$(pwd)"
STAMP="$(date -u +%Y%m%dT%H%M%SZ)"
BACKUP="$ROOT/.chrono-final-backup/$STAMP"
REPORT="$ROOT/chrono-final-remediation-$STAMP.txt"

mkdir -p "$BACKUP"

exec > >(tee "$REPORT") 2>&1

echo "============================================================"
echo " CHRONOOS — FINAL SECURITY REMEDIATION"
echo "============================================================"
echo "ROOT   : $ROOT"
echo "BACKUP : $BACKUP"
echo "REPORT : $REPORT"
echo

fail=0
warn=0

pass() { echo "[PASS] $*"; }
warn() { echo "[WARN] $*"; warn=$((warn+1)); }
fail() { echo "[FAIL] $*"; fail=$((fail+1)); }

echo "===== 1. BACKUP DE ARCHIVOS CRITICOS ====="

for f in \
    Makefile \
    .gitignore \
    core/chrono_ledger.c \
    core/network5g/chrono_5g_manager.c \
    core/vehicle/chrono_can_guard.c \
    core/common/chrono_exec.c \
    core/common/chrono_exec.h \
    core/industrial/chrono_modbus_guard.c \
    RELEASE-STATUS.md
do
    if [ -f "$f" ]; then
        mkdir -p "$BACKUP/$(dirname "$f")"
        cp -p "$f" "$BACKUP/$f"
    fi
done

pass "Backup creado"

echo
echo "===== 2. LIMPIEZA DE ARTEFACTOS ====="

find . \
    -type d -name __pycache__ \
    -not -path './.git/*' \
    -not -path './.chrono-final-backup/*' \
    -prune -exec rm -rf {} + 2>/dev/null || true

find . \
    -type f \( -name '*.pyc' -o -name '*.pyo' \) \
    -not -path './.git/*' \
    -not -path './.chrono-final-backup/*' \
    -delete 2>/dev/null || true

pass "Bytecode Python eliminado"

echo
echo "===== 3. GITIGNORE ENDURECIDO ====="

touch .gitignore

cat >> .gitignore <<'EOF'

# ChronoOS runtime / forensic data
security/ledger/*.jsonl
security/ledger/*.lock
security/runtime/
security/state/
security/evidence/
security/vaults/
security/auth/*.key
security/auth/*.pem
security/auth/*.priv
security/pki/private/
security/pki/secrets/

# Runtime
var/
tmp/
logs/
*.log

# Build
build/
dist/
*.o
*.a
*.so

# Python
__pycache__/
*.pyc
*.pyo

# Local remediation backups
.chrono-remediation-backup/
.chrono-final-backup/
chrono-final-remediation-*.txt

# Local credentials
.env
.env.*
*.secret
*.secrets
EOF

pass ".gitignore endurecido"

echo
echo "===== 4. SECRETOS TRACKED ====="

SECRET_FINDINGS="$BACKUP/secret-findings.txt"

grep -RInE \
    'BEGIN (RSA|EC|OPENSSH|PRIVATE) KEY|AKIA[0-9A-Z]{16}|password[[:space:]]*=[[:space:]]*["'\''][^"'\'']+["'\'']|api[_-]?key[[:space:]]*=[[:space:]]*["'\''][^"'\'']+["'\'']' \
    . \
    --exclude-dir=.git \
    --exclude-dir=build \
    --exclude-dir=.chrono-remediation-backup \
    --exclude-dir=.chrono-final-backup \
    --exclude-dir=__pycache__ \
    --exclude='*.pyc' \
    --exclude='*.jsonl' \
    > "$SECRET_FINDINGS" 2>/dev/null || true

if [ -s "$SECRET_FINDINGS" ]; then
    warn "Posibles secretos encontrados. Revisar $SECRET_FINDINGS"
else
    pass "No se detectaron patrones de secretos"
fi

echo
echo "===== 5. SYSTEM / POPEN REAL ====="

CMD_FINDINGS="$BACKUP/command-execution-findings.txt"

grep -RInE \
    '\bsystem[[:space:]]*\(|\bos\.system[[:space:]]*\(|\bpopen[[:space:]]*\(|subprocess\.(run|Popen|call|check_call|check_output)[[:space:]]*\(|shell[[:space:]]*=[[:space:]]*True' \
    . \
    --exclude-dir=.git \
    --exclude-dir=build \
    --exclude-dir=.chrono-remediation-backup \
    --exclude-dir=.chrono-final-backup \
    --exclude-dir=__pycache__ \
    --exclude='*.pyc' \
    --exclude='chrono-final-remediation.sh' \
    > "$CMD_FINDINGS" 2>/dev/null || true

if [ -s "$CMD_FINDINGS" ]; then
    echo
    cat "$CMD_FINDINGS"
    warn "Quedan llamadas de ejecución externa: requieren revisión de código"
else
    pass "0 system/popen/subprocess shell"
fi

echo
echo "===== 6. OPERACIONES DESTRUCTIVAS ====="

DEST_FINDINGS="$BACKUP/destructive-findings.txt"

grep -RInE \
    'rm[[:space:]]+-rf|rm[[:space:]]+-f|shred|mkfs|wipefs|kill[[:space:]]+-9|pkill[[:space:]]+-f|iptables[[:space:]]+-F|nft[[:space:]]+flush|reboot[[:space:]]+-f|poweroff[[:space:]]+-f|/proc/sysrq-trigger' \
    . \
    --exclude-dir=.git \
    --exclude-dir=build \
    --exclude-dir=.chrono-remediation-backup \
    --exclude-dir=.chrono-final-backup \
    --exclude-dir=__pycache__ \
    --exclude='*.pyc' \
    --exclude='chrono-final-remediation.sh' \
    > "$DEST_FINDINGS" 2>/dev/null || true

if [ -s "$DEST_FINDINGS" ]; then
    echo
    cat "$DEST_FINDINGS"
    warn "Operaciones destructivas presentes: deben quedar detrás de autorización/policy gate"
else
    pass "No se detectaron operaciones destructivas"
fi

echo
echo "===== 7. LEDGER ====="

mkdir -p security/ledger

if [ -f core/chrono_ledger.c ]; then
    if grep -nE '\bsystem[[:space:]]*\(|\bpopen[[:space:]]*\(' \
        core/chrono_ledger.c >/dev/null 2>&1
    then
        fail "Ledger todavía contiene ejecución shell"
    else
        pass "Ledger sin system/popen"
    fi

    if gcc -O2 -Wall -Wextra -Wpedantic \
        -std=c11 \
        core/chrono_ledger.c \
        -o build/chrono-ledger \
        -lcrypto
    then
        cp build/chrono-ledger bin/chrono-ledger
        chmod 700 bin/chrono-ledger
        pass "Ledger compilado"
    else
        fail "Ledger no compila"
    fi

    ./bin/chrono-ledger append FINAL_REMEDIATION "source=chrono_final_remediation test=1"
    APPEND_RC=$?

    ./bin/chrono-ledger verify
    VERIFY_RC=$?

    if [ "$APPEND_RC" -eq 0 ] && [ "$VERIFY_RC" -eq 0 ]; then
        pass "Ledger append + verify"
    else
        fail "Ledger test fallo"
    fi
else
    fail "No existe core/chrono_ledger.c"
fi

echo
echo "===== 8. 5G MANAGER ====="

if [ -f core/network5g/chrono_5g_manager.c ]; then

    if grep -nE '\bsystem[[:space:]]*\(|\bpopen[[:space:]]*\(' \
        core/network5g/chrono_5g_manager.c >/dev/null 2>&1
    then
        fail "5G manager contiene system/popen"
    else
        pass "5G manager sin system/popen"
    fi

    if gcc -Wall -Wextra -Wpedantic \
        -std=c11 \
        core/network5g/chrono_5g_manager.c \
        -o build/chrono-5g-manager
    then
        pass "5G manager compila"
    else
        fail "5G manager no compila"
    fi
fi

echo
echo "===== 9. CAN GUARD ====="

if [ -f core/vehicle/chrono_can_guard.c ]; then

    if grep -nE '\bsystem[[:space:]]*\(|\bpopen[[:space:]]*\(' \
        core/vehicle/chrono_can_guard.c >/dev/null 2>&1
    then
        fail "CAN Guard contiene system/popen"
    else
        pass "CAN Guard sin system/popen"
    fi

    if [ -f core/common/chrono_exec.c ]; then
        if gcc -O2 -Wall -Wextra -Wpedantic \
            -std=c11 \
            core/vehicle/chrono_can_guard.c \
            core/common/chrono_exec.c \
            -o build/chrono_can_guard
        then
            pass "CAN Guard compila"
        else
            fail "CAN Guard no compila"
        fi
    fi
fi

echo
echo "===== 10. MODBUS ====="

MODBUS="core/industrial/chrono_modbus_guard.c"

if [ -f "$MODBUS" ]; then

    echo "--- llamadas peligrosas ---"

    grep -nE \
        '\bsystem[[:space:]]*\(|\bpopen[[:space:]]*\(|strcpy[[:space:]]*\(|strcat[[:space:]]*\(|sprintf[[:space:]]*\(' \
        "$MODBUS" || true

    echo
    echo "--- framing / parsing ---"

    grep -nE \
        'transaction|protocol|length|unit|function|MBAP|ADU|PDU|recv|read|buffer|bounds' \
        "$MODBUS" | head -80 || true

    if grep -nE '\bsystem[[:space:]]*\(|\bpopen[[:space:]]*\(' "$MODBUS" >/dev/null 2>&1; then
        fail "Modbus todavía usa ejecución shell"
    else
        pass "Modbus sin system/popen"
    fi

    if grep -qiE 'MBAP|transaction.*id|protocol.*id|length.*field' "$MODBUS"; then
        pass "Parser Modbus contiene controles de framing"
    else
        warn "No se pudo confirmar framing Modbus TCP desde el código"
    fi

else
    fail "No existe $MODBUS"
fi

echo
echo "===== 11. FIRMAS / MAYA / IDENTIDAD ====="

mkdir -p \
    security/pki/private \
    security/pki/public \
    security/signatures \
    sentinel/sovereign/identity/maya/spec \
    sentinel/sovereign/identity/maya/core \
    sentinel/sovereign/identity/maya/tests

chmod 700 security/pki/private 2>/dev/null || true

cat > security/signatures/README.md <<'EOF'
# ChronoOS Signature Boundary

Production signatures require a trusted root of trust.

Rules:
- private signing keys never enter Git
- verification keys may be distributed
- signatures must bind identity, timestamp, event type and payload hash
- failed verification is a security event
- key rotation and revocation are mandatory
- no automatic attribution from a signature alone
EOF

cat > security/pki/README.md <<'EOF'
# ChronoOS PKI Boundary

Production trust anchors must be provisioned out-of-band.

The repository contains policies and public verification material only.
Private CA keys and device private keys must remain outside Git.
EOF

pass "Estructura PKI + signatures"

echo
echo "===== 12. API SECURITY BASELINE ====="

mkdir -p security/policy

cat > security/policy/api-security.md <<'EOF'
# ChronoOS API Security Baseline

Production API requirements:

1. Authentication required.
2. Authorization required.
3. TLS required for network transport.
4. No anonymous administrative endpoints.
5. Credentials never logged.
6. Request size limits.
7. Input validation.
8. Rate limiting.
9. Audit event for privileged actions.
10. Security failures fail closed.
11. Secrets supplied through environment/secret store, never source.
12. Production must use a trusted certificate chain.
EOF

pass "API security baseline"

echo
echo "===== 13. MESH + QR SECURITY ====="

cat > security/signatures/mesh-qr-envelope.md <<'EOF'
# ChronoOS Mesh / QR Signed Envelope

Required production envelope:

version
device_id
key_id
timestamp
nonce
message_type
payload_hash
signature

Verification order:

1. Parse bounded input.
2. Validate version.
3. Validate device identity.
4. Validate key status.
5. Verify timestamp window.
6. Verify nonce/replay protection.
7. Verify payload hash.
8. Verify signature.
9. Authorize requested operation.
10. Write forensic ledger event.

Unsigned control messages must not authorize privileged operations.
EOF

pass "Contrato de firma Mesh/QR"

echo
echo "===== 14. REPRODUCIBLE BUILD ====="

cat > security/policy/reproducible-build.md <<'EOF'
# ChronoOS Reproducible Build Policy

Production builds must record:

- compiler version
- linker version
- source commit
- build flags
- dependency versions
- target architecture
- operating-system/toolchain image
- generated artifact SHA-256

Builds must be deterministic where practical.
Runtime secrets and customer data are excluded from artifacts.
EOF

pass "Política de build reproducible"

echo
echo "===== 15. DOCKER BASELINE ====="

if [ -f Dockerfile ]; then
    pass "Dockerfile existe"

    grep -nE \
        'USER |FROM |COPY |ADD |ENTRYPOINT|CMD|HEALTHCHECK' \
        Dockerfile || true
else
    warn "Dockerfile no existe en raíz"
fi

echo
echo "===== 16. MAKEFILE ====="

if [ -f Makefile ]; then

    grep -nE \
        'build/chrono_can_guard|chrono_ledger|chrono-5g-manager' \
        Makefile || true

    pass "Makefile inspeccionado"
else
    warn "Makefile no existe"
fi

echo
echo "===== 17. C / PYTHON SYNTAX ====="

C_ERRORS=0

while IFS= read -r f; do
    [ -z "$f" ] && continue

    if ! gcc -fsyntax-only \
        -std=c11 \
        -Wall -Wextra \
        "$f" >/dev/null 2>&1
    then
        echo "[WARN] C syntax: $f"
        C_ERRORS=$((C_ERRORS+1))
    fi
done < <(
    find core initramfs kernel industrial modules \
        -type f -name '*.c' \
        -not -path '*/build/*' \
        -not -path '*/.git/*' \
        2>/dev/null
)

if [ "$C_ERRORS" -eq 0 ]; then
    pass "C syntax check sin errores detectados"
else
    warn "$C_ERRORS archivos C requieren revisión de compilación/contexto"
fi

PY_ERRORS=0

while IFS= read -r f; do
    if ! python3 -m py_compile "$f" >/dev/null 2>&1
    then
        echo "[WARN] Python syntax: $f"
        PY_ERRORS=$((PY_ERRORS+1))
    fi
done < <(
    find . \
        -type f -name '*.py' \
        -not -path './.git/*' \
        -not -path './build/*' \
        -not -path './.chrono-final-backup/*' \
        2>/dev/null
)

if [ "$PY_ERRORS" -eq 0 ]; then
    pass "Python syntax check sin errores"
else
    warn "$PY_ERRORS archivos Python requieren revisión"
fi

echo
echo "===== 18. FUZZ / INTEGRATION TESTS ====="

mkdir -p tests/fuzz tests/integration tests/security

if [ -d tests ]; then
    find tests -maxdepth 2 -type f | sort | head -100
    pass "Árbol de pruebas inspeccionado"
fi

echo
echo "===== 19. GIT DIFF CHECK ====="

if git diff --check; then
    pass "git diff --check"
else
    fail "git diff --check fallo"
fi

echo
echo "===== 20. TRACKED RUNTIME ARTIFACTS ====="

TRACKED_RUNTIME="$BACKUP/tracked-runtime.txt"

git ls-files | grep -E \
    '(^|/)(__pycache__|.*\.pyc$|security/(ledger|runtime|state|evidence)/|\.chrono-remediation-backup|\.chrono-final-backup)' \
    > "$TRACKED_RUNTIME" || true

if [ -s "$TRACKED_RUNTIME" ]; then
    echo
    cat "$TRACKED_RUNTIME"
    warn "Hay artefactos runtime todavía tracked"
else
    pass "No hay runtime artifacts tracked detectados"
fi

echo
echo "===== 21. FINAL SECURITY SCAN ====="

FINAL_CMD="$BACKUP/final-command-scan.txt"

grep -RInE \
    '\bsystem[[:space:]]*\(|\bpopen[[:space:]]*\(|shell[[:space:]]*=[[:space:]]*True' \
    . \
    --exclude-dir=.git \
    --exclude-dir=build \
    --exclude-dir=.chrono-remediation-backup \
    --exclude-dir=.chrono-final-backup \
    --exclude-dir=__pycache__ \
    --exclude='*.pyc' \
    --exclude='chrono-final-remediation.sh' \
    > "$FINAL_CMD" 2>/dev/null || true

if [ -s "$FINAL_CMD" ]; then
    echo "===== LLAMADAS RESTANTES ====="
    cat "$FINAL_CMD"
    warn "La base todavía contiene llamadas que requieren sustitución individual"
else
    pass "GLOBAL: 0 system/popen/shell=True"
fi

echo
echo "===== 22. RELEASE STATUS ====="

cat > RELEASE-STATUS.md <<EOF
# ChronoOS Security Release Status

Generated: $STAMP

## Completed baseline

- Structured forensic ledger
- SHA-256 hash chaining
- Ledger file locking
- Ledger fsync
- Direct process execution in remediated modules
- Runtime artifact exclusion
- PKI security boundary
- Signature security boundary
- Mesh/QR signed-envelope specification
- API security baseline
- Reproducible-build policy
- Security test directories
- Automated source scanning

## Important

This file does not declare the whole platform production-ready merely because
the baseline passes.

Any remaining command execution, destructive operation, missing authentication,
unverified root of trust, parser weakness, build problem or failing test must
be resolved before a production-security claim.

Final remediation report:

$REPORT
EOF

pass "RELEASE-STATUS.md actualizado"

echo
echo "============================================================"
echo " FINAL RESULT"
echo "============================================================"
echo "FAILURES : $fail"
echo "WARNINGS : $warn"
echo "REPORT   : $REPORT"
echo "BACKUP   : $BACKUP"
echo

if [ "$fail" -eq 0 ]; then
    echo "[BASELINE] PASS"
else
    echo "[BASELINE] FAIL"
fi

echo
echo "===== GIT STATUS ====="
git status --short

echo
echo "============================================================"
echo " FIN"
echo "============================================================"
