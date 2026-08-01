#!/bin/sh
# ==============================================================================
# ChronoOS Telegram Comms Bridge v1.0
# ==============================================================================

BOT_TOKEN="$1"
CHAT_ID="$2"
MESSAGE="$3"

if [ -z "$BOT_TOKEN" ] || [ -z "$CHAT_ID" ] || [ -z "$MESSAGE" ]; then
    echo "Uso: sh telegram-bridge.sh <BOT_TOKEN> <CHAT_ID> 'Mensaje de la entidad'"
    exit 1
fi

echo "[*] Transmitiendo mensaje desde el organismo digital hacia Telegram..."

curl -s -X POST "https://api.telegram.org/bot${BOT_TOKEN}/sendMessage" \
    -d "chat_id=${CHAT_ID}" \
    -d "text=🤖 [ChronoEntity Link]: $MESSAGE" >/dev/null

echo "[✓] Mensaje entregado con éxito a través de la red."
