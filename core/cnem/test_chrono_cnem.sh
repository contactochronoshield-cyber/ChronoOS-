#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="$ROOT/bin/chrono-cnem"

mkdir -p "$ROOT/bin"

cc -std=c11 -Wall -Wextra -Werror \
    -I"$ROOT/core" \
    "$ROOT/core/cnem/chrono_cnem.c" \
    "$ROOT/core/cnem/test_chrono_cnem.c" \
    -o "$BIN"

"$BIN" --self-test

echo "[OK] Chrono Network Event Model test passed"
