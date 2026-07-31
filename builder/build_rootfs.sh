#!/usr/bin/env bash
# ==============================================================================
# ChronoOS Real Rootfs Builder (Versión Compatible con Termux / Sin Sudo)
# ==============================================================================

set -euo pipefail

BUILD_DIR="$HOME/chrono-os/build_root"
ALPINE_VERSION="3.20.0"
ARCH="aarch64" # Arquitectura típica de móviles Android / Termux
TARBALL="alpine-minirootfs-$ALPINE_VERSION-$ARCH.tar.gz"
DOWNLOAD_URL="https://dl-cdn.alpinelinux.org/alpine/v3.20/releases/$ARCH/$TARBALL"

echo "=== INICIANDO CONSTRUCCIÓN DE ROOTFS SOBERANO (MODO USUARIO) ==="

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

if [ ! -f "$TARBALL" ]; then
    echo "[*] Descargando el sistema base minimalista (Alpine Linux para ARM64)..."
    curl -LO "$DOWNLOAD_URL"
else
    echo "[✔] Imagen base ya descargada localmente."
fi

echo "[*] Extrayendo el sistema de archivos raíz (Rootfs)..."
rm -rf rootfs && mkdir rootfs
tar -xzf "$TARBALL" -C rootfs

echo "[*] Inyectando los binarios y herramientas de ChronoOS..."
mkdir -p rootfs/opt/chrono/core
cp -r "$HOME/chrono-os/core/"* rootfs/opt/chrono/core/ 2>/dev/null || true

echo "nameserver 1.1.1.1" > rootfs/etc/resolv.conf

echo "[✔] Rootfs construido con éxito en: $BUILD_DIR/rootfs"
echo "[*] Entorno listo para ser ejecutado o empaquetado."
