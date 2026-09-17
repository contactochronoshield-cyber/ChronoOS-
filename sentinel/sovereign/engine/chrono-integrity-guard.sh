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
bin/chrono-sensor-guard
sentinel/README.md
sentinel/sovereign/policy/sovereignty.conf
sentinel/sovereign/engine/chrono-sovereignty.sh
sentinel/sovereign/engine/chrono-process-guard.sh
sentinel/sovereign/engine/chrono-network-guard.sh
sentinel/sovereign/engine/chrono-integrity-guard.sh
sentinel/sovereign/engine/chrono-anti-persistence.sh
sentinel/sovereign/engine/chrono-sovereign-controller.sh
sentinel/sovereign/sensors/chrono-sensor-guard.sh
"

printf '%s\n' "===== CHRONO INTEGRITY GUARD ====="
printf '%s\n' "Status: ACTIVE"
printf '%s\n' "Mode: VERIFY"
printf '%s\n' "Purpose: critical system integrity verification"
printf '\n'

log "INTEGRITY_GUARD_START"

TOTAL=0
PRESENT=0
MISSING=0
MATCHED=0
MISMATCHED=0

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
            log "FILE_PRESENT | $relative | SHA256_UNAVAILABLE"
        fi
    else
        MISSING=$((MISSING + 1))

        printf '[MISSING] %s\n' "$relative"
        log "FILE_MISSING | $relative"
    fi
done

printf '\n%s\n' "----- STRUCTURE SUMMARY -----"
printf 'Files checked: %s\n' "$TOTAL"
printf 'Present: %s\n' "$PRESENT"
printf 'Missing: %s\n' "$MISSING"

if [ "$MISSING" -eq 0 ]; then
    printf '%s\n' "Critical file structure: OK"
    log "INTEGRITY_STRUCTURE_OK"
else
    printf '%s\n' "Critical file structure: WARNING"
    log "INTEGRITY_STRUCTURE_WARNING | missing=$MISSING"
fi

printf '\n%s\n' "----- SOVEREIGN BASELINE -----"

if [ ! -f "$BASELINE" ]; then
    printf '%s\n' "Baseline: NOT CREATED"
    printf '%s\n' "Mode: observation only"
    log "BASELINE_NOT_CREATED"
else
    printf '%s\n' "Baseline: PRESENT"

    log "BASELINE_PRESENT | $BASELINE"

    if command -v sha256sum >/dev/null 2>&1; then

        while IFS='  ' read -r expected relative
        do
            [ -n "$expected" ] || continue
            [ -n "$relative" ] || continue

            target="$CHRONO_ROOT/$relative"

            if [ ! -f "$target" ]; then
                printf '[BASELINE_MISSING] %s\n' "$relative"
                log "BASELINE_FILE_MISSING | $relative"
                MISMATCHED=$((MISMATCHED + 1))
                continue
            fi

            actual="$(sha256sum "$target" | awk '{print $1}')"

            if [ "$actual" = "$expected" ]; then
                printf '[MATCH] %s\n' "$relative"
                MATCHED=$((MATCHED + 1))
                log "BASELINE_MATCH | $relative"
            else
                printf '[MISMATCH] %s\n' "$relative"
                printf '     Expected: %s\n' "$expected"
                printf '     Actual:   %s\n' "$actual"

                MISMATCHED=$((MISMATCHED + 1))

                log "BASELINE_MISMATCH | $relative | expected=$expected | actual=$actual"
            fi
        done < "$BASELINE"

        printf '\n%s\n' "----- BASELINE RESULT -----"
        printf 'Baseline matches: %s\n' "$MATCHED"
        printf 'Baseline mismatches: %s\n' "$MISMATCHED"

        if [ "$MISMATCHED" -eq 0 ]; then
            printf '%s\n' "Integrity baseline: MATCH"
            log "INTEGRITY_BASELINE_MATCH"
        else
            printf '%s\n' "Integrity baseline: MISMATCH"
            log "INTEGRITY_BASELINE_MISMATCH | count=$MISMATCHED"
        fi
    else
        printf '%s\n' "SHA-256 unavailable: baseline verification skipped"
        log "BASELINE_VERIFY_SKIPPED | sha256sum_unavailable"
    fi
fi

printf '\n%s\n' "----- INTEGRITY POLICY -----"
printf '%s\n' "Cryptographic verification: ENABLED"
printf '%s\n' "Unauthorized modification: ALERT"
printf '%s\n' "Automatic remediation: DISABLED"
printf '%s\n' "Evidence preservation: ENABLED"

log "INTEGRITY_GUARD_COMPLETE"

printf '\n%s\n' "Chrono Integrity Guard: COMPLETE"
printf '%s\n' "Log: $LOGFILE"
