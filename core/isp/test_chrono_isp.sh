#!/usr/bin/env bash
set -euo pipefail

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
BIN="$ROOT/bin/chrono-isp"
TEST_DIR="$ROOT/var/isp"
OUT="$TEST_DIR/.chrono-isp-test.out"

echo "===== CHRONO ISP INTELLIGENCE TEST ====="

mkdir -p "$TEST_DIR"

test -x "$BIN"

"$BIN" --self-test > "$OUT"

grep -q "=== CHRONO ISP INTELLIGENCE ===" "$OUT"
grep -q "Version:       1.0.0" "$OUT"
grep -q "BGP:           UP" "$OUT"
grep -q "RPKI valid:    YES" "$OUT"
grep -q "IPv6:          ENABLED" "$OUT"
grep -q "Operator approval:     REQUIRED" "$OUT"
grep -q "STATUS: DEGRADED" "$OUT"
grep -q "\[OK\] Chrono ISP Intelligence self-test passed" "$OUT"

rm -f "$OUT"

echo "[OK] Chrono ISP Intelligence test passed"
