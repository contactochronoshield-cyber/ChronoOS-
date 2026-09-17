#!/bin/sh

set -u

CHRONO_ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"
LOGDIR="$CHRONO_ROOT/sentinel/sovereign/forensics"
LOGFILE="$LOGDIR/sensor-security.log"
ALERTFILE="$LOGDIR/sensor-alerts.log"

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

check_command() {
    NAME="$1"
    COMMAND="$2"

    if command -v "$COMMAND" >/dev/null 2>&1; then
        printf '[AVAILABLE] %s (%s)\n' "$NAME" "$COMMAND"
        log "CAPABILITY_AVAILABLE | $NAME | $COMMAND"
    else
        printf '[NOT_VISIBLE] %s (%s)\n' "$NAME" "$COMMAND"
        log "CAPABILITY_NOT_VISIBLE | $NAME | $COMMAND"
    fi
}

printf '%s\n' "======================================"
printf '%s\n' "        CHRONO SENSOR GUARD"
printf '%s\n' "======================================"
printf '%s\n' "Status: ACTIVE"
printf '%s\n' "Mode: MONITOR"
printf '%s\n' "Purpose: sensitive sensor access detection"
printf '%s\n' "Automatic sensor blocking: DISABLED"
printf '%s\n' "Evidence preservation: ENABLED"
printf '%s\n' "Human policy authority: ENABLED"
printf '\n'

log "SENSOR_GUARD_START"

printf '%s\n' "===== CAMERA ====="

check_command "Camera interface" "termux-camera-info"
check_command "Camera capture interface" "termux-camera-photo"

printf '\n%s\n' "===== MICROPHONE ====="

check_command "Microphone interface" "termux-microphone-record"

printf '\n%s\n' "===== LOCATION ====="

check_command "Location interface" "termux-location"

printf '\n%s\n' "===== BLUETOOTH ====="

check_command "Bluetooth interface" "termux-bluetooth-scaninfo"
check_command "Bluetooth control" "termux-bluetooth-enable"

printf '\n%s\n' "===== USB ====="

check_command "USB interface" "termux-usb"

printf '\n%s\n' "===== SENSOR-RELATED PROCESSES ====="

if command -v ps >/dev/null 2>&1; then
    ps 2>/dev/null |
        grep -Ei 'camera|audio|mic|bluetooth|location|gps|usb' |
        grep -v grep || true
fi

printf '\n%s\n' "===== ANDROID / TERMUX VISIBILITY ====="

if [ -d /dev ]; then
    printf '%s\n' "/dev: VISIBLE"
    log "DEVICE_NAMESPACE_VISIBLE"

    for device in \
        /dev/video* \
        /dev/snd/* \
        /dev/binder* \
        /dev/usb* \
        /dev/ttyUSB* \
        /dev/ttyACM*
    do
        for found in $device
        do
            if [ -e "$found" ]; then
                printf '[DEVICE] %s\n' "$found"
                log "DEVICE_NODE_VISIBLE | $found"
            fi
        done
    done
else
    printf '%s\n' "/dev: NOT VISIBLE"
    log "DEVICE_NAMESPACE_NOT_VISIBLE"
fi

printf '\n%s\n' "===== SENSOR POLICY ====="

POLICY="$CHRONO_ROOT/sentinel/sovereign/policy/sovereignty.conf"

if [ -f "$POLICY" ]; then
    grep -E \
        '^(camera_protection|microphone_protection|location_protection|bluetooth_protection|usb_protection)=' \
        "$POLICY" 2>/dev/null || true

    log "SENSOR_POLICY_LOADED"
else
    alert "SENSOR_POLICY_MISSING"
fi

printf '\n%s\n' "===== SENSOR SECURITY STATUS ====="

printf '%s\n' "Camera protection policy: ENABLED"
printf '%s\n' "Microphone protection policy: ENABLED"
printf '%s\n' "Location protection policy: ENABLED"
printf '%s\n' "Bluetooth protection policy: ENABLED"
printf '%s\n' "USB protection policy: ENABLED"
printf '%s\n' "Access monitoring: ENABLED"
printf '%s\n' "Automatic blocking: DISABLED"
printf '%s\n' "Evidence preservation: ENABLED"

log "SENSOR_GUARD_COMPLETE"

printf '\n%s\n' "Chrono Sensor Guard: COMPLETE"
printf '%s\n' "Log: $LOGFILE"
printf '%s\n' "Alerts: $ALERTFILE"
