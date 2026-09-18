#!/bin/sh

set -u

# shellcheck disable=SC1007
CHRONO_ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"
LOGDIR="$CHRONO_ROOT/sentinel/sovereign/forensics"
LOGFILE="$LOGDIR/process-security.log"

mkdir -p "$LOGDIR"

log() {
    printf '%s | %s\n' \
        "$(date -u '+%Y-%m-%dT%H:%M:%SZ')" \
        "$*" >> "$LOGFILE"
}

printf '%s\n' "===== CHRONO PROCESS GUARD ====="
printf '%s\n' "Mode: MONITOR"
printf '%s\n' "Purpose: process and persistence detection"
printf '\n'

log "PROCESS_GUARD_START"

printf '%s\n' "----- RUNNING PROCESSES -----"

if command -v ps >/dev/null 2>&1; then
    ps -A 2>/dev/null | head -80
else
    printf '%s\n' "ps: unavailable"
    log "PS_UNAVAILABLE"
fi

printf '\n%s\n' "----- PERSISTENCE LOCATIONS -----"

for path in \
    "$HOME/.termux/boot" \
    "$HOME/.config/autostart" \
    "$HOME/.config/systemd/user" \
    "/etc/systemd/system" \
    "/etc/init.d"
do
    if [ -d "$path" ]; then
        printf '%s\n' "[CHECK] $path"
        find "$path" -maxdepth 2 -type f -print 2>/dev/null
        log "PERSISTENCE_PATH_CHECKED | $path"
    fi
done

printf '\n%s\n' "----- SUSPICIOUS EXECUTABLE LOCATIONS -----"

for path in \
    "$HOME/.local/bin" \
    "$HOME/bin" \
    "/tmp" \
    "/var/tmp"
do
    if [ -d "$path" ]; then
        printf '%s\n' "[CHECK] $path"
        find "$path" -maxdepth 2 -type f -perm -111 -print 2>/dev/null
        log "EXECUTABLE_PATH_CHECKED | $path"
    fi
done

printf '\n%s\n' "----- ENVIRONMENT -----"

printf 'USER=%s\n' "${USER:-unknown}"
printf 'HOME=%s\n' "${HOME:-unknown}"
printf 'SHELL=%s\n' "${SHELL:-unknown}"

log "PROCESS_GUARD_COMPLETE"

printf '\n%s\n' "Chrono Process Guard: COMPLETE"
printf '%s\n' "Log: $LOGFILE"
