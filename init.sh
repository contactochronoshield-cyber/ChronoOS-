#!/bin/sh
# ==============================================================================
# ChronoOS Master Initialization & Environment Bootstrapper v5.0
# Optimizado para Termux, Android y Sistemas POSIX
# ==============================================================================

CYAN='\033[1;36m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
RED='\033[1;31m'
NC='\033[0m'

echo -e "${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║         CHRONOOS: MASTER ENVIRONMENT INITIALIZATION          ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}"

# 1. Verificación de entorno de ejecución
if [ -d "/data/data/com.termux" ]; then
    echo -e "${GREEN}[+] Entorno móvil detectado: Termux / Android System${NC}"
    export CHRONO_ROOT="$HOME/chrono-os"
else
    echo -e "${GREEN}[+] Entorno estándar POSIX / Linux detectado${NC}"
    export CHRONO_ROOT="$(pwd)"
fi

# 2. Creación de estructura de directorios críticos
echo -e "${YELLOW}[*] Verificando y construyendo directorios de almacenamiento seguro...${NC}"
mkdir -p "$CHRONO_ROOT/vault" \
         "$CHRONO_ROOT/crypto_keys" \
         "$CHRONO_ROOT/net_state/sentinel" \
         "$CHRONO_ROOT/initramfs/bin" \
         "$CHRONO_ROOT/logs"

# Asegurar permisos estrictos en carpetas de bóvedas (Solo lectura/escritura para el propietario)
chmod 700 "$CHRONO_ROOT/vault"
chmod 700 "$CHRONO_ROOT/crypto_keys"

echo -e "${GREEN}[✓] Estructura de directorios blindados configurada correctamente.${NC}"

# 3. Verificación de dependencias esenciales
echo -e "${YELLOW}[*] Validando herramientas del sistema (git, openssl, find, wc)...${NC}"
for cmd in git openssl find wc; do
    if command -v "$cmd" >/dev/null 2>&1; then
        echo -e "    [Found] -> $cmd"
    else
        echo -e "${RED}    [Missing] -> $cmd (Se recomienda instalarlo vía pkg install o apt)${NC}"
    fi
done

echo -e "\n${GREEN}[✓] ChronoOS inicializado con éxito y listo para operación avanzada.${NC}"
