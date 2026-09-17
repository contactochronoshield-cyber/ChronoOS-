#!/usr/bin/env bash

set -u

CHRONO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
LOGDIR="$CHRONO_ROOT/sentinel/sovereign/forensics"
STATEFILE="$CHRONO_ROOT/sentinel/sovereign/state/security-state"
LOGFILE="$LOGDIR/security-state.log"

mkdir -p "$LOGDIR"

TIMESTAMP="$(date -u '+%Y-%m-%dT%H:%M:%SZ')"

STATE="SECURE"
REASON="All monitored security components passed"

PROCESS_STATUS="UNKNOWN"
NETWORK_STATUS="UNKNOWN"
INTEGRITY_STATUS="UNKNOWN"
PERSISTENCE_STATUS="UNKNOWN"
SENSOR_STATUS="UNKNOWN"

set_state() {
    local new_state="$1"
    local reason="$2"

    case "$new_state" in
        INCIDENT)
            STATE="INCIDENT"
            REASON="$reason"
            ;;
        WARNING)
            if [ "$STATE" != "INCIDENT" ]; then
                STATE="WARNING"
                REASON="$reason"
            fi
            ;;
    esac
}

latest_match() {
    local log="$1"
    local pattern="$2"

    [ -f "$log" ] || return 1

    grep -E "$pattern" "$log" | tail -n 1
}

check_integrity() {
    local log="$LOGDIR/integrity-security.log"
    local latest=""

    if [ ! -f "$log" ]; then
        INTEGRITY_STATUS="UNKNOWN"
        set_state "WARNING" "Integrity evidence unavailable"
        return
    fi

    latest="$(latest_match "$log" \
        'INTEGRITY_BASELINE_(MATCH|MISMATCH)')"

    if [ -z "$latest" ]; then
        INTEGRITY_STATUS="UNKNOWN"
        set_state "WARNING" "Integrity state could not be established"
        return
    fi

    case "$latest" in
        *"INTEGRITY_BASELINE_MISMATCH"*)
            INTEGRITY_STATUS="INCIDENT"
            set_state "INCIDENT" \
                "Critical integrity baseline mismatch detected"
            ;;
        *"INTEGRITY_BASELINE_MATCH"*)
            INTEGRITY_STATUS="SECURE"
            ;;
    esac
}

check_process() {
    local log="$LOGDIR/process-security.log"

    if [ ! -f "$log" ]; then
        PROCESS_STATUS="UNKNOWN"
        set_state "WARNING" "Process security evidence unavailable"
        return
    fi

    if tail -n 100 "$log" | grep -qE \
        "ALERT|BLOCK|SUSPICIOUS|VIOLATION"; then
        PROCESS_STATUS="INCIDENT"
        set_state "INCIDENT" "Process security event detected"
    else
        PROCESS_STATUS="SECURE"
    fi
}

check_network() {
    local log="$LOGDIR/network-security.log"
    local latest=""

    if [ ! -f "$log" ]; then
        NETWORK_STATUS="UNKNOWN"
        set_state "WARNING" "Network security evidence unavailable"
        return
    fi

    latest="$(tail -n 100 "$log" | grep -E \
        "ALERT|BLOCK|SUSPICIOUS|VIOLATION|NORMAL|SECURE|COMPLETE" \
        | tail -n 1)"

    if [ -z "$latest" ]; then
        NETWORK_STATUS="SECURE"
        return
    fi

    if printf '%s\n' "$latest" | grep -qE \
        "ALERT|BLOCK|SUSPICIOUS|VIOLATION"; then
        NETWORK_STATUS="WARNING"
        set_state "WARNING" \
            "Latest network security event requires review"
    else
        NETWORK_STATUS="SECURE"
    fi
}

check_persistence() {
    local log="$LOGDIR/anti-persistence.log"

    if [ ! -f "$log" ]; then
        PERSISTENCE_STATUS="UNKNOWN"
        set_state "WARNING" "Anti-persistence evidence unavailable"
        return
    fi

    if tail -n 100 "$log" | grep -qE \
        "UNAUTHORIZED|SUSPICIOUS|PERSISTENCE_DETECTED"; then
        PERSISTENCE_STATUS="INCIDENT"
        set_state "INCIDENT" \
            "Suspicious persistence activity detected"
    else
        PERSISTENCE_STATUS="SECURE"
    fi
}

check_sensors() {
    local log="$LOGDIR/sensor-security.log"

    if [ ! -f "$log" ]; then
        SENSOR_STATUS="UNKNOWN"
        set_state "WARNING" "Sensor security evidence unavailable"
        return
    fi

    if tail -n 100 "$log" | grep -qE \
        "SECURITY_ALERT|UNAUTHORIZED|VIOLATION"; then
        SENSOR_STATUS="WARNING"
        set_state "WARNING" "Sensor security event detected"
    else
        SENSOR_STATUS="SECURE"
    fi
}

check_integrity
check_process
check_network
check_persistence
check_sensors

cat > "$STATEFILE" <<STATE
CHRONO_SECURITY_STATE_V1
timestamp=$TIMESTAMP
state=$STATE
reason=$REASON
process=$PROCESS_STATUS
network=$NETWORK_STATUS
integrity=$INTEGRITY_STATUS
anti_persistence=$PERSISTENCE_STATUS
sensors=$SENSOR_STATUS
ai_authority=DISABLED
human_security_policy_authority=ACTIVE
automatic_remediation=DISABLED
STATE

echo "$TIMESTAMP | SECURITY_STATE | $STATE | $REASON" >> "$LOGFILE"

echo "===== CHRONO SECURITY STATE ENGINE ====="
echo "Status: ACTIVE"
echo "Global Security State: $STATE"
echo "Reason: $REASON"
echo ""
echo "----- COMPONENT STATES -----"
echo "Process Guard:        $PROCESS_STATUS"
echo "Network Guard:        $NETWORK_STATUS"
echo "Integrity Guard:      $INTEGRITY_STATUS"
echo "Anti-Persistence:    $PERSISTENCE_STATUS"
echo "Sensor Guard:         $SENSOR_STATUS"
echo ""
echo "----- AUTHORITY -----"
echo "AI System Authority: DISABLED"
echo "Human Policy Authority: ACTIVE"
echo "Automatic Remediation: DISABLED"
echo ""
echo "State file: $STATEFILE"
echo "Security log: $LOGFILE"
echo ""
echo "Chrono Security State Engine: COMPLETE"
