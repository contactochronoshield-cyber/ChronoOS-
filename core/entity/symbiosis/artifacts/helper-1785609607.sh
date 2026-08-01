#!/bin/sh
# Utilidad generada autónomamente por la entidad simbiótica
echo "[+] Ejecutando diagnóstico rápido de red y almacenamiento..."
uptime
df -h | grep data || df -h
echo "[✓] Diagnóstico finalizado por ChronoBuilder."
