#!/usr/bin/env bash
set -euo pipefail

required=(
    security/policy
    security/pki
    security/signatures
    security/ledger
    security/evidence
    security/build
)

for path in "${required[@]}"; do
    test -d "$path"
    echo "PASS: $path"
done

if find . -type f -name '*.pyc' -o -name '*.pyo' | grep -q .; then
    echo "FAIL: Python bytecode present"
    exit 1
fi

echo "Security layout: PASS"
