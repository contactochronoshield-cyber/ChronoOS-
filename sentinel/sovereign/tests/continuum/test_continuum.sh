#!/usr/bin/env bash

set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"
BIN="$ROOT/bin/chrono-continuum"
TEST_ROOT="$ROOT/var/continuum-test"

fail()
{
    echo "[FAIL] $1"
    rm -rf "$TEST_ROOT"
    exit 1
}

echo "[*] Chrono Continuum test"

test -x "$BIN" || fail "chrono-continuum no está compilado"

rm -rf "$TEST_ROOT"

"$BIN" init "$TEST_ROOT" || fail "init falló"

test -f "$TEST_ROOT/identity" || fail "identity no creado"
test -f "$TEST_ROOT/state" || fail "state no creado"
test -f "$TEST_ROOT/events.log" || fail "events.log no creado"

"$BIN" event "$TEST_ROOT" HARDWARE_REGISTERED system "initial infrastructure state" \
    || fail "evento 1 falló"

"$BIN" event "$TEST_ROOT" CONFIGURATION_CHANGED operator "network configuration updated" \
    || fail "evento 2 falló"

"$BIN" event "$TEST_ROOT" SECURITY_VERIFIED sentinel "integrity verification passed" \
    || fail "evento 3 falló"

VERIFY_OUTPUT="$("$BIN" verify "$TEST_ROOT")" \
    || fail "verificación inicial falló"

echo "$VERIFY_OUTPUT"

echo "$VERIFY_OUTPUT" | grep -q "state=VERIFIED" \
    || fail "cadena inicial no verificada"

echo "$VERIFY_OUTPUT" | grep -q "events=3" \
    || fail "cantidad inicial de eventos incorrecta"

python3 - "$TEST_ROOT/events.log" <<'PY'
from pathlib import Path
import sys

p = Path(sys.argv[1])
s = p.read_text()

old = "network configuration updated"
new = "network configuration compromised"

if old not in s:
    raise SystemExit(1)

p.write_text(s.replace(old, new, 1))
PY

if "$BIN" verify "$TEST_ROOT" > "$TEST_ROOT/tamper.out" 2>&1; then
    cat "$TEST_ROOT/tamper.out"
    fail "la alteración no fue detectada"
fi

cat "$TEST_ROOT/tamper.out"

grep -q "VERIFY_FAILURE=HASH_MISMATCH" "$TEST_ROOT/tamper.out" \
    || fail "no se detectó HASH_MISMATCH"

grep -q "sequence=2" "$TEST_ROOT/tamper.out" \
    || fail "no se identificó el evento alterado"

python3 - "$TEST_ROOT/events.log" <<'PY'
from pathlib import Path
import sys

p = Path(sys.argv[1])
s = p.read_text()

old = "network configuration compromised"
new = "network configuration updated"

if old not in s:
    raise SystemExit(1)

p.write_text(s.replace(old, new, 1))
PY

VERIFY_OUTPUT="$("$BIN" verify "$TEST_ROOT")" \
    || fail "verificación después de restaurar falló"

echo "$VERIFY_OUTPUT"

echo "$VERIFY_OUTPUT" | grep -q "state=VERIFIED" \
    || fail "cadena no recuperó estado VERIFIED"

echo "$VERIFY_OUTPUT" | grep -q "events=3" \
    || fail "cantidad final de eventos incorrecta"

rm -rf "$TEST_ROOT"

echo "[OK] Chrono Continuum test passed"
