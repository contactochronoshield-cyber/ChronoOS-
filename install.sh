#!/bin/sh

echo "[*] Instalando ChronoOS CLI..."
chmod +x ~/chrono-os/chrono
cp ~/chrono-os/chrono $PREFIX/bin/chrono

if [ -f "$PREFIX/bin/chrono" ]; then
    echo "[✔] ChronoOS se instaló correctamente."
    echo "Escribe 'chrono' para iniciar."
else
    echo "[!] Error en la instalación."
fi
