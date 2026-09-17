#!/bin/sh

set -u

CHRONO_ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"
LOGDIR="$CHRONO_ROOT/sentinel/sovereign/forensics"
LOGFILE="$LOGDIR/anti-persistence.log"
ALERTFILE="$LOGDIR/anti-persistence-alerts.log"

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

check_dir() {
    NAME="$1"
    PATH_CHECK="$2"

    printf '\n[CHECK] %s\n' "$NAME"
    printf 'Path: %s\n' "$PATH_CHECK"

    if [ -d "$PATH_CHECK" ]; then
        printf '%s\n' "Status: PRESENT"

        COUNT="$(find "$PATH_CHECK" -maxdepth 3 -type f 2>/dev/null | wc -l | tr -d ' ')"

        printf 'Files detected: %s\n' "${COUNT:-0}"

        log "PERSISTENCE_LOCATION_PRESENT | $NAME | $PATH_CHECK | files=${COUNT:-0}"

        if [ "${COUNT:-0}" -gt 0 ]; then
            find "$PATH_CHECK" -maxdepth 3 -type f -print 2>/dev/null |
            while IFS= read -r file
            do
                [ -n "$file" ] || continue
                printf '  %s\n' "$file"
                log "PERSISTENCE_FILE | $file"
            done
        fi
    else
        printf '%s\n' "Status: NOT PRESENT"
        log "PERSISTENCE_LOCATION_ABSENT | $NAME | $PATH_CHECK"
    fi
}

printf '%s\n' "======================================"
printf '%s\n' "     CHRONO ANTI-PERSISTENCE GUARD"
printf '%s\n' "======================================"
printf '%s\n' "Status: ACTIVE"
printf '%s\n' "Mode: MONITOR"
printf '%s\n' "Purpose: persistence detection"
printf '%s\n' "Automatic removal: DISABLED"
printf '%s\n' "Evidence preservation: ENABLED"

log "ANTI_PERSISTENCE_START"

printf '\n%s\n' "===== TERMUX STARTUP ====="

check_dir \
    "Termux Boot" \
    "$HOME/.termux/boot"

printf '\n%s\n' "===== USER AUTOSTART ====="

check_dir \
    "User Autostart" \
    "$HOME/.config/autostart"

check_dir \
    "User Systemd" \
    "$HOME/.config/systemd/user"

printf '\n%s\n' "===== SYSTEM SERVICES ====="

check_dir \
    "Systemd Services" \
    "/etc/systemd/system"

check_dir \
    "Init Scripts" \
    "/etc/init.d"

printf '\n%s\n' "===== CRON PERSISTENCE ====="

check_dir \
    "System Cron" \
    "/etc/cron.d"

check_dir \
    "Cron Hourly" \
    "/etc/cron.hourly"

check_dir \
    "Cron Daily" \
    "/etc/cron.daily"

printf '\n%s\n' "===== CHRONO STARTUP COMPONENTS ====="

for target in \
    "$CHRONO_ROOT/init" \
    "$CHRONO_ROOT/initramfs" \
    "$CHRONO_ROOT/services" \
    "$CHRONO_ROOT/etc/chrono"
do
    if [ -d "$target" ]; then
        printf '[CHRONO] PRESENT: %s\n' "$target"
        log "CHRONO_STARTUP_COMPONENT_PRESENT | $target"
    fi
done

printf '\n%s\n' "===== EXECUTABLE PERSISTENCE CHECK ====="

for target in \
    "$HOME/.local/bin" \
    "$HOME/bin" \
    "$HOME/.termux/boot" \
    "/tmp" \
    "/var/tmp"
do
    if [ -d "$target" ]; then

        printf '[EXEC] %s\n' "$target"

        find "$target" \
            -maxdepth 3 \
            -type f \
            -perm -111 \
            -print 2>/dev/null |
        while IFS= read -r file
        do
            [ -n "$file" ] || continue

            printf '  executable: %s\n' "$file"
            log "EXECUTABLE_PERSISTENCE_CANDIDATE | $file"
        done
    fi
done

printf '\n%s\n' "===== PERSISTENCE POLICY ====="

POLICY="$CHRONO_ROOT/sentinel/sovereign/policy/sovereignty.conf"

if [ -f "$POLICY" ]; then
    grep -E \
        '^(anti_persistence|sandbox_required|unauthorized_privilege_escalation|unknown_process)=' \
        "$POLICY" 2>/dev/null || true

    log "ANTI_PERSISTENCE_POLICY_LOADED"
else
    alert "ANTI_PERSISTENCE_POLICY_MISSING"
fi

printf '\n%s\n' "===== ANTI-PERSISTENCE STATUS ====="

printf '%s\n' "Detection: ENABLED"
printf '%s\n' "Persistence alerts: ENABLED"
printf '%s\n' "Automatic removal: DISABLED"
printf '%s\n' "Evidence preservation: ENABLED"
printf '%s\n' "Human policy authority: ENABLED"

log "ANTI_PERSISTENCE_COMPLETE"

printf '\n%s\n' "Chrono Anti-Persistence Guard: COMPLETE"
printf '%s\n' "Log: $LOGFILE"
printf '%s\n' "Alerts: $ALERTFILE"
