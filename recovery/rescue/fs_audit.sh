#!/bin/bash
# Auditoría de integridad de bloques y sistemas de archivos tras un apagón
echo "[ChronoRecovery] Verificando integridad de particiones raíz..."
fsck -y /dev/root || {
    echo "[ALERTA] Daño estructural detectado en partición. Aplicando rollback a la última snapshot limpia..."
    /recovery/rollback/restore_last.sh
}
