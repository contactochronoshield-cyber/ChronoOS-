#!/bin/sh
# ChronoOS - Behavior Correlator
# Correlaciona eventos del Ledger para detectar patrones de ataque
LEDGER="/mnt/data/chrono_ledger.log"
[ -f "$LEDGER" ] || LEDGER="./var/logs/chrono_ledger.log"

echo "[chrono-behavior-correlator] Correlacionando comportamiento..."
EVENTS=$(wc -l < "$LEDGER" 2>/dev/null || echo 0)
AUTH_FAILS=$(grep -c "AUTH_FAILED\|DENIED" "$LEDGER" 2>/dev/null || echo 0)
echo "[correlator] Eventos totales: $EVENTS | Fallos de auth: $AUTH_FAILS"

# Patron: mas de 5 fallos de auth = posible ataque
if [ "$AUTH_FAILS" -gt 5 ]; then
    echo "[correlator] PATRON DETECTADO: multiples fallos de autenticacion"
    exit 2
fi
exit 0
