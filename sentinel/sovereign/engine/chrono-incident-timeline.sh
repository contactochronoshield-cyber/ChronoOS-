#!/bin/sh

set -eu

BASE="sentinel/sovereign"
FORENSICS="$BASE/forensics"
STATE="$BASE/state"

TIMELINE="$FORENSICS/incident-timeline.log"
INCIDENTS="$FORENSICS/incidents.log"
TIMELINE_STATE="$STATE/incident-timeline-state"

mkdir -p "$FORENSICS" "$STATE"

timestamp() {
    date -u '+%Y-%m-%dT%H:%M:%SZ'
}

printf '%s\n' "======================================"
printf '%s\n' "       CHRONO INCIDENT TIMELINE"
printf '%s\n' "======================================"
printf '%s\n' "Mode: FORENSIC TIMELINE"
printf '%s\n' "Evidence preservation: ENABLED"
printf '%s\n' "Automatic remediation: DISABLED"
printf '%s\n' ""

if [ ! -f "$INCIDENTS" ]; then
    printf '%s\n' "No incident records available."
    printf '%s\n' "State: EMPTY"

    {
        printf '%s\n' "CHRONO_INCIDENT_TIMELINE_V1"
        printf 'timestamp=%s\n' "$(timestamp)"
        printf 'state=EMPTY\n'
        printf 'incidents=0\n'
    } > "$TIMELINE_STATE"

    exit 0
fi

COUNT=$(wc -l < "$INCIDENTS" | tr -d ' ')

printf '%s\n' "Incident records: $COUNT"
printf '%s\n' ""

printf '%s\n' "===== TIMELINE ====="

while IFS= read -r INCIDENT; do
    [ -n "$INCIDENT" ] || continue

    EVENT_TIME=$(printf '%s\n' "$INCIDENT" | awk -F' \| ' '{print $1}')
    EVENT_ID=$(printf '%s\n' "$INCIDENT" | awk -F' \| ' '{print $2}')
    SEVERITY=$(printf '%s\n' "$INCIDENT" | awk -F' \| ' '{print $3}')
    SOURCE=$(printf '%s\n' "$INCIDENT" | awk -F' \| ' '{print $4}')
    DESCRIPTION=$(printf '%s\n' "$INCIDENT" | awk -F' \| ' '{print $5}')

    ENTRY="$EVENT_TIME | $EVENT_ID | $SEVERITY | $SOURCE | $DESCRIPTION"

    printf '%s\n' "$ENTRY"
    printf '%s\n' "$ENTRY" >> "$TIMELINE"

done < "$INCIDENTS"

{
    printf '%s\n' "CHRONO_INCIDENT_TIMELINE_V1"
    printf 'timestamp=%s\n' "$(timestamp)"
    printf 'state=RECORDED\n'
    printf 'incidents=%s\n' "$COUNT"
} > "$TIMELINE_STATE"

printf '%s\n' ""
printf '%s\n' "Timeline state: RECORDED"
printf '%s\n' "Timeline: $TIMELINE"
printf '%s\n' "Chrono Incident Timeline: COMPLETE"

exit 0
