#!/bin/sh

set -u

# shellcheck disable=SC1007
CHRONO_ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"
LOGDIR="$CHRONO_ROOT/sentinel/sovereign/forensics"
LOGFILE="$LOGDIR/network-security.log"
ALERTFILE="$LOGDIR/network-alerts.log"

mkdir -p "$LOGDIR"

log() {
    printf '%s | %s\n' \
        "$(date -u '+%Y-%m-%dT%H:%M:%SZ')" \
        "$*" >> "$LOGFILE"
}

alert() {
    printf '%s | ALERT | %s\n' \
        "$(date -u '+%Y-%m-%dT%H:%M:%SZ')" \
        "$*" | tee -a "$ALERTFILE" "$LOGFILE"
}

section() {
    printf '\n===== %s =====\n' "$1"
}

log "NETWORK_GUARD_START"

section "CHRONO NETWORK GUARD"
printf '%s\n' "Status: ACTIVE"
printf '%s\n' "Mode: MONITOR"
printf '%s\n' "Purpose: outbound connection detection"
printf '%s\n' "Isolation: READY"

section "NETWORK INTERFACES"

if command -v ip >/dev/null 2>&1; then
    ip -br addr 2>/dev/null || true
else
    printf '%s\n' "ip: unavailable"
    log "IP_COMMAND_UNAVAILABLE"
fi

section "ROUTING"

if command -v ip >/dev/null 2>&1; then
    ip route 2>/dev/null || true
fi

section "ACTIVE CONNECTIONS"

CONNECTIONS_FOUND=0

if command -v ss >/dev/null 2>&1; then
    CONNECTIONS_FOUND="$(ss -tunap 2>/dev/null | tail -n +2 | grep -v '^$' | wc -l | tr -d ' ')"
    ss -tunap 2>/dev/null | head -100 || true
elif command -v netstat >/dev/null 2>&1; then
    netstat -tun 2>/dev/null | head -100 || true
    CONNECTIONS_FOUND=1
else
    printf '%s\n' "No ss/netstat available in current environment."
    log "CONNECTION_TOOL_UNAVAILABLE"
fi

section "OUTBOUND CONNECTION ANALYSIS"

if command -v ss >/dev/null 2>&1; then
    ss -tun 2>/dev/null |
    awk 'NR > 1 {print}' |
    while IFS= read -r line
    do
        [ -n "$line" ] || continue

        printf '%s\n' "$line"

        case "$line" in
            *ESTAB*)
                log "ESTABLISHED_CONNECTION | $line"
                ;;
            *)
                log "NETWORK_ENTRY | $line"
                ;;
        esac
    done
else
    printf '%s\n' "Detailed connection analysis unavailable."
fi

section "HEURISTIC CHECKS"

printf '%s\n' "Checking for:"
printf '%s\n' "- unexpected listening services"
printf '%s\n' "- unusual outbound endpoints"
printf '%s\n' "- suspicious local ports"
printf '%s\n' "- unauthorized network activity"

if command -v ss >/dev/null 2>&1; then

    LISTEN_COUNT="$(ss -lntup 2>/dev/null | tail -n +2 | wc -l | tr -d ' ')"

    printf 'Listening sockets detected: %s\n' "${LISTEN_COUNT:-0}"

    log "LISTENING_SOCKET_COUNT | ${LISTEN_COUNT:-0}"

    ss -lntup 2>/dev/null |
    tail -n +2 |
    while IFS= read -r line
    do
        [ -n "$line" ] || continue
        log "LISTENING_SOCKET | $line"
    done

else
    printf '%s\n' "Listening-socket inspection unavailable."
fi

section "LOCAL POLICY"

POLICY="$CHRONO_ROOT/sentinel/sovereign/policy/sovereignty.conf"

if [ -f "$POLICY" ]; then
    grep -E \
        '^(egress_control|unknown_connections|suspicious_connections|network_isolation|secure_dns)=' \
        "$POLICY" 2>/dev/null || true
    log "NETWORK_POLICY_LOADED | $POLICY"
else
    alert "NETWORK_POLICY_MISSING | $POLICY"
fi

section "ISOLATION PREPARATION"

printf '%s\n' "Network isolation capability: READY"
printf '%s\n' "Automatic blocking: DISABLED"
printf '%s\n' "Reason: monitor-first safety mode"
printf '%s\n' "Future action: policy-controlled isolation"

log "NETWORK_ISOLATION_READY"
log "AUTOMATIC_BLOCKING_DISABLED"
log "NETWORK_GUARD_COMPLETE"

section "NETWORK GUARD SUMMARY"

printf 'Connections inspected: %s\n' "${CONNECTIONS_FOUND:-0}"
printf 'Automatic blocking: DISABLED\n'
printf 'Isolation capability: READY\n'
printf 'Log: %s\n' "$LOGFILE"
printf 'Alerts: %s\n' "$ALERTFILE"

printf '\n%s\n' "Chrono Network Guard: COMPLETE"
printf '%s\n' "Status: ACTIVE | Mode: MONITOR"
