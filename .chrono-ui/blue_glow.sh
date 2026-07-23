#!/bin/sh

# ==========================================
# ChronoOS Core Initialization Script
# ==========================================

# Definición de Colores (ANSI)
CYAN='\033[0;36m'
BLUE='\033[0;34m'
GREEN='\033[0;32m'
GRAY='\033[1;30m'
NC='\033[0m' # No Color

# 1. Función para obtener la IP Local de forma segura
get_local_ip() {
    ip_addr=""
    if command -v ip >/dev/null 2>&1; then
        ip_addr=$(ip addr show wlan0 2>/dev/null | awk '/inet / {print $2}' | cut -d/ -f1)
    fi
    if [ -z "$ip_addr" ] && command -v ifconfig >/dev/null 2>&1; then
        ip_addr=$(ifconfig wlan0 2>/dev/null | awk '/inet / {print $2}')
    fi
    if [ -z "$ip_addr" ]; then
        ip_addr="Offline / No Interface"
    fi
    echo "$ip_addr"
}

# 2. Renderizado del Banner ASCII
clear
cat << "EOF"
 ██████╗██╗  ██╗██████╗  ██████╗ ███╗   ██╗██████╗ 
██╔════╝██║  ██║██╔══██╗██╔═══██╗████╗  ██║██╔══██╗
██║     ███████║██████╔╝██║   ██║██╔██╗ ██║██║  ██║
██║     ██╔══██║██╔══██╗██║   ██║██║╚██╗██║██║  ██║
╚██████╗██║  ██║██║  ██║╚██████╔╝██║ ╚████║██████╔╝
 ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═════╝ 
EOF

cat << "EOF"
███████╗██╗  ██╗██╗███╗   ██╗██╗     ██████╗ 
██╔════╝██║  ██║██║████╗  ██║██║     ██╔══██╗
███████╗███████║██║██╔██╗ ██║██║     ██║  ██║
╚════██║██╔══██║██║██║╚██╗██║██║     ██║  ██║
███████║██║  ██║██║██║ ╚████║███████╗██████╔╝
╚══════╝╚═╝  ╚═╝╚═╝╚═╝  ╚═══╝╚══════╝╚═════╝ 
EOF

echo ""
echo "  ✦ CHRONO SHIELD NETWORKS ✦"
echo "  ✦ DIGITAL SOVEREIGNTY MODE ✦"
echo "  ✦ POST-INTERNET SYSTEM (ChronoOS) ✦"
echo ""

# 3. Métrica y Estado de ChronoOS
USUARIO=$(whoami)
FECHA=$(date +"%a %b %d %T %Z %Y")
IP_LOCAL=$(get_local_ip)
ARQ=$(uname -m)

echo -e "${BLUE}OS Core:${NC}      ChronoOS Kernel v1.0 (${ARQ})"
echo -e "${BLUE}Usuario:${NC}      ${USUARIO}"
echo -e "${BLUE}Fecha:${NC}        ${FECHA}"
echo -e "${BLUE}IP Local:${NC}     ${GREEN}${IP_LOCAL}${NC}"
echo -e "${GRAY}--------------------------------------------------${NC}"

