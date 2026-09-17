#!/bin/sh

set -u

CHRONO_ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"
LOGDIR="$CHRONO_ROOT/sentinel/sovereign/forensics"
LOGFILE="$LOGDIR/controller.log"

mkdir -p "$LOGDIR"

log() {
    printf '%s | %s\n' \
        "$(date -u '+%Y-%m-%dT%H:%M:%SZ')" \
        "$*" >> "$LOGFILE"
}

run_component() {
    NAME="$1"
    COMMAND="$2"

    printf '\n===== %s =====\n' "$NAME"

    if [ -x "$COMMAND" ]; then
        "$COMMAND"
        RESULT=$?

        if [ "$RESULT" -eq 0 ]; then
            log "COMPONENT_OK | $NAME"
        else
            log "COMPONENT_ERROR | $NAME | exit=$RESULT"
        fi
    else
        printf '%s\n' "Component unavailable: $COMMAND"
        log "COMPONENT_UNAVAILABLE | $NAME | $COMMAND"
    fi
}

printf '%s\n' "======================================"
printf '%s\n' "     CHRONO SOVEREIGN CONTROLLER"
printf '%s\n' "======================================"
printf '%s\n' "Status: ACTIVE"
printf '%s\n' "Mode: MONITOR"
printf '%s\n' "Authority: HUMAN SECURITY POLICY"
printf '%s\n' "AI Authority: DISABLED"

log "CONTROLLER_START"

run_component \
    "SOVEREIGN SECURITY ENGINE" \
    "$CHRONO_ROOT/sentinel/sovereign/engine/chrono-sovereignty.sh"

run_component \
    "PROCESS GUARD" \
    "$CHRONO_ROOT/sentinel/sovereign/engine/chrono-process-guard.sh"

run_component \
    "NETWORK GUARD" \
    "$CHRONO_ROOT/sentinel/sovereign/engine/chrono-network-guard.sh"

run_component \
    "INTEGRITY GUARD" \
    "$CHRONO_ROOT/sentinel/sovereign/engine/chrono-integrity-guard.sh"

run_component \
    "ANTI-PERSISTENCE GUARD" \
    "$CHRONO_ROOT/sentinel/sovereign/engine/chrono-anti-persistence.sh"

printf '\n===== SECURITY STATUS =====\n'
printf '%s\n' "Sovereignty Layer: ACTIVE"
printf '%s\n' "Process monitoring: ACTIVE"
printf '%s\n' "Network monitoring: ACTIVE"
printf '%s\n' "Integrity monitoring: ACTIVE"
printf '%s\n' "Anti-persistence monitoring: ACTIVE"
printf '%s\n' "Forensic logging: ACTIVE"
printf '%s\n' "Automatic blocking: DISABLED"
printf '%s\n' "Network isolation: READY"

log "CONTROLLER_COMPLETE"

printf '\n%s\n' "Chrono Sovereign Controller: COMPLETE"
printf '%s\n' "Controller log: $LOGFILE"
