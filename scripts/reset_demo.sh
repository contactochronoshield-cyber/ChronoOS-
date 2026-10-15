#!/bin/sh
echo "[*] Reiniciando entorno de demostracion..."
mkdir -p security/vaults_demo security/auth
cp templates/inventario_demo.txt security/vaults_demo/ 2>/dev/null || true
cp templates/registro_fiscal_demo.txt security/vaults_demo/ 2>/dev/null || true
head -c 32 /dev/urandom > security/auth/demo.key
echo "[OK] Demo lista."
