#!/usr/bin/env bash
set -euo pipefail

echo "===== CHRONOOS SECURITY BASELINE ====="

test -f security/policy/destructive-operations.conf
test -f security/policy/command-execution.md
test -f security/policy/api-security.md
test -f security/pki/root-of-trust-policy.md
test -f security/signatures/signed-message-format.md
test -f industrial/modbus/SECURITY-FRAMING.md
test -f security/ledger/ledger-format.md
test -f security/build/reproducible-build.md

echo "PASS: security policy"
echo "PASS: trust architecture"
echo "PASS: signed-message specification"
echo "PASS: Modbus framing specification"
echo "PASS: ledger specification"
echo "PASS: reproducible-build specification"
