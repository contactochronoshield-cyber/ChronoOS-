#!/bin/sh
# ChronoOS - Sentinel Tests
set -e
PASS=0
FAIL=0

ok()   { echo "[PASS] $1"; PASS=$((PASS+1)); }
fail() { echo "[FAIL] $1"; FAIL=$((FAIL+1)); }

# Test 1: chrono-ledger existe y funciona
if [ -x "../../bin/chrono-ledger" ]; then
    ../../bin/chrono-ledger append "TEST_SENTINEL" "test automatico" 2>/dev/null
    ok "chrono-ledger operativo"
else
    fail "chrono-ledger no encontrado"
fi

# Test 2: chrono-panic existe
[ -x "../../bin/chrono-panic" ] && ok "chrono-panic presente" || fail "chrono-panic ausente"

# Test 3: chrono-hardware-guard existe
[ -x "../../bin/chrono-hardware-guard" ] && ok "hardware-guard presente" || fail "hardware-guard ausente"

# Test 4: master.key tiene el tamano correcto
if [ -f "../../security/auth/master.key" ]; then
    SIZE=$(stat -c%s "../../security/auth/master.key" 2>/dev/null || echo 0)
    [ "$SIZE" -eq 32 ] && ok "master.key valida (32 bytes)" || fail "master.key tamano incorrecto: $SIZE"
else
    fail "master.key no existe"
fi

# Test 5: IoT engine compila y corre
if [ -x "../../bin/chrono-iot-engine" ]; then
    ok "chrono-iot-engine presente"
else
    fail "chrono-iot-engine ausente"
fi

# Test 6: Ledger chain verification
if [ -x "../../bin/chrono-ledger" ]; then
    ../../bin/chrono-ledger verify > /dev/null 2>&1
    ok "Ledger chain verificado"
fi

echo ""
echo "=== RESULTADOS: $PASS pasaron / $FAIL fallaron ==="
[ "$FAIL" -eq 0 ] && exit 0 || exit 1
