#!/usr/bin/env bash
set -e
echo "Iniciando validación de sintaxis en subsistemas..."
bash -n core/chrono
echo "[✔] Todas las pruebas de sintaxis pasaron correctamente."
