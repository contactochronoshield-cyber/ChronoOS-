#!/bin/bash
# Script de Respaldo de Emergencia para ChronoOS (Evento Carrington)

BACKUP_DIR="/vault/carrinton_safe"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
TARGET_ARCHIVE="$BACKUP_DIR/chrono_core_snapshot_$TIMESTAMP.tar.gz"

mkdir -p "$BACKUP_DIR"

echo "[ChronoGuard] Empaquetando estado crítico del sistema para almacenamiento en frío..."
tar --exclude='/var/logs/*' -czf "$TARGET_ARCHIVE" /core /etc/chrono /vault/vaults

# Asegurar permisos de solo lectura para evitar corrupción por reinicios anómalos
chmod 444 "$TARGET_ARCHIVE"
echo "[ChronoGuard] Snapshot seguro generado en: $TARGET_ARCHIVE"
