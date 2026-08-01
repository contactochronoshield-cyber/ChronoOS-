#!/bin/sh
# ==============================================================================
# ChronoOS Sentinel: Active Network Air-Gap & Interface Auditor v1.0
# ==============================================================================

CYAN='\033[1;36m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
RED='\033[1;31m'
NC='\033[0m'

echo -e "${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║         CHRONOOS SENTINEL: ACTIVE NETWORK DEFENDER           ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""

echo -e "${YELLOW}[*] Escaneando interfaces de red activas...${NC}"
if command -v ip >/dev/null 2>&1; then
    ip -o link show | awk -F': ' '{print "    [Interface Detectada] -> " $2}'
else
    ifconfig -a 2>/dev/null | grep "^[a-zA-Z]" | awk '{print "    [Interface] -> " $1}'
fi

echo -e "\n${YELLOW}[*] Verificando rutas de enlace predeterminadas (Gateway)...${NC}"
DEFAULT_GW=$(ip route show 0/0 2>/dev/null | awk '{print $3}' || route -n 2>/dev/null | grep '^0.0.0.0' | awk '{print $2}')
if [ -n "$DEFAULT_GW" ]; then
    echo -e "${GREEN}    [+] Gateway Activo detectado en: $DEFAULT_GW${NC}"
else
    echo -e "${RED}    [!] Alerta: No se detecta pasarela activa (Entorno Air-Gapped / Aislado)${NC}"
fi

echo -e "\n${YELLOW}[*] Estado de conexiones establecidas en sockets locales:${NC}"
netstat -an 2>/dev/null | grep ESTABLISHED | head -n 5 || echo -e "    [i] Sockets bajo control estricto o sin conexiones externas activas."

echo -e "\n${GREEN}[✓] Auditoría de red Sentinel completada con éxito.${NC}"
