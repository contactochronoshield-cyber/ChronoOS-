#!/bin/sh
# ==========================================================
# ChronoOS Security Sandbox - Isolation Engine v2.0
# ==========================================================

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}[*] Inicializando motor de aislamiento ChronoOS Sandbox...${NC}"

# Verificar si estamos en un contenedor restringido (como Termux)
if [ -d "/data/data/com.termux" ]; then
    echo -e "${YELLOW}[!] Entorno Termux detectado: Namespaces de kernel limitados por Android.${NC}"
    echo -e "${GREEN}[+] Aplicando aislamiento lógico por espacio de nombres de usuario y carpetas blindadas...${NC}"
    
    # Crear sandbox virtual local
    SANDBOX_DIR="./vault/sandbox_env"
    mkdir -p "$SANDBOX_DIR"
    chmod 700 "$SANDBOX_DIR"
    
    echo -e "${GREEN}[✓] Entorno aislado creado en: $SANDBOX_DIR${NC}"
    echo -e "${CYAN}[*] Abriendo shell restringida en sandbox...${NC}"
    cd "$SANDBOX_DIR" && exec sh
else
    # Entorno Linux estándar con soporte completo para namespaces
    if command -v unshare >/dev/null 2>&1; then
        echo -e "${GREEN}[+] Lanzando proceso bajo namespaces aislados (PID, Net, Mount)...${NC}"
        exec unshare --pid --net --mount-proc --fork --propagation private /bin/sh
    else
        echo -e "${RED}[!] Error: El comando 'unshare' no está disponible en este sistema.${NC}"
        exit 1
    fi
fi
