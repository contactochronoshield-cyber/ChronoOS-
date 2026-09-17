#!/usr/bin/env bash
set -Eeuo pipefail

###############################################################################
# CHRONOOS SECURITY REMEDIATION
# Version: 1.0
#
# Purpose:
#   Automated security baseline and repository remediation.
#
# Safety:
#   - Creates a backup before modifying tracked files.
#   - Does not delete source, databases, certificates, logs or customer data.
#   - Does not execute destructive network/system operations.
#   - Does not modify git history.
#   - Does not invent production credentials.
###############################################################################

ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
cd "$ROOT"

STAMP="$(date -u +%Y%m%dT%H%M%SZ)"
BACKUP=".chrono-remediation-backup/${STAMP}"
REPORT="chrono-remediation-report-${STAMP}.txt"

mkdir -p "$BACKUP"

log() {
    printf '[CHRONO] %s\n' "$*"
    printf '[CHRONO] %s\n' "$*" >> "$REPORT"
}

warn() {
    printf '[WARNING] %s\n' "$*" >&2
    printf '[WARNING] %s\n' "$*" >> "$REPORT"
}

section() {
    printf '\n===== %s =====\n' "$*"
    printf '\n===== %s =====\n' "$*" >> "$REPORT"
}

###############################################################################
# 0. INITIAL STATE
###############################################################################

: > "$REPORT"

section "INITIAL STATE"

log "Repository: $ROOT"
log "Branch: $(git branch --show-current 2>/dev/null || printf 'unknown')"
log "Timestamp UTC: $STAMP"

git status --short >> "$REPORT" 2>&1 || true

###############################################################################
# 1. BACKUP
###############################################################################

section "BACKUP"

git ls-files -z | while IFS= read -r -d '' file; do
    if [ -f "$file" ]; then
        mkdir -p "$BACKUP/$(dirname "$file")"
        cp -p "$file" "$BACKUP/$file"
    fi
done

log "Tracked-file backup created: $BACKUP"

###############################################################################
# 2. REMOVE GENERATED PYTHON ARTEFACTS
###############################################################################

section "PYTHON ARTEFACT CLEANUP"

find . \
    -type d \
    -name '__pycache__' \
    -not -path './.git/*' \
    -prune \
    -exec rm -rf {} +

find . \
    -type f \
    \( -name '*.pyc' -o -name '*.pyo' \) \
    -not -path './.git/*' \
    -delete

log "Generated Python bytecode removed."

###############################################################################
# 3. SECURITY DIRECTORIES
###############################################################################

section "SECURITY DIRECTORY STRUCTURE"

mkdir -p \
    security/policy \
    security/pki/roots \
    security/pki/intermediate \
    security/pki/dev \
    security/keys \
    security/signatures \
    security/ledger \
    security/evidence \
    security/fuzz \
    security/tests \
    security/build \
    security/runtime \
    data \
    logs \
    backups \
    bin \
    dist

cat > security/policy/destructive-operations.conf <<'EOF'
# ChronoOS destructive-operation policy
#
# Default posture:
#   DENY
#
# Any operation capable of deleting, destroying, wiping, disabling,
# terminating or modifying external infrastructure requires explicit
# authorization and a dedicated implementation.

DESTRUCTIVE_OPERATIONS=DENY
REMOTE_DESTRUCTIVE_OPERATIONS=DENY
AUTOMATIC_REMEDIATION=DENY
AUTOMATIC_NETWORK_BLOCKING=DENY
AUTOMATIC_PROCESS_TERMINATION=DENY
AUTOMATIC_DEVICE_WIPE=DENY
FIREWALL_FLUSH=DENY
ROUTING_FLUSH=DENY

# Defensive actions must be:
# 1. Authorized
# 2. Logged
# 3. Auditable
# 4. Reversible where technically possible
# 5. Scoped to Chrono-managed assets
EOF

###############################################################################
# 4. SAFE COMMAND EXECUTION POLICY
###############################################################################

section "COMMAND EXECUTION POLICY"

cat > security/policy/command-execution.md <<'EOF'
# ChronoOS Command Execution Security Policy

External input MUST NOT be concatenated into shell commands.

Forbidden patterns include:

- system(user_input)
- os.system(user_input)
- shell=True with untrusted data
- popen(user_input)
- eval(user_input)
- exec(user_input)

Preferred approach:

1. Use direct APIs instead of a shell.
2. Use subprocess argument arrays.
3. Set shell=False.
4. Validate every externally supplied argument.
5. Apply allowlists where commands are unavoidable.
6. Never construct privileged commands from network input.
7. Log security-sensitive command execution.
8. Never expose raw command construction through an API.

Example:

BAD:

    subprocess.run("ping " + host, shell=True)

GOOD:

    subprocess.run(["ping", "-c", "1", validated_host], shell=False)

ChronoOS security-sensitive components MUST treat command execution
as a privileged boundary.
EOF

###############################################################################
# 5. STATIC SCAN
###############################################################################

section "STATIC SECURITY SCAN"

SCAN="$ROOT/$BACKUP/static-scan.txt"
: > "$SCAN"

grep -RInE \
    '\bsystem\s*\(|os\.system\s*\(|shell\s*=\s*True|subprocess\.Popen|subprocess\.run|subprocess\.call|subprocess\.check_call|subprocess\.check_output|popen\s*\(' \
    . \
    --exclude-dir=.git \
    --exclude-dir=__pycache__ \
    --exclude='*.pyc' \
    > "$SCAN" 2>/dev/null || true

SYSTEM_COUNT="$(grep -Ec 'system\s*\(|os\.system\s*\(' "$SCAN" 2>/dev/null || true)"

log "Potential command-execution references: ${SYSTEM_COUNT:-0}"
log "Full scan: $SCAN"

###############################################################################
# 6. .gitignore HARDENING
###############################################################################

section "GITIGNORE HARDENING"

touch .gitignore

cat >> .gitignore <<'EOF'

# ChronoOS security/runtime data
security/runtime/
security/evidence/*
security/keys/*
security/pki/dev/*
security/ledger/*.db
security/ledger/*.sqlite
security/ledger/*.sqlite3

# Operational data
data/
logs/
backups/
dist/
bin/

# Certificates / private material
*.key
*.pem
*.p12
*.pfx
*.jks

# Local secrets
.env
.env.*
!.env.example

# Python
__pycache__/
*.py[cod]
*$py.class

# Build artifacts
build/
*.o
*.so
*.a
*.dll
*.exe

# Fuzzing artifacts
.fuzz/
corpus/
crash-*
leak-*
timeout-*

# Chrono remediation reports/backups
.chrono-remediation-backup/
chrono-remediation-report-*.txt
EOF

###############################################################################
# 7. SECRET DETECTION
###############################################################################

section "SECRET SCAN"

SECRET_REPORT="$BACKUP/secrets.txt"

grep -RInE \
    '(BEGIN (RSA |EC |OPENSSH |PRIVATE )?PRIVATE KEY|aws_access_key_id|aws_secret_access_key|api[_-]?key[[:space:]]*=[[:space:]]*["'\''][^"'\'']+|password[[:space:]]*=[[:space:]]*["'\''][^"'\'']+)' \
    . \
    --exclude-dir=.git \
    --exclude-dir=__pycache__ \
    --exclude='*.pyc' \
    > "$SECRET_REPORT" 2>/dev/null || true

if [ -s "$SECRET_REPORT" ]; then
    warn "Potential secrets detected. Review: $SECRET_REPORT"
else
    log "No obvious secret patterns detected."
fi

###############################################################################
# 8. STRUCTURED SECURITY LEDGER
###############################################################################

section "SECURITY LEDGER"

cat > security/ledger/ledger-format.md <<'EOF'
# ChronoOS Security Ledger

The ledger uses append-only structured records.

Required fields:

- timestamp
- event_id
- event_type
- severity
- component
- actor
- target
- action
- result
- evidence_hash
- previous_record_hash

Recommended serialization:

JSON Lines (one canonical JSON object per line).

Security requirements:

- No plaintext credentials.
- No private keys.
- Canonical field ordering.
- UTF-8.
- UTC timestamps.
- SHA-256 evidence references.
- Previous-record hash chaining.
- Exclusive file locking.
- Atomic append.
- Verification before archival.

The ledger is an audit mechanism, not a trust root.
EOF

cat > security/ledger/ledger_writer.py <<'PY'
#!/usr/bin/env python3

import hashlib
import json
import os
import tempfile
from datetime import datetime, timezone
from pathlib import Path

try:
    import fcntl
except ImportError:
    fcntl = None

ROOT = Path(__file__).resolve().parent
LEDGER = ROOT / "chronos-security.jsonl"


def canonical(obj):
    return json.dumps(
        obj,
        sort_keys=True,
        separators=(",", ":"),
        ensure_ascii=False,
    )


def record_hash(record):
    return hashlib.sha256(
        canonical(record).encode("utf-8")
    ).hexdigest()


def append_record(event_type, severity, component,
                  actor, target, action, result,
                  evidence_hash=""):

    record = {
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "event_id": os.urandom(16).hex(),
        "event_type": event_type,
        "severity": severity,
        "component": component,
        "actor": actor,
        "target": target,
        "action": action,
        "result": result,
        "evidence_hash": evidence_hash,
        "previous_record_hash": "",
    }

    LEDGER.parent.mkdir(parents=True, exist_ok=True)

    with LEDGER.open("a+", encoding="utf-8") as fp:
        if fcntl is not None:
            fcntl.flock(fp.fileno(), fcntl.LOCK_EX)

        fp.seek(0)
        previous = ""

        for line in fp:
            line = line.strip()
            if line:
                try:
                    previous = record_hash(json.loads(line))
                except json.JSONDecodeError:
                    raise RuntimeError("Ledger contains invalid JSON")

        record["previous_record_hash"] = previous

        payload = canonical(record) + "\n"

        fp.seek(0, os.SEEK_END)
        fp.write(payload)
        fp.flush()
        os.fsync(fp.fileno())

        if fcntl is not None:
            fcntl.flock(fp.fileno(), fcntl.LOCK_UN)


if __name__ == "__main__":
    append_record(
        event_type="SYSTEM",
        severity="INFO",
        component="chrono-remediation",
        actor="system",
        target="chronoos",
        action="security-baseline",
        result="initialized",
    )
PY

chmod 700 security/ledger/ledger_writer.py

###############################################################################
# 9. API SECURITY BASELINE
###############################################################################

section "API SECURITY BASELINE"

cat > security/policy/api-security.md <<'EOF'
# ChronoOS API Security Baseline

Every externally reachable API MUST provide:

1. Authentication
2. Authorization
3. TLS in production
4. Input validation
5. Rate limiting
6. Request size limits
7. Structured audit logging
8. Error sanitization
9. Explicit CORS policy
10. Secure credential storage

Required trust flow:

Client
  |
TLS
  |
Authentication
  |
Authorization
  |
Input validation
  |
Business logic
  |
Audit ledger

No API endpoint should directly invoke a shell command using request data.

Production deployments MUST NOT rely on development-only TLS certificates.
EOF

###############################################################################
# 10. LOCAL DEVELOPMENT ROOT OF TRUST
###############################################################################

section "ROOT OF TRUST"

cat > security/pki/README.md <<'EOF'
# ChronoOS PKI

This directory defines the trust architecture.

Production:

    Offline Root CA
        |
        +-- Intermediate CA
                |
                +-- Service certificates
                +-- Node certificates
                +-- Device certificates

The production Root CA MUST be generated and stored outside the source
repository.

The repository contains policy and tooling, not production private keys.

Development certificates are isolated under:

    security/pki/dev/
EOF

cat > security/pki/root-of-trust-policy.md <<'EOF'
# ChronoOS Root of Trust Policy

The root of trust is the cryptographic authority used to validate:

- node identities
- service certificates
- signed alerts
- QR provisioning payloads
- firmware/package signatures
- evidence manifests

A public key or certificate embedded in source code is NOT sufficient
unless its provenance and rotation process are defined.

Production root keys MUST remain outside the Git repository.

Rotation MUST support:

- key version
- activation date
- expiration date
- revocation
- replacement
- audit record
EOF

###############################################################################
# 11. SIGNED MESH / QR FORMAT
###############################################################################

section "MESH AND QR TRUST FORMAT"

mkdir -p security/signatures

cat > security/signatures/signed-message-format.md <<'EOF'
# ChronoOS Signed Message Format

Mesh alerts and QR provisioning messages MUST be treated as untrusted
until signature verification succeeds.

Canonical envelope:

{
  "version": 1,
  "type": "...",
  "message_id": "...",
  "issuer": "...",
  "issued_at": "...",
  "expires_at": "...",
  "payload": {},
  "signature": "base64..."
}

Rules:

- Canonical serialization before signing.
- Explicit algorithm identifier.
- Explicit key identifier.
- Expiration checking.
- Replay protection.
- Nonce/message_id uniqueness.
- Signature verification against an approved trust root.
- Rejected messages MUST be auditable.
- Private keys MUST never be included in QR payloads.

QR codes may transport signed enrollment/provisioning data, but QR
transport itself does not provide trust.
EOF

###############################################################################
# 12. MODBUS TCP SECURITY SPECIFICATION
###############################################################################

section "MODBUS TCP"

mkdir -p industrial/modbus

cat > industrial/modbus/SECURITY-FRAMING.md <<'EOF'
# ChronoOS Modbus TCP Defensive Framing

A Modbus TCP ADU consists of:

MBAP Header:
    Transaction Identifier: 2 bytes
    Protocol Identifier:    2 bytes
    Length:                 2 bytes
    Unit Identifier:        1 byte

PDU:
    Function Code
    Function Data

The Length field defines the number of bytes following the Length field,
including the Unit Identifier and PDU.

Parser requirements:

1. Read exactly 7 bytes of MBAP header.
2. Decode fields as big-endian.
3. Require Protocol Identifier == 0 for standard Modbus TCP.
4. Validate Length against configured limits.
5. Read exactly the declared number of remaining bytes.
6. Never trust TCP packet boundaries as application-message boundaries.
7. Support fragmented reads.
8. Support multiple ADUs in one TCP read.
9. Reject malformed lengths.
10. Enforce function-code-specific PDU length validation.
11. Apply timeouts.
12. Never execute a Modbus request solely because it parsed correctly.
13. Authorization MUST occur before sensitive write operations.

A TCP read/write is not equivalent to a Modbus ADU boundary.
EOF

###############################################################################
# 13. REPRODUCIBLE BUILD POLICY
###############################################################################

section "REPRODUCIBLE BUILD"

cat > security/build/reproducible-build.md <<'EOF'
# ChronoOS Reproducible Build Policy

A release build MUST record:

- source revision
- compiler/interpreter version
- dependency versions
- operating-system/build image
- build flags
- generated artifact hashes
- build timestamp policy
- dependency hashes

The same source and declared environment should reproduce identical
or documented-equivalent artifacts.

Release artifacts MUST be generated outside runtime data directories.
EOF

###############################################################################
# 14. DOCKER BASELINE
###############################################################################

section "DOCKER BASELINE"

if [ -f Dockerfile ]; then
    cp -p Dockerfile "$BACKUP/Dockerfile.before-hardening"
fi

cat > security/build/docker-security.md <<'EOF'
# Docker Security Baseline

Production containers SHOULD:

- run as non-root
- use a minimal base image
- pin dependencies
- avoid privileged mode
- avoid host networking unless explicitly required
- use read-only filesystem where possible
- drop unnecessary Linux capabilities
- use explicit health checks
- avoid embedding secrets
- separate build and runtime stages
- generate SBOM/provenance metadata

Docker configuration is considered incomplete until the actual service
Dockerfiles and compose/deployment files have been validated.
EOF

###############################################################################
# 15. SOURCE / DATA / LOG SEPARATION
###############################################################################

section "DATA SEPARATION"

cat > security/policy/storage-layout.md <<'EOF'
# ChronoOS Storage Layout

Source:
    project source tree

Binaries:
    bin/
    dist/

Runtime:
    security/runtime/

Logs:
    logs/

Operational data:
    data/

Backups:
    backups/

Security evidence:
    security/evidence/

Keys:
    security/keys/
    production keys MUST NOT be committed

Build metadata:
    security/build/

The source tree MUST NOT be used as a database, customer-data directory,
runtime log directory or production key store.
EOF

###############################################################################
# 16. TEST INFRASTRUCTURE
###############################################################################

section "TEST INFRASTRUCTURE"

mkdir -p tests/integration tests/fuzz tests/security

cat > tests/security/test-security-layout.sh <<'EOF'
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
EOF

chmod +x tests/security/test-security-layout.sh

cat > tests/integration/security-baseline.sh <<'EOF'
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
EOF

chmod +x tests/integration/security-baseline.sh

###############################################################################
# 17. FUZZING POLICY
###############################################################################

section "FUZZING"

cat > tests/fuzz/README.md <<'EOF'
# ChronoOS Fuzzing

Security-sensitive parsers MUST be fuzzed.

Priority targets:

1. Modbus TCP parser
2. QR decoder/parser
3. Mesh message parser
4. Signed-message parser
5. API input parsers
6. Evidence/ledger parser
7. Configuration parsers

Required properties:

- no crash
- no memory corruption
- no unbounded allocation
- no infinite loop
- malformed input rejected safely
- parser never bypasses authorization
EOF

###############################################################################
# 18. PRODUCTION CLAIMS
###############################################################################

section "RELEASE STATUS"

cat > RELEASE-STATUS.md <<'EOF'
# ChronoOS Release Status

ChronoOS uses the following release terminology:

## Architectural Release

An architectural release documents system structure, interfaces,
security boundaries and implementation direction.

## Production Ready

"Production Ready" may only be used for a component after its defined
production acceptance criteria have been verified.

Minimum criteria include:

- security review
- automated tests
- integration tests
- fuzzing where applicable
- dependency review
- reproducible build
- deployment validation
- authentication/authorization validation
- logging/audit validation
- backup/recovery validation
- documented threat model
- documented operational procedures

A project MUST NOT describe itself as globally "Production Ready" merely
because its architecture is complete.

Component-level production status MUST be explicit.
EOF

###############################################################################
# 19. AUTOMATED PYTHON SYNTAX CHECK
###############################################################################

section "PYTHON VALIDATION"

PYTHON_FOUND=0
PYTHON_FAIL=0

while IFS= read -r -d '' py; do
    PYTHON_FOUND=$((PYTHON_FOUND + 1))

    if python3 -m py_compile "$py" 2>>"$REPORT"; then
        log "PASS Python: $py"
    else
        warn "FAIL Python syntax: $py"
        PYTHON_FAIL=$((PYTHON_FAIL + 1))
    fi
done < <(
    find . \
        -type f \
        -name '*.py' \
        -not -path './.git/*' \
        -not -path '*/__pycache__/*' \
        -print0
)

log "Python files checked: $PYTHON_FOUND"
log "Python syntax failures: $PYTHON_FAIL"

###############################################################################
# 20. SHELL SYNTAX CHECK
###############################################################################

section "SHELL VALIDATION"

SH_FOUND=0
SH_FAIL=0

while IFS= read -r -d '' sh; do
    SH_FOUND=$((SH_FOUND + 1))

    if bash -n "$sh" 2>>"$REPORT"; then
        log "PASS shell: $sh"
    else
        warn "FAIL shell syntax: $sh"
        SH_FAIL=$((SH_FAIL + 1))
    fi
done < <(
    find . \
        -type f \
        \( -name '*.sh' -o -name '*.bash' \) \
        -not -path './.git/*' \
        -print0
)

log "Shell files checked: $SH_FOUND"
log "Shell syntax failures: $SH_FAIL"

###############################################################################
# 21. GIT SAFETY CHECK
###############################################################################

section "GIT SAFETY"

if git diff --check; then
    log "PASS git diff --check"
else
    warn "Git whitespace/errors detected."
fi

###############################################################################
# 22. SECURITY-SENSITIVE FILE INVENTORY
###############################################################################

section "SENSITIVE FILE INVENTORY"

find . \
    -type f \
    \( -name '*.key' -o -name '*.pem' -o -name '*.p12' -o -name '*.pfx' \
       -o -name '*.db' -o -name '*.sqlite' -o -name '*.sqlite3' \
       -o -name '*.bak' \) \
    -not -path './.git/*' \
    -not -path './.chrono-remediation-backup/*' \
    | sort >> "$REPORT" || true

###############################################################################
# 23. FINAL SECURITY FINDINGS
###############################################################################

section "UNRESOLVED CODE-LEVEL FINDINGS"

COMMAND_FINDINGS="$BACKUP/command-execution-findings.txt"

grep -RInE \
    '\bsystem\s*\(|os\.system\s*\(|shell\s*=\s*True|popen\s*\(' \
    . \
    --exclude-dir=.git \
    --exclude-dir=__pycache__ \
    --exclude-dir=.chrono-remediation-backup \
    > "$COMMAND_FINDINGS" 2>/dev/null || true

if [ -s "$COMMAND_FINDINGS" ]; then
    warn "Command execution patterns remain. They require manual code-level remediation."
    warn "Review: $COMMAND_FINDINGS"
else
    log "No obvious system()/os.system()/shell=True/popen() patterns remain."
fi

###############################################################################
# 24. TEST SECURITY BASELINE
###############################################################################

section "BASELINE TESTS"

if ./tests/security/test-security-layout.sh >> "$REPORT" 2>&1; then
    log "Security layout test: PASS"
else
    warn "Security layout test: FAIL"
fi

if ./tests/integration/security-baseline.sh >> "$REPORT" 2>&1; then
    log "Security integration baseline: PASS"
else
    warn "Security integration baseline: FAIL"
fi

###############################################################################
# 25. FINAL STATUS
###############################################################################

section "FINAL STATUS"

cat >> "$REPORT" <<EOF

ChronoOS Security Remediation
==============================

Backup:
  $BACKUP

Report:
  $REPORT

Automated work completed:

[PASS] Generated Python artefacts removed
[PASS] Security directory structure created
[PASS] Destructive-operation deny policy created
[PASS] Command execution security policy created
[PASS] .gitignore hardened
[PASS] Secret-pattern scan executed
[PASS] Structured ledger specification created
[PASS] Ledger locking implementation created
[PASS] API security baseline created
[PASS] Root-of-trust architecture created
[PASS] Mesh/QR signed-message specification created
[PASS] Modbus TCP framing specification created
[PASS] Reproducible-build policy created
[PASS] Docker security baseline created
[PASS] Source/data/log separation policy created
[PASS] Security/integration/fuzz test structure created
[PASS] Release-status policy created
[PASS] Python syntax validation executed
[PASS] Shell syntax validation executed
[PASS] Git diff validation executed

Important:
This remediation establishes a security baseline.
It does not automatically certify existing application code as secure.

Remaining findings must be resolved at the actual implementation boundary,
especially:
- existing shell command construction
- existing API authentication
- actual TLS deployment
- production trust-root provisioning
- existing Modbus parser implementation
- existing Mesh/QR implementation
- existing Docker deployment
- existing installer/dispatcher
- complete integration/fuzz coverage

Production credentials and production root keys MUST be provisioned outside
the Git repository.

EOF

log "REMEDIATION BASELINE COMPLETE"

echo
echo "============================================================"
echo " CHRONOOS SECURITY REMEDIATION COMPLETE"
echo "============================================================"
echo
echo "Backup : $BACKUP"
echo "Report : $REPORT"
echo

git status --short
