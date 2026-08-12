#!/bin/sh
# ChronoOS - Despliegue de Nucleo 5G Privado
# Instala y configura Open5GS (nucleo REAL, no reinventado) en el Core Box.
# Esto es el trabajo de EMPAQUETADO/INTEGRACION que vende CSN - no la
# reimplementacion del protocolo 5G.
#
# REQUIERE: Ubuntu/Debian en el Core Box real (Beelink), MongoDB, y
# opcionalmente un gNodeB compatible (radio 5G/LTE fisica) para pruebas
# end-to-end. Esto documenta el despliegue de software; la parte de radio
# fisica (antena, licencia de espectro) es un tema aparte de hardware/legal.

echo "=============================================="
echo " ChronoOS - Despliegue Nucleo 5G Privado (Open5GS)"
echo "=============================================="

echo "[*] Este script documenta el despliegue REAL sobre un Core Box con"
echo "    Ubuntu Server (no Termux/Android - Open5GS necesita systemd real)."
echo ""
echo "Pasos (ejecutar en el Core Box, no en el telefono de desarrollo):"
echo ""
echo "  1. sudo apt update && sudo apt install -y mongodb-org"
echo "  2. sudo add-apt-repository ppa:open5gs/latest"
echo "  3. sudo apt install -y open5gs"
echo "  4. Configurar etc/open5gs/amf.yaml con el PLMN del cliente"
echo "     (codigo de red movil privado, se asigna segun regulacion local)"
echo "  5. Copiar bin/chrono-5g-manager al Core Box"
echo "  6. Registrar suscriptores: chrono-5g-manager register <imsi> ..."
echo ""
echo "[i] Este script es una GUIA - el despliegue real se hace en el"
echo "    hardware Beelink cuando este disponible, no en Termux."
