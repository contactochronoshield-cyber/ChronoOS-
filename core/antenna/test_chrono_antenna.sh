#!/usr/bin/env bash
set -euo pipefail

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
BIN="$ROOT/bin/chrono-antenna"
TEST_DIR="$ROOT/var/antenna"
OUT="$TEST_DIR/.chrono-antenna-test.out"

echo "===== CHRONO ANTENNA INTELLIGENCE TEST ====="

mkdir -p "$TEST_DIR"

test -x "$BIN"

"$BIN" --self-test > "$OUT"

grep -q "\[OK\] antenna validation" "$OUT"
grep -q "STATUS: OPERATIONAL" "$OUT"

rm -f "$OUT"

echo "[OK] Chrono Antenna Intelligence test passed"
