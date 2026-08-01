#!/bin/sh
SELECTED="$1"
CONFIG_FILE="./themes/active.theme"

if [ -f "./themes/${SELECTED}.theme" ]; then
    cp "./themes/${SELECTED}.theme" "$CONFIG_FILE"
    echo "[✓] Tema aplicado con éxito: $SELECTED"
else
    echo "Temas disponibles: cyberpunk, monolith, neon, amber, arctic, bloodmoon, emerald"
    echo "Uso: sh themes/set-theme.sh <nombre_tema>"
fi
