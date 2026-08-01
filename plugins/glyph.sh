#!/bin/sh
# ChronoOS Custom Terminal Glyphs (ChronoGlyphs)

# Cargar tema activo si existe
if [ -f "./themes/active.theme" ]; then
    . ./themes/active.theme
else
    COLOR_PRIMARY="\033[1;32m"
    COLOR_SECONDARY="\033[1;36m"
    COLOR_ALERT="\033[1;31m"
    COLOR_BG="\033[0m"
fi

TYPE="$1"

case "$TYPE" in
    "shield")
        printf "${COLOR_PRIMARY}[ 🛡️⚡ CHRONO-SEC ]${COLOR_BG}\n"
        ;;
    "vault")
        printf "${COLOR_SECONDARY}[ 🔒🗝️ CHRONO-VAULT ]${COLOR_BG}\n"
        ;;
    "panic")
        printf "${COLOR_ALERT}[ ☢️💥 CHRONO-PANIC ]${COLOR_BG}\n"
        ;;
    "pulse")
        printf "${COLOR_PRIMARY}[ 🌐🟢 SYSTEM-ONLINE ]${COLOR_BG}\n"
        ;;
    *)
        echo "Glyphs disponibles: shield, vault, panic, pulse"
        echo "Uso: sh plugins/glyph.sh <tipo>"
        ;;
es.
