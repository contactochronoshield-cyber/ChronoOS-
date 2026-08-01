#!/bin/sh
# ==============================================================================
# ChronoOS - Bare-Metal Disk Installer
# Author: Daniel Gonzales / Chrono Shield Networks
# Description: Instalador nativo de ChronoOS en disco físico (estilo Debian/Ubuntu).
# ==============================================================================

if [ "$(id -u)" -ne 0 ]; then
    echo "[!] Error: Este script debe ejecutarse como root (sudo)."
    exit 1
fi

TARGET_DISK=$1

if [ -z "$TARGET_DISK" ]; then
    echo "Uso: sudo ./scripts/install_disk.sh /dev/sdX"
    echo "Dispositivos de almacenamiento disponibles:"
    lsblk -d -o NAME,SIZE,MODEL
    exit 1
fi

if [ ! -b "$TARGET_DISK" ]; then
    echo "[!] Error: El dispositivo $TARGET_DISK no es un disco válido."
    exit 1
fi

echo "===================================================================="
echo "[!] ADVERTENCIA: Todos los datos en $TARGET_DISK serán BORRADOS."
echo "===================================================================="
printf "Estás a punto de instalar ChronoOS en %s. ¿Continuar? (y/N): " "$TARGET_DISK"
read -r CONFIRM

if [ "$CONFIRM" != "y" ] && [ "$CONFIRM" != "Y" ]; then
    echo "[*] Instalación cancelada por el usuario."
    exit 0
fi

echo "[*] Preparando particiones en $TARGET_DISK..."

# Limpiar tabla de particiones y crear esquema GPT básico con partición raíz ext4
parted -s "$TARGET_DISK" mklabel gpt
parted -s "$TARGET_DISK" mkpart primary ext4 1MiB 100%

# Detectar partición creada (ej. /dev/sda1 o /dev/nvme0n1p1)
if echo "$TARGET_DISK" | grep -q "nvme"; then
    PARTITION="${TARGET_DISK}p1"
else
    PARTITION="${TARGET_DISK}1"
fi

echo "[*] Formateando $PARTITION en ext4..."
mkfs.ext4 -F "$PARTITION"

MOUNT_DIR="/mnt/chrono-target"
mkdir -p "$MOUNT_DIR"
mount "$PARTITION" "$MOUNT_DIR"

echo "[*] Copiando el sistema de archivos raíz de ChronoOS..."
if [ -d "build_root/rootfs" ]; then
    cp -a build_root/rootfs/* "$MOUNT_DIR/"
else
    echo "[!] Error: No se encuentra 'build_root/rootfs'. Asegúrate de tener el rootfs preparado."
    umount "$MOUNT_DIR"
    exit 1
fi

echo "[*] Configurando puntos de montaje y fstab..."
echo "UUID=$(blkid -s UUID -o value "$PARTITION") / ext4 defaults 0 1" > "$MOUNT_DIR/etc/fstab"

echo "[*] Instalando GRUB Bootloader en $TARGET_DISK..."
grub-install --target=i386-pc --boot-directory="$MOUNT_DIR/boot" "$TARGET_DISK" 2>/dev/null || \
grub-install --target=x86_64-efi --efi-directory="$MOUNT_DIR/boot/efi" --boot-directory="$MOUNT_DIR/boot" "$TARGET_DISK" 2>/dev/null

echo "[✓] ¡Instalación de ChronoOS en $TARGET_DISK completada con éxito!"
echo "[*] Puedes desmontar o reiniciar hacia el nuevo sistema operativo soberano."

umount "$MOUNT_DIR"
