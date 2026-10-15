#!/bin/sh
# ChronoOS - Threat Engine
# Detecta amenazas en el Ledger y las clasifica por severidad
LEDGER="/mnt/data/chrono_ledger.log"
[ -f "$LEDGER" ] || LEDGER="./var/logs/chrono_ledger.log"

echo "[chrono-threat-engine] Analizando amenazas..."
CRITICAL=$(grep -c "CRITICAL\|PANIC\|BLOCKED" "$LEDGER" 2>/dev/null || echo 0)
WARNING=$(grep -c "WARNING\|ANOMALY\|FAILED" "$LEDGER" 2>/dev/null || echo 0)
echo "[threat-engine] Criticas: $CRITICAL | Advertencias: $WARNING"
exit 0
