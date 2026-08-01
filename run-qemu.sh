#!/bin/bash
# ==========================================================
# ChronoOS QEMU Virtual Machine Launcher
# ==========================================================

CYAN='\033[1;36m'
GREEN='\033[1;32m'
RED='\033[1;31m'
NC='\033[0m'

echo -e "${CYAN}[*] Verificando disponibilidad de QEMU...${NC}"
if ! command -v qemu-system-x86_64 >/dev/null 2>&1; then
    echo -e "${RED}[!] Error: qemu-system-x86_64 no está instalado en el sistema.${NC}"
    exit 1
fi

echo -e "${GREEN}[+] Lanzando entorno virtualizado ChronoOS en QEMU...${NC}"
qemu-system-x86_64 \
    -m 512M \
    -smp 2 \
    -nographic \
    -net nic -net user
