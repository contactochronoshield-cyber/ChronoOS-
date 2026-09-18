#!/bin/sh

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/../../.." && pwd)

BIN="$REPO_ROOT/bin"
THREAT="${1:-}"
SEVERITY="${2:-UNKNOWN}"

if [ -z "$THREAT" ]; then
    echo "[ERROR] Threat type requerido"
    echo "Uso: $0 THREAT [SEVERITY]"
    exit 2
fi

ledger() {
    if [ -x "$BIN/chrono-ledger" ]; then
        "$BIN/chrono-ledger" append "$1" "$2" >/dev/null 2>&1
        return $?
    fi
    return 1
}

case "$THREAT" in

    USB_UNAUTHORIZED)
        echo "[RESPONSE] USB no autorizado: evento registrado"
        ledger "USB_UNAUTHORIZED" "severity=$SEVERITY action=record_only"
        ;;

    GAS_CRITICAL)
        echo "[RESPONSE] Gas critico: solicitando notificacion de evacuacion"
        ledger "GAS_CRITICAL" "severity=$SEVERITY action=notify"
        if [ -x "$BIN/chrono-community-alert" ]; then
            "$BIN/chrono-community-alert" send \
                EVACUACION \
                "Gas critico detectado" \
                "Sentinel" >/dev/null 2>&1 || \
                echo "[WARN] Notificador no disponible o fallo"
        else
            echo "[WARN] chrono-community-alert no disponible"
        fi
        ;;

    MODBUS_ANOMALY)
        echo "[RESPONSE] Anomalia Modbus: evento registrado; no se ejecuta bloqueo PLC automaticamente"
        ledger "MODBUS_ANOMALY" "severity=$SEVERITY action=record_only"
        ;;

    PANIC)
        echo "[RESPONSE] PANIC requiere autorizacion explicita"
        echo "[RESPONSE] No se ejecuta protocolo destructivo automaticamente"
        ledger "PANIC_REQUESTED" "severity=$SEVERITY action=authorization_required"
        exit 3
        ;;

    *)
        echo "[RESPONSE] Amenaza desconocida: registrando"
        ledger "UNKNOWN_THREAT" "severity=$SEVERITY threat=$THREAT"
        ;;
esac
