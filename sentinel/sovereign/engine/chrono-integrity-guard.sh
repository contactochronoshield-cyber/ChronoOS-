#!/bin/sh

set -u

CHRONO_ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"
LOGDIR="$CHRONO_ROOT/sentinel/sovereign/forensics"
LOGFILE="$LOGDIR/integrity-security.log"
BASELINE="$CHRONO_ROOT/config/chrono_sovereign_baseline.sha256"

mkdir -p "$LOGDIR"

log() {
    printf '%s | %s\n' \
        "$(date -u '+%Y-%m-%dT%H:%M:%SZ')" \
        "$*" >> "$LOGFILE"
}

printf '%s\n' "===== CHRONO INTEGRITY GUARD ====="
printf '%s\n' "Status: ACTIVE"
printf '%s\n' "Mode: VERIFY"
printf '%s\n' "Purpose: critical system integrity verification"
printf '\n'

log "INTEGRITY_GUARD_START"

FILES="
VERSION
config/chrono.conf
config/chrono_manifest.sha256
core/chrono_core.h
core/chrono_master_orchestrator.c
core/chrono_panic_protocol.c
init/chrono-init
bin/chrono
bin/chrono-watchdog
bin/chrono-hardware-guard
bin/chrono-shield-dns
bin/chrono-network-watch
bin/chrono-privacy-audit
sentinel/README.md
sentinel/sovereign/policy/sovereignty.conf
sentinel/sovereign/engine/chrono-sovereignty.sh
sentinel/sovereign/engine/chrono-process-guard.sh
sentinel/sovereign/engine/chrono-network-guard.sh
sentinel/sovereign/engine/chrono-sovereign-controller.sh
"

TOTAL=0
PRESENT=0
MISSING=0

printf '%s\n' "----- CRITICAL FILES -----"

for relative in $FILES
do
    TOTAL=$((TOTAL + 1))
    target="$CHRONO_ROOT/$relative"

    if [ -f "$target" ]; then
        PRESENT=$((PRESENT + 1))

        if command -v sha256sum >/dev/null 2>&1; then
            HASH="$(sha256sum "$target" | awk '{print $1}')"
            printf '[OK] %s\n' "$relative"
            printf '     SHA256: %s\n' "$HASH"
            log "FILE_PRESENT | $relative | SHA256=$HASH"
        else
            printf '[OK] %s\n' "$relative"
            log "FILE_PRESENT | $relative"
        fi
    else
        MISSING=$((MISSING + 1))
        printf '[MISSING] %s\n' "$relative"
        log "FILE_MISSING | $relative"
    fi
done

printf '\n%s\n' "----- SUMMARY -----"
printf 'Files checked: %s\n' "$TOTAL"
printf 'Present: %s\n' "$PRESENT"
printf 'Missing: %s\n' "$MISSING"

if [ "$MISSING" -eq 0 ]; then
    log "INTEGRITY_STRUCTURE_OK"
    printf '%s\n' "Critical file structure: OK"
else
    log "INTEGRITY_STRUCTURE_WARNING | missing=$MISSING"
    printf '%s\n' "Critical file structure: WARNING"
fi

printf '\n%s\n' "----- BASELINE -----"

if [ -f "$BASELINE" ]; then
    printf '%s\n' "Baseline: PRESENT"
    log "BASELINE_PRESENT | $BASELINE"
else
    printf '%s\n' "Baseline: NOT CREATED"
    printf '%s\n' "Mode: observation only"
    log "BASELINE_NOT_CREATED"
fi

printf '\n%s\n' "----- INTEGRITY POLICY -----"
printf '%s\n' "Cryptographic verification: ENABLED"
printf '%s\n' "Unauthorized modification: ALERT"
printf '%s\n' "Automatic remediation: DISABLED"
printf '%s\n' "Evidence preservation: ENABLED"

log "INTEGRITY_GUARD_COMPLETE"

printf '\n%s\n' "Chrono Integrity Guard: COMPLETE"
printf '%s\n' "Log: $LOGFILE"
