#!/bin/sh

set -u

# shellcheck disable=SC1007
CHRONO_ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"

LOGDIR="$CHRONO_ROOT/sentinel/sovereign/forensics"
THREATDIR="$CHRONO_ROOT/sentinel/sovereign/threats"
STATEDIR="$CHRONO_ROOT/sentinel/sovereign/state"

LOGFILE="$LOGDIR/threat-engine.log"
INCIDENTFILE="$LOGDIR/incidents.log"
STATEFILE="$STATEDIR/threat-state"

mkdir -p "$LOGDIR" "$THREATDIR" "$STATEDIR"

timestamp() {
    date -u '+%Y-%m-%dT%H:%M:%SZ'
}

log() {
    printf '%s | %s\n' "$(timestamp)" "$*" >> "$LOGFILE"
}

incident() {
    ID="$1"
    SEVERITY="$2"
    SOURCE="$3"
    REASON="$4"

    printf '%s | INCIDENT=%s | SEVERITY=%s | SOURCE=%s | %s\n' \
        "$(timestamp)" "$ID" "$SEVERITY" "$SOURCE" "$REASON" \
        >> "$INCIDENTFILE"

    log "INCIDENT | $ID | $SEVERITY | $SOURCE | $REASON"
}

analyze_exact_events() {
    FILE="$1"
    PATTERN="$2"
    ID="$3"
    SEVERITY="$4"
    SOURCE="$5"
    REASON="$6"

    [ -f "$FILE" ] || return 1

    if grep -E "$PATTERN" "$FILE" >/dev/null 2>&1; then
        incident "$ID" "$SEVERITY" "$SOURCE" "$REASON"
        return 0
    fi

    return 1
}

printf '%s\n' "======================================"
printf '%s\n' "       CHRONO THREAT ENGINE"
printf '%s\n' "======================================"
printf '%s\n' "Mode: DETECTION"
printf '%s\n' "Threat intelligence: ENABLED"
printf '%s\n' "Behavior correlation: ENABLED"
printf '%s\n' "Automatic remediation: DISABLED"

log "THREAT_ENGINE_START"

THREATS=0

PROCESS_LOG="$LOGDIR/process-security.log"
NETWORK_LOG="$LOGDIR/network-security.log"
INTEGRITY_LOG="$LOGDIR/integrity-security.log"
PERSISTENCE_LOG="$LOGDIR/anti-persistence.log"
SENSOR_LOG="$LOGDIR/sensor-security.log"

printf '\n===== PROCESS ANALYSIS =====\n'

if analyze_exact_events \
    "$PROCESS_LOG" \
    '(^| \| )(PROCESS_SUSPICIOUS|PROCESS_UNAUTHORIZED|PRIVILEGE_ESCALATION|UNKNOWN_PROCESS)( \||$)' \
    "PROC-001" "HIGH" "PROCESS" \
    "Verified suspicious process or privilege event detected"
then
    THREATS=$((THREATS + 1))
fi

printf '\n===== NETWORK ANALYSIS =====\n'

if analyze_exact_events \
    "$NETWORK_LOG" \
    '(^| \| )(SUSPICIOUS_CONNECTION|UNKNOWN_CONNECTION|UNAUTHORIZED_CONNECTION|NETWORK_ANOMALY)( \||$)' \
    "NET-001" "HIGH" "NETWORK" \
    "Verified suspicious network event detected"
then
    THREATS=$((THREATS + 1))
fi

printf '\n===== INTEGRITY ANALYSIS =====\n'

if analyze_exact_events \
    "$INTEGRITY_LOG" \
    '(^| \| )(INTEGRITY_MISMATCH|FILE_MODIFIED|TAMPER_DETECTED|HASH_MISMATCH|INTEGRITY_FAILURE)( \||$)' \
    "INT-001" "CRITICAL" "INTEGRITY" \
    "Verified critical integrity anomaly detected"
then
    THREATS=$((THREATS + 1))
fi

printf '\n===== PERSISTENCE ANALYSIS =====\n'

if analyze_exact_events \
    "$PERSISTENCE_LOG" \
    '(^| \| )(PERSISTENCE_DETECTED|UNAUTHORIZED_PERSISTENCE|PERSISTENCE_MODIFIED|SUSPICIOUS_PERSISTENCE)( \||$)' \
    "PER-001" "HIGH" "PERSISTENCE" \
    "Verified persistence anomaly detected"
then
    THREATS=$((THREATS + 1))
fi

printf '\n===== SENSOR ANALYSIS =====\n'

if analyze_exact_events \
    "$SENSOR_LOG" \
    '(^| \| )(UNAUTHORIZED_SENSOR_ACCESS|SUSPICIOUS_SENSOR_ACCESS|SENSOR_ABUSE|SENSOR_ANOMALY)( \||$)' \
    "SEN-001" "HIGH" "SENSOR" \
    "Verified suspicious sensor access detected"
then
    THREATS=$((THREATS + 1))
fi

printf '\n===== THREAT CORRELATION =====\n'

if [ "$THREATS" -eq 0 ]; then
    STATE="SECURE"
    REASON="No verified threat indicators found in monitored evidence"
else
    STATE="INCIDENT"
    REASON="$THREATS verified threat indicator group(s) detected"
fi

printf '%s\n' "Threat groups: $THREATS"
printf '%s\n' "Threat state: $STATE"
printf '%s\n' "Reason: $REASON"

{
    printf 'CHRONO_THREAT_STATE_V1\n'
    printf 'timestamp=%s\n' "$(timestamp)"
    printf 'state=%s\n' "$STATE"
    printf 'threat_groups=%s\n' "$THREATS"
    printf 'reason=%s\n' "$REASON"
} > "$STATEFILE"

log "THREAT_STATE | $STATE | groups=$THREATS"
log "THREAT_ENGINE_COMPLETE"

printf '\n%s\n' "Chrono Threat Engine: COMPLETE"
printf '%s\n' "State: $STATE"
printf '%s\n' "Threat log: $LOGFILE"
printf '%s\n' "Incident log: $INCIDENTFILE"

exit 0
