#!/usr/bin/env bash
set -euo pipefail

INITRAMFS_DIR="initramfs"
BUSYBOX_BIN="$INITRAMFS_DIR/bin/busybox"

echo "[*] Preparando estructura de directorios del sistema operativo..."
mkdir -p "$INITRAMFS_DIR/bin" "$INITRAMFS_DIR/sbin" "$INITRAMFS_DIR/etc" \
         "$INITRAMFS_DIR/proc" "$INITRAMFS_DIR/sys" "$INITRAMFS_DIR/dev" \
         "$INITRAMFS_DIR/root" "$INITRAMFS_DIR/tmp"

# Descargar BusyBox estático oficial si no existe
if [ ! -f "$BUSYBOX_BIN" ]; then
    echo "[*] Descargando binario estático de BusyBox (Core Utils de Linux)..."
    # Detectar arquitectura o usar x86_64 por defecto para contenedores/QEMU
    curl -L https://busybox.net/downloads/binaries/1.35.0-x86_64-linux-musl/busybox -o "$BUSYBOX_BIN"
    chmod +x "$BUSYBOX_BIN"
fi

echo "[*] Enlazando enlaces simbólicos del sistema operativo (sh, ls, mount, ip, clear)..."
cd "$INITRAMFS_DIR/bin"
for cmd in sh ls cat mount umount ps kill mkdir touch grep ip clear reboot poweroff; do
    ln -sf busybox "$cmd"
done
cd - > /dev/null

echo "[✔] Sistema de archivos raíz (Rootfs) poblado con éxito con binarios reales."
