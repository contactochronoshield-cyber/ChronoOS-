#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="$ROOT/bin/chrono-assurance"

mkdir -p "$ROOT/bin"

cc -std=c11 -Wall -Wextra -Werror \
    -I"$ROOT/core" \
    "$ROOT/core/assurance/chrono_assurance.c" \
    "$ROOT/core/assurance/test_chrono_assurance.c" \
    -o "$BIN"

"$BIN" --self-test

echo "[OK] Chrono Assurance Engine test passed"
