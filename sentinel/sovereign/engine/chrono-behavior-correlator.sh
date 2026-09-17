#!/bin/sh

set -eu

BASE="sentinel/sovereign"
FORENSICS="$BASE/forensics"
STATE="$BASE/state"

PROCESS_LOG="$FORENSICS/process-security.log"
NETWORK_LOG="$FORENSICS/network-security.log"
INTEGRITY_LOG="$FORENSICS/integrity-security.log"
PERSISTENCE_LOG="$FORENSICS/anti-persistence.log"
SENSOR_LOG="$FORENSICS/sensor-security.log"

CORRELATION_LOG="$FORENSICS/behavior-correlation.log"
INCIDENT_LOG="$FORENSICS/incidents.log"
STATEFILE="$STATE/behavior-state"

mkdir -p "$FORENSICS" "$STATE"

timestamp() {
    date -u '+%Y-%m-%dT%H:%M:%SZ'
}

log() {
    printf '%s | %s\n' "$(timestamp)" "$1" >> "$CORRELATION_LOG"
}

has_event() {
    FILE="$1"
    PATTERN="$2"

    [ -f "$FILE" ] || return 1

    grep -Eq "$PATTERN" "$FILE"
}

PROCESS_SIGNAL=0
NETWORK_SIGNAL=0
INTEGRITY_SIGNAL=0
PERSISTENCE_SIGNAL=0
SENSOR_SIGNAL=0

printf '%s\n' "======================================"
printf '%s\n' "       CHRONO BEHAVIOR CORRELATOR"
printf '%s\n' "======================================"
printf '%s\n' "Mode: CORRELATION"
printf '%s\n' "Automatic remediation: DISABLED"
printf '%s\n' ""

printf '%s\n' "===== SIGNAL ANALYSIS ====="

if has_event "$PROCESS_LOG" \
    '(^| \| )(PROCESS_SUSPICIOUS|PROCESS_UNAUTHORIZED|PRIVILEGE_ESCALATION|UNKNOWN_PROCESS)( \||$)'
then
    PROCESS_SIGNAL=1
    printf '%s\n' "PROCESS_SIGNAL=1"
fi

if has_event "$NETWORK_LOG" \
    '(^| \| )(SUSPICIOUS_CONNECTION|UNKNOWN_CONNECTION|UNAUTHORIZED_CONNECTION|NETWORK_ANOMALY)( \||$)'
then
    NETWORK_SIGNAL=1
    printf '%s\n' "NETWORK_SIGNAL=1"
fi

if has_event "$INTEGRITY_LOG" \
    '(^| \| )(INTEGRITY_MISMATCH|FILE_MODIFIED|TAMPER_DETECTED|HASH_MISMATCH|INTEGRITY_FAILURE)( \||$)'
then
    INTEGRITY_SIGNAL=1
    printf '%s\n' "INTEGRITY_SIGNAL=1"
fi

if has_event "$PERSISTENCE_LOG" \
    '(^| \| )(PERSISTENCE_DETECTED|UNAUTHORIZED_PERSISTENCE|PERSISTENCE_MODIFIED|SUSPICIOUS_PERSISTENCE)( \||$)'
then
    PERSISTENCE_SIGNAL=1
    printf '%s\n' "PERSISTENCE_SIGNAL=1"
fi

if has_event "$SENSOR_LOG" \
    '(^| \| )(UNAUTHORIZED_SENSOR_ACCESS|SUSPICIOUS_SENSOR_ACCESS|SENSOR_ABUSE|SENSOR_ANOMALY)( \||$)'
then
    SENSOR_SIGNAL=1
    printf '%s\n' "SENSOR_SIGNAL=1"
fi

SIGNALS=$((PROCESS_SIGNAL + NETWORK_SIGNAL + INTEGRITY_SIGNAL + PERSISTENCE_SIGNAL + SENSOR_SIGNAL))

printf '%s\n' ""
printf '%s\n' "Total behavioral signals: $SIGNALS"

STATE="SECURE"
REASON="No correlated behavioral threat pattern detected"
SEVERITY="INFO"
INCIDENT=""

#
# Correlation rules
#

# Critical:
# Integrity compromise + persistence
if [ "$INTEGRITY_SIGNAL" -eq 1 ] &&
   [ "$PERSISTENCE_SIGNAL" -eq 1 ]; then
    STATE="INCIDENT"
    SEVERITY="CRITICAL"
    INCIDENT="CORR-001"
    REASON="Integrity compromise correlated with persistence activity"
fi

# Critical:
# Process abuse + integrity compromise
if [ "$PROCESS_SIGNAL" -eq 1 ] &&
   [ "$INTEGRITY_SIGNAL" -eq 1 ]; then
    STATE="INCIDENT"
    SEVERITY="CRITICAL"
    INCIDENT="CORR-002"
    REASON="Suspicious process behavior correlated with integrity compromise"
fi

# High:
# Process + network + persistence
if [ "$PROCESS_SIGNAL" -eq 1 ] &&
   [ "$NETWORK_SIGNAL" -eq 1 ] &&
   [ "$PERSISTENCE_SIGNAL" -eq 1 ]; then
    STATE="INCIDENT"
    SEVERITY="HIGH"
    INCIDENT="CORR-003"
    REASON="Process, network and persistence anomalies correlated"
fi

# High:
# Network + sensor + process
if [ "$NETWORK_SIGNAL" -eq 1 ] &&
   [ "$SENSOR_SIGNAL" -eq 1 ] &&
   [ "$PROCESS_SIGNAL" -eq 1 ]; then
    STATE="INCIDENT"
    SEVERITY="HIGH"
    INCIDENT="CORR-004"
    REASON="Process, network and sensor anomalies correlated"
fi

#
# Record result
#

if [ "$STATE" = "INCIDENT" ]; then
    printf '%s | INCIDENT=%s | SEVERITY=%s | %s\n' \
        "$(timestamp)" \
        "$INCIDENT" \
        "$SEVERITY" \
        "$REASON" >> "$INCIDENT_LOG"

    log "CORRELATED_INCIDENT | $INCIDENT | severity=$SEVERITY"
else
    log "CORRELATION_SECURE | signals=$SIGNALS"
fi

{
    printf '%s\n' "CHRONO_BEHAVIOR_STATE_V1"
    printf 'timestamp=%s\n' "$(timestamp)"
    printf 'state=%s\n' "$STATE"
    printf 'signals=%s\n' "$SIGNALS"
    printf 'severity=%s\n' "$SEVERITY"
    printf 'reason=%s\n' "$REASON"
} > "$STATEFILE"

printf '%s\n' ""
printf '%s\n' "===== CORRELATION RESULT ====="
printf '%s\n' "Behavior state: $STATE"
printf '%s\n' "Signals: $SIGNALS"
printf '%s\n' "Severity: $SEVERITY"
printf '%s\n' "Reason: $REASON"

printf '%s\n' ""
printf '%s\n' "Chrono Behavior Correlator: COMPLETE"

exit 0
