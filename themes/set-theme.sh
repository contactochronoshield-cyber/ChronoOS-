#!/bin/sh
SELECTED="$1"
CONFIG_FILE="./themes/active.theme"

if [ "$SELECTED" = "cyberpunk" ] || [ "$SELECTED" = "monolith" ]; then
    cp "./themes/${SELECTED}.theme" "$CONFIG_FILE"
    echo "[✓] Tema aplicado con éxito: $SELECTED"
else
    echo "Uso: sh themes/set-theme.sh [cyberpunk|monolith]"
fi
