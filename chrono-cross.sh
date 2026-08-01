#!/bin/bash
# ChronoOS Cross-Platform & Environment Compatibility Engine

echo "[*] Detectando arquitectura y entorno de ejecución..."

TARGET_ENV="generic"

if [ -d "/data/data/com.termux" ]; then
    echo "[+] Entorno detectado: Termux (Android / Contenedor móvil)"
    TARGET_ENV="termux"
elif [ -d "/apex" ] || [ -d "/system" ]; then
    echo "[+] Entorno detectado: Sistema Android nativo / AOSP"
    TARGET_ENV="android_native"
else
    echo "[+] Entorno detectado: Linux / Entorno POSIX estándar"
    TARGET_ENV="posix"
fi

echo "[*] Configurando perfiles de compatibilidad para: $TARGET_ENV"

mkdir -p /etc/chrono/repos /var/lib/chrono

if [ "$TARGET_ENV" = "termux" ]; then
    echo "[*] Aplicando parches de rutas para entorno sin root..."
    export CHRONO_ROOT=$HOME/chrono-os
else
    export CHRONO_ROOT="/"
fi

echo "[✓] Capa de compatibilidad multiplataforma cargada correctamente."
