#!/bin/sh

PASS=0
FAIL=0

# shellcheck disable=SC1007
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
# shellcheck disable=SC1007
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/../../.." && pwd)

BIN="$REPO_ROOT/bin"
TMP="$REPO_ROOT/.chrono-test-runtime"

cleanup() {
    rm -rf "$TMP"
}
trap cleanup EXIT INT TERM

mkdir -p "$TMP"

ok() {
    echo "[PASS] $1"
    PASS=$((PASS + 1))
}

fail() {
    echo "[FAIL] $1"
    FAIL=$((FAIL + 1))
}

echo "=== CHRONOOS SENTINEL TESTS ==="

# 1. Ledger
if [ -x "$BIN/chrono-ledger" ]; then
    if "$BIN/chrono-ledger" append "TEST_SENTINEL" "test automatico" >/dev/null 2>&1; then
        ok "chrono-ledger operativo"
    else
        fail "chrono-ledger no pudo registrar evento"
    fi
else
    fail "chrono-ledger no encontrado"
fi

# 2. IoT engine
if [ -x "$BIN/chrono-iot-engine" ]; then
    ok "chrono-iot-engine presente"
else
    fail "chrono-iot-engine ausente"
fi

# 3. CAN guard
if [ -x "$BIN/chrono-can-guard" ]; then
    ok "chrono-can-guard presente"
else
    fail "chrono-can-guard ausente"
fi

# 4. Context auth, si existe en el proyecto
if [ -f "$REPO_ROOT/core/auth/chrono_context_auth.c" ]; then
    if [ -x "$BIN/chrono-context-auth" ]; then
        ok "chrono-context-auth presente"
    else
        fail "chrono-context-auth no construido"
    fi
else
    echo "[INFO] chrono-context-auth: fuente no presente; prueba omitida"
fi

# 5. TPM, si existe en el proyecto
if [ -f "$REPO_ROOT/core/tpm/chrono_tpm.c" ]; then
    if [ -x "$BIN/chrono-tpm" ]; then
        ok "chrono-tpm presente"
    else
        fail "chrono-tpm no construido"
    fi
else
    echo "[INFO] chrono-tpm: fuente no presente; prueba omitida"
fi

# 6. Fixture temporal para autenticación.
# Nunca se utiliza una clave privada real del repositorio.
MASTER_KEY="$TMP/master.key"
if dd if=/dev/zero of="$MASTER_KEY" bs=32 count=1 >/dev/null 2>&1; then
    SIZE=$(wc -c < "$MASTER_KEY")
    if [ "$SIZE" -eq 32 ]; then
        ok "fixture temporal master.key: 32 bytes"
    else
        fail "fixture temporal master.key incorrecto"
    fi
else
    fail "no se pudo crear fixture temporal"
fi

# 7. Ledger verification
if [ -x "$BIN/chrono-ledger" ]; then
    if "$BIN/chrono-ledger" verify >/dev/null 2>&1; then
        ok "Ledger chain verificado"
    else
        fail "Ledger chain verification fallo"
    fi
fi

echo
echo "=== RESULTADOS: $PASS pasaron / $FAIL fallaron ==="

if [ "$FAIL" -eq 0 ]; then
    echo "[OK] Sentinel tests completados"
    exit 0
fi

echo "[FAIL] Sentinel tests con errores"
exit 1
