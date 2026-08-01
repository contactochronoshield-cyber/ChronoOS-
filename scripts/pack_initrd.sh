#!/bin/sh
# ==============================================================================
# ChronoOS - Initrd Packer Engine
# Author: Daniel Gonzales / Chrono Shield Networks
# Description: Empaqueta el árbol del initramfs en una imagen initrd.img (cpio.gz)
#              lista para arquitecturas de arranque en memoria.
# ==============================================================================

INITRAMFS_DIR="./initramfs"
OUTPUT_DIR="./build"
OUTPUT_IMG="$OUTPUT_DIR/initrd.img"

if [ ! -d "$INITRAMFS_DIR" ]; then
    echo "[!] Error: No se encuentra el directorio '$INITRAMFS_DIR'."
    exit 1
fi

mkdir -p "$OUTPUT_DIR"

echo "[*] Empaquetando '$INITRAMFS_DIR' en '$OUTPUT_IMG'..."

cd "$INITRAMFS_DIR"
find . -print0 | cpio --null -ov --format=newc 2>/dev/null | gzip -9 > "../$OUTPUT_IMG"
cd - > /dev/null

if [ -f "$OUTPUT_IMG" ]; then
    SIZE=$(du -h "$OUTPUT_IMG" | awk '{print $1}')
    echo "[✓] Imagen initrd generada con éxito: $OUTPUT_IMG ($SIZE)"
else
    echo "[!] Error crítico al generar la imagen initrd."
    exit 1
fi
