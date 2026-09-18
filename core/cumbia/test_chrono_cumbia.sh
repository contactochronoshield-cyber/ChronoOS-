#!/usr/bin/env bash
set -euo pipefail

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
BIN="$ROOT/bin/chrono-cumbia"
TEST_DIR="$ROOT/var/cumbia"
OUT="$TEST_DIR/.chrono-cumbia-test.out"

echo "===== CUMBIA TEST ====="

mkdir -p "$TEST_DIR"

test -x "$BIN"

"$BIN" --self-test > "$OUT"

grep -q "=== CUMBIA ===" "$OUT"
grep -q "Version:       1.0.0" "$OUT"
grep -q "Authorized:    YES" "$OUT"
grep -q "Management:    OK" "$OUT"
grep -q "BGP:           UP" "$OUT"
grep -q "RPKI:          VALID" "$OUT"
grep -q "STATUS: ONLINE" "$OUT"
grep -q "\[OK\] Cumbia self-test passed" "$OUT"

rm -f "$OUT"

echo "[OK] Cumbia test passed"
